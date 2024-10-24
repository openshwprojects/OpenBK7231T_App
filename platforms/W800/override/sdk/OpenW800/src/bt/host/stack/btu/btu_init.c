/******************************************************************************
 *
 *  Copyright (C) 2000-2012 Broadcom Corporation
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
 *  This module contains the routines that load and shutdown the core stack
 *  components.
 *
 ******************************************************************************/

#include "bt_target.h"
#include <string.h>
#include "gki.h"
#include "dyn_mem.h"

#include "btu.h"
#include "btm_int.h"
#include "sdpint.h"
#include "l2c_int.h"

#if (BLE_INCLUDED == TRUE)
#include "gatt_api.h"
#include "gatt_int.h"
#if SMP_INCLUDED == TRUE
#include "smp_int.h"
#endif
#endif

#ifdef BLUEDROID70
// Increase BTU task thread priority to avoid pre-emption
// of audio realated tasks.
#define BTU_TASK_THREAD_PRIORITY -19

extern fixed_queue_t *btif_msg_queue;

// Communication queue from bta thread to bt_workqueue.
fixed_queue_t *btu_bta_msg_queue;

// Communication queue from hci thread to bt_workqueue.
extern fixed_queue_t *btu_hci_msg_queue;

// General timer queue.
fixed_queue_t *btu_general_alarm_queue;

thread_t *bt_workqueue_thread;
static const char *BT_WORKQUEUE_NAME = "bt_workqueue";

void btu_task_start_up(void *context);
void btu_task_shut_down(void *context);
#endif

extern void PLATFORM_DisableHciTransport(uint8_t bDisable);
/*****************************************************************************
**                          V A R I A B L E S                                *
******************************************************************************/
// TODO(cmanton) Move this out of this file
const BD_ADDR   BT_BD_ANY = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};



/*****************************************************************************
**
** Function         btu_init_core
**
** Description      Initialize control block memory for each core component.
**
**
** Returns          void
**
******************************************************************************/
void btu_init_core(void)
{
    /* Initialize the mandatory core stack components */
    btm_init();
    l2c_init();
    sdp_init();
#if BLE_INCLUDED == TRUE
    gatt_init();
#if (defined(SMP_INCLUDED) && SMP_INCLUDED == TRUE)
    SMP_Init();
#endif
    btm_ble_init();
#endif
}

/*****************************************************************************
**
** Function         btu_free_core
**
** Description      Releases control block memory for each core component.
**
**
** Returns          void
**
******************************************************************************/
void btu_free_core(void)
{
    /* Free the mandatory core stack components */
    l2c_free();
#if BLE_INCLUDED == TRUE
    gatt_free();
    btm_ble_free();
#endif
    sdp_deinit();
    btm_free();
#if (defined(SMP_INCLUDED) && SMP_INCLUDED == TRUE)
    SMP_Free();
#endif
}

#ifdef BLUEDROID70
/*****************************************************************************
**
** Function         BTU_StartUp
**
** Description      Initializes the BTU control block.
**
**                  NOTE: Must be called before creating any tasks
**                      (RPC, BTU, HCIT, APPL, etc.)
**
** Returns          void
**
******************************************************************************/
void BTU_StartUp(void)
{
    btu_trace_level = HCI_INITIAL_TRACE_LEVEL;
    btu_bta_msg_queue = fixed_queue_new(SIZE_MAX);

    if(btu_bta_msg_queue == NULL) {
        goto error_exit;
    }

    btu_general_alarm_queue = fixed_queue_new(SIZE_MAX);

    if(btu_general_alarm_queue == NULL) {
        goto error_exit;
    }

    bt_workqueue_thread = thread_new(BT_WORKQUEUE_NAME);

    if(bt_workqueue_thread == NULL) {
        goto error_exit;
    }

    thread_set_priority(bt_workqueue_thread, BTU_TASK_THREAD_PRIORITY);
    // Continue startup on bt workqueue thread.
    thread_post(bt_workqueue_thread, btu_task_start_up, NULL);
    return;
error_exit:
    ;
    LOG_ERROR(LOG_TAG, "%s Unable to allocate resources for bt_workqueue", __func__);
    BTU_ShutDown();
}

void BTU_ShutDown(void)
{
    btu_task_shut_down(NULL);
    fixed_queue_free(btu_bta_msg_queue, NULL);
    btu_bta_msg_queue = NULL;
    fixed_queue_free(btu_general_alarm_queue, NULL);
    btu_general_alarm_queue = NULL;
    thread_free(bt_workqueue_thread);
    bt_workqueue_thread = NULL;
}
#endif
/*****************************************************************************
**
** Function         BTE_Init
**
** Description      Initializes the BTU control block.
**
**                  NOTE: Must be called before creating any tasks
**                      (RPC, BTU, HCIT, APPL, etc.)
**
** Returns          void
**
******************************************************************************/
void BTE_Init(void)
{
    int i = 0;
    wm_memset(&btu_cb, 0, sizeof(tBTU_CB));
    btu_cb.hcit_acl_pkt_size = BTU_DEFAULT_DATA_SIZE + HCI_DATA_PREAMBLE_SIZE;
#if (BLE_INCLUDED == TRUE)
    btu_cb.hcit_ble_acl_pkt_size = BTU_DEFAULT_BLE_DATA_SIZE + HCI_DATA_PREAMBLE_SIZE;
#endif
    btu_cb.trace_level = HCI_INITIAL_TRACE_LEVEL;

    for(i = 0; i < BTU_MAX_LOCAL_CTRLS; i++) {  /* include BR/EDR */
        btu_cb.hci_cmd_cb[i].cmd_window = 1;
    }
}


/*****************************************************************************
**
** Function         BTU_AclPktSize
**
** Description      export the ACL packet size.
**
** Returns          uint16_t
**
******************************************************************************/
uint16_t BTU_AclPktSize(void)
{
    return btu_cb.hcit_acl_pkt_size;
}
/*****************************************************************************
**
** Function         BTU_BleAclPktSize
**
** Description      export the BLE ACL packet size.
**
** Returns          uint16_t
**
******************************************************************************/
uint16_t BTU_BleAclPktSize(void)
{
#if BLE_INCLUDED == TRUE
    return btu_cb.hcit_ble_acl_pkt_size;
#else
    return 0;
#endif
}

/*******************************************************************************
**
** Function         btu_uipc_rx_cback
**
** Description
**
**
** Returns          void
**
*******************************************************************************/
void btu_uipc_rx_cback(BT_HDR *p_msg)
{
    BT_TRACE(TRACE_LAYER_BTM, TRACE_TYPE_DEBUG, "btu_uipc_rx_cback event 0x%x, len %d, offset %d",
             p_msg->event, p_msg->len, p_msg->offset);
    GKI_send_msg(BTU_TASK, BTU_HCI_RCV_MBOX, p_msg);
}
