#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"

#if (WM_BLE_INCLUDED == CFG_ON)


#include "wm_ble_client.h"
#include "wm_ble_client_demo.h"
#include "wm_ble_gap.h"
#include "wm_bt_util.h"

static tls_ble_callback_t tls_demo_at_cb_ptr;

/** Callback invoked in response to register_client */
void ble_client_demo_register_client_callback(int status, int client_if,
        uint16_t app_uuid)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, status=%d,client_if=%d,uuid=0x%04x\r\n", __FUNCTION__, status,
                            client_if, app_uuid);
    tls_ble_msg_t msg;
    msg.cli_register.status = status;
    msg.cli_register.client_if = client_if;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_CL_REGISTER_EVT, &msg); }
}
void ble_client_demo_deregister_client_callback(int status, int client_if)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, status=%d,client_if=%d\r\n", __FUNCTION__, status, client_if);
    tls_ble_msg_t msg;
    msg.cli_register.status = status;
    msg.cli_register.client_if = client_if;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_CL_DEREGISTER_EVT, &msg); }

    tls_demo_at_cb_ptr = NULL;
}


/** GATT open callback invoked in response to open */
void ble_client_demo_connect_callback(int conn_id, int status, int client_if, tls_bt_addr_t *bda)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, status=%d,client_if=%d,conn_id=%d\r\n", __FUNCTION__, status,
                            client_if, conn_id);
    tls_ble_msg_t msg;
    memcpy(msg.cli_open.bd_addr, bda->address, 6);
    msg.cli_open.client_if = client_if;
    msg.cli_open.conn_id = conn_id;
    msg.cli_open.status = status;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_CL_OPEN_EVT, &msg); }
}

/** Callback invoked in response to close */
void ble_client_demo_disconnect_callback(int conn_id, int status, int reason,
        int client_if, tls_bt_addr_t *bda)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, status = %d, reason=%d, conn_id=%d\r\n", __FUNCTION__, status, reason,
                            conn_id);
    tls_ble_msg_t msg;
    msg.cli_close.client_if = client_if;
    msg.cli_close.conn_id = conn_id;
    msg.cli_close.status = status;
    msg.cli_close.reason = reason;
    memcpy(msg.cli_close.remote_bda, bda->address, 6);

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_CL_CLOSE_EVT, &msg); }
}

/**
 * Invoked in response to search_service when the GATT service search
 * has been completed.
 */
void ble_client_demo_search_complete_callback(int conn_id, int status)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d, status=%d\r\n", __FUNCTION__, conn_id, status);
    tls_ble_msg_t msg;
    msg.cli_search_cmpl.conn_id = conn_id;
    msg.cli_search_cmpl.status = status;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_CL_SEARCH_CMPL_EVT, &msg); }
}

void ble_client_demo_search_service_result_callback(int conn_id, tls_bt_uuid_t *p_uuid,
        uint8_t inst_id)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

/** Callback invoked in response to [de]register_for_notification */
void ble_client_demo_register_for_notification_callback(int conn_id,
        int registered, int status, uint16_t handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d, registered=%d, handle=%d\r\n", __FUNCTION__, conn_id,
                            registered, handle);
    tls_ble_msg_t msg;
    msg.cli_reg_notify.conn_id = conn_id;
    msg.cli_reg_notify.handle = handle;
    msg.cli_reg_notify.reg = registered;
    msg.cli_reg_notify.status = status;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_CL_REG_NOTIFY_EVT, &msg); }
}

/**
 * Remote device notification callback, invoked when a remote device sends
 * a notification or indication that a client has registered for.
 */
void ble_client_demo_notify_callback(int conn_id, uint8_t *value, tls_bt_addr_t *addr,
                                     uint16_t handle, uint16_t len, uint8_t is_notify)
{
    TLS_BT_APPL_TRACE_DEBUG("%s\r\n", __FUNCTION__);
    tls_ble_msg_t msg;
    memcpy(msg.cli_notif.bda, addr->address, 6);
    msg.cli_notif.conn_id = conn_id;
    msg.cli_notif.handle = handle;
    msg.cli_notif.is_notify = is_notify;
    msg.cli_notif.len = len;
    msg.cli_notif.value = value;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_CL_NOTIF_EVT, &msg); }
}

/** Reports result of a GATT read operation */
void ble_client_demo_read_characteristic_callback(int conn_id, int status,
        uint16_t handle, uint8_t *value, int length, uint16_t value_type, uint8_t p_status)
{
    //hci_dbg_hexstring("read out:", value, length);
    TLS_BT_APPL_TRACE_DEBUG("%s\r\n", __FUNCTION__);
    tls_ble_msg_t msg;
    msg.cli_read.conn_id = conn_id;
    msg.cli_read.handle = handle;
    msg.cli_read.len = length;
    msg.cli_read.status = status;
    msg.cli_read.value = value;
    msg.cli_read.value_type = value_type;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_CL_READ_CHAR_EVT, &msg); }
}

/** GATT write characteristic operation callback */
void ble_client_demo_write_characteristic_callback(int conn_id, int status, uint16_t handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d,handle=%d\r\n", __FUNCTION__, conn_id, handle);
    tls_ble_msg_t msg;
    msg.cli_write.conn_id = conn_id;
    msg.cli_write.handle = handle;
    msg.cli_write.status = status;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_CL_WRITE_CHAR_EVT, &msg); }
}

/** GATT execute prepared write callback */
void ble_client_demo_execute_write_callback(int conn_id, int status)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

/** Callback invoked in response to read_descriptor */
void ble_client_demo_read_descriptor_callback(int conn_id, int status, uint16_t handle,
        uint8_t *p_value, uint16_t length, uint16_t value_type, uint8_t pa_status)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d, handle=%d\r\n", __FUNCTION__, conn_id, handle);
    //hci_dbg_hexstring("value", p_value, length);
    tls_ble_msg_t msg;
    msg.cli_read.conn_id = conn_id;
    msg.cli_read.handle = handle;
    msg.cli_read.len = length;
    msg.cli_read.status = status;
    msg.cli_read.value = p_value;
    msg.cli_read.value_type = value_type;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_CL_READ_DESCR_EVT, &msg); }
}

/** Callback invoked in response to write_descriptor */
void ble_client_demo_write_descriptor_callback(int conn_id, int status, uint16_t handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
    tls_ble_msg_t msg;
    msg.cli_write.conn_id = conn_id;
    msg.cli_write.handle = handle;
    msg.cli_write.status = status;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_CL_WRITE_DESCR_EVT, &msg); }
}

/** Callback triggered in response to read_remote_rssi */
void ble_client_demo_read_remote_rssi_callback(int client_if, tls_bt_addr_t *bda,
        int rssi, int status)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, client_if=%d\r\n", __FUNCTION__, client_if);
}

/**
 * Callback indicating the status of a listen() operation
 */
void ble_client_demo_listen_callback(int status, int server_if)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, server_if=%d\r\n", __FUNCTION__, server_if);
    tls_ble_msg_t msg;
    msg.cli_listen.client_if = server_if;
    msg.cli_listen.status = status;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_CL_LISTEN_EVT, &msg); }
}

/** Callback invoked when the MTU for a given connection changes */
void ble_client_demo_configure_mtu_callback(int conn_id, int status, int mtu)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
    tls_ble_msg_t msg;
    msg.cli_cfg_mtu.conn_id = conn_id;
    msg.cli_cfg_mtu.status = status;
    msg.cli_cfg_mtu.mtu = mtu;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_CL_CFG_MTU_EVT, &msg); }
}

/**
 * Callback notifying an application that a remote device connection is currently congested
 * and cannot receive any more data. An application should avoid sending more data until
 * a further callback is received indicating the congestion status has been cleared.
 */
void ble_client_demo_congestion_callback(int conn_id, uint8_t congested)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
    tls_ble_msg_t msg;
    msg.cli_congest.congested = congested;
    msg.cli_congest.conn_id = conn_id;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_CL_CONGEST_EVT, &msg); }
}

/** GATT get database callback */
void ble_client_demo_get_gatt_db_callback(int status, int conn_id, tls_btgatt_db_element_t *db,
        int count)
{
    tls_ble_msg_t msg;
    TLS_BT_APPL_TRACE_DEBUG("===========btgattc_get_gatt_db_callback(count=%d)(conn_id=%d)================\r\n",
                            count, conn_id);
#if 0
    int i = 0;
    uint16_t cared_handle, tmp_uuid;

    for(i = 0; i < count; i++) {
        if(db->type == 0) {
            hci_dbg_hexstring("#", db->uuid.uu + 12, 2);
            hci_dbg_msg("type:%d, attr_handle:%d, properties:0x%02x, s=%d, e=%d\r\n", db->type,
                        db->attribute_handle, db->properties, db->start_handle, db->end_handle);
        } else {
            hci_dbg_hexstring("\t#", db->uuid.uu + 12, 2);
            tmp_uuid = db->uuid.uu[12] << 8 | db->uuid.uu[13];

            if(tmp_uuid == 0xBC2A) {
                cared_handle = db->attribute_handle;
            }

            hci_dbg_msg("\ttype:%d, attr_handle:%d, properties:0x%02x, s=%d, e=%d\r\n", db->type,
                        db->attribute_handle, db->properties, db->start_handle, db->end_handle);
        }

        db++;
    }

#endif
    msg.cli_db.conn_id = conn_id;
    msg.cli_db.count = count;
    msg.cli_db.db = db;
    msg.cli_db.status = status;

    if(tls_demo_at_cb_ptr) { (tls_demo_at_cb_ptr)(WM_BLE_CL_REPORT_DB_EVT, &msg); }
}

/** GATT services between start_handle and end_handle were removed */
void ble_client_demo_services_removed_callback(int conn_id, uint16_t start_handle,
        uint16_t end_handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

/** GATT services were added */
void ble_client_demo_services_added_callback(int conn_id, tls_btgatt_db_element_t *added,
        int added_count)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

static const wm_ble_client_callbacks_t  swmbleclientcb = {
    ble_client_demo_register_client_callback,
    ble_client_demo_deregister_client_callback,
    ble_client_demo_connect_callback,
    ble_client_demo_disconnect_callback,
    ble_client_demo_search_complete_callback,
    ble_client_demo_search_service_result_callback,
    ble_client_demo_register_for_notification_callback,
    ble_client_demo_notify_callback,
    ble_client_demo_read_characteristic_callback,
    ble_client_demo_write_characteristic_callback,
    ble_client_demo_read_descriptor_callback,
    ble_client_demo_write_descriptor_callback,
    ble_client_demo_execute_write_callback,
    ble_client_demo_read_remote_rssi_callback,
    ble_client_demo_listen_callback,
    ble_client_demo_configure_mtu_callback,
    ble_client_demo_congestion_callback,
    ble_client_demo_get_gatt_db_callback,
    ble_client_demo_services_removed_callback,
    ble_client_demo_services_added_callback,
} ;

int tls_ble_demo_cli_init(uint16_t demo_uuid, tls_ble_callback_t at_cb_ptr)
{
    tls_bt_status_t status;

    if(tls_demo_at_cb_ptr) {
        TLS_BT_APPL_TRACE_WARNING("%s, done already\r\n", __FUNCTION__);
        return TLS_BT_STATUS_DONE;
    }

    tls_demo_at_cb_ptr = at_cb_ptr;
    status = tls_ble_client_register_client(demo_uuid, &swmbleclientcb);

    if(status != TLS_BT_STATUS_SUCCESS) {
        tls_demo_at_cb_ptr = NULL;
        TLS_BT_APPL_TRACE_ERROR("%s, failed, clear the tls_demo_at_cb_ptr\r\n", __FUNCTION__);
    }

    return status;
}

int tls_ble_demo_cli_deinit(int client_if)
{
    if(tls_demo_at_cb_ptr == NULL) {
        TLS_BT_APPL_TRACE_WARNING("%s, done already\r\n", __FUNCTION__);
        return TLS_BT_STATUS_DONE;
    }

    return tls_ble_client_unregister_client(client_if);
}


#endif


