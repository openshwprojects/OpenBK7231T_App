/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef ANDROID_INCLUDE_BT_GATT_CLIENT_H
#define ANDROID_INCLUDE_BT_GATT_CLIENT_H

#include <stdint.h>
#include "bt_gatt_types.h"
#include "bt_common_types.h"



/**
 * Buffer sizes for maximum attribute length and maximum read/write
 * operation buffer size.
 */
#define BTGATT_MAX_ATTR_LEN 600

/** Buffer type for unformatted reads/writes */
typedef struct {
    uint8_t             value[BTGATT_MAX_ATTR_LEN];
    uint16_t            len;
} btgatt_unformatted_value_t;

/** Parameters for GATT read operations */
typedef struct {
    uint16_t           handle;
    btgatt_unformatted_value_t value;
    uint16_t            value_type;
    uint8_t             status;
} btgatt_read_params_t;

/** Parameters for GATT write operations */
typedef struct {
    btgatt_srvc_id_t    srvc_id;
    btgatt_gatt_id_t    char_id;
    btgatt_gatt_id_t    descr_id;
    uint8_t             status;
} btgatt_write_params_t;

/** Attribute change notification parameters */
typedef struct {
    uint8_t             value[BTGATT_MAX_ATTR_LEN];
    tls_bt_addr_t         bda;
    uint16_t            handle;
    uint16_t            len;
    uint8_t             is_notify;
} btgatt_notify_params_t;

typedef struct {
    uint8_t  client_if;
    uint8_t  action;
    uint8_t  filt_index;
    uint16_t feat_seln;
    uint16_t list_logic_type;
    uint8_t  filt_logic_type;
    uint8_t  rssi_high_thres;
    uint8_t  rssi_low_thres;
    uint8_t  dely_mode;
    uint16_t found_timeout;
    uint16_t lost_timeout;
    uint8_t  found_timeout_cnt;
    uint16_t  num_of_tracking_entries;
} btgatt_filt_param_setup_t;

typedef struct {
    tls_bt_addr_t        *bda1;
    tls_bt_uuid_t          *uuid1;
    uint16_t            u1;
    uint16_t            u2;
    uint16_t            u3;
    uint16_t            u4;
    uint16_t            u5;
} btgatt_test_params_t;

/* BT GATT client error codes */
typedef enum {
    BT_GATTC_COMMAND_SUCCESS = 0,    /* 0  Command succeeded                 */
    BT_GATTC_COMMAND_STARTED,        /* 1  Command started OK.               */
    BT_GATTC_COMMAND_BUSY,           /* 2  Device busy with another command  */
    BT_GATTC_COMMAND_STORED,         /* 3 request is stored in control block */
    BT_GATTC_NO_RESOURCES,           /* 4  No resources to issue command     */
    BT_GATTC_MODE_UNSUPPORTED,       /* 5  Request for 1 or more unsupported modes */
    BT_GATTC_ILLEGAL_VALUE,          /* 6  Illegal command /parameter value  */
    BT_GATTC_INCORRECT_STATE,        /* 7  Device in wrong state for request  */
    BT_GATTC_UNKNOWN_ADDR,           /* 8  Unknown remote BD address         */
    BT_GATTC_DEVICE_TIMEOUT,         /* 9  Device timeout                    */
    BT_GATTC_INVALID_CONTROLLER_OUTPUT,/* 10  An incorrect value was received from HCI */
    BT_GATTC_SECURITY_ERROR,          /* 11 Authorization or security failure or not authorized  */
    BT_GATTC_DELAYED_ENCRYPTION_CHECK, /*12 Delayed encryption check */
    BT_GATTC_ERR_PROCESSING           /* 12 Generic error                     */
} btgattc_error_t;

/** BT-GATT Client callback structure. */

/** Callback invoked in response to register_client */
typedef void (*register_client_callback)(int status, int client_if,
        tls_bt_uuid_t *app_uuid);

/** Callback for scan results */
typedef void (*scan_result_callback)(tls_bt_addr_t *bda, int rssi, uint8_t *adv_data);

/** GATT open callback invoked in response to open */
typedef void (*connect_callback)(int conn_id, int status, int client_if, tls_bt_addr_t *bda);

/** Callback invoked in response to close */
typedef void (*disconnect_callback)(int conn_id, int status,
                                    int client_if, tls_bt_addr_t *bda);

/**
 * Invoked in response to search_service when the GATT service search
 * has been completed.
 */
typedef void (*search_complete_callback)(int conn_id, int status);

typedef void (*search_srvc_res_callback)(int conn_id, tls_bt_uuid_t *uuid, uint8_t inst_id);

/** Callback invoked in response to [de]register_for_notification */
typedef void (*register_for_notification_callback)(int conn_id,
        int registered, int status, uint16_t handle);

/**
 * Remote device notification callback, invoked when a remote device sends
 * a notification or indication that a client has registered for.
 */
typedef void (*notify_callback)(int conn_id, btgatt_notify_params_t *p_data);

/** Reports result of a GATT read operation */
typedef void (*read_characteristic_callback)(int conn_id, int status,
        btgatt_read_params_t *p_data);

/** GATT write characteristic operation callback */
typedef void (*write_characteristic_callback)(int conn_id, int status, uint16_t handle);

/** GATT execute prepared write callback */
typedef void (*execute_write_callback)(int conn_id, int status);

/** Callback invoked in response to read_descriptor */
typedef void (*read_descriptor_callback)(int conn_id, int status,
        btgatt_read_params_t *p_data);

/** Callback invoked in response to write_descriptor */
typedef void (*write_descriptor_callback)(int conn_id, int status, uint16_t handle);

/** Callback triggered in response to read_remote_rssi */
typedef void (*read_remote_rssi_callback)(int client_if, tls_bt_addr_t *bda,
        int rssi, int status);

/**
 * Callback indicating the status of a listen() operation
 */
typedef void (*listen_callback)(int status, int server_if);

/** Callback invoked when the MTU for a given connection changes */
typedef void (*configure_mtu_callback)(int conn_id, int status, int mtu);

/** Callback invoked when a scan filter configuration command has completed */
typedef void (*scan_filter_cfg_callback)(int action, int client_if, int status, int filt_type,
        int avbl_space);

/** Callback invoked when scan param has been added, cleared, or deleted */
typedef void (*scan_filter_param_callback)(int action, int client_if, int status,
        int avbl_space);

/** Callback invoked when a scan filter configuration command has completed */
typedef void (*scan_filter_status_callback)(int enable, int client_if, int status);

/** Callback invoked when multi-adv enable operation has completed */
typedef void (*multi_adv_enable_callback)(int client_if, int status);

/** Callback invoked when multi-adv param update operation has completed */
typedef void (*multi_adv_update_callback)(int client_if, int status);

/** Callback invoked when multi-adv instance data set operation has completed */
typedef void (*multi_adv_data_callback)(int client_if, int status);

/** Callback invoked when multi-adv disable operation has completed */
typedef void (*multi_adv_disable_callback)(int client_if, int status);

/**
 * Callback notifying an application that a remote device connection is currently congested
 * and cannot receive any more data. An application should avoid sending more data until
 * a further callback is received indicating the congestion status has been cleared.
 */
typedef void (*congestion_callback)(int conn_id, uint8_t congested);
/** Callback invoked when batchscan storage config operation has completed */
typedef void (*batchscan_cfg_storage_callback)(int client_if, int status);

/** Callback invoked when batchscan enable / disable operation has completed */
typedef void (*batchscan_enable_disable_callback)(int action, int client_if, int status);

/** Callback invoked when batchscan reports are obtained */
typedef void (*batchscan_reports_callback)(int client_if, int status, int report_format,
        int num_records, int data_len, uint8_t *rep_data);

/** Callback invoked when batchscan storage threshold limit is crossed */
typedef void (*batchscan_threshold_callback)(int client_if);

/** Track ADV VSE callback invoked when tracked device is found or lost */
typedef void (*track_adv_event_callback)(btgatt_track_adv_info_t *p_track_adv_info);

/** Callback invoked when scan parameter setup has completed */
typedef void (*scan_parameter_setup_completed_callback)(int client_if,
        btgattc_error_t status);

/** GATT get database callback */
typedef void (*get_gatt_db_callback)(int conn_id, tls_btgatt_db_element_t *db, int count);

/** GATT services between start_handle and end_handle were removed */
typedef void (*services_removed_callback)(int conn_id, uint16_t start_handle, uint16_t end_handle);

/** GATT services were added */
typedef void (*services_added_callback)(int conn_id, tls_btgatt_db_element_t *added,
                                        int added_count);

typedef struct {
    register_client_callback            register_client_cb;
    scan_result_callback                scan_result_cb;
    connect_callback                    open_cb;
    disconnect_callback                 close_cb;
    search_complete_callback            search_complete_cb;
    search_srvc_res_callback            search_serv_res_cb;
    register_for_notification_callback  register_for_notification_cb;
    notify_callback                     notify_cb;
    read_characteristic_callback        read_characteristic_cb;
    write_characteristic_callback       write_characteristic_cb;
    read_descriptor_callback            read_descriptor_cb;
    write_descriptor_callback           write_descriptor_cb;
    execute_write_callback              execute_write_cb;
    read_remote_rssi_callback           read_remote_rssi_cb;
    listen_callback                     listen_cb;
    configure_mtu_callback              configure_mtu_cb;
    scan_filter_cfg_callback            scan_filter_cfg_cb;
    scan_filter_param_callback          scan_filter_param_cb;
    scan_filter_status_callback         scan_filter_status_cb;
    multi_adv_enable_callback           multi_adv_enable_cb;
    multi_adv_update_callback           multi_adv_update_cb;
    multi_adv_data_callback             multi_adv_data_cb;
    multi_adv_disable_callback          multi_adv_disable_cb;
    congestion_callback                 congestion_cb;
    batchscan_cfg_storage_callback      batchscan_cfg_storage_cb;
    batchscan_enable_disable_callback   batchscan_enb_disable_cb;
    batchscan_reports_callback          batchscan_reports_cb;
    batchscan_threshold_callback        batchscan_threshold_cb;
    track_adv_event_callback            track_adv_event_cb;
    scan_parameter_setup_completed_callback scan_parameter_setup_completed_cb;
    get_gatt_db_callback                get_gatt_db_cb;
    services_removed_callback           services_removed_cb;
    services_added_callback             services_added_cb;
} btgatt_client_callbacks_t;

/** Represents the standard BT-GATT client interface. */

typedef struct {
    /** Registers a GATT client application with the stack */
    tls_bt_status_t (*register_client)(tls_bt_uuid_t *uuid);

    /** Unregister a client application from the stack */
    tls_bt_status_t (*unregister_client)(int client_if);

    /** Start or stop LE device scanning */
    tls_bt_status_t (*scan)(uint8_t start);

    /** Create a connection to a remote LE or dual-mode device */
    tls_bt_status_t (*connect)(int client_if, const tls_bt_addr_t *bd_addr,
                               uint8_t is_direct, int transport);

    /** Disconnect a remote device or cancel a pending connection */
    tls_bt_status_t (*disconnect)(int client_if, const tls_bt_addr_t *bd_addr,
                                  int conn_id);

    /** Start or stop advertisements to listen for incoming connections */
    tls_bt_status_t (*listen)(int client_if, uint8_t start);

    /** Clear the attribute cache for a given device */
    tls_bt_status_t (*refresh)(int client_if, const tls_bt_addr_t *bd_addr);

    /**
     * Enumerate all GATT services on a connected device.
     * Optionally, the results can be filtered for a given UUID.
     */
    tls_bt_status_t (*search_service)(int conn_id, tls_bt_uuid_t *filter_uuid);

    /** Read a characteristic on a remote device */
    tls_bt_status_t (*read_characteristic)(int conn_id, uint16_t handle,
                                           int auth_req);

    /** Write a remote characteristic */
    tls_bt_status_t (*write_characteristic)(int conn_id, uint16_t handle,
                                            int write_type, int len, int auth_req,
                                            char *p_value);

    /** Read the descriptor for a given characteristic */
    tls_bt_status_t (*read_descriptor)(int conn_id, uint16_t handle, int auth_req);

    /** Write a remote descriptor for a given characteristic */
    tls_bt_status_t (*write_descriptor)(int conn_id, uint16_t handle,
                                        int write_type, int len,
                                        int auth_req, char *p_value);

    /** Execute a prepared write operation */
    tls_bt_status_t (*execute_write)(int conn_id, int execute);

    /**
     * Register to receive notifications or indications for a given
     * characteristic
     */
    tls_bt_status_t (*register_for_notification)(int client_if,
            const tls_bt_addr_t *bd_addr, uint16_t handle);

    /** Deregister a previous request for notifications/indications */
    tls_bt_status_t (*deregister_for_notification)(int client_if,
            const tls_bt_addr_t *bd_addr, uint16_t handle);

    /** Request RSSI for a given remote device */
    tls_bt_status_t (*read_remote_rssi)(int client_if, const tls_bt_addr_t *bd_addr);

    /** Setup scan filter params */
    tls_bt_status_t (*scan_filter_param_setup)(btgatt_filt_param_setup_t filt_param);


    /** Configure a scan filter condition  */
    tls_bt_status_t (*scan_filter_add_remove)(int client_if, int action, int filt_type,
            int filt_index, int company_id,
            int company_id_mask, const tls_bt_uuid_t *p_uuid,
            const tls_bt_uuid_t *p_uuid_mask, const tls_bt_addr_t *bd_addr,
            char addr_type, int data_len, char *p_data, int mask_len,
            char *p_mask);

    /** Clear all scan filter conditions for specific filter index*/
    tls_bt_status_t (*scan_filter_clear)(int client_if, int filt_index);

    /** Enable / disable scan filter feature*/
    tls_bt_status_t (*scan_filter_enable)(int client_if, uint8_t enable);

    /** Determine the type of the remote device (LE, BR/EDR, Dual-mode) */
    int (*get_device_type)(const tls_bt_addr_t *bd_addr);

    /** Set the advertising data or scan response data */
    tls_bt_status_t (*set_adv_data)(int client_if, uint8_t set_scan_rsp, uint8_t include_name,
                                    uint8_t include_txpower, int min_interval, int max_interval, int appearance,
                                    uint16_t manufacturer_len, char *manufacturer_data,
                                    uint16_t service_data_len, char *service_data,
                                    uint16_t service_uuid_len, char *service_uuid);

    /** Configure the MTU for a given connection */
    tls_bt_status_t (*configure_mtu)(int conn_id, int mtu);

    /** Request a connection parameter update */
    tls_bt_status_t (*conn_parameter_update)(const tls_bt_addr_t *bd_addr, int min_interval,
            int max_interval, int latency, int timeout);

    /** Sets the LE scan interval and window in units of N*0.625 msec */
    tls_bt_status_t (*set_scan_parameters)(int client_if, int scan_interval, int scan_window);

    /* Setup the parameters as per spec, user manual specified values and enable multi ADV */
    tls_bt_status_t (*multi_adv_enable)(int client_if, int min_interval, int max_interval, int adv_type,
                                        int chnl_map, int tx_power, int timeout_s);

    /* Update the parameters as per spec, user manual specified values and restart multi ADV */
    tls_bt_status_t (*multi_adv_update)(int client_if, int min_interval, int max_interval, int adv_type,
                                        int chnl_map, int tx_power, int timeout_s);

    /* Setup the data for the specified instance */
    tls_bt_status_t (*multi_adv_set_inst_data)(int client_if, uint8_t set_scan_rsp,
            uint8_t include_name,
            uint8_t incl_txpower, int appearance, int manufacturer_len,
            char *manufacturer_data, int service_data_len,
            char *service_data, int service_uuid_len, char *service_uuid);

    /* Disable the multi adv instance */
    tls_bt_status_t (*multi_adv_disable)(int client_if);

    /* Configure the batchscan storage */
    tls_bt_status_t (*batchscan_cfg_storage)(int client_if, int batch_scan_full_max,
            int batch_scan_trunc_max, int batch_scan_notify_threshold);

    /* Enable batchscan */
    tls_bt_status_t (*batchscan_enb_batch_scan)(int client_if, int scan_mode,
            int scan_interval, int scan_window, int addr_type, int discard_rule);

    /* Disable batchscan */
    tls_bt_status_t (*batchscan_dis_batch_scan)(int client_if);

    /* Read out batchscan reports */
    tls_bt_status_t (*batchscan_read_reports)(int client_if, int scan_mode);

    /** Test mode interface */
    tls_bt_status_t (*test_command)(int command, btgatt_test_params_t *params);

    /** Get gatt db content */
    tls_bt_status_t (*get_gatt_db)(int conn_id);

} btgatt_client_interface_t;



#endif /* ANDROID_INCLUDE_BT_GATT_CLIENT_H */
