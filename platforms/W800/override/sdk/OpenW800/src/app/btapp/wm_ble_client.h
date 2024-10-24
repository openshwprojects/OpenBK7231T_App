#ifndef __WM_BLE_CLIENT_H__
#define __WM_BLE_CLIENT_H__

#include "wm_bt_def.h"
/** WM-BT-GATT Client callback structure. */

/** Callback invoked in response to register_client */
typedef void (*wm_ble_client_register_client_callback)(int status, int client_if,
        uint16_t app_uuid);
typedef void (*wm_ble_client_deregister_client_callback)(int status, int client_if);


/** GATT open callback invoked in response to open */
typedef void (*wm_ble_client_connect_callback)(int conn_id, int status, int client_if,
        tls_bt_addr_t *bda);

/** Callback invoked in response to close */
typedef void (*wm_ble_client_disconnect_callback)(int conn_id, int status, int reason,
        int client_if, tls_bt_addr_t *bda);

/**
 * Invoked in response to search_service when the GATT service search
 * has been completed.
 */
typedef void (*wm_ble_client_search_complete_callback)(int conn_id, int status);

typedef void (*wm_ble_client_search_srvc_res_callback)(int conn_id, uint16_t app_uuid,
        uint8_t inst_id);

/** Callback invoked in response to [de]register_for_notification */
typedef void (*wm_ble_client_register_for_notification_callback)(int conn_id,
        int registered, int status, uint16_t handle);

/**
 * Remote device notification callback, invoked when a remote device sends
 * a notification or indication that a client has registered for.
 */
typedef void (*wm_ble_client_notify_callback)(int conn_id, uint8_t *p_value, tls_bt_addr_t *bda,
        uint16_t handle, uint16_t len, uint8_t is_notify);

/** Reports result of a GATT read operation */
typedef void (*wm_ble_client_read_characteristic_callback)(int conn_id, int status, uint16_t handle,
        uint8_t *p_value, uint16_t length, uint16_t value_type, uint8_t pa_status);

/** GATT write characteristic operation callback */
typedef void (*wm_ble_client_write_characteristic_callback)(int conn_id, int status,
        uint16_t handle);

/** GATT execute prepared write callback */
typedef void (*wm_ble_client_execute_write_callback)(int conn_id, int status);

/** Callback invoked in response to read_descriptor */
typedef void (*wm_ble_client_read_descriptor_callback)(int conn_id, int status, uint16_t handle,
        uint8_t *p_value, uint16_t length, uint16_t value_type, uint8_t pa_status);

/** Callback invoked in response to write_descriptor */
typedef void (*wm_ble_client_write_descriptor_callback)(int conn_id, int status, uint16_t handle);

/** Callback triggered in response to read_remote_rssi */
typedef void (*wm_ble_client_read_remote_rssi_callback)(int client_if, tls_bt_addr_t *bda,
        int rssi, int status);

/**
 * Callback indicating the status of a listen() operation
 */
typedef void (*wm_ble_client_listen_callback)(int status, int server_if);

/** Callback invoked when the MTU for a given connection changes */
typedef void (*wm_ble_client_configure_mtu_callback)(int conn_id, int status, int mtu);

/**
 * Callback notifying an application that a remote device connection is currently congested
 * and cannot receive any more data. An application should avoid sending more data until
 * a further callback is received indicating the congestion status has been cleared.
 */
typedef void (*wm_ble_client_congestion_callback)(int conn_id, uint8_t congested);


/** Callback invoked when scan parameter setup has completed */
typedef void (*wm_ble_client_scan_parameter_setup_completed_callback)(int client_if,
        uint8_t status);

/** GATT get database callback */
typedef void (*wm_ble_client_get_gatt_db_callback)(int status, int conn_id,
        tls_btgatt_db_element_t *db, int count);

/** GATT services between start_handle and end_handle were removed */
typedef void (*wm_ble_client_services_removed_callback)(int conn_id, uint16_t start_handle,
        uint16_t end_handle);

/** GATT services were added */
typedef void (*wm_ble_client_services_added_callback)(int conn_id, tls_btgatt_db_element_t *added,
        int added_count);

typedef struct {
    wm_ble_client_register_client_callback            register_client_cb;
    wm_ble_client_deregister_client_callback          deregister_client_cb;
    wm_ble_client_connect_callback                    open_cb;
    wm_ble_client_disconnect_callback                 close_cb;
    wm_ble_client_search_complete_callback            search_complete_cb;
    wm_ble_client_search_srvc_res_callback            search_serv_res_cb;
    wm_ble_client_register_for_notification_callback  register_for_notification_cb;
    wm_ble_client_notify_callback                     notify_cb;
    wm_ble_client_read_characteristic_callback        read_characteristic_cb;
    wm_ble_client_write_characteristic_callback       write_characteristic_cb;
    wm_ble_client_read_descriptor_callback            read_descriptor_cb;
    wm_ble_client_write_descriptor_callback           write_descriptor_cb;
    wm_ble_client_execute_write_callback              execute_write_cb;
    wm_ble_client_read_remote_rssi_callback           read_remote_rssi_cb;
    wm_ble_client_listen_callback                     listen_cb;
    wm_ble_client_configure_mtu_callback              configure_mtu_cb;
    wm_ble_client_congestion_callback                 congestion_cb;
    wm_ble_client_get_gatt_db_callback                get_gatt_db_cb;
    wm_ble_client_services_removed_callback           services_removed_cb;
    wm_ble_client_services_added_callback             services_added_cb;

} wm_ble_client_callbacks_t;

/** Represents the standard BT-GATT client interface. */

/** Registers a wm GATT client application with the interface with stack */
tls_bt_status_t tls_ble_client_register_client(uint16_t app_uuid,
        wm_ble_client_callbacks_t *client_callback);

/** Unregister a client application from the stack */
tls_bt_status_t tls_ble_client_unregister_client(int client_if);

tls_bt_status_t tls_ble_client_init();
tls_bt_status_t tls_ble_client_deinit();


#endif

