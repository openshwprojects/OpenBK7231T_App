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


#ifndef ANDROID_INCLUDE_BT_GATT_H
#define ANDROID_INCLUDE_BT_GATT_H

#include <stdint.h>
#include "bt_gatt_client.h"
#include "bt_gatt_server.h"
#include "bt_gatt_dm.h"


/** BT-GATT callbacks */
typedef struct {
    /** Set to sizeof(btgatt_callbacks_t) */
    size_t size;

    const btgatt_dm_callbacks_t *dm;
    int dm_ref_count;

    /** GATT Client callbacks */
    const btgatt_client_callbacks_t *client;
    int client_ref_count;

    /** GATT Server callbacks */
    const btgatt_server_callbacks_t *server;
    int server_ref_count;

    /** GATT valid type*/
    int valid_type;
} btgatt_callbacks_t;

/** Represents the standard Bluetooth GATT interface. */
typedef struct {
    /** Set to sizeof(btgatt_interface_t) */
    size_t          size;

    /**
     * Initializes the interface and provides callback routines
     */
    tls_bt_status_t (*init)(const btgatt_callbacks_t *callbacks, int valid_type);

    /** Closes the interface */
    void (*cleanup)(int valid_type);

    /** Pointer to the GATT client interface methods.*/
    const btgatt_client_interface_t *client;

    /** Pointer to the GATT server interface methods.*/
    const btgatt_server_interface_t *server;
} btgatt_interface_t;

/** Add one BLE dm interface for GATT. */
typedef struct {
    /** Set to sizeof(btgatt_dm_interface_t)*/

    /**
    *Set ble adv/scan rsp paramters
    */
    tls_bt_status_t (*btgatt_set_adv_or_scan_rsp)(tls_ble_dm_adv_data_t *p_data);

    /**
    *Set ble adv param
    */
    tls_bt_status_t (*btgatt_set_adv_param)(tls_ble_dm_adv_param_t *p_param);

    /**
    *Disable adv
    */
    tls_bt_status_t (*btgatt_disable_adv)(uint8_t id);
    /**
    *Set ble discoverable or connectable;
    */
    tls_bt_status_t (*btgatt_set_visibility)(bool discoverable, bool connectable);

    /**
     * Set BLE privacy
     */
    tls_bt_status_t (*btgatt_dm_set_privacy)(uint8_t enable);
    /**
    *
    *Start an app timer
    */
    int (*btgatt_get_timer_id)();
    tls_bt_status_t (*btgatt_start_timer)(uint8_t id, uint32_t timeout_ms, void *p_callback_ptr);
    tls_bt_status_t (*btgatt_stop_timer)(uint8_t id);
    void (*btgatt_free_timer_id)(uint8_t id);

    /**
    *app async processing handler
    */
    void (*btgatt_evt_triger)(int evt_id, void *p_callback_ptr);
    /**
    *Set data length
    */
    tls_bt_status_t (*btgatt_dm_set_data_length)(tls_bt_addr_t *addr, uint16_t length);

    tls_bt_status_t (*btgatt_dm_set_adv_policy)(uint8_t policy);

} btgatt_dm_interface_t;


#endif /* ANDROID_INCLUDE_BT_GATT_H */
