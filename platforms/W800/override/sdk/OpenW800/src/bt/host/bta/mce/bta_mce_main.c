/******************************************************************************
 *
 *  Copyright (C) 2014 The Android Open Source Project
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
 *  This is the main implementation file for the BTA MCE I/F
 *
 ******************************************************************************/
#include "bt_target.h"
#if defined(BTA_MCE_INCLUDED) && (BTA_MCE_INCLUDED == TRUE)
#include <stddef.h>
#include "bta_api.h"
#include "bta_sys.h"
#include "bta_mce_api.h"
#include "bta_mce_int.h"

/*****************************************************************************
** Constants and types
*****************************************************************************/

#if BTA_DYNAMIC_MEMORY == FALSE
tBTA_MCE_CB bta_mce_cb;
#endif

/* state machine action enumeration list */
#define BTA_MCE_NUM_ACTIONS  (BTA_MCE_MAX_INT_EVT & 0x00ff)

/* type for action functions */
typedef void (*tBTA_MCE_ACTION)(tBTA_MCE_MSG *p_data);

/* action function list */
const tBTA_MCE_ACTION bta_mce_action[] = {
    bta_mce_enable,                    /* BTA_MCE_API_ENABLE_EVT */
    bta_mce_get_remote_mas_instances,  /* BTA_MCE_API_GET_REMOTE_MAS_INSTANCES_EVT */
};

/*******************************************************************************
**
** Function         bta_mce_sm_execute
**
** Description      State machine event handling function for MCE
**
**
** Returns          void
**
*******************************************************************************/
uint8_t bta_mce_sm_execute(BT_HDR *p_msg)
{
    if(p_msg == NULL) {
        return FALSE;
    }

    uint8_t ret = FALSE;
    uint16_t action = (p_msg->event & 0x00ff);

    /* execute action functions */
    if(action < BTA_MCE_NUM_ACTIONS) {
        (*bta_mce_action[action])((tBTA_MCE_MSG *)p_msg);
        ret = TRUE;
    }

    return(ret);
}
#endif
