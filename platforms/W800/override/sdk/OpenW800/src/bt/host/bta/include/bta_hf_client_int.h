/******************************************************************************
 *
 *  Copyright (c) 2014 The Android Open Source Project
 *  Copyright (C) 2003-2012 Broadcom Corporation
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

#include "bta_sys.h"
#include "bta_api.h"
#include "bta_hf_client_api.h"
#include "bta_hf_client_at.h"

/*****************************************************************************
**  Constants
*****************************************************************************/
#define HFP_VERSION_1_1         0x0101
#define HFP_VERSION_1_5         0x0105
#define HFP_VERSION_1_6         0x0106

/* RFCOMM MTU SIZE */
#define BTA_HF_CLIENT_MTU       256

/* profile role for connection */
#define BTA_HF_CLIENT_ACP       0       /* accepted connection */
#define BTA_HF_CLIENT_INT       1       /* initiating connection */

/* Time (in milliseconds) to wait for retry in case of collision */
#ifndef BTA_HF_CLIENT_COLLISION_TIMER_MS
#define BTA_HF_CLIENT_COLLISION_TIMER_MS        2411
#endif

enum {
    /* these events are handled by the state machine */
    BTA_HF_CLIENT_API_REGISTER_EVT = BTA_SYS_EVT_START(BTA_ID_HS),
    BTA_HF_CLIENT_API_DEREGISTER_EVT,
    BTA_HF_CLIENT_API_OPEN_EVT,
    BTA_HF_CLIENT_API_CLOSE_EVT,
    BTA_HF_CLIENT_API_AUDIO_OPEN_EVT,
    BTA_HF_CLIENT_API_AUDIO_CLOSE_EVT,
    BTA_HF_CLIENT_RFC_OPEN_EVT,
    BTA_HF_CLIENT_RFC_CLOSE_EVT,
    BTA_HF_CLIENT_RFC_SRV_CLOSE_EVT,
    BTA_HF_CLIENT_RFC_DATA_EVT,
    BTA_HF_CLIENT_DISC_ACP_RES_EVT,
    BTA_HF_CLIENT_DISC_INT_RES_EVT,
    BTA_HF_CLIENT_DISC_OK_EVT,
    BTA_HF_CLIENT_DISC_FAIL_EVT,
    BTA_HF_CLIENT_SCO_OPEN_EVT,
    BTA_HF_CLIENT_SCO_CLOSE_EVT,
    BTA_HF_CLIENT_SEND_AT_CMD_EVT,
#if (BTM_SCO_HCI_INCLUDED == TRUE )
    BTA_HF_CLIENT_CI_SCO_DATA_EVT,
#endif /* (BTM_SCO_HCI_INCLUDED == TRUE ) */
    BTA_HF_CLIENT_MAX_EVT,

    /* these events are handled outside of the state machine */
    BTA_HF_CLIENT_API_ENABLE_EVT,
    BTA_HF_CLIENT_API_DISABLE_EVT
};

/*****************************************************************************
**  Data types
*****************************************************************************/

/* data type for BTA_HF_CLIENT_API_ENABLE_EVT */
typedef struct {
    BT_HDR                     hdr;
    tBTA_HF_CLIENT_CBACK      *p_cback;
} tBTA_HF_CLIENT_API_ENABLE;

/* data type for BTA_HF_CLIENT_API_REGISTER_EVT */
typedef struct {
    BT_HDR                     hdr;
    tBTA_HF_CLIENT_CBACK      *p_cback;
    tBTA_SEC                   sec_mask;
    tBTA_HF_CLIENT_FEAT        features;
    char                       name[BTA_SERVICE_NAME_LEN + 1];
} tBTA_HF_CLIENT_API_REGISTER;

/* data type for BTA_HF_CLIENT_API_OPEN_EVT */
typedef struct {
    BT_HDR              hdr;
    BD_ADDR             bd_addr;
    tBTA_SEC            sec_mask;
} tBTA_HF_CLIENT_API_OPEN;

/* data type for BTA_HF_CLIENT_DISC_RESULT_EVT */
typedef struct {
    BT_HDR          hdr;
    uint16_t          status;
} tBTA_HF_CLIENT_DISC_RESULT;

/* data type for RFCOMM events */
typedef struct {
    BT_HDR          hdr;
    uint16_t          port_handle;
} tBTA_HF_CLIENT_RFC;

/* generic purpose data type for other events */
typedef struct {
    BT_HDR          hdr;
    uint8_t         bool_val;
    uint8_t           uint8_val;
    uint32_t          uint32_val1;
    uint32_t          uint32_val2;
    char            str[BTA_HF_CLIENT_NUMBER_LEN + 1];
} tBTA_HF_CLIENT_DATA_VAL;

/* union of all event datatypes */
typedef union {
    BT_HDR                         hdr;
    tBTA_HF_CLIENT_API_ENABLE      api_enable;
    tBTA_HF_CLIENT_API_REGISTER    api_register;
    tBTA_HF_CLIENT_API_OPEN        api_open;
    tBTA_HF_CLIENT_DISC_RESULT     disc_result;
    tBTA_HF_CLIENT_RFC             rfc;
    tBTA_HF_CLIENT_DATA_VAL        val;

} tBTA_HF_CLIENT_DATA;

/* type for each service control block */
typedef struct {
    uint16_t              serv_handle;    /* RFCOMM server handle */
    BD_ADDR             peer_addr;      /* peer bd address */
    tSDP_DISCOVERY_DB   *p_disc_db;     /* pointer to discovery database */
    uint16_t              conn_handle;    /* RFCOMM handle of connected service */
    tBTA_SEC            serv_sec_mask;  /* server security mask */
    tBTA_SEC            cli_sec_mask;   /* client security mask */
    tBTA_HF_CLIENT_FEAT        features;       /* features registered by application */
    tBTA_HF_CLIENT_PEER_FEAT   peer_features;  /* peer device features */
    tBTA_HF_CLIENT_CHLD_FEAT   chld_features;  /* call handling features */
    uint16_t              peer_version;   /* profile version of peer device */
    uint8_t               peer_scn;       /* peer scn */
    uint8_t               role;           /* initiator/acceptor role */
    uint16_t              sco_idx;        /* SCO handle */
    uint8_t               sco_state;      /* SCO state variable */
    uint8_t             sco_close_rfc;   /* TRUE if also close RFCOMM after SCO */
    uint8_t             retry_with_sco_only;
    uint8_t             deregister;     /* TRUE if service shutting down */
    uint8_t             svc_conn;       /* set to TRUE when service level connection is up */
    uint8_t             send_at_reply;  /* set to TRUE to notify framework about AT results */
    tBTA_HF_CLIENT_AT_CB at_cb;         /* AT Parser control block */
    uint8_t               state;          /* state machine state */
    tBTM_SCO_CODEC_TYPE negotiated_codec; /* negotiated codec */
    TIMER_LIST_ENT      collision_timer; /* Collision timer */
    uint8_t             collision_timer_on; /*Indicate timer state*/
} tBTA_HF_CLIENT_SCB;

/* sco states */
enum {
    BTA_HF_CLIENT_SCO_SHUTDOWN_ST,  /* no listening, no connection */
    BTA_HF_CLIENT_SCO_LISTEN_ST,    /* listening */
    BTA_HF_CLIENT_SCO_OPENING_ST,   /* connection opening */
    BTA_HF_CLIENT_SCO_OPEN_CL_ST,   /* opening connection being closed */
    BTA_HF_CLIENT_SCO_OPEN_ST,      /* open */
    BTA_HF_CLIENT_SCO_CLOSING_ST,   /* closing */
    BTA_HF_CLIENT_SCO_CLOSE_OP_ST,  /* closing sco being opened */
    BTA_HF_CLIENT_SCO_SHUTTING_ST   /* sco shutting down */
};

/* type for AG control block */
typedef struct {
    tBTA_HF_CLIENT_SCB         scb;             /* service control block */
    uint32_t                     sdp_handle;
    uint8_t                      scn;
    tBTA_HF_CLIENT_CBACK       *p_cback;        /* application callback */
    uint8_t                    msbc_enabled;
} tBTA_HF_CLIENT_CB;

/*****************************************************************************
**  Global data
*****************************************************************************/

/* control block declaration */
extern tBTA_HF_CLIENT_CB bta_hf_client_cb;

/*****************************************************************************
**  Function prototypes
*****************************************************************************/

/* main functions */
extern void bta_hf_client_scb_init(void);
extern void bta_hf_client_scb_disable(void);
extern uint8_t bta_hf_client_hdl_event(BT_HDR *p_msg);
extern void bta_hf_client_sm_execute(uint16_t event,
                                     tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_slc_seq(uint8_t error);
extern void bta_hf_client_collision_cback(tBTA_SYS_CONN_STATUS status, uint8_t id,
        uint8_t app_id, BD_ADDR peer_addr);
extern void bta_hf_client_resume_open();

/* SDP functions */
extern uint8_t bta_hf_client_add_record(char *p_service_name,
                                        uint8_t scn, tBTA_HF_CLIENT_FEAT features,
                                        uint32_t sdp_handle);
extern void bta_hf_client_create_record(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_del_record(tBTA_HF_CLIENT_DATA *p_data);
extern uint8_t bta_hf_client_sdp_find_attr(void);
extern void bta_hf_client_do_disc(void);
extern void bta_hf_client_free_db(tBTA_HF_CLIENT_DATA *p_data);

/* RFCOMM functions */
extern void bta_hf_client_setup_port(uint16_t handle);
extern void bta_hf_client_start_server(void);
extern void bta_hf_client_close_server(void);
extern void bta_hf_client_rfc_do_open(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_rfc_do_close(tBTA_HF_CLIENT_DATA *p_data);

/* SCO functions */
extern void bta_hf_client_sco_listen(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_sco_shutdown(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_sco_conn_open(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_sco_conn_close(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_sco_open(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_sco_close(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_cback_sco(uint8_t event);
#if (BTM_SCO_HCI_INCLUDED == TRUE )
extern void bta_hf_client_ci_sco_data(tBTA_HF_CLIENT_DATA *p_data);
#endif


/* AT command functions */
extern void bta_hf_client_at_parse(char *buf, unsigned int len);
extern void bta_hf_client_send_at_brsf(void);
extern void bta_hf_client_send_at_bac(void);
extern void bta_hf_client_send_at_cind(uint8_t status);
extern void bta_hf_client_send_at_cmer(uint8_t activate);
extern void bta_hf_client_send_at_chld(char cmd, uint32_t idx);
extern void bta_hf_client_send_at_clip(uint8_t activate);
extern void bta_hf_client_send_at_ccwa(uint8_t activate);
extern void bta_hf_client_send_at_cmee(uint8_t activate);
extern void bta_hf_client_send_at_cops(uint8_t query);
extern void bta_hf_client_send_at_clcc(void);
extern void bta_hf_client_send_at_bvra(uint8_t enable);
extern void bta_hf_client_send_at_vgs(uint32_t volume);
extern void bta_hf_client_send_at_vgm(uint32_t volume);
extern void bta_hf_client_send_at_atd(char *number, uint32_t memory);
extern void bta_hf_client_send_at_bldn(void);
extern void bta_hf_client_send_at_ata(void);
extern void bta_hf_client_send_at_chup(void);
extern void bta_hf_client_send_at_btrh(uint8_t query, uint32_t val);
extern void bta_hf_client_send_at_vts(char code);
extern void bta_hf_client_send_at_bcc(void);
extern void bta_hf_client_send_at_bcs(uint32_t codec);
extern void bta_hf_client_send_at_cnum(void);
extern void bta_hf_client_send_at_nrec(void);
extern void bta_hf_client_send_at_binp(uint32_t action);
extern void bta_hf_client_send_at_bia(void);

/* Action functions */
extern void bta_hf_client_register(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_deregister(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_start_dereg(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_start_close(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_start_open(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_rfc_acp_open(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_rfc_open(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_rfc_fail(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_disc_fail(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_open_fail(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_rfc_close(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_disc_acp_res(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_rfc_data(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_disc_int_res(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_svc_conn_open(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_ind(tBTA_HF_CLIENT_IND_TYPE type, uint16_t value);
extern void bta_hf_client_evt_val(tBTA_HF_CLIENT_EVT type, uint16_t value);
extern void bta_hf_client_operator_name(char *name);
extern void bta_hf_client_clip(char *number);
extern void bta_hf_client_ccwa(char *number);
extern void bta_hf_client_at_result(tBTA_HF_CLIENT_AT_RESULT_TYPE type, uint16_t cme);
extern void bta_hf_client_clcc(uint32_t idx, uint8_t incoming, uint8_t status, uint8_t mpty,
                               char *number);
extern void bta_hf_client_cnum(char *number, uint16_t service);
extern void bta_hf_client_binp(char *number);

/* Commands handling functions */
extern void bta_hf_client_dial(tBTA_HF_CLIENT_DATA *p_data);
extern void bta_hf_client_send_at_cmd(tBTA_HF_CLIENT_DATA *p_data);
