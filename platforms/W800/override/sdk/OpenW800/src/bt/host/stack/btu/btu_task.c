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


#include <assert.h>
#include "bt_target.h"
#include "gki.h"
#include "bt_types.h"
#include "hcimsgs.h"
#include "l2c_int.h"
#include "btu.h"

#include "sdpint.h"


#include "btm_api.h"
#include "btm_int.h"

#if (defined(EVAL) && EVAL == TRUE)
#include "btu_eval.h"
#endif

#if GAP_INCLUDED == TRUE
#include "gap_int.h"
#endif

#if (defined(OBX_INCLUDED) && OBX_INCLUDED == TRUE)
#include "obx_int.h"

#if (defined(BIP_INCLUDED) && BIP_INCLUDED == TRUE)
#include "bip_int.h"
#endif /* BIP */

#if (BPP_SND_INCLUDED == TRUE ||  BPP_INCLUDED == TRUE)
#include "bpp_int.h"
#endif /* BPP */

#endif /* OBX */

#include "bt_trace.h"

/* BTE application task */
#if APPL_INCLUDED == TRUE
#include "bte_appl.h"
#endif

#if (defined(RPC_INCLUDED) && RPC_INCLUDED == TRUE)
#include "rpct_main.h"
#endif

#if (defined(BNEP_INCLUDED) && BNEP_INCLUDED == TRUE)
#include "bnep_int.h"
#endif

#if (defined(PAN_INCLUDED) && PAN_INCLUDED == TRUE)
#include "pan_int.h"
#endif

#if (defined(SAP_SERVER_INCLUDED) && SAP_SERVER_INCLUDED == TRUE)
#include "sap_int.h"
#endif

#if (defined(HID_DEV_INCLUDED) && HID_DEV_INCLUDED == TRUE )
#include "hidd_int.h"
#endif

#if (defined(HID_HOST_INCLUDED) && HID_HOST_INCLUDED == TRUE )
#include "hidh_int.h"
#endif

#if (defined(AVDT_INCLUDED) && AVDT_INCLUDED == TRUE)
#include "avdt_int.h"
#else
extern void avdt_rcv_sync_info(BT_HDR *p_buf);   /* this is for hci_test */
#endif

#if (defined(MCA_INCLUDED) && MCA_INCLUDED == TRUE)
#include "mca_api.h"
#include "mca_defs.h"
#include "mca_int.h"
#endif


#if (defined(BTU_BTA_INCLUDED) && BTU_BTA_INCLUDED == TRUE)
#include "bta_sys.h"
#endif

#if (BLE_INCLUDED == TRUE)
#include "gatt_int.h"
#if (SMP_INCLUDED == TRUE)
#include "smp_int.h"
#endif
#include "btm_ble_int.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

extern void BTE_InitStack(void);
extern void BTE_DeinitStack(void);

#ifdef __cplusplus
}
#endif

/* Define BTU storage area
*/
#if BTU_DYNAMIC_MEMORY == FALSE
tBTU_CB  btu_cb;
#endif


/* Define a function prototype to allow a generic timeout handler */
typedef void (tUSER_TIMEOUT_FUNC)(TIMER_LIST_ENT *p_tle);

static int hci4_preload_cmpl = 0;

/* Define BTU storage area
*/
uint8_t btu_trace_level = HCI_INITIAL_TRACE_LEVEL;

/*******************************************************************************
**
** Function         btu_task
**
** Description      This is the main task of the Bluetooth Upper Layers unit.
**                  It sits in a loop waiting for messages, and dispatches them
**                  to the appropiate handlers.
**
** Returns          should never return
**
*******************************************************************************/
uint32_t btu_task_prev(uint32_t param)
{
    hci4_preload_cmpl = 0;
    GKI_wait(0xFFFF, 0);
    return 0;
}


uint32_t btu_task(uint32_t event)
{
    BT_HDR          *p_msg;
    uint8_t            i;
    uint16_t           mask;
    uint8_t          handled;
#if (defined(HCISU_H4_INCLUDED) && HCISU_H4_INCLUDED == TRUE)

    if(hci4_preload_cmpl == 0) {
        if(event & EVENT_MASK(GKI_SHUTDOWN_EVT)) {
            /* indicates BT ENABLE abort */
            BT_TRACE(TRACE_LAYER_BTU, TRACE_TYPE_WARNING, "btu_task start abort!");
        } else if(event & BT_EVT_PRELOAD_CMPL) {
            hci4_preload_cmpl = 1;
            //BT_TRACE(TRACE_LAYER_BTU, TRACE_TYPE_API, "btu_task received preload complete event");
            /* Initialize the mandatory core stack control blocks
               (BTU, BTM, L2CAP, and SDP)
             */
            btu_init_core();
            /* Initialize any optional stack components */
            BTE_InitStack();
#if (defined(BTU_BTA_INCLUDED) && BTU_BTA_INCLUDED == TRUE)
            bta_sys_init();
#endif
            /* Initialise platform trace levels at this point as BTE_InitStack() and bta_sys_init()
             * reset the control blocks and preset the trace level with XXX_INITIAL_TRACE_LEVEL
             */
#if ( BT_USE_TRACES==TRUE )
            //BTE_InitTraceLevels();
#endif
            /* Send a startup evt message to BTIF_TASK to kickstart the init procedure */
            GKI_send_event(BTIF_TASK, BT_EVT_TRIGGER_STACK_INIT);
        } else {
            BT_TRACE(TRACE_LAYER_BTU, TRACE_TYPE_WARNING,
                     "btu_task ignore evt %04x while pending for preload complete", event);
        }

        GKI_wait(0xFFFF, 0);
        return (0);
    };

#endif

    /*process, events */
    do {
        if(event & TASK_MBOX_0_EVT_MASK) {
            /* Process all messages in the queue */
            while((p_msg = (BT_HDR *) GKI_read_mbox(BTU_HCI_RCV_MBOX)) != NULL) {
                /* Determine the input message type. */
                switch(p_msg->event & BT_EVT_MASK) {
                    case BT_EVT_TO_BTU_HCI_ACL:
                        /* All Acl Data goes to L2CAP */
                        l2c_rcv_acl_data(p_msg);
                        break;

                    case BT_EVT_TO_BTU_L2C_SEG_XMIT:
                        /* L2CAP segment transmit complete */
                        l2c_link_segments_xmitted(p_msg);
                        break;

                    case BT_EVT_TO_BTU_HCI_SCO:
#if BTM_SCO_INCLUDED == TRUE
                        btm_route_sco_data(p_msg);
                        break;
#endif

                    case BT_EVT_TO_BTU_HCI_EVT:
                        btu_hcif_process_event((uint8_t)(p_msg->event & BT_SUB_EVT_MASK), p_msg);
                        GKI_freebuf(p_msg);
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
                        /* If host receives events which it doesn't response to, */
                        /* host should start idle timer to enter sleep mode.     */
                        //BT_TRACE(TRACE_LAYER_BTU, TRACE_TYPE_WARNING,"run into btu_check_bt_sleep, %s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
                        btu_check_bt_sleep();
#endif
                        break;

                    case BT_EVT_TO_BTU_HCI_CMD:
                        btu_hcif_send_cmd((uint8_t)(p_msg->event & BT_SUB_EVT_MASK), p_msg);
                        break;
#if (defined(GAP_CONN_INCLUDED) && GAP_CONN_INCLUDED == TRUE && GAP_CONN_POST_EVT_INCLUDED == TRUE)

                    case BT_EVT_TO_GAP_MSG :
                        gap_proc_btu_event(p_msg);
                        GKI_freebuf(p_msg);
                        break;
#endif

                    case BT_EVT_TO_START_TIMER :
                        /* Start free running 1 second timer for list management */
                        GKI_start_timer(TIMER_0, GKI_SECS_TO_TICKS(1), TRUE);
                        GKI_freebuf(p_msg);
                        break;

                    case BT_EVT_TO_STOP_TIMER:
                        if(GKI_timer_queue_is_empty(&btu_cb.timer_queue)) {
                            GKI_stop_timer(TIMER_0);
                        }

                        GKI_freebuf(p_msg);
                        break;

                    case BT_EVT_TO_START_TIMER_ONESHOT:
                        if(!GKI_timer_queue_is_empty(&btu_cb.timer_queue_oneshot)) {
                            TIMER_LIST_ENT *tle = GKI_timer_getfirst(&btu_cb.timer_queue_oneshot);
                            // Start non-repeating timer.
                            GKI_start_timer(TIMER_3, tle->ticks, FALSE);
                        } else {
                            BTM_TRACE_WARNING("Oneshot timer queue empty when received start request");
                        }

                        GKI_freebuf(p_msg);
                        break;

                    case BT_EVT_TO_STOP_TIMER_ONESHOT:
                        if(GKI_timer_queue_is_empty(&btu_cb.timer_queue_oneshot)) {
                            GKI_stop_timer(TIMER_3);
                        } else {
                            BTM_TRACE_WARNING("Oneshot timer queue not empty when received stop request");
                        }

                        GKI_freebuf(p_msg);
                        break;
#if defined(QUICK_TIMER_TICKS_PER_SEC) && (QUICK_TIMER_TICKS_PER_SEC > 0)

                    case BT_EVT_TO_START_QUICK_TIMER :
                        GKI_start_timer(TIMER_2, QUICK_TIMER_TICKS, TRUE);
                        GKI_freebuf(p_msg);
                        break;
#endif

                    default:
                        i = 0;
                        mask = (uint16_t)(p_msg->event & BT_EVT_MASK);
                        handled = FALSE;

                        for(; !handled && i < BTU_MAX_REG_EVENT; i++) {
                            if(btu_cb.event_reg[i].event_cb == NULL) {
                                continue;
                            }

                            if(mask == btu_cb.event_reg[i].event_range) {
                                if(btu_cb.event_reg[i].event_cb) {
                                    btu_cb.event_reg[i].event_cb(p_msg);
                                    handled = TRUE;
                                }
                            }
                        }

                        if(handled == FALSE) {
                            GKI_freebuf(p_msg);
                        }

                        break;
                }
            }
        }

        if(event & TIMER_0_EVT_MASK) {
            GKI_update_timer_list(&btu_cb.timer_queue, 1);

            while(!GKI_timer_queue_is_empty(&btu_cb.timer_queue)) {
                TIMER_LIST_ENT *p_tle = GKI_timer_getfirst(&btu_cb.timer_queue);

                if(p_tle->ticks != 0) {
                    break;
                }

                GKI_remove_from_timer_list(&btu_cb.timer_queue, p_tle);

                //BT_TRACE(TRACE_LAYER_BTU, TRACE_TYPE_WARNING,"%s [%s] %d, p_tle->event:%d\n", __FILE__, __FUNCTION__, __LINE__, p_tle->event);
                /* call timer callback */
                if(p_tle->p_cback) {
                    (*p_tle->p_cback)((void *)p_tle->param);
                } else {
                    assert(0);
                }
            }

            /* if timer list is empty stop periodic GKI timer */
            if(btu_cb.timer_queue.p_first == NULL) {
                GKI_stop_timer(TIMER_0);
            }
        }

#if defined(QUICK_TIMER_TICKS_PER_SEC) && (QUICK_TIMER_TICKS_PER_SEC > 0)

        if(event & TIMER_2_EVT_MASK) {
            btu_process_quick_timer_evt();
        }

#endif
#if (defined(BTU_BTA_INCLUDED) && BTU_BTA_INCLUDED == TRUE)

        if(event & TASK_MBOX_2_EVT_MASK) {
            while((p_msg = (BT_HDR *) GKI_read_mbox(TASK_MBOX_2)) != NULL) {
                bta_sys_event(p_msg);
            }
        }

        if(event & TIMER_1_EVT_MASK) {
            bta_sys_timer_update();
        }

#endif

        if(event & TIMER_3_EVT_MASK) {
            BTM_TRACE_API("Received oneshot timer event complete");

            if(!GKI_timer_queue_is_empty(&btu_cb.timer_queue_oneshot)) {
                //TIMER_LIST_ENT *p_tle = GKI_timer_getfirst(&btu_cb.timer_queue_oneshot);
                int32_t ticks_since_last_update = GKI_timer_ticks_getinitial(GKI_timer_getfirst(
                        &btu_cb.timer_queue_oneshot));
                GKI_update_timer_list(&btu_cb.timer_queue_oneshot, ticks_since_last_update);
            }

            while(!GKI_timer_queue_is_empty(&btu_cb.timer_queue_oneshot)) {
                TIMER_LIST_ENT *p_tle = GKI_timer_getfirst(&btu_cb.timer_queue_oneshot);

                if(p_tle->ticks != 0) {
                    break;
                }

                GKI_remove_from_timer_list(&btu_cb.timer_queue_oneshot, p_tle);

                /* call timer callback */
                if(p_tle->p_cback) {
                    (*p_tle->p_cback)((void *)p_tle->param);
                } else {
                    assert(0);
                }
            }

            /* Update GKI timer with new tick value from first timer. */
            if(!GKI_timer_queue_is_empty(&btu_cb.timer_queue_oneshot)) {
                TIMER_LIST_ENT *p_tle = GKI_timer_getfirst(&btu_cb.timer_queue_oneshot);

                if(p_tle->ticks > 0) {
                    GKI_start_timer(TIMER_3, p_tle->ticks, FALSE);
                }
            } else {
                GKI_stop_timer(TIMER_3);
            }
        }

        if(event & EVENT_MASK(APPL_EVT_7)) {
            BT_TRACE(TRACE_LAYER_BTU, TRACE_TYPE_WARNING, "Mark break or return...\n");
            break;
        }
    } while(0);

    GKI_wait(0xFFFF, 0);
    return(0);
}

/*******************************************************************************
**
** Function         btu_start_timer
**
** Description      Start a timer for the specified amount of time.
**                  NOTE: The timeout resolution is in SECONDS! (Even
**                          though the timer structure field is ticks)
**
** Returns          void
**
*******************************************************************************/
void btu_start_timer(TIMER_LIST_ENT *p_tle, uint16_t type, uint32_t timeout)
{
    BT_HDR *p_msg;
    GKI_disable();

    /* if timer list is currently empty, start periodic GKI timer */
    if(btu_cb.timer_queue.p_first == NULL) {
        /* if timer starts on other than BTU task */
        if(GKI_get_taskid() != BTU_TASK) {
            /* post event to start timer in BTU task */
            if((p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE)) != NULL) {
                p_msg->event = BT_EVT_TO_START_TIMER;
                GKI_send_msg(BTU_TASK, TASK_MBOX_0, p_msg);
            }
        } else {
            /* Start free running 1 second timer for list management */
            GKI_start_timer(TIMER_0, GKI_SECS_TO_TICKS(1), TRUE);
        }
    }

    GKI_remove_from_timer_list(&btu_cb.timer_queue, p_tle);
    p_tle->event = type;
    p_tle->ticks = timeout;
    p_tle->ticks_initial = timeout;
    GKI_add_to_timer_list(&btu_cb.timer_queue, p_tle);
    GKI_enable();
}

/*******************************************************************************
**
** Function         btu_remaining_time
**
** Description      Return amount of time to expire
**
** Returns          time in second
**
*******************************************************************************/
uint32_t btu_remaining_time(TIMER_LIST_ENT *p_tle)
{
    return(GKI_get_remaining_ticks(&btu_cb.timer_queue, p_tle));
}

/*******************************************************************************
**
** Function         btu_stop_timer
**
** Description      Stop a timer.
**
** Returns          void
**
*******************************************************************************/
void btu_stop_timer(TIMER_LIST_ENT *p_tle)
{
    BT_HDR *p_msg;
    GKI_disable();
    GKI_remove_from_timer_list(&btu_cb.timer_queue, p_tle);

    /* if timer is stopped on other than BTU task */
    if(GKI_get_taskid() != BTU_TASK) {
        /* post event to stop timer in BTU task */
        if((p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE)) != NULL) {
            p_msg->event = BT_EVT_TO_STOP_TIMER;
            GKI_send_msg(BTU_TASK, TASK_MBOX_0, p_msg);
        }
    } else {
        /* if timer list is empty stop periodic GKI timer */
        if(btu_cb.timer_queue.p_first == NULL) {
            GKI_stop_timer(TIMER_0);
        }
    }

    GKI_enable();
}

#if defined(QUICK_TIMER_TICKS_PER_SEC) && (QUICK_TIMER_TICKS_PER_SEC > 0)
/*******************************************************************************
**
** Function         btu_start_quick_timer
**
** Description      Start a timer for the specified amount of time.
**                  NOTE: The timeout resolution depends on including modules.
**                  QUICK_TIMER_TICKS_PER_SEC should be used to convert from
**                  time to ticks.
**
**
** Returns          void
**
*******************************************************************************/
void btu_start_quick_timer(TIMER_LIST_ENT *p_tle, uint16_t type, uint32_t timeout)
{
    BT_HDR *p_msg;
    GKI_disable();

    /* if timer list is currently empty, start periodic GKI timer */
    if(btu_cb.quick_timer_queue.p_first == NULL) {
        /* script test calls stack API without posting event */
        if(GKI_get_taskid() != BTU_TASK) {
            /* post event to start timer in BTU task */
            if((p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE)) != NULL) {
                p_msg->event = BT_EVT_TO_START_QUICK_TIMER;
                GKI_send_msg(BTU_TASK, TASK_MBOX_0, p_msg);
            }
        } else {
            GKI_start_timer(TIMER_2, QUICK_TIMER_TICKS, TRUE);
        }
    }

    GKI_remove_from_timer_list(&btu_cb.quick_timer_queue, p_tle);
    p_tle->event = type;
    p_tle->ticks = timeout;
    p_tle->ticks_initial = timeout;
    GKI_add_to_timer_list(&btu_cb.quick_timer_queue, p_tle);
    GKI_enable();
}


/*******************************************************************************
**
** Function         btu_stop_quick_timer
**
** Description      Stop a timer.
**
** Returns          void
**
*******************************************************************************/
void btu_stop_quick_timer(TIMER_LIST_ENT *p_tle)
{
    GKI_disable();
    GKI_remove_from_timer_list(&btu_cb.quick_timer_queue, p_tle);

    /* if timer list is empty stop periodic GKI timer */
    if(btu_cb.quick_timer_queue.p_first == NULL) {
        GKI_stop_timer(TIMER_2);
    }

    GKI_enable();
}

/*******************************************************************************
**
** Function         btu_process_quick_timer_evt
**
** Description      Process quick timer event
**
** Returns          void
**
*******************************************************************************/
void btu_process_quick_timer_evt(void)
{
    process_quick_timer_evt(&btu_cb.quick_timer_queue);

    /* if timer list is empty stop periodic GKI timer */
    if(btu_cb.quick_timer_queue.p_first == NULL) {
        GKI_stop_timer(TIMER_2);
    }
}

/*******************************************************************************
**
** Function         process_quick_timer_evt
**
** Description      Process quick timer event
**
** Returns          void
**
*******************************************************************************/
void process_quick_timer_evt(TIMER_LIST_Q *p_tlq)
{
    TIMER_LIST_ENT  *p_tle;
    GKI_update_timer_list(p_tlq, 1);

    while((p_tlq->p_first) && (!p_tlq->p_first->ticks)) {
        p_tle = p_tlq->p_first;
        GKI_remove_from_timer_list(p_tlq, p_tle);

        /* call timer callback */
        if(p_tle->p_cback) {
            (*p_tle->p_cback)((void *)p_tle->param);
        }
    }
}
#endif /* defined(QUICK_TIMER_TICKS_PER_SEC) && (QUICK_TIMER_TICKS_PER_SEC > 0) */

/*
 * Starts a oneshot timer with a timeout in seconds.
 */
void btu_start_timer_oneshot(TIMER_LIST_ENT *p_tle, uint16_t type, uint32_t timeout_in_secs)
{
    int32_t timeout_in_ticks = GKI_SECS_TO_TICKS(timeout_in_secs);
    BTM_TRACE_DEBUG("Starting oneshot timer type:%d timeout:%ds", type, timeout_in_secs);
    GKI_disable();

    if(GKI_timer_queue_is_empty(&btu_cb.timer_queue_oneshot)) {
    }

    GKI_remove_from_timer_list(&btu_cb.timer_queue_oneshot, p_tle);
    p_tle->event = type;
    p_tle->ticks = timeout_in_ticks;
    p_tle->ticks_initial = timeout_in_ticks;
    GKI_add_to_timer_list(&btu_cb.timer_queue_oneshot, p_tle);

    /* RPC to BTU thread if timer start request from non-BTU task */
    if(GKI_get_taskid() != BTU_TASK) {
        /* post event to start timer in BTU task */
        BTM_TRACE_WARNING("Posting oneshot timer event to btu_task");
        BT_HDR *p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE);

        if(p_msg != NULL) {
            p_msg->event = BT_EVT_TO_START_TIMER_ONESHOT;
            GKI_send_msg(BTU_TASK, TASK_MBOX_0, p_msg);
        }
    } else {
        TIMER_LIST_ENT *tle = GKI_timer_getfirst(&btu_cb.timer_queue_oneshot);
        GKI_start_timer(TIMER_3, tle->ticks, FALSE);
    }

    GKI_enable();
}

void btu_stop_timer_oneshot(TIMER_LIST_ENT *p_tle)
{
    GKI_disable();
    GKI_remove_from_timer_list(&btu_cb.timer_queue_oneshot, p_tle);

    if(GKI_get_taskid() != BTU_TASK) {
        /* post event to stop timer in BTU task */
        BT_HDR *p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE);

        if(p_msg != NULL) {
            p_msg->event = BT_EVT_TO_STOP_TIMER_ONESHOT;
            GKI_send_msg(BTU_TASK, TASK_MBOX_0, p_msg);
        }
    } else {
        if(GKI_timer_queue_is_empty(&btu_cb.timer_queue_oneshot)) {
            BTM_TRACE_DEBUG("Stopping oneshot timer TIMER_3\r\n");
            GKI_stop_timer(TIMER_3);
        } else {
            BTM_TRACE_DEBUG("Request to stop oneshot timer with non empty queue\r\n");
        }
    }

    GKI_enable();
}

#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btu_check_bt_sleep
**
** Description      This function is called to check if controller can go to sleep.
**
** Returns          void
**
*******************************************************************************/
void btu_check_bt_sleep(void)
{
    if((btu_cb.hci_cmd_cb[LOCAL_BR_EDR_CONTROLLER_ID].cmd_cmpl_q.count == 0)
            && (btu_cb.hci_cmd_cb[LOCAL_BR_EDR_CONTROLLER_ID].cmd_xmit_q.count == 0)) {
        if(l2cb.controller_xmit_window == l2cb.num_lm_acl_bufs) {
            /* enable dev to sleep  in the cmd cplt and cmd status only and num cplt packet */
            HCI_LP_ALLOW_BT_DEVICE_SLEEP();
        }
    }
}
#endif
