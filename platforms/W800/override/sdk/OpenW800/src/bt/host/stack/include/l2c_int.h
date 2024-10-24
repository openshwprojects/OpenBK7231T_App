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
 *  This file contains L2CAP internal definitions
 *
 ******************************************************************************/
#ifndef L2C_INT_H
#define L2C_INT_H

#include <stdbool.h>

#include "osi/include/alarm.h"
#include "osi/include/fixed_queue.h"
#include "osi/include/list.h"
#include "btm_api.h"
#include "bt_common.h"
#include "gki.h"
#include "l2c_api.h"
#include "l2cdefs.h"

#define L2CAP_MIN_MTU   48      /* Minimum acceptable MTU is 48 bytes */

/* LE credit based L2CAP connection parameters */
#define L2CAP_LE_MIN_MTU            23
#define L2CAP_LE_MIN_MPS            23
#define L2CAP_LE_MAX_MPS            65533
#define L2CAP_LE_MIN_CREDIT         0
#define L2CAP_LE_MAX_CREDIT         65535
#define L2CAP_LE_DEFAULT_MTU        512
#define L2CAP_LE_DEFAULT_MPS        23
#define L2CAP_LE_DEFAULT_CREDIT     1

/*
 * Timeout values (in milliseconds).
 */
#define L2CAP_LINK_ROLE_SWITCH_TIMEOUT_MS  (10 * 1000)  /* 10 seconds */
#define L2CAP_LINK_CONNECT_TIMEOUT_MS      (60 * 1000)  /* 30 seconds */
#define L2CAP_LINK_CONNECT_EXT_TIMEOUT_MS (120 * 1000)  /* 120 seconds */
#define L2CAP_ECHO_RSP_TIMEOUT_MS          (30 * 1000)  /* 30 seconds */
#define L2CAP_LINK_FLOW_CONTROL_TIMEOUT_MS  (2 * 1000)  /* 2 seconds */
#define L2CAP_LINK_DISCONNECT_TIMEOUT_MS   (30 * 1000)  /* 30 seconds */
#define L2CAP_CHNL_CONNECT_TIMEOUT_MS      (60 * 1000)  /* 60 seconds */
#define L2CAP_CHNL_CONNECT_EXT_TIMEOUT_MS (120 * 1000)  /* 120 seconds */
#define L2CAP_CHNL_CFG_TIMEOUT_MS          (30 * 1000)  /* 30 seconds */
#define L2CAP_CHNL_DISCONNECT_TIMEOUT_MS   (10 * 1000)  /* 10 seconds */
#define L2CAP_DELAY_CHECK_SM4_TIMEOUT_MS    (2 * 1000)  /* 2 seconds */
#define L2CAP_WAIT_INFO_RSP_TIMEOUT_MS      (3 * 1000)  /* 3 seconds */
#define L2CAP_BLE_LINK_CONNECT_TIMEOUT_MS  (30 * 1000)  /* 30 seconds */
//#define L2CAP_FCR_ACK_TIMEOUT_MS                  200   /* 200 milliseconds */

/* quick timer uses millisecond unit */
#define L2CAP_DEFAULT_RETRANS_TOUT   2000         /* 2000 milliseconds */
#define L2CAP_DEFAULT_MONITOR_TOUT   12000        /* 12000 milliseconds */
#define L2CAP_FCR_ACK_TOUT           200          /* 200 milliseconds */

/* Define the possible L2CAP channel states. The names of
** the states may seem a bit strange, but they are taken from
** the Bluetooth specification.
*/
typedef enum {
    CST_CLOSED,                           /* Channel is in closed state           */
    CST_ORIG_W4_SEC_COMP,                 /* Originator waits security clearence  */
    CST_TERM_W4_SEC_COMP,                 /* Acceptor waits security clearence    */
    CST_W4_L2CAP_CONNECT_RSP,             /* Waiting for peer conenct response    */
    CST_W4_L2CA_CONNECT_RSP,              /* Waiting for upper layer connect rsp  */
    CST_CONFIG,                           /* Negotiating configuration            */
    CST_OPEN,                             /* Data transfer state                  */
    CST_W4_L2CAP_DISCONNECT_RSP,          /* Waiting for peer disconnect rsp      */
    CST_W4_L2CA_DISCONNECT_RSP            /* Waiting for upper layer disc rsp     */
} tL2C_CHNL_STATE;

/* Define the possible L2CAP link states
*/
typedef enum {
    LST_DISCONNECTED,
    LST_CONNECT_HOLDING,
    LST_CONNECTING_WAIT_SWITCH,
    LST_CONNECTING,
    LST_CONNECTED,
    LST_DISCONNECTING
} tL2C_LINK_STATE;



/* Define input events to the L2CAP link and channel state machines. The names
** of the events may seem a bit strange, but they are taken from
** the Bluetooth specification.
*/
#define L2CEVT_LP_CONNECT_CFM          0          /* Lower layer connect confirm          */
#define L2CEVT_LP_CONNECT_CFM_NEG      1          /* Lower layer connect confirm (failed) */
#define L2CEVT_LP_CONNECT_IND          2          /* Lower layer connect indication       */
#define L2CEVT_LP_DISCONNECT_IND       3          /* Lower layer disconnect indication    */
#define L2CEVT_LP_QOS_CFM              4          /* Lower layer QOS confirmation         */
#define L2CEVT_LP_QOS_CFM_NEG          5          /* Lower layer QOS confirmation (failed)*/
#define L2CEVT_LP_QOS_VIOLATION_IND    6          /* Lower layer QOS violation indication */

#define L2CEVT_SEC_COMP                7          /* Security cleared successfully        */
#define L2CEVT_SEC_COMP_NEG            8          /* Security procedure failed            */

#define L2CEVT_L2CAP_CONNECT_REQ      10          /* Peer connection request              */
#define L2CEVT_L2CAP_CONNECT_RSP      11          /* Peer connection response             */
#define L2CEVT_L2CAP_CONNECT_RSP_PND  12          /* Peer connection response pending     */
#define L2CEVT_L2CAP_CONNECT_RSP_NEG  13          /* Peer connection response (failed)    */
#define L2CEVT_L2CAP_CONFIG_REQ       14          /* Peer configuration request           */
#define L2CEVT_L2CAP_CONFIG_RSP       15          /* Peer configuration response          */
#define L2CEVT_L2CAP_CONFIG_RSP_NEG   16          /* Peer configuration response (failed) */
#define L2CEVT_L2CAP_DISCONNECT_REQ   17          /* Peer disconnect request              */
#define L2CEVT_L2CAP_DISCONNECT_RSP   18          /* Peer disconnect response             */
#define L2CEVT_L2CAP_INFO_RSP         19          /* Peer information response            */
#define L2CEVT_L2CAP_DATA             20          /* Peer data                            */

#define L2CEVT_L2CA_CONNECT_REQ       21          /* Upper layer connect request          */
#define L2CEVT_L2CA_CONNECT_RSP       22          /* Upper layer connect response         */
#define L2CEVT_L2CA_CONNECT_RSP_NEG   23          /* Upper layer connect response (failed)*/
#define L2CEVT_L2CA_CONFIG_REQ        24          /* Upper layer config request           */
#define L2CEVT_L2CA_CONFIG_RSP        25          /* Upper layer config response          */
#define L2CEVT_L2CA_CONFIG_RSP_NEG    26          /* Upper layer config response (failed) */
#define L2CEVT_L2CA_DISCONNECT_REQ    27          /* Upper layer disconnect request       */
#define L2CEVT_L2CA_DISCONNECT_RSP    28          /* Upper layer disconnect response      */
#define L2CEVT_L2CA_DATA_READ         29          /* Upper layer data read                */
#define L2CEVT_L2CA_DATA_WRITE        30          /* Upper layer data write               */
#define L2CEVT_L2CA_FLUSH_REQ         31          /* Upper layer flush                    */

#define L2CEVT_TIMEOUT                32          /* Timeout                              */
#define L2CEVT_SEC_RE_SEND_CMD        33          /* btm_sec has enough info to proceed   */

#define L2CEVT_ACK_TIMEOUT            34          /* RR delay timeout                     */

#define L2CEVT_L2CA_SEND_FLOW_CONTROL_CREDIT     35    /* Upper layer credit packet */
#define L2CEVT_L2CAP_RECV_FLOW_CONTROL_CREDIT    36    /* Peer credit packet */

/* Bitmask to skip over Broadcom feature reserved (ID) to avoid sending two
   successive ID values, '0' id only or both */
#define L2CAP_ADJ_BRCM_ID           0x1
#define L2CAP_ADJ_ZERO_ID           0x2
#define L2CAP_ADJ_ID                0x3

/* Return values for l2cu_process_peer_cfg_req() */
#define L2CAP_PEER_CFG_UNACCEPTABLE     0
#define L2CAP_PEER_CFG_OK               1
#define L2CAP_PEER_CFG_DISCONNECT       2

/* eL2CAP option constants */
#define L2CAP_MIN_RETRANS_TOUT          2000    /* Min retransmission timeout if no flush timeout or PBF */
#define L2CAP_MIN_MONITOR_TOUT          12000   /* Min monitor timeout if no flush timeout or PBF */

#define L2CAP_MAX_FCR_CFG_TRIES         2       /* Config attempts before disconnecting */

typedef uint8_t tL2C_BLE_FIXED_CHNLS_MASK;

typedef struct {
    uint8_t       next_tx_seq;                /* Next sequence number to be Tx'ed         */
    uint8_t       last_rx_ack;                /* Last sequence number ack'ed by the peer  */
    uint8_t       next_seq_expected;          /* Next peer sequence number expected       */
    uint8_t       last_ack_sent;              /* Last peer sequence number ack'ed         */
    uint8_t       num_tries;                  /* Number of retries to send a packet       */
    uint8_t       max_held_acks;              /* Max acks we can hold before sending      */

    uint8_t     remote_busy;                /* TRUE if peer has flowed us off           */
    uint8_t     local_busy;                 /* TRUE if we have flowed off the peer      */

    uint8_t     rej_sent;                   /* Reject was sent                          */
    uint8_t     srej_sent;                  /* Selective Reject was sent                */
    uint8_t     wait_ack;                   /* Transmitter is waiting ack (poll sent)   */
    uint8_t     rej_after_srej;             /* Send a REJ when SREJ clears              */

    uint8_t     send_f_rsp;                 /* We need to send an F-bit response        */

    uint16_t      rx_sdu_len;                 /* Length of the SDU being received         */
    BT_HDR      *p_rx_sdu;                  /* Buffer holding the SDU being received    */
    fixed_queue_t *waiting_for_ack_q;       /* Buffers sent and waiting for peer to ack */
    fixed_queue_t *srej_rcv_hold_q;         /* Buffers rcvd but held pending SREJ rsp   */
    fixed_queue_t *retrans_q;               /* Buffers being retransmitted              */

    TIMER_LIST_ENT ack_timer;                /* Timer delaying RR                        */
    TIMER_LIST_ENT mon_retrans_timer;       /* Timer Monitor or Retransmission          */

#if (L2CAP_ERTM_STATS == TRUE)
    uint32_t      connect_tick_count;         /* Time channel was established             */
    uint32_t      ertm_pkt_counts[2];         /* Packets sent and received                */
    uint32_t      ertm_byte_counts[2];        /* Bytes   sent and received                */
    uint32_t      s_frames_sent[4];           /* S-frames sent (RR, REJ, RNR, SREJ)       */
    uint32_t      s_frames_rcvd[4];           /* S-frames rcvd (RR, REJ, RNR, SREJ)       */
    uint32_t      xmit_window_closed;         /* # of times the xmit window was closed    */
    uint32_t      controller_idle;            /* # of times less than 2 packets in controller */
    /* when the xmit window was closed          */
    uint32_t      pkts_retransmitted;         /* # of packets that were retransmitted     */
    uint32_t      retrans_touts;              /* # of retransmission timouts              */
    uint32_t      xmit_ack_touts;             /* # of xmit ack timouts                    */

#define L2CAP_ERTM_STATS_NUM_AVG 10
#define L2CAP_ERTM_STATS_AVG_NUM_SAMPLES 100
    uint32_t      ack_delay_avg_count;
    uint32_t      ack_delay_avg_index;
    uint32_t      throughput_start;
    uint32_t      throughput[L2CAP_ERTM_STATS_NUM_AVG];
    uint32_t      ack_delay_avg[L2CAP_ERTM_STATS_NUM_AVG];
    uint32_t      ack_delay_min[L2CAP_ERTM_STATS_NUM_AVG];
    uint32_t      ack_delay_max[L2CAP_ERTM_STATS_NUM_AVG];
    uint32_t      ack_q_count_avg[L2CAP_ERTM_STATS_NUM_AVG];
    uint32_t      ack_q_count_min[L2CAP_ERTM_STATS_NUM_AVG];
    uint32_t      ack_q_count_max[L2CAP_ERTM_STATS_NUM_AVG];
#endif
} tL2C_FCRB;


/* Define a registration control block. Every application (e.g. RFCOMM, SDP,
** TCS etc) that registers with L2CAP is assigned one of these.
*/
#if (L2CAP_UCD_INCLUDED == TRUE)
#define L2C_UCD_RCB_ID              0x00
#define L2C_UCD_STATE_UNUSED        0x00
#define L2C_UCD_STATE_W4_DATA       0x01
#define L2C_UCD_STATE_W4_RECEPTION  0x02
#define L2C_UCD_STATE_W4_MTU        0x04

typedef struct {
    uint8_t               state;
    tL2CAP_UCD_CB_INFO  cb_info;
} tL2C_UCD_REG;
#endif

typedef struct {
    uint8_t                 in_use;
    uint16_t                  psm;
    uint16_t
    real_psm;               /* This may be a dummy RCB for an o/b connection but */
    /* this is the real PSM that we need to connect to   */
#if (L2CAP_UCD_INCLUDED == TRUE)
    tL2C_UCD_REG            ucd;
#endif

    tL2CAP_APPL_INFO        api;
} tL2C_RCB;


#ifndef L2CAP_CBB_DEFAULT_DATA_RATE_BUFF_QUOTA
#define L2CAP_CBB_DEFAULT_DATA_RATE_BUFF_QUOTA 100
#endif

typedef void (tL2CAP_SEC_CBACK)(BD_ADDR bd_addr, tBT_TRANSPORT trasnport,
                                void *p_ref_data, tBTM_STATUS result);

typedef struct {
    uint16_t                  psm;
    tBT_TRANSPORT           transport;
    uint8_t                 is_originator;
    tL2CAP_SEC_CBACK        *p_callback;
    void                    *p_ref_data;
} tL2CAP_SEC_DATA;

/* Define a channel control block (CCB). There may be many channel control blocks
** between the same two Bluetooth devices (i.e. on the same link).
** Each CCB has unique local and remote CIDs. All channel control blocks on
** the same physical link and are chained together.
*/
typedef struct t_l2c_ccb {
    uint8_t             in_use;                 /* TRUE when in use, FALSE when not */
    tL2C_CHNL_STATE     chnl_state;             /* Channel state                    */
    tL2CAP_LE_CFG_INFO  local_conn_cfg;         /* Our config for ble conn oriented channel */
    tL2CAP_LE_CFG_INFO  peer_conn_cfg;          /* Peer device config ble conn oriented channel */
    uint8_t
    is_first_seg;           /* Dtermine whether the received packet is the first segment or not */
    BT_HDR             *ble_sdu;                /* Buffer for storing unassembled sdu*/
    uint16_t              ble_sdu_length;         /* Length of unassembled sdu length*/
    struct t_l2c_ccb    *p_next_ccb;            /* Next CCB in the chain            */
    struct t_l2c_ccb    *p_prev_ccb;            /* Previous CCB in the chain        */
    struct t_l2c_linkcb *p_lcb;                 /* Link this CCB is assigned to     */

    uint16_t              local_cid;              /* Local CID                        */
    uint16_t              remote_cid;             /* Remote CID                       */

    TIMER_LIST_ENT l2c_ccb_timer;         /* CCB Timer Entry */

    tL2C_RCB            *p_rcb;                 /* Registration CB for this Channel */
    uint8_t                should_free_rcb;        /* True if RCB was allocated on the heap */

#define IB_CFG_DONE     0x01
#define OB_CFG_DONE     0x02
#define RECONFIG_FLAG   0x04                    /* True after initial configuration */
#define CFG_DONE_MASK   (IB_CFG_DONE | OB_CFG_DONE)

    uint8_t               config_done;            /* Configuration flag word         */
    uint8_t               local_id;               /* Transaction ID for local trans  */
    uint8_t               remote_id;              /* Transaction ID for local  */

#define CCB_FLAG_NO_RETRY       0x01            /* no more retry */
#define CCB_FLAG_SENT_PENDING   0x02            /* already sent pending response */
    uint8_t               flags;

    tL2CAP_CFG_INFO     our_cfg;                /* Our saved configuration options    */
    tL2CAP_CH_CFG_BITS  peer_cfg_bits;          /* Store what peer wants to configure */
    tL2CAP_CFG_INFO     peer_cfg;               /* Peer's saved configuration options */

    fixed_queue_t       *xmit_hold_q;            /* Transmit data hold queue         */
    uint8_t             cong_sent;              /* Set when congested status sent   */
    uint16_t              buff_quota;             /* Buffer quota before sending congestion   */

    tL2CAP_CHNL_PRIORITY ccb_priority;          /* Channel priority                 */
    tL2CAP_CHNL_DATA_RATE tx_data_rate;         /* Channel Tx data rate             */
    tL2CAP_CHNL_DATA_RATE rx_data_rate;         /* Channel Rx data rate             */

    /* Fields used for eL2CAP */
    tL2CAP_ERTM_INFO    ertm_info;
    tL2C_FCRB           fcrb;
    uint16_t              tx_mps;                 /* TX MPS adjusted based on current controller */
    uint16_t              max_rx_mtu;
    uint8_t               fcr_cfg_tries;          /* Max number of negotiation attempts */
    uint8_t             peer_cfg_already_rejected; /* If mode rejected once, set to TRUE */
    uint8_t             out_cfg_fcr_present;    /* TRUE if cfg response shoulkd include fcr options */

#define L2CAP_CFG_FCS_OUR   0x01                /* Our desired config FCS option */
#define L2CAP_CFG_FCS_PEER  0x02                /* Peer's desired config FCS option */
#define L2CAP_BYPASS_FCS    (L2CAP_CFG_FCS_OUR | L2CAP_CFG_FCS_PEER)
    uint8_t               bypass_fcs;

#if (L2CAP_NON_FLUSHABLE_PB_INCLUDED == TRUE)
    uint8_t             is_flushable;                   /* TRUE if channel is flushable     */
#endif

#if (L2CAP_NUM_FIXED_CHNLS > 0) || (L2CAP_UCD_INCLUDED == TRUE)
    uint16_t              fixed_chnl_idle_tout;   /* Idle timeout to use for the fixed channel       */
#endif
    uint16_t              tx_data_len;
} tL2C_CCB;

/***********************************************************************
** Define a queue of linked CCBs.
*/
typedef struct {
    tL2C_CCB        *p_first_ccb;               /* The first channel in this queue */
    tL2C_CCB        *p_last_ccb;                /* The last  channel in this queue */
} tL2C_CCB_Q;

#if (L2CAP_ROUND_ROBIN_CHANNEL_SERVICE == TRUE)

/* Round-Robin service for the same priority channels */
#define L2CAP_NUM_CHNL_PRIORITY     3           /* Total number of priority group (high, medium, low)*/
#define L2CAP_CHNL_PRIORITY_WEIGHT  5           /* weight per priority for burst transmission quota */
#define L2CAP_GET_PRIORITY_QUOTA(pri) ((L2CAP_NUM_CHNL_PRIORITY - (pri)) * L2CAP_CHNL_PRIORITY_WEIGHT)

/* CCBs within the same LCB are served in round robin with priority                       */
/* It will make sure that low priority channel (for example, HF signaling on RFCOMM)      */
/* can be sent to headset even if higher priority channel (for example, AV media channel) */
/* is congested.                                                                          */

typedef struct {
    tL2C_CCB        *p_serve_ccb;               /* current serving ccb within priority group */
    tL2C_CCB        *p_first_ccb;               /* first ccb of priority group */
    uint8_t           num_ccb;                    /* number of channels in priority group */
    uint8_t           quota;                      /* burst transmission quota */
} tL2C_RR_SERV;

#endif /* (L2CAP_ROUND_ROBIN_CHANNEL_SERVICE == TRUE) */

/* Define a link control block. There is one link control block between
** this device and any other device (i.e. BD ADDR).
*/
typedef struct t_l2c_linkcb {
    uint8_t             in_use;                     /* TRUE when in use, FALSE when not */
    tL2C_LINK_STATE     link_state;

    TIMER_LIST_ENT l2c_lcb_timer;             /* Timer entry for timeout evt */
    uint16_t              handle;                     /* The handle used with LM          */

    tL2C_CCB_Q          ccb_queue;                  /* Queue of CCBs on this LCB        */

    tL2C_CCB            *p_pending_ccb;             /* ccb of waiting channel during link disconnect */
    TIMER_LIST_ENT info_resp_timer;           /* Timer entry for info resp timeout evt */
    BD_ADDR             remote_bd_addr;             /* The BD address of the remote     */

    uint8_t               link_role;                  /* Master or slave                  */
    uint8_t               id;
    uint8_t               cur_echo_id;                /* Current id value for echo request */
    tL2CA_ECHO_RSP_CB   *p_echo_rsp_cb;             /* Echo response callback           */
    uint16_t              idle_timeout;               /* Idle timeout                     */
    uint8_t             is_bonding;                 /* True - link active only for bonding */

    uint16_t              link_flush_tout;            /* Flush timeout used               */

    uint16_t              link_xmit_quota;            /* Num outstanding pkts allowed     */
    uint16_t              sent_not_acked;             /* Num packets sent but not acked   */

    uint8_t             partial_segment_being_sent; /* Set TRUE when a partial segment  */
    /* is being sent.                   */
    uint8_t             w4_info_rsp;                /* TRUE when info request is active */
    uint8_t               info_rx_bits;               /* set 1 if received info type */
    uint32_t              peer_ext_fea;               /* Peer's extended features mask    */
    list_t              *link_xmit_data_q;          /* Link transmit data buffer queue  */

    uint8_t               peer_chnl_mask[L2CAP_FIXED_CHNL_ARRAY_SIZE];
#if (L2CAP_UCD_INCLUDED == TRUE)
    uint16_t              ucd_mtu;                    /* peer MTU on UCD */
    fixed_queue_t       *ucd_out_sec_pending_q;     /* Security pending outgoing UCD packet  */
    fixed_queue_t       *ucd_in_sec_pending_q;       /* Security pending incoming UCD packet  */
#endif

    BT_HDR              *p_hcit_rcv_acl;            /* Current HCIT ACL buf being rcvd  */
    uint16_t              idle_timeout_sv;            /* Save current Idle timeout        */
    uint8_t               acl_priority;               /* L2C_PRIORITY_NORMAL or L2C_PRIORITY_HIGH */
    tL2CA_NOCP_CB       *p_nocp_cb;                 /* Num Cmpl pkts callback           */

#if (L2CAP_NUM_FIXED_CHNLS > 0)
    tL2C_CCB            *p_fixed_ccbs[L2CAP_NUM_FIXED_CHNLS];
    uint16_t              disc_reason;
#endif

    tBT_TRANSPORT       transport;
#if (BLE_INCLUDED == TRUE)
    tBLE_ADDR_TYPE      ble_addr_type;
    uint16_t              tx_data_len;            /* tx data length used in data length extension */
    fixed_queue_t
    *le_sec_pending_q;      /* LE coc channels waiting for security check completion */
    uint8_t               sec_act;
#define L2C_BLE_CONN_UPDATE_DISABLE 0x1  /* disable update connection parameters */
#define L2C_BLE_NEW_CONN_PARAM      0x2  /* new connection parameter to be set */
#define L2C_BLE_UPDATE_PENDING      0x4  /* waiting for connection update finished */
#define L2C_BLE_NOT_DEFAULT_PARAM   0x8  /* not using default connection parameters */
    uint8_t               conn_update_mask;

    uint16_t              min_interval; /* parameters as requested by peripheral */
    uint16_t              max_interval;
    uint16_t              latency;
    uint16_t              timeout;

#endif

#if (L2CAP_ROUND_ROBIN_CHANNEL_SERVICE == TRUE)
    /* each priority group is limited burst transmission  */
    /* round robin service for the same priority channels */
    tL2C_RR_SERV        rr_serv[L2CAP_NUM_CHNL_PRIORITY];
    uint8_t               rr_pri;                             /* current serving priority group */
#endif

} tL2C_LCB;

/* Define the L2CAP control structure
*/
typedef struct {
    uint8_t           l2cap_trace_level;
    uint16_t          controller_xmit_window;         /* Total ACL window for all links   */

    uint16_t          round_robin_quota;              /* Round-robin link quota           */
    uint16_t          round_robin_unacked;            /* Round-robin unacked              */
    uint8_t         check_round_robin;              /* Do a round robin check           */

    uint8_t         is_cong_cback_context;

    tL2C_LCB        lcb_pool[MAX_L2CAP_LINKS];      /* Link Control Block pool          */
    tL2C_CCB        ccb_pool[MAX_L2CAP_CHANNELS];   /* Channel Control Block pool       */
    tL2C_RCB        rcb_pool[MAX_L2CAP_CLIENTS];    /* Registration info pool           */

    tL2C_CCB        *p_free_ccb_first;              /* Pointer to first free CCB        */
    tL2C_CCB        *p_free_ccb_last;               /* Pointer to last  free CCB        */

    uint8_t
    desire_role;                    /* desire to be master/slave when accepting a connection */
    uint8_t         disallow_switch;                /* FALSE, to allow switch at create conn */
    uint16_t          num_lm_acl_bufs;                /* # of ACL buffers on controller   */
    uint16_t          idle_timeout;                   /* Idle timeout                     */

    list_t          *rcv_pending_q;                 /* Recv pending queue               */
    TIMER_LIST_ENT receive_hold_timer;            /* Timer entry for rcv hold    */

    tL2C_LCB        *p_cur_hcit_lcb;                /* Current HCI Transport buffer     */
    uint16_t          num_links_active;               /* Number of links active           */

#if (L2CAP_NON_FLUSHABLE_PB_INCLUDED == TRUE)
    uint16_t
    non_flushable_pbf;              /* L2CAP_PKT_START_NON_FLUSHABLE if controller supports */
    /* Otherwise, L2CAP_PKT_START */
    uint8_t         is_flush_active;                /* TRUE if an HCI_Enhanced_Flush has been sent */
#endif

#if L2CAP_CONFORMANCE_TESTING == TRUE
    uint32_t          test_info_resp;                 /* Conformance testing needs a dynamic response */
#endif

#if (L2CAP_NUM_FIXED_CHNLS > 0)
    tL2CAP_FIXED_CHNL_REG   fixed_reg[L2CAP_NUM_FIXED_CHNLS];   /* Reg info for fixed channels */
#endif

#if (BLE_INCLUDED == TRUE)
    uint16_t
    num_ble_links_active;               /* Number of LE links active           */
    uint8_t                  is_ble_connecting;
    BD_ADDR                  ble_connecting_bda;
    uint16_t                   controller_le_xmit_window;         /* Total ACL window for all links   */
    tL2C_BLE_FIXED_CHNLS_MASK l2c_ble_fixed_chnls_mask;         // LE fixed channels mask
    uint16_t                   num_lm_ble_bufs;                   /* # of ACL buffers on controller   */
    uint16_t
    ble_round_robin_quota;              /* Round-robin link quota           */
    uint16_t
    ble_round_robin_unacked;            /* Round-robin unacked              */
    uint8_t                  ble_check_round_robin;              /* Do a round robin check           */
    tL2C_RCB                 ble_rcb_pool[BLE_MAX_L2CAP_CLIENTS]; /* Registration info pool           */
#endif

    tL2CA_ECHO_DATA_CB      *p_echo_data_cb;                /* Echo data callback */

#if (defined(L2CAP_HIGH_PRI_CHAN_QUOTA_IS_CONFIGURABLE) && (L2CAP_HIGH_PRI_CHAN_QUOTA_IS_CONFIGURABLE == TRUE))
    uint16_t
    high_pri_min_xmit_quota;    /* Minimum number of ACL credit for high priority link */
#endif /* (L2CAP_HIGH_PRI_CHAN_QUOTA_IS_CONFIGURABLE == TRUE) */

    uint16_t          dyn_psm;
} tL2C_CB;



/* Define a structure that contains the information about a connection.
** This structure is used to pass between functions, and not all the
** fields will always be filled in.
*/
typedef struct {
    BD_ADDR         bd_addr;                        /* Remote BD address        */
    uint8_t           status;                         /* Connection status        */
    uint16_t          psm;                            /* PSM of the connection    */
    uint16_t          l2cap_result;                   /* L2CAP result             */
    uint16_t          l2cap_status;                   /* L2CAP status             */
    uint16_t          remote_cid;                     /* Remote CID               */
} tL2C_CONN_INFO;


typedef void (tL2C_FCR_MGMT_EVT_HDLR)(uint8_t, tL2C_CCB *);

/* The offset in a buffer that L2CAP will use when building commands.
*/
#define L2CAP_SEND_CMD_OFFSET       0


/* Number of ACL buffers to use for high priority channel
*/
#if (!defined(L2CAP_HIGH_PRI_CHAN_QUOTA_IS_CONFIGURABLE) || (L2CAP_HIGH_PRI_CHAN_QUOTA_IS_CONFIGURABLE == FALSE))
#define L2CAP_HIGH_PRI_MIN_XMIT_QUOTA_A     (L2CAP_HIGH_PRI_MIN_XMIT_QUOTA)
#else
#define L2CAP_HIGH_PRI_MIN_XMIT_QUOTA_A     (l2cb.high_pri_min_xmit_quota)
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* L2CAP global data
************************************
*/
#if (!defined L2C_DYNAMIC_MEMORY) || (L2C_DYNAMIC_MEMORY == FALSE)
extern tL2C_CB  l2cb;
#else
extern tL2C_CB *l2c_cb_ptr;
#define l2cb (*l2c_cb_ptr)
#endif


/* Functions provided by l2c_main.c
************************************
*/
void l2c_init(void);
void l2c_free(void);

extern void     l2c_receive_hold_timer_timeout(void *data);
extern void     l2c_ccb_timer_timeout(void *data);
extern void     l2c_lcb_timer_timeout(void *data);
extern void     l2c_fcrb_ack_timer_timeout(void *data);
extern uint8_t    l2c_data_write(uint16_t cid, BT_HDR *p_data, uint16_t flag);
extern void     l2c_rcv_acl_data(BT_HDR *p_msg);
extern void     l2c_process_held_packets(uint8_t timed_out);

/* Functions provided by l2c_utils.c
************************************
*/
extern tL2C_LCB *l2cu_allocate_lcb(BD_ADDR p_bd_addr, uint8_t is_bonding, tBT_TRANSPORT transport);
extern uint8_t  l2cu_start_post_bond_timer(uint16_t handle);
extern void     l2cu_release_lcb(tL2C_LCB *p_lcb);
extern tL2C_LCB *l2cu_find_lcb_by_bd_addr(BD_ADDR p_bd_addr, tBT_TRANSPORT transport);
extern tL2C_LCB *l2cu_find_lcb_by_handle(uint16_t handle);
extern void     l2cu_update_lcb_4_bonding(BD_ADDR p_bd_addr, uint8_t is_bonding);

extern uint8_t    l2cu_get_conn_role(tL2C_LCB *p_this_lcb);
extern uint8_t  l2cu_set_acl_priority(BD_ADDR bd_addr, uint8_t priority, uint8_t reset_after_rs);

extern void     l2cu_enqueue_ccb(tL2C_CCB *p_ccb);
extern void     l2cu_dequeue_ccb(tL2C_CCB *p_ccb);
extern void     l2cu_change_pri_ccb(tL2C_CCB *p_ccb, tL2CAP_CHNL_PRIORITY priority);

extern tL2C_CCB *l2cu_allocate_ccb(tL2C_LCB *p_lcb, uint16_t cid);
extern void     l2cu_release_ccb(tL2C_CCB *p_ccb);
extern tL2C_CCB *l2cu_find_ccb_by_cid(tL2C_LCB *p_lcb, uint16_t local_cid);
extern tL2C_CCB *l2cu_find_ccb_by_remote_cid(tL2C_LCB *p_lcb, uint16_t remote_cid);
extern void     l2cu_adj_id(tL2C_LCB *p_lcb, uint8_t adj_mask);
extern uint8_t  l2c_is_cmd_rejected(uint8_t cmd_code, uint8_t id, tL2C_LCB *p_lcb);

extern void     l2cu_send_peer_cmd_reject(tL2C_LCB *p_lcb, uint16_t reason,
        uint8_t rem_id, uint16_t p1, uint16_t p2);
extern void     l2cu_send_peer_connect_req(tL2C_CCB *p_ccb);
extern void     l2cu_send_peer_connect_rsp(tL2C_CCB *p_ccb, uint16_t result, uint16_t status);
extern void     l2cu_send_peer_config_req(tL2C_CCB *p_ccb, tL2CAP_CFG_INFO *p_cfg);
extern void     l2cu_send_peer_config_rsp(tL2C_CCB *p_ccb, tL2CAP_CFG_INFO *p_cfg);
extern void     l2cu_send_peer_config_rej(tL2C_CCB *p_ccb, uint8_t *p_data, uint16_t data_len,
        uint16_t rej_len);
extern void     l2cu_send_peer_disc_req(tL2C_CCB *p_ccb);
extern void     l2cu_send_peer_disc_rsp(tL2C_LCB *p_lcb, uint8_t remote_id, uint16_t local_cid,
                                        uint16_t remote_cid);
extern void     l2cu_send_peer_echo_req(tL2C_LCB *p_lcb, uint8_t *p_data, uint16_t data_len);
extern void     l2cu_send_peer_echo_rsp(tL2C_LCB *p_lcb, uint8_t id, uint8_t *p_data,
                                        uint16_t data_len);
extern void     l2cu_send_peer_info_rsp(tL2C_LCB *p_lcb, uint8_t id, uint16_t info_type);
extern void     l2cu_reject_connection(tL2C_LCB *p_lcb, uint16_t remote_cid, uint8_t rem_id,
                                       uint16_t result);
extern void     l2cu_send_peer_info_req(tL2C_LCB *p_lcb, uint16_t info_type);
extern void     l2cu_set_acl_hci_header(BT_HDR *p_buf, tL2C_CCB *p_ccb);
extern void     l2cu_check_channel_congestion(tL2C_CCB *p_ccb);
extern void     l2cu_disconnect_chnl(tL2C_CCB *p_ccb);

#if (L2CAP_NON_FLUSHABLE_PB_INCLUDED == TRUE)
extern void     l2cu_set_non_flushable_pbf(uint8_t);
#endif

#if (BLE_INCLUDED == TRUE)
extern void l2cu_send_peer_ble_par_req(tL2C_LCB *p_lcb, uint16_t min_int, uint16_t max_int,
                                       uint16_t latency, uint16_t timeout);
extern void l2cu_send_peer_ble_par_rsp(tL2C_LCB *p_lcb, uint16_t reason, uint8_t rem_id);
extern void l2cu_reject_ble_connection(tL2C_LCB *p_lcb, uint8_t rem_id, uint16_t result);
extern void l2cu_send_peer_ble_credit_based_conn_res(tL2C_CCB *p_ccb, uint16_t result);
extern void l2cu_send_peer_ble_credit_based_conn_req(tL2C_CCB *p_ccb);
extern void l2cu_send_peer_ble_flow_control_credit(tL2C_CCB *p_ccb, uint16_t credit_value);
extern void l2cu_send_peer_ble_credit_based_disconn_req(tL2C_CCB *p_ccb);
#endif

extern uint8_t l2cu_initialize_fixed_ccb(tL2C_LCB *p_lcb, uint16_t fixed_cid,
        tL2CAP_FCR_OPTS *p_fcr);
extern void    l2cu_no_dynamic_ccbs(tL2C_LCB *p_lcb);
extern void    l2cu_process_fixed_chnl_resp(tL2C_LCB *p_lcb);

/* Functions provided by l2c_ucd.c
************************************
*/
#if (L2CAP_UCD_INCLUDED == TRUE)
void l2c_ucd_delete_sec_pending_q(tL2C_LCB  *p_lcb);
void l2c_ucd_enqueue_pending_out_sec_q(tL2C_CCB  *p_ccb, void *p_data);
uint8_t l2c_ucd_check_pending_info_req(tL2C_CCB  *p_ccb);
uint8_t l2c_ucd_check_pending_out_sec_q(tL2C_CCB  *p_ccb);
void l2c_ucd_send_pending_out_sec_q(tL2C_CCB  *p_ccb);
void l2c_ucd_discard_pending_out_sec_q(tL2C_CCB  *p_ccb);
uint8_t l2c_ucd_check_pending_in_sec_q(tL2C_CCB  *p_ccb);
void l2c_ucd_send_pending_in_sec_q(tL2C_CCB  *p_ccb);
void l2c_ucd_discard_pending_in_sec_q(tL2C_CCB  *p_ccb);
uint8_t l2c_ucd_check_rx_pkts(tL2C_LCB  *p_lcb, BT_HDR *p_msg);
uint8_t l2c_ucd_process_event(tL2C_CCB *p_ccb, uint16_t event, void *p_data);
#endif

/* Functions provided for Broadcom Aware
****************************************
*/
extern uint8_t  l2cu_check_feature_req(tL2C_LCB *p_lcb, uint8_t id, uint8_t *p_data,
                                       uint16_t data_len);
extern void     l2cu_check_feature_rsp(tL2C_LCB *p_lcb, uint8_t id, uint8_t *p_data,
                                       uint16_t data_len);
extern void     l2cu_send_feature_req(tL2C_CCB *p_ccb);

extern tL2C_RCB *l2cu_allocate_rcb(uint16_t psm);
extern tL2C_RCB *l2cu_find_rcb_by_psm(uint16_t psm);
extern void     l2cu_release_rcb(tL2C_RCB *p_rcb);
extern tL2C_RCB *l2cu_allocate_ble_rcb(uint16_t psm);
extern tL2C_RCB *l2cu_find_ble_rcb_by_psm(uint16_t psm);

extern uint8_t    l2cu_process_peer_cfg_req(tL2C_CCB *p_ccb, tL2CAP_CFG_INFO *p_cfg);
extern void     l2cu_process_peer_cfg_rsp(tL2C_CCB *p_ccb, tL2CAP_CFG_INFO *p_cfg);
extern void     l2cu_process_our_cfg_req(tL2C_CCB *p_ccb, tL2CAP_CFG_INFO *p_cfg);
extern void     l2cu_process_our_cfg_rsp(tL2C_CCB *p_ccb, tL2CAP_CFG_INFO *p_cfg);

extern void     l2cu_device_reset(void);
extern tL2C_LCB *l2cu_find_lcb_by_state(tL2C_LINK_STATE state);
extern uint8_t  l2cu_lcb_disconnecting(void);

extern uint8_t l2cu_create_conn(tL2C_LCB *p_lcb, tBT_TRANSPORT transport);
extern uint8_t l2cu_create_conn_after_switch(tL2C_LCB *p_lcb);
extern BT_HDR *l2cu_get_next_buffer_to_send(tL2C_LCB *p_lcb);
extern void    l2cu_resubmit_pending_sec_req(BD_ADDR p_bda);
extern void    l2cu_initialize_amp_ccb(tL2C_LCB *p_lcb);
extern void    l2cu_adjust_out_mps(tL2C_CCB *p_ccb);

/* Functions provided by l2c_link.c
************************************
*/
extern uint8_t  l2c_link_hci_conn_req(BD_ADDR bd_addr);
extern uint8_t  l2c_link_hci_conn_comp(uint8_t status, uint16_t handle, BD_ADDR p_bda);
extern uint8_t  l2c_link_hci_disc_comp(uint16_t handle, uint8_t reason);
extern uint8_t  l2c_link_hci_qos_violation(uint16_t handle);
extern void     l2c_link_timeout(tL2C_LCB *p_lcb);
extern void     l2c_info_resp_timer_timeout(void *data);
extern void     l2c_link_check_send_pkts(tL2C_LCB *p_lcb, tL2C_CCB *p_ccb, BT_HDR *p_buf);
extern void     l2c_link_adjust_allocation(void);
extern void     l2c_link_process_num_completed_pkts(uint8_t *p);
extern void     l2c_link_process_num_completed_blocks(uint8_t controller_id, uint8_t *p,
        uint16_t evt_len);
extern void     l2c_link_processs_num_bufs(uint16_t num_lm_acl_bufs);
extern uint8_t    l2c_link_pkts_rcvd(uint16_t *num_pkts, uint16_t *handles);
extern void     l2c_link_role_changed(BD_ADDR bd_addr, uint8_t new_role, uint8_t hci_status);
extern void     l2c_link_sec_comp(BD_ADDR p_bda, tBT_TRANSPORT trasnport, void *p_ref_data,
                                  uint8_t status);
extern void     l2c_link_segments_xmitted(BT_HDR *p_msg);
extern void     l2c_pin_code_request(BD_ADDR bd_addr);
extern void     l2c_link_adjust_chnl_allocation(void);

#if (BLE_INCLUDED == TRUE)
extern void     l2c_link_processs_ble_num_bufs(uint16_t num_lm_acl_bufs);
#endif

#if L2CAP_WAKE_PARKED_LINK == TRUE
extern uint8_t  l2c_link_check_power_mode(tL2C_LCB *p_lcb);
#define L2C_LINK_CHECK_POWER_MODE(x) l2c_link_check_power_mode ((x))
#else  // L2CAP_WAKE_PARKED_LINK
#define L2C_LINK_CHECK_POWER_MODE(x) (FALSE)
#endif  // L2CAP_WAKE_PARKED_LINK

#if L2CAP_CONFORMANCE_TESTING == TRUE
/* Used only for conformance testing */
extern void l2cu_set_info_rsp_mask(uint32_t mask);
#endif

/* Functions provided by l2c_csm.c
************************************
*/
extern void l2c_csm_execute(tL2C_CCB *p_ccb, uint16_t event, void *p_data);

extern void l2c_enqueue_peer_data(tL2C_CCB *p_ccb, BT_HDR *p_buf);


/* Functions provided by l2c_fcr.c
************************************
*/
extern void     l2c_fcr_cleanup(tL2C_CCB *p_ccb);
extern void     l2c_fcr_proc_pdu(tL2C_CCB *p_ccb, BT_HDR *p_buf);
extern void     l2c_fcr_proc_tout(tL2C_CCB *p_ccb);
extern void     l2c_fcr_proc_ack_tout(tL2C_CCB *p_ccb);
extern void     l2c_fcr_send_S_frame(tL2C_CCB *p_ccb, uint16_t function_code, uint16_t pf_bit);
extern BT_HDR   *l2c_fcr_clone_buf(BT_HDR *p_buf, uint16_t new_offset, uint16_t no_of_bytes);
extern uint8_t  l2c_fcr_is_flow_controlled(tL2C_CCB *p_ccb);
extern BT_HDR   *l2c_fcr_get_next_xmit_sdu_seg(tL2C_CCB *p_ccb, uint16_t max_packet_length);
extern void     l2c_fcr_start_timer(tL2C_CCB *p_ccb);
extern void     l2c_lcc_proc_pdu(tL2C_CCB *p_ccb, BT_HDR *p_buf);
extern BT_HDR   *l2c_lcc_get_next_xmit_sdu_seg(tL2C_CCB *p_ccb, uint16_t max_packet_length);

/* Configuration negotiation */
extern uint8_t    l2c_fcr_chk_chan_modes(tL2C_CCB *p_ccb);
extern uint8_t  l2c_fcr_adj_our_req_options(tL2C_CCB *p_ccb, tL2CAP_CFG_INFO *p_cfg);
extern void     l2c_fcr_adj_our_rsp_options(tL2C_CCB *p_ccb, tL2CAP_CFG_INFO *p_peer_cfg);
extern uint8_t  l2c_fcr_renegotiate_chan(tL2C_CCB *p_ccb, tL2CAP_CFG_INFO *p_cfg);
extern uint8_t    l2c_fcr_process_peer_cfg_req(tL2C_CCB *p_ccb, tL2CAP_CFG_INFO *p_cfg);
extern void     l2c_fcr_adj_monitor_retran_timeout(tL2C_CCB *p_ccb);
extern void     l2c_fcr_stop_timer(tL2C_CCB *p_ccb);

/* Functions provided by l2c_ble.c
************************************
*/
#if (BLE_INCLUDED == TRUE)
extern uint8_t l2cble_create_conn(tL2C_LCB *p_lcb);
extern void l2cble_process_sig_cmd(tL2C_LCB *p_lcb, uint8_t *p, uint16_t pkt_len);
extern void l2cble_conn_comp(uint16_t handle, uint8_t role, BD_ADDR bda, tBLE_ADDR_TYPE type,
                             uint16_t conn_interval, uint16_t conn_latency, uint16_t conn_timeout);
extern uint8_t l2cble_init_direct_conn(tL2C_LCB *p_lcb);
extern void l2cble_notify_le_connection(BD_ADDR bda);
extern void l2c_ble_link_adjust_allocation(void);
extern void l2cble_process_conn_update_evt(uint16_t handle, uint8_t status);

extern void l2cble_credit_based_conn_req(tL2C_CCB *p_ccb);
extern void l2cble_credit_based_conn_res(tL2C_CCB *p_ccb, uint16_t result);
extern void l2cble_send_peer_disc_req(tL2C_CCB *p_ccb);
extern void l2cble_send_flow_control_credit(tL2C_CCB *p_ccb, uint16_t credit_value);
extern uint8_t l2ble_sec_access_req(BD_ADDR bd_addr, uint16_t psm, uint8_t is_originator,
                                    tL2CAP_SEC_CBACK *p_callback, void *p_ref_data);

#if (defined BLE_LLT_INCLUDED) && (BLE_LLT_INCLUDED == TRUE)
extern void l2cble_process_rc_param_request_evt(uint16_t handle, uint16_t int_min, uint16_t int_max,
        uint16_t latency, uint16_t timeout);
#endif

extern void l2cble_update_data_length(tL2C_LCB *p_lcb);
extern uint8_t l2cble_set_fixed_channel_tx_data_length(BD_ADDR remote_bda, uint16_t fix_cid,
        uint16_t tx_mtu);
extern void l2cble_process_data_length_change_event(uint16_t handle, uint16_t tx_data_len,
        uint16_t rx_data_len);

#endif
extern void l2cu_process_fixed_disc_cback(tL2C_LCB *p_lcb);

#ifdef __cplusplus
}
#endif

#endif
