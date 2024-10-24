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

#if (WM_BT_INCLUDED == CFG_ON || WM_BLE_INCLUDED == CFG_ON)

#include "wm_bt_def.h"

#include "wm_bt_util.h"
#include "wm_dbg.h"
#include "wm_mem.h"

extern size_t ringbuffer_size(const ringbuffer_t *rb);;

tls_bt_log_level_t tls_appl_trace_level =  TLS_BT_LOG_DEBUG;

static tls_bt_uuid_t app_base_uuid = {
    {
        0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
        0x00, 0x10, 0x00, 0x00, 0x34, 0x12, 0x00, 0x00
    }
};

uint16_t app_uuid128_to_uuid16(tls_bt_uuid_t *uuid)
{
    uint16_t id = 0;
    //id = ((uint16_t)uuid->uu[12]) | (((uint16_t)uuid->uu[13]) << 8);
    memcpy(&id, uuid->uu + 12, 2);
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    return id;
}
tls_bt_uuid_t *app_uuid16_to_uuid128(uint16_t uuid16)
{
    memcpy(app_base_uuid.uu + 12, &uuid16, 2);
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    return &app_base_uuid;
}

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

const char *tls_bt_host_evt_2_str(uint32_t event)
{
    switch(event) {
            CASE_RETURN_STR(WM_BT_ADAPTER_STATE_CHG_EVT)
            CASE_RETURN_STR(WM_BT_ADAPTER_PROP_CHG_EVT)
            CASE_RETURN_STR(WM_BT_RMT_DEVICE_PROP_EVT)
            CASE_RETURN_STR(WM_BT_DEVICE_FOUND_EVT)
            CASE_RETURN_STR(WM_BT_DISCOVERY_STATE_CHG_EVT)
            CASE_RETURN_STR(WM_BT_REQUEST_EVT)
            CASE_RETURN_STR(WM_BT_SSP_REQUEST_EVT)
            CASE_RETURN_STR(WM_BT_PIN_REQUEST_EVT)
            CASE_RETURN_STR(WM_BT_BOND_STATE_CHG_EVT)
            CASE_RETURN_STR(WM_BT_ACL_STATE_CHG_EVT)
            CASE_RETURN_STR(WM_BT_ENERGY_INFO_EVT)
            CASE_RETURN_STR(WM_BT_LE_TEST_EVT)

        default:
            return "unkown bt host evt";
    }
}

const char *tls_dm_evt_2_str(uint32_t event)
{
    switch(event) {
            CASE_RETURN_STR(WM_BLE_DM_SET_ADV_DATA_CMPL_EVT)
            CASE_RETURN_STR(WM_BLE_DM_TIMER_EXPIRED_EVT)
            CASE_RETURN_STR(WM_BLE_DM_TRIGER_EVT)
            CASE_RETURN_STR(WM_BLE_DM_SCAN_RES_EVT)
            CASE_RETURN_STR(WM_BLE_DM_SET_SCAN_PARAM_CMPL_EVT)
            CASE_RETURN_STR(WM_BLE_DM_SCAN_RES_CMPL_EVT)
            CASE_RETURN_STR(WM_BLE_DM_REPORT_RSSI_EVT)
            CASE_RETURN_STR(WM_BLE_DM_SEC_EVT)
            CASE_RETURN_STR(WM_BLE_DM_ADV_STARTED_EVT)
            CASE_RETURN_STR(WM_BLE_DM_ADV_STOPPED_EVT)

        default:
            return "unkown dm evt";
    }
}

const char *tls_bt_status_2_str(uint32_t event)
{
    switch(event) {
            CASE_RETURN_STR(TLS_BT_STATUS_SUCCESS)
            CASE_RETURN_STR(TLS_BT_STATUS_FAIL)
            CASE_RETURN_STR(TLS_BT_STATUS_NOT_READY)
            CASE_RETURN_STR(TLS_BT_STATUS_NOMEM)
            CASE_RETURN_STR(TLS_BT_STATUS_BUSY)
            CASE_RETURN_STR(TLS_BT_STATUS_DONE)
            CASE_RETURN_STR(TLS_BT_STATUS_UNSUPPORTED)
            CASE_RETURN_STR(TLS_BT_STATUS_PARM_INVALID)
            CASE_RETURN_STR(TLS_BT_STATUS_UNHANDLED)
            CASE_RETURN_STR(TLS_BT_STATUS_AUTH_FAILURE)
            CASE_RETURN_STR(TLS_BT_STATUS_RMT_DEV_DOWN)
            CASE_RETURN_STR(TLS_BT_STATUS_AUTH_REJECTED)
            CASE_RETURN_STR(TLS_BT_STATUS_THREAD_FAILED)
            CASE_RETURN_STR(TLS_BT_STATUS_INTERNAL_ERROR)
            CASE_RETURN_STR(TLS_BT_STATUS_CTRL_ENABLE_FAILED)
            CASE_RETURN_STR(TLS_BT_STATUS_HOST_ENABLE_FAILED)
            CASE_RETURN_STR(TLS_BT_STATUS_CTRL_DISABLE_FAILED)
            CASE_RETURN_STR(TLS_BT_STATUS_HOST_DISABLE_FAILED)

        default:
            return "unknown tls_bt_status";
    }
}

const char *tls_gatt_evt_2_str(uint32_t event)
{
    switch(event) {
            CASE_RETURN_STR(WM_BLE_CL_REGISTER_EVT)
            CASE_RETURN_STR(WM_BLE_CL_DEREGISTER_EVT)
            CASE_RETURN_STR(WM_BLE_CL_READ_CHAR_EVT)
            CASE_RETURN_STR(WM_BLE_CL_WRITE_CHAR_EVT)
            CASE_RETURN_STR(WM_BLE_CL_PREP_WRITE_EVT)
            CASE_RETURN_STR(WM_BLE_CL_EXEC_CMPL_EVT)
            CASE_RETURN_STR(WM_BLE_CL_SEARCH_CMPL_EVT)
            CASE_RETURN_STR(WM_BLE_CL_SEARCH_RES_EVT)
            CASE_RETURN_STR(WM_BLE_CL_READ_DESCR_EVT)
            CASE_RETURN_STR(WM_BLE_CL_WRITE_DESCR_EVT)
            CASE_RETURN_STR(WM_BLE_CL_NOTIF_EVT)
            CASE_RETURN_STR(WM_BLE_CL_OPEN_EVT)
            CASE_RETURN_STR(WM_BLE_CL_CLOSE_EVT)
            CASE_RETURN_STR(WM_BLE_CL_LISTEN_EVT)
            CASE_RETURN_STR(WM_BLE_CL_CFG_MTU_EVT)
            CASE_RETURN_STR(WM_BLE_CL_CONGEST_EVT)
            CASE_RETURN_STR(WM_BLE_CL_REPORT_DB_EVT)
            CASE_RETURN_STR(WM_BLE_CL_REG_NOTIFY_EVT)
            CASE_RETURN_STR(WM_BLE_CL_DEREG_NOTIFY_EVT)
            CASE_RETURN_STR(WM_BLE_SE_REGISTER_EVT)
            CASE_RETURN_STR(WM_BLE_SE_DEREGISTER_EVT)
            CASE_RETURN_STR(WM_BLE_SE_CONNECT_EVT)
            CASE_RETURN_STR(WM_BLE_SE_DISCONNECT_EVT)
            CASE_RETURN_STR(WM_BLE_SE_CREATE_EVT)
            CASE_RETURN_STR(WM_BLE_SE_ADD_INCL_SRVC_EVT)
            CASE_RETURN_STR(WM_BLE_SE_ADD_CHAR_EVT)
            CASE_RETURN_STR(WM_BLE_SE_ADD_CHAR_DESCR_EVT)
            CASE_RETURN_STR(WM_BLE_SE_START_EVT)
            CASE_RETURN_STR(WM_BLE_SE_STOP_EVT)
            CASE_RETURN_STR(WM_BLE_SE_DELETE_EVT)
            CASE_RETURN_STR(WM_BLE_SE_READ_EVT)
            CASE_RETURN_STR(WM_BLE_SE_WRITE_EVT)
            CASE_RETURN_STR(WM_BLE_SE_EXEC_WRITE_EVT)
            CASE_RETURN_STR(WM_BLE_SE_CONFIRM_EVT)
            CASE_RETURN_STR(WM_BLE_SE_RESP_EVT)
            CASE_RETURN_STR(WM_BLE_SE_CONGEST_EVT)
            CASE_RETURN_STR(WM_BLE_SE_MTU_EVT)

        default:
            return "unknown gatt evt";
    }
}

const char *tls_spp_evt_2_str(uint32_t event)
{
    switch(event) {
            CASE_RETURN_STR(WM_SPP_INIT_EVT)
            CASE_RETURN_STR(WM_SPP_DISCOVERY_COMP_EVT)
            CASE_RETURN_STR(WM_SPP_OPEN_EVT)
            CASE_RETURN_STR(WM_SPP_CLOSE_EVT)
            CASE_RETURN_STR(WM_SPP_START_EVT)
            CASE_RETURN_STR(WM_SPP_CL_INIT_EVT)
            CASE_RETURN_STR(WM_SPP_DATA_IND_EVT)
            CASE_RETURN_STR(WM_SPP_CONG_EVT)
            CASE_RETURN_STR(WM_SPP_WRITE_EVT)
            CASE_RETURN_STR(WM_SPP_SRV_OPEN_EVT)

        default:
            return "unknown spp evt";
    }
}
ringbuffer_t *bt_ringbuffer_init(const size_t size)
{
    ringbuffer_t *p = tls_mem_alloc(sizeof(ringbuffer_t));
    p->base = tls_mem_alloc(size);
    p->head = p->tail = p->base;
    p->total = p->available = size;
    return p;
}

void bt_ringbuffer_free(ringbuffer_t *rb)
{
    if(rb != NULL) {
        tls_mem_free(rb->base);
    }

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
    uint32_t cpu_sr = tls_os_set_critical();

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
    tls_os_release_critical(cpu_sr);
    return length;
}

uint32_t bt_ringbuffer_delete(ringbuffer_t *rb, uint32_t length)
{
    assert(rb);
    uint32_t cpu_sr = tls_os_set_critical();

    if(length > bt_ringbuffer_size(rb)) {
        length = bt_ringbuffer_size(rb);
    }

    rb->head += length;

    if(rb->head >= (rb->base + rb->total)) {
        rb->head -= rb->total;
    }

    rb->available += length;
    tls_os_release_critical(cpu_sr);
    return length;
}

uint32_t bt_ringbuffer_peek(const ringbuffer_t *rb, int offset, uint8_t *p, uint32_t length)
{
    assert(rb);
    assert(p);
    assert(offset >= 0);
    uint32_t cpu_sr = tls_os_set_critical();
    assert((uint32_t)offset <= ringbuffer_size(rb));
    uint8_t *b = ((rb->head - rb->base + offset) % rb->total) + rb->base;
    const size_t bytes_to_copy = (offset + length > ringbuffer_size(rb)) ? ringbuffer_size(
            rb) - offset : length;

    for(size_t copied = 0; copied < bytes_to_copy; ++copied) {
        *p++ = *b++;

        if(b >= (rb->base + rb->total)) {
            b = rb->base;
        }
    }

    tls_os_release_critical(cpu_sr);
    return bytes_to_copy;
}

uint32_t bt_ringbuffer_pop(ringbuffer_t *rb, uint8_t *p, uint32_t length)
{
    assert(rb);
    assert(p);
    uint32_t cpu_sr = tls_os_set_critical();
    const uint32_t copied = bt_ringbuffer_peek(rb, 0, p, length);
    rb->head += copied;

    if(rb->head >= (rb->base + rb->total)) {
        rb->head -= rb->total;
    }

    rb->available += copied;
    tls_os_release_critical(cpu_sr);
    return copied;
}
#endif


