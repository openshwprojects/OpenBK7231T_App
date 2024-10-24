/*****************************************************************************
**
**  Name:           wm_uart_ble_if.c
**
**  Description:    This file contains the  implemention of uart_ble_passthrough
**
*****************************************************************************/
#include <assert.h>
#include <string.h>

#include "wm_bt_config.h"

#if (WM_NIMBLE_INCLUDED == CFG_ON)

#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "wm_bt_app.h"
#include "wm_ble_gap.h"
#include "wm_ble_uart_if.h"
#include "wm_ble_server_api_demo.h"
#include "wm_ble_client_api_demo.h"
#include "wm_bt_util.h"
#include "wm_mem.h"
#include "wm_include.h"
#include "wm_cpu.h"
#include "wm_bt_util.h"

#include "wm_gpio_afsel.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */


#define WM_UART_TAST_STK_SIZE	      512
#define WM_DISP_TAST_STK_SIZE	      1024

#define WM_UART_RX_BUF_SIZE	        512
#define WM_UART_TASK_PRIO           20

#define WM_MSG_OPEN_UART            0x00
#define WM_MSG_UART_RECEIVE_DATA    0x01
#define WM_MSG_AVAILABLE            0x02
#define WM_UART_DEFAULT_THRESH      244

#define WM_UART_RECV_QUEUE_SIZE     12
static OS_STK   wm_uart_task_stk[WM_UART_TAST_STK_SIZE];
static OS_STK   wm_disp_task_stk[WM_DISP_TAST_STK_SIZE];
static struct ble_gap_event_listener ble_uart_event_listener;


/**
 * @typedef struct WM_UART
 */
typedef struct WM_UART
{
    tls_os_task_t *uart_recv_task;
    tls_os_task_t *uart_disp_task;
    tls_os_queue_t *wm_uart_queue_recv;       //uart iterface recv queue;
    tls_os_queue_t *wm_queue_msg_available;   //uart msg to mesh;
    int8_t uart_idx;
    uint16_t thresh;                            //
    uint8_t *wm_buffer;
    int bandrate;
    TLS_UART_PMODE_T parity;
    TLS_UART_STOPBITS_T stopbits;
    char *rx_buf;
    int rx_msg_num;
    int rx_data_len;
    ringbuffer_t *wm_msg_buffer_ptr;  //
    int (*dispatch_func)(uint8_t *payload, int length);
} WM_UART_ST;

static WM_UART_ST *wm_uart = NULL;


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 * Send payload to uart, the payload is from gatt notfification or gatt write.
 *
 *
 */

void wm_uart_send_notify(tls_uart_msg_out_t type, uint8_t *msg, int len)
{
    if(wm_uart == NULL) return;
    if(type != UART_OUTPUT_DATA) return;

    tls_uart_write_async(wm_uart->uart_idx, (char *)msg, (uint16_t)len);
}


/**
 * Payload sent out, and Continue to check if anything can be sent out
 * 
 * On Server: gatt notification tx E_DONE;
 * On Client: Write to remote already;
 *
 */

void wm_uart_sent_cb(tls_ble_uart_mode_t mode, int status)
{
    int ret;
    int rx_len;
    
    TLS_BT_APPL_TRACE_DEBUG("UART Sent CBACK:%d, status=%d\r\n", mode, status);
    
    if(wm_uart->thresh)
    {
        rx_len = min(wm_uart->thresh, bt_ringbuffer_size(wm_uart->wm_msg_buffer_ptr));
        if(rx_len < wm_uart->thresh)
        {
            rx_len = 0; //less than thresh 
        }
    }else
    {
        rx_len = bt_ringbuffer_size(wm_uart->wm_msg_buffer_ptr);
        rx_len = min(WM_UART_DEFAULT_THRESH, rx_len);
    }

    if(rx_len >0)
    {
        bt_ringbuffer_peek(wm_uart->wm_msg_buffer_ptr, 0, wm_uart->wm_buffer, rx_len);
    
        ret = wm_uart->dispatch_func(wm_uart->wm_buffer, rx_len);

        if(ret == 0)
        {
            bt_ringbuffer_delete(wm_uart->wm_msg_buffer_ptr, rx_len);
        }else
        {
            TLS_BT_APPL_TRACE_WARNING("UART[%d] Send ret:%d\r\n", mode, ret);
        }
    }    
}


static s16 wm_uart_rx(u16 len)
{
    if (NULL == wm_uart)
    {
        return WM_FAILED;
    }

    wm_uart->rx_data_len += len;
    if (wm_uart->rx_msg_num < 3)
    {
        wm_uart->rx_msg_num++;
        tls_os_queue_send(wm_uart->wm_uart_queue_recv, (void *) WM_MSG_UART_RECEIVE_DATA, 0);
    }

    return WM_SUCCESS;
}

static void wm_dispatch_task(void *sdata)
{
    WM_UART_ST *uart = (WM_UART_ST *) sdata;
    void *msg;
    int ret = 0;
    int rx_len = 0;
    
    for (;;)
    {
        tls_os_queue_receive(uart->wm_queue_msg_available, (void **) &msg, 0, 0);
        switch ((u32) msg)
        {

            case WM_MSG_AVAILABLE:
            {
                if(uart->thresh)
                {
                    rx_len = min(uart->thresh, bt_ringbuffer_size(uart->wm_msg_buffer_ptr));
                    if(rx_len < uart->thresh)
                    {
                        rx_len = 0; //less than thresh 
                    }
                }else
                {
                    rx_len = bt_ringbuffer_size(uart->wm_msg_buffer_ptr);
                    rx_len = min(WM_UART_DEFAULT_THRESH, rx_len);
                }

                if(rx_len >0)
                {
                    bt_ringbuffer_peek(uart->wm_msg_buffer_ptr, 0, uart->wm_buffer, rx_len);
                
                    ret = uart->dispatch_func(uart->wm_buffer, rx_len);

                    if(ret == 0)
                    {
                        bt_ringbuffer_delete(uart->wm_msg_buffer_ptr, rx_len);
                    }
                }
            }
            break;

        default:
            break;
        }
    }
}


static void wm_uart_task(void *sdata)
{
    WM_UART_ST *uart = (WM_UART_ST *) sdata;
    tls_uart_options_t opt;
    void *msg;
    int ret = 0;
    int len = 0;
    int rx_len = 0;

    for (;;)
    {
        tls_os_queue_receive(uart->wm_uart_queue_recv, (void **) &msg, 0, 0);
        switch ((u32) msg)
        {
        case WM_MSG_OPEN_UART:
        {
            opt.baudrate = uart->bandrate;
            opt.paritytype = uart->parity;
            opt.stopbits = uart->stopbits;
            opt.charlength = TLS_UART_CHSIZE_8BIT;
            opt.flow_ctrl = TLS_UART_FLOW_CTRL_NONE;

            //选择待使用的引脚及具体的复用功能
            /* UART1_RX-PB07  UART1_TX-PB06 */
            if(uart->uart_idx == 1)
            {
                wm_uart1_rx_config(WM_IO_PB_07);
                wm_uart1_tx_config(WM_IO_PB_06);
            }

            if (WM_SUCCESS != tls_uart_port_init(uart->uart_idx, &opt, 0))
            {
                TLS_BT_APPL_TRACE_ERROR("uart%d init error\r\n", uart->uart_idx);
            }

            tls_uart_rx_callback_register((u16) uart->uart_idx, (s16(*)(u16, void*))wm_uart_rx, NULL);			
        }
        break;

        case WM_MSG_UART_RECEIVE_DATA:
        {
            rx_len = uart->rx_data_len;
            while (rx_len > 0)
            {
                len = (rx_len > WM_UART_RX_BUF_SIZE) ? WM_UART_RX_BUF_SIZE : rx_len;
                memset(uart->rx_buf, 0, (WM_UART_RX_BUF_SIZE + 1));
                ret = tls_uart_read(uart->uart_idx, (u8 *) uart->rx_buf, len);  /* input */
                if (ret <= 0)
                {
                    break;
                }

                rx_len -= ret;
                uart->rx_data_len -= ret;

                len = bt_ringbuffer_available(wm_uart->wm_msg_buffer_ptr);
                if(ret > len)
                {
                    TLS_BT_APPL_TRACE_WARNING("Warning ,mesh cache buffer full...\r\n");
                }
                bt_ringbuffer_insert(wm_uart->wm_msg_buffer_ptr, (uint8_t *) uart->rx_buf, ret);
                
                tls_os_queue_send(wm_uart->wm_queue_msg_available, (void *) WM_MSG_AVAILABLE, 0);
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



static int ble_uart_init(int8_t uart_idx, int bandrate, int parity, int stopbits, uint16_t thresh, int (*dispatch_func)(uint8_t *payload, int length))
{
    tls_os_status_t status;
    
    TLS_BT_APPL_TRACE_DEBUG("uart demo param=%d,%d, %d, %d, %d\r\n", uart_idx, bandrate, parity, stopbits, thresh);
    if (NULL == wm_uart)
    {
        wm_uart = tls_mem_alloc(sizeof(WM_UART_ST));
        if (NULL == wm_uart)
        {
            goto _error;
        }
        memset(wm_uart, 0, sizeof(WM_UART_ST));

        wm_uart->rx_buf = tls_mem_alloc(WM_UART_RX_BUF_SIZE + 1);
        if (NULL == wm_uart->rx_buf)
        {
            goto _error1;
        }
        wm_uart->thresh = thresh;

        if(0 == thresh)
        {
            thresh = WM_UART_DEFAULT_THRESH;   //default thresh buffer size;
        }
        wm_uart->wm_buffer = (uint8_t *)tls_mem_alloc(thresh);
        if(!wm_uart->wm_buffer){
            goto _error1;
        }

        wm_uart->wm_msg_buffer_ptr = bt_ringbuffer_init(2048);
        if (NULL == wm_uart->wm_msg_buffer_ptr)
        {
            goto _error1;
        }        
        
        status = tls_os_queue_create(&(wm_uart->wm_uart_queue_recv), 6);
        assert(status == 0);
        status = tls_os_task_create(wm_uart->uart_recv_task, "buc",
                           wm_uart_task,
                           (void *) wm_uart,
                           (void *) wm_uart_task_stk, /** 任务栈的起始地址 */
                           WM_UART_TAST_STK_SIZE*4,                         /** 任务栈的大小     */
                           WM_UART_TASK_PRIO, 0);
        assert(status == 0);
        status = tls_os_queue_create(&(wm_uart->wm_queue_msg_available), 6);
        assert(status == 0);
        status = tls_os_task_create(wm_uart->uart_disp_task, "bud",
                   wm_dispatch_task,
                   (void *) wm_uart,
                   (void *) wm_disp_task_stk, /** 任务栈的起始地址 */
                   WM_DISP_TAST_STK_SIZE*4,                         /** 任务栈的大小     */
                   WM_UART_TASK_PRIO+1, 0);
        assert(status == 0);
    }
    if (0 == bandrate)
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
    if(-1 == uart_idx)
    {
        uart_idx = TLS_UART_1;
    }

    wm_uart->dispatch_func = dispatch_func;
    wm_uart->uart_idx = uart_idx;
    wm_uart->bandrate = bandrate;
    wm_uart->parity = (TLS_UART_PMODE_T) parity;
    wm_uart->stopbits = (TLS_UART_STOPBITS_T) stopbits;
    wm_uart->rx_msg_num = 0;
    wm_uart->rx_data_len = 0;
    tls_os_queue_send(wm_uart->wm_uart_queue_recv, (void *) WM_MSG_OPEN_UART, 0);

    return WM_SUCCESS;

_error1:
    if(wm_uart)
    {
        if(wm_uart->rx_buf)
        {
            tls_mem_free(wm_uart->rx_buf);
            wm_uart->rx_buf = NULL;
        }
        if(wm_uart->wm_buffer)
        {
            tls_mem_free(wm_uart->wm_buffer);
            wm_uart->wm_buffer = NULL;
        }
    }
    tls_mem_free(wm_uart);
    wm_uart = NULL;
_error:
    TLS_BT_APPL_TRACE_ERROR("ble_uart_init failed\n");
    return WM_FAILED;
}

static int ble_gap_evt_cb(struct ble_gap_event *event, void *arg)
{
    TLS_BT_APPL_TRACE_DEBUG("%s\r\n", __FUNCTION__);
	if(event->type == BLE_GAP_EVENT_HOST_SHUTDOWN)
	{
        if(wm_uart)
        {
            if(wm_uart->rx_buf)
            {
                tls_mem_free(wm_uart->rx_buf);
                wm_uart->rx_buf = NULL;
            }
            if(wm_uart->wm_buffer)
            {
                tls_mem_free(wm_uart->wm_buffer);
                wm_uart->wm_buffer = NULL;
            }
            if(wm_uart->wm_msg_buffer_ptr)
            {
                bt_ringbuffer_free(wm_uart->wm_msg_buffer_ptr);
                wm_uart->wm_msg_buffer_ptr = NULL;
            }
            if(wm_uart->uart_recv_task)
            {
                tls_os_task_del_by_task_handle(wm_uart->uart_recv_task, NULL);
                wm_uart->uart_recv_task = NULL;
            }
            if(wm_uart->uart_disp_task)
            {
                tls_os_task_del_by_task_handle(wm_uart->uart_disp_task, NULL);
                wm_uart->uart_disp_task = NULL;
            }
            tls_os_queue_delete(wm_uart->wm_uart_queue_recv);
            tls_os_queue_delete(wm_uart->wm_queue_msg_available);
            tls_uart_rx_callback_register((u16) wm_uart->uart_idx, NULL, NULL);
            tls_mem_free(wm_uart);
            wm_uart = NULL;

        } 
	}

    return 0;
}

/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

int tls_ble_uart_init(tls_ble_uart_mode_t mode,uint8_t uart_idx, tls_uart_options_t *p_hci_if)
{
    int rc,rc1;

    CHECK_SYSTEM_READY();
    
    if(mode == BLE_UART_SERVER_MODE)
    {
        rc = tls_ble_server_demo_api_init(wm_uart_send_notify, wm_uart_sent_cb);
        if(!p_hci_if)
        {
            rc1 = ble_uart_init(uart_idx, 0, 0, 0, 244, tls_ble_server_demo_api_send_msg);
        }else
        {
            rc1 = ble_uart_init(uart_idx, p_hci_if->baudrate, p_hci_if->paritytype, p_hci_if->stopbits, 244, tls_ble_server_demo_api_send_msg);
        }
    }else if(mode == BLE_UART_CLIENT_MODE)
    {
        rc = tls_ble_client_demo_api_init(wm_uart_send_notify,wm_uart_sent_cb);
        if(!p_hci_if)
        {
            rc1 = ble_uart_init(uart_idx, 0, 0, 0, 244, tls_ble_client_demo_api_send_msg);
        }else
        {
            rc1 = ble_uart_init(uart_idx, p_hci_if->baudrate, p_hci_if->paritytype, p_hci_if->stopbits, 244, tls_ble_server_demo_api_send_msg);
        }
    }else
    {
        return BLE_HS_EINVAL;
    }

    if(rc != 0 || rc1 != 0)
    {
        
    }
    
    ble_gap_event_listener_register(&ble_uart_event_listener,
                                ble_gap_evt_cb, NULL);

    return rc+rc1;
}



int tls_ble_uart_deinit(tls_ble_uart_mode_t mode, uint8_t uart_id)
{
    int rc = 0;
    
    CHECK_SYSTEM_READY();

    if(mode == BLE_UART_SERVER_MODE) {
        rc = tls_ble_server_demo_api_deinit();
    } else if(mode == BLE_UART_CLIENT_MODE) {
        rc = tls_ble_client_demo_api_deinit();
    }

    if(wm_uart)
    {
        if(wm_uart->rx_buf)
        {
            tls_mem_free(wm_uart->rx_buf);
            wm_uart->rx_buf = NULL;
        }
        if(wm_uart->wm_buffer)
        {
            tls_mem_free(wm_uart->wm_buffer);
            wm_uart->wm_buffer = NULL;
        }
        if(wm_uart->wm_msg_buffer_ptr)
        {
            bt_ringbuffer_free(wm_uart->wm_msg_buffer_ptr);
            wm_uart->wm_msg_buffer_ptr = NULL;
        }
        if(wm_uart->uart_recv_task)
        {
            tls_os_task_del_by_task_handle(wm_uart->uart_recv_task, NULL);
            wm_uart->uart_recv_task = NULL;
        }
        if(wm_uart->uart_disp_task)
        {
            tls_os_task_del_by_task_handle(wm_uart->uart_disp_task, NULL);
            wm_uart->uart_disp_task = NULL;
        }
        tls_os_queue_delete(wm_uart->wm_uart_queue_recv);
        tls_os_queue_delete(wm_uart->wm_queue_msg_available);
        tls_uart_rx_callback_register((u16) wm_uart->uart_idx, NULL, NULL);

        tls_mem_free(wm_uart);
        wm_uart = NULL;

    }
 
    ble_gap_event_listener_unregister(&ble_uart_event_listener);

    return rc;
}


#endif
