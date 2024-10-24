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
 *  This is the API implementation file for the Multi-Channel Adaptation
 *  Protocol (MCAP).
 *
 ******************************************************************************/
#include <assert.h>
#include <string.h>

#include "bt_target.h"
#if defined(MCA_INCLUDED) &&(MCA_INCLUDED == TRUE)
#include "btm_api.h"
#include "btm_int.h"
#include "mca_api.h"
#include "mca_defs.h"
#include "mca_int.h"

#include "btu.h"


/*******************************************************************************
**
** Function         mca_process_timeout
**
** Description      This function is called by BTU when an MCA timer
**                  expires.
**
**                  This function is for use internal to the stack only.
**
** Returns          void
**
*******************************************************************************/
void mca_ccb_timer_timeout(void *data)
{
    tMCA_CCB *p_ccb = (tMCA_CCB *)data;
    mca_ccb_event(p_ccb, MCA_CCB_RSP_TOUT_EVT, NULL);
}

/*******************************************************************************
**
** Function         MCA_Init
**
** Description      Initialize MCAP main control block.
**                  This function is called at stack start up.
**
** Returns          void
**
*******************************************************************************/
void MCA_Init(void)
{
    wm_memset(&mca_cb, 0, sizeof(tMCA_CB));
#if defined(MCA_INITIAL_TRACE_LEVEL)
    mca_cb.trace_level = MCA_INITIAL_TRACE_LEVEL;
#else
    mca_cb.trace_level = BT_TRACE_LEVEL_NONE;
#endif
}

/*******************************************************************************
**
** Function         MCA_SetTraceLevel
**
** Description      This function sets the debug trace level for MCA.
**                  If 0xff is passed, the current trace level is returned.
**
**                  Input Parameters:
**                      level:  The level to set the MCA tracing to:
**                      0xff-returns the current setting.
**                      0-turns off tracing.
**                      >= 1-Errors.
**                      >= 2-Warnings.
**                      >= 3-APIs.
**                      >= 4-Events.
**                      >= 5-Debug.
**
** Returns          The new trace level or current trace level if
**                  the input parameter is 0xff.
**
*******************************************************************************/
uint8_t MCA_SetTraceLevel(uint8_t level)
{
    if(level != 0xFF) {
        mca_cb.trace_level = level;
    }

    return (mca_cb.trace_level);
}

/*******************************************************************************
**
** Function         MCA_Register
**
** Description      This function registers an MCAP implementation.
**                  It is assumed that the control channel PSM and data channel
**                  PSM are not used by any other instances of the stack.
**                  If the given p_reg->ctrl_psm is 0, this handle is INT only.
**
** Returns          0, if failed. Otherwise, the MCA handle.
**
*******************************************************************************/
tMCA_HANDLE MCA_Register(tMCA_REG *p_reg, tMCA_CTRL_CBACK *p_cback)
{
    tMCA_RCB    *p_rcb;
    tMCA_HANDLE handle = 0;
    tL2CAP_APPL_INFO l2c_cacp_appl;
    tL2CAP_APPL_INFO l2c_dacp_appl;
    assert(p_reg != NULL);
    assert(p_cback != NULL);
    MCA_TRACE_API("MCA_Register: ctrl_psm:0x%x, data_psm:0x%x", p_reg->ctrl_psm, p_reg->data_psm);

    if((p_rcb = mca_rcb_alloc(p_reg)) != NULL) {
        if(p_reg->ctrl_psm) {
            if(L2C_INVALID_PSM(p_reg->ctrl_psm) || L2C_INVALID_PSM(p_reg->data_psm)) {
                MCA_TRACE_ERROR("INVALID_PSM");
                return 0;
            }

            l2c_cacp_appl = *(tL2CAP_APPL_INFO *)&mca_l2c_int_appl;
            l2c_cacp_appl.pL2CA_ConnectCfm_Cb = NULL;
            l2c_dacp_appl = *(tL2CAP_APPL_INFO *)&l2c_cacp_appl;
            l2c_cacp_appl.pL2CA_ConnectInd_Cb = mca_l2c_cconn_ind_cback;
            l2c_dacp_appl.pL2CA_ConnectInd_Cb = mca_l2c_dconn_ind_cback;

            if(L2CA_Register(p_reg->ctrl_psm, (tL2CAP_APPL_INFO *) &l2c_cacp_appl) &&
                    L2CA_Register(p_reg->data_psm, (tL2CAP_APPL_INFO *) &l2c_dacp_appl)) {
                /* set security level */
                BTM_SetSecurityLevel(FALSE, "", BTM_SEC_SERVICE_MCAP_CTRL, p_reg->sec_mask,
                                     p_reg->ctrl_psm, BTM_SEC_PROTO_MCA, MCA_CTRL_TCID);
                /* in theory, we do not need this one for data_psm
                 * If we don't, L2CAP rejects with security block (3),
                 * which is different reject code from what MCAP spec suggests.
                 * we set this one, so mca_l2c_dconn_ind_cback can reject /w no resources (4) */
                BTM_SetSecurityLevel(FALSE, "", BTM_SEC_SERVICE_MCAP_DATA, p_reg->sec_mask,
                                     p_reg->data_psm, BTM_SEC_PROTO_MCA, MCA_CTRL_TCID);
            } else {
                MCA_TRACE_ERROR("Failed to register to L2CAP");
                return 0;
            }
        } else {
            p_rcb->reg.data_psm = 0;
        }

        handle = mca_rcb_to_handle(p_rcb);
        p_rcb->p_cback = p_cback;
        p_rcb->reg.rsp_tout = p_reg->rsp_tout;
    }

    return handle;
}


/*******************************************************************************
**
** Function         MCA_Deregister
**
** Description      This function is called to deregister an MCAP implementation.
**                  Before this function can be called, all control and data
**                  channels must be removed with MCA_DisconnectReq and MCA_CloseReq.
**
** Returns          void
**
*******************************************************************************/
void MCA_Deregister(tMCA_HANDLE handle)
{
    tMCA_RCB *p_rcb = mca_rcb_by_handle(handle);
    MCA_TRACE_API("MCA_Deregister: %d", handle);

    if(p_rcb && p_rcb->reg.ctrl_psm) {
        L2CA_Deregister(p_rcb->reg.ctrl_psm);
        L2CA_Deregister(p_rcb->reg.data_psm);
        btm_sec_clr_service_by_psm(p_rcb->reg.ctrl_psm);
        btm_sec_clr_service_by_psm(p_rcb->reg.data_psm);
    }

    mca_rcb_dealloc(handle);
}


/*******************************************************************************
**
** Function         MCA_CreateDep
**
** Description      Create a data endpoint.  If the MDEP is created successfully,
**                  the MDEP ID is returned in *p_dep. After a data endpoint is
**                  created, an application can initiate a connection between this
**                  endpoint and an endpoint on a peer device.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
tMCA_RESULT MCA_CreateDep(tMCA_HANDLE handle, tMCA_DEP *p_dep, tMCA_CS *p_cs)
{
    tMCA_RESULT result = MCA_BAD_HANDLE;
    int       i;
    tMCA_RCB *p_rcb = mca_rcb_by_handle(handle);
    tMCA_CS  *p_depcs;
    assert(p_dep != NULL);
    assert(p_cs != NULL);
    assert(p_cs->p_data_cback != NULL);
    MCA_TRACE_API("MCA_CreateDep: %d", handle);

    if(p_rcb) {
        if(p_cs->max_mdl > MCA_NUM_MDLS) {
            MCA_TRACE_ERROR("max_mdl: %d is too big", p_cs->max_mdl);
            result = MCA_BAD_PARAMS;
        } else {
            p_depcs = p_rcb->dep;

            if(p_cs->type == MCA_TDEP_ECHO) {
                if(p_depcs->p_data_cback) {
                    MCA_TRACE_ERROR("Already has ECHO MDEP");
                    return MCA_NO_RESOURCES;
                }

                wm_memcpy(p_depcs, p_cs, sizeof(tMCA_CS));
                *p_dep = 0;
                result = MCA_SUCCESS;
            } else {
                result = MCA_NO_RESOURCES;
                /* non-echo MDEP starts from 1 */
                p_depcs++;

                for(i = 1; i < MCA_NUM_DEPS; i++, p_depcs++) {
                    if(p_depcs->p_data_cback == NULL) {
                        wm_memcpy(p_depcs, p_cs, sizeof(tMCA_CS));
                        /* internally use type as the mdep id */
                        p_depcs->type = i;
                        *p_dep = i;
                        result = MCA_SUCCESS;
                        break;
                    }
                }
            }
        }
    }

    return result;
}


/*******************************************************************************
**
** Function         MCA_DeleteDep
**
** Description      Delete a data endpoint.  This function is called when
**                  the implementation is no longer using a data endpoint.
**                  If this function is called when the endpoint is connected
**                  the connection is closed and the data endpoint
**                  is removed.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
tMCA_RESULT MCA_DeleteDep(tMCA_HANDLE handle, tMCA_DEP dep)
{
    tMCA_RESULT result = MCA_BAD_HANDLE;
    tMCA_RCB *p_rcb = mca_rcb_by_handle(handle);
    tMCA_DCB *p_dcb;
    int      i, max;
    tMCA_CS  *p_depcs;
    MCA_TRACE_API("MCA_DeleteDep: %d dep:%d", handle, dep);

    if(p_rcb) {
        if(dep < MCA_NUM_DEPS && p_rcb->dep[dep].p_data_cback) {
            result = MCA_SUCCESS;
            p_rcb->dep[dep].p_data_cback = NULL;
            p_depcs = &(p_rcb->dep[dep]);
            i = handle - 1;
            max = MCA_NUM_MDLS * MCA_NUM_LINKS;
            p_dcb = &mca_cb.dcb[i * max];

            /* make sure no MDL exists for this MDEP */
            for(i = 0; i < max; i++, p_dcb++) {
                if(p_dcb->state && p_dcb->p_cs == p_depcs) {
                    mca_dcb_event(p_dcb, MCA_DCB_API_CLOSE_EVT, NULL);
                }
            }
        }
    }

    return result;
}

/*******************************************************************************
**
** Function         MCA_ConnectReq
**
** Description      This function initiates an MCAP control channel connection
**                  to the peer device.  When the connection is completed, an
**                  MCA_CONNECT_IND_EVT is reported to the application via its
**                  control callback function.
**                  This control channel is identified by the tMCA_CL.
**                  If the connection attempt fails, an MCA_DISCONNECT_IND_EVT is
**                  reported. The security mask parameter overrides the outgoing
**                  security mask set in MCA_Register().
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
tMCA_RESULT MCA_ConnectReq(tMCA_HANDLE handle, BD_ADDR bd_addr,
                           uint16_t ctrl_psm, uint16_t sec_mask)
{
    tMCA_RESULT result = MCA_BAD_HANDLE;
    tMCA_CCB    *p_ccb;
    tMCA_TC_TBL *p_tbl;
    MCA_TRACE_API("MCA_ConnectReq: %d psm:0x%x", handle, ctrl_psm);

    if((p_ccb = mca_ccb_by_bd(handle, bd_addr)) == NULL) {
        p_ccb = mca_ccb_alloc(handle, bd_addr);
    } else {
        MCA_TRACE_ERROR("control channel already exists");
        return MCA_BUSY;
    }

    if(p_ccb) {
        p_ccb->ctrl_vpsm = L2CA_Register(ctrl_psm, (tL2CAP_APPL_INFO *)&mca_l2c_int_appl);
        result = MCA_NO_RESOURCES;

        if(p_ccb->ctrl_vpsm) {
            BTM_SetSecurityLevel(TRUE, "", BTM_SEC_SERVICE_MCAP_CTRL, sec_mask,
                                 p_ccb->ctrl_vpsm, BTM_SEC_PROTO_MCA, MCA_CTRL_TCID);
            p_ccb->lcid = mca_l2c_open_req(bd_addr, p_ccb->ctrl_vpsm, NULL);

            if(p_ccb->lcid) {
                p_tbl = mca_tc_tbl_calloc(p_ccb);

                if(p_tbl) {
                    p_tbl->state = MCA_TC_ST_CONN;
                    p_ccb->sec_mask = sec_mask;
                    result = MCA_SUCCESS;
                }
            }
        }

        if(result != MCA_SUCCESS) {
            mca_ccb_dealloc(p_ccb, NULL);
        }
    }

    return result;
}


/*******************************************************************************
**
** Function         MCA_DisconnectReq
**
** Description      This function disconnect an MCAP control channel
**                  to the peer device.
**                  If associated data channel exists, they are disconnected.
**                  When the MCL is disconnected an MCA_DISCONNECT_IND_EVT is
**                  reported to the application via its control callback function.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
tMCA_RESULT MCA_DisconnectReq(tMCA_CL mcl)
{
    tMCA_RESULT result = MCA_BAD_HANDLE;
    tMCA_CCB *p_ccb = mca_ccb_by_hdl(mcl);
    MCA_TRACE_API("MCA_DisconnectReq: %d ", mcl);

    if(p_ccb) {
        result = MCA_SUCCESS;
        mca_ccb_event(p_ccb, MCA_CCB_API_DISCONNECT_EVT, NULL);
    }

    return result;
}


/*******************************************************************************
**
** Function         MCA_CreateMdl
**
** Description      This function sends a CREATE_MDL request to the peer device.
**                  When the response is received, a MCA_CREATE_CFM_EVT is reported
**                  with the given MDL ID.
**                  If the response is successful, a data channel is open
**                  with the given p_chnl_cfg
**                  If p_chnl_cfg is NULL, the data channel is not initiated until
**                  MCA_DataChnlCfg is called to provide the p_chnl_cfg.
**                  When the data channel is open successfully, a MCA_OPEN_CFM_EVT
**                  is reported. This data channel is identified as tMCA_DL.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
tMCA_RESULT MCA_CreateMdl(tMCA_CL mcl, tMCA_DEP dep, uint16_t data_psm,
                          uint16_t mdl_id, uint8_t peer_dep_id,
                          uint8_t cfg, const tMCA_CHNL_CFG *p_chnl_cfg)
{
    tMCA_RESULT     result = MCA_BAD_HANDLE;
    tMCA_CCB        *p_ccb = mca_ccb_by_hdl(mcl);
    tMCA_DCB        *p_dcb;
    MCA_TRACE_API("MCA_CreateMdl: %d dep=%d mdl_id=%d peer_dep_id=%d", mcl, dep, mdl_id, peer_dep_id);

    if(p_ccb) {
        if(p_ccb->p_tx_req || p_ccb->p_rx_msg || p_ccb->cong) {
            MCA_TRACE_ERROR("pending req");
            return MCA_BUSY;
        }

        if((peer_dep_id > MCA_MAX_MDEP_ID) || (!MCA_IS_VALID_MDL_ID(mdl_id))) {
            MCA_TRACE_ERROR("bad peer dep id:%d or bad mdl id: %d ", peer_dep_id, mdl_id);
            return MCA_BAD_PARAMS;
        }

        if(mca_ccb_uses_mdl_id(p_ccb, mdl_id)) {
            MCA_TRACE_ERROR("mdl id: %d is used in the control link", mdl_id);
            return MCA_BAD_MDL_ID;
        }

        p_dcb = mca_dcb_alloc(p_ccb, dep);
        result = MCA_NO_RESOURCES;

        if(p_dcb) {
            /* save the info required by dcb connection */
            p_dcb->p_chnl_cfg       = p_chnl_cfg;
            p_dcb->mdl_id           = mdl_id;
            tMCA_CCB_MSG *p_evt_data =
                            (tMCA_CCB_MSG *)GKI_getbuf(sizeof(tMCA_CCB_MSG));

            if(!p_ccb->data_vpsm) {
                p_ccb->data_vpsm = L2CA_Register(data_psm, (tL2CAP_APPL_INFO *)&mca_l2c_int_appl);
            }

            if(p_ccb->data_vpsm) {
                p_evt_data->dcb_idx     = mca_dcb_to_hdl(p_dcb);
                p_evt_data->mdep_id     = peer_dep_id;
                p_evt_data->mdl_id      = mdl_id;
                p_evt_data->param       = cfg;
                p_evt_data->op_code     = MCA_OP_MDL_CREATE_REQ;
                p_evt_data->hdr.event   = MCA_CCB_API_REQ_EVT;
                p_evt_data->hdr.layer_specific   = FALSE;
                mca_ccb_event(p_ccb, MCA_CCB_API_REQ_EVT, (tMCA_CCB_EVT *)p_evt_data);
                return MCA_SUCCESS;
            } else {
                GKI_freebuf(p_evt_data);
            }

            mca_dcb_dealloc(p_dcb, NULL);
        }
    }

    return result;
}


/*******************************************************************************
**
** Function         MCA_CreateMdlRsp
**
** Description      This function sends a CREATE_MDL response to the peer device
**                  in response to a received MCA_CREATE_IND_EVT.
**                  If the rsp_code is successful, a data channel is open
**                  with the given p_chnl_cfg
**                  When the data channel is open successfully, a MCA_OPEN_IND_EVT
**                  is reported. This data channel is identified as tMCA_DL.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
tMCA_RESULT MCA_CreateMdlRsp(tMCA_CL mcl, tMCA_DEP dep,
                             uint16_t mdl_id, uint8_t cfg, uint8_t rsp_code,
                             const tMCA_CHNL_CFG *p_chnl_cfg)
{
    tMCA_RESULT     result = MCA_BAD_HANDLE;
    tMCA_CCB        *p_ccb = mca_ccb_by_hdl(mcl);
    tMCA_CCB_MSG    evt_data;
    tMCA_DCB        *p_dcb;
    MCA_TRACE_API("MCA_CreateMdlRsp: %d dep=%d mdl_id=%d cfg=%d rsp_code=%d", mcl, dep, mdl_id, cfg,
                  rsp_code);
    assert(p_chnl_cfg != NULL);

    if(p_ccb) {
        if(p_ccb->cong) {
            MCA_TRACE_ERROR("congested");
            return MCA_BUSY;
        }

        if(p_ccb->p_rx_msg && (p_ccb->p_rx_msg->mdep_id == dep)
                && (p_ccb->p_rx_msg->mdl_id == mdl_id) && (p_ccb->p_rx_msg->op_code == MCA_OP_MDL_CREATE_REQ)) {
            result = MCA_SUCCESS;
            evt_data.dcb_idx    = 0;

            if(rsp_code == MCA_RSP_SUCCESS) {
                p_dcb = mca_dcb_alloc(p_ccb, dep);

                if(p_dcb) {
                    evt_data.dcb_idx    = mca_dcb_to_hdl(p_dcb);
                    p_dcb->p_chnl_cfg   = p_chnl_cfg;
                    p_dcb->mdl_id       = mdl_id;
                } else {
                    rsp_code = MCA_RSP_MDEP_BUSY;
                    result = MCA_NO_RESOURCES;
                }
            }

            if(result == MCA_SUCCESS) {
                evt_data.mdl_id     = mdl_id;
                evt_data.param      = cfg;
                evt_data.rsp_code   = rsp_code;
                evt_data.op_code    = MCA_OP_MDL_CREATE_RSP;
                mca_ccb_event(p_ccb, MCA_CCB_API_RSP_EVT, (tMCA_CCB_EVT *)&evt_data);
            }
        } else {
            MCA_TRACE_ERROR("The given MCL is not expecting a MCA_CreateMdlRsp with the given parameters");
            result = MCA_BAD_PARAMS;
        }
    }

    return result;
}

/*******************************************************************************
**
** Function         MCA_CloseReq
**
** Description      Close a data channel.  When the channel is closed, an
**                  MCA_CLOSE_CFM_EVT is sent to the application via the
**                  control callback function for this handle.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
tMCA_RESULT MCA_CloseReq(tMCA_DL mdl)
{
    tMCA_RESULT     result = MCA_BAD_HANDLE;
    tMCA_DCB *p_dcb = mca_dcb_by_hdl(mdl);
    MCA_TRACE_API("MCA_CloseReq: %d ", mdl);

    if(p_dcb) {
        result = MCA_SUCCESS;
        mca_dcb_event(p_dcb, MCA_DCB_API_CLOSE_EVT, NULL);
    }

    return result;
}


/*******************************************************************************
**
** Function         MCA_ReconnectMdl
**
** Description      This function sends a RECONNECT_MDL request to the peer device.
**                  When the response is received, a MCA_RECONNECT_CFM_EVT is reported.
**                  If p_chnl_cfg is NULL, the data channel is not initiated until
**                  MCA_DataChnlCfg is called to provide the p_chnl_cfg.
**                  If the response is successful, a data channel is open.
**                  When the data channel is open successfully, a MCA_OPEN_CFM_EVT
**                  is reported.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
tMCA_RESULT MCA_ReconnectMdl(tMCA_CL mcl, tMCA_DEP dep, uint16_t data_psm,
                             uint16_t mdl_id, const tMCA_CHNL_CFG *p_chnl_cfg)
{
    tMCA_RESULT     result = MCA_BAD_HANDLE;
    tMCA_CCB        *p_ccb = mca_ccb_by_hdl(mcl);
    tMCA_DCB        *p_dcb;
    MCA_TRACE_API("MCA_ReconnectMdl: %d ", mcl);
    assert(p_chnl_cfg != NULL);

    if(p_ccb) {
        if(p_ccb->p_tx_req || p_ccb->p_rx_msg || p_ccb->cong) {
            MCA_TRACE_ERROR("pending req");
            return MCA_BUSY;
        }

        if(!MCA_IS_VALID_MDL_ID(mdl_id)) {
            MCA_TRACE_ERROR("bad mdl id: %d ", mdl_id);
            return MCA_BAD_PARAMS;
        }

        if(mca_ccb_uses_mdl_id(p_ccb, mdl_id)) {
            MCA_TRACE_ERROR("mdl id: %d is used in the control link", mdl_id);
            return MCA_BAD_MDL_ID;
        }

        p_dcb = mca_dcb_alloc(p_ccb, dep);
        result = MCA_NO_RESOURCES;

        if(p_dcb) {
            tMCA_CCB_MSG *p_evt_data =
                            (tMCA_CCB_MSG *)GKI_getbuf(sizeof(tMCA_CCB_MSG));
            p_dcb->p_chnl_cfg       = p_chnl_cfg;
            p_dcb->mdl_id           = mdl_id;

            if(!p_ccb->data_vpsm) {
                p_ccb->data_vpsm = L2CA_Register(data_psm, (tL2CAP_APPL_INFO *)&mca_l2c_int_appl);
            }

            p_evt_data->dcb_idx     = mca_dcb_to_hdl(p_dcb);
            p_evt_data->mdl_id      = mdl_id;
            p_evt_data->op_code     = MCA_OP_MDL_RECONNECT_REQ;
            p_evt_data->hdr.event   = MCA_CCB_API_REQ_EVT;
            mca_ccb_event(p_ccb, MCA_CCB_API_REQ_EVT, (tMCA_CCB_EVT *)p_evt_data);
            return MCA_SUCCESS;
        }
    }

    return result;
}


/*******************************************************************************
**
** Function         MCA_ReconnectMdlRsp
**
** Description      This function sends a RECONNECT_MDL response to the peer device
**                  in response to a MCA_RECONNECT_IND_EVT event.
**                  If the response is successful, a data channel is open.
**                  When the data channel is open successfully, a MCA_OPEN_IND_EVT
**                  is reported.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
tMCA_RESULT MCA_ReconnectMdlRsp(tMCA_CL mcl, tMCA_DEP dep,
                                uint16_t mdl_id, uint8_t rsp_code,
                                const tMCA_CHNL_CFG *p_chnl_cfg)
{
    tMCA_RESULT     result = MCA_BAD_HANDLE;
    tMCA_CCB        *p_ccb = mca_ccb_by_hdl(mcl);
    tMCA_CCB_MSG    evt_data;
    tMCA_DCB        *p_dcb;
    MCA_TRACE_API("MCA_ReconnectMdlRsp: %d ", mcl);
    assert(p_chnl_cfg != NULL);

    if(p_ccb) {
        if(p_ccb->cong) {
            MCA_TRACE_ERROR("congested");
            return MCA_BUSY;
        }

        if(p_ccb->p_rx_msg && (p_ccb->p_rx_msg->mdl_id == mdl_id) &&
                (p_ccb->p_rx_msg->op_code == MCA_OP_MDL_RECONNECT_REQ)) {
            result = MCA_SUCCESS;
            evt_data.dcb_idx    = 0;

            if(rsp_code == MCA_RSP_SUCCESS) {
                p_dcb = mca_dcb_alloc(p_ccb, dep);

                if(p_dcb) {
                    evt_data.dcb_idx    = mca_dcb_to_hdl(p_dcb);
                    p_dcb->p_chnl_cfg   = p_chnl_cfg;
                    p_dcb->mdl_id       = mdl_id;
                } else {
                    MCA_TRACE_ERROR("Out of MDL for this MDEP");
                    rsp_code = MCA_RSP_MDEP_BUSY;
                    result = MCA_NO_RESOURCES;
                }
            }

            evt_data.mdl_id     = mdl_id;
            evt_data.rsp_code   = rsp_code;
            evt_data.op_code    = MCA_OP_MDL_RECONNECT_RSP;
            mca_ccb_event(p_ccb, MCA_CCB_API_RSP_EVT, (tMCA_CCB_EVT *)&evt_data);
        } else {
            MCA_TRACE_ERROR("The given MCL is not expecting a MCA_ReconnectMdlRsp with the given parameters");
            result = MCA_BAD_PARAMS;
        }
    }

    return result;
}


/*******************************************************************************
**
** Function         MCA_DataChnlCfg
**
** Description      This function initiates a data channel connection toward the
**                  connected peer device.
**                  When the data channel is open successfully, a MCA_OPEN_CFM_EVT
**                  is reported. This data channel is identified as tMCA_DL.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
tMCA_RESULT MCA_DataChnlCfg(tMCA_CL mcl, const tMCA_CHNL_CFG *p_chnl_cfg)
{
    tMCA_RESULT     result = MCA_BAD_HANDLE;
    tMCA_CCB        *p_ccb = mca_ccb_by_hdl(mcl);
    tMCA_DCB        *p_dcb;
    tMCA_TC_TBL *p_tbl;
    MCA_TRACE_API("MCA_DataChnlCfg: %d ", mcl);
    assert(p_chnl_cfg != NULL);

    if(p_ccb) {
        result = MCA_NO_RESOURCES;

        if((p_ccb->p_tx_req == NULL) || (p_ccb->status != MCA_CCB_STAT_PENDING) ||
                ((p_dcb = mca_dcb_by_hdl(p_ccb->p_tx_req->dcb_idx)) == NULL)) {
            MCA_TRACE_ERROR("The given MCL is not expecting this API:%d", p_ccb->status);
            return result;
        }

        p_dcb->p_chnl_cfg       = p_chnl_cfg;
        BTM_SetSecurityLevel(TRUE, "", BTM_SEC_SERVICE_MCAP_DATA, p_ccb->sec_mask,
                             p_ccb->data_vpsm, BTM_SEC_PROTO_MCA, p_ccb->p_tx_req->dcb_idx);
        p_dcb->lcid = mca_l2c_open_req(p_ccb->peer_addr, p_ccb->data_vpsm, p_dcb->p_chnl_cfg);

        if(p_dcb->lcid) {
            p_tbl = mca_tc_tbl_dalloc(p_dcb);

            if(p_tbl) {
                p_tbl->state = MCA_TC_ST_CONN;
                result = MCA_SUCCESS;
            }
        }
    }

    return result;
}


/*******************************************************************************
**
** Function         MCA_Abort
**
** Description      This function sends a ABORT_MDL request to the peer device.
**                  When the response is received, a MCA_ABORT_CFM_EVT is reported.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
tMCA_RESULT MCA_Abort(tMCA_CL mcl)
{
    tMCA_RESULT     result = MCA_BAD_HANDLE;
    tMCA_CCB        *p_ccb = mca_ccb_by_hdl(mcl);
    tMCA_DCB        *p_dcb;
    MCA_TRACE_API("MCA_Abort: %d", mcl);

    if(p_ccb) {
        result = MCA_NO_RESOURCES;

        /* verify that we are waiting for data channel to come up with the given mdl */
        if((p_ccb->p_tx_req == NULL) || (p_ccb->status != MCA_CCB_STAT_PENDING) ||
                ((p_dcb = mca_dcb_by_hdl(p_ccb->p_tx_req->dcb_idx)) == NULL)) {
            MCA_TRACE_ERROR("The given MCL is not expecting this API:%d", p_ccb->status);
            return result;
        }

        if(p_ccb->cong) {
            MCA_TRACE_ERROR("congested");
            return MCA_BUSY;
        }

        tMCA_CCB_MSG *p_evt_data =
                        (tMCA_CCB_MSG *)GKI_getbuf(sizeof(tMCA_CCB_MSG));
        result = MCA_SUCCESS;
        p_evt_data->op_code     = MCA_OP_MDL_ABORT_REQ;
        p_evt_data->hdr.event   = MCA_CCB_API_REQ_EVT;
        mca_ccb_event(p_ccb, MCA_CCB_API_REQ_EVT, (tMCA_CCB_EVT *)p_evt_data);
    }

    return result;
}


/*******************************************************************************
**
** Function         MCA_Delete
**
** Description      This function sends a DELETE_MDL request to the peer device.
**                  When the response is received, a MCA_DELETE_CFM_EVT is reported.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
tMCA_RESULT MCA_Delete(tMCA_CL mcl, uint16_t mdl_id)
{
    tMCA_RESULT     result = MCA_BAD_HANDLE;
    tMCA_CCB        *p_ccb = mca_ccb_by_hdl(mcl);
    MCA_TRACE_API("MCA_Delete: %d ", mcl);

    if(p_ccb) {
        if(p_ccb->cong) {
            MCA_TRACE_ERROR("congested");
            return MCA_BUSY;
        }

        if(!MCA_IS_VALID_MDL_ID(mdl_id) && (mdl_id != MCA_ALL_MDL_ID)) {
            MCA_TRACE_ERROR("bad mdl id: %d ", mdl_id);
            return MCA_BAD_PARAMS;
        }

        tMCA_CCB_MSG *p_evt_data = (tMCA_CCB_MSG *)GKI_getbuf(sizeof(tMCA_CCB_MSG));
        result = MCA_SUCCESS;
        p_evt_data->mdl_id      = mdl_id;
        p_evt_data->op_code     = MCA_OP_MDL_DELETE_REQ;
        p_evt_data->hdr.event   = MCA_CCB_API_REQ_EVT;
        mca_ccb_event(p_ccb, MCA_CCB_API_REQ_EVT, (tMCA_CCB_EVT *)p_evt_data);
    }

    return result;
}

/*******************************************************************************
**
** Function         MCA_WriteReq
**
** Description      Send a data packet to the peer device.
**
**                  The application passes the packet using the BT_HDR structure.
**                  The offset field must be equal to or greater than L2CAP_MIN_OFFSET.
**                  This allows enough space in the buffer for the L2CAP header.
**
**                  The memory pointed to by p_pkt must be a GKI buffer
**                  allocated by the application.  This buffer will be freed
**                  by the protocol stack; the application must not free
**                  this buffer.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
tMCA_RESULT MCA_WriteReq(tMCA_DL mdl, BT_HDR *p_pkt)
{
    tMCA_RESULT     result = MCA_BAD_HANDLE;
    tMCA_DCB *p_dcb = mca_dcb_by_hdl(mdl);
    tMCA_DCB_EVT    evt_data;
    MCA_TRACE_API("MCA_WriteReq: %d ", mdl);

    if(p_dcb) {
        if(p_dcb->cong) {
            result = MCA_BUSY;
        } else {
            evt_data.p_pkt  = p_pkt;
            result = MCA_SUCCESS;
            mca_dcb_event(p_dcb, MCA_DCB_API_WRITE_EVT, &evt_data);
        }
    }

    return result;
}

/*******************************************************************************
**
** Function         MCA_GetL2CapChannel
**
** Description      Get the L2CAP CID used by the given data channel handle.
**
** Returns          L2CAP channel ID if successful, otherwise 0.
**
*******************************************************************************/
uint16_t MCA_GetL2CapChannel(tMCA_DL mdl)
{
    uint16_t  lcid = 0;
    tMCA_DCB *p_dcb = mca_dcb_by_hdl(mdl);
    MCA_TRACE_API("MCA_GetL2CapChannel: %d ", mdl);

    if(p_dcb) {
        lcid = p_dcb->lcid;
    }

    return lcid;
}
#endif
