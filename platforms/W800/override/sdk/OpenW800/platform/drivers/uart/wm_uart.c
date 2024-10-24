/**
 * @file    wm_uart.c
 *
 * @brief   uart Driver Module
 *
 * @author  dave
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */
#include <string.h>
#include "wm_regs.h"
#include "wm_uart.h"
#include "wm_debug.h"
#include "wm_irq.h"
#include "wm_config.h"
#include "wm_mem.h"
#include "wm_dma.h"
#include "wm_cpu.h"


#if TLS_CONFIG_UART

#define RX_CACHE_LIMIT  128
#define STEP_SIZE       (HR_UART1_BASE_ADDR - HR_UART0_BASE_ADDR)

extern void tls_irq_priority(u8 vec_no, u32 prio);
void tls_uart_tx_callback_register(u16 uart_no, s16(*tx_callback) (struct tls_uart_port *port));

const u32 baud_rates[] = { 2000000, 1500000, 1250000, 1000000, 921600, 460800,
                           230400, 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1800, 1200, 600 };
struct tls_uart_port uart_port[TLS_UART_MAX];
static u8 uart_rx_byte_cb_flag[TLS_UART_MAX] = {0};

static void tls_uart_tx_enable(struct tls_uart_port *port)
{
    u32 ucon;

    ucon = port->regs->UR_LC;
    ucon |= ULCON_TX_EN;
    port->regs->UR_LC = ucon;
}

/**
 * @brief	This function is used to continue transfer data.
 * @param[in] port: is the uart port.
 * @retval
 */
static void tls_uart_tx_chars(struct tls_uart_port *port)
{
	struct dl_list *pending_list = &port->tx_msg_pending_list;
	tls_uart_tx_msg_t *tx_msg = NULL;
	int tx_count;
	u32 cpu_sr;

/* send some chars */
	tx_count = 32;
	cpu_sr = tls_os_set_critical();
	if (!dl_list_empty(pending_list))
	{
		tx_msg = dl_list_first(pending_list, tls_uart_tx_msg_t, list);
		while (tx_count-- > 0 && tx_msg->offset < tx_msg->buflen)
		{
		/* 检查tx fifo是否已满 */
			if ((port->regs->UR_FIFOS & UFS_TX_FIFO_CNT_MASK) == port->tx_fifofull)
			{
				break;
			}
			port->regs->UR_TXW = tx_msg->buf[tx_msg->offset];
			tx_msg->offset++;
			port->icount.tx++;
		}

		if (tx_msg->offset >= tx_msg->buflen)
		{
			dl_list_del(&tx_msg->list);
			dl_list_add_tail(&port->tx_msg_to_be_freed_list, &tx_msg->list);
			tls_os_release_critical(cpu_sr);

			if (port->tx_sent_callback)
				port->tx_sent_callback(port);
			
			if (port->tx_callback)
				port->tx_callback(port);
		}else{
				tls_os_release_critical(cpu_sr);
		}
	}
	else{
		tls_os_release_critical(cpu_sr);
    }
}

static void UartRegInit(int uart_no)
{
    u32 bd;
	u32 apbclk;
	tls_sys_clk sysclk;

	tls_sys_clk_get(&sysclk);
	apbclk = sysclk.apbclk * 1000000;

    if (TLS_UART_0 == uart_no)
    {
        bd = (apbclk / (16 * 115200) - 1) | (((apbclk % (115200 * 16)) * 16 / (115200 * 16)) << 16);
        tls_reg_write32(HR_UART0_BAUD_RATE_CTRL, bd);
    /* Line control register : Normal,No parity,1 stop,8 bits, only use tx */
        tls_reg_write32(HR_UART0_LINE_CTRL, ULCON_WL8 | ULCON_TX_EN);

    /* disable auto flow control */
        tls_reg_write32(HR_UART0_FLOW_CTRL, 0);
    /* disable dma */
        tls_reg_write32(HR_UART0_DMA_CTRL, 0);
    /* one byte tx */
        tls_reg_write32(HR_UART0_FIFO_CTRL, 0);
    /* disable interrupt */
        tls_reg_write32(HR_UART0_INT_MASK, 0xFF);
    }
	else
	{		
	/* 4 byte tx, 8 bytes rx */
        tls_reg_write32(HR_UART0_FIFO_CTRL + uart_no*STEP_SIZE, (0x01 << 2) | (0x02 << 4));
    /* enable rx timeout, disable rx dma, disable tx dma */
        tls_reg_write32(HR_UART0_DMA_CTRL + uart_no*STEP_SIZE, (8 << 3) | (1 << 2));
    /* enable rx/timeout interrupt */
        tls_reg_write32(HR_UART0_INT_MASK + uart_no*STEP_SIZE, ~(3 << 2));
	}
}

int tls_uart_check_baudrate(u32 baudrate)
{
    int i;

    for (i = 0; i < sizeof(baud_rates) / sizeof(u32); i++)
    {
        if (baudrate == baud_rates[i])
            return 1;
    }
/* not found match baudrate */
    return -1;
}

u32 tls_uart_get_baud_rate(struct tls_uart_port * port)
{
    if ((port != NULL) && (port->regs != NULL))
        return port->opts.baudrate;
    else
        return 0;
}

static int tls_uart_set_baud_rate_inside(struct tls_uart_port *port, u32 baudrate)
{
    int index;
    u32 value;
	u32 apbclk;
	tls_sys_clk sysclk;

    index = tls_uart_check_baudrate(baudrate);
    if (index < 0)
    {
        return WM_FAILED;
    }
    tls_sys_clk_get(&sysclk);
	apbclk = sysclk.apbclk * 1000000;
    value = (apbclk / (16 * baudrate) - 1) |
            (((apbclk % (baudrate * 16)) * 16 / (baudrate * 16)) << 16);
    port->regs->UR_BD = value;

    port->opts.baudrate = baudrate;

    return WM_SUCCESS;
}


static int tls_uart_set_parity_inside(struct tls_uart_port *port, TLS_UART_PMODE_T paritytype)
{
    if (port == NULL)
        return WM_FAILED;

    port->opts.paritytype = paritytype;

    if (paritytype == TLS_UART_PMODE_DISABLED)
        port->regs->UR_LC &= ~ULCON_PMD_EN;
    else if (paritytype == TLS_UART_PMODE_EVEN)
    {
        port->regs->UR_LC &= ~ULCON_PMD_MASK;
        port->regs->UR_LC |= ULCON_PMD_EVEN;
    }
    else if (paritytype == TLS_UART_PMODE_ODD)
    {
        port->regs->UR_LC &= ~ULCON_PMD_MASK;
        port->regs->UR_LC |= ULCON_PMD_ODD;
    }
    else
        return WM_FAILED;

    return WM_SUCCESS;

}

#if 0
static TLS_UART_PMODE_T tls_uart_get_parity(struct tls_uart_port * port)
{
    return port->opts.paritytype;
}
#endif

static int tls_uart_set_data_bits(struct tls_uart_port *port, TLS_UART_CHSIZE_T charlength)
{
    if (!port)
        return WM_FAILED;

    port->opts.charlength = charlength;

    port->regs->UR_LC &= ~ULCON_WL_MASK;

    if (charlength == TLS_UART_CHSIZE_5BIT)
        port->regs->UR_LC |= ULCON_WL5;
    else if (charlength == TLS_UART_CHSIZE_6BIT)
        port->regs->UR_LC |= ULCON_WL6;
    else if (charlength == TLS_UART_CHSIZE_7BIT)
        port->regs->UR_LC |= ULCON_WL7;
    else if (charlength == TLS_UART_CHSIZE_8BIT)
        port->regs->UR_LC |= ULCON_WL8;
    else
        return WM_FAILED;

    return WM_SUCCESS;
}

#if 0
static TLS_UART_CHSIZE_T tls_uart_get_data_bits(struct tls_uart_port * port)
{
    return port->opts.charlength;
}
#endif

static int tls_uart_set_stop_bits_inside(struct tls_uart_port *port, TLS_UART_STOPBITS_T stopbits)
{
    if (!port)
        return WM_FAILED;

    port->opts.stopbits = stopbits;

    if (stopbits == TLS_UART_TWO_STOPBITS)
        port->regs->UR_LC |= ULCON_STOP_2;
    else
        port->regs->UR_LC &= ~ULCON_STOP_2;

    return WM_SUCCESS;
}

#if 0
static TLS_UART_STOPBITS_T tls_uart_get_stop_bits(struct tls_uart_port * port)
{
    return port->opts.stopbits;
}
#endif

static TLS_UART_STATUS_T tls_uart_set_flow_ctrl(struct tls_uart_port * port, TLS_UART_FLOW_CTRL_MODE_T flow_ctrl)
{
    TLS_UART_STATUS_T status = TLS_UART_STATUS_OK;

    if (!port)
        return TLS_UART_STATUS_ERROR;

// port->opts.flow_ctrl = flow_ctrl;
// //不能在这里修改，为了配合透传和AT指令，软件会自己修改flowctrl配置，但是参数还是固定不变的
//printf("\nport %d flow ctrl==%d\n",port->uart_no,flow_ctrl);
    switch (flow_ctrl)
    {
        case TLS_UART_FLOW_CTRL_NONE:
            port->regs->UR_FC = 0;
            break;
        case TLS_UART_FLOW_CTRL_HARDWARE:
            if (TLS_UART_FLOW_CTRL_HARDWARE == port->opts.flow_ctrl)
            {
                port->regs->UR_FC = (1UL << 0) | (6UL << 2);
            }
            break;
        default:
            return TLS_UART_STATUS_ERROR;
    }

    return status;
}

void tls_uart_set_fc_status(int uart_no, TLS_UART_FLOW_CTRL_MODE_T status)
{
    struct tls_uart_port *port;

	port = &uart_port[uart_no];
    port->fcStatus = status;
    tls_uart_set_flow_ctrl(port, status);
    if (TLS_UART_FLOW_CTRL_HARDWARE == port->opts.flow_ctrl && 0 == status && port->hw_stopped) // 准备关闭流控时，发现tx已经停止，需要再打开tx
    {
        tls_uart_tx_enable(port);
        tls_uart_tx_chars(port);
        port->hw_stopped = 0;
    }
}

void tls_uart_rx_disable(struct tls_uart_port *port)
{
    u32 ucon;

    ucon = port->regs->UR_LC;
    ucon &= ~ULCON_RX_EN;
    port->regs->UR_LC = ucon;
}

void tls_uart_rx_enable(struct tls_uart_port *port)
{
    port->regs->UR_LC |= ULCON_RX_EN;
}

static void tls_uart_tx_disable(struct tls_uart_port *port)
{
    u32 ucon;

    ucon = port->regs->UR_LC;
    ucon &= ~ULCON_TX_EN;
    port->regs->UR_LC = ucon;
}


static int tls_uart_config(struct tls_uart_port *port, struct tls_uart_options *opts)
{
    if (NULL == port || NULL == opts)
        return WM_FAILED;
	
    tls_uart_set_baud_rate_inside(port, opts->baudrate);
    tls_uart_set_parity_inside(port, opts->paritytype);
    tls_uart_set_data_bits(port, opts->charlength);
    tls_uart_set_stop_bits_inside(port, opts->stopbits);
    port->opts.flow_ctrl = opts->flow_ctrl;
    tls_uart_set_flow_ctrl(port, opts->flow_ctrl);

    {
    /* clear interrupt */
        port->regs->UR_INTS = 0xFFFFFFFF;
    /* enable interupt */
        port->regs->UR_INTM = 0x0;
        port->regs->UR_DMAC = (4UL << UDMA_RX_FIFO_TIMEOUT_SHIFT) | UDMA_RX_FIFO_TIMEOUT;
    }

/* config FIFO control */
    port->regs->UR_FIFOC = UFC_TX_FIFO_LVL_16_BYTE | UFC_RX_FIFO_LVL_16_BYTE |
        UFC_TX_FIFO_RESET | UFC_RX_FIFO_RESET;
    port->regs->UR_LC &= ~(ULCON_TX_EN | ULCON_RX_EN);
    port->regs->UR_LC |= ULCON_RX_EN | ULCON_TX_EN;

    return WM_SUCCESS;
}

/**
 *	@brief  handle a change of clear-to-send state
 *	@param[in] port: uart_port structure for the open port
 *	@param[in] status: new clear to send status, nonzero if active
 */
static void uart_handle_cts_change(struct tls_uart_port *port, unsigned int status)
{
    if (((1 == port->fcStatus)
         && (port->opts.flow_ctrl == TLS_UART_FLOW_CTRL_HARDWARE))
        && (port->uart_no == TLS_UART_1))
    {
        if (port->hw_stopped)
        {
            if (status)
            {
                port->hw_stopped = 0;
                tls_uart_tx_enable(port);
                tls_uart_tx_chars(port);
            }
        }
        else
        {
            if (!status)
            {
                port->hw_stopped = 1;
                tls_uart_tx_disable(port);
            }
        }
    }
}

static void uart_tx_finish_callback(void *arg)
{
    if (arg)
    {
        tls_mem_free(arg);
    }
}

int tls_uart_tx_remain_len(struct tls_uart_port *port)
{
    tls_uart_tx_msg_t *tx_msg = NULL;
    u16 buf_len = 0;
    u32 cpu_sr;
    cpu_sr = tls_os_set_critical();
    dl_list_for_each(tx_msg, &port->tx_msg_pending_list, tls_uart_tx_msg_t, list)
    {
        buf_len += tx_msg->buflen;
    }
    tls_os_release_critical(cpu_sr);
    return TLS_UART_TX_BUF_SIZE - buf_len;
}

/**
 * @brief	This function is used to fill tx buffer.
 * @param[in] port: is the uart port.
 * @param[in] buf:	is the user buffer.
 * @param[in] count: is the user data length
 * @retval
 */
int tls_uart_fill_buf(struct tls_uart_port *port, char *buf, u32 count)
{
    tls_uart_tx_msg_t *uart_tx_msg;
    int ret = 0;
    u32 cpu_sr;

    uart_tx_msg = tls_mem_alloc(sizeof(tls_uart_tx_msg_t));
    if (uart_tx_msg == NULL)
    {
        TLS_DBGPRT_ERR("mem err\n");
        return -1;
    }
    dl_list_init(&uart_tx_msg->list);
    uart_tx_msg->buf = tls_mem_alloc(count);
    if (uart_tx_msg->buf == NULL)
    {
        tls_mem_free(uart_tx_msg);
        TLS_DBGPRT_ERR("mem err 1 count=%d\n", count);
        return -1;
    }
    memcpy(uart_tx_msg->buf, buf, count);
    uart_tx_msg->buflen = count;
    uart_tx_msg->offset = 0;
    uart_tx_msg->finish_callback = uart_tx_finish_callback;
    uart_tx_msg->callback_arg = uart_tx_msg->buf;
    cpu_sr = tls_os_set_critical();
    dl_list_add_tail(&port->tx_msg_pending_list, &uart_tx_msg->list);
    tls_os_release_critical(cpu_sr);
    return ret;
}

/**
 * @brief	free the data buffer has been transmitted.
 * @param[in] port: is the uart port.
 * @retval
 */
s16 tls_uart_free_tx_sent_data(struct tls_uart_port *port)
{
    tls_uart_tx_msg_t *tx_msg = NULL;
    u32 cpu_sr = tls_os_set_critical();
    while (!dl_list_empty(&port->tx_msg_to_be_freed_list))
    {
        tx_msg = dl_list_first(&port->tx_msg_to_be_freed_list, tls_uart_tx_msg_t, list);
        dl_list_del(&tx_msg->list);
        tls_os_release_critical(cpu_sr);
        if (tx_msg->buf != NULL)
        {
            if (tx_msg->finish_callback)
                tx_msg->finish_callback(tx_msg->callback_arg);
            tls_mem_free(tx_msg);
        }
        cpu_sr = tls_os_set_critical();
    }
    tls_os_release_critical(cpu_sr);	
	return 0;
}

/**
 * @brief	This function is used to start transfer data.
 * @param[in] port: is the uart port.
 * @retval
 */
void tls_uart_tx_chars_start(struct tls_uart_port *port)
{
    struct dl_list *pending_list = &port->tx_msg_pending_list;
    tls_uart_tx_msg_t *tx_msg = NULL;
    int tx_count;
    u32 cpu_sr;

    /* send some chars */
    tx_count = 32;
    cpu_sr = tls_os_set_critical();
    if (!dl_list_empty(pending_list))
    {
        tx_msg = dl_list_first(pending_list, tls_uart_tx_msg_t, list);
        while (tx_count-- > 0 && tx_msg->offset < tx_msg->buflen)
        {
        /* 检查tx fifo是否已满 */
            if ((port->regs->UR_FIFOS & UFS_TX_FIFO_CNT_MASK) ==
                port->tx_fifofull)
            {
                break;
            }
            port->regs->UR_TXW = tx_msg->buf[tx_msg->offset];
            tx_msg->offset++;
            port->icount.tx++;
        }

        if (tx_msg->offset >= tx_msg->buflen)
        {
            dl_list_del(&tx_msg->list);
            dl_list_add_tail(&port->tx_msg_to_be_freed_list, &tx_msg->list);
            tls_os_release_critical(cpu_sr);
			
			if (port->tx_sent_callback)
				port->tx_sent_callback(port);

            if (port->tx_callback)
                port->tx_callback(port);
        }else{
                tls_os_release_critical(cpu_sr);
        }
    }else{
        tls_os_release_critical(cpu_sr);

    }
}

void tls_set_uart_rx_status(int uart_no, int status)
{
    u32 cpu_sr;
    struct tls_uart_port *port;

    if (TLS_UART_1 == uart_no)
    {
        port = &uart_port[1];
        if ((TLS_UART_RX_DISABLE == port->rxstatus
             && TLS_UART_RX_DISABLE == status)
            || (TLS_UART_RX_ENABLE == port->rxstatus
                && TLS_UART_RX_ENABLE == status))
            return;

        if (TLS_UART_RX_DISABLE == status)
        {
            if ((TLS_UART_FLOW_CTRL_HARDWARE == port->opts.flow_ctrl)
                && (TLS_UART_FLOW_CTRL_HARDWARE == port->fcStatus))
            {
                cpu_sr = tls_os_set_critical();
                // 关rxfifo trigger level interrupt和overrun error
                port->regs->UR_INTM |= ((0x1 << 2) | (0x01 << 8));
                port->rxstatus = TLS_UART_RX_DISABLE;
                tls_os_release_critical(cpu_sr);
            }
        }
        else
        {
            cpu_sr = tls_os_set_critical();
            uart_port[1].regs->UR_INTM &= ~((0x1 << 2) | (0x01 << 8));
            port->rxstatus = TLS_UART_RX_ENABLE;
            tls_os_release_critical(cpu_sr);
        }
    }
}

ATTRIBUTE_ISR void UART0_IRQHandler(void)
{
    struct tls_uart_port *port = &uart_port[0];
    struct tls_uart_circ_buf *recv = &port->recv;
    u8 rx_byte_cb_flag = uart_rx_byte_cb_flag[0];
    u32 intr_src;
    u32 rx_fifocnt;
    u32 fifos;
    u8 ch;
    u32 rxlen = 0;
    csi_kernel_intrpt_enter();

/* check interrupt status */
    intr_src = port->regs->UR_INTS;
    port->regs->UR_INTS = intr_src;

    if ((intr_src & UART_RX_INT_FLAG) && (0 == (port->regs->UR_INTM & UIS_RX_FIFO)))
    {
        rx_fifocnt = (port->regs->UR_FIFOS >> 6) & 0x3F;
        while (rx_fifocnt-- > 0)
        {
            ch = (u8) port->regs->UR_RXW;
            if (intr_src & UART_RX_ERR_INT_FLAG)
            {
                port->regs->UR_INTS |= UART_RX_ERR_INT_FLAG;
                TLS_DBGPRT_INFO("\nrx err=%x,c=%d,ch=%x\n", intr_src, rx_fifocnt, ch);
            /* not insert to buffer */
                continue;
            }
            if (CIRC_SPACE(recv->head, recv->tail, TLS_UART_RX_BUF_SIZE) <= 2)
            {
                TLS_DBGPRT_INFO("\nrx buf overrun int_src=%x\n", intr_src);
                if (TLS_UART_FLOW_CTRL_HARDWARE == port->fcStatus)
                {
                    tls_set_uart_rx_status(port->uart_no, TLS_UART_RX_DISABLE);
                    rx_fifocnt = 0; // 如果有硬件流控，关闭接收，把最后一个字符放进环形buffer中
                }
                else
                    break;
            }

        /* insert the character into the buffer */
            recv->buf[recv->head] = ch;
            recv->head = (recv->head + 1) & (TLS_UART_RX_BUF_SIZE - 1);
            rxlen++;
            if(port->rx_callback != NULL && rx_byte_cb_flag)
            {
                port->rx_callback(1, port->priv_data);
            }
        }
        if (rxlen && port->rx_callback != NULL && !rx_byte_cb_flag)
        {
            port->rx_callback(rxlen, port->priv_data);
        }
    }

    if (intr_src & UART_TX_INT_FLAG)
    {
        tls_uart_tx_chars(port);
    }

    if (intr_src & UIS_CTS_CHNG)
    {
        fifos = port->regs->UR_FIFOS;
        uart_handle_cts_change(port, fifos & UFS_CST_STS);
    }
    csi_kernel_intrpt_exit();
}


void tls_uart_push(int index, u8* data, int length)
{
	int i = 0;
	struct tls_uart_port *port = &uart_port[1];
    struct tls_uart_circ_buf *recv = &port->recv;
	while(i<length)
	{
	    recv->buf[recv->head] = data[i];
        recv->head = (recv->head + 1) & (TLS_UART_RX_BUF_SIZE - 1);
		i++;
	}
}

ATTRIBUTE_ISR void UART1_IRQHandler(void)
{
    struct tls_uart_port *port = &uart_port[1];
    struct tls_uart_circ_buf *recv = &port->recv;
    u8 rx_byte_cb_flag = uart_rx_byte_cb_flag[1];
    u32 intr_src;
    u32 rx_fifocnt;
    u32 fifos;
    u8 ch = 0;
    u8 escapefifocnt = 0;
    u32 rxlen = 0;
    csi_kernel_intrpt_enter();

    intr_src = port->regs->UR_INTS;
    port->regs->UR_INTS = intr_src;

	if (intr_src & UIS_OVERRUN)
	{
		port->regs->UR_INTS |= UIS_OVERRUN;
		if(port->tx_dma_on)
		{
			tls_reg_write32((int)&port->regs->UR_DMAC, (tls_reg_read32((int)&port->regs->UR_DMAC) & ~0x01));
			tls_reg_write32((int)&port->regs->UR_DMAC, (tls_reg_read32((int)&port->regs->UR_DMAC) | 0x01));
		}
	}
    if ((intr_src & UART_RX_INT_FLAG) && (0 == (port->regs->UR_INTM & UIS_RX_FIFO)))
    {
        rx_fifocnt = (port->regs->UR_FIFOS >> 6) & 0x3F;
        escapefifocnt = rx_fifocnt;
        port->plus_char_cnt = 0;
        
        if (CIRC_SPACE(recv->head, recv->tail, TLS_UART_RX_BUF_SIZE) <= RX_CACHE_LIMIT)
        {
            recv->tail = (recv->tail + RX_CACHE_LIMIT) & (TLS_UART_RX_BUF_SIZE - 1);
        }

        if (intr_src & UART_RX_ERR_INT_FLAG)
        {
	        while (rx_fifocnt-- > 0)
	        {
	            ch = (u8) port->regs->UR_RXW;
	        }
            rxlen = 0;
            port->regs->UR_INTS |= UART_RX_ERR_INT_FLAG;
            TLS_DBGPRT_INFO("\nrx err=%x,c=%d,ch=%x\n", intr_src, rx_fifocnt, ch);
        }
		else
		{
			rxlen = rx_fifocnt;
	        while (rx_fifocnt-- > 0)
	        {
	            ch = (u8) port->regs->UR_RXW;
	            recv->buf[recv->head] = ch;
	            recv->head = (recv->head + 1) & (TLS_UART_RX_BUF_SIZE - 1);
				if(port->rx_callback != NULL && rx_byte_cb_flag)
				{
					port->rx_callback(1, port->priv_data);
				}
	        }
		}

        if( escapefifocnt==3 && ch=='+')
        {
            switch(recv->head-1)
            {
                case 0:
                    if(recv->buf[TLS_UART_RX_BUF_SIZE-1]=='+' && recv->buf[TLS_UART_RX_BUF_SIZE-2]=='+')
                        port->plus_char_cnt = 3;
                    break;
                case 1:
                    if(recv->buf[0]=='+' && recv->buf[TLS_UART_RX_BUF_SIZE-1]=='+')
                        port->plus_char_cnt = 3;
                    break;               
                default:
                    if(recv->buf[recv->head-2]=='+' && recv->buf[recv->head-3]=='+')
                        port->plus_char_cnt = 3;
                    break;
            }
        }
        if (rxlen && port->rx_callback!=NULL && !rx_byte_cb_flag)
        {
            port->rx_callback(rxlen, port->priv_data);
        }
    }
    if (intr_src & UART_TX_INT_FLAG)
    {
        tls_uart_tx_chars(port);
    }
    if (intr_src & UIS_CTS_CHNG)
    {
        fifos = port->regs->UR_FIFOS;
        uart_handle_cts_change(port, fifos & UFS_CST_STS);
    }
    csi_kernel_intrpt_exit();	
}

static int findOutIntUart(void)
{
	int i;
	u32 regValue;
	
	for( i=TLS_UART_2; i< TLS_UART_MAX; i++ )
	{
		regValue = tls_reg_read32(HR_UART0_INT_SRC + i*STEP_SIZE);
		regValue &= 0x1FF;
		if( regValue )
			break;
	}
	return i;
}

ATTRIBUTE_ISR void UART2_4_IRQHandler(void)
{
	int intUartNum = findOutIntUart();
    struct tls_uart_port *port = &uart_port[intUartNum];
    struct tls_uart_circ_buf *recv = &port->recv;
    u8 rx_byte_cb_flag = uart_rx_byte_cb_flag[intUartNum];	
    u32 intr_src;
    u32 rx_fifocnt;
    u32 fifos;
    u8 escapefifocnt = 0;	
    u32 rxlen = 0;
    u8 ch = 0;
    csi_kernel_intrpt_enter();

    intr_src = port->regs->UR_INTS;

	if (intr_src & UIS_OVERRUN)
	{
		port->regs->UR_INTS |= UIS_OVERRUN;
		if(port->tx_dma_on)
		{
			tls_reg_write32((int)&port->regs->UR_DMAC, (tls_reg_read32((int)&port->regs->UR_DMAC) & ~0x01));
			tls_reg_write32((int)&port->regs->UR_DMAC, (tls_reg_read32((int)&port->regs->UR_DMAC) | 0x01));
		}
	}
    if ((intr_src & UART_RX_INT_FLAG) && (0 == (port->regs->UR_INTM & UIS_RX_FIFO)))
    {
        rx_fifocnt = (port->regs->UR_FIFOS >> 6) & 0x3F;
        escapefifocnt = rx_fifocnt;
        port->plus_char_cnt = 0;
        rxlen = rx_fifocnt;
        
        if (CIRC_SPACE(recv->head, recv->tail, TLS_UART_RX_BUF_SIZE) <= RX_CACHE_LIMIT)
        {
            recv->tail = (recv->tail + RX_CACHE_LIMIT) & (TLS_UART_RX_BUF_SIZE - 1);
        }
        
        while (rx_fifocnt-- > 0)
        {
            ch = (u8) port->regs->UR_RXW;
            if (intr_src & UART_RX_ERR_INT_FLAG)
            {
                port->regs->UR_INTS |= UART_RX_ERR_INT_FLAG;
                TLS_DBGPRT_INFO("\nrx err=%x,c=%d,ch=%x\n", intr_src, rx_fifocnt, ch);
                continue;
            }
            recv->buf[recv->head] = ch;
            recv->head = (recv->head + 1) & (TLS_UART_RX_BUF_SIZE - 1);
            if(port->rx_callback != NULL && rx_byte_cb_flag)
            {
                port->rx_callback(1, port->priv_data);
            }
        }

        if( escapefifocnt==3 && ch=='+')
        {
            switch(recv->head-1)
            {
                case 0:
                    if(recv->buf[TLS_UART_RX_BUF_SIZE-1]=='+' && recv->buf[TLS_UART_RX_BUF_SIZE-2]=='+')
                        port->plus_char_cnt = 3;
                    break;
                case 1:
                    if(recv->buf[0]=='+' && recv->buf[TLS_UART_RX_BUF_SIZE-1]=='+')
                        port->plus_char_cnt = 3;
                    break;               
                default:
                    if(recv->buf[recv->head-2]=='+' && recv->buf[recv->head-3]=='+')
                        port->plus_char_cnt = 3;
                    break;
            }
        }
        if (rxlen && port->rx_callback!=NULL && !rx_byte_cb_flag)
        {
            port->rx_callback(rxlen, port->priv_data);
        }
    }

    if (intr_src & UART_TX_INT_FLAG)
    {
        tls_uart_tx_chars(port);
    }

    if (intr_src & UIS_CTS_CHNG)
    {
        fifos = port->regs->UR_FIFOS;
        uart_handle_cts_change(port, fifos & UFS_CST_STS);
    }
    port->regs->UR_INTS = intr_src;
    csi_kernel_intrpt_exit();	
}

/**
 * @brief	This function is used to initial uart port.
 *
 * @param[in] uart_no: is the uart number.
 *	- \ref TLS_UART_0 TLS_UART_1 TLS_UART_2 TLS_UART_3 TLS_UART_4 TLS_UART5
 * @param[in] opts: is the uart setting options,if this param is NULL,this function will use the default options.
 * @param[in] modeChoose:; choose uart2 mode or 7816 mode when uart_no is TLS_UART_2, 0 for uart2 mode and 1 for 7816 mode.
 *
 * @retval
 *	- \ref WM_SUCCESS
 *	- \ref WM_FAILED
 *
 * @note When the system is initialized, the function has been called, so users can not call the function.
 */
int tls_uart_port_init(u16 uart_no, tls_uart_options_t * opts, u8 modeChoose)
{
    struct tls_uart_port *port;
    int ret;
    char *bufrx;                // ,*buftx
    tls_uart_options_t opt;
	if (TLS_UART_MAX <= uart_no)
	{
		return WM_FAILED;
	}

	switch( uart_no )
	{
		case TLS_UART_0:
		case TLS_UART_1:
		    tls_irq_disable((UART0_IRQn+uart_no));	
			break;
		case TLS_UART_2:
		case TLS_UART_3:
		case TLS_UART_4:
		case TLS_UART_5:
			tls_irq_disable(UART24_IRQn);
			break;
	}

    UartRegInit(uart_no);
	if (uart_port[uart_no].recv.buf)
	{
		tls_mem_free((void *)uart_port[uart_no].recv.buf);
		uart_port[uart_no].recv.buf = NULL;
	}
	memset(&uart_port[uart_no], 0, sizeof(struct tls_uart_port));
    port = &uart_port[uart_no];
    port->regs = (TLS_UART_REGS_T *)(HR_UART0_BASE_ADDR + uart_no*STEP_SIZE);
	if( uart_no==TLS_UART_2 )
	{ 
		(modeChoose == 1)?(port->regs->UR_LC |= (1 << 24)):(port->regs->UR_LC &= ~(0x1000000));
	}
    port->uart_no = uart_no;

    if (NULL == opts)
    {
        opt.baudrate = UART_BAUDRATE_B115200;
        opt.charlength = TLS_UART_CHSIZE_8BIT;
        opt.flow_ctrl = TLS_UART_FLOW_CTRL_NONE;
        opt.paritytype = TLS_UART_PMODE_DISABLED;
        opt.stopbits = TLS_UART_ONE_STOPBITS;
        ret = tls_uart_config(port, &opt);
    }
    else
    {
        ret = tls_uart_config(port, opts);
    }

    if (ret != WM_SUCCESS)
        return WM_FAILED;
    port->rxstatus = TLS_UART_RX_ENABLE;
	switch( uart_no )
	{
		case TLS_UART_0:
		case TLS_UART_1:
			port->uart_irq_no = (UART0_IRQn+uart_no);
			break;
		case TLS_UART_2:
		case TLS_UART_3:
		case TLS_UART_4:
		case TLS_UART_5:
			port->uart_irq_no = UART24_IRQn;
			break;
	}

    if (port->recv.buf == NULL)
	{
		bufrx = tls_mem_alloc(TLS_UART_RX_BUF_SIZE);
		if (!bufrx)
		    return WM_FAILED;
	    memset(bufrx, 0, TLS_UART_RX_BUF_SIZE);
		port->recv.buf = (u8 *) bufrx;	
    }
    port->recv.head = 0;
    port->recv.tail = 0;
    port->tx_fifofull = 16;
    dl_list_init(&port->tx_msg_pending_list);
    dl_list_init(&port->tx_msg_to_be_freed_list);
    tls_uart_tx_callback_register(uart_no, tls_uart_free_tx_sent_data);
  
    tls_irq_enable(port->uart_irq_no); /* enable uart interrupt */
    return WM_SUCCESS;
}

/**
 * @brief	This function is used to register uart rx interrupt.
 *
 * @param[in] uart_no: is the uart numer.
 * @param[in] callback: is the uart rx interrupt call back function.
 *
 * @retval
 *
 * @note This function should be called after the fucntion tls_uart_port_init() or it won't work.
 */
void tls_uart_rx_callback_register(u16 uart_no, s16(*rx_callback) (u16 len, void* user_data), void *priv_data)
{
	uart_port[uart_no].rx_callback = rx_callback;
	uart_port[uart_no].priv_data = priv_data;
}

void tls_uart_rx_byte_callback_flag(u16 uart_no, u8 flag)
{
	uart_rx_byte_cb_flag[uart_no] = flag;
}

/**
 * @brief	This function is used to register uart tx interrupt.
 *
 * @param[in] uart_no: is the uart numer.
 * @param[in] callback: is the uart tx interrupt call back function.
 *
 * @retval
 */
void tls_uart_tx_callback_register(u16 uart_no, s16(*tx_callback) (struct tls_uart_port *port))
{
	uart_port[uart_no].tx_callback = tx_callback;
}
void tls_uart_tx_sent_callback_register(u16 uart_no, s16(*tx_callback) (struct tls_uart_port *port))
{
	uart_port[uart_no].tx_sent_callback = tx_callback;
}


int tls_uart_try_read(u16 uart_no, int32_t read_size)
{
    int data_cnt = 0;
    struct tls_uart_port *port = NULL;
    struct tls_uart_circ_buf *recv;

	port = &uart_port[uart_no];
    recv = &port->recv;
    data_cnt = CIRC_CNT(recv->head, recv->tail, TLS_UART_RX_BUF_SIZE);
    if(data_cnt >= read_size)
    {
        return read_size;
    }else
    {
        return 0;
    }
}

/**
 * @brief	This function is used to copy circular buffer data to user buffer.
 * @param[in] uart_no: is the uart numer.
 * @param[in] buf: is the user buffer.
 * @param[in] readsize: is the user read size.
 * @retval
 */
int tls_uart_read(u16 uart_no, u8 * buf, u16 readsize)
{
    int data_cnt, buflen, bufcopylen;
    struct tls_uart_port *port = NULL;
    struct tls_uart_circ_buf *recv;

    if (NULL == buf || readsize < 1)
    {
        return WM_FAILED;
    }

    port = &uart_port[uart_no];
    recv = &port->recv;
    data_cnt = CIRC_CNT(recv->head, recv->tail, TLS_UART_RX_BUF_SIZE);
	(data_cnt >= readsize)?(buflen = readsize):(buflen = data_cnt);
    if ((recv->tail + buflen) > TLS_UART_RX_BUF_SIZE)
    {
        bufcopylen = (TLS_UART_RX_BUF_SIZE - recv->tail);
        MEMCPY(buf, (void *)(recv->buf + recv->tail), bufcopylen);
        MEMCPY(buf + bufcopylen, (void *)recv->buf, buflen - bufcopylen);
    }
    else
    {
        MEMCPY(buf, (void *)(recv->buf + recv->tail), buflen);
    }
    recv->tail = (recv->tail + buflen) & (TLS_UART_RX_BUF_SIZE - 1);
    return buflen;
}

/**
 * @brief          This function is used to transfer data throuth DMA.
 *
 * @param[in]      buf                is a buf for saving user data
 * @param[in]      writesize        is the user data length
 * @param[in]      cmpl_callback  function point,when the transfer is completed, the function will be called.
 *
 * @retval         WM_SUCCESS    success
 * @retval         WM_FAILED       failed
 *
 * @note           Only uart1 support DMA transfer.
 */
static void tls_uart_dma_write_complte_callback(void *parg)
{
    u32 dma_uart_ch = (u32)parg;    
    u16 uart_no = (dma_uart_ch&0x00FFFF00)>>8;
    u8 dma_ch = dma_uart_ch&0xFF;	
    struct tls_uart_port *port = &uart_port[uart_no];

    tls_dma_free(dma_ch);
    port->tx_dma_on = FALSE;

    if(port->tx_sent_callback)
    {
        port->tx_sent_callback((void*)(u32)uart_no);
    }
    
} 
int tls_uart_dma_write(char *buf, u16 writesize, void (*cmpl_callback) (void *p), u16 uart_no)
{
    unsigned char dmaCh = 0;
    struct tls_dma_descriptor DmaDesc;
    struct tls_uart_port *port = &uart_port[uart_no];

    if (NULL == buf || writesize < 1 || writesize >= 4096)
    {
        TLS_DBGPRT_ERR("param err\n");
        return WM_FAILED;
    }
    if (port->tx_dma_on)
    {
        TLS_DBGPRT_ERR("transmiting,wait\n");
        return WM_FAILED;
    }

    /* Request DMA Channel */
    dmaCh = tls_dma_request(0xFF, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_UART_TX) | TLS_DMA_FLAGS_HARD_MODE);
    if (dmaCh == 0xFF)
    {
        TLS_DBGPRT_ERR("dma request err\n");
        return WM_FAILED;
    }
	tls_reg_write32(HR_DMA_CHNL_SEL, uart_no);    
	tls_reg_write32((int)&port->regs->UR_DMAC, (tls_reg_read32((int)&port->regs->UR_DMAC) & ~0x01));
    
    port->tx_sent_callback = (s16(*) (struct tls_uart_port *))cmpl_callback;
    tls_dma_irq_register(dmaCh, tls_uart_dma_write_complte_callback, (void *)(u32)(dmaCh|uart_no<<8), TLS_DMA_IRQ_TRANSFER_DONE);

    /* Enable uart TX DMA */
    port->tx_dma_on = TRUE;
    tls_reg_write32((int)&port->regs->UR_DMAC, (tls_reg_read32((int)&port->regs->UR_DMAC) | 0x01));   
    DmaDesc.src_addr = (int) buf;
    DmaDesc.dest_addr = (int)&port->regs->UR_TXW;
	DmaDesc.dma_ctrl = TLS_DMA_DESC_CTRL_SRC_ADD_INC | TLS_DMA_DESC_CTRL_DATA_SIZE_BYTE | (writesize << 7);
    DmaDesc.valid = TLS_DMA_DESC_VALID;
    DmaDesc.next = NULL;
    tls_dma_start(dmaCh, &DmaDesc, 0);

    return WM_SUCCESS;
}

/**
 * @brief          This function is used to transfer data asynchronous.
 *
 * @param[in]      uart_no     is the uart number
 * @param[in]      buf     is a buf for saving user data.
 * @param[in]      writesize  is the data length
 *
 * @retval         WM_SUCCESS    success
 * @retval         WM_FAILED       failed
 *
 * @note           The function only start transmission, fill buffer in the callback function.
 */
int tls_uart_write_async(u16 uart_no, char *buf, u16 writesize)
{
    struct tls_uart_port *port = NULL;
    int ret;

    if (NULL == buf || writesize < 1)
    {
        TLS_DBGPRT_ERR("param err\n");
        return WM_FAILED;
    }

	port = &uart_port[uart_no];
    ret = tls_uart_fill_buf(port, buf, writesize);
    if (0 == ret)
    {
        tls_uart_tx_chars_start(port);
    }

    return ret;
}

/**
 * @brief          get the data length has been transmitted.
 *
 * @param[in]      uart_no     is the uart number
 *
 * @retval        the length has been transmitted 
 *
 */
int tls_uart_tx_length(u16 uart_no)
{
    struct tls_uart_port *port = &uart_port[uart_no];

    return port->icount.tx;
}

/**
 * @brief          This function is used to transfer data synchronous.
 *
 * @param[in]      uart_no     is the uart number
 * @param[in]      buf     is a buf for saving user data.
 * @param[in]      writesize  is the data length
 *
 * @retval         WM_SUCCESS    success
 * @retval         WM_FAILED       failed
 *
 * @note           The function only start transmission, fill buffer in the callback function.
 */
int tls_uart_write(u16 uart_no, char *buf, u16 writesize)
{
    return tls_uart_write_async(uart_no, buf, writesize);

}


/**
 * @brief          This function is used to set uart parity.
 *
 * @param[in]      paritytype     is a parity type defined in TLS_UART_PMODE_T
 *
 * @retval         WM_SUCCESS    success
 * @retval         WM_FAILED       failed
 *
 */
int tls_uart_set_parity(u16 uart_no, TLS_UART_PMODE_T paritytype)
{
	return tls_uart_set_parity_inside(&uart_port[uart_no], paritytype);
}

/**
 * @brief          This function is used to set uart baudrate.
 *
 * @param[in]      uart_no     is the uart number
 * @param[in]      baudrate     is the baudrate user want used,the unit is HZ.
 *
 * @retval         WM_SUCCESS    success
 * @retval         WM_FAILED       failed
 *
 */
int tls_uart_set_baud_rate(u16 uart_no, u32 baudrate)
{
	return tls_uart_set_baud_rate_inside(&uart_port[uart_no], baudrate);
}

/**
 * @brief          This function is used to set uart stop bits.
 *
 * @param[in]      uart_no     is the uart number
 * @param[in]      stopbits     is a stop bit type defined in TLS_UART_STOPBITS_T.
 *
 * @retval         WM_SUCCESS    success
 * @retval         WM_FAILED       failed
 *
 */
int tls_uart_set_stop_bits(u16 uart_no, TLS_UART_STOPBITS_T stopbits)
{
	return tls_uart_set_stop_bits_inside(&uart_port[uart_no], stopbits);
}

int tls_uart_dma_off(u16 uart_no)
{
	uart_port[uart_no].tx_dma_on = FALSE;
	return WM_SUCCESS;
}

#endif
//TLS_CONFIG_UART
