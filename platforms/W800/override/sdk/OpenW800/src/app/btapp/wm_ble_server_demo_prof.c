#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"

#if (WM_BLE_INCLUDED == CFG_ON)

#include "wm_ble_server.h"
#include "wm_ble_gatt.h"
#include "wm_ble_server_demo_prof.h"
#include "wm_bt_util.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static tls_ble_callback_t tls_demo_at_cb_ptr;
static int demo_server_indication_timer_id = -1;
static uint8_t  g_server_if;
static uint16_t g_conn_id = -1;
static uint16_t g_char_handle;
static uint16_t g_desc_attr_handle = 0;;

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void ble_demo_server_register_app_cb(int status, int server_if, uint16_t app_uuid)
{
    TLS_BT_APPL_TRACE_DEBUG("demo server created, status=%d, server_if=%d, app_uuid=0x%04x\r\n", status,
                            server_if, app_uuid);
    tls_ble_msg_t msg;
    msg.ser_register.status = status;
    msg.ser_register.server_if = server_if;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_SE_REGISTER_EVT, &msg); }
}
void ble_demo_server_deregister_app_cb(int status, int server_if)
{
    TLS_BT_APPL_TRACE_DEBUG("demo server unregister...status=%d, server_if=%d\r\n", status, server_if);
    tls_ble_msg_t msg;
    msg.ser_register.status = status;
    msg.ser_register.server_if = server_if;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_SE_DEREGISTER_EVT, &msg); }

    tls_demo_at_cb_ptr = NULL;
}

void ble_demo_server_connection_cb(int conn_id, int server_if, int connected, tls_bt_addr_t *bda,
                                   uint16_t reason)
{
    TLS_BT_APPL_TRACE_DEBUG("%s...conn_id=%d, server_if=%d, connected=%d, reason=%d\r\n", __FUNCTION__,
                            conn_id, server_if, connected, reason);
    tls_ble_msg_t msg;
    memcpy(msg.ser_connect.addr, bda->address, 6);
    msg.ser_connect.connected = connected;
    msg.ser_connect.conn_id = conn_id;
    msg.ser_connect.server_if = server_if;
    msg.ser_connect.reason = reason;
    g_server_if = server_if;

    if(connected) {
        if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_SE_CONNECT_EVT, &msg); }

        g_conn_id = conn_id;
    } else {
        if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_SE_DISCONNECT_EVT, &msg); }

        g_conn_id = -1;
    }
}

void ble_demo_server_service_added_cb(int status, int server_if, int inst_id, bool is_primary,
                                      uint16_t app_uuid, int srvc_handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s...status=%d, server_if=%d,uuid=0x%04x srvc_handle=%d\r\n", __FUNCTION__,
                            status, server_if, app_uuid, srvc_handle);
    tls_ble_msg_t msg;
    msg.ser_create.inst_id = inst_id;
    msg.ser_create.is_primary = is_primary;
    msg.ser_create.server_if = server_if;
    msg.ser_create.service_id = srvc_handle;
    msg.ser_create.status = status;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_SE_CREATE_EVT, &msg); }
}

void ble_demo_server_included_service_added_cb(int status, int server_if,
        int srvc_handle,
        int incl_srvc_handle)
{
}

void ble_demo_server_characteristic_added_cb(int status, int server_if, uint16_t char_id,
        int srvc_handle, int char_handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s...status=%d, server_if=%d,uuid=0x%04x srvc_handle=%d, char_handle=%d\r\n",
                            __FUNCTION__, status, server_if, char_id, srvc_handle, char_handle);
    g_char_handle = char_handle;
    tls_ble_msg_t msg;
    msg.ser_add_char.attr_id = char_handle;
    msg.ser_add_char.server_if = server_if;
    msg.ser_add_char.service_id = srvc_handle;
    msg.ser_add_char.status = status;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_SE_ADD_CHAR_EVT, &msg); }
}

void ble_demo_server_descriptor_added_cb(int status, int server_if,
        uint16_t descr_id, int srvc_handle,
        int descr_handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s...status=%d, server_if=%d,uuid=0x%04x srvc_handle=%d, descr_handle=%d\r\n",
                            __FUNCTION__, status, server_if, descr_id, srvc_handle, descr_handle);
    g_desc_attr_handle = descr_handle;
    tls_ble_msg_t msg;
    msg.ser_add_char_descr.attr_id = descr_handle;
    msg.ser_add_char_descr.server_if = server_if;
    msg.ser_add_char_descr.service_id = srvc_handle;
    msg.ser_add_char_descr.status = status;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_SE_ADD_CHAR_DESCR_EVT, &msg); }
}

void ble_demo_server_service_started_cb(int status, int server_if, int srvc_handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, status=%d, server_if=%d, srvc_handle=%d\r\n", __FUNCTION__, status,
                            server_if, srvc_handle);
    tls_ble_msg_t msg;
    msg.ser_start_srvc.server_if = server_if;
    msg.ser_start_srvc.service_id = srvc_handle;
    msg.ser_start_srvc.status = status;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_SE_START_EVT, &msg); }
}

void ble_demo_server_service_stopped_cb(int status, int server_if, int srvc_handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, status=%d, server_if=%d, srvc_handle=%d\r\n", __FUNCTION__, status,
                            server_if, srvc_handle);
    tls_ble_msg_t msg;
    msg.ser_start_srvc.server_if = server_if;
    msg.ser_start_srvc.service_id = srvc_handle;
    msg.ser_start_srvc.status = status;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_SE_STOP_EVT, &msg); }
}

void ble_demo_server_service_deleted_cb(int status, int server_if, int srvc_handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, status=%d, server_if=%d, srvc_handle=%d\r\n", __FUNCTION__, status,
                            server_if, srvc_handle);
    tls_ble_msg_t msg;
    msg.ser_start_srvc.server_if = server_if;
    msg.ser_start_srvc.service_id = srvc_handle;
    msg.ser_start_srvc.status = status;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_SE_DELETE_EVT, &msg); }
}

void ble_demo_server_request_read_cb(int conn_id, int trans_id, tls_bt_addr_t *bda,
                                     int attr_handle, int offset, bool is_long)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d, trans_id=%d, attr_handle=%d\r\n", __FUNCTION__, conn_id,
                            trans_id, attr_handle);
    uint8_t resp[] = "12345678\r\n";
    tls_ble_msg_t msg;
    msg.ser_read.conn_id = conn_id;
    msg.ser_read.handle = attr_handle;
    msg.ser_read.is_long = is_long;
    msg.ser_read.offset = offset;
    msg.ser_read.trans_id = trans_id;
    memcpy(msg.ser_read.remote_bda, bda->address, 6);
    //if(tls_demo_at_cb_ptr)(tls_demo_at_cb_ptr)(WM_BLE_SE_READ_EVT, &msg);
    tls_ble_server_send_response(conn_id, trans_id, 0, offset, attr_handle, 0, resp, 10);
}


void ble_demo_server_indication_started(int id)
{
    int len = 0;
    uint8_t ind[12];

    if(g_conn_id < 0) { return; }

    len = sprintf(ind, "BLE, %d\r\n", tls_os_get_time());
    tls_ble_server_send_indication(g_server_if, g_char_handle, g_conn_id, len, 1, ind);
    tls_dm_start_timer(demo_server_indication_timer_id, 2000, ble_demo_server_indication_started);
}

void ble_demo_server_request_write_cb(int conn_id, int trans_id,
                                      tls_bt_addr_t *bda, int attr_handle,
                                      int offset, int length,
                                      bool need_rsp, bool is_prep, uint8_t *value)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d, trans_id=%d, attr_handle=%d, length=%d, is_prep=%d, need_resp=%d\r\n",
                            __FUNCTION__, conn_id, trans_id, attr_handle, length, is_prep, need_rsp);
    //hci_dbg_hexstring("request write:", value , length);

    if((value[0] == 0x00 || value[0] == 0x02) && (g_desc_attr_handle == attr_handle)) {
        TLS_BT_APPL_TRACE_DEBUG("This is an indication enable msg(%d)\r\n", value[0]);

        if(value[0] == 0x02) {
            demo_server_indication_timer_id = tls_dm_get_timer_id();
            tls_dm_start_timer(demo_server_indication_timer_id, 2000, ble_demo_server_indication_started);
        } else {
            if(demo_server_indication_timer_id >= 0) {
                tls_dm_stop_timer(demo_server_indication_timer_id);
                tls_dm_free_timer_id(demo_server_indication_timer_id);
            };
        }

        return;
    }

    tls_ble_msg_t msg;
    msg.ser_write.conn_id = conn_id;
    msg.ser_write.handle = attr_handle;
    msg.ser_write.is_prep = is_prep;
    msg.ser_write.len = length;
    msg.ser_write.need_rsp = need_rsp;
    msg.ser_write.offset = offset;
    msg.ser_write.trans_id = trans_id;
    msg.ser_write.value = value;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_SE_WRITE_EVT, &msg); }

    tls_ble_server_send_indication(g_server_if, attr_handle, conn_id, length, 1, value);
}

void ble_demo_server_request_exec_write_cb(int conn_id, int trans_id,
        tls_bt_addr_t *bda, int exec_write)
{
}

void ble_demo_server_response_confirmation_cb(int status, int conn_id, int trans_id)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d, trans_id=%d\r\n", __FUNCTION__, conn_id, trans_id);
    tls_ble_msg_t msg;
    msg.ser_resp.status = status;
    msg.ser_resp.conn_id = conn_id;
    msg.ser_resp.trans_id = trans_id;
    //if(tls_demo_at_cb_ptr)(tls_demo_at_cb_ptr)(WM_BLE_SE_RESP_EVT, &msg);
}

void ble_demo_server_indication_sent_cb(int conn_id, int status)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d, status=%d\r\n", __FUNCTION__, conn_id, status);
    tls_ble_msg_t msg;
    msg.ser_confirm.conn_id = conn_id;
    msg.ser_confirm.status = status;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_SE_CONFIRM_EVT, &msg); }
}

void ble_demo_server_congestion_cb(int conn_id, bool congested)
{
    tls_ble_msg_t msg;
    msg.ser_congest.conn_id = conn_id;
    msg.ser_congest.congested = congested;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_SE_CONGEST_EVT, &msg); }
}

void ble_demo_server_mtu_changed_cb(int conn_id, int mtu)
{
    tls_ble_msg_t msg;
    msg.ser_mtu.conn_id = conn_id;
    msg.ser_mtu.mtu = mtu;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_SE_MTU_EVT, &msg); }
}

static const wm_ble_server_callbacks_t servercb = {
    ble_demo_server_register_app_cb,
    ble_demo_server_deregister_app_cb,
    ble_demo_server_connection_cb,
    ble_demo_server_service_added_cb,
    ble_demo_server_included_service_added_cb,
    ble_demo_server_characteristic_added_cb,
    ble_demo_server_descriptor_added_cb,
    ble_demo_server_service_started_cb,
    ble_demo_server_service_stopped_cb,
    ble_demo_server_service_deleted_cb,
    ble_demo_server_request_read_cb,
    ble_demo_server_request_write_cb,
    ble_demo_server_request_exec_write_cb,
    ble_demo_server_response_confirmation_cb,
    ble_demo_server_indication_sent_cb,
    ble_demo_server_congestion_cb,
    ble_demo_server_mtu_changed_cb
};


int tls_ble_demo_prof_init(uint16_t demo_uuid, tls_ble_callback_t at_cb_ptr)
{
    tls_bt_status_t status;

    if(tls_demo_at_cb_ptr) { return TLS_BT_STATUS_BUSY; }

    tls_demo_at_cb_ptr = at_cb_ptr;
    status = tls_ble_server_register_server(demo_uuid, &servercb);

    if(status != TLS_BT_STATUS_SUCCESS) {
        tls_demo_at_cb_ptr = NULL;
    }

    return status;
}
int tls_ble_demo_prof_deinit(int server_if)
{
    if(tls_demo_at_cb_ptr == NULL) { return TLS_BT_STATUS_DONE; }

    return tls_ble_server_unregister_server(server_if);
}

#endif




