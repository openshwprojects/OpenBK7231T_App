/**
 * @file    wm_hspi_task.c
 *
 * @brief   High speed spi task Module
 *
 * @author  dave
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */

#include <string.h>
#include "wm_hspi_task.h"
#include "wm_debug.h"
#include "wm_regs.h"
#include "wm_params.h"
#include "wm_mem.h"
#include "wm_hspi.h"
#if (GCC_COMPILE==1)
#include "wm_cmdp_hostif_gcc.h"
#else
#include "wm_cmdp_hostif.h"
#endif
#include "wm_config.h"
#include "wm_mem.h"
#include "wm_wl_task.h"
#include "wm_fwup.h"
#include "lwip/mem.h"

#if (TLS_CONFIG_HS_SPI && TLS_CONFIG_HOSTIF)
//efine      HSPI_RX_TASK_STK_SIZE          512
//efine      HSPI_TX_TASK_STK_SIZE          512


/*
 * hspi rx/tx task stack
 */

struct tls_hspi g_hspi;
extern struct tls_slave_hspi g_slave_hspi;

/**
 * @struct hspi_tx_data
 */
struct hspi_tx_data
{
    struct tls_hspi *hspi;
    struct tls_hostif_tx_msg *tx_msg;
};

#if HSPI_TX_MEM_MALLOC
static void hspi_free_txbuf(struct tls_hspi *hspi)
{
    struct tls_hspi_tx_desc *tx_desc;
    u8 n = 0;

    if (1 == hspi->tls_slave_hspi->txdoneflag)
    {
        hspi->tls_slave_hspi->txdoneflag = 0;
    }
    else
        return;

    tx_desc = hspi->tls_slave_hspi->curr_tx_desc;

    while (n < HSPI_TXBUF_NUM)
    {
        if ((0 == (tx_desc->valid_ctrl & BIT(31)))
            && (tx_desc->txbuf_addr != NULL))
        {
            if (tx_desc->txbuf_addr == tx_desc->buf_addr[0])
            {
                mem_free((void *) tx_desc->txbuf_addr);
            }
            else
            {
                pbuf_free((void *) tx_desc->txbuf_addr);
            }
            tx_desc->txbuf_addr = NULL;
            tx_desc->buf_addr[0] = NULL;
        }
        n++;
        tx_desc = (struct tls_hspi_tx_desc *) tx_desc->next_desc_addr;
    }
}
#endif


static int hspi_socket_recv(struct tls_hspi *hspi,
                            struct tls_hspi_tx_desc *tx_desc,
                            struct tls_hostif_tx_msg *tx_msg)
{
    volatile u16 offset = 0;
    u8 skt_num = tx_msg->u.msg_tcp.sock;
    struct pbuf *p;
    u16 buflen, copylen;
    u8 head_size;
    struct tls_hostif_extaddr *extaddr;
    u16 total_len, padlen;
    struct tls_hostif *hif = tls_get_hostif();

    p = (struct pbuf *) tx_msg->u.msg_tcp.p;
    buflen = p->tot_len;
    if (tx_msg->type == HOSTIF_TX_MSG_TYPE_UDP)
    {
        skt_num = skt_num | (1 << 6);
        head_size =
            sizeof(struct tls_hostif_hdr) + sizeof(struct tls_hostif_extaddr);
    }
    else
        head_size = sizeof(struct tls_hostif_hdr);
    offset = tx_msg->offset;

// TLS_DBGPRT_INFO("pbuf len = %d, offset = %d\n", buflen, offset);

#if HSPI_TX_MEM_MALLOC
    if (tx_desc->txbuf_addr != NULL)
    {
        printf("\nhspi_socket_recv txbuf not null,error\n");
        hspi_free_txbuf(hspi);
    }

    tx_desc->txbuf_addr = (u32) p;
    tx_desc->buf_addr[0] = (u32) p->payload - head_size;
    if (tx_desc->buf_addr[0] % 4 != 0)
    {
        u8 *temp;
        temp = (u8 *) (((u32) tx_desc->buf_addr[0]) / 4 * 4);
        printf("bufaddr==%x,p==%x,len==%d\n", tx_desc->buf_addr[0], temp,
               p->tot_len);
        memmove(temp, (void *) tx_desc->buf_addr[0], p->tot_len + head_size);
        tx_desc->buf_addr[0] = (u32) temp;
    }
// printf("\nhspi_socket_recv txbuf
// addr==%x,addr0=%x\n",tx_desc->txbuf_addr,tx_desc->buf_addr[0]);
    copylen = buflen;
    offset = 0;
// hspitxtotlen += p->tot_len;
#else
    do
    {
        if ((buflen - offset) > (HSPI_TXBUF_SIZE - head_size))
            copylen = HSPI_TXBUF_SIZE - head_size;
        else
            copylen = buflen - offset;

    /* copy the contents of the received buffer into hspi tx buffer */
        copylen = pbuf_copy_partial(p,
                                    (u8 *) (tx_desc->buf_addr[0] + head_size +
                                            offset), copylen, offset);
        offset += copylen;
        TLS_DBGPRT_INFO("copylen = %d, offset = %d\n", copylen, offset);
    }
    while (offset < p->tot_len);
#endif
/* fill header */
    tls_hostif_fill_hdr(hif,
                        (struct tls_hostif_hdr *) tx_desc->buf_addr[0],
                        PACKET_TYPE_DATA, copylen, 0, skt_num, 0);
    if (tx_msg->type == HOSTIF_TX_MSG_TYPE_UDP)
    {
        extaddr = (struct tls_hostif_extaddr *) (tx_desc->buf_addr[0] +
                                                 sizeof(struct tls_hostif_hdr));
		extaddr->ip_addr = ip_addr_get_ip4_u32(&tx_msg->u.msg_udp.ip_addr);
        extaddr->remote_port = host_to_be16(tx_msg->u.msg_udp.port);
        extaddr->local_port = host_to_be16(tx_msg->u.msg_udp.localport);
    }
/* fill hspi tx descriptor */
    total_len = copylen + head_size;
    padlen = (4 - (total_len & 0x3)) & 0x3;
    total_len += padlen;
    tx_desc->buf_info = total_len << 12;
/* enable hspi tx */
    tx_desc->valid_ctrl = (1UL << 31);
    tls_reg_write32(HR_SDIO_RXEN, 0x01);
#if !HSPI_TX_MEM_MALLOC
    offset = 0;
    pbuf_free(p);
#endif

    return 0;
}

static int hspi_tx(struct hspi_tx_data *tx_data)
{
    struct tls_hspi *hspi = tx_data->hspi;
    struct tls_hspi_tx_desc *tx_desc;
    struct tls_hostif_tx_msg *tx_msg = tx_data->tx_msg;
//    struct tls_hostif *hif = tls_get_hostif();
//    u32 cpu_sr;
    int err = 0;
//    int offset;

#if HSPI_TX_MEM_MALLOC
// printf("\ntx done flag=%d\n",hspi->tls_slave_hspi->txdoneflag);
    if (1 == hspi->tls_slave_hspi->txdoneflag)
    {
        hspi->tls_slave_hspi->txdoneflag = 0;
        hspi_free_txbuf(hspi);
    }
#endif
    tx_desc = hspi->tls_slave_hspi->curr_tx_desc;

    while (tx_desc->valid_ctrl & BIT(31))
    {
        ;
    }

    switch (tx_msg->type)
    {
        case HOSTIF_TX_MSG_TYPE_EVENT:
        case HOSTIF_TX_MSG_TYPE_CMDRSP:
            if (tx_msg->u.msg_cmdrsp.buflen > HSPI_TXBUF_SIZE)
            {
                tx_msg->u.msg_cmdrsp.buflen = HSPI_TXBUF_SIZE;
            }
#if HSPI_TX_MEM_MALLOC
            if (tx_desc->txbuf_addr != NULL)
            {
                printf("\nhspi resp tx buf not error\n");
                hspi_free_txbuf(hspi);
            }

            {
                tx_desc->txbuf_addr =
                    (u32) mem_malloc(tx_msg->u.msg_cmdrsp.buflen + 1);
                if (NULL == tx_desc->txbuf_addr)
                {
                    printf("\nhspi resp tx buf malloc error\n");
                    break;
                }
                tx_desc->buf_addr[0] = tx_desc->txbuf_addr;
            }
        // printf("\nhspi tx bufaddr==%x\n",tx_desc->txbuf_addr);
#endif
            MEMCPY((char *) tx_desc->buf_addr[0],
                   tx_msg->u.msg_cmdrsp.buf, tx_msg->u.msg_cmdrsp.buflen);
            tls_mem_free(tx_msg->u.msg_cmdrsp.buf);
            tx_desc->buf_info = (tx_msg->u.msg_cmdrsp.buflen) << 12;

            tx_desc->valid_ctrl = (1UL << 31);
            tls_reg_write32(HR_SDIO_RXEN, 0x01);
            break;

        case HOSTIF_TX_MSG_TYPE_UDP:
        case HOSTIF_TX_MSG_TYPE_TCP:
            hspi_socket_recv(hspi, tx_desc, tx_msg);
            break;

        default:
        /* cant go here */
            err = -1;
            break;
    }
    tx_desc = (struct tls_hspi_tx_desc *) tx_desc->next_desc_addr;
    hspi->tls_slave_hspi->curr_tx_desc = tx_desc;

    if (tx_msg)
        tls_mem_free(tx_msg);
    if (tx_data)
        tls_mem_free(tx_data);

    return err;
}

#if 0
void tls_hspi_tx_task(void *data)
{
    u8 err;
    struct tls_hspi *hspi = (struct tls_hspi *) data;

    for (;;)
    {
        err = tls_os_sem_acquire(hspi->tx_msg_sem, 0);
        if (err == TLS_OS_SUCCESS)
        {
            hspi_tx(hspi);
        }
        else
        {
            TLS_DBGPRT_ERR("hspi_tx_task acquire semaphore, "
                           "error type %d", err);
        }
    }
}
#endif

void tls_set_hspi_fwup_mode(u8 ifenable)
{
    struct tls_hspi *hspi = &g_hspi;

    hspi->iffwup = ifenable;

    return;
}

static void hspi_fwup_rsp(int status)
{
    char *cmd_rsp = NULL;
    u8 total_len, rsp_len;
    u8 hostif_type;
    struct tls_hostif *hif;
    struct tls_hostif_hdr *hdr;
    struct tls_hostif_cmd_hdr *rsp;

    rsp_len = sizeof(struct tls_hostif_cmd_hdr);
    total_len = sizeof(struct tls_hostif_hdr) + rsp_len;
    cmd_rsp = tls_mem_alloc(total_len);
    if (NULL == cmd_rsp)
    {
        return;
    }

    memset(cmd_rsp, 0, total_len);
    hif = tls_get_hostif();
    hdr = (struct tls_hostif_hdr *) cmd_rsp;
    hdr->sync = 0xAA;
    hdr->type = PACKET_TYPE_DATA;
    hdr->length = host_to_be16(rsp_len);
    hdr->seq_num = hif->hspi_tx_seq++;
    hdr->flag = 0;
    hdr->dest_addr = 0;
    hdr->chk = 0;

    rsp = (struct tls_hostif_cmd_hdr *) (cmd_rsp + (total_len - rsp_len));
    rsp->msg_type = 0x2;
    rsp->code = HOSTIF_CMD_UPDD;
    rsp->err = status;
    rsp->ext = 0;

    hostif_type = HOSTIF_MODE_HSPI;

    if (tls_hostif_process_cmdrsp(hostif_type, cmd_rsp, total_len))
    {
        tls_mem_free(cmd_rsp);
    }

    return;
}

static void hspi_fwup_send(struct tls_hspi *hspi,
                           struct tls_hspi_rx_desc *rx_desc)
{
    int err = 0;
    int session_id;
    struct tls_hostif_hdr *hdr;

    hspi->iffwup = 0;

    session_id = tls_fwup_get_current_session_id();
    if ((0 == session_id)
        || (TLS_FWUP_STATUS_OK != tls_fwup_current_state(session_id)))
    {
        err = -CMD_ERR_INV_PARAMS;
        hspi_fwup_rsp(err);
        return;
    }

    hdr = (struct tls_hostif_hdr *) rx_desc->buf_addr;
    err = tls_fwup_request_sync(session_id,
                                (u8 *) hdr + sizeof(struct tls_hostif_hdr),
                                be_to_host16(hdr->length));
    hspi_fwup_rsp(err);

    return;
}

static int hspi_net_send(struct tls_hspi *hspi,
                         struct tls_hspi_rx_desc *rx_desc)
{
    struct tls_hostif_hdr *hdr;
    struct tls_hostif_socket_info skt_info;
    struct tls_hostif_ricmd_ext_hdr *ext_hdr;
// struct tls_hostif_cmd_hdr *cmd_hdr;
    int socket_num;
    u8 dest_type;
    u32 buflen;
    char *buf;
	int ret = 0;

// TLS_DBGPRT_INFO("----------->\n");

    hdr = (struct tls_hostif_hdr *) rx_desc->buf_addr;
    if (hdr->type != 0x0)
        return -1;

    buflen = be_to_host16(hdr->length);
    if (buflen > (HSPI_RXBUF_SIZE - sizeof(struct tls_hostif_hdr)))
        return -1;

    socket_num = hdr->dest_addr & 0x3F;
    dest_type = (hdr->dest_addr & 0xC0) >> 6;

    skt_info.socket = socket_num;
    skt_info.proto = dest_type;
    if (dest_type == 1)
    {
    /* udp */
        ext_hdr =
            (struct tls_hostif_ricmd_ext_hdr *) ((char *) rx_desc->buf_addr +
                                                 sizeof(struct tls_hostif_hdr));
        skt_info.remote_ip = ext_hdr->remote_ip;
        skt_info.remote_port = ext_hdr->remote_port;
        skt_info.local_port = ext_hdr->local_port;
        skt_info.socket = 0;
        buf = (char *) ext_hdr + sizeof(struct tls_hostif_ricmd_ext_hdr);
    }
    else
    {
        buf = (char *) ((char *) rx_desc->buf_addr +
                        sizeof(struct tls_hostif_hdr));
    }
	do
	{
		ret = tls_hostif_send_data(&skt_info, buf, buflen);
		if (0 == ret)
		{
			break;
		}
		tls_os_time_delay(10);
	}while(ret == -1);

	if (ret != 0)
	{
		printf("send failed in spi net send function\r\n");
	}

    return 0;
}

static int hspi_rx_data(void *data)
{
    struct tls_hspi *hspi = (struct tls_hspi *) data;
    struct tls_hspi_rx_desc *rx_desc;

/* get rx descriptor */
    rx_desc = hspi->tls_slave_hspi->curr_rx_desc;
    while (!(rx_desc->valid_ctrl & BIT(31)))
    {
#if 0
        {
            int i;
            int errorflag = 0;
        // for(i = 0;i < 32;i ++)
            for (i = 0; i < 1500; i++)
            {
                if (*(u8 *) (rx_desc->buf_addr + i) != 0x38)
                {
                    errorflag++;
                    printf("[%d]=[%x]\n", i, *(u8 *) (rx_desc->buf_addr + i));
                }
            }
            printf("show over[%c][%c],errflag=%d\n",
                   *(u8 *) (rx_desc->buf_addr),
                   *(u8 *) (rx_desc->buf_addr + 1499), errorflag);
            if (errorflag > 0)
                while (1);
#if 0
            for (i = 1468; i < 1500; i++)
            {
                printf("[%x]", *(u8 *) (rx_desc->buf_addr + i));
            }
#endif
            printf("\n");

        }

#endif
        if (hspi->iffwup)
        {
            hspi_fwup_send(hspi, rx_desc);
        }
        else
        {
        /* transmit data to lwip stack */
            hspi_net_send(hspi, rx_desc);
        }

    // hspi_free_rxdesc(rx_desc);
        rx_desc->valid_ctrl = BIT(31);
    /* 设置hspi/sdio tx enable寄存器，让sdio硬件知道有可用的tx descriptor */
        tls_reg_write32(HR_SDIO_TXEN, BIT(0));

        rx_desc = (struct tls_hspi_rx_desc *) rx_desc->next_desc_addr;
        hspi->tls_slave_hspi->curr_rx_desc = rx_desc;
    }
#if 0                           // 测试hspi传输数据
    {
        char *txbuf;
#if 1
        txbuf = tls_mem_alloc(1600);
        if (NULL == txbuf)
        {
            printf("\nmalloc error\n");
            return;
        }
        memset(txbuf, 0x35, 1500);
#endif
        tls_hspi_tx_data(txbuf, 1500);

    // tls_hspi_tx_data("lhy1", 4);
#if 1
        tls_mem_free(txbuf);
#endif
    }
#endif
    return 0;

}

static int hspi_rx_cmd(void *data)
{
//    struct tls_hspi *hspi = (struct tls_hspi *)data;
    struct tls_hostif_hdr *hdr;
    u8 err;
    u8 *buf;
    u16 len, type;
/* get command from HSPI RX CMD buffer */

    buf = (u8 *) SDIO_CMD_RXBUF_ADDR;

    err = tls_hostif_hdr_check(buf, SDIO_CMD_RXBUF_SIZE);
    if (!err)
    {
        hdr = (struct tls_hostif_hdr *) buf;
        if (hdr->type == 0x1)
            type = HOSTIF_HSPI_RI_CMD;
        else if (hdr->type == 0x2)
            type = HOSTIF_HSPI_AT_CMD;
        else
        {
        /* enable command buffer */
            tls_reg_write32(HR_SDIO_DOWNCMDVALID, 0x1);
            return 0;
        }
        len = hdr->length;
#if 0
        {
            int i;
            char *txbuf;
            printf("\nlen=%d\n", be_to_host16(len));
            for (i = 0; i < be_to_host16(len) + 8; i++)
            {
                printf("[%x]", buf[i]);
                if (i > 0 && i % 32 == 0)
                    printf("\n");
            }
            printf("\n");
        }

#endif


        tls_hostif_cmd_handler(type, (char *) buf, len);
    }
    else
    {
    // TODO:
    }

/* enable command buffer */
    tls_reg_write32(HR_SDIO_DOWNCMDVALID, 0x1);
    return 0;
}

#if 0
void tls_hspi_rx_task(void *data)
{
    struct tls_hspi *hspi = (struct tls_hspi *) data;
    struct tls_hostif_hdr *hdr;
    u8 err;
    u32 *msg;
    u8 *buf;
    u16 len, type;

    for (;;)
    {
        err = tls_os_queue_receive(hspi->rx_msg_queue, (void **) &msg, 0, 0);
        if (!err)
        {
            switch ((u32) msg)
            {
                case HSPI_RX_CMD_MSG:
                /* get command from HSPI RX CMD buffer */
                    buf = (u8 *) SDIO_CMD_RXBUF_ADDR;

                    err = tls_hostif_hdr_check(buf, SDIO_CMD_RXBUF_SIZE);
                    if (!err)
                    {
                        hdr = (struct tls_hostif_hdr *) buf;
                        if (hdr->type == 0x1)
                            type = HOSTIF_HSPI_RI_CMD;
                        else if (hdr->type == 0x2)
                            type = HOSTIF_HSPI_AT_CMD;
                        else
                        {
                        /* enable command buffer */
                            tls_reg_write32(HR_SDIO_DOWNCMDVALID, 0x1);
                            break;
                        }
                        len = hdr->length;
                        tls_hostif_cmd_handler(type, (char *) buf, len);
                    }
                    else
                    {
                    // TODO:
                    }

                /* enable command buffer */
                    tls_reg_write32(HR_SDIO_DOWNCMDVALID, 0x1);
                    break;
                case HSPI_RX_DATA_MSG:
                    hspi_rx_data(hspi);
                    break;
                default:
                    break;
            }
        }
        else
        {
            TLS_DBGPRT_INFO("err = %d\n", err);
        }
    }
}
#endif

static void tls_hspi_ram_info_dump(void)
{
    TLS_DBGPRT_INFO("HSPI_TXBUF_BASE_ADDR       : 0x%x -- 0x%x \n",
                    HSPI_TXBUF_BASE_ADDR, HSPI_TX_DESC_BASE_ADDR - 1);
    TLS_DBGPRT_INFO("HSPI_TX_DESC_BASE_ADDR     : 0x%x -- 0x%x \n",
                    HSPI_TX_DESC_BASE_ADDR, HSPI_RXBUF_BASE_ADDR - 1);
    TLS_DBGPRT_INFO("HSPI_RXBUF_BASE_ADDR       : 0x%x -- 0x%x \n",
                    HSPI_RXBUF_BASE_ADDR, HSPI_RX_DESC_BASE_ADDR - 1);
    TLS_DBGPRT_INFO("HSPI_RX_DESC_BASE_ADDR     : 0x%x -- 0x%x \n",
                    HSPI_RX_DESC_BASE_ADDR,
                    HSPI_RX_DESC_BASE_ADDR + HSPI_RX_DESC_TOTAL_SIZE);
}

extern struct task_parameter wl_task_param_hostif;
static s16 tls_hspi_rx_cmd_cb(char *buf)
{
    struct tls_hspi *hspi = &g_hspi;

// if(hspi->rx_msg_queue)
// tls_os_queue_send(hspi->rx_msg_queue, (void *)HSPI_RX_CMD_MSG, 0);
    tls_wl_task_callback_static(&wl_task_param_hostif,
                                (start_routine) hspi_rx_cmd, hspi, 0,
                                TLS_MSG_ID_HSPI_RX_CMD);
    return WM_SUCCESS;
}

static s16 tls_hspi_rx_data_cb(char *buf)
{
    struct tls_hspi *hspi = &g_hspi;

// if(hspi->rx_msg_queue)
// tls_os_queue_send(hspi->rx_msg_queue, (void *)HSPI_RX_DATA_MSG, 0);
    tls_wl_task_callback_static(&wl_task_param_hostif,
                                (start_routine) hspi_rx_data, hspi, 0,
                                TLS_MSG_ID_HSPI_RX_DATA);
    return WM_SUCCESS;
}

static s16 tls_hspi_tx_data_cb(char *buf)
{
//    struct tls_hspi *hspi = &g_hspi;

// if(hspi->tx_msg_sem)
// tls_os_sem_release(hspi->tx_msg_sem);
#if HSPI_TX_MEM_MALLOC
    tls_wl_task_callback_static(&wl_task_param_hostif,
                                (start_routine) hspi_free_txbuf, hspi, 0,
                                TLS_MSG_ID_HSPI_TX_DATA);
#endif
    return WM_SUCCESS;
}

static void hspi_send_tx_msg(u8 hostif_mode, struct tls_hostif_tx_msg *tx_msg,
                             bool is_event)
{
//    u32 cpu_sr;
    struct hspi_tx_data *tx_data = NULL;
    if (tx_msg == NULL)
        return;
    switch (hostif_mode)
    {
        case HOSTIF_MODE_HSPI:
            tx_data = tls_mem_alloc(sizeof(struct hspi_tx_data));
            if (tx_data == NULL)
            {
                free_tx_msg_buffer(tx_msg);
                tls_mem_free(tx_msg);
                return;
            }
            tx_data->tx_msg = tx_msg;
            tx_data->hspi = &g_hspi;
            if (tls_wl_task_callback
                (&wl_task_param_hostif, (start_routine) hspi_tx, tx_data, 0))
            {
                free_tx_msg_buffer(tx_msg);
                tls_mem_free(tx_msg);
                tls_mem_free(tx_data);
            }
            break;
        default:
            free_tx_msg_buffer(tx_msg);
            tls_mem_free(tx_msg);
            break;
    }
}

int tls_hspi_init(void)
{
    struct tls_hspi *hspi;
//    char *stk;
//    int err;
//    void *rx_msg;
    struct tls_hostif *hif = tls_get_hostif();
    u8 mode;

    hspi = &g_hspi;
    memset(hspi, 0, sizeof(struct tls_hspi));

    tls_param_get(TLS_PARAM_ID_USRINTF, &mode, TRUE);

    tls_slave_spi_init();
    tls_set_high_speed_interface_type(mode);
    hspi->tls_slave_hspi = &g_slave_hspi;

    tls_hspi_rx_cmd_callback_register(tls_hspi_rx_cmd_cb);
    tls_hspi_rx_data_callback_register(tls_hspi_rx_data_cb);
    tls_hspi_tx_data_callback_register(tls_hspi_tx_data_cb);
// tls_os_sem_create(&hspi->tx_msg_sem, 0);
    hif->hspi_send_tx_msg_callback = hspi_send_tx_msg;

    tls_hspi_ram_info_dump();
#if 0
/* create rx messge queue */
#define HSPI_RX_MSG_SIZE     20
    rx_msg = tls_mem_alloc(HSPI_RX_MSG_SIZE * sizeof(void *));
    if (!rx_msg)
        return WM_FAILED;

    err = tls_os_queue_create(&hspi->rx_msg_queue, rx_msg, HSPI_RX_MSG_SIZE, 0);
    if (err)
        return WM_FAILED;

/* create hspi rx task */
    stk = tls_mem_alloc(HSPI_RX_TASK_STK_SIZE * sizeof(u32));
    if (!stk)
        return WM_FAILED;
    memset(stk, 0, HSPI_RX_TASK_STK_SIZE * sizeof(u32));
    tls_os_task_create(NULL, "hspi_rx_task", tls_hspi_rx_task, (void *) hspi, (void *) stk, /* 任务栈的起始地址
                                                                                             */
                       HSPI_RX_TASK_STK_SIZE * sizeof(u32), /* 任务栈的大小 */
                       TLS_HSPI_RX_TASK_PRIO, 0);
/* create hspi tx task */
    stk = tls_mem_alloc(HSPI_TX_TASK_STK_SIZE * sizeof(u32));
    if (!stk)
        return WM_FAILED;
    memset(stk, 0, HSPI_TX_TASK_STK_SIZE * sizeof(u32));
    tls_os_task_create(NULL, "hspi_tx_task", tls_hspi_tx_task, (void *) hspi, (void *) stk, /* 任务栈的起始地址
                                                                                             */
                       HSPI_TX_TASK_STK_SIZE * sizeof(u32), /* 任务栈的大小 */
                       TLS_HSPI_TX_TASK_PRIO, 0);
#endif

    tls_hostif_set_net_status_callback();
    tls_hostif_send_event_init_cmplt();

    return WM_SUCCESS;
}


#endif
