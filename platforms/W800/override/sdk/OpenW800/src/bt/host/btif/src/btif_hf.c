/******************************************************************************
 *
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
 *  Filename:      btif_hf.c
 *
 *  Description:   Handsfree Profile Bluetooth Interface
 *
 *
 ***********************************************************************************/

#define LOG_TAG "bt_btif_hf"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bt_target.h"
#if defined(BTA_HFP_HSP_AG_INCLUDED) && (BTA_HFP_HSP_AG_INCLUDED == TRUE)
#include <hardware/bluetooth.h>
#include <hardware/bt_hf.h>

#include "bta_ag_api.h"
#include "btcore/include/bdaddr.h"
#include "btif_common.h"
#include "btif_profile_queue.h"
#include "btif_util.h"

/************************************************************************************
**  Constants & Macros
************************************************************************************/
#ifndef BTIF_HSAG_SERVICE_NAME
#define BTIF_HSAG_SERVICE_NAME ("Headset Gateway")
#endif

#ifndef BTIF_HFAG_SERVICE_NAME
#define BTIF_HFAG_SERVICE_NAME ("Handsfree Gateway")
#endif

#ifndef BTIF_HF_SERVICES
#define BTIF_HF_SERVICES    (BTA_HSP_SERVICE_MASK | BTA_HFP_SERVICE_MASK )
#endif

#ifndef BTIF_HF_SERVICE_NAMES
#define BTIF_HF_SERVICE_NAMES {BTIF_HSAG_SERVICE_NAME , BTIF_HFAG_SERVICE_NAME}
#endif

#ifndef BTIF_HF_SECURITY
#define BTIF_HF_SECURITY    (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#if (BTM_WBS_INCLUDED == TRUE )
#ifndef BTIF_HF_FEATURES
#define BTIF_HF_FEATURES   ( BTA_AG_FEAT_3WAY | \
                             BTA_AG_FEAT_ECNR   | \
                             BTA_AG_FEAT_REJECT | \
                             BTA_AG_FEAT_ECS    | \
                             BTA_AG_FEAT_EXTERR | \
                             BTA_AG_FEAT_BTRH   | \
                             BTA_AG_FEAT_VREC   | \
                             BTA_AG_FEAT_CODEC |\
                             BTA_AG_FEAT_UNAT)
#endif
#else
#ifndef BTIF_HF_FEATURES
#define BTIF_HF_FEATURES   ( BTA_AG_FEAT_3WAY | \
                             BTA_AG_FEAT_ECNR   | \
                             BTA_AG_FEAT_REJECT | \
                             BTA_AG_FEAT_ECS    | \
                             BTA_AG_FEAT_EXTERR | \
                             BTA_AG_FEAT_BTRH   | \
                             BTA_AG_FEAT_VREC   | \
                             BTA_AG_FEAT_UNAT)
#endif
#endif

#define BTIF_HF_CALL_END_TIMEOUT       6

#define BTIF_HF_INVALID_IDX       -1

/* Number of BTIF-HF control blocks */
#define BTIF_HF_NUM_CB       2

/* Max HF clients supported from App */
uint16_t btif_max_hf_clients = 1;

/* HF app ids for service registration */
typedef enum {
    BTIF_HF_ID_1 = 0,
    BTIF_HF_ID_2,
#if (BTIF_HF_NUM_CB == 3)
    BTIF_HF_ID_3
#endif
} bthf_hf_id_t;

uint16_t bthf_hf_id[BTIF_HF_NUM_CB] = {BTIF_HF_ID_1, BTIF_HF_ID_2,
#if (BTIF_HF_NUM_CB == 3)
                                       BTIF_HF_ID_3
#endif
                                      };

/************************************************************************************
**  Local type definitions
************************************************************************************/

/************************************************************************************
**  Static variables
************************************************************************************/
static bthf_callbacks_t *bt_hf_callbacks = NULL;
static int hf_idx = BTIF_HF_INVALID_IDX;

#define CHECK_BTHF_INIT() if (bt_hf_callbacks == NULL)\
    {\
        BTIF_TRACE_WARNING("BTHF: %s: BTHF not initialized", __FUNCTION__);\
        return TLS_BT_STATUS_NOT_READY;\
    }\
    else\
    {\
        BTIF_TRACE_EVENT("BTHF: %s", __FUNCTION__);\
    }

#define CHECK_BTHF_SLC_CONNECTED() if (bt_hf_callbacks == NULL)\
    {\
        BTIF_TRACE_WARNING("BTHF: %s: BTHF not initialized", __FUNCTION__);\
        return TLS_BT_STATUS_NOT_READY;\
    }\
    else if (btif_hf_cb.state != BTHF_CONNECTION_STATE_SLC_CONNECTED)\
    {\
        BTIF_TRACE_WARNING("BTHF: %s: SLC connection not up. state=%s", __FUNCTION__, dump_hf_conn_state(btif_hf_cb.state));\
        return TLS_BT_STATUS_NOT_READY;\
    }\
    else\
    {\
        BTIF_TRACE_EVENT("BTHF: %s", __FUNCTION__);\
    }

/* BTIF-HF control block to map bdaddr to BTA handle */
typedef struct _btif_hf_cb {
    uint16_t                  handle;
    tls_bt_addr_t             connected_bda;
    bthf_connection_state_t state;
    bthf_vr_state_t         vr_state;
    tBTA_AG_PEER_FEAT       peer_feat;
    int                     num_active;
    int                     num_held;
    struct timespec         call_end_timestamp;
    struct timespec         connected_timestamp;
    bthf_call_state_t       call_setup_state;
} btif_hf_cb_t;

static btif_hf_cb_t btif_hf_cb[BTIF_HF_NUM_CB];

/************************************************************************************
**  Static functions
************************************************************************************/

/************************************************************************************
**  Externs
************************************************************************************/
/* By default, even though codec negotiation is enabled, we will not use WBS as the default
* codec unless this variable is set to TRUE.
*/
#ifndef BTIF_HF_WBS_PREFERRED
#define BTIF_HF_WBS_PREFERRED   FALSE
#endif

uint8_t btif_conf_hf_force_wbs = BTIF_HF_WBS_PREFERRED;

/************************************************************************************
**  Functions
************************************************************************************/

/*******************************************************************************
**
** Function         is_connected
**
** Description      Internal function to check if HF is connected
**
** Returns          TRUE if connected
**
*******************************************************************************/
static uint8_t is_connected(tls_bt_addr_t *bd_addr)
{
    int i;

    for(i = 0; i < btif_max_hf_clients; ++i) {
        if(((btif_hf_cb[i].state == BTHF_CONNECTION_STATE_CONNECTED) ||
                (btif_hf_cb[i].state == BTHF_CONNECTION_STATE_SLC_CONNECTED)) &&
                ((bd_addr == NULL) || (bdcmp(bd_addr->address,
                                             btif_hf_cb[i].connected_bda.address) == 0))) {
            return TRUE;
        }
    }

    return FALSE;
}

/*******************************************************************************
**
** Function         btif_hf_idx_by_bdaddr
**
** Description      Internal function to get idx by bdaddr
**
** Returns          idx
**
*******************************************************************************/
static int btif_hf_idx_by_bdaddr(tls_bt_addr_t *bd_addr)
{
    int i;

    for(i = 0; i < btif_max_hf_clients; ++i) {
        if((bdcmp(bd_addr->address,
                  btif_hf_cb[i].connected_bda.address) == 0)) {
            return i;
        }
    }

    return BTIF_HF_INVALID_IDX;
}

/*******************************************************************************
**
** Function         callstate_to_callsetup
**
** Description      Converts HAL call state to BTA call setup indicator value
**
** Returns          BTA call indicator value
**
*******************************************************************************/
static uint8_t callstate_to_callsetup(bthf_call_state_t call_state)
{
    uint8_t call_setup = 0;

    if(call_state == BTHF_CALL_STATE_INCOMING) {
        call_setup = 1;
    }

    if(call_state == BTHF_CALL_STATE_DIALING) {
        call_setup = 2;
    }

    if(call_state == BTHF_CALL_STATE_ALERTING) {
        call_setup = 3;
    }

    return call_setup;
}

/*******************************************************************************
**
** Function         send_at_result
**
** Description      Send AT result code (OK/ERROR)
**
** Returns          void
**
*******************************************************************************/
static void send_at_result(uint8_t ok_flag, uint16_t errcode, int idx)
{
    tBTA_AG_RES_DATA    ag_res;
    wm_memset(&ag_res, 0, sizeof(ag_res));
    ag_res.ok_flag = ok_flag;

    if(ok_flag == BTA_AG_OK_ERROR) {
        ag_res.errcode = errcode;
    }

    BTA_AgResult(btif_hf_cb[idx].handle, BTA_AG_UNAT_RES, &ag_res);
}

/*******************************************************************************
**
** Function         send_indicator_update
**
** Description      Send indicator update (CIEV)
**
** Returns          void
**
*******************************************************************************/
static void send_indicator_update(uint16_t indicator, uint16_t value)
{
    tBTA_AG_RES_DATA ag_res;
    wm_memset(&ag_res, 0, sizeof(tBTA_AG_RES_DATA));
    ag_res.ind.id = indicator;
    ag_res.ind.value = value;
    BTA_AgResult(BTA_AG_HANDLE_ALL, BTA_AG_IND_RES, &ag_res);
}

void clear_phone_state_multihf(int idx)
{
    btif_hf_cb[idx].call_setup_state = BTHF_CALL_STATE_IDLE;
    btif_hf_cb[idx].num_active = btif_hf_cb[idx].num_held = 0;
}

/*******************************************************************************
**
** Function         btif_hf_latest_connected_idx
**
** Description      Returns idx for latest connected HF
**
** Returns          int
**
*******************************************************************************/
static int btif_hf_latest_connected_idx()
{
    struct timespec         now, conn_time_delta;
    int latest_conn_idx = BTIF_HF_INVALID_IDX, i;
    clock_gettime(CLOCK_MONOTONIC, &now);
    conn_time_delta.tv_sec = now.tv_sec;

    for(i = 0; i < btif_max_hf_clients; i++) {
        if(btif_hf_cb[i].state == BTHF_CONNECTION_STATE_SLC_CONNECTED) {
            if((now.tv_sec - btif_hf_cb[i].connected_timestamp.tv_sec)
                    < conn_time_delta.tv_sec) {
                conn_time_delta.tv_sec =
                                now.tv_sec - btif_hf_cb[i].connected_timestamp.tv_sec;
                latest_conn_idx = i;
            }
        }
    }

    return latest_conn_idx;
}

/*******************************************************************************
**
** Function         btif_hf_check_if_slc_connected
**
** Description      Returns BT_STATUS_SUCCESS if SLC is up for any HF
**
** Returns          bt_status_t
**
*******************************************************************************/
static tls_bt_status_t btif_hf_check_if_slc_connected()
{
    if(bt_hf_callbacks == NULL) {
        BTIF_TRACE_WARNING("BTHF: %s: BTHF not initialized", __FUNCTION__);
        return TLS_BT_STATUS_NOT_READY;
    } else {
        int i;

        for(i = 0; i < btif_max_hf_clients; i++) {
            if(btif_hf_cb[i].state == BTHF_CONNECTION_STATE_SLC_CONNECTED) {
                BTIF_TRACE_EVENT("BTHF: %s: slc connected for idx = %d",
                                 __FUNCTION__, i);
                return TLS_BT_STATUS_SUCCESS;
            }
        }

        BTIF_TRACE_WARNING("BTHF: %s: No SLC connection up", __FUNCTION__);
        return TLS_BT_STATUS_NOT_READY;
    }
}

/*****************************************************************************
**   Section name (Group of functions)
*****************************************************************************/

/*****************************************************************************
**
**   btif hf api functions (no context switch)
**
*****************************************************************************/

/*******************************************************************************
**
** Function         btif_hf_upstreams_evt
**
** Description      Executes HF UPSTREAMS events in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_hf_upstreams_evt(uint16_t event, char *p_param)
{
    tBTA_AG *p_data = (tBTA_AG *)p_param;
    bdstr_t bdstr;
    int idx = p_data->hdr.handle - 1;
    BTIF_TRACE_DEBUG("%s: event=%s", __FUNCTION__, dump_hf_event(event));

    if((idx < 0) || (idx >= BTIF_HF_NUM_CB)) {
        BTIF_TRACE_ERROR("%s: Invalid index %d", __FUNCTION__, idx);
        return;
    }

    switch(event) {
        case BTA_AG_ENABLE_EVT:
        case BTA_AG_DISABLE_EVT:
            break;

        case BTA_AG_REGISTER_EVT:
            btif_hf_cb[idx].handle = p_data->reg.hdr.handle;
            BTIF_TRACE_DEBUG("%s: BTA_AG_REGISTER_EVT,"
                             "btif_hf_cb.handle = %d", __FUNCTION__, btif_hf_cb[idx].handle);
            break;

        case BTA_AG_OPEN_EVT:
            if(p_data->open.status == BTA_AG_SUCCESS) {
                bdcpy(btif_hf_cb[idx].connected_bda.address,
                      p_data->open.bd_addr);
                btif_hf_cb[idx].state = BTHF_CONNECTION_STATE_CONNECTED;
                btif_hf_cb[idx].peer_feat = 0;
                clear_phone_state_multihf(idx);
            } else if(btif_hf_cb[idx].state == BTHF_CONNECTION_STATE_CONNECTING) {
                btif_hf_cb[idx].state = BTHF_CONNECTION_STATE_DISCONNECTED;
            } else {
                BTIF_TRACE_WARNING("%s: AG open failed, but another device connected. status=%d state=%d connected device=%s",
                                   __FUNCTION__, p_data->open.status, btif_hf_cb[idx].state,
                                   bdaddr_to_string(&btif_hf_cb[idx].connected_bda, bdstr, sizeof(bdstr)));
                break;
            }

            HAL_CBACK(bt_hf_callbacks, connection_state_cb, btif_hf_cb[idx].state,
                      &btif_hf_cb[idx].connected_bda);

            if(btif_hf_cb[idx].state == BTHF_CONNECTION_STATE_DISCONNECTED) {
                bdsetany(btif_hf_cb[idx].connected_bda.address);
            }

            if(p_data->open.status != BTA_AG_SUCCESS) {
                btif_queue_advance();
            }

            break;

        case BTA_AG_CLOSE_EVT:
            btif_hf_cb[idx].connected_timestamp.tv_sec = 0;
            btif_hf_cb[idx].state = BTHF_CONNECTION_STATE_DISCONNECTED;
            BTIF_TRACE_DEBUG("%s: BTA_AG_CLOSE_EVT,"
                             "idx = %d, btif_hf_cb.handle = %d", __FUNCTION__, idx,
                             btif_hf_cb[idx].handle);
            HAL_CBACK(bt_hf_callbacks, connection_state_cb, btif_hf_cb[idx].state,
                      &btif_hf_cb[idx].connected_bda);
            bdsetany(btif_hf_cb[idx].connected_bda.address);
            btif_hf_cb[idx].peer_feat = 0;
            clear_phone_state_multihf(idx);
            hf_idx = btif_hf_latest_connected_idx();
            /* If AG_OPEN was received but SLC was not setup in a specified time (10 seconds),
            ** then AG_CLOSE may be received. We need to advance the queue here
            */
            btif_queue_advance();
            break;

        case BTA_AG_CONN_EVT:
            clock_gettime(CLOCK_MONOTONIC,
                          &btif_hf_cb[idx].connected_timestamp);
            BTIF_TRACE_DEBUG("%s: BTA_AG_CONN_EVT, idx = %d ",
                             __FUNCTION__, idx);
            btif_hf_cb[idx].peer_feat = p_data->conn.peer_feat;
            btif_hf_cb[idx].state = BTHF_CONNECTION_STATE_SLC_CONNECTED;
            hf_idx = btif_hf_latest_connected_idx();
            HAL_CBACK(bt_hf_callbacks, connection_state_cb, btif_hf_cb[idx].state,
                      &btif_hf_cb[idx].connected_bda);
            btif_queue_advance();
            break;

        case BTA_AG_AUDIO_OPEN_EVT:
            hf_idx = idx;
            HAL_CBACK(bt_hf_callbacks, audio_state_cb, BTHF_AUDIO_STATE_CONNECTED,
                      &btif_hf_cb[idx].connected_bda);
            break;

        case BTA_AG_AUDIO_CLOSE_EVT:
            HAL_CBACK(bt_hf_callbacks, audio_state_cb, BTHF_AUDIO_STATE_DISCONNECTED,
                      &btif_hf_cb[idx].connected_bda);
            break;

        /* BTA auto-responds, silently discard */
        case BTA_AG_SPK_EVT:
        case BTA_AG_MIC_EVT:
            HAL_CBACK(bt_hf_callbacks, volume_cmd_cb,
                      (event == BTA_AG_SPK_EVT) ? BTHF_VOLUME_TYPE_SPK :
                      BTHF_VOLUME_TYPE_MIC, p_data->val.num,
                      &btif_hf_cb[idx].connected_bda);
            break;

        case BTA_AG_AT_A_EVT:
            if((btif_hf_cb[0].num_held + btif_hf_cb[0].num_active) == 0) {
                hf_idx = idx;
            } else {
                BTIF_TRACE_DEBUG("Donot set hf_idx for ATA since already in a call");
            }

            HAL_CBACK(bt_hf_callbacks, answer_call_cmd_cb,
                      &btif_hf_cb[idx].connected_bda);
            break;

        /* Java needs to send OK/ERROR for these commands */
        case BTA_AG_AT_BLDN_EVT:
        case BTA_AG_AT_D_EVT:
            if((btif_hf_cb[0].num_held + btif_hf_cb[0].num_active) == 0) {
                hf_idx = idx;
            } else {
                BTIF_TRACE_DEBUG("Donot set hf_idx for BLDN/D since already in a call");
            }

            HAL_CBACK(bt_hf_callbacks, dial_call_cmd_cb,
                      (event == BTA_AG_AT_D_EVT) ? p_data->val.str : NULL,
                      &btif_hf_cb[idx].connected_bda);
            break;

        case BTA_AG_AT_CHUP_EVT:
            HAL_CBACK(bt_hf_callbacks, hangup_call_cmd_cb,
                      &btif_hf_cb[idx].connected_bda);
            break;

        case BTA_AG_AT_CIND_EVT:
            HAL_CBACK(bt_hf_callbacks, cind_cmd_cb,
                      &btif_hf_cb[idx].connected_bda);
            break;

        case BTA_AG_AT_VTS_EVT:
            HAL_CBACK(bt_hf_callbacks, dtmf_cmd_cb, p_data->val.str[0],
                      &btif_hf_cb[idx].connected_bda);
            break;

        case BTA_AG_AT_BVRA_EVT:
            HAL_CBACK(bt_hf_callbacks, vr_cmd_cb,
                      (p_data->val.num == 1) ? BTHF_VR_STATE_STARTED :
                      BTHF_VR_STATE_STOPPED, &btif_hf_cb[idx].connected_bda);
            break;

        case BTA_AG_AT_NREC_EVT:
            HAL_CBACK(bt_hf_callbacks, nrec_cmd_cb,
                      (p_data->val.num == 1) ? BTHF_NREC_START : BTHF_NREC_STOP,
                      &btif_hf_cb[idx].connected_bda);
            break;

        /* TODO: Add a callback for CBC */
        case BTA_AG_AT_CBC_EVT:
            break;

        case BTA_AG_AT_CKPD_EVT:
            HAL_CBACK(bt_hf_callbacks, key_pressed_cmd_cb,
                      &btif_hf_cb[idx].connected_bda);
            break;
#if (BTM_WBS_INCLUDED == TRUE )

        case BTA_AG_WBS_EVT:
            BTIF_TRACE_DEBUG("BTA_AG_WBS_EVT Set codec status %d codec %d 1=CVSD 2=MSBC", \
                             p_data->val.hdr.status, p_data->val.num);

            if(p_data->val.num == BTA_AG_CODEC_CVSD) {
                HAL_CBACK(bt_hf_callbacks, wbs_cb, BTHF_WBS_NO, &btif_hf_cb[idx].connected_bda);
            } else if(p_data->val.num == BTA_AG_CODEC_MSBC) {
                HAL_CBACK(bt_hf_callbacks, wbs_cb, BTHF_WBS_YES, &btif_hf_cb[idx].connected_bda);
            } else {
                HAL_CBACK(bt_hf_callbacks, wbs_cb, BTHF_WBS_NONE, &btif_hf_cb[idx].connected_bda);
            }

            break;
#endif

        /* Java needs to send OK/ERROR for these commands */
        case BTA_AG_AT_CHLD_EVT:
            HAL_CBACK(bt_hf_callbacks, chld_cmd_cb, atoi(p_data->val.str),
                      &btif_hf_cb[idx].connected_bda);
            break;

        case BTA_AG_AT_CLCC_EVT:
            HAL_CBACK(bt_hf_callbacks, clcc_cmd_cb,
                      &btif_hf_cb[idx].connected_bda);
            break;

        case BTA_AG_AT_COPS_EVT:
            HAL_CBACK(bt_hf_callbacks, cops_cmd_cb,
                      &btif_hf_cb[idx].connected_bda);
            break;

        case BTA_AG_AT_UNAT_EVT:
            HAL_CBACK(bt_hf_callbacks, unknown_at_cmd_cb, p_data->val.str,
                      &btif_hf_cb[idx].connected_bda);
            break;

        case BTA_AG_AT_CNUM_EVT:
            HAL_CBACK(bt_hf_callbacks, cnum_cmd_cb,
                      &btif_hf_cb[idx].connected_bda);
            break;

        /* TODO: Some of these commands may need to be sent to app. For now respond with error */
        case BTA_AG_AT_BINP_EVT:
        case BTA_AG_AT_BTRH_EVT:
            send_at_result(BTA_AG_OK_ERROR, BTA_AG_ERR_OP_NOT_SUPPORTED, idx);
            break;

        case BTA_AG_AT_BAC_EVT:
            BTIF_TRACE_DEBUG("AG Bitmap of peer-codecs %d", p_data->val.num);
#if (BTM_WBS_INCLUDED == TRUE )

            /* If the peer supports mSBC and the BTIF prefferred codec is also mSBC, then
            we should set the BTA AG Codec to mSBC. This would trigger a +BCS to mSBC at the time
            of SCO connection establishment */
            if((btif_conf_hf_force_wbs == TRUE) && (p_data->val.num & BTA_AG_CODEC_MSBC)) {
                BTIF_TRACE_EVENT("%s btif_hf override-Preferred Codec to MSBC", __FUNCTION__);
                BTA_AgSetCodec(btif_hf_cb[idx].handle, BTA_AG_CODEC_MSBC);
            } else {
                BTIF_TRACE_EVENT("%s btif_hf override-Preferred Codec to CVSD", __FUNCTION__);
                BTA_AgSetCodec(btif_hf_cb[idx].handle, BTA_AG_CODEC_CVSD);
            }

#endif
            break;

        case BTA_AG_AT_BCS_EVT:
            BTIF_TRACE_DEBUG("AG final seleded codec is %d 1=CVSD 2=MSBC", p_data->val.num);
            /*  no BTHF_WBS_NONE case, becuase HF1.6 supported device can send BCS */
            HAL_CBACK(bt_hf_callbacks, wbs_cb, (p_data->val.num == BTA_AG_CODEC_MSBC) ? \
                      BTHF_WBS_YES : BTHF_WBS_NO, &btif_hf_cb[idx].connected_bda);
            break;

        default:
            BTIF_TRACE_WARNING("%s: Unhandled event: %d", __FUNCTION__, event);
            break;
    }
}

/*******************************************************************************
**
** Function         bte_hf_evt
**
** Description      Switches context from BTE to BTIF for all HF events
**
** Returns          void
**
*******************************************************************************/

static void bte_hf_evt(tBTA_AG_EVT event, tBTA_AG *p_data)
{
    tls_bt_status_t status;
    int param_len = 0;

    /* TODO: BTA sends the union members and not tBTA_AG. If using param_len=sizeof(tBTA_AG), we get a crash on wm_memcpy */
    if(BTA_AG_REGISTER_EVT == event) {
        param_len = sizeof(tBTA_AG_REGISTER);
    } else if(BTA_AG_OPEN_EVT == event) {
        param_len = sizeof(tBTA_AG_OPEN);
    } else if(BTA_AG_CONN_EVT == event) {
        param_len = sizeof(tBTA_AG_CONN);
    } else if((BTA_AG_CLOSE_EVT == event) || (BTA_AG_AUDIO_OPEN_EVT == event)
              || (BTA_AG_AUDIO_CLOSE_EVT == event)) {
        param_len = sizeof(tBTA_AG_HDR);
    } else if(p_data) {
        param_len = sizeof(tBTA_AG_VAL);
    }

    /* switch context to btif task context (copy full union size for convenience) */
    status = btif_transfer_context(btif_hf_upstreams_evt, (uint16_t)event, (void *)p_data, param_len,
                                   NULL);
    /* catch any failed context transfers */
    ASSERTC(status == TLS_BT_STATUS_SUCCESS, "context transfer failed", status);
}

/*******************************************************************************
**
** Function         btif_in_hf_generic_evt
**
** Description     Processes generic events to be sent to JNI that are not triggered from the BTA.
**                      Always runs in BTIF context
**
** Returns          void
**
*******************************************************************************/
static void btif_in_hf_generic_evt(uint16_t event, char *p_param)
{
    int idx = btif_hf_idx_by_bdaddr((tls_bt_addr_t *)p_param);
    BTIF_TRACE_EVENT("%s: event=%d", __FUNCTION__, event);

    if((idx < 0) || (idx >= BTIF_HF_NUM_CB)) {
        BTIF_TRACE_ERROR("%s: Invalid index %d", __FUNCTION__, idx);
        return;
    }

    switch(event) {
        case BTIF_HFP_CB_AUDIO_CONNECTING: {
            HAL_CBACK(bt_hf_callbacks, audio_state_cb, BTHF_AUDIO_STATE_CONNECTING,
                      &btif_hf_cb[idx].connected_bda);
        }
        break;

        default: {
            BTIF_TRACE_WARNING("%s : Unknown event 0x%x", __FUNCTION__, event);
        }
        break;
    }
}

/*******************************************************************************
**
** Function         btif_hf_init
**
** Description     initializes the hf interface
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t init(bthf_callbacks_t *callbacks, int max_hf_clients)
{
    btif_max_hf_clients = max_hf_clients;
    BTIF_TRACE_DEBUG("%s - max_hf_clients=%d", __func__, btif_max_hf_clients);
    bt_hf_callbacks = callbacks;
    wm_memset(&btif_hf_cb, 0, sizeof(btif_hf_cb));
    /* Invoke the enable service API to the core to set the appropriate service_id
     * Internally, the HSP_SERVICE_ID shall also be enabled if HFP is enabled (phone)
     * othwerwise only HSP is enabled (tablet)
    */
#if (defined(BTIF_HF_SERVICES) && (BTIF_HF_SERVICES & BTA_HFP_SERVICE_MASK))
    btif_enable_service(BTA_HFP_SERVICE_ID);
#else
    btif_enable_service(BTA_HSP_SERVICE_ID);
#endif

    for(int i = 0; i < btif_max_hf_clients; i++) {
        clear_phone_state_multihf(i);
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         connect
**
** Description     connect to headset
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t connect_int(tls_bt_addr_t *bd_addr, uint16_t uuid)
{
    CHECK_BTHF_INIT();
    int i;

    for(i = 0; i < btif_max_hf_clients;) {
        if(((btif_hf_cb[i].state == BTHF_CONNECTION_STATE_CONNECTED) ||
                (btif_hf_cb[i].state == BTHF_CONNECTION_STATE_SLC_CONNECTED))) {
            i++;
        } else {
            break;
        }
    }

    if(i == btif_max_hf_clients) {
        return TLS_BT_STATUS_BUSY;
    }

    if(!is_connected(bd_addr)) {
        btif_hf_cb[i].state = BTHF_CONNECTION_STATE_CONNECTING;
        bdcpy(btif_hf_cb[i].connected_bda.address, bd_addr->address);
        BTA_AgOpen(btif_hf_cb[i].handle, btif_hf_cb[i].connected_bda.address,
                   BTIF_HF_SECURITY, BTIF_HF_SERVICES);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_BUSY;
}

static tls_bt_status_t connect(tls_bt_addr_t *bd_addr)
{
    CHECK_BTHF_INIT();
    return btif_queue_connect(UUID_SERVCLASS_AG_HANDSFREE, bd_addr, connect_int);
}

/*******************************************************************************
**
** Function         disconnect
**
** Description      disconnect from headset
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t disconnect(tls_bt_addr_t *bd_addr)
{
    CHECK_BTHF_INIT();
    int idx = btif_hf_idx_by_bdaddr(bd_addr);

    if((idx < 0) || (idx >= BTIF_HF_NUM_CB)) {
        BTIF_TRACE_ERROR("%s: Invalid index %d", __FUNCTION__, idx);
        return TLS_BT_STATUS_FAIL;
    }

    if(is_connected(bd_addr) && (idx != BTIF_HF_INVALID_IDX)) {
        BTA_AgClose(btif_hf_cb[idx].handle);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         connect_audio
**
** Description     create an audio connection
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t connect_audio(tls_bt_addr_t *bd_addr)
{
    CHECK_BTHF_INIT();
    int idx = btif_hf_idx_by_bdaddr(bd_addr);

    if((idx < 0) || (idx >= BTIF_HF_NUM_CB)) {
        BTIF_TRACE_ERROR("%s: Invalid index %d", __FUNCTION__, idx);
        return TLS_BT_STATUS_FAIL;
    }

    /* Check if SLC is connected */
    if(btif_hf_check_if_slc_connected() != TLS_BT_STATUS_SUCCESS) {
        return TLS_BT_STATUS_NOT_READY;
    }

    if(is_connected(bd_addr) && (idx != BTIF_HF_INVALID_IDX)) {
        BTA_AgAudioOpen(btif_hf_cb[idx].handle);
        /* Inform the application that the audio connection has been initiated successfully */
        btif_transfer_context(btif_in_hf_generic_evt, BTIF_HFP_CB_AUDIO_CONNECTING,
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
static tls_bt_status_t disconnect_audio(tls_bt_addr_t *bd_addr)
{
    CHECK_BTHF_INIT();
    int idx = btif_hf_idx_by_bdaddr(bd_addr);

    if((idx < 0) || (idx >= BTIF_HF_NUM_CB)) {
        BTIF_TRACE_ERROR("%s: Invalid index %d", __FUNCTION__, idx);
        return TLS_BT_STATUS_FAIL;
    }

    if(is_connected(bd_addr) && (idx != BTIF_HF_INVALID_IDX)) {
        BTA_AgAudioClose(btif_hf_cb[idx].handle);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         start_voice_recognition
**
** Description      start voice recognition
**
** Returns          bt_status_t
**
*******************************************************************************/
static tls_bt_status_t start_voice_recognition(tls_bt_addr_t *bd_addr)
{
    CHECK_BTHF_INIT();
    int idx = btif_hf_idx_by_bdaddr(bd_addr);

    if((idx < 0) || (idx >= BTIF_HF_NUM_CB)) {
        BTIF_TRACE_ERROR("%s: Invalid index %d", __FUNCTION__, idx);
        return TLS_BT_STATUS_FAIL;
    }

    if(is_connected(bd_addr) && (idx != BTIF_HF_INVALID_IDX)) {
        if(btif_hf_cb[idx].peer_feat & BTA_AG_PEER_FEAT_VREC) {
            tBTA_AG_RES_DATA ag_res;
            wm_memset(&ag_res, 0, sizeof(ag_res));
            ag_res.state = 1;
            BTA_AgResult(btif_hf_cb[idx].handle, BTA_AG_BVRA_RES, &ag_res);
            return TLS_BT_STATUS_SUCCESS;
        } else {
            return TLS_BT_STATUS_UNSUPPORTED;
        }
    }

    return TLS_BT_STATUS_NOT_READY;
}

/*******************************************************************************
**
** Function         stop_voice_recognition
**
** Description      stop voice recognition
**
** Returns          bt_status_t
**
*******************************************************************************/
static tls_bt_status_t stop_voice_recognition(tls_bt_addr_t *bd_addr)
{
    CHECK_BTHF_INIT();
    int idx = btif_hf_idx_by_bdaddr(bd_addr);

    if((idx < 0) || (idx >= BTIF_HF_NUM_CB)) {
        BTIF_TRACE_ERROR("%s: Invalid index %d", __FUNCTION__, idx);
        return TLS_BT_STATUS_FAIL;
    }

    if(is_connected(bd_addr) && (idx != BTIF_HF_INVALID_IDX)) {
        if(btif_hf_cb[idx].peer_feat & BTA_AG_PEER_FEAT_VREC) {
            tBTA_AG_RES_DATA ag_res;
            wm_memset(&ag_res, 0, sizeof(ag_res));
            ag_res.state = 0;
            BTA_AgResult(btif_hf_cb[idx].handle, BTA_AG_BVRA_RES, &ag_res);
            return TLS_BT_STATUS_SUCCESS;
        } else {
            return TLS_BT_STATUS_UNSUPPORTED;
        }
    }

    return TLS_BT_STATUS_NOT_READY;
}

/*******************************************************************************
**
** Function         volume_control
**
** Description      volume control
**
** Returns          bt_status_t
**
*******************************************************************************/
static tls_bt_status_t volume_control(bthf_volume_type_t type, int volume,
                                      tls_bt_addr_t *bd_addr)
{
    CHECK_BTHF_INIT();
    int idx = btif_hf_idx_by_bdaddr(bd_addr);

    if((idx < 0) || (idx >= BTIF_HF_NUM_CB)) {
        BTIF_TRACE_ERROR("%s: Invalid index %d", __FUNCTION__, idx);
        return TLS_BT_STATUS_FAIL;
    }

    tBTA_AG_RES_DATA ag_res;
    wm_memset(&ag_res, 0, sizeof(tBTA_AG_RES_DATA));

    if(is_connected(bd_addr) && (idx != BTIF_HF_INVALID_IDX)) {
        ag_res.num = volume;
        BTA_AgResult(btif_hf_cb[idx].handle,
                     (type == BTHF_VOLUME_TYPE_SPK) ? BTA_AG_SPK_RES : BTA_AG_MIC_RES,
                     &ag_res);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         device_status_notification
**
** Description      Combined device status change notification
**
** Returns          bt_status_t
**
*******************************************************************************/
static tls_bt_status_t device_status_notification(bthf_network_state_t ntk_state,
        bthf_service_type_t svc_type, int signal, int batt_chg)
{
    CHECK_BTHF_INIT();

    if(is_connected(NULL)) {
        /* send all indicators to BTA.
        ** BTA will make sure no duplicates are sent out
        */
        send_indicator_update(BTA_AG_IND_SERVICE,
                              (ntk_state == BTHF_NETWORK_STATE_AVAILABLE) ? 1 : 0);
        send_indicator_update(BTA_AG_IND_ROAM,
                              (svc_type == BTHF_SERVICE_TYPE_HOME) ? 0 : 1);
        send_indicator_update(BTA_AG_IND_SIGNAL, signal);
        send_indicator_update(BTA_AG_IND_BATTCHG, batt_chg);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         cops_response
**
** Description      Response for COPS command
**
** Returns          bt_status_t
**
*******************************************************************************/
static tls_bt_status_t cops_response(const char *cops, tls_bt_addr_t *bd_addr)
{
    CHECK_BTHF_INIT();
    int idx = btif_hf_idx_by_bdaddr(bd_addr);

    if((idx < 0) || (idx >= BTIF_HF_NUM_CB)) {
        BTIF_TRACE_ERROR("%s: Invalid index %d", __FUNCTION__, idx);
        return TLS_BT_STATUS_FAIL;
    }

    if(is_connected(bd_addr) && (idx != BTIF_HF_INVALID_IDX)) {
        tBTA_AG_RES_DATA    ag_res;
        /* Format the response */
        sprintf(ag_res.str, "0,0,\"%.16s\"", cops);
        ag_res.ok_flag = BTA_AG_OK_DONE;
        BTA_AgResult(btif_hf_cb[idx].handle, BTA_AG_COPS_RES, &ag_res);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         cind_response
**
** Description      Response for CIND command
**
** Returns          bt_status_t
**
*******************************************************************************/
static tls_bt_status_t cind_response(int svc, int num_active, int num_held,
                                     bthf_call_state_t call_setup_state,
                                     int signal, int roam, int batt_chg,
                                     tls_bt_addr_t *bd_addr)
{
    CHECK_BTHF_INIT();
    int idx = btif_hf_idx_by_bdaddr(bd_addr);

    if((idx < 0) || (idx >= BTIF_HF_NUM_CB)) {
        BTIF_TRACE_ERROR("%s: Invalid index %d", __FUNCTION__, idx);
        return TLS_BT_STATUS_FAIL;
    }

    if(is_connected(bd_addr) && (idx != BTIF_HF_INVALID_IDX)) {
        tBTA_AG_RES_DATA    ag_res;
        wm_memset(&ag_res, 0, sizeof(ag_res));
        /* per the errata 2043, call=1 implies atleast one call is in progress (active/held)
        ** https://www.bluetooth.org/errata/errata_view.cfm?errata_id=2043
        **/
        sprintf(ag_res.str, "%d,%d,%d,%d,%d,%d,%d",
                (num_active + num_held) ? 1 : 0,                       /* Call state */
                callstate_to_callsetup(call_setup_state),              /* Callsetup state */
                svc,                                                   /* network service */
                signal,                                                /* Signal strength */
                roam,                                                  /* Roaming indicator */
                batt_chg,                                              /* Battery level */
                ((num_held == 0) ? 0 : ((num_active == 0) ? 2 : 1)));          /* Call held */
        BTA_AgResult(btif_hf_cb[idx].handle, BTA_AG_CIND_RES, &ag_res);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         formatted_at_response
**
** Description      Pre-formatted AT response, typically in response to unknown AT cmd
**
** Returns          bt_status_t
**
*******************************************************************************/
static tls_bt_status_t formatted_at_response(const char *rsp, tls_bt_addr_t *bd_addr)
{
    CHECK_BTHF_INIT();
    tBTA_AG_RES_DATA    ag_res;
    int idx = btif_hf_idx_by_bdaddr(bd_addr);

    if((idx < 0) || (idx >= BTIF_HF_NUM_CB)) {
        BTIF_TRACE_ERROR("%s: Invalid index %d", __FUNCTION__, idx);
        return TLS_BT_STATUS_FAIL;
    }

    if(is_connected(bd_addr) && (idx != BTIF_HF_INVALID_IDX)) {
        /* Format the response and send */
        wm_memset(&ag_res, 0, sizeof(ag_res));
        strncpy(ag_res.str, rsp, BTA_AG_AT_MAX_LEN);
        BTA_AgResult(btif_hf_cb[idx].handle, BTA_AG_UNAT_RES, &ag_res);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         at_response
**
** Description      ok/error response
**
** Returns          bt_status_t
**
*******************************************************************************/
static tls_bt_status_t at_response(bthf_at_response_t response_code,
                                   int error_code, tls_bt_addr_t *bd_addr)
{
    CHECK_BTHF_INIT();
    int idx = btif_hf_idx_by_bdaddr(bd_addr);

    if((idx < 0) || (idx >= BTIF_HF_NUM_CB)) {
        BTIF_TRACE_ERROR("%s: Invalid index %d", __FUNCTION__, idx);
        return TLS_BT_STATUS_FAIL;
    }

    if(is_connected(bd_addr) && (idx != BTIF_HF_INVALID_IDX)) {
        send_at_result((response_code == BTHF_AT_RESPONSE_OK) ? BTA_AG_OK_DONE
                       : BTA_AG_OK_ERROR, error_code, idx);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         clcc_response
**
** Description      response for CLCC command
**                  Can be iteratively called for each call index. Call index
**                  of 0 will be treated as NULL termination (Completes response)
**
** Returns          bt_status_t
**
*******************************************************************************/
static tls_bt_status_t clcc_response(int index, bthf_call_direction_t dir,
                                     bthf_call_state_t state, bthf_call_mode_t mode,
                                     bthf_call_mpty_type_t mpty, const char *number,
                                     bthf_call_addrtype_t type, tls_bt_addr_t *bd_addr)
{
    CHECK_BTHF_INIT();
    int idx = btif_hf_idx_by_bdaddr(bd_addr);

    if((idx < 0) || (idx >= BTIF_HF_NUM_CB)) {
        BTIF_TRACE_ERROR("%s: Invalid index %d", __FUNCTION__, idx);
        return TLS_BT_STATUS_FAIL;
    }

    if(is_connected(bd_addr) && (idx != BTIF_HF_INVALID_IDX)) {
        tBTA_AG_RES_DATA    ag_res;
        int                 xx;
        wm_memset(&ag_res, 0, sizeof(ag_res));

        /* Format the response */
        if(index == 0) {
            ag_res.ok_flag = BTA_AG_OK_DONE;
        } else {
            BTIF_TRACE_EVENT("clcc_response: [%d] dir %d state %d mode %d number = %s type = %d",
                             index, dir, state, mode, number, type);
            xx = sprintf(ag_res.str, "%d,%d,%d,%d,%d",
                         index, dir, state, mode, mpty);

            if(number) {
                if((type == BTHF_CALL_ADDRTYPE_INTERNATIONAL) && (*number != '+')) {
                    sprintf(&ag_res.str[xx], ",\"+%s\",%d", number, type);
                } else {
                    sprintf(&ag_res.str[xx], ",\"%s\",%d", number, type);
                }
            }
        }

        BTA_AgResult(btif_hf_cb[idx].handle, BTA_AG_CLCC_RES, &ag_res);
        return TLS_BT_STATUS_SUCCESS;
    }

    return TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         phone_state_change
**
** Description      notify of a call state change
**                  number & type: valid only for incoming & waiting call
**
** Returns          bt_status_t
**
*******************************************************************************/

static tls_bt_status_t phone_state_change(int num_active, int num_held,
        bthf_call_state_t call_setup_state,
        const char *number, bthf_call_addrtype_t type)
{
    tBTA_AG_RES res = 0xff;
    tBTA_AG_RES_DATA ag_res;
    tls_bt_status_t status = TLS_BT_STATUS_SUCCESS;
    uint8_t activeCallUpdated = FALSE;
    int idx, i;

    /* hf_idx is index of connected HS that sent ATA/BLDN,
            otherwise index of latest connected HS */
    if(hf_idx != BTIF_HF_INVALID_IDX) {
        idx = hf_idx;
    } else {
        idx = btif_hf_latest_connected_idx();
    }

    BTIF_TRACE_DEBUG("phone_state_change: idx = %d", idx);

    /* Check if SLC is connected */
    if(btif_hf_check_if_slc_connected() != TLS_BT_STATUS_SUCCESS) {
        return TLS_BT_STATUS_NOT_READY;
    }

    BTIF_TRACE_DEBUG("phone_state_change: num_active=%d [prev: %d]  num_held=%d[prev: %d]"
                     " call_setup=%s [prev: %s]", num_active, btif_hf_cb[idx].num_active,
                     num_held, btif_hf_cb[idx].num_held, dump_hf_call_state(call_setup_state),
                     dump_hf_call_state(btif_hf_cb[idx].call_setup_state));

    /* if all indicators are 0, send end call and return */
    if(num_active == 0 && num_held == 0 && call_setup_state == BTHF_CALL_STATE_IDLE) {
        BTIF_TRACE_DEBUG("%s: Phone on hook", __FUNCTION__);

        /* record call termination timestamp  if  there was an active/held call  or
                   callsetup state > BTHF_CALL_STATE_IDLE */
        if((btif_hf_cb[idx].call_setup_state != BTHF_CALL_STATE_IDLE) ||
                (btif_hf_cb[idx].num_active) || (btif_hf_cb[idx].num_held)) {
            BTIF_TRACE_DEBUG("%s: Record call termination timestamp", __FUNCTION__);
            clock_gettime(CLOCK_MONOTONIC, &btif_hf_cb[0].call_end_timestamp);
        }

        BTA_AgResult(BTA_AG_HANDLE_ALL, BTA_AG_END_CALL_RES, NULL);
        hf_idx = BTIF_HF_INVALID_IDX;

        /* if held call was present, reset that as well */
        if(btif_hf_cb[idx].num_held) {
            send_indicator_update(BTA_AG_IND_CALLHELD, 0);
        }

        goto update_call_states;
    }

    /* active state can change when:
    ** 1. an outgoing/incoming call was answered
    ** 2. an held was resumed
    ** 3. without callsetup notifications, call became active
    ** (3) can happen if call is active and a headset connects to us
    **
    ** In the case of (3), we will have to notify the stack of an active
    ** call, instead of sending an indicator update. This will also
    ** force the SCO to be setup. Handle this special case here prior to
    ** call setup handling
    */
    if(((num_active + num_held) > 0) && (btif_hf_cb[idx].num_active == 0)
            && (btif_hf_cb[idx].num_held == 0) &&
            (btif_hf_cb[idx].call_setup_state == BTHF_CALL_STATE_IDLE)) {
        BTIF_TRACE_DEBUG("%s: Active/Held call notification received without call setup update",
                         __FUNCTION__);
        wm_memset(&ag_res, 0, sizeof(tBTA_AG_RES_DATA));
        ag_res.audio_handle = btif_hf_cb[idx].handle;

        /* Addition call setup with the Active call
        ** CIND response should have been updated.
        ** just open SCO conenction.
        */
        if(call_setup_state != BTHF_CALL_STATE_IDLE) {
            res = BTA_AG_MULTI_CALL_RES;
        } else {
            res = BTA_AG_OUT_CALL_CONN_RES;
        }

        BTA_AgResult(BTA_AG_HANDLE_ALL, res, &ag_res);
        activeCallUpdated = TRUE;
    }

    /* Ringing call changed? */
    if(call_setup_state != btif_hf_cb[idx].call_setup_state) {
        BTIF_TRACE_DEBUG("%s: Call setup states changed. old: %s new: %s",
                         __FUNCTION__, dump_hf_call_state(btif_hf_cb[idx].call_setup_state),
                         dump_hf_call_state(call_setup_state));
        wm_memset(&ag_res, 0, sizeof(tBTA_AG_RES_DATA));

        switch(call_setup_state) {
            case BTHF_CALL_STATE_IDLE: {
                switch(btif_hf_cb[idx].call_setup_state) {
                    case BTHF_CALL_STATE_INCOMING:
                        if(num_active > btif_hf_cb[idx].num_active) {
                            res = BTA_AG_IN_CALL_CONN_RES;
                            ag_res.audio_handle = btif_hf_cb[idx].handle;
                        } else if(num_held > btif_hf_cb[idx].num_held) {
                            res = BTA_AG_IN_CALL_HELD_RES;
                        } else {
                            res = BTA_AG_CALL_CANCEL_RES;
                        }

                        break;

                    case BTHF_CALL_STATE_DIALING:
                    case BTHF_CALL_STATE_ALERTING:
                        if(num_active > btif_hf_cb[idx].num_active) {
                            ag_res.audio_handle = BTA_AG_HANDLE_SCO_NO_CHANGE;
                            res = BTA_AG_OUT_CALL_CONN_RES;
                        } else {
                            res = BTA_AG_CALL_CANCEL_RES;
                        }

                        break;

                    default:
                        BTIF_TRACE_ERROR("%s: Incorrect Call setup state transition", __FUNCTION__);
                        status = TLS_BT_STATUS_PARM_INVALID;
                        break;
                }
            }
            break;

            case BTHF_CALL_STATE_INCOMING:
                if(num_active || num_held) {
                    res = BTA_AG_CALL_WAIT_RES;
                } else {
                    res = BTA_AG_IN_CALL_RES;
                }

                if(number) {
                    int xx = 0;

                    if((type == BTHF_CALL_ADDRTYPE_INTERNATIONAL) && (*number != '+')) {
                        xx = sprintf(ag_res.str, "\"+%s\"", number);
                    } else {
                        xx = sprintf(ag_res.str, "\"%s\"", number);
                    }

                    ag_res.num = type;

                    if(res == BTA_AG_CALL_WAIT_RES) {
                        sprintf(&ag_res.str[xx], ",%d", type);
                    }
                }

                break;

            case BTHF_CALL_STATE_DIALING:
                if(!(num_active + num_held)) {
                    ag_res.audio_handle = btif_hf_cb[idx].handle;
                }

                res = BTA_AG_OUT_CALL_ORIG_RES;
                break;

            case BTHF_CALL_STATE_ALERTING:

                /* if we went from idle->alert, force SCO setup here. dialing usually triggers it */
                if((btif_hf_cb[idx].call_setup_state == BTHF_CALL_STATE_IDLE) &&
                        !(num_active + num_held)) {
                    ag_res.audio_handle = btif_hf_cb[idx].handle;
                }

                res = BTA_AG_OUT_CALL_ALERT_RES;
                break;

            default:
                BTIF_TRACE_ERROR("%s: Incorrect new ringing call state", __FUNCTION__);
                status = TLS_BT_STATUS_PARM_INVALID;
                break;
        }

        BTIF_TRACE_DEBUG("%s: Call setup state changed. res=%d, audio_handle=%d", __FUNCTION__, res,
                         ag_res.audio_handle);

        if(res) {
            BTA_AgResult(BTA_AG_HANDLE_ALL, res, &ag_res);
        }

        /* if call setup is idle, we have already updated call indicator, jump out */
        if(call_setup_state == BTHF_CALL_STATE_IDLE) {
            /* check & update callheld */
            if((num_held > 0) && (num_active > 0)) {
                send_indicator_update(BTA_AG_IND_CALLHELD, 1);
            }

            goto update_call_states;
        }
    }

    wm_memset(&ag_res, 0, sizeof(tBTA_AG_RES_DATA));

    /* per the errata 2043, call=1 implies atleast one call is in progress (active/held)
    ** https://www.bluetooth.org/errata/errata_view.cfm?errata_id=2043
    ** Handle call indicator change
    **/
    if(!activeCallUpdated && ((num_active + num_held) !=
                              (btif_hf_cb[idx].num_active + btif_hf_cb[idx].num_held))) {
        BTIF_TRACE_DEBUG("%s: Active call states changed. old: %d new: %d", __FUNCTION__,
                         btif_hf_cb[idx].num_active, num_active);
        send_indicator_update(BTA_AG_IND_CALL, ((num_active + num_held) > 0) ? 1 : 0);
    }

    /* Held Changed? */
    if(num_held != btif_hf_cb[idx].num_held  ||
            ((num_active == 0) && ((num_held + btif_hf_cb[idx].num_held) > 1))) {
        BTIF_TRACE_DEBUG("%s: Held call states changed. old: %d new: %d",
                         __FUNCTION__, btif_hf_cb[idx].num_held, num_held);
        send_indicator_update(BTA_AG_IND_CALLHELD, ((num_held == 0) ? 0 : ((num_active == 0) ? 2 : 1)));
    }

    /* Calls Swapped? */
    if((call_setup_state == btif_hf_cb[idx].call_setup_state) &&
            (num_active && num_held) &&
            (num_active == btif_hf_cb[idx].num_active) &&
            (num_held == btif_hf_cb[idx].num_held)) {
        BTIF_TRACE_DEBUG("%s: Calls swapped", __FUNCTION__);
        send_indicator_update(BTA_AG_IND_CALLHELD, 1);
    }

update_call_states:

    for(i = 0; i < btif_max_hf_clients; i++) {
        btif_hf_cb[i].num_active = num_active;
        btif_hf_cb[i].num_held = num_held;
        btif_hf_cb[i].call_setup_state = call_setup_state;
    }

    return status;
}

/*******************************************************************************
**
** Function         btif_hf_is_call_idle
**
** Description      returns true if no call is in progress
**
** Returns          bt_status_t
**
*******************************************************************************/
uint8_t btif_hf_is_call_idle()
{
    if(bt_hf_callbacks == NULL) {
        return TRUE;
    }

    for(int i = 0; i < btif_max_hf_clients; ++i) {
        if((btif_hf_cb[i].call_setup_state != BTHF_CALL_STATE_IDLE)
                || ((btif_hf_cb[i].num_held + btif_hf_cb[i].num_active) > 0)) {
            return FALSE;
        }
    }

    return TRUE;
}

/*******************************************************************************
**
** Function         btif_hf_call_terminated_recently
**
** Description      Checks if a call has been terminated
**
** Returns          bt_status_t
**
*******************************************************************************/
uint8_t btif_hf_call_terminated_recently()
{
    struct timespec         now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if(now.tv_sec < btif_hf_cb[0].call_end_timestamp.tv_sec +
            BTIF_HF_CALL_END_TIMEOUT) {
        return TRUE;
    } else {
        btif_hf_cb[0].call_end_timestamp.tv_sec = 0;
        return FALSE;
    }
}

/*******************************************************************************
**
** Function         cleanup
**
** Description      Closes the HF interface
**
** Returns          bt_status_t
**
*******************************************************************************/
static void  cleanup(void)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);

    if(bt_hf_callbacks) {
        btif_disable_service(BTA_HFP_SERVICE_ID);
        bt_hf_callbacks = NULL;
    }
}

/*******************************************************************************
**
** Function         configure_wbs
**
** Description      set to over-ride the current WBS configuration.
**                  It will not send codec setting cmd to the controller now.
**                  It just change the configure.
**
** Returns          bt_status_t
**
*******************************************************************************/
static tls_bt_status_t  configure_wbs(tls_bt_addr_t *bd_addr, bthf_wbs_config_t config)
{
    CHECK_BTHF_INIT();
    int idx = btif_hf_idx_by_bdaddr(bd_addr);

    if((idx < 0) || (idx >= BTIF_HF_NUM_CB)) {
        BTIF_TRACE_ERROR("%s: Invalid index %d", __FUNCTION__, idx);
        return TLS_BT_STATUS_FAIL;
    }

    BTIF_TRACE_EVENT("%s config is %d", __FUNCTION__, config);

    if(config == BTHF_WBS_YES) {
        BTA_AgSetCodec(btif_hf_cb[idx].handle, BTA_AG_CODEC_MSBC);
    } else if(config == BTHF_WBS_NO) {
        BTA_AgSetCodec(btif_hf_cb[idx].handle, BTA_AG_CODEC_CVSD);
    } else {
        BTA_AgSetCodec(btif_hf_cb[idx].handle, BTA_AG_CODEC_NONE);
    }

    return TLS_BT_STATUS_SUCCESS;
}

static const bthf_interface_t bthfInterface = {
    sizeof(bthfInterface),
    init,
    connect,
    disconnect,
    connect_audio,
    disconnect_audio,
    start_voice_recognition,
    stop_voice_recognition,
    volume_control,
    device_status_notification,
    cops_response,
    cind_response,
    formatted_at_response,
    at_response,
    clcc_response,
    phone_state_change,
    cleanup,
    configure_wbs,
};

/*******************************************************************************
**
** Function         btif_hf_execute_service
**
** Description      Initializes/Shuts down the service
**
** Returns          BT_STATUS_SUCCESS on success, BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_hf_execute_service(uint8_t b_enable)
{
    char *p_service_names[] = BTIF_HF_SERVICE_NAMES;
    int i;

    if(b_enable) {
        /* Enable and register with BTA-AG */
        BTA_AgEnable(BTA_AG_PARSE, bte_hf_evt);

        for(i = 0; i < btif_max_hf_clients; i++) {
            BTA_AgRegister(BTIF_HF_SERVICES, BTIF_HF_SECURITY,
                           BTIF_HF_FEATURES, p_service_names, bthf_hf_id[i]);
        }
    } else {
        /* De-register AG */
        for(i = 0; i < btif_max_hf_clients; i++) {
            BTA_AgDeregister(btif_hf_cb[i].handle);
        }

        /* Disable AG */
        BTA_AgDisable();
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_hf_get_interface
**
** Description      Get the hf callback interface
**
** Returns          bthf_interface_t
**
*******************************************************************************/
const bthf_interface_t *btif_hf_get_interface()
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    return &bthfInterface;
}
#endif
