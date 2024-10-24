/**
 * @file    wm_uart_task.c
 *
 * @brief   uart task Module
 *
 * @author  dave
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */

#include "wm_uart_task.h"
#include "wm_debug.h"
#include "wm_regs.h"
#include "wm_params.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "wm_fwup.h"
#if (GCC_COMPILE==1)
#include "wm_cmdp_hostif_gcc.h"
#else
#include "wm_cmdp_hostif.h"
#endif
#include "wm_irq.h"
#include "utils.h"
#include "wm_config.h"
#include "wm_socket.h"
#include "wm_mem.h"
#include "wm_wl_task.h"
#include "wm_io.h"

#if (TLS_CONFIG_HOSTIF && TLS_CONFIG_UART)
//#define      UART0_TX_TASK_STK_SIZE          256
//#define      UART0_RX_TASK_STK_SIZE          256
//#define      UART1_TX_TASK_STK_SIZE          256
//#define      UART1_RX_TASK_STK_SIZE          300

static int uart_port_for_at_cmd = TLS_UART_1;
extern struct tls_uart_port uart_port[TLS_UART_MAX];
struct tls_uart uart_st[TLS_UART_MAX];
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
static u32 uart1_delaytime = 0;
//static u8 tcptimedelayflag = 0;
#endif

#define UART_NET_SEND_DATA_SIZE      512
//char uart_net_send_data[UART_NET_SEND_DATA_SIZE];

struct uart_tx_msg
{
    struct tls_uart *uart;
    struct tls_hostif_tx_msg *tx_msg;
};

extern void tls_uart_set_fc_status(int uart_no,
                                   TLS_UART_FLOW_CTRL_MODE_T status);
extern void tls_set_uart_rx_status(int uart_no, int status);
extern int tls_uart_fill_buf(struct tls_uart_port *port, char *buf, u32 count);
extern void tls_uart_tx_chars_start(struct tls_uart_port *port);
extern void tls_uart_free_tx_sent_data(struct tls_uart_port *port);
extern void tls_uart_tx_callback_register(u16 uart_no,
                                          s16(*tx_callback) (struct
                                                             tls_uart_port *
                                                             port));

void uart_rx_timeout_handler(void * arg);
void uart_rx(struct tls_uart *uart);
void uart_tx(struct uart_tx_msg *tx_data);
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
#if TLS_CONFIG_CMD_NET_USE_LIST_FTR	
extern struct tls_uart_net_msg*sockrecvmit[TLS_MAX_NETCONN_NUM];
#else
extern struct tls_uart_circ_buf *sockrecvmit[TLS_MAX_NETCONN_NUM];
#endif
#else
extern struct tls_uart_circ_buf *sockrecvmit[MEMP_NUM_NETCONN];
extern fd_set fdatsockets;
#endif
static void uart_tx_event_finish_callback(void *arg)
{
    if (arg)
        tls_mem_free(arg);
}

#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
static void uart_tx_socket_finish_callback(void *arg)
{
    if (arg)
    {
        struct pbuf *p = (struct pbuf *) arg;
        pbuf_free(p);
    }
}
#endif

extern struct task_parameter wl_task_param_hostif;
static void uart_send_tx_msg(u8 hostif_mode, struct tls_hostif_tx_msg *tx_msg,
                             bool is_event)
{
    struct uart_tx_msg *tx_data = NULL;
    if (tx_msg == NULL)
        return;
//TLS_DBGPRT_INFO("hostif_mode=%d, tx_msg->u.msg_tcp.p=0x%x, uart_st[1].uart_port=0x%x\n", hostif_mode, tx_msg->u.msg_tcp.p, uart_st[1].uart_port);
    switch (hostif_mode)
    {
        case HOSTIF_MODE_UART0:
            if (uart_st[0].uart_port == NULL)
            {
                free_tx_msg_buffer(tx_msg);
                tls_mem_free(tx_msg);
                return;
            }
        // tls_os_mailbox_send(uart_st[0].tx_mailbox, (void
        // *)MBOX_MSG_UART_TX);
            tx_data = tls_mem_alloc(sizeof(struct uart_tx_msg));
            if (tx_data == NULL)
            {
                free_tx_msg_buffer(tx_msg);
                tls_mem_free(tx_msg);
                return;
            }
            tx_data->tx_msg = tx_msg;
            tx_data->uart = &uart_st[0];
            if (tls_wl_task_callback
                (&wl_task_param_hostif, (start_routine) uart_tx, tx_data, 0))
            {
                TLS_DBGPRT_INFO("send tx msg error.\n");
                free_tx_msg_buffer(tx_msg);
                tls_mem_free(tx_msg);
                tls_mem_free(tx_data);
                return;
            }
            break;
        case HOSTIF_MODE_UART1_LS:
        case HOSTIF_MODE_UART1_HS:
            if (uart_st[uart_port_for_at_cmd].uart_port == NULL)
            {
                free_tx_msg_buffer(tx_msg);
                tls_mem_free(tx_msg);
                return;
            }
            if (is_event
                && (hostif_mode != HOSTIF_MODE_UART1_HS
                    || uart_st[uart_port_for_at_cmd].cmd_mode != UART_RICMD_MODE))
            {
                free_tx_msg_buffer(tx_msg);
                tls_mem_free(tx_msg);
                return;
            }
            tx_data = tls_mem_alloc(sizeof(struct uart_tx_msg));
            if (tx_data == NULL)
            {
                free_tx_msg_buffer(tx_msg);
                tls_mem_free(tx_msg);
                return;
            }
            tx_data->tx_msg = tx_msg;
            tx_data->uart = &uart_st[uart_port_for_at_cmd];
            if (tls_wl_task_callback
                (&wl_task_param_hostif, (start_routine) uart_tx, tx_data, 0))
            {
                TLS_DBGPRT_INFO("send tx msg error.\n");
                free_tx_msg_buffer(tx_msg);
                tls_mem_free(tx_msg);
                tls_mem_free(tx_data);
                return;
            }
            break;
        default:
            break;
    }
}

static void uart_get_uart1_port(struct tls_uart_port **uart1_port)
{
	if (uart_port_for_at_cmd <= TLS_UART_5)
	{
    	*uart1_port = uart_st[uart_port_for_at_cmd].uart_port;
	}
	else
	{
		*uart1_port = uart_st[TLS_UART_1].uart_port;
	}
}

static void uart_set_uart1_mode(u32 cmd_mode)
{
    uart_st[uart_port_for_at_cmd].cmd_mode = cmd_mode;
    if (UART_TRANS_MODE == cmd_mode)
    {
        tls_uart_set_fc_status(uart_st[uart_port_for_at_cmd].uart_port->uart_no,
                               TLS_UART_FLOW_CTRL_HARDWARE);
    }
    else
    {
        tls_uart_set_fc_status(uart_st[uart_port_for_at_cmd].uart_port->uart_no,
                               TLS_UART_FLOW_CTRL_NONE);
    }
}

static void uart_set_uart0_mode(u32 cmd_mode)
{
    uart_st[0].cmd_mode = cmd_mode;
    if (UART_TRANS_MODE == cmd_mode)
    {
        tls_uart_set_fc_status(uart_st[0].uart_port->uart_no,
                               TLS_UART_FLOW_CTRL_HARDWARE);
    }
    else
    {
        tls_uart_set_fc_status(uart_st[0].uart_port->uart_no,
                               TLS_UART_FLOW_CTRL_NONE);
    }
}

static void uart_set_uart1_sock_param(u16 sksnd_cnt, bool rx_idle)
{
    uart_st[uart_port_for_at_cmd].sksnd_cnt = sksnd_cnt;
    //uart_st[1].rx_idle = rx_idle;
}

s16 uart_tx_sent_callback(struct tls_uart_port *port)
{
    return tls_wl_task_callback_static(&wl_task_param_hostif,
                                       (start_routine)
                                       tls_uart_free_tx_sent_data, port, 0,
                                       TLS_MSG_ID_UART_SENT_FREE);
}

void tls_uart_init(void)
{
    struct tls_uart_options uart_opts;
    struct tls_uart *uart;
    struct tls_hostif *hif = tls_get_hostif();
    struct tls_param_uart uart_cfg;

    memset(uart_st, 0, TLS_UART_MAX* sizeof(struct tls_uart));
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
    memset(sockrecvmit, 0, TLS_MAX_NETCONN_NUM);
#else
	memset(sockrecvmit, 0, MEMP_NUM_NETCONN);
	FD_ZERO(&fdatsockets);
#endif
// init socket config
    tls_cmd_init_socket_cfg();
    tls_cmd_register_set_uart0_mode(uart_set_uart0_mode);
    
/* setting uart0 */    
    if (WM_SUCCESS != tls_uart_port_init(TLS_UART_0, NULL, 0))
        return;
    tls_uart_tx_callback_register(TLS_UART_0, uart_tx_sent_callback);
    uart = tls_uart_open(TLS_UART_0, TLS_UART_MODE_INT);
    if (NULL == uart)
        return;

    uart->cmd_mode = UART_ATCMD_MODE;
    hif->uart_send_tx_msg_callback = uart_send_tx_msg;
    tls_param_get(TLS_PARAM_ID_UART, (void *) &uart_cfg, 0);
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
    if (uart_cfg.baudrate)
    {
        uart1_delaytime =
            (UART_NET_SEND_DATA_SIZE * (10 * 1000000 / uart_cfg.baudrate) /
             1000 + 5);
    }
    else
    {
        uart1_delaytime = 100;
    }
#endif
    tls_cmd_register_get_uart1_port(uart_get_uart1_port);
    tls_cmd_register_set_uart1_mode(uart_set_uart1_mode);
    tls_cmd_register_set_uart1_sock_param(uart_set_uart1_sock_param);
    if (hif->hostif_mode == HOSTIF_MODE_UART1_HS)
    {
    //根据flash读取的参数配置串口寄存器 
        uart_opts.baudrate = uart_cfg.baudrate;
        uart_opts.charlength = TLS_UART_CHSIZE_8BIT;
        uart_opts.flow_ctrl = (enum TLS_UART_FLOW_CTRL_MODE) uart_cfg.flow;
        uart_opts.paritytype = (enum TLS_UART_PMODE) uart_cfg.parity;
        uart_opts.stopbits = (enum TLS_UART_STOPBITS) uart_cfg.stop_bits;

        if (WM_SUCCESS != tls_uart_port_init(uart_port_for_at_cmd, &uart_opts, 0))
            return;
        tls_uart_tx_callback_register(uart_port_for_at_cmd, uart_tx_sent_callback);
        uart = tls_uart_open(uart_port_for_at_cmd, TLS_UART_MODE_INT);
        if (NULL == uart)
            return;

        uart->cmd_mode = UART_RICMD_MODE;
    }
    else if (hif->hostif_mode == HOSTIF_MODE_UART1_LS)
    {
    //根据flash读取的参数配置串口寄存器 
        uart_opts.baudrate = uart_cfg.baudrate;
        if (uart_cfg.charsize == 0)
        {
            uart_opts.charlength = TLS_UART_CHSIZE_8BIT;
        }
        else
        {
            uart_opts.charlength = (TLS_UART_CHSIZE_T) uart_cfg.charsize;
        }
        uart_opts.flow_ctrl = (enum TLS_UART_FLOW_CTRL_MODE) uart_cfg.flow;
        uart_opts.paritytype = (enum TLS_UART_PMODE) uart_cfg.parity;
        uart_opts.stopbits = (enum TLS_UART_STOPBITS) uart_cfg.stop_bits;
        if (WM_SUCCESS != tls_uart_port_init(uart_port_for_at_cmd, &uart_opts, 0))
            return;
        tls_uart_tx_callback_register(uart_port_for_at_cmd, uart_tx_sent_callback);
        uart = tls_uart_open(uart_port_for_at_cmd, TLS_UART_MODE_INT);
        if (NULL == uart)
            return;
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
        if (tls_cmd_get_auto_mode())
        {
            uart->cmd_mode = UART_TRANS_MODE;
            tls_uart_set_fc_status(uart->uart_port->uart_no,
                                   TLS_UART_FLOW_CTRL_HARDWARE);
        }
        else
#endif // TLS_CONFIG_SOCKET_RAW
        {
            uart->cmd_mode = UART_ATCMD_MODE;   // 指令模式关闭流控
            tls_uart_set_fc_status(uart->uart_port->uart_no,
                                   TLS_UART_FLOW_CTRL_NONE);
        }
    }
    else
    {
        ;
    }
// tls_uart_tx_sent_register(uart_tx_sent_callback);
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
    tls_hostif_set_net_status_callback();
#endif
}

static s16 tls_uart0_task_rx_cb(u16 len, void *p)
{
    struct tls_uart *uart = &uart_st[0];


    if ((UART_TRANS_MODE == uart->cmd_mode)
        && (3 == uart->uart_port->plus_char_cnt))
    {
        uart->cmd_mode = UART_ATCMD_MODE;
        tls_uart_set_fc_status(uart->uart_port->uart_no,
                               TLS_UART_FLOW_CTRL_NONE);
    }
// if(uart->rx_mailbox)
// tls_os_mailbox_send(uart->rx_mailbox, (void *)MBOX_MSG_UART_RX);
// tls_os_queue_send(uart->rx_mailbox, (void *)0, 0);
    if (tls_wl_task_callback_static
        (&wl_task_param_hostif, (start_routine) uart_rx, uart, 0,
         TLS_MSG_ID_UART0_RX))
    {
        return WM_FAILED;
    }
    return WM_SUCCESS;
}

s16 tls_uart1_task_rx_cb(u16 len, void *p)
{
    struct tls_uart *uart = &uart_st[uart_port_for_at_cmd];


    if ((UART_TRANS_MODE == uart->cmd_mode)
        && (3 == uart->uart_port->plus_char_cnt))
    {
        uart->cmd_mode = UART_ATCMD_MODE;
        tls_uart_set_fc_status(uart->uart_port->uart_no,
                               TLS_UART_FLOW_CTRL_NONE);
    }
// if(uart->rx_mailbox)
// tls_os_mailbox_send(uart->rx_mailbox, (void *)MBOX_MSG_UART_RX);
// tls_os_queue_send(uart->rx_mailbox, (void *)0, 0);
    if (tls_wl_task_callback_static
        (&wl_task_param_hostif, (start_routine) uart_rx, uart, 0,
         TLS_MSG_ID_UART1_RX))
    {
        return WM_FAILED;
    }
    return WM_SUCCESS;
}

#if 0
void tls_uart_0_rx_task(void *data)
{
    struct tls_uart *uart = (struct tls_uart *) data;
    int err;
    u32 *msg = NULL;

    for (;;)
    {
    // err = tls_os_mailbox_receive(uart->rx_mailbox, (void **)&msg, 0) ;
        err = tls_os_queue_receive(uart->rx_mailbox, (void **) &msg, 0, 0);
        if (!err)
        {
            uart_rx(uart);
        }
        else
        {
            TLS_DBGPRT_ERR("port->rx_sem err\n");
        }
    }
}

void tls_uart_0_tx_task(void *data)
{
    struct tls_uart *uart = (struct tls_uart *) data;
    u32 *msg = NULL;
    int err;

    for (;;)
    {
        err = tls_os_mailbox_receive(uart->tx_mailbox, (void **) &msg, 0);
        if (err == 0)
        {
            uart_tx(uart);
        }
        else
        {
            TLS_DBGPRT_ERR("port->tx_sem err\n");
        }
    }

}

void tls_uart_1_rx_task(void *data)
{
    struct tls_uart *uart = (struct tls_uart *) data;
    u32 *msg;
    int err;

    for (;;)
    {
    // err = tls_os_mailbox_receive(uart->rx_mailbox, (void **)&msg, 0) ;
        err = tls_os_queue_receive(uart->rx_mailbox, (void **) &msg, 0, 0);
        if (!err)
        {
            uart_rx(uart);
        }
        else
        {
            TLS_DBGPRT_ERR("port->rx_sem err\n");
        }
    }
}

void tls_uart_1_tx_task(void *data)
{
    struct tls_uart *uart = (struct tls_uart *) data;
    u32 *msg;
    int err;

    for (;;)
    {
        err = tls_os_mailbox_receive(uart->tx_mailbox, (void **) &msg, 0);
        if (err == 0)
        {
            uart_tx(uart);
        }
        else
        {
            TLS_DBGPRT_ERR("port->tx_sem err\n");
        }
    }
}
#endif

void tls_uart_set_at_cmd_port(int at_cmd_port)
{
	if (at_cmd_port && (at_cmd_port < TLS_UART_MAX))
	{
		uart_port_for_at_cmd = at_cmd_port;
	}
	else
	{
		uart_port_for_at_cmd = TLS_UART_1;
	}
}


int tls_uart_get_at_cmd_port(void)
{
	return uart_port_for_at_cmd;
}

struct tls_uart *tls_uart_open(u32 uart_no, TLS_UART_MODE_T uart_mode)
{
    struct tls_uart *uart;
//    char *stk;
//  void * rx_msg  = NULL;
    if (uart_no == TLS_UART_0)
    {
        uart = &uart_st[uart_no];
        memset(uart, 0, sizeof(struct tls_uart));
        uart->uart_port = &uart_port[uart_no];
        tls_uart_rx_callback_register(uart_no, tls_uart0_task_rx_cb, NULL);
    }
    else if (uart_no <= TLS_UART_5)
    {
        uart = &uart_st[uart_no];
        memset(uart, 0, sizeof(struct tls_uart));
        uart->uart_port = &uart_port[uart_no];
        tls_uart_rx_callback_register(uart_no, tls_uart1_task_rx_cb, NULL);
    }
    else
        return NULL;

    uart->uart_port->uart_mode = uart_mode;

#if 0
    if (uart_mode == TLS_UART_MODE_POLL)
    {
    /* uart poll mode */
        uart->tx_cb = NULL;
    }
    else
#endif
    {
#if 0
    /* 创建uart 发送任务和信号量 */
        if (uart_no == TLS_UART_0)
        {
            stk = tls_mem_alloc(UART0_TX_TASK_STK_SIZE * sizeof(u32));
            if (!stk)
                return NULL;
            memset(stk, 0, UART0_TX_TASK_STK_SIZE * sizeof(u32));
            tls_os_task_create(NULL, "uart_0_tx", tls_uart_0_tx_task, (void *) uart, (void *) stk,  /* 任务栈的起始地址
                                                                                                     */
                               UART0_TX_TASK_STK_SIZE * sizeof(u32),    /* 任务栈的大小
                                                                         */
                               TLS_UART0_TX_TASK_PRIO, 0);
            stk = tls_mem_alloc(UART0_RX_TASK_STK_SIZE * sizeof(u32));
            if (!stk)
                return NULL;
            memset(stk, 0, UART0_RX_TASK_STK_SIZE * sizeof(u32));
            tls_os_task_create(NULL, "uart_0_rx", tls_uart_0_rx_task, (void *) uart, (void *) stk,  /* 任务栈的起始地址
                                                                                                     */
                               UART0_RX_TASK_STK_SIZE * sizeof(u32),    /* 任务栈的大小
                                                                         */
                               TLS_UART0_RX_TASK_PRIO, 0);

        }
        else
        {
            stk = tls_mem_alloc(UART1_TX_TASK_STK_SIZE * sizeof(u32));
            if (!stk)
                return NULL;
            memset(stk, 0, UART1_TX_TASK_STK_SIZE * sizeof(u32));
            tls_os_task_create(NULL, "uart_1_tx", tls_uart_1_tx_task, (void *) uart, (void *) stk,  /* 任务栈的起始地址
                                                                                                     */
                               UART1_TX_TASK_STK_SIZE * sizeof(u32),    /* 任务栈的大小
                                                                         */
                               TLS_UART1_TX_TASK_PRIO, 0);
            stk = tls_mem_alloc(UART1_RX_TASK_STK_SIZE * sizeof(u32));
            if (!stk)
                return NULL;
            memset(stk, 0, UART1_RX_TASK_STK_SIZE * sizeof(u32));
            tls_os_task_create(NULL, "uart_1_rx", tls_uart_1_rx_task, (void *) uart, (void *) stk,  /* 任务栈的起始地址
                                                                                                     */
                               UART1_RX_TASK_STK_SIZE * sizeof(u32),    /* 任务栈的大小
                                                                         */
                               TLS_UART1_RX_TASK_PRIO, 0);
        }
#endif


    // tls_os_sem_create(&port->rx_sem, 0);
    // tls_os_mailbox_create(&uart->rx_mailbox, (void *)0, 0, 0);

    // tls_os_queue_create(&uart->rx_mailbox, 64);

    // tls_os_mailbox_create(&uart->tx_mailbox, (void *)0, 0, 0);

    }
    return uart;
}

int tls_uart_close(struct tls_uart *uart)
{
    return WM_FAILED;
}


static u8 *find_atcmd_eol(u8 * src, u32 len)
{
    u8 *p = NULL;
    u8 *q = NULL;

    p = memchr(src, '\r', len);
    q = memchr(src, '\n', len);

    if (p && q)
    {
        if ((p - q) > 1)
        {
            return q;
        }
        if ((q - p) > 1)
        {
            return p;
        }
        if ((p - q) == 1)
        {
            return p;
        }
        if ((q - p) == 1)
        {
            return q;
        }
        return NULL;
    }

    if (p)
    {
        return p;
    }

    if (q)
    {
        return q;
    }
    return NULL;
}

static void modify_atcmd_tail(struct tls_uart_circ_buf *recv, u8 ** p)
{
    u32 cmd_len;

	if (*p >= &recv->buf[recv->tail])
	{
    	cmd_len = *p - &recv->buf[recv->tail];
	}else{
		cmd_len = *p + TLS_UART_RX_BUF_SIZE  - &recv->buf[recv->tail];
	}
    if (cmd_len > 512)
    {
        recv->tail = recv->head;
        *p = NULL;
        TLS_DBGPRT_INFO("EOF char find > 512 \r\n");
    }
    else
    {
        recv->tail = (recv->tail + cmd_len) & (TLS_UART_RX_BUF_SIZE - 1);
    }
}

static u8 *parse_atcmd_eol(struct tls_uart *uart)
{
    struct tls_uart_circ_buf *recv = &uart->uart_port->recv;
    u8 *p = NULL;


/* jump to end of line */
    if (recv->head > recv->tail)
    {
//      TLS_DBGPRT_INFO("1 recv->tail = %d, recv->head = %d \r\n", recv->tail, recv->head);
        p = find_atcmd_eol((u8 *)&recv->buf[recv->tail], (u32)(recv->head - recv->tail));
        if (p)
        {
            modify_atcmd_tail(recv, &p);
        }
    }
    else
    {
//      TLS_DBGPRT_INFO("2 recv->tail = %d, recv->head = %d \r\n", recv->tail, recv->head);
    /* check buf[tail - END] */
        p = find_atcmd_eol((u8 *)&recv->buf[recv->tail],
                           TLS_UART_RX_BUF_SIZE - recv->tail);

        if (!p)
        {
        /* check buf[0 - HEAD] */
            p = find_atcmd_eol((u8 *)&recv->buf[0], recv->head);
            if (p)
            {
                modify_atcmd_tail(recv, &p);
//              TLS_DBGPRT_INFO("3 find recv->tail = %d \r\n", recv->tail);
            }
        }
        else
        {
            modify_atcmd_tail(recv, &p);
        }
    }

/* jump over EOF char */
    if ((recv->buf[recv->tail] == '\r') || (recv->buf[recv->tail] == '\n'))
    {
        recv->tail = (recv->tail + 1) & (TLS_UART_RX_BUF_SIZE - 1);
    }
    return p;
}

#if 0
static void uart_wait_tx_finished(struct tls_uart *uart)
{
    struct tls_uart_circ_buf *xmit = &uart->uart_port->xmit;

    while (!uart_circ_empty(xmit))
    {
        tls_os_time_delay(2);
    }
}
#endif

static void parse_atcmd_line(struct tls_uart *uart)
{
    struct tls_uart_circ_buf *recv = &uart->uart_port->recv;
    u8 *ptr_eol;
    u32 cmd_len, tail_len = 0;
    u8 *atcmd_start = NULL;
    char *buf;
    u8 hostif_uart_type;

//  TLS_DBGPRT_INFO("A1 %d, %d\r\n", recv->tail, recv->head);
    while ((CIRC_CNT(recv->head, recv->tail, TLS_UART_RX_BUF_SIZE) >= 4)
           && (atcmd_start == NULL))
    {                           // check "at+" char
        if (((recv->buf[recv->tail] == 'A') || (recv->buf[recv->tail] == 'a'))
            &&
            ((recv->buf[(recv->tail + 1) & (TLS_UART_RX_BUF_SIZE - 1)] == 'T')
             || (recv->buf[(recv->tail + 1) & (TLS_UART_RX_BUF_SIZE - 1)] ==
                 't'))
            && (recv->buf[(recv->tail + 2) & (TLS_UART_RX_BUF_SIZE - 1)] ==
                '+'))
        {
            atcmd_start = (u8 *)&recv->buf[recv->tail];
            recv->tail = (recv->tail + 3) & (TLS_UART_RX_BUF_SIZE - 1);
            ptr_eol = parse_atcmd_eol(uart);
        // TLS_DBGPRT_INFO("ptr_eol = 0x%x\n", ptr_eol);
            if (!ptr_eol)
            {                   // 没有结束符，可能只收到半个命令
                if (CIRC_CNT(recv->head, recv->tail, TLS_UART_RX_BUF_SIZE) >
                    512)
                {
                    recv->tail = recv->head;
                }
                else
                {
                    recv->tail = (recv->tail - 3) & (TLS_UART_RX_BUF_SIZE - 1);
                }
                break;
            }
        // 获取命令长度
            if (ptr_eol >= atcmd_start)
            {
                cmd_len = ptr_eol - atcmd_start;
            }
            else
            {
                tail_len =
                    (u32) (&recv->buf[TLS_UART_RX_BUF_SIZE - 1] - atcmd_start +
                           1);
                cmd_len = tail_len + (ptr_eol - &recv->buf[0]);
            }
            buf = tls_mem_alloc(cmd_len + 2);
            if (!buf)
            {
                return;
            }
            if (ptr_eol >= atcmd_start)
            {
                MEMCPY(buf, atcmd_start, cmd_len);
            }
            else
            {
                MEMCPY(buf, atcmd_start, tail_len);
                MEMCPY(buf + tail_len, (void *)&recv->buf[0], ptr_eol - (u8 *)&recv->buf[0]);
            }

            if (buf[cmd_len - 2] == '\r' || buf[cmd_len - 2] == '\n')
            {
                buf[cmd_len - 2] = '\n';
                buf[cmd_len - 1] = '\0';
                cmd_len = cmd_len - 1;
            }
            else if (buf[cmd_len - 1] == '\r' || buf[cmd_len - 1] == '\n')
            {
                buf[cmd_len - 1] = '\n';
                buf[cmd_len] = '\0';
                cmd_len = cmd_len;
            }
            else
            {
                buf[cmd_len] = '\n';
                buf[cmd_len + 1] = '\0';
                cmd_len = cmd_len + 1;
            }

            if (uart->uart_port->uart_no == TLS_UART_0)
            {
                hostif_uart_type = HOSTIF_UART0_AT_CMD;
            }
            else
            {
                hostif_uart_type = HOSTIF_UART1_AT_CMD;
            }
            tls_hostif_cmd_handler(hostif_uart_type, buf, cmd_len);
            tls_mem_free(buf);
            atcmd_start = NULL;
            if (CIRC_CNT(recv->head, recv->tail, TLS_UART_RX_BUF_SIZE) > 0)
            {
                if (uart->uart_port->uart_no == TLS_UART_0)
                {
                    tls_uart0_task_rx_cb(CIRC_CNT
                                         (recv->head, recv->tail,
                                          TLS_UART_RX_BUF_SIZE), NULL);
                }
                else
                {
                    tls_uart1_task_rx_cb(CIRC_CNT
                                         (recv->head, recv->tail,
                                          TLS_UART_RX_BUF_SIZE), NULL);
                }
                break;
            }
        }
        else
        {                       // start of string is not "at+", and string not
                                // include '\r' and '\n' eat the string
//            ptr_eol = parse_atcmd_eol(uart);
//           if (!ptr_eol)
            {
                recv->tail = (recv->tail + 1)%TLS_UART_RX_BUF_SIZE;
            }
        }
    }
//  TLS_DBGPRT_INFO("A2 %d, %d\r\n", recv->tail, recv->head);
}
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
//char uart_net_send_data[UART_NET_SEND_DATA_SIZE];
char *uart_net_send_data = NULL;


void uart_net_send(struct tls_uart *uart, u32 head, u32 tail, int count)
{
    struct tls_uart_circ_buf *recv = &uart->uart_port->recv;
    int buflen;
    int bufcopylen = 0;
    struct tls_hostif_socket_info skt_info;
    u8 def_socket;
    int err = 0;
    int remaincount = count;

    static u16 printfFreq = 0;

	if (uart_net_send_data == NULL)
	{
		uart_net_send_data = tls_mem_alloc(UART_NET_SEND_DATA_SIZE * sizeof(char));
		if (uart_net_send_data == NULL)
		{
			return;
		}
		memset(uart_net_send_data, 0, (UART_NET_SEND_DATA_SIZE * sizeof(char)));
	}

    //printf("uart_net_send count %d\n", count);
RESENDBUF:
    if (remaincount >= UART_NET_SEND_DATA_SIZE)
    {
        buflen = UART_NET_SEND_DATA_SIZE;
        remaincount = remaincount - UART_NET_SEND_DATA_SIZE;
    }
    else
    {
        buflen = remaincount;
        remaincount = 0;
    }
    if ((tail + buflen) > TLS_UART_RX_BUF_SIZE)
    {
        bufcopylen = (TLS_UART_RX_BUF_SIZE - tail);
        MEMCPY(uart_net_send_data, (u8 *)recv->buf + tail, bufcopylen);
        MEMCPY(uart_net_send_data + bufcopylen, (u8 *)recv->buf, buflen - bufcopylen);
    }
    else
    {
        MEMCPY(uart_net_send_data, (u8 *)recv->buf + tail, buflen);
    }
    def_socket = tls_cmd_get_default_socket();
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
    if (def_socket)
#endif
    {
        skt_info.socket = def_socket;

        do
        {
            err = tls_hostif_send_data(&skt_info, uart_net_send_data, buflen);
            if (ERR_VAL == err)
            {
                printf("\nsocket err val\n");
                tls_set_uart_rx_status(uart->uart_port->uart_no, TLS_UART_RX_DISABLE);
                tls_wl_task_untimeout(&wl_task_param_hostif, uart_rx_timeout_handler, uart);
                tls_wl_task_add_timeout(&wl_task_param_hostif, uart1_delaytime, uart_rx_timeout_handler, uart);
                return;
            }
            if (err == ERR_MEM)
            {
                if (TLS_UART_FLOW_CTRL_HARDWARE == uart->uart_port->fcStatus && 
                    TLS_UART_FLOW_CTRL_HARDWARE == uart->uart_port->opts.flow_ctrl)
                {
                    tls_os_time_delay(10);
                    continue;
                }
                else
                {
                    printfFreq ++;
                    if(printfFreq%50 == 0)
                    {
                        printf("ERR_MEM\r\n");
                    }
                    break;
                }
            }

        }
        while (err == ERR_MEM);
    }
    recv->tail = (recv->tail + buflen) & (TLS_UART_RX_BUF_SIZE - 1);
    if (uart->cmd_mode == UART_ATSND_MODE)
    {
        if (remaincount > 0)
        {
            tail = recv->tail;
            goto RESENDBUF;
        }
    }
    else
    {
        if (remaincount >= UART_NET_SEND_DATA_SIZE)
        {
            tail = recv->tail;
            goto RESENDBUF;
        }
    }

    buflen = remaincount;
    if (buflen)
    {
        tls_wl_task_untimeout(&wl_task_param_hostif, uart_rx_timeout_handler, uart);
		tls_wl_task_add_timeout(&wl_task_param_hostif, uart1_delaytime, uart_rx_timeout_handler, uart);
    }

    if (TLS_UART_FLOW_CTRL_HARDWARE == uart->uart_port->fcStatus && 
        TLS_UART_FLOW_CTRL_HARDWARE == uart->uart_port->opts.flow_ctrl)
    {
        buflen = CIRC_SPACE(recv->head, recv->tail, TLS_UART_RX_BUF_SIZE);
        //printf("\nbuffer len==%d\n",buflen);
        if (buflen > TLS_UART_RX_BUF_SIZE / 2)
        {
            tls_set_uart_rx_status(uart->uart_port->uart_no, TLS_UART_RX_ENABLE);
        }
        else
        {
            tls_wl_task_untimeout(&wl_task_param_hostif, uart_rx_timeout_handler, uart);
			tls_wl_task_add_timeout(&wl_task_param_hostif, uart1_delaytime, uart_rx_timeout_handler, uart);
        }
    }
}
#endif
#if !TLS_CONFIG_CMD_NET_USE_LIST_FTR	
static int cache_tcp_recv(struct tls_hostif_tx_msg *tx_msg)
{
    struct tls_uart_circ_buf *precvmit =
        tls_hostif_get_recvmit(tx_msg->u.msg_tcp.sock);
//    struct tls_hostif *hif = tls_get_hostif();
    struct pbuf *p;
    u16 buflen;
    u16 copylen;
    u32 tail = 0;
    bool overflow = 0;

    p = (struct pbuf *) tx_msg->u.msg_tcp.p;
    if (p->tot_len >= TLS_SOCKET_RECV_BUF_SIZE)
    {
        tx_msg->offset = p->tot_len - TLS_SOCKET_RECV_BUF_SIZE + 1;
    }
    TLS_DBGPRT_INFO("p->tot_len=%d\n", p->tot_len);
    TLS_DBGPRT_INFO("precvmit->head=%d, precvmit->tail=%d\n", precvmit->head,
                    precvmit->tail);
    buflen = p->tot_len - tx_msg->offset;
    tail = precvmit->tail;
    while (1)
    {
        copylen = CIRC_SPACE_TO_END_FULL(precvmit->head,
                                         tail, TLS_SOCKET_RECV_BUF_SIZE);
        if (copylen == 0)
        {
            tail = 0;
            overflow = 1;
            continue;
        }
        if (buflen < copylen)
            copylen = buflen;
        pbuf_copy_partial(p,
                          (u8 *)precvmit->buf + precvmit->head,
                          copylen, tx_msg->offset);
        precvmit->head =
            (precvmit->head + copylen) & (TLS_SOCKET_RECV_BUF_SIZE - 1);
        TLS_DBGPRT_INFO("precvmit->head=%d, precvmit->tail=%d\n",
                        precvmit->head, precvmit->tail);
        tx_msg->offset += copylen;
        buflen -= copylen;
        if (tx_msg->offset >= p->tot_len)
            break;
    };
    if (overflow)
        precvmit->tail = precvmit->head + 1;

/* 检查pbuf的数据是否已经都拷贝到uart缓存了 */
    if (tx_msg->offset >= p->tot_len)
    {
        pbuf_free(p);
    }

    return copylen;
}
#endif
#if 0
static int uart_tcp_recv(struct tls_uart_port *port,
                         struct tls_hostif_tx_msg *tx_msg)
{
    struct tls_hostif *hif = tls_get_hostif();
    struct pbuf *p;
    u16 buflen;
    u16 copylen;
    u32 cpu_sr;

    p = (struct pbuf *) tx_msg->u.msg_tcp.p;
    buflen = p->tot_len - tx_msg->offset;

    while (1)
    {
    /* copy the contents of the received buffer into uart tx buffer */
        copylen = CIRC_SPACE_TO_END(port->xmit.head,
                                    port->xmit.tail, TLS_UART_TX_BUF_SIZE);
        if (buflen < copylen)
            copylen = buflen;
        if (copylen <= 0)
        {
            tls_uart_tx_chars_start(port);
            tls_os_time_delay(2);
            continue;
        }
        copylen = pbuf_copy_partial(p,
                                    port->xmit.buf + port->xmit.head,
                                    copylen, tx_msg->offset);
        port->xmit.head =
            (port->xmit.head + copylen) & (TLS_UART_TX_BUF_SIZE - 1);
        tx_msg->offset += copylen;
        if (tx_msg->offset >= p->tot_len)
            break;
    };

/* 检查pbuf的数据是否已经都拷贝到uart缓存了 */
    if (tx_msg->offset >= p->tot_len)
    {
        pbuf_free(p);
        cpu_sr = tls_os_set_critical();
        dl_list_del(&tx_msg->list);
        dl_list_add_tail(&hif->tx_msg_list, &tx_msg->list);
        tls_os_release_critical(cpu_sr);
    }

    return copylen;
}
#endif

#if 0
static void uart_tx_timeout_check(struct tls_uart *uart)
{
    struct tls_uart_circ_buf *xmit = &uart->uart_port->xmit;
    struct tls_hostif *hif = tls_get_hostif();
    struct tls_hostif_tx_msg *tx_msg;
    u32 cpu_sr;

/* check if host receive is stop, if stop, discard msg */
    if (CIRC_CNT(xmit->head, xmit->tail, TLS_UART_TX_BUF_SIZE) ==
        (TLS_UART_TX_BUF_SIZE - 1))
    {
        while (1)
        {
            tx_msg = dl_list_first(&uart->tx_msg_pending_list,
                                   struct tls_hostif_tx_msg, list);
            if (!tx_msg)
                break;
            if (time_after(tls_os_get_time(), tx_msg->time + 120 * HZ))
            {
                switch (tx_msg->type)
                {
                    case HOSTIF_TX_MSG_TYPE_EVENT:
                        tls_mem_free(tx_msg->u.msg_cmdrsp.buf);
                        cpu_sr = tls_os_set_critical();
                        dl_list_del(&tx_msg->list);
                        dl_list_add_tail(&hif->tx_event_msg_list,
                                         &tx_msg->list);
                        tls_os_release_critical(cpu_sr);
                        break;

                    case HOSTIF_TX_MSG_TYPE_UDP:
                        pbuf_free(tx_msg->u.msg_udp.p);
                        cpu_sr = tls_os_set_critical();
                        dl_list_del(&tx_msg->list);
                        dl_list_add_tail(&hif->tx_msg_list, &tx_msg->list);
                        tls_os_release_critical(cpu_sr);
                        break;

                    case HOSTIF_TX_MSG_TYPE_TCP:
                        pbuf_free(tx_msg->u.msg_tcp.p);
                        cpu_sr = tls_os_set_critical();
                        dl_list_del(&tx_msg->list);
                        dl_list_add_tail(&hif->tx_msg_list, &tx_msg->list);
                        tls_os_release_critical(cpu_sr);
                        break;

                    default:
                        break;
                }
            }
            else
            {
                break;
            }
        }
    }
}
#endif
/*　
 * 处理流程说明：
 * 首先判断上次的同步帧是否已经处理完成，如果已经处理结束，
 *          则检查缓存head指向的字节，判断是否是0xAA(SYN_FLAG)，
 *          如果是，则检查缓存的长度是否大于等于8，如果不是
 *          则返回，如果是，则提取字节标记和长度信息，校验信息，
 *          计算校验和，检查校验值是否匹配，
 */
static int ricmd_handle_sync(struct tls_uart *uart,
                             struct tls_uart_circ_buf *recv)
{
#if 0
    int numbytes;
    u8 type;
    u8 sn, flag, dest;
    u8 orgi_crc8, new_crc8;
    u32 frm_len;
    u32 count;
    u32 skip_count = 0;
    u32 head = recv->head;
    int i;

    if (!(port->inputstate & INS_SYNC_CHAR))
    {
    /* 对上次接收的数据，没有SYN标记需要等待处理 */
        if (recv->buf[recv->head] == RICMD_SYNC_FLAG)
        {
            numbytes = CIRC_CNT(recv->head, recv->tail, TLS_UART_RX_BUF_SIZE);
            if (numbytes < 8)
            {
            /* 先不处理 */
                skip_count = 0;
                return skip_count;
            }
            else
            {
                if ((TLS_UART_RX_BUF_SIZE - recv->head) >= 8)
                    MEMCPY(&port->sync_header[0], &recv->buf[recv->head], 8);
                else
                {
                    count = TLS_UART_RX_BUF_SIZE - recv->head;
                    MEMCPY(port->sync_header, &recv->buf[recv->head], count);
                    MEMCPY(recv->sync_header + count, recv->buf, 8 - count);
                }
            /* skip sync flag */
                recv->head = (recv->head + 1) & (TLS_UART_RX_BUF_SIZE - 1);
            /* ricmd type */
                type = recv->buf[recv->head];
                recv->head = (recv->head + 1) & (TLS_UART_RX_BUF_SIZE - 1);

            /* data length */
                if (recv->head == (TLS_UART_RX_BUF_SIZE - 1))
                {
                    frm_len = recv->buf[recv->head] << 8 | recv->buf[0];
                }
                else
                {
                    frm_len = recv->buf[recv->head] << 8 |
                        recv->buf[recv->head + 1];
                }
                recv->head = (recv->head + 2) & (TLS_UART_RX_BUF_SIZE - 1);

            /* ricmd sn */
                sn = recv->buf[recv->head];
                recv->head = (recv->head + 1) & (TLS_UART_RX_BUF_SIZE - 1);
            /* ricmd flag */
                flag = recv->buf[recv->head];
                recv->head = (recv->head + 1) & (TLS_UART_RX_BUF_SIZE - 1);
            /* ricmd dest */
                dest = recv->buf[recv->head];
                port->ricmd_info.dest = dest;
                recv->head = (recv->head + 1) & (TLS_UART_RX_BUF_SIZE - 1);
            /* ricmd crc */
                orig_crc8 = recv->buf[recv->head];
                recv->head = (recv->head + 1) & (TLS_UART_RX_BUF_SIZE - 1);

#if 0
            /* check for crc */
                new_crc8 = get_crc8(&inbuf->sync_header[1], 3);
                skip_count++;

                TLS_DBGPRT_INFO
                    ("recevied crc value = 0x%x, check crc value = %x\n",
                     orgi_crc8, new_crc8);
#endif

                new_crc8 = orig_crc8;
                if (orgi_crc8 == new_crc8)
                {
                    port->inputstate |= INS_SYNC_CHAR;
                    switch (type)
                    {
                        case 0x0:
                            port->inputstate |= INS_CMD;
                            break;
                        case 0x1:
                            port->inputstate |= INS_DATA;
                            break;
                        default:
                            data->inputstate |= INS_RAW;
                            break;
                    }
                    port->ricmd_info.length = frm_len;
                    TLS_DBGPRT_INFO("data length = %d\n", frm_len);

                }
                else
                {
                    TLS_DBGPRT_INFO("crc check failed, drop the char\n");
                /* crc 错误，忽略收到的字符，继续处理后面的字符 */
                }
            }
        }
        else
        {
        /* regular data byte */
            skip_count = 1;
            return skip_count;
        }
    }

    return skip_count;
#endif
    return 0;
}

static int data_loop(struct tls_uart *uart,
                     int numbytes, struct tls_uart_circ_buf *recv)
{

    return 0;
}

#define MAX_RICMD_LENGTH      200
//u8 ricmd_buffer[MAX_RICMD_LENGTH + 8];
u8 *ricmd_buffer = NULL;

static int cmd_loop(struct tls_uart *uart,
                    int numbytes, struct tls_uart_circ_buf *recv)
{
    unsigned cbytes = uart->ricmd_info.cbytes;
    unsigned procbytes = 0;
    unsigned char c;

	if (ricmd_buffer == NULL)
	{
		ricmd_buffer = tls_mem_alloc(MAX_RICMD_LENGTH + 8);
		if (ricmd_buffer == NULL)
		{
			return -1;
		}
		memset(ricmd_buffer, 0, (MAX_RICMD_LENGTH + 8));
	}

    while (procbytes < numbytes)
    {
        c = recv->head;
        procbytes++;

    /* append to line buffer if possible */
        if (cbytes < MAX_RICMD_LENGTH)
            ricmd_buffer[8 + cbytes] = c;
        uart->ricmd_info.cbytes++;

        if (uart->ricmd_info.cbytes == uart->ricmd_info.length)
        {
            tls_hostif_cmd_handler(HOSTIF_UART1_RI_CMD,
                                   (char *) ricmd_buffer,
                                   uart->ricmd_info.length + 8);
            uart->ricmd_info.cbytes = 0;
            uart->inputstate = 0;
            break;
        }
    }
    return procbytes;
}

void parse_ricmd_line(struct tls_uart *uart)
{
    struct tls_uart_circ_buf *recv = &uart->uart_port->recv;
    int skip_count;
    int numbytes;
    int procbytes;

    while (!uart_circ_empty(recv))
    {
    /* check for frame header */
        skip_count = ricmd_handle_sync(uart, recv);
        if ((skip_count == 0) && !(uart->inputstate & INS_SYNC_CHAR))
            break;

        if (uart->inputstate & INS_SYNC_CHAR)
        {
        /* process a contiguous block of bytes */
            numbytes = CIRC_CNT(recv->head, recv->tail, TLS_UART_RX_BUF_SIZE);

            if (uart->inputstate & INS_RICMD)
                procbytes = cmd_loop(uart, numbytes, recv);
            else if (uart->inputstate & INS_DATA)
                procbytes = data_loop(uart, numbytes, recv);
            else
                procbytes = numbytes;
        }
        else
        {
        /* 没有需要处理的数据(第一个字符不是SYNC_FLAG)，而且以前的包已经处理完成
         */
            procbytes = skip_count;
        }
        recv->head = (recv->head + procbytes) & (TLS_UART_RX_BUF_SIZE - 1);
    }

    return;
}

#define UART_UPFW_DATA_SIZE sizeof(struct tls_fwup_block)

static int uart_fwup_rsp(u8 portno, int status)
{
    char *cmd_rsp = NULL;
    u32 len;
    u8 hostif_type;

    cmd_rsp = tls_mem_alloc(16);
    if (NULL == cmd_rsp)
    {
        return -1;
    }
    if (status)
    {
        len = sprintf(cmd_rsp, "+OK=%d\r\n\r\n", status);
    }
    else
    {
        len = sprintf(cmd_rsp, "+ERR=%d\r\n\r\n", status);
    }

    if (TLS_UART_0 == portno)
    {
        hostif_type = HOSTIF_MODE_UART0;
    }
    else
    {
        hostif_type = HOSTIF_MODE_UART1_LS;
    }

    if (tls_hostif_process_cmdrsp(hostif_type, cmd_rsp, len))
    {
        tls_mem_free(cmd_rsp);
    }
    return 0;
}

void uart_fwup_send(struct tls_uart *uart)
{
    struct tls_uart_circ_buf *recv = &uart->uart_port->recv;
    u32 data_cnt = CIRC_CNT(recv->head, recv->tail, TLS_UART_RX_BUF_SIZE);
    struct tls_fwup_block *pfwup = NULL;
    u8 *p;
    u32 i, session_id, status;

    if (data_cnt >= UART_UPFW_DATA_SIZE)
    {
        uart->cmd_mode = UART_ATCMD_MODE;
    // TLS_DBGPRT_INFO("D1 %d, %d\r\n", recv->tail, recv->head);
        pfwup = (struct tls_fwup_block *) tls_mem_alloc(UART_UPFW_DATA_SIZE);
        if (!pfwup)
        {
            recv->tail =
                (recv->tail + UART_UPFW_DATA_SIZE) & (TLS_UART_RX_BUF_SIZE - 1);
            return;
        }
        p = (u8 *) pfwup;
        for (i = 0; i < UART_UPFW_DATA_SIZE; i++)
        {
            *p++ = recv->buf[recv->tail++];
            recv->tail &= TLS_UART_RX_BUF_SIZE - 1;
        }
        session_id = tls_fwup_get_current_session_id();
//      TLS_DBGPRT_INFO("%d, %d\r\n", pfwup->number, pfwup->sum);
        if (session_id)
        {
            if (get_crc32((u8 *) pfwup, UART_UPFW_DATA_SIZE - 12) ==
                pfwup->crc32)
            {
                if (TLS_FWUP_STATUS_OK == tls_fwup_set_update_numer(pfwup->number))
                {
                    struct tls_fwup_block *blk;
                    u8 *buffer;
						
					blk = (struct tls_fwup_block *)pfwup;
					buffer = blk->data;
                    status = 1;
                    uart_fwup_rsp(uart->uart_port->uart_no, status);
                    tls_fwup_request_sync(session_id, buffer, TLS_FWUP_BLK_SIZE);
                }
                else
                {
                    TLS_DBGPRT_INFO("tls_fwup_set_update_numer err!!!\r\n");
                    status = 0;
                    uart_fwup_rsp(uart->uart_port->uart_no, status);
                }
            }
            else
            {
            // tls_fwup_set_crc_error(session_id);
                TLS_DBGPRT_INFO("err crc32 !!!\r\n");
                status = 0;
                uart_fwup_rsp(uart->uart_port->uart_no, status);
            }
        }
        tls_mem_free(pfwup);
    // TLS_DBGPRT_INFO("D2 %d, %d\r\n", recv->tail, recv->head);
    }
}
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
void uart_rx_timeout_handler(void * arg)
{
	int data_cnt;
	struct tls_uart *uart = (struct tls_uart *)arg;
	struct tls_uart_circ_buf *recv = &uart->uart_port->recv;
	
	if (uart->cmd_mode == UART_TRANS_MODE)
    {
        data_cnt = CIRC_CNT(recv->head, recv->tail, TLS_UART_RX_BUF_SIZE);

        if (data_cnt)
        {
            uart_net_send(uart, recv->head, recv->tail, data_cnt);
        }
    }
}
#endif
void uart_rx(struct tls_uart *uart)
{
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
    struct tls_uart_circ_buf *recv = &uart->uart_port->recv;
    int data_cnt;
    u8 send_data = 0;
#endif
    int err = 0;
    u8 len = 0;
    char *cmd_rsp = NULL;
//TLS_DBGPRT_INFO("port->cmd_mode=%d\r\n", port->cmd_mode);
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
    if (uart->cmd_mode == UART_TRANS_MODE)
    {
        data_cnt = CIRC_CNT(recv->head, recv->tail, TLS_UART_RX_BUF_SIZE);
        if (data_cnt >= UART_NET_SEND_DATA_SIZE)
        {
            send_data = 1;
            tls_wl_task_untimeout(&wl_task_param_hostif, uart_rx_timeout_handler, uart);
        }
        else
        {
            tls_wl_task_untimeout(&wl_task_param_hostif, uart_rx_timeout_handler, uart);
			tls_wl_task_add_timeout(&wl_task_param_hostif, uart1_delaytime, uart_rx_timeout_handler, uart);
        }

        if (send_data)
        {
            uart_net_send(uart, recv->head, recv->tail, data_cnt);
        }
    }
    else
#endif // TLS_CONFIG_SOCKET_RAW
    if (uart->cmd_mode == UART_ATCMD_MODE)
    {
        if (uart->uart_port->plus_char_cnt == 3)
        {
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
            tls_wl_task_untimeout(&wl_task_param_hostif, uart_rx_timeout_handler, uart);
#endif
            cmd_rsp = tls_mem_alloc(strlen("+OK\r\n\r\n") + 1);
            if (!cmd_rsp)
                return;
            len = sprintf(cmd_rsp, "+OK\r\n\r\n");
            uart->uart_port->plus_char_cnt = 0;
            err = tls_hostif_process_cmdrsp(HOSTIF_MODE_UART1_LS, cmd_rsp, len);
            if (err)
            {
                tls_mem_free(cmd_rsp);
            }
        }
        parse_atcmd_line(uart);
    }
    else if (uart->cmd_mode == UART_ATDATA_MODE)
    {
        uart_fwup_send(uart);
    }
    else if (uart->cmd_mode == UART_RICMD_MODE)
    {
        parse_ricmd_line(uart);
    }
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
    else if (UART_ATSND_MODE == uart->cmd_mode)
    {
        data_cnt = CIRC_CNT(recv->head, recv->tail, TLS_UART_RX_BUF_SIZE);
        if (data_cnt >= uart->sksnd_cnt)
        {
            send_data = 1;
        }
        else
        {
        /* do nothing */
            return;
        }
        if (send_data)
        {
            uart_net_send(uart, recv->head, recv->tail, uart->sksnd_cnt);
        }

        uart->cmd_mode = UART_ATCMD_MODE;
/*        extern u8 default_socket;
        default_socket = tls_cmd_get_default_socket();
        cmd_rsp = tls_mem_alloc(strlen("+OK=%u,%u\r\n\r\n")+1);
		if (!cmd_rsp)
    		return;
		len = sprintf(cmd_rsp, "+OK=%u,%u\r\n\r\n",default_socket,uart->sksnd_cnt);
		err = tls_hostif_process_cmdrsp(HOSTIF_MODE_UART1_LS, cmd_rsp, len);
	    if (err){
	        tls_mem_free(cmd_rsp);
		    }*/
//      tls_ timer2 _stop();
//      if(data_cnt > uart->sksnd_cnt)
//      {
//          uart_rx(uart);
//      }
    }
#endif // TLS_CONFIG_SOCKET_RAW
    else
    {
    /* TODO: */
        ;
    }
//  recv->head = recv->tail = 0;
}

void uart_tx(struct uart_tx_msg *tx_data)
{
    struct tls_uart *uart = tx_data->uart;
    struct tls_hostif *hif = tls_get_hostif();
    struct tls_hostif_tx_msg *tx_msg = tx_data->tx_msg;
    u32 cpu_sr;
    tls_uart_tx_msg_t *uart_tx_msg;
    struct pbuf *p;

//TLS_DBGPRT_INFO("tx_msg->type=%d\r\n", tx_msg->type);
    switch (tx_msg->type)
    {
        case HOSTIF_TX_MSG_TYPE_EVENT:
        case HOSTIF_TX_MSG_TYPE_CMDRSP:
#if 0
            uart_tx_msg = tls_mem_alloc(sizeof(tls_uart_tx_msg_t));
            if (uart_tx_msg == NULL)
            {
                uart_tx_event_finish_callback(tx_msg->u.msg_event.buf);
                goto out;
            }
            dl_list_init(&uart_tx_msg->list);
            uart_tx_msg->buf = tx_msg->u.msg_event.buf;
            uart_tx_msg->buflen = tx_msg->u.msg_event.buflen;
            uart_tx_msg->offset = 0;
            uart_tx_msg->finish_callback = uart_tx_event_finish_callback;
            uart_tx_msg->callback_arg = tx_msg->u.msg_event.buf;
        // if (tx_msg->offset >= tx_msg->u.msg_event.buflen) {
        // tls_mem_free(tx_msg->u.msg_event.buf);
            cpu_sr = tls_os_set_critical();
            dl_list_add_tail(&uart->uart_port->tx_msg_pending_list,
                             &uart_tx_msg->list);
            tls_os_release_critical(cpu_sr);
        // }
#endif
            tls_uart_fill_buf(uart->uart_port, tx_msg->u.msg_event.buf,
                              tx_msg->u.msg_event.buflen);
            uart_tx_event_finish_callback(tx_msg->u.msg_event.buf);
            tls_uart_tx_chars_start(uart->uart_port);
            break;
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
        // Tcp and Udp both use the below case.
        case HOSTIF_TX_MSG_TYPE_UDP:
        case HOSTIF_TX_MSG_TYPE_TCP:
            if (uart->cmd_mode == UART_TRANS_MODE || hif->rptmode)
            {
            // if (uart_circ_chars_pending(&uart->uart_port->xmit) > 4000) {
            // return;
            // }
                p = (struct pbuf *) tx_msg->u.msg_tcp.p;
                uart_tx_msg = tls_mem_alloc(sizeof(tls_uart_tx_msg_t));
                if (uart_tx_msg == NULL)
                {
                    uart_tx_socket_finish_callback(p);
                    goto out;
                }
                dl_list_init(&uart_tx_msg->list);
                uart_tx_msg->buf = p->payload;
                uart_tx_msg->buflen = p->tot_len;
                uart_tx_msg->offset = 0;
                uart_tx_msg->finish_callback = uart_tx_socket_finish_callback;
                uart_tx_msg->callback_arg = p;

                cpu_sr = tls_os_set_critical();
                dl_list_add_tail(&uart->uart_port->tx_msg_pending_list,
                                 &uart_tx_msg->list);
                tls_os_release_critical(cpu_sr);
            // uart_tcp_recv(uart->uart_port, tx_msg);
                tls_uart_tx_chars_start(uart->uart_port);
            }
            else
            {
#if TLS_CONFIG_CMD_NET_USE_LIST_FTR			
				struct tls_uart_net_buf *net_buf;
				struct tls_uart_net_msg *net_msg;            

				net_msg = tls_hostif_get_recvmit(tx_msg->u.msg_tcp.sock);
				net_buf = tls_mem_alloc(sizeof(struct tls_uart_net_buf));
				p = (struct pbuf *) tx_msg->u.msg_tcp.p;
				if (net_buf == NULL)
				{
					pbuf_free(p);
					goto out;
				}

				dl_list_init(&net_buf->list);
				net_buf->buf = p->payload;
				net_buf->pbuf = p;
				net_buf->buflen = p->tot_len;
				net_buf->offset = 0;
				
				cpu_sr = tls_os_set_critical();
				dl_list_add_tail(&net_msg->tx_msg_pending_list,
								 &net_buf->list);
				tls_os_release_critical(cpu_sr);    
#else				
				cache_tcp_recv(tx_msg);
#endif				            
            }
            break;
#endif // TLS_CONFIG_SOCKET_RAW
        default:
            break;
    }
  out:
    if (tx_msg)
        tls_mem_free(tx_msg);
    if (tx_data)
        tls_mem_free(tx_data);
}
#endif /* CONFIG_UART */
