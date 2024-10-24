/******************************************************************************
 *
 *  Copyright (C) 2009-2013 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#ifndef BTIF_GATT_UTIL_H
#define BTIF_GATT_UTIL_H

#include "bluetooth.h"
#include "bt_gatt.h"

#include "bta_gatt_api.h"

void btif_to_bta_uuid(tBT_UUID *p_dest, tls_bt_uuid_t *p_src);
void btif_to_bta_response(tBTA_GATTS_RSP *p_dest, btgatt_response_t *p_src);
void btif_to_bta_uuid_mask(tBTA_DM_BLE_PF_COND_MASK *p_mask, tls_bt_uuid_t *p_src);

void bta_to_btif_uuid(tls_bt_uuid_t *p_dest, tBT_UUID *p_src);

uint16_t set_read_value(btgatt_read_params_t *p_dest, tBTA_GATTC_READ *p_src);
uint16_t get_uuid16(tBT_UUID *p_uuid);

void btif_gatt_check_encrypted_link(BD_ADDR bd_addr, tBTA_GATT_TRANSPORT transport);
extern void btif_gatt_move_track_adv_data(btgatt_track_adv_info_t *p_dest,
        btgatt_track_adv_info_t *p_src);


#endif

