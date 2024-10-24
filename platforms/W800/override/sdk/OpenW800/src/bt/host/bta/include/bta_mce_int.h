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
 *  This is the private interface file for the BTA MCE I/F
 *
 ******************************************************************************/
#ifndef BTA_MCE_INT_H
#define BTA_MCE_INT_H

#include "bta_sys.h"
#include "bta_api.h"
#include "bta_mce_api.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

enum {
    /* these events are handled by the state machine */
    BTA_MCE_API_ENABLE_EVT = BTA_SYS_EVT_START(BTA_ID_MCE),
    BTA_MCE_API_GET_REMOTE_MAS_INSTANCES_EVT,
    BTA_MCE_MAX_INT_EVT
};

/* data type for BTA_MCE_API_ENABLE_EVT */
typedef struct {
    BT_HDR             hdr;
    tBTA_MCE_DM_CBACK  *p_cback;
} tBTA_MCE_API_ENABLE;

/* data type for BTA_MCE_API_GET_REMOTE_MAS_INSTANCES_EVT */
typedef struct {
    BT_HDR   hdr;
    BD_ADDR  bd_addr;
} tBTA_MCE_API_GET_REMOTE_MAS_INSTANCES;

/* union of all data types */
typedef union {
    /* GKI event buffer header */
    BT_HDR                                 hdr;
    tBTA_MCE_API_ENABLE                    enable;
    tBTA_MCE_API_GET_REMOTE_MAS_INSTANCES  get_rmt_mas;
} tBTA_MCE_MSG;

/* MCE control block */
typedef struct {
    uint8_t              sdp_active;  /* see BTA_MCE_SDP_ACT_* */
    BD_ADDR            remote_addr;
    tBTA_MCE_DM_CBACK  *p_dm_cback;
} tBTA_MCE_CB;

enum {
    BTA_MCE_SDP_ACT_NONE = 0,
    BTA_MCE_SDP_ACT_YES       /* waiting for SDP result */
};

/* MCE control block */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_MCE_CB bta_mce_cb;
#else
extern tBTA_MCE_CB *bta_mce_cb_ptr;
#define bta_mce_cb (*bta_mce_cb_ptr)
#endif

/* config struct */
extern tBTA_MCE_CFG *p_bta_mce_cfg;

extern uint8_t bta_mce_sm_execute(BT_HDR *p_msg);

extern void bta_mce_enable(tBTA_MCE_MSG *p_data);
extern void bta_mce_get_remote_mas_instances(tBTA_MCE_MSG *p_data);

#endif /* BTA_MCE_INT_H */
