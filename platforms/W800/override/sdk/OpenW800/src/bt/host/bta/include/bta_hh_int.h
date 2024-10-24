/******************************************************************************
 *
 *  Copyright (C) 2005-2012 Broadcom Corporation
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
 *  This file contains BTA HID Host internal definitions
 *
 ******************************************************************************/

#ifndef BTA_HH_INT_H
#define BTA_HH_INT_H

#include "bta_sys.h"
#include "utl.h"
#include "bta_hh_api.h"

#if BTA_HH_LE_INCLUDED == TRUE
#include "bta_gatt_api.h"
#endif

/* can be moved to bta_api.h */
#define BTA_HH_MAX_RPT_CHARS    8

#if (BTA_GATT_INCLUDED == FALSE || BLE_INCLUDED == FALSE)
#undef BTA_HH_LE_INCLUDED
#define BTA_HH_LE_INCLUDED      FALSE
#endif

/* state machine events, these events are handled by the state machine */
enum {
    BTA_HH_API_OPEN_EVT     = BTA_SYS_EVT_START(BTA_ID_HH),
    BTA_HH_API_CLOSE_EVT,
    BTA_HH_INT_OPEN_EVT,
    BTA_HH_INT_CLOSE_EVT,
    BTA_HH_INT_DATA_EVT,
    BTA_HH_INT_CTRL_DATA,
    BTA_HH_INT_HANDSK_EVT,
    BTA_HH_SDP_CMPL_EVT,
    BTA_HH_API_WRITE_DEV_EVT,
    BTA_HH_API_GET_DSCP_EVT,
    BTA_HH_API_MAINT_DEV_EVT,
    BTA_HH_OPEN_CMPL_EVT,
#if (defined BTA_HH_LE_INCLUDED && BTA_HH_LE_INCLUDED == TRUE)
    BTA_HH_GATT_CLOSE_EVT,
    BTA_HH_GATT_OPEN_EVT,
    BTA_HH_START_ENC_EVT,
    BTA_HH_ENC_CMPL_EVT,
    BTA_HH_GATT_READ_CHAR_CMPL_EVT,
    BTA_HH_GATT_WRITE_CHAR_CMPL_EVT,
    BTA_HH_GATT_READ_DESCR_CMPL_EVT,
    BTA_HH_GATT_WRITE_DESCR_CMPL_EVT,
    BTA_HH_API_SCPP_UPDATE_EVT,
    BTA_HH_GATT_ENC_CMPL_EVT,
#endif

    /* not handled by execute state machine */
    BTA_HH_API_ENABLE_EVT,
    BTA_HH_API_DISABLE_EVT,
    BTA_HH_DISC_CMPL_EVT
};
typedef uint16_t tBTA_HH_INT_EVT;         /* HID host internal events */

#define BTA_HH_INVALID_EVT      (BTA_HH_DISC_CMPL_EVT + 1)

/* event used to map between BTE event and BTA event */
#define BTA_HH_FST_TRANS_CB_EVT         BTA_HH_GET_RPT_EVT
#define BTA_HH_FST_BTE_TRANS_EVT        HID_TRANS_GET_REPORT

/* sub event code used for device maintainence API call */
#define BTA_HH_ADD_DEV          0
#define BTA_HH_REMOVE_DEV       1

/* state machine states */
enum {
    BTA_HH_NULL_ST,
    BTA_HH_IDLE_ST,
    BTA_HH_W4_CONN_ST,
    BTA_HH_CONN_ST
#if (defined BTA_HH_LE_INCLUDED && BTA_HH_LE_INCLUDED == TRUE)
    , BTA_HH_W4_SEC
#endif
    , BTA_HH_INVALID_ST   /* Used to check invalid states before executing SM function */

};
typedef uint8_t tBTA_HH_STATE;

/* data structure used to send a command/data to HID device */
typedef struct {
    BT_HDR           hdr;
    uint8_t            t_type;
    uint8_t            param;
    uint8_t            rpt_id;
#if (defined BTA_HH_LE_INCLUDED && BTA_HH_LE_INCLUDED == TRUE)
    uint8_t            srvc_id;
#endif
    uint16_t           data;
    BT_HDR           *p_data;
} tBTA_HH_CMD_DATA;

/* data type for BTA_HH_API_ENABLE_EVT */
typedef struct {
    BT_HDR              hdr;
    uint8_t               sec_mask;
    uint8_t               service_name[BTA_SERVICE_NAME_LEN + 1];
    tBTA_HH_CBACK   *p_cback;
} tBTA_HH_API_ENABLE;

typedef struct {
    BT_HDR          hdr;
    BD_ADDR         bd_addr;
    uint8_t           sec_mask;
    tBTA_HH_PROTO_MODE  mode;
} tBTA_HH_API_CONN;

/* internal event data from BTE HID callback */
typedef struct {
    BT_HDR          hdr;
    BD_ADDR         addr;
    uint32_t          data;
    BT_HDR          *p_data;
} tBTA_HH_CBACK_DATA;

typedef struct {
    BT_HDR              hdr;
    BD_ADDR             bda;
    uint16_t              attr_mask;
    uint16_t              sub_event;
    uint8_t               sub_class;
    uint8_t               app_id;
    tBTA_HH_DEV_DSCP_INFO      dscp_info;
} tBTA_HH_MAINT_DEV;

#if BTA_HH_LE_INCLUDED == TRUE
typedef struct {
    BT_HDR              hdr;
    uint16_t              conn_id;
    tBTA_GATT_REASON
    reason;         /* disconnect reason code, not useful when connect event is reported */

} tBTA_HH_LE_CLOSE;

typedef struct {
    BT_HDR              hdr;
    uint16_t              scan_int;
    uint16_t              scan_win;
} tBTA_HH_SCPP_UPDATE;
#endif
/* union of all event data types */
typedef union {
    BT_HDR                   hdr;
    tBTA_HH_API_ENABLE       api_enable;
    tBTA_HH_API_CONN         api_conn;
    tBTA_HH_CMD_DATA         api_sndcmd;
    tBTA_HH_CBACK_DATA       hid_cback;
    tBTA_HH_STATUS           status;
    tBTA_HH_MAINT_DEV        api_maintdev;
#if BTA_HH_LE_INCLUDED == TRUE
    tBTA_HH_LE_CLOSE         le_close;
    tBTA_GATTC_OPEN          le_open;
    tBTA_HH_SCPP_UPDATE      le_scpp_update;
    tBTA_GATTC_ENC_CMPL_CB   le_enc_cmpl;
#endif
} tBTA_HH_DATA;

#if (defined BTA_HH_LE_INCLUDED && BTA_HH_LE_INCLUDED == TRUE)
typedef struct {
    uint8_t                   index;
    uint8_t                 in_use;
    uint8_t                   srvc_inst_id;
    uint8_t                   char_inst_id;
    tBTA_HH_RPT_TYPE        rpt_type;
    uint16_t                  uuid;
    uint8_t                   rpt_id;
    uint8_t                 client_cfg_exist;
    uint16_t                  client_cfg_value;
} tBTA_HH_LE_RPT;

#ifndef BTA_HH_LE_RPT_MAX
#define BTA_HH_LE_RPT_MAX       20
#endif

typedef struct {
    uint8_t                 in_use;
    uint8_t                   srvc_inst_id;
    tBTA_HH_LE_RPT          report[BTA_HH_LE_RPT_MAX];

    uint16_t                  proto_mode_handle;
    uint8_t                   control_point_handle;

    uint8_t                 expl_incl_srvc;
    uint8_t                   incl_srvc_inst; /* assuming only one included service : battery service */
    uint8_t                   cur_expl_char_idx; /* currently discovering service index */
    uint8_t                   *rpt_map;
    uint16_t                  ext_rpt_ref;
    tBTA_HH_DEV_DESCR       descriptor;

} tBTA_HH_LE_HID_SRVC;

/* convert a HID handle to the LE CB index */
#define BTA_HH_GET_LE_CB_IDX(x)         (((x) >> 4) - 1)
/* convert a GATT connection ID to HID device handle, it is the hi 4 bits of a uint8_t */
#define BTA_HH_GET_LE_DEV_HDL(x)        (uint8_t)(((x)  + 1) << 4)
/* check to see if th edevice handle is a LE device handle */
#define BTA_HH_IS_LE_DEV_HDL(x)        ((x) & 0xf0)
#define BTA_HH_IS_LE_DEV_HDL_VALID(x)  (((x)>>4) <= BTA_HH_LE_MAX_KNOWN)
#endif

/* device control block */
typedef struct {
    tBTA_HH_DEV_DSCP_INFO  dscp_info;      /* report descriptor and DI information */
    BD_ADDR             addr;           /* BD-Addr of the HID device */
    uint16_t              attr_mask;      /* attribute mask */
    uint16_t              w4_evt;         /* W4_handshake event name */
    uint8_t               index;          /* index number referenced to handle index */
    uint8_t               sub_class;      /* Cod sub class */
    uint8_t               sec_mask;       /* security mask */
    uint8_t               app_id;         /* application ID for this connection */
    uint8_t
    hid_handle;     /* device handle : low 4 bits for regular HID: HID_HOST_MAX_DEVICES can not exceed 15;
                                                            high 4 bits for LE HID: GATT_MAX_PHY_CHANNEL can not exceed 15 */
    uint8_t             vp;             /* virtually unplug flag */
    uint8_t             in_use;         /* control block currently in use */
    uint8_t             incoming_conn;  /* is incoming connection? */
    uint8_t               incoming_hid_handle;  /* temporary handle for incoming connection? */
    uint8_t             opened;         /* TRUE if device successfully opened HID connection */
    tBTA_HH_PROTO_MODE  mode;           /* protocol mode */
    tBTA_HH_STATE       state;          /* CB state */

#if (BTA_HH_LE_INCLUDED == TRUE)
#define BTA_HH_LE_DISC_NONE     0x00
#define BTA_HH_LE_DISC_HIDS     0x01
#define BTA_HH_LE_DISC_DIS      0x02
#define BTA_HH_LE_DISC_SCPS     0x04

    uint8_t               disc_active;
    tBTA_HH_STATUS      status;
    tBTA_GATT_REASON    reason;
    uint8_t             is_le_device;
    tBTA_HH_LE_HID_SRVC hid_srvc;
    uint16_t              conn_id;
    uint8_t             in_bg_conn;
    uint8_t               clt_cfg_idx;
    uint16_t              scan_refresh_char_handle;
    uint8_t             scps_supported;

#define BTA_HH_LE_SCPS_NOTIFY_NONE    0
#define BTA_HH_LE_SCPS_NOTIFY_SPT  0x01
#define BTA_HH_LE_SCPS_NOTIFY_ENB  0x02
    uint8_t               scps_notify;   /* scan refresh supported/notification enabled */
#endif

    uint8_t             security_pending;
} tBTA_HH_DEV_CB;

/* key board parsing control block */
typedef struct {
    uint8_t             mod_key[4]; /* ctrl, shift(upper), Alt, GUI */
    uint8_t             num_lock;
    uint8_t             caps_lock;
    uint8_t               last_report[BTA_HH_MAX_RPT_CHARS];
} tBTA_HH_KB_CB;

/******************************************************************************
** Main Control Block
*******************************************************************************/
typedef struct {
    tBTA_HH_KB_CB           kb_cb;                  /* key board control block,
                                                       suppose BTA will connect
                                                       to only one keyboard at
                                                        the same time */
    tBTA_HH_DEV_CB          kdev[BTA_HH_MAX_DEVICE]; /* device control block */
    tBTA_HH_DEV_CB         *p_cur;              /* current device control
                                                       block idx, used in sdp */
    uint8_t                   cb_index[BTA_HH_MAX_KNOWN]; /* maintain a CB index
                                                        map to dev handle */
#if (defined BTA_HH_LE_INCLUDED && BTA_HH_LE_INCLUDED == TRUE)
    uint8_t
    le_cb_index[BTA_HH_MAX_DEVICE]; /* maintain a CB index map to LE dev handle */
    tBTA_GATTC_IF           gatt_if;
#endif
    tBTA_HH_CBACK       *p_cback;               /* Application callbacks */
    tSDP_DISCOVERY_DB      *p_disc_db;
    uint8_t                   trace_level;            /* tracing level */
    uint8_t                   cnt_num;                /* connected device number */
    uint8_t                 w4_disable;             /* w4 disable flag */
}
tBTA_HH_CB;

#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_HH_CB  bta_hh_cb;
#else
extern tBTA_HH_CB *bta_hh_cb_ptr;
#define bta_hh_cb (*bta_hh_cb_ptr)
#endif

/* from bta_hh_cfg.c */
extern tBTA_HH_CFG *p_bta_hh_cfg;

/*****************************************************************************
**  Function prototypes
*****************************************************************************/
extern uint8_t bta_hh_hdl_event(BT_HDR *p_msg);
extern void bta_hh_sm_execute(tBTA_HH_DEV_CB *p_cb, uint16_t event,
                              tBTA_HH_DATA *p_data);

/* action functions */
extern void bta_hh_api_disc_act(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_open_act(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_close_act(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_data_act(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_ctrl_dat_act(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_start_sdp(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_sdp_cmpl(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_write_dev_act(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_get_dscp_act(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_handsk_act(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_maint_dev_act(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_open_cmpl_act(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_open_failure(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);

/* utility functions */
extern uint8_t  bta_hh_find_cb(BD_ADDR bda);
extern void bta_hh_parse_keybd_rpt(tBTA_HH_BOOT_RPT *p_kb_data,
                                   uint8_t *p_report, uint16_t report_len);
extern void bta_hh_parse_mice_rpt(tBTA_HH_BOOT_RPT *p_kb_data,
                                  uint8_t *p_report, uint16_t report_len);
extern uint8_t bta_hh_tod_spt(tBTA_HH_DEV_CB *p_cb, uint8_t sub_class);
extern void bta_hh_clean_up_kdev(tBTA_HH_DEV_CB *p_cb);

extern void bta_hh_add_device_to_list(tBTA_HH_DEV_CB *p_cb, uint8_t handle,
                                      uint16_t attr_mask,
                                      tHID_DEV_DSCP_INFO *p_dscp_info,
                                      uint8_t sub_class, uint16_t max_latency, uint16_t min_tout, uint8_t app_id);
extern void bta_hh_update_di_info(tBTA_HH_DEV_CB *p_cb, uint16_t vendor_id, uint16_t product_id,
                                  uint16_t version, uint8_t flag);
extern void bta_hh_cleanup_disable(tBTA_HH_STATUS status);

extern uint8_t bta_hh_dev_handle_to_cb_idx(uint8_t dev_handle);

/* action functions used outside state machine */
extern void bta_hh_api_enable(tBTA_HH_DATA *p_data);
extern void bta_hh_api_disable(void);
extern void bta_hh_disc_cmpl(void);

extern tBTA_HH_STATUS bta_hh_read_ssr_param(BD_ADDR bd_addr, uint16_t *p_max_ssr_lat,
        uint16_t *p_min_ssr_tout);

/* functions for LE HID */
extern void bta_hh_le_enable(void);
extern uint8_t bta_hh_le_is_hh_gatt_if(tBTA_GATTC_IF client_if);
extern void bta_hh_le_deregister(void);
extern uint8_t bta_hh_is_le_device(tBTA_HH_DEV_CB *p_cb, BD_ADDR remote_bda);
extern void bta_hh_le_open_conn(tBTA_HH_DEV_CB *p_cb, BD_ADDR remote_bda);
extern void bta_hh_le_api_disc_act(tBTA_HH_DEV_CB *p_cb);
extern void bta_hh_le_get_dscp_act(tBTA_HH_DEV_CB *p_cb);
extern void bta_hh_le_write_dev_act(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern uint8_t bta_hh_le_add_device(tBTA_HH_DEV_CB *p_cb, tBTA_HH_MAINT_DEV *p_dev_info);
extern void bta_hh_le_remove_dev_bg_conn(tBTA_HH_DEV_CB *p_cb);
extern void bta_hh_le_open_fail(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_gatt_open(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_gatt_close(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_start_security(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf);
extern void bta_hh_start_srvc_discovery(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf);
extern void bta_hh_w4_le_read_char_cmpl(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf);
extern void bta_hh_le_read_char_cmpl(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf);
extern void bta_hh_w4_le_read_descr_cmpl(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf);
extern void bta_hh_le_read_descr_cmpl(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf);
extern void bta_hh_w4_le_write_cmpl(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf);
extern void bta_hh_le_write_cmpl(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf);
extern void bta_hh_le_write_char_descr_cmpl(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf);
extern void bta_hh_start_security(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf);
extern void bta_hh_security_cmpl(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf);
extern void bta_hh_le_update_scpp(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf);
extern void bta_hh_le_notify_enc_cmpl(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data);
extern void bta_hh_ci_load_rpt(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf);

#if BTA_HH_DEBUG
extern void bta_hh_trace_dev_db(void);
#endif

#endif

