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

#ifndef HCIMSGS_H
#define HCIMSGS_H

#include "bt_target.h"
#include "hcidefs.h"
#include "bt_types.h"

void bte_main_hci_send(BT_HDR *p_msg, uint16_t event);
void bte_main_lpm_allow_bt_device_sleep(void);

/* Message by message.... */

#define HCIC_GET_UINT8(p, off)    (uint8_t)(*((uint8_t *)((p) + 1) + p->offset + 3 + (off)))

#define HCIC_GET_UINT16(p, off)  (uint16_t)((*((uint8_t *)((p) + 1) + p->offset + 3 + (off)) + \
        (*((uint8_t *)((p) + 1) + p->offset + 3 + (off) + 1) << 8)))

#define HCIC_GET_UINT32(p, off)  (uint32_t)((*((uint8_t *)((p) + 1) + p->offset + 3 + (off)) + \
        (*((uint8_t *)((p) + 1) + p->offset + 3 + (off) + 1) << 8) + \
        (*((uint8_t *)((p) + 1) + p->offset + 3 + (off) + 2) << 16) + \
        (*((uint8_t *)((p) + 1) + p->offset + 3 + (off) + 3) << 24)))

#define HCIC_GET_ARRAY(p, off, x, len) \
    { \
        uint8_t *qq = ((uint8_t *)((p) + 1) + p->offset + 3 + (off)); uint8_t *rr = (uint8_t *)x; \
        int ii; for (ii = 0; ii < len; ii++) *rr++ = *qq++; \
    }

#define HCIC_GET_ARRAY16(p, off, x) \
    { \
        uint8_t *qq = ((uint8_t *)((p) + 1) + p->offset + 3 + (off)); uint8_t *rr = (uint8_t *)x + 15; \
        int ii; for (ii = 0; ii < 16; ii++) *rr-- = *qq++; \
    }

#define HCIC_GET_BDADDR(p, off, x) \
    { \
        uint8_t *qq = ((uint8_t *)((p) + 1) + p->offset + 3 + (off)); uint8_t *rr = (uint8_t *)x + BD_ADDR_LEN - 1; \
        int ii; for (ii = 0; ii < BD_ADDR_LEN; ii++) *rr-- = *qq++; \
    }

#define HCIC_GET_DEVCLASS(p, off, x) \
    { \
        uint8_t *qq = ((uint8_t *)((p) + 1) + p->offset + 3 + (off)); uint8_t *rr = (uint8_t *)x + DEV_CLASS_LEN - 1; \
        int ii; for (ii = 0; ii < DEV_CLASS_LEN; ii++) *rr-- = *qq++; \
    }

#define HCIC_GET_LAP(p, off, x) \
    { \
        uint8_t *qq = ((uint8_t *)((p) + 1) + p->offset + 3 + (off)); uint8_t *rr = (uint8_t *)x + LAP_LEN - 1; \
        int ii; for (ii = 0; ii < LAP_LEN; ii++) *rr-- = *qq++; \
    }

#define HCIC_GET_POINTER(p, off) ((uint8_t *)((p) + 1) + p->offset + 3 + (off))


extern uint8_t btsnd_hcic_inquiry(const LAP inq_lap, uint8_t duration,
                                  uint8_t response_cnt);

#define HCIC_PARAM_SIZE_INQUIRY 5


#define HCIC_INQ_INQ_LAP_OFF    0
#define HCIC_INQ_DUR_OFF        3
#define HCIC_INQ_RSP_CNT_OFF    4
/* Inquiry */

/* Inquiry Cancel */
extern uint8_t btsnd_hcic_inq_cancel(void);

#define HCIC_PARAM_SIZE_INQ_CANCEL   0

/* Periodic Inquiry Mode */
extern uint8_t btsnd_hcic_per_inq_mode(uint16_t max_period, uint16_t min_period,
                                       const LAP inq_lap, uint8_t duration,
                                       uint8_t response_cnt);

#define HCIC_PARAM_SIZE_PER_INQ_MODE    9

#define HCI_PER_INQ_MAX_INTRVL_OFF  0
#define HCI_PER_INQ_MIN_INTRVL_OFF  2
#define HCI_PER_INQ_INQ_LAP_OFF     4
#define HCI_PER_INQ_DURATION_OFF    7
#define HCI_PER_INQ_RSP_CNT_OFF     8
/* Periodic Inquiry Mode */

/* Exit Periodic Inquiry Mode */
extern uint8_t btsnd_hcic_exit_per_inq(void);

#define HCIC_PARAM_SIZE_EXIT_PER_INQ   0
/* Create Connection */
extern uint8_t btsnd_hcic_create_conn(BD_ADDR dest, uint16_t packet_types,
                                      uint8_t page_scan_rep_mode,
                                      uint8_t page_scan_mode,
                                      uint16_t clock_offset,
                                      uint8_t allow_switch);

#define HCIC_PARAM_SIZE_CREATE_CONN  13

#define HCIC_CR_CONN_BD_ADDR_OFF        0
#define HCIC_CR_CONN_PKT_TYPES_OFF      6
#define HCIC_CR_CONN_REP_MODE_OFF       8
#define HCIC_CR_CONN_PAGE_SCAN_MODE_OFF 9
#define HCIC_CR_CONN_CLK_OFF_OFF        10
#define HCIC_CR_CONN_ALLOW_SWITCH_OFF   12
/* Create Connection */

/* Disconnect */
extern uint8_t btsnd_hcic_disconnect(uint16_t handle, uint8_t reason);

#define HCIC_PARAM_SIZE_DISCONNECT 3

#define HCI_DISC_HANDLE_OFF             0
#define HCI_DISC_REASON_OFF             2
/* Disconnect */

#if BTM_SCO_INCLUDED == TRUE
/* Add SCO Connection */
extern uint8_t btsnd_hcic_add_SCO_conn(uint16_t handle, uint16_t packet_types);
#endif /* BTM_SCO_INCLUDED */

#define HCIC_PARAM_SIZE_ADD_SCO_CONN    4

#define HCI_ADD_SCO_HANDLE_OFF          0
#define HCI_ADD_SCO_PACKET_TYPES_OFF    2
/* Add SCO Connection */

/* Create Connection Cancel */
extern uint8_t btsnd_hcic_create_conn_cancel(BD_ADDR dest);

#define HCIC_PARAM_SIZE_CREATE_CONN_CANCEL  6

#define HCIC_CR_CONN_CANCEL_BD_ADDR_OFF     0
/* Create Connection Cancel */

/* Accept Connection Request */
extern uint8_t btsnd_hcic_accept_conn(BD_ADDR bd_addr, uint8_t role);

#define HCIC_PARAM_SIZE_ACCEPT_CONN     7

#define HCI_ACC_CONN_BD_ADDR_OFF        0
#define HCI_ACC_CONN_ROLE_OFF           6
/* Accept Connection Request */

/* Reject Connection Request */
extern uint8_t btsnd_hcic_reject_conn(BD_ADDR bd_addr, uint8_t reason);

#define HCIC_PARAM_SIZE_REJECT_CONN      7

#define HCI_REJ_CONN_BD_ADDR_OFF        0
#define HCI_REJ_CONN_REASON_OFF         6
/* Reject Connection Request */

/* Link Key Request Reply */
extern uint8_t btsnd_hcic_link_key_req_reply(BD_ADDR bd_addr,
        LINK_KEY link_key);

#define HCIC_PARAM_SIZE_LINK_KEY_REQ_REPLY   22

#define HCI_LINK_KEY_REPLY_BD_ADDR_OFF  0
#define HCI_LINK_KEY_REPLY_LINK_KEY_OFF 6
/* Link Key Request Reply  */

/* Link Key Request Neg Reply */
extern uint8_t btsnd_hcic_link_key_neg_reply(BD_ADDR bd_addr);

#define HCIC_PARAM_SIZE_LINK_KEY_NEG_REPLY   6

#define HCI_LINK_KEY_NEG_REP_BD_ADR_OFF 0
/* Link Key Request Neg Reply  */

/* PIN Code Request Reply */
extern uint8_t btsnd_hcic_pin_code_req_reply(BD_ADDR bd_addr,
        uint8_t pin_code_len,
        PIN_CODE pin_code);

#define HCIC_PARAM_SIZE_PIN_CODE_REQ_REPLY   23

#define HCI_PIN_CODE_REPLY_BD_ADDR_OFF  0
#define HCI_PIN_CODE_REPLY_PIN_LEN_OFF  6
#define HCI_PIN_CODE_REPLY_PIN_CODE_OFF 7
/* PIN Code Request Reply  */

/* Link Key Request Neg Reply */
extern uint8_t btsnd_hcic_pin_code_neg_reply(BD_ADDR bd_addr);

#define HCIC_PARAM_SIZE_PIN_CODE_NEG_REPLY   6

#define HCI_PIN_CODE_NEG_REP_BD_ADR_OFF 0
/* Link Key Request Neg Reply  */

/* Change Connection Type */
extern uint8_t btsnd_hcic_change_conn_type(uint16_t handle, uint16_t packet_types);

#define HCIC_PARAM_SIZE_CHANGE_CONN_TYPE     4

#define HCI_CHNG_PKT_TYPE_HANDLE_OFF    0
#define HCI_CHNG_PKT_TYPE_PKT_TYPE_OFF  2
/* Change Connection Type */

#define HCIC_PARAM_SIZE_CMD_HANDLE      2

#define HCI_CMD_HANDLE_HANDLE_OFF       0

extern uint8_t btsnd_hcic_auth_request(uint16_t handle);      /* Authentication Request */

/* Set Connection Encryption */
extern uint8_t btsnd_hcic_set_conn_encrypt(uint16_t handle, uint8_t enable);
#define HCIC_PARAM_SIZE_SET_CONN_ENCRYPT     3


#define HCI_SET_ENCRYPT_HANDLE_OFF      0
#define HCI_SET_ENCRYPT_ENABLE_OFF      2
/* Set Connection Encryption */

extern uint8_t btsnd_hcic_change_link_key(uint16_t handle);   /* Change Connection Link Key */

/* Master Link Key */
extern uint8_t btsnd_hcic_master_link_key(uint8_t key_flag);

#define HCIC_PARAM_SIZE_MASTER_LINK_KEY 1

#define HCI_MASTER_KEY_FLAG_OFF         0
/* Master Link Key */

/* Remote Name Request */
extern uint8_t btsnd_hcic_rmt_name_req(BD_ADDR bd_addr,
                                       uint8_t page_scan_rep_mode,
                                       uint8_t page_scan_mode,
                                       uint16_t clock_offset);

#define HCIC_PARAM_SIZE_RMT_NAME_REQ   10

#define HCI_RMT_NAME_BD_ADDR_OFF        0
#define HCI_RMT_NAME_REP_MODE_OFF       6
#define HCI_RMT_NAME_PAGE_SCAN_MODE_OFF 7
#define HCI_RMT_NAME_CLK_OFF_OFF        8
/* Remote Name Request */

/* Remote Name Request Cancel */
extern uint8_t btsnd_hcic_rmt_name_req_cancel(BD_ADDR bd_addr);

#define HCIC_PARAM_SIZE_RMT_NAME_REQ_CANCEL   6

#define HCI_RMT_NAME_CANCEL_BD_ADDR_OFF       0
/* Remote Name Request Cancel */

extern uint8_t btsnd_hcic_rmt_features_req(uint16_t handle);      /* Remote Features Request */

/* Remote Extended Features */
extern uint8_t btsnd_hcic_rmt_ext_features(uint16_t handle, uint8_t page_num);

#define HCIC_PARAM_SIZE_RMT_EXT_FEATURES   3

#define HCI_RMT_EXT_FEATURES_HANDLE_OFF    0
#define HCI_RMT_EXT_FEATURES_PAGE_NUM_OFF  2
/* Remote Extended Features */


/* Local Extended Features */
extern uint8_t btsnd_hcic_read_local_ext_features(uint8_t page_num);

#define HCIC_PARAM_SIZE_LOCAL_EXT_FEATURES     1

#define HCI_LOCAL_EXT_FEATURES_PAGE_NUM_OFF    0
/* Local Extended Features */


extern uint8_t btsnd_hcic_rmt_ver_req(uint16_t handle);           /* Remote Version Info Request */
extern uint8_t btsnd_hcic_read_rmt_clk_offset(uint16_t handle);   /* Remote Clock Offset */
extern uint8_t btsnd_hcic_read_lmp_handle(uint16_t handle);       /* Remote LMP Handle */

extern uint8_t btsnd_hcic_setup_esco_conn(uint16_t handle,
        uint32_t tx_bw, uint32_t rx_bw,
        uint16_t max_latency, uint16_t voice,
        uint8_t retrans_effort,
        uint16_t packet_types);
#define HCIC_PARAM_SIZE_SETUP_ESCO      17

#define HCI_SETUP_ESCO_HANDLE_OFF       0
#define HCI_SETUP_ESCO_TX_BW_OFF        2
#define HCI_SETUP_ESCO_RX_BW_OFF        6
#define HCI_SETUP_ESCO_MAX_LAT_OFF      10
#define HCI_SETUP_ESCO_VOICE_OFF        12
#define HCI_SETUP_ESCO_RETRAN_EFF_OFF   14
#define HCI_SETUP_ESCO_PKT_TYPES_OFF    15


extern uint8_t btsnd_hcic_accept_esco_conn(BD_ADDR bd_addr,
        uint32_t tx_bw, uint32_t rx_bw,
        uint16_t max_latency,
        uint16_t content_fmt,
        uint8_t retrans_effort,
        uint16_t packet_types);
#define HCIC_PARAM_SIZE_ACCEPT_ESCO     21

#define HCI_ACCEPT_ESCO_BDADDR_OFF      0
#define HCI_ACCEPT_ESCO_TX_BW_OFF       6
#define HCI_ACCEPT_ESCO_RX_BW_OFF       10
#define HCI_ACCEPT_ESCO_MAX_LAT_OFF     14
#define HCI_ACCEPT_ESCO_VOICE_OFF       16
#define HCI_ACCEPT_ESCO_RETRAN_EFF_OFF  18
#define HCI_ACCEPT_ESCO_PKT_TYPES_OFF   19


extern uint8_t btsnd_hcic_reject_esco_conn(BD_ADDR bd_addr, uint8_t reason);
#define HCIC_PARAM_SIZE_REJECT_ESCO     7

#define HCI_REJECT_ESCO_BDADDR_OFF      0
#define HCI_REJECT_ESCO_REASON_OFF      6

/* Hold Mode */
extern uint8_t btsnd_hcic_hold_mode(uint16_t handle, uint16_t max_hold_period,
                                    uint16_t min_hold_period);

#define HCIC_PARAM_SIZE_HOLD_MODE       6

#define HCI_HOLD_MODE_HANDLE_OFF        0
#define HCI_HOLD_MODE_MAX_PER_OFF       2
#define HCI_HOLD_MODE_MIN_PER_OFF       4
/* Hold Mode */

/* Sniff Mode */
extern uint8_t btsnd_hcic_sniff_mode(uint16_t handle,
                                     uint16_t max_sniff_period,
                                     uint16_t min_sniff_period,
                                     uint16_t sniff_attempt,
                                     uint16_t sniff_timeout);

#define HCIC_PARAM_SIZE_SNIFF_MODE      10


#define HCI_SNIFF_MODE_HANDLE_OFF       0
#define HCI_SNIFF_MODE_MAX_PER_OFF      2
#define HCI_SNIFF_MODE_MIN_PER_OFF      4
#define HCI_SNIFF_MODE_ATTEMPT_OFF      6
#define HCI_SNIFF_MODE_TIMEOUT_OFF      8
/* Sniff Mode */

extern uint8_t btsnd_hcic_exit_sniff_mode(uint16_t handle);       /* Exit Sniff Mode */

/* Park Mode */
extern uint8_t btsnd_hcic_park_mode(uint16_t handle,
                                    uint16_t beacon_max_interval,
                                    uint16_t beacon_min_interval);

#define HCIC_PARAM_SIZE_PARK_MODE       6

#define HCI_PARK_MODE_HANDLE_OFF        0
#define HCI_PARK_MODE_MAX_PER_OFF       2
#define HCI_PARK_MODE_MIN_PER_OFF       4
/* Park Mode */

extern uint8_t btsnd_hcic_exit_park_mode(uint16_t handle);   /* Exit Park Mode */

/* QoS Setup */
extern uint8_t btsnd_hcic_qos_setup(uint16_t handle, uint8_t flags,
                                    uint8_t service_type,
                                    uint32_t token_rate, uint32_t peak,
                                    uint32_t latency, uint32_t delay_var);

#define HCIC_PARAM_SIZE_QOS_SETUP       20

#define HCI_QOS_HANDLE_OFF              0
#define HCI_QOS_FLAGS_OFF               2
#define HCI_QOS_SERVICE_TYPE_OFF        3
#define HCI_QOS_TOKEN_RATE_OFF          4
#define HCI_QOS_PEAK_BANDWIDTH_OFF      8
#define HCI_QOS_LATENCY_OFF             12
#define HCI_QOS_DELAY_VAR_OFF           16
/* QoS Setup */

extern uint8_t btsnd_hcic_role_discovery(uint16_t handle);        /* Role Discovery */

/* Switch Role Request */
extern uint8_t btsnd_hcic_switch_role(BD_ADDR bd_addr, uint8_t role);

#define HCIC_PARAM_SIZE_SWITCH_ROLE  7

#define HCI_SWITCH_BD_ADDR_OFF          0
#define HCI_SWITCH_ROLE_OFF             6
/* Switch Role Request */

extern uint8_t btsnd_hcic_read_policy_set(uint16_t handle);       /* Read Policy Settings */

/* Write Policy Settings */
extern uint8_t btsnd_hcic_write_policy_set(uint16_t handle, uint16_t settings);

#define HCIC_PARAM_SIZE_WRITE_POLICY_SET     4

#define HCI_WRITE_POLICY_HANDLE_OFF          0
#define HCI_WRITE_POLICY_SETTINGS_OFF        2
/* Write Policy Settings */

/* Read Default Policy Settings */
extern uint8_t btsnd_hcic_read_def_policy_set(void);

#define HCIC_PARAM_SIZE_READ_DEF_POLICY_SET           0
/* Read Default Policy Settings */

/* Write Default Policy Settings */
extern uint8_t btsnd_hcic_write_def_policy_set(uint16_t settings);

#define HCIC_PARAM_SIZE_WRITE_DEF_POLICY_SET     2

#define HCI_WRITE_DEF_POLICY_SETTINGS_OFF        0
/* Write Default Policy Settings */

/* Flow Specification */
extern uint8_t btsnd_hcic_flow_specification(uint16_t handle, uint8_t flags,
        uint8_t flow_direct,
        uint8_t service_type,
        uint32_t token_rate,
        uint32_t token_bucket_size,
        uint32_t peak, uint32_t latency);

#define HCIC_PARAM_SIZE_FLOW_SPEC             21

#define HCI_FLOW_SPEC_HANDLE_OFF              0
#define HCI_FLOW_SPEC_FLAGS_OFF               2
#define HCI_FLOW_SPEC_FLOW_DIRECT_OFF         3
#define HCI_FLOW_SPEC_SERVICE_TYPE_OFF        4
#define HCI_FLOW_SPEC_TOKEN_RATE_OFF          5
#define HCI_FLOW_SPEC_TOKEN_BUCKET_SIZE_OFF   9
#define HCI_FLOW_SPEC_PEAK_BANDWIDTH_OFF      13
#define HCI_FLOW_SPEC_LATENCY_OFF             17
/* Flow Specification */

/******************************************
**    Lisbon Features
*******************************************/
#if BTM_SSR_INCLUDED == TRUE
/* Sniff Subrating */
extern uint8_t btsnd_hcic_sniff_sub_rate(uint16_t handle, uint16_t max_lat,
        uint16_t min_remote_lat,
        uint16_t min_local_lat);

#define HCIC_PARAM_SIZE_SNIFF_SUB_RATE             8

#define HCI_SNIFF_SUB_RATE_HANDLE_OFF              0
#define HCI_SNIFF_SUB_RATE_MAX_LAT_OFF             2
#define HCI_SNIFF_SUB_RATE_MIN_REM_LAT_OFF         4
#define HCI_SNIFF_SUB_RATE_MIN_LOC_LAT_OFF         6
/* Sniff Subrating */

#else   /* BTM_SSR_INCLUDED == FALSE */

#define btsnd_hcic_sniff_sub_rate(handle, max_lat, min_remote_lat, min_local_lat) FALSE

#endif  /* BTM_SSR_INCLUDED */

/* Extended Inquiry Response */
extern void btsnd_hcic_write_ext_inquiry_response(void *buffer, uint8_t fec_req);

#define HCIC_PARAM_SIZE_EXT_INQ_RESP        241

#define HCIC_EXT_INQ_RESP_FEC_OFF     0
#define HCIC_EXT_INQ_RESP_RESPONSE    1

extern uint8_t btsnd_hcic_read_ext_inquiry_response(void);   /* Read Extended Inquiry Response */
/* Write Simple Pairing Mode */
/**** Simple Pairing Commands ****/
extern uint8_t btsnd_hcic_write_simple_pairing_mode(uint8_t mode);

#define HCIC_PARAM_SIZE_W_SIMP_PAIR     1

#define HCIC_WRITE_SP_MODE_OFF          0


extern uint8_t btsnd_hcic_read_simple_pairing_mode(void);

#define HCIC_PARAM_SIZE_R_SIMP_PAIR     0

/* Write Simple Pairing Debug Mode */
extern uint8_t btsnd_hcic_write_simp_pair_debug_mode(uint8_t debug_mode);

#define HCIC_PARAM_SIZE_SIMP_PAIR_DBUG  1

#define HCIC_WRITE_SP_DBUG_MODE_OFF     0

/* IO Capabilities Response */
extern uint8_t btsnd_hcic_io_cap_req_reply(BD_ADDR bd_addr, uint8_t capability,
        uint8_t oob_present, uint8_t auth_req);

#define HCIC_PARAM_SIZE_IO_CAP_RESP     9

#define HCI_IO_CAP_BD_ADDR_OFF          0
#define HCI_IO_CAPABILITY_OFF           6
#define HCI_IO_CAP_OOB_DATA_OFF         7
#define HCI_IO_CAP_AUTH_REQ_OFF         8

/* IO Capabilities Req Neg Reply */
extern uint8_t btsnd_hcic_io_cap_req_neg_reply(BD_ADDR bd_addr, uint8_t err_code);

#define HCIC_PARAM_SIZE_IO_CAP_NEG_REPLY 7

#define HCI_IO_CAP_NR_BD_ADDR_OFF        0
#define HCI_IO_CAP_NR_ERR_CODE           6

/* Read Local OOB Data */
extern uint8_t btsnd_hcic_read_local_oob_data(void);

#define HCIC_PARAM_SIZE_R_LOCAL_OOB     0


extern uint8_t btsnd_hcic_user_conf_reply(BD_ADDR bd_addr, uint8_t is_yes);

#define HCIC_PARAM_SIZE_UCONF_REPLY     6

#define HCI_USER_CONF_BD_ADDR_OFF       0


extern uint8_t btsnd_hcic_user_passkey_reply(BD_ADDR bd_addr, uint32_t value);

#define HCIC_PARAM_SIZE_U_PKEY_REPLY    10

#define HCI_USER_PASSKEY_BD_ADDR_OFF    0
#define HCI_USER_PASSKEY_VALUE_OFF      6


extern uint8_t btsnd_hcic_user_passkey_neg_reply(BD_ADDR bd_addr);

#define HCIC_PARAM_SIZE_U_PKEY_NEG_REPLY 6

#define HCI_USER_PASSKEY_NEG_BD_ADDR_OFF 0

/* Remote OOB Data Request Reply */
extern uint8_t btsnd_hcic_rem_oob_reply(BD_ADDR bd_addr, uint8_t *p_c,
                                        uint8_t *p_r);

#define HCIC_PARAM_SIZE_REM_OOB_REPLY   38

#define HCI_REM_OOB_DATA_BD_ADDR_OFF    0
#define HCI_REM_OOB_DATA_C_OFF          6
#define HCI_REM_OOB_DATA_R_OFF          22

/* Remote OOB Data Request Negative Reply */
extern uint8_t btsnd_hcic_rem_oob_neg_reply(BD_ADDR bd_addr);

#define HCIC_PARAM_SIZE_REM_OOB_NEG_REPLY   6

#define HCI_REM_OOB_DATA_NEG_BD_ADDR_OFF    0

/* Read Tx Power Level */
extern uint8_t btsnd_hcic_read_inq_tx_power(void);

#define HCIC_PARAM_SIZE_R_TX_POWER      0

/* Write Tx Power Level */
extern uint8_t btsnd_hcic_write_inq_tx_power(int8_t level);

#define HCIC_PARAM_SIZE_W_TX_POWER      1

#define HCIC_WRITE_TX_POWER_LEVEL_OFF   0
/* Read Default Erroneous Data Reporting */
extern uint8_t btsnd_hcic_read_default_erroneous_data_rpt(void);

#define HCIC_PARAM_SIZE_R_ERR_DATA_RPT      0

/* Write Default Erroneous Data Reporting */
extern uint8_t btsnd_hcic_write_default_erroneous_data_rpt(uint8_t level);

#define HCIC_PARAM_SIZE_W_ERR_DATA_RPT      1

#define HCIC_WRITE_ERR_DATA_RPT_OFF   0


#if L2CAP_NON_FLUSHABLE_PB_INCLUDED == TRUE
extern uint8_t btsnd_hcic_enhanced_flush(uint16_t handle, uint8_t packet_type);

#define HCIC_PARAM_SIZE_ENHANCED_FLUSH  3
#endif


extern uint8_t btsnd_hcic_send_keypress_notif(BD_ADDR bd_addr, uint8_t notif);

#define HCIC_PARAM_SIZE_SEND_KEYPRESS_NOTIF    7

#define HCI_SEND_KEYPRESS_NOTIF_BD_ADDR_OFF    0
#define HCI_SEND_KEYPRESS_NOTIF_NOTIF_OFF      6


extern uint8_t btsnd_hcic_refresh_encryption_key(uint16_t
        handle);       /* Refresh Encryption Key */

/**** end of Simple Pairing Commands ****/


extern uint8_t btsnd_hcic_set_event_mask(uint8_t local_controller_id, BT_EVENT_MASK evt_mask);

#define HCIC_PARAM_SIZE_SET_EVENT_MASK  8
#define HCI_EVENT_MASK_MASK_OFF         0
/* Set Event Mask */

/* Reset */
extern uint8_t btsnd_hcic_set_event_mask_page_2(uint8_t local_controller_id,
        BT_EVENT_MASK event_mask);

#define HCIC_PARAM_SIZE_SET_EVENT_MASK_PAGE_2   8
#define HCI_EVENT_MASK_MASK_OFF                 0
/* Set Event Mask Page 2 */

/* Reset */
extern uint8_t btsnd_hcic_reset(uint8_t local_controller_id);

#define HCIC_PARAM_SIZE_RESET           0
/* Reset */

/* Store Current Settings */
#define MAX_FILT_COND   (sizeof (BD_ADDR) + 1)

extern uint8_t btsnd_hcic_set_event_filter(uint8_t filt_type,
        uint8_t filt_cond_type,
        uint8_t *filt_cond,
        uint8_t filt_cond_len);

#define HCIC_PARAM_SIZE_SET_EVT_FILTER  9

#define HCI_FILT_COND_FILT_TYPE_OFF     0
#define HCI_FILT_COND_COND_TYPE_OFF     1
#define HCI_FILT_COND_FILT_OFF          2
/* Set Event Filter */

extern uint8_t btsnd_hcic_flush(uint8_t local_controller_id,
                                uint16_t handle);                 /* Flush */

/* Create New Unit Type */
extern uint8_t btsnd_hcic_new_unit_key(void);

#define HCIC_PARAM_SIZE_NEW_UNIT_KEY     0
/* Create New Unit Type */

/* Read Stored Key */
extern uint8_t btsnd_hcic_read_stored_key(BD_ADDR bd_addr,
        uint8_t read_all_flag);

#define HCIC_PARAM_SIZE_READ_STORED_KEY 7

#define HCI_READ_KEY_BD_ADDR_OFF        0
#define HCI_READ_KEY_ALL_FLAG_OFF       6
/* Read Stored Key */

#define MAX_WRITE_KEYS 10
/* Write Stored Key */
extern uint8_t btsnd_hcic_write_stored_key(uint8_t num_keys, BD_ADDR *bd_addr,
        LINK_KEY *link_key);

#define HCIC_PARAM_SIZE_WRITE_STORED_KEY  sizeof(btmsg_hcic_write_stored_key_t)

#define HCI_WRITE_KEY_NUM_KEYS_OFF          0
#define HCI_WRITE_KEY_BD_ADDR_OFF           1
#define HCI_WRITE_KEY_KEY_OFF               7
/* only 0x0b keys cab be sent in one HCI command */
#define HCI_MAX_NUM_OF_LINK_KEYS_PER_CMMD   0x0b
/* Write Stored Key */

/* Delete Stored Key */
extern uint8_t btsnd_hcic_delete_stored_key(BD_ADDR bd_addr, uint8_t delete_all_flag);

#define HCIC_PARAM_SIZE_DELETE_STORED_KEY        7

#define HCI_DELETE_KEY_BD_ADDR_OFF      0
#define HCI_DELETE_KEY_ALL_FLAG_OFF     6
/* Delete Stored Key */

/* Change Local Name */
extern uint8_t btsnd_hcic_change_name(BD_NAME name);

#define HCIC_PARAM_SIZE_CHANGE_NAME     BD_NAME_LEN

#define HCI_CHANGE_NAME_NAME_OFF        0
/* Change Local Name */


#define HCIC_PARAM_SIZE_READ_CMD     0

#define HCIC_PARAM_SIZE_WRITE_PARAM1     1

#define HCIC_WRITE_PARAM1_PARAM_OFF      0

#define HCIC_PARAM_SIZE_WRITE_PARAM2     2

#define HCIC_WRITE_PARAM2_PARAM_OFF      0

#define HCIC_PARAM_SIZE_WRITE_PARAM3     3

#define HCIC_WRITE_PARAM3_PARAM_OFF      0

#define HCIC_PARAM_SIZE_SET_AFH_CHANNELS    10

extern uint8_t btsnd_hcic_read_pin_type(void);                          /* Read PIN Type */
extern uint8_t btsnd_hcic_write_pin_type(uint8_t type);                   /* Write PIN Type */
extern uint8_t btsnd_hcic_read_auto_accept(void);                       /* Read Auto Accept */
extern uint8_t btsnd_hcic_write_auto_accept(uint8_t flag);                /* Write Auto Accept */
extern uint8_t btsnd_hcic_read_name(void);                              /* Read Local Name */
extern uint8_t btsnd_hcic_read_conn_acc_tout(uint8_t
        local_controller_id);       /* Read Connection Accept Timout */
extern uint8_t btsnd_hcic_write_conn_acc_tout(uint8_t local_controller_id,
        uint16_t tout);   /* Write Connection Accept Timout */
extern uint8_t btsnd_hcic_read_page_tout(void);                         /* Read Page Timout */
extern uint8_t btsnd_hcic_write_page_tout(uint16_t timeout);              /* Write Page Timout */
extern uint8_t btsnd_hcic_read_scan_enable(void);                       /* Read Scan Enable */
extern uint8_t btsnd_hcic_write_scan_enable(uint8_t flag);                /* Write Scan Enable */
extern uint8_t btsnd_hcic_read_pagescan_cfg(
                void);                      /* Read Page Scan Activity */

extern uint8_t btsnd_hcic_write_pagescan_cfg(uint16_t interval,
        uint16_t window);            /* Write Page Scan Activity */

#define HCIC_PARAM_SIZE_WRITE_PAGESCAN_CFG  4

#define HCI_SCAN_CFG_INTERVAL_OFF       0
#define HCI_SCAN_CFG_WINDOW_OFF         2
/* Write Page Scan Activity */

extern uint8_t btsnd_hcic_read_inqscan_cfg(void);       /* Read Inquiry Scan Activity */

/* Write Inquiry Scan Activity */
extern uint8_t btsnd_hcic_write_inqscan_cfg(uint16_t interval, uint16_t window);

#define HCIC_PARAM_SIZE_WRITE_INQSCAN_CFG    4

#define HCI_SCAN_CFG_INTERVAL_OFF       0
#define HCI_SCAN_CFG_WINDOW_OFF         2
/* Write Inquiry Scan Activity */

extern uint8_t btsnd_hcic_read_auth_enable(
                void);                        /* Read Authentication Enable */
extern uint8_t btsnd_hcic_write_auth_enable(uint8_t
        flag);                 /* Write Authentication Enable */
extern uint8_t btsnd_hcic_read_encr_mode(void);                          /* Read encryption mode */
extern uint8_t btsnd_hcic_write_encr_mode(uint8_t
        mode);                   /* Write encryption mode */
extern uint8_t btsnd_hcic_read_dev_class(void);                          /* Read Class of Device */
extern uint8_t btsnd_hcic_write_dev_class(DEV_CLASS dev);                /* Write Class of Device */
extern uint8_t btsnd_hcic_read_voice_settings(void);                     /* Read Voice Settings */
extern uint8_t btsnd_hcic_write_voice_settings(uint16_t
        flags);            /* Write Voice Settings */

/* Host Controller to Host flow control */
#define HCI_HOST_FLOW_CTRL_OFF          0
#define HCI_HOST_FLOW_CTRL_ACL_ON       1
#define HCI_HOST_FLOW_CTRL_SCO_ON       2
#define HCI_HOST_FLOW_CTRL_BOTH_ON      3

extern uint8_t btsnd_hcic_set_host_flow_ctrl(uint8_t
        value);          /* Enable/disable flow control toward host */


extern uint8_t btsnd_hcic_read_auto_flush_tout(uint16_t handle);      /* Read Retransmit Timout */

extern uint8_t btsnd_hcic_write_auto_flush_tout(uint16_t handle,
        uint16_t timeout);    /* Write Retransmit Timout */

#define HCIC_PARAM_SIZE_WRITE_AUTO_FLUSH_TOUT    4

#define HCI_FLUSH_TOUT_HANDLE_OFF       0
#define HCI_FLUSH_TOUT_TOUT_OFF         2

extern uint8_t btsnd_hcic_read_num_bcast_xmit(
                void);                    /* Read Num Broadcast Retransmits */
extern uint8_t btsnd_hcic_write_num_bcast_xmit(uint8_t
        num);              /* Write Num Broadcast Retransmits */
extern uint8_t btsnd_hcic_read_hold_mode_act(
                void);                     /* Read Hold Mode Activity */
extern uint8_t btsnd_hcic_write_hold_mode_act(uint8_t
        flags);             /* Write Hold Mode Activity */

extern uint8_t btsnd_hcic_read_tx_power(uint16_t handle, uint8_t type);     /* Read Tx Power */

#define HCIC_PARAM_SIZE_READ_TX_POWER    3

#define HCI_READ_TX_POWER_HANDLE_OFF    0
#define HCI_READ_TX_POWER_TYPE_OFF      2

/* Read transmit power level parameter */
#define HCI_READ_CURRENT                0x00
#define HCI_READ_MAXIMUM                0x01

extern uint8_t btsnd_hcic_read_sco_flow_enable(
                void);                       /* Read Authentication Enable */
extern uint8_t btsnd_hcic_write_sco_flow_enable(uint8_t
        flag);                /* Write Authentication Enable */

/* Set Host Buffer Size */
extern uint8_t btsnd_hcic_set_host_buf_size(uint16_t acl_len,
        uint8_t sco_len,
        uint16_t acl_num,
        uint16_t sco_num);

#define HCIC_PARAM_SIZE_SET_HOST_BUF_SIZE    7

#define HCI_HOST_BUF_SIZE_ACL_LEN_OFF   0
#define HCI_HOST_BUF_SIZE_SCO_LEN_OFF   2
#define HCI_HOST_BUF_SIZE_ACL_NUM_OFF   3
#define HCI_HOST_BUF_SIZE_SCO_NUM_OFF   5


extern uint8_t btsnd_hcic_host_num_xmitted_pkts(uint8_t num_handles,
        uint16_t *handle,
        uint16_t *num_pkts);         /* Set Host Buffer Size */

#define HCIC_PARAM_SIZE_NUM_PKTS_DONE_SIZE    sizeof(btmsg_hcic_num_pkts_done_t)

#define MAX_DATA_HANDLES        10

#define HCI_PKTS_DONE_NUM_HANDLES_OFF   0
#define HCI_PKTS_DONE_HANDLE_OFF        1
#define HCI_PKTS_DONE_NUM_PKTS_OFF      3

extern uint8_t btsnd_hcic_read_link_super_tout(uint8_t local_controller_id,
        uint16_t handle);   /* Read Link Supervision Timeout */

/* Write Link Supervision Timeout */
extern uint8_t btsnd_hcic_write_link_super_tout(uint8_t local_controller_id, uint16_t handle,
        uint16_t timeout);

#define HCIC_PARAM_SIZE_WRITE_LINK_SUPER_TOUT        4

#define HCI_LINK_SUPER_TOUT_HANDLE_OFF  0
#define HCI_LINK_SUPER_TOUT_TOUT_OFF    2
/* Write Link Supervision Timeout */

extern uint8_t btsnd_hcic_read_max_iac(void);                       /* Read Num Supported IAC */
extern uint8_t btsnd_hcic_read_cur_iac_lap(void);                   /* Read Current IAC LAP */

extern uint8_t btsnd_hcic_write_cur_iac_lap(uint8_t num_cur_iac,
        LAP *const iac_lap);   /* Write Current IAC LAP */

#define MAX_IAC_LAPS    0x40

#define HCI_WRITE_IAC_LAP_NUM_OFF       0
#define HCI_WRITE_IAC_LAP_LAP_OFF       1
/* Write Current IAC LAP */

/* Read Clock */
extern uint8_t btsnd_hcic_read_clock(uint16_t handle, uint8_t which_clock);

#define HCIC_PARAM_SIZE_READ_CLOCK      3

#define HCI_READ_CLOCK_HANDLE_OFF       0
#define HCI_READ_CLOCK_WHICH_CLOCK      2
/* Read Clock */

#ifdef TESTER_ENABLE

#define HCIC_PARAM_SIZE_ENTER_TEST_MODE  2

#define HCI_ENTER_TEST_HANDLE_OFF        0

#define HCIC_PARAM_SIZE_TEST_CNTRL          10
#define HCI_TEST_CNTRL_HANDLE_OFF           0
#define HCI_TEST_CNTRL_SCENARIO_OFF         2
#define HCI_TEST_CNTRL_HOPPINGMODE_OFF      3
#define HCI_TEST_CNTRL_TX_FREQ_OFF          4
#define HCI_TEST_CNTRL_RX_FREQ_OFF          5
#define HCI_TEST_CNTRL_PWR_CNTRL_MODE_OFF   6
#define HCI_TEST_CNTRL_POLL_PERIOD_OFF      7
#define HCI_TEST_CNTRL_PKT_TYPE_OFF         8
#define HCI_TEST_CNTRL_LENGTH_OFF           9

#endif

extern uint8_t btsnd_hcic_read_page_scan_per(
                void);                    /* Read Page Scan Period Mode */
extern uint8_t btsnd_hcic_write_page_scan_per(uint8_t
        mode);             /* Write Page Scan Period Mode */
extern uint8_t btsnd_hcic_read_page_scan_mode(void);                   /* Read Page Scan Mode */
extern uint8_t btsnd_hcic_write_page_scan_mode(uint8_t mode);            /* Write Page Scan Mode */
extern uint8_t btsnd_hcic_read_local_ver(uint8_t
        local_controller_id);          /* Read Local Version Info */
extern uint8_t btsnd_hcic_read_local_supported_cmds(uint8_t
        local_controller_id);   /* Read Local Supported Commands */
extern uint8_t btsnd_hcic_read_local_features(
                void);                   /* Read Local Supported Features */
extern uint8_t btsnd_hcic_read_buffer_size(void);                      /* Read Local buffer sizes */
extern uint8_t btsnd_hcic_read_country_code(void);                     /* Read Country Code */
extern uint8_t btsnd_hcic_read_bd_addr(void);                          /* Read Local BD_ADDR */
extern uint8_t btsnd_hcic_read_fail_contact_count(uint8_t local_controller_id,
        uint16_t handle);   /* Read Failed Contact Counter */
extern uint8_t btsnd_hcic_reset_fail_contact_count(uint8_t local_controller_id,
        uint16_t handle);   /* Reset Failed Contact Counter */
extern uint8_t btsnd_hcic_get_link_quality(uint16_t handle);             /* Get Link Quality */
extern uint8_t btsnd_hcic_read_rssi(uint16_t handle);                    /* Read RSSI */
extern uint8_t btsnd_hcic_read_loopback_mode(void);                    /* Read Loopback Mode */
extern uint8_t btsnd_hcic_write_loopback_mode(uint8_t mode);             /* Write Loopback Mode */
extern uint8_t btsnd_hcic_enable_test_mode(
                void);                      /* Enable Device Under Test Mode */
extern uint8_t btsnd_hcic_write_pagescan_type(uint8_t type);             /* Write Page Scan Type */
extern uint8_t btsnd_hcic_read_pagescan_type(void);                    /* Read Page Scan Type */
extern uint8_t btsnd_hcic_write_inqscan_type(uint8_t
        type);              /* Write Inquiry Scan Type */
extern uint8_t btsnd_hcic_read_inqscan_type(void);                     /* Read Inquiry Scan Type */
extern uint8_t btsnd_hcic_write_inquiry_mode(uint8_t type);              /* Write Inquiry Mode */
extern uint8_t btsnd_hcic_read_inquiry_mode(void);                     /* Read Inquiry Mode */
extern uint8_t btsnd_hcic_set_afh_channels(uint8_t first, uint8_t last);
extern uint8_t btsnd_hcic_write_afh_channel_assessment_mode(uint8_t mode);
extern uint8_t btsnd_hcic_set_afh_host_channel_class(uint8_t *p_afhchannelmap);
extern uint8_t btsnd_hcic_read_afh_channel_assessment_mode(void);
extern uint8_t btsnd_hcic_read_afh_channel_map(uint16_t handle);
extern uint8_t btsnd_hcic_nop(void);                               /* NOP */

/* Send HCI Data */
extern void btsnd_hcic_data(BT_HDR *p_buf, uint16_t len, uint16_t handle, uint8_t boundary,
                            uint8_t broadcast);

#define HCI_DATA_HANDLE_MASK 0x0FFF

#define HCID_GET_HANDLE_EVENT(p)  (uint16_t)((*((uint8_t *)((p) + 1) + p->offset) + \
        (*((uint8_t *)((p) + 1) + p->offset + 1) << 8)))

#define HCID_GET_HANDLE(u16) (uint16_t)((u16) & HCI_DATA_HANDLE_MASK)

#define HCI_DATA_EVENT_MASK   3
#define HCI_DATA_EVENT_OFFSET 12
#define HCID_GET_EVENT(u16)   (uint8_t)(((u16) >> HCI_DATA_EVENT_OFFSET) & HCI_DATA_EVENT_MASK)

#define HCI_DATA_BCAST_MASK   3
#define HCI_DATA_BCAST_OFFSET 10
#define HCID_GET_BCAST(u16)   (uint8_t)(((u16) >> HCI_DATA_BCAST_OFFSET) & HCI_DATA_BCAST_MASK)

#define HCID_GET_ACL_LEN(p)     (uint16_t)((*((uint8_t *)((p) + 1) + p->offset + 2) + \
        (*((uint8_t *)((p) + 1) + p->offset + 3) << 8)))

#define HCID_HEADER_SIZE      4

#define HCID_GET_SCO_LEN(p)  (*((uint8_t *)((p) + 1) + p->offset + 2))

extern void btsnd_hcic_vendor_spec_cmd(void *buffer, uint16_t opcode,
                                       uint8_t len, uint8_t *p_data,
                                       void *p_cmd_cplt_cback);


/*********************************************************************************
**                                                                              **
**                          H C I    E V E N T S                                **
**                                                                              **
*********************************************************************************/

/* Inquiry Complete Event */
extern void btsnd_hcie_inq_comp(void *buffer, uint8_t status);

#define HCIE_PARAM_SIZE_INQ_COMP  1

/* Inquiry Response Event */
extern void btsnd_hcie_inq_res(void *buffer, uint8_t num_resp, uint8_t **bd_addr,
                               uint8_t *page_scan_rep_mode, uint8_t *page_scan_per_mode,
                               uint8_t *page_scan_mode, uint8_t **dev_class,
                               uint16_t *clock_offset);

/* Connection Complete Event */
extern void btsnd_hcie_connection_comp(void *buffer, uint8_t status, uint16_t handle,
                                       BD_ADDR bd_addr, uint8_t link_type, uint8_t encr_mode);

#define HCIE_PARAM_SIZE_CONNECTION_COMP    11


#define HCI_LINK_TYPE_SCO               0x00
#define HCI_LINK_TYPE_ACL               0x01

#define HCI_ENCRYPT_MODE_DISABLED       0x00
#define HCI_ENCRYPT_MODE_POINT_TO_POINT 0x01
#define HCI_ENCRYPT_MODE_ALL            0x02


/* Connection Request Event */
extern void btsnd_hcie_connection_req(void *buffer, BD_ADDR bd_addr, DEV_CLASS dev_class,
                                      uint8_t link_type);

#define HCIE_PARAM_SIZE_CONNECTION_REQ  10

#define HCI_LINK_TYPE_SCO               0x00
#define HCI_LINK_TYPE_ACL               0x01


/* Disonnection Complete Event */
extern void btsnd_hcie_disc_comp(void *buffer, uint8_t status, uint16_t handle, uint8_t reason);

#define HCIE_PARAM_SIZE_DISC_COMP  4


/* Authentication Complete Event */
extern void btsnd_hcie_auth_comp(void *buffer, uint8_t status, uint16_t handle);

#define HCIE_PARAM_SIZE_AUTH_COMP  3


/* Remote Name Request Complete Event */
extern void btsnd_hcie_rmt_name_req_comp(void *buffer, uint8_t status, BD_ADDR bd_addr,
        BD_NAME name);

#define HCIE_PARAM_SIZE_RMT_NAME_REQ_COMP  (1 + BD_ADDR_LEN + BD_NAME_LEN)


/* Encryption Change Event */
extern void btsnd_hcie_encryption_change(void *buffer, uint8_t status, uint16_t handle,
        uint8_t enable);

#define HCIE_PARAM_SIZE_ENCR_CHANGE  4


/* Connection Link Key Change Event */
extern void btsnd_hcie_conn_link_key_change(void *buffer, uint8_t status, uint16_t handle);

#define HCIE_PARAM_SIZE_LINK_KEY_CHANGE  3


/* Encryption Key Refresh Complete Event */
extern void btsnd_hcie_encrypt_key_refresh(void *buffer, uint8_t status, uint16_t handle);

#define HCIE_PARAM_SIZE_ENCRYPT_KEY_REFRESH  3


/* Master Link Key Complete Event */
extern void btsnd_hcie_master_link_key(void *buffer, uint8_t status, uint16_t handle, uint8_t flag);

#define HCIE_PARAM_SIZE_MASTER_LINK_KEY  4


/* Read Remote Supported Features Complete Event */
extern void btsnd_hcie_read_rmt_features(void *buffer, uint8_t status, uint16_t handle,
        uint8_t *features);

#define LMP_FEATURES_SIZE   8
#define HCIE_PARAM_SIZE_READ_RMT_FEATURES  11


/* Read Remote Extended Features Complete Event */
extern void btsnd_hcie_read_rmt_ext_features(void *buffer, uint8_t status, uint16_t handle,
        uint8_t page_num,
        uint8_t max_page_num, uint8_t *features);

#define EXT_LMP_FEATURES_SIZE   8
#define HCIE_PARAM_SIZE_READ_RMT_EXT_FEATURES  13


/* Read Remote Version Complete Event */
extern void btsnd_hcie_read_rmt_version(void *buffer, uint8_t status, uint16_t handle,
                                        uint8_t version,
                                        uint16_t comp_name, uint16_t sub_version);

#define HCIE_PARAM_SIZE_READ_RMT_VERSION  8


/* QOS setup complete */
extern void btsnd_hcie_qos_setup_compl(void *buffer, uint8_t status, uint16_t handle, uint8_t flags,
                                       uint8_t service_type, uint32_t token_rate, uint32_t peak,
                                       uint32_t latency, uint32_t delay_var);

#define HCIE_PARAM_SIZE_QOS_SETUP_COMP 21


/* Flow Specification complete */
extern void btsnd_hcie_flow_spec_compl(void *buffer, uint8_t status, uint16_t handle, uint8_t flags,
                                       uint8_t flow_direction, uint8_t service_type, uint32_t token_rate, uint32_t token_bucket_size,
                                       uint32_t peak, uint32_t latency);

#define HCIE_PARAM_SIZE_FLOW_SPEC_COMP 22


/*  Command Complete Event */
extern void btsnd_hcie_cmd_comp(void *buffer, uint8_t max_host_cmds, uint16_t opcode,
                                uint8_t status);

#define HCIE_PARAM_SIZE_CMD_COMP  4


/*  Command Complete with pre-filled in parameters */
extern void btsnd_hcie_cmd_comp_params(void *buffer, uint8_t max_host_cmds, uint16_t cmd_opcode,
                                       uint8_t status);

#define HCI_CMD_COMPL_PARAM_OFFSET 4


/*  Command Complete Event with 1-byte param */
extern void btsnd_hcie_cmd_comp_param1(void *buffer, uint8_t max_host_cmds, uint16_t opcode,
                                       uint8_t status, uint8_t param1);

#define HCIE_PARAM_SIZE_CMD_COMP_PARAM1  5

/*  Command Complete Event with 2-byte param */
extern void btsnd_hcie_cmd_comp_param2(void *buffer, uint8_t max_host_cmds, uint16_t opcode,
                                       uint8_t status, uint16_t param2);

#define HCIE_PARAM_SIZE_CMD_COMP_PARAM2  6


/*  Command Complete Event with BD-addr as param */
extern void btsnd_hcie_cmd_comp_bd_addr(void *buffer, uint8_t max_host_cmds, uint16_t opcode,
                                        uint8_t status, BD_ADDR bd_addr);

#define HCIE_PARAM_SIZE_CMD_COMP_BD_ADDR  10


/*  Command Pending Event */
extern void btsnd_hcie_cmd_status(void *buffer, uint8_t status, uint8_t max_host_cmds,
                                  uint16_t opcode);

#define HCIE_PARAM_SIZE_CMD_STATUS  4


/*  HW failure Event */
extern void btsnd_hcie_hw_failure(void *buffer, uint8_t code);

#define HCIE_PARAM_SIZE_HW_FAILURE 1


/*  Flush Occured Event */
extern void btsnd_hcie_flush_occured(void *buffer, uint16_t handle);

#define HCIE_PARAM_SIZE_FLUSH_OCCURED  2


/*  Role Changed Event */
extern void btsnd_hcie_role_change(void *buffer, uint8_t status, BD_ADDR bd_addr, uint8_t role);

#define HCIE_PARAM_SIZE_ROLE_CHANGE  8


/* Ready for Data Packets Event */
extern void btsnd_hcie_num_compl_pkts(void *buffer, uint8_t num_handles, uint16_t *p_handle,
                                      uint16_t *num_pkts);

#define MAX_DATA_HANDLES        10


/* Mode Change Event */
extern void btsnd_hcie_mode_change(void *buffer, uint8_t status, uint16_t handle,
                                   uint8_t mode, uint16_t interval);

#define HCIE_PARAM_SIZE_MODE_CHANGE  6
#define MAX_DATA_HANDLES        10



/* Return Link Keys Event */
extern void btsnd_hcie_return_link_keys(void *buffer, uint8_t num_keys, BD_ADDR *bd_addr,
                                        LINK_KEY *link_key);

/* This should not be more than 0x0b */
#define MAX_LINK_KEYS 10



/* PIN Code Request Event */
extern void btsnd_hcie_pin_code_req(void *buffer, BD_ADDR bd_addr);

#define HCIE_PARAM_SIZE_PIN_CODE_REQ  6



/* Link Key Request Event */
extern void btsnd_hcie_link_key_req(void *buffer, BD_ADDR bd_addr);

#define HCIE_PARAM_SIZE_LINK_KEY_REQ  6



/* Link Key Notification Event */
extern void btsnd_hcie_link_key_notify(void *buffer, BD_ADDR bd_addr, LINK_KEY link_key,
                                       uint8_t key_type);

#define HCIE_PARAM_SIZE_LINK_KEY_NOTIFY  23



/* Loopback Command Event */
extern void btsnd_hcie_loopback_command(void *buffer, uint8_t data_len, uint8_t *data);

#define HCIE_PARAM_SIZE_LOOPBACK_COMMAND  sizeof(btmsg_hcie_loopback_cmd_t)



/* Data Buffer Overflow Event */
extern void btsnd_hcie_data_buf_overflow(void *buffer, uint8_t link_type);

#define HCIE_PARAM_SIZE_DATA_BUF_OVERFLOW  1



/* Max Slots Change Event */
extern void btsnd_hcie_max_slots_change(void *buffer, uint16_t handle, uint8_t max_slots);

#define HCIE_PARAM_SIZE_MAX_SLOTS_CHANGE  3


/* Read Clock Offset Complet Event */
extern void btsnd_hcie_read_clock_off_comp(void *buffer, uint8_t status, uint16_t handle,
        uint16_t clock_offset);

#define HCIE_PARAM_SIZE_READ_CLOCK_OFF_COMP  5



/* Connection Packet Type Change Event */
extern void btsnd_hcie_pkt_type_change(void *buffer, uint8_t status, uint16_t handle,
                                       uint16_t pkt_type);

#define HCIE_PARAM_SIZE_PKT_TYPE_CHANGE  5



/* QOS violation Event */
extern void btsnd_hcie_qos_violation(void *buffer, uint16_t handle);

#define HCIE_PARAM_SIZE_QOS_VIOLATION  2



/* Page Scan Mode Change Event */
extern void btsnd_hcie_pagescan_mode_chng(void *buffer, BD_ADDR bd_addr, uint8_t mode);

#define HCIE_PARAM_SIZE_PAGE_SCAN_MODE_CHNG  7


/* Page Scan Repetition Mode Change Event */
extern void btsnd_hcie_pagescan_rep_mode_chng(void *buffer, BD_ADDR bd_addr, uint8_t mode);

#define HCIE_PARAM_SIZE_PAGE_SCAN_REP_MODE_CHNG  7


/* Sniff Sub Rate Event */
extern void btsnd_hcie_sniff_sub_rate(void *buffer, uint8_t status, uint16_t handle,
                                      uint16_t max_tx_lat, uint16_t max_rx_lat,
                                      uint16_t min_remote_timeout, uint16_t min_local_timeout);

#define HCIE_PARAM_SIZE_SNIFF_SUB_RATE  11



/* Extended Inquiry Result Event */
extern void btsnd_hcie_ext_inquiry_result(void *buffer, uint8_t num_resp, uint8_t **bd_addr,
        uint8_t *page_scan_rep_mode, uint8_t *reserved,
        uint8_t **dev_class, uint16_t *clock_offset, uint8_t *rssi, uint8_t *p_data);


#if (BLE_INCLUDED == TRUE)
/********************************************************************************
** BLE Commands
**      Note: "local_controller_id" is for transport, not counted in HCI message size
*********************************************************************************/
#define HCIC_BLE_RAND_DI_SIZE                   8
#define HCIC_BLE_ENCRYT_KEY_SIZE                16
#define HCIC_BLE_IRK_SIZE                       16

#define HCIC_PARAM_SIZE_SET_USED_FEAT_CMD       8
#define HCIC_PARAM_SIZE_WRITE_RANDOM_ADDR_CMD    6
#define HCIC_PARAM_SIZE_BLE_WRITE_ADV_PARAMS    15
#define HCIC_PARAM_SIZE_BLE_WRITE_SCAN_RSP      31
#define HCIC_PARAM_SIZE_WRITE_ADV_ENABLE        1
#define HCIC_PARAM_SIZE_BLE_WRITE_SCAN_PARAM    7
#define HCIC_PARAM_SIZE_BLE_WRITE_SCAN_ENABLE   2
#define HCIC_PARAM_SIZE_BLE_CREATE_LL_CONN      25
#define HCIC_PARAM_SIZE_BLE_CREATE_CONN_CANCEL  0
#define HCIC_PARAM_SIZE_CLEAR_WHITE_LIST        0
#define HCIC_PARAM_SIZE_ADD_WHITE_LIST          7
#define HCIC_PARAM_SIZE_REMOVE_WHITE_LIST       7
#define HCIC_PARAM_SIZE_BLE_UPD_LL_CONN_PARAMS  14
#define HCIC_PARAM_SIZE_SET_HOST_CHNL_CLASS     5
#define HCIC_PARAM_SIZE_READ_CHNL_MAP         2
#define HCIC_PARAM_SIZE_BLE_READ_REMOTE_FEAT    2
#define HCIC_PARAM_SIZE_BLE_ENCRYPT             32
#define HCIC_PARAM_SIZE_BLE_RAND                0
#define HCIC_PARAM_SIZE_WRITE_LE_HOST_SUPPORTED 2

#define HCIC_BLE_RAND_DI_SIZE                   8
#define HCIC_BLE_ENCRYT_KEY_SIZE                16
#define HCIC_PARAM_SIZE_BLE_START_ENC           (4 + HCIC_BLE_RAND_DI_SIZE + HCIC_BLE_ENCRYT_KEY_SIZE)
#define HCIC_PARAM_SIZE_LTK_REQ_REPLY           (2 + HCIC_BLE_ENCRYT_KEY_SIZE)
#define HCIC_PARAM_SIZE_LTK_REQ_NEG_REPLY       2
#define HCIC_BLE_CHNL_MAP_SIZE                  5
#define HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA      31

#define HCIC_PARAM_SIZE_BLE_ADD_DEV_RESOLVING_LIST      (7 + HCIC_BLE_IRK_SIZE * 2)
#define HCIC_PARAM_SIZE_BLE_RM_DEV_RESOLVING_LIST       7
#define HCIC_PARAM_SIZE_BLE_CLEAR_RESOLVING_LIST        0
#define HCIC_PARAM_SIZE_BLE_READ_RESOLVING_LIST_SIZE    0
#define HCIC_PARAM_SIZE_BLE_READ_RESOLVABLE_ADDR_PEER   7
#define HCIC_PARAM_SIZE_BLE_READ_RESOLVABLE_ADDR_LOCAL  7
#define HCIC_PARAM_SIZE_BLE_SET_ADDR_RESOLUTION_ENABLE  1
#define HCIC_PARAM_SIZE_BLE_SET_RAND_PRIV_ADDR_TIMOUT   2
#define HCIC_PARAM_SIZE_BLE_SET_DATA_LENGTH             6
#define HCIC_PARAM_SIZE_BLE_WRITE_EXTENDED_SCAN_PARAM  11

/* ULP HCI command */
extern uint8_t btsnd_hcic_ble_set_evt_mask(BT_EVENT_MASK event_mask);

extern uint8_t btsnd_hcic_ble_read_buffer_size(void);

extern uint8_t btsnd_hcic_ble_read_local_spt_feat(void);

extern uint8_t btsnd_hcic_ble_set_local_used_feat(uint8_t feat_set[8]);

extern uint8_t btsnd_hcic_ble_set_random_addr(BD_ADDR random_addr);

extern uint8_t btsnd_hcic_ble_write_adv_params(uint16_t adv_int_min, uint16_t adv_int_max,
        uint8_t adv_type, uint8_t addr_type_own,
        uint8_t addr_type_dir, BD_ADDR direct_bda,
        uint8_t channel_map, uint8_t adv_filter_policy);

extern uint8_t btsnd_hcic_ble_read_adv_chnl_tx_power(void);

extern uint8_t btsnd_hcic_ble_set_adv_data(uint8_t data_len, uint8_t *p_data);

extern uint8_t btsnd_hcic_ble_set_scan_rsp_data(uint8_t data_len, uint8_t *p_scan_rsp);

extern uint8_t btsnd_hcic_ble_set_adv_enable(uint8_t adv_enable);

extern uint8_t btsnd_hcic_ble_set_scan_params(uint8_t scan_type,
        uint16_t scan_int, uint16_t scan_win,
        uint8_t addr_type, uint8_t scan_filter_policy);

extern uint8_t btsnd_hcic_ble_set_scan_enable(uint8_t scan_enable, uint8_t duplicate);

extern uint8_t btsnd_hcic_ble_create_ll_conn(uint16_t scan_int, uint16_t scan_win,
        uint8_t init_filter_policy, uint8_t addr_type_peer, BD_ADDR bda_peer, uint8_t addr_type_own,
        uint16_t conn_int_min, uint16_t conn_int_max, uint16_t conn_latency, uint16_t conn_timeout,
        uint16_t min_ce_len, uint16_t max_ce_len);

extern uint8_t btsnd_hcic_ble_create_conn_cancel(void);

extern uint8_t btsnd_hcic_ble_read_white_list_size(void);

extern uint8_t btsnd_hcic_ble_clear_white_list(void);

extern uint8_t btsnd_hcic_ble_add_white_list(uint8_t addr_type, BD_ADDR bda);

extern uint8_t btsnd_hcic_ble_remove_from_white_list(uint8_t addr_type, BD_ADDR bda);

extern uint8_t btsnd_hcic_ble_upd_ll_conn_params(uint16_t handle, uint16_t conn_int_min,
        uint16_t conn_int_max,
        uint16_t conn_latency, uint16_t conn_timeout, uint16_t min_len, uint16_t max_len);

extern uint8_t btsnd_hcic_ble_set_host_chnl_class(uint8_t chnl_map[HCIC_BLE_CHNL_MAP_SIZE]);

extern uint8_t btsnd_hcic_ble_read_chnl_map(uint16_t handle);

extern uint8_t btsnd_hcic_ble_read_remote_feat(uint16_t handle);

extern uint8_t btsnd_hcic_ble_encrypt(uint8_t *key, uint8_t key_len, uint8_t *plain_text,
                                      uint8_t pt_len, void *p_cmd_cplt_cback);

extern uint8_t btsnd_hcic_ble_rand(void *p_cmd_cplt_cback);

extern uint8_t btsnd_hcic_ble_start_enc(uint16_t handle,
                                        uint8_t rand[HCIC_BLE_RAND_DI_SIZE],
                                        uint16_t ediv, uint8_t ltk[HCIC_BLE_ENCRYT_KEY_SIZE]);

extern uint8_t btsnd_hcic_ble_ltk_req_reply(uint16_t handle, uint8_t ltk[HCIC_BLE_ENCRYT_KEY_SIZE]);

extern uint8_t btsnd_hcic_ble_ltk_req_neg_reply(uint16_t handle);

extern uint8_t btsnd_hcic_ble_read_supported_states(void);

extern uint8_t btsnd_hcic_ble_read_suggested_default_data_length(void);

#if (defined BLE_PRIVACY_SPT && BLE_PRIVACY_SPT == TRUE)
extern uint8_t btsnd_hcic_ble_read_resolving_list_size(void);
#endif

extern uint8_t btsnd_hcic_ble_write_host_supported(uint8_t le_host_spt, uint8_t simul_le_host_spt);

extern uint8_t btsnd_hcic_ble_read_host_supported(void);

extern uint8_t btsnd_hcic_ble_receiver_test(uint8_t rx_freq);

extern uint8_t btsnd_hcic_ble_transmitter_test(uint8_t tx_freq, uint8_t test_data_len,
        uint8_t payload);
extern uint8_t btsnd_hcic_ble_test_end(void);

#if (defined BLE_LLT_INCLUDED) && (BLE_LLT_INCLUDED == TRUE)

#define HCIC_PARAM_SIZE_BLE_RC_PARAM_REQ_REPLY           14
extern uint8_t btsnd_hcic_ble_rc_param_req_reply(uint16_t handle,
        uint16_t conn_int_min, uint16_t conn_int_max,
        uint16_t conn_latency, uint16_t conn_timeout,
        uint16_t min_ce_len, uint16_t max_ce_len);

#define HCIC_PARAM_SIZE_BLE_RC_PARAM_REQ_NEG_REPLY       3
extern uint8_t btsnd_hcic_ble_rc_param_req_neg_reply(uint16_t handle, uint8_t reason);

#endif /* BLE_LLT_INCLUDED */

extern uint8_t btsnd_hcic_ble_set_data_length(uint16_t conn_handle, uint16_t tx_octets,
        uint16_t tx_time);

extern uint8_t btsnd_hcic_ble_add_device_resolving_list(uint8_t addr_type_peer,
        BD_ADDR bda_peer,
        uint8_t irk_peer[HCIC_BLE_IRK_SIZE],
        uint8_t irk_local[HCIC_BLE_IRK_SIZE]);

extern uint8_t btsnd_hcic_ble_rm_device_resolving_list(uint8_t addr_type_peer,
        BD_ADDR bda_peer);

extern uint8_t btsnd_hcic_ble_clear_resolving_list(void);

extern uint8_t btsnd_hcic_ble_read_resolvable_addr_peer(uint8_t addr_type_peer,
        BD_ADDR bda_peer);

extern uint8_t btsnd_hcic_ble_read_resolvable_addr_local(uint8_t addr_type_peer,
        BD_ADDR bda_peer);

extern uint8_t btsnd_hcic_ble_set_addr_resolution_enable(uint8_t addr_resolution_enable);

extern uint8_t btsnd_hcic_ble_set_rand_priv_addr_timeout(uint16_t rpa_timout);

#endif /* BLE_INCLUDED */

extern uint8_t btsnd_hcic_read_authenticated_payload_tout(uint16_t handle);

extern uint8_t btsnd_hcic_write_authenticated_payload_tout(uint16_t handle,
        uint16_t timeout);

#define HCIC_PARAM_SIZE_WRITE_AUTHENT_PAYLOAD_TOUT  4

#define HCI__WRITE_AUTHENT_PAYLOAD_TOUT_HANDLE_OFF  0
#define HCI__WRITE_AUTHENT_PAYLOAD_TOUT_TOUT_OFF    2

#endif
