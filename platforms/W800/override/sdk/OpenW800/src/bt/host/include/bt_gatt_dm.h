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


#ifndef ANDROID_INCLUDE_BT_GATT_DM_H
#define ANDROID_INCLUDE_BT_GATT_DM_H

#include <stdint.h>

#include "bt_gatt_types.h"

typedef enum {
    BTIF_GATT_DM_SET_ADV_DATA,
    BTIF_GATT_DM_SET_ADV_PARAM,
    BTIF_GATT_DM_SET_ADV_EXT_PARAM,
    BTIF_GATT_DM_UPDATE_ADV_PARAM,
    BTIF_GATT_DM_SET_ADV_ENABLE,
    BTIF_GATT_DM_SET_ADV_DISABLE,
    BTIF_GATT_DM_SET_PRIVACY,
    BTIF_GATT_DM_START_TIMER,
    BTIF_GATT_DM_STOP_TIMER,
    BTIF_GATT_DM_EVT_TRIGER,
    BTIF_GATT_DM_SET_DATA_LENGTH,
    BTIF_GATT_DM_SET_ADV_FILTER_POLICY,
    BTIF_GATT_DM_SCAN_START,
    BTIF_GATT_DM_SCAN_STOP,
    BTIF_GATT_DM_SET_SCAN_PARAMS,
    BTIF_GATT_DM_CONN_PARAM_UPDT,
    BTIF_GATT_DM_READ_RSSI,
    BTIF_GATT_DM_ADV_ACT,
    BTIF_GATT_DM_SEC_ACT
} btif_gatt_dm_event_t;

typedef struct {
    uint16_t disc;
    uint16_t conn;
} __attribute__((packed))  btif_dm_visibility_t;

typedef struct {
    uint8_t id;
    uint32_t ms;
    void *callback_ptr;
} __attribute__((packed))  btif_dm_timer_t;

typedef struct {
    int evt;
    void *priv_data;
} __attribute__((packed))  btif_dm_evt_t;


typedef struct {
    tls_bt_addr_t addr;
    uint16_t length;
} __attribute__((packed))  btif_dm_set_data_length_t;


typedef void (*adv_enable_callback)(int status);
typedef void (*adv_update_data_callback)(int status);
typedef void (*adv_config_data_callback)(int status);
typedef void (*adv_disable_callback)(int status);
typedef void (*dm_timeout_callback)(uint8_t id, int32_t func_ptr);
typedef void (*dm_triger_callback)(int evt_id, int32_t func_ptr);

typedef struct {
    adv_enable_callback        adv_enable_cb;
    adv_update_data_callback             adv_update_data_cb;
    adv_config_data_callback          adv_config_data_cb;
    adv_disable_callback adv_disable_cb;
    dm_timeout_callback  dm_timeout_cb;
    dm_triger_callback dm_triger_cb;

} btgatt_dm_callbacks_t;




#endif

