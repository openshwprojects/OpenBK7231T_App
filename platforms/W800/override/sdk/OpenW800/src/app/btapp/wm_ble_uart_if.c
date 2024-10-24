/*****************************************************************************
**
**  Name:           wm_uart_ble_if.c
**
**  Description:    This file contains the  implemention of uart_ble_passthrough
**
*****************************************************************************/
#include <assert.h>

#include "wm_bt_config.h"

#if (WM_BLE_INCLUDED == CFG_ON)

#include "wm_ble_uart_if.h"
#include "wm_ble_server_api_demo.h"
#include "wm_ble_client_api_demo.h"
#include "wm_bt_util.h"
#include "wm_mem.h"




static ringbuffer_t *rb_ptr_t = NULL;
static uint8_t g_uart_id = -1;
#define RING_BUFFER_SIZE (4096)
static tls_ble_uart_mode_t g_bum = BLE_UART_SERVER_MODE;


static void wm_uart_async_write(uint8_t *p_data, uint32_t length)
{
    //TLS_BT_APPL_TRACE_API("%s , send to uart %d bytes\r\n", __FUNCTION__, length);
    tls_uart_write_async(g_uart_id, (char *)p_data, (uint16_t)length);
}
static int16_t wm_uart_async_read_cb(uint16_t size, void *user_data)
{
    int read_out = 0;
    uint32_t cache_length = 0;
    uint32_t mtu = 0;
    tls_bt_status_t bt_status;

    if(size <= 0) { return; }

    if(bt_ringbuffer_available(rb_ptr_t) < size) {
        TLS_BT_APPL_TRACE_WARNING("uart_ble_cache_buffer is full\r\n");
        return;
    }

    uint8_t *tmp_ptr = tls_mem_alloc(size);
    cache_length = bt_ringbuffer_size(rb_ptr_t);
    read_out = tls_uart_read(g_uart_id, tmp_ptr, size);

    //TLS_BT_APPL_TRACE_DEBUG("%s , need_read(%d),read_out(%d),cache_length(%d)\r\n", __FUNCTION__, size, read_out, cache_length);
    //if no cache data, send directly; otherwise append to cache buffer
    if(cache_length == 0) {
        mtu = tls_ble_server_demo_api_get_mtu();
        cache_length = MIN(mtu, read_out);

        if(cache_length) {
            /*send out*/
            //TLS_BT_APPL_TRACE_DEBUG("send out %d bytes\r\n", cache_length);
            if(g_bum == BLE_UART_SERVER_MODE) {
                bt_status = tls_ble_server_demo_api_send_msg(tmp_ptr, cache_length);
            } else {
                bt_status = tls_ble_client_demo_api_send_msg(tmp_ptr, cache_length);
            }

            if(bt_status == TLS_BT_STATUS_BUSY) {
                bt_ringbuffer_insert(rb_ptr_t, tmp_ptr, cache_length);
            }

            /*append the left to ringbuffer*/
            if(cache_length < read_out) {
                //TLS_BT_APPL_TRACE_DEBUG("insert %d bytes\r\n", read_out-cache_length);
                bt_ringbuffer_insert(rb_ptr_t, tmp_ptr + cache_length, read_out - cache_length);
            }
        }
    } else {
        //TLS_BT_APPL_TRACE_DEBUG("total insert %d bytes\r\n", read_out);
        bt_ringbuffer_insert(rb_ptr_t, tmp_ptr, read_out);
    }

    tls_mem_free(tmp_ptr);
}

int tls_ble_uart_init(tls_ble_uart_mode_t mode, uint8_t uart_id, tls_uart_options_t *p_hci_if)
{
    int status;
    TLS_BT_APPL_TRACE_API("%s , uart_id=%d\r\n", __FUNCTION__, uart_id);

    if(rb_ptr_t) { return TLS_BT_STATUS_DONE; }

    g_uart_id = uart_id;
    g_bum = mode;

    if(mode == BLE_UART_SERVER_MODE) {
        status = tls_ble_server_demo_api_init(wm_uart_async_write);
    } else if(mode == BLE_UART_CLIENT_MODE) {
        status = tls_ble_client_demo_api_init(wm_uart_async_write);
    } else {
        return TLS_BT_STATUS_UNSUPPORTED;
    }

    if(status != TLS_BT_STATUS_SUCCESS) {
        return status;
    }

    rb_ptr_t = bt_ringbuffer_init(RING_BUFFER_SIZE);

    if(rb_ptr_t == NULL) {
        if(mode == BLE_UART_SERVER_MODE) {
            tls_ble_server_demo_api_deinit();
        } else if(mode == BLE_UART_CLIENT_MODE) {
            tls_ble_client_demo_api_deinit();
        }

        return TLS_BT_STATUS_NOMEM;
    }

    status = tls_uart_port_init(uart_id, NULL, 0);

    if(status != WM_SUCCESS) {
        if(mode == BLE_UART_SERVER_MODE) {
            tls_ble_server_demo_api_deinit();
        } else if(mode == BLE_UART_CLIENT_MODE) {
            tls_ble_client_demo_api_deinit();
        }

        bt_ringbuffer_free(rb_ptr_t);
        rb_ptr_t = NULL;
        return TLS_BT_STATUS_FAIL;
    }

    tls_uart_rx_callback_register(uart_id, wm_uart_async_read_cb, (void *)NULL);
    return TLS_BT_STATUS_SUCCESS;
}

int tls_ble_uart_deinit(tls_ble_uart_mode_t mode, uint8_t uart_id)
{
    if(rb_ptr_t == NULL)
    { return TLS_BT_STATUS_DONE; }

    if(rb_ptr_t) {
        bt_ringbuffer_free(rb_ptr_t);
    }

    rb_ptr_t = NULL;

    //TODO deinit uart interface???

    if(mode == BLE_UART_SERVER_MODE) {
        tls_ble_server_demo_api_deinit();
    } else if(mode == BLE_UART_CLIENT_MODE) {
        tls_ble_client_demo_api_deinit();
    }

    return TLS_BT_STATUS_SUCCESS;
}
uint32_t tls_ble_uart_buffer_size()
{
    return bt_ringbuffer_size(rb_ptr_t);
}
uint32_t tls_ble_uart_buffer_available()
{
    return bt_ringbuffer_available(rb_ptr_t);
}

uint32_t tls_ble_uart_buffer_read(uint8_t *ptr, uint32_t length)
{
    return bt_ringbuffer_pop(rb_ptr_t, ptr, length);
}
uint32_t tls_ble_uart_buffer_peek(uint8_t *ptr, uint32_t length)
{
    return bt_ringbuffer_peek(rb_ptr_t, 0, ptr, length);
}
uint32_t tls_ble_uart_buffer_delete(uint32_t length)
{
    return bt_ringbuffer_delete(rb_ptr_t, length);
}

#endif
