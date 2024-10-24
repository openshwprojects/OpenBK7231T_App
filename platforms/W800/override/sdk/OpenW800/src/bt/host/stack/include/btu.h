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
 *  this file contains the main Bluetooth Upper Layer definitions. The Broadcom
 *  implementations of L2CAP RFCOMM, SDP and the BTIf run as one GKI task. The
 *  btu_task switches between them.
 *
 ******************************************************************************/

#ifndef BTU_H
#define BTU_H

#include "bt_target.h"
#include "gki.h"
#include "bt_common.h"

// HACK(zachoverflow): temporary dark magic
#define BTU_POST_TO_TASK_NO_GOOD_HORRIBLE_HACK 0x1700 // didn't look used in bt_types...here goes nothing
typedef struct {
    void (*callback)(BT_HDR *);
} post_to_task_hack_t;

typedef struct {
    void (*callback)(BT_HDR *);
    BT_HDR *response;
    void *context;
} command_complete_hack_t;

typedef struct {
    void (*callback)(BT_HDR *);
    uint8_t status;
    BT_HDR *command;
    void *context;
} command_status_hack_t;

/* Define the BTU mailbox usage
*/
#define BTU_HCI_RCV_MBOX        TASK_MBOX_0     /* Messages from HCI  */
#define BTU_BTIF_MBOX           TASK_MBOX_1     /* Messages to BTIF   */

/* callbacks
*/
typedef void (*tBTU_TIMER_CALLBACK)(TIMER_LIST_ENT *p_tle);
typedef void (*tBTU_EVENT_CALLBACK)(BT_HDR *p_hdr);


/* Define the timer types maintained by BTU
*/
#define BTU_TTYPE_BTM_DEV_CTL       1
#define BTU_TTYPE_L2CAP_LINK        2
#define BTU_TTYPE_L2CAP_CHNL        3
#define BTU_TTYPE_L2CAP_HOLD        4
#define BTU_TTYPE_SDP               5
#define BTU_TTYPE_BTM_SCO           6
#define BTU_TTYPE_BTM_ACL           9
#define BTU_TTYPE_BTM_RMT_NAME      10
#define BTU_TTYPE_RFCOMM_MFC        11
#define BTU_TTYPE_RFCOMM_PORT       12
#define BTU_TTYPE_TCS_L2CAP         13
#define BTU_TTYPE_TCS_CALL          14
#define BTU_TTYPE_TCS_WUG           15
#define BTU_TTYPE_AUTO_SYNC         16
#define BTU_TTYPE_CTP_RECON         17
#define BTU_TTYPE_CTP_T100          18
#define BTU_TTYPE_CTP_GUARD         19
#define BTU_TTYPE_CTP_DETACH        20

#define BTU_TTYPE_SPP_CONN_RETRY    21
#define BTU_TTYPE_USER_FUNC         22

#define BTU_TTYPE_FTP_DISC          25
#define BTU_TTYPE_OPP_DISC          26

#define BTU_TTYPE_CTP_TL_DISCVY     28
#define BTU_TTYPE_IPFRAG_TIMER      29
#define BTU_TTYPE_HSP2_AT_CMD_TO    30
#define BTU_TTYPE_HSP2_REPEAT_RING  31

#define BTU_TTYPE_CTP_GW_INIT       32
#define BTU_TTYPE_CTP_GW_CONN       33
#define BTU_TTYPE_CTP_GW_IDLE       35

#define BTU_TTYPE_ICP_L2CAP         36
#define BTU_TTYPE_ICP_T100          37

#define BTU_TTYPE_HSP2_WAIT_OK      38

/* HCRP Timers */
#define BTU_TTYPE_HCRP_NOTIF_REG    39
#define BTU_TTYPE_HCRP_PROTO_RSP    40
#define BTU_TTYPE_HCRP_CR_GRANT     41
#define BTU_TTYPE_HCRP_CR_CHECK     42
#define BTU_TTYPE_HCRP_W4_CLOSE     43

/* HCRPM Timers */
#define BTU_TTYPE_HCRPM_NOTIF_REG   44
#define BTU_TTYPE_HCRPM_NOTIF_KEEP  45
#define BTU_TTYPE_HCRPM_API_RSP     46
#define BTU_TTYPE_HCRPM_W4_OPEN     47
#define BTU_TTYPE_HCRPM_W4_CLOSE    48

/* BNEP Timers */
#define BTU_TTYPE_BNEP              50

/* OBX */
#define BTU_TTYPE_OBX_CLIENT_TO     51
#define BTU_TTYPE_OBX_SERVER_TO     52
#define BTU_TTYPE_OBX_SVR_SESS_TO   53


#define BTU_TTYPE_HSP2_SDP_FAIL_TO  55
#define BTU_TTYPE_HSP2_SDP_RTRY_TO  56

/* BTU internal */
/* unused                           60 */

#define BTU_TTYPE_AVDT_CCB_RET      61
#define BTU_TTYPE_AVDT_CCB_RSP      62
#define BTU_TTYPE_AVDT_CCB_IDLE     63
#define BTU_TTYPE_AVDT_SCB_TC       64

#define BTU_TTYPE_HID_DEV_REPAGE_TO 65
#define BTU_TTYPE_HID_HOST_REPAGE_TO 66

#define BTU_TTYPE_HSP2_DELAY_CKPD_RCV 67

#define BTU_TTYPE_SAP_TO            68

/* BPP Timer */
#define BTU_TTYPE_BPP_REF_CHNL     72

/* LP HC idle Timer */
#define BTU_TTYPE_LP_HC_IDLE_TO 74

/* Patch RAM Timer */
#define BTU_TTYPE_PATCHRAM_TO 75

/* eL2CAP Info Request and other proto cmds timer */
#define BTU_TTYPE_L2CAP_FCR_ACK     78
#define BTU_TTYPE_L2CAP_INFO        79

/* BTU internal for BR/EDR and AMP HCI command timeout (reserve up to 3 AMP controller) */
#define BTU_TTYPE_BTU_CMD_CMPL                      80
#define BTU_TTYPE_BTU_AMP1_CMD_CMPL                 81
#define BTU_TTYPE_BTU_AMP2_CMD_CMPL                 82
#define BTU_TTYPE_BTU_AMP3_CMD_CMPL                 83

#define BTU_TTYPE_MCA_CCB_RSP                       98

/* BTU internal timer for BLE activity */
#define BTU_TTYPE_BLE_INQUIRY                       99
#define BTU_TTYPE_BLE_GAP_LIM_DISC                  100
#define BTU_TTYPE_ATT_WAIT_FOR_RSP                  101
#define BTU_TTYPE_SMP_PAIRING_CMD                   102
#define BTU_TTYPE_BLE_RANDOM_ADDR                   103
#define BTU_TTYPE_ATT_WAIT_FOR_APP_RSP              104
#define BTU_TTYPE_ATT_WAIT_FOR_IND_ACK              105

#define BTU_TTYPE_BLE_GAP_FAST_ADV                  106
#define BTU_TTYPE_BLE_OBSERVE                       107


#define BTU_TTYPE_UCD_TO                            108
#define BTU_TTYPE_BLE_GAP_BROADCAST                 109


/* Define the BTU_TASK APPL events
*/
#if (defined(NFC_SHARED_TRANSPORT_ENABLED) && (NFC_SHARED_TRANSPORT_ENABLED==TRUE))
#define BTU_NFC_AVAILABLE_EVT   EVENT_MASK(APPL_EVT_0)  /* Notifies BTU task that NFC is available (used for shared NFC+BT transport) */
#endif

/* This is the inquiry response information held by BTU, and available
** to applications.
*/
typedef struct {
    BD_ADDR     remote_bd_addr;
    uint8_t       page_scan_rep_mode;
    uint8_t       page_scan_per_mode;
    uint8_t       page_scan_mode;
    DEV_CLASS   dev_class;
    uint16_t      clock_offset;
} tBTU_INQ_INFO;



#define BTU_MAX_REG_TIMER     (2)   /* max # timer callbacks which may register */
#define BTU_MAX_REG_EVENT     (6)   /* max # event callbacks which may register */
#define BTU_DEFAULT_DATA_SIZE (0x2a0)

#if (BLE_INCLUDED == TRUE)
#define BTU_DEFAULT_BLE_DATA_SIZE   (27)
#endif

/* structure to hold registered timers */
typedef struct {
    TIMER_LIST_ENT          *p_tle;      /* timer entry */
    tBTU_TIMER_CALLBACK     timer_cb;    /* callback triggered when timer expires */
} tBTU_TIMER_REG;

/* structure to hold registered event callbacks */
typedef struct {
    uint16_t                  event_range;  /* start of event range */
    tBTU_EVENT_CALLBACK     event_cb;     /* callback triggered when event is in range */
} tBTU_EVENT_REG;

#define NFC_MAX_LOCAL_CTRLS     0

/* the index to BTU command queue array */
#define NFC_CONTROLLER_ID       (1)
#define BTU_MAX_LOCAL_CTRLS     (1 + NFC_MAX_LOCAL_CTRLS) /* only BR/EDR */

/* AMP HCI control block */
typedef struct {
    BUFFER_Q         cmd_xmit_q;
    BUFFER_Q         cmd_cmpl_q;
    uint16_t           cmd_window;
    TIMER_LIST_ENT   cmd_cmpl_timer;        /* Command complete timer */
#if (defined(BTU_CMD_CMPL_TOUT_DOUBLE_CHECK) && BTU_CMD_CMPL_TOUT_DOUBLE_CHECK == TRUE)
    uint8_t          checked_hcisu;
#endif
} tHCI_CMD_CB;

/* Define structure holding BTU variables
*/
typedef struct {
    tBTU_TIMER_REG   timer_reg[BTU_MAX_REG_TIMER];
    tBTU_EVENT_REG   event_reg[BTU_MAX_REG_EVENT];

    TIMER_LIST_Q  quick_timer_queue;        /* Timer queue for transport level (100/10 msec)*/
    TIMER_LIST_Q  timer_queue;              /* Timer queue for normal BTU task (1 second)   */
    TIMER_LIST_Q  timer_queue_oneshot;      /* Timer queue for oneshot BTU tasks */

    TIMER_LIST_ENT   cmd_cmpl_timer;        /* Command complete timer */

    uint16_t    hcit_acl_data_size;           /* Max ACL data size across HCI transport    */
    uint16_t    hcit_acl_pkt_size;            /* Max ACL packet size across HCI transport  */
    /* (this is data size plus 4 bytes overhead) */

#if BLE_INCLUDED == TRUE
    uint16_t    hcit_ble_acl_data_size;           /* Max BLE ACL data size across HCI transport    */
    uint16_t    hcit_ble_acl_pkt_size;            /* Max BLE ACL packet size across HCI transport  */
    /* (this is data size plus 4 bytes overhead) */
#endif

    uint8_t     reset_complete;             /* TRUE after first ack from device received */
    uint8_t       trace_level;                /* Trace level for HCI layer */

    tHCI_CMD_CB hci_cmd_cb[BTU_MAX_LOCAL_CTRLS]; /* including BR/EDR */
} tBTU_CB;

#ifdef __cplusplus
extern "C" {
#endif
/* Global BTU data */
#if BTU_DYNAMIC_MEMORY == FALSE
extern tBTU_CB  btu_cb;
#else
extern tBTU_CB *btu_cb_ptr;
#define btu_cb (*btu_cb_ptr)
#endif

/* Global BTU data */
extern uint8_t btu_trace_level;

extern const BD_ADDR        BT_BD_ANY;

/* Functions provided by btu_task.c
************************************
*/
extern void btu_start_timer(TIMER_LIST_ENT *p_tle, uint16_t type, uint32_t timeout);
extern void btu_stop_timer(TIMER_LIST_ENT *p_tle);
extern void btu_start_timer_oneshot(TIMER_LIST_ENT *p_tle, uint16_t type, uint32_t timeout);
extern void btu_stop_timer_oneshot(TIMER_LIST_ENT *p_tle);

extern uint32_t btu_remaining_time(TIMER_LIST_ENT *p_tle);

extern void btu_uipc_rx_cback(BT_HDR *p_msg);

extern void btu_hcif_flush_cmd_queue(void);
/*
** Quick Timer
*/
#if defined(QUICK_TIMER_TICKS_PER_SEC) && (QUICK_TIMER_TICKS_PER_SEC > 0)
#define QUICK_TIMER_TICKS (GKI_SECS_TO_TICKS (1)/QUICK_TIMER_TICKS_PER_SEC)
extern void btu_start_quick_timer(TIMER_LIST_ENT *p_tle, uint16_t type, uint32_t timeout);
extern void btu_stop_quick_timer(TIMER_LIST_ENT *p_tle);
extern void btu_process_quick_timer_evt(void);
extern void process_quick_timer_evt(TIMER_LIST_Q *p_tlq);
#endif

#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
extern void btu_check_bt_sleep(void);
#endif

/* Functions provided by btu_hcif.c
************************************
*/
extern void  btu_hcif_process_event(uint8_t controller_id, BT_HDR *p_buf);
extern void  btu_hcif_send_cmd(uint8_t controller_id, BT_HDR *p_msg);
extern void  btu_hcif_send_host_rdy_for_data(void);
extern void  btu_hcif_cmd_timeout(void *p_data);

/* Functions provided by btu_core.c
************************************
*/
extern void  btu_init_core(void);
extern void  btu_free_core(void);

void BTU_StartUp(void);
void BTU_ShutDown(void);
extern void  BTE_Init(void);
extern uint16_t BTU_AclPktSize(void);
extern uint16_t BTU_BleAclPktSize(void);

#ifdef __cplusplus
}
#endif

#endif
