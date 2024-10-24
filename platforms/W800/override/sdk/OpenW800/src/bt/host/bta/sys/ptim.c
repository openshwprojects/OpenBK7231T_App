/******************************************************************************
 *
 *  Copyright (C) 2003-2012 Broadcom Corporation
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
 *  Protocol timer services.
 *
 ******************************************************************************/

#include "bt_target.h"
#include "gki.h"
#include "ptim.h"
#include "bta_sys.h"

/*******************************************************************************
**
** Function         ptim_init
**
** Description      Initialize a protocol timer control block.  Parameter
**                  period is the GKI timer period in milliseconds.  Parameter
**                  timer_id is the GKI timer id.
**
** Returns          void
**
*******************************************************************************/
void ptim_init(tPTIM_CB *p_cb, uint16_t period, uint8_t timer_id)
{
    GKI_init_timer_list(&p_cb->timer_queue);
    p_cb->period = period;
    p_cb->timer_id = timer_id;
}

/*******************************************************************************
**
** Function         ptim_timer_update
**
** Description      Update the protocol timer list and handle expired timers.
**                  This function is called from the task running the protocol
**                  timers when the periodic GKI timer expires.
**
** Returns          void
**
*******************************************************************************/
void ptim_timer_update(tPTIM_CB *p_cb)
{
    TIMER_LIST_ENT *p_tle;
    BT_HDR *p_msg;
    uint32_t new_ticks_count;
    int32_t  period_in_ticks;
    /* To handle the case when the function is called less frequently than the period
       we must convert determine the number of ticks since the last update, then
       convert back to milliseconds before updating timer list */
    new_ticks_count = GKI_get_tick_count();

    /* Check for wrapped condition */
    if(new_ticks_count >= p_cb->last_gki_ticks) {
        period_in_ticks = (int32_t)(new_ticks_count - p_cb->last_gki_ticks);
    } else {
        period_in_ticks = (int32_t)(((uint32_t)0xffffffff - p_cb->last_gki_ticks)
                                    + new_ticks_count + 1);
    }

    /* update timer list */
    GKI_update_timer_list(&p_cb->timer_queue, GKI_TICKS_TO_MS(period_in_ticks));
    p_cb->last_gki_ticks = new_ticks_count;

    /* while there are expired timers */
    while((p_cb->timer_queue.p_first) && (p_cb->timer_queue.p_first->ticks <= 0)) {
        /* removed expired timer from list */
        p_tle = p_cb->timer_queue.p_first;
        GKI_remove_from_timer_list(&p_cb->timer_queue, p_tle);

        /* call timer callback */
        if(p_tle->p_cback) {
            (*p_tle->p_cback)(p_tle);
        } else if(p_tle->event) {
            if((p_msg = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL) {
                p_msg->event = p_tle->event;
                p_msg->layer_specific = (uint16_t)p_tle->data;
                bta_sys_sendmsg(p_msg);
            }
        }
    }

    /* if timer list is empty stop periodic GKI timer */
    if(p_cb->timer_queue.p_first == NULL) {
        GKI_stop_timer(p_cb->timer_id);
    }
}

/*******************************************************************************
**
** Function         ptim_start_timer
**
** Description      Start a protocol timer for the specified amount
**                  of time in seconds.
**
** Returns          void
**
*******************************************************************************/
void ptim_start_timer(tPTIM_CB *p_cb, TIMER_LIST_ENT *p_tle, uint16_t type, int32_t timeout)
{
    /* if timer list is currently empty, start periodic GKI timer */
    if(p_cb->timer_queue.p_first == NULL) {
        p_cb->last_gki_ticks = GKI_get_tick_count();
        GKI_start_timer(p_cb->timer_id, GKI_MS_TO_TICKS(p_cb->period), TRUE);
    }

    GKI_remove_from_timer_list(&p_cb->timer_queue, p_tle);
    p_tle->event = type;
    p_tle->ticks = timeout;
    p_tle->ticks_initial = timeout;
    GKI_add_to_timer_list(&p_cb->timer_queue, p_tle);
}

/*******************************************************************************
**
** Function         ptim_stop_timer
**
** Description      Stop a protocol timer.
**
** Returns          void
**
*******************************************************************************/
void ptim_stop_timer(tPTIM_CB *p_cb, TIMER_LIST_ENT *p_tle)
{
    GKI_remove_from_timer_list(&p_cb->timer_queue, p_tle);

    /* if timer list is empty stop periodic GKI timer */
    if(p_cb->timer_queue.p_first == NULL) {
        GKI_stop_timer(p_cb->timer_id);
    }
}

uint32_t ptim_get_timer_remaining_ms(tPTIM_CB *p_cb, TIMER_LIST_ENT *p_tle)
{
    /*GKI process ms equals to ticks ???, I will verify it later*/
    uint32_t r_ticks = GKI_get_remaining_ticks(&p_cb->timer_queue, p_tle);
    return r_ticks;
}
