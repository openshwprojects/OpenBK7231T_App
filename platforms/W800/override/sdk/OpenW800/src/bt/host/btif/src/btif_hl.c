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
 *  Filename:      btif_hl.c
 *
 *  Description:   Health Device Profile Bluetooth Interface
 *
 *
 ***********************************************************************************/
#define LOG_TAG "bt_btif_hl"
#include "bt_target.h"
#if defined(BTA_HL_INCLUDED) && (BTA_HL_INCLUDED == TRUE)
#include "btif_hl.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/prctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include <hardware/bt_hl.h>

#include "bta_api.h"
#include "btif_common.h"
#include "btif_storage.h"
#include "btif_util.h"
#include "btu.h"
#include "mca_api.h"
#include "osi/include/list.h"
#include "osi/include/log.h"

#define MAX_DATATYPE_SUPPORTED 8

extern int btif_hl_update_maxfd(int max_org_s);
extern void btif_hl_select_monitor_callback(fd_set *p_cur_set, fd_set *p_org_set);
extern void btif_hl_select_wakeup_callback(fd_set *p_org_set, int wakeup_signal);
extern int btif_hl_update_maxfd(int max_org_s);
extern void btif_hl_select_monitor_callback(fd_set *p_cur_set, fd_set *p_org_set);
extern void btif_hl_select_wakeup_callback(fd_set *p_org_set, int wakeup_signal);
extern void btif_hl_soc_thread_init(void);
extern void btif_hl_release_mcl_sockets(uint8_t app_idx, uint8_t mcl_idx);
extern uint8_t btif_hl_create_socket(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx);
extern void btif_hl_release_socket(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx);

extern fixed_queue_t *btu_general_alarm_queue;

btif_hl_cb_t btif_hl_cb;
btif_hl_cb_t *p_btif_hl_cb = &btif_hl_cb;

/************************************************************************************
**  Static variables
************************************************************************************/
static bthl_callbacks_t  bt_hl_callbacks_cb;
static bthl_callbacks_t *bt_hl_callbacks = NULL;

/* signal socketpair to wake up select loop */

const int btif_hl_signal_select_wakeup = 1;
const int btif_hl_signal_select_exit = 2;
const int btif_hl_signal_select_close_connected = 3;

static int listen_s = -1;
static int connected_s = -1;
static int select_thread_id = -1;
static int signal_fds[2] = { -1, -1 };
static list_t *soc_queue;
static int reg_counter;

static inline int btif_hl_select_wakeup(void);
static inline int btif_hl_select_close_connected(void);
static inline int btif_hl_close_select_thread(void);
static uint8_t btif_hl_get_next_app_id(void);
static int btif_hl_get_next_channel_id(uint8_t app_id);
static void btif_hl_init_next_app_id(void);
static void btif_hl_init_next_channel_id(void);
static void btif_hl_ctrl_cback(tBTA_HL_CTRL_EVT event, tBTA_HL_CTRL *p_data);
static void btif_hl_set_state(btif_hl_state_t state);
static btif_hl_state_t btif_hl_get_state(void);
static void btif_hl_cback(tBTA_HL_EVT event, tBTA_HL *p_data);
static void btif_hl_proc_cb_evt(uint16_t event, char *p_param);

#define CHECK_CALL_CBACK(P_CB, P_CBACK, ...)\
    if (P_CB && P_CB->P_CBACK) {            \
        P_CB->P_CBACK(__VA_ARGS__);         \
    }                                       \
    else {                                  \
        ASSERTC(0, "Callback is NULL", 0);  \
    }

#define BTIF_HL_CALL_CBACK(P_CB, P_CBACK, ...)\
    if((p_btif_hl_cb->state != BTIF_HL_STATE_DISABLING) &&\
            (p_btif_hl_cb->state != BTIF_HL_STATE_DISABLED))  \
    {                                                     \
        if (P_CB && P_CB->P_CBACK) {                       \
            P_CB->P_CBACK(__VA_ARGS__);                    \
        }                                                  \
        else {                                             \
            ASSERTC(0, "Callback is NULL", 0);             \
        }                                                  \
    }

#define CHECK_BTHL_INIT() if (bt_hl_callbacks == NULL)\
    {\
        BTIF_TRACE_WARNING("BTHL: %s: BTHL not initialized", __FUNCTION__);\
        return TLS_BT_STATUS_NOT_READY;\
    }\
    else\
    {\
        BTIF_TRACE_EVENT("BTHL: %s", __FUNCTION__);\
    }

static const btif_hl_data_type_cfg_t data_type_table[] = {
    /* Data Specilization                   Ntx     Nrx (from Bluetooth SIG's HDP whitepaper)*/
    {BTIF_HL_DATA_TYPE_PULSE_OXIMETER,      9216,   256},
    {BTIF_HL_DATA_TYPE_BLOOD_PRESSURE_MON,  896,    224},
    {BTIF_HL_DATA_TYPE_BODY_THERMOMETER,    896,    224},
    {BTIF_HL_DATA_TYPE_BODY_WEIGHT_SCALE,   896,    224},
    {BTIF_HL_DATA_TYPE_GLUCOSE_METER,       896,    224},
    {BTIF_HL_DATA_TYPE_STEP_COUNTER,        6624,   224},
    {BTIF_HL_DATA_TYPE_BCA,                 7730,   1230},
    {BTIF_HL_DATA_TYPE_PEAK_FLOW,       2030,   224},
    {BTIF_HL_DATA_TYPE_ACTIVITY_HUB,        5120,   224},
    {BTIF_HL_DATA_TYPE_AMM,                 1024,   64}
};

#define BTIF_HL_DATA_TABLE_SIZE  (sizeof(data_type_table) / sizeof(btif_hl_data_type_cfg_t))
#define BTIF_HL_DEFAULT_SRC_TX_APDU_SIZE   10240 /* use this size if the data type is not
                                                    defined in the table; for future proof */
#define BTIF_HL_DEFAULT_SRC_RX_APDU_SIZE   512  /* use this size if the data type is not
                                                   defined in the table; for future proof */

#define BTIF_HL_ECHO_MAX_TX_RX_APDU_SIZE 1024

/************************************************************************************
**  Static utility functions
************************************************************************************/

#define BTIF_IF_GET_NAME 16
void btif_hl_display_calling_process_name(void)
{
    char name[16];
    prctl(BTIF_IF_GET_NAME, name, 0, 0, 0);
    BTIF_TRACE_DEBUG("Process name (%s)", name);
}
#define BTIF_TIMEOUT_CCH_NO_DCH_MS   (30 * 1000)

/*******************************************************************************
**
** Function      btif_hl_if_channel_setup_pending
**
** Description   check whether channel id is in setup pending state or not
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_if_channel_setup_pending(int channel_id, uint8_t *p_app_idx, uint8_t *p_mcl_idx)
{
    btif_hl_app_cb_t    *p_acb;
    btif_hl_mcl_cb_t    *p_mcb;
    uint8_t i, j;
    uint8_t found = FALSE;
    *p_app_idx = 0;
    *p_mcl_idx = 0;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        p_acb  = BTIF_HL_GET_APP_CB_PTR(i);

        if(p_acb->in_use) {
            for(j = 0; j < BTA_HL_NUM_MCLS; j++) {
                p_mcb = BTIF_HL_GET_MCL_CB_PTR(i, j);

                if(p_mcb->in_use &&
                        p_mcb->is_connected && p_mcb->pcb.channel_id == channel_id) {
                    found = TRUE;
                    *p_app_idx = i;
                    *p_mcl_idx = j;
                    break;
                }
            }
        }

        if(found) {
            break;
        }
    }

    BTIF_TRACE_DEBUG("%s found=%d channel_id=0x%08x",
                     __FUNCTION__, found, channel_id, *p_app_idx, *p_mcl_idx);
    return found;
}
/*******************************************************************************
**
** Function      btif_hl_num_dchs_in_use
**
** Description find number of DCHs in use
**
** Returns      uint8_t
*******************************************************************************/
uint8_t btif_hl_num_dchs_in_use(uint8_t mcl_handle)
{
    btif_hl_app_cb_t     *p_acb;
    btif_hl_mcl_cb_t    *p_mcb;
    uint8_t               i, j, x;
    uint8_t               cnt = 0;

    for(i = 0; i < BTA_HL_NUM_APPS; i++) {
        BTIF_TRACE_DEBUG("btif_hl_num_dchs:i = %d", i);
        p_acb = BTIF_HL_GET_APP_CB_PTR(i);

        if(p_acb && p_acb->in_use) {
            for(j = 0; j < BTA_HL_NUM_MCLS ; j++) {
                if(p_acb->mcb[j].in_use)
                    BTIF_TRACE_DEBUG("btif_hl_num_dchs:mcb in use j=%d, mcl_handle=%d,mcb handle=%d",
                                     j, mcl_handle, p_acb->mcb[j].mcl_handle);

                if(p_acb->mcb[j].in_use &&
                        (p_acb->mcb[j].mcl_handle == mcl_handle)) {
                    p_mcb = &p_acb->mcb[j];
                    BTIF_TRACE_DEBUG("btif_hl_num_dchs: mcl handle found j =%d", j);

                    for(x = 0; x < BTA_HL_NUM_MDLS_PER_MCL ; x ++) {
                        if(p_mcb->mdl[x].in_use) {
                            BTIF_TRACE_DEBUG("btif_hl_num_dchs_in_use:found x =%d", x);
                            cnt++;
                        }
                    }
                }
            }
        }
    }

    BTIF_TRACE_DEBUG("%s dch in use count=%d", __FUNCTION__, cnt);
    return cnt;
}
/*******************************************************************************
**
** Function      btif_hl_timer_timeout
**
** Description   Process timer timeout
**
** Returns      void
*******************************************************************************/
void btif_hl_timer_timeout(void *data)
{
    btif_hl_mcl_cb_t    *p_mcb = (btif_hl_mcl_cb_t *)data;
    BTIF_TRACE_DEBUG("%s", __func__);

    if(p_mcb->is_connected) {
        BTIF_TRACE_DEBUG("Idle timeout Close CCH mcl_handle=%d",
                         p_mcb->mcl_handle);
        BTA_HlCchClose(p_mcb->mcl_handle);
    } else {
        BTIF_TRACE_DEBUG("CCH idle timeout But CCH not connected");
    }
}

/*******************************************************************************
**
** Function      btif_hl_stop_cch_timer
**
** Description  stop CCH timer
**
** Returns      void
*******************************************************************************/
void btif_hl_stop_cch_timer(uint8_t app_idx, uint8_t mcl_idx)
{
    btif_hl_mcl_cb_t    *p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    BTIF_TRACE_DEBUG("%s app_idx=%d, mcl_idx=%d", __func__, app_idx, mcl_idx);
    alarm_cancel(p_mcb->cch_timer);
}

/*******************************************************************************
**
** Function      btif_hl_start_cch_timer
**
** Description  start CCH timer
**
** Returns      void
*******************************************************************************/
void btif_hl_start_cch_timer(uint8_t app_idx, uint8_t mcl_idx)
{
    btif_hl_mcl_cb_t    *p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    BTIF_TRACE_DEBUG("%s app_idx=%d, mcl_idx=%d", __func__, app_idx, mcl_idx);
    alarm_free(p_mcb->cch_timer);
    p_mcb->cch_timer = alarm_new("btif_hl.mcl_cch_timer");
    alarm_set_on_queue(p_mcb->cch_timer, BTIF_TIMEOUT_CCH_NO_DCH_MS,
                       btif_hl_timer_timeout, p_mcb,
                       btu_general_alarm_queue);
}

/*******************************************************************************
**
** Function      btif_hl_find_mdl_idx
**
** Description  Find the MDL index using MDL ID
**
** Returns      uint8_t
**
*******************************************************************************/
static uint8_t btif_hl_find_mdl_idx(uint8_t app_idx, uint8_t mcl_idx, uint16_t mdl_id,
                                    uint8_t *p_mdl_idx)
{
    btif_hl_mcl_cb_t      *p_mcb  = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_MDLS_PER_MCL ; i ++) {
        if(p_mcb->mdl[i].in_use  &&
                (mdl_id != 0) &&
                (p_mcb->mdl[i].mdl_id == mdl_id)) {
            found = TRUE;
            *p_mdl_idx = i;
            break;
        }
    }

    BTIF_TRACE_DEBUG("%s found=%d mdl_id=%d mdl_idx=%d ",
                     __FUNCTION__, found, mdl_id, i);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_is_the_first_reliable_existed
**
** Description  This function checks whether the first reliable DCH channel
**              has been setup on the MCL or not
**
** Returns      uint8_t - TRUE exist
**                        FALSE does not exist
**
*******************************************************************************/
uint8_t btif_hl_is_the_first_reliable_existed(uint8_t app_idx, uint8_t mcl_idx)
{
    btif_hl_mcl_cb_t          *p_mcb  = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    uint8_t is_existed = FALSE;
    uint8_t i ;

    for(i = 0; i < BTA_HL_NUM_MDLS_PER_MCL; i++) {
        if(p_mcb->mdl[i].in_use && p_mcb->mdl[i].is_the_first_reliable) {
            is_existed = TRUE;
            break;
        }
    }

    BTIF_TRACE_DEBUG("bta_hl_is_the_first_reliable_existed is_existed=%d  ", is_existed);
    return is_existed;
}
/*******************************************************************************
**
** Function      btif_hl_clean_delete_mdl
**
** Description   Cleanup the delete mdl control block
**
** Returns     Nothing
**
*******************************************************************************/
static void btif_hl_clean_delete_mdl(btif_hl_delete_mdl_t *p_cb)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    wm_memset(p_cb, 0, sizeof(btif_hl_delete_mdl_t));
}

/*******************************************************************************
**
** Function      btif_hl_clean_pcb
**
** Description   Cleanup the pending chan control block
**
** Returns     Nothing
**
*******************************************************************************/
static void btif_hl_clean_pcb(btif_hl_pending_chan_cb_t *p_pcb)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    wm_memset(p_pcb, 0, sizeof(btif_hl_pending_chan_cb_t));
}

/*******************************************************************************
**
** Function      btif_hl_clean_mdl_cb
**
** Description   Cleanup the MDL control block
**
** Returns     Nothing
**
*******************************************************************************/
static void btif_hl_clean_mdl_cb(btif_hl_mdl_cb_t *p_dcb)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    GKI_free_and_reset_buf((void **)&p_dcb->p_rx_pkt);
    GKI_free_and_reset_buf((void **)&p_dcb->p_tx_pkt);
    wm_memset(p_dcb, 0, sizeof(btif_hl_mdl_cb_t));
}

/*******************************************************************************
**
** Function      btif_hl_reset_mcb
**
** Description   Reset MCL control block
**
** Returns      uint8_t
**
*******************************************************************************/
static void btif_hl_clean_mcl_cb(uint8_t app_idx, uint8_t mcl_idx)
{
    btif_hl_mcl_cb_t     *p_mcb;
    BTIF_TRACE_DEBUG("%s app_idx=%d, mcl_idx=%d", __FUNCTION__, app_idx, mcl_idx);
    p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    alarm_free(p_mcb->cch_timer);
    wm_memset(p_mcb, 0, sizeof(btif_hl_mcl_cb_t));
}

/*******************************************************************************
**
** Function      btif_hl_find_sdp_idx_using_mdep_filter
**
** Description  This function finds the SDP record index using MDEP filter parameters
**
** Returns      uint8_t
**
*******************************************************************************/
static void btif_hl_reset_mdep_filter(uint8_t app_idx)
{
    btif_hl_app_cb_t          *p_acb  = BTIF_HL_GET_APP_CB_PTR(app_idx);
    p_acb->filter.num_elems = 0;
}

/*******************************************************************************
**
** Function      btif_hl_find_sdp_idx_using_mdep_filter
**
** Description  This function finds the SDP record index using MDEP filter parameters
**
** Returns      uint8_t
**
*******************************************************************************/
static uint8_t btif_hl_find_sdp_idx_using_mdep_filter(uint8_t app_idx, uint8_t mcl_idx,
        uint8_t *p_sdp_idx)
{
    btif_hl_app_cb_t          *p_acb  = BTIF_HL_GET_APP_CB_PTR(app_idx);
    btif_hl_mcl_cb_t          *p_mcb  = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    uint8_t                   i, j, num_recs, num_elems, num_mdeps, mdep_idx;
    tBTA_HL_MDEP_ROLE       peer_mdep_role;
    uint16_t                  data_type;
    tBTA_HL_SDP_MDEP_CFG    *p_mdep;
    uint8_t                 found = FALSE;
    uint8_t                 elem_found;
    BTIF_TRACE_DEBUG("btif_hl_find_sdp_idx_using_mdep_filter");
    num_recs = p_mcb->sdp.num_recs;
    num_elems = p_acb->filter.num_elems;

    if(!num_elems) {
        BTIF_TRACE_DEBUG("btif_hl_find_sdp_idx_using_mdep_filter num_elem=0");
        *p_sdp_idx = 0;
        found = TRUE;
        return found;
    }

    for(i = 0; i < num_recs; i++) {
        num_mdeps = p_mcb->sdp.sdp_rec[i].num_mdeps;

        for(j = 0; j < num_elems; j++) {
            data_type = p_acb->filter.elem[j].data_type;
            peer_mdep_role = p_acb->filter.elem[j].peer_mdep_role;
            elem_found = FALSE;
            mdep_idx = 0;

            while(!elem_found && mdep_idx < num_mdeps) {
                p_mdep = &(p_mcb->sdp.sdp_rec[i].mdep_cfg[mdep_idx]);

                if((p_mdep->data_type == data_type) &&
                        (p_mdep->mdep_role == peer_mdep_role)) {
                    elem_found = TRUE;
                } else {
                    mdep_idx++;
                }
            }

            if(!elem_found) {
                found = FALSE;
                break;
            } else {
                found = TRUE;
            }
        }

        if(found) {
            BTIF_TRACE_DEBUG("btif_hl_find_sdp_idx_using_mdep_filter found idx=%d", i);
            *p_sdp_idx = i;
            break;
        }
    }

    BTIF_TRACE_DEBUG("%s found=%d sdp_idx=%d", __FUNCTION__, found, *p_sdp_idx);
    btif_hl_reset_mdep_filter(app_idx);
    return found;
}
/*******************************************************************************
**
** Function      btif_hl_is_reconnect_possible
**
** Description  check reconnect is possible or not
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_is_reconnect_possible(uint8_t app_idx, uint8_t mcl_idx,  int mdep_cfg_idx,
                                      tBTA_HL_DCH_OPEN_PARAM *p_dch_open_api, tBTA_HL_MDL_ID *p_mdl_id)
{
    btif_hl_app_cb_t     *p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);
    btif_hl_mcl_cb_t     *p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_DCH_CFG      local_cfg = p_dch_open_api->local_cfg;
    tBTA_HL_DCH_MODE     dch_mode = BTA_HL_DCH_MODE_RELIABLE;
    uint8_t              use_mdl_dch_mode = FALSE;
    btif_hl_mdl_cfg_t    *p_mdl;
    btif_hl_mdl_cfg_t    *p_mdl1;
    uint8_t                i, j;
    uint8_t              is_reconnect_ok = FALSE;
    uint8_t              stream_mode_avail = FALSE;
    uint16_t               data_type =
                    p_acb->sup_feature.mdep[mdep_cfg_idx].mdep_cfg.data_cfg[0].data_type;
    tBTA_HL_MDEP_ID      peer_mdep_id = p_dch_open_api->peer_mdep_id;
    uint8_t                mdl_idx;
    BTIF_TRACE_DEBUG("%s app_idx=%d mcl_idx=%d mdep_cfg_idx=%d",
                     __FUNCTION__, app_idx, mcl_idx, mdep_cfg_idx);

    switch(local_cfg) {
        case BTA_HL_DCH_CFG_NO_PREF:
            if(!btif_hl_is_the_first_reliable_existed(app_idx, mcl_idx)) {
                dch_mode = BTA_HL_DCH_MODE_RELIABLE;
            } else {
                use_mdl_dch_mode = TRUE;
            }

            break;

        case BTA_HL_DCH_CFG_RELIABLE:
            dch_mode = BTA_HL_DCH_MODE_RELIABLE;
            break;

        case BTA_HL_DCH_CFG_STREAMING:
            dch_mode = BTA_HL_DCH_MODE_STREAMING;
            break;

        default:
            BTIF_TRACE_ERROR("Invalid local_cfg=%d", local_cfg);
            return is_reconnect_ok;
            break;
    }

    BTIF_TRACE_DEBUG("local_cfg=%d use_mdl_dch_mode=%d dch_mode=%d ",
                     local_cfg, use_mdl_dch_mode, dch_mode);

    for(i = 0, p_mdl = &p_acb->mdl_cfg[0] ; i < BTA_HL_NUM_MDL_CFGS; i++, p_mdl++) {
        if(p_mdl->base.active &&
                p_mdl->extra.data_type == data_type &&
                (p_mdl->extra.peer_mdep_id != BTA_HL_INVALID_MDEP_ID && p_mdl->extra.peer_mdep_id == peer_mdep_id)
                &&
                wm_memcpy(p_mdl->base.peer_bd_addr, p_mcb->bd_addr, sizeof(BD_ADDR)) &&
                p_mdl->base.mdl_id &&
                !btif_hl_find_mdl_idx(app_idx, mcl_idx, p_mdl->base.mdl_id, &mdl_idx)) {
            BTIF_TRACE_DEBUG("i=%d Matched active=%d   mdl_id =%d, mdl_dch_mode=%d",
                             i, p_mdl->base.active, p_mdl->base.mdl_id, p_mdl->base.dch_mode);

            if(!use_mdl_dch_mode) {
                if(p_mdl->base.dch_mode == dch_mode) {
                    is_reconnect_ok = TRUE;
                    *p_mdl_id = p_mdl->base.mdl_id;
                    BTIF_TRACE_DEBUG("reconnect is possible dch_mode=%d mdl_id=%d", dch_mode, p_mdl->base.mdl_id);
                    break;
                }
            } else {
                is_reconnect_ok = TRUE;

                for(j = i, p_mdl1 = &p_acb->mdl_cfg[i]; j < BTA_HL_NUM_MDL_CFGS; j++, p_mdl1++) {
                    if(p_mdl1->base.active &&
                            p_mdl1->extra.data_type == data_type &&
                            (p_mdl1->extra.peer_mdep_id != BTA_HL_INVALID_MDEP_ID
                             && p_mdl1->extra.peer_mdep_id == peer_mdep_id) &&
                            wm_memcpy(p_mdl1->base.peer_bd_addr, p_mcb->bd_addr, sizeof(BD_ADDR)) &&
                            p_mdl1->base.dch_mode == BTA_HL_DCH_MODE_STREAMING) {
                        stream_mode_avail = TRUE;
                        BTIF_TRACE_DEBUG("found streaming mode mdl index=%d", j);
                        break;
                    }
                }

                if(stream_mode_avail) {
                    dch_mode = BTA_HL_DCH_MODE_STREAMING;
                    *p_mdl_id = p_mdl1->base.mdl_id;
                    BTIF_TRACE_DEBUG("reconnect is ok index=%d dch_mode=streaming  mdl_id=%d", j, *p_mdl_id);
                    break;
                } else {
                    dch_mode = p_mdl->base.dch_mode;
                    *p_mdl_id = p_mdl->base.mdl_id;
                    BTIF_TRACE_DEBUG("reconnect is ok index=%d  dch_mode=%d mdl_id=%d", i,  p_mdl->base.dch_mode,
                                     *p_mdl_id);
                    break;
                }
            }
        }
    }

    BTIF_TRACE_DEBUG("is_reconnect_ok  dch_mode=%d mdl_id=%d", is_reconnect_ok, dch_mode, *p_mdl_id);
    return is_reconnect_ok;
}

/*******************************************************************************
**
** Function      btif_hl_dch_open
**
** Description   Process DCH open request using the DCH Open API parameters
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_dch_open(uint8_t app_id, BD_ADDR bd_addr,
                         tBTA_HL_DCH_OPEN_PARAM *p_dch_open_api,
                         int mdep_cfg_idx,
                         btif_hl_pend_dch_op_t op, int *channel_id)
{
    btif_hl_app_cb_t            *p_acb;
    btif_hl_mcl_cb_t            *p_mcb;
    btif_hl_pending_chan_cb_t   *p_pcb;
    uint8_t                       app_idx, mcl_idx;
    uint8_t                     status = FALSE;
    tBTA_HL_MDL_ID              mdl_id;
    tBTA_HL_DCH_RECONNECT_PARAM reconnect_param;
    BTIF_TRACE_DEBUG("%s app_id=%d ",
                     __FUNCTION__, app_id);
    BTIF_TRACE_DEBUG("DB [%02x:%02x:%02x:%02x:%02x:%02x]",
                     bd_addr[0],  bd_addr[1], bd_addr[2],  bd_addr[3], bd_addr[4],  bd_addr[5]);

    if(btif_hl_find_app_idx(app_id, &app_idx)) {
        if(btif_hl_find_mcl_idx(app_idx, bd_addr, &mcl_idx)) {
            p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
            p_pcb = BTIF_HL_GET_PCB_PTR(app_idx, mcl_idx);

            if(!p_pcb->in_use) {
                p_mcb->req_ctrl_psm = p_dch_open_api->ctrl_psm;
                p_pcb->in_use = TRUE;
                *channel_id       =
                                p_pcb->channel_id = (int) btif_hl_get_next_channel_id(app_id);
                p_pcb->cb_state = BTIF_HL_CHAN_CB_STATE_CONNECTING_PENDING;
                p_pcb->mdep_cfg_idx = mdep_cfg_idx;
                wm_memcpy(p_pcb->bd_addr, bd_addr, sizeof(BD_ADDR));
                p_pcb->op = op;

                if(p_mcb->sdp.num_recs) {
                    if(p_mcb->valid_sdp_idx) {
                        p_dch_open_api->ctrl_psm  = p_mcb->ctrl_psm;
                    }

                    if(!btif_hl_is_reconnect_possible(app_idx, mcl_idx, mdep_cfg_idx, p_dch_open_api, &mdl_id)) {
                        BTIF_TRACE_DEBUG("Issue DCH open");
                        BTA_HlDchOpen(p_mcb->mcl_handle, p_dch_open_api);
                    } else {
                        reconnect_param.ctrl_psm = p_mcb->ctrl_psm;
                        reconnect_param.mdl_id = mdl_id;;
                        BTIF_TRACE_DEBUG("Issue Reconnect ctrl_psm=0x%x mdl_id=0x%x", reconnect_param.ctrl_psm,
                                         reconnect_param.mdl_id);
                        BTA_HlDchReconnect(p_mcb->mcl_handle, &reconnect_param);
                    }

                    status = TRUE;
                } else {
                    p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);
                    p_mcb->cch_oper = BTIF_HL_CCH_OP_DCH_OPEN;
                    BTA_HlSdpQuery(app_id, p_acb->app_handle, bd_addr);
                    status = TRUE;
                }
            }
        }
    }

    BTIF_TRACE_DEBUG("status=%d ", status);
    return status;
}
/*******************************************************************************
**
** Function      btif_hl_copy_bda
**
** Description  copy bt_bdaddr_t to BD_ADDR format
**
** Returns      void
**
*******************************************************************************/
void btif_hl_copy_bda(tls_bt_addr_t *bd_addr, BD_ADDR  bda)
{
    uint8_t i;

    for(i = 0; i < 6; i++) {
        bd_addr->address[i] = bda[i] ;
    }
}
/*******************************************************************************
**
** Function      btif_hl_copy_bda
**
** Description  display bt_bdaddr_t
**
** Returns      uint8_t
**
*******************************************************************************/
void btif_hl_display_bt_bda(tls_bt_addr_t *bd_addr)
{
    BTIF_TRACE_DEBUG("DB [%02x:%02x:%02x:%02x:%02x:%02x]",
                     bd_addr->address[0],   bd_addr->address[1], bd_addr->address[2],
                     bd_addr->address[3],  bd_addr->address[4],   bd_addr->address[5]);
}

/*******************************************************************************
**
** Function         btif_hl_dch_abort
**
** Description      Process DCH abort request
**
** Returns          Nothing
**
*******************************************************************************/
void  btif_hl_dch_abort(uint8_t app_idx, uint8_t mcl_idx)
{
    btif_hl_mcl_cb_t      *p_mcb;
    BTIF_TRACE_DEBUG("%s app_idx=%d mcl_idx=%d", __FUNCTION__, app_idx, mcl_idx);
    p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

    if(p_mcb->is_connected) {
        BTA_HlDchAbort(p_mcb->mcl_handle);
    } else {
        p_mcb->pcb.abort_pending = TRUE;
    }
}
/*******************************************************************************
**
** Function      btif_hl_cch_open
**
** Description   Process CCH open request
**
** Returns     Nothing
**
*******************************************************************************/
uint8_t btif_hl_cch_open(uint8_t app_id, BD_ADDR bd_addr, uint16_t ctrl_psm,
                         int mdep_cfg_idx,
                         btif_hl_pend_dch_op_t op, int *channel_id)
{
    btif_hl_app_cb_t            *p_acb;
    btif_hl_mcl_cb_t            *p_mcb;
    btif_hl_pending_chan_cb_t   *p_pcb;
    uint8_t                       app_idx, mcl_idx;
    uint8_t                     status = TRUE;
    BTIF_TRACE_DEBUG("%s app_id=%d ctrl_psm=%d mdep_cfg_idx=%d op=%d",
                     __FUNCTION__, app_id, ctrl_psm, mdep_cfg_idx, op);
    BTIF_TRACE_DEBUG("DB [%02x:%02x:%02x:%02x:%02x:%02x]",
                     bd_addr[0],  bd_addr[1], bd_addr[2],  bd_addr[3], bd_addr[4],  bd_addr[5]);

    if(btif_hl_find_app_idx(app_id, &app_idx)) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);

        if(!btif_hl_find_mcl_idx(app_idx, bd_addr, &mcl_idx)) {
            if(btif_hl_find_avail_mcl_idx(app_idx, &mcl_idx)) {
                p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
                alarm_free(p_mcb->cch_timer);
                wm_memset(p_mcb, 0, sizeof(btif_hl_mcl_cb_t));
                p_mcb->in_use = TRUE;
                bdcpy(p_mcb->bd_addr, bd_addr);

                if(!ctrl_psm) {
                    p_mcb->cch_oper = BTIF_HL_CCH_OP_MDEP_FILTERING;
                } else {
                    p_mcb->cch_oper        = BTIF_HL_CCH_OP_MATCHED_CTRL_PSM;
                    p_mcb->req_ctrl_psm    = ctrl_psm;
                }

                p_pcb = BTIF_HL_GET_PCB_PTR(app_idx, mcl_idx);
                p_pcb->in_use = TRUE;
                p_pcb->mdep_cfg_idx = mdep_cfg_idx;
                wm_memcpy(p_pcb->bd_addr, bd_addr, sizeof(BD_ADDR));
                p_pcb->op = op;

                switch(op) {
                    case BTIF_HL_PEND_DCH_OP_OPEN:
                        *channel_id       =
                                        p_pcb->channel_id = (int) btif_hl_get_next_channel_id(app_id);
                        p_pcb->cb_state = BTIF_HL_CHAN_CB_STATE_CONNECTING_PENDING;
                        break;

                    case BTIF_HL_PEND_DCH_OP_DELETE_MDL:
                        p_pcb->channel_id =  p_acb->delete_mdl.channel_id;
                        p_pcb->cb_state = BTIF_HL_CHAN_CB_STATE_DESTROYED_PENDING;
                        break;

                    default:
                        break;
                }

                BTA_HlSdpQuery(app_id, p_acb->app_handle, bd_addr);
            } else {
                status = FALSE;
                BTIF_TRACE_ERROR("Open CCH request discarded- No mcl cb");
            }
        } else {
            status = FALSE;
            BTIF_TRACE_ERROR("Open CCH request discarded- already in USE");
        }
    } else {
        status = FALSE;
        BTIF_TRACE_ERROR("Invalid app_id=%d", app_id);
    }

    if(channel_id) {
        BTIF_TRACE_DEBUG("status=%d channel_id=0x%08x", status, *channel_id);
    } else {
        BTIF_TRACE_DEBUG("status=%d ", status);
    }

    return status;
}

/*******************************************************************************
**
** Function      btif_hl_find_mdl_idx_using_handle
**
** Description  Find the MDL index using channel id
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_find_mdl_cfg_idx_using_channel_id(int channel_id,
        uint8_t *p_app_idx,
        uint8_t *p_mdl_cfg_idx)
{
    btif_hl_app_cb_t      *p_acb;
    btif_hl_mdl_cfg_t     *p_mdl;
    uint8_t found = FALSE;
    uint8_t i, j;
    int mdl_cfg_channel_id;
    *p_app_idx = 0;
    *p_mdl_cfg_idx = 0;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(i);

        for(j = 0; j < BTA_HL_NUM_MDL_CFGS; j++) {
            p_mdl = BTIF_HL_GET_MDL_CFG_PTR(i, j);
            mdl_cfg_channel_id = *(BTIF_HL_GET_MDL_CFG_CHANNEL_ID_PTR(i, j));

            if(p_acb->in_use &&
                    p_mdl->base.active &&
                    (mdl_cfg_channel_id == channel_id)) {
                found = TRUE;
                *p_app_idx = i;
                *p_mdl_cfg_idx = j;
                break;
            }
        }
    }

    BTIF_TRACE_EVENT("%s found=%d channel_id=0x%08x, app_idx=%d mdl_cfg_idx=%d  ",
                     __FUNCTION__, found, channel_id, i, j);
    return found;
}
/*******************************************************************************
**
** Function      btif_hl_find_mdl_idx_using_handle
**
** Description  Find the MDL index using channel id
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_find_mdl_idx_using_channel_id(int channel_id,
        uint8_t *p_app_idx, uint8_t *p_mcl_idx,
        uint8_t *p_mdl_idx)
{
    btif_hl_app_cb_t      *p_acb;
    btif_hl_mcl_cb_t      *p_mcb;
    btif_hl_mdl_cb_t      *p_dcb;
    uint8_t found = FALSE;
    uint8_t i, j, k;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(i);

        for(j = 0; j < BTA_HL_NUM_MCLS; j++) {
            p_mcb = BTIF_HL_GET_MCL_CB_PTR(i, j);

            for(k = 0; k < BTA_HL_NUM_MDLS_PER_MCL; k++) {
                p_dcb = BTIF_HL_GET_MDL_CB_PTR(i, j, k);

                if(p_acb->in_use &&
                        p_mcb->in_use &&
                        p_dcb->in_use &&
                        (p_dcb->channel_id == channel_id)) {
                    found = TRUE;
                    *p_app_idx = i;
                    *p_mcl_idx = j;
                    *p_mdl_idx = k;
                    break;
                }
            }
        }
    }

    BTIF_TRACE_DEBUG("%s found=%d app_idx=%d mcl_idx=%d mdl_idx=%d  ",
                     __FUNCTION__, found, i, j, k);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_find_channel_id_using_mdl_id
**
** Description  Find channel id using mdl_id'
**
** Returns      uint8_t
*********************************************************************************/
uint8_t btif_hl_find_channel_id_using_mdl_id(uint8_t app_idx, tBTA_HL_MDL_ID mdl_id,
        int *p_channel_id)
{
    btif_hl_app_cb_t      *p_acb;
    btif_hl_mdl_cfg_t     *p_mdl;
    uint8_t found = FALSE;
    uint8_t j = 0;
    int mdl_cfg_channel_id;
    p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);

    if(p_acb && p_acb->in_use) {
        for(j = 0; j < BTA_HL_NUM_MDL_CFGS; j++) {
            p_mdl = BTIF_HL_GET_MDL_CFG_PTR(app_idx, j);
            mdl_cfg_channel_id = *(BTIF_HL_GET_MDL_CFG_CHANNEL_ID_PTR(app_idx, j));

            if(p_mdl->base.active && (p_mdl->base.mdl_id == mdl_id)) {
                found = TRUE;
                *p_channel_id = mdl_cfg_channel_id;
                break;
            }
        }
    }

    BTIF_TRACE_EVENT("%s found=%d channel_id=0x%08x, mdl_id=0x%x app_idx=%d mdl_cfg_idx=%d  ",
                     __FUNCTION__, found, *p_channel_id, mdl_id, app_idx, j);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_find_mdl_idx_using_handle
**
** Description  Find the MDL index using handle
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_find_mdl_idx_using_handle(tBTA_HL_MDL_HANDLE mdl_handle,
        uint8_t *p_app_idx, uint8_t *p_mcl_idx,
        uint8_t *p_mdl_idx)
{
    btif_hl_app_cb_t      *p_acb;
    btif_hl_mcl_cb_t      *p_mcb;
    btif_hl_mdl_cb_t      *p_dcb;
    uint8_t found = FALSE;
    uint8_t i, j, k;
    *p_app_idx = 0;
    *p_mcl_idx = 0;
    *p_mdl_idx = 0;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(i);

        for(j = 0; j < BTA_HL_NUM_MCLS; j++) {
            p_mcb = BTIF_HL_GET_MCL_CB_PTR(i, j);

            for(k = 0; k < BTA_HL_NUM_MDLS_PER_MCL; k++) {
                p_dcb = BTIF_HL_GET_MDL_CB_PTR(i, j, k);

                if(p_acb->in_use &&
                        p_mcb->in_use &&
                        p_dcb->in_use &&
                        (p_dcb->mdl_handle == mdl_handle)) {
                    found = TRUE;
                    *p_app_idx = i;
                    *p_mcl_idx = j;
                    *p_mdl_idx = k;
                    break;
                }
            }
        }
    }

    BTIF_TRACE_EVENT("%s found=%d app_idx=%d mcl_idx=%d mdl_idx=%d  ",
                     __FUNCTION__, found, i, j, k);
    return found;
}
/*******************************************************************************
**
** Function        btif_hl_find_peer_mdep_id
**
** Description      Find the peer MDEP ID from the received SPD records
**
** Returns          uint8_t
**
*******************************************************************************/
static uint8_t btif_hl_find_peer_mdep_id(uint8_t app_id, BD_ADDR bd_addr,
        tBTA_HL_MDEP_ROLE local_mdep_role,
        uint16_t data_type,
        tBTA_HL_MDEP_ID *p_peer_mdep_id)
{
    uint8_t               app_idx, mcl_idx;
    btif_hl_mcl_cb_t     *p_mcb;
    tBTA_HL_SDP_REC     *p_rec;
    uint8_t               i, num_mdeps;
    uint8_t             found = FALSE;
    tBTA_HL_MDEP_ROLE   peer_mdep_role;
    BTIF_TRACE_DEBUG("%s app_id=%d local_mdep_role=%d, data_type=%d",
                     __FUNCTION__, app_id, local_mdep_role, data_type);
    BTIF_TRACE_DEBUG("DB [%02x:%02x:%02x:%02x:%02x:%02x]",
                     bd_addr[0],  bd_addr[1],
                     bd_addr[2],  bd_addr[3],
                     bd_addr[4],  bd_addr[5]);
    BTIF_TRACE_DEBUG("local_mdep_role=%d", local_mdep_role);
    BTIF_TRACE_DEBUG("data_type=%d", data_type);

    if(local_mdep_role == BTA_HL_MDEP_ROLE_SINK) {
        peer_mdep_role = BTA_HL_MDEP_ROLE_SOURCE;
    } else {
        peer_mdep_role = BTA_HL_MDEP_ROLE_SINK;
    }

    if(btif_hl_find_app_idx(app_id, &app_idx)) {
        if(btif_hl_find_mcl_idx(app_idx, bd_addr, &mcl_idx)) {
            p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
            BTIF_TRACE_DEBUG("app_idx=%d mcl_idx=%d", app_idx, mcl_idx);
            BTIF_TRACE_DEBUG("valid_spd_idx=%d sdp_idx=%d", p_mcb->valid_sdp_idx, p_mcb->sdp_idx);

            if(p_mcb->valid_sdp_idx) {
                p_rec = &p_mcb->sdp.sdp_rec[p_mcb->sdp_idx];
                num_mdeps = p_rec->num_mdeps;
                BTIF_TRACE_DEBUG("num_mdeps=%d", num_mdeps);

                for(i = 0; i < num_mdeps; i++) {
                    BTIF_TRACE_DEBUG("p_rec->mdep_cfg[%d].mdep_role=%d", i, p_rec->mdep_cfg[i].mdep_role);
                    BTIF_TRACE_DEBUG("p_rec->mdep_cfg[%d].data_type =%d", i, p_rec->mdep_cfg[i].data_type);

                    if((p_rec->mdep_cfg[i].mdep_role == peer_mdep_role) &&
                            (p_rec->mdep_cfg[i].data_type == data_type)) {
                        found = TRUE;
                        *p_peer_mdep_id = p_rec->mdep_cfg[i].mdep_id;
                        break;
                    }
                }
            }
        }
    }

    BTIF_TRACE_DEBUG("found =%d  *p_peer_mdep_id=%d", found,  *p_peer_mdep_id);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_find_mdep_cfg_idx
**
** Description  Find the MDEP configuration index using local MDEP_ID
**
** Returns      uint8_t
**
*******************************************************************************/
static  uint8_t btif_hl_find_mdep_cfg_idx(uint8_t app_idx,  tBTA_HL_MDEP_ID local_mdep_id,
        uint8_t *p_mdep_cfg_idx)
{
    btif_hl_app_cb_t      *p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_SUP_FEATURE     *p_sup_feature = &p_acb->sup_feature;
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < p_sup_feature->num_of_mdeps; i++) {
        BTIF_TRACE_DEBUG("btif_hl_find_mdep_cfg_idx: mdep_id=%d app_idx = %d",
                         p_sup_feature->mdep[i].mdep_id, app_idx);

        if(p_sup_feature->mdep[i].mdep_id == local_mdep_id) {
            found = TRUE;
            *p_mdep_cfg_idx = i;
            break;
        }
    }

    BTIF_TRACE_DEBUG("%s found=%d mdep_idx=%d local_mdep_id=%d app_idx=%d ",
                     __FUNCTION__, found, i, local_mdep_id, app_idx);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_find_mcl_idx
**
** Description  Find the MCL index using BD address
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_find_mcl_idx(uint8_t app_idx, BD_ADDR p_bd_addr, uint8_t *p_mcl_idx)
{
    uint8_t found = FALSE;
    uint8_t i;
    btif_hl_mcl_cb_t  *p_mcb;
    *p_mcl_idx = 0;

    for(i = 0; i < BTA_HL_NUM_MCLS ; i ++) {
        p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, i);

        if(p_mcb->in_use &&
                (!memcmp(p_mcb->bd_addr, p_bd_addr, BD_ADDR_LEN))) {
            found = TRUE;
            *p_mcl_idx = i;
            break;
        }
    }

    BTIF_TRACE_DEBUG("%s found=%d idx=%d", __FUNCTION__, found, i);
    return found;
}
/*******************************************************************************
**
** Function         btif_hl_init
**
** Description      HL initialization function.
**
** Returns          void
**
*******************************************************************************/
static void btif_hl_init(void)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    wm_memset(p_btif_hl_cb, 0, sizeof(btif_hl_cb_t));
    btif_hl_init_next_app_id();
    btif_hl_init_next_channel_id();
}
/*******************************************************************************
**
** Function         btif_hl_disable
**
** Description      Disable initialization function.
**
** Returns          void
**
*******************************************************************************/
static void btif_hl_disable(void)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    if((p_btif_hl_cb->state != BTIF_HL_STATE_DISABLING) &&
            (p_btif_hl_cb->state != BTIF_HL_STATE_DISABLED)) {
        btif_hl_set_state(BTIF_HL_STATE_DISABLING);
        BTA_HlDisable();
    }
}
/*******************************************************************************
**
** Function      btif_hl_is_no_active_app
**
** Description  Find whether or not  any APP is still in use
**
** Returns      uint8_t
**
*******************************************************************************/
static uint8_t btif_hl_is_no_active_app(void)
{
    uint8_t no_active_app = TRUE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        if(btif_hl_cb.acb[i].in_use) {
            no_active_app = FALSE;
            break;
        }
    }

    BTIF_TRACE_DEBUG("%s no_active_app=%d  ", __FUNCTION__, no_active_app);
    return no_active_app;
}

/*******************************************************************************
**
** Function      btif_hl_free_app_idx
**
** Description free an application control block
**
** Returns      void
**
*******************************************************************************/
static void btif_hl_free_app_idx(uint8_t app_idx)
{
    if((app_idx < BTA_HL_NUM_APPS) && btif_hl_cb.acb[app_idx].in_use) {
        btif_hl_cb.acb[app_idx].in_use = FALSE;

        for(size_t i = 0; i < BTA_HL_NUM_MCLS; i++) {
            alarm_free(btif_hl_cb.acb[app_idx].mcb[i].cch_timer);
        }

        wm_memset(&btif_hl_cb.acb[app_idx], 0, sizeof(btif_hl_app_cb_t));
    }
}
/*******************************************************************************
**
** Function      btif_hl_set_state
**
** Description set HL state
**
** Returns      void
**
*******************************************************************************/
static void btif_hl_set_state(btif_hl_state_t state)
{
    BTIF_TRACE_DEBUG("btif_hl_set_state:  %d ---> %d ", p_btif_hl_cb->state, state);
    p_btif_hl_cb->state = state;
}

/*******************************************************************************
**
** Function      btif_hl_set_state
**
** Description get HL state
**
** Returns      btif_hl_state_t
**
*******************************************************************************/

static btif_hl_state_t btif_hl_get_state(void)
{
    BTIF_TRACE_DEBUG("btif_hl_get_state:  %d   ", p_btif_hl_cb->state);
    return p_btif_hl_cb->state;
}

/*******************************************************************************
**
** Function      btif_hl_find_data_type_idx
**
** Description  Find the index in the data type table
**
** Returns      uint8_t
**
*******************************************************************************/
static uint8_t  btif_hl_find_data_type_idx(uint16_t data_type, uint8_t *p_idx)
{
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTIF_HL_DATA_TABLE_SIZE; i++) {
        if(data_type_table[i].data_type == data_type) {
            found = TRUE;
            *p_idx = i;
            break;
        }
    }

    BTIF_TRACE_DEBUG("%s found=%d, data_type=0x%x idx=%d", __FUNCTION__, found, data_type, i);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_get_max_tx_apdu_size
**
** Description  Find the maximum TX APDU size for the specified data type and
**              MDEP role
**
** Returns      uint16_t
**
*******************************************************************************/
uint16_t  btif_hl_get_max_tx_apdu_size(tBTA_HL_MDEP_ROLE mdep_role,
                                       uint16_t data_type)
{
    uint8_t idx;
    uint16_t max_tx_apdu_size = 0;

    if(btif_hl_find_data_type_idx(data_type, &idx)) {
        if(mdep_role == BTA_HL_MDEP_ROLE_SOURCE) {
            max_tx_apdu_size = data_type_table[idx].max_tx_apdu_size;
        } else {
            max_tx_apdu_size = data_type_table[idx].max_rx_apdu_size;
        }
    } else {
        if(mdep_role == BTA_HL_MDEP_ROLE_SOURCE) {
            max_tx_apdu_size = BTIF_HL_DEFAULT_SRC_TX_APDU_SIZE;
        } else {
            max_tx_apdu_size = BTIF_HL_DEFAULT_SRC_RX_APDU_SIZE;
        }
    }

    BTIF_TRACE_DEBUG("%s mdep_role=%d data_type=0x%4x size=%d",
                     __FUNCTION__, mdep_role, data_type, max_tx_apdu_size);
    return max_tx_apdu_size;
}

/*******************************************************************************
**
** Function      btif_hl_get_max_rx_apdu_size
**
** Description  Find the maximum RX APDU size for the specified data type and
**              MDEP role
**
** Returns      uint16_t
**
*******************************************************************************/
uint16_t  btif_hl_get_max_rx_apdu_size(tBTA_HL_MDEP_ROLE mdep_role,
                                       uint16_t data_type)
{
    uint8_t  idx;
    uint16_t max_rx_apdu_size = 0;

    if(btif_hl_find_data_type_idx(data_type, &idx)) {
        if(mdep_role == BTA_HL_MDEP_ROLE_SOURCE) {
            max_rx_apdu_size = data_type_table[idx].max_rx_apdu_size;
        } else {
            max_rx_apdu_size = data_type_table[idx].max_tx_apdu_size;
        }
    } else {
        if(mdep_role == BTA_HL_MDEP_ROLE_SOURCE) {
            max_rx_apdu_size = BTIF_HL_DEFAULT_SRC_RX_APDU_SIZE;
        } else {
            max_rx_apdu_size = BTIF_HL_DEFAULT_SRC_TX_APDU_SIZE;
        }
    }

    BTIF_TRACE_DEBUG("%s mdep_role=%d data_type=0x%4x size=%d",
                     __FUNCTION__, mdep_role, data_type, max_rx_apdu_size);
    return max_rx_apdu_size;
}

/*******************************************************************************
**
** Function      btif_hl_if_channel_setup_pending
**
** Description
**
** Returns      uint8_t
**
*******************************************************************************/

static uint8_t btif_hl_get_bta_mdep_role(bthl_mdep_role_t mdep, tBTA_HL_MDEP_ROLE *p)
{
    uint8_t status = TRUE;

    switch(mdep) {
        case BTHL_MDEP_ROLE_SOURCE:
            *p = BTA_HL_MDEP_ROLE_SOURCE;
            break;

        case BTHL_MDEP_ROLE_SINK:
            *p = BTA_HL_MDEP_ROLE_SINK;
            break;

        default:
            *p = BTA_HL_MDEP_ROLE_SOURCE;
            status = FALSE;
            break;
    }

    BTIF_TRACE_DEBUG("%s status=%d bta_mdep_role=%d (%d:btif)",
                     __FUNCTION__, status, *p, mdep);
    return status;
}
/*******************************************************************************
**
** Function btif_hl_get_bta_channel_type
**
** Description convert bthl channel type to BTA DCH channel type
**
** Returns uint8_t
**
*******************************************************************************/

static uint8_t btif_hl_get_bta_channel_type(bthl_channel_type_t channel_type, tBTA_HL_DCH_CFG *p)
{
    uint8_t status = TRUE;

    switch(channel_type) {
        case BTHL_CHANNEL_TYPE_RELIABLE:
            *p = BTA_HL_DCH_CFG_RELIABLE;
            break;

        case BTHL_CHANNEL_TYPE_STREAMING:
            *p = BTA_HL_DCH_CFG_STREAMING;
            break;

        case BTHL_CHANNEL_TYPE_ANY:
            *p = BTA_HL_DCH_CFG_NO_PREF;
            break;

        default:
            status = FALSE;
            break;
    }

    BTIF_TRACE_DEBUG("%s status = %d BTA DCH CFG=%d (1-rel 2-strm",
                     __FUNCTION__, status, *p);
    return status;
}
/*******************************************************************************
**
** Function btif_hl_get_next_app_id
**
** Description get next applcation id
**
** Returns uint8_t
**
*******************************************************************************/

static uint8_t btif_hl_get_next_app_id()
{
    uint8_t next_app_id = btif_hl_cb.next_app_id;
    btif_hl_cb.next_app_id++;
    return next_app_id;
}
/*******************************************************************************
**
** Function btif_hl_get_next_channel_id
**
** Description get next channel id
**
** Returns int
**
*******************************************************************************/
static int btif_hl_get_next_channel_id(uint8_t app_id)
{
    uint16_t next_channel_id = btif_hl_cb.next_channel_id;
    int channel_id;
    btif_hl_cb.next_channel_id++;
    channel_id = (app_id << 16) + next_channel_id;
    BTIF_TRACE_DEBUG("%s channel_id=0x%08x, app_id=0x%02x next_channel_id=0x%04x", __FUNCTION__,
                     channel_id, app_id,  next_channel_id);
    return channel_id;
}
/*******************************************************************************
**
** Function btif_hl_get_app_id
**
** Description get the applicaiton id associated with the channel id
**
** Returns uint8_t
**
*******************************************************************************/

static uint8_t btif_hl_get_app_id(int channel_id)
{
    uint8_t app_id = (uint8_t)(channel_id >> 16);
    BTIF_TRACE_DEBUG("%s channel_id=0x%08x, app_id=0x%02x ", __FUNCTION__, channel_id, app_id);
    return app_id;
}
/*******************************************************************************
**
** Function btif_hl_init_next_app_id
**
** Description initialize the application id
**
** Returns void
**
*******************************************************************************/
static void btif_hl_init_next_app_id(void)
{
    btif_hl_cb.next_app_id = 1;
}
/*******************************************************************************
**
** Function btif_hl_init_next_channel_id
**
** Description initialize the channel id
**
** Returns void
**
*******************************************************************************/
static void btif_hl_init_next_channel_id(void)
{
    btif_hl_cb.next_channel_id = 1;
}

/*******************************************************************************
**
** Function      btif_hl_find_app_idx_using_handle
**
** Description  Find the applicaiton index using handle
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_find_app_idx_using_handle(tBTA_HL_APP_HANDLE app_handle,
        uint8_t *p_app_idx)
{
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        if(btif_hl_cb.acb[i].in_use &&
                (btif_hl_cb.acb[i].app_handle == app_handle)) {
            found = TRUE;
            *p_app_idx = i;
            break;
        }
    }

    BTIF_TRACE_EVENT("%s status=%d handle=%d app_idx=%d ",
                     __FUNCTION__, found, app_handle, i);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_find_app_idx_using_app_id
**
** Description  Find the applicaiton index using app_id
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_find_app_idx_using_app_id(uint8_t app_id,
        uint8_t *p_app_idx)
{
    uint8_t found = FALSE;
    uint8_t i;
    *p_app_idx = 0;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        if(btif_hl_cb.acb[i].in_use &&
                (btif_hl_cb.acb[i].app_id == app_id)) {
            found = TRUE;
            *p_app_idx = i;
            break;
        }
    }

    BTIF_TRACE_EVENT("%s found=%d app_id=%d app_idx=%d ",
                     __FUNCTION__, found, app_id, i);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_find_mcl_idx_using_handle
**
** Description  Find the MCL index using handle
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_find_mcl_idx_using_handle(tBTA_HL_MCL_HANDLE mcl_handle,
        uint8_t *p_app_idx, uint8_t *p_mcl_idx)
{
    btif_hl_app_cb_t  *p_acb;
    uint8_t         found = FALSE;
    uint8_t i, j;

    for(i = 0; i < BTA_HL_NUM_APPS; i++) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(i);

        for(j = 0; j < BTA_HL_NUM_MCLS ; j++) {
            if(p_acb->mcb[j].in_use)
                BTIF_TRACE_DEBUG("btif_hl_find_mcl_idx_using_handle:app_idx=%d,"
                                 "mcl_idx =%d mcl_handle=%d", i, j, p_acb->mcb[j].mcl_handle);

            if(p_acb->mcb[j].in_use &&
                    (p_acb->mcb[j].mcl_handle == mcl_handle)) {
                found = TRUE;
                *p_app_idx = i;
                *p_mcl_idx = j;
                break;
            }
        }
    }

    BTIF_TRACE_DEBUG("%s found=%d app_idx=%d mcl_idx=%d", __FUNCTION__,
                     found, i, j);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_find_mdl_idx_using_mdl_id
**
** Description  Find the mdl index using mdl_id
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_find_mcl_idx_using_mdl_id(uint8_t mdl_id, uint8_t mcl_handle,
        uint8_t *p_app_idx, uint8_t *p_mcl_idx)
{
    btif_hl_app_cb_t  *p_acb;
    btif_hl_mcl_cb_t  *p_mcb;
    uint8_t         found = FALSE;
    uint8_t i, j, x;

    for(i = 0; i < BTA_HL_NUM_APPS; i++) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(i);

        for(j = 0; j < BTA_HL_NUM_MCLS ; j++) {
            if(p_acb->mcb[j].in_use &&
                    (p_acb->mcb[j].mcl_handle == mcl_handle)) {
                p_mcb = &p_acb->mcb[j];
                BTIF_TRACE_DEBUG("btif_hl_find_mcl_idx_using_mdl_id: mcl handle found j =%d", j);

                for(x = 0; x < BTA_HL_NUM_MDLS_PER_MCL ; x ++) {
                    if(p_mcb->mdl[x].in_use && p_mcb->mdl[x].mdl_id == mdl_id) {
                        BTIF_TRACE_DEBUG("btif_hl_find_mcl_idx_using_mdl_id:found x =%d", x);
                        found = TRUE;
                        *p_app_idx = i;
                        *p_mcl_idx = j;
                        break;
                    }
                }
            }
        }
    }

    BTIF_TRACE_DEBUG("%s found=%d app_idx=%d mcl_idx=%d", __FUNCTION__,
                     found, i, j);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_find_mcl_idx_using_deleted_mdl_id
**
** Description  Find the app index deleted_mdl_id
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_find_app_idx_using_deleted_mdl_id(uint8_t mdl_id,
        uint8_t *p_app_idx)
{
    btif_hl_app_cb_t  *p_acb;
    uint8_t         found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_APPS; i++) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(i);

        if(p_acb->delete_mdl.active) {
            BTIF_TRACE_DEBUG("%s: app_idx=%d, mdl_id=%d",
                             __FUNCTION__, i, mdl_id);
        }

        if(p_acb->delete_mdl.active &&
                (p_acb->delete_mdl.mdl_id == mdl_id)) {
            found = TRUE;
            *p_app_idx = i;
            break;
        }
    }

    BTIF_TRACE_DEBUG("%s found=%d app_idx=%d", __FUNCTION__,
                     found, i);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_stop_timer_using_handle
**
** Description  clean control channel cb using handle
**
** Returns      void
**
*******************************************************************************/
static void btif_hl_stop_timer_using_handle(tBTA_HL_MCL_HANDLE mcl_handle)
{
    btif_hl_app_cb_t  *p_acb;
    uint8_t i, j;

    for(i = 0; i < BTA_HL_NUM_APPS; i++) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(i);

        for(j = 0; j < BTA_HL_NUM_MCLS ; j++) {
            if(p_acb->mcb[j].in_use &&
                    (p_acb->mcb[j].mcl_handle == mcl_handle)) {
                btif_hl_stop_cch_timer(i, j);
            }
        }
    }
}

/*******************************************************************************
**
** Function      btif_hl_find_mcl_idx_using_app_idx
**
** Description  Find the MCL index using handle
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_find_mcl_idx_using_app_idx(tBTA_HL_MCL_HANDLE mcl_handle,
        uint8_t p_app_idx, uint8_t *p_mcl_idx)
{
    btif_hl_app_cb_t  *p_acb;
    uint8_t         found = FALSE;
    uint8_t j;
    p_acb = BTIF_HL_GET_APP_CB_PTR(p_app_idx);

    for(j = 0; j < BTA_HL_NUM_MCLS ; j++) {
        if(p_acb->mcb[j].in_use &&
                (p_acb->mcb[j].mcl_handle == mcl_handle)) {
            found = TRUE;
            *p_mcl_idx = j;
            break;
        }
    }

    BTIF_TRACE_DEBUG("%s found=%dmcl_idx=%d", __FUNCTION__,
                     found, j);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_clean_mdls_using_app_idx
**
** Description  clean dch cpntrol bloack using app_idx
**
** Returns      void
**
*******************************************************************************/
void btif_hl_clean_mdls_using_app_idx(uint8_t app_idx)
{
    btif_hl_app_cb_t  *p_acb;
    btif_hl_mcl_cb_t  *p_mcb;
    btif_hl_mdl_cb_t  *p_dcb;
    uint8_t j, x, y;
    tls_bt_addr_t     bd_addr;
    p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);

    for(j = 0; j < BTA_HL_NUM_MCLS ; j++) {
        if(p_acb->mcb[j].in_use) {
            p_mcb = &p_acb->mcb[j];
            BTIF_TRACE_DEBUG("btif_hl_find_mcl_idx_using_mdl_id: mcl handle found j =%d", j);

            for(x = 0; x < BTA_HL_NUM_MDLS_PER_MCL ; x ++) {
                if(p_mcb->mdl[x].in_use) {
                    p_dcb = BTIF_HL_GET_MDL_CB_PTR(app_idx, j, x);
                    btif_hl_release_socket(app_idx, j, x);

                    for(y = 0; y < 6; y++) {
                        bd_addr.address[y] = p_mcb->bd_addr[y];
                    }

                    BTIF_HL_CALL_CBACK(bt_hl_callbacks, channel_state_cb,  p_acb->app_id,
                                       &bd_addr, p_dcb->local_mdep_cfg_idx,
                                       p_dcb->channel_id, BTHL_CONN_STATE_DISCONNECTED, 0);
                    btif_hl_clean_mdl_cb(p_dcb);

                    if(!btif_hl_num_dchs_in_use(p_mcb->mcl_handle)) {
                        BTA_HlCchClose(p_mcb->mcl_handle);
                    }

                    BTIF_TRACE_DEBUG("remote DCH close success mdl_idx=%d", x);
                }
            }
        }
    }
}

/*******************************************************************************
**
** Function      btif_hl_find_app_idx
**
** Description  Find the application index using application ID
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_find_app_idx(uint8_t app_id, uint8_t *p_app_idx)
{
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        if(btif_hl_cb.acb[i].in_use &&
                (btif_hl_cb.acb[i].app_id == app_id)) {
            found = TRUE;
            *p_app_idx = i;
            break;
        }
    }

    BTIF_TRACE_DEBUG("%s found=%d app_idx=%d", __FUNCTION__, found, i);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_find_app_idx
**
** Description  Find the application index using application ID
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_find_app_idx_using_mdepId(uint8_t mdep_id, uint8_t *p_app_idx)
{
    uint8_t found = FALSE;
    uint8_t i;
    *p_app_idx = 0;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        BTIF_TRACE_DEBUG("btif_hl_find_app_idx_using_mdepId: MDEP-ID = %d",
                         btif_hl_cb.acb[i].sup_feature.mdep[0].mdep_id);

        if(btif_hl_cb.acb[i].in_use &&
                (btif_hl_cb.acb[i].sup_feature.mdep[0].mdep_id == mdep_id)) {
            found = TRUE;
            *p_app_idx = i;
            break;
        }
    }

    BTIF_TRACE_DEBUG("%s found=%d app_idx=%d", __FUNCTION__, found, i);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_find_avail_mdl_idx
**
** Description  Find a not in-use MDL index
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_find_avail_mdl_idx(uint8_t app_idx, uint8_t mcl_idx,
                                   uint8_t *p_mdl_idx)
{
    btif_hl_mcl_cb_t      *p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_MDLS_PER_MCL ; i ++) {
        if(!p_mcb->mdl[i].in_use) {
            btif_hl_clean_mdl_cb(&p_mcb->mdl[i]);
            found = TRUE;
            *p_mdl_idx = i;
            break;
        }
    }

    BTIF_TRACE_DEBUG("%s found=%d idx=%d", __FUNCTION__, found, i);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_find_avail_mcl_idx
**
** Description  Find a not in-use MDL index
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t btif_hl_find_avail_mcl_idx(uint8_t app_idx, uint8_t *p_mcl_idx)
{
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_MCLS ; i ++) {
        if(!btif_hl_cb.acb[app_idx].mcb[i].in_use) {
            found = TRUE;
            *p_mcl_idx = i;
            break;
        }
    }

    BTIF_TRACE_DEBUG("%s found=%d mcl_idx=%d", __FUNCTION__, found, i);
    return found;
}

/*******************************************************************************
**
** Function      btif_hl_find_avail_app_idx
**
** Description  Find a not in-use APP index
**
** Returns      uint8_t
**
*******************************************************************************/
static uint8_t btif_hl_find_avail_app_idx(uint8_t *p_idx)
{
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        if(!btif_hl_cb.acb[i].in_use) {
            found = TRUE;
            *p_idx = i;
            break;
        }
    }

    BTIF_TRACE_DEBUG("%s found=%d app_idx=%d", __FUNCTION__, found, i);
    return found;
}

/*******************************************************************************
**
** Function         btif_hl_proc_dereg_cfm
**
** Description      Process the de-registration confirmation
**
** Returns          Nothing
**
*******************************************************************************/
static void btif_hl_proc_dereg_cfm(tBTA_HL *p_data)

{
    btif_hl_app_cb_t        *p_acb;
    uint8_t                   app_idx;
    int                     app_id = 0;
    bthl_app_reg_state_t    state = BTHL_APP_REG_STATE_DEREG_SUCCESS;
    BTIF_TRACE_DEBUG("%s de-reg status=%d app_handle=%d", __FUNCTION__,
                     p_data->dereg_cfm.status, p_data->dereg_cfm.app_handle);

    if(btif_hl_find_app_idx_using_app_id(p_data->dereg_cfm.app_id, &app_idx)) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);
        app_id = (int) p_acb->app_id;

        if(p_data->dereg_cfm.status == BTA_HL_STATUS_OK) {
            btif_hl_clean_mdls_using_app_idx(app_idx);

            for(size_t i = 0; i < BTA_HL_NUM_MCLS; i++) {
                alarm_free(p_acb->mcb[i].cch_timer);
            }

            wm_memset(p_acb, 0, sizeof(btif_hl_app_cb_t));
        } else {
            state = BTHL_APP_REG_STATE_DEREG_FAILED;
        }

        BTIF_TRACE_DEBUG("call reg state callback app_id=%d state=%d", app_id, state);
        BTIF_HL_CALL_CBACK(bt_hl_callbacks, app_reg_state_cb, app_id, state);

        if(btif_hl_is_no_active_app()) {
            btif_hl_disable();
        }
    }
}

/*******************************************************************************
**
** Function         btif_hl_proc_reg_cfm
**
** Description      Process the registration confirmation
**
** Returns          Nothing
**
*******************************************************************************/
static void btif_hl_proc_reg_cfm(tBTA_HL *p_data)
{
    btif_hl_app_cb_t       *p_acb;
    uint8_t                  app_idx;
    bthl_app_reg_state_t   state = BTHL_APP_REG_STATE_REG_SUCCESS;
    BTIF_TRACE_DEBUG("%s reg status=%d app_handle=%d", __FUNCTION__, p_data->reg_cfm.status,
                     p_data->reg_cfm.app_handle);

    if(btif_hl_find_app_idx(p_data->reg_cfm.app_id, &app_idx)) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);

        if(p_data->reg_cfm.status == BTA_HL_STATUS_OK) {
            p_acb->app_handle = p_data->reg_cfm.app_handle;
        } else {
            btif_hl_free_app_idx(app_idx);
            reg_counter--;
            state = BTHL_APP_REG_STATE_REG_FAILED;
        }

        BTIF_TRACE_DEBUG("%s call reg state callback app_id=%d reg state=%d", __FUNCTION__,
                         p_data->reg_cfm.app_id, state);
        BTIF_HL_CALL_CBACK(bt_hl_callbacks, app_reg_state_cb, ((int) p_data->reg_cfm.app_id), state);
    }
}

/*******************************************************************************
**
** Function btif_hl_set_chan_cb_state
**
** Description set the channel callback state
**
** Returns void
**
*******************************************************************************/
void btif_hl_set_chan_cb_state(uint8_t app_idx, uint8_t mcl_idx, btif_hl_chan_cb_state_t state)
{
    btif_hl_pending_chan_cb_t   *p_pcb = BTIF_HL_GET_PCB_PTR(app_idx, mcl_idx);
    btif_hl_chan_cb_state_t cur_state = p_pcb->cb_state;

    if(cur_state != state) {
        p_pcb->cb_state = state;
        BTIF_TRACE_DEBUG("%s state %d--->%d", __FUNCTION__, cur_state, state);
    }
}
/*******************************************************************************
**
** Function btif_hl_send_destroyed_cb
**
** Description send the channel destroyed callback
**
** Returns void
**
*******************************************************************************/
void btif_hl_send_destroyed_cb(btif_hl_app_cb_t        *p_acb)
{
    tls_bt_addr_t     bd_addr;
    int             app_id = (int) btif_hl_get_app_id(p_acb->delete_mdl.channel_id);
    btif_hl_copy_bda(&bd_addr, p_acb->delete_mdl.bd_addr);
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    BTIF_TRACE_DEBUG("call channel state callback channel_id=0x%08x mdep_cfg_idx=%d, state=%d fd=%d",
                     p_acb->delete_mdl.channel_id,
                     p_acb->delete_mdl.mdep_cfg_idx, BTHL_CONN_STATE_DESTROYED, 0);
    btif_hl_display_bt_bda(&bd_addr);
    BTIF_HL_CALL_CBACK(bt_hl_callbacks, channel_state_cb,  app_id,
                       &bd_addr, p_acb->delete_mdl.mdep_cfg_idx,
                       p_acb->delete_mdl.channel_id, BTHL_CONN_STATE_DESTROYED, 0);
}
/*******************************************************************************
**
** Function btif_hl_send_disconnecting_cb
**
** Description send a channel disconnecting callback
**
** Returns void
**
*******************************************************************************/
void btif_hl_send_disconnecting_cb(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx)
{
    btif_hl_mdl_cb_t        *p_dcb = BTIF_HL_GET_MDL_CB_PTR(app_idx,  mcl_idx, mdl_idx);
    btif_hl_soc_cb_t        *p_scb = p_dcb->p_scb;
    tls_bt_addr_t             bd_addr;
    int                     app_id = (int) btif_hl_get_app_id(p_scb->channel_id);
    btif_hl_copy_bda(&bd_addr, p_scb->bd_addr);
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    BTIF_TRACE_DEBUG("call channel state callback  channel_id=0x%08x mdep_cfg_idx=%d, state=%d fd=%d",
                     p_scb->channel_id,
                     p_scb->mdep_cfg_idx, BTHL_CONN_STATE_DISCONNECTING, p_scb->socket_id[0]);
    btif_hl_display_bt_bda(&bd_addr);
    BTIF_HL_CALL_CBACK(bt_hl_callbacks, channel_state_cb,  app_id,
                       &bd_addr, p_scb->mdep_cfg_idx,
                       p_scb->channel_id, BTHL_CONN_STATE_DISCONNECTING, p_scb->socket_id[0]);
}
/*******************************************************************************
**
** Function btif_hl_send_setup_connecting_cb
**
** Description send a channel connecting callback
**
** Returns void
**
*******************************************************************************/
void btif_hl_send_setup_connecting_cb(uint8_t app_idx, uint8_t mcl_idx)
{
    btif_hl_pending_chan_cb_t   *p_pcb = BTIF_HL_GET_PCB_PTR(app_idx, mcl_idx);
    tls_bt_addr_t                 bd_addr;
    int                         app_id = (int) btif_hl_get_app_id(p_pcb->channel_id);
    btif_hl_copy_bda(&bd_addr, p_pcb->bd_addr);

    if(p_pcb->in_use && p_pcb->cb_state == BTIF_HL_CHAN_CB_STATE_CONNECTING_PENDING) {
        BTIF_TRACE_DEBUG("%s", __FUNCTION__);
        BTIF_TRACE_DEBUG("call channel state callback  channel_id=0x%08x mdep_cfg_idx=%d state=%d fd=%d",
                         p_pcb->channel_id,
                         p_pcb->mdep_cfg_idx, BTHL_CONN_STATE_CONNECTING, 0);
        btif_hl_display_bt_bda(&bd_addr);
        BTIF_HL_CALL_CBACK(bt_hl_callbacks, channel_state_cb, app_id,
                           &bd_addr, p_pcb->mdep_cfg_idx,
                           p_pcb->channel_id, BTHL_CONN_STATE_CONNECTING, 0);
        btif_hl_set_chan_cb_state(app_idx, mcl_idx, BTIF_HL_CHAN_CB_STATE_CONNECTED_PENDING);
    }
}
/*******************************************************************************
**
** Function btif_hl_send_setup_disconnected_cb
**
** Description send a channel disconnected callback
**
** Returns void
**
*******************************************************************************/
void btif_hl_send_setup_disconnected_cb(uint8_t app_idx, uint8_t mcl_idx)
{
    btif_hl_pending_chan_cb_t   *p_pcb = BTIF_HL_GET_PCB_PTR(app_idx, mcl_idx);
    tls_bt_addr_t                 bd_addr;
    int                         app_id = (int) btif_hl_get_app_id(p_pcb->channel_id);
    btif_hl_copy_bda(&bd_addr, p_pcb->bd_addr);
    BTIF_TRACE_DEBUG("%s p_pcb->in_use=%d", __FUNCTION__, p_pcb->in_use);

    if(p_pcb->in_use) {
        BTIF_TRACE_DEBUG("%p_pcb->cb_state=%d", p_pcb->cb_state);

        if(p_pcb->cb_state == BTIF_HL_CHAN_CB_STATE_CONNECTING_PENDING) {
            BTIF_TRACE_DEBUG("call channel state callback  channel_id=0x%08x mdep_cfg_idx=%d state=%d fd=%d",
                             p_pcb->channel_id,
                             p_pcb->mdep_cfg_idx, BTHL_CONN_STATE_CONNECTING, 0);
            btif_hl_display_bt_bda(&bd_addr);
            BTIF_HL_CALL_CBACK(bt_hl_callbacks, channel_state_cb, app_id,
                               &bd_addr, p_pcb->mdep_cfg_idx,
                               p_pcb->channel_id, BTHL_CONN_STATE_CONNECTING, 0);
            BTIF_TRACE_DEBUG("call channel state callback  channel_id=0x%08x mdep_cfg_idx=%d state=%d fd=%d",
                             p_pcb->channel_id,
                             p_pcb->mdep_cfg_idx, BTHL_CONN_STATE_DISCONNECTED, 0);
            btif_hl_display_bt_bda(&bd_addr);
            BTIF_HL_CALL_CBACK(bt_hl_callbacks, channel_state_cb, app_id,
                               &bd_addr, p_pcb->mdep_cfg_idx,
                               p_pcb->channel_id, BTHL_CONN_STATE_DISCONNECTED, 0);
        } else if(p_pcb->cb_state == BTIF_HL_CHAN_CB_STATE_CONNECTED_PENDING) {
            BTIF_TRACE_DEBUG("call channel state callback  channel_id=0x%08x mdep_cfg_idx=%d state=%d fd=%d",
                             p_pcb->channel_id,
                             p_pcb->mdep_cfg_idx, BTHL_CONN_STATE_DISCONNECTED, 0);
            btif_hl_display_bt_bda(&bd_addr);
            BTIF_HL_CALL_CBACK(bt_hl_callbacks, channel_state_cb,  app_id,
                               &bd_addr, p_pcb->mdep_cfg_idx,
                               p_pcb->channel_id, BTHL_CONN_STATE_DISCONNECTED, 0);
        }

        btif_hl_clean_pcb(p_pcb);
    }
}
/*******************************************************************************
**
** Function         btif_hl_proc_sdp_query_cfm
**
** Description      Process the SDP query confirmation
**
** Returns          Nothing
**
*******************************************************************************/
static uint8_t btif_hl_proc_sdp_query_cfm(tBTA_HL *p_data)
{
    btif_hl_app_cb_t                *p_acb;
    btif_hl_mcl_cb_t                *p_mcb;
    tBTA_HL_SDP                     *p_sdp;
    tBTA_HL_CCH_OPEN_PARAM          open_param;
    uint8_t                           app_idx, mcl_idx, sdp_idx = 0;
    uint8_t                           num_recs, i, num_mdeps, j;
    btif_hl_cch_op_t                old_cch_oper;
    uint8_t                         status = FALSE;
    btif_hl_pending_chan_cb_t     *p_pcb;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    p_sdp = p_data->sdp_query_cfm.p_sdp;
    num_recs = p_sdp->num_recs;
    BTIF_TRACE_DEBUG("num of SDP records=%d", num_recs);

    for(i = 0; i < num_recs; i++) {
        BTIF_TRACE_DEBUG("rec_idx=%d ctrl_psm=0x%x data_psm=0x%x",
                         (i + 1), p_sdp->sdp_rec[i].ctrl_psm, p_sdp->sdp_rec[i].data_psm);
        BTIF_TRACE_DEBUG("MCAP supported procedures=0x%x", p_sdp->sdp_rec[i].mcap_sup_proc);
        num_mdeps = p_sdp->sdp_rec[i].num_mdeps;
        BTIF_TRACE_DEBUG("num of mdeps =%d", num_mdeps);

        for(j = 0; j < num_mdeps; j++) {
            BTIF_TRACE_DEBUG("mdep_idx=%d mdep_id=0x%x data_type=0x%x mdep_role=0x%x",
                             (j + 1),
                             p_sdp->sdp_rec[i].mdep_cfg[j].mdep_id,
                             p_sdp->sdp_rec[i].mdep_cfg[j].data_type,
                             p_sdp->sdp_rec[i].mdep_cfg[j].mdep_role);
        }
    }

    if(btif_hl_find_app_idx_using_app_id(p_data->sdp_query_cfm.app_id, &app_idx)) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);

        if(btif_hl_find_mcl_idx(app_idx, p_data->sdp_query_cfm.bd_addr, &mcl_idx)) {
            p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

            if(p_mcb->cch_oper != BTIF_HL_CCH_OP_NONE) {
                wm_memcpy(&p_mcb->sdp, p_sdp, sizeof(tBTA_HL_SDP));
                old_cch_oper = p_mcb->cch_oper;
                p_mcb->cch_oper = BTIF_HL_CCH_OP_NONE;

                switch(old_cch_oper) {
                    case BTIF_HL_CCH_OP_MDEP_FILTERING:
                        status = btif_hl_find_sdp_idx_using_mdep_filter(app_idx,
                                 mcl_idx, &sdp_idx);
                        break;

                    default:
                        break;
                }

                if(status) {
                    p_mcb->sdp_idx       = sdp_idx;
                    p_mcb->valid_sdp_idx = TRUE;
                    p_mcb->ctrl_psm      = p_mcb->sdp.sdp_rec[sdp_idx].ctrl_psm;

                    switch(old_cch_oper) {
                        case BTIF_HL_CCH_OP_MDEP_FILTERING:
                            p_pcb = BTIF_HL_GET_PCB_PTR(app_idx, mcl_idx);

                            if(p_pcb->in_use) {
                                if(!p_pcb->abort_pending) {
                                    switch(p_pcb->op) {
                                        case BTIF_HL_PEND_DCH_OP_OPEN:
                                            btif_hl_send_setup_connecting_cb(app_idx, mcl_idx);
                                            break;

                                        case BTIF_HL_PEND_DCH_OP_DELETE_MDL:
                                        default:
                                            break;
                                    }

                                    open_param.ctrl_psm = p_mcb->ctrl_psm;
                                    bdcpy(open_param.bd_addr, p_mcb->bd_addr);
                                    open_param.sec_mask =
                                                    (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT);
                                    BTA_HlCchOpen(p_acb->app_id, p_acb->app_handle, &open_param);
                                } else {
                                    BTIF_TRACE_DEBUG("channel abort pending");
                                }
                            }

                            break;

                        case BTIF_HL_CCH_OP_DCH_OPEN:
                            status = btif_hl_proc_pending_op(app_idx, mcl_idx);
                            break;

                        default:
                            BTIF_TRACE_ERROR("Invalid CCH oper %d", old_cch_oper);
                            break;
                    }
                } else {
                    BTIF_TRACE_ERROR("Can not find SDP idx discard CCH Open request");
                }
            }
        }
    }

    return status;
}

/*******************************************************************************
**
** Function         btif_hl_proc_cch_open_ind
**
** Description      Process the CCH open indication
**
** Returns          Nothing
**
*******************************************************************************/
static void btif_hl_proc_cch_open_ind(tBTA_HL *p_data)

{
    btif_hl_mcl_cb_t         *p_mcb;
    uint8_t                   mcl_idx;
    int                     i;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    for(i = 0; i < BTA_HL_NUM_APPS; i++) {
        if(btif_hl_cb.acb[i].in_use) {
            if(!btif_hl_find_mcl_idx(i, p_data->cch_open_ind.bd_addr, &mcl_idx)) {
                if(btif_hl_find_avail_mcl_idx(i, &mcl_idx)) {
                    p_mcb = BTIF_HL_GET_MCL_CB_PTR(i, mcl_idx);
                    alarm_free(p_mcb->cch_timer);
                    wm_memset(p_mcb, 0, sizeof(btif_hl_mcl_cb_t));
                    p_mcb->in_use = TRUE;
                    p_mcb->is_connected = TRUE;
                    p_mcb->mcl_handle = p_data->cch_open_ind.mcl_handle;
                    bdcpy(p_mcb->bd_addr, p_data->cch_open_ind.bd_addr);
                    btif_hl_start_cch_timer(i, mcl_idx);
                }
            } else {
                BTIF_TRACE_ERROR("The MCL already exist for cch_open_ind");
            }
        }
    }
}

/*******************************************************************************
**
** Function         btif_hl_proc_pending_op
**
** Description      Process the pending dch operation.
**
** Returns          Nothing
**
*******************************************************************************/
uint8_t btif_hl_proc_pending_op(uint8_t app_idx, uint8_t mcl_idx)

{
    btif_hl_app_cb_t            *p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);
    btif_hl_mcl_cb_t            *p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    btif_hl_pending_chan_cb_t   *p_pcb;
    uint8_t                     status = FALSE;
    tBTA_HL_DCH_OPEN_PARAM      dch_open;
    tBTA_HL_MDL_ID              mdl_id;
    tBTA_HL_DCH_RECONNECT_PARAM reconnect_param;
    p_pcb = BTIF_HL_GET_PCB_PTR(app_idx, mcl_idx);

    if(p_pcb->in_use) {
        switch(p_pcb->op) {
            case BTIF_HL_PEND_DCH_OP_OPEN:
                if(!p_pcb->abort_pending) {
                    BTIF_TRACE_DEBUG("op BTIF_HL_PEND_DCH_OP_OPEN");
                    dch_open.ctrl_psm = p_mcb->ctrl_psm;
                    dch_open.local_mdep_id = p_acb->sup_feature.mdep[p_pcb->mdep_cfg_idx].mdep_id;

                    if(btif_hl_find_peer_mdep_id(p_acb->app_id, p_mcb->bd_addr,
                                                 p_acb->sup_feature.mdep[p_pcb->mdep_cfg_idx].mdep_cfg.mdep_role,
                                                 p_acb->sup_feature.mdep[p_pcb->mdep_cfg_idx].mdep_cfg.data_cfg[0].data_type,
                                                 &dch_open.peer_mdep_id)) {
                        dch_open.local_cfg = p_acb->channel_type[p_pcb->mdep_cfg_idx];

                        if((p_acb->sup_feature.mdep[p_pcb->mdep_cfg_idx].mdep_cfg.mdep_role == BTA_HL_MDEP_ROLE_SOURCE)
                                && !btif_hl_is_the_first_reliable_existed(app_idx, mcl_idx)) {
                            dch_open.local_cfg = BTA_HL_DCH_CFG_RELIABLE;
                        }

                        dch_open.sec_mask = (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT);
                        BTIF_TRACE_DEBUG("dch_open.local_cfg=%d  ", dch_open.local_cfg);
                        btif_hl_send_setup_connecting_cb(app_idx, mcl_idx);

                        if(!btif_hl_is_reconnect_possible(app_idx, mcl_idx, p_pcb->mdep_cfg_idx, &dch_open, &mdl_id)) {
                            BTIF_TRACE_DEBUG("Issue DCH open, mcl_handle=%d", p_mcb->mcl_handle);
                            BTA_HlDchOpen(p_mcb->mcl_handle, &dch_open);
                        } else {
                            reconnect_param.ctrl_psm = p_mcb->ctrl_psm;
                            reconnect_param.mdl_id = mdl_id;;
                            BTIF_TRACE_DEBUG("Issue Reconnect ctrl_psm=0x%x mdl_id=0x%x", reconnect_param.ctrl_psm,
                                             reconnect_param.mdl_id);
                            BTA_HlDchReconnect(p_mcb->mcl_handle, &reconnect_param);
                        }

                        status = TRUE;
                    }
                } else {
                    btif_hl_send_setup_disconnected_cb(app_idx, mcl_idx);
                    status = TRUE;
                }

                break;

            case BTIF_HL_PEND_DCH_OP_DELETE_MDL:
                BTA_HlDeleteMdl(p_mcb->mcl_handle, p_acb->delete_mdl.mdl_id);
                status = TRUE;
                break;

            default:
                break;
        }
    }

    return status;
}

/*******************************************************************************
**
** Function         btif_hl_proc_cch_open_cfm
**
** Description      Process the CCH open confirmation
**
** Returns          Nothing
**
*******************************************************************************/
static uint8_t btif_hl_proc_cch_open_cfm(tBTA_HL *p_data)

{
    btif_hl_mcl_cb_t         *p_mcb;
    uint8_t                    app_idx, mcl_idx;
    uint8_t                  status = FALSE;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    if(btif_hl_find_app_idx_using_app_id(p_data->cch_open_cfm.app_id, &app_idx)) {
        BTIF_TRACE_DEBUG("app_idx=%d", app_idx);

        if(btif_hl_find_mcl_idx(app_idx, p_data->cch_open_cfm.bd_addr, &mcl_idx)) {
            p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
            BTIF_TRACE_DEBUG("mcl_idx=%d, mcl_handle=%d", mcl_idx, p_data->cch_open_cfm.mcl_handle);
            p_mcb->mcl_handle = p_data->cch_open_cfm.mcl_handle;
            p_mcb->is_connected = TRUE;
            status = btif_hl_proc_pending_op(app_idx, mcl_idx);

            if(status) {
                btif_hl_start_cch_timer(app_idx, mcl_idx);
            }
        }
    }

    return status;
}

/*******************************************************************************
**
** Function      btif_hl_clean_mcb_using_handle
**
** Description  clean control channel cb using handle
**
** Returns      void
**
*******************************************************************************/
static void btif_hl_clean_mcb_using_handle(tBTA_HL_MCL_HANDLE mcl_handle)
{
    btif_hl_app_cb_t  *p_acb;
    uint8_t i, j;

    for(i = 0; i < BTA_HL_NUM_APPS; i++) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(i);

        for(j = 0; j < BTA_HL_NUM_MCLS ; j++) {
            if(p_acb->mcb[j].in_use)
                BTIF_TRACE_DEBUG("btif_hl_find_mcl_idx_using_handle: app_idx=%d,"
                                 "mcl_idx =%d mcl_handle=%d", i, j, p_acb->mcb[j].mcl_handle);

            if(p_acb->mcb[j].in_use &&
                    (p_acb->mcb[j].mcl_handle == mcl_handle)) {
                btif_hl_stop_cch_timer(i, j);
                btif_hl_release_mcl_sockets(i, j);
                btif_hl_send_setup_disconnected_cb(i, j);
                btif_hl_clean_mcl_cb(i, j);
            }
        }
    }
}

/*******************************************************************************
**
** Function         btif_hl_proc_cch_close_ind
**
** Description      Process the CCH close indication
**
** Returns          Nothing
**
*******************************************************************************/
static void btif_hl_proc_cch_close_ind(tBTA_HL *p_data)

{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    btif_hl_clean_mcb_using_handle(p_data->cch_close_ind.mcl_handle);
}

/*******************************************************************************
**
** Function         btif_hl_proc_cch_close_cfm
**
** Description      Process the CCH close confirmation
**
** Returns          Nothing
**
*******************************************************************************/
static void btif_hl_proc_cch_close_cfm(tBTA_HL *p_data)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    btif_hl_clean_mcb_using_handle(p_data->cch_close_ind.mcl_handle);
}

/*******************************************************************************
**
** Function         btif_hl_proc_create_ind
**
** Description      Process the MDL create indication
**
** Returns          Nothing
**
*******************************************************************************/
static void btif_hl_proc_create_ind(tBTA_HL *p_data)
{
    btif_hl_app_cb_t         *p_acb;
    btif_hl_mcl_cb_t         *p_mcb;
    tBTA_HL_MDEP            *p_mdep;
    uint8_t                   orig_app_idx, mcl_idx, mdep_cfg_idx;
    uint8_t                 first_reliable_exist;
    uint8_t                 success = TRUE;
    tBTA_HL_DCH_CFG         rsp_cfg = BTA_HL_DCH_CFG_UNKNOWN;
    tBTA_HL_DCH_CREATE_RSP  rsp_code = BTA_HL_DCH_CREATE_RSP_CFG_REJ;
    tBTA_HL_DCH_CREATE_RSP_PARAM create_rsp_param;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    // Find the correct app_idx based on the mdep_id;
    btif_hl_find_app_idx_using_mdepId(p_data->dch_create_ind.local_mdep_id, &orig_app_idx);

    if(btif_hl_find_mcl_idx(orig_app_idx, p_data->dch_create_ind.bd_addr, &mcl_idx)) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(orig_app_idx);
        p_mcb = BTIF_HL_GET_MCL_CB_PTR(orig_app_idx, mcl_idx);

        if(btif_hl_find_mdep_cfg_idx(orig_app_idx, p_data->dch_create_ind.local_mdep_id, &mdep_cfg_idx)) {
            p_mdep = &(p_acb->sup_feature.mdep[mdep_cfg_idx]);
            first_reliable_exist = btif_hl_is_the_first_reliable_existed(orig_app_idx, mcl_idx);

            switch(p_mdep->mdep_cfg.mdep_role) {
                case BTA_HL_MDEP_ROLE_SOURCE:
                    if(p_data->dch_create_ind.cfg == BTA_HL_DCH_CFG_NO_PREF) {
                        if(first_reliable_exist) {
                            rsp_cfg = p_acb->channel_type[mdep_cfg_idx];
                        } else {
                            rsp_cfg = BTA_HL_DCH_CFG_RELIABLE;
                        }

                        rsp_code = BTA_HL_DCH_CREATE_RSP_SUCCESS;
                    }

                    break;

                case BTA_HL_MDEP_ROLE_SINK:
                    BTIF_TRACE_DEBUG("btif_hl_proc_create_ind:BTA_HL_MDEP_ROLE_SINK");

                    if((p_data->dch_create_ind.cfg  == BTA_HL_DCH_CFG_RELIABLE) ||
                            (first_reliable_exist && (p_data->dch_create_ind.cfg  == BTA_HL_DCH_CFG_STREAMING))) {
                        rsp_code = BTA_HL_DCH_CREATE_RSP_SUCCESS;
                        rsp_cfg = p_data->dch_create_ind.cfg;
                        BTIF_TRACE_DEBUG("btif_hl_proc_create_ind:BTA_HL_MDEP_ROLE_SINK cfg = %d", rsp_cfg);
                    }

                    break;

                default:
                    break;
            }
        }
    } else {
        success = FALSE;
    }

    if(success) {
        BTIF_TRACE_DEBUG("create response rsp_code=%d rsp_cfg=%d", rsp_code, rsp_cfg);
        create_rsp_param.local_mdep_id = p_data->dch_create_ind.local_mdep_id;
        create_rsp_param.mdl_id = p_data->dch_create_ind.mdl_id;
        create_rsp_param.rsp_code = rsp_code;
        create_rsp_param.cfg_rsp = rsp_cfg;
        BTA_HlDchCreateRsp(p_mcb->mcl_handle, &create_rsp_param);
    }
}

/*******************************************************************************
**
** Function         btif_hl_proc_dch_open_ind
**
** Description      Process the DCH open indication
**
** Returns          Nothing
**
*******************************************************************************/
static void btif_hl_proc_dch_open_ind(tBTA_HL *p_data)

{
    btif_hl_mdl_cb_t         *p_dcb;
    uint8_t                    orig_app_idx, mcl_idx, mdl_idx, mdep_cfg_idx;
    uint8_t close_dch = FALSE;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    // Find the correct app_idx based on the mdep_id;
    btif_hl_find_app_idx_using_mdepId(p_data->dch_open_ind.local_mdep_id, &orig_app_idx);

    if(btif_hl_find_mcl_idx_using_app_idx(p_data->dch_open_ind.mcl_handle, orig_app_idx, &mcl_idx)) {
        if(btif_hl_find_avail_mdl_idx(orig_app_idx, mcl_idx, &mdl_idx)) {
            p_dcb = BTIF_HL_GET_MDL_CB_PTR(orig_app_idx, mcl_idx, mdl_idx);

            if(btif_hl_find_mdep_cfg_idx(orig_app_idx, p_data->dch_open_ind.local_mdep_id, &mdep_cfg_idx)) {
                p_dcb->in_use               = TRUE;
                p_dcb->mdl_handle           =  p_data->dch_open_ind.mdl_handle;
                p_dcb->local_mdep_cfg_idx   = mdep_cfg_idx;
                p_dcb->local_mdep_id        = p_data->dch_open_ind.local_mdep_id;
                p_dcb->mdl_id               = p_data->dch_open_ind.mdl_id;
                p_dcb->dch_mode             = p_data->dch_open_ind.dch_mode;
                p_dcb->dch_mode             = p_data->dch_open_ind.dch_mode;
                p_dcb->is_the_first_reliable = p_data->dch_open_ind.first_reliable;
                p_dcb->mtu                  = p_data->dch_open_ind.mtu;

                if(btif_hl_find_channel_id_using_mdl_id(orig_app_idx, p_dcb->mdl_id, &p_dcb->channel_id)) {
                    BTIF_TRACE_DEBUG(" app_idx=%d mcl_idx=%d mdl_idx=%d channel_id=%d",
                                     orig_app_idx, mcl_idx, mdl_idx, p_dcb->channel_id);

                    if(!btif_hl_create_socket(orig_app_idx, mcl_idx, mdl_idx)) {
                        BTIF_TRACE_ERROR("Unable to create socket");
                        close_dch = TRUE;
                    }
                } else {
                    BTIF_TRACE_ERROR("Unable find channel id for mdl_id=0x%x", p_dcb->mdl_id);
                    close_dch = TRUE;
                }
            } else {
                BTIF_TRACE_ERROR("INVALID_LOCAL_MDEP_ID mdep_id=%d", p_data->dch_open_cfm.local_mdep_id);
                close_dch = TRUE;
            }

            if(close_dch) {
                btif_hl_clean_mdl_cb(p_dcb);
            }
        } else {
            close_dch = TRUE;
        }
    } else {
        close_dch = TRUE;
    }

    if(close_dch) {
        BTA_HlDchClose(p_data->dch_open_cfm.mdl_handle);
    }
}

/*******************************************************************************
**
** Function         btif_hl_proc_dch_open_cfm
**
** Description      Process the DCH close confirmation
**
** Returns          Nothing
**
*******************************************************************************/
static uint8_t btif_hl_proc_dch_open_cfm(tBTA_HL *p_data)

{
    btif_hl_mdl_cb_t            *p_dcb;
    btif_hl_pending_chan_cb_t   *p_pcb;
    uint8_t                    app_idx, mcl_idx, mdl_idx, mdep_cfg_idx;
    uint8_t                  status = FALSE;
    uint8_t                  close_dch = FALSE;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    // Find the correct app_idx based on the mdep_id;
    btif_hl_find_app_idx_using_mdepId(p_data->dch_open_cfm.local_mdep_id, &app_idx);

    if(btif_hl_find_mcl_idx_using_app_idx(p_data->dch_open_cfm.mcl_handle, app_idx, &mcl_idx)) {
        p_pcb = BTIF_HL_GET_PCB_PTR(app_idx, mcl_idx);

        if(btif_hl_find_avail_mdl_idx(app_idx, mcl_idx, &mdl_idx)) {
            p_dcb = BTIF_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);

            if(btif_hl_find_mdep_cfg_idx(app_idx, p_data->dch_open_cfm.local_mdep_id, &mdep_cfg_idx)) {
                p_dcb->in_use               = TRUE;
                p_dcb->mdl_handle           = p_data->dch_open_cfm.mdl_handle;
                p_dcb->local_mdep_cfg_idx   = mdep_cfg_idx;
                p_dcb->local_mdep_id        = p_data->dch_open_cfm.local_mdep_id;
                p_dcb->mdl_id               = p_data->dch_open_cfm.mdl_id;
                p_dcb->dch_mode             = p_data->dch_open_cfm.dch_mode;
                p_dcb->is_the_first_reliable = p_data->dch_open_cfm.first_reliable;
                p_dcb->mtu                  = p_data->dch_open_cfm.mtu;
                p_dcb->channel_id           = p_pcb->channel_id;
                BTIF_TRACE_DEBUG("app_idx=%d mcl_idx=%d mdl_idx=%d",  app_idx, mcl_idx, mdl_idx);
                btif_hl_send_setup_connecting_cb(app_idx, mcl_idx);

                if(btif_hl_create_socket(app_idx, mcl_idx, mdl_idx)) {
                    status = TRUE;
                    BTIF_TRACE_DEBUG("app_idx=%d mcl_idx=%d mdl_idx=%d p_dcb->channel_id=0x%08x",
                                     app_idx, mcl_idx, mdl_idx, p_dcb->channel_id);
                    btif_hl_clean_pcb(p_pcb);
                } else {
                    BTIF_TRACE_ERROR("Unable to create socket");
                    close_dch = TRUE;
                }
            } else {
                BTIF_TRACE_ERROR("INVALID_LOCAL_MDEP_ID mdep_id=%d", p_data->dch_open_cfm.local_mdep_id);
                close_dch = TRUE;
            }

            if(close_dch) {
                btif_hl_clean_mdl_cb(p_dcb);
                BTA_HlDchClose(p_data->dch_open_cfm.mdl_handle);
            }
        }
    }

    return status;
}
/*******************************************************************************
**
** Function         btif_hl_proc_dch_reconnect_cfm
**
** Description      Process the DCH reconnect indication
**
** Returns          Nothing
**
*******************************************************************************/
static uint8_t btif_hl_proc_dch_reconnect_cfm(tBTA_HL *p_data)
{
    btif_hl_mdl_cb_t            *p_dcb;
    btif_hl_pending_chan_cb_t   *p_pcb;
    uint8_t                    app_idx, mcl_idx, mdl_idx, mdep_cfg_idx;
    uint8_t                  status = FALSE;
    uint8_t                  close_dch = FALSE;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    btif_hl_find_app_idx_using_mdepId(p_data->dch_reconnect_cfm.local_mdep_id, &app_idx);

    if(btif_hl_find_mcl_idx_using_app_idx(p_data->dch_reconnect_cfm.mcl_handle, app_idx, &mcl_idx)) {
        p_pcb = BTIF_HL_GET_PCB_PTR(app_idx, mcl_idx);

        if(btif_hl_find_avail_mdl_idx(app_idx, mcl_idx, &mdl_idx)) {
            p_dcb = BTIF_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);

            if(btif_hl_find_mdep_cfg_idx(app_idx, p_data->dch_reconnect_cfm.local_mdep_id, &mdep_cfg_idx)) {
                p_dcb->in_use               = TRUE;
                p_dcb->mdl_handle           = p_data->dch_reconnect_cfm.mdl_handle;
                p_dcb->local_mdep_cfg_idx   = mdep_cfg_idx;
                p_dcb->local_mdep_id        = p_data->dch_reconnect_cfm.local_mdep_id;
                p_dcb->mdl_id               = p_data->dch_reconnect_cfm.mdl_id;
                p_dcb->dch_mode             = p_data->dch_reconnect_cfm.dch_mode;
                p_dcb->is_the_first_reliable = p_data->dch_reconnect_cfm.first_reliable;
                p_dcb->mtu                  = p_data->dch_reconnect_cfm.mtu;
                p_dcb->channel_id           = p_pcb->channel_id;
                BTIF_TRACE_DEBUG("app_idx=%d mcl_idx=%d mdl_idx=%d",  app_idx, mcl_idx, mdl_idx);
                btif_hl_send_setup_connecting_cb(app_idx, mcl_idx);

                if(btif_hl_create_socket(app_idx, mcl_idx, mdl_idx)) {
                    status = TRUE;
                    BTIF_TRACE_DEBUG("app_idx=%d mcl_idx=%d mdl_idx=%d p_dcb->channel_id=0x%08x",
                                     app_idx, mcl_idx, mdl_idx, p_dcb->channel_id);
                    btif_hl_clean_pcb(p_pcb);
                } else {
                    BTIF_TRACE_ERROR("Unable to create socket");
                    close_dch = TRUE;
                }
            } else {
                BTIF_TRACE_ERROR("INVALID_LOCAL_MDEP_ID mdep_id=%d", p_data->dch_open_cfm.local_mdep_id);
                close_dch = TRUE;
            }

            if(close_dch) {
                btif_hl_clean_mdl_cb(p_dcb);
                BTA_HlDchClose(p_data->dch_reconnect_cfm.mdl_handle);
            }
        }
    }

    return status;
}
/*******************************************************************************
**
** Function         btif_hl_proc_dch_reconnect_ind
**
** Description      Process the DCH reconnect indication
**
** Returns          Nothing
**
*******************************************************************************/
static void btif_hl_proc_dch_reconnect_ind(tBTA_HL *p_data)

{
    btif_hl_app_cb_t        *p_acb;
    btif_hl_mdl_cb_t        *p_dcb;
    uint8_t                   app_idx, mcl_idx, mdl_idx, mdep_cfg_idx;
    uint8_t                 close_dch = FALSE;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    // Find the correct app_idx based on the mdep_id;
    btif_hl_find_app_idx_using_mdepId(p_data->dch_reconnect_ind.local_mdep_id, &app_idx);

    if(btif_hl_find_mcl_idx_using_app_idx(p_data->dch_reconnect_ind.mcl_handle, app_idx, &mcl_idx)) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);
        BTIF_TRACE_DEBUG("btif_hl_proc_dch_reconnect_ind: app_idx = %d, mcl_idx = %d",
                         app_idx, mcl_idx);

        if(btif_hl_find_avail_mdl_idx(app_idx, mcl_idx, &mdl_idx)) {
            p_dcb = BTIF_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);

            if(btif_hl_find_mdep_cfg_idx(app_idx, p_data->dch_reconnect_ind.local_mdep_id, &mdep_cfg_idx)) {
                p_dcb->in_use               = TRUE;
                p_dcb->mdl_handle           = p_data->dch_reconnect_ind.mdl_handle;
                p_dcb->local_mdep_cfg_idx   = mdep_cfg_idx;
                p_dcb->local_mdep_id        = p_data->dch_reconnect_ind.local_mdep_id;
                p_dcb->mdl_id               = p_data->dch_reconnect_ind.mdl_id;
                p_dcb->dch_mode             = p_data->dch_reconnect_ind.dch_mode;
                p_dcb->dch_mode             = p_data->dch_reconnect_ind.dch_mode;
                p_dcb->is_the_first_reliable = p_data->dch_reconnect_ind.first_reliable;
                p_dcb->mtu                  = p_data->dch_reconnect_ind.mtu;
                p_dcb->channel_id           = btif_hl_get_next_channel_id(p_acb->app_id);
                BTIF_TRACE_DEBUG(" app_idx=%d mcl_idx=%d mdl_idx=%d channel_id=%d",
                                 app_idx, mcl_idx, mdl_idx, p_dcb->channel_id);

                if(!btif_hl_create_socket(app_idx, mcl_idx, mdl_idx)) {
                    BTIF_TRACE_ERROR("Unable to create socket");
                    close_dch = TRUE;
                }
            } else {
                BTIF_TRACE_ERROR("INVALID_LOCAL_MDEP_ID mdep_id=%d", p_data->dch_open_cfm.local_mdep_id);
                close_dch = TRUE;
            }

            if(close_dch) {
                btif_hl_clean_mdl_cb(p_dcb);
            }
        } else {
            close_dch = TRUE;
        }
    } else {
        close_dch = TRUE;
    }

    if(close_dch) {
        BTA_HlDchClose(p_data->dch_reconnect_ind.mdl_handle);
    }
}

/*******************************************************************************
**
** Function         btif_hl_proc_dch_close_ind
**
** Description      Process the DCH close indication
**
** Returns          Nothing
**
*******************************************************************************/
static void btif_hl_proc_dch_close_ind(tBTA_HL *p_data)

{
    btif_hl_mdl_cb_t         *p_dcb;
    btif_hl_mcl_cb_t         *p_mcb;
    uint8_t                   app_idx, mcl_idx, mdl_idx;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    if(btif_hl_find_mdl_idx_using_handle(p_data->dch_close_ind.mdl_handle,
                                         &app_idx, &mcl_idx, &mdl_idx)) {
        p_dcb = BTIF_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
        btif_hl_release_socket(app_idx, mcl_idx, mdl_idx);
        btif_hl_send_setup_disconnected_cb(app_idx, mcl_idx);
        p_mcb =  BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
        btif_hl_clean_mdl_cb(p_dcb);

        if(!btif_hl_num_dchs_in_use(p_mcb->mcl_handle)) {
            btif_hl_start_cch_timer(app_idx, mcl_idx);
        }

        BTIF_TRACE_DEBUG("remote DCH close success mdl_idx=%d", mdl_idx);
    }
}

/*******************************************************************************
**
** Function         btif_hl_proc_dch_close_cfm
**
** Description      Process the DCH reconnect confirmation
**
** Returns          Nothing
**
*******************************************************************************/
static void btif_hl_proc_dch_close_cfm(tBTA_HL *p_data)

{
    btif_hl_mdl_cb_t         *p_dcb;
    btif_hl_mcl_cb_t         *p_mcb;
    uint8_t                   app_idx, mcl_idx, mdl_idx;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    if(btif_hl_find_mdl_idx_using_handle(p_data->dch_close_cfm.mdl_handle,
                                         &app_idx, &mcl_idx, &mdl_idx)) {
        p_dcb = BTIF_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
        btif_hl_release_socket(app_idx, mcl_idx, mdl_idx);
        btif_hl_clean_mdl_cb(p_dcb);
        p_mcb =  BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

        if(!btif_hl_num_dchs_in_use(p_mcb->mcl_handle)) {
            btif_hl_start_cch_timer(app_idx, mcl_idx);
        }

        BTIF_TRACE_DEBUG(" local DCH close success mdl_idx=%d", mdl_idx);
    }
}

/*******************************************************************************
**
** Function         btif_hl_proc_abort_ind
**
** Description      Process the abort indicaiton
**
** Returns          Nothing
**
*******************************************************************************/
static void btif_hl_proc_abort_ind(tBTA_HL_MCL_HANDLE mcl_handle)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    btif_hl_app_cb_t  *p_acb;
    uint8_t i, j;

    for(i = 0; i < BTA_HL_NUM_APPS; i++) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(i);

        for(j = 0; j < BTA_HL_NUM_MCLS ; j++) {
            if(p_acb->mcb[j].in_use) {
                BTIF_TRACE_DEBUG("btif_hl_find_mcl_idx_using_handle: app_idx=%d,mcl_idx =%d mcl_handle=%d", i, j,
                                 p_acb->mcb[j].mcl_handle);
            }

            if(p_acb->mcb[j].in_use &&
                    (p_acb->mcb[j].mcl_handle == mcl_handle)) {
                btif_hl_stop_cch_timer(i, j);
                btif_hl_send_setup_disconnected_cb(i, j);
                btif_hl_clean_mcl_cb(i, j);
            }
        }
    }
}

/*******************************************************************************
**
** Function         btif_hl_proc_abort_cfm
**
** Description      Process the abort confirmation
**
** Returns          Nothing
**
*******************************************************************************/
static void btif_hl_proc_abort_cfm(tBTA_HL_MCL_HANDLE mcl_handle)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    btif_hl_app_cb_t  *p_acb;
    uint8_t i, j;

    for(i = 0; i < BTA_HL_NUM_APPS; i++) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(i);

        for(j = 0; j < BTA_HL_NUM_MCLS ; j++) {
            if(p_acb->mcb[j].in_use) {
                BTIF_TRACE_DEBUG("btif_hl_find_mcl_idx_using_handle: app_idx=%d,mcl_idx =%d mcl_handle=%d", i, j,
                                 p_acb->mcb[j].mcl_handle);
            }

            if(p_acb->mcb[j].in_use &&
                    (p_acb->mcb[j].mcl_handle == mcl_handle)) {
                btif_hl_stop_cch_timer(i, j);
                btif_hl_send_setup_disconnected_cb(i, j);
                btif_hl_clean_mcl_cb(i, j);
            }
        }
    }
}

/*******************************************************************************
**
** Function         btif_hl_proc_send_data_cfm
**
** Description      Process the send data confirmation
**
** Returns          Nothing
**
*******************************************************************************/
static void btif_hl_proc_send_data_cfm(tBTA_HL_MDL_HANDLE mdl_handle,
                                       tBTA_HL_STATUS status)
{
    uint8_t                   app_idx, mcl_idx, mdl_idx;
    btif_hl_mdl_cb_t         *p_dcb;
    UNUSED(status);
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    if(btif_hl_find_mdl_idx_using_handle(mdl_handle,
                                         &app_idx, &mcl_idx, &mdl_idx)) {
        p_dcb = BTIF_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
        GKI_free_and_reset_buf((void **)&p_dcb->p_tx_pkt);
        BTIF_TRACE_DEBUG("send success free p_tx_pkt tx_size=%d",
                         p_dcb->tx_size);
        p_dcb->tx_size = 0;
    }
}

/*******************************************************************************
**
** Function         btif_hl_proc_dch_cong_ind
**
** Description      Process the DCH congestion change indication
**
** Returns          Nothing
**
*******************************************************************************/
static void btif_hl_proc_dch_cong_ind(tBTA_HL *p_data)

{
    btif_hl_mdl_cb_t         *p_dcb;
    uint8_t                   app_idx, mcl_idx, mdl_idx;
    BTIF_TRACE_DEBUG("btif_hl_proc_dch_cong_ind");

    if(btif_hl_find_mdl_idx_using_handle(p_data->dch_cong_ind.mdl_handle, &app_idx, &mcl_idx,
                                         &mdl_idx)) {
        p_dcb = BTIF_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
        p_dcb->cong = p_data->dch_cong_ind.cong;
    }
}

/*******************************************************************************
**
** Function         btif_hl_proc_reg_request
**
** Description      Process registration request
**
** Returns          void
**
*******************************************************************************/
static void btif_hl_proc_reg_request(uint8_t app_idx, uint8_t  app_id,
                                     tBTA_HL_REG_PARAM *p_reg_param,
                                     tBTA_HL_CBACK *p_cback)
{
    UNUSED(p_cback);
    BTIF_TRACE_DEBUG("%s app_idx=%d app_id=%d", __FUNCTION__, app_idx, app_id);

    if(reg_counter > 1) {
        BTIF_TRACE_DEBUG("btif_hl_proc_reg_request: calling uPDATE");
        BTA_HlUpdate(app_id, p_reg_param, TRUE, btif_hl_cback);
    } else {
        BTA_HlRegister(app_id, p_reg_param, btif_hl_cback);
    }
}

/*******************************************************************************
**
** Function         btif_hl_proc_cb_evt
**
** Description      Process HL callback events
**
** Returns          void
**
*******************************************************************************/
static void btif_hl_proc_cb_evt(uint16_t event, char *p_param)
{
    btif_hl_evt_cb_t                *p_data = (btif_hl_evt_cb_t *)p_param;
    tls_bt_addr_t                     bd_addr;
    bthl_channel_state_t            state = BTHL_CONN_STATE_DISCONNECTED;
    uint8_t                         send_chan_cb = TRUE;
    tBTA_HL_REG_PARAM               reg_param;
    btif_hl_app_cb_t                *p_acb;
    BTIF_TRACE_DEBUG("%s event %d", __FUNCTION__, event);
    btif_hl_display_calling_process_name();

    switch(event) {
        case BTIF_HL_SEND_CONNECTED_CB:
        case BTIF_HL_SEND_DISCONNECTED_CB:
            if(p_data->chan_cb.cb_state == BTIF_HL_CHAN_CB_STATE_CONNECTED_PENDING) {
                state = BTHL_CONN_STATE_CONNECTED;
            } else if(p_data->chan_cb.cb_state == BTIF_HL_CHAN_CB_STATE_DISCONNECTED_PENDING) {
                state = BTHL_CONN_STATE_DISCONNECTED;
            } else {
                send_chan_cb = FALSE;
            }

            if(send_chan_cb) {
                btif_hl_copy_bda(&bd_addr, p_data->chan_cb.bd_addr);
                BTIF_TRACE_DEBUG("state callbk: ch_id=0x%08x cb_state=%d state=%d  fd=%d",
                                 p_data->chan_cb.channel_id,
                                 p_data->chan_cb.cb_state,
                                 state,  p_data->chan_cb.fd);
                btif_hl_display_bt_bda(&bd_addr);
                BTIF_HL_CALL_CBACK(bt_hl_callbacks, channel_state_cb,  p_data->chan_cb.app_id,
                                   &bd_addr, p_data->chan_cb.mdep_cfg_index,
                                   p_data->chan_cb.channel_id, state, p_data->chan_cb.fd);
            }

            break;

        case BTIF_HL_REG_APP:
            p_acb  = BTIF_HL_GET_APP_CB_PTR(p_data->reg.app_idx);
            BTIF_TRACE_DEBUG("Rcv BTIF_HL_REG_APP app_idx=%d reg_pending=%d", p_data->reg.app_idx,
                             p_acb->reg_pending);

            if(btif_hl_get_state() == BTIF_HL_STATE_ENABLED && p_acb->reg_pending) {
                BTIF_TRACE_DEBUG("Rcv BTIF_HL_REG_APP reg_counter=%d", reg_counter);
                p_acb->reg_pending = FALSE;
                reg_param.dev_type = p_acb->dev_type;
                reg_param.sec_mask = BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT;
                reg_param.p_srv_name = p_acb->srv_name;
                reg_param.p_srv_desp = p_acb->srv_desp;
                reg_param.p_provider_name = p_acb->provider_name;
                btif_hl_proc_reg_request(p_data->reg.app_idx, p_acb->app_id, &reg_param, btif_hl_cback);
            } else {
                BTIF_TRACE_DEBUG("reg request is processed state=%d reg_pending=%d", btif_hl_get_state(),
                                 p_acb->reg_pending);
            }

            break;

        case BTIF_HL_UNREG_APP:
            BTIF_TRACE_DEBUG("Rcv BTIF_HL_UNREG_APP app_idx=%d", p_data->unreg.app_idx);
            p_acb = BTIF_HL_GET_APP_CB_PTR(p_data->unreg.app_idx);

            if(btif_hl_get_state() == BTIF_HL_STATE_ENABLED) {
                if(reg_counter >= 1) {
                    BTA_HlUpdate(p_acb->app_id, NULL, FALSE, NULL);
                } else {
                    BTA_HlDeregister(p_acb->app_id, p_acb->app_handle);
                }
            }

            break;

        case BTIF_HL_UPDATE_MDL:
            BTIF_TRACE_DEBUG("Rcv BTIF_HL_UPDATE_MDL app_idx=%d", p_data->update_mdl.app_idx);
            p_acb = BTIF_HL_GET_APP_CB_PTR(p_data->update_mdl.app_idx);
            break;

        default:
            BTIF_TRACE_ERROR("Unknown event %d", event);
            break;
    }
}

/*******************************************************************************
**
** Function         btif_hl_upstreams_evt
**
** Description      Process HL events
**
** Returns          void
**
*******************************************************************************/
static void btif_hl_upstreams_evt(uint16_t event, char *p_param)
{
    tBTA_HL *p_data = (tBTA_HL *)p_param;
    uint8_t                 app_idx, mcl_idx;
    btif_hl_app_cb_t      *p_acb;
    btif_hl_mcl_cb_t      *p_mcb = NULL;
    btif_hl_pend_dch_op_t  pending_op;
    uint8_t status;
    BTIF_TRACE_DEBUG("%s event %d", __FUNCTION__, event);
    btif_hl_display_calling_process_name();

    switch(event) {
        case BTA_HL_REGISTER_CFM_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_REGISTER_CFM_EVT");
            BTIF_TRACE_DEBUG("app_id=%d app_handle=%d status=%d ",
                             p_data->reg_cfm.app_id,
                             p_data->reg_cfm.app_handle,
                             p_data->reg_cfm.status);
            btif_hl_proc_reg_cfm(p_data);
            break;

        case BTA_HL_SDP_INFO_IND_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_SDP_INFO_IND_EVT");
            BTIF_TRACE_DEBUG("app_handle=%d ctrl_psm=0x%04x data_psm=0x%04x x_spec=%d mcap_sup_procs=0x%02x",
                             p_data->sdp_info_ind.app_handle,
                             p_data->sdp_info_ind.ctrl_psm,
                             p_data->sdp_info_ind.data_psm,
                             p_data->sdp_info_ind.data_x_spec,
                             p_data->sdp_info_ind.mcap_sup_procs);
            //btif_hl_proc_sdp_info_ind(p_data);
            break;

        case BTA_HL_DEREGISTER_CFM_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_DEREGISTER_CFM_EVT");
            BTIF_TRACE_DEBUG("app_handle=%d status=%d ",
                             p_data->dereg_cfm.app_handle,
                             p_data->dereg_cfm.status);
            btif_hl_proc_dereg_cfm(p_data);
            break;

        case BTA_HL_SDP_QUERY_CFM_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_SDP_QUERY_CFM_EVT");
            BTIF_TRACE_DEBUG("app_handle=%d app_id =%d,status =%d",
                             p_data->sdp_query_cfm.app_handle, p_data->sdp_query_cfm.app_id,
                             p_data->sdp_query_cfm.status);
            BTIF_TRACE_DEBUG("DB [%02x] [%02x] [%02x] [%02x] [%02x] [%02x]",
                             p_data->sdp_query_cfm.bd_addr[0], p_data->sdp_query_cfm.bd_addr[1],
                             p_data->sdp_query_cfm.bd_addr[2], p_data->sdp_query_cfm.bd_addr[3],
                             p_data->sdp_query_cfm.bd_addr[4], p_data->sdp_query_cfm.bd_addr[5]);

            if(p_data->sdp_query_cfm.status == BTA_HL_STATUS_OK) {
                status = btif_hl_proc_sdp_query_cfm(p_data);
            } else {
                status = FALSE;
            }

            if(!status) {
                BTIF_TRACE_DEBUG("BTA_HL_SDP_QUERY_CFM_EVT Status = %d",
                                 p_data->sdp_query_cfm.status);

                if(btif_hl_find_app_idx_using_app_id(p_data->sdp_query_cfm.app_id, &app_idx)) {
                    p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);

                    if(btif_hl_find_mcl_idx(app_idx, p_data->sdp_query_cfm.bd_addr, &mcl_idx)) {
                        p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

                        if((p_mcb->cch_oper ==  BTIF_HL_CCH_OP_MDEP_FILTERING) ||
                                (p_mcb->cch_oper == BTIF_HL_CCH_OP_DCH_OPEN)) {
                            pending_op = p_mcb->pcb.op;

                            switch(pending_op) {
                                case BTIF_HL_PEND_DCH_OP_OPEN:
                                    btif_hl_send_setup_disconnected_cb(app_idx, mcl_idx);
                                    break;

                                case BTIF_HL_PEND_DCH_OP_RECONNECT:
                                case BTIF_HL_PEND_DCH_OP_DELETE_MDL:
                                default:
                                    break;
                            }

                            if(!p_mcb->is_connected) {
                                btif_hl_clean_mcl_cb(app_idx, mcl_idx);
                            }
                        }
                    }
                }
            }

            break;

        case BTA_HL_CCH_OPEN_CFM_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_CCH_OPEN_CFM_EVT");
            BTIF_TRACE_DEBUG("app_id=%d,app_handle=%d mcl_handle=%d status =%d",
                             p_data->cch_open_cfm.app_id,
                             p_data->cch_open_cfm.app_handle,
                             p_data->cch_open_cfm.mcl_handle,
                             p_data->cch_open_cfm.status);
            BTIF_TRACE_DEBUG("DB [%02x] [%02x] [%02x] [%02x] [%02x] [%02x]",
                             p_data->cch_open_cfm.bd_addr[0], p_data->cch_open_cfm.bd_addr[1],
                             p_data->cch_open_cfm.bd_addr[2], p_data->cch_open_cfm.bd_addr[3],
                             p_data->cch_open_cfm.bd_addr[4], p_data->cch_open_cfm.bd_addr[5]);

            if(p_data->cch_open_cfm.status == BTA_HL_STATUS_OK ||
                    p_data->cch_open_cfm.status == BTA_HL_STATUS_DUPLICATE_CCH_OPEN) {
                status = btif_hl_proc_cch_open_cfm(p_data);
            } else {
                status = FALSE;
            }

            if(!status) {
                if(btif_hl_find_app_idx_using_app_id(p_data->cch_open_cfm.app_id, &app_idx)) {
                    p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);

                    if(btif_hl_find_mcl_idx(app_idx, p_data->cch_open_cfm.bd_addr, &mcl_idx)) {
                        p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
                        pending_op = p_mcb->pcb.op;

                        switch(pending_op) {
                            case BTIF_HL_PEND_DCH_OP_OPEN:
                                btif_hl_send_setup_disconnected_cb(app_idx, mcl_idx);
                                break;

                            case BTIF_HL_PEND_DCH_OP_RECONNECT:
                            case BTIF_HL_PEND_DCH_OP_DELETE_MDL:
                            default:
                                break;
                        }

                        btif_hl_clean_mcl_cb(app_idx, mcl_idx);
                    }
                }
            }

            break;

        case BTA_HL_DCH_OPEN_CFM_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_DCH_OPEN_CFM_EVT");
            BTIF_TRACE_DEBUG("mcl_handle=%d mdl_handle=0x%x status=%d ",
                             p_data->dch_open_cfm.mcl_handle,
                             p_data->dch_open_cfm.mdl_handle,
                             p_data->dch_open_cfm.status);
            BTIF_TRACE_DEBUG("first_reliable =%d dch_mode=%d local_mdep_id=%d mdl_id=%d mtu=%d",
                             p_data->dch_open_cfm.first_reliable,
                             p_data->dch_open_cfm.dch_mode,
                             p_data->dch_open_cfm.local_mdep_id,
                             p_data->dch_open_cfm.mdl_id,
                             p_data->dch_open_cfm.mtu);

            if(p_data->dch_open_cfm.status == BTA_HL_STATUS_OK) {
                status = btif_hl_proc_dch_open_cfm(p_data);
            } else {
                status = FALSE;
            }

            if(!status) {
                if(btif_hl_find_mcl_idx_using_handle(p_data->dch_open_cfm.mcl_handle, &app_idx, &mcl_idx)) {
                    p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
                    pending_op = p_mcb->pcb.op;

                    switch(pending_op) {
                        case BTIF_HL_PEND_DCH_OP_OPEN:
                            btif_hl_send_setup_disconnected_cb(app_idx, mcl_idx);
                            break;

                        case BTIF_HL_PEND_DCH_OP_RECONNECT:
                        case BTIF_HL_PEND_DCH_OP_DELETE_MDL:
                        default:
                            break;
                    }
                }
            }

            break;

        case BTA_HL_CCH_OPEN_IND_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_CCH_OPEN_IND_EVT");
            BTIF_TRACE_DEBUG("app_handle=%d mcl_handle=%d",
                             p_data->cch_open_ind.app_handle,
                             p_data->cch_open_ind.mcl_handle);
            BTIF_TRACE_DEBUG("DB [%02x] [%02x] [%02x] [%02x] [%02x] [%02x]",
                             p_data->cch_open_ind.bd_addr[0], p_data->cch_open_ind.bd_addr[1],
                             p_data->cch_open_ind.bd_addr[2], p_data->cch_open_ind.bd_addr[3],
                             p_data->cch_open_ind.bd_addr[4], p_data->cch_open_ind.bd_addr[5]);
            btif_hl_proc_cch_open_ind(p_data);
            break;

        case BTA_HL_DCH_CREATE_IND_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_DCH_CREATE_IND_EVT");
            BTIF_TRACE_DEBUG("mcl_handle=%d",
                             p_data->dch_create_ind.mcl_handle);
            BTIF_TRACE_DEBUG("local_mdep_id =%d mdl_id=%d cfg=%d",
                             p_data->dch_create_ind.local_mdep_id,
                             p_data->dch_create_ind.mdl_id,
                             p_data->dch_create_ind.cfg);
            btif_hl_proc_create_ind(p_data);
            break;

        case BTA_HL_DCH_OPEN_IND_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_DCH_OPEN_IND_EVT");
            BTIF_TRACE_DEBUG("mcl_handle=%d mdl_handle=0x%x",
                             p_data->dch_open_ind.mcl_handle,
                             p_data->dch_open_ind.mdl_handle);
            BTIF_TRACE_DEBUG("first_reliable =%d dch_mode=%d local_mdep_id=%d mdl_id=%d mtu=%d",
                             p_data->dch_open_ind.first_reliable,
                             p_data->dch_open_ind.dch_mode,
                             p_data->dch_open_ind.local_mdep_id,
                             p_data->dch_open_ind.mdl_id,
                             p_data->dch_open_ind.mtu);
            btif_hl_proc_dch_open_ind(p_data);
            break;

        case BTA_HL_DELETE_MDL_IND_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_DELETE_MDL_IND_EVT");
            BTIF_TRACE_DEBUG("mcl_handle=%d mdl_id=0x%x",
                             p_data->delete_mdl_ind.mcl_handle,
                             p_data->delete_mdl_ind.mdl_id);
            break;

        case BTA_HL_DELETE_MDL_CFM_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_DELETE_MDL_CFM_EVT");
            BTIF_TRACE_DEBUG("mcl_handle=%d mdl_id=0x%x status=%d",
                             p_data->delete_mdl_cfm.mcl_handle,
                             p_data->delete_mdl_cfm.mdl_id,
                             p_data->delete_mdl_cfm.status);

            if(btif_hl_find_app_idx_using_deleted_mdl_id(p_data->delete_mdl_cfm.mdl_id,
                    &app_idx)) {
                p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);
                btif_hl_send_destroyed_cb(p_acb);
                btif_hl_clean_delete_mdl(&p_acb->delete_mdl);
            }

            break;

        case BTA_HL_DCH_RECONNECT_CFM_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_DCH_RECONNECT_CFM_EVT");
            BTIF_TRACE_DEBUG("mcl_handle=%d mdl_handle=%d status=%d   ",
                             p_data->dch_reconnect_cfm.mcl_handle,
                             p_data->dch_reconnect_cfm.mdl_handle,
                             p_data->dch_reconnect_cfm.status);
            BTIF_TRACE_DEBUG("first_reliable =%d dch_mode=%d mdl_id=%d mtu=%d",
                             p_data->dch_reconnect_cfm.first_reliable,
                             p_data->dch_reconnect_cfm.dch_mode,
                             p_data->dch_reconnect_cfm.mdl_id,
                             p_data->dch_reconnect_cfm.mtu);

            if(p_data->dch_reconnect_cfm.status == BTA_HL_STATUS_OK) {
                status = btif_hl_proc_dch_reconnect_cfm(p_data);
            } else {
                status = FALSE;
            }

            if(!status) {
                if(btif_hl_find_mcl_idx_using_handle(p_data->dch_open_cfm.mcl_handle, &app_idx, &mcl_idx)) {
                    p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
                    pending_op = p_mcb->pcb.op;

                    switch(pending_op) {
                        case BTIF_HL_PEND_DCH_OP_OPEN:
                            btif_hl_send_setup_disconnected_cb(app_idx, mcl_idx);
                            break;

                        case BTIF_HL_PEND_DCH_OP_RECONNECT:
                        case BTIF_HL_PEND_DCH_OP_DELETE_MDL:
                        default:
                            break;
                    }
                }
            }

            break;

        case BTA_HL_CCH_CLOSE_CFM_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_CCH_CLOSE_CFM_EVT");
            BTIF_TRACE_DEBUG("mcl_handle=%d status =%d",
                             p_data->cch_close_cfm.mcl_handle,
                             p_data->cch_close_cfm.status);

            if(p_data->cch_close_cfm.status == BTA_HL_STATUS_OK) {
                btif_hl_proc_cch_close_cfm(p_data);
            }

            break;

        case BTA_HL_CCH_CLOSE_IND_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_CCH_CLOSE_IND_EVT");
            BTIF_TRACE_DEBUG("mcl_handle =%d intentional_close=%s",
                             p_data->cch_close_ind.mcl_handle,
                             (p_data->cch_close_ind.intentional ? "Yes" : "No"));
            btif_hl_proc_cch_close_ind(p_data);
            break;

        case BTA_HL_DCH_CLOSE_IND_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_DCH_CLOSE_IND_EVT");
            BTIF_TRACE_DEBUG("mdl_handle=%d intentional_close=%s",
                             p_data->dch_close_ind.mdl_handle,
                             (p_data->dch_close_ind.intentional ? "Yes" : "No"));
            btif_hl_proc_dch_close_ind(p_data);
            break;

        case BTA_HL_DCH_CLOSE_CFM_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_DCH_CLOSE_CFM_EVT");
            BTIF_TRACE_DEBUG("mdl_handle=%d status=%d ",
                             p_data->dch_close_cfm.mdl_handle,
                             p_data->dch_close_cfm.status);

            if(p_data->dch_close_cfm.status == BTA_HL_STATUS_OK) {
                btif_hl_proc_dch_close_cfm(p_data);
            }

            break;

        case BTA_HL_DCH_ECHO_TEST_CFM_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_DCH_ECHO_TEST_CFM_EVT");
            BTIF_TRACE_DEBUG("mcl_handle=%d    status=%d",
                             p_data->echo_test_cfm.mcl_handle,
                             p_data->echo_test_cfm.status);
            /* not supported */
            break;

        case BTA_HL_DCH_RECONNECT_IND_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_DCH_RECONNECT_IND_EVT");
            BTIF_TRACE_DEBUG("mcl_handle=%d mdl_handle=5d",
                             p_data->dch_reconnect_ind.mcl_handle,
                             p_data->dch_reconnect_ind.mdl_handle);
            BTIF_TRACE_DEBUG("first_reliable =%d dch_mode=%d mdl_id=%d mtu=%d",
                             p_data->dch_reconnect_ind.first_reliable,
                             p_data->dch_reconnect_ind.dch_mode,
                             p_data->dch_reconnect_ind.mdl_id,
                             p_data->dch_reconnect_ind.mtu);
            btif_hl_proc_dch_reconnect_ind(p_data);
            break;

        case BTA_HL_CONG_CHG_IND_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_CONG_CHG_IND_EVT");
            BTIF_TRACE_DEBUG("mdl_handle=%d cong =%d",
                             p_data->dch_cong_ind.mdl_handle,
                             p_data->dch_cong_ind.cong);
            btif_hl_proc_dch_cong_ind(p_data);
            break;

        case BTA_HL_DCH_ABORT_IND_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_DCH_ABORT_IND_EVT");
            BTIF_TRACE_DEBUG("mcl_handle=%d",
                             p_data->dch_abort_ind.mcl_handle);
            btif_hl_proc_abort_ind(p_data->dch_abort_ind.mcl_handle);
            break;

        case BTA_HL_DCH_ABORT_CFM_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_DCH_ABORT_CFM_EVT");
            BTIF_TRACE_DEBUG("mcl_handle=%d status =%d",
                             p_data->dch_abort_cfm.mcl_handle,
                             p_data->dch_abort_cfm.status);

            if(p_data->dch_abort_cfm.status == BTA_HL_STATUS_OK) {
                btif_hl_proc_abort_cfm(p_data->dch_abort_ind.mcl_handle);
            }

            break;

        case BTA_HL_DCH_SEND_DATA_CFM_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_DCH_SEND_DATA_CFM_EVT");
            BTIF_TRACE_DEBUG("mdl_handle=0x%x status =%d",
                             p_data->dch_send_data_cfm.mdl_handle,
                             p_data->dch_send_data_cfm.status);
            btif_hl_proc_send_data_cfm(p_data->dch_send_data_cfm.mdl_handle,
                                       p_data->dch_send_data_cfm.status);
            break;

        case BTA_HL_DCH_RCV_DATA_IND_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_DCH_RCV_DATA_IND_EVT");
            BTIF_TRACE_DEBUG("mdl_handle=0x%x ",
                             p_data->dch_rcv_data_ind.mdl_handle);
            /* do nothing here */
            break;

        default:
            BTIF_TRACE_DEBUG("Unknown Event (0x%02x)...", event);
            break;
    }
}

/*******************************************************************************
**
** Function         btif_hl_cback
**
** Description      Callback function for HL events
**
** Returns          void
**
*******************************************************************************/
static void btif_hl_cback(tBTA_HL_EVT event, tBTA_HL *p_data)
{
    tls_bt_status_t status;
    int param_len = 0;
    BTIF_TRACE_DEBUG("%s event %d", __FUNCTION__, event);
    btif_hl_display_calling_process_name();

    switch(event) {
        case BTA_HL_REGISTER_CFM_EVT:
            param_len = sizeof(tBTA_HL_REGISTER_CFM);
            break;

        case BTA_HL_SDP_INFO_IND_EVT:
            param_len = sizeof(tBTA_HL_SDP_INFO_IND);
            break;

        case BTA_HL_DEREGISTER_CFM_EVT:
            param_len = sizeof(tBTA_HL_DEREGISTER_CFM);
            break;

        case BTA_HL_SDP_QUERY_CFM_EVT:
            param_len = sizeof(tBTA_HL_SDP_QUERY_CFM);
            break;

        case BTA_HL_CCH_OPEN_CFM_EVT:
            param_len = sizeof(tBTA_HL_CCH_OPEN_CFM);
            break;

        case BTA_HL_DCH_OPEN_CFM_EVT:
            param_len = sizeof(tBTA_HL_DCH_OPEN_CFM);
            break;

        case BTA_HL_CCH_OPEN_IND_EVT:
            param_len = sizeof(tBTA_HL_CCH_OPEN_IND);
            break;

        case BTA_HL_DCH_CREATE_IND_EVT:
            param_len = sizeof(tBTA_HL_DCH_CREATE_IND);
            break;

        case BTA_HL_DCH_OPEN_IND_EVT:
            param_len = sizeof(tBTA_HL_DCH_OPEN_IND);
            break;

        case BTA_HL_DELETE_MDL_IND_EVT:
            param_len = sizeof(tBTA_HL_MDL_IND);
            break;

        case BTA_HL_DELETE_MDL_CFM_EVT:
            param_len = sizeof(tBTA_HL_MDL_CFM);
            break;

        case BTA_HL_DCH_RECONNECT_CFM_EVT:
            param_len = sizeof(tBTA_HL_DCH_OPEN_CFM);
            break;

        case BTA_HL_CCH_CLOSE_CFM_EVT:
            param_len = sizeof(tBTA_HL_MCL_CFM);
            break;

        case BTA_HL_CCH_CLOSE_IND_EVT:
            param_len = sizeof(tBTA_HL_CCH_CLOSE_IND);
            break;

        case BTA_HL_DCH_CLOSE_IND_EVT:
            param_len = sizeof(tBTA_HL_DCH_CLOSE_IND);
            break;

        case BTA_HL_DCH_CLOSE_CFM_EVT:
            param_len = sizeof(tBTA_HL_MDL_CFM);
            break;

        case BTA_HL_DCH_ECHO_TEST_CFM_EVT:
            param_len = sizeof(tBTA_HL_MCL_CFM);
            break;

        case BTA_HL_DCH_RECONNECT_IND_EVT:
            param_len = sizeof(tBTA_HL_DCH_OPEN_IND);
            break;

        case BTA_HL_CONG_CHG_IND_EVT:
            param_len = sizeof(tBTA_HL_DCH_CONG_IND);
            break;

        case BTA_HL_DCH_ABORT_IND_EVT:
            param_len = sizeof(tBTA_HL_MCL_IND);
            break;

        case BTA_HL_DCH_ABORT_CFM_EVT:
            param_len = sizeof(tBTA_HL_MCL_CFM);
            break;

        case BTA_HL_DCH_SEND_DATA_CFM_EVT:
            param_len = sizeof(tBTA_HL_MDL_CFM);
            break;

        case BTA_HL_DCH_RCV_DATA_IND_EVT:
            param_len = sizeof(tBTA_HL_MDL_IND);
            break;

        default:
            param_len = sizeof(tBTA_HL_MDL_IND);
            break;
    }

    status = btif_transfer_context(btif_hl_upstreams_evt, (uint16_t)event, (void *)p_data, param_len,
                                   NULL);
    /* catch any failed context transfers */
    ASSERTC(status == TLS_BT_STATUS_SUCCESS, "context transfer failed", status);
}

/*******************************************************************************
**
** Function         btif_hl_upstreams_ctrl_evt
**
** Description      Callback function for HL control events in the BTIF task context
**
** Returns          void
**
*******************************************************************************/
static void btif_hl_upstreams_ctrl_evt(uint16_t event, char *p_param)
{
    tBTA_HL_CTRL *p_data = (tBTA_HL_CTRL *) p_param;
    uint8_t               i;
    tBTA_HL_REG_PARAM   reg_param;
    btif_hl_app_cb_t    *p_acb;
    BTIF_TRACE_DEBUG("%s event %d", __FUNCTION__, event);
    btif_hl_display_calling_process_name();

    switch(event) {
        case BTA_HL_CTRL_ENABLE_CFM_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_CTRL_ENABLE_CFM_EVT");
            BTIF_TRACE_DEBUG("status=%d", p_data->enable_cfm.status);

            if(p_data->enable_cfm.status == BTA_HL_STATUS_OK) {
                btif_hl_set_state(BTIF_HL_STATE_ENABLED);

                for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
                    p_acb = BTIF_HL_GET_APP_CB_PTR(i);

                    if(p_acb->in_use && p_acb->reg_pending) {
                        p_acb->reg_pending = FALSE;
                        reg_param.dev_type = p_acb->dev_type;
                        reg_param.sec_mask = BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT;
                        reg_param.p_srv_name = p_acb->srv_name;
                        reg_param.p_srv_desp = p_acb->srv_desp;
                        reg_param.p_provider_name = p_acb->provider_name;
                        BTIF_TRACE_DEBUG("Register pending app_id=%d", p_acb->app_id);
                        btif_hl_proc_reg_request(i, p_acb->app_id, &reg_param, btif_hl_cback);
                    }
                }
            }

            break;

        case BTA_HL_CTRL_DISABLE_CFM_EVT:
            BTIF_TRACE_DEBUG("Rcv BTA_HL_CTRL_DISABLE_CFM_EVT");
            BTIF_TRACE_DEBUG("status=%d",
                             p_data->disable_cfm.status);

            if(p_data->disable_cfm.status == BTA_HL_STATUS_OK) {
                for(size_t i = 0; i < BTA_HL_NUM_APPS; i++) {
                    for(size_t j = 0; j < BTA_HL_NUM_MCLS; j++) {
                        alarm_free(p_btif_hl_cb->acb[i].mcb[j].cch_timer);
                    }
                }

                wm_memset(p_btif_hl_cb, 0, sizeof(btif_hl_cb_t));
                btif_hl_set_state(BTIF_HL_STATE_DISABLED);
            }

            break;

        default:
            break;
    }
}

/*******************************************************************************
**
** Function         btif_hl_ctrl_cback
**
** Description      Callback function for HL control events
**
** Returns          void
**
*******************************************************************************/
static void btif_hl_ctrl_cback(tBTA_HL_CTRL_EVT event, tBTA_HL_CTRL *p_data)
{
    tls_bt_status_t status;
    int param_len = 0;
    BTIF_TRACE_DEBUG("%s event %d", __FUNCTION__, event);
    btif_hl_display_calling_process_name();

    switch(event) {
        case BTA_HL_CTRL_ENABLE_CFM_EVT:
        case BTA_HL_CTRL_DISABLE_CFM_EVT:
            param_len = sizeof(tBTA_HL_CTRL_ENABLE_DISABLE);
            break;

        default:
            break;
    }

    status = btif_transfer_context(btif_hl_upstreams_ctrl_evt, (uint16_t)event, (void *)p_data,
                                   param_len, NULL);
    ASSERTC(status == TLS_BT_STATUS_SUCCESS, "context transfer failed", status);
}
/*******************************************************************************
**
** Function         connect_channel
**
** Description     connect a data channel
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t connect_channel(int app_id, tls_bt_addr_t *bd_addr, int mdep_cfg_index,
                                       int *channel_id)
{
    uint8_t                   app_idx, mcl_idx;
    btif_hl_app_cb_t        *p_acb = NULL;
    btif_hl_pending_chan_cb_t   *p_pcb = NULL;
    btif_hl_mcl_cb_t        *p_mcb = NULL;
    tls_bt_status_t             status = TLS_BT_STATUS_SUCCESS;
    tBTA_HL_DCH_OPEN_PARAM  dch_open;
    BD_ADDR                 bda;
    uint8_t i;
    CHECK_BTHL_INIT();
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    btif_hl_display_calling_process_name();

    for(i = 0; i < 6; i++) {
        bda[i] = (uint8_t) bd_addr->address[i];
    }

    if(btif_hl_find_app_idx(((uint8_t)app_id), &app_idx)) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);

        if(btif_hl_find_mcl_idx(app_idx, bda, &mcl_idx)) {
            p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

            if(p_mcb->is_connected) {
                dch_open.ctrl_psm = p_mcb->ctrl_psm;
                dch_open.local_mdep_id = p_acb->sup_feature.mdep[mdep_cfg_index].mdep_id;
                BTIF_TRACE_DEBUG("connect_channel: app_idx =%d, mdep_cfg_indx =%d, mdep_id =%d app_id= %d", app_idx,
                                 mdep_cfg_index, dch_open.local_mdep_id, app_id);

                if(btif_hl_find_peer_mdep_id(p_acb->app_id, p_mcb->bd_addr,
                                             p_acb->sup_feature.mdep[mdep_cfg_index].mdep_cfg.mdep_role,
                                             p_acb->sup_feature.mdep[mdep_cfg_index].mdep_cfg.data_cfg[0].data_type, &dch_open.peer_mdep_id)) {
                    dch_open.local_cfg = p_acb->channel_type[mdep_cfg_index];

                    if((p_acb->sup_feature.mdep[mdep_cfg_index].mdep_cfg.mdep_role == BTA_HL_MDEP_ROLE_SOURCE)
                            && !btif_hl_is_the_first_reliable_existed(app_idx, mcl_idx)) {
                        dch_open.local_cfg = BTA_HL_DCH_CFG_RELIABLE;
                    }

                    dch_open.sec_mask = (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT);

                    if(!btif_hl_dch_open(p_acb->app_id, bda, &dch_open,
                                         mdep_cfg_index, BTIF_HL_PEND_DCH_OP_OPEN, channel_id)) {
                        status = TLS_BT_STATUS_FAIL;
                        BTIF_TRACE_EVENT("%s loc0 status = TLS_BT_STATUS_FAIL", __FUNCTION__);
                    }
                } else {
                    p_mcb->cch_oper = BTIF_HL_CCH_OP_MDEP_FILTERING;
                    p_pcb = BTIF_HL_GET_PCB_PTR(app_idx, mcl_idx);
                    p_pcb->in_use = TRUE;
                    p_pcb->mdep_cfg_idx = mdep_cfg_index;
                    wm_memcpy(p_pcb->bd_addr, bda, sizeof(BD_ADDR));
                    p_pcb->op = BTIF_HL_PEND_DCH_OP_OPEN;
                    BTA_HlSdpQuery(app_id, p_acb->app_handle, bda);
                }
            } else {
                status = TLS_BT_STATUS_FAIL;
            }
        } else {
            p_acb->filter.num_elems = 1;
            p_acb->filter.elem[0].data_type =
                            p_acb->sup_feature.mdep[mdep_cfg_index].mdep_cfg.data_cfg[mdep_cfg_index].data_type;

            if(p_acb->sup_feature.mdep[mdep_cfg_index].mdep_cfg.mdep_role == BTA_HL_MDEP_ROLE_SINK) {
                p_acb->filter.elem[0].peer_mdep_role = BTA_HL_MDEP_ROLE_SOURCE;
            } else {
                p_acb->filter.elem[0].peer_mdep_role = BTA_HL_MDEP_ROLE_SINK;
            }

            if(!btif_hl_cch_open(p_acb->app_id, bda, 0, mdep_cfg_index,
                                 BTIF_HL_PEND_DCH_OP_OPEN,
                                 channel_id)) {
                status = TLS_BT_STATUS_FAIL;
            }
        }
    } else {
        status = TLS_BT_STATUS_FAIL;
    }

    BTIF_TRACE_DEBUG("%s status=%d channel_id=0x%08x", __FUNCTION__, status, *channel_id);
    return status;
}
/*******************************************************************************
**
** Function         destroy_channel
**
** Description      destroy a data channel
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t destroy_channel(int channel_id)
{
    uint8_t app_idx, mcl_idx, mdl_cfg_idx, mdep_cfg_idx = 0;
    tls_bt_status_t status = TLS_BT_STATUS_SUCCESS;
    btif_hl_mdl_cfg_t     *p_mdl;
    btif_hl_mcl_cb_t     *p_mcb;
    btif_hl_app_cb_t     *p_acb;
    CHECK_BTHL_INIT();
    BTIF_TRACE_EVENT("%s channel_id=0x%08x", __FUNCTION__, channel_id);
    btif_hl_display_calling_process_name();

    if(btif_hl_if_channel_setup_pending(channel_id, &app_idx, &mcl_idx)) {
        btif_hl_dch_abort(app_idx, mcl_idx);
    } else {
        if(btif_hl_find_mdl_cfg_idx_using_channel_id(channel_id, &app_idx, &mdl_cfg_idx))
            //       if(btif_hl_find_mdl_idx_using_channel_id(channel_id, &app_idx,&mcl_idx, &mdl_idx))
        {
            p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);

            if(!p_acb->delete_mdl.active) {
                p_mdl = BTIF_HL_GET_MDL_CFG_PTR(app_idx, mdl_cfg_idx);
                p_acb->delete_mdl.active = TRUE;
                p_acb->delete_mdl.mdl_id = p_mdl->base.mdl_id;
                p_acb->delete_mdl.channel_id = channel_id;
                p_acb->delete_mdl.mdep_cfg_idx = p_mdl->extra.mdep_cfg_idx;
                wm_memcpy(p_acb->delete_mdl.bd_addr, p_mdl->base.peer_bd_addr, sizeof(BD_ADDR));

                if(btif_hl_find_mcl_idx(app_idx, p_mdl->base.peer_bd_addr, &mcl_idx)) {
                    p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

                    if(p_mcb->is_connected) {
                        BTIF_TRACE_DEBUG("calling BTA_HlDeleteMdl mdl_id=%d", p_acb->delete_mdl.mdl_id);
                        BTA_HlDeleteMdl(p_mcb->mcl_handle, p_acb->delete_mdl.mdl_id);
                    } else {
                        status = TLS_BT_STATUS_FAIL;
                    }
                } else {
                    BTIF_TRACE_DEBUG("btif_hl_delete_mdl calling btif_hl_cch_open");
                    mdep_cfg_idx = p_mdl->extra.mdep_cfg_idx;
                    p_acb->filter.num_elems = 1;
                    p_acb->filter.elem[0].data_type =
                                    p_acb->sup_feature.mdep[mdep_cfg_idx].mdep_cfg.data_cfg[mdep_cfg_idx].data_type;

                    if(p_acb->sup_feature.mdep[mdep_cfg_idx].mdep_cfg.mdep_role == BTA_HL_MDEP_ROLE_SINK) {
                        p_acb->filter.elem[0].peer_mdep_role = BTA_HL_MDEP_ROLE_SOURCE;
                    } else {
                        p_acb->filter.elem[0].peer_mdep_role = BTA_HL_MDEP_ROLE_SINK;
                    }

                    if(btif_hl_cch_open(p_acb->app_id, p_acb->delete_mdl.bd_addr, 0,
                                        mdep_cfg_idx,
                                        BTIF_HL_PEND_DCH_OP_DELETE_MDL, NULL)) {
                        status = TLS_BT_STATUS_FAIL;
                    }
                }

                if(status == TLS_BT_STATUS_FAIL) {
                    /* fail for now  */
                    btif_hl_clean_delete_mdl(&p_acb->delete_mdl);
                }
            } else {
                status = TLS_BT_STATUS_BUSY;
            }
        } else {
            status = TLS_BT_STATUS_FAIL;
        }
    }

    return status;
}
/*******************************************************************************
**
** Function         unregister_application
**
** Description     unregister an HDP application
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t unregister_application(int app_id)
{
    uint8_t               app_idx;
    int                 len;
    tls_bt_status_t         status = TLS_BT_STATUS_SUCCESS;
    btif_hl_evt_cb_t    evt_param;
    CHECK_BTHL_INIT();
    BTIF_TRACE_EVENT("%s app_id=%d", __FUNCTION__, app_id);
    btif_hl_display_calling_process_name();

    if(btif_hl_find_app_idx(((uint8_t)app_id), &app_idx)) {
        evt_param.unreg.app_idx = app_idx;
        reg_counter --;
        len = sizeof(btif_hl_unreg_t);
        status = btif_transfer_context(btif_hl_proc_cb_evt, BTIF_HL_UNREG_APP,
                                       (char *) &evt_param, len, NULL);
        ASSERTC(status == TLS_BT_STATUS_SUCCESS, "context transfer failed", status);
    } else {
        status  = TLS_BT_STATUS_FAIL;
    }

    BTIF_TRACE_DEBUG("de-reg return status=%d", status);
    return status;
}
/*******************************************************************************
**
** Function         register_application
**
** Description     register an HDP application
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t register_application(bthl_reg_param_t *p_reg_param, int *app_id)
{
    btif_hl_app_cb_t            *p_acb;
    tBTA_HL_SUP_FEATURE         *p_sup;
    tBTA_HL_MDEP_CFG            *p_cfg;
    tBTA_HL_MDEP_DATA_TYPE_CFG  *p_data;
    uint8_t                       app_idx = 0, i = 0;
    bthl_mdep_cfg_t             *p_mdep_cfg;
    tls_bt_status_t                 status = TLS_BT_STATUS_SUCCESS;
    btif_hl_evt_cb_t            evt_param;
    int                         len;
    CHECK_BTHL_INIT();
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    btif_hl_display_calling_process_name();

    if(btif_hl_get_state() == BTIF_HL_STATE_DISABLED) {
        btif_hl_init();
        btif_hl_set_state(BTIF_HL_STATE_ENABLING);
        BTA_HlEnable(btif_hl_ctrl_cback);
    }

    if(!btif_hl_find_avail_app_idx(&app_idx)) {
        BTIF_TRACE_ERROR("Unable to allocate a new application control block");
        return TLS_BT_STATUS_FAIL;
    }

    p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);
    p_acb->in_use = TRUE;
    p_acb->app_id = btif_hl_get_next_app_id();

    if(p_reg_param->application_name != NULL) {
        strncpy(p_acb->application_name, p_reg_param->application_name, BTIF_HL_APPLICATION_NAME_LEN);
    }

    if(p_reg_param->provider_name != NULL) {
        strncpy(p_acb->provider_name, p_reg_param->provider_name, BTA_PROVIDER_NAME_LEN);
    }

    if(p_reg_param->srv_name != NULL) {
        strncpy(p_acb->srv_name, p_reg_param->srv_name, BTA_SERVICE_NAME_LEN);
    }

    if(p_reg_param->srv_desp != NULL) {
        strncpy(p_acb->srv_desp, p_reg_param->srv_desp, BTA_SERVICE_DESP_LEN);
    }

    p_sup = &p_acb->sup_feature;
    p_sup->advertize_source_sdp = TRUE;
    p_sup->echo_cfg.max_rx_apdu_size = BTIF_HL_ECHO_MAX_TX_RX_APDU_SIZE;
    p_sup->echo_cfg.max_tx_apdu_size = BTIF_HL_ECHO_MAX_TX_RX_APDU_SIZE;
    p_sup->num_of_mdeps = p_reg_param->number_of_mdeps;

    for(i = 0, p_mdep_cfg = p_reg_param->mdep_cfg ; i <  p_sup->num_of_mdeps; i++, p_mdep_cfg++) {
        p_cfg = &p_sup->mdep[i].mdep_cfg;
        p_cfg->num_of_mdep_data_types = 1;
        p_data  = &p_cfg->data_cfg[0];

        if(!btif_hl_get_bta_mdep_role(p_mdep_cfg->mdep_role, &(p_cfg->mdep_role))) {
            BTIF_TRACE_ERROR("Invalid mdep_role=%d", p_mdep_cfg->mdep_role);
            status = TLS_BT_STATUS_FAIL;
            break;
        } else {
            if(p_cfg->mdep_role == BTA_HL_MDEP_ROLE_SINK) {
                p_sup->app_role_mask |= BTA_HL_MDEP_ROLE_MASK_SINK;
            } else {
                p_sup->app_role_mask |=  BTA_HL_MDEP_ROLE_MASK_SOURCE;
            }

            if((p_sup->app_role_mask & BTA_HL_MDEP_ROLE_MASK_SINK) &&
                    (p_sup->app_role_mask & BTA_HL_MDEP_ROLE_MASK_SINK)) {
                p_acb->dev_type = BTA_HL_DEVICE_TYPE_DUAL;
            } else if(p_sup->app_role_mask & BTA_HL_MDEP_ROLE_MASK_SINK) {
                p_acb->dev_type = BTA_HL_DEVICE_TYPE_SINK;
            } else {
                p_acb->dev_type = BTA_HL_DEVICE_TYPE_SOURCE;
            }

            p_data->data_type = (uint16_t) p_mdep_cfg->data_type;
            p_data->max_rx_apdu_size = btif_hl_get_max_rx_apdu_size(p_cfg->mdep_role, p_data->data_type);
            p_data->max_tx_apdu_size = btif_hl_get_max_tx_apdu_size(p_cfg->mdep_role, p_data->data_type);

            if(p_mdep_cfg->mdep_description != NULL) {
                strncpy(p_data->desp, p_mdep_cfg->mdep_description, BTA_SERVICE_DESP_LEN);
            }

            if(!btif_hl_get_bta_channel_type(p_mdep_cfg->channel_type, &(p_acb->channel_type[i]))) {
                BTIF_TRACE_ERROR("Invalid channel_type=%d", p_mdep_cfg->channel_type);
                status = TLS_BT_STATUS_FAIL;
                break;
            }
        }
    }

    if(status == TLS_BT_STATUS_SUCCESS) {
        *app_id = (int) p_acb->app_id;
        evt_param.reg.app_idx = app_idx;
        len = sizeof(btif_hl_reg_t);
        p_acb->reg_pending = TRUE;
        reg_counter++;
        BTIF_TRACE_DEBUG("calling btif_transfer_context status=%d app_id=%d", status, *app_id);
        status = btif_transfer_context(btif_hl_proc_cb_evt, BTIF_HL_REG_APP,
                                       (char *) &evt_param, len, NULL);
        ASSERTC(status == TLS_BT_STATUS_SUCCESS, "context transfer failed", status);
    } else {
        btif_hl_free_app_idx(app_idx);
    }

    BTIF_TRACE_DEBUG("register_application status=%d app_id=%d", status, *app_id);
    return status;
}

/*******************************************************************************
**
** Function      btif_hl_save_mdl_cfg
**
** Description  Save the MDL configuration
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t  btif_hl_save_mdl_cfg(uint8_t mdep_id, uint8_t item_idx,
                              tBTA_HL_MDL_CFG *p_mdl_cfg)
{
    btif_hl_mdl_cfg_t   *p_mdl = NULL;
    uint8_t             success = FALSE;
    btif_hl_app_cb_t    *p_acb;
    btif_hl_mcl_cb_t    *p_mcb;
    uint8_t               app_idx, mcl_idx, len;
    tls_bt_status_t         bt_status;
    btif_hl_evt_cb_t    evt_param;
    int                 *p_channel_id;
    BTIF_TRACE_DEBUG("%s mdep_id=%d item_idx=%d, local_mdep_id=%d mdl_id=0x%x dch_mode=%d",
                     __FUNCTION__, mdep_id, item_idx, p_mdl_cfg->local_mdep_id,
                     p_mdl_cfg->mdl_id, p_mdl_cfg->dch_mode);

    if(btif_hl_find_app_idx_using_mdepId(mdep_id, &app_idx)) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);
        p_mdl = BTIF_HL_GET_MDL_CFG_PTR(app_idx, item_idx);
        p_channel_id = BTIF_HL_GET_MDL_CFG_CHANNEL_ID_PTR(app_idx, item_idx);

        if(p_mdl) {
            wm_memcpy(&p_mdl->base, p_mdl_cfg, sizeof(tBTA_HL_MDL_CFG));

            if(btif_hl_find_mcl_idx(app_idx, p_mdl->base.peer_bd_addr, &mcl_idx)) {
                p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

                if(p_mcb->pcb.in_use) {
                    *p_channel_id = p_mcb->pcb.channel_id;
                } else {
                    *p_channel_id = btif_hl_get_next_channel_id(p_acb->app_id);
                }

                p_mdl->extra.mdep_cfg_idx = p_mcb->pcb.mdep_cfg_idx;
                p_mdl->extra.data_type =
                                p_acb->sup_feature.mdep[p_mcb->pcb.mdep_cfg_idx].mdep_cfg.data_cfg[0].data_type;

                if(!btif_hl_find_peer_mdep_id(p_acb->app_id, p_mcb->bd_addr,
                                              p_acb->sup_feature.mdep[p_mcb->pcb.mdep_cfg_idx].mdep_cfg.mdep_role,
                                              p_acb->sup_feature.mdep[p_mcb->pcb.mdep_cfg_idx].mdep_cfg.data_cfg[0].data_type,
                                              &p_mdl->extra.peer_mdep_id)) {
                    p_mdl->extra.peer_mdep_id = BTA_HL_INVALID_MDEP_ID;
                }

                BTIF_TRACE_DEBUG("%s app_idx=%d item_idx=%d mld_id=0x%x",
                                 __FUNCTION__, app_idx, item_idx, p_mdl->base.mdl_id);
                evt_param.update_mdl.app_idx = app_idx;
                len = sizeof(btif_hl_update_mdl_t);
                BTIF_TRACE_DEBUG("send BTIF_HL_UPDATE_MDL event app_idx=%d  ", app_idx);

                if((bt_status = btif_transfer_context(btif_hl_proc_cb_evt, BTIF_HL_UPDATE_MDL,
                                                      (char *) &evt_param, len, NULL)) == TLS_BT_STATUS_SUCCESS) {
                    success = TRUE;
                }

                ASSERTC(bt_status == TLS_BT_STATUS_SUCCESS, "context transfer failed", bt_status);
            }
        }
    }

    BTIF_TRACE_DEBUG("%s success=%d  ", __FUNCTION__, success);
    return success;
}

/*******************************************************************************
**
** Function      btif_hl_delete_mdl_cfg
**
** Description  Delete the MDL configuration
**
** Returns      uint8_t
**
*******************************************************************************/
uint8_t  btif_hl_delete_mdl_cfg(uint8_t mdep_id, uint8_t item_idx)
{
    btif_hl_mdl_cfg_t     *p_mdl = NULL;
    uint8_t             success = FALSE;
    uint8_t               app_idx, len;
    tls_bt_status_t         bt_status;
    btif_hl_evt_cb_t    evt_param;

    if(btif_hl_find_app_idx_using_mdepId(mdep_id, &app_idx)) {
        p_mdl = BTIF_HL_GET_MDL_CFG_PTR(app_idx, item_idx);

        if(p_mdl) {
            wm_memset(p_mdl, 0, sizeof(btif_hl_mdl_cfg_t));
            evt_param.update_mdl.app_idx = app_idx;
            len = sizeof(btif_hl_update_mdl_t);
            BTIF_TRACE_DEBUG("send BTIF_HL_UPDATE_MDL event app_idx=%d  ", app_idx);

            if((bt_status = btif_transfer_context(btif_hl_proc_cb_evt, BTIF_HL_UPDATE_MDL,
                                                  (char *) &evt_param, len, NULL)) == TLS_BT_STATUS_SUCCESS) {
                success = TRUE;
            }

            ASSERTC(bt_status == TLS_BT_STATUS_SUCCESS, "context transfer failed", bt_status);
        }
    }

    BTIF_TRACE_DEBUG("%s success=%d  ", __FUNCTION__, success);
    return success;
}

/*******************************************************************************
**
** Function         init
**
** Description     initializes the hl interface
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t init(bthl_callbacks_t *callbacks)
{
    tls_bt_status_t status = TLS_BT_STATUS_SUCCESS;
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    btif_hl_display_calling_process_name();
    bt_hl_callbacks_cb = *callbacks;
    bt_hl_callbacks = &bt_hl_callbacks_cb;
    btif_hl_soc_thread_init();
    reg_counter = 0;
    return status;
}
/*******************************************************************************
**
** Function         cleanup
**
** Description      Closes the HL interface
**
** Returns          void
**
*******************************************************************************/
static void  cleanup(void)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    btif_hl_display_calling_process_name();

    if(bt_hl_callbacks) {
        btif_disable_service(BTA_HDP_SERVICE_ID);
        bt_hl_callbacks = NULL;
        reg_counter = 0;
    }

    btif_hl_disable();
    btif_hl_close_select_thread();
}

static const bthl_interface_t bthlInterface = {
    sizeof(bthl_interface_t),
    init,
    register_application,
    unregister_application,
    connect_channel,
    destroy_channel,
    cleanup,
};

/*******************************************************************************
**
** Function         btif_hl_get_interface
**
** Description      Get the hl callback interface
**
** Returns          bthf_interface_t
**
*******************************************************************************/
const bthl_interface_t *btif_hl_get_interface()
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    return &bthlInterface;
}

/*******************************************************************************
**
** Function btif_hl_update_maxfd
**
** Description Update the max fd if the input fd is greater than the current max fd
**
** Returns int
**
*******************************************************************************/
int btif_hl_update_maxfd(int max_org_s)
{
    int maxfd = max_org_s;
    BTIF_TRACE_DEBUG("btif_hl_update_maxfd max_org_s= %d", max_org_s);

    for(const list_node_t *node = list_begin(soc_queue);
            node != list_end(soc_queue); node = list_next(node)) {
        btif_hl_soc_cb_t *p_scb = list_node(node);

        if(maxfd < p_scb->max_s) {
            maxfd = p_scb->max_s;
            BTIF_TRACE_DEBUG("btif_hl_update_maxfd maxfd=%d", maxfd);
        }
    }

    BTIF_TRACE_DEBUG("btif_hl_update_maxfd final *p_max_s=%d", maxfd);
    return maxfd;
}
/*******************************************************************************
**
** Function btif_hl_get_socket_state
**
** Description get socket state
**
** Returns btif_hl_soc_state_t
**
*******************************************************************************/
btif_hl_soc_state_t btif_hl_get_socket_state(btif_hl_soc_cb_t *p_scb)
{
    BTIF_TRACE_DEBUG("btif_hl_get_socket_state state=%d", p_scb->state);
    return p_scb->state;
}
/*******************************************************************************
**
** Function btif_hl_set_socket_state
**
** Description set socket state
**
** Returns void
**
*******************************************************************************/
void btif_hl_set_socket_state(btif_hl_soc_cb_t *p_scb, btif_hl_soc_state_t new_state)
{
    BTIF_TRACE_DEBUG("btif_hl_set_socket_state %d---->%d", p_scb->state, new_state);
    p_scb->state = new_state;
}
/*******************************************************************************
**
** Function btif_hl_release_mcl_sockets
**
** Description Release all sockets on the MCL
**
** Returns void
**
*******************************************************************************/
void btif_hl_release_mcl_sockets(uint8_t app_idx, uint8_t mcl_idx)
{
    uint8_t               i;
    btif_hl_mdl_cb_t    *p_dcb;
    uint8_t             found = FALSE;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    for(i = 0; i < BTA_HL_NUM_MDLS_PER_MCL ; i ++) {
        p_dcb = BTIF_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, i);

        if(p_dcb && p_dcb->in_use && p_dcb->p_scb) {
            BTIF_TRACE_DEBUG("found socket for app_idx=%d mcl_id=%d, mdl_idx=%d", app_idx, mcl_idx, i);
            btif_hl_set_socket_state(p_dcb->p_scb, BTIF_HL_SOC_STATE_W4_REL);
            p_dcb->p_scb = NULL;
            found = TRUE;
        }
    }

    if(found) {
        btif_hl_select_close_connected();
    }
}
/*******************************************************************************
**
** Function btif_hl_release_socket
**
** Description release a specified socket
**
** Returns void
**
*******************************************************************************/
void btif_hl_release_socket(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx)
{
    btif_hl_soc_cb_t       *p_scb = NULL;
    btif_hl_mdl_cb_t      *p_dcb = BTIF_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    BTIF_TRACE_DEBUG("app_idx=%d mcl_idx=%d mdl_idx=%d",  app_idx, mcl_idx, mdl_idx);

    if(p_dcb && p_dcb->p_scb) {
        p_scb = p_dcb->p_scb;
        btif_hl_set_socket_state(p_scb,  BTIF_HL_SOC_STATE_W4_REL);
        p_dcb->p_scb = NULL;
        btif_hl_select_close_connected();
    }
}
/*******************************************************************************
**
** Function btif_hl_create_socket
**
** Description create a socket
**
** Returns uint8_t
**
*******************************************************************************/
uint8_t btif_hl_create_socket(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx)
{
    btif_hl_mcl_cb_t      *p_mcb = BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    btif_hl_mdl_cb_t      *p_dcb = BTIF_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    uint8_t               status = FALSE;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    if(p_dcb) {
        btif_hl_soc_cb_t *p_scb =
                        (btif_hl_soc_cb_t *)GKI_getbuf(sizeof(btif_hl_soc_cb_t));

        if(socketpair(AF_UNIX, SOCK_STREAM, 0, p_scb->socket_id) >= 0) {
            BTIF_TRACE_DEBUG("socket id[0]=%d id[1]=%d", p_scb->socket_id[0], p_scb->socket_id[1]);
            p_dcb->p_scb = p_scb;
            p_scb->app_idx = app_idx;
            p_scb->mcl_idx = mcl_idx;
            p_scb->mdl_idx = mdl_idx;
            p_scb->channel_id = p_dcb->channel_id;
            p_scb->mdep_cfg_idx = p_dcb->local_mdep_cfg_idx;
            wm_memcpy(p_scb->bd_addr, p_mcb->bd_addr, sizeof(BD_ADDR));
            btif_hl_set_socket_state(p_scb,  BTIF_HL_SOC_STATE_W4_ADD);
            p_scb->max_s = p_scb->socket_id[1];
            list_append(soc_queue, (void *)p_scb);
            btif_hl_select_wakeup();
            status = TRUE;
        } else {
            GKI_free_and_reset_buf((void **)&p_scb);
        }
    }

    BTIF_TRACE_DEBUG("%s status=%d", __FUNCTION__, status);
    return status;
}
/*******************************************************************************
**
** Function btif_hl_add_socket_to_set
**
** Description Add a socket
**
** Returns void
**
*******************************************************************************/
void btif_hl_add_socket_to_set(fd_set *p_org_set)
{
    btif_hl_mdl_cb_t                *p_dcb = NULL;
    btif_hl_mcl_cb_t                *p_mcb = NULL;
    btif_hl_app_cb_t                *p_acb = NULL;
    btif_hl_evt_cb_t                evt_param;
    tls_bt_status_t                     status;
    int                             len;
    BTIF_TRACE_DEBUG("entering %s", __FUNCTION__);

    for(const list_node_t *node = list_begin(soc_queue);
            node != list_end(soc_queue); node = list_next(node)) {
        btif_hl_soc_cb_t *p_scb = list_node(node);
        BTIF_TRACE_DEBUG("btif_hl_add_socket_to_set first p_scb=0x%x", p_scb);

        if(btif_hl_get_socket_state(p_scb) == BTIF_HL_SOC_STATE_W4_ADD) {
            btif_hl_set_socket_state(p_scb, BTIF_HL_SOC_STATE_W4_READ);
            FD_SET(p_scb->socket_id[1], p_org_set);
            BTIF_TRACE_DEBUG("found and set socket_id=%d is_set=%d",
                             p_scb->socket_id[1], FD_ISSET(p_scb->socket_id[1], p_org_set));
            p_mcb = BTIF_HL_GET_MCL_CB_PTR(p_scb->app_idx, p_scb->mcl_idx);
            p_dcb = BTIF_HL_GET_MDL_CB_PTR(p_scb->app_idx, p_scb->mcl_idx, p_scb->mdl_idx);
            p_acb = BTIF_HL_GET_APP_CB_PTR(p_scb->app_idx);

            if(p_mcb && p_dcb) {
                btif_hl_stop_timer_using_handle(p_mcb->mcl_handle);
                evt_param.chan_cb.app_id = p_acb->app_id;
                wm_memcpy(evt_param.chan_cb.bd_addr, p_mcb->bd_addr, sizeof(BD_ADDR));
                evt_param.chan_cb.channel_id = p_dcb->channel_id;
                evt_param.chan_cb.fd = p_scb->socket_id[0];
                evt_param.chan_cb.mdep_cfg_index = (int) p_dcb->local_mdep_cfg_idx;
                evt_param.chan_cb.cb_state = BTIF_HL_CHAN_CB_STATE_CONNECTED_PENDING;
                len = sizeof(btif_hl_send_chan_state_cb_t);
                status = btif_transfer_context(btif_hl_proc_cb_evt, BTIF_HL_SEND_CONNECTED_CB,
                                               (char *) &evt_param, len, NULL);
                ASSERTC(status == TLS_BT_STATUS_SUCCESS, "context transfer failed", status);
            }
        }
    }

    BTIF_TRACE_DEBUG("leaving %s", __FUNCTION__);
}

/*******************************************************************************
**
** Function btif_hl_close_socket
**
** Description close a socket
**
** Returns void
**
*******************************************************************************/
void btif_hl_close_socket(fd_set *p_org_set)
{
    BTIF_TRACE_DEBUG("entering %s", __FUNCTION__);

    for(const list_node_t *node = list_begin(soc_queue);
            node != list_end(soc_queue); node = list_next(node)) {
        btif_hl_soc_cb_t *p_scb = list_node(node);

        if(btif_hl_get_socket_state(p_scb) == BTIF_HL_SOC_STATE_W4_REL) {
            BTIF_TRACE_DEBUG("app_idx=%d mcl_id=%d, mdl_idx=%d",
                             p_scb->app_idx, p_scb->mcl_idx, p_scb->mdl_idx);
            btif_hl_set_socket_state(p_scb, BTIF_HL_SOC_STATE_IDLE);

            if(p_scb->socket_id[1] != -1) {
                FD_CLR(p_scb->socket_id[1], p_org_set);
                shutdown(p_scb->socket_id[1], SHUT_RDWR);
                close(p_scb->socket_id[1]);
                btif_hl_evt_cb_t evt_param;
                evt_param.chan_cb.app_id = (int) btif_hl_get_app_id(p_scb->channel_id);
                wm_memcpy(evt_param.chan_cb.bd_addr, p_scb->bd_addr, sizeof(BD_ADDR));
                evt_param.chan_cb.channel_id = p_scb->channel_id;
                evt_param.chan_cb.fd = p_scb->socket_id[0];
                evt_param.chan_cb.mdep_cfg_index = (int) p_scb->mdep_cfg_idx;
                evt_param.chan_cb.cb_state = BTIF_HL_CHAN_CB_STATE_DISCONNECTED_PENDING;
                int len = sizeof(btif_hl_send_chan_state_cb_t);
                tls_bt_status_t status = btif_transfer_context(btif_hl_proc_cb_evt,
                                         BTIF_HL_SEND_DISCONNECTED_CB,
                                         (char *) &evt_param, len, NULL);
                ASSERTC(status == TLS_BT_STATUS_SUCCESS, "context transfer failed", status);
            }
        }
    }

    for(const list_node_t *node = list_begin(soc_queue);
            node != list_end(soc_queue);) {
        // We may mutate this list so we need keep track of
        // the current node and only remove items behind.
        btif_hl_soc_cb_t *p_scb = list_node(node);
        BTIF_TRACE_DEBUG("p_scb=0x%x", p_scb);
        node = list_next(node);

        if(btif_hl_get_socket_state(p_scb) == BTIF_HL_SOC_STATE_IDLE) {
            btif_hl_mdl_cb_t *p_dcb = BTIF_HL_GET_MDL_CB_PTR(p_scb->app_idx,
                                      p_scb->mcl_idx, p_scb->mdl_idx);
            BTIF_TRACE_DEBUG("idle socket app_idx=%d mcl_id=%d, mdl_idx=%d p_dcb->in_use=%d",
                             p_scb->app_idx, p_scb->mcl_idx, p_scb->mdl_idx, p_dcb->in_use);
            list_remove(soc_queue, p_scb);
            GKI_freebuf(p_scb);
            p_dcb->p_scb = NULL;
        }
    }

    BTIF_TRACE_DEBUG("leaving %s", __FUNCTION__);
}

/*******************************************************************************
**
** Function btif_hl_select_wakeup_callback
**
** Description Select wakup callback to add or close a socket
**
** Returns void
**
*******************************************************************************/

void btif_hl_select_wakeup_callback(fd_set *p_org_set,  int wakeup_signal)
{
    BTIF_TRACE_DEBUG("entering %s wakeup_signal=0x%04x", __FUNCTION__, wakeup_signal);

    if(wakeup_signal == btif_hl_signal_select_wakeup) {
        btif_hl_add_socket_to_set(p_org_set);
    } else if(wakeup_signal == btif_hl_signal_select_close_connected) {
        btif_hl_close_socket(p_org_set);
    }

    BTIF_TRACE_DEBUG("leaving %s", __FUNCTION__);
}

/*******************************************************************************
**
** Function btif_hl_select_monitor_callback
**
** Description Select monitor callback to check pending socket actions
**
** Returns void
**
*******************************************************************************/
void btif_hl_select_monitor_callback(fd_set *p_cur_set, fd_set *p_org_set)
{
    UNUSED(p_org_set);
    BTIF_TRACE_DEBUG("entering %s", __FUNCTION__);

    for(const list_node_t *node = list_begin(soc_queue);
            node != list_end(soc_queue); node = list_next(node)) {
        btif_hl_soc_cb_t *p_scb = list_node(node);

        if(btif_hl_get_socket_state(p_scb) == BTIF_HL_SOC_STATE_W4_READ) {
            if(FD_ISSET(p_scb->socket_id[1], p_cur_set)) {
                BTIF_TRACE_DEBUG("read data state= BTIF_HL_SOC_STATE_W4_READ");
                btif_hl_mdl_cb_t *p_dcb = BTIF_HL_GET_MDL_CB_PTR(p_scb->app_idx,
                                          p_scb->mcl_idx, p_scb->mdl_idx);
                assert(p_dcb != NULL);

                if(p_dcb->p_tx_pkt) {
                    BTIF_TRACE_ERROR("Rcv new pkt but the last pkt is still not been"
                                     "  sent tx_size=%d", p_dcb->tx_size);
                    GKI_free_and_reset_buf((void **)&p_dcb->p_tx_pkt);
                }

                p_dcb->p_tx_pkt = GKI_getbuf(p_dcb->mtu);
                ssize_t r;
                OSI_NO_INTR(r = recv(p_scb->socket_id[1], p_dcb->p_tx_pkt,
                                     p_dcb->mtu, MSG_DONTWAIT));

                if(r > 0) {
                    BTIF_TRACE_DEBUG("btif_hl_select_monitor_callback send data r =%d", r);
                    p_dcb->tx_size = r;
                    BTIF_TRACE_DEBUG("btif_hl_select_monitor_callback send data tx_size=%d", p_dcb->tx_size);
                    BTA_HlSendData(p_dcb->mdl_handle, p_dcb->tx_size);
                } else {
                    BTIF_TRACE_DEBUG("btif_hl_select_monitor_callback receive failed r=%d", r);
                    BTA_HlDchClose(p_dcb->mdl_handle);
                }
            }
        }
    }

    if(list_is_empty(soc_queue)) {
        BTIF_TRACE_DEBUG("btif_hl_select_monitor_queue is empty");
    }

    BTIF_TRACE_DEBUG("leaving %s", __FUNCTION__);
}

/*******************************************************************************
**
** Function btif_hl_select_wakeup_init
**
** Description select loop wakup init
**
** Returns int
**
*******************************************************************************/
static inline int btif_hl_select_wakeup_init(fd_set *set)
{
    BTIF_TRACE_DEBUG("%s", __func__);

    if(signal_fds[0] == -1 && socketpair(AF_UNIX, SOCK_STREAM, 0, signal_fds) < 0) {
        BTIF_TRACE_ERROR("socketpair failed: %s", strerror(errno));
        return -1;
    }

    BTIF_TRACE_DEBUG("btif_hl_select_wakeup_init signal_fds[0]=%d signal_fds[1]=%d", signal_fds[0],
                     signal_fds[1]);
    FD_SET(signal_fds[0], set);
    return signal_fds[0];
}

/*******************************************************************************
**
** Function btif_hl_select_wakeup
**
** Description send a signal to wakupo the select loop
**
** Returns int
**
*******************************************************************************/
static inline int btif_hl_select_wakeup(void)
{
    char sig_on = btif_hl_signal_select_wakeup;
    BTIF_TRACE_DEBUG("%s", __func__);
    ssize_t ret;
    OSI_NO_INTR(ret = send(signal_fds[1], &sig_on, sizeof(sig_on), 0));
    return (int)ret;
}

/*******************************************************************************
**
** Function btif_hl_select_close_connected
**
** Description send a signal to close a socket
**
** Returns int
**
*******************************************************************************/
static inline int btif_hl_select_close_connected(void)
{
    char sig_on = btif_hl_signal_select_close_connected;
    BTIF_TRACE_DEBUG("%s", __func__);
    ssize_t ret;
    OSI_NO_INTR(ret = send(signal_fds[1], &sig_on, sizeof(sig_on), 0));
    return (int)ret;
}

/*******************************************************************************
**
** Function btif_hl_close_select_thread
**
** Description send signal to close the thread and then close all signal FDs
**
** Returns int
**
*******************************************************************************/
static inline int btif_hl_close_select_thread(void)
{
    ssize_t result = 0;
    char sig_on = btif_hl_signal_select_exit;
    BTIF_TRACE_DEBUG("%", __func__);
    OSI_NO_INTR(result = send(signal_fds[1], &sig_on, sizeof(sig_on), 0));

    if(btif_is_enabled()) {
        /* Wait for the select_thread_id to exit if BT is still enabled
        and only this profile getting  cleaned up*/
        if(select_thread_id != -1) {
            pthread_join(select_thread_id, NULL);
            select_thread_id = -1;
        }
    }

    list_free(soc_queue);
    soc_queue = NULL;
    return (int)result;
}

/*******************************************************************************
**
** Function btif_hl_select_wake_reset
**
** Description clear the received signal for the select loop
**
** Returns int
**
*******************************************************************************/
static inline int btif_hl_select_wake_reset(void)
{
    char sig_recv = 0;
    BTIF_TRACE_DEBUG("%s", __func__);
    ssize_t r;
    OSI_NO_INTR(r = recv(signal_fds[0], &sig_recv, sizeof(sig_recv),
                         MSG_WAITALL));
    return (int)sig_recv;
}
/*******************************************************************************
**
** Function btif_hl_select_wake_signaled
**
** Description check whether a fd is set or not
**
** Returns int
**
*******************************************************************************/
static inline int btif_hl_select_wake_signaled(fd_set *set)
{
    BTIF_TRACE_DEBUG("btif_hl_select_wake_signaled");
    return FD_ISSET(signal_fds[0], set);
}
/*******************************************************************************
**
** Function btif_hl_thread_cleanup
**
** Description  shut down and clean up the select loop
**
** Returns void
**
*******************************************************************************/
static void btif_hl_thread_cleanup()
{
    if(listen_s != -1) {
        close(listen_s);
    }

    if(connected_s != -1) {
        shutdown(connected_s, SHUT_RDWR);
        close(connected_s);
    }

    listen_s = connected_s = -1;
    BTIF_TRACE_DEBUG("hl thread cleanup");
}
/*******************************************************************************
**
** Function btif_hl_select_thread
**
** Description the select loop
**
** Returns void
**
*******************************************************************************/
static void *btif_hl_select_thread(void *arg)
{
    fd_set org_set, curr_set;
    int r, max_curr_s, max_org_s;
    UNUSED(arg);
    BTIF_TRACE_DEBUG("entered btif_hl_select_thread");
    FD_ZERO(&org_set);
    max_org_s = btif_hl_select_wakeup_init(&org_set);
    BTIF_TRACE_DEBUG("max_s=%d ", max_org_s);

    for(;;) {
        r = 0;
        BTIF_TRACE_DEBUG("set curr_set = org_set ");
        curr_set = org_set;
        max_curr_s = max_org_s;
        int ret = select((max_curr_s + 1), &curr_set, NULL, NULL, NULL);
        BTIF_TRACE_DEBUG("select unblocked ret=%d", ret);

        if(ret == -1) {
            if(errno == EINTR) {
                continue;
            }

            BTIF_TRACE_DEBUG("select() ret -1, exit the thread");
            btif_hl_thread_cleanup();
            select_thread_id = -1;
            return 0;
        } else if(ret) {
            BTIF_TRACE_DEBUG("btif_hl_select_wake_signaled, signal ret=%d", ret);

            if(btif_hl_select_wake_signaled(&curr_set)) {
                r = btif_hl_select_wake_reset();
                BTIF_TRACE_DEBUG("btif_hl_select_wake_signaled, signal:%d", r);

                if(r == btif_hl_signal_select_wakeup || r == btif_hl_signal_select_close_connected) {
                    btif_hl_select_wakeup_callback(&org_set, r);
                } else if(r == btif_hl_signal_select_exit) {
                    btif_hl_thread_cleanup();
                    BTIF_TRACE_DEBUG("Exit hl_select_thread for btif_hl_signal_select_exit");
                    return 0;
                }
            }

            btif_hl_select_monitor_callback(&curr_set, &org_set);
            max_org_s = btif_hl_update_maxfd(max_org_s);
        } else {
            BTIF_TRACE_DEBUG("no data, select ret: %d\n", ret);
        }
    }

    BTIF_TRACE_DEBUG("leaving hl_select_thread");
    return 0;
}

/*******************************************************************************
**
** Function create_thread
**
** Description creat a select loop
**
** Returns pthread_t
**
*******************************************************************************/
static inline pthread_t create_thread(void *(*start_routine)(void *), void *arg)
{
    BTIF_TRACE_DEBUG("create_thread: entered");
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
    pthread_t thread_id = -1;

    if(pthread_create(&thread_id, &thread_attr, start_routine, arg) != 0) {
        BTIF_TRACE_ERROR("pthread_create : %s", strerror(errno));
        return -1;
    }

    BTIF_TRACE_DEBUG("create_thread: thread created successfully");
    return thread_id;
}

/*******************************************************************************
**
** Function         btif_hl_soc_thread_init
**
** Description      HL select loop init function.
**
** Returns          void
**
*******************************************************************************/
void btif_hl_soc_thread_init(void)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    soc_queue = list_new(NULL);

    if(soc_queue == NULL) {
        LOG_ERROR(LOG_TAG, "%s unable to allocate resources for thread", __func__);
    }

    select_thread_id = create_thread(btif_hl_select_thread, NULL);
}
/*******************************************************************************
**
** Function btif_hl_load_mdl_config
**
** Description load the MDL configuation from the application control block
**
** Returns uint8_t
**
*******************************************************************************/
uint8_t btif_hl_load_mdl_config(uint8_t app_id, uint8_t buffer_size,
                                tBTA_HL_MDL_CFG *p_mdl_buf)
{
    uint8_t app_idx;
    uint8_t result = FALSE;
    btif_hl_app_cb_t          *p_acb;
    tBTA_HL_MDL_CFG *p;
    int i;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    if(btif_hl_find_app_idx(app_id, &app_idx)) {
        p_acb  = BTIF_HL_GET_APP_CB_PTR(app_idx);

        for(i = 0, p = p_mdl_buf; i < buffer_size; i++, p++) {
            wm_memcpy(p, &p_acb->mdl_cfg[i].base, sizeof(tBTA_HL_MDL_CFG));
        }

        result = TRUE;
    }

    BTIF_TRACE_DEBUG("result=%d", result);
    return result;
}
#endif
