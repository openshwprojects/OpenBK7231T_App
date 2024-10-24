/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
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
 *  This is the implementation of the API for the HeaLth device profile (HL)
 *  subsystem of BTA, Broadcom Corp's Bluetooth application layer for mobile
 *  phones.
 *
 ******************************************************************************/

#include <string.h>

#include "bt_target.h"
#if defined(BTA_HL_INCLUDED) && (BTA_HL_INCLUDED == TRUE)

#include "bt_common.h"
#include "bta_hl_api.h"
#include "bta_hl_int.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

static const tBTA_SYS_REG bta_hl_reg = {
    bta_hl_hdl_event,
    BTA_HlDisable
};

/*******************************************************************************
**
** Function         BTA_HlEnable
**
** Description      Enable the HL subsystems.  This function must be
**                  called before any other functions in the HL API are called.
**                  When the enable operation is completed the callback function
**                  will be called with an BTA_HL_CTRL_ENABLE_CFM_EVT event.
**
** Parameters       p_cback - HL event call back function
**
** Returns          void
**
*******************************************************************************/
void BTA_HlEnable(tBTA_HL_CTRL_CBACK *p_ctrl_cback)
{
    tBTA_HL_API_ENABLE *p_buf =
                    (tBTA_HL_API_ENABLE *)GKI_getbuf(sizeof(tBTA_HL_API_ENABLE));
    /* register with BTA system manager */
    bta_sys_register(BTA_ID_HL, &bta_hl_reg);
    p_buf->hdr.event = BTA_HL_API_ENABLE_EVT;
    p_buf->p_cback = p_ctrl_cback;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HlDisable
**
** Description     Disable the HL subsystem.
**
** Returns          void
**
*******************************************************************************/
void BTA_HlDisable(void)
{
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    bta_sys_deregister(BTA_ID_HL);
    p_buf->event = BTA_HL_API_DISABLE_EVT;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HlUpdate
**
** Description      Register an HDP application
**
** Parameters       app_id        - Application ID
**                  p_reg_param   - non-platform related parameters for the
**                                  HDP application
**                  p_cback       - HL event callback fucntion
**
** Returns          void
**
*******************************************************************************/
void BTA_HlUpdate(uint8_t app_id, tBTA_HL_REG_PARAM *p_reg_param,
                  uint8_t is_register, tBTA_HL_CBACK *p_cback)
{
    tBTA_HL_API_UPDATE *p_buf =
                    (tBTA_HL_API_UPDATE *)GKI_getbuf(sizeof(tBTA_HL_API_UPDATE));
    APPL_TRACE_DEBUG("%s", __func__);
    p_buf->hdr.event = BTA_HL_API_UPDATE_EVT;
    p_buf->app_id = app_id;
    p_buf->is_register = is_register;

    if(is_register) {
        p_buf->sec_mask = (p_reg_param->sec_mask | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT);
        p_buf->p_cback = p_cback;

        if(p_reg_param->p_srv_name) {
            strlcpy(p_buf->srv_name, p_reg_param->p_srv_name, BTA_SERVICE_NAME_LEN);
        } else {
            p_buf->srv_name[0] = 0;
        }

        if(p_reg_param->p_srv_desp) {
            strlcpy(p_buf->srv_desp, p_reg_param->p_srv_desp, BTA_SERVICE_DESP_LEN);
        } else {
            p_buf->srv_desp[0] = 0;
        }

        if(p_reg_param->p_provider_name) {
            strlcpy(p_buf->provider_name, p_reg_param->p_provider_name, BTA_PROVIDER_NAME_LEN);
        } else {
            p_buf->provider_name[0] = 0;
        }
    }

    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HlRegister
**
** Description      Register an HDP application
**
** Parameters       app_id        - Application ID
**                  p_reg_param   - non-platform related parameters for the
**                                  HDP application
**                  p_cback       - HL event callback fucntion
**
** Returns          void
**
*******************************************************************************/
void BTA_HlRegister(uint8_t  app_id,
                    tBTA_HL_REG_PARAM *p_reg_param,
                    tBTA_HL_CBACK *p_cback)
{
    tBTA_HL_API_REGISTER *p_buf =
                    (tBTA_HL_API_REGISTER *)GKI_getbuf(sizeof(tBTA_HL_API_REGISTER));
    p_buf->hdr.event    = BTA_HL_API_REGISTER_EVT;
    p_buf->app_id       = app_id;
    p_buf->sec_mask     = (p_reg_param->sec_mask | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT);
    p_buf->p_cback = p_cback;

    if(p_reg_param->p_srv_name) {
        strlcpy(p_buf->srv_name, p_reg_param->p_srv_name, BTA_SERVICE_NAME_LEN);
    } else {
        p_buf->srv_name[0] = 0;
    }

    if(p_reg_param->p_srv_desp) {
        strlcpy(p_buf->srv_desp, p_reg_param->p_srv_desp, BTA_SERVICE_DESP_LEN);
    } else {
        p_buf->srv_desp[0] = 0;
    }

    if(p_reg_param->p_provider_name) {
        strlcpy(p_buf->provider_name, p_reg_param->p_provider_name, BTA_PROVIDER_NAME_LEN);
    } else {
        p_buf->provider_name[0] = 0;
    }

    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HlDeregister
**
** Description      Deregister an HDP application
**
** Parameters       app_handle - Application handle
**
** Returns           void
**
*******************************************************************************/
void BTA_HlDeregister(uint8_t app_id, tBTA_HL_APP_HANDLE app_handle)
{
    tBTA_HL_API_DEREGISTER *p_buf =
                    (tBTA_HL_API_DEREGISTER *)GKI_getbuf(sizeof(tBTA_HL_API_DEREGISTER));
    p_buf->hdr.event   = BTA_HL_API_DEREGISTER_EVT;
    p_buf->app_id      = app_id;
    p_buf->app_handle  = app_handle;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HlCchOpen
**
** Description      Open a Control channel connection with the specified BD address
**
** Parameters       app_handle - Application Handle
**                  p_open_param - parameters for opening a control channel
**
** Returns          void
**
**                  Note: The control PSM value is used to select which
**                  HDP insatnce should be used in case the peer device support
**                  multiple HDP instances. Also, if the control PSM value is zero
**                  then the first HDP instance is used for the control channel setup
*******************************************************************************/
void BTA_HlCchOpen(uint8_t app_id, tBTA_HL_APP_HANDLE app_handle,
                   tBTA_HL_CCH_OPEN_PARAM *p_open_param)
{
    tBTA_HL_API_CCH_OPEN *p_buf =
                    (tBTA_HL_API_CCH_OPEN *)GKI_getbuf(sizeof(tBTA_HL_API_CCH_OPEN));
    p_buf->hdr.event = BTA_HL_API_CCH_OPEN_EVT;
    p_buf->app_id = app_id;
    p_buf->app_handle = app_handle;
    p_buf->sec_mask = (p_open_param->sec_mask | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT);
    bdcpy(p_buf->bd_addr, p_open_param->bd_addr);
    p_buf->ctrl_psm = p_open_param->ctrl_psm;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HlCchClose
**
** Description      Close a Control channel connection with the specified MCL
**                  handle
**
** Parameters       mcl_handle - MCL handle
**
** Returns          void
**
*******************************************************************************/
void BTA_HlCchClose(tBTA_HL_MCL_HANDLE mcl_handle)
{
    tBTA_HL_API_CCH_CLOSE *p_buf =
                    (tBTA_HL_API_CCH_CLOSE *)GKI_getbuf(sizeof(tBTA_HL_API_CCH_CLOSE));
    p_buf->hdr.event = BTA_HL_API_CCH_CLOSE_EVT;
    p_buf->mcl_handle = mcl_handle;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HlDchOpen
**
** Description      Open a data channel connection with the specified DCH parameters
**
** Parameters       mcl_handle - MCL handle
**                  p_open_param - parameters for opening a data channel
**
** Returns          void
**
*******************************************************************************/
void BTA_HlDchOpen(tBTA_HL_MCL_HANDLE mcl_handle,
                   tBTA_HL_DCH_OPEN_PARAM *p_open_param)
{
    tBTA_HL_API_DCH_OPEN *p_buf =
                    (tBTA_HL_API_DCH_OPEN *)GKI_getbuf(sizeof(tBTA_HL_API_DCH_OPEN));
    p_buf->hdr.event = BTA_HL_API_DCH_OPEN_EVT;
    p_buf->mcl_handle = mcl_handle;
    p_buf->ctrl_psm = p_open_param->ctrl_psm;
    p_buf->local_mdep_id = p_open_param->local_mdep_id;
    p_buf->peer_mdep_id = p_open_param->peer_mdep_id;
    p_buf->local_cfg = p_open_param->local_cfg;
    p_buf->sec_mask = (p_open_param->sec_mask | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT);
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HlDchReconnect
**
** Description      Reconnect a data channel with the specified MDL_ID
**
** Parameters       mcl_handle      - MCL handle
*8                  p_recon_param   - parameters for reconnecting a data channel
**
** Returns          void
**
*******************************************************************************/
void BTA_HlDchReconnect(tBTA_HL_MCL_HANDLE mcl_handle,
                        tBTA_HL_DCH_RECONNECT_PARAM *p_recon_param)
{
    tBTA_HL_API_DCH_RECONNECT *p_buf =
                    (tBTA_HL_API_DCH_RECONNECT *)GKI_getbuf(sizeof(tBTA_HL_API_DCH_RECONNECT));
    p_buf->hdr.event = BTA_HL_API_DCH_RECONNECT_EVT;
    p_buf->mcl_handle = mcl_handle;
    p_buf->ctrl_psm = p_recon_param->ctrl_psm;
    p_buf->mdl_id = p_recon_param->mdl_id;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HlDchClose
**
** Description      Close a data channel with the specified MDL handle
**
** Parameters       mdl_handle  - MDL handle
**
** Returns          void
**
*******************************************************************************/
void BTA_HlDchClose(tBTA_HL_MDL_HANDLE mdl_handle)
{
    tBTA_HL_API_DCH_CLOSE *p_buf =
                    (tBTA_HL_API_DCH_CLOSE *)GKI_getbuf(sizeof(tBTA_HL_API_DCH_CLOSE));
    p_buf->hdr.event = BTA_HL_API_DCH_CLOSE_EVT;
    p_buf->mdl_handle = mdl_handle;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HlDchAbort
**
** Description      Abort the current data channel setup with the specified MCL
**                  handle
**
** Parameters       mcl_handle  - MCL handle
**
**
** Returns          void
**
*******************************************************************************/
void BTA_HlDchAbort(tBTA_HL_MCL_HANDLE mcl_handle)
{
    tBTA_HL_API_DCH_ABORT *p_buf =
                    (tBTA_HL_API_DCH_ABORT *)GKI_getbuf(sizeof(tBTA_HL_API_DCH_ABORT));
    p_buf->hdr.event = BTA_HL_API_DCH_ABORT_EVT;
    p_buf->mcl_handle = mcl_handle;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HlSendData
**
** Description      Send an APDU to the peer device
**
** Parameters       mdl_handle  - MDL handle
**                  pkt_size    - size of the data packet to be sent
**
** Returns          void
**
*******************************************************************************/
void BTA_HlSendData(tBTA_HL_MDL_HANDLE mdl_handle,
                    uint16_t           pkt_size)
{
    tBTA_HL_API_SEND_DATA *p_buf =
                    (tBTA_HL_API_SEND_DATA *)GKI_getbuf(sizeof(tBTA_HL_API_SEND_DATA));
    p_buf->hdr.event = BTA_HL_API_SEND_DATA_EVT;
    p_buf->mdl_handle = mdl_handle;
    p_buf->pkt_size = pkt_size;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HlDeleteMdl
**
** Description      Delete the specified MDL_ID within the specified MCL handle
**
** Parameters       mcl_handle  - MCL handle
**                  mdl_id      - MDL ID
**
** Returns          void
**
**                  note: If mdl_id = 0xFFFF then this means to delete all MDLs
**                        and this value can only be used with DeleteMdl request only
**                        not other requests
**
*******************************************************************************/
void BTA_HlDeleteMdl(tBTA_HL_MCL_HANDLE mcl_handle,
                     tBTA_HL_MDL_ID mdl_id)
{
    tBTA_HL_API_DELETE_MDL *p_buf =
                    (tBTA_HL_API_DELETE_MDL *)GKI_getbuf(sizeof(tBTA_HL_API_DELETE_MDL));
    p_buf->hdr.event = BTA_HL_API_DELETE_MDL_EVT;
    p_buf->mcl_handle = mcl_handle;
    p_buf->mdl_id = mdl_id;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HlDchEchoTest
**
** Description      Initiate an echo test with the specified MCL handle
**
** Parameters       mcl_handle           - MCL handle
*8                  p_echo_test_param   -  parameters for echo testing
**
** Returns          void
**
*******************************************************************************/
void BTA_HlDchEchoTest(tBTA_HL_MCL_HANDLE  mcl_handle,
                       tBTA_HL_DCH_ECHO_TEST_PARAM *p_echo_test_param)
{
    tBTA_HL_API_DCH_ECHO_TEST *p_buf =
                    (tBTA_HL_API_DCH_ECHO_TEST *)GKI_getbuf(sizeof(tBTA_HL_API_DCH_ECHO_TEST));
    p_buf->hdr.event = BTA_HL_API_DCH_ECHO_TEST_EVT;
    p_buf->mcl_handle = mcl_handle;
    p_buf->ctrl_psm = p_echo_test_param->ctrl_psm;
    p_buf->local_cfg = p_echo_test_param->local_cfg;
    p_buf->pkt_size = p_echo_test_param->pkt_size;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HlSdpQuery
**
** Description      SDP query request for the specified BD address
**
** Parameters       app_handle      - application handle
**                  bd_addr         - BD address
**
** Returns          void
**
*******************************************************************************/
void BTA_HlSdpQuery(uint8_t  app_id, tBTA_HL_APP_HANDLE app_handle,
                    BD_ADDR bd_addr)
{
    tBTA_HL_API_SDP_QUERY *p_buf =
                    (tBTA_HL_API_SDP_QUERY *)GKI_getbuf(sizeof(tBTA_HL_API_SDP_QUERY));
    p_buf->hdr.event = BTA_HL_API_SDP_QUERY_EVT;
    p_buf->app_id = app_id;
    p_buf->app_handle = app_handle;
    bdcpy(p_buf->bd_addr, bd_addr);
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HlDchCreateMdlRsp
**
** Description      Set the Response and configuration values for the Create MDL
**                  request
**
** Parameters       mcl_handle  - MCL handle
**                  p_rsp_param - parameters specified whether the request should
**                                be accepted or not and if it should be accepted
**                                then it also specified the configuration response
**                                value
**
** Returns          void
**
*******************************************************************************/
void BTA_HlDchCreateRsp(tBTA_HL_MCL_HANDLE mcl_handle,
                        tBTA_HL_DCH_CREATE_RSP_PARAM *p_rsp_param)
{
    tBTA_HL_API_DCH_CREATE_RSP *p_buf =
                    (tBTA_HL_API_DCH_CREATE_RSP *)GKI_getbuf(sizeof(tBTA_HL_API_DCH_CREATE_RSP));
    p_buf->hdr.event = BTA_HL_API_DCH_CREATE_RSP_EVT;
    p_buf->mcl_handle = mcl_handle;
    p_buf->mdl_id = p_rsp_param->mdl_id;
    p_buf->local_mdep_id = p_rsp_param->local_mdep_id;
    p_buf->rsp_code = p_rsp_param->rsp_code;
    p_buf->cfg_rsp = p_rsp_param->cfg_rsp;
    bta_sys_sendmsg(p_buf);
}

#endif /* HL_INCLUDED */
