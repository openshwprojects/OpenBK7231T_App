/******************************************************************************
 *
 *  Copyright (C) 2004-2012 Broadcom Corporation
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
 *  This is the private interface file for the BTA advanced audio/video.
 *
 ******************************************************************************/
#ifndef BTA_AV_INT_H
#define BTA_AV_INT_H

#include "osi/include/list.h"
#include "bta_sys.h"
#include "bta_api.h"
#include "bta_av_api.h"
#include "avdt_api.h"
#include "bta_av_co.h"
#include "gki.h"

#define BTA_AV_DEBUG TRUE

/*****************************************************************************
**  Constants
*****************************************************************************/

enum {
    /* these events are handled by the AV main state machine */
    BTA_AV_API_DISABLE_EVT = BTA_SYS_EVT_START(BTA_ID_AV),
    BTA_AV_API_REMOTE_CMD_EVT,
    BTA_AV_API_VENDOR_CMD_EVT,
    BTA_AV_API_VENDOR_RSP_EVT,
    BTA_AV_API_META_RSP_EVT,
    BTA_AV_API_RC_CLOSE_EVT,
    BTA_AV_AVRC_OPEN_EVT,
    BTA_AV_AVRC_MSG_EVT,
    BTA_AV_AVRC_NONE_EVT,

    /* these events are handled by the AV stream state machine */
    BTA_AV_API_OPEN_EVT,
    BTA_AV_API_CLOSE_EVT,
    BTA_AV_AP_START_EVT,        /* the following 2 events must be in the same order as the *API_*EVT */
    BTA_AV_AP_STOP_EVT,
    BTA_AV_API_RECONFIG_EVT,
    BTA_AV_API_PROTECT_REQ_EVT,
    BTA_AV_API_PROTECT_RSP_EVT,
    BTA_AV_API_RC_OPEN_EVT,
    BTA_AV_SRC_DATA_READY_EVT,
    BTA_AV_CI_SETCONFIG_OK_EVT,
    BTA_AV_CI_SETCONFIG_FAIL_EVT,
    BTA_AV_SDP_DISC_OK_EVT,
    BTA_AV_SDP_DISC_FAIL_EVT,
    BTA_AV_STR_DISC_OK_EVT,
    BTA_AV_STR_DISC_FAIL_EVT,
    BTA_AV_STR_GETCAP_OK_EVT,
    BTA_AV_STR_GETCAP_FAIL_EVT,
    BTA_AV_STR_OPEN_OK_EVT,
    BTA_AV_STR_OPEN_FAIL_EVT,
    BTA_AV_STR_START_OK_EVT,
    BTA_AV_STR_START_FAIL_EVT,
    BTA_AV_STR_CLOSE_EVT,
    BTA_AV_STR_CONFIG_IND_EVT,
    BTA_AV_STR_SECURITY_IND_EVT,
    BTA_AV_STR_SECURITY_CFM_EVT,
    BTA_AV_STR_WRITE_CFM_EVT,
    BTA_AV_STR_SUSPEND_CFM_EVT,
    BTA_AV_STR_RECONFIG_CFM_EVT,
    BTA_AV_AVRC_TIMER_EVT,
    BTA_AV_AVDT_CONNECT_EVT,
    BTA_AV_AVDT_DISCONNECT_EVT,
    BTA_AV_ROLE_CHANGE_EVT,
    BTA_AV_AVDT_DELAY_RPT_EVT,
    BTA_AV_ACP_CONNECT_EVT,
    BTA_AV_API_OFFLOAD_START_EVT,
    BTA_AV_API_OFFLOAD_START_RSP_EVT,

    /* these events are handled outside of the state machine */
    BTA_AV_API_ENABLE_EVT,
    BTA_AV_API_REGISTER_EVT,
    BTA_AV_API_DEREGISTER_EVT,
    BTA_AV_API_DISCONNECT_EVT,
    BTA_AV_CI_SRC_DATA_READY_EVT,
    BTA_AV_SIG_CHG_EVT,
    BTA_AV_SIGNALLING_TIMER_EVT,
    BTA_AV_SDP_AVRC_DISC_EVT,
    BTA_AV_AVRC_CLOSE_EVT,
    BTA_AV_CONN_CHG_EVT,
    BTA_AV_DEREG_COMP_EVT,
#if (BTA_AV_SINK_INCLUDED == TRUE)
    BTA_AV_API_SINK_ENABLE_EVT,
#endif
#if (AVDT_REPORTING == TRUE)
    BTA_AV_AVDT_RPT_CONN_EVT,
#endif
    BTA_AV_API_START_EVT,       /* the following 2 events must be in the same order as the *AP_*EVT */
    BTA_AV_API_STOP_EVT
};

/* events for AV control block state machine */
#define BTA_AV_FIRST_SM_EVT     BTA_AV_API_DISABLE_EVT
#define BTA_AV_LAST_SM_EVT      BTA_AV_AVRC_NONE_EVT

/* events for AV stream control block state machine */
#define BTA_AV_FIRST_SSM_EVT    BTA_AV_API_OPEN_EVT

/* events that do not go through state machine */
#define BTA_AV_FIRST_NSM_EVT    BTA_AV_API_ENABLE_EVT
#define BTA_AV_LAST_NSM_EVT     BTA_AV_API_STOP_EVT

/* API events passed to both SSMs (by bta_av_api_to_ssm) */
#define BTA_AV_FIRST_A2S_API_EVT    BTA_AV_API_START_EVT
#define BTA_AV_FIRST_A2S_SSM_EVT    BTA_AV_AP_START_EVT

#define BTA_AV_LAST_EVT             BTA_AV_API_STOP_EVT

/* maximum number of SEPS in stream discovery results */
#define BTA_AV_NUM_SEPS         32

/* initialization value for AVRC handle */
#define BTA_AV_RC_HANDLE_NONE   0xFF

/* size of database for service discovery */
#define BTA_AV_DISC_BUF_SIZE        1000

/* offset of media type in codec info byte array */
#define BTA_AV_MEDIA_TYPE_IDX       1

/* maximum length of AVDTP security data */
#define BTA_AV_SECURITY_MAX_LEN     400

/* check number of buffers queued at L2CAP when this amount of buffers are queued to L2CAP */
#define BTA_AV_QUEUE_DATA_CHK_NUM   L2CAP_HIGH_PRI_MIN_XMIT_QUOTA

/* the number of ACL links with AVDT */
#define BTA_AV_NUM_LINKS            AVDT_NUM_LINKS

#define BTA_AV_CO_ID_TO_BE_STREAM(p, u32) {*(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)(u32); }
#define BTA_AV_BE_STREAM_TO_CO_ID(u32, p) {u32 = (((uint32_t)(*((p) + 2))) + (((uint32_t)(*((p) + 1))) << 8) + (((uint32_t)(*(p))) << 16)); (p) += 3;}

/* these bits are defined for bta_av_cb.multi_av */
#define BTA_AV_MULTI_AV_SUPPORTED   0x01
#define BTA_AV_MULTI_AV_IN_USE      0x02


/*****************************************************************************
**  Data types
*****************************************************************************/

/* function types for call-out functions */
typedef uint8_t (*tBTA_AV_CO_INIT)(uint8_t *p_codec_type, uint8_t *p_codec_info,
                                   uint8_t *p_num_protect, uint8_t *p_protect_info, uint8_t index);
typedef void (*tBTA_AV_CO_DISC_RES)(tBTA_AV_HNDL hndl, uint8_t num_seps,
                                    uint8_t num_snk, uint8_t num_src, BD_ADDR addr, uint16_t uuid_local);
typedef uint8_t (*tBTA_AV_CO_GETCFG)(tBTA_AV_HNDL hndl, tBTA_AV_CODEC codec_type,
                                     uint8_t *p_codec_info, uint8_t *p_sep_info_idx, uint8_t seid,
                                     uint8_t *p_num_protect, uint8_t *p_protect_info);
typedef void (*tBTA_AV_CO_SETCFG)(tBTA_AV_HNDL hndl, tBTA_AV_CODEC codec_type,
                                  uint8_t *p_codec_info, uint8_t seid, BD_ADDR addr,
                                  uint8_t num_protect, uint8_t *p_protect_info,
                                  uint8_t t_local_sep, uint8_t avdt_handle);
typedef void (*tBTA_AV_CO_OPEN)(tBTA_AV_HNDL hndl,
                                tBTA_AV_CODEC codec_type, uint8_t *p_codec_info,
                                uint16_t mtu);
typedef void (*tBTA_AV_CO_CLOSE)(tBTA_AV_HNDL hndl, tBTA_AV_CODEC codec_type, uint16_t mtu);
typedef void (*tBTA_AV_CO_START)(tBTA_AV_HNDL hndl, tBTA_AV_CODEC codec_type, uint8_t *p_codec_info,
                                 uint8_t *p_no_rtp_hdr);
typedef void (*tBTA_AV_CO_STOP)(tBTA_AV_HNDL hndl, tBTA_AV_CODEC codec_type);
typedef void *(*tBTA_AV_CO_DATAPATH)(tBTA_AV_CODEC codec_type,
                                     uint32_t *p_len, uint32_t *p_timestamp);
typedef void (*tBTA_AV_CO_DELAY)(tBTA_AV_HNDL hndl, uint16_t delay);

/* the call-out functions for one stream */
typedef struct {
    tBTA_AV_CO_INIT     init;
    tBTA_AV_CO_DISC_RES disc_res;
    tBTA_AV_CO_GETCFG   getcfg;
    tBTA_AV_CO_SETCFG   setcfg;
    tBTA_AV_CO_OPEN     open;
    tBTA_AV_CO_CLOSE    close;
    tBTA_AV_CO_START    start;
    tBTA_AV_CO_STOP     stop;
    tBTA_AV_CO_DATAPATH data;
    tBTA_AV_CO_DELAY    delay;
} tBTA_AV_CO_FUNCTS;

/* data type for BTA_AV_API_ENABLE_EVT */
typedef struct {
    BT_HDR              hdr;
    tBTA_AV_CBACK       *p_cback;
    tBTA_AV_FEAT        features;
    tBTA_SEC            sec_mask;
} tBTA_AV_API_ENABLE;

/* data type for BTA_AV_API_REG_EVT */
typedef struct {
    BT_HDR              hdr;
    char                p_service_name[BTA_SERVICE_NAME_LEN + 1];
    uint8_t               app_id;
    tBTA_AV_DATA_CBACK       *p_app_data_cback;
    uint16_t              service_uuid;
} tBTA_AV_API_REG;


enum {
    BTA_AV_RS_NONE,     /* straight API call */
    BTA_AV_RS_OK,       /* the role switch result - successful */
    BTA_AV_RS_FAIL,     /* the role switch result - failed */
    BTA_AV_RS_DONE      /* the role switch is done - continue */
};
typedef uint8_t tBTA_AV_RS_RES;
/* data type for BTA_AV_API_OPEN_EVT */
typedef struct {
    BT_HDR              hdr;
    BD_ADDR             bd_addr;
    uint8_t             use_rc;
    tBTA_SEC            sec_mask;
    tBTA_AV_RS_RES      switch_res;
    uint16_t              uuid;  /* uuid of initiator */
} tBTA_AV_API_OPEN;

/* data type for BTA_AV_API_STOP_EVT */
typedef struct {
    BT_HDR              hdr;
    uint8_t             suspend;
    uint8_t             flush;
} tBTA_AV_API_STOP;

/* data type for BTA_AV_API_DISCONNECT_EVT */
typedef struct {
    BT_HDR              hdr;
    BD_ADDR             bd_addr;
} tBTA_AV_API_DISCNT;

/* data type for BTA_AV_API_PROTECT_REQ_EVT */
typedef struct {
    BT_HDR              hdr;
    uint8_t               *p_data;
    uint16_t              len;
} tBTA_AV_API_PROTECT_REQ;

/* data type for BTA_AV_API_PROTECT_RSP_EVT */
typedef struct {
    BT_HDR              hdr;
    uint8_t               *p_data;
    uint16_t              len;
    uint8_t               error_code;
} tBTA_AV_API_PROTECT_RSP;

/* data type for BTA_AV_API_REMOTE_CMD_EVT */
typedef struct {
    BT_HDR              hdr;
    tAVRC_MSG_PASS      msg;
    uint8_t               label;
} tBTA_AV_API_REMOTE_CMD;

/* data type for BTA_AV_API_VENDOR_CMD_EVT and RSP */
typedef struct {
    BT_HDR              hdr;
    tAVRC_MSG_VENDOR    msg;
    uint8_t               label;
} tBTA_AV_API_VENDOR;

/* data type for BTA_AV_API_RC_OPEN_EVT */
typedef struct {
    BT_HDR              hdr;
} tBTA_AV_API_OPEN_RC;

/* data type for BTA_AV_API_RC_CLOSE_EVT */
typedef struct {
    BT_HDR              hdr;
} tBTA_AV_API_CLOSE_RC;

/* data type for BTA_AV_API_META_RSP_EVT */
typedef struct {
    BT_HDR              hdr;
    uint8_t             is_rsp;
    uint8_t               label;
    tBTA_AV_CODE        rsp_code;
    BT_HDR              *p_pkt;
} tBTA_AV_API_META_RSP;


/* data type for BTA_AV_API_RECONFIG_EVT */
typedef struct {
    BT_HDR              hdr;
    uint8_t               codec_info[AVDT_CODEC_SIZE];    /* codec configuration */
    uint8_t               *p_protect_info;
    uint8_t               num_protect;
    uint8_t             suspend;
    uint8_t               sep_info_idx;
} tBTA_AV_API_RCFG;

/* data type for BTA_AV_CI_SETCONFIG_OK_EVT and BTA_AV_CI_SETCONFIG_FAIL_EVT */
typedef struct {
    BT_HDR              hdr;
    tBTA_AV_HNDL        hndl;
    uint8_t               err_code;
    uint8_t               category;
    uint8_t               num_seid;
    uint8_t               *p_seid;
    uint8_t             recfg_needed;
    uint8_t               avdt_handle;  /* local sep type for which this stream will be set up */
} tBTA_AV_CI_SETCONFIG;

/* data type for all stream events from AVDTP */
typedef struct {
    BT_HDR              hdr;
    tAVDT_CFG           cfg;        /* configuration/capabilities parameters */
    tAVDT_CTRL          msg;        /* AVDTP callback message parameters */
    BD_ADDR             bd_addr;    /* bd address */
    uint8_t               handle;
    uint8_t               avdt_event;
    uint8_t             initiator; /* TRUE, if local device initiates the SUSPEND */
} tBTA_AV_STR_MSG;

/* data type for BTA_AV_AVRC_MSG_EVT */
typedef struct {
    BT_HDR              hdr;
    tAVRC_MSG           msg;
    uint8_t               handle;
    uint8_t               label;
    uint8_t               opcode;
} tBTA_AV_RC_MSG;

/* data type for BTA_AV_AVRC_OPEN_EVT, BTA_AV_AVRC_CLOSE_EVT */
typedef struct {
    BT_HDR              hdr;
    BD_ADDR             peer_addr;
    uint8_t               handle;
} tBTA_AV_RC_CONN_CHG;

/* data type for BTA_AV_CONN_CHG_EVT */
typedef struct {
    BT_HDR              hdr;
    BD_ADDR             peer_addr;
    uint8_t             is_up;
} tBTA_AV_CONN_CHG;

/* data type for BTA_AV_ROLE_CHANGE_EVT */
typedef struct {
    BT_HDR              hdr;
    uint8_t               new_role;
    uint8_t               hci_status;
} tBTA_AV_ROLE_RES;

/* data type for BTA_AV_SDP_DISC_OK_EVT */
typedef struct {
    BT_HDR              hdr;
    uint16_t              avdt_version;   /* AVDTP protocol version */
} tBTA_AV_SDP_RES;

/* data type for BTA_AV_API_OFFLOAD_RSP_EVT */
typedef struct {
    BT_HDR              hdr;
    tBTA_AV_STATUS      status;
} tBTA_AV_API_STATUS_RSP;


/* type for SEP control block */
typedef struct {
    uint8_t               av_handle;         /* AVDTP handle */
    tBTA_AV_CODEC       codec_type;        /* codec type */
    uint8_t               tsep;              /* SEP type of local SEP */
    tBTA_AV_DATA_CBACK  *p_app_data_cback; /* Application callback for media packets */
} tBTA_AV_SEP;


/* initiator/acceptor role for adaption */
#define BTA_AV_ROLE_AD_INT          0x00       /* initiator */
#define BTA_AV_ROLE_AD_ACP          0x01       /* acceptor */

/* initiator/acceptor signaling roles */
#define BTA_AV_ROLE_START_ACP       0x00
#define BTA_AV_ROLE_START_INT       0x10    /* do not change this value */

#define BTA_AV_ROLE_SUSPEND         0x20    /* suspending on start */
#define BTA_AV_ROLE_SUSPEND_OPT     0x40    /* Suspend on Start option is set */

/* union of all event datatypes */
typedef union {
    BT_HDR                  hdr;
    tBTA_AV_API_ENABLE      api_enable;
    tBTA_AV_API_REG         api_reg;
    tBTA_AV_API_OPEN        api_open;
    tBTA_AV_API_STOP        api_stop;
    tBTA_AV_API_DISCNT      api_discnt;
    tBTA_AV_API_PROTECT_REQ api_protect_req;
    tBTA_AV_API_PROTECT_RSP api_protect_rsp;
    tBTA_AV_API_REMOTE_CMD  api_remote_cmd;
    tBTA_AV_API_VENDOR      api_vendor;
    tBTA_AV_API_RCFG        api_reconfig;
    tBTA_AV_CI_SETCONFIG    ci_setconfig;
    tBTA_AV_STR_MSG         str_msg;
    tBTA_AV_RC_MSG          rc_msg;
    tBTA_AV_RC_CONN_CHG     rc_conn_chg;
    tBTA_AV_CONN_CHG        conn_chg;
    tBTA_AV_ROLE_RES        role_res;
    tBTA_AV_SDP_RES         sdp_res;
    tBTA_AV_API_META_RSP    api_meta_rsp;
    tBTA_AV_API_STATUS_RSP  api_status_rsp;
} tBTA_AV_DATA;

typedef void (tBTA_AV_VDP_DATA_ACT)(void *p_scb);

typedef struct {
    tBTA_AV_VDP_DATA_ACT    *p_act;
    uint8_t                   *p_frame;
    uint16_t                  buf_size;
    uint32_t                  len;
    uint32_t                  offset;
    uint32_t                  timestamp;
} tBTA_AV_VF_INFO;

typedef union {
    tBTA_AV_VF_INFO     vdp;            /* used for video channels only */
    tBTA_AV_API_OPEN    open;           /* used only before open and role switch
                                           is needed on another AV channel */
} tBTA_AV_Q_INFO;

#define BTA_AV_Q_TAG_OPEN               0x01 /* after API_OPEN, before STR_OPENED */
#define BTA_AV_Q_TAG_START              0x02 /* before start sending media packets */
#define BTA_AV_Q_TAG_STREAM             0x03 /* during streaming */

#define BTA_AV_WAIT_ACP_CAPS_ON         0x01 /* retriving the peer capabilities */
#define BTA_AV_WAIT_ACP_CAPS_STARTED    0x02 /* started while retriving peer capabilities */
#define BTA_AV_WAIT_ROLE_SW_RES_OPEN    0x04 /* waiting for role switch result after API_OPEN, before STR_OPENED */
#define BTA_AV_WAIT_ROLE_SW_RES_START   0x08 /* waiting for role switch result before streaming */
#define BTA_AV_WAIT_ROLE_SW_STARTED     0x10 /* started while waiting for role switch result */
#define BTA_AV_WAIT_ROLE_SW_RETRY       0x20 /* set when retry on timeout */
#define BTA_AV_WAIT_CHECK_RC            0x40 /* set when the timer is used by role switch */
#define BTA_AV_WAIT_ROLE_SW_FAILED      0x80 /* role switch failed */

#define BTA_AV_WAIT_ROLE_SW_BITS        (BTA_AV_WAIT_ROLE_SW_RES_OPEN|BTA_AV_WAIT_ROLE_SW_RES_START|BTA_AV_WAIT_ROLE_SW_STARTED|BTA_AV_WAIT_ROLE_SW_RETRY)

/* Bitmap for collision, coll_mask */
#define BTA_AV_COLL_INC_TMR             0x01 /* Timer is running for incoming L2C connection */
#define BTA_AV_COLL_API_CALLED          0x02 /* API open was called while incoming timer is running */

/* type for AV stream control block */
typedef struct {
    const tBTA_AV_ACT   *p_act_tbl;     /* the action table for stream state machine */
    const tBTA_AV_CO_FUNCTS *p_cos;     /* the associated callout functions */
    uint8_t             sdp_discovery_started; /* variable to determine whether SDP is started */
    tBTA_AV_SEP         seps[BTA_AV_MAX_SEPS];
    tAVDT_CFG           *p_cap;         /* buffer used for get capabilities */
    list_t              *a2d_list;      /* used for audio channels only */
    tBTA_AV_Q_INFO      q_info;
    tAVDT_SEP_INFO      sep_info[BTA_AV_NUM_SEPS];      /* stream discovery results */
    tAVDT_CFG           cfg;            /* local SEP configuration */
    TIMER_LIST_ENT      avrc_ct_timer; /* delay timer for AVRC CT */
    BD_ADDR             peer_addr;      /* peer BD address */
    uint16_t              l2c_cid;        /* L2CAP channel ID */
    uint16_t              stream_mtu;     /* MTU of stream */
    uint16_t              avdt_version;   /* the avdt version of peer device */
    tBTA_SEC            sec_mask;       /* security mask */
    tBTA_AV_CODEC       codec_type;     /* codec type */
    uint8_t               media_type;     /* Media type */
    uint8_t             cong;           /* TRUE if AVDTP congested */
    tBTA_AV_STATUS      open_status;    /* open failure status */
    tBTA_AV_CHNL        chnl;           /* the channel: audio/video */
    tBTA_AV_HNDL        hndl;           /* the handle: ((hdi + 1)|chnl) */
    uint16_t
    cur_psc_mask;   /* Protocol service capabilities mask for current connection */
    uint8_t               avdt_handle;    /* AVDTP handle */
    uint8_t               hdi;            /* the index to SCB[] */
    uint8_t               num_seps;       /* number of seps returned by stream discovery */
    uint8_t               num_disc_snks;  /* number of discovered snks */
    uint8_t               num_disc_srcs;  /* number of discovered srcs */
    uint8_t               sep_info_idx;   /* current index into sep_info */
    uint8_t               sep_idx;        /* current index into local seps[] */
    uint8_t               rcfg_idx;       /* reconfig requested index into sep_info */
    uint8_t               state;          /* state machine state */
    uint8_t               avdt_label;     /* AVDTP label */
    uint8_t               app_id;         /* application id */
    uint8_t               num_recfg;      /* number of reconfigure sent */
    uint8_t               role;
    uint8_t               l2c_bufs;       /* the number of buffers queued to L2CAP */
    uint8_t               rc_handle;      /* connected AVRCP handle */
    uint8_t             use_rc;         /* TRUE if AVRCP is allowed */
    uint8_t             started;        /* TRUE if stream started */
    uint8_t               co_started;     /* non-zero, if stream started from call-out perspective */
    uint8_t
    recfg_sup;      /* TRUE if the first attempt to reconfigure the stream was successfull, else False if command fails */
    uint8_t
    suspend_sup;    /* TRUE if Suspend stream is supported, else FALSE if suspend command fails */
    uint8_t             deregistring;   /* TRUE if deregistering */
    uint8_t             sco_suspend;    /* TRUE if SUSPEND is issued automatically for SCO */
    uint8_t               coll_mask;      /* Mask to check incoming and outgoing collision */
    tBTA_AV_API_OPEN    open_api;       /* Saved OPEN api message */
    uint8_t               wait;           /* set 0x1, when getting Caps as ACP, set 0x2, when started */
    uint8_t               q_tag;          /* identify the associated q_info union member */
    uint8_t             no_rtp_hdr;     /* TRUE if add no RTP header*/
    uint16_t              uuid_int;       /*intended UUID of Initiator to connect to */
    uint8_t             offload_start_pending;
    uint8_t             skip_sdp;       /* Decides if sdp to be done prior to profile connection */
} tBTA_AV_SCB;

#define BTA_AV_RC_ROLE_MASK     0x10
#define BTA_AV_RC_ROLE_INT      0x00
#define BTA_AV_RC_ROLE_ACP      0x10

#define BTA_AV_RC_CONN_MASK     0x20

/* type for AV RCP control block */
/* index to this control block is the rc handle */
typedef struct {
    uint8_t   status;
    uint8_t   handle;
    uint8_t   shdl;   /* stream handle (hdi + 1) */
    uint8_t   lidx;   /* (index+1) to LCB */
    tBTA_AV_FEAT        peer_features;  /* peer features mask */
} tBTA_AV_RCB;
#define BTA_AV_NUM_RCB      (BTA_AV_NUM_STRS  + 2)

enum {
    BTA_AV_LCB_FREE,
    BTA_AV_LCB_FIND
};

/* type for AV ACL Link control block */
typedef struct {
    BD_ADDR             addr;           /* peer BD address */
    uint8_t               conn_msk;       /* handle mask of connected stream handle */
    uint8_t               lidx;           /* index + 1 */
} tBTA_AV_LCB;

/* type for stream state machine action functions */
typedef void (*tBTA_AV_SACT)(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);


/* type for AV control block */
typedef struct {
    tBTA_AV_SCB         *p_scb[BTA_AV_NUM_STRS];    /* stream control block */
    tSDP_DISCOVERY_DB   *p_disc_db;     /* pointer to discovery database */
    tBTA_AV_CBACK       *p_cback;       /* application callback function */
    tBTA_AV_RCB         rcb[BTA_AV_NUM_RCB];  /* RCB control block */
    tBTA_AV_LCB         lcb[BTA_AV_NUM_LINKS + 1]; /* link control block */
    TIMER_LIST_ENT       link_signalling_timer;
    TIMER_LIST_ENT       accept_signalling_timer; /* timer to monitor signalling when accepting */
    uint32_t              sdp_a2d_handle; /* SDP record handle for audio src */
#if (BTA_AV_SINK_INCLUDED == TRUE)
    uint32_t              sdp_a2d_snk_handle; /* SDP record handle for audio snk */
#endif
    uint32_t              sdp_vdp_handle; /* SDP record handle for video src */
    tBTA_AV_FEAT        features;       /* features mask */
    tBTA_SEC            sec_mask;       /* security mask */
    tBTA_AV_HNDL        handle;         /* the handle for SDP activity */
    uint8_t             disabling;      /* TRUE if api disabled called */
    uint8_t
    disc;           /* (hdi+1) or (rc_handle|BTA_AV_CHNL_MSK) if p_disc_db is in use */
    uint8_t               state;          /* state machine state */
    uint8_t               conn_rc;        /* handle mask of connected RCP channels */
    uint8_t               conn_audio;     /* handle mask of connected audio channels */
    uint8_t               conn_video;     /* handle mask of connected video channels */
    uint8_t               conn_lcb;       /* index mask of used LCBs */
    uint8_t               audio_open_cnt; /* number of connected audio channels */
    uint8_t               reg_audio;      /* handle mask of registered audio channels */
    uint8_t               reg_video;      /* handle mask of registered video channels */
    uint8_t               rc_acp_handle;
    uint8_t               rc_acp_idx;     /* (index + 1) to RCB */
    uint8_t               rs_idx;         /* (index + 1) to SCB for the one waiting for RS on open */
    uint8_t             sco_occupied;   /* TRUE if SCO is being used or call is in progress */
    uint8_t               audio_streams;  /* handle mask of streaming audio channels */
    uint8_t               video_streams;  /* handle mask of streaming video channels */
} tBTA_AV_CB;



/*****************************************************************************
**  Global data
*****************************************************************************/

/* control block declaration */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_AV_CB bta_av_cb;
#else
extern tBTA_AV_CB *bta_av_cb_ptr;
#define bta_av_cb (*bta_av_cb_ptr)
#endif

/* config struct */
extern tBTA_AV_CFG *p_bta_av_cfg;
extern const tBTA_AV_CFG bta_avk_cfg;
extern const tBTA_AV_CFG bta_av_cfg;

/* rc id config struct */
extern uint16_t *p_bta_av_rc_id;
extern uint16_t *p_bta_av_rc_id_ac;

extern const tBTA_AV_SACT bta_av_a2d_action[];
extern const tBTA_AV_CO_FUNCTS bta_av_a2d_cos;
extern const tBTA_AV_SACT bta_av_vdp_action[];
extern tAVDT_CTRL_CBACK *const bta_av_dt_cback[];
extern void bta_av_stream_data_cback(uint8_t handle, BT_HDR *p_pkt, uint32_t time_stamp,
                                     uint8_t m_pt);

/*****************************************************************************
**  Function prototypes
*****************************************************************************/
/* utility functions */
extern tBTA_AV_SCB *bta_av_hndl_to_scb(uint16_t handle);
extern uint8_t bta_av_chk_start(tBTA_AV_SCB *p_scb);
extern void bta_av_restore_switch(void);
extern uint16_t bta_av_chk_mtu(tBTA_AV_SCB *p_scb, uint16_t mtu);
extern void bta_av_conn_cback(uint8_t handle, BD_ADDR bd_addr, uint8_t event, tAVDT_CTRL *p_data);
extern uint8_t bta_av_rc_create(tBTA_AV_CB *p_cb, uint8_t role, uint8_t shdl, uint8_t lidx);
extern void bta_av_stream_chg(tBTA_AV_SCB *p_scb, uint8_t started);
extern uint8_t bta_av_is_scb_opening(tBTA_AV_SCB *p_scb);
extern uint8_t bta_av_is_scb_incoming(tBTA_AV_SCB *p_scb);
extern void bta_av_set_scb_sst_init(tBTA_AV_SCB *p_scb);
extern uint8_t bta_av_is_scb_init(tBTA_AV_SCB *p_scb);
extern void bta_av_set_scb_sst_incoming(tBTA_AV_SCB *p_scb);
extern tBTA_AV_LCB *bta_av_find_lcb(BD_ADDR addr, uint8_t op);


/* main functions */
extern void bta_av_api_deregister(tBTA_AV_DATA *p_data);
extern void bta_av_dup_audio_buf(tBTA_AV_SCB *p_scb, BT_HDR *p_buf);
extern void bta_av_sm_execute(tBTA_AV_CB *p_cb, uint16_t event, tBTA_AV_DATA *p_data);
extern void bta_av_ssm_execute(tBTA_AV_SCB *p_scb, uint16_t event, tBTA_AV_DATA *p_data);
extern uint8_t bta_av_hdl_event(BT_HDR *p_msg);
#if (defined(BTA_AV_DEBUG) && BTA_AV_DEBUG == TRUE)
extern char *bta_av_evt_code(uint16_t evt_code);
#endif
extern uint8_t bta_av_switch_if_needed(tBTA_AV_SCB *p_scb);
extern uint8_t bta_av_link_role_ok(tBTA_AV_SCB *p_scb, uint8_t bits);
extern uint8_t bta_av_is_rcfg_sst(tBTA_AV_SCB *p_scb);

/* nsm action functions */
extern void bta_av_api_disconnect(tBTA_AV_DATA *p_data);
extern void bta_av_sig_chg(tBTA_AV_DATA *p_data);
extern void bta_av_signalling_timer(tBTA_AV_DATA *p_data);
extern void bta_av_rc_disc_done(tBTA_AV_DATA *p_data);
extern void bta_av_rc_closed(tBTA_AV_DATA *p_data);
extern void bta_av_rc_disc(uint8_t disc);
extern void bta_av_conn_chg(tBTA_AV_DATA *p_data);
extern void bta_av_dereg_comp(tBTA_AV_DATA *p_data);

/* sm action functions */
extern void bta_av_disable(tBTA_AV_CB *p_cb, tBTA_AV_DATA *p_data);
extern void bta_av_rc_opened(tBTA_AV_CB *p_cb, tBTA_AV_DATA *p_data);
extern void bta_av_rc_remote_cmd(tBTA_AV_CB *p_cb, tBTA_AV_DATA *p_data);
extern void bta_av_rc_vendor_cmd(tBTA_AV_CB *p_cb, tBTA_AV_DATA *p_data);
extern void bta_av_rc_vendor_rsp(tBTA_AV_CB *p_cb, tBTA_AV_DATA *p_data);
extern void bta_av_rc_msg(tBTA_AV_CB *p_cb, tBTA_AV_DATA *p_data);
extern void bta_av_rc_close(tBTA_AV_CB *p_cb, tBTA_AV_DATA *p_data);
extern void bta_av_rc_meta_rsp(tBTA_AV_CB *p_cb, tBTA_AV_DATA *p_data);
extern void bta_av_rc_free_rsp(tBTA_AV_CB *p_cb, tBTA_AV_DATA *p_data);
extern void bta_av_rc_free_msg(tBTA_AV_CB *p_cb, tBTA_AV_DATA *p_data);

extern tBTA_AV_RCB *bta_av_get_rcb_by_shdl(uint8_t shdl);
extern void bta_av_del_rc(tBTA_AV_RCB *p_rcb);

/* ssm action functions */
extern void bta_av_do_disc_a2d(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_cleanup(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_free_sdb(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_config_ind(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_disconnect_req(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_security_req(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_security_rsp(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_setconfig_rsp(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_str_opened(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_security_ind(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_security_cfm(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_do_close(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_connect_req(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_sdp_failed(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_disc_results(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_disc_res_as_acp(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_open_failed(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_getcap_results(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_setconfig_rej(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_discover_req(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_conn_failed(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_do_start(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_str_stopped(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_reconfig(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_data_path(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_start_ok(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_start_failed(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_str_closed(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_clr_cong(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_suspend_cfm(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_rcfg_str_ok(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_rcfg_failed(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_rcfg_connect(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_rcfg_discntd(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_suspend_cont(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_rcfg_cfm(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_rcfg_open(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_security_rej(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_open_rc(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_chk_2nd_start(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_save_caps(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_rej_conn(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_rej_conn(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_set_use_rc(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_cco_close(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_switch_role(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_role_res(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_delay_co(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_open_at_inc(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_offload_req(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_offload_rsp(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);


/* ssm action functions - vdp specific */
extern void bta_av_do_disc_vdp(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_vdp_str_opened(tBTA_AV_SCB *p_scb, tBTA_AV_DATA *p_data);
extern void bta_av_reg_vdp(tAVDT_CS *p_cs, char *p_service_name, void *p_data);

#endif /* BTA_AV_INT_H */
