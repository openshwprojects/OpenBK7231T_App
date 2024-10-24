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
 *  This file contains compile-time configurable constants for the audio
 *  gateway.
 *
 ******************************************************************************/
#include <string.h>
#include "bt_target.h"
#if defined(BTA_HFP_HSP_AG_INCLUDED) && (BTA_HFP_HSP_AG_INCLUDED == TRUE)
#include "bt_common.h"
#include "bta_api.h"
#include "bta_ag_api.h"

#ifndef BTA_AG_CIND_INFO
#define BTA_AG_CIND_INFO     "(\"call\",(0,1)),(\"callsetup\",(0-3)),(\"service\",(0-3)),(\"signal\",(0-6)),(\"roam\",(0,1)),(\"battchg\",(0-5)),(\"callheld\",(0-2)),(\"bearer\",(0-7))"
#endif

#ifndef BTA_AG_CHLD_VAL_ECC
#define BTA_AG_CHLD_VAL_ECC  "(0,1,1x,2,2x,3,4)"
#endif

#ifndef BTA_AG_CHLD_VAL
#define BTA_AG_CHLD_VAL  "(0,1,2,3,4)"
#endif

#ifndef BTA_AG_CONN_TIMEOUT
#define BTA_AG_CONN_TIMEOUT     5000
#endif

#ifndef BTA_AG_SCO_PKT_TYPES
/* S1 packet type setting from HFP 1.5 spec */
#define BTA_AG_SCO_PKT_TYPES    /* BTM_SCO_LINK_ALL_PKT_MASK */ (BTM_SCO_LINK_ONLY_MASK          | \
        BTM_SCO_PKT_TYPES_MASK_EV3      | \
        BTM_SCO_PKT_TYPES_MASK_NO_3_EV3 | \
        BTM_SCO_PKT_TYPES_MASK_NO_2_EV5 | \
        BTM_SCO_PKT_TYPES_MASK_NO_3_EV5)
#endif

const tBTA_AG_CFG bta_ag_cfg = {
    BTA_AG_CIND_INFO,
    BTA_AG_CONN_TIMEOUT,
    BTA_AG_SCO_PKT_TYPES,
    BTA_AG_CHLD_VAL_ECC,
    BTA_AG_CHLD_VAL
};

tBTA_AG_CFG *p_bta_ag_cfg = (tBTA_AG_CFG *) &bta_ag_cfg;
#endif
