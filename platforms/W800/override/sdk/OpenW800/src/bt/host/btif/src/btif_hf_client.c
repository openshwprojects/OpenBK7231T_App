/******************************************************************************
 *
 *  Copyright (c) 2014 The Android Open Source Project
 *  Copyright (C) 2009-2012 Broadcom Corporation
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
 *  Filename:      btif_hf_client.c
 *
 *  Description:   Handsfree Profile (HF role) Bluetooth Interface
 *
 *
 ***********************************************************************************/

#define LOG_TAG "bt_btif_hfc"

#include <stdlib.h>
#include <string.h>
#include "bt_target.h"
#if defined(BTA_HFP_HSP_INCLUDED) && (BTA_HFP_HSP_INCLUDED == TRUE)
#include "bluetooth.h"
#include "bt_hf_client.h"

#include "bt_utils.h"
#include "bta_hf_client_api.h"
#include "btcore/include/bdaddr.h"
#include "btif_common.h"
#include "btif_profile_queue.h"
#include "btif_util.h"
#include "wm_bt_def.h"
#include "wm_bt_hf_client.h"

/************************************************************************************
**  Constants & Macros
************************************************************************************/

#ifndef BTIF_HF_CLIENT_SERVICE_NAME
#define BTIF_HF_CLIENT_SERVICE_NAME ("Handsfree")
#endif

#ifndef BTIF_HF_CLIENT_SECURITY
#define BTIF_HF_CLIENT_SECURITY    (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTIF_HF_CLIENT_FEATURES
#define BTIF_HF_CLIENT_FEATURES   ( BTA_HF_CLIENT_FEAT_ECNR  | \
                                    BTA_HF_CLIENT_FEAT_3WAY  | \
                                    BTA_HF_CLIENT_FEAT_CLI   | \
                                    BTA_HF_CLIENT_FEAT_VREC  | \
                                    BTA_HF_CLIENT_FEAT_VOL   | \
                                    BTA_HF_CLIENT_FEAT_ECS   | \
                                    BTA_HF_CLIENT_FEAT_ECC   | \
                                    BTA_HF_CLIENT_FEAT_CODEC)
#endif

/************************************************************************************
**  Local type definitions
************************************************************************************/

/************************************************************************************
**  Static variables
************************************************************************************/
static tls_bthf_client_callback_t bt_hf_client_callbacks = NULL;
static uint32_t btif_hf_client_features = 0;

const char *btif_hf_client_version = "1.6";


#define CHECK_BTHF_CLIENT_INIT() if (bt_hf_client_callbacks == NULL)\
    {\
        BTIF_TRACE_WARNING("BTHF CLIENT: %s: not initialized", __FUNCTION__);\
        return TLS_BT_STATUS_NOT_READY;\
    }\
    else\
    {\
        BTIF_TRACE_EVENT("BTHF CLIENT: %s", __FUNCTION__);\
    }

#define CHECK_BTHF_CLIENT_SLC_CONNECTED() if (bt_hf_client_callbacks == NULL)\
    {\
        BTIF_TRACE_WARNING("BTHF CLIENT: %s: not initialized", __FUNCTION__);\
        return TLS_BT_STATUS_NOT_READY;\
    }\
    else if (btif_hf_client_cb.state != BTHF_CLIENT_CONNECTION_STATE_SLC_CONNECTED)\
    {\
        BTIF_TRACE_WARNING("BTHF CLIENT: %s: SLC connection not up. state=%s",\
                           __FUNCTION__, \
                           dump_hf_conn_state(btif_hf_client_cb.state));\
        return TLS_BT_STATUS_NOT_READY;\
    }\
    else\
    {\
        BTIF_TRACE_EVENT("BTHF CLIENT: %s", __FUNCTION__);\
    }

/* BTIF-HF control block to map bdaddr to BTA handle */
typedef struct {
    uint16_t                          handle;
    tls_bt_addr_t                     connected_bda;
    bthf_client_connection_state_t  state;
    bthf_client_vr_state_t          vr_state;
    tBTA_HF_CLIENT_PEER_FEAT        peer_feat;
    tBTA_HF_CLIENT_CHLD_FEAT        chld_feat;
} btif_hf_client_cb_t;

static btif_hf_client_cb_t btif_hf_client_cb;

/************************************************************************************
**  Static functions
************************************************************************************/

/*******************************************************************************
**
** Function        btif_in_hf_client_generic_evt
**
** Description     Processes generic events to be sent to JNI that are not triggered from the BTA.
**                 Always runs in BTIF context
**
** Returns          void
**
*******************************************************************************/
static void btif_in_hf_client_generic_evt(uint16_t event, char *p_param)
{
    tls_bthf_client_msg_t msg;
    UNUSED(p_param);
    BTIF_TRACE_EVENT("%s: event=%d", __FUNCTION__, event);

    switch(event) {
        case BTIF_HF_CLIENT_CB_AUDIO_CONNECTING: {
            msg.audio_state_msg.state = BTHF_AUDIO_STATE_CONNECTING;
            msg.audio_state_msg.bd_addr = &btif_hf_client_cb.connected_bda;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_AUDIO_STATE_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, audio_state_cb, (bthf_client_audio_state_t)BTHF_AUDIO_STATE_CONNECTING, &btif_hf_client_cb.connected_bda);
        }
        break;

        default: {
            BTIF_TRACE_WARNING("%s : Unknown event 0x%x", __FUNCTION__, event);
        }
        break;
    }
}

/************************************************************************************
**  Externs
************************************************************************************/

/************************************************************************************
**  Functions
************************************************************************************/

static void clear_state(void)
{
    wm_memset(&btif_hf_client_cb, 0, sizeof(btif_hf_client_cb_t));
}

static uint8_t is_connected(tls_bt_addr_t *bd_addr)
{
    if(((btif_hf_client_cb.state == BTHF_CLIENT_CONNECTION_STATE_CONNECTED) ||
            (btif_hf_client_cb.state == BTHF_CLIENT_CONNECTION_STATE_SLC_CONNECTED)) &&
            ((bd_addr == NULL) || (bdcmp(bd_addr->address, btif_hf_client_cb.connected_bda.address) == 0))) {
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************
**   Section name (Group of functions)
*****************************************************************************/

/*****************************************************************************
**
**   btif hf api functions (no context switch)
**
*****************************************************************************/




static tls_bt_status_t connect_int(tls_bt_addr_t *bd_addr, uint16_t uuid)
{
    if(is_connected(bd_addr)) {
        return TLS_BT_STATUS_BUSY;
    }

    btif_hf_client_cb.state = BTHF_CLIENT_CONNECTION_STATE_CONNECTING;
    bdcpy(btif_hf_client_cb.connected_bda.address, bd_addr->address);
    BTA_HfClientOpen(btif_hf_client_cb.handle, btif_hf_client_cb.connected_bda.address,
                     BTIF_HF_CLIENT_SECURITY);
    return TLS_BT_STATUS_SUCCESS;
}


static void process_ind_evt(tBTA_HF_CLIENT_IND *ind)
{
    tls_bthf_client_msg_t msg;

    switch(ind->type) {
        case BTA_HF_CLIENT_IND_CALL:
            msg.call_msg.call = ind->value;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_CALL_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, call_cb, ind->value);
            break;

        case BTA_HF_CLIENT_IND_CALLSETUP:
            msg.callsetup_msg.callsetup = ind->value;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_CALLSETUP_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, callsetup_cb, ind->value);
            break;

        case BTA_HF_CLIENT_IND_CALLHELD:
            msg.callheld_msg.callheld = ind->value;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_CALLHELD_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, callheld_cb, ind->value);
            break;

        case BTA_HF_CLIENT_IND_SERVICE:
            msg.network_state_msg.state = ind->value;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_NETWORK_STATE_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, network_state_cb, ind->value);
            break;

        case BTA_HF_CLIENT_IND_SIGNAL:
            msg.network_signal_msg.signal_strength = ind->value;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_NETWORK_SIGNAL_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, network_signal_cb, ind->value);
            break;

        case BTA_HF_CLIENT_IND_ROAM:
            msg.network_roaming_msg.type = ind->value;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_NETWORK_ROAMING_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, network_roaming_cb, ind->value);
            break;

        case BTA_HF_CLIENT_IND_BATTCH:
            msg.battery_level_msg.battery_level = ind->value;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_BATTERY_LEVEL_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, battery_level_cb, ind->value);
            break;

        default:
            break;
    }
}

/*******************************************************************************
**
** Function         btif_hf_client_upstreams_evt
**
** Description      Executes HF CLIENT UPSTREAMS events in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_hf_client_upstreams_evt(uint16_t event, char *p_param)
{
    tBTA_HF_CLIENT *p_data = (tBTA_HF_CLIENT *)p_param;
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)	
    bdstr_t bdstr;
#endif
    tls_bthf_client_msg_t msg;
    BTIF_TRACE_DEBUG("%s: event=%s (%u)", __FUNCTION__, dump_hf_client_event(event), event);

    switch(event) {
        case BTA_HF_CLIENT_ENABLE_EVT:
        case BTA_HF_CLIENT_DISABLE_EVT:
            break;

        case BTA_HF_CLIENT_REGISTER_EVT:
            btif_hf_client_cb.handle = p_data->reg.handle;
            break;

        case BTA_HF_CLIENT_OPEN_EVT:
            if(p_data->open.status == BTA_HF_CLIENT_SUCCESS) {
                bdcpy(btif_hf_client_cb.connected_bda.address, p_data->open.bd_addr);
                btif_hf_client_cb.state = BTHF_CLIENT_CONNECTION_STATE_CONNECTED;
                btif_hf_client_cb.peer_feat = 0;
                btif_hf_client_cb.chld_feat = 0;
                //clear_phone_state();
            } else if(btif_hf_client_cb.state == BTHF_CLIENT_CONNECTION_STATE_CONNECTING) {
                btif_hf_client_cb.state = BTHF_CLIENT_CONNECTION_STATE_DISCONNECTED;
            } else {
                BTIF_TRACE_WARNING("%s: HF CLient open failed, but another device connected. status=%d state=%d connected device=%s",
                                   __FUNCTION__, p_data->open.status, btif_hf_client_cb.state,
                                   bdaddr_to_string(&btif_hf_client_cb.connected_bda, bdstr, sizeof(bdstr)));
                break;
            }

            msg.connection_state_msg.bd_addr = &btif_hf_client_cb.connected_bda;
            msg.connection_state_msg.state = btif_hf_client_cb.state;
            msg.connection_state_msg.chld_feat = 0;
            msg.connection_state_msg.peer_feat = 0;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_CONNECTION_STATE_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, connection_state_cb, btif_hf_client_cb.state,0, 0, &btif_hf_client_cb.connected_bda);

            if(btif_hf_client_cb.state == BTHF_CLIENT_CONNECTION_STATE_DISCONNECTED) {
                bdsetany(btif_hf_client_cb.connected_bda.address);
            }

            if(p_data->open.status != BTA_HF_CLIENT_SUCCESS) {
                btif_queue_advance();
            }

            break;

        case BTA_HF_CLIENT_CONN_EVT:
            btif_hf_client_cb.peer_feat = p_data->conn.peer_feat;
            btif_hf_client_cb.chld_feat = p_data->conn.chld_feat;
            btif_hf_client_cb.state = BTHF_CLIENT_CONNECTION_STATE_SLC_CONNECTED;
            msg.connection_state_msg.bd_addr = &btif_hf_client_cb.connected_bda;
            msg.connection_state_msg.state = btif_hf_client_cb.state;
            msg.connection_state_msg.chld_feat = btif_hf_client_cb.chld_feat;
            msg.connection_state_msg.peer_feat = btif_hf_client_cb.peer_feat;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_CONNECTION_STATE_EVT, &msg); }

#if 0
            HAL_CBACK(bt_hf_client_callbacks, connection_state_cb, btif_hf_client_cb.state,
                      btif_hf_client_cb.peer_feat, btif_hf_client_cb.chld_feat, &btif_hf_client_cb.connected_bda);
#endif

            /* Inform the application about in-band ringtone */
            if(btif_hf_client_cb.peer_feat & BTA_HF_CLIENT_PEER_INBAND) {
                msg.in_band_ring_tone_msg.state = BTHF_CLIENT_IN_BAND_RINGTONE_PROVIDED;

                if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_IN_BAND_RING_TONE_EVT, &msg); }

                //HAL_CBACK(bt_hf_client_callbacks, in_band_ring_tone_cb, BTHF_CLIENT_IN_BAND_RINGTONE_PROVIDED);
            }

            btif_queue_advance();
            break;

        case BTA_HF_CLIENT_CLOSE_EVT:
            btif_hf_client_cb.state = BTHF_CLIENT_CONNECTION_STATE_DISCONNECTED;
            msg.connection_state_msg.bd_addr = &btif_hf_client_cb.connected_bda;
            msg.connection_state_msg.state = btif_hf_client_cb.state;
            msg.connection_state_msg.chld_feat = 0;
            msg.connection_state_msg.peer_feat = 0;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_CONNECTION_STATE_EVT, &msg); }

#if 0
            HAL_CBACK(bt_hf_client_callbacks, connection_state_cb,  btif_hf_client_cb.state,
                      0, 0, &btif_hf_client_cb.connected_bda);
#endif
            bdsetany(btif_hf_client_cb.connected_bda.address);
            btif_hf_client_cb.peer_feat = 0;
            btif_hf_client_cb.chld_feat = 0;
            btif_queue_advance();
            break;

        case BTA_HF_CLIENT_IND_EVT:
            process_ind_evt(&p_data->ind);
            break;

        case BTA_HF_CLIENT_MIC_EVT:
            msg.volume_change_msg.type = BTHF_CLIENT_VOLUME_TYPE_MIC;
            msg.volume_change_msg.volume = p_data->val.value;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_VOLUME_CHANGE_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, volume_change_cb, BTHF_CLIENT_VOLUME_TYPE_MIC, p_data->val.value);
            break;

        case BTA_HF_CLIENT_SPK_EVT:
            msg.volume_change_msg.type = BTHF_CLIENT_VOLUME_TYPE_SPK;
            msg.volume_change_msg.volume = p_data->val.value;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_VOLUME_CHANGE_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, volume_change_cb, BTHF_CLIENT_VOLUME_TYPE_SPK, p_data->val.value);
            break;

        case BTA_HF_CLIENT_VOICE_REC_EVT:
            msg.vr_cmd_msg.state = p_data->val.value;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_VR_CMD_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, vr_cmd_cb, p_data->val.value);
            break;

        case BTA_HF_CLIENT_OPERATOR_NAME_EVT:
            msg.current_operator_msg.name = p_data->operator.name;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_CURRENT_OPERATOR_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, current_operator_cb, p_data->operator.name);
            break;

        case BTA_HF_CLIENT_CLIP_EVT:
            msg.clip_msg.number = p_data->number.number;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_CLIP_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, clip_cb, p_data->number.number);
            break;

        case BTA_HF_CLIENT_BINP_EVT:
            msg.last_voice_tag_number_msg.number = p_data->number.number;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_LAST_VOICE_TAG_NUMBER_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, last_voice_tag_number_callback, p_data->number.number);
            break;

        case BTA_HF_CLIENT_CCWA_EVT:
            msg.call_waiting_msg.number = p_data->number.number;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_CALL_WAITING_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, call_waiting_cb, p_data->number.number);
            break;

        case BTA_HF_CLIENT_AT_RESULT_EVT:
            msg.cmd_complete_msg.cme = p_data->result.cme;
            msg.cmd_complete_msg.type = p_data->result.type;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_CMD_COMPLETE_EVT, &msg); }

            //HAL_CBACK(bt_hf_client_callbacks, cmd_complete_cb, p_data->result.type, p_data->result.cme);
            break;

        case BTA_HF_CLIENT_CLCC_EVT:
            msg.current_calls_msg.index = p_data->clcc.idx;
            msg.current_calls_msg.dir = p_data->clcc.inc ? BTHF_CLIENT_CALL_DIRECTION_INCOMING :
                                        BTHF_CLIENT_CALL_DIRECTION_OUTGOING;
            msg.current_calls_msg.state = p_data->clcc.status;
            msg.current_calls_msg.number = p_data->clcc.number_present ? p_data->clcc.number : NULL;
            msg.current_calls_msg.mpty = p_data->clcc.mpty ? BTHF_CLIENT_CALL_MPTY_TYPE_MULTI :
                                         BTHF_CLIENT_CALL_MPTY_TYPE_SINGLE;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_CURRENT_CALLS_EVT, &msg); }

#if 0
            HAL_CBACK(bt_hf_client_callbacks, current_calls_cb, p_data->clcc.idx,
                      p_data->clcc.inc ? BTHF_CLIENT_CALL_DIRECTION_INCOMING : BTHF_CLIENT_CALL_DIRECTION_OUTGOING,
                      p_data->clcc.status,
                      p_data->clcc.mpty ? BTHF_CLIENT_CALL_MPTY_TYPE_MULTI : BTHF_CLIENT_CALL_MPTY_TYPE_SINGLE,
                      p_data->clcc.number_present ? p_data->clcc.number : NULL);
#endif
            break;

        case BTA_HF_CLIENT_CNUM_EVT:
            if(p_data->cnum.service == 4) {
                msg.subscriber_info_msg.type = BTHF_CLIENT_SERVICE_VOICE;
                msg.subscriber_info_msg.name = p_data->cnum.number;
            } else if(p_data->cnum.service == 5) {
                msg.subscriber_info_msg.type = BTHF_CLIENT_SERVICE_FAX;
                msg.subscriber_info_msg.name = p_data->cnum.number;
            } else {
                msg.subscriber_info_msg.type = BTHF_CLIENT_SERVICE_UNKNOWN;
                msg.subscriber_info_msg.name = p_data->cnum.number;
            }

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_SUBSCRIBER_INFO_EVT, &msg); }

            break;

        case BTA_HF_CLIENT_BTRH_EVT:
            if(p_data->val.value <= BTRH_CLIENT_RESP_AND_HOLD_REJECT) {
                msg.resp_and_hold_msg.resp_and_hold = p_data->val.value;

                if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_RESP_AND_HOLD_EVT, &msg); }

                //HAL_CBACK(bt_hf_client_callbacks, resp_and_hold_cb, p_data->val.value);
            }

            break;

        case BTA_HF_CLIENT_BSIR_EVT:
            if(p_data->val.value != 0) {
                msg.in_band_ring_tone_msg.state = BTHF_CLIENT_IN_BAND_RINGTONE_PROVIDED;
            } else {
                msg.in_band_ring_tone_msg.state = BTHF_CLIENT_IN_BAND_RINGTONE_NOT_PROVIDED;
            }

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_IN_BAND_RING_TONE_EVT, &msg); }

            break;

        case BTA_HF_CLIENT_AUDIO_OPEN_EVT:
            msg.audio_state_msg.state = BTHF_CLIENT_AUDIO_STATE_CONNECTED;
            msg.audio_state_msg.bd_addr = &btif_hf_client_cb.connected_bda;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_AUDIO_STATE_EVT, &msg); }

            break;

        case BTA_HF_CLIENT_AUDIO_MSBC_OPEN_EVT:
            msg.audio_state_msg.state = BTHF_CLIENT_AUDIO_STATE_CONNECTED_MSBC;
            msg.audio_state_msg.bd_addr = &btif_hf_client_cb.connected_bda;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_AUDIO_STATE_EVT, &msg); }

            break;

        case BTA_HF_CLIENT_AUDIO_CLOSE_EVT:
            msg.audio_state_msg.state = BTHF_CLIENT_AUDIO_STATE_DISCONNECTED;
            msg.audio_state_msg.bd_addr = &btif_hf_client_cb.connected_bda;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_AUDIO_STATE_EVT, &msg); }

            break;

        case BTA_HF_CLIENT_RING_INDICATION:
            msg.ring_indication_msg.ring = 1;

            if(bt_hf_client_callbacks) { bt_hf_client_callbacks(WM_BTHF_CLIENT_RING_INDICATION_EVT, &msg); }

            break;

        default:
            BTIF_TRACE_WARNING("%s: Unhandled event: %d", __FUNCTION__, event);
            break;
    }
}

/*******************************************************************************
**
** Function         bte_hf_client_evt
**
** Description      Switches context from BTE to BTIF for all HF Client events
**
** Returns          void
**
*******************************************************************************/

static void bte_hf_client_evt(tBTA_HF_CLIENT_EVT event, tBTA_HF_CLIENT *p_data)
{
    tls_bt_status_t status;
    /* switch context to btif task context (copy full union size for convenience) */
    status = btif_transfer_context(btif_hf_client_upstreams_evt, (uint16_t)event, (void *)p_data,
                                   sizeof(*p_data), NULL);
    /* catch any failed context transfers */
    ASSERTC(status == TLS_BT_STATUS_SUCCESS, "context transfer failed", status);
}

/*******************************************************************************
**
** Function         btif_hf_client_execute_service
**
** Description      Initializes/Shuts down the service
**
** Returns          BT_STATUS_SUCCESS on success, BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_hf_client_execute_service(uint8_t b_enable)
{
    BTIF_TRACE_EVENT("%s enable:%d", __FUNCTION__, b_enable);

    if(b_enable) {
        /* Enable and register with BTA-HFClient */
        BTA_HfClientEnable(bte_hf_client_evt);

        if(strcmp(btif_hf_client_version, "1.6") == 0) {
            BTIF_TRACE_EVENT("Support Codec Nego. %d ", BTIF_HF_CLIENT_FEATURES);
            BTA_HfClientRegister(BTIF_HF_CLIENT_SECURITY, BTIF_HF_CLIENT_FEATURES,
                                 BTIF_HF_CLIENT_SERVICE_NAME);
        } else {
            BTIF_TRACE_EVENT("No Codec Nego Supported");
            btif_hf_client_features = BTIF_HF_CLIENT_FEATURES;
            btif_hf_client_features = btif_hf_client_features & (~BTA_HF_CLIENT_FEAT_CODEC);
            BTIF_TRACE_EVENT("btif_hf_client_features is   %d", btif_hf_client_features);
            BTA_HfClientRegister(BTIF_HF_CLIENT_SECURITY, btif_hf_client_features,
                                 BTIF_HF_CLIENT_SERVICE_NAME);
        }
    } else {
        BTA_HfClientDeregister(btif_hf_client_cb.handle);
        BTA_HfClientDisable();
    }

    return TLS_BT_STATUS_SUCCESS;
}



/**exported api*/

/*******************************************************************************
**
** Function         tls_bt_hf_client_init
**
** Description     initializes the hf interface
**
** Returns         bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_init(tls_bthf_client_callback_t callback)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    bt_hf_client_callbacks = callback;
    btif_enable_service(BTA_HFP_HS_SERVICE_ID);
    clear_state();
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         tls_bt_hf_client_deinit
**
** Description      Closes the HF interface
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_deinit(void)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);

    if(bt_hf_client_callbacks) {
        btif_disable_service(BTA_HFP_HS_SERVICE_ID);
        bt_hf_client_callbacks = NULL;
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         tls_bt_hf_client_connect
**
** Description     connect to audio gateway
**
** Returns         bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_connect(tls_bt_addr_t *bd_addr)
{
    BTIF_TRACE_EVENT("HFP Client version is  %s", btif_hf_client_version);
    CHECK_BTHF_CLIENT_INIT();
    return btif_queue_connect(UUID_SERVCLASS_HF_HANDSFREE, bd_addr, connect_int);
}

/*******************************************************************************
**
** Function         tls_bt_hf_client_disconnect
**
** Description      disconnect from audio gateway
**
** Returns         bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_disconnect(tls_bt_addr_t *bd_addr)
{
    CHECK_BTHF_CLIENT_INIT();

    if(is_connected(bd_addr)) {
        BTA_HfClientClose(btif_hf_client_cb.handle);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_FAIL;
}
/*******************************************************************************
**
** Function         tls_bt_hf_client_connect_audio
**
** Description     create an audio connection
**
** Returns         bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_connect_audio(tls_bt_addr_t *bd_addr)
{
    CHECK_BTHF_CLIENT_SLC_CONNECTED();

    if(is_connected(bd_addr)) {
        if((BTIF_HF_CLIENT_FEATURES & BTA_HF_CLIENT_FEAT_CODEC) &&
                (btif_hf_client_cb.peer_feat & BTA_HF_CLIENT_PEER_CODEC)) {
            BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_BCC, 0, 0, NULL);
        } else {
            BTA_HfClientAudioOpen(btif_hf_client_cb.handle);
        }

        /* Inform the application that the audio connection has been initiated successfully */
        btif_transfer_context(btif_in_hf_client_generic_evt, BTIF_HF_CLIENT_CB_AUDIO_CONNECTING,
                              (char *)bd_addr, sizeof(tls_bt_addr_t), NULL);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_FAIL;
}


/*******************************************************************************
**
** Function         disconnect_audio
**
** Description      close the audio connection
**
** Returns         bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_disconnect_audio(tls_bt_addr_t *bd_addr)
{
    CHECK_BTHF_CLIENT_SLC_CONNECTED();

    if(is_connected(bd_addr)) {
        BTA_HfClientAudioClose(btif_hf_client_cb.handle);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         tls_bt_hf_client_start_voice_recognition
**
** Description      start voice recognition
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_start_voice_recognition(void)
{
    CHECK_BTHF_CLIENT_SLC_CONNECTED();

    if(btif_hf_client_cb.peer_feat & BTA_HF_CLIENT_PEER_FEAT_VREC) {
        BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_BVRA, 1, 0, NULL);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_UNSUPPORTED;
}

/*******************************************************************************
**
** Function         tls_bt_hf_client_stop_voice_recognition
**
** Description      stop voice recognition
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_stop_voice_recognition(void)
{
    CHECK_BTHF_CLIENT_SLC_CONNECTED();

    if(btif_hf_client_cb.peer_feat & BTA_HF_CLIENT_PEER_FEAT_VREC) {
        BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_BVRA, 0, 0, NULL);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_UNSUPPORTED;
}
/*******************************************************************************
**
** Function         tls_bt_hf_client_volume_control
**
** Description      volume control
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_volume_control(tls_bthf_client_volume_type_t type, int volume)
{
    CHECK_BTHF_CLIENT_SLC_CONNECTED();

    switch(type) {
        case BTHF_CLIENT_VOLUME_TYPE_SPK:
            BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_VGS, volume, 0, NULL);
            break;

        case BTHF_CLIENT_VOLUME_TYPE_MIC:
            BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_VGM, volume, 0, NULL);
            break;

        default:
            return TLS_BT_STATUS_UNSUPPORTED;
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         tls_bt_hf_client_dial
**
** Description      place a call
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_dial(const char *number)
{
    CHECK_BTHF_CLIENT_SLC_CONNECTED();

    if(number) {
        BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_ATD, 0, 0, number);
    } else {
        BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_BLDN, 0, 0, NULL);
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         tls_bt_hf_client_dial_memory
**
** Description      place a call with number specified by location (speed dial)
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_dial_memory(int location)
{
    CHECK_BTHF_CLIENT_SLC_CONNECTED();
    BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_ATD, location, 0, NULL);
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         tls_bt_hf_client_handle_call_action
**
** Description      handle specified call related action
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_handle_call_action(tls_bthf_client_call_action_t action, int idx)
{
    CHECK_BTHF_CLIENT_SLC_CONNECTED();

    switch(action) {
        case BTHF_CLIENT_CALL_ACTION_CHLD_0:
            if(btif_hf_client_cb.chld_feat & BTA_HF_CLIENT_CHLD_REL) {
                BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_CHLD, 0, 0, NULL);
                break;
            }

            return TLS_BT_STATUS_UNSUPPORTED;

        case BTHF_CLIENT_CALL_ACTION_CHLD_1:

            // CHLD 1 is mandatory for 3 way calling
            if(btif_hf_client_cb.peer_feat & BTA_HF_CLIENT_PEER_FEAT_3WAY) {
                BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_CHLD, 1, 0, NULL);
                break;
            }

            return TLS_BT_STATUS_UNSUPPORTED;

        case BTHF_CLIENT_CALL_ACTION_CHLD_2:

            // CHLD 2 is mandatory for 3 way calling
            if(btif_hf_client_cb.peer_feat & BTA_HF_CLIENT_PEER_FEAT_3WAY) {
                BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_CHLD, 2, 0, NULL);
                break;
            }

            return TLS_BT_STATUS_UNSUPPORTED;

        case BTHF_CLIENT_CALL_ACTION_CHLD_3:
            if(btif_hf_client_cb.chld_feat & BTA_HF_CLIENT_CHLD_MERGE) {
                BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_CHLD, 3, 0, NULL);
                break;
            }

            return TLS_BT_STATUS_UNSUPPORTED;

        case BTHF_CLIENT_CALL_ACTION_CHLD_4:
            if(btif_hf_client_cb.chld_feat & BTA_HF_CLIENT_CHLD_MERGE_DETACH) {
                BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_CHLD, 4, 0, NULL);
                break;
            }

            return TLS_BT_STATUS_UNSUPPORTED;

        case BTHF_CLIENT_CALL_ACTION_CHLD_1x:
            if(btif_hf_client_cb.peer_feat & BTA_HF_CLIENT_PEER_ECC) {
                if(idx < 1) {
                    return TLS_BT_STATUS_FAIL;
                }

                BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_CHLD, 1, idx, NULL);
                break;
            }

            return TLS_BT_STATUS_UNSUPPORTED;

        case BTHF_CLIENT_CALL_ACTION_CHLD_2x:
            if(btif_hf_client_cb.peer_feat & BTA_HF_CLIENT_PEER_ECC) {
                if(idx < 1) {
                    return TLS_BT_STATUS_FAIL;
                }

                BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_CHLD, 2, idx, NULL);
                break;
            }

            return TLS_BT_STATUS_UNSUPPORTED;

        case BTHF_CLIENT_CALL_ACTION_ATA:
            BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_ATA, 0, 0, NULL);
            break;

        case BTHF_CLIENT_CALL_ACTION_CHUP:
            BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_CHUP, 0, 0, NULL);
            break;

        case BTHF_CLIENT_CALL_ACTION_BTRH_0:
            BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_BTRH, 0, 0, NULL);
            break;

        case BTHF_CLIENT_CALL_ACTION_BTRH_1:
            BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_BTRH, 1, 0, NULL);
            break;

        case BTHF_CLIENT_CALL_ACTION_BTRH_2:
            BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_BTRH, 2, 0, NULL);
            break;

        default:
            return TLS_BT_STATUS_FAIL;
    }

    return TLS_BT_STATUS_SUCCESS;
}


/*******************************************************************************
**
** Function         tls_bt_hf_client_query_current_calls
**
** Description      query list of current calls
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_query_current_calls(void)
{
    CHECK_BTHF_CLIENT_SLC_CONNECTED();

    if(btif_hf_client_cb.peer_feat & BTA_HF_CLIENT_PEER_ECS) {
        BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_CLCC, 0, 0, NULL);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_UNSUPPORTED;
}

/*******************************************************************************
**
** Function         tls_bt_hf_client_query_current_operator_name
**
** Description      query current selected operator name
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_query_current_operator_name(void)
{
    CHECK_BTHF_CLIENT_SLC_CONNECTED();
    BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_COPS, 0, 0, NULL);
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         tls_bt_hf_client_retrieve_subscriber_info
**
** Description      retrieve subscriber number information
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_retrieve_subscriber_info(void)
{
    CHECK_BTHF_CLIENT_SLC_CONNECTED();
    BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_CNUM, 0, 0, NULL);
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         send_dtmf
**
** Description      send dtmf
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_send_dtmf(char code)
{
    CHECK_BTHF_CLIENT_SLC_CONNECTED();
    BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_VTS, code, 0, NULL);
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         tls_bt_hf_client_request_last_voice_tag_number
**
** Description      Request number from AG for VR purposes
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_request_last_voice_tag_number(void)
{
    CHECK_BTHF_CLIENT_SLC_CONNECTED();

    if(btif_hf_client_cb.peer_feat & BTA_HF_CLIENT_PEER_VTAG) {
        BTA_HfClientSendAT(btif_hf_client_cb.handle, BTA_HF_CLIENT_AT_CMD_BINP, 1, 0, NULL);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_UNSUPPORTED;
}

/*******************************************************************************
**
** Function         send_at_cmd
**
** Description      Send requested AT command to rempte device.
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t tls_bt_hf_client_send_at_cmd(int cmd, int val1, int val2, const char *arg)
{
    CHECK_BTHF_CLIENT_SLC_CONNECTED();
    BTIF_TRACE_EVENT("%s Cmd %d val1 %d val2 %d arg %s",
                     __FUNCTION__, cmd, val1, val2, arg);
    BTA_HfClientSendAT(btif_hf_client_cb.handle, cmd, val1, val2, arg);
    return TLS_BT_STATUS_SUCCESS;
}

tls_bt_status_t tls_bt_hf_client_send_audio(tls_bt_addr_t *bd_addr, uint8_t *p_data,
        uint16_t length)
{
    CHECK_BTHF_CLIENT_SLC_CONNECTED();
    return TLS_BT_STATUS_UNSUPPORTED;
}






#endif
