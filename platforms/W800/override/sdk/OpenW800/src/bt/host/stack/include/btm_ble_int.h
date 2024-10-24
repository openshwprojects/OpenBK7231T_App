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
 *  this file contains the main Bluetooth Manager (BTM) internal
 *  definitions.
 *
 ******************************************************************************/

#ifndef BTM_BLE_INT_H
#define BTM_BLE_INT_H

#include "bt_target.h"
#include "bt_common.h"
#include "gki.h"
#include "hcidefs.h"
#include "btm_ble_api.h"
#include "btm_int.h"

#if BLE_INCLUDED == TRUE && SMP_INCLUDED == TRUE
#include "smp_api.h"
#endif


/* scanning enable status */
#define BTM_BLE_SCAN_ENABLE      0x01
#define BTM_BLE_SCAN_DISABLE     0x00

/* advertising enable status */
#define BTM_BLE_ADV_ENABLE     0x01
#define BTM_BLE_ADV_DISABLE    0x00

/* use the high 4 bits unused by inquiry mode */
#define BTM_BLE_SELECT_SCAN     0x20
#define BTM_BLE_NAME_REQUEST    0x40
#define BTM_BLE_OBSERVE         0x80

#define BTM_BLE_MAX_WL_ENTRY        1
#define BTM_BLE_AD_DATA_LEN         31

#define BTM_BLE_ENC_MASK    0x03

#define BTM_BLE_DUPLICATE_ENABLE        1
#define BTM_BLE_DUPLICATE_DISABLE       0

#ifndef BTM_BLE_GAP_DISC_SCAN_INT
#define BTM_BLE_GAP_DISC_SCAN_INT      18         /* Interval(scan_int) = 11.25 ms= 0x0010 * 0.625 ms */
#endif

#ifndef BTM_BLE_GAP_DISC_SCAN_WIN
#define BTM_BLE_GAP_DISC_SCAN_WIN      18         /* scan_window = 11.25 ms= 0x0010 * 0.625 ms */
#endif

#define BTM_BLE_GAP_ADV_INT            512        /* Tgap(gen_disc) = 1.28 s= 512 * 0.625 ms */
#define BTM_BLE_GAP_LIM_TIMEOUT_MS     (180 * 1000) /* Tgap(lim_timeout) = 180s max */
#define BTM_BLE_LOW_LATENCY_SCAN_INT   0x40       /* Interval(scan_int) = 5s= 8000 * 0.625 ms ,winnermicro controller doesnot support continous scan if classic bt is inquiring...*/
#define BTM_BLE_LOW_LATENCY_SCAN_WIN   0x60       /* scan_window = 5s= 8000 * 0.625 ms */


#define BTM_BLE_GAP_ADV_FAST_INT_1         48         /* TGAP(adv_fast_interval1) = 30(used) ~ 60 ms  = 48 *0.625 */
#define BTM_BLE_GAP_ADV_FAST_INT_2         160         /* TGAP(adv_fast_interval2) = 100(used) ~ 150 ms = 160 * 0.625 ms */
#define BTM_BLE_GAP_ADV_SLOW_INT           512         /* Tgap(adv_slow_interval) = 1.28 s= 512 * 0.625 ms */
#define BTM_BLE_GAP_ADV_DIR_MAX_INT        800         /* Tgap(dir_conn_adv_int_max) = 500 ms = 800 * 0.625 ms */
#define BTM_BLE_GAP_ADV_DIR_MIN_INT        400         /* Tgap(dir_conn_adv_int_min) = 250 ms = 400 * 0.625 ms */

#define BTM_BLE_GAP_FAST_ADV_TIMEOUT_MS    (90 * 1000)

#define BTM_BLE_SEC_REQ_ACT_NONE           0
#define BTM_BLE_SEC_REQ_ACT_ENCRYPT        1 /* encrypt the link using current key or key refresh */
#define BTM_BLE_SEC_REQ_ACT_PAIR           2
#define BTM_BLE_SEC_REQ_ACT_DISCARD        3 /* discard the sec request while encryption is started but not completed */
typedef uint8_t   tBTM_BLE_SEC_REQ_ACT;

#define BLE_STATIC_PRIVATE_MSB_MASK          0x3f
#define BLE_RESOLVE_ADDR_MSB                 0x40   /*  most significant bit, bit7, bit6 is 01 to be resolvable random */
#define BLE_RESOLVE_ADDR_MASK                0xc0   /* bit 6, and bit7 */
#define BTM_BLE_IS_RESOLVE_BDA(x)           ((x[0] & BLE_RESOLVE_ADDR_MASK) == BLE_RESOLVE_ADDR_MSB)

#define BLE_PUBLIC_ADDR_MSB_MASK            0xC0
#define BLE_PUBLIC_ADDR_MSB                 0x80   /*  most significant bit, bit7, bit6 is 10 to be public address*/
#define BTM_IS_PUBLIC_BDA(x)               ((x[0]  & BLE_PUBLIC_ADDR_MSB) == BLE_PUBLIC_ADDR_MSB_MASK)

/* LE scan activity bit mask, continue with LE inquiry bits */
#define BTM_LE_SELECT_CONN_ACTIVE      0x40     /* selection connection is in progress */
#define BTM_LE_OBSERVE_ACTIVE          0x80     /* observe is in progress */

/* BLE scan activity mask checking */
#define BTM_BLE_IS_SCAN_ACTIVE(x)   ((x) & BTM_BLE_SCAN_ACTIVE_MASK)
#define BTM_BLE_IS_INQ_ACTIVE(x)   ((x) & BTM_BLE_INQUIRY_MASK)
#define BTM_BLE_IS_OBS_ACTIVE(x)   ((x) & BTM_LE_OBSERVE_ACTIVE)
#define BTM_BLE_IS_SEL_CONN_ACTIVE(x)   ((x) & BTM_LE_SELECT_CONN_ACTIVE)

/* BLE ADDR type ID bit */
#define BLE_ADDR_TYPE_ID_BIT 0x02

#define BTM_VSC_CHIP_CAPABILITY_L_VERSION 55
#define BTM_VSC_CHIP_CAPABILITY_M_VERSION 95

typedef struct {
    uint16_t              data_mask;
    uint8_t               pure_data;
    uint8_t               *p_flags;
    uint8_t               ad_data[BTM_BLE_AD_DATA_LEN];
    uint8_t               *p_pad;
} tBTM_BLE_LOCAL_ADV_DATA;

typedef struct {
    uint32_t          inq_count;          /* Used for determining if a response has already been      */
    /* received for the current inquiry operation. (We do not   */
    /* want to flood the caller with multiple responses from    */
    /* the same device.                                         */
    uint8_t         scan_rsp;
    tBLE_BD_ADDR    le_bda;
} tINQ_LE_BDADDR;

#define BTM_BLE_ADV_DATA_LEN_MAX        31
#define BTM_BLE_CACHE_ADV_DATA_MAX      62

#define BTM_BLE_ISVALID_PARAM(x, min, max)  (((x) >= (min) && (x) <= (max)) || ((x) == BTM_BLE_CONN_PARAM_UNDEF))

/* 15 minutes minimum for random address refreshing */
#define BTM_BLE_PRIVATE_ADDR_INT_MS     (15 * 60 * 1000)

typedef struct {
    uint16_t discoverable_mode;
    uint16_t connectable_mode;
    uint32_t scan_window;
    uint32_t scan_interval;
    uint8_t scan_type; /* current scan type: active or passive */
    uint8_t scan_duplicate_filter; /* duplicate filter enabled for scan */
    uint16_t adv_interval_min;
    uint16_t adv_interval_max;
    tBTM_BLE_AFP afp; /* advertising filter policy */
    tBTM_BLE_SFP sfp; /* scanning filter policy */

    tBLE_ADDR_TYPE adv_addr_type;
    uint8_t evt_type;
    uint8_t adv_mode;
    tBLE_BD_ADDR direct_bda;
    tBTM_BLE_EVT directed_conn;
    uint8_t fast_adv_on;
    TIMER_LIST_ENT   fast_adv_timer;
    uint8_t broadcast_adv_timer_on;
    TIMER_LIST_ENT   ble_broadcast_timer;

    uint8_t adv_len;
    uint8_t adv_data_cache[BTM_BLE_CACHE_ADV_DATA_MAX];

    /* inquiry BD addr database */
    uint8_t num_bd_entries;
    uint8_t max_bd_entries;
    tBTM_BLE_LOCAL_ADV_DATA adv_data;
    tBTM_BLE_ADV_CHNL_MAP adv_chnl_map;

    TIMER_LIST_ENT   inquiry_timer;
    uint8_t scan_rsp;
    uint8_t state; /* Current state that the inquiry process is in */
    int8_t tx_power;
} tBTM_BLE_INQ_CB;


/* random address resolving complete callback */
typedef void (tBTM_BLE_RESOLVE_CBACK)(void *match_rec, void *p);

typedef void (tBTM_BLE_ADDR_CBACK)(BD_ADDR_PTR static_random, void *p);

/* random address management control block */
typedef struct {
    tBLE_ADDR_TYPE              own_addr_type;         /* local device LE address type */
    BD_ADDR                     private_addr;
    BD_ADDR                     random_bda;
    uint8_t                     busy;
    tBTM_BLE_ADDR_CBACK         *p_generate_cback;
    void                        *p;
    TIMER_LIST_ENT   refresh_raddr_timer;
} tBTM_LE_RANDOM_CB;

#define BTM_BLE_MAX_BG_CONN_DEV_NUM    10

typedef struct {
    uint16_t              min_conn_int;
    uint16_t              max_conn_int;
    uint16_t              slave_latency;
    uint16_t              supervision_tout;

} tBTM_LE_CONN_PRAMS;


typedef struct {
    BD_ADDR     bd_addr;
    uint8_t       attr;
    uint8_t     is_connected;
    uint8_t     in_use;
} tBTM_LE_BG_CONN_DEV;

/* white list using state as a bit mask */
#define BTM_BLE_WL_IDLE         0
#define BTM_BLE_WL_INIT         1
#define BTM_BLE_WL_SCAN         2
#define BTM_BLE_WL_ADV          4
typedef uint8_t tBTM_BLE_WL_STATE;

/* resolving list using state as a bit mask */
#define BTM_BLE_RL_IDLE         0
#define BTM_BLE_RL_INIT         1
#define BTM_BLE_RL_SCAN         2
#define BTM_BLE_RL_ADV          4
typedef uint8_t tBTM_BLE_RL_STATE;

/* BLE connection state */
#define BLE_CONN_IDLE    0
#define BLE_DIR_CONN     1
#define BLE_BG_CONN      2
#define BLE_CONN_CANCEL  3
typedef uint8_t tBTM_BLE_CONN_ST;

typedef struct {
    void    *p_param;
} tBTM_BLE_CONN_REQ;

/* LE state request */
#define BTM_BLE_STATE_INVALID               0
#define BTM_BLE_STATE_CONN_ADV              1
#define BTM_BLE_STATE_INIT                  2
#define BTM_BLE_STATE_MASTER                3
#define BTM_BLE_STATE_SLAVE                 4
#define BTM_BLE_STATE_LO_DUTY_DIR_ADV       5
#define BTM_BLE_STATE_HI_DUTY_DIR_ADV       6
#define BTM_BLE_STATE_NON_CONN_ADV          7
#define BTM_BLE_STATE_PASSIVE_SCAN          8
#define BTM_BLE_STATE_ACTIVE_SCAN           9
#define BTM_BLE_STATE_SCAN_ADV              10
#define BTM_BLE_STATE_MAX                   11
typedef uint8_t tBTM_BLE_STATE;

#define BTM_BLE_STATE_CONN_ADV_BIT          0x0001
#define BTM_BLE_STATE_INIT_BIT              0x0002
#define BTM_BLE_STATE_MASTER_BIT            0x0004
#define BTM_BLE_STATE_SLAVE_BIT             0x0008
#define BTM_BLE_STATE_LO_DUTY_DIR_ADV_BIT   0x0010
#define BTM_BLE_STATE_HI_DUTY_DIR_ADV_BIT   0x0020
#define BTM_BLE_STATE_NON_CONN_ADV_BIT      0x0040
#define BTM_BLE_STATE_PASSIVE_SCAN_BIT      0x0080
#define BTM_BLE_STATE_ACTIVE_SCAN_BIT       0x0100
#define BTM_BLE_STATE_SCAN_ADV_BIT          0x0200
typedef uint16_t tBTM_BLE_STATE_MASK;

#define BTM_BLE_STATE_ALL_MASK              0x03ff
#define BTM_BLE_STATE_ALL_ADV_MASK          (BTM_BLE_STATE_NON_CONN_ADV_BIT|BTM_BLE_STATE_CONN_ADV_BIT|BTM_BLE_STATE_LO_DUTY_DIR_ADV_BIT|BTM_BLE_STATE_HI_DUTY_DIR_ADV_BIT|BTM_BLE_STATE_SCAN_ADV_BIT)
#define BTM_BLE_STATE_ALL_SCAN_MASK         (BTM_BLE_STATE_PASSIVE_SCAN_BIT|BTM_BLE_STATE_ACTIVE_SCAN_BIT)
#define BTM_BLE_STATE_ALL_CONN_MASK         (BTM_BLE_STATE_MASTER_BIT|BTM_BLE_STATE_SLAVE_BIT)

#ifndef BTM_LE_RESOLVING_LIST_MAX
#define BTM_LE_RESOLVING_LIST_MAX     0x20
#endif

typedef struct {
    BD_ADDR         *resolve_q_random_pseudo;
    uint8_t           *resolve_q_action;
    uint8_t           q_next;
    uint8_t           q_pending;
} tBTM_BLE_RESOLVE_Q;

typedef struct {
    uint8_t     in_use;
    uint8_t     to_add;
    BD_ADDR     bd_addr;
    uint8_t       attr;
} tBTM_BLE_WL_OP;

/* BLE privacy mode */
#define BTM_PRIVACY_NONE    0              /* BLE no privacy */
#define BTM_PRIVACY_1_1     1              /* BLE privacy 1.1, do not support privacy 1.0 */
#define BTM_PRIVACY_1_2     2              /* BLE privacy 1.2 */
#define BTM_PRIVACY_MIXED   3              /* BLE privacy mixed mode, broadcom propietary mode */
typedef uint8_t tBTM_PRIVACY_MODE;

/* data length change event callback */
typedef void (tBTM_DATA_LENGTH_CHANGE_CBACK)(uint16_t max_tx_length, uint16_t max_rx_length);

/* Define BLE Device Management control structure
*/
typedef struct {
    uint8_t scan_activity;         /* LE scan activity mask */

    /*****************************************************
    **      BLE Inquiry
    *****************************************************/
    tBTM_BLE_INQ_CB inq_var;

    /* observer callback and timer */
    tBTM_INQ_RESULTS_CB *p_obs_results_cb;
    tBTM_CMPL_CB *p_obs_cmpl_cb;
    TIMER_LIST_ENT   observer_timer;

    /* background connection procedure cb value */
    tBTM_BLE_CONN_TYPE bg_conn_type;
    uint32_t scan_int;
    uint32_t scan_win;
    tBTM_BLE_SEL_CBACK *p_select_cback;

    /* white list information */
    uint8_t white_list_avail_size;
    tBTM_BLE_WL_STATE wl_state;

    /*ble_suggested_default_data_length*/
    uint16_t ble_suggested_default_data_length;

    fixed_queue_t *conn_pending_q;
    tBTM_BLE_CONN_ST conn_state;

    /* random address management control block */
    tBTM_LE_RANDOM_CB addr_mgnt_cb;

    uint8_t enabled;

#if BLE_PRIVACY_SPT == TRUE
    uint8_t mixed_mode; /* privacy 1.2 mixed mode is on or not */
    tBTM_PRIVACY_MODE privacy_mode; /* privacy mode */
    uint8_t resolving_list_avail_size; /* resolving list available size */
    tBTM_BLE_RESOLVE_Q resolving_list_pend_q; /* Resolving list queue */
    tBTM_BLE_RL_STATE suspended_rl_state; /* Suspended resolving list state */
    uint8_t *irk_list_mask; /* IRK list availability mask, up to max entry bits */
    tBTM_BLE_RL_STATE rl_state; /* Resolving list state */
#endif

    tBTM_BLE_WL_OP wl_op_q[BTM_BLE_MAX_BG_CONN_DEV_NUM];

    /* current BLE link state */
    tBTM_BLE_STATE_MASK cur_states; /* bit mask of tBTM_BLE_STATE */
    uint8_t link_count[2]; /* total link count master and slave*/
} tBTM_BLE_CB;

#ifdef __cplusplus
extern "C" {
#endif

extern void btm_ble_adv_raddr_timer_timeout(void *data);
extern void btm_ble_refresh_raddr_timer_timeout(void *data);
extern void btm_ble_process_adv_pkt(uint8_t *p);
extern void btm_ble_proc_scan_rsp_rpt(uint8_t *p);
extern tBTM_STATUS btm_ble_read_remote_name(BD_ADDR remote_bda, tBTM_INQ_INFO *p_cur,
        tBTM_CMPL_CB *p_cb);
extern uint8_t btm_ble_cancel_remote_name(BD_ADDR remote_bda);

extern tBTM_STATUS btm_ble_set_discoverability(uint16_t combined_mode);
extern tBTM_STATUS btm_ble_set_connectability(uint16_t combined_mode);
extern tBTM_STATUS btm_ble_start_inquiry(uint8_t mode, uint8_t   duration);
extern void btm_ble_stop_scan(void);
extern void btm_clear_all_pending_le_entry(void);

extern void btm_ble_stop_scan();
extern uint8_t btm_ble_send_extended_scan_params(uint8_t scan_type, uint32_t scan_int,
        uint32_t scan_win, uint8_t addr_type_own,
        uint8_t scan_filter_policy);
extern void btm_ble_stop_inquiry(void);
extern void btm_ble_init(void);
extern void btm_ble_free(void);

extern void btm_ble_connected(uint8_t *bda, uint16_t handle, uint8_t enc_mode, uint8_t role,
                              tBLE_ADDR_TYPE addr_type, uint8_t addr_matched);
extern void btm_ble_read_remote_features_complete(uint8_t *p);
extern void btm_ble_write_adv_enable_complete(uint8_t *p);
extern void btm_ble_conn_complete(uint8_t *p, uint16_t evt_len, uint8_t enhanced);
extern void btm_read_ble_local_supported_states_complete(uint8_t *p, uint16_t evt_len);
extern tBTM_BLE_CONN_ST btm_ble_get_conn_st(void);
extern void btm_ble_set_conn_st(tBTM_BLE_CONN_ST new_st);
extern uint8_t *btm_ble_build_adv_data(tBTM_BLE_AD_MASK *p_data_mask, uint8_t **p_dst,
                                       tBTM_BLE_ADV_DATA *p_data);
extern tBTM_STATUS btm_ble_start_adv(void);
extern tBTM_STATUS btm_ble_stop_adv(void);
extern tBTM_STATUS btm_ble_start_scan(void);
extern void btm_ble_create_ll_conn_complete(uint8_t status);
extern void btm_read_ble_suggested_default_data_length_complete(uint8_t *p, uint16_t evt_len);


/* LE security function from btm_sec.c */
#if SMP_INCLUDED == TRUE
extern void btm_ble_link_sec_check(BD_ADDR bd_addr, tBTM_LE_AUTH_REQ auth_req,
                                   tBTM_BLE_SEC_REQ_ACT *p_sec_req_act);
extern void btm_ble_ltk_request_reply(BD_ADDR bda,  uint8_t use_stk, BT_OCTET16 stk);
extern uint8_t btm_proc_smp_cback(tSMP_EVT event, BD_ADDR bd_addr, tSMP_EVT_DATA *p_data);
extern tBTM_STATUS btm_ble_set_encryption(BD_ADDR bd_addr, tBTM_BLE_SEC_ACT sec_act,
        uint8_t link_role);
extern void btm_ble_ltk_request(uint16_t handle, uint8_t rand[8], uint16_t ediv);
extern tBTM_STATUS btm_ble_start_encrypt(BD_ADDR bda, uint8_t use_stk, BT_OCTET16 stk);
extern void btm_ble_link_encrypted(BD_ADDR bd_addr, uint8_t encr_enable);
#endif

/* LE device management functions */
extern void btm_ble_reset_id(void);

/* security related functions */
extern void btm_ble_increment_sign_ctr(BD_ADDR bd_addr, uint8_t is_local);
extern uint8_t btm_get_local_div(BD_ADDR bd_addr, uint16_t *p_div);
extern uint8_t btm_ble_get_enc_key_type(BD_ADDR bd_addr, uint8_t *p_key_types);

extern void btm_ble_test_command_complete(uint8_t *p);
extern void btm_ble_rand_enc_complete(uint8_t *p, uint16_t op_code,
                                      tBTM_RAND_ENC_CB *p_enc_cplt_cback);

extern void btm_sec_save_le_key(BD_ADDR bd_addr, tBTM_LE_KEY_TYPE key_type,
                                tBTM_LE_KEY_VALUE *p_keys, uint8_t pass_to_application);
extern void btm_ble_update_sec_key_size(BD_ADDR bd_addr, uint8_t enc_key_size);
extern uint8_t btm_ble_read_sec_key_size(BD_ADDR bd_addr);

/* white list function */
extern uint8_t btm_update_dev_to_white_list(uint8_t to_add, BD_ADDR bd_addr);
extern void btm_update_scanner_filter_policy(tBTM_BLE_SFP scan_policy);
extern void btm_update_adv_filter_policy(tBTM_BLE_AFP adv_policy);
extern void btm_ble_clear_white_list(void);
extern void btm_read_white_list_size_complete(uint8_t *p, uint16_t evt_len);
extern void btm_ble_add_2_white_list_complete(uint8_t status);
extern void btm_ble_remove_from_white_list_complete(uint8_t *p, uint16_t evt_len);
extern void btm_ble_clear_white_list_complete(uint8_t *p, uint16_t evt_len);
extern void btm_ble_white_list_init(uint8_t white_list_size);

/* background connection function */
extern uint8_t btm_ble_suspend_bg_conn(void);
extern uint8_t btm_ble_resume_bg_conn(void);
extern void btm_ble_initiate_select_conn(BD_ADDR bda);
extern uint8_t btm_ble_start_auto_conn(uint8_t start);
extern uint8_t btm_ble_start_select_conn(uint8_t start, tBTM_BLE_SEL_CBACK   *p_select_cback);
extern uint8_t btm_ble_renew_bg_conn_params(uint8_t add, BD_ADDR bd_addr);
extern void btm_write_dir_conn_wl(BD_ADDR target_addr);
extern void btm_ble_update_mode_operation(uint8_t link_role, BD_ADDR bda, uint8_t status);
extern uint8_t btm_execute_wl_dev_operation(void);
extern void btm_ble_update_link_topology_mask(uint8_t role, uint8_t increase);

/* direct connection utility */
extern uint8_t btm_send_pending_direct_conn(void);
extern void btm_ble_enqueue_direct_conn_req(void *p_param);

/* BLE address management */
extern void btm_gen_resolvable_private_addr(void *p_cmd_cplt_cback);
extern void btm_gen_non_resolvable_private_addr(tBTM_BLE_ADDR_CBACK *p_cback, void *p);
extern void btm_ble_resolve_random_addr(BD_ADDR random_bda, tBTM_BLE_RESOLVE_CBACK *p_cback,
                                        void *p);
extern void btm_gen_resolve_paddr_low(tBTM_RAND_ENC *p);

/*  privacy function */
#if (defined BLE_PRIVACY_SPT && BLE_PRIVACY_SPT == TRUE)
/* BLE address mapping with CS feature */
extern uint8_t btm_identity_addr_to_random_pseudo(BD_ADDR bd_addr, uint8_t *p_addr_type,
        uint8_t refresh);
extern uint8_t btm_random_pseudo_to_identity_addr(BD_ADDR random_pseudo,
        uint8_t *p_static_addr_type);
extern void btm_ble_refresh_peer_resolvable_private_addr(BD_ADDR pseudo_bda, BD_ADDR rra,
        uint8_t rra_type);
extern void btm_ble_refresh_local_resolvable_private_addr(BD_ADDR pseudo_addr, BD_ADDR local_rpa);
extern void btm_ble_read_resolving_list_entry_complete(uint8_t *p, uint16_t evt_len) ;
extern void btm_ble_remove_resolving_list_entry_complete(uint8_t *p, uint16_t evt_len);
extern void btm_ble_add_resolving_list_entry_complete(uint8_t *p, uint16_t evt_len);
extern void btm_ble_clear_resolving_list_complete(uint8_t *p, uint16_t evt_len);
extern void btm_read_ble_resolving_list_size_complete(uint8_t *p, uint16_t evt_len);
extern void btm_ble_enable_resolving_list(uint8_t);
extern uint8_t btm_ble_disable_resolving_list(uint8_t rl_mask, uint8_t to_resume);
extern void btm_ble_enable_resolving_list_for_platform(uint8_t rl_mask);
extern void btm_ble_resolving_list_init(uint8_t max_irk_list_sz);
extern void btm_ble_resolving_list_cleanup(void);
#endif

extern void btm_ble_multi_adv_configure_rpa(tBTM_BLE_MULTI_ADV_INST *p_inst);
extern void btm_ble_multi_adv_init(void);
extern void *btm_ble_multi_adv_get_ref(uint8_t inst_id);
extern void btm_ble_multi_adv_cleanup(void);
extern void btm_ble_multi_adv_reenable(uint8_t inst_id);
extern void btm_ble_multi_adv_enb_privacy(uint8_t enable);
extern char btm_ble_map_adv_tx_power(int tx_power_index);
extern void btm_ble_batchscan_init(void);
extern void btm_ble_batchscan_cleanup(void);
extern void btm_ble_adv_filter_init(void);
extern void btm_ble_adv_filter_cleanup(void);
extern uint8_t btm_ble_topology_check(tBTM_BLE_STATE_MASK request);
extern uint8_t btm_ble_clear_topology_mask(tBTM_BLE_STATE_MASK request_state);
extern uint8_t btm_ble_set_topology_mask(tBTM_BLE_STATE_MASK request_state);

#if BTM_BLE_CONFORMANCE_TESTING == TRUE
extern void btm_ble_set_no_disc_if_pair_fail(uint8_t disble_disc);
extern void btm_ble_set_test_mac_value(uint8_t enable, uint8_t *p_test_mac_val);
extern void btm_ble_set_test_local_sign_cntr_value(uint8_t enable, uint32_t test_local_sign_cntr);
extern void btm_set_random_address(BD_ADDR random_bda);
extern void btm_ble_set_keep_rfu_in_auth_req(uint8_t keep_rfu);
#endif


#ifdef __cplusplus
}
#endif

#endif
