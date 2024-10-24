#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"

#if (WM_BLE_INCLUDED == CFG_ON)

#include "wm_ble_gatt.h"
#include "wm_ble_server.h"
#include "wm_bt_util.h"

/*
 * STRUCTURE DEFINITIONS
 ****************************************************************************************
 */

typedef struct {
    uint16_t uuid;
    int server_if;
    int connect_id;
    wm_ble_server_callbacks_t *ps_callbak;
    tls_bt_addr_t addr;
    uint8_t connected;
    uint8_t in_use;
} app_ble_server_t;

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
#define GATT_MAX_SR_PROFILES 8

static app_ble_server_t app_env[GATT_MAX_SR_PROFILES] = {0};

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static int get_free_app_env_index()
{
    int index = 0;

    for(index = 0; index < GATT_MAX_SR_PROFILES; index++) {
        if(app_env[index].in_use == 0) {
            return index;
        }
    }

    return -1;
}
static int get_app_env_index_by_uuid(uint16_t uuid)
{
    int index = 0;

    for(index = 0; index < GATT_MAX_SR_PROFILES; index++) {
        if(app_env[index].in_use == 1 && app_env[index].uuid == uuid) {
            return index;
        }
    }

    return -1;
}

static int get_app_env_index_by_server_if(int server_if)
{
    int index = 0;

    for(index = 0; index < GATT_MAX_SR_PROFILES; index++) {
        if(app_env[index].in_use == 1 && app_env[index].server_if == server_if) {
            return index;
        }
    }

    return -1;
}
static int get_app_env_index_by_conn_id(int conn_id)
{
    int index = 0;

    for(index = 0; index < GATT_MAX_SR_PROFILES; index++) {
        if(app_env[index].in_use == 1 && app_env[index].connect_id == conn_id) {
            return index;
        }
    }

    return -1;
}

void btgatts_register_app_cb(int status, int server_if, tls_bt_uuid_t *uuid)
{
    int index = -1;
    uint16_t app_uuid = app_uuid128_to_uuid16(uuid);
    TLS_BT_APPL_TRACE_VERBOSE("%s ,status = %d, server_if = %d, uuid=0x%04x\r\n", __FUNCTION__, status,
                              server_if, app_uuid);
    index = get_app_env_index_by_uuid(app_uuid);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,status = %d, server_if = %d, uuid=0x%04x(index=%d)\r\n", __FUNCTION__,
                                status, server_if, app_uuid, index);

        for(index = 0; index < GATT_MAX_SR_PROFILES; index++) {
            TLS_BT_APPL_TRACE_DEBUG("index=%d, in_use=%d, uuid=0x%04x\r\n", index, app_env[index].in_use,
                                    app_env[index].uuid);
        }

        return;
    }

    if(status != 0) {
        app_env[index].in_use = 0;
    }

    app_env[index].server_if = server_if;
    TLS_HAL_CBACK(app_env[index].ps_callbak, register_server_cb, status, server_if, app_uuid);
}

void btgatts_deregister_app_cb(int status, int server_if)
{
    TLS_BT_APPL_TRACE_VERBOSE("%s ,status = %d, server_if = %d\r\n", __FUNCTION__, status, server_if);
    int index = -1;
    index = get_app_env_index_by_server_if(server_if);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,status = %d, server_if = %d\r\n", __FUNCTION__, status, server_if);
        return;
    }

    app_env[index].in_use = 0;
    TLS_HAL_CBACK(app_env[index].ps_callbak, deregister_server_cb, status, server_if);
}

void btgatts_connection_cb(int conn_id, int server_if, int connected, tls_bt_addr_t *bda,
                           uint16_t reason)
{
    int index = -1;

    if(connected) {
        TLS_BT_APPL_TRACE_VERBOSE("%s, server_if=%d, connected = %d, conn_id=%d\r\n", __FUNCTION__,
                                  server_if, connected, conn_id);
    } else {
        TLS_BT_APPL_TRACE_VERBOSE("%s, server_if=%d, connected = %d, conn_id=%d, reason=0x%04x\r\n",
                                  __FUNCTION__, server_if, connected, conn_id, reason);
    }

    index = get_app_env_index_by_server_if(server_if);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, server_if=%d,connected = %d, conn_id=%d\r\n", __FUNCTION__, server_if,
                                connected, conn_id);
        return;
    }

    app_env[index].connected = connected;
    app_env[index].connect_id = conn_id;
    memcpy(&app_env[index].addr, bda, sizeof(tls_bt_addr_t));
    TLS_HAL_CBACK(app_env[index].ps_callbak, connection_cb, conn_id, server_if, connected, bda, reason);
}

void btgatts_service_added_cb(int status, int server_if, uint8_t inst_id, uint8_t is_primary,
                              tls_bt_uuid_t *uuid,  int srvc_handle)
{
    int index = -1;
    uint16_t app_uuid;
    TLS_BT_APPL_TRACE_VERBOSE("%s ,status = %d,server_if=%d srvc_handle = %d\r\n", __FUNCTION__, status,
                              server_if, srvc_handle);
    index = get_app_env_index_by_server_if(server_if);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,status = %d,server_if=%d srvc_handle = %d\r\n", __FUNCTION__, status,
                                server_if, srvc_handle);
        return;
    }

    app_uuid = app_uuid128_to_uuid16(uuid);
    TLS_HAL_CBACK(app_env[index].ps_callbak, service_added_cb, status, server_if, inst_id, is_primary,
                  app_uuid, srvc_handle);
}

void btgatts_included_service_added_cb(int status, int server_if,
                                       int srvc_handle,
                                       int incl_srvc_handle)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s ,status = %d,server_if=%d srvc_handle = %d\r\n", __FUNCTION__, status,
                              server_if, srvc_handle);
    index = get_app_env_index_by_server_if(server_if);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,status = %d,server_if=%d srvc_handle = %d\r\n", __FUNCTION__, status,
                                server_if, srvc_handle);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, included_service_added_cb, status, server_if, srvc_handle,
                  incl_srvc_handle);
}

void btgatts_characteristic_added_cb(int status, int server_if, tls_bt_uuid_t *char_id,
                                     int srvc_handle, int char_handle)
{
    int index = -1;
    uint16_t app_uuid;
    index = get_app_env_index_by_server_if(server_if);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,status = %d, srvc_handle = %d, char_handle = %d\r\n", __FUNCTION__,
                                status, srvc_handle, char_handle);
        return;
    }

    TLS_BT_APPL_TRACE_VERBOSE("%s ,status = %d, srvc_handle = %d, char_handle = %d\r\n", __FUNCTION__,
                              status, srvc_handle, char_handle);
    app_uuid = app_uuid128_to_uuid16(char_id);
    TLS_HAL_CBACK(app_env[index].ps_callbak, characteristic_added_cb, status, server_if, app_uuid,
                  srvc_handle, char_handle);
}

void btgatts_descriptor_added_cb(int status, int server_if,
                                 tls_bt_uuid_t *descr_id, int srvc_handle,
                                 int descr_handle)
{
    int index = -1;
    uint16_t app_uuid;
    index = get_app_env_index_by_server_if(server_if);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,status = %d, srvc_handle = %d, descr_handle = %d\r\n", __FUNCTION__,
                                status, srvc_handle, descr_handle);
        return;
    }

    TLS_BT_APPL_TRACE_VERBOSE("%s ,status = %d, srvc_handle = %d, descr_handle = %d\r\n", __FUNCTION__,
                              status, srvc_handle, descr_handle);
    app_uuid = app_uuid128_to_uuid16(descr_id);
    TLS_HAL_CBACK(app_env[index].ps_callbak, descriptor_added_cb, status, server_if, app_uuid,
                  srvc_handle, descr_handle);
}

void btgatts_service_started_cb(int status, int server_if, int srvc_handle)
{
    int index = -1;
    index = get_app_env_index_by_server_if(server_if);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,status = %d, server_if = %d, srvc_handle = %d\r\n", __FUNCTION__,
                                status, server_if, srvc_handle);
        return;
    }

    TLS_BT_APPL_TRACE_VERBOSE("%s ,status = %d, server_if = %d, srvc_handle = %d\r\n", __FUNCTION__,
                              status, server_if, srvc_handle);
    TLS_HAL_CBACK(app_env[index].ps_callbak, service_started_cb, status, server_if, srvc_handle);
}

void btgatts_service_stopped_cb(int status, int server_if, int srvc_handle)
{
    int index = -1;
    index = get_app_env_index_by_server_if(server_if);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,status = %d, server_if = %d, srvc_handle = %d\r\n", __FUNCTION__,
                                status, server_if, srvc_handle);
        return;
    }

    TLS_BT_APPL_TRACE_VERBOSE("%s ,status = %d, server_if = %d, srvc_handle = %d\r\n", __FUNCTION__,
                              status, server_if, srvc_handle);
    TLS_HAL_CBACK(app_env[index].ps_callbak, service_stopped_cb, status, server_if, srvc_handle);
}

void btgatts_service_deleted_cb(int status, int server_if, int srvc_handle)
{
    int index = -1;
    index = get_app_env_index_by_server_if(server_if);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,status = %d, server_if = %d, srvc_handle = %d\r\n", __FUNCTION__,
                                status, server_if, srvc_handle);
        return;
    }

    TLS_BT_APPL_TRACE_VERBOSE("%s ,status = %d, server_if = %d, srvc_handle = %d\r\n", __FUNCTION__,
                              status, server_if, srvc_handle);
    TLS_HAL_CBACK(app_env[index].ps_callbak, service_deleted_cb, status, server_if, srvc_handle);
}

void btgatts_request_read_cb(int conn_id, int trans_id, tls_bt_addr_t *bda,
                             int attr_handle, int offset, bool is_long)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s ,conn_id = %d, trans_id = %d, attr_handle = %d\r\n", __FUNCTION__,
                              conn_id, trans_id, attr_handle);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,conn_id = %d, trans_id = %d, attr_handle = %d\r\n", __FUNCTION__,
                                conn_id, trans_id, attr_handle);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, request_read_cb, conn_id, trans_id, bda, attr_handle,
                  offset, is_long);
}

void btgatts_request_write_cb(int conn_id, int trans_id,
                              tls_bt_addr_t *bda, int attr_handle,
                              int offset, int length,
                              bool need_rsp, bool is_prep, uint8_t *value)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s,conn_id=%d, trans_id=%d, attr_handle=%d, is_prep=%d, need_rsp=%s\r\n",
                              __FUNCTION__, conn_id, trans_id, attr_handle, is_prep, need_rsp == true ? "yes" : "no");

    if(need_rsp) {
        tls_ble_server_send_response(conn_id, trans_id, TLS_BT_STATUS_SUCCESS, offset, attr_handle, 0,
                                     value, length);
    }

    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,conn_id = %d, trans_id = %d, attr_handle = %d\r\n", __FUNCTION__,
                                conn_id, trans_id, attr_handle);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, request_write_cb, conn_id, trans_id, bda, attr_handle,
                  offset, length, need_rsp, is_prep, value);
}

void btgatts_request_exec_write_cb(int conn_id, int trans_id,
                                   tls_bt_addr_t *bda, int exec_write)
{
    int index = -1;
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,conn_id = %d, trans_id = %d, attr_handle=%d,exec_write = %d\r\n",
                                __FUNCTION__, conn_id, trans_id, 0, exec_write);
        return;
    }

    tls_ble_server_send_response(conn_id, trans_id, TLS_BT_STATUS_SUCCESS, 0, 0 /*dummy attr_handle*/,
                                 0, "0"/*dummy response*/, 1);
    TLS_HAL_CBACK(app_env[index].ps_callbak, request_exec_write_cb, conn_id, trans_id, bda, exec_write);
}

void btgatts_response_confirmation_cb(int status, uint16_t conn_id, uint16_t trans_id)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s\r\n", __FUNCTION__);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,conn_id = %d, trans_id = %d\r\n", __FUNCTION__, conn_id, trans_id);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, response_confirmation_cb, status, conn_id, trans_id);
}

void btgatts_indication_sent_cb(int conn_id, int status)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s\r\n", __FUNCTION__);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,conn_id = %d, status = %d\r\n", __FUNCTION__, conn_id, status);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, indication_sent_cb, conn_id, status);
}

void btgatts_congestion_cb(int conn_id, bool congested)
{
    int index = -1;
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,conn_id = %d, congested = %d\r\n", __FUNCTION__, conn_id, congested);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, congestion_cb, conn_id, congested);
}

void btgatts_mtu_changed_cb(int conn_id, int mtu)
{
    int index = -1;
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,conn_id = %d, mtu = %d\r\n", __FUNCTION__, conn_id, mtu);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, mtu_changed_cb, conn_id, mtu);
}



static void tls_ble_server_event_handler(tls_ble_evt_t evt, tls_ble_msg_t *msg)
{
    //TLS_BT_APPL_TRACE_EVENT("%s, event:%s,%d\r\n", __FUNCTION__, tls_gatt_evt_2_str(evt), evt);
    tls_bt_addr_t addr;

    switch(evt) {
        case WM_BLE_SE_REGISTER_EVT:
            btgatts_register_app_cb(msg->ser_register.status, msg->ser_register.server_if,
                                    &msg->ser_register.app_uuid);
            break;

        case WM_BLE_SE_DEREGISTER_EVT:
            btgatts_deregister_app_cb(msg->ser_register.status, msg->ser_register.server_if);
            break;

        case WM_BLE_SE_CONNECT_EVT:
            memcpy(addr.address, msg->ser_connect.addr, 6);
            btgatts_connection_cb(msg->ser_connect.conn_id, msg->ser_connect.server_if,
                                  msg->ser_connect.connected, &addr, msg->ser_connect.reason);
            break;

        case WM_BLE_SE_DISCONNECT_EVT:
            memcpy(addr.address, msg->ser_disconnect.addr, 6);
            btgatts_connection_cb(msg->ser_disconnect.conn_id, msg->ser_disconnect.server_if,
                                  msg->ser_disconnect.connected, &addr, msg->ser_disconnect.reason);
            break;

        case WM_BLE_SE_CREATE_EVT:
            btgatts_service_added_cb(msg->ser_create.status, msg->ser_create.server_if, msg->ser_create.inst_id,
                                     msg->ser_create.is_primary, &msg->ser_create.uuid, msg->ser_create.service_id);
            break;

        case WM_BLE_SE_ADD_CHAR_EVT:
            btgatts_characteristic_added_cb(msg->ser_add_char.status, msg->ser_add_char.server_if,
                                            &msg->ser_add_char.uuid, msg->ser_add_char.service_id, msg->ser_add_char.attr_id);
            break;

        case WM_BLE_SE_ADD_CHAR_DESCR_EVT:
            btgatts_descriptor_added_cb(msg->ser_add_char_descr.status, msg->ser_add_char_descr.server_if,
                                        &msg->ser_add_char_descr.uuid, msg->ser_add_char_descr.service_id, msg->ser_add_char_descr.attr_id);
            break;

        case WM_BLE_SE_START_EVT:
            btgatts_service_started_cb(msg->ser_start_srvc.status, msg->ser_start_srvc.server_if,
                                       msg->ser_start_srvc.service_id);
            break;

        case WM_BLE_SE_STOP_EVT:
            btgatts_service_stopped_cb(msg->ser_stop_srvc.status, msg->ser_stop_srvc.server_if,
                                       msg->ser_stop_srvc.service_id);
            break;

        case WM_BLE_SE_DELETE_EVT:
            btgatts_service_deleted_cb(msg->ser_delete_srvc.status, msg->ser_delete_srvc.server_if,
                                       msg->ser_delete_srvc.service_id);
            break;

        case WM_BLE_SE_READ_EVT:
            memcpy(addr.address, msg->ser_read.remote_bda, 6);
            btgatts_request_read_cb(msg->ser_read.conn_id, msg->ser_read.trans_id, &addr, msg->ser_read.handle,
                                    msg->ser_read.offset, msg->ser_read.is_long);
            break;

        case WM_BLE_SE_WRITE_EVT:
            memcpy(addr.address, msg->ser_write.remote_bda, 6);
            btgatts_request_write_cb(msg->ser_write.conn_id, msg->ser_write.trans_id, &addr,
                                     msg->ser_write.handle, msg->ser_write.offset, msg->ser_write.len, msg->ser_write.need_rsp,
                                     msg->ser_write.is_prep, msg->ser_write.value);
            break;

        case WM_BLE_SE_EXEC_WRITE_EVT:
            memcpy(addr.address, msg->ser_exec_write.remote_bda, 6);
            btgatts_request_exec_write_cb(msg->ser_exec_write.conn_id, msg->ser_exec_write.trans_id, &addr,
                                          msg->ser_exec_write.exec_write);
            break;

        case WM_BLE_SE_CONFIRM_EVT:
            btgatts_indication_sent_cb(msg->ser_confirm.conn_id, msg->ser_confirm.status);
            break;

        case WM_BLE_SE_RESP_EVT:
            btgatts_response_confirmation_cb(msg->ser_resp.status, msg->ser_resp.conn_id,
                                             msg->ser_resp.trans_id);
            break;

        case WM_BLE_SE_CONGEST_EVT:
            btgatts_congestion_cb(msg->ser_congest.conn_id, msg->ser_congest.congested);
            break;

        case WM_BLE_SE_MTU_EVT:
            btgatts_mtu_changed_cb(msg->ser_mtu.conn_id, msg->ser_mtu.mtu);
            break;

        case WM_BLE_SE_ADD_INCL_SRVC_EVT:
            btgatts_included_service_added_cb(msg->ser_add_incl_srvc.status, msg->ser_add_incl_srvc.server_if,
                                              msg->ser_add_incl_srvc.service_id, msg->ser_add_incl_srvc.attr_id);
            break;

        default:
            TLS_BT_APPL_TRACE_WARNING("Warning Unknown ble server app evt=%d\r\n", evt);
            break;
    }
}


/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

int tls_ble_server_init()
{
    memset(&app_env, 0, sizeof(app_ble_server_t)*GATT_MAX_SR_PROFILES);
    return tls_ble_server_app_init(tls_ble_server_event_handler);
}

int tls_ble_server_deinit()
{
    return tls_ble_server_app_deinit();
}

/** Registers a GATT server application with the stack */
int tls_ble_server_register_server(uint16_t app_uuid, wm_ble_server_callbacks_t *callback)
{
    int index = -1;
    tls_bt_status_t status;
    TLS_BT_APPL_TRACE_VERBOSE("### tls_ble_server_app_register start\r\n");
    index = get_app_env_index_by_uuid(app_uuid);

    if(index >= 0) {
        return TLS_BT_STATUS_DONE;
    }

    index = get_free_app_env_index();

    if(index < 0) {
        return TLS_BT_STATUS_NOMEM;
    }

    app_env[index].in_use = 1;
    app_env[index].uuid = app_uuid;
    app_env[index].ps_callbak = callback;
    status = tls_ble_server_app_register(app_uuid16_to_uuid128(app_uuid));

    if(status != TLS_BT_STATUS_SUCCESS) {
        TLS_BT_APPL_TRACE_ERROR("### tls_ble_server_app_register, failed,index=%d, app_uuid=0x%04x\r\n",
                                index, app_uuid);
        app_env[index].in_use = 0;
    } else {
        TLS_BT_APPL_TRACE_VERBOSE("### tls_ble_server_app_register,end, index=%d, app_uuid=0x%04x\r\n",
                                  index, app_uuid);
    }

    return status;
}

/** Unregister a server application from the stack */
int tls_ble_server_unregister_server(int server_if)
{
    int index = -1;
    index = get_app_env_index_by_server_if(server_if);

    if(index < 0) {
        return TLS_BT_STATUS_PARM_INVALID;
    }

    return tls_ble_server_app_unregister(server_if);
}

#endif
