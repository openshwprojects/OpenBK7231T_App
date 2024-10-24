/******************************************************************************
 *
 *  Copyright (C) 2009-2014 Broadcom Corporation
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
 *  Filename:      btif_gatt_client.c
 *
 *  Description:   GATT client implementation
 *
 *******************************************************************************/

#define LOG_TAG "bt_btif_gattc"

#include <errno.h>
#include "bluetooth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bt_target.h"
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
#include "bdaddr.h"
#include "btif_common.h"
#include "btif_util.h"

#include "bt_gatt.h"

#include "bta_api.h"
#include "bta_gatt_api.h"
#include "btif_config.h"
#include "btif_dm.h"
#include "btif_gatt.h"
#include "btif_gatt_multi_adv_util.h"
#include "btif_gatt_util.h"
#include "btif_storage.h"
#include "btif_storage.h"
#include "osi/include/log.h"
#include "vendor_api.h"
#include "wm_ble_gatt.h"
#include "wm_bt_def.h"

/*******************************************************************************
**  Constants & Macros
********************************************************************************/

#define CHECK_BTGATT_INIT() if (tls_ble_cb == NULL)\
    {\
        LOG_WARN(LOG_TAG, "%s: BTGATT not initialized", __FUNCTION__);\
        return TLS_BT_STATUS_NOT_READY;\
    } else {\
        LOG_VERBOSE(LOG_TAG, "%s", __FUNCTION__);\
    }

#define BLE_RESOLVE_ADDR_MSB                 0x40   /* bit7, bit6 is 01 to be resolvable random */
#define BLE_RESOLVE_ADDR_MASK                0xc0   /* bit 6, and bit7 */
#define BTM_BLE_IS_RESOLVE_BDA(x)           ((x[0] & BLE_RESOLVE_ADDR_MASK) == BLE_RESOLVE_ADDR_MSB)

typedef enum {
    BTIF_GATTC_REGISTER_APP = 1000,
    BTIF_GATTC_UNREGISTER_APP,
    BTIF_GATTC_SCAN_START,
    BTIF_GATTC_SCAN_STOP,
    BTIF_GATTC_OPEN,
    BTIF_GATTC_CLOSE,
    BTIF_GATTC_SEARCH_SERVICE,
    BTIF_GATTC_READ_CHAR,
    BTIF_GATTC_READ_CHAR_DESCR,
    BTIF_GATTC_WRITE_CHAR,
    BTIF_GATTC_WRITE_CHAR_DESCR,
    BTIF_GATTC_EXECUTE_WRITE,
    BTIF_GATTC_REG_FOR_NOTIFICATION,
    BTIF_GATTC_DEREG_FOR_NOTIFICATION,
    BTIF_GATTC_REFRESH,
    BTIF_GATTC_READ_RSSI,
    BTIF_GATTC_LISTEN,
    BTIF_GATTC_SET_ADV_DATA,
    BTIF_GATTC_CONFIGURE_MTU,
    BTIF_GATTC_CONN_PARAM_UPDT,
    BTIF_GATTC_SCAN_FILTER_PARAM_SETUP,
    BTIF_GATTC_SCAN_FILTER_CONFIG,
    BTIF_GATTC_SCAN_FILTER_CLEAR,
    BTIF_GATTC_SCAN_FILTER_ENABLE,
    BTIF_GATTC_SET_SCAN_PARAMS,
    BTIF_GATTC_ADV_INSTANCE_ENABLE,
    BTIF_GATTC_ADV_INSTANCE_UPDATE,
    BTIF_GATTC_ADV_INSTANCE_SET_DATA,
    BTIF_GATTC_ADV_INSTANCE_DISABLE,
    BTIF_GATTC_CONFIG_STORAGE_PARAMS,
    BTIF_GATTC_ENABLE_BATCH_SCAN,
    BTIF_GATTC_READ_BATCH_SCAN_REPORTS,
    BTIF_GATTC_DISABLE_BATCH_SCAN,
    BTIF_GATTC_GET_GATT_DB
} btif_gattc_event_t;


#define BTIF_GATT_OBSERVE_EVT   0x1000
#define BTIF_GATTC_RSSI_EVT     0x1001
#define BTIF_GATTC_SCAN_FILTER_EVT  0x1003
#define BTIF_GATTC_SCAN_PARAM_EVT   0x1004

#define ENABLE_BATCH_SCAN 1
#define DISABLE_BATCH_SCAN 0

/*******************************************************************************
**  Local type definitions
********************************************************************************/
typedef struct {
    uint8_t report_format;
    uint16_t data_len;
    uint8_t num_records;
    uint8_t *p_rep_data;
} btgatt_batch_reports;

typedef struct {
    uint8_t  status;
    uint8_t  client_if;
    uint8_t  action;
    uint8_t  avbl_space;
    uint8_t  lost_timeout;
    tBLE_ADDR_TYPE addr_type;
    uint8_t  batch_scan_full_max;
    uint8_t  batch_scan_trunc_max;
    uint8_t  batch_scan_notify_threshold;
    tBTA_BLE_BATCH_SCAN_MODE scan_mode;
    uint32_t scan_interval;
    uint32_t scan_window;
    tBTA_BLE_DISCARD_RULE discard_rule;
    btgatt_batch_reports  read_reports;
} btgatt_batch_track_cb_t;

typedef tBTA_DM_BLE_PF_FILT_PARAMS btgatt_adv_filt_param_t;

typedef struct {
    uint8_t     client_if;
    uint8_t     action;
    tBTA_DM_BLE_PF_COND_TYPE filt_type;
    tls_bt_addr_t bd_addr;
    uint8_t     value[BTGATT_MAX_ATTR_LEN];
    uint8_t     value_len;
    uint8_t     filt_index;
    uint16_t    conn_id;
    uint16_t    company_id_mask;
    tls_bt_uuid_t   uuid;
    tls_bt_uuid_t   uuid_mask;
    uint8_t     value_mask[BTGATT_MAX_ATTR_LEN];
    uint8_t     value_mask_len;
    uint8_t     has_mask;
    uint8_t     addr_type;
    uint8_t     status;
    tBTA_DM_BLE_PF_AVBL_SPACE avbl_space;
    tBTA_DM_BLE_SCAN_COND_OP cond_op;
    btgatt_adv_filt_param_t adv_filt_param;
} btgatt_adv_filter_cb_t;

typedef struct {
    uint8_t     value[BTGATT_MAX_ATTR_LEN];
    uint8_t     inst_id;
    tls_bt_addr_t bd_addr;
    btgatt_srvc_id_t srvc_id;
    btgatt_srvc_id_t incl_srvc_id;
    btgatt_gatt_id_t char_id;
    btgatt_gatt_id_t descr_id;
    uint16_t    handle;
    tls_bt_uuid_t   uuid;
    tls_bt_uuid_t   uuid_mask;
    uint16_t    conn_id;
    uint16_t    len;
    uint16_t    mask;
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
    tBT_DEVICE_TYPE device_type;
    btgatt_transport_t transport;
} __attribute__((packed)) btif_gattc_cb_t;



/*******************************************************************************
**  Static variables
********************************************************************************/
static uint8_t rssi_request_client_if;
static tls_ble_callback_t tls_ble_cb = NULL;


/*******************************************************************************
**  Static functions
********************************************************************************/




static btgattc_error_t btif_gattc_translate_btm_status(tBTM_STATUS status)
{
    switch(status) {
        case BTM_SUCCESS:
        case BTM_SUCCESS_NO_SECURITY:
            return BT_GATTC_COMMAND_SUCCESS;

        case BTM_CMD_STARTED:
            return BT_GATTC_COMMAND_STARTED;

        case BTM_BUSY:
            return BT_GATTC_COMMAND_BUSY;

        case BTM_CMD_STORED:
            return BT_GATTC_COMMAND_STORED;

        case BTM_NO_RESOURCES:
            return BT_GATTC_NO_RESOURCES;

        case BTM_MODE_UNSUPPORTED:
        case BTM_WRONG_MODE:
        case BTM_MODE4_LEVEL4_NOT_SUPPORTED:
            return BT_GATTC_MODE_UNSUPPORTED;

        case BTM_ILLEGAL_VALUE:
        case BTM_SCO_BAD_LENGTH:
            return BT_GATTC_ILLEGAL_VALUE;

        case BTM_UNKNOWN_ADDR:
            return BT_GATTC_UNKNOWN_ADDR;

        case BTM_DEVICE_TIMEOUT:
            return BT_GATTC_DEVICE_TIMEOUT;

        case BTM_FAILED_ON_SECURITY:
        case BTM_REPEATED_ATTEMPTS:
        case BTM_NOT_AUTHORIZED:
            return BT_GATTC_SECURITY_ERROR;

        case BTM_DEV_RESET:
        case BTM_ILLEGAL_ACTION:
            return BT_GATTC_INCORRECT_STATE;

        case BTM_BAD_VALUE_RET:
            return BT_GATTC_INVALID_CONTROLLER_OUTPUT;

        case BTM_DELAY_CHECK:
            return BT_GATTC_DELAYED_ENCRYPTION_CHECK;

        case BTM_ERR_PROCESSING:
        default:
            return BT_GATTC_ERR_PROCESSING;
    }
}

static void btapp_gattc_req_data(uint16_t event, char *p_dest, char *p_src)
{
    tBTA_GATTC *p_dest_data = (tBTA_GATTC *) p_dest;
    tBTA_GATTC *p_src_data = (tBTA_GATTC *) p_src;

    if(!p_src_data || !p_dest_data) {
        return;
    }

    // Copy basic structure first
    maybe_non_aligned_memcpy(p_dest_data, p_src_data, sizeof(*p_src_data));

    // Allocate buffer for request data if necessary
    switch(event) {
        case BTA_GATTC_READ_CHAR_EVT:
        case BTA_GATTC_READ_DESCR_EVT:
            if(p_src_data->read.p_value != NULL) {
                p_dest_data->read.p_value = GKI_getbuf(sizeof(tBTA_GATT_UNFMT));
                wm_memcpy(p_dest_data->read.p_value, p_src_data->read.p_value,
                          sizeof(tBTA_GATT_UNFMT));

                // Allocate buffer for att value if necessary
                if(p_src_data->read.p_value->len > 0 &&
                        p_src_data->read.p_value->p_value != NULL) {
                    p_dest_data->read.p_value->p_value =
                                    GKI_getbuf(p_src_data->read.p_value->len);
                    wm_memcpy(p_dest_data->read.p_value->p_value,
                              p_src_data->read.p_value->p_value,
                              p_src_data->read.p_value->len);
                }
            } else {
                BTIF_TRACE_WARNING("%s :Src read.p_value ptr is NULL for event  0x%x",
                                   __FUNCTION__, event);
                p_dest_data->read.p_value = NULL;
            }

            break;

        default:
            break;
    }
}

static void btapp_gattc_free_req_data(uint16_t event, tBTA_GATTC *p_data)
{
    switch(event) {
        case BTA_GATTC_READ_CHAR_EVT:
        case BTA_GATTC_READ_DESCR_EVT:
            if(p_data != NULL && p_data->read.p_value != NULL) {
                if(p_data->read.p_value->len > 0) {
                    GKI_free_and_reset_buf((void **)&p_data->read.p_value->p_value);
                }

                GKI_free_and_reset_buf((void **)&p_data->read.p_value);
            }

            break;

        default:
            break;
    }
}





static void btif_gattc_upstreams_evt(uint16_t event, char *p_param)
{
    LOG_VERBOSE(LOG_TAG, "%s: Event %d", __FUNCTION__, event);
    tBTA_GATTC *p_data = (tBTA_GATTC *) p_param;
    tls_ble_msg_t msg;

    switch(event) {
        case BTA_GATTC_REG_EVT: {
            tls_bt_uuid_t app_uuid;
            bta_to_btif_uuid(&app_uuid, &p_data->reg_oper.app_uuid);
            bta_to_btif_uuid(&msg.cli_register.app_uuid, &p_data->reg_oper.app_uuid);
            msg.cli_register.client_if = p_data->reg_oper.client_if;
            msg.cli_register.status = p_data->reg_oper.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_REGISTER_EVT, &msg); }

            break;
        }

        case BTA_GATTC_DEREG_EVT: {
            msg.cli_register.client_if = p_data->reg_oper.client_if;
            msg.cli_register.status = p_data->reg_oper.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_DEREGISTER_EVT, &msg); }

            break;
        }

        case BTA_GATTC_READ_CHAR_EVT: {
            btgatt_read_params_t data;
            set_read_value(&data, &p_data->read);
            msg.cli_read.conn_id = p_data->read.conn_id;
            msg.cli_read.handle = data.handle; //p_data->read.handle;
            msg.cli_read.len =  data.value.len;//p_data->read.p_value->len;
            msg.cli_read.status = p_data->read.status;
            msg.cli_read.value = &data.value.value[0];
            msg.cli_read.value_type = 0;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_READ_CHAR_EVT, &msg); }

            break;
        }

        case BTA_GATTC_WRITE_CHAR_EVT:
        case BTA_GATTC_PREP_WRITE_EVT: {
            msg.cli_write.conn_id = p_data->write.conn_id;
            msg.cli_write.handle = p_data->write.handle;
            msg.cli_write.status = p_data->write.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_WRITE_CHAR_EVT, &msg); }

            break;
        }

        case BTA_GATTC_EXEC_EVT: {
            msg.cli_exec_cmpl.conn_id = p_data->exec_cmpl.conn_id;
            msg.cli_exec_cmpl.status = p_data->exec_cmpl.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_EXEC_CMPL_EVT, &msg); }

            break;
        }

        case BTA_GATTC_SEARCH_CMPL_EVT: {
            msg.cli_search_cmpl.conn_id = p_data->search_cmpl.conn_id;
            msg.cli_search_cmpl.status = p_data->search_cmpl.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_SEARCH_CMPL_EVT, &msg); }

            break;
        }

        case BTA_GATTC_SEARCH_RES_EVT: {
            //tls_bt_uuid_t uuid;
            //bta_to_btif_uuid(&uuid, &(p_data->srvc_res.service_uuid.uuid));
            msg.cli_search_res.conn_id = p_data->srvc_res.conn_id;
            msg.cli_search_res.inst_id = p_data->srvc_res.service_uuid.inst_id;
            bta_to_btif_uuid(&msg.cli_search_res.uuid, &(p_data->srvc_res.service_uuid.uuid));

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_SEARCH_RES_EVT, &msg); }

            break;
        }

        case BTA_GATTC_READ_DESCR_EVT: {
            btgatt_read_params_t data;
            set_read_value(&data, &p_data->read);
            msg.cli_read.conn_id = p_data->read.conn_id;
            msg.cli_read.handle = data.handle; //p_data->read.handle;
            msg.cli_read.len =  data.value.len;//p_data->read.p_value->len;
            msg.cli_read.status = p_data->read.status;
            msg.cli_read.value = &data.value.value[0];
            msg.cli_read.value_type = 0;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_READ_DESCR_EVT, &msg); }

            break;
        }

        case BTA_GATTC_WRITE_DESCR_EVT: {
            msg.cli_write.conn_id = p_data->write.conn_id;
            msg.cli_write.handle = p_data->write.handle;
            msg.cli_write.status = p_data->write.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_WRITE_DESCR_EVT, &msg); }

            break;
        }

        case BTA_GATTC_NOTIF_EVT: {
            bdcpy(msg.cli_notif.bda, p_data->notify.bda);
            msg.cli_notif.conn_id = p_data->notify.conn_id;
            msg.cli_notif.handle = p_data->notify.handle;
            msg.cli_notif.is_notify = p_data->notify.is_notify;
            msg.cli_notif.len = p_data->notify.len;
            msg.cli_notif.value = &p_data->notify.value[0];

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_NOTIF_EVT, &msg); }

            if(p_data->notify.is_notify == FALSE) {
                BTA_GATTC_SendIndConfirm(p_data->notify.conn_id, p_data->notify.handle);
            }

            break;
        }

        case BTA_GATTC_OPEN_EVT: {
            //bt_bdaddr_t bda;
            //bdcpy(bda.address, p_data->open.remote_bda);
            bdcpy(msg.cli_open.bd_addr, p_data->open.remote_bda);
            msg.cli_open.client_if = p_data->open.client_if;
            msg.cli_open.conn_id = p_data->open.conn_id;
            msg.cli_open.status = p_data->open.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_OPEN_EVT, &msg); }

            if(GATT_DEF_BLE_MTU_SIZE != p_data->open.mtu && p_data->open.mtu) {
                msg.cli_cfg_mtu.conn_id = p_data->open.conn_id;
                msg.cli_cfg_mtu.mtu = p_data->open.mtu;
                msg.cli_cfg_mtu.status = p_data->open.status;

                if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_CFG_MTU_EVT, &msg); }
            }

            if(p_data->open.status == BTA_GATT_OK) {
                btif_gatt_check_encrypted_link(p_data->open.remote_bda, p_data->open.transport);
            }

            break;
        }

        case BTA_GATTC_CLOSE_EVT: {
            //bt_bdaddr_t bda;
            //bdcpy(bda.address, p_data->close.remote_bda);
            msg.cli_close.client_if = p_data->close.client_if;
            msg.cli_close.conn_id = p_data->close.conn_id;
            msg.cli_close.reason = p_data->status;
            bdcpy(msg.cli_close.remote_bda, p_data->close.remote_bda);
            msg.cli_close.status = p_data->close.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_CLOSE_EVT, &msg); }

            break;
        }

        case BTA_GATTC_ACL_EVT:
            LOG_DEBUG(LOG_TAG, "BTA_GATTC_ACL_EVT: status = %d", p_data->status);
            /* Ignore for now */
            break;

        case BTA_GATTC_CANCEL_OPEN_EVT:
            break;

        case BTIF_GATTC_RSSI_EVT: {
            //btif_gattc_cb_t *p_btif_cb = (btif_gattc_cb_t *) p_param;
            //HAL_CBACK(bt_gatt_callbacks, client->read_remote_rssi_cb, p_btif_cb->client_if,
            //          &p_btif_cb->bd_addr, p_btif_cb->rssi, p_btif_cb->status);
            break;
        }

        case BTA_GATTC_LISTEN_EVT: {
            msg.cli_listen.client_if = p_data->reg_oper.client_if;
            msg.cli_listen.status = p_data->reg_oper.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_LISTEN_EVT, &msg); }

            break;
        }

        case BTA_GATTC_CFG_MTU_EVT: {
            msg.cli_cfg_mtu.conn_id = p_data->cfg_mtu.conn_id;
            msg.cli_cfg_mtu.mtu = p_data->cfg_mtu.mtu;
            msg.cli_cfg_mtu.status = p_data->cfg_mtu.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_CFG_MTU_EVT, &msg); }

            break;
        }

        case BTA_GATTC_MULT_ADV_ENB_EVT: {
            break;
        }

        case BTA_GATTC_MULT_ADV_UPD_EVT: {
            break;
        }

        case BTA_GATTC_MULT_ADV_DATA_EVT: {
            break;
        }

        case BTA_GATTC_MULT_ADV_DIS_EVT: {
            btif_gattc_cb_t *p_btif_cb = (btif_gattc_cb_t *) p_param;
            btif_gattc_clear_clientif(p_btif_cb->client_if, TRUE);
            break;
        }

        case BTA_GATTC_ADV_DATA_EVT: {
            btif_gattc_cleanup_inst_cb(STD_ADV_INSTID, FALSE);
            /* No HAL callback available */
            break;
        }

        case BTA_GATTC_CONGEST_EVT:
            msg.cli_congest.congested = p_data->congest.congested;
            msg.cli_congest.conn_id = p_data->congest.conn_id;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_CONGEST_EVT, &msg); }

            break;

        case BTA_GATTC_BTH_SCAN_CFG_EVT: {
            break;
        }

        case BTA_GATTC_BTH_SCAN_ENB_EVT: {
            break;
        }

        case BTA_GATTC_BTH_SCAN_DIS_EVT: {
            break;
        }

        case BTA_GATTC_BTH_SCAN_THR_EVT: {
            break;
        }

        case BTA_GATTC_BTH_SCAN_RD_EVT: {
            break;
        }

        case BTA_GATTC_SCAN_FLT_CFG_EVT: {
            break;
        }

        case BTA_GATTC_SCAN_FLT_PARAM_EVT: {
            break;
        }

        case BTA_GATTC_SCAN_FLT_STATUS_EVT: {
            break;
        }

        case BTA_GATTC_ADV_VSC_EVT: {
            break;
        }

        case BTIF_GATTC_SCAN_PARAM_EVT: {
            break;
        }

        default:
            LOG_ERROR(LOG_TAG, "%s: Unhandled event (%d)!", __FUNCTION__, event);
            break;
    }

    btapp_gattc_free_req_data(event, p_data);
}

static void bta_gattc_cback(tBTA_GATTC_EVT event, tBTA_GATTC *p_data)
{
    tls_bt_status_t status = btif_transfer_context(btif_gattc_upstreams_evt,
                             (uint16_t) event, (void *) p_data, sizeof(tBTA_GATTC), btapp_gattc_req_data);
    ASSERTC(status == TLS_BT_STATUS_SUCCESS, "Context transfer failed!", status);
}
static void btm_read_rssi_cb(tBTM_RSSI_RESULTS *p_result)
{
    btif_gattc_cb_t btif_cb;
    bdcpy(btif_cb.bd_addr.address, p_result->rem_bda);
    btif_cb.rssi = p_result->rssi;
    btif_cb.status = p_result->status;
    btif_cb.client_if = rssi_request_client_if;
    btif_transfer_context(btif_gattc_upstreams_evt, BTIF_GATTC_RSSI_EVT,
                          (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

#if (BLE_VND_INCLUDED == TRUE)
static tls_bt_status_t btif_gattc_multi_adv_disable(int client_if);

static void btif_multi_adv_stop_cb(void *data)
{
    int client_if = PTR_TO_INT(data);
    btif_gattc_multi_adv_disable(client_if);   // Does context switch
}

static void bta_gattc_multi_adv_cback(tBTA_BLE_MULTI_ADV_EVT event, uint8_t inst_id,
                                      void *p_ref, tBTA_STATUS call_status)
{
    btif_gattc_cb_t btif_cb;
    tBTA_GATTC_EVT upevt;
    uint8_t client_if = 0;

    if(NULL == p_ref) {
        BTIF_TRACE_WARNING("%s Invalid p_ref received", __FUNCTION__);
    } else {
        client_if = *(uint8_t *) p_ref;
    }

    BTIF_TRACE_DEBUG("%s -Inst ID %d, Status:%x, client_if:%d", __FUNCTION__, inst_id, call_status,
                     client_if);
    btif_cb.status = call_status;
    btif_cb.client_if = client_if;
    btif_cb.inst_id = inst_id;

    switch(event) {
        case BTA_BLE_MULTI_ADV_ENB_EVT:
            upevt = BTA_GATTC_MULT_ADV_ENB_EVT;
            break;

        case BTA_BLE_MULTI_ADV_DISABLE_EVT:
            upevt = BTA_GATTC_MULT_ADV_DIS_EVT;
            break;

        case BTA_BLE_MULTI_ADV_PARAM_EVT:
            upevt = BTA_GATTC_MULT_ADV_UPD_EVT;
            break;

        case BTA_BLE_MULTI_ADV_DATA_EVT:
            upevt = BTA_GATTC_MULT_ADV_DATA_EVT;
            break;

        default:
            return;
    }

    tls_bt_status_t status = btif_transfer_context(btif_gattc_upstreams_evt, (uint16_t) upevt,
                             (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
    ASSERTC(status == TLS_BT_STATUS_SUCCESS, "Context transfer failed!", status);
}

static void bta_gattc_set_adv_data_cback(tBTA_STATUS call_status)
{
    UNUSED(call_status);
    btif_gattc_cb_t btif_cb;
    btif_cb.status = call_status;
    btif_cb.action = 0;
    btif_transfer_context(btif_gattc_upstreams_evt, BTA_GATTC_ADV_DATA_EVT,
                          (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static void bta_batch_scan_setup_cb(tBTA_BLE_BATCH_SCAN_EVT evt,
                                    tBTA_DM_BLE_REF_VALUE ref_value, tBTA_STATUS status)
{
    uint8_t upevt = 0;
    btgatt_batch_track_cb_t btif_scan_track_cb;
    btif_scan_track_cb.status = status;
    btif_scan_track_cb.client_if = ref_value;
    BTIF_TRACE_DEBUG("bta_batch_scan_setup_cb-Status:%x, client_if:%d, evt=%d",
                     status, ref_value, evt);

    switch(evt) {
        case BTA_BLE_BATCH_SCAN_ENB_EVT: {
            upevt = BTA_GATTC_BTH_SCAN_ENB_EVT;
            break;
        }

        case BTA_BLE_BATCH_SCAN_DIS_EVT: {
            upevt = BTA_GATTC_BTH_SCAN_DIS_EVT;
            break;
        }

        case BTA_BLE_BATCH_SCAN_CFG_STRG_EVT: {
            upevt = BTA_GATTC_BTH_SCAN_CFG_EVT;
            break;
        }

        case BTA_BLE_BATCH_SCAN_DATA_EVT: {
            upevt = BTA_GATTC_BTH_SCAN_RD_EVT;
            break;
        }

        case BTA_BLE_BATCH_SCAN_THRES_EVT: {
            upevt = BTA_GATTC_BTH_SCAN_THR_EVT;
            break;
        }

        default:
            return;
    }

    btif_transfer_context(btif_gattc_upstreams_evt, upevt, (char *) &btif_scan_track_cb,
                          sizeof(btgatt_batch_track_cb_t), NULL);
}

static void bta_batch_scan_threshold_cb(tBTA_DM_BLE_REF_VALUE ref_value)
{
    btgatt_batch_track_cb_t btif_scan_track_cb;
    btif_scan_track_cb.status = 0;
    btif_scan_track_cb.client_if = ref_value;
    BTIF_TRACE_DEBUG("%s - client_if:%d", __FUNCTION__, ref_value);
    btif_transfer_context(btif_gattc_upstreams_evt, BTA_GATTC_BTH_SCAN_THR_EVT,
                          (char *) &btif_scan_track_cb, sizeof(btif_gattc_cb_t), NULL);
}

static void bta_batch_scan_reports_cb(tBTA_DM_BLE_REF_VALUE ref_value, uint8_t report_format,
                                      uint8_t num_records, uint16_t data_len,
                                      uint8_t *p_rep_data, tBTA_STATUS status)
{
    btgatt_batch_track_cb_t btif_scan_track_cb;
    wm_memset(&btif_scan_track_cb, 0, sizeof(btgatt_batch_track_cb_t));
    BTIF_TRACE_DEBUG("%s - client_if:%d, %d, %d, %d", __FUNCTION__, ref_value, status, num_records,
                     data_len);
    btif_scan_track_cb.status = status;
    btif_scan_track_cb.client_if = ref_value;
    btif_scan_track_cb.read_reports.report_format = report_format;
    btif_scan_track_cb.read_reports.data_len = data_len;
    btif_scan_track_cb.read_reports.num_records = num_records;

    if(data_len > 0) {
        btif_scan_track_cb.read_reports.p_rep_data = GKI_getbuf(data_len);
        wm_memcpy(btif_scan_track_cb.read_reports.p_rep_data, p_rep_data, data_len);
        GKI_freebuf(p_rep_data);
    }

    btif_transfer_context(btif_gattc_upstreams_evt, BTA_GATTC_BTH_SCAN_RD_EVT,
                          (char *) &btif_scan_track_cb, sizeof(btgatt_batch_track_cb_t), NULL);

    if(data_len > 0) {
        GKI_free_and_reset_buf((void **)&btif_scan_track_cb.read_reports.p_rep_data);
    }
}

static void bta_track_adv_event_cb(tBTA_DM_BLE_TRACK_ADV_DATA *p_track_adv_data)
{
    btgatt_track_adv_info_t btif_scan_track_cb;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    btif_gatt_move_track_adv_data(&btif_scan_track_cb,
                                  (btgatt_track_adv_info_t *)p_track_adv_data);
    btif_transfer_context(btif_gattc_upstreams_evt, BTA_GATTC_ADV_VSC_EVT,
                          (char *) &btif_scan_track_cb, sizeof(btgatt_track_adv_info_t), NULL);
}
static void bta_scan_filt_cfg_cb(tBTA_DM_BLE_PF_ACTION action, tBTA_DM_BLE_SCAN_COND_OP cfg_op,
                                 tBTA_DM_BLE_PF_AVBL_SPACE avbl_space, tBTA_STATUS status,
                                 tBTA_DM_BLE_REF_VALUE ref_value)
{
    btgatt_adv_filter_cb_t btif_cb;
    btif_cb.status = status;
    btif_cb.action = action;
    btif_cb.cond_op = cfg_op;
    btif_cb.avbl_space = avbl_space;
    btif_cb.client_if = ref_value;
    btif_transfer_context(btif_gattc_upstreams_evt, BTA_GATTC_SCAN_FLT_CFG_EVT,
                          (char *) &btif_cb, sizeof(btgatt_adv_filter_cb_t), NULL);
}

static void bta_scan_filt_param_setup_cb(uint8_t action_type,
        tBTA_DM_BLE_PF_AVBL_SPACE avbl_space,
        tBTA_DM_BLE_REF_VALUE ref_value, tBTA_STATUS status)
{
    btgatt_adv_filter_cb_t btif_cb;
    btif_cb.status = status;
    btif_cb.action = action_type;
    btif_cb.client_if = ref_value;
    btif_cb.avbl_space = avbl_space;
    btif_transfer_context(btif_gattc_upstreams_evt, BTA_GATTC_SCAN_FLT_PARAM_EVT,
                          (char *) &btif_cb, sizeof(btgatt_adv_filter_cb_t), NULL);
}

static void bta_scan_filt_status_cb(uint8_t action, tBTA_STATUS status,
                                    tBTA_DM_BLE_REF_VALUE ref_value)
{
    btgatt_adv_filter_cb_t btif_cb;
    btif_cb.status = status;
    btif_cb.action = action;
    btif_cb.client_if = ref_value;
    btif_transfer_context(btif_gattc_upstreams_evt, BTA_GATTC_SCAN_FLT_STATUS_EVT,
                          (char *) &btif_cb, sizeof(btgatt_adv_filter_cb_t), NULL);
}
#endif
static void btgattc_free_event_data(uint16_t event, char *event_data)
{
    switch(event) {
        case BTIF_GATTC_ADV_INSTANCE_SET_DATA:
        case BTIF_GATTC_SET_ADV_DATA: {
            btif_adv_data_t *adv_data = (btif_adv_data_t *)event_data;
            btif_gattc_adv_data_cleanup(adv_data);
            break;
        }

        default:
            break;
    }
}

static void btgattc_handle_event(uint16_t event, char *p_param)
{
    tBTA_GATT_STATUS           status;
    tBT_UUID                   uuid;
    tBTA_GATT_UNFMT            descr_val;
    btif_gattc_cb_t *p_cb = (btif_gattc_cb_t *) p_param;

    if(!p_cb) {
        return;
    }

    LOG_VERBOSE(LOG_TAG, "%s: Event %d", __FUNCTION__, event);

    switch(event) {
        case BTIF_GATTC_REGISTER_APP:
            btif_to_bta_uuid(&uuid, &p_cb->uuid);
            btif_gattc_incr_app_count();
            BTA_GATTC_AppRegister(&uuid, bta_gattc_cback);
            break;

        case BTIF_GATTC_UNREGISTER_APP:
            btif_gattc_clear_clientif(p_cb->client_if, TRUE);
            btif_gattc_decr_app_count();
            BTA_GATTC_AppDeregister(p_cb->client_if);
            break;

        case BTIF_GATTC_OPEN: {
            // Ensure device is in inquiry database
            int addr_type = 0;
            int device_type = 0;
            tls_ble_msg_t msg;
            tBTA_GATT_TRANSPORT transport = BTA_GATT_TRANSPORT_LE;

            if(btif_get_device_type(&p_cb->bd_addr, &addr_type, &device_type) == TRUE
                    && device_type != BT_DEVICE_TYPE_BREDR) {
                BTIF_TRACE_DEBUG("***Add device, addr_type=%d, device_type=%d\r\n", addr_type, device_type);
                BTA_DmAddBleDevice(p_cb->bd_addr.address, addr_type, device_type);
                //BTA_DmAddBleDevice(p_cb->bd_addr.address, BLE_ADDR_PUBLIC, BT_DEVICE_TYPE_BLE);
            }

            // Check for background connections
            if(!p_cb->is_direct) {
                // Check for privacy 1.0 and 1.1 controller and do not start background
                // connection if RPA offloading is not supported, since it will not
                // connect after change of random address
                if(
                                (p_cb->addr_type == BLE_ADDR_RANDOM) &&
                                BTM_BLE_IS_RESOLVE_BDA(p_cb->bd_addr.address)) {
                    tBTM_BLE_VSC_CB vnd_capabilities;
                    BTM_BleGetVendorCapabilities(&vnd_capabilities);

                    if(!vnd_capabilities.rpa_offloading) {
                        bdcpy(msg.cli_open.bd_addr, p_cb->bd_addr.address);
                        msg.cli_open.client_if = p_cb->client_if;
                        msg.cli_open.conn_id = 0;
                        msg.cli_open.status = TLS_BT_STATUS_UNSUPPORTED;

                        if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_OPEN_EVT, &msg); }

                        return;
                    }
                }

                BTA_DmBleSetBgConnType(BTM_BLE_CONN_AUTO, NULL);
            }

            // Determine transport
            if(p_cb->transport != GATT_TRANSPORT_AUTO) {
                transport = p_cb->transport;
            } else {
                switch(device_type) {
                    case BT_DEVICE_TYPE_BREDR:
                        transport = BTA_GATT_TRANSPORT_BR_EDR;
                        break;

                    case BT_DEVICE_TYPE_BLE:
                        transport = BTA_GATT_TRANSPORT_LE;
                        break;

                    case BT_DEVICE_TYPE_DUMO:
                        if(p_cb->transport == GATT_TRANSPORT_LE) {
                            transport = BTA_GATT_TRANSPORT_LE;
                        } else {
                            transport = BTA_GATT_TRANSPORT_BR_EDR;
                        }

                        break;
                }
            }

            // Connect!
            BTIF_TRACE_DEBUG("%s Transport=%d, device type=%d",
                             __func__, transport, device_type);
            BTA_GATTC_Open(p_cb->client_if, p_cb->bd_addr.address, p_cb->is_direct, transport);
            break;
        }

        case BTIF_GATTC_CLOSE:

            // Disconnect established connections
            if(p_cb->conn_id != 0) {
                BTA_GATTC_Close(p_cb->conn_id);
            } else {
                BTA_GATTC_CancelOpen(p_cb->client_if, p_cb->bd_addr.address, TRUE);
            }

            // Cancel pending background connections (remove from whitelist)
            BTA_GATTC_CancelOpen(p_cb->client_if, p_cb->bd_addr.address, FALSE);
            break;

        case BTIF_GATTC_SEARCH_SERVICE: {
            if(p_cb->search_all) {
                BTA_GATTC_ServiceSearchRequest(p_cb->conn_id, NULL);
            } else {
                btif_to_bta_uuid(&uuid, &p_cb->uuid);
                BTA_GATTC_ServiceSearchRequest(p_cb->conn_id, &uuid);
            }

            break;
        }

        case BTIF_GATTC_GET_GATT_DB: {
            tls_ble_msg_t msg;
            tls_btgatt_db_element_t *db = NULL;
            int count = 0;
            msg.cli_db.status = BTA_GATT_OK;
            msg.cli_db.conn_id = p_cb->conn_id;
            //BTA_GATTC_GetGattDb(p_cb->conn_id, 0x0000, 0xFFFF, &db, &count);
            BTA_GATTC_GetGattDb(p_cb->conn_id, 0x0000, 0xFFFF, &msg.cli_db.db, &msg.cli_db.count);

            if(msg.cli_db.count < 0) {
                msg.cli_db.status = BTA_GATT_WRONG_STATE;
                msg.cli_db.count = 0;
            }

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_REPORT_DB_EVT, &msg); }

            GKI_freebuf(msg.cli_db.db);
            break;
        }

        case BTIF_GATTC_READ_CHAR:
            BTA_GATTC_ReadCharacteristic(p_cb->conn_id, p_cb->handle, p_cb->auth_req);
            break;

        case BTIF_GATTC_READ_CHAR_DESCR:
            BTA_GATTC_ReadCharDescr(p_cb->conn_id, p_cb->handle, p_cb->auth_req);
            break;

        case BTIF_GATTC_WRITE_CHAR:
            BTA_GATTC_WriteCharValue(p_cb->conn_id, p_cb->handle, p_cb->write_type,
                                     p_cb->len, p_cb->value, p_cb->auth_req);
            break;

        case BTIF_GATTC_WRITE_CHAR_DESCR:
            descr_val.len = p_cb->len;
            descr_val.p_value = p_cb->value;
            BTA_GATTC_WriteCharDescr(p_cb->conn_id, p_cb->handle,
                                     p_cb->write_type, &descr_val,
                                     p_cb->auth_req);
            break;

        case BTIF_GATTC_EXECUTE_WRITE:
            BTA_GATTC_ExecuteWrite(p_cb->conn_id, p_cb->action);
            break;

        case BTIF_GATTC_REG_FOR_NOTIFICATION: {
            status = BTA_GATTC_RegisterForNotifications(p_cb->client_if,
                     p_cb->bd_addr.address, p_cb->handle);
            tls_ble_msg_t msg;
            msg.cli_reg_notify.conn_id = p_cb->conn_id;
            msg.cli_reg_notify.status = status;
            msg.cli_reg_notify.reg = true;
            msg.cli_reg_notify.handle = p_cb->handle;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_REG_NOTIFY_EVT, &msg); }

            break;
        }

        case BTIF_GATTC_DEREG_FOR_NOTIFICATION: {
            status = BTA_GATTC_DeregisterForNotifications(p_cb->client_if,
                     p_cb->bd_addr.address, p_cb->handle);
            tls_ble_msg_t msg;
            msg.cli_reg_notify.conn_id = p_cb->conn_id;
            msg.cli_reg_notify.status = status;
            msg.cli_reg_notify.reg = true;
            msg.cli_reg_notify.handle = p_cb->handle;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_CL_REG_NOTIFY_EVT, &msg); }
        }
        break;

        case BTIF_GATTC_REFRESH:
            BTA_GATTC_Refresh(p_cb->bd_addr.address);
            break;

        case BTIF_GATTC_READ_RSSI:
            rssi_request_client_if = p_cb->client_if;
            BTM_ReadRSSI(p_cb->bd_addr.address, (tBTM_CMPL_CB *)btm_read_rssi_cb);
            break;
#if (BLE_VND_INCLUDED == TRUE)

        case BTIF_GATTC_SCAN_FILTER_PARAM_SETUP: {
            btgatt_adv_filter_cb_t *p_adv_filt_cb = (btgatt_adv_filter_cb_t *) p_param;

            if(1 == p_adv_filt_cb->adv_filt_param.dely_mode) {
                BTA_DmBleTrackAdvertiser(p_adv_filt_cb->client_if, bta_track_adv_event_cb);
            }

            BTA_DmBleScanFilterSetup(p_adv_filt_cb->action, p_adv_filt_cb->filt_index,
                                     &p_adv_filt_cb->adv_filt_param, NULL, bta_scan_filt_param_setup_cb,
                                     p_adv_filt_cb->client_if);
            break;
        }

        case BTIF_GATTC_SCAN_FILTER_CONFIG: {
            btgatt_adv_filter_cb_t *p_adv_filt_cb = (btgatt_adv_filter_cb_t *) p_param;
            tBTA_DM_BLE_PF_COND_PARAM cond;
            wm_memset(&cond, 0, sizeof(cond));

            switch(p_adv_filt_cb->filt_type) {
                case BTA_DM_BLE_PF_ADDR_FILTER: // 0
                    bdcpy(cond.target_addr.bda, p_adv_filt_cb->bd_addr.address);
                    cond.target_addr.type = p_adv_filt_cb->addr_type;
                    BTA_DmBleCfgFilterCondition(p_adv_filt_cb->action,
                                                p_adv_filt_cb->filt_type, p_adv_filt_cb->filt_index,
                                                &cond, bta_scan_filt_cfg_cb,
                                                p_adv_filt_cb->client_if);
                    break;

                case BTA_DM_BLE_PF_SRVC_DATA: // 1
                    BTA_DmBleCfgFilterCondition(p_adv_filt_cb->action,
                                                p_adv_filt_cb->filt_type, p_adv_filt_cb->filt_index,
                                                NULL, bta_scan_filt_cfg_cb, p_adv_filt_cb->client_if);
                    break;

                case BTA_DM_BLE_PF_SRVC_UUID: { // 2
                    tBTA_DM_BLE_PF_COND_MASK uuid_mask;
                    cond.srvc_uuid.p_target_addr = NULL;
                    cond.srvc_uuid.cond_logic = BTA_DM_BLE_PF_LOGIC_AND;
                    btif_to_bta_uuid(&cond.srvc_uuid.uuid, &p_adv_filt_cb->uuid);
                    cond.srvc_uuid.p_uuid_mask = NULL;

                    if(p_adv_filt_cb->has_mask) {
                        btif_to_bta_uuid_mask(&uuid_mask, &p_adv_filt_cb->uuid_mask);
                        cond.srvc_uuid.p_uuid_mask = &uuid_mask;
                    }

                    BTA_DmBleCfgFilterCondition(p_adv_filt_cb->action,
                                                p_adv_filt_cb->filt_type, p_adv_filt_cb->filt_index,
                                                &cond, bta_scan_filt_cfg_cb,
                                                p_adv_filt_cb->client_if);
                    break;
                }

                case BTA_DM_BLE_PF_SRVC_SOL_UUID: { // 3
                    cond.solicitate_uuid.p_target_addr = NULL;
                    cond.solicitate_uuid.cond_logic = BTA_DM_BLE_PF_LOGIC_AND;
                    btif_to_bta_uuid(&cond.solicitate_uuid.uuid, &p_adv_filt_cb->uuid);
                    BTA_DmBleCfgFilterCondition(p_adv_filt_cb->action,
                                                p_adv_filt_cb->filt_type, p_adv_filt_cb->filt_index,
                                                &cond, bta_scan_filt_cfg_cb,
                                                p_adv_filt_cb->client_if);
                    break;
                }

                case BTA_DM_BLE_PF_LOCAL_NAME: { // 4
                    cond.local_name.data_len = p_adv_filt_cb->value_len;
                    cond.local_name.p_data = p_adv_filt_cb->value;
                    BTA_DmBleCfgFilterCondition(p_adv_filt_cb->action,
                                                p_adv_filt_cb->filt_type, p_adv_filt_cb->filt_index,
                                                &cond, bta_scan_filt_cfg_cb,
                                                p_adv_filt_cb->client_if);
                    break;
                }

                case BTA_DM_BLE_PF_MANU_DATA: { // 5
                    cond.manu_data.company_id = p_adv_filt_cb->conn_id;
                    cond.manu_data.company_id_mask = p_adv_filt_cb->company_id_mask;
                    cond.manu_data.data_len = p_adv_filt_cb->value_len;
                    cond.manu_data.p_pattern = p_adv_filt_cb->value;
                    cond.manu_data.p_pattern_mask = p_adv_filt_cb->value_mask;
                    BTA_DmBleCfgFilterCondition(p_adv_filt_cb->action,
                                                p_adv_filt_cb->filt_type, p_adv_filt_cb->filt_index,
                                                &cond, bta_scan_filt_cfg_cb,
                                                p_adv_filt_cb->client_if);
                    break;
                }

                case BTA_DM_BLE_PF_SRVC_DATA_PATTERN: { //6
                    cond.srvc_data.data_len = p_adv_filt_cb->value_len;
                    cond.srvc_data.p_pattern = p_adv_filt_cb->value;
                    cond.srvc_data.p_pattern_mask = p_adv_filt_cb->value_mask;
                    BTA_DmBleCfgFilterCondition(p_adv_filt_cb->action,
                                                p_adv_filt_cb->filt_type, p_adv_filt_cb->filt_index,
                                                &cond, bta_scan_filt_cfg_cb,
                                                p_adv_filt_cb->client_if);
                    break;
                }

                default:
                    LOG_ERROR(LOG_TAG, "%s: Unknown filter type (%d)!", __FUNCTION__, p_cb->action);
                    break;
            }

            break;
        }

        case BTIF_GATTC_SCAN_FILTER_CLEAR: {
            btgatt_adv_filter_cb_t *p_adv_filt_cb = (btgatt_adv_filter_cb_t *) p_param;
            BTA_DmBleCfgFilterCondition(BTA_DM_BLE_SCAN_COND_CLEAR, BTA_DM_BLE_PF_TYPE_ALL,
                                        p_adv_filt_cb->filt_index, NULL, bta_scan_filt_cfg_cb,
                                        p_adv_filt_cb->client_if);
            break;
        }

        case BTIF_GATTC_SCAN_FILTER_ENABLE: {
            btgatt_adv_filter_cb_t *p_adv_filt_cb = (btgatt_adv_filter_cb_t *) p_param;
            BTA_DmEnableScanFilter(p_adv_filt_cb->action, bta_scan_filt_status_cb,
                                   p_adv_filt_cb->client_if);
            break;
        }

#endif

        case BTIF_GATTC_LISTEN:
#if (defined(BLE_PERIPHERAL_MODE_SUPPORT) && (BLE_PERIPHERAL_MODE_SUPPORT == TRUE))
            BTA_GATTC_Listen(p_cb->client_if, p_cb->start, NULL);
#else
            BTA_GATTC_Broadcast(p_cb->client_if, p_cb->start);
#endif
            break;

        case BTIF_GATTC_SET_ADV_DATA: {
            break;
        }

#if (BLE_VND_INCLUDED == TRUE)

        case BTIF_GATTC_ADV_INSTANCE_ENABLE: {
            btgatt_multi_adv_inst_cb *p_inst_cb = (btgatt_multi_adv_inst_cb *) p_param;
            int cbindex = -1, arrindex = -1;
            arrindex = btif_multi_adv_add_instid_map(p_inst_cb->client_if, INVALID_ADV_INST, true);

            if(arrindex >= 0) {
                cbindex = btif_gattc_obtain_idx_for_datacb(p_inst_cb->client_if, CLNT_IF_IDX);
            }

            if(cbindex >= 0 && arrindex >= 0) {
                btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();
                wm_memcpy(&p_multi_adv_data_cb->inst_cb[cbindex].param,
                          &p_inst_cb->param, sizeof(tBTA_BLE_ADV_PARAMS));
                p_multi_adv_data_cb->inst_cb[cbindex].timeout_s = p_inst_cb->timeout_s;
                BTIF_TRACE_DEBUG("%s, client_if value: %d", __FUNCTION__,
                                 p_multi_adv_data_cb->clntif_map[arrindex + arrindex]);
                BTA_BleEnableAdvInstance(&(p_multi_adv_data_cb->inst_cb[cbindex].param),
                                         bta_gattc_multi_adv_cback,
                                         &(p_multi_adv_data_cb->clntif_map[arrindex + arrindex]));
            } else {
                /* let the error propagate up from BTA layer */
                BTIF_TRACE_ERROR("%s invalid index in BTIF_GATTC_ENABLE_ADV", __FUNCTION__);
                BTA_BleEnableAdvInstance(&p_inst_cb->param, bta_gattc_multi_adv_cback, NULL);
            }

            break;
        }

        case BTIF_GATTC_ADV_INSTANCE_UPDATE: {
            btgatt_multi_adv_inst_cb *p_inst_cb = (btgatt_multi_adv_inst_cb *) p_param;
            int inst_id = btif_multi_adv_instid_for_clientif(p_inst_cb->client_if);
            int cbindex = btif_gattc_obtain_idx_for_datacb(p_inst_cb->client_if, CLNT_IF_IDX);

            if(inst_id >= 0 && cbindex >= 0 && NULL != p_inst_cb) {
                btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();
                wm_memcpy(&p_multi_adv_data_cb->inst_cb[cbindex].param, &p_inst_cb->param,
                          sizeof(tBTA_BLE_ADV_PARAMS));
                BTA_BleUpdateAdvInstParam((uint8_t)inst_id,
                                          &(p_multi_adv_data_cb->inst_cb[cbindex].param));
            } else {
                BTIF_TRACE_ERROR("%s invalid index in BTIF_GATTC_UPDATE_ADV", __FUNCTION__);
            }

            break;
        }

        case BTIF_GATTC_ADV_INSTANCE_SET_DATA: {
            btif_adv_data_t *p_adv_data = (btif_adv_data_t *) p_param;
            int cbindex = btif_gattc_obtain_idx_for_datacb(p_adv_data->client_if, CLNT_IF_IDX);
            int inst_id = btif_multi_adv_instid_for_clientif(p_adv_data->client_if);

            if(inst_id >= 0 && cbindex >= 0 && btif_gattc_copy_datacb(cbindex, p_adv_data, true)) {
                btgatt_multi_adv_common_data *p_multi_adv_data_cb =
                                btif_obtain_multi_adv_data_cb();
                BTA_BleCfgAdvInstData(
                                (uint8_t)inst_id,
                                p_adv_data->set_scan_rsp,
                                p_multi_adv_data_cb->inst_cb[cbindex].mask,
                                &p_multi_adv_data_cb->inst_cb[cbindex].data);
            } else {
                BTIF_TRACE_ERROR(
                                "%s:%s: failed to get invalid instance data: inst_id:%d "
                                "cbindex:%d",
                                __func__, "BTIF_GATTC_ADV_INSTANCE_SET_DATA", inst_id, cbindex);
            }

            break;
        }

        case BTIF_GATTC_ADV_INSTANCE_DISABLE: {
            btgatt_multi_adv_inst_cb *p_inst_cb = (btgatt_multi_adv_inst_cb *) p_param;
            int inst_id = btif_multi_adv_instid_for_clientif(p_inst_cb->client_if);

            if(inst_id >= 0) {
                BTA_BleDisableAdvInstance((uint8_t)inst_id);
            } else {
                BTIF_TRACE_ERROR("%s invalid instance ID in BTIF_GATTC_DISABLE_ADV", __FUNCTION__);
            }

            break;
        }

#endif

        case BTIF_GATTC_CONFIGURE_MTU:
            BTA_GATTC_ConfigureMTU(p_cb->conn_id, p_cb->len);
            break;
#if (BLE_VND_INCLUDED == TRUE)

        case BTIF_GATTC_CONFIG_STORAGE_PARAMS: {
            btgatt_batch_track_cb_t *p_scan_track_cb = (btgatt_batch_track_cb_t *) p_param;
            BTA_DmBleSetStorageParams(p_scan_track_cb->batch_scan_full_max,
                                      p_scan_track_cb->batch_scan_trunc_max, p_scan_track_cb->batch_scan_notify_threshold,
                                      bta_batch_scan_setup_cb, bta_batch_scan_threshold_cb, bta_batch_scan_reports_cb,
                                      (tBTA_DM_BLE_REF_VALUE) p_scan_track_cb->client_if);
            break;
        }

        case BTIF_GATTC_ENABLE_BATCH_SCAN: {
            btgatt_batch_track_cb_t *p_scan_track_cb = (btgatt_batch_track_cb_t *) p_param;
            BTA_DmBleEnableBatchScan(p_scan_track_cb->scan_mode, p_scan_track_cb->scan_interval,
                                     p_scan_track_cb->scan_window, p_scan_track_cb->discard_rule,
                                     p_scan_track_cb->addr_type, p_scan_track_cb->client_if);
            break;
        }

        case BTIF_GATTC_DISABLE_BATCH_SCAN: {
            btgatt_batch_track_cb_t *p_scan_track_cb = (btgatt_batch_track_cb_t *) p_param;
            BTA_DmBleDisableBatchScan(p_scan_track_cb->client_if);
            break;
        }

        case BTIF_GATTC_READ_BATCH_SCAN_REPORTS: {
            btgatt_batch_track_cb_t *p_scan_track_cb = (btgatt_batch_track_cb_t *) p_param;
            BTA_DmBleReadScanReports(p_scan_track_cb->scan_mode, p_scan_track_cb->client_if);
            break;
        }

#endif

        default:
            LOG_ERROR(LOG_TAG, "%s: Unknown event (%d)!", __FUNCTION__, event);
            break;
    }

    btgattc_free_event_data(event, p_param);
}

/*******************************************************************************
**  Client API Functions
********************************************************************************/

tls_bt_status_t tls_ble_client_app_init(tls_ble_callback_t callback)
{
    if(tls_ble_cb == NULL) {
        tls_ble_cb = callback;
    } else {
        return TLS_BT_STATUS_DONE;
    }

    return TLS_BT_STATUS_SUCCESS;
}

tls_bt_status_t tls_ble_client_app_deinit()
{
    if(tls_ble_cb) {
        tls_ble_cb = NULL;
    } else {
        return TLS_BT_STATUS_DONE;
    }

    return TLS_BT_STATUS_SUCCESS;
}
tls_bt_status_t tls_ble_client_app_register(tls_bt_uuid_t *uuid)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    wm_memcpy(&btif_cb.uuid, uuid, sizeof(tls_bt_uuid_t));
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_REGISTER_APP,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}
tls_bt_status_t tls_ble_client_app_unregister(uint8_t client_if)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_UNREGISTER_APP,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

tls_bt_status_t tls_ble_client_connect(uint8_t client_if,
                                       const tls_bt_addr_t *bd_addr, uint8_t is_direct, int transport)
{
    uint8_t adv_state;
    uint8_t scan_state;
    CHECK_BTGATT_INIT();
    adv_state = BTA_DmGetBleAdvertiseState();
    scan_state = BTA_DmGetBleScanState();

    if(adv_state) {
        return TLS_BT_STATUS_UNSUPPORTED;
    }

    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    btif_cb.is_direct = is_direct ? 1 : 0;
    btif_cb.transport = (btgatt_transport_t)transport;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_OPEN,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

tls_bt_status_t tls_ble_client_disconnect(uint8_t client_if, const tls_bt_addr_t *bd_addr,
        int conn_id)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    btif_cb.conn_id = (uint16_t) conn_id;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_CLOSE,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}
tls_bt_status_t tls_ble_client_listen(uint8_t client_if, uint8_t start)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    btif_cb.start = start ? 1 : 0;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_LISTEN,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

tls_bt_status_t tls_ble_client_refresh(uint8_t client_if, const tls_bt_addr_t *bd_addr)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_REFRESH,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}
tls_bt_status_t tls_ble_client_search_service(uint16_t conn_id, tls_bt_uuid_t *filter_uuid)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.search_all = filter_uuid ? 0 : 1;

    if(filter_uuid) {
        wm_memcpy(&btif_cb.uuid, filter_uuid, sizeof(tls_bt_uuid_t));
    }

    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SEARCH_SERVICE,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

tls_bt_status_t tls_ble_client_write_characteristic(uint16_t conn_id, uint16_t handle,
        int write_type, int len, int auth_req, char *p_value)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.handle = (uint16_t) handle;
    btif_cb.auth_req = (uint8_t) auth_req;
    btif_cb.write_type = (uint8_t) write_type;
    btif_cb.len = len > BTGATT_MAX_ATTR_LEN ? BTGATT_MAX_ATTR_LEN : len;
    wm_memcpy(btif_cb.value, p_value, btif_cb.len);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_WRITE_CHAR,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}
tls_bt_status_t tls_ble_client_read_characteristic(uint16_t conn_id, uint16_t handle, int auth_req)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.handle = (uint16_t) handle;
    btif_cb.auth_req = (uint8_t) auth_req;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_READ_CHAR,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

tls_bt_status_t tls_ble_client_read_descriptor(uint16_t conn_id, uint16_t handle, int auth_req)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.handle = (uint16_t) handle;
    btif_cb.auth_req = (uint8_t) auth_req;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_READ_CHAR_DESCR,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}
tls_bt_status_t tls_ble_client_write_descriptor(uint16_t conn_id, uint16_t handle, int write_type,
        int len, int auth_req, char *p_value)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.handle = (uint16_t) handle;
    btif_cb.auth_req = (uint8_t) auth_req;
    btif_cb.write_type = (uint8_t) write_type;
    btif_cb.len = len > BTGATT_MAX_ATTR_LEN ? BTGATT_MAX_ATTR_LEN : len;
    wm_memcpy(btif_cb.value, p_value, btif_cb.len);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_WRITE_CHAR_DESCR,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}
tls_bt_status_t tls_ble_client_execute_write(uint16_t conn_id, int execute)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.action = (uint8_t) execute;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_EXECUTE_WRITE,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}
tls_bt_status_t tls_ble_client_register_for_notification(int client_if,
        const tls_bt_addr_t *bd_addr, uint16_t handle, uint16_t conn_id)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    btif_cb.handle = handle;
    btif_cb.conn_id = conn_id;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_REG_FOR_NOTIFICATION,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

tls_bt_status_t tls_ble_client_deregister_for_notification(int client_if,
        const tls_bt_addr_t *bd_addr, uint16_t handle, uint16_t conn_id)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    btif_cb.handle = handle;
    btif_cb.conn_id = conn_id;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_DEREG_FOR_NOTIFICATION,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}
tls_bt_status_t tls_ble_client_configure_mtu(uint16_t conn_id, uint16_t mtu)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = conn_id;
    btif_cb.len = mtu; // Re-use len field
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_CONFIGURE_MTU,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

tls_bt_status_t tls_ble_client_get_gatt_db(uint16_t conn_id)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_GET_GATT_DB,
                                 (char *) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}



#endif
