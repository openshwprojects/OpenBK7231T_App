/******************************************************************************
 *
 *  Copyright (C) 2001-2012 Broadcom Corporation
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
 *  This file contains internally used BNEP definitions
 *
 ******************************************************************************/

#ifndef  BNEP_INT_H
#define  BNEP_INT_H

#include "bt_target.h"
#include "bt_common.h"
#include "bnep_api.h"
#include "btm_int.h"
#include "btu.h"


/* BNEP frame types
*/
#define BNEP_FRAME_GENERAL_ETHERNET                 0x00
#define BNEP_FRAME_CONTROL                          0x01
#define BNEP_FRAME_COMPRESSED_ETHERNET              0x02
#define BNEP_FRAME_COMPRESSED_ETHERNET_SRC_ONLY     0x03
#define BNEP_FRAME_COMPRESSED_ETHERNET_DEST_ONLY    0x04


/* BNEP filter control message types
*/
#define BNEP_CONTROL_COMMAND_NOT_UNDERSTOOD         0x00
#define BNEP_SETUP_CONNECTION_REQUEST_MSG           0x01
#define BNEP_SETUP_CONNECTION_RESPONSE_MSG          0x02
#define BNEP_FILTER_NET_TYPE_SET_MSG                0x03
#define BNEP_FILTER_NET_TYPE_RESPONSE_MSG           0x04
#define BNEP_FILTER_MULTI_ADDR_SET_MSG              0x05
#define BNEP_FILTER_MULTI_ADDR_RESPONSE_MSG         0x06


/* BNEP header extension types
*/
#define BNEP_EXTENSION_FILTER_CONTROL   0x00


/* BNEP Setup Connection response codes
*/
#define BNEP_SETUP_CONN_OK              0x0000
#define BNEP_SETUP_INVALID_DEST_UUID    0x0001
#define BNEP_SETUP_INVALID_SRC_UUID     0x0002
#define BNEP_SETUP_INVALID_UUID_SIZE    0x0003
#define BNEP_SETUP_CONN_NOT_ALLOWED     0x0004


/* BNEP filter control response codes
*/
#define BNEP_FILTER_CRL_OK              0x0000
#define BNEP_FILTER_CRL_UNSUPPORTED     0x0001
#define BNEP_FILTER_CRL_BAD_RANGE       0x0002
#define BNEP_FILTER_CRL_MAX_REACHED     0x0003
#define BNEP_FILTER_CRL_SECURITY_ERR    0x0004


/* 802.1p protocol packet will have actual protocol field in side the payload */
#define BNEP_802_1_P_PROTOCOL           0x8100

/* Timeout definitions.
*/
#define BNEP_CONN_TIMEOUT_MS        (20 * 1000)      /* Connection related timeout */
#define BNEP_HOST_TIMEOUT_MS        (200 * 1000)     /* host responce timeout */
#define BNEP_FILTER_SET_TIMEOUT_MS  (10 * 1000)

/* Define the Out-Flow default values. */
#define  BNEP_OFLOW_QOS_FLAG                 0
#define  BNEP_OFLOW_SERV_TYPE                0
#define  BNEP_OFLOW_TOKEN_RATE               0
#define  BNEP_OFLOW_TOKEN_BUCKET_SIZE        0
#define  BNEP_OFLOW_PEAK_BANDWIDTH           0
#define  BNEP_OFLOW_LATENCY                  0
#define  BNEP_OFLOW_DELAY_VARIATION          0

/* Define the In-Flow default values. */
#define  BNEP_IFLOW_QOS_FLAG                 0
#define  BNEP_IFLOW_SERV_TYPE                0
#define  BNEP_IFLOW_TOKEN_RATE               0
#define  BNEP_IFLOW_TOKEN_BUCKET_SIZE        0
#define  BNEP_IFLOW_PEAK_BANDWIDTH           0
#define  BNEP_IFLOW_LATENCY                  0
#define  BNEP_IFLOW_DELAY_VARIATION          0

#define BNEP_FLUSH_TO                       0xFFFF

#define BNEP_MAX_RETRANSMITS                 3

/* Define the BNEP Connection Control Block
*/
typedef struct {
#define BNEP_STATE_IDLE              0
#define BNEP_STATE_CONN_START        1
#define BNEP_STATE_CFG_SETUP         2
#define BNEP_STATE_CONN_SETUP        3
#define BNEP_STATE_SEC_CHECKING      4
#define BNEP_STATE_SETUP_RCVD        5
#define BNEP_STATE_CONNECTED         6
    uint8_t             con_state;

#define BNEP_FLAGS_IS_ORIG           0x01
#define BNEP_FLAGS_HIS_CFG_DONE      0x02
#define BNEP_FLAGS_MY_CFG_DONE       0x04
#define BNEP_FLAGS_L2CAP_CONGESTED   0x08
#define BNEP_FLAGS_FILTER_RESP_PEND  0x10
#define BNEP_FLAGS_MULTI_RESP_PEND   0x20
#define BNEP_FLAGS_SETUP_RCVD        0x40
#define BNEP_FLAGS_CONN_COMPLETED    0x80
    uint8_t             con_flags;
    BT_HDR            *p_pending_data;

    uint16_t            l2cap_cid;
    BD_ADDR           rem_bda;
    uint16_t            rem_mtu_size;
    TIMER_LIST_ENT           conn_timer;
    fixed_queue_t     *xmit_q;

    uint16_t            sent_num_filters;
    uint16_t            sent_prot_filter_start[BNEP_MAX_PROT_FILTERS];
    uint16_t            sent_prot_filter_end[BNEP_MAX_PROT_FILTERS];

    uint16_t            sent_mcast_filters;
    BD_ADDR           sent_mcast_filter_start[BNEP_MAX_MULTI_FILTERS];
    BD_ADDR           sent_mcast_filter_end[BNEP_MAX_MULTI_FILTERS];

    uint16_t            rcvd_num_filters;
    uint16_t            rcvd_prot_filter_start[BNEP_MAX_PROT_FILTERS];
    uint16_t            rcvd_prot_filter_end[BNEP_MAX_PROT_FILTERS];

    uint16_t            rcvd_mcast_filters;
    BD_ADDR           rcvd_mcast_filter_start[BNEP_MAX_MULTI_FILTERS];
    BD_ADDR           rcvd_mcast_filter_end[BNEP_MAX_MULTI_FILTERS];

    uint16_t            bad_pkts_rcvd;
    uint8_t             re_transmits;
    uint16_t            handle;
    tBT_UUID          prv_src_uuid;
    tBT_UUID          prv_dst_uuid;
    tBT_UUID          src_uuid;
    tBT_UUID          dst_uuid;

} tBNEP_CONN;


/*  The main BNEP control block
*/
typedef struct {
    tL2CAP_CFG_INFO         l2cap_my_cfg;                   /* My L2CAP config     */
    tBNEP_CONN              bcb[BNEP_MAX_CONNECTIONS];

    tBNEP_CONNECT_IND_CB    *p_conn_ind_cb;
    tBNEP_CONN_STATE_CB     *p_conn_state_cb;
    tBNEP_DATA_IND_CB       *p_data_ind_cb;
    tBNEP_DATA_BUF_CB       *p_data_buf_cb;
    tBNEP_FILTER_IND_CB     *p_filter_ind_cb;
    tBNEP_MFILTER_IND_CB    *p_mfilter_ind_cb;
    tBNEP_TX_DATA_FLOW_CB   *p_tx_data_flow_cb;

    tL2CAP_APPL_INFO        reg_info;

    uint8_t                 profile_registered;             /* TRUE when we got our BD addr */
    uint8_t                   trace_level;

} tBNEP_CB;

#ifdef __cplusplus
extern "C" {
#endif

/* Global BNEP data
*/
#if BNEP_DYNAMIC_MEMORY == FALSE
extern tBNEP_CB  bnep_cb;
#else
extern tBNEP_CB  *bnep_cb_ptr;
#define bnep_cb (*bnep_cb_ptr)
#endif

/* Functions provided by bnep_main.c
*/
extern tBNEP_RESULT bnep_register_with_l2cap(void);
extern void        bnep_disconnect(tBNEP_CONN *p_bcb, uint16_t reason);
extern tBNEP_CONN *bnep_conn_originate(uint8_t *p_bd_addr);
extern void        bnep_conn_timer_timeout(void *data);
extern void        bnep_connected(tBNEP_CONN *p_bcb);


/* Functions provided by bnep_utils.c
*/
extern tBNEP_CONN *bnepu_find_bcb_by_cid(uint16_t cid);
extern tBNEP_CONN *bnepu_find_bcb_by_bd_addr(uint8_t *p_bda);
extern tBNEP_CONN *bnepu_allocate_bcb(BD_ADDR p_rem_bda);
extern void        bnepu_release_bcb(tBNEP_CONN *p_bcb);
extern void        bnepu_send_peer_our_filters(tBNEP_CONN *p_bcb);
extern void        bnepu_send_peer_our_multi_filters(tBNEP_CONN *p_bcb);
extern uint8_t     bnepu_does_dest_support_prot(tBNEP_CONN *p_bcb, uint16_t protocol);
extern void        bnepu_build_bnep_hdr(tBNEP_CONN *p_bcb, BT_HDR *p_buf, uint16_t protocol,
                                        uint8_t *p_src_addr, uint8_t *p_dest_addr, uint8_t ext_bit);
extern void        test_bnepu_build_bnep_hdr(tBNEP_CONN *p_bcb, BT_HDR *p_buf, uint16_t protocol,
        uint8_t *p_src_addr, uint8_t *p_dest_addr, uint8_t type);

extern tBNEP_CONN *bnepu_get_route_to_dest(uint8_t *p_bda);
extern void        bnepu_check_send_packet(tBNEP_CONN *p_bcb, BT_HDR *p_buf);
extern void        bnep_send_command_not_understood(tBNEP_CONN *p_bcb, uint8_t cmd_code);
extern void        bnepu_process_peer_filter_set(tBNEP_CONN *p_bcb, uint8_t *p_filters,
        uint16_t len);
extern void        bnepu_process_peer_filter_rsp(tBNEP_CONN *p_bcb, uint8_t *p_data);
extern void        bnepu_process_multicast_filter_rsp(tBNEP_CONN *p_bcb, uint8_t *p_data);
extern void        bnep_send_conn_req(tBNEP_CONN *p_bcb);
extern void        bnep_send_conn_responce(tBNEP_CONN *p_bcb, uint16_t resp_code);
extern void        bnep_process_setup_conn_req(tBNEP_CONN *p_bcb, uint8_t *p_setup, uint8_t len);
extern void        bnep_process_setup_conn_responce(tBNEP_CONN *p_bcb, uint8_t *p_setup);
extern uint8_t       *bnep_process_control_packet(tBNEP_CONN *p_bcb, uint8_t *p, uint16_t *len,
        uint8_t is_ext);
extern void        bnep_sec_check_complete(BD_ADDR bd_addr, tBT_TRANSPORT trasnport,
        void *p_ref_data, uint8_t result);
extern tBNEP_RESULT bnep_is_packet_allowed(tBNEP_CONN *p_bcb, BD_ADDR p_dest_addr,
        uint16_t protocol,
        uint8_t fw_ext_present, uint8_t *p_data);
extern uint32_t      bnep_get_uuid32(tBT_UUID *src_uuid);



#ifdef __cplusplus
}
#endif


#endif
