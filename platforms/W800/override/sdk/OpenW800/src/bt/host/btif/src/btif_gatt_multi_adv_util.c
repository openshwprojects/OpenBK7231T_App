/******************************************************************************
 *
 *  Copyright (C) 2014  Broadcom Corporation
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

/*******************************************************************************
 *
 *  Filename:      btif_gatt_multi_adv_util.c
 *
 *  Description:   Multi ADV helper implementation
 *
 *******************************************************************************/

#define LOG_TAG "bt_btif_gatt"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "bt_target.h"

#if (BLE_INCLUDED == TRUE)
#include "btu.h"
#include "bluetooth.h"
#include "bt_gatt.h"

#include "bta_gatt_api.h"
#include "btif_common.h"
#include "btif_gatt_multi_adv_util.h"
#include "btif_gatt_util.h"

#ifdef USE_ALARM
extern fixed_queue_t *btu_general_alarm_queue;
#endif
/*******************************************************************************
**  Static variables
********************************************************************************/
static int user_app_count = 0;
static btgatt_multi_adv_common_data *p_multi_adv_com_data_cb = NULL;

btgatt_multi_adv_common_data *btif_obtain_multi_adv_data_cb()
{
    int max_adv_inst = BTM_BleMaxMultiAdvInstanceCount();

    if(0 == max_adv_inst) {
        max_adv_inst = 1;
    }

    BTIF_TRACE_DEBUG("%s, Count:%d", __FUNCTION__, max_adv_inst);

    if(NULL == p_multi_adv_com_data_cb) {
        p_multi_adv_com_data_cb = GKI_getbuf(sizeof(btgatt_multi_adv_common_data));
        /* Storing both client_if and inst_id details */
        p_multi_adv_com_data_cb->clntif_map =
                        GKI_getbuf((max_adv_inst * INST_ID_IDX_MAX) * sizeof(int8_t));
        p_multi_adv_com_data_cb->inst_cb =
                        GKI_getbuf((max_adv_inst + 1) * sizeof(btgatt_multi_adv_inst_cb));

        for(int i = 0; i < max_adv_inst * 2; i += 2) {
            p_multi_adv_com_data_cb->clntif_map[i] = INVALID_ADV_INST;
            p_multi_adv_com_data_cb->clntif_map[i + 1] = INVALID_ADV_INST;
        }
    }

    return p_multi_adv_com_data_cb;
}

void btif_gattc_incr_app_count(void)
{
    // TODO: Instead of using a fragile reference counter here, one could
    //       simply track the client_if instances that are in the map.
    ++user_app_count;
}

void btif_gattc_decr_app_count(void)
{
    if(user_app_count > 0) {
        user_app_count--;
    }

    if((user_app_count == 0) && (p_multi_adv_com_data_cb != NULL)) {
        GKI_freebuf(p_multi_adv_com_data_cb->clntif_map);
        GKI_freebuf(p_multi_adv_com_data_cb->inst_cb);
        GKI_free_and_reset_buf((void **)&p_multi_adv_com_data_cb);
    }
}

int btif_multi_adv_add_instid_map(int client_if, int inst_id, uint8_t gen_temp_instid)
{
    int i = 1;
    btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();

    if(NULL == p_multi_adv_data_cb) {
        return INVALID_ADV_INST;
    }

    for(i = 1; i <  BTM_BleMaxMultiAdvInstanceCount(); i++) {
        if(client_if == p_multi_adv_data_cb->clntif_map[i + i]) {
            if(!gen_temp_instid) {
                // Write the final inst_id value obtained from stack layer
                p_multi_adv_data_cb->clntif_map[i + (i + 1)] = inst_id;
                BTIF_TRACE_DEBUG("%s -Index: %d, Found client_if: %d", __FUNCTION__,
                                 i, p_multi_adv_data_cb->clntif_map[i + i]);
                break;
            } else {
                //Store the passed in inst_id value
                if(inst_id != INVALID_ADV_INST) {
                    p_multi_adv_data_cb->clntif_map[i + (i + 1)] = inst_id;
                } else {
                    p_multi_adv_data_cb->clntif_map[i + (i + 1)] = (i + 1);
                }

                BTIF_TRACE_DEBUG("%s - Index:%d,Found client_if: %d", __FUNCTION__,
                                 i, p_multi_adv_data_cb->clntif_map[i + i]);
                break;
            }
        }
    }

    if(i <  BTM_BleMaxMultiAdvInstanceCount()) {
        return i;
    }

    // If client ID if is not found, then write both values
    for(i = 1; i <  BTM_BleMaxMultiAdvInstanceCount(); i++) {
        if(INVALID_ADV_INST == p_multi_adv_data_cb->clntif_map[i + i]) {
            p_multi_adv_data_cb->clntif_map[i + i] = client_if;

            if(inst_id != INVALID_ADV_INST) {
                p_multi_adv_data_cb->clntif_map[i + (i + 1)] = inst_id;
            } else {
                p_multi_adv_data_cb->clntif_map[i + (i + 1)] = (i + 1);
            }

            BTIF_TRACE_DEBUG("%s -Not found - Index:%d, client_if: %d, Inst ID: %d",
                             __FUNCTION__, i,
                             p_multi_adv_data_cb->clntif_map[i + i],
                             p_multi_adv_data_cb->clntif_map[i + (i + 1)]);
            break;
        }
    }

    if(i <  BTM_BleMaxMultiAdvInstanceCount()) {
        return i;
    }

    return INVALID_ADV_INST;
}

int btif_multi_adv_instid_for_clientif(int client_if)
{
    int i = 1, ret = INVALID_ADV_INST;
    btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();

    if(NULL == p_multi_adv_data_cb) {
        return INVALID_ADV_INST;
    }

    // Retrieve the existing inst_id for the client_if value
    for(i = 1; i <  BTM_BleMaxMultiAdvInstanceCount(); i++) {
        if(client_if == p_multi_adv_data_cb->clntif_map[i + i]) {
            BTIF_TRACE_DEBUG("%s - Client if found", __FUNCTION__, client_if);
            ret = p_multi_adv_data_cb->clntif_map[i + (i + 1)];
        }
    }

    return ret;
}

int btif_gattc_obtain_idx_for_datacb(int value, int clnt_inst_index)
{
    int i = 1;
    btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();

    if(NULL == p_multi_adv_data_cb) {
        return INVALID_ADV_INST;
    }

    // Retrieve the array index for the inst_id value
    for(i = 1; i <  BTM_BleMaxMultiAdvInstanceCount(); i++) {
        if(value == p_multi_adv_data_cb->clntif_map[i + (i + clnt_inst_index)]) {
            break;
        }
    }

    if(i <  BTM_BleMaxMultiAdvInstanceCount()) {
        BTIF_TRACE_DEBUG("%s, %d", __FUNCTION__, i);
        return i;
    }

    BTIF_TRACE_DEBUG("%s Invalid instance", __FUNCTION__);
    return INVALID_ADV_INST;
}

void btif_gattc_adv_data_packager(int client_if, uint8_t set_scan_rsp,
                                  uint8_t include_name, uint8_t include_txpower, int min_interval, int max_interval,
                                  int appearance, int manufacturer_len, char *manufacturer_data,
                                  int service_data_len, char *service_data, int service_uuid_len,
                                  char *service_uuid, btif_adv_data_t *p_multi_adv_inst)
{
    wm_memset(p_multi_adv_inst, 0, sizeof(btif_adv_data_t));
    p_multi_adv_inst->client_if = (uint8_t) client_if;
    p_multi_adv_inst->set_scan_rsp = set_scan_rsp;
    p_multi_adv_inst->include_name = include_name;
    p_multi_adv_inst->include_txpower = include_txpower;
    p_multi_adv_inst->min_interval = min_interval;
    p_multi_adv_inst->max_interval = max_interval;
    p_multi_adv_inst->appearance = appearance;
    p_multi_adv_inst->manufacturer_len = manufacturer_len;

    if(manufacturer_len > 0) {
        p_multi_adv_inst->p_manufacturer_data = GKI_getbuf(manufacturer_len);
        wm_memcpy(p_multi_adv_inst->p_manufacturer_data, manufacturer_data, manufacturer_len);
    }

    p_multi_adv_inst->service_data_len = service_data_len;

    if(service_data_len > 0) {
        p_multi_adv_inst->p_service_data = GKI_getbuf(service_data_len);
        wm_memcpy(p_multi_adv_inst->p_service_data, service_data, service_data_len);
    }

    p_multi_adv_inst->service_uuid_len = service_uuid_len;

    if(service_uuid_len > 0) {
        p_multi_adv_inst->p_service_uuid = GKI_getbuf(service_uuid_len);
        wm_memcpy(p_multi_adv_inst->p_service_uuid, service_uuid, service_uuid_len);
    }
}

void btif_gattc_adv_data_cleanup(btif_adv_data_t *adv)
{
    GKI_free_and_reset_buf((void **)&adv->p_service_data);
    GKI_free_and_reset_buf((void **)&adv->p_service_uuid);
    GKI_free_and_reset_buf((void **)&adv->p_manufacturer_data);
}

uint8_t btif_gattc_copy_datacb(int cbindex, const btif_adv_data_t *p_adv_data,
                               uint8_t bInstData)
{
    btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();

    if(NULL == p_multi_adv_data_cb || cbindex < 0) {
        return false;
    }

    BTIF_TRACE_DEBUG("%s", __func__);
    wm_memset(&p_multi_adv_data_cb->inst_cb[cbindex].data, 0,
              sizeof(p_multi_adv_data_cb->inst_cb[cbindex].data));
    p_multi_adv_data_cb->inst_cb[cbindex].mask = 0;

    if(!p_adv_data->set_scan_rsp) {
        p_multi_adv_data_cb->inst_cb[cbindex].mask = BTM_BLE_AD_BIT_FLAGS;
        p_multi_adv_data_cb->inst_cb[cbindex].data.flag = ADV_FLAGS_GENERAL;

        if(p_multi_adv_data_cb->inst_cb[cbindex].timeout_s) {
            p_multi_adv_data_cb->inst_cb[cbindex].data.flag = ADV_FLAGS_LIMITED;
        }

        if(p_multi_adv_data_cb->inst_cb[cbindex].param.adv_type == BTA_BLE_NON_CONNECT_EVT)
            p_multi_adv_data_cb->inst_cb[cbindex].data.flag &=
                            ~(BTA_DM_LIMITED_DISC | BTA_DM_GENERAL_DISC);

        if(p_multi_adv_data_cb->inst_cb[cbindex].data.flag == 0) {
            p_multi_adv_data_cb->inst_cb[cbindex].mask = 0;
        }
    }

    if(p_adv_data->include_name) {
        p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_DEV_NAME;
    }

    if(p_adv_data->include_txpower) {
        p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_TX_PWR;
    }

    if(false == bInstData && p_adv_data->min_interval > 0 && p_adv_data->max_interval > 0 &&
            p_adv_data->max_interval > p_adv_data->min_interval) {
        p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_INT_RANGE;
        p_multi_adv_data_cb->inst_cb[cbindex].data.int_range.low =
                        p_adv_data->min_interval;
        p_multi_adv_data_cb->inst_cb[cbindex].data.int_range.hi =
                        p_adv_data->max_interval;
    } else if(true == bInstData) {
        if(p_multi_adv_data_cb->inst_cb[cbindex].param.adv_int_min > 0 &&
                p_multi_adv_data_cb->inst_cb[cbindex].param.adv_int_max > 0 &&
                p_multi_adv_data_cb->inst_cb[cbindex].param.adv_int_max >
                p_multi_adv_data_cb->inst_cb[cbindex].param.adv_int_min) {
            p_multi_adv_data_cb->inst_cb[cbindex].data.int_range.low =
                            p_multi_adv_data_cb->inst_cb[cbindex].param.adv_int_min;
            p_multi_adv_data_cb->inst_cb[cbindex].data.int_range.hi =
                            p_multi_adv_data_cb->inst_cb[cbindex].param.adv_int_max;
        }

        if(p_adv_data->include_txpower) {
            p_multi_adv_data_cb->inst_cb[cbindex].data.tx_power =
                            p_multi_adv_data_cb->inst_cb[cbindex].param.tx_power;
        }
    }

    if(p_adv_data->appearance != 0) {
        p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_APPEARANCE;
        p_multi_adv_data_cb->inst_cb[cbindex].data.appearance = p_adv_data->appearance;
    }

    if(p_adv_data->manufacturer_len > 0 &&
            p_adv_data->p_manufacturer_data != NULL &&
            p_adv_data->manufacturer_len < MAX_SIZE_MANUFACTURER_DATA) {
        p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_MANU;
        p_multi_adv_data_cb->inst_cb[cbindex].data.manu.len =
                        p_adv_data->manufacturer_len;
        wm_memcpy(&p_multi_adv_data_cb->inst_cb[cbindex].data.manu.val,
                  p_adv_data->p_manufacturer_data, p_adv_data->manufacturer_len);
    }

    if(p_adv_data->service_data_len > 0 &&
            p_adv_data->p_service_data != NULL &&
            p_adv_data->service_data_len < MAX_SIZE_PROPRIETARY_ELEMENT) {
        BTIF_TRACE_DEBUG("%s - In service_data", __func__);
        tBTA_BLE_PROPRIETARY *p_prop = &p_multi_adv_data_cb->inst_cb[cbindex].data.proprietary;
        p_prop->num_elem = 1;
        tBTA_BLE_PROP_ELEM *p_elem = &p_prop->elem[0];
        p_elem->adv_type = BTM_BLE_AD_TYPE_SERVICE_DATA;
        p_elem->len = p_adv_data->service_data_len;
        wm_memcpy(p_elem->val, p_adv_data->p_service_data,
                  p_adv_data->service_data_len);
        p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_PROPRIETARY;
    }

    if(p_adv_data->service_uuid_len && p_adv_data->p_service_uuid) {
        uint16_t *p_uuid_out16 = NULL;
        uint32_t *p_uuid_out32 = NULL;

        for(int position = 0; position < p_adv_data->service_uuid_len; position += LEN_UUID_128) {
            tls_bt_uuid_t uuid;
            wm_memset(&uuid, 0, sizeof(uuid));
            wm_memcpy(&uuid.uu, p_adv_data->p_service_uuid + position, LEN_UUID_128);
            tBT_UUID bt_uuid;
            wm_memset(&bt_uuid, 0, sizeof(bt_uuid));
            btif_to_bta_uuid(&bt_uuid, &uuid);

            switch(bt_uuid.len) {
                case(LEN_UUID_16): {
                    if(p_multi_adv_data_cb->inst_cb[cbindex].data.services.num_service == 0) {
                        p_multi_adv_data_cb->inst_cb[cbindex].data.services.list_cmpl = FALSE;
                        p_uuid_out16 = p_multi_adv_data_cb->inst_cb[cbindex].data.services.uuid;
                    }

                    if(p_multi_adv_data_cb->inst_cb[cbindex].data.services.num_service < MAX_16BIT_SERVICES) {
                        BTIF_TRACE_DEBUG("%s - In 16-UUID_data", __func__);
                        p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_SERVICE;
                        ++p_multi_adv_data_cb->inst_cb[cbindex].data.services.num_service;
                        *p_uuid_out16++ = bt_uuid.uu.uuid16;
                    }

                    break;
                }

                case(LEN_UUID_32): {
                    if(p_multi_adv_data_cb->inst_cb[cbindex].data.service_32b.num_service == 0) {
                        p_multi_adv_data_cb->inst_cb[cbindex].data.service_32b.list_cmpl = FALSE;
                        p_uuid_out32 = p_multi_adv_data_cb->inst_cb[cbindex].data.service_32b.uuid;
                    }

                    if(p_multi_adv_data_cb->inst_cb[cbindex].data.service_32b.num_service < MAX_32BIT_SERVICES) {
                        BTIF_TRACE_DEBUG("%s - In 32-UUID_data", __func__);
                        p_multi_adv_data_cb->inst_cb[cbindex].mask |= BTM_BLE_AD_BIT_SERVICE_32;
                        ++p_multi_adv_data_cb->inst_cb[cbindex].data.service_32b.num_service;
                        *p_uuid_out32++ = bt_uuid.uu.uuid32;
                    }

                    break;
                }

                case(LEN_UUID_128): {
                    /* Currently, only one 128-bit UUID is supported */
                    if(p_multi_adv_data_cb->inst_cb[cbindex].data.services_128b.num_service == 0) {
                        BTIF_TRACE_DEBUG("%s - In 128-UUID_data", __FUNCTION__);
                        p_multi_adv_data_cb->inst_cb[cbindex].mask |=
                                        BTM_BLE_AD_BIT_SERVICE_128;
                        wm_memcpy(p_multi_adv_data_cb->inst_cb[cbindex]
                                  .data.services_128b.uuid128,
                                  bt_uuid.uu.uuid128, LEN_UUID_128);
                        BTIF_TRACE_DEBUG(
                                        "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x",
                                        bt_uuid.uu.uuid128[0], bt_uuid.uu.uuid128[1],
                                        bt_uuid.uu.uuid128[2], bt_uuid.uu.uuid128[3],
                                        bt_uuid.uu.uuid128[4], bt_uuid.uu.uuid128[5],
                                        bt_uuid.uu.uuid128[6], bt_uuid.uu.uuid128[7],
                                        bt_uuid.uu.uuid128[8], bt_uuid.uu.uuid128[9],
                                        bt_uuid.uu.uuid128[10], bt_uuid.uu.uuid128[11],
                                        bt_uuid.uu.uuid128[12], bt_uuid.uu.uuid128[13],
                                        bt_uuid.uu.uuid128[14], bt_uuid.uu.uuid128[15]);
                        ++p_multi_adv_data_cb->inst_cb[cbindex]
                        .data.services_128b.num_service;
                        p_multi_adv_data_cb->inst_cb[cbindex]
                        .data.services_128b.list_cmpl = TRUE;
                    }

                    break;
                }

                default:
                    break;
            }
        }
    }

    return true;
}

void btif_gattc_clear_clientif(int client_if, uint8_t stop_timer)
{
    btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();

    if(NULL == p_multi_adv_data_cb) {
        return;
    }

    // Clear both the inst_id and client_if values
    for(int i = 0; i < BTM_BleMaxMultiAdvInstanceCount() * 2; i += 2) {
        if(client_if == p_multi_adv_data_cb->clntif_map[i]) {
            btif_gattc_cleanup_inst_cb(p_multi_adv_data_cb->clntif_map[i + 1], stop_timer);

            if(stop_timer) {
                p_multi_adv_data_cb->clntif_map[i] = INVALID_ADV_INST;
                p_multi_adv_data_cb->clntif_map[i + 1] = INVALID_ADV_INST;
                BTIF_TRACE_DEBUG("Cleaning up index %d for clnt_if :%d,", i / 2, client_if);
            }

            break;
        }
    }
}

void btif_gattc_cleanup_inst_cb(int inst_id, uint8_t stop_timer)
{
    // Check for invalid instance id
    if(inst_id < 0 || inst_id >= BTM_BleMaxMultiAdvInstanceCount()) {
        return;
    }

    btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();

    if(NULL == p_multi_adv_data_cb) {
        return;
    }

    int cbindex = (STD_ADV_INSTID == inst_id) ?
                  STD_ADV_INSTID : btif_gattc_obtain_idx_for_datacb(inst_id, INST_ID_IDX);

    if(cbindex < 0) {
        return;
    }

    BTIF_TRACE_DEBUG("%s: inst_id %d, cbindex %d", __func__, inst_id, cbindex);
    btif_gattc_cleanup_multi_inst_cb(&p_multi_adv_data_cb->inst_cb[cbindex], stop_timer);
}

void btif_gattc_cleanup_multi_inst_cb(btgatt_multi_adv_inst_cb *p_multi_inst_cb,
                                      uint8_t stop_timer)
{
    if(p_multi_inst_cb == NULL) {
        return;
    }

    // Discoverability timer cleanup
    if(stop_timer) {
#ifdef USE_ALARM
        alarm_free(p_multi_inst_cb->multi_adv_timer);
        p_multi_inst_cb->multi_adv_timer = NULL;
#endif
    }

    wm_memset(&p_multi_inst_cb->data, 0, sizeof(p_multi_inst_cb->data));
}

void btif_multi_adv_timer_ctrl(int client_if, TIMER_CBACK cb)
{
    int inst_id = btif_multi_adv_instid_for_clientif(client_if);

    if(inst_id == INVALID_ADV_INST) {
        return;
    }

    int cbindex = btif_gattc_obtain_idx_for_datacb(inst_id, INST_ID_IDX);

    if(cbindex == INVALID_ADV_INST) {
        return;
    }

    btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();

    if(p_multi_adv_data_cb == NULL) {
        return;
    }

    btgatt_multi_adv_inst_cb *inst_cb = &p_multi_adv_data_cb->inst_cb[cbindex];

    if(cb == NULL) {
#ifdef USE_ALARM
        alarm_free(inst_cb->multi_adv_timer);
        inst_cb->multi_adv_timer = NULL;
#else
        btu_stop_timer_oneshot(&inst_cb->multi_adv_timer);
#endif
    } else {
        if(inst_cb->timeout_s != 0) {
#ifdef USE_ALARM
            alarm_free(inst_cb->multi_adv_timer);
            inst_cb->multi_adv_timer = alarm_new("btif_gatt.multi_adv_timer");
            alarm_set_on_queue(inst_cb->multi_adv_timer,
                               inst_cb->timeout_s * 1000,
                               cb, INT_TO_PTR(client_if),
                               btu_general_alarm_queue);
#else
            inst_cb->multi_adv_timer.param = (TIMER_PARAM_TYPE)INT_TO_PTR(client_if);
            inst_cb->multi_adv_timer.p_cback = (TIMER_CBACK *)&cb;
            btu_start_timer_oneshot(&inst_cb->multi_adv_timer, BTU_TTYPE_USER_FUNC, inst_cb->timeout_s);
#endif
        }
    }
}

#endif
