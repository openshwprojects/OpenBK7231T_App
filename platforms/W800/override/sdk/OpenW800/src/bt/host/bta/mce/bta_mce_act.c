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
 *  This file contains action functions for MCE.
 *
 ******************************************************************************/
#include "bt_target.h"
#if defined(BTA_MCE_INCLUDED) && (BTA_MCE_INCLUDED == TRUE)
#include <hardware/bluetooth.h>
#include <arpa/inet.h>

#include "bt_types.h"
#include "bt_common.h"
#include "utl.h"
#include "bta_sys.h"
#include "bta_api.h"
#include "bta_mce_api.h"
#include "bta_mce_int.h"
#include "btm_api.h"
#include "btm_int.h"
#include "sdp_api.h"
#include <string.h>

/*****************************************************************************
**  Constants
*****************************************************************************/

static const tBT_UUID bta_mce_mas_uuid = {
    .len = 2,
    .uu.uuid16 = UUID_SERVCLASS_MESSAGE_ACCESS
};

/*******************************************************************************
**
** Function     bta_mce_search_cback
**
** Description  Callback from btm after search is completed
**
** Returns      void
**
*******************************************************************************/

static void bta_mce_search_cback(uint16_t result, void *user_data)
{
    tSDP_DISC_REC *p_rec = NULL;
    tBTA_MCE_MAS_DISCOVERY_COMP evt_data;
    int found = 0;
    APPL_TRACE_DEBUG("bta_mce_start_discovery_cback res: 0x%x", result);
    bta_mce_cb.sdp_active = BTA_MCE_SDP_ACT_NONE;

    if(bta_mce_cb.p_dm_cback == NULL) {
        return;
    }

    evt_data.status = BTA_MCE_FAILURE;
    bdcpy(evt_data.remote_addr, bta_mce_cb.remote_addr);
    evt_data.num_mas = 0;

    if(result == SDP_SUCCESS || result == SDP_DB_FULL) {
        do {
            tSDP_DISC_ATTR *p_attr;
            tSDP_PROTOCOL_ELEM pe;
            p_rec = SDP_FindServiceUUIDInDb(p_bta_mce_cfg->p_sdp_db, (tBT_UUID *) &bta_mce_mas_uuid, p_rec);
            APPL_TRACE_DEBUG("p_rec:%p", p_rec);

            if(p_rec == NULL) {
                break;
            }

            if(!SDP_FindProtocolListElemInRec(p_rec, UUID_PROTOCOL_RFCOMM, &pe)) {
                continue;
            }

            evt_data.mas[found].scn = pe.params[0];

            if((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_SERVICE_NAME)) == NULL) {
                continue;
            }

            evt_data.mas[found].p_srv_name = (char *) p_attr->attr_value.v.array;
            evt_data.mas[found].srv_name_len = SDP_DISC_ATTR_LEN(p_attr->attr_len_type);

            if((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_MAS_INSTANCE_ID)) == NULL) {
                break;
            }

            evt_data.mas[found].instance_id = p_attr->attr_value.v.u8;

            if((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_SUPPORTED_MSG_TYPE)) == NULL) {
                break;
            }

            evt_data.mas[found].msg_type = p_attr->attr_value.v.u8;
            found++;
        } while(p_rec != NULL && found < BTA_MCE_MAX_MAS_INSTANCES);

        evt_data.num_mas = found;
        evt_data.status = BTA_MCE_SUCCESS;
    }

    bta_mce_cb.p_dm_cback(BTA_MCE_MAS_DISCOVERY_COMP_EVT, (tBTA_MCE *) &evt_data, user_data);
}

/*******************************************************************************
**
** Function     bta_mce_enable
**
** Description  Initializes the MCE I/F
**
** Returns      void
**
*******************************************************************************/
void bta_mce_enable(tBTA_MCE_MSG *p_data)
{
    tBTA_MCE_STATUS status = BTA_MCE_SUCCESS;
    bta_mce_cb.p_dm_cback = p_data->enable.p_cback;
    bta_mce_cb.p_dm_cback(BTA_MCE_ENABLE_EVT, (tBTA_MCE *)&status, NULL);
}

/*******************************************************************************
**
** Function     bta_mce_get_remote_mas_instances
**
** Description  Discovers MAS instances on remote device
**
** Returns      void
**
*******************************************************************************/
void bta_mce_get_remote_mas_instances(tBTA_MCE_MSG *p_data)
{
    if(p_data == NULL) {
        APPL_TRACE_DEBUG("MCE control block handle is null");
        return;
    }

    tBTA_MCE_STATUS status = BTA_MCE_FAILURE;
    APPL_TRACE_DEBUG("%s in, sdp_active:%d", __FUNCTION__, bta_mce_cb.sdp_active);

    if(bta_mce_cb.sdp_active != BTA_MCE_SDP_ACT_NONE) {
        /* SDP is still in progress */
        status = BTA_MCE_BUSY;

        if(bta_mce_cb.p_dm_cback) {
            bta_mce_cb.p_dm_cback(BTA_MCE_MAS_DISCOVERY_COMP_EVT, (tBTA_MCE *)&status, NULL);
        }

        return;
    }

    bta_mce_cb.sdp_active = BTA_MCE_SDP_ACT_YES;
    bdcpy(bta_mce_cb.remote_addr, p_data->get_rmt_mas.bd_addr);
    SDP_InitDiscoveryDb(p_bta_mce_cfg->p_sdp_db, p_bta_mce_cfg->sdp_db_size, 1,
                        (tBT_UUID *) &bta_mce_mas_uuid, 0, NULL);

    if(!SDP_ServiceSearchAttributeRequest2(p_data->get_rmt_mas.bd_addr, p_bta_mce_cfg->p_sdp_db,
                                           bta_mce_search_cback, NULL)) {
        bta_mce_cb.sdp_active = BTA_MCE_SDP_ACT_NONE;

        /* failed to start SDP. report the failure right away */
        if(bta_mce_cb.p_dm_cback) {
            bta_mce_cb.p_dm_cback(BTA_MCE_MAS_DISCOVERY_COMP_EVT, (tBTA_MCE *)&status, NULL);
        }
    }

    /*
    else report the result when the cback is called
    */
}
#endif
