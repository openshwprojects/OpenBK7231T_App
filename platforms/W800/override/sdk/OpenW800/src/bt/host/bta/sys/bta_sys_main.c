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
 *  This is the main implementation file for the BTA system manager.
 *
 ******************************************************************************/

#define LOG_TAG "bt_bta_sys_main"

#include <assert.h>
#include <string.h>

#include "bta_api.h"
#include "bta_sys.h"
#include "bta_sys_int.h"
#include "btm_api.h"
#include "bt_common.h"
#include "gki.h"
#include "ptim.h"
#include "osi/include/fixed_queue.h"
#include "osi/include/hash_functions.h"
#include "osi/include/hash_map.h"
#include "osi/include/log.h"
#include "osi/include/osi.h"
#include "utl.h"

#if( defined BTA_AR_INCLUDED ) && (BTA_AR_INCLUDED == TRUE)
#include "bta_ar_api.h"
#endif

/* system manager control block definition */
#if BTA_DYNAMIC_MEMORY == TRUE
tBTA_SYS_CB *bta_sys_cb_ptr;
#else
tBTA_SYS_CB bta_sys_cb;
#endif

#ifdef USE_ALARM
fixed_queue_t *btu_bta_alarm_queue;
extern thread_t *bt_workqueue_thread;
// Communication queue between btu_task and bta.
extern fixed_queue_t *btu_bta_msg_queue;
#endif

/* trace level */
/* TODO Hard-coded trace levels -  Needs to be configurable */
uint8_t appl_trace_level = BT_TRACE_LEVEL_WARNING; //APPL_INITIAL_TRACE_LEVEL;
uint8_t btif_trace_level = BT_TRACE_LEVEL_VERBOSE;



static const tBTA_SYS_REG bta_sys_hw_reg = {
    bta_sys_sm_execute,
    NULL
};


/* type for action functions */
typedef void (*tBTA_SYS_ACTION)(tBTA_SYS_HW_MSG *p_data);

/* action function list */
const tBTA_SYS_ACTION bta_sys_action[] = {
    /* device manager local device API events - cf bta_sys.h for events */
    bta_sys_hw_api_enable,             /* 0  BTA_SYS_HW_API_ENABLE_EVT    */
    bta_sys_hw_evt_enabled,           /* 1  BTA_SYS_HW_EVT_ENABLED_EVT */
    bta_sys_hw_evt_stack_enabled,       /* 2  BTA_SYS_HW_EVT_STACK_ENABLED_EVT */
    bta_sys_hw_api_disable,             /* 3  BTA_SYS_HW_API_DISABLE_EVT     */
    bta_sys_hw_evt_disabled,           /* 4  BTA_SYS_HW_EVT_DISABLED_EVT  */
    bta_sys_hw_error                        /* 5   BTA_SYS_HW_ERROR_EVT  */
};

/* state machine action enumeration list */
enum {
    /* device manager local device API events */
    BTA_SYS_HW_API_ENABLE,
    BTA_SYS_HW_EVT_ENABLED,
    BTA_SYS_HW_EVT_STACK_ENABLED,
    BTA_SYS_HW_API_DISABLE,
    BTA_SYS_HW_EVT_DISABLED,
    BTA_SYS_HW_ERROR
};

#define BTA_SYS_NUM_ACTIONS  (BTA_SYS_MAX_EVT & 0x00ff)
#define BTA_SYS_IGNORE       BTA_SYS_NUM_ACTIONS

/* state table information */
#define BTA_SYS_ACTIONS              2       /* number of actions */
#define BTA_SYS_NEXT_STATE           2       /* position of next state */
#define BTA_SYS_NUM_COLS             3       /* number of columns in state tables */


/* state table for OFF state */
const uint8_t bta_sys_hw_off[][BTA_SYS_NUM_COLS] = {
    /* Event                    Action 1               Action 2             Next State */
    /* API_ENABLE    */  {BTA_SYS_HW_API_ENABLE,    BTA_SYS_IGNORE,     BTA_SYS_HW_STARTING},
    /* EVT_ENABLED   */  {BTA_SYS_IGNORE,           BTA_SYS_IGNORE,     BTA_SYS_HW_STARTING},
    /* STACK_ENABLED */  {BTA_SYS_IGNORE,           BTA_SYS_IGNORE,     BTA_SYS_HW_ON},
    /* API_DISABLE   */  {BTA_SYS_HW_EVT_DISABLED,  BTA_SYS_IGNORE,     BTA_SYS_HW_OFF},
    /* EVT_DISABLED  */  {BTA_SYS_IGNORE,           BTA_SYS_IGNORE,     BTA_SYS_HW_OFF},
    /* EVT_ERROR     */  {BTA_SYS_IGNORE,           BTA_SYS_IGNORE,     BTA_SYS_HW_OFF}
};

const uint8_t bta_sys_hw_starting[][BTA_SYS_NUM_COLS] = {
    /* Event                    Action 1                   Action 2               Next State */
    /* API_ENABLE    */  {BTA_SYS_IGNORE,               BTA_SYS_IGNORE,         BTA_SYS_HW_STARTING}, /* wait for completion event */
    /* EVT_ENABLED   */  {BTA_SYS_HW_EVT_ENABLED,       BTA_SYS_IGNORE,         BTA_SYS_HW_STARTING},
    /* STACK_ENABLED */  {BTA_SYS_HW_EVT_STACK_ENABLED, BTA_SYS_IGNORE,         BTA_SYS_HW_ON},
    /* API_DISABLE   */  {BTA_SYS_IGNORE,               BTA_SYS_IGNORE,         BTA_SYS_HW_STOPPING}, /* successive disable/enable: change state wait for completion to disable */
    /* EVT_DISABLED  */  {BTA_SYS_HW_EVT_DISABLED,      BTA_SYS_HW_API_ENABLE,  BTA_SYS_HW_STARTING}, /* successive enable/disable: notify, then restart HW */
    /* EVT_ERROR */      {BTA_SYS_HW_ERROR,             BTA_SYS_IGNORE,         BTA_SYS_HW_ON}
};

const uint8_t bta_sys_hw_on[][BTA_SYS_NUM_COLS] = {
    /* Event                    Action 1                   Action 2               Next State */
    /* API_ENABLE    */  {BTA_SYS_HW_API_ENABLE,        BTA_SYS_IGNORE,         BTA_SYS_HW_ON},
    /* EVT_ENABLED   */  {BTA_SYS_IGNORE,               BTA_SYS_IGNORE,         BTA_SYS_HW_ON},
    /* STACK_ENABLED */  {BTA_SYS_IGNORE,               BTA_SYS_IGNORE,         BTA_SYS_HW_ON},
    /* API_DISABLE   */  {BTA_SYS_HW_API_DISABLE,       BTA_SYS_IGNORE,         BTA_SYS_HW_ON}, /* don't change the state here, as some other modules might be active */
    /* EVT_DISABLED */   {BTA_SYS_HW_ERROR,             BTA_SYS_IGNORE,         BTA_SYS_HW_ON},
    /* EVT_ERROR */      {BTA_SYS_HW_ERROR,             BTA_SYS_IGNORE,         BTA_SYS_HW_ON}
};

const uint8_t bta_sys_hw_stopping[][BTA_SYS_NUM_COLS] = {
    /* Event                    Action 1                   Action 2               Next State */
    /* API_ENABLE    */  {BTA_SYS_IGNORE,               BTA_SYS_IGNORE,         BTA_SYS_HW_STARTING}, /* change state, and wait for completion event to enable */
    /* EVT_ENABLED   */  {BTA_SYS_HW_EVT_ENABLED,       BTA_SYS_IGNORE,         BTA_SYS_HW_STOPPING}, /* successive enable/disable: finish the enable before disabling */
    /* STACK_ENABLED */  {BTA_SYS_HW_EVT_STACK_ENABLED, BTA_SYS_HW_API_DISABLE, BTA_SYS_HW_STOPPING}, /* successive enable/disable: notify, then stop */
    /* API_DISABLE   */  {BTA_SYS_IGNORE,               BTA_SYS_IGNORE,         BTA_SYS_HW_STOPPING}, /* wait for completion event */
    /* EVT_DISABLED  */  {BTA_SYS_HW_EVT_DISABLED,      BTA_SYS_IGNORE,         BTA_SYS_HW_OFF},
    /* EVT_ERROR     */  {BTA_SYS_HW_API_DISABLE,       BTA_SYS_IGNORE,         BTA_SYS_HW_STOPPING}
};

typedef const uint8_t (*tBTA_SYS_ST_TBL)[BTA_SYS_NUM_COLS];

/* state table */
const tBTA_SYS_ST_TBL bta_sys_st_tbl[] = {
    bta_sys_hw_off,
    bta_sys_hw_starting,
    bta_sys_hw_on,
    bta_sys_hw_stopping
};

/*******************************************************************************
**
** Function         bta_sys_init
**
** Description      BTA initialization; called from task initialization.
**
**
** Returns          void
**
*******************************************************************************/
void bta_sys_init(void)
{
    wm_memset(&bta_sys_cb, 0, sizeof(tBTA_SYS_CB));
#ifdef USE_ALARM
    btu_bta_alarm_queue = fixed_queue_new(SIZE_MAX);
    alarm_register_processing_queue(btu_bta_alarm_queue, bt_workqueue_thread);
#else
    ptim_init(&bta_sys_cb.ptim_cb, BTA_SYS_TIMER_PERIOD, p_bta_sys_cfg->timer);
    bta_sys_cb.task_id = GKI_get_taskid();
#endif
    appl_trace_level = APPL_INITIAL_TRACE_LEVEL;
    /* register BTA SYS message handler */
    bta_sys_register(BTA_ID_SYS,  &bta_sys_hw_reg);
    /* register for BTM notifications */
    BTM_RegisterForDeviceStatusNotif((tBTM_DEV_STATUS_CB *)&bta_sys_hw_btm_cback);
#if( defined BTA_AR_INCLUDED ) && (BTA_AR_INCLUDED == TRUE)
    bta_ar_init();
#endif
}

void bta_sys_free(void)
{
#ifdef USE_ALARM
    alarm_unregister_processing_queue(btu_bta_alarm_queue);
    fixed_queue_free(btu_bta_alarm_queue, NULL);
    btu_bta_alarm_queue = NULL;
#endif
#if BTA_DYNAMIC_MEMORY == TRUE

    if(bta_sys_cb_ptr) {
        GKI_os_free(bta_sys_cb_ptr);
        bta_sys_cb_ptr = NULL;
    }

#endif
}

/*******************************************************************************
**
** Function         bta_dm_sm_execute
**
** Description      State machine event handling function for DM
**
**
** Returns          void
**
*******************************************************************************/
uint8_t bta_sys_sm_execute(BT_HDR *p_msg)
{
    uint8_t freebuf = TRUE;
    tBTA_SYS_ST_TBL      state_table;
    uint8_t               action;
    int                 i;
    APPL_TRACE_EVENT("bta_sys_sm_execute state:%d, event:0x%x",  bta_sys_cb.state, p_msg->event);
    /* look up the state table for the current state */
    state_table = bta_sys_st_tbl[bta_sys_cb.state];
    /* update state */
    bta_sys_cb.state = state_table[p_msg->event & 0x00ff][BTA_SYS_NEXT_STATE];

    /* execute action functions */
    for(i = 0; i < BTA_SYS_ACTIONS; i++) {
        if((action = state_table[p_msg->event & 0x00ff][i]) != BTA_SYS_IGNORE) {
            (*bta_sys_action[action])((tBTA_SYS_HW_MSG *) p_msg);
        } else {
            break;
        }
    }

    return freebuf;
}


void bta_sys_hw_register(tBTA_SYS_HW_MODULE module, tBTA_SYS_HW_CBACK *cback)
{
    bta_sys_cb.sys_hw_cback[module] = cback;
}


void bta_sys_hw_unregister(tBTA_SYS_HW_MODULE module)
{
    bta_sys_cb.sys_hw_cback[module] = NULL;
}

/*******************************************************************************
**
** Function         bta_sys_hw_btm_cback
**
** Description     This function is registered by BTA SYS to BTM in order to get status notifications
**
**
** Returns
**
*******************************************************************************/
void bta_sys_hw_btm_cback(tBTM_DEV_STATUS status)
{
    tBTA_SYS_HW_MSG *sys_event =
                    (tBTA_SYS_HW_MSG *)GKI_getbuf(sizeof(tBTA_SYS_HW_MSG));
    APPL_TRACE_DEBUG("%s was called with parameter: %i", __func__, status);

    /* send a message to BTA SYS */
    if(status == BTM_DEV_STATUS_UP) {
        sys_event->hdr.event = BTA_SYS_EVT_STACK_ENABLED_EVT;
    } else if(status == BTM_DEV_STATUS_DOWN) {
        sys_event->hdr.event = BTA_SYS_ERROR_EVT;
    } else {
        /* BTM_DEV_STATUS_CMD_TOUT is ignored for now. */
        GKI_free_and_reset_buf((void **)&sys_event);
    }

    if(sys_event) {
        bta_sys_sendmsg(sys_event);
    }
}

/*******************************************************************************
**
** Function         bta_sys_hw_error
**
** Description     In case the HW device stops answering... Try to turn it off, then re-enable all
**                      previously active SW modules.
**
** Returns          success or failure
**
*******************************************************************************/
void bta_sys_hw_error(tBTA_SYS_HW_MSG *p_sys_hw_msg)
{
    uint8_t module_index;
    UNUSED(p_sys_hw_msg);
    APPL_TRACE_DEBUG("%s", __FUNCTION__);

    for(module_index = 0; module_index < BTA_SYS_MAX_HW_MODULES; module_index++) {
        if(bta_sys_cb.sys_hw_module_active & ((uint32_t)1 << module_index)) {
            switch(module_index) {
                case BTA_SYS_HW_BLUETOOTH:

                    /* Send BTA_SYS_HW_ERROR_EVT to DM */
                    if(bta_sys_cb.sys_hw_cback[module_index] != NULL) {
                        bta_sys_cb.sys_hw_cback[module_index](BTA_SYS_HW_ERROR_EVT);
                    }

                    break;

                default:
                    /* not yet supported */
                    break;
            }
        }
    }
}



/*******************************************************************************
**
** Function         bta_sys_hw_enable
**
** Description     this function is called after API enable and HW has been turned on
**
**
** Returns          success or failure
**
*******************************************************************************/

void bta_sys_hw_api_enable(tBTA_SYS_HW_MSG *p_sys_hw_msg)
{
    if((!bta_sys_cb.sys_hw_module_active) && (bta_sys_cb.state != BTA_SYS_HW_ON)) {
        /* register which HW module was turned on */
        bta_sys_cb.sys_hw_module_active |= ((uint32_t)1 << p_sys_hw_msg->hw_module);
        tBTA_SYS_HW_MSG *p_msg =
                        (tBTA_SYS_HW_MSG *)GKI_getbuf(sizeof(tBTA_SYS_HW_MSG));
        p_msg->hdr.event = BTA_SYS_EVT_ENABLED_EVT;
        p_msg->hw_module = p_sys_hw_msg->hw_module;
        bta_sys_sendmsg(p_msg);
    } else {
        /* register which HW module was turned on */
        bta_sys_cb.sys_hw_module_active |= ((uint32_t)1 << p_sys_hw_msg->hw_module);

        /* HW already in use, so directly notify the caller */
        if(bta_sys_cb.sys_hw_cback[p_sys_hw_msg->hw_module ] != NULL) {
            bta_sys_cb.sys_hw_cback[p_sys_hw_msg->hw_module ](BTA_SYS_HW_ON_EVT);
        }
    }

    APPL_TRACE_EVENT("bta_sys_hw_api_enable for %d, active modules 0x%04X",
                     p_sys_hw_msg->hw_module, bta_sys_cb.sys_hw_module_active);
}

/*******************************************************************************
**
** Function         bta_sys_hw_disable
**
** Description     if no other module is using the HW, this function will call ( if defined ) a user-macro to turn off the HW
**
**
** Returns          success or failure
**
*******************************************************************************/
void bta_sys_hw_api_disable(tBTA_SYS_HW_MSG *p_sys_hw_msg)
{
    APPL_TRACE_DEBUG("bta_sys_hw_api_disable for %d, active modules: 0x%04X",
                     p_sys_hw_msg->hw_module, bta_sys_cb.sys_hw_module_active);
    /* make sure the related SW blocks were stopped */
    bta_sys_disable(p_sys_hw_msg->hw_module);
    /* register which module we turn off */
    bta_sys_cb.sys_hw_module_active &=  ~((uint32_t)1 << p_sys_hw_msg->hw_module);

    /* if there are still some SW modules using the HW, just provide an answer to the calling */
    if(bta_sys_cb.sys_hw_module_active != 0) {
        /*  if there are still some SW modules using the HW,  directly notify the caller */
        if(bta_sys_cb.sys_hw_cback[p_sys_hw_msg->hw_module ] != NULL) {
            bta_sys_cb.sys_hw_cback[p_sys_hw_msg->hw_module ](BTA_SYS_HW_OFF_EVT);
        }
    } else {
        /* manually update the state of our system */
        bta_sys_cb.state = BTA_SYS_HW_STOPPING;
        tBTA_SYS_HW_MSG *p_msg =
                        (tBTA_SYS_HW_MSG *)GKI_getbuf(sizeof(tBTA_SYS_HW_MSG));
        p_msg->hdr.event = BTA_SYS_EVT_DISABLED_EVT;
        p_msg->hw_module = p_sys_hw_msg->hw_module;
        bta_sys_sendmsg(p_msg);
    }
}

/*******************************************************************************
**
** Function         bta_sys_hw_event_enabled
**
** Description
**
**
** Returns          success or failure
**
*******************************************************************************/
void bta_sys_hw_evt_enabled(tBTA_SYS_HW_MSG *p_sys_hw_msg)
{
    APPL_TRACE_EVENT("bta_sys_hw_evt_enabled for %i", p_sys_hw_msg->hw_module);
    BTM_DeviceReset(NULL);
}


/*******************************************************************************
**
** Function         bta_sys_hw_event_disabled
**
** Description
**
**
** Returns          success or failure
**
*******************************************************************************/
void bta_sys_hw_evt_disabled(tBTA_SYS_HW_MSG *p_sys_hw_msg)
{
    uint8_t hw_module_index;
    APPL_TRACE_DEBUG("bta_sys_hw_evt_disabled - module 0x%X", p_sys_hw_msg->hw_module);

    for(hw_module_index = 0; hw_module_index < BTA_SYS_MAX_HW_MODULES; hw_module_index++) {
        if(bta_sys_cb.sys_hw_cback[hw_module_index] != NULL) {
            bta_sys_cb.sys_hw_cback[hw_module_index](BTA_SYS_HW_OFF_EVT);
        }
    }
}

/*******************************************************************************
**
** Function         bta_sys_hw_event_stack_enabled
**
** Description     we receive this event once the SW side is ready ( stack, FW download,... ),
**                       i.e. we can really start using the device. So notify the app.
**
** Returns          success or failure
**
*******************************************************************************/
void bta_sys_hw_evt_stack_enabled(tBTA_SYS_HW_MSG *p_sys_hw_msg)
{
    uint8_t hw_module_index;
    UNUSED(p_sys_hw_msg);
    APPL_TRACE_DEBUG(" bta_sys_hw_evt_stack_enabled!notify the callers");

    for(hw_module_index = 0; hw_module_index < BTA_SYS_MAX_HW_MODULES; hw_module_index++) {
        if(bta_sys_cb.sys_hw_cback[hw_module_index] != NULL) {
            bta_sys_cb.sys_hw_cback[hw_module_index](BTA_SYS_HW_ON_EVT);
        }
    }
}




/*******************************************************************************
**
** Function         bta_sys_event
**
** Description      BTA event handler; called from task event handler.
**
**
** Returns          void
**
*******************************************************************************/
void bta_sys_event(BT_HDR *p_msg)
{
    uint8_t       id;
    uint8_t     freebuf = TRUE;
    APPL_TRACE_EVENT("BTA got event 0x%x", p_msg->event);
    /* get subsystem id from event */
    id = (uint8_t)(p_msg->event >> 8);

    /* verify id and call subsystem event handler */
    if((id < BTA_ID_MAX) && (bta_sys_cb.reg[id] != NULL)) {
        freebuf = (*bta_sys_cb.reg[id]->evt_hdlr)(p_msg);
    } else {
        APPL_TRACE_DEBUG("BTA got unregistered event id %d\r\n", id);
    }

    if(freebuf) {
        GKI_freebuf(p_msg);
    }
}

/*******************************************************************************
**
** Function         bta_sys_timer_update
**
** Description      Update the BTA timer list and handle expired timers.
**
** Returns          void
**
*******************************************************************************/
void bta_sys_timer_update(void)
{
    if(!bta_sys_cb.timers_disabled) {
        ptim_timer_update(&bta_sys_cb.ptim_cb);
    }
}

/*******************************************************************************
**
** Function         bta_sys_register
**
** Description      Called by other BTA subsystems to register their event
**                  handler.
**
**
** Returns          void
**
*******************************************************************************/
void bta_sys_register(uint8_t id, const tBTA_SYS_REG *p_reg)
{
    bta_sys_cb.reg[id] = (tBTA_SYS_REG *) p_reg;
    bta_sys_cb.is_reg[id] = TRUE;
}

/*******************************************************************************
**
** Function         bta_sys_deregister
**
** Description      Called by other BTA subsystems to de-register
**                  handler.
**
**
** Returns          void
**
*******************************************************************************/
void bta_sys_deregister(uint8_t id)
{
    bta_sys_cb.is_reg[id] = FALSE;
}

/*******************************************************************************
**
** Function         bta_sys_is_register
**
** Description      Called by other BTA subsystems to get registeration
**                  status.
**
**
** Returns          void
**
*******************************************************************************/
uint8_t bta_sys_is_register(uint8_t id)
{
    return bta_sys_cb.is_reg[id];
}

/*******************************************************************************
**
** Function         bta_sys_sendmsg
**
** Description      Send a GKI message to BTA.  This function is designed to
**                  optimize sending of messages to BTA.  It is called by BTA
**                  API functions and call-in functions.
**
**
** Returns          void
**
*******************************************************************************/
void bta_sys_sendmsg(void *p_msg)
{
    APPL_TRACE_DEBUG("bta_sys_sendmsg:0x%08x, taskid:%d, mbox:%d\n", p_msg, bta_sys_cb.task_id,
                     p_bta_sys_cfg->mbox);
    GKI_send_msg(bta_sys_cb.task_id, p_bta_sys_cfg->mbox, p_msg);
}

/*******************************************************************************
**
** Function         bta_sys_start_timer
**
** Description      Start a protocol timer for the specified amount
**                  of time in milliseconds.
**
** Returns          void
**
*******************************************************************************/
void bta_sys_start_timer(TIMER_LIST_ENT *p_tle, uint64_t interval, uint16_t event,
                         uint16_t layer_specific)
{
#if 0
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    p_buf->event = event;
    p_buf->layer_specific = layer_specific;
    alarm_set_on_queue(alarm, interval, bta_sys_sendmsg, p_buf,
                       btu_bta_alarm_queue);
#else
    UNUSED(layer_specific);   //filled in alarm->data;
    ptim_start_timer(&bta_sys_cb.ptim_cb, p_tle, event, interval);
#endif
}
/*******************************************************************************
**
** Function         bta_sys_stop_timer
**
** Description      Stop a BTA timer.
**
** Returns          void
**
*******************************************************************************/
void bta_sys_stop_timer(TIMER_LIST_ENT *p_tle)
{
    ptim_stop_timer(&bta_sys_cb.ptim_cb, p_tle);
}

uint32_t bta_sys_get_timer_remaining_ms(TIMER_LIST_ENT *p_tle)
{
    return ptim_get_timer_remaining_ms(&bta_sys_cb.ptim_cb, p_tle);
}
/*******************************************************************************
**
** Function         bta_sys_disable
**
** Description      For each registered subsystem execute its disable function.
**
** Returns          void
**
*******************************************************************************/
void bta_sys_disable(tBTA_SYS_HW_MODULE module)
{
    int bta_id = 0;
    int bta_id_max = 0;
    APPL_TRACE_DEBUG("bta_sys_disable: module %i", module);

    switch(module) {
        case BTA_SYS_HW_BLUETOOTH:
            bta_id = BTA_ID_DM;
            bta_id_max = BTA_ID_BLUETOOTH_MAX;
            break;

        default:
            APPL_TRACE_WARNING("bta_sys_disable: unkown module");
            return;
    }

    for(; bta_id <= bta_id_max; bta_id++) {
        if(bta_sys_cb.reg[bta_id] != NULL) {
            if(bta_sys_cb.is_reg[bta_id] == TRUE  &&  bta_sys_cb.reg[bta_id]->disable != NULL) {
                (*bta_sys_cb.reg[bta_id]->disable)();
            }
        }
    }
}

/*******************************************************************************
**
** Function         bta_sys_disable_timers
**
** Description      Disable sys timer event handling
**
** Returns          void
**
*******************************************************************************/
void bta_sys_disable_timers(void)
{
    bta_sys_cb.timers_disabled = TRUE;
}

/*******************************************************************************
**
** Function         bta_sys_set_trace_level
**
** Description      Set trace level for BTA
**
** Returns          void
**
*******************************************************************************/
void bta_sys_set_trace_level(uint8_t level)
{
    appl_trace_level = level;
}

/*******************************************************************************
**
** Function         bta_sys_get_sys_features
**
** Description      Returns sys_features to other BTA modules.
**
** Returns          sys_features
**
*******************************************************************************/
uint16_t bta_sys_get_sys_features(void)
{
    return bta_sys_cb.sys_features;
}
