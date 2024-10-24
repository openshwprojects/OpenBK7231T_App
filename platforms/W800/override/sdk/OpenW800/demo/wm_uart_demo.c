/**
 * @file    wm_uart_demo.c
 *
 * @brief   uart demo function
 *
 * @author  dave
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */

#include <string.h>
#include "wm_include.h"
#include "wm_http_fwup.h"
#include "wm_sockets.h"
#include "wm_cpu.h"
#include "wm_demo.h"

#if DEMO_UARTx
#include "wm_gpio_afsel.h"

#define USE_DMA_TX_FTR  0
#if USE_DMA_TX_FTR
#include "wm_dma.h"
extern int tls_uart_dma_off(u16 uart_no);
#endif

#define DEMO_UART_TAST_STK_SIZE	1024
#define DEMO_UART_RX_BUF_SIZE	1024

static OS_STK   demo_uart_task_stk[DEMO_UART_TAST_STK_SIZE];



/**
 * @typedef struct DEMO_UART
 */
typedef struct DEMO_UART
{
    tls_os_queue_t *demo_uart_q;
    //    OS_STK *demo_uart_task_stk;
    int bandrate;
    TLS_UART_PMODE_T parity;
    TLS_UART_STOPBITS_T stopbits;
    char *rx_buf;
    int rx_msg_num;
    int rx_data_len;
} DEMO_UART_ST;

static DEMO_UART_ST *demo_uart = NULL;

#if USE_DMA_TX_FTR
static void uart_dma_done(void *uart_no)
{
    u16 ch = *((u16 *)uart_no);
    tls_uart_dma_off(ch);
}
#endif
static s16 demo_uart_rx(u16 len)
{
    if (NULL == demo_uart)
    {
        return WM_FAILED;
    }

    demo_uart->rx_data_len += len;
    if (demo_uart->rx_msg_num < 3)
    {
        demo_uart->rx_msg_num++;
        tls_os_queue_send(demo_uart->demo_uart_q, (void *) DEMO_MSG_UART_RECEIVE_DATA, 0);
    }

    return WM_SUCCESS;
}



static void demo_uart_task(void *sdata)
{
    DEMO_UART_ST *uart = (DEMO_UART_ST *) sdata;
    tls_uart_options_t opt;
    void *msg;
    int ret = 0;
    int len = 0;
    int rx_len = 0;

    for (;;)
    {
        tls_os_queue_receive(uart->demo_uart_q, (void **) &msg, 0, 0);
        switch ((u32) msg)
        {
        case DEMO_MSG_OPEN_UART:
        {
            opt.baudrate = uart->bandrate;
            opt.paritytype = uart->parity;
            opt.stopbits = uart->stopbits;
            opt.charlength = TLS_UART_CHSIZE_8BIT;
            opt.flow_ctrl = TLS_UART_FLOW_CTRL_NONE;

            //选择待使用的引脚及具体的复用功能
            /* UART1_RX-PB07  UART1_TX-PB06 */
            wm_uart1_rx_config(WM_IO_PB_07);
            wm_uart1_tx_config(WM_IO_PB_06);

            if (WM_SUCCESS != tls_uart_port_init(TLS_UART_1, &opt, 0))
            {
                printf("uart1 init error\n");
            }

            tls_uart_rx_callback_register((u16) TLS_UART_1, (s16(*)(u16, void*))demo_uart_rx, NULL);			
        }
        break;

        case DEMO_MSG_UART_RECEIVE_DATA:
        {
            rx_len = uart->rx_data_len;
            while (rx_len > 0)
            {
                len = (rx_len > DEMO_UART_RX_BUF_SIZE) ? DEMO_UART_RX_BUF_SIZE : rx_len;
                memset(uart->rx_buf, 0, (DEMO_UART_RX_BUF_SIZE + 1));
                ret = tls_uart_read(TLS_UART_1, (u8 *) uart->rx_buf, len);  /* input */
                if (ret <= 0)
                {
                    break;
                }

                rx_len -= ret;
                uart->rx_data_len -= ret;

#if USE_DMA_TX_FTR
                tls_uart_dma_write(uart->rx_buf, len, uart_dma_done, TLS_UART_1);
#else
                tls_uart_write(TLS_UART_1, uart->rx_buf, ret);  /* output */
#endif
            }
            if (uart->rx_msg_num > 0)
            {
                uart->rx_msg_num--;
            }
        }
        break;

        default:
            break;
        }
    }
}



int uart_demo(int bandrate, int parity, int stopbits)
{
    printf("\nuart demo param=%d, %d, %d\n", bandrate, parity, stopbits);
    if (NULL == demo_uart)
    {
        demo_uart = tls_mem_alloc(sizeof(DEMO_UART_ST));
        if (NULL == demo_uart)
        {
            goto _error;
        }
        memset(demo_uart, 0, sizeof(DEMO_UART_ST));

        demo_uart->rx_buf = tls_mem_alloc(DEMO_UART_RX_BUF_SIZE + 1);
        if (NULL == demo_uart->rx_buf)
        {
            goto _error1;
        }
        tls_os_queue_create(&(demo_uart->demo_uart_q), DEMO_QUEUE_SIZE);
        tls_os_task_create(NULL, NULL,
                           demo_uart_task,
                           (void *) demo_uart,
                           (void *) demo_uart_task_stk, /** 任务栈的起始地址 */
                           DEMO_UART_TAST_STK_SIZE,                         /** 任务栈的大小     */
                           DEMO_UART_TASK_PRIO, 0);
    }
    if (-1 == bandrate)
    {
        bandrate = 115200;
    }
    if (-1 == parity)
    {
        parity = TLS_UART_PMODE_DISABLED;
    }
    if (-1 == stopbits)
    {
        stopbits = TLS_UART_ONE_STOPBITS;
    }
    demo_uart->bandrate = bandrate;
    demo_uart->parity = (TLS_UART_PMODE_T) parity;
    demo_uart->stopbits = (TLS_UART_STOPBITS_T) stopbits;
    demo_uart->rx_msg_num = 0;
    demo_uart->rx_data_len = 0;
    tls_os_queue_send(demo_uart->demo_uart_q, (void *) DEMO_MSG_OPEN_UART, 0);

    return WM_SUCCESS;

_error1:
    tls_mem_free(demo_uart);
    demo_uart = NULL;
_error:
    printf("\nmem error\n");
    return WM_FAILED;
}


#endif
