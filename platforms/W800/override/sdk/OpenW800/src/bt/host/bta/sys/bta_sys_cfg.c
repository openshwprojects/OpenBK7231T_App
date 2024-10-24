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
 *  This file contains compile-time configurable constants for the BTA
 *  system manager.
 *
 ******************************************************************************/

#include "bt_target.h"
#include "gki.h"
#include "bta_sys.h"

/* GKI task mailbox event for BTA. */
#ifndef BTA_MBOX_EVT
#define BTA_MBOX_EVT                TASK_MBOX_2_EVT_MASK
#endif

/* GKI task mailbox for BTA. */
#ifndef BTA_MBOX
#define BTA_MBOX                    TASK_MBOX_2
#endif

/* GKI timer id used for protocol timer for BTA. */
#ifndef BTA_TIMER
#define BTA_TIMER                   TIMER_1
#endif

const tBTA_SYS_CFG bta_sys_cfg = {
    BTA_MBOX_EVT,               /* GKI mailbox event */
    BTA_MBOX,                   /* GKI mailbox id */
    BTA_TIMER,                  /* GKI timer id */
    APPL_INITIAL_TRACE_LEVEL    /* initial trace level */
};

tBTA_SYS_CFG *p_bta_sys_cfg = (tBTA_SYS_CFG *) &bta_sys_cfg;


