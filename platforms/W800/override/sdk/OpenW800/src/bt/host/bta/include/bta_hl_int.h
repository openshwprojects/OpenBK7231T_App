/******************************************************************************
 *
 *  Copyright (C) 1998-2012 Broadcom Corporation
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

/******************************************************************************
 *
 *  This is the private file for the message access equipment (MSE)
 *  subsystem.
 *
 ******************************************************************************/
#ifndef BTA_HL_INT_H
#define BTA_HL_INT_H

#include "bt_target.h"
#include "bta_sys.h"
#include "bta_hl_api.h"
#include "bta_hl_co.h"
#include "l2cdefs.h"


typedef uint16_t (tBTA_HL_ALLOCATE_PSM)(void);

/*****************************************************************************
**  Constants and data types
*****************************************************************************/

#ifndef BTA_HL_DISC_SIZE
#define BTA_HL_DISC_SIZE                1600
#endif
#define BTA_HL_NUM_SRCH_ATTR            10
#define BTA_HL_MIN_SDP_MDEP_LEN         7

/* L2CAP defualt parameters */
#define BTA_HL_L2C_TX_WIN_SIZE          10
#define BTA_HL_L2C_MAX_TRANSMIT         32
#define BTA_HL_L2C_RTRANS_TOUT          2000
#define BTA_HL_L2C_MON_TOUT             12000
#define BTA_HL_L2C_MPS                  1017

/* L2CAP FCS setting*/
#define BTA_HL_MCA_USE_FCS              MCA_FCS_USE
#define BTA_HL_MCA_NO_FCS               MCA_FCS_BYPASS
#define BTA_HL_L2C_USE_FCS              1
#define BTA_HL_L2C_NO_FCS               0
#define BTA_HL_DEFAULT_SOURCE_FCS       BTA_HL_L2C_USE_FCS
#define BTA_HL_MCA_FCS_USE_MASK         MCA_FCS_USE_MASK

/* SDP Operations */
#define BTA_HL_SDP_OP_NONE                  0
#define BTA_HL_SDP_OP_CCH_INIT              1
#define BTA_HL_SDP_OP_DCH_OPEN_INIT         2
#define BTA_HL_SDP_OP_DCH_RECONNECT_INIT    3
#define BTA_HL_SDP_OP_SDP_QUERY_NEW         4
#define BTA_HL_SDP_OP_SDP_QUERY_CURRENT     5

typedef uint8_t tBTA_HL_SDP_OPER;

/* CCH Operations */
#define     BTA_HL_CCH_OP_NONE            0
#define     BTA_HL_CCH_OP_LOCAL_OPEN      1
#define     BTA_HL_CCH_OP_REMOTE_OPEN     2
#define     BTA_HL_CCH_OP_LOCAL_CLOSE     3
#define     BTA_HL_CCH_OP_REMOTE_CLOSE    4

typedef uint8_t tBTA_HL_CCH_OPER;

/* Pending DCH close operations when closing a CCH */
#define    BTA_HL_CCH_CLOSE_OP_DCH_NONE     0
#define    BTA_HL_CCH_CLOSE_OP_DCH_ABORT    1
#define    BTA_HL_CCH_CLOSE_OP_DCH_CLOSE    2
typedef uint8_t tBTA_HL_CCH_CLOSE_DCH_OPER;

/* DCH Operations */
#define    BTA_HL_DCH_OP_NONE                    0
#define    BTA_HL_DCH_OP_REMOTE_CREATE           1
#define    BTA_HL_DCH_OP_LOCAL_OPEN              2
#define    BTA_HL_DCH_OP_REMOTE_OPEN             3
#define    BTA_HL_DCH_OP_LOCAL_CLOSE             4
#define    BTA_HL_DCH_OP_REMOTE_CLOSE            5
#define    BTA_HL_DCH_OP_LOCAL_DELETE            6
#define    BTA_HL_DCH_OP_REMOTE_DELETE           7
#define    BTA_HL_DCH_OP_LOCAL_RECONNECT         8
#define    BTA_HL_DCH_OP_REMOTE_RECONNECT        9
#define    BTA_HL_DCH_OP_LOCAL_CLOSE_ECHO_TEST   10
#define    BTA_HL_DCH_OP_LOCAL_CLOSE_RECONNECT   11

typedef uint8_t tBTA_HL_DCH_OPER;

/* Echo test Operations */
#define    BTA_HL_ECHO_OP_NONE                 0
#define    BTA_HL_ECHO_OP_CI_GET_ECHO_DATA     1
#define    BTA_HL_ECHO_OP_SDP_INIT             2
#define    BTA_HL_ECHO_OP_MDL_CREATE_CFM       3
#define    BTA_HL_ECHO_OP_DCH_OPEN_CFM         4
#define    BTA_HL_ECHO_OP_LOOP_BACK            5
#define    BTA_HL_ECHO_OP_CI_PUT_ECHO_DATA     6
#define    BTA_HL_ECHO_OP_DCH_CLOSE_CFM        7
#define    BTA_HL_ECHO_OP_OPEN_IND             8
#define    BTA_HL_ECHO_OP_ECHO_PKT             9

typedef uint8_t tBTA_HL_ECHO_OPER;

/* abort status mask for abort_oper */

#define BTA_HL_ABORT_NONE_MASK      0x00
#define BTA_HL_ABORT_PENDING_MASK   0x01
#define BTA_HL_ABORT_LOCAL_MASK     0x10
#define BTA_HL_ABORT_REMOTE_MASK    0x20
#define BTA_HL_ABORT_CCH_CLOSE_MASK 0x40

/* call out mask for cout_oper */
#define BTA_HL_CO_NONE_MASK          0x00
#define BTA_HL_CO_GET_TX_DATA_MASK   0x01
#define BTA_HL_CO_PUT_RX_DATA_MASK   0x02
#define BTA_HL_CO_GET_ECHO_DATA_MASK 0x04
#define BTA_HL_CO_PUT_ECHO_DATA_MASK 0x08

typedef struct {
    uint16_t      mtu;
    uint8_t       fcs;                    /* '0' No FCS, otherwise '1' */
} tBTA_HL_L2CAP_CFG_INFO;


/* State Machine Events */
enum {
    /* these events are handled by the state machine */
    BTA_HL_CCH_OPEN_EVT     = BTA_SYS_EVT_START(BTA_ID_HL),
    BTA_HL_CCH_SDP_OK_EVT,
    BTA_HL_CCH_SDP_FAIL_EVT,
    BTA_HL_MCA_CONNECT_IND_EVT,
    BTA_HL_MCA_DISCONNECT_IND_EVT,
    BTA_HL_CCH_CLOSE_EVT,
    BTA_HL_CCH_CLOSE_CMPL_EVT,
    BTA_HL_MCA_RSP_TOUT_IND_EVT,
    /* DCH EVENT */
    BTA_HL_DCH_SDP_INIT_EVT,
    BTA_HL_DCH_OPEN_EVT,
    BTA_HL_MCA_CREATE_IND_EVT,
    BTA_HL_MCA_CREATE_CFM_EVT,
    BTA_HL_MCA_OPEN_IND_EVT,

    BTA_HL_MCA_OPEN_CFM_EVT,
    BTA_HL_DCH_CLOSE_EVT,
    BTA_HL_MCA_CLOSE_IND_EVT,
    BTA_HL_MCA_CLOSE_CFM_EVT,
    BTA_HL_API_SEND_DATA_EVT,

    BTA_HL_MCA_RCV_DATA_EVT,
    BTA_HL_DCH_CLOSE_CMPL_EVT,
    BTA_HL_DCH_RECONNECT_EVT,
    BTA_HL_DCH_SDP_FAIL_EVT,
    BTA_HL_MCA_RECONNECT_IND_EVT,

    BTA_HL_MCA_RECONNECT_CFM_EVT,
    BTA_HL_DCH_CLOSE_ECHO_TEST_EVT,
    BTA_HL_API_DCH_CREATE_RSP_EVT,
    BTA_HL_DCH_ABORT_EVT,
    BTA_HL_MCA_ABORT_IND_EVT,

    BTA_HL_MCA_ABORT_CFM_EVT,
    BTA_HL_MCA_CONG_CHG_EVT,
    BTA_HL_CI_GET_TX_DATA_EVT,
    BTA_HL_CI_PUT_RX_DATA_EVT,
    BTA_HL_CI_GET_ECHO_DATA_EVT,
    BTA_HL_DCH_ECHO_TEST_EVT,
    BTA_HL_CI_PUT_ECHO_DATA_EVT,

    /* these events are handled outside the state machine */
    BTA_HL_API_ENABLE_EVT,
    BTA_HL_API_DISABLE_EVT,
    BTA_HL_API_UPDATE_EVT,
    BTA_HL_API_REGISTER_EVT,
    BTA_HL_API_DEREGISTER_EVT,
    BTA_HL_API_CCH_OPEN_EVT,
    BTA_HL_API_CCH_CLOSE_EVT,
    BTA_HL_API_DCH_OPEN_EVT,
    BTA_HL_API_DCH_RECONNECT_EVT,
    BTA_HL_API_DCH_CLOSE_EVT,
    BTA_HL_API_DELETE_MDL_EVT,
    BTA_HL_API_DCH_ABORT_EVT,

    BTA_HL_API_DCH_ECHO_TEST_EVT,
    BTA_HL_API_SDP_QUERY_EVT,
    BTA_HL_SDP_QUERY_OK_EVT,
    BTA_HL_SDP_QUERY_FAIL_EVT,
    BTA_HL_MCA_DELETE_IND_EVT,
    BTA_HL_MCA_DELETE_CFM_EVT
};
typedef uint16_t tBTA_HL_INT_EVT;

#define BTA_HL_DCH_EVT_MIN BTA_HL_DCH_SDP_INIT_EVT
#define BTA_HL_DCH_EVT_MAX 0xFFFF


/* state machine states */
enum {
    BTA_HL_CCH_IDLE_ST = 0,      /* Idle  */
    BTA_HL_CCH_OPENING_ST,       /* Opening a connection*/
    BTA_HL_CCH_OPEN_ST,          /* Connection is open */
    BTA_HL_CCH_CLOSING_ST        /* Closing is in progress */
};
typedef uint8_t tBTA_HL_CCH_STATE;

enum {
    BTA_HL_DCH_IDLE_ST = 0,      /* Idle  */
    BTA_HL_DCH_OPENING_ST,       /* Opening a connection*/
    BTA_HL_DCH_OPEN_ST,          /* Connection is open */
    BTA_HL_DCH_CLOSING_ST        /* Closing is in progress */
};
typedef uint8_t tBTA_HL_DCH_STATE;


typedef struct {
    BT_HDR              hdr;
    tBTA_HL_CTRL_CBACK  *p_cback;        /* pointer to control callback function */
} tBTA_HL_API_ENABLE;

typedef struct {
    BT_HDR              hdr;
    uint8_t               app_id;
    uint8_t             is_register;        /* Update HL application due to register or deregister */
    tBTA_HL_CBACK       *p_cback;           /* pointer to application callback function */
    tBTA_HL_DEVICE_TYPE dev_type;           /* sink, source or dual roles */
    tBTA_SEC            sec_mask;           /* security mask for accepting conenction*/
    char                srv_name[BTA_SERVICE_NAME_LEN +
                                                      1];       /* service name to be used in the SDP; null terminated*/
    char                srv_desp[BTA_SERVICE_DESP_LEN +
                                                      1];       /* service description to be used in the SDP; null terminated */
    char                provider_name[BTA_PROVIDER_NAME_LEN +
                                                            1];  /* provide name to be used in the SDP; null terminated */

} tBTA_HL_API_UPDATE;

typedef struct {
    BT_HDR              hdr;
    uint8_t               app_id;
    tBTA_HL_CBACK       *p_cback;        /* pointer to application callback function */
    tBTA_HL_DEVICE_TYPE dev_type;           /* sink, source or dual roles */
    tBTA_SEC            sec_mask;           /* security mask for accepting conenction*/
    char                srv_name[BTA_SERVICE_NAME_LEN +
                                                      1];       /* service name to be used in the SDP; null terminated*/
    char                srv_desp[BTA_SERVICE_DESP_LEN +
                                                      1];       /* service description to be used in the SDP; null terminated */
    char                provider_name[BTA_PROVIDER_NAME_LEN +
                                                            1];  /* provide name to be used in the SDP; null terminated */
} tBTA_HL_API_REGISTER;

typedef struct {
    BT_HDR                  hdr;
    uint8_t                   app_id;
    tBTA_HL_CBACK           *p_cback;        /* pointer to application callback function */
    tBTA_HL_APP_HANDLE      app_handle;
} tBTA_HL_API_DEREGISTER;

typedef struct {
    BT_HDR                  hdr;
    uint8_t                   app_id;
    tBTA_HL_APP_HANDLE      app_handle;
    uint16_t                  ctrl_psm;
    BD_ADDR                 bd_addr;        /* Address of peer device */
    tBTA_SEC                sec_mask;       /* security mask for initiating connection*/
} tBTA_HL_API_CCH_OPEN;


typedef struct {
    BT_HDR                  hdr;
    tBTA_HL_MCL_HANDLE      mcl_handle;
} tBTA_HL_API_CCH_CLOSE;



typedef struct {
    BT_HDR                  hdr;
    tBTA_HL_MCL_HANDLE      mcl_handle;
    uint16_t                  ctrl_psm;
    tBTA_HL_MDEP_ID         local_mdep_id;     /* local MDEP ID */
    tBTA_HL_MDEP_ID         peer_mdep_id;      /* peer mdep id */
    tBTA_HL_DCH_CFG         local_cfg;
    tBTA_SEC                sec_mask;          /* security mask for initiating connection*/
} tBTA_HL_API_DCH_OPEN;


typedef struct {
    BT_HDR                  hdr;

    tBTA_HL_MCL_HANDLE      mcl_handle;
    uint16_t                  ctrl_psm;
    tBTA_HL_MDL_ID          mdl_id;
} tBTA_HL_API_DCH_RECONNECT;

typedef struct {
    BT_HDR                  hdr;
    tBTA_HL_MDL_HANDLE      mdl_handle;
} tBTA_HL_API_DCH_CLOSE;

typedef struct {
    BT_HDR                  hdr;
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_MDL_ID          mdl_id;
} tBTA_HL_API_DELETE_MDL;

typedef struct {
    BT_HDR              hdr;
    tBTA_HL_MCL_HANDLE mcl_handle;
} tBTA_HL_API_DCH_ABORT;


typedef struct {
    BT_HDR              hdr;
    tBTA_HL_MDL_HANDLE  mdl_handle;
    uint16_t              pkt_size;
} tBTA_HL_API_SEND_DATA;

typedef struct {
    BT_HDR              hdr;
    tBTA_HL_MCL_HANDLE  mcl_handle;
    uint16_t              ctrl_psm;
    uint16_t              pkt_size;
    tBTA_HL_DCH_CFG     local_cfg;
} tBTA_HL_API_DCH_ECHO_TEST;

typedef struct {
    BT_HDR                  hdr;
    uint8_t                   app_idx;
    uint8_t                   mcl_idx;
    uint8_t                 release_mcl_cb;
} tBTA_HL_CCH_SDP;


/* MCA callback event parameters. */
typedef struct {
    BT_HDR              hdr;
    tBTA_HL_APP_HANDLE  app_handle;
    tBTA_HL_MCL_HANDLE  mcl_handle;
    tMCA_CTRL       mca_data;
} tBTA_HL_MCA_EVT;


/* MCA callback event parameters. */
typedef struct {
    BT_HDR          hdr;
    uint8_t           app_idx;
    uint8_t           mcl_idx;
    uint8_t           mdl_idx;
    BT_HDR          *p_pkt;
} tBTA_HL_MCA_RCV_DATA_EVT;


typedef struct {
    BT_HDR                  hdr;
    uint8_t                   app_idx;
    uint8_t                   mcl_idx;
    uint8_t                   mdl_idx;
} tBTA_HL_DCH_SDP;

typedef struct {
    BT_HDR                  hdr;
    tBTA_HL_APP_HANDLE      app_handle;
    uint8_t                   app_id;
    BD_ADDR                 bd_addr;        /* Address of peer device */
} tBTA_HL_API_SDP_QUERY;

typedef struct {
    BT_HDR                  hdr;
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_MDL_ID          mdl_id;
    tBTA_HL_MDEP_ID         local_mdep_id;
    tBTA_HL_DCH_CREATE_RSP  rsp_code;
    tBTA_HL_DCH_CFG         cfg_rsp;
} tBTA_HL_API_DCH_CREATE_RSP;

typedef struct {
    BT_HDR                  hdr;
    tBTA_HL_MDL_HANDLE      mdl_handle;
    tBTA_HL_STATUS          status;
} tBTA_HL_CI_GET_PUT_DATA;

typedef struct {
    BT_HDR                  hdr;
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_STATUS          status;
} tBTA_HL_CI_ECHO_DATA;

/* union of all state machine event data types */
typedef union {
    BT_HDR                      hdr;
    tBTA_HL_API_ENABLE          api_enable; /* data for BTA_MSE_API_ENABLE_EVT */
    tBTA_HL_API_UPDATE          api_update;
    tBTA_HL_API_REGISTER        api_reg;
    tBTA_HL_API_DEREGISTER      api_dereg;
    tBTA_HL_API_CCH_OPEN        api_cch_open;
    tBTA_HL_API_CCH_CLOSE       api_cch_close;
    tBTA_HL_API_DCH_CREATE_RSP  api_dch_create_rsp;
    tBTA_HL_API_DCH_OPEN        api_dch_open;
    tBTA_HL_API_DCH_RECONNECT   api_dch_reconnect;
    tBTA_HL_API_DCH_CLOSE       api_dch_close;
    tBTA_HL_API_DELETE_MDL      api_delete_mdl;
    tBTA_HL_API_DCH_ABORT       api_dch_abort;
    tBTA_HL_API_SEND_DATA       api_send_data;
    tBTA_HL_API_DCH_ECHO_TEST   api_dch_echo_test;
    tBTA_HL_API_SDP_QUERY       api_sdp_query;

    tBTA_HL_CCH_SDP             cch_sdp;
    tBTA_HL_MCA_EVT             mca_evt;
    tBTA_HL_MCA_RCV_DATA_EVT    mca_rcv_data_evt;
    tBTA_HL_DCH_SDP             dch_sdp; /* for DCH_OPEN_EVT and DCH_RECONNECT_EVT */
    tBTA_HL_CI_GET_PUT_DATA     ci_get_put_data;
    tBTA_HL_CI_ECHO_DATA        ci_get_put_echo_data;
} tBTA_HL_DATA;


typedef struct {
    uint8_t                 in_use;
    uint16_t                  mdl_id;
    tBTA_HL_MDL_HANDLE      mdl_handle;
    tBTA_HL_DCH_OPER        dch_oper;
    uint8_t                 intentional_close;
    tBTA_HL_DCH_STATE       dch_state;
    uint8_t                   abort_oper;
    uint16_t                  req_data_psm;
    uint16_t                  max_rx_apdu_size;
    uint16_t                  max_tx_apdu_size;
    BT_HDR                  *p_tx_pkt;
    BT_HDR                  *p_rx_pkt;
    tBTA_HL_MDEP_ID         local_mdep_id;
    uint8_t                   local_mdep_cfg_idx;
    tBTA_HL_DCH_CFG         local_cfg;
    tBTA_HL_DCH_CFG         remote_cfg;
    tBTA_HL_MDEP_ID         peer_mdep_id;
    uint16_t  peer_data_type;
    tBTA_HL_MDEP_ROLE       peer_mdep_role;
    tBTA_HL_DCH_MODE        dch_mode;
    tBTA_SEC                sec_mask;
    uint8_t                 is_the_first_reliable;
    uint8_t                 delete_mdl;
    uint16_t                  mtu;
    tMCA_CHNL_CFG           chnl_cfg;
    uint8_t                 mdl_cfg_idx_included;
    uint8_t                   mdl_cfg_idx;
    uint8_t                   echo_oper;
    uint8_t                 cong;
    uint8_t                 close_pending;
    uint8_t                   cout_oper;
    BT_HDR                  *p_echo_tx_pkt;
    BT_HDR                  *p_echo_rx_pkt;
    tBTA_HL_STATUS          ci_put_echo_data_status;
} tBTA_HL_MDL_CB;

typedef struct {
    tBTA_HL_MDL_CB          mdl[BTA_HL_NUM_MDLS_PER_MCL];
    tBTA_HL_DELETE_MDL      delete_mdl;
    uint8_t                 in_use;
    tBTA_HL_CCH_STATE       cch_state;
    uint16_t                  req_ctrl_psm;
    uint16_t                  ctrl_psm;
    uint16_t                  data_psm;
    BD_ADDR                 bd_addr;
    uint16_t                  cch_mtu;
    uint16_t                  sec_mask;
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tSDP_DISCOVERY_DB       *p_db;         /* pointer to discovery database */
    tSDP_DISC_CMPL_CB       *sdp_cback;
    tBTA_HL_SDP_OPER        sdp_oper;
    uint8_t                 close_pending;
    uint8_t                   sdp_mdl_idx;
    tBTA_HL_SDP             sdp;
    uint8_t                   cch_oper;
    uint8_t                   force_close_local_cch_opening;
    uint8_t                 intentional_close;
    uint8_t                 rsp_tout;
    uint8_t                   timer_oper;
    uint8_t                 echo_test;
    uint8_t                   echo_mdl_idx;
    uint8_t                   cch_close_dch_oper;
    uint8_t                   app_id;
} tBTA_HL_MCL_CB;

typedef struct {
    tBTA_HL_MCL_CB       mcb[BTA_HL_NUM_MCLS]; /* application Control Blocks */
    tBTA_HL_CBACK        *p_cback;            /* pointer to control callback function */
    uint8_t              in_use;              /* this CB is in use*/
    uint8_t              deregistering;
    uint8_t                app_id;
    uint32_t               sdp_handle;    /* SDP record handle */
    tBTA_HL_SUP_FEATURE  sup_feature;
    tBTA_HL_MDL_CFG      mdl_cfg[BTA_HL_NUM_MDL_CFGS];
    tBTA_HL_DEVICE_TYPE  dev_type;
    tBTA_HL_APP_HANDLE   app_handle;
    uint16_t               ctrl_psm;   /* L2CAP PSM for the MCAP control channel */
    uint16_t               data_psm;   /* L2CAP PSM for the MCAP data channel */
    uint16_t               sec_mask;   /* Security mask for BTM_SetSecurityLevel() */

    char                 srv_name[BTA_SERVICE_NAME_LEN +
                                                       1];       /* service name to be used in the SDP; null terminated*/
    char                 srv_desp[BTA_SERVICE_DESP_LEN +
                                                       1];       /* service description to be used in the SDP; null terminated */
    char                 provider_name[BTA_PROVIDER_NAME_LEN +
                                                             1];  /* provide name to be used in the SDP; null terminated */

    tMCA_CTRL_CBACK      *p_mcap_cback;            /* pointer to MCAP callback function */
    tMCA_DATA_CBACK      *p_data_cback;
} tBTA_HL_APP_CB;


typedef struct {
    uint8_t             in_use;
    tBTA_HL_SDP_OPER    sdp_oper;
    uint8_t               app_idx;
    uint8_t               mcl_idx;
    uint8_t               mdl_idx;
} tBTA_HL_SDP_CB;

typedef struct {
    uint8_t         in_use;
    uint8_t           app_idx;
    uint8_t           mcl_idx;
} tBTA_HL_TIMER_CB;

typedef struct {
    tBTA_HL_APP_CB        acb[BTA_HL_NUM_APPS];      /* HL Control Blocks */
    tBTA_HL_CTRL_CBACK    *p_ctrl_cback;            /* pointer to control callback function */
    uint8_t               enable;
    uint8_t               disabling;

    tBTA_HL_SDP_CB        scb[BTA_HL_NUM_SDP_CBACKS];
    tBTA_HL_TIMER_CB      tcb[BTA_HL_NUM_TIMERS];
    uint8_t               enable_random_psm;
    tBTA_HL_ALLOCATE_PSM  *p_alloc_psm;
} tBTA_HL_CB;

/******************************************************************************
**  Configuration Definitions
*******************************************************************************/
/* Configuration structure */

/*****************************************************************************
**  Global data
*****************************************************************************/

/* HL control block */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_HL_CB  bta_hl_cb;
#else
extern tBTA_HL_CB *bta_hl_cb_ptr;
#define bta_hl_cb (*bta_hl_cb_ptr)
#endif

#define BTA_HL_GET_CB_PTR() &(bta_hl_cb)
#define BTA_HL_GET_APP_CB_PTR(app_idx) &(bta_hl_cb.acb[(app_idx)])
#define BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx) &(bta_hl_cb.acb[(app_idx)].mcb[(mcl_idx)])
#define BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx) &(bta_hl_cb.acb[(app_idx)].mcb[(mcl_idx)].mdl[mdl_idx])
#define BTA_HL_GET_MDL_CFG_PTR(app_idx, item_idx) &(bta_hl_cb.acb[(app_idx)].mdl_cfg[(item_idx)])
#define BTA_HL_GET_ECHO_CFG_PTR(app_idx)  &(bta_hl_cb.acb[(app_idx)].sup_feature.echo_cfg)
#define BTA_HL_GET_MDEP_CFG_PTR(app_idx, mdep_cfg_idx)  &(bta_hl_cb.acb[(app_idx)].sup_feature.mdep[mdep_cfg_idx].mdep_cfg)
#define BTA_HL_GET_DATA_CFG_PTR(app_idx, mdep_cfg_idx, data_cfg_idx)  \
    &(bta_hl_cb.acb[(app_idx)].sup_feature.mdep[mdep_cfg_idx].mdep_cfg.data_cfg[data_cfg_idx])
#define BTA_HL_GET_BUF_PTR(p_pkt) ((uint8_t *)((uint8_t *) (p_pkt+1) + p_pkt->offset))

/*****************************************************************************
**  Function prototypes
*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
/* main */
extern uint8_t bta_hl_hdl_event(BT_HDR *p_msg);
/* sdp */
extern uint8_t bta_hl_fill_sup_feature_list(const tSDP_DISC_ATTR  *p_attr,
        tBTA_HL_SUP_FEATURE_LIST_ELEM *p_list);
extern tBTA_HL_STATUS bta_hl_sdp_update(uint8_t app_id);
extern tBTA_HL_STATUS bta_hl_sdp_register(uint8_t app_idx);
extern tSDP_DISC_REC *bta_hl_find_sink_or_src_srv_class_in_db(const tSDP_DISCOVERY_DB *p_db,
        const tSDP_DISC_REC *p_start_rec);

/* action routines */
extern void bta_hl_dch_ci_get_tx_data(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                      tBTA_HL_DATA *p_data);
extern void bta_hl_dch_ci_put_rx_data(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                      tBTA_HL_DATA *p_data);
extern void bta_hl_dch_ci_get_echo_data(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                        tBTA_HL_DATA *p_data);

extern void bta_hl_dch_echo_test(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                 tBTA_HL_DATA *p_data);
extern void bta_hl_dch_ci_put_echo_data(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                        tBTA_HL_DATA *p_data);
extern void bta_hl_dch_send_data(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                 tBTA_HL_DATA *p_data);
extern void bta_hl_dch_sdp_fail(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_cong_change(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                       tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_reconnect_ind(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
        tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_reconnect_cfm(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
        tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_reconnect(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                     tBTA_HL_DATA *p_data);
extern void bta_hl_dch_sdp_init(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                tBTA_HL_DATA *p_data);
extern void bta_hl_dch_close_echo_test(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                       tBTA_HL_DATA *p_data);
extern void bta_hl_dch_create_rsp(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                  tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_rcv_data(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                    tBTA_HL_DATA *p_data);
extern void bta_hl_dch_close_cmpl(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                  tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_close_cfm(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                     tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_close_ind(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                     tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_close(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                 tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_delete_ind(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                      tBTA_HL_DATA *p_data);

extern void bta_hl_dch_mca_delete_cfm(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                      tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_delete(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                  tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_abort_ind(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                     tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_abort_cfm(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                     tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_abort(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                 tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_open_ind(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                    tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_open_cfm(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                    tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_create_ind(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                      tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_create_cfm(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                      tBTA_HL_DATA *p_data);
extern void bta_hl_dch_mca_create(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                  tBTA_HL_DATA *p_data);
extern void bta_hl_deallocate_spd_cback(uint8_t sdp_cback_idx);
extern tSDP_DISC_CMPL_CB *bta_hl_allocate_spd_cback(tBTA_HL_SDP_OPER sdp_oper, uint8_t app_idx,
        uint8_t mcl_idx,
        uint8_t mdl_idx,
        uint8_t *p_sdp_cback_idx);
extern tBTA_HL_STATUS bta_hl_init_sdp(tBTA_HL_SDP_OPER sdp_oper, uint8_t app_idx, uint8_t mcl_idx,
                                      uint8_t mdl_idx);
extern void bta_hl_cch_sdp_init(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data);
extern void bta_hl_cch_mca_open(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data);
extern void bta_hl_cch_mca_close(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data);
extern void bta_hl_cch_close_cmpl(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data);
extern void bta_hl_cch_mca_disconnect(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data);
extern void bta_hl_cch_mca_disc_open(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data);
extern void bta_hl_cch_mca_rsp_tout(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data);
extern void bta_hl_cch_mca_connect(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data);

/* State machine drivers  */
extern void bta_hl_cch_sm_execute(uint8_t inst_idx, uint8_t mcl_idx,
                                  uint16_t event, tBTA_HL_DATA *p_data);
extern void bta_hl_dch_sm_execute(uint8_t inst_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                  uint16_t event, tBTA_HL_DATA *p_data);
/* MCAP callback functions  */
extern void bta_hl_mcap_ctrl_cback(tMCA_HANDLE handle, tMCA_CL mcl, uint8_t event,
                                   tMCA_CTRL *p_data);

extern void bta_hl_mcap_data_cback(tMCA_DL mdl, BT_HDR *p_pkt);

/* utility functions  */
extern uint8_t bta_hl_set_ctrl_psm_for_dch(uint8_t app_idx, uint8_t mcl_idx,
        uint8_t mdl_idx, uint16_t ctrl_psm);
extern uint8_t bta_hl_find_sdp_idx_using_ctrl_psm(tBTA_HL_SDP *p_sdp,
        uint16_t ctrl_psm,
        uint8_t *p_sdp_idx);
extern uint16_t bta_hl_set_user_tx_buf_size(uint16_t max_tx_size);
extern uint16_t bta_hl_set_user_rx_buf_size(uint16_t mtu);
extern uint8_t bta_hl_set_tx_win_size(uint16_t mtu, uint16_t mps);
extern uint16_t bta_hl_set_mps(uint16_t mtu);
extern void bta_hl_clean_mdl_cb(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx);
extern BT_HDR *bta_hl_get_buf(uint16_t data_size, uint8_t fcs_use);
extern uint8_t bta_hl_find_service_in_db(uint8_t app_idx, uint8_t mcl_idx,
        uint16_t service_uuid,
        tSDP_DISC_REC **pp_rec);
extern uint16_t bta_hl_get_service_uuids(uint8_t sdp_oper, uint8_t app_idx, uint8_t mcl_idx,
        uint8_t mdl_idx);
extern uint8_t bta_hl_find_echo_cfg_rsp(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdep_idx,
                                        uint8_t cfg,
                                        uint8_t *p_cfg_rsp);
extern uint8_t bta_hl_validate_cfg(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                   uint8_t cfg);
extern uint8_t bta_hl_find_cch_cb_indexes(tBTA_HL_DATA *p_msg,
        uint8_t *p_app_idx,
        uint8_t  *p_mcl_idx);
extern uint8_t bta_hl_find_dch_cb_indexes(tBTA_HL_DATA *p_msg,
        uint8_t *p_app_idx,
        uint8_t *p_mcl_idx,
        uint8_t *p_mdl_idx);
extern uint16_t  bta_hl_allocate_mdl_id(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx);
extern uint8_t bta_hl_find_mdl_idx_using_handle(tBTA_HL_MDL_HANDLE mdl_handle,
        uint8_t *p_app_idx, uint8_t *p_mcl_idx,
        uint8_t *p_mdl_idx);
extern uint8_t bta_hl_find_mdl_idx(uint8_t app_idx, uint8_t mcl_idx, uint16_t mdl_id,
                                   uint8_t *p_mdl_idx);
extern uint8_t bta_hl_find_an_active_mdl_idx(uint8_t app_idx, uint8_t mcl_idx,
        uint8_t *p_mdl_idx);
extern uint8_t bta_hl_find_dch_setup_mdl_idx(uint8_t app_idx, uint8_t mcl_idx,
        uint8_t *p_mdl_idx);
extern uint8_t bta_hl_find_an_in_use_mcl_idx(uint8_t app_idx,
        uint8_t *p_mcl_idx);
extern uint8_t bta_hl_find_an_in_use_app_idx(uint8_t *p_app_idx);
extern uint8_t bta_hl_find_app_idx(uint8_t app_id, uint8_t *p_app_idx);
extern uint8_t bta_hl_find_app_idx_using_handle(tBTA_HL_APP_HANDLE app_handle,
        uint8_t *p_app_idx);
extern uint8_t bta_hl_find_mcl_idx_using_handle(tBTA_HL_MCL_HANDLE mcl_handle,
        uint8_t *p_app_idx, uint8_t *p_mcl_idx);
extern uint8_t bta_hl_find_mcl_idx(uint8_t app_idx, BD_ADDR p_bd_addr, uint8_t *p_mcl_idx);
extern uint8_t bta_hl_is_the_first_reliable_existed(uint8_t app_idx, uint8_t mcl_idx);
extern uint8_t  bta_hl_find_non_active_mdl_cfg(uint8_t app_idx, uint8_t start_mdl_cfg_idx,
        uint8_t *p_mdl_cfg_idx);
extern uint8_t  bta_hl_find_avail_mdl_cfg_idx(uint8_t app_idx, uint8_t mcl_idx,
        uint8_t *p_mdl_cfg_idx);
extern uint8_t  bta_hl_find_mdl_cfg_idx(uint8_t app_idx, uint8_t mcl_idx,
                                        tBTA_HL_MDL_ID mdl_id, uint8_t *p_mdl_cfg_idx);
extern uint8_t  bta_hl_get_cur_time(uint8_t app_idx, uint8_t *p_cur_time);
extern void bta_hl_sort_cfg_time_idx(uint8_t app_idx, uint8_t *a, uint8_t n);
extern void  bta_hl_compact_mdl_cfg_time(uint8_t app_idx, uint8_t mdep_id);
extern uint8_t  bta_hl_is_mdl_exsit_in_mcl(uint8_t app_idx, BD_ADDR bd_addr,
        tBTA_HL_MDL_ID mdl_id);
extern uint8_t  bta_hl_delete_mdl_cfg(uint8_t app_idx, BD_ADDR bd_addr,
                                      tBTA_HL_MDL_ID mdl_id);
extern uint8_t  bta_hl_is_mdl_value_valid(tBTA_HL_MDL_ID mdl_id);
extern uint8_t bta_hl_find_mdep_cfg_idx(uint8_t app_idx,
                                        tBTA_HL_MDEP_ID local_mdep_id, uint8_t *p_mdep_cfg_idx);
extern void bta_hl_find_rxtx_apdu_size(uint8_t app_idx, uint8_t mdep_cfg_idx,
                                       uint16_t *p_rx_apu_size,
                                       uint16_t *p_tx_apu_size);
extern uint8_t bta_hl_validate_peer_cfg(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                        tBTA_HL_MDEP_ID peer_mdep_id,
                                        tBTA_HL_MDEP_ROLE peer_mdep_role,
                                        uint8_t sdp_idx);
extern tBTA_HL_STATUS bta_hl_chk_local_cfg(uint8_t app_idx, uint8_t mcl_idx,
        uint8_t mdep_cfg_idx,
        tBTA_HL_DCH_CFG local_cfg);

extern uint8_t bta_hl_validate_reconnect_params(uint8_t app_idx, uint8_t mcl_idx,
        tBTA_HL_API_DCH_RECONNECT *p_reconnect,
        uint8_t *p_mdep_cfg_idx, uint8_t *p_mdl_cfg_idx);
extern uint8_t bta_hl_find_avail_mcl_idx(uint8_t app_idx, uint8_t *p_mcl_idx);
extern uint8_t bta_hl_find_avail_mdl_idx(uint8_t app_idx, uint8_t mcl_idx,
        uint8_t *p_mdl_idx);
extern uint8_t bta_hl_is_a_duplicate_id(uint8_t app_id);
extern uint8_t bta_hl_find_avail_app_idx(uint8_t *p_idx);
extern tBTA_HL_STATUS bta_hl_app_update(uint8_t app_id, uint8_t is_register);
extern tBTA_HL_STATUS bta_hl_app_registration(uint8_t app_idx);
extern void bta_hl_discard_data(uint16_t event, tBTA_HL_DATA *p_data);
extern void bta_hl_save_mdl_cfg(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx);
extern void bta_hl_set_dch_chan_cfg(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                    tBTA_HL_DATA *p_data);
extern uint8_t bta_hl_get_l2cap_cfg(tBTA_HL_MDL_HANDLE mdl_hnd, tBTA_HL_L2CAP_CFG_INFO *p_cfg);
extern uint8_t bta_hl_validate_chan_cfg(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx);
extern uint8_t bta_hl_is_cong_on(uint8_t app_id, BD_ADDR bd_addr, tBTA_HL_MDL_ID mdl_id);
extern void bta_hl_check_cch_close(uint8_t app_idx, uint8_t mcl_idx,
                                   tBTA_HL_DATA *p_data, uint8_t check_dch_setup);
extern void bta_hl_clean_app(uint8_t app_idx);
extern void bta_hl_check_deregistration(uint8_t app_idx, tBTA_HL_DATA *p_data);
extern void bta_hl_check_disable(tBTA_HL_DATA *p_data);
extern void  bta_hl_build_abort_ind(tBTA_HL *p_evt_data,
                                    tBTA_HL_APP_HANDLE app_handle,
                                    tBTA_HL_MCL_HANDLE mcl_handle);
extern void  bta_hl_build_abort_cfm(tBTA_HL *p_evt_data,
                                    tBTA_HL_APP_HANDLE app_handle,
                                    tBTA_HL_MCL_HANDLE mcl_handle,
                                    tBTA_HL_STATUS status);
extern void  bta_hl_build_dch_close_cfm(tBTA_HL *p_evt_data,
                                        tBTA_HL_APP_HANDLE app_handle,
                                        tBTA_HL_MCL_HANDLE mcl_handle,
                                        tBTA_HL_MDL_HANDLE mdl_handle,
                                        tBTA_HL_STATUS status);
extern void  bta_hl_build_dch_close_ind(tBTA_HL *p_evt_data,
                                        tBTA_HL_APP_HANDLE app_handle,
                                        tBTA_HL_MCL_HANDLE mcl_handle,
                                        tBTA_HL_MDL_HANDLE mdl_handle,
                                        uint8_t intentional);
extern void  bta_hl_build_send_data_cfm(tBTA_HL *p_evt_data,
                                        tBTA_HL_APP_HANDLE app_handle,
                                        tBTA_HL_MCL_HANDLE mcl_handle,
                                        tBTA_HL_MDL_HANDLE mdl_handle,
                                        tBTA_HL_STATUS status);
extern void  bta_hl_build_rcv_data_ind(tBTA_HL *p_evt_data,
                                       tBTA_HL_APP_HANDLE app_handle,
                                       tBTA_HL_MCL_HANDLE mcl_handle,
                                       tBTA_HL_MDL_HANDLE mdl_handle);
extern void  bta_hl_build_cch_open_cfm(tBTA_HL *p_evt_data,
                                       uint8_t app_id,
                                       tBTA_HL_APP_HANDLE app_handle,
                                       tBTA_HL_MCL_HANDLE mcl_handle,
                                       BD_ADDR bd_addr,
                                       tBTA_HL_STATUS status);
extern void  bta_hl_build_cch_open_ind(tBTA_HL *p_evt_data,
                                       tBTA_HL_APP_HANDLE app_handle,
                                       tBTA_HL_MCL_HANDLE mcl_handle,
                                       BD_ADDR bd_addr);
extern void  bta_hl_build_cch_close_cfm(tBTA_HL *p_evt_data,
                                        tBTA_HL_APP_HANDLE app_handle,
                                        tBTA_HL_MCL_HANDLE mcl_handle,
                                        tBTA_HL_STATUS status);
extern void   bta_hl_build_cch_close_ind(tBTA_HL *p_evt_data,
        tBTA_HL_APP_HANDLE app_handle,
        tBTA_HL_MCL_HANDLE mcl_handle,
        uint8_t intentional);

extern void  bta_hl_build_dch_open_cfm(tBTA_HL *p_evt_data,
                                       tBTA_HL_APP_HANDLE app_handle,
                                       tBTA_HL_MCL_HANDLE mcl_handle,
                                       tBTA_HL_MDL_HANDLE mdl_handle,
                                       tBTA_HL_MDEP_ID local_mdep_id,
                                       tBTA_HL_MDL_ID mdl_id,
                                       tBTA_HL_DCH_MODE dch_mode,
                                       uint8_t first_reliable,
                                       uint16_t mtu,
                                       tBTA_HL_STATUS status);

extern void  bta_hl_build_delete_mdl_cfm(tBTA_HL *p_evt_data,
        tBTA_HL_APP_HANDLE app_handle,
        tBTA_HL_MCL_HANDLE mcl_handle,
        tBTA_HL_MDL_ID mdl_id,
        tBTA_HL_STATUS status);
extern void  bta_hl_build_echo_test_cfm(tBTA_HL *p_evt_data,
                                        tBTA_HL_APP_HANDLE app_handle,
                                        tBTA_HL_MCL_HANDLE mcl_handle,
                                        tBTA_HL_STATUS status);
extern void  bta_hl_build_sdp_query_cfm(tBTA_HL *p_evt_data,
                                        uint8_t app_id,
                                        tBTA_HL_APP_HANDLE app_handle,
                                        BD_ADDR bd_addr,
                                        tBTA_HL_SDP *p_sdp,
                                        tBTA_HL_STATUS status);

#if (BTA_HL_DEBUG == TRUE)
extern  char *bta_hl_status_code(tBTA_HL_STATUS status);
extern char *bta_hl_evt_code(tBTA_HL_INT_EVT evt_code);
#endif
#ifdef __cplusplus
}
#endif
#endif /* BTA_MSE_INT_H */


