/*****************************************************************************
**
**  Name:           wm_bt_util.c
**
**  Description:    This file contains the ulils for applicaiton
**
*****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "wm_bt_config.h"

#if (WM_NIMBLE_INCLUDED == CFG_ON)

#include "wm_dbg.h"
#include "wm_mem.h"
#include "list.h"
#include "host/ble_hs.h"
#include "wm_bt_util.h"

typedef struct {
    struct dl_list list;
    struct ble_npl_callout co;
    struct ble_npl_mutex list_mutex;
    void *priv_data;
} async_evt_t;

typedef struct {
    async_evt_t *evt;
    app_async_func_t *func;
    void *arg;
} ble_async_t;


static async_evt_t  async_evt_list;

tls_bt_log_level_t tls_appl_trace_level =  TLS_BT_LOG_NONE;


void tls_bt_log(uint32_t level, const char *fmt_str, ...)
{
    u32 time = tls_os_get_time();
    u32 hour, min, second, ms = 0;
    second = time / HZ;
    ms = (time % HZ) * 2;
    hour = second / 3600;
    min = (second % 3600) / 60;
    second = (second % 3600) % 60;

    if(level == TLS_TRACE_TYPE_ERROR) {
        printf("[WM_E] <%d:%02d:%02d.%03d> ", hour, min, second, ms);
    } else if(level == TLS_TRACE_TYPE_WARNING) {
        printf("[WM_W] <%d:%02d:%02d.%03d> ", hour, min, second, ms);
    } else {
        printf("[WM_I] <%d:%02d:%02d.%03d> ", hour, min, second, ms);
    }

    if(1) {
        va_list args;
        /* printf args */
        va_start(args, fmt_str);
        vprintf(fmt_str, args);
        va_end(args);
    } else {
        return;
    }
}

#ifndef CASE_RETURN_STR
#define CASE_RETURN_STR(const) case const: return #const;
#endif

const char *tls_bt_gap_evt_2_str(uint32_t event)
{
    switch(event) {
            CASE_RETURN_STR(BLE_GAP_EVENT_CONNECT)
            CASE_RETURN_STR(BLE_GAP_EVENT_DISCONNECT)
            CASE_RETURN_STR(BLE_GAP_EVENT_CONN_UPDATE)
            CASE_RETURN_STR(BLE_GAP_EVENT_CONN_UPDATE_REQ)
            CASE_RETURN_STR(BLE_GAP_EVENT_L2CAP_UPDATE_REQ)
            CASE_RETURN_STR(BLE_GAP_EVENT_TERM_FAILURE)
            CASE_RETURN_STR(BLE_GAP_EVENT_DISC)
            CASE_RETURN_STR(BLE_GAP_EVENT_DISC_COMPLETE)
            CASE_RETURN_STR(BLE_GAP_EVENT_ADV_COMPLETE)
            CASE_RETURN_STR(BLE_GAP_EVENT_ENC_CHANGE)
            CASE_RETURN_STR(BLE_GAP_EVENT_PASSKEY_ACTION)
            CASE_RETURN_STR(BLE_GAP_EVENT_NOTIFY_RX)
            CASE_RETURN_STR(BLE_GAP_EVENT_NOTIFY_TX)
            CASE_RETURN_STR(BLE_GAP_EVENT_SUBSCRIBE)
            CASE_RETURN_STR(BLE_GAP_EVENT_MTU)
            CASE_RETURN_STR(BLE_GAP_EVENT_IDENTITY_RESOLVED)
            CASE_RETURN_STR(BLE_GAP_EVENT_REPEAT_PAIRING)
            CASE_RETURN_STR(BLE_GAP_EVENT_PHY_UPDATE_COMPLETE)
            CASE_RETURN_STR(BLE_GAP_EVENT_EXT_DISC)
            CASE_RETURN_STR(BLE_GAP_EVENT_PERIODIC_SYNC)
            CASE_RETURN_STR(BLE_GAP_EVENT_PERIODIC_REPORT)
            CASE_RETURN_STR(BLE_GAP_EVENT_PERIODIC_SYNC_LOST)
            CASE_RETURN_STR(BLE_GAP_EVENT_SCAN_REQ_RCVD)
            CASE_RETURN_STR(BLE_GAP_EVENT_PERIODIC_TRANSFER)

        default:
            return "unkown bt host evt";
    }
}


#define BLE_HS_ENOERR 0

const char *tls_bt_rc_2_str(uint32_t event)
{
    switch(event) {
            CASE_RETURN_STR(BLE_HS_ENOERR)
            CASE_RETURN_STR(BLE_HS_EAGAIN)
            CASE_RETURN_STR(BLE_HS_EALREADY)
            CASE_RETURN_STR(BLE_HS_EINVAL)
            CASE_RETURN_STR(BLE_HS_EMSGSIZE)
            CASE_RETURN_STR(BLE_HS_ENOENT)
            CASE_RETURN_STR(BLE_HS_ENOMEM)
            CASE_RETURN_STR(BLE_HS_ENOTCONN)
            CASE_RETURN_STR(BLE_HS_ENOTSUP)
            CASE_RETURN_STR(BLE_HS_EAPP)
            CASE_RETURN_STR(BLE_HS_EBADDATA)
            CASE_RETURN_STR(BLE_HS_EOS)
            CASE_RETURN_STR(BLE_HS_ECONTROLLER)
            CASE_RETURN_STR(BLE_HS_ETIMEOUT)
            CASE_RETURN_STR(BLE_HS_EDONE)
            CASE_RETURN_STR(BLE_HS_EBUSY)
            CASE_RETURN_STR(BLE_HS_EREJECT)
            CASE_RETURN_STR(BLE_HS_EUNKNOWN)
            CASE_RETURN_STR(BLE_HS_EROLE)
            CASE_RETURN_STR(BLE_HS_ETIMEOUT_HCI)
            CASE_RETURN_STR(BLE_HS_ENOMEM_EVT)
            CASE_RETURN_STR(BLE_HS_ENOADDR)
            CASE_RETURN_STR(BLE_HS_ENOTSYNCED)
            CASE_RETURN_STR(BLE_HS_EAUTHEN)
            CASE_RETURN_STR(BLE_HS_EAUTHOR)
            CASE_RETURN_STR(BLE_HS_EENCRYPT)
            CASE_RETURN_STR(BLE_HS_EENCRYPT_KEY_SZ)
            CASE_RETURN_STR(BLE_HS_ESTORE_CAP)
            CASE_RETURN_STR(BLE_HS_ESTORE_FAIL)
            CASE_RETURN_STR(BLE_HS_EPREEMPTED)
            CASE_RETURN_STR(BLE_HS_EDISABLED)
            CASE_RETURN_STR(BLE_HS_ESTALLED)

        default:
            return "unknown tls_bt_status";
    }
}



static void async_evt_func(struct ble_npl_event *ev)
{
    ble_async_t *bat = (ble_async_t *)ev->arg;
    async_evt_t *aet = (async_evt_t *)bat->evt;
    bat->func(bat->arg);
    //free timer;
    ble_npl_callout_deinit(&aet->co);
    //remove from list;
    ble_npl_mutex_pend(&async_evt_list.list_mutex, 0);
    dl_list_del(&aet->list);
    ble_npl_mutex_release(&async_evt_list.list_mutex);
    //free aysnc_evt;
    tls_mem_free(aet);
    //free ble_async;
    tls_mem_free(bat);
}



static void tls_ble_free_left_aysnc_list()
{
    async_evt_t *evt = NULL;
    async_evt_t *evt_next = NULL;

    if(dl_list_empty(&async_evt_list.list))
    { return ; }

    ble_npl_mutex_pend(&async_evt_list.list_mutex, 0);
    dl_list_for_each_safe(evt, evt_next, &async_evt_list.list, async_evt_t, list) {
        if(ble_npl_callout_is_active(&evt->co)) {
            ble_npl_callout_stop(&evt->co);
        }

        ble_npl_callout_deinit(&evt->co);
        dl_list_del(&evt->list);
        tls_mem_free(evt->priv_data);
        tls_mem_free(evt);
    }
    ble_npl_mutex_release(&async_evt_list.list_mutex);
}

int tls_bt_util_deinit(void)
{
    /*Do not forget to release  async event list*/
    tls_ble_free_left_aysnc_list();
    ble_npl_mutex_deinit(&async_evt_list.list_mutex);
    return 0;
}

int tls_bt_util_init(void)
{
    dl_list_init(&async_evt_list.list);
    ble_npl_mutex_init(&async_evt_list.list_mutex);
    return 0;
}

int tls_bt_async_proc_func(app_async_func_t *app_cb, void *app_arg, int ticks)
{
    async_evt_t *aet = (async_evt_t *)tls_mem_alloc(sizeof(async_evt_t));
    assert(aet != NULL);
    ble_async_t *bat = (ble_async_t *)tls_mem_alloc(sizeof(ble_async_t));
    assert(bat != NULL);
    bat->evt = aet;
    bat->func = app_cb;
    bat->arg = app_arg;
    /*remember the ble_async ptr, in case the timer is running, when do all reset.we can free it, see ble_svc_deinit*/
    aet->priv_data = (void *)bat;
    ble_npl_mutex_pend(&async_evt_list.list_mutex, 0);
    dl_list_add_tail(&async_evt_list.list, &aet->list);
    ble_npl_mutex_release(&async_evt_list.list_mutex);
    ble_npl_callout_init(&aet->co, nimble_port_get_dflt_eventq(), async_evt_func, (void *)bat);
    ble_npl_callout_reset(&aet->co, ticks);
    return 0;
}



ringbuffer_t *bt_ringbuffer_init(const size_t size)
{
    ringbuffer_t *p = tls_mem_alloc(sizeof(ringbuffer_t));
    p->base = tls_mem_alloc(size);
    p->head = p->tail = p->base;
    p->total = p->available = size;
    ble_npl_mutex_init(&p->buffer_mutex);
    return p;
}

void bt_ringbuffer_free(ringbuffer_t *rb)
{
    if(rb != NULL) {
        tls_mem_free(rb->base);
    }
    ble_npl_mutex_deinit(&rb->buffer_mutex);

    tls_mem_free(rb);
}
uint32_t bt_ringbuffer_available(const ringbuffer_t *rb)
{
    assert(rb);
    return rb->available;
}
uint32_t bt_ringbuffer_size(const ringbuffer_t *rb)
{
    assert(rb);
    return rb->total - rb->available;
}

uint32_t bt_ringbuffer_insert(ringbuffer_t *rb, const uint8_t *p, uint32_t length)
{
    assert(rb);
    assert(p);
    ble_npl_mutex_pend(&rb->buffer_mutex, 0);

    if(length > bt_ringbuffer_available(rb)) {
        length = bt_ringbuffer_available(rb);
    }

    for(size_t i = 0; i != length; ++i) {
        *rb->tail++ = *p++;

        if(rb->tail >= (rb->base + rb->total)) {
            rb->tail = rb->base;
        }
    }

    rb->available -= length;
    ble_npl_mutex_release(&rb->buffer_mutex);
    return length;
}

uint32_t bt_ringbuffer_delete(ringbuffer_t *rb, uint32_t length)
{
    assert(rb);
    ble_npl_mutex_pend(&rb->buffer_mutex, 0);

    if(length > bt_ringbuffer_size(rb)) {
        length = bt_ringbuffer_size(rb);
    }

    rb->head += length;

    if(rb->head >= (rb->base + rb->total)) {
        rb->head -= rb->total;
    }

    rb->available += length;
    ble_npl_mutex_release(&rb->buffer_mutex);
    return length;
}

uint32_t bt_ringbuffer_peek(const ringbuffer_t *rb, int offset, uint8_t *p, uint32_t length)
{
    assert(rb);
    assert(p);
    assert(offset >= 0);
    ble_npl_mutex_pend((struct ble_npl_mutex *)&rb->buffer_mutex, 0);
    assert((uint32_t)offset <= bt_ringbuffer_size(rb));
    uint8_t *b = ((rb->head - rb->base + offset) % rb->total) + rb->base;
    const size_t bytes_to_copy = (offset + length > bt_ringbuffer_size(rb)) ? bt_ringbuffer_size(
            rb) - offset : length;

    for(size_t copied = 0; copied < bytes_to_copy; ++copied) {
        *p++ = *b++;

        if(b >= (rb->base + rb->total)) {
            b = rb->base;
        }
    }

    ble_npl_mutex_release((struct ble_npl_mutex *)&rb->buffer_mutex);
    return bytes_to_copy;
}

uint32_t bt_ringbuffer_pop(ringbuffer_t *rb, uint8_t *p, uint32_t length)
{
    assert(rb);
    assert(p);
    
    const uint32_t copied = bt_ringbuffer_peek(rb, 0, p, length);
    ble_npl_mutex_pend((struct ble_npl_mutex *)&rb->buffer_mutex, 0);
    rb->head += copied;

    if(rb->head >= (rb->base + rb->total)) {
        rb->head -= rb->total;
    }

    rb->available += copied;
    ble_npl_mutex_release((struct ble_npl_mutex *)&rb->buffer_mutex);
    return copied;
}

#if 0
uint8_t string_to_bdaddr(const char *string, tls_bt_addr_t *addr)
{
    assert(string != NULL);
    assert(addr != NULL);
    tls_bt_addr_t new_addr;
    uint8_t *ptr = new_addr.address;
    uint8_t ret = sscanf(string, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                         &ptr[0], &ptr[1], &ptr[2], &ptr[3], &ptr[4], &ptr[5]) == 6;

    if(ret) {
        memcpy(addr, &new_addr, sizeof(tls_bt_addr_t));
    }

    return ret;
}
#endif

#endif
