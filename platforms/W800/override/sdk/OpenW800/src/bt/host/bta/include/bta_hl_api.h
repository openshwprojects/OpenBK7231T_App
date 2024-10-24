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

/******************************************************************************
 *
 *  This is the public interface file for the HeaLth device profile (HL)
 *  subsystem of BTA, Broadcom's Bluetooth application layer for mobile
 *  phones.
 *
 ******************************************************************************/
#ifndef BTA_HL_API_H
#define BTA_HL_API_H

#include "bta_api.h"
#include "btm_api.h"
#include "mca_api.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/
/* Extra Debug Code */
#ifndef BTA_HL_DEBUG
#define BTA_HL_DEBUG           TRUE
#endif

#ifndef BTA_HL_NUM_APPS
#define BTA_HL_NUM_APPS                 12
#endif

#ifndef BTA_HL_NUM_MDEPS
#define BTA_HL_NUM_MDEPS                13
#endif

#ifndef BTA_HL_NUM_MCLS
#define BTA_HL_NUM_MCLS                 7
#endif

#ifndef BTA_HL_NUM_MDLS_PER_MDEP
#define BTA_HL_NUM_MDLS_PER_MDEP        4
#endif

#ifndef BTA_HL_NUM_MDLS_PER_MCL
#define BTA_HL_NUM_MDLS_PER_MCL         10
#endif

#ifndef BTA_HL_NUM_DATA_TYPES
#define BTA_HL_NUM_DATA_TYPES            5   /* maximum number of data types can be supported
    per MDEP ID */
#endif

#define BTA_HL_MCAP_RSP_TOUT            2    /* 2 seconds */

#ifndef BTA_HL_CCH_NUM_FILTER_ELEMS
#define BTA_HL_CCH_NUM_FILTER_ELEMS     3
#endif

#ifndef BTA_HL_NUM_SDP_CBACKS
#define BTA_HL_NUM_SDP_CBACKS           7
#endif

#ifndef BTA_HL_NUM_SDP_RECS
#define BTA_HL_NUM_SDP_RECS             5
#endif

#ifndef BTA_HL_NUM_SDP_MDEPS
#define BTA_HL_NUM_SDP_MDEPS            12
#endif

#ifndef BTA_HL_NUM_SVC_ELEMS
#define BTA_HL_NUM_SVC_ELEMS            2
#endif

#ifndef BTA_HL_NUM_PROTO_ELEMS
#define BTA_HL_NUM_PROTO_ELEMS          2
#endif

#define BTA_HL_VERSION_01_00            0x0100
#define BTA_HL_NUM_ADD_PROTO_LISTS      1
#define BTA_HL_NUM_ADD_PROTO_ELEMS      2
#define BTA_HL_MDEP_SEQ_SIZE            20
#define BTA_HL_VAL_ARRY_SIZE            320

#ifndef BTA_HL_NUM_MDL_CFGS
#define BTA_HL_NUM_MDL_CFGS             16    /* numer of MDL cfg saved in the persistent memory*/
#endif

#define BTA_HL_NUM_TIMERS               7

#define BTA_HL_CCH_RSP_TOUT             2000
#define BTA_HL_MAX_TIME                 255
#define BTA_HL_MIN_TIME                 1
#define BTA_HL_INVALID_APP_HANDLE       0xFF
#define BTA_HL_INVALID_MCL_HANDLE       0xFF
#define BTA_HL_INVALID_MDL_HANDLE       0xFFFF

#define BTA_HL_STATUS_OK                    0
#define BTA_HL_STATUS_FAIL                  1   /* Used to pass all other errors */
#define BTA_HL_STATUS_ABORTED               2
#define BTA_HL_STATUS_NO_RESOURCE           3
#define BTA_HL_STATUS_LAST_ITEM             4
#define BTA_HL_STATUS_DUPLICATE_APP_ID      5
#define BTA_HL_STATUS_INVALID_APP_HANDLE    6
#define BTA_HL_STATUS_INVALID_MCL_HANDLE    7
#define BTA_HL_STATUS_MCAP_REG_FAIL         8
#define BTA_HL_STATUS_MDEP_CO_FAIL          9
#define BTA_HL_STATUS_ECHO_CO_FAIL          10
#define BTA_HL_STATUS_MDL_CFG_CO_FAIL       11
#define BTA_HL_STATUS_SDP_NO_RESOURCE       12
#define BTA_HL_STATUS_SDP_FAIL              13
#define BTA_HL_STATUS_NO_CCH                14
#define BTA_HL_STATUS_NO_MCL                15

#define BTA_HL_STATUS_NO_FIRST_RELIABLE     17
#define BTA_HL_STATUS_INVALID_DCH_CFG       18
#define BTA_HL_STATUS_INVALID_MDL_HANDLE    19
#define BTA_HL_STATUS_INVALID_BD_ADDR       20
#define BTA_HL_STATUS_INVALID_RECONNECT_CFG 21
#define BTA_HL_STATUS_ECHO_TEST_BUSY        22
#define BTA_HL_STATUS_INVALID_LOCAL_MDEP_ID 23
#define BTA_HL_STATUS_INVALID_MDL_ID        24
#define BTA_HL_STATUS_NO_MDL_ID_FOUND       25
#define BTA_HL_STATUS_DCH_BUSY              26  /* DCH is congested*/
#define BTA_HL_STATUS_INVALID_CTRL_PSM      27
#define BTA_HL_STATUS_DUPLICATE_CCH_OPEN    28

typedef uint8_t tBTA_HL_STATUS;
typedef tMCA_HANDLE tBTA_HL_APP_HANDLE;
typedef tMCA_CL     tBTA_HL_MCL_HANDLE;
typedef tMCA_DL     tBTA_HL_MDL_HANDLE;
enum {
    BTA_HL_DEVICE_TYPE_SINK,
    BTA_HL_DEVICE_TYPE_SOURCE,
    BTA_HL_DEVICE_TYPE_DUAL
};

typedef uint8_t tBTA_HL_DEVICE_TYPE;



#define BTA_HL_SDP_IEEE_11073_20601             0x01

#define BTA_HL_MCAP_SUP_RECONNECT_MASK_INIT          2 /* 0x02 */
#define BTA_HL_MCAP_SUP_RECONNECT_MASK_ACCEPT        4 /* 0x04 */
#define BTA_HL_MCAP_SUP_CSP_MASK_SYNC_SLAVE          0 /* 0x08 */
#define BTA_HL_MCAP_SUP_CSP_MASK_SYNC_MASTER         0 /* 0x10 */

#define BTA_HL_MCAP_SUP_PROC_MASK  (BTA_HL_MCAP_SUP_RECONNECT_MASK_INIT | \
                                    BTA_HL_MCAP_SUP_RECONNECT_MASK_ACCEPT | \
                                    BTA_HL_MCAP_SUP_CSP_MASK_SYNC_SLAVE | \
                                    BTA_HL_MCAP_SUP_CSP_MASK_SYNC_MASTER)
#define BTA_HL_MDEP_ROLE_SOURCE         0x00
#define BTA_HL_MDEP_ROLE_SINK           0x01

typedef uint8_t tBTA_HL_MDEP_ROLE;

#define BTA_HL_MDEP_ROLE_MASK_SOURCE    0x01     /* bit mask */
#define BTA_HL_MDEP_ROLE_MASK_SINK      0x02
typedef uint8_t tBTA_HL_MDEP_ROLE_MASK;


#define BTA_HL_ECHO_TEST_MDEP_ID        0
#define BTA_HL_ECHO_TEST_MDEP_CFG_IDX   0

#define BTA_HL_INVALID_MDEP_ID     0xFF
typedef tMCA_DEP tBTA_HL_MDEP_ID; /* 0 is for echo test,
                                   0x01-0x7F availave for use,
                                   0x80-0xFF reserved*/


#define BTA_HL_DELETE_ALL_MDL_IDS   0xFFFF
#define BTA_HL_MAX_MDL_VAL          0xFEFF
typedef uint16_t tBTA_HL_MDL_ID;  /* 0x0000 reserved,
                                   0x0001-0xFEFF dynamic range,
                                   0xFF00-0xFFFE reserved,
                                   0xFFFF indicates all MDLs*/

#define BTA_HL_MDEP_DESP_LEN       35

#define BTA_HL_DCH_MODE_RELIABLE    0
#define BTA_HL_DCH_MODE_STREAMING   1

typedef uint8_t tBTA_HL_DCH_MODE;

#define BTA_HL_DCH_CFG_NO_PREF      0
#define BTA_HL_DCH_CFG_RELIABLE     1
#define BTA_HL_DCH_CFG_STREAMING    2
#define BTA_HL_DCH_CFG_UNKNOWN      0xFF

typedef uint8_t tBTA_HL_DCH_CFG;

/* The Default DCH CFG for the echo test when the device is a Source */
#define BTA_HL_DEFAULT_ECHO_TEST_SRC_DCH_CFG BTA_HL_DCH_CFG_RELIABLE

#define BTA_HL_DCH_CREATE_RSP_SUCCESS 0
#define BTA_HL_DCH_CREATE_RSP_CFG_REJ 1

typedef uint8_t tBTA_HL_DCH_CREATE_RSP;

#define BTA_HL_MCAP_SUP_PROC_RECONNECT_INIT 0x02
#define BTA_HL_MCAP_SUP_PROC_RECONNECT_APT  0x04
#define BTA_HL_MCAP_SUP_PROC_CSP_SLAVE      0x08
#define BTA_HL_MCAP_SUP_PROC_CSP_MASTER     0x10

typedef uint8_t tBTA_HL_SUP_PROC_MASK;

typedef struct {
    uint16_t                  max_rx_apdu_size;  /* local rcv MTU */
    uint16_t                  max_tx_apdu_size;  /* maximum TX APDU size*/
} tBTA_HL_ECHO_CFG;


typedef struct {
    uint16_t                  data_type;
    uint16_t                  max_rx_apdu_size;  /* local rcv MTU */
    uint16_t                  max_tx_apdu_size;  /* maximum TX APDU size*/
    char                    desp[BTA_HL_MDEP_DESP_LEN + 1];
} tBTA_HL_MDEP_DATA_TYPE_CFG;


typedef struct {
    tBTA_HL_MDEP_ROLE           mdep_role;
    uint8_t                       num_of_mdep_data_types;
    tBTA_HL_MDEP_DATA_TYPE_CFG  data_cfg[BTA_HL_NUM_DATA_TYPES];
} tBTA_HL_MDEP_CFG;

typedef struct {
    tBTA_HL_MDEP_ID         mdep_id;  /* MDEP ID 0x01-0x7F */
    tBTA_HL_MDEP_CFG        mdep_cfg;
    uint8_t                   ori_app_id;
} tBTA_HL_MDEP;

typedef struct {
    tBTA_HL_MDEP            mdep[BTA_HL_NUM_MDEPS];
    tBTA_HL_ECHO_CFG        echo_cfg;
    tBTA_HL_MDEP_ROLE_MASK  app_role_mask;
    uint8_t                 advertize_source_sdp;
    uint8_t                   num_of_mdeps;
} tBTA_HL_SUP_FEATURE;

typedef struct {
    uint8_t                 delete_req_pending;
    tBTA_HL_MDL_ID          mdl_id;
    tBTA_HL_MCL_HANDLE      mcl_handle;
} tBTA_HL_DELETE_MDL;

typedef struct {
    uint8_t                   time;
    uint16_t                  mtu;
    tBTA_HL_MDL_ID          mdl_id;
    tBTA_HL_MDEP_ID         local_mdep_id;
    tBTA_HL_MDEP_ROLE       local_mdep_role;
    uint8_t                 active;     /* true if this item is in use */
    tBTA_HL_DCH_MODE        dch_mode;
    uint8_t                   fcs;
    BD_ADDR                 peer_bd_addr;
} tBTA_HL_MDL_CFG;


/* Maximum number of supported feature list items (list_elem in tSDP_SUP_FEATURE_ELEM) */
#define BTA_HL_NUM_SUP_FEATURE_ELEMS     13
#define BTA_HL_SUP_FEATURE_SDP_BUF_SIZE  512
/* This structure is used to add supported feature lists and find supported feature elements */
typedef struct {
    uint8_t       mdep_id;
    uint16_t      data_type;
    tBTA_HL_MDEP_ROLE       mdep_role;
    char        *p_mdep_desp;
} tBTA_HL_SUP_FEATURE_ELEM;

typedef struct {
    uint16_t                      num_elems;
    tBTA_HL_SUP_FEATURE_ELEM   list_elem[BTA_HL_NUM_SUP_FEATURE_ELEMS];
} tBTA_HL_SUP_FEATURE_LIST_ELEM;


typedef struct {
    tBTA_HL_DEVICE_TYPE     dev_type;           /* sink, source or dual roles */
    tBTA_SEC                sec_mask;           /* security mask for accepting conenction*/
    const char              *p_srv_name;        /* service name to be used in the SDP; null terminated*/
    const char
    *p_srv_desp;        /* service description to be used in the SDP; null terminated */
    const char
    *p_provider_name;   /* provide name to be used in the SDP; null terminated */
} tBTA_HL_REG_PARAM;

typedef struct {
    uint16_t                  ctrl_psm;
    BD_ADDR                 bd_addr;           /* Address of peer device */
    tBTA_SEC                sec_mask;          /* security mask for initiating connection*/
} tBTA_HL_CCH_OPEN_PARAM;


typedef struct {
    uint16_t                  ctrl_psm;
    tBTA_HL_MDEP_ID         local_mdep_id;     /* local MDEP ID */
    tBTA_HL_MDEP_ID         peer_mdep_id;      /* peer mdep id */
    tBTA_HL_DCH_CFG         local_cfg;
    tBTA_SEC                sec_mask;          /* security mask for initiating connection*/
} tBTA_HL_DCH_OPEN_PARAM;


typedef struct {
    uint16_t                  ctrl_psm;
    tBTA_HL_MDL_ID          mdl_id;
} tBTA_HL_DCH_RECONNECT_PARAM;


typedef struct {
    uint16_t                  ctrl_psm;
    uint16_t                  pkt_size;
    tBTA_HL_DCH_CFG         local_cfg;
} tBTA_HL_DCH_ECHO_TEST_PARAM;

typedef struct {
    uint16_t                  buf_size;
    uint8_t                   p_buf;        /* buffer pointer */
} tBTA_HL_DCH_BUF_INFO;

typedef struct {
    tBTA_HL_MDEP_ID         local_mdep_id;  /* local MDEP ID */
    tBTA_HL_MDL_ID          mdl_id;
    tBTA_HL_DCH_CREATE_RSP  rsp_code;
    tBTA_HL_DCH_CFG         cfg_rsp;
} tBTA_HL_DCH_CREATE_RSP_PARAM;

typedef struct {
    uint16_t              data_type;
    uint8_t               mdep_id;
    tBTA_HL_MDEP_ROLE   mdep_role;
    char                mdep_desp[BTA_HL_MDEP_DESP_LEN + 1];
} tBTA_HL_SDP_MDEP_CFG;

typedef struct {
    uint16_t                  ctrl_psm;
    uint16_t                  data_psm;
    uint8_t                   mcap_sup_proc;
    uint8_t                   num_mdeps; /* number of mdep elements from SDP*/
    char                    srv_name[BTA_SERVICE_NAME_LEN + 1];
    char                    srv_desp[BTA_SERVICE_DESP_LEN + 1];
    char                    provider_name[BTA_PROVIDER_NAME_LEN + 1];
    tBTA_HL_SDP_MDEP_CFG    mdep_cfg[BTA_HL_NUM_SDP_MDEPS];
} tBTA_HL_SDP_REC;

typedef struct {
    uint8_t                num_recs;
    tBTA_HL_SDP_REC      sdp_rec[BTA_HL_NUM_SDP_RECS];
} tBTA_HL_SDP;

/* HL control callback function events */
enum {
    BTA_HL_CTRL_ENABLE_CFM_EVT            = 0,
    BTA_HL_CTRL_DISABLE_CFM_EVT
};
typedef uint8_t tBTA_HL_CTRL_EVT;
/* Structure associated with BTA_HL_ENABLE_EVT
   BTA_HL_DISABLE_EVT */

typedef struct {
    tBTA_HL_STATUS  status;
} tBTA_HL_CTRL_ENABLE_DISABLE;

typedef union {
    tBTA_HL_CTRL_ENABLE_DISABLE  enable_cfm;
    tBTA_HL_CTRL_ENABLE_DISABLE  disable_cfm;
} tBTA_HL_CTRL;

/* HL instance callback function events */
enum {
    BTA_HL_REGISTER_CFM_EVT           = 0,
    BTA_HL_DEREGISTER_CFM_EVT,
    BTA_HL_CCH_OPEN_IND_EVT,
    BTA_HL_CCH_OPEN_CFM_EVT,
    BTA_HL_CCH_CLOSE_IND_EVT,
    BTA_HL_CCH_CLOSE_CFM_EVT,
    BTA_HL_DCH_CREATE_IND_EVT,
    BTA_HL_DCH_OPEN_IND_EVT,
    BTA_HL_DCH_OPEN_CFM_EVT,
    BTA_HL_DCH_CLOSE_IND_EVT,
    BTA_HL_DCH_CLOSE_CFM_EVT,
    BTA_HL_DCH_RECONNECT_IND_EVT,
    BTA_HL_DCH_RECONNECT_CFM_EVT,

    BTA_HL_DCH_ABORT_IND_EVT,
    BTA_HL_DCH_ABORT_CFM_EVT,
    BTA_HL_DELETE_MDL_IND_EVT,
    BTA_HL_DELETE_MDL_CFM_EVT,
    BTA_HL_DCH_SEND_DATA_CFM_EVT,
    BTA_HL_DCH_RCV_DATA_IND_EVT,
    BTA_HL_CONG_CHG_IND_EVT,
    BTA_HL_DCH_ECHO_TEST_CFM_EVT,
    BTA_HL_SDP_QUERY_CFM_EVT,
    BTA_HL_SDP_INFO_IND_EVT
};
typedef uint8_t tBTA_HL_EVT;


typedef struct {
    tBTA_HL_STATUS          status;        /* start status */
    uint8_t                   app_id;
    tBTA_HL_APP_HANDLE      app_handle;
} tBTA_HL_REGISTER_CFM;


typedef struct {
    tBTA_HL_STATUS          status;        /* start status */
    uint8_t                   app_id;
    tBTA_HL_APP_HANDLE      app_handle;
} tBTA_HL_DEREGISTER_CFM;


typedef struct {
    uint8_t                 intentional;
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_APP_HANDLE      app_handle;
} tBTA_HL_CCH_CLOSE_IND;


typedef struct {
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_APP_HANDLE      app_handle;
} tBTA_HL_MCL_IND;

typedef struct {
    tBTA_HL_STATUS          status;             /* connection status */
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_APP_HANDLE      app_handle;
} tBTA_HL_MCL_CFM;

typedef struct {
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_APP_HANDLE      app_handle;
    BD_ADDR                 bd_addr; /* address of peer device */
} tBTA_HL_CCH_OPEN_IND;

typedef struct {
    tBTA_HL_STATUS          status;             /* connection status */
    uint8_t                   app_id;
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_APP_HANDLE      app_handle;
    BD_ADDR                 bd_addr;            /* address of peer device */
} tBTA_HL_CCH_OPEN_CFM;

typedef struct {
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_APP_HANDLE      app_handle;
    tBTA_HL_MDEP_ID         local_mdep_id;
    tBTA_HL_MDL_ID          mdl_id;             /* MCAP data link ID for this
                                                   data channel conenction    */
    tBTA_HL_DCH_CFG         cfg;                /* dch cfg requested by the peer device */
    BD_ADDR                 bd_addr; /* address of peer device */

} tBTA_HL_DCH_CREATE_IND;

typedef struct {
    tBTA_HL_MDL_HANDLE      mdl_handle;
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_APP_HANDLE      app_handle;
    tBTA_HL_MDEP_ID         local_mdep_id;
    tBTA_HL_MDL_ID          mdl_id;             /* MCAP data link ID for this
                                                   data channel conenction    */
    tBTA_HL_DCH_MODE        dch_mode;           /* data channel mode - reliable or streaming*/

    uint8_t                 first_reliable;  /* whether this is the first reliable data channel */
    uint16_t                  mtu;
} tBTA_HL_DCH_OPEN_IND;

typedef struct {
    tBTA_HL_STATUS          status;             /* connection status */
    tBTA_HL_MDL_HANDLE      mdl_handle;
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_APP_HANDLE      app_handle;
    tBTA_HL_MDEP_ID         local_mdep_id;
    tBTA_HL_MDL_ID          mdl_id;             /* MCAP data link ID for this
                                                   data channel conenction    */
    tBTA_HL_DCH_MODE        dch_mode;           /* data channel mode - reliable or streaming*/
    uint8_t                 first_reliable;     /* whether this is the first reliable data channel */
    uint16_t                  mtu;
} tBTA_HL_DCH_OPEN_CFM;


typedef struct {
    uint8_t                 intentional;
    tBTA_HL_MDL_HANDLE      mdl_handle;
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_APP_HANDLE      app_handle;
} tBTA_HL_DCH_CLOSE_IND;


typedef struct {
    tBTA_HL_MDL_HANDLE      mdl_handle;
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_APP_HANDLE      app_handle;
} tBTA_HL_MDL_IND;

typedef struct {
    tBTA_HL_STATUS          status;
    tBTA_HL_MDL_HANDLE      mdl_handle;
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_APP_HANDLE      app_handle;
} tBTA_HL_MDL_CFM;

typedef struct {
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_APP_HANDLE      app_handle;
    tBTA_HL_MDL_ID          mdl_id;
} tBTA_HL_DELETE_MDL_IND;

typedef struct {
    tBTA_HL_STATUS          status;
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_APP_HANDLE      app_handle;
    tBTA_HL_MDL_ID          mdl_id;
} tBTA_HL_DELETE_MDL_CFM;

typedef struct {
    tBTA_HL_MDL_HANDLE      mdl_handle;
    tBTA_HL_MCL_HANDLE      mcl_handle;
    tBTA_HL_APP_HANDLE      app_handle;
    uint8_t                 cong;
} tBTA_HL_DCH_CONG_IND;

typedef struct {
    tBTA_HL_APP_HANDLE      app_handle;
    uint16_t                  ctrl_psm;
    uint16_t                  data_psm;
    uint8_t                   data_x_spec;
    uint8_t                   mcap_sup_procs;
} tBTA_HL_SDP_INFO_IND;

typedef struct {
    tBTA_HL_STATUS          status;
    uint8_t                   app_id;
    tBTA_HL_APP_HANDLE      app_handle;
    BD_ADDR                 bd_addr;
    tBTA_HL_SDP             *p_sdp;
} tBTA_HL_SDP_QUERY_CFM;

typedef union {
    tBTA_HL_REGISTER_CFM        reg_cfm;
    tBTA_HL_DEREGISTER_CFM      dereg_cfm;
    tBTA_HL_CCH_OPEN_IND        cch_open_ind;
    tBTA_HL_CCH_OPEN_CFM        cch_open_cfm;
    tBTA_HL_CCH_CLOSE_IND       cch_close_ind;
    tBTA_HL_MCL_CFM             cch_close_cfm;
    tBTA_HL_DCH_CREATE_IND      dch_create_ind;
    tBTA_HL_DCH_OPEN_IND        dch_open_ind;
    tBTA_HL_DCH_OPEN_CFM        dch_open_cfm;
    tBTA_HL_DCH_CLOSE_IND       dch_close_ind;
    tBTA_HL_MDL_CFM             dch_close_cfm;
    tBTA_HL_DCH_OPEN_IND        dch_reconnect_ind;
    tBTA_HL_DCH_OPEN_CFM        dch_reconnect_cfm;
    tBTA_HL_MCL_IND             dch_abort_ind;
    tBTA_HL_MCL_CFM             dch_abort_cfm;
    tBTA_HL_DELETE_MDL_IND      delete_mdl_ind;
    tBTA_HL_DELETE_MDL_CFM      delete_mdl_cfm;
    tBTA_HL_MDL_CFM             dch_send_data_cfm;
    tBTA_HL_MDL_IND             dch_rcv_data_ind;
    tBTA_HL_DCH_CONG_IND        dch_cong_ind;
    tBTA_HL_MCL_CFM             echo_test_cfm;
    tBTA_HL_SDP_QUERY_CFM       sdp_query_cfm;
    tBTA_HL_SDP_INFO_IND        sdp_info_ind;

} tBTA_HL;

/* HL callback functions */
typedef void tBTA_HL_CTRL_CBACK(tBTA_HL_CTRL_EVT event, tBTA_HL_CTRL *p_data);
typedef void tBTA_HL_CBACK(tBTA_HL_EVT event, tBTA_HL *p_data);


/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/**************************
**  API Functions
***************************/

/*******************************************************************************
**
** Function         BTA_HlEnable
**
** Description      Enable the HL subsystems.  This function must be
**                  called before any other functions in the HL API are called.
**                  When the enable operation is completed the callback function
**                  will be called with an BTA_HL_CTRL_ENABLE_CFM_EVT event.
**
** Parameters       p_cback - HL event call back function
**
** Returns          void
**
*******************************************************************************/
extern void BTA_HlEnable(tBTA_HL_CTRL_CBACK *p_ctrl_cback);
/*******************************************************************************
**
** Function         BTA_HlDisable
**
** Description     Disable the HL subsystem.
**
** Returns          void
**
*******************************************************************************/
extern void BTA_HlDisable(void);

/*******************************************************************************
**
** Function         BTA_HlUpdate
**
** Description      Register an HDP application
**
** Parameters       app_id        - Application ID
**                  p_reg_param   - non-platform related parameters for the
**                                  HDP application
**                  p_cback       - HL event callback fucntion
**
** Returns          void
**
*******************************************************************************/
extern void BTA_HlUpdate(uint8_t  app_id,
                         tBTA_HL_REG_PARAM *p_reg_param, uint8_t is_register,
                         tBTA_HL_CBACK *p_cback);

/*******************************************************************************
**
** Function         BTA_HlRegister
**
** Description      Register a HDP application
**
**
** Parameters       app_id        - hdp application ID
**                  p_reg_param   - non-platform related parameters for the
**                                  HDP application
**                  p_cback       - HL event callback fucntion
**
** Returns          void
**
*******************************************************************************/
extern void BTA_HlRegister(uint8_t  app_id,
                           tBTA_HL_REG_PARAM *p_reg_param,
                           tBTA_HL_CBACK *p_cback);

/*******************************************************************************
**
** Function         BTA_HlDeregister
**
** Description      Deregister an HDP application
**
** Parameters       app_handle - Application handle
**
** Returns         void
**
*******************************************************************************/
extern void BTA_HlDeregister(uint8_t app_id, tBTA_HL_APP_HANDLE app_handle);

/*******************************************************************************
**
** Function         BTA_HlCchOpen
**
** Description      Open a Control channel connection with the specified BD address
**                  and the control PSM value is used to select which
**                  HDP insatnce should be used in case the peer device support
**                  multiple HDP instances.
**
**
** Parameters       app_handle - Application Handle
**                  p_open_param - parameters for opening a control channel
**
** Returns          void
**
**                  Note: If the control PSM value is zero then the first HDP
**                        instance is used for the control channel setup
*******************************************************************************/
extern void BTA_HlCchOpen(uint8_t app_id, tBTA_HL_APP_HANDLE app_handle,
                          tBTA_HL_CCH_OPEN_PARAM *p_open_param);

/*******************************************************************************
**
** Function         BTA_HlCchClose
**
** Description      Close a Control channel connection with the specified MCL
**                  handle
**
** Parameters       mcl_handle - MCL handle
**
** Returns          void
**
*******************************************************************************/
extern  void BTA_HlCchClose(tBTA_HL_MCL_HANDLE mcl_handle);

/*******************************************************************************
**
** Function         BTA_HlDchOpen
**
** Description      Open a data channel connection with the specified DCH parameters
**
** Parameters       mcl_handle - MCL handle
**                  p_open_param - parameters for opening a data channel
**
** Returns          void
**
*******************************************************************************/
extern  void BTA_HlDchOpen(tBTA_HL_MCL_HANDLE mcl_handle,
                           tBTA_HL_DCH_OPEN_PARAM *p_open_param);
/*******************************************************************************
**
** Function         BTA_HlDchReconnect
**
** Description      Reconnect a data channel with the specified MDL_ID
**
** Parameters       mcl_handle      - MCL handle
*8                  p_recon_param   - parameters for reconnecting a data channel
**
** Returns          void
**
*******************************************************************************/
extern void BTA_HlDchReconnect(tBTA_HL_MCL_HANDLE mcl_handle,
                               tBTA_HL_DCH_RECONNECT_PARAM *p_recon_param);
/*******************************************************************************
**
** Function         BTA_HlDchClose
**
** Description      Close a data channel with the specified MDL handle
**
** Parameters       mdl_handle  - MDL handle
**
** Returns          void
**
*******************************************************************************/
extern void BTA_HlDchClose(tBTA_HL_MDL_HANDLE mdl_handle);

/*******************************************************************************
**
** Function         BTA_HlDchAbort
**
** Description      Abort the current data channel setup with the specified MCL
**                  handle
**
** Parameters       mcl_handle  - MCL handle
**
**
** Returns          void
**
*******************************************************************************/
extern void BTA_HlDchAbort(tBTA_HL_MCL_HANDLE mcl_handle);

/*******************************************************************************
**
** Function         BTA_HlSendData
**
** Description      Send an APDU to the peer device
**
** Parameters       mdl_handle  - MDL handle
**                  pkt_size    - size of the data packet to be sent
**
** Returns          void
**
*******************************************************************************/
extern void BTA_HlSendData(tBTA_HL_MDL_HANDLE mdl_handle,
                           uint16_t           pkt_size);

/*******************************************************************************
**
** Function         BTA_HlDeleteMdl
**
** Description      Delete the specified MDL_ID within the specified MCL handle
**
** Parameters       mcl_handle  - MCL handle
**                  mdl_id      - MDL ID
**
** Returns          void
**
**                  note: If mdl_id = 0xFFFF then this means to delete all MDLs
**                        and this value can only be used with DeleteMdl request only
**                        not other requests
**
*******************************************************************************/
extern void BTA_HlDeleteMdl(tBTA_HL_MCL_HANDLE mcl_handle,
                            tBTA_HL_MDL_ID mdl_id);

/*******************************************************************************
**
** Function         BTA_HlDchEchoTest
**
** Description      Initiate an echo test with the specified MCL handle
**
** Parameters       mcl_handle           - MCL handle
*8                  p_echo_test_param   -  parameters for echo testing
**
** Returns          void
**
*******************************************************************************/
extern void BTA_HlDchEchoTest(tBTA_HL_MCL_HANDLE  mcl_handle,
                              tBTA_HL_DCH_ECHO_TEST_PARAM *p_echo_test_param);

/*******************************************************************************
**
** Function         BTA_HlSdpQuery
**
** Description      SDP query request for the specified BD address
**
** Parameters       app_id
                        app_handle      - application handle
**                  bd_addr         - BD address
**
** Returns          void
**
*******************************************************************************/
extern  void BTA_HlSdpQuery(uint8_t  app_id, tBTA_HL_APP_HANDLE app_handle,
                            BD_ADDR bd_addr);

/*******************************************************************************
**
** Function         BTA_HlDchCreateMdlRsp
**
** Description      Set the Response and configuration values for the Create MDL
**                  request
**
** Parameters       mcl_handle  - MCL handle
**                  p_rsp_param - parameters specified whether the request should
**                                be accepted or not and if it should be accepted
**                                then it also specified the configuration response
**                                value
**
** Returns          void
**
*******************************************************************************/
extern void BTA_HlDchCreateRsp(tBTA_HL_MCL_HANDLE mcl_handle,
                               tBTA_HL_DCH_CREATE_RSP_PARAM *p_rsp_param);



#ifdef __cplusplus

}
#endif

#endif /* BTA_HL_API_H */
