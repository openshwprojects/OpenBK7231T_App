/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ANDROID_INCLUDE_BLUETOOTH_H
#define ANDROID_INCLUDE_BLUETOOTH_H

#include "stdbool.h"
#include "stdint.h"
#include "stddef.h"

#include "wm_bt_def.h"


/** Bluetooth Adapter Visibility Modes*/
typedef enum {
    BT_SCAN_MODE_NONE,
    BT_SCAN_MODE_CONNECTABLE,
    BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE
} bt_scan_mode_t;

/** Bluetooth PinKey Code */
typedef struct {
    uint8_t pin[16];
} __attribute__((packed))bt_pin_code_t;

typedef struct {
    int32_t app_uid;
    uint64_t tx_bytes;
    uint64_t rx_bytes;
} __attribute__((packed))bt_uid_traffic_t;

/** Bluetooth SDP service record */
typedef struct {
    tls_bt_uuid_t uuid;
    uint16_t channel;
    char name[256]; // what's the maximum length
} bt_service_record_t;

/** Bluetooth Remote Version info */
typedef struct {
    int version;
    int sub_ver;
    int manufacturer;
} bt_remote_version_t;

typedef struct {
    uint16_t version_supported;
    uint8_t local_privacy_enabled;
    uint8_t max_adv_instance;
    uint8_t rpa_offload_supported;
    uint8_t max_irk_list_size;
    uint8_t max_adv_filter_supported;
    uint8_t activity_energy_info_supported;
    uint16_t scan_result_storage_size;
    uint16_t total_trackable_advertisers;
    uint8_t extended_scan_support;
    uint8_t debug_logging_supported;
} bt_local_le_features_t;

/** Bluetooth Out Of Band data for bonding */
typedef struct {
    uint8_t c192[16]; /* Simple Pairing Hash C-192 */
    uint8_t r192[16]; /* Simple Pairing Randomizer R-192 */
    uint8_t c256[16]; /* Simple Pairing Hash C-256 */
    uint8_t r256[16]; /* Simple Pairing Randomizer R-256 */
    uint8_t sm_tk[16]; /* Security Manager TK Value */
    uint8_t le_sc_c[16]; /* LE Secure Connections Random Value */
    uint8_t le_sc_r[16]; /* LE Secure Connections Random Value */
} bt_out_of_band_data_t;




/** Bluetooth Device Type */
typedef enum {
    BT_DEVICE_DEVTYPE_BREDR = 0x1,
    BT_DEVICE_DEVTYPE_BLE,
    BT_DEVICE_DEVTYPE_DUAL
} bt_device_type_t;

#define BT_MAX_NUM_UUIDS 16


#endif /* ANDROID_INCLUDE_BLUETOOTH_H */
