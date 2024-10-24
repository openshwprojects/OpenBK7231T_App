#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"

#if (WM_BLE_INCLUDED == CFG_ON)


#include "wm_ble_gatt.h"
#include "wm_ble_client.h"
#include "wm_bt_util.h"
/*
 * STRUCTURE DEFINITIONS
 ****************************************************************************************
 */

typedef struct {
    uint16_t uuid;
    int client_if;
    int connect_id[WM_BLE_MAX_CONNECTION];
    wm_ble_client_callbacks_t *ps_callbak;
    uint8_t in_use;

} app_ble_client_t;

/*
 * DEFINES
 ****************************************************************************************
 */

#define GATT_MAX_CNT_SUPPORT    7

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static app_ble_client_t app_env[GATT_MAX_CNT_SUPPORT] = {0};

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static int get_free_app_env_index()
{
    int index = 0;

    for(index = 0; index < GATT_MAX_CNT_SUPPORT; index++) {
        if(app_env[index].in_use == 0) {
            return index;
        }
    }

    return -1;
}
static int get_app_env_index_by_uuid(uint16_t uuid)
{
    int index = 0;

    for(index = 0; index < GATT_MAX_CNT_SUPPORT; index++) {
        if(app_env[index].in_use == 1 && app_env[index].uuid == uuid) {
            return index;
        }
    }

    return -1;
}

static int get_app_env_index_by_client_if(int client_if)
{
    int index = 0;

    for(index = 0; index < GATT_MAX_CNT_SUPPORT; index++) {
        if(app_env[index].in_use == 1 && app_env[index].client_if == client_if) {
            return index;
        }
    }

    return -1;
}
static int get_app_env_index_by_conn_id(int conn_id)
{
    int index = 0;
    int conn_id_index = 0;

    for(index = 0; index < GATT_MAX_CNT_SUPPORT; index++) {
        if(app_env[index].in_use == 1) {
            for(conn_id_index = 0; conn_id_index < WM_BLE_MAX_CONNECTION; conn_id_index++) {
                if(app_env[index].connect_id[conn_id_index] == conn_id) {
                    return index;
                }
            }
        }
    }

    return -1;
}

/** Callback invoked in response to register_client */
void btgattc_register_client_callback(int status, int client_if,
                                      tls_bt_uuid_t *uuid)
{
    TLS_BT_APPL_TRACE_VERBOSE("%s ,status = %d, client_if = %d\r\n", __FUNCTION__, status, client_if);
    uint16_t app_uuid = 0;
    int index = 0;
    app_uuid = app_uuid128_to_uuid16(uuid);
    index = get_app_env_index_by_uuid(app_uuid);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s ,status = %d, client_if = %d,uuid=0x%04x\r\n", __FUNCTION__, status,
                                client_if, app_uuid);
        return;
    }

    if(status != 0) {
        app_env[index].in_use = 0;
    }

    app_env[index].client_if = client_if;
    TLS_HAL_CBACK(app_env[index].ps_callbak, register_client_cb, status, client_if, app_uuid);
}

void btgattc_deregister_client_callback(int status, int client_if)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s, status=%d, client_if=%d\r\n", __FUNCTION__, status, client_if);
    index = get_app_env_index_by_client_if(client_if);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, status=%d, client_if=%d\r\n", __FUNCTION__, status, client_if);
        return;
    }

    app_env[index].in_use = 0;
    TLS_HAL_CBACK(app_env[index].ps_callbak, deregister_client_cb, status, client_if);
}
/** GATT open callback invoked in response to open */
void btgattc_connect_callback(int conn_id, int status, int client_if, tls_bt_addr_t *bda)
{
    int index = -1;
    int conn_id_index = 0;
    TLS_BT_APPL_TRACE_VERBOSE("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
    index = get_app_env_index_by_client_if(client_if);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, status=%d, client_if=%d, conn_id=%d\r\n", __FUNCTION__, status,
                                client_if, conn_id);
        return;
    }

    //app_env[index].connected_status = status;
    //app_env[index].connect_id = conn_id;
    //memcpy(&app_env[index].addr, bda, sizeof(tls_bt_addr_t));
    /*find a free pos to store the connection id belongs to the client_if*/
    for(conn_id_index = 0; conn_id_index < WM_BLE_MAX_CONNECTION; conn_id_index++) {
        if((status == 0)) {
            if(app_env[index].connect_id[conn_id_index] == 0) {
                app_env[index].connect_id[conn_id_index] = conn_id;
                break;
            }
        } else {
            if(app_env[index].connect_id[conn_id_index] == conn_id) {
                app_env[index].connect_id[conn_id_index] = 0;
                break;
            }
        }
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, open_cb, conn_id, status, client_if, bda);
}

/** Callback invoked in response to close */
void btgattc_disconnect_callback(int conn_id, int status, int reason,
                                 int client_if, tls_bt_addr_t *bda)
{
    int index = -1;
    int conn_id_index = 0;
    TLS_BT_APPL_TRACE_VERBOSE("%s, client_if=%d, conn_id=%d\r\n", __FUNCTION__, client_if, conn_id);
    index = get_app_env_index_by_client_if(client_if);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, status=%d, reason=%d, client_if=%d, conn_id=%d\r\n", __FUNCTION__,
                                status, reason, client_if, conn_id);
        return;
    }

    for(conn_id_index = 0; conn_id_index < WM_BLE_MAX_CONNECTION; conn_id_index++) {
        if(app_env[index].connect_id[conn_id_index] == conn_id) {
            app_env[index].connect_id[conn_id_index] = 0;
            break;
        }
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, close_cb, conn_id, status, reason, client_if, bda);
}

/**
 * Invoked in response to search_service when the GATT service search
 * has been completed.
 */
void btgattc_search_complete_callback(int conn_id, int status)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, status=%d, conn_id=%d\r\n", __FUNCTION__, status,  conn_id);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, search_complete_cb, conn_id, status);
}

void btgattc_search_service_result_callback(int conn_id, tls_bt_uuid_t *p_uuid, uint8_t inst_id)
{
    int index = -1;
    uint16_t app_uuid = 0;
    TLS_BT_APPL_TRACE_VERBOSE("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, inst_id=%d, conn_id=%d\r\n", __FUNCTION__, inst_id,  conn_id);
        return;
    }

    app_uuid = app_uuid128_to_uuid16(p_uuid);
    TLS_HAL_CBACK(app_env[index].ps_callbak, search_serv_res_cb, conn_id, app_uuid, inst_id);
}

/** Callback invoked in response to [de]register_for_notification */
void btgattc_register_for_notification_callback(int conn_id,
        int registered, int status, uint16_t handle)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s, status=%d, conn_id=%d,registered=%d, handle=%d\r\n", __FUNCTION__,
                              status,  conn_id, registered, handle);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, status=%d, conn_id=%d,registered=%d, handle=%d\r\n", __FUNCTION__,
                                status,  conn_id, registered, handle);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, register_for_notification_cb, conn_id, registered, status,
                  handle);
}

/**
 * Remote device notification callback, invoked when a remote device sends
 * a notification or indication that a client has registered for.
 */
void btgattc_notify_callback(int conn_id,  uint8_t *value, tls_bt_addr_t *bda, uint16_t handle,
                             uint16_t len, uint8_t is_notify)
{
    int index = -1;
    //TLS_BT_APPL_TRACE_VERBOSE("%s, is_notify=%d, conn_id=%d,len=%d, handle=%d\r\n", __FUNCTION__, is_notify,  conn_id,len, handle);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, is_notify=%d, conn_id=%d,len=%d, handle=%d\r\n", __FUNCTION__,
                                is_notify,  conn_id, len, handle);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, notify_cb, conn_id, value, bda, handle, len, is_notify);
}

/** Reports result of a GATT read operation */
void btgattc_read_characteristic_callback(int conn_id, int status,
        uint16_t handle, uint8_t *value, int len, int value_type, int rstatus)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s, status=%d, conn_id=%d,handle=%d,len=%d\r\n", __FUNCTION__, status,
                              conn_id, handle, len);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, status=%d, conn_id=%d,handle=%d,len=%d\r\n", __FUNCTION__, status,
                                conn_id, handle, len);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, read_characteristic_cb, conn_id, status, handle, value,
                  len, value_type, rstatus);
}

/** GATT write characteristic operation callback */
void btgattc_write_characteristic_callback(int conn_id, int status, uint16_t handle)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, status=%d, conn_id=%d,handle=%d\r\n", __FUNCTION__, status, conn_id,
                                handle);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, write_characteristic_cb, conn_id, status, handle);
}

/** GATT execute prepared write callback */
void btgattc_execute_write_callback(int conn_id, int status)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, status=%d, conn_id=%d\r\n", __FUNCTION__, status, conn_id);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, execute_write_cb, conn_id, status);
}

/** Callback invoked in response to read_descriptor */
void btgattc_read_descriptor_callback(int conn_id, int status, uint16_t handle, uint8_t *value,
                                      int len, int value_type, int rstatus)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s, status=%d, conn_id=%d,len=%d\r\n", __FUNCTION__, status, conn_id,
                              len);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, status=%d, conn_id=%d,len=%d\r\n", __FUNCTION__, status, conn_id, len);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, read_descriptor_cb, conn_id, status, handle, value,
                  len, value_type, rstatus);
}

/** Callback invoked in response to write_descriptor */
void btgattc_write_descriptor_callback(int conn_id, int status, uint16_t handle)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, status=%d, conn_id=%d,handle=%d\r\n", __FUNCTION__, status, conn_id,
                                handle);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, write_descriptor_cb, conn_id, status, handle);
}

/**
 * Callback indicating the status of a listen() operation
 */
void btgattc_listen_callback(int status, int server_if)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s, server_if=%d\r\n", __FUNCTION__, server_if);
    index = get_app_env_index_by_client_if(server_if);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, status=%d, server_if=%d\r\n", __FUNCTION__, status, server_if);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, listen_cb, status, server_if);
}

/** Callback invoked when the MTU for a given connection changes */
void btgattc_configure_mtu_callback(int conn_id, int status, int mtu)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s, status=%d, conn_id=%d,mtu=%d\r\n", __FUNCTION__, status, conn_id,
                              mtu);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, status=%d, conn_id=%d,mtu=%d\r\n", __FUNCTION__, status, conn_id, mtu);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, configure_mtu_cb, conn_id, status, mtu);
}

/**
 * Callback notifying an application that a remote device connection is currently congested
 * and cannot receive any more data. An application should avoid sending more data until
 * a further callback is received indicating the congestion status has been cleared.
 */
void btgattc_congestion_callback(int conn_id, uint8_t congested)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, congested=%d, conn_id=%d\r\n", __FUNCTION__, congested, conn_id);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, congestion_cb, conn_id, congested);
}


/** GATT get database callback */
void btgattc_get_gatt_db_callback(int status, int conn_id, tls_btgatt_db_element_t *db, int count)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s, status=%d, conn_id=%d, count=%d\r\n", __FUNCTION__, status, conn_id,
                              count);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, count=%d, conn_id=%d\r\n", __FUNCTION__, count, conn_id);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, get_gatt_db_cb, status, conn_id, db, count);
}

/** GATT services between start_handle and end_handle were removed */
void btgattc_services_removed_callback(int conn_id, uint16_t start_handle, uint16_t end_handle)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s, conn_id=%d,start_handle=%d,end_handle=%d\r\n", __FUNCTION__, conn_id,
                              start_handle, end_handle);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, conn_id=%d,start_handle=%d,end_handle=%d\r\n", __FUNCTION__, conn_id,
                                start_handle, end_handle);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, services_removed_cb, conn_id, start_handle, end_handle);
}

/** GATT services were added */
void btgattc_services_added_callback(int conn_id, tls_btgatt_db_element_t *added, int added_count)
{
    int index = -1;
    TLS_BT_APPL_TRACE_VERBOSE("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
    index = get_app_env_index_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, conn_id=%d,added_count=%d\r\n", __FUNCTION__, conn_id, added_count);
        return;
    }

    TLS_HAL_CBACK(app_env[index].ps_callbak, services_added_cb, conn_id, added, added_count);
}



static void tls_ble_client_event_handler(tls_ble_evt_t evt, tls_ble_msg_t *msg)
{
    //TLS_BT_APPL_TRACE_EVENT("%s, event:%s,%d\r\n", __FUNCTION__, tls_gatt_evt_2_str(evt), evt);
    tls_bt_addr_t addr;

    switch(evt) {
        case WM_BLE_CL_REGISTER_EVT:
            btgattc_register_client_callback(msg->cli_register.status, msg->cli_register.client_if,
                                             &msg->cli_register.app_uuid);
            break;

        case WM_BLE_CL_DEREGISTER_EVT:
            btgattc_deregister_client_callback(msg->cli_register.status, msg->cli_register.client_if);
            break;

        case WM_BLE_CL_OPEN_EVT:
            memcpy(addr.address, msg->cli_open.bd_addr, 6);
            btgattc_connect_callback(msg->cli_open.conn_id, msg->cli_open.status, msg->cli_open.client_if,
                                     &addr);
            break;

        case WM_BLE_CL_CLOSE_EVT:
            memcpy(addr.address, msg->cli_close.remote_bda, 6);
            btgattc_disconnect_callback(msg->cli_close.conn_id, msg->cli_close.status, msg->cli_close.reason,
                                        msg->cli_close.client_if, &addr);
            break;

        case WM_BLE_CL_SEARCH_CMPL_EVT:
            btgattc_search_complete_callback(msg->cli_search_cmpl.conn_id, msg->cli_search_cmpl.status);
            break;

        case WM_BLE_CL_SEARCH_RES_EVT:
            btgattc_search_service_result_callback(msg->cli_search_res.conn_id, &msg->cli_search_res.uuid,
                                                   msg->cli_search_res.inst_id);
            break;

        case WM_BLE_CL_REG_NOTIFY_EVT:
            btgattc_register_for_notification_callback(msg->cli_reg_notify.conn_id, msg->cli_reg_notify.reg,
                    msg->cli_reg_notify.status, msg->cli_reg_notify.handle);
            break;

        case WM_BLE_CL_NOTIF_EVT:
            memcpy(addr.address, msg->cli_notif.bda, 6);
            btgattc_notify_callback(msg->cli_notif.conn_id, msg->cli_notif.value, &addr, msg->cli_notif.handle,
                                    msg->cli_notif.len, msg->cli_notif.is_notify);
            break;

        case WM_BLE_CL_READ_CHAR_EVT:
            btgattc_read_characteristic_callback(msg->cli_read.conn_id, msg->cli_read.status,
                                                 msg->cli_read.handle, msg->cli_read.value, msg->cli_read.len, msg->cli_read.value_type,
                                                 msg->cli_read.status);
            break;

        case WM_BLE_CL_WRITE_CHAR_EVT:
            btgattc_write_characteristic_callback(msg->cli_write.conn_id, msg->cli_write.status,
                                                  msg->cli_write.handle);
            break;

        case WM_BLE_CL_EXEC_CMPL_EVT:
            btgattc_execute_write_callback(msg->cli_write.conn_id, msg->cli_write.status);
            break;

        case WM_BLE_CL_READ_DESCR_EVT:
            btgattc_read_descriptor_callback(msg->cli_read.conn_id, msg->cli_read.status, msg->cli_read.handle,
                                             msg->cli_read.value, msg->cli_read.len, msg->cli_read.value_type, msg->cli_read.status);
            break;

        case WM_BLE_CL_WRITE_DESCR_EVT:
            btgattc_write_descriptor_callback(msg->cli_write.conn_id, msg->cli_write.status,
                                              msg->cli_write.handle);
            break;

        case WM_BLE_CL_LISTEN_EVT:
            btgattc_listen_callback(msg->cli_listen.status, msg->cli_listen.client_if);
            break;

        case WM_BLE_CL_CFG_MTU_EVT:
            btgattc_configure_mtu_callback(msg->cli_cfg_mtu.conn_id, msg->cli_cfg_mtu.status,
                                           msg->cli_cfg_mtu.mtu);
            break;

        case WM_BLE_CL_CONGEST_EVT:
            btgattc_congestion_callback(msg->cli_congest.conn_id, msg->cli_congest.congested);
            break;

        case WM_BLE_CL_REPORT_DB_EVT:
            btgattc_get_gatt_db_callback(msg->cli_db.status, msg->cli_db.conn_id, msg->cli_db.db,
                                         msg->cli_db.count);
            break;

        default:
            TLS_BT_APPL_TRACE_WARNING("warning, unknow ble client evt=%d\r\n", evt);
            break;
    }
}
/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

tls_bt_status_t tls_ble_client_init()
{
    memset(&app_env, 0, sizeof(app_ble_client_t)*GATT_MAX_CNT_SUPPORT);
    tls_ble_client_app_init(tls_ble_client_event_handler);
    return TLS_BT_STATUS_SUCCESS;
}

tls_bt_status_t tls_ble_client_deinit()
{
    tls_ble_client_app_deinit();
    return TLS_BT_STATUS_SUCCESS;
}

tls_bt_status_t tls_ble_client_register_client(uint16_t app_uuid,
        wm_ble_client_callbacks_t *callback)
{
    int index = -1;
    tls_bt_status_t status;
    index = get_app_env_index_by_uuid(app_uuid);

    if(index >= 0) {
        TLS_BT_APPL_TRACE_WARNING("0x%04x, already registered\r\n", app_uuid)
        return TLS_BT_STATUS_DONE;
    }

    index = get_free_app_env_index();

    if(index < 0) {
        return TLS_BT_STATUS_NOMEM;
    }

    app_env[index].in_use = 1;
    app_env[index].uuid = app_uuid;
    app_env[index].ps_callbak = callback;
    status = tls_ble_client_app_register(app_uuid16_to_uuid128(app_uuid));

    if(status != TLS_BT_STATUS_SUCCESS) {
        app_env[index].in_use = 0;
    }

    return status;
}


/** Unregister a client application from the stack */
tls_bt_status_t tls_ble_client_unregister_client(int client_if)
{
    int index = -1;
    index = get_app_env_index_by_client_if(client_if);

    if(index < 0) {
        return TLS_BT_STATUS_PARM_INVALID;
    }

    return tls_ble_client_app_unregister(client_if);
}

#endif
