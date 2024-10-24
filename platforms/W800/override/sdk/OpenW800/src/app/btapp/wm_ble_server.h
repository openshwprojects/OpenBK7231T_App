#ifndef __WM_BLE_SERVER_H__
#define __WM_BLE_SERVER_H__

#include "wm_bt_def.h"

/** Callback invoked in response to register_server */
typedef void (*wm_ble_server_register_server_callback)(int status, int server_if,
        uint16_t app_uuid);
typedef void (*wm_ble_server_deregister_server_callback)(int status, int server_if);


/** Callback indicating that a remote device has connected or been disconnected */
typedef void (*wm_ble_server_connection_callback)(int conn_id, int server_if, int connected,
        tls_bt_addr_t *bda, uint16_t reason);

/** Callback invoked in response to create_service */
typedef void (*wm_ble_server_service_added_callback)(int status, int server_if,
        int inst_id, int primary, uint16_t uuid, int srvc_handle);

/** Callback indicating that an included service has been added to a service */
typedef void (*wm_ble_server_included_service_added_callback)(int status, int server_if,
        int srvc_handle, int incl_srvc_handle);

/** Callback invoked when a characteristic has been added to a service */
typedef void (*wm_ble_server_characteristic_added_callback)(int status, int server_if,
        uint16_t uuid, int srvc_handle, int char_handle);

/** Callback invoked when a descriptor has been added to a characteristic */
typedef void (*wm_ble_server_descriptor_added_callback)(int status, int server_if,
        uint16_t uuid, int srvc_handle, int descr_handle);

/** Callback invoked in response to start_service */
typedef void (*wm_ble_server_service_started_callback)(int status, int server_if,
        int srvc_handle);

/** Callback invoked in response to stop_service */
typedef void (*wm_ble_server_service_stopped_callback)(int status, int server_if,
        int srvc_handle);

/** Callback triggered when a service has been deleted */
typedef void (*wm_ble_server_service_deleted_callback)(int status, int server_if,
        int srvc_handle);

/**
 * Callback invoked when a remote device has requested to read a characteristic
 * or descriptor. The application must respond by calling send_response
 */
typedef void (*wm_ble_server_request_read_callback)(int conn_id, int trans_id, tls_bt_addr_t *bda,
        int attr_handle, int offset, uint8_t is_long);

/**
 * Callback invoked when a remote device has requested to write to a
 * characteristic or descriptor.
 */
typedef void (*wm_ble_server_request_write_callback)(int conn_id, int trans_id, tls_bt_addr_t *bda,
        int attr_handle, int offset, int length,
        uint8_t need_rsp, uint8_t is_prep, uint8_t *value);

/** Callback invoked when a previously prepared write is to be executed */
typedef void (*wm_ble_server_request_exec_write_callback)(int conn_id, int trans_id,
        tls_bt_addr_t *bda, int exec_write);

/**
 * Callback triggered in response to send_response if the remote device
 * sends a confirmation.
 */
typedef void (*wm_ble_server_response_confirmation_callback)(int status, int conn_id, int trans_id);

/**
 * Callback confirming that a notification or indication has been sent
 * to a remote device.
 */
typedef void (*wm_ble_server_indication_sent_callback)(int conn_id, int status);

/**
 * Callback notifying an application that a remote device connection is currently congested
 * and cannot receive any more data. An application should avoid sending more data until
 * a further callback is received indicating the congestion status has been cleared.
 */
typedef void (*wm_ble_server_congestion_callback)(int conn_id, uint8_t congested);

/** Callback invoked when the MTU for a given connection changes */
typedef void (*wm_ble_server_mtu_changed_callback)(int conn_id, int mtu);


typedef struct {
    wm_ble_server_register_server_callback        register_server_cb;
    wm_ble_server_deregister_server_callback      deregister_server_cb;
    wm_ble_server_connection_callback             connection_cb;
    wm_ble_server_service_added_callback          service_added_cb;
    wm_ble_server_included_service_added_callback included_service_added_cb;
    wm_ble_server_characteristic_added_callback   characteristic_added_cb;
    wm_ble_server_descriptor_added_callback       descriptor_added_cb;
    wm_ble_server_service_started_callback        service_started_cb;
    wm_ble_server_service_stopped_callback        service_stopped_cb;
    wm_ble_server_service_deleted_callback        service_deleted_cb;
    wm_ble_server_request_read_callback           request_read_cb;
    wm_ble_server_request_write_callback          request_write_cb;
    wm_ble_server_request_exec_write_callback     request_exec_write_cb;
    wm_ble_server_response_confirmation_callback  response_confirmation_cb;
    wm_ble_server_indication_sent_callback        indication_sent_cb;
    wm_ble_server_congestion_callback             congestion_cb;
    wm_ble_server_mtu_changed_callback            mtu_changed_cb;
} wm_ble_server_callbacks_t;

/*Init the GATT server application*/
int tls_ble_server_init();
int tls_ble_server_deinit();


/** Registers a GATT server application with the stack */
int tls_ble_server_register_server(uint16_t app_uuid, wm_ble_server_callbacks_t *callback);

/** Unregister a server application from the stack */
int tls_ble_server_unregister_server(int server_if);

#endif

