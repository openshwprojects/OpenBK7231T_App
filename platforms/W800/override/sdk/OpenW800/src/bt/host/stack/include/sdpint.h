/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
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
 *  This file contains internally used SDP definitions
 *
 ******************************************************************************/

#ifndef  SDP_INT_H
#define  SDP_INT_H

#include "bt_target.h"
#include "gki.h"
#include "sdp_api.h"
#include "l2c_api.h"


/* Continuation length - we use a 2-byte offset */
#define SDP_CONTINUATION_LEN        2
#define SDP_MAX_CONTINUATION_LEN    16          /* As per the spec */

/* Timeout definitions. */
#define SDP_INACT_TIMEOUT_MS  (30 * 1000)    /* Inactivity timeout (in ms) */


/* Define the Out-Flow default values. */
#define  SDP_OFLOW_QOS_FLAG                 0
#define  SDP_OFLOW_SERV_TYPE                0
#define  SDP_OFLOW_TOKEN_RATE               0
#define  SDP_OFLOW_TOKEN_BUCKET_SIZE        0
#define  SDP_OFLOW_PEAK_BANDWIDTH           0
#define  SDP_OFLOW_LATENCY                  0
#define  SDP_OFLOW_DELAY_VARIATION          0

/* Define the In-Flow default values. */
#define  SDP_IFLOW_QOS_FLAG                 0
#define  SDP_IFLOW_SERV_TYPE                0
#define  SDP_IFLOW_TOKEN_RATE               0
#define  SDP_IFLOW_TOKEN_BUCKET_SIZE        0
#define  SDP_IFLOW_PEAK_BANDWIDTH           0
#define  SDP_IFLOW_LATENCY                  0
#define  SDP_IFLOW_DELAY_VARIATION          0

#define  SDP_LINK_TO                        0

/* Define the type of device notification. */
/* (Inquiry Scan and Page Scan)            */
#define  SDP_DEVICE_NOTI_LEN                sizeof (BT_HDR) +           \
    HCIC_PREAMBLE_SIZE +        \
    HCIC_PARAM_SIZE_WRITE_PARAM1

#define  SDP_DEVICE_NOTI_FLAG               0x03

/* Define the Protocol Data Unit (PDU) types.
*/
#define  SDP_PDU_ERROR_RESPONSE                 0x01
#define  SDP_PDU_SERVICE_SEARCH_REQ             0x02
#define  SDP_PDU_SERVICE_SEARCH_RSP             0x03
#define  SDP_PDU_SERVICE_ATTR_REQ               0x04
#define  SDP_PDU_SERVICE_ATTR_RSP               0x05
#define  SDP_PDU_SERVICE_SEARCH_ATTR_REQ        0x06
#define  SDP_PDU_SERVICE_SEARCH_ATTR_RSP        0x07

/* Max UUIDs and attributes we support per sequence */
#define     MAX_UUIDS_PER_SEQ       16
#define     MAX_ATTR_PER_SEQ        16

/* Max length we support for any attribute */
#ifdef SDP_MAX_ATTR_LEN
#define MAX_ATTR_LEN SDP_MAX_ATTR_LEN
#else
#define     MAX_ATTR_LEN            256
#endif

/* Internal UUID sequence representation */
typedef struct {
    uint16_t     len;
    uint8_t      value[MAX_UUID_SIZE];
} tUID_ENT;

typedef struct {
    uint16_t      num_uids;
    tUID_ENT    uuid_entry[MAX_UUIDS_PER_SEQ];
} tSDP_UUID_SEQ;


/* Internal attribute sequence definitions */
typedef struct {
    uint16_t      start;
    uint16_t      end;
} tATT_ENT;

typedef struct {
    uint16_t      num_attr;
    tATT_ENT    attr_entry[MAX_ATTR_PER_SEQ];
} tSDP_ATTR_SEQ;


/* Define the attribute element of the SDP database record */
typedef struct {
    uint32_t  len;           /* Number of bytes in the entry */
    uint8_t   *value_ptr;    /* Points to attr_pad */
    uint16_t  id;
    uint8_t   type;
} tSDP_ATTRIBUTE;

/* An SDP record consists of a handle, and 1 or more attributes */
typedef struct {
    uint32_t              record_handle;
    uint32_t              free_pad_ptr;
    uint16_t              num_attributes;
    tSDP_ATTRIBUTE      attribute[SDP_MAX_REC_ATTR];
    uint8_t               attr_pad[SDP_MAX_PAD_LEN];
} tSDP_RECORD;


/* Define the SDP database */
typedef struct {
    uint32_t         di_primary_handle;       /* Device ID Primary record or NULL if nonexistent */
    uint16_t         num_records;
    tSDP_RECORD    record[SDP_MAX_RECORDS];
} tSDP_DB;

enum {
    SDP_IS_SEARCH,
    SDP_IS_ATTR_SEARCH,
};

#if SDP_SERVER_ENABLED == TRUE
/* Continuation information for the SDP server response */
typedef struct {
    uint16_t            next_attr_index; /* attr index for next continuation response */
    uint16_t
    next_attr_start_id;  /* attr id to start with for the attr index in next cont. response */
    tSDP_RECORD       *prev_sdp_rec; /* last sdp record that was completely sent in the response */
    uint8_t           last_attr_seq_desc_sent; /* whether attr seq length has been sent previously */
    uint16_t
    attr_offset; /* offset within the attr to keep trak of partial attributes in the responses */
} tSDP_CONT_INFO;
#endif  /* SDP_SERVER_ENABLED == TRUE */

/* Define the SDP Connection Control Block */
typedef struct {
#define SDP_STATE_IDLE              0
#define SDP_STATE_CONN_SETUP        1
#define SDP_STATE_CFG_SETUP         2
#define SDP_STATE_CONNECTED         3
    uint8_t             con_state;

#define SDP_FLAGS_IS_ORIG           0x01
#define SDP_FLAGS_HIS_CFG_DONE      0x02
#define SDP_FLAGS_MY_CFG_DONE       0x04
    uint8_t             con_flags;

    BD_ADDR           device_address;
    TIMER_LIST_ENT sdp_conn_timer;
    uint16_t            rem_mtu_size;
    uint16_t            connection_id;
    uint16_t            list_len;                 /* length of the response in the GKI buffer */
    uint8_t             *rsp_list;                /* pointer to GKI buffer holding response */

#if SDP_CLIENT_ENABLED == TRUE
    tSDP_DISCOVERY_DB *p_db;                    /* Database to save info into   */
    tSDP_DISC_CMPL_CB *p_cb;                    /* Callback for discovery done  */
    tSDP_DISC_CMPL_CB2
    *p_cb2;                   /* Callback for discovery done piggy back with the user data */
    void               *user_data;              /* piggy back user data */
    uint32_t            handles[SDP_MAX_DISC_SERVER_RECS]; /* Discovered server record handles */
    uint16_t            num_handles;              /* Number of server handles     */
    uint16_t            cur_handle;               /* Current handle being processed */
    uint16_t            transaction_id;
    uint16_t            disconnect_reason;        /* Disconnect reason            */
#if (defined(SDP_BROWSE_PLUS) && SDP_BROWSE_PLUS == TRUE)
    uint16_t            cur_uuid_idx;
#endif

#define SDP_DISC_WAIT_CONN          0
#define SDP_DISC_WAIT_HANDLES       1
#define SDP_DISC_WAIT_ATTR          2
#define SDP_DISC_WAIT_SEARCH_ATTR   3
#define SDP_DISC_WAIT_CANCEL        5

    uint8_t             disc_state;
    uint8_t             is_attr_search;
#endif  /* SDP_CLIENT_ENABLED == TRUE */

#if SDP_SERVER_ENABLED == TRUE
    uint16_t            cont_offset;              /* Continuation state data in the server response */
    tSDP_CONT_INFO
    cont_info;                /* structure to hold continuation information for the server response */
#endif  /* SDP_SERVER_ENABLED == TRUE */

} tCONN_CB;


/*  The main SDP control block */
typedef struct {
    tL2CAP_CFG_INFO   l2cap_my_cfg;             /* My L2CAP config     */
    tCONN_CB          ccb[SDP_MAX_CONNECTIONS];
#if SDP_SERVER_ENABLED == TRUE
    tSDP_DB           server_db;
#endif
    tL2CAP_APPL_INFO  reg_info;                 /* L2CAP Registration info */
    uint16_t            max_attr_list_size;       /* Max attribute list size to use   */
    uint16_t            max_recs_per_search;      /* Max records we want per seaarch  */
    uint8_t             trace_level;
} tSDP_CB;

#ifdef __cplusplus
extern "C" {
#endif
/* Global SDP data */
#if SDP_DYNAMIC_MEMORY == FALSE
extern tSDP_CB  sdp_cb;
#else
extern tSDP_CB *sdp_cb_ptr;
#define sdp_cb (*sdp_cb_ptr)
#endif

#ifdef __cplusplus
}
#endif

/* Functions provided by sdp_main.c */
extern void     sdp_init(void);
extern void     sdp_deinit(void);
extern void     sdp_disconnect(tCONN_CB *p_ccb, uint16_t reason);

#if (defined(SDP_DEBUG) && SDP_DEBUG == TRUE)
extern uint16_t sdp_set_max_attr_list_size(uint16_t max_size);
#endif

/* Functions provided by sdp_conn.c
*/
extern void sdp_conn_rcv_l2e_conn_ind(BT_HDR *p_msg);
extern void sdp_conn_rcv_l2e_conn_cfm(BT_HDR *p_msg);
extern void sdp_conn_rcv_l2e_disc(BT_HDR *p_msg);
extern void sdp_conn_rcv_l2e_config_ind(BT_HDR *p_msg);
extern void sdp_conn_rcv_l2e_config_cfm(BT_HDR *p_msg);
extern void sdp_conn_rcv_l2e_conn_failed(BT_HDR *p_msg);
extern void sdp_conn_rcv_l2e_connected(BT_HDR *p_msg);
extern void sdp_conn_rcv_l2e_conn_failed(BT_HDR *p_msg);
extern void sdp_conn_rcv_l2e_data(BT_HDR *p_msg);
extern void sdp_conn_timer_timeout(void *data);

extern tCONN_CB *sdp_conn_originate(uint8_t *p_bd_addr);

/* Functions provided by sdp_utils.c
*/
extern tCONN_CB *sdpu_find_ccb_by_cid(uint16_t cid);
extern tCONN_CB *sdpu_find_ccb_by_db(tSDP_DISCOVERY_DB *p_db);
extern tCONN_CB *sdpu_allocate_ccb(void);
extern void      sdpu_release_ccb(tCONN_CB *p_ccb);

extern uint8_t    *sdpu_build_attrib_seq(uint8_t *p_out, uint16_t *p_attr, uint16_t num_attrs);
extern uint8_t    *sdpu_build_attrib_entry(uint8_t *p_out, tSDP_ATTRIBUTE *p_attr);
extern void      sdpu_build_n_send_error(tCONN_CB *p_ccb, uint16_t trans_num, uint16_t error_code,
        char *p_error_text);

extern uint8_t    *sdpu_extract_attr_seq(uint8_t *p, uint16_t param_len, tSDP_ATTR_SEQ *p_seq);
extern uint8_t    *sdpu_extract_uid_seq(uint8_t *p, uint16_t param_len, tSDP_UUID_SEQ *p_seq);

extern uint8_t    *sdpu_get_len_from_type(uint8_t *p, uint8_t type, uint32_t *p_len);
extern uint8_t  sdpu_is_base_uuid(uint8_t *p_uuid);
extern uint8_t  sdpu_compare_uuid_arrays(uint8_t *p_uuid1, uint32_t len1, uint8_t *p_uuid2,
        uint16_t len2);
extern uint8_t  sdpu_compare_bt_uuids(tBT_UUID *p_uuid1, tBT_UUID *p_uuid2);
extern uint8_t  sdpu_compare_uuid_with_attr(tBT_UUID *p_btuuid, tSDP_DISC_ATTR *p_attr);

extern void     sdpu_sort_attr_list(uint16_t num_attr, tSDP_DISCOVERY_DB *p_db);
extern uint16_t sdpu_get_list_len(tSDP_UUID_SEQ   *uid_seq, tSDP_ATTR_SEQ   *attr_seq);
extern uint16_t sdpu_get_attrib_seq_len(tSDP_RECORD *p_rec, tSDP_ATTR_SEQ *attr_seq);
extern uint16_t sdpu_get_attrib_entry_len(tSDP_ATTRIBUTE *p_attr);
extern uint8_t *sdpu_build_partial_attrib_entry(uint8_t *p_out, tSDP_ATTRIBUTE *p_attr,
        uint16_t len, uint16_t *offset);
extern void sdpu_uuid16_to_uuid128(uint16_t uuid16, uint8_t *p_uuid128);

/* Functions provided by sdp_db.c
*/
extern tSDP_RECORD    *sdp_db_service_search(tSDP_RECORD *p_rec, tSDP_UUID_SEQ *p_seq);
extern tSDP_RECORD    *sdp_db_find_record(uint32_t handle);
extern tSDP_ATTRIBUTE *sdp_db_find_attr_in_rec(tSDP_RECORD *p_rec, uint16_t start_attr,
        uint16_t end_attr);


/* Functions provided by sdp_server.c
*/
#if SDP_SERVER_ENABLED == TRUE
extern void     sdp_server_handle_client_req(tCONN_CB *p_ccb, BT_HDR *p_msg);
#else
#define sdp_server_handle_client_req(p_ccb, p_msg)
#endif

/* Functions provided by sdp_discovery.c
*/
#if SDP_CLIENT_ENABLED == TRUE
extern void sdp_disc_connected(tCONN_CB *p_ccb);
extern void sdp_disc_server_rsp(tCONN_CB *p_ccb, BT_HDR *p_msg);
#else
#define sdp_disc_connected(p_ccb)
#define sdp_disc_server_rsp(p_ccb, p_msg)
#endif



#endif
