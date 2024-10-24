/******************************************************************************
 *
 *  Copyright (C) 2004-2012 Broadcom Corporation
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
 *  This is the implementation of the API for PAN subsystem of BTA,
 *  Broadcom's Bluetooth application layer for mobile phones.
 *
 ******************************************************************************/

#include <string.h>
#include "bdroid_buildcfg.h"
#include "bta_pan_api.h"
#include "bt_utils.h"
#if defined(BTA_PAN_INCLUDED) && BTA_PAN_INCLUDED == TRUE
#include "bta_api.h"
#include "bta_sys.h"
#include "pan_api.h"
#include "bt_common.h"
#include "bta_pan_int.h"

static const tBTA_SYS_REG bta_pan_reg = {
    bta_pan_hdl_event,
    BTA_PanDisable
};

/*******************************************************************************
**
** Function         BTA_PanEnable
**
** Description      Enable PAN service.  This function must be
**                  called before any other functions in the PAN API are called.
**                  When the enable operation is complete the callback function
**                  will be called with a BTA_PAN_ENABLE_EVT.
**
** Returns          void
**
*******************************************************************************/
void BTA_PanEnable(tBTA_PAN_CBACK p_cback)
{
    tBTA_PAN_API_ENABLE *p_buf =
                    (tBTA_PAN_API_ENABLE *)GKI_getbuf(sizeof(tBTA_PAN_API_ENABLE));
    /* register with BTA system manager */
    bta_sys_register(BTA_ID_PAN, &bta_pan_reg);
    p_buf->hdr.event = BTA_PAN_API_ENABLE_EVT;
    p_buf->p_cback = p_cback;
    bta_sys_sendmsg(p_buf);
}



/*******************************************************************************
**
** Function         BTA_PanDisable
**
** Description      Disables PAN service.
**
**
** Returns          void
**
*******************************************************************************/
void BTA_PanDisable(void)
{
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    bta_sys_deregister(BTA_ID_PAN);
    p_buf->event = BTA_PAN_API_DISABLE_EVT;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_PanSetRole
**
** Description      Sets PAN roles. When the enable operation is complete
**                  the callback function will be called with a BTA_PAN_SET_ROLE_EVT.
**
** Returns          void
**
*******************************************************************************/
void BTA_PanSetRole(tBTA_PAN_ROLE role, tBTA_PAN_ROLE_INFO *p_user_info,
                    tBTA_PAN_ROLE_INFO *p_gn_info,
                    tBTA_PAN_ROLE_INFO *p_nap_info)
{
    tBTA_PAN_API_SET_ROLE  *p_buf =
                    (tBTA_PAN_API_SET_ROLE *)GKI_getbuf(sizeof(tBTA_PAN_API_SET_ROLE));
    p_buf->hdr.event = BTA_PAN_API_SET_ROLE_EVT;
    p_buf->role = role;

    if(p_user_info && (role & BTA_PAN_ROLE_PANU)) {
        if(p_user_info->p_srv_name) {
            strlcpy(p_buf->user_name, p_user_info->p_srv_name, BTA_SERVICE_NAME_LEN);
        } else {
            p_buf->user_name[0] = 0;
        }

        p_buf->user_app_id = p_user_info->app_id;
        p_buf->user_sec_mask = p_user_info->sec_mask;
    }

    if(p_gn_info && (role & BTA_PAN_ROLE_GN)) {
        if(p_gn_info->p_srv_name) {
            strlcpy(p_buf->gn_name, p_gn_info->p_srv_name, BTA_SERVICE_NAME_LEN);
        } else {
            p_buf->gn_name[0] = 0;
        }

        p_buf->gn_app_id = p_gn_info->app_id;
        p_buf->gn_sec_mask = p_gn_info->sec_mask;
    }

    if(p_nap_info && (role & BTA_PAN_ROLE_NAP)) {
        if(p_nap_info->p_srv_name) {
            strlcpy(p_buf->nap_name, p_nap_info->p_srv_name, BTA_SERVICE_NAME_LEN);
        } else {
            p_buf->nap_name[0] = 0;
        }

        p_buf->nap_app_id = p_nap_info->app_id;
        p_buf->nap_sec_mask = p_nap_info->sec_mask;
    }

    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_PanOpen
**
** Description      Opens a connection to a peer device.
**                  When connection is open callback function is called
**                  with a BTA_PAN_OPEN_EVT.
**
**
** Returns          void
**
*******************************************************************************/
void BTA_PanOpen(BD_ADDR bd_addr, tBTA_PAN_ROLE    local_role, tBTA_PAN_ROLE    peer_role)
{
    tBTA_PAN_API_OPEN *p_buf =
                    (tBTA_PAN_API_OPEN *)GKI_getbuf(sizeof(tBTA_PAN_API_OPEN));
    p_buf->hdr.event = BTA_PAN_API_OPEN_EVT;
    p_buf->local_role = local_role;
    p_buf->peer_role = peer_role;
    bdcpy(p_buf->bd_addr, bd_addr);
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_PanClose
**
** Description      Close a PAN  connection to a peer device.
**
**
** Returns          void
**
*******************************************************************************/
void BTA_PanClose(uint16_t handle)
{
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    p_buf->event = BTA_PAN_API_CLOSE_EVT;
    p_buf->layer_specific = handle;
    bta_sys_sendmsg(p_buf);
}
#else

void BTA_PanEnable(tBTA_PAN_CBACK p_cback)
{
    UNUSED(p_cback);
}

void BTA_PanDisable(void)
{
}

void BTA_PanSetRole(tBTA_PAN_ROLE role, tBTA_PAN_ROLE_INFO *p_user_info,
                    tBTA_PAN_ROLE_INFO *p_gn_info,
                    tBTA_PAN_ROLE_INFO *p_nap_info)
{
    UNUSED(role);
    UNUSED(p_user_info);
    UNUSED(p_gn_info);
    UNUSED(p_nap_info);
}

void BTA_PanOpen(BD_ADDR bd_addr, tBTA_PAN_ROLE local_role, tBTA_PAN_ROLE peer_role)
{
    UNUSED(bd_addr);
    UNUSED(local_role);
    UNUSED(peer_role);
}

void BTA_PanClose(uint16_t handle)
{
    UNUSED(handle);
}

#endif /* BTA_PAN_INCLUDED */
