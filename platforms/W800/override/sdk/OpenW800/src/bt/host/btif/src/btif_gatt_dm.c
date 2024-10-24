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


/************************************************************************************
 *
 *  Filename:      btif_gatt_dm.c
 *
 *  Description:   GATT device manager implementation
 *
 ***********************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#define LOG_TAG "BtGatt.btif"
#include "bt_target.h"
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
#include "bluetooth.h"
#include "bt_gatt.h"
#include "bt_gatt_dm.h"
#include "btif_common.h"
#include "btif_util.h"
#include "gki.h"
#include "bta_api.h"
#include "bta_gatt_api.h"
#include "bdaddr.h"
#include "btif_dm.h"
#include "btif_storage.h"

#include "btif_gatt.h"
#include "btif_gatt_util.h"
#include "wm_ble.h"
#include "bte_appl.h"

#define BTIF_GATT_DM_OBSERVE_EVT         3000
#define BTIF_GATT_DM_TIMER_EVT           3001
#define BTIF_GATT_DM_TRIGER_EVT          3002
#define BTIF_GATT_DM_ADV_DATA_UPDATE_EVT 3003
#define BTIF_GATT_DM_SCAN_PARAM_EVT      3004
#define BTIF_GATT_DM_RSSI_EVT            3005
#define BTIF_GATT_DM_OBSERVE_CMPL_EVT    3006
#define BTIF_GATT_DM_SEC_EVT             3007
#define BTIF_GATT_DM_GAP_ADV_EVT         3008


#define BTIF_GATT_MAX_OBSERVED_DEV 40

#define MAX_APP_TIMER_COUNT 12
#define APP_BTU_TTYPE_USER_FUNC 22

#define CHECK_BTGATT_INIT() if (tls_ble_dm_cb == NULL)\
    {\
        LOG_WARN(LOG_TAG, "%s: BTGATT not initialized", __FUNCTION__);\
        return TLS_BT_STATUS_NOT_READY;\
    } else {\
        LOG_VERBOSE(LOG_TAG, "%s", __FUNCTION__);\
    }

typedef struct {
    uint8_t     value[BTGATT_MAX_ATTR_LEN];
    uint8_t     inst_id;
    tls_bt_addr_t bd_addr;
    uint32_t    scan_interval;
    uint32_t    scan_window;
    uint8_t     client_if;
    uint8_t     action;
    uint8_t     is_direct;
    uint8_t     search_all;
    uint8_t     auth_req;
    uint8_t     write_type;
    uint8_t     status;
    uint8_t     addr_type;
    uint8_t     start;
    uint8_t     has_mask;
    int8_t      rssi;
    uint8_t     flag;
    uint32_t     evt_id; // for event triger id;
    int32_t     evt_ptr;
    uint8_t     timer_id;
    uint8_t     req_status;
    tBT_DEVICE_TYPE device_type;
    btgatt_transport_t transport;
    uint16_t num_clients;
} __attribute__((packed)) btif_gatt_dm_cb_t;

typedef struct {
    tls_bt_addr_t bd_addr;
    uint16_t    min_interval;
    uint16_t    max_interval;
    uint16_t    timeout;
    uint16_t    latency;
} btif_conn_param_cb_t;

typedef struct {
    tls_bt_addr_t bd_addr;
    uint8_t     in_use;
} __attribute__((packed)) btif_gatt_dm_dev_t;

typedef struct {
    btif_gatt_dm_dev_t remote_dev[BTIF_GATT_MAX_OBSERVED_DEV];
    uint8_t            addr_type;
    uint8_t            next_storage_idx;
} __attribute__((packed)) btif_gatt_dm_dev_cb_t;



static TIMER_LIST_ENT app_timer[MAX_APP_TIMER_COUNT];

static tls_ble_dm_callback_t tls_ble_dm_cb = NULL;
static btif_gatt_dm_dev_cb_t  btif_gatt_dm_dev_cb;
static btif_gatt_dm_dev_cb_t  *p_dev_cb = &btif_gatt_dm_dev_cb;




static void btif_gatt_dm_add_remote_bdaddr(BD_ADDR p_bda, uint8_t addr_type)
{
    uint8_t i;

    for(i = 0; i < BTIF_GATT_MAX_OBSERVED_DEV; i++) {
        if(!p_dev_cb->remote_dev[i].in_use) {
            wm_memcpy(p_dev_cb->remote_dev[i].bd_addr.address, p_bda, BD_ADDR_LEN);
            p_dev_cb->addr_type = addr_type;
            p_dev_cb->remote_dev[i].in_use = TRUE;
            LOG_VERBOSE(LOG_TAG, "%s device added idx=%d", __FUNCTION__, i);
            break;
        }
    }

    if(i == BTIF_GATT_MAX_OBSERVED_DEV) {
        i = p_dev_cb->next_storage_idx;
        wm_memcpy(p_dev_cb->remote_dev[i].bd_addr.address, p_bda, BD_ADDR_LEN);
        p_dev_cb->addr_type = addr_type;
        p_dev_cb->remote_dev[i].in_use = TRUE;
        LOG_VERBOSE(LOG_TAG, "%s device overwrite idx=%d", __FUNCTION__, i);
        p_dev_cb->next_storage_idx++;

        if(p_dev_cb->next_storage_idx >= BTIF_GATT_MAX_OBSERVED_DEV) {
            p_dev_cb->next_storage_idx = 0;
        }
    }
}

static uint8_t btif_gatt_dm_find_bdaddr(BD_ADDR p_bda)
{
    uint8_t i;

    for(i = 0; i < BTIF_GATT_MAX_OBSERVED_DEV; i++) {
        if(p_dev_cb->remote_dev[i].in_use &&
                !memcmp(p_dev_cb->remote_dev[i].bd_addr.address, p_bda, BD_ADDR_LEN)) {
            return TRUE;
        }
    }

    return FALSE;
}
static void btif_gatt_dm_update_properties(btif_gatt_dm_cb_t *p_btif_cb)
{
    uint8_t remote_name_len;
    uint8_t *p_eir_remote_name = NULL;
    tls_bt_bdname_t bdname;
    p_eir_remote_name = BTM_CheckEirData(p_btif_cb->value,
                                         BTM_EIR_COMPLETE_LOCAL_NAME_TYPE, &remote_name_len);

    if(p_eir_remote_name == NULL) {
        p_eir_remote_name = BTM_CheckEirData(p_btif_cb->value,
                                             BT_EIR_SHORTENED_LOCAL_NAME_TYPE, &remote_name_len);
    }

    if(p_eir_remote_name) {
        wm_memcpy(bdname.name, p_eir_remote_name, remote_name_len);
        bdname.name[remote_name_len] = '\0';
        LOG_VERBOSE(LOG_TAG, "%s BLE device name=%s len=%d dev_type=%d", __FUNCTION__, bdname.name,
                    remote_name_len, p_btif_cb->device_type);
#ifndef NO_NVRAM_FOR_BLE_PROP
        btif_dm_update_ble_remote_properties(p_btif_cb->bd_addr.address,   bdname.name,
                                             p_btif_cb->device_type);
#endif
    }
}


static void btif_gatt_dm_upstreams_evt(uint16_t event, char *p_param)
{
    btif_gatt_dm_cb_t *p_btif_cb = (btif_gatt_dm_cb_t *) p_param;
    tls_ble_dm_msg_t msg;

    switch(event) {
        case BTIF_GATT_DM_OBSERVE_EVT: {
            uint8_t remote_name_len;
            uint8_t *p_eir_remote_name = NULL;
            bt_device_type_t dev_type;
            tls_bt_property_t properties;
            p_eir_remote_name = BTM_CheckEirData(p_btif_cb->value,
                                                 BTM_EIR_COMPLETE_LOCAL_NAME_TYPE, &remote_name_len);

            if(p_eir_remote_name == NULL) {
                p_eir_remote_name = BTM_CheckEirData(p_btif_cb->value,
                                                     BT_EIR_SHORTENED_LOCAL_NAME_TYPE, &remote_name_len);
            }

            if((p_btif_cb->addr_type != BLE_ADDR_RANDOM) || (p_eir_remote_name)) {
                if(!btif_gatt_dm_find_bdaddr(p_btif_cb->bd_addr.address)) {
                    btif_gatt_dm_add_remote_bdaddr(p_btif_cb->bd_addr.address, p_btif_cb->addr_type);
                    btif_gatt_dm_update_properties(p_btif_cb);
                }
            }

            dev_type =  p_btif_cb->device_type;
            BTIF_STORAGE_FILL_PROPERTY(&properties, WM_BT_PROPERTY_TYPE_OF_DEVICE, sizeof(dev_type), &dev_type);
#ifndef NO_NVRAM_FOR_BLE_PROP
            btif_storage_set_remote_device_property(&(p_btif_cb->bd_addr), &properties, 0);
            btif_storage_set_remote_addr_type(&p_btif_cb->bd_addr, p_btif_cb->addr_type, 1);
#endif
            msg.dm_scan_result.rssi = p_btif_cb->rssi;
            bdcpy(msg.dm_scan_result.address, p_btif_cb->bd_addr.address);
            msg.dm_scan_result.value = &p_btif_cb->value[0];

            if(tls_ble_dm_cb) { tls_ble_dm_cb(WM_BLE_DM_SCAN_RES_EVT, &msg); }

            break;
        }

        case BTIF_GATT_DM_OBSERVE_CMPL_EVT: {
            msg.dm_scan_result_cmpl.num_responses = p_btif_cb->num_clients;

            if(tls_ble_dm_cb) { tls_ble_dm_cb(WM_BLE_DM_SCAN_RES_CMPL_EVT, &msg); }

            break;
        }

        case BTIF_GATT_DM_TIMER_EVT: {
            msg.dm_timer_expired.id = p_btif_cb->timer_id;
            msg.dm_timer_expired.func_ptr = app_timer[p_btif_cb->timer_id].data;

            if(tls_ble_dm_cb) { tls_ble_dm_cb(WM_BLE_DM_TIMER_EXPIRED_EVT, &msg); }

            break;
        }

        case BTIF_GATT_DM_TRIGER_EVT: {
            msg.dm_evt_trigered.id = p_btif_cb->evt_id;
            msg.dm_evt_trigered.func_ptr = p_btif_cb->evt_ptr;

            if(tls_ble_dm_cb) { tls_ble_dm_cb(WM_BLE_DM_TRIGER_EVT, &msg); }

            break;
        }

        case BTIF_GATT_DM_ADV_DATA_UPDATE_EVT: {
            msg.dm_set_adv_data_cmpl.status = p_btif_cb->req_status;

            if(tls_ble_dm_cb) { tls_ble_dm_cb(WM_BLE_DM_SET_ADV_DATA_CMPL_EVT, &msg); }

            break;
        }

        case BTIF_GATT_DM_SCAN_PARAM_EVT: {
            msg.dm_set_scan_param_cmpl.dm_id = p_btif_cb->client_if;
            msg.dm_set_scan_param_cmpl.status = p_btif_cb->status;

            if(tls_ble_dm_cb) { tls_ble_dm_cb(WM_BLE_DM_SET_SCAN_PARAM_CMPL_EVT, &msg); }

            break;
        }

        case BTIF_GATT_DM_RSSI_EVT: {
            bdcpy(msg.dm_report_rssi.address, p_btif_cb->bd_addr.address);
            msg.dm_report_rssi.rssi = p_btif_cb->rssi;
            msg.dm_report_rssi.status = p_btif_cb->status;

            if(tls_ble_dm_cb) { tls_ble_dm_cb(WM_BLE_DM_REPORT_RSSI_EVT, &msg); }

            break;
        }

        case BTIF_GATT_DM_SEC_EVT: {
            bdcpy(msg.dm_sec_result.address, p_btif_cb->bd_addr.address);
            msg.dm_sec_result.transport = p_btif_cb->transport;
            msg.dm_sec_result.status = p_btif_cb->status;

            if(tls_ble_dm_cb) { tls_ble_dm_cb(WM_BLE_DM_SEC_EVT, &msg); }

            break;
        }

        case BTIF_GATT_DM_GAP_ADV_EVT: {
            msg.dm_adv_cmpl.status = p_btif_cb->status;

            if(p_btif_cb->action) {
                if(tls_ble_dm_cb) { tls_ble_dm_cb(WM_BLE_DM_ADV_STARTED_EVT, &msg); }
            } else {
                if(tls_ble_dm_cb) { tls_ble_dm_cb(WM_BLE_DM_ADV_STOPPED_EVT, &msg); }
            }
        }

        default:
            break;
    }
}

static void bta_scan_results_cb(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data)
{
    btif_gatt_dm_cb_t btif_cb;
    uint8_t len;

    switch(event) {
        case BTA_DM_INQ_RES_EVT: {
            bdcpy(btif_cb.bd_addr.address, p_data->inq_res.bd_addr);
            btif_cb.device_type = p_data->inq_res.device_type;
            btif_cb.rssi = p_data->inq_res.rssi;
            btif_cb.addr_type = p_data->inq_res.ble_addr_type;
            btif_cb.flag = p_data->inq_res.flag;

            if(p_data->inq_res.p_eir) {
                wm_memcpy(btif_cb.value, p_data->inq_res.p_eir, 62);

                if(BTM_CheckEirData(p_data->inq_res.p_eir, BTM_EIR_COMPLETE_LOCAL_NAME_TYPE,
                                    &len)) {
                    p_data->inq_res.remt_name_not_required  = TRUE;
                }
            }

            btif_transfer_context(btif_gatt_dm_upstreams_evt, BTIF_GATT_DM_OBSERVE_EVT,
                                  (char *) &btif_cb, sizeof(btif_gatt_dm_cb_t), NULL);
        }
        break;

        case BTA_DM_INQ_CMPL_EVT: {
            BTIF_TRACE_DEBUG("%s  BLE observe complete. Num Resp %d\r\n",
                             __FUNCTION__, p_data->inq_cmpl.num_resps);
            btif_cb.num_clients = p_data->inq_cmpl.num_resps;
            btif_transfer_context(btif_gatt_dm_upstreams_evt, BTIF_GATT_DM_OBSERVE_CMPL_EVT,
                                  (char *) &btif_cb, sizeof(btif_gatt_dm_cb_t), NULL);
            return;
        }

        default:
            BTIF_TRACE_WARNING("%s : Unknown event 0x%x", __FUNCTION__, event);
            return;
    }
}
static void bta_gatt_dm_set_adv_data_cback(tBTA_STATUS req_status)
{
    btif_gatt_dm_cb_t btif_cb;
    btif_cb.req_status = req_status;
    btif_transfer_context(btif_gatt_dm_upstreams_evt, BTIF_GATT_DM_ADV_DATA_UPDATE_EVT,
                          (char *) &btif_cb, sizeof(btif_gatt_dm_cb_t), NULL);
}

static void bta_gatt_dm_timeout_cback(void *param)
{
    btif_gatt_dm_cb_t btif_cb;
    btif_cb.timer_id = (uint8_t)PTR_TO_INT(param);
    btif_transfer_context(btif_gatt_dm_upstreams_evt, BTIF_GATT_DM_TIMER_EVT,
                          (char *) &btif_cb, sizeof(btif_gatt_dm_cb_t), NULL);
}

static void bta_gatt_dm_evt_triger_cback(int evt_id, void *callback_ptr)
{
    btif_gatt_dm_cb_t btif_cb;
    btif_cb.evt_id = evt_id;
    btif_cb.evt_ptr = PTR_TO_INT(callback_ptr);
    btif_transfer_context(btif_gatt_dm_upstreams_evt, BTIF_GATT_DM_TRIGER_EVT,
                          (char *) &btif_cb, sizeof(btif_gatt_dm_cb_t), NULL);
}
static void bta_scan_param_setup_cb(tGATT_IF client_if, tBTM_STATUS status)
{
    btif_gatt_dm_cb_t btif_cb;
    btif_cb.status = status;
    btif_cb.client_if = client_if;
    btif_transfer_context(btif_gatt_dm_upstreams_evt, BTIF_GATT_DM_SCAN_PARAM_EVT,
                          (char *)&btif_cb, sizeof(btif_gatt_dm_cb_t), NULL);
}
static void btm_read_rssi_cb(tBTM_RSSI_RESULTS *p_result)
{
    btif_gatt_dm_cb_t btif_cb;
    bdcpy(btif_cb.bd_addr.address, p_result->rem_bda);
    btif_cb.rssi = p_result->rssi;
    btif_cb.status = p_result->status;
    //btif_cb.client_if = rssi_request_client_if;
    btif_transfer_context(btif_gatt_dm_upstreams_evt, BTIF_GATT_DM_RSSI_EVT,
                          (char *) &btif_cb, sizeof(btif_gatt_dm_cb_t), NULL);
}

static void btm_gap_adv_cb(uint8_t action, uint8_t inst_id, void *p_ref, tBTA_STATUS status)
{
    btif_gatt_dm_cb_t btif_cb;
    btif_cb.action = action;
    btif_cb.status = status;
    btif_transfer_context(btif_gatt_dm_upstreams_evt, BTIF_GATT_DM_GAP_ADV_EVT,
                          (char *) &btif_cb, sizeof(btif_gatt_dm_cb_t), NULL);
}

static void btm_set_sec_cb(BD_ADDR bd_addr, tBTA_TRANSPORT transport, tBTA_STATUS result)
{
    btif_gatt_dm_cb_t btif_cb;
    bdcpy(btif_cb.bd_addr.address, bd_addr);
    btif_cb.transport = transport;
    btif_cb.status = result;
    btif_transfer_context(btif_gatt_dm_upstreams_evt, BTIF_GATT_DM_SEC_EVT,
                          (char *) &btif_cb, sizeof(btif_gatt_dm_cb_t), NULL);
}


static void btif_gatt_dm_init_dev_cb(void)
{
    wm_memset(p_dev_cb, 0, sizeof(btif_gatt_dm_dev_cb_t));
}

static void btgatt_dm_handle_event(uint16_t event, char *p_param)
{
    tBTA_GATT_STATUS           status;
    BTIF_TRACE_EVENT("%s: Event %d", __FUNCTION__, event);

    switch(event) {
        case BTIF_GATT_DM_SET_ADV_DATA: {
            tls_ble_dm_adv_data_t *p_adv_data = (tls_ble_dm_adv_data_t *) p_param;
            tBTA_BLE_ADV_DATA s_data;// = GKI_getbuf(sizeof(tBTA_BLE_ADV_DATA));
            tBTA_BLE_AD_MASK mask = 0;

            if(p_adv_data->set_scan_rsp == false) {
                wm_memset(&s_data, 0, sizeof(tBTA_BLE_ADV_DATA));

                if(p_adv_data->pure_data) {
                    mask = BTM_BLE_AD_BIT_PURE_DATA;
                    wm_memcpy(s_data.manu.val, p_adv_data->manufacturer_data, p_adv_data->manufacturer_len);
                    s_data.manu.len = p_adv_data->manufacturer_len;
                } else {
                    mask = BTM_BLE_AD_BIT_FLAGS;

                    if(p_adv_data->manufacturer_len > 0) {
                        mask |= BTM_BLE_AD_BIT_MANU;
                        wm_memcpy(s_data.manu.val, p_adv_data->manufacturer_data, p_adv_data->manufacturer_len);
                        s_data.manu.len = p_adv_data->manufacturer_len;
                    }

                    if(p_adv_data->include_name) {
                        mask |= BTM_BLE_AD_BIT_DEV_NAME;
                    }
                }

                BTA_DmBleSetAdvConfig(mask, (tBTA_BLE_ADV_DATA *)&s_data, bta_gatt_dm_set_adv_data_cback);
            } else {
                mask = 0;
                wm_memset(&s_data, 0, sizeof(tBTA_BLE_ADV_DATA));

                if(p_adv_data->pure_data) {
                    mask = BTM_BLE_AD_BIT_PURE_DATA;
                    wm_memcpy(s_data.manu.val, p_adv_data->manufacturer_data, p_adv_data->manufacturer_len);
                    s_data.manu.len = p_adv_data->manufacturer_len;
                } else {
                    if(p_adv_data->manufacturer_len > 0) {
                        mask |= BTM_BLE_AD_BIT_MANU;
                        wm_memcpy(s_data.manu.val, p_adv_data->manufacturer_data, p_adv_data->manufacturer_len);
                        s_data.manu.len = p_adv_data->manufacturer_len;
                    }

                    if(p_adv_data->include_name) {
                        mask |= BTM_BLE_AD_BIT_DEV_NAME;
                    }
                }

                BTA_DmBleSetScanRsp(mask, (tBTA_BLE_ADV_DATA *)&s_data, NULL);
            }

            /*TODO , copy ble adv data*/
            break;
        }

        case BTIF_GATT_DM_SET_ADV_PARAM: {
            tls_ble_dm_adv_param_t *p_adv_param = (tls_ble_dm_adv_param_t *) p_param;
            BTA_DmSetBleAdvParams(p_adv_param->adv_int_min, p_adv_param->adv_int_max, p_adv_param->dir_addr);
            break;
        }

        case BTIF_GATT_DM_SET_ADV_EXT_PARAM: {
            tls_ble_dm_adv_ext_param_t *p_adv_ext_param = (tls_ble_dm_adv_ext_param_t *) p_param;
            BTA_DmSetBleAdvExtParams(p_adv_ext_param->adv_int_min, p_adv_ext_param->adv_int_max,
                                     p_adv_ext_param->adv_type, p_adv_ext_param->own_addr_type,
                                     p_adv_ext_param->chnl_map, p_adv_ext_param->afp, p_adv_ext_param->peer_addr_type,
                                     p_adv_ext_param->dir_addr);
            break;
        }

        case BTIF_GATT_DM_UPDATE_ADV_PARAM: {
            break;
        }

        case BTIF_GATT_DM_SET_ADV_ENABLE: {
            btif_dm_visibility_t *p_visibility = (btif_dm_visibility_t *)p_param;
            BTA_DmSetVisibility(p_visibility->disc | BTA_DM_IGNORE, p_visibility->conn | BTA_DM_IGNORE,
                                BTA_DM_IGNORE, BTA_DM_IGNORE);
            break;
        }

        case BTIF_GATT_DM_SET_ADV_DISABLE: {
            btif_dm_visibility_t *p_visibility = (btif_dm_visibility_t *)p_param;
            BTA_DmSetVisibility(p_visibility->disc | BTA_DM_IGNORE, p_visibility->conn | BTA_DM_IGNORE,
                                BTA_DM_IGNORE, BTA_DM_IGNORE);
            break;
        }

        case BTIF_GATT_DM_SET_PRIVACY: {
            uint8_t value = *(uint8_t *)p_param;
            BTA_DmBleConfigLocalPrivacy(value);
            break;
        }

        case BTIF_GATT_DM_SET_ADV_FILTER_POLICY: {
            uint8_t value = *(uint8_t *)p_param;
            BTM_BleUpdateAdvFilterPolicy(value);
            break;
        }

        case BTIF_GATT_DM_START_TIMER: {
            btif_dm_timer_t *p_timer_param = (btif_dm_timer_t *)p_param;
            uint8_t id = p_timer_param->id;
            uint32_t timeout_ms = p_timer_param->ms;
            //app_timer[id].in_use = 1;
            app_timer[id].data = (TIMER_PARAM_TYPE)PTR_TO_INT(p_timer_param->callback_ptr);
            app_timer[id].param = (TIMER_PARAM_TYPE)INT_TO_PTR(id);
            app_timer[id].p_cback = (TIMER_CBACK *)&bta_gatt_dm_timeout_cback;
            btu_start_timer_oneshot(&app_timer[id], APP_BTU_TTYPE_USER_FUNC, timeout_ms / 1000);
            break;
        }

        case BTIF_GATT_DM_STOP_TIMER: {
            btif_dm_timer_t *p_timer_param = (btif_dm_timer_t *)p_param;
            uint8_t id = p_timer_param->id;
            btu_stop_timer_oneshot(&app_timer[id]);
            break;
        }

        case BTIF_GATT_DM_EVT_TRIGER: {
            btif_dm_evt_t *p_evt = (btif_dm_evt_t *)p_param;
            bta_gatt_dm_evt_triger_cback(p_evt->evt, p_evt->priv_data);
            break;
        }

        case BTIF_GATT_DM_SET_DATA_LENGTH: {
            btif_dm_set_data_length_t *p_data_length = (btif_dm_set_data_length_t *)p_param;
            BTA_DmBleSetDataLength(p_data_length->addr.address, p_data_length->length);
            break;
        }

        case BTIF_GATT_DM_SCAN_START: {
            btif_gatt_dm_init_dev_cb();
            BTA_DmBleObserve(TRUE, 0, bta_scan_results_cb);
            break;
        }

        case BTIF_GATT_DM_SCAN_STOP: {
            BTA_DmBleObserve(FALSE, 0, bta_scan_results_cb);
            break;
        }

        case BTIF_GATT_DM_SET_SCAN_PARAMS: {
            btif_gatt_dm_cb_t *p_cb = (btif_gatt_dm_cb_t *)p_param;
            BTA_DmSetBleScanParams(p_cb->client_if, p_cb->scan_interval, p_cb->scan_window,
                                   p_cb->action, bta_scan_param_setup_cb);
            break;
        }

        case BTIF_GATT_DM_CONN_PARAM_UPDT: {
            btif_conn_param_cb_t *p_conn_param_cb = (btif_conn_param_cb_t *) p_param;

            if(BTA_DmGetConnectionState(p_conn_param_cb->bd_addr.address)) {
                BTA_DmBleUpdateConnectionParams(p_conn_param_cb->bd_addr.address,
                                                p_conn_param_cb->min_interval, p_conn_param_cb->max_interval,
                                                p_conn_param_cb->latency, p_conn_param_cb->timeout);
            } else {
                BTA_DmSetBlePrefConnParams(p_conn_param_cb->bd_addr.address,
                                           p_conn_param_cb->min_interval, p_conn_param_cb->max_interval,
                                           p_conn_param_cb->latency, p_conn_param_cb->timeout);
            }

            break;
        }

        case BTIF_GATT_DM_READ_RSSI: {
            btif_gatt_dm_cb_t *p_cb = (btif_gatt_dm_cb_t *)p_param;
            BTM_ReadRSSI(p_cb->bd_addr.address, (tBTM_CMPL_CB *)btm_read_rssi_cb);
            break;
        }

        case BTIF_GATT_DM_ADV_ACT: {
            btif_gatt_dm_cb_t *p_cb = (btif_gatt_dm_cb_t *)p_param;
            BTA_DmBleBroadcast(p_cb->start, (uint16_t)p_cb->scan_window, btm_gap_adv_cb);
            break;
        }

        case BTIF_GATT_DM_SEC_ACT: {
            btif_gatt_dm_cb_t *p_cb = (btif_gatt_dm_cb_t *)p_param;
            BTA_DmSetEncryption(p_cb->bd_addr.address, BTA_TRANSPORT_LE, btm_set_sec_cb, p_cb->action);
            break;
        }

        default:
            BTIF_TRACE_ERROR("%s: Unknown event (%d)!", __FUNCTION__, event);
            break;
    }
}

tls_bt_status_t tls_ble_dm_init(tls_ble_dm_callback_t callback)
{
    if(tls_ble_dm_cb == NULL) {
        tls_ble_dm_cb = callback;
        memset(&app_timer, 0, sizeof(TIMER_LIST_ENT)*MAX_APP_TIMER_COUNT);
    } else {
        return TLS_BT_STATUS_DONE;
    }

    return TLS_BT_STATUS_SUCCESS;
}
tls_bt_status_t tls_ble_dm_deinit()
{
    if(tls_ble_dm_cb == NULL) {
        return TLS_BT_STATUS_DONE;
    } else {
        tls_ble_dm_cb = NULL;
    }

    return TLS_BT_STATUS_SUCCESS;
}
tls_bt_status_t tls_ble_adv(uint8_t adv_type)
{
    uint8_t scan_state = 0;
    uint8_t adv_state = 0;
    btif_dm_visibility_t dm_visibility;
    CHECK_BTGATT_INIT();
#if 0
    scan_state = BTA_DmGetBleScanState();

    if(scan_state) {
        return TLS_BT_STATUS_UNSUPPORTED;
    }

#endif
    adv_state = BTA_DmGetBleAdvertiseState();

    if(adv_type == 1) {
        if(adv_state) { return TLS_BT_STATUS_DONE; }

        dm_visibility.conn = BTA_DM_BLE_CONNECTABLE;
        dm_visibility.disc = BTA_DM_BLE_GENERAL_DISCOVERABLE;
    } else if(adv_type == 2) {
        if(adv_state) { return TLS_BT_STATUS_DONE; }

        dm_visibility.conn = BTA_DM_BLE_NON_CONNECTABLE;
        dm_visibility.disc = BTA_DM_BLE_GENERAL_DISCOVERABLE;
    } else {
        if(!adv_state) { return TLS_BT_STATUS_DONE; }

        dm_visibility.conn = BTA_DM_BLE_NON_CONNECTABLE;
        dm_visibility.disc = BTA_DM_BLE_NON_DISCOVERABLE;
    }

    return btif_transfer_context(btgatt_dm_handle_event, BTIF_GATT_DM_SET_ADV_DISABLE,
                                 (char *) &dm_visibility, sizeof(btif_dm_visibility_t), NULL);
}
tls_bt_status_t tls_ble_set_adv_data(tls_ble_dm_adv_data_t *data)
{
    CHECK_BTGATT_INIT();
    return btif_transfer_context(btgatt_dm_handle_event, BTIF_GATT_DM_SET_ADV_DATA,
                                 (char *) data, sizeof(tls_ble_dm_adv_data_t), NULL);
}

tls_bt_status_t tls_ble_set_adv_param(tls_ble_dm_adv_param_t *param)
{
    CHECK_BTGATT_INIT();
    return btif_transfer_context(btgatt_dm_handle_event, BTIF_GATT_DM_SET_ADV_PARAM,
                                 (char *) param, sizeof(tls_ble_dm_adv_param_t), NULL);
}
tls_bt_status_t tls_ble_set_adv_ext_param(tls_ble_dm_adv_ext_param_t *param)
{
    CHECK_BTGATT_INIT();
#define BLE_ISVALID_PARAM(x, min, max)  ((x) >= (min) && (x) <= (max))

    if(!BLE_ISVALID_PARAM(param->adv_int_min, 0x20, 0x4000)
            || !BLE_ISVALID_PARAM(param->adv_int_max, 0x20, 0x4000)) {
        return TLS_BT_STATUS_PARM_INVALID;
    }

    if(param->adv_int_min > param->adv_int_max) {
        return TLS_BT_STATUS_PARM_INVALID;
    }

    return btif_transfer_context(btgatt_dm_handle_event, BTIF_GATT_DM_SET_ADV_EXT_PARAM,
                                 (char *) param, sizeof(tls_ble_dm_adv_ext_param_t), NULL);
}

tls_bt_status_t tls_ble_scan(bool start)
{
    uint8_t adv_state;
    uint8_t scan_state;
    CHECK_BTGATT_INIT();
#if 0
    adv_state = BTA_DmGetBleAdvertiseState();

    if(adv_state) {
        return TLS_BT_STATUS_UNSUPPORTED;
    }

#endif
    scan_state = BTA_DmGetBleScanState();

    if(start) {
        if(scan_state) {
            return TLS_BT_STATUS_DONE;
        }
    } else {
        if(scan_state == 0) {
            return TLS_BT_STATUS_DONE;
        }
    }

    return btif_transfer_context(btgatt_dm_handle_event,
                                 start ? BTIF_GATT_DM_SCAN_START : BTIF_GATT_DM_SCAN_STOP,
                                 (char *) NULL, 0, NULL);
}
tls_bt_status_t tls_ble_set_scan_param(int window, int interval, uint8_t scan_mode)
{
    CHECK_BTGATT_INIT();
    btif_gatt_dm_cb_t btif_cb;
    btif_cb.client_if = 0;  //defalut value; who care it?
    btif_cb.scan_interval = interval;
    btif_cb.scan_window = window;
    btif_cb.action = scan_mode;
    return btif_transfer_context(btgatt_dm_handle_event, BTIF_GATT_DM_SET_SCAN_PARAMS,
                                 (char *) &btif_cb, sizeof(btif_gatt_dm_cb_t), NULL);
}

int8_t tls_dm_get_timer_id()
{
    int i = 0;
    CHECK_BTGATT_INIT();

    for(i = 0; i < MAX_APP_TIMER_COUNT; i++) {
        if(app_timer[i].in_use == 0) {
            app_timer[i].in_use = 1;
            return i;
        }
    }

    return -1;
}
void tls_dm_free_timer_id(uint8_t id)
{
    CHECK_BTGATT_INIT();

    if(id >= MAX_APP_TIMER_COUNT)
    { return; }

    app_timer[id].in_use = 0;
}
tls_bt_status_t tls_dm_start_timer(uint8_t id, uint32_t timeout_ms,
                                   tls_ble_dm_timer_callback_t callback_ptr)
{
    CHECK_BTGATT_INIT();
    tls_bt_status_t status = TLS_BT_STATUS_UNSUPPORTED;

    if(id >= MAX_APP_TIMER_COUNT) {
        return status;
    }

    btif_dm_timer_t timer_param;
    timer_param.id = id;
    timer_param.ms = timeout_ms;
    timer_param.callback_ptr = (void *)callback_ptr;
    status = btif_transfer_context(btgatt_dm_handle_event, BTIF_GATT_DM_START_TIMER,
                                   (char *) &timer_param, sizeof(timer_param), NULL);
    return status;
}
tls_bt_status_t tls_dm_stop_timer(uint8_t id)
{
    CHECK_BTGATT_INIT();

    if(id > MAX_APP_TIMER_COUNT) { return TLS_BT_STATUS_PARM_INVALID; }

    tls_bt_status_t status = TLS_BT_STATUS_UNSUPPORTED;
    btif_dm_timer_t timer_param;
    timer_param.id = id;
    timer_param.ms = 0;
    status = btif_transfer_context(btgatt_dm_handle_event, BTIF_GATT_DM_STOP_TIMER,
                                   (char *) &timer_param, sizeof(timer_param), NULL);
    return status;
}
tls_bt_status_t tls_dm_evt_triger(int evt_id, tls_ble_dm_triger_callback_t callback)
{
    CHECK_BTGATT_INIT();
    btif_dm_evt_t evt;
    evt.evt = evt_id;
    evt.priv_data = (void *)callback;
    return btif_transfer_context(btgatt_dm_handle_event, BTIF_GATT_DM_EVT_TRIGER,
                                 (char *) &evt, sizeof(evt), NULL);
}
tls_bt_status_t tls_dm_set_data_length(tls_bt_addr_t *bd_addr, uint16_t length)
{
    CHECK_BTGATT_INIT();
    tls_bt_status_t status;
    btif_dm_set_data_length_t dm_length;
    dm_length.length = length;
    bdcpy(dm_length.addr.address, bd_addr->address);
    status = btif_transfer_context(btgatt_dm_handle_event, BTIF_GATT_DM_SET_DATA_LENGTH,
                                   (char *) &dm_length, sizeof(btif_dm_set_data_length_t), NULL);
    return status;
}
tls_bt_status_t tls_dm_set_privacy(uint8_t enable)
{
    CHECK_BTGATT_INIT();
    tls_bt_status_t status = TLS_BT_STATUS_SUCCESS;
    uint8_t value = enable;
    status = btif_transfer_context(btgatt_dm_handle_event, BTIF_GATT_DM_SET_PRIVACY,
                                   (char *) &value, sizeof(uint8_t), NULL);
    return status;
}
tls_bt_status_t tls_ble_conn_parameter_update(const tls_bt_addr_t *bd_addr, int min_interval,
        int max_interval, int latency, int timeout)
{
    CHECK_BTGATT_INIT();
    btif_conn_param_cb_t btif_cb;
    btif_cb.min_interval = min_interval;
    btif_cb.max_interval = max_interval;
    btif_cb.latency = latency;
    btif_cb.timeout = timeout;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    return btif_transfer_context(btgatt_dm_handle_event, BTIF_GATT_DM_CONN_PARAM_UPDT,
                                 (char *) &btif_cb, sizeof(btif_conn_param_cb_t), NULL);
}
tls_bt_status_t tls_dm_read_remote_rssi(const tls_bt_addr_t *bd_addr)
{
    CHECK_BTGATT_INIT();
    btif_gatt_dm_cb_t btif_cb;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    return btif_transfer_context(btgatt_dm_handle_event, BTIF_GATT_DM_READ_RSSI,
                                 (char *) &btif_cb, sizeof(btif_gatt_dm_cb_t), NULL);
}
tls_bt_status_t tls_ble_set_sec_io_cap(uint8_t io_cap)
{
    CHECK_BTGATT_INIT();
    bte_appl_cfg.ble_io_cap = io_cap;
    return TLS_BT_STATUS_SUCCESS;
}
tls_bt_status_t tls_ble_set_sec_auth_req(uint8_t auth_req)
{
    CHECK_BTGATT_INIT();
    bte_appl_cfg.ble_auth_req = auth_req;
    return TLS_BT_STATUS_SUCCESS;
}
tls_bt_status_t tls_ble_set_sec(const tls_bt_addr_t *bd_addr, uint8_t sec_act)
{
    CHECK_BTGATT_INIT();
    btif_gatt_dm_cb_t btif_cb;
    memcpy(&btif_cb.bd_addr, bd_addr, sizeof(tls_bt_addr_t));
    btif_cb.action = sec_act;
    return btif_transfer_context(btgatt_dm_handle_event, BTIF_GATT_DM_SEC_ACT,
                                 (char *) &btif_cb, sizeof(btif_gatt_dm_cb_t), NULL);
}
tls_bt_status_t tls_ble_gap_adv(uint8_t start, int duration)
{
    CHECK_BTGATT_INIT();
    btif_gatt_dm_cb_t btif_cb;
    btif_cb.start = start;
    btif_cb.scan_window = (uint32_t)duration; //reuse the scan_window;
    return btif_transfer_context(btgatt_dm_handle_event, BTIF_GATT_DM_ADV_ACT,
                                 (char *) &btif_cb, sizeof(btif_gatt_dm_cb_t), NULL);
}

#endif

