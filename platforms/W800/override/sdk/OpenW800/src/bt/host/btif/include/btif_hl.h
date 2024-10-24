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

#ifndef BTIF_HL_H
#define BTIF_HL_H

#include <hardware/bluetooth.h>

#include "bta_hl_api.h"
#include "bt_common.h"
#include "osi/include/alarm.h"

/*******************************************************************************
**  Constants & Macros
********************************************************************************/

#define BTIF_HL_DATA_TYPE_NONE               0x0000
#define BTIF_HL_DATA_TYPE_PULSE_OXIMETER     0x1004   /* from BT assigned number */
#define BTIF_HL_DATA_TYPE_BLOOD_PRESSURE_MON 0x1007
#define BTIF_HL_DATA_TYPE_BODY_THERMOMETER   0x1008
#define BTIF_HL_DATA_TYPE_BODY_WEIGHT_SCALE  0x100F
#define BTIF_HL_DATA_TYPE_GLUCOSE_METER      0x1011
#define BTIF_HL_DATA_TYPE_STEP_COUNTER       0x1068
#define BTIF_HL_DATA_TYPE_BCA                0x1014
#define BTIF_HL_DATA_TYPE_PEAK_FLOW          0x1015
#define BTIF_HL_DATA_TYPE_CARDIO             0x1029
#define BTIF_HL_DATA_TYPE_ACTIVITY_HUB       0x1047
#define BTIF_HL_DATA_TYPE_AMM                0x1048

#define BTIF_HL_CCH_NUM_FILTER_ELEMS            3
#define BTIF_HL_APPLICATION_NAME_LEN          512

/*******************************************************************************
**  Type definitions and return values
********************************************************************************/

typedef enum {
    BTIF_HL_SOC_STATE_IDLE,
    BTIF_HL_SOC_STATE_W4_ADD,
    BTIF_HL_SOC_STATE_W4_CONN,
    BTIF_HL_SOC_STATE_W4_READ,
    BTIF_HL_SOC_STATE_W4_REL
} btif_hl_soc_state_t;

typedef enum {
    BTIF_HL_STATE_DISABLED,
    BTIF_HL_STATE_DISABLING,
    BTIF_HL_STATE_ENABLED,
    BTIF_HL_STATE_ENABLING,
} btif_hl_state_t;

typedef enum {
    BTIF_HL_CCH_OP_NONE,
    BTIF_HL_CCH_OP_MDEP_FILTERING,
    BTIF_HL_CCH_OP_MATCHED_CTRL_PSM,
    BTIF_HL_CCH_OP_DCH_OPEN,
    BTIF_HL_CCH_OP_DCH_RECONNECT,
    BTIF_HL_CCH_OP_DCH_ECHO_TEST
} btif_hl_cch_op_t;

typedef enum {
    BTIF_HL_PEND_DCH_OP_NONE,
    BTIF_HL_PEND_DCH_OP_DELETE_MDL,
    BTIF_HL_PEND_DCH_OP_OPEN,
    BTIF_HL_PEND_DCH_OP_RECONNECT
} btif_hl_pend_dch_op_t;

typedef enum {
    BTIF_HL_DCH_OP_NONE,
    BTIF_HL_DCH_OP_DISC
} btif_hl_dch_op_t;

typedef enum {
    BTIF_HL_CHAN_CB_STATE_NONE,
    BTIF_HL_CHAN_CB_STATE_CONNECTING_PENDING,
    BTIF_HL_CHAN_CB_STATE_CONNECTED_PENDING,

    BTIF_HL_CHAN_CB_STATE_DISCONNECTING_PENDING,
    BTIF_HL_CHAN_CB_STATE_DISCONNECTED_PENDING,
    BTIF_HL_CHAN_CB_STATE_DESTROYED_PENDING,
} btif_hl_chan_cb_state_t;

enum {
    BTIF_HL_SEND_CONNECTED_CB,
    BTIF_HL_SEND_DISCONNECTED_CB,
    BTIF_HL_REG_APP,
    BTIF_HL_UNREG_APP,
    BTIF_HL_UPDATE_MDL,
};

typedef struct {
    uint8_t mdep_cfg_idx;
    int data_type;
    tBTA_HL_MDEP_ID peer_mdep_id;
} btif_hl_extra_mdl_cfg_t;

typedef struct {
    tBTA_HL_MDL_CFG         base;
    btif_hl_extra_mdl_cfg_t extra;
} btif_hl_mdl_cfg_t;

typedef struct {
    uint8_t active;
    uint8_t app_idx;
} btif_hl_app_data_t;

typedef struct {
    int                     channel_id;
    BD_ADDR                 bd_addr;
    uint8_t                   mdep_cfg_idx;
    int                     max_s;
    int                     socket_id[2];
    uint8_t                   app_idx;
    uint8_t                   mcl_idx;
    uint8_t                   mdl_idx;
    btif_hl_soc_state_t     state;
} btif_hl_soc_cb_t;

typedef struct {
    uint16_t                  data_type;
    uint16_t                  max_tx_apdu_size;
    uint16_t                  max_rx_apdu_size;
} btif_hl_data_type_cfg_t;

typedef struct {
    uint16_t                  data_type;
    tBTA_HL_MDEP_ROLE       peer_mdep_role;
} btif_hl_filter_elem_t;

typedef struct {
    uint8_t                   num_elems;
    btif_hl_filter_elem_t   elem[BTIF_HL_CCH_NUM_FILTER_ELEMS];
} btif_hl_cch_filter_t;

typedef struct {
    uint8_t                 in_use;
    uint16_t                  mdl_id;
    tBTA_HL_MDL_HANDLE      mdl_handle;
    btif_hl_dch_op_t        dch_oper;
    tBTA_HL_MDEP_ID         local_mdep_id;
    uint8_t                   local_mdep_cfg_idx;
    tBTA_HL_DCH_CFG         local_cfg;
    tBTA_HL_MDEP_ID         peer_mdep_id;
    uint16_t                  peer_data_type;
    tBTA_HL_MDEP_ROLE       peer_mdep_role;
    tBTA_HL_DCH_MODE        dch_mode;
    tBTA_SEC                sec_mask;
    uint8_t                 is_the_first_reliable;
    uint8_t                 delete_mdl;
    uint16_t                  mtu;
    tMCA_CHNL_CFG           chnl_cfg;
    uint16_t                  tx_size;
    uint8_t                   *p_tx_pkt;
    uint8_t                   *p_rx_pkt;
    uint8_t                 cong;
    btif_hl_soc_cb_t        *p_scb;
    int                     channel_id;
} btif_hl_mdl_cb_t;

typedef struct {
    int                     channel_id;
    int                     mdep_cfg_idx;
    uint8_t                 in_use;
    btif_hl_chan_cb_state_t cb_state;
    btif_hl_pend_dch_op_t   op;
    BD_ADDR                 bd_addr;
    uint8_t                 abort_pending;
} btif_hl_pending_chan_cb_t;

typedef struct {
    btif_hl_mdl_cb_t        mdl[BTA_HL_NUM_MDLS_PER_MCL];
    uint8_t                 in_use;
    uint8_t                 is_connected;
    uint16_t                  req_ctrl_psm;
    uint16_t                  ctrl_psm;
    uint16_t                  data_psm;
    BD_ADDR                 bd_addr;
    uint16_t                  cch_mtu;
    tBTA_SEC                sec_mask;
    tBTA_HL_MCL_HANDLE      mcl_handle;
    btif_hl_pending_chan_cb_t pcb;
    uint8_t                 valid_sdp_idx;
    uint8_t                   sdp_idx;
    tBTA_HL_SDP             sdp;
    btif_hl_cch_op_t        cch_oper;
    TIMER_LIST_ENT  cch_timer;
} btif_hl_mcl_cb_t;

typedef struct {
    uint8_t                 active;
    uint16_t                  mdl_id;
    uint8_t                   mdep_cfg_idx;
    BD_ADDR                 bd_addr;
    int                     channel_id;
} btif_hl_delete_mdl_t;

typedef struct {
    btif_hl_mcl_cb_t        mcb[BTA_HL_NUM_MCLS]; /* application Control Blocks */
    uint8_t                 in_use;              /* this CB is in use*/
    uint8_t                 reg_pending;
    uint8_t                   app_id;

    tBTA_HL_SUP_FEATURE     sup_feature;
    tBTA_HL_DCH_CFG         channel_type[BTA_HL_NUM_MDEPS];
    tBTA_HL_SDP_INFO_IND    sdp_info_ind;
    btif_hl_cch_filter_t    filter;

    btif_hl_mdl_cfg_t       mdl_cfg[BTA_HL_NUM_MDL_CFGS];
    int                     mdl_cfg_channel_id[BTA_HL_NUM_MDL_CFGS];

    btif_hl_delete_mdl_t    delete_mdl;
    tBTA_HL_DEVICE_TYPE     dev_type;
    tBTA_HL_APP_HANDLE      app_handle;
    uint16_t                  sec_mask;   /* Security mask for BTM_SetSecurityLevel() */
    char                    srv_name[BTA_SERVICE_NAME_LEN +
                                                          1];       /* service name to be used in the SDP; null terminated*/
    char                    srv_desp[BTA_SERVICE_DESP_LEN +
                                                          1];       /* service description to be used in the SDP; null terminated */
    char                    provider_name[BTA_PROVIDER_NAME_LEN +
                                                                1];  /* provide name to be used in the SDP; null terminated */
    char                    application_name[BTIF_HL_APPLICATION_NAME_LEN + 1];  /* applicaiton name */
} btif_hl_app_cb_t;

typedef struct {
    uint8_t                 in_use;
    uint8_t                   app_idx;
} btif_hl_pending_reg_cb_t;

/* BTIF-HL control block  */
typedef struct {
    btif_hl_app_cb_t        acb[BTA_HL_NUM_APPS];      /* HL Control Blocks */
    tBTA_HL_CTRL_CBACK      *p_ctrl_cback;             /* pointer to control callback function */
    uint8_t                   next_app_id;
    uint16_t                  next_channel_id;
    btif_hl_state_t         state;
} btif_hl_cb_t;

typedef uint8_t btif_hl_evt_t;

typedef struct {
    int                     app_id;
    BD_ADDR                 bd_addr;
    int                     mdep_cfg_index;
    int                     channel_id;
    btif_hl_chan_cb_state_t cb_state;
    int                     fd;
} btif_hl_send_chan_state_cb_t;

typedef struct {
    uint8_t app_idx;
} btif_hl_reg_t;

typedef btif_hl_reg_t btif_hl_unreg_t;
typedef btif_hl_reg_t btif_hl_update_mdl_t;

typedef union {
    btif_hl_send_chan_state_cb_t    chan_cb;
    btif_hl_reg_t           reg;
    btif_hl_unreg_t         unreg;
    btif_hl_update_mdl_t    update_mdl;
} btif_hl_evt_cb_t;

/*******************************************************************************
**  Functions
********************************************************************************/

#define BTIF_HL_GET_CB_PTR() &(btif_hl_cb)
#define BTIF_HL_GET_APP_CB_PTR(app_idx) &(btif_hl_cb.acb[(app_idx)])
#define BTIF_HL_GET_MCL_CB_PTR(app_idx, mcl_idx) &(btif_hl_cb.acb[(app_idx)].mcb[(mcl_idx)])
#define BTIF_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx) &(btif_hl_cb.acb[(app_idx)].mcb[(mcl_idx)].mdl[mdl_idx])
#define BTIF_HL_GET_PCB_PTR(app_idx, mcl_idx) &(btif_hl_cb.acb[app_idx].mcb[mcl_idx].pcb)
#define BTIF_HL_GET_MDL_CFG_PTR(app_idx, item_idx) &(btif_hl_cb.acb[(app_idx)].mdl_cfg[(item_idx)])
#define BTIF_HL_GET_MDL_CFG_CHANNEL_ID_PTR(app_idx, item_idx) &(btif_hl_cb.acb[(app_idx)].mdl_cfg_channel_id[(item_idx)])

extern btif_hl_cb_t  btif_hl_cb;
extern btif_hl_cb_t *p_btif_hl_cb;

extern uint8_t btif_hl_find_mcl_idx(uint8_t app_idx, BD_ADDR p_bd_addr, uint8_t *p_mcl_idx);
extern uint8_t btif_hl_find_app_idx(uint8_t app_id, uint8_t *p_app_idx);
extern uint8_t btif_hl_find_avail_mcl_idx(uint8_t app_idx, uint8_t *p_mcl_idx);
extern uint8_t btif_hl_find_avail_mdl_idx(uint8_t app_idx, uint8_t mcl_idx,
        uint8_t *p_mdl_idx);
extern uint8_t btif_hl_find_mcl_idx_using_handle(tBTA_HL_MCL_HANDLE mcl_handle,
        uint8_t *p_app_idx, uint8_t *p_mcl_idx);
extern uint8_t  btif_hl_save_mdl_cfg(uint8_t app_id, uint8_t item_idx, tBTA_HL_MDL_CFG *p_mdl_cfg);
extern uint8_t  btif_hl_delete_mdl_cfg(uint8_t app_id, uint8_t item_idx);
extern uint8_t btif_hl_find_mdl_idx_using_handle(tBTA_HL_MDL_HANDLE mdl_handle,
        uint8_t *p_app_idx, uint8_t *p_mcl_idx,
        uint8_t *p_mdl_idx);
extern void btif_hl_abort_pending_chan_setup(uint8_t app_idx, uint8_t mcl_idx);
extern uint8_t btif_hl_proc_pending_op(uint8_t app_idx, uint8_t mcl_idx);
extern uint8_t btif_hl_load_mdl_config(uint8_t app_id, uint8_t buffer_size,
                                       tBTA_HL_MDL_CFG *p_mdl_buf);
#endif
