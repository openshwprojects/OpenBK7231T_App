/******************************************************************************
 *
 *  Copyright (C) 2002-2012 Broadcom Corporation
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
 *  This file contains the definition of the btm control block when
 *  BTM_DYNAMIC_MEMORY is used.
 *
 ******************************************************************************/
#include <string.h>
#include <assert.h>
#include "bt_types.h"
#include "bt_target.h"
#include "btm_int.h"

/* Global BTM control block structure
*/
#if BTM_DYNAMIC_MEMORY == FALSE
tBTM_CB  btm_cb;
#else
tBTM_CB *btm_cb_ptr = NULL;
#endif

/*******************************************************************************
**
** Function         btm_init
**
** Description      This function is called at BTM startup to allocate the
**                  control block (if using dynamic memory), and initializes the
**                  tracing level.  It then initializes the various components of
**                  btm.
**
** Returns          void
**
*******************************************************************************/
void btm_init(void)
{
    /* All fields are cleared; nonzero fields are reinitialized in appropriate function */
#if BTM_DYNAMIC_MEMORY == TRUE
    btm_cb_ptr = (tBTM_CB *)GKI_os_malloc(sizeof(tBTM_CB));
    assert(btm_cb_ptr != NULL);
#endif
    wm_memset(&btm_cb, 0, sizeof(tBTM_CB));
    btm_cb.page_queue = fixed_queue_new(SIZE_MAX);
    btm_cb.sec_pending_q = fixed_queue_new(SIZE_MAX);
#ifdef USE_ALARM
    btm_cb.sec_collision_timer = alarm_new("btm.sec_collision_timer");
    btm_cb.pairing_timer = alarm_new("btm.pairing_timer");
#endif
#if defined(BTM_INITIAL_TRACE_LEVEL)
    btm_cb.trace_level = BTM_INITIAL_TRACE_LEVEL;
#else
    btm_cb.trace_level = BT_TRACE_LEVEL_NONE;    /* No traces */
#endif
    /* Initialize BTM component structures */
    btm_inq_db_init();                  /* Inquiry Database and Structures */
    btm_acl_init();                     /* ACL Database and Structures */
    /* Security Manager Database and Structures */
    //hci_dbg_msg("Skip pts secure only mode\r\n");

    if(/*stack_config_get_interface()->get_pts_secure_only_mode()*/0) {
        btm_sec_init(BTM_SEC_MODE_SC);
    } else {
        btm_sec_init(BTM_SEC_MODE_SP);
    }

#if BTM_SCO_INCLUDED == TRUE
    btm_sco_init();                     /* SCO Database and Structures (If included) */
#endif
    btm_cb.sec_dev_rec = list_new(GKI_freebuf);
    btm_dev_init();                     /* Device Manager Structures & HCI_Reset */
}
/*******************************************************************************
**
** Function         btm_free
**
** Description      This function is called at btu core free the fixed queue
**
** Returns          void
**
*******************************************************************************/
void btm_free(void)
{
    fixed_queue_free(btm_cb.page_queue, GKI_freebuf);
    fixed_queue_free(btm_cb.sec_pending_q, GKI_freebuf);
    list_free(btm_cb.sec_dev_rec);
#if BTM_DYNAMIC_MEMORY == TRUE

    if(btm_cb_ptr) {
        GKI_os_free(btm_cb_ptr);
        btm_cb_ptr = NULL;
    }

#endif
}


