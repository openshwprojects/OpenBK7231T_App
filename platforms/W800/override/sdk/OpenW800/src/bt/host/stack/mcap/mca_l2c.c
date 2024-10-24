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
 *  This is the implementation file for the MCAP at L2CAP Interface.
 *
 ******************************************************************************/
#include <string.h>

#include "bt_target.h"
#if defined(MCA_INCLUDED) &&(MCA_INCLUDED == TRUE)
#include "bt_utils.h"
#include "btm_api.h"
#include "btm_int.h"
#include "mca_api.h"
#include "mca_defs.h"
#include "mca_int.h"


/* L2CAP callback function structure */
const tL2CAP_APPL_INFO mca_l2c_int_appl = {
    NULL,
    mca_l2c_connect_cfm_cback,
    NULL,
    mca_l2c_config_ind_cback,
    mca_l2c_config_cfm_cback,
    mca_l2c_disconnect_ind_cback,
    mca_l2c_disconnect_cfm_cback,
    NULL,
    mca_l2c_data_ind_cback,
    mca_l2c_congestion_ind_cback,
    NULL
};

/* Control channel eL2CAP default options */
const tL2CAP_FCR_OPTS mca_l2c_fcr_opts_def = {
    L2CAP_FCR_ERTM_MODE,            /* Mandatory for MCAP */
    MCA_FCR_OPT_TX_WINDOW_SIZE,     /* Tx window size */
    MCA_FCR_OPT_MAX_TX_B4_DISCNT,   /* Maximum transmissions before disconnecting */
    MCA_FCR_OPT_RETX_TOUT,          /* Retransmission timeout (2 secs) */
    MCA_FCR_OPT_MONITOR_TOUT,       /* Monitor timeout (12 secs) */
    MCA_FCR_OPT_MPS_SIZE            /* MPS segment size */
};


/*******************************************************************************
**
** Function         mca_sec_check_complete_term
**
** Description      The function called when Security Manager finishes
**                  verification of the service side connection
**
** Returns          void
**
*******************************************************************************/
static void mca_sec_check_complete_term(BD_ADDR bd_addr, tBT_TRANSPORT transport, void *p_ref_data,
                                        uint8_t res)
{
    tMCA_TC_TBL     *p_tbl = (tMCA_TC_TBL *)p_ref_data;
    tL2CAP_CFG_INFO cfg;
    tL2CAP_ERTM_INFO ertm_info;
    UNUSED(transport);
    MCA_TRACE_DEBUG("mca_sec_check_complete_term res: %d", res);

    if(res == BTM_SUCCESS) {
        MCA_TRACE_DEBUG("lcid:x%x id:x%x", p_tbl->lcid, p_tbl->id);
        /* Set the FCR options: control channel mandates ERTM */
        ertm_info.preferred_mode    = mca_l2c_fcr_opts_def.mode;
        ertm_info.allowed_modes     = L2CAP_FCR_CHAN_OPT_ERTM;
        ertm_info.user_rx_buf_size  = MCA_USER_RX_BUF_SIZE;
        ertm_info.user_tx_buf_size  = MCA_USER_TX_BUF_SIZE;
        ertm_info.fcr_rx_buf_size   = MCA_FCR_RX_BUF_SIZE;
        ertm_info.fcr_tx_buf_size   = MCA_FCR_TX_BUF_SIZE;
        /* Send response to the L2CAP layer. */
        L2CA_ErtmConnectRsp(bd_addr, p_tbl->id, p_tbl->lcid, L2CAP_CONN_OK, L2CAP_CONN_OK, &ertm_info);
        /* transition to configuration state */
        p_tbl->state = MCA_TC_ST_CFG;
        /* Send L2CAP config req */
        mca_set_cfg_by_tbl(&cfg, p_tbl);
        L2CA_ConfigReq(p_tbl->lcid, &cfg);
    } else {
        L2CA_ConnectRsp(bd_addr, p_tbl->id, p_tbl->lcid, L2CAP_CONN_SECURITY_BLOCK, L2CAP_CONN_OK);
        mca_tc_close_ind(p_tbl, L2CAP_CONN_SECURITY_BLOCK);
    }
}

/*******************************************************************************
**
** Function         mca_sec_check_complete_orig
**
** Description      The function called when Security Manager finishes
**                  verification of the service side connection
**
** Returns          void
**
*******************************************************************************/
static void mca_sec_check_complete_orig(BD_ADDR bd_addr, tBT_TRANSPORT transport, void *p_ref_data,
                                        uint8_t res)
{
    tMCA_TC_TBL     *p_tbl = (tMCA_TC_TBL *)p_ref_data;
    tL2CAP_CFG_INFO cfg;
    UNUSED(bd_addr);
    UNUSED(transport);
    MCA_TRACE_DEBUG("mca_sec_check_complete_orig res: %d", res);

    if(res == BTM_SUCCESS) {
        /* set channel state */
        p_tbl->state = MCA_TC_ST_CFG;
        /* Send L2CAP config req */
        mca_set_cfg_by_tbl(&cfg, p_tbl);
        L2CA_ConfigReq(p_tbl->lcid, &cfg);
    } else {
        L2CA_DisconnectReq(p_tbl->lcid);
        mca_tc_close_ind(p_tbl, L2CAP_CONN_SECURITY_BLOCK);
    }
}
/*******************************************************************************
**
** Function         mca_l2c_cconn_ind_cback
**
** Description      This is the L2CAP connect indication callback function.
**
** Returns          void
**
*******************************************************************************/
void mca_l2c_cconn_ind_cback(BD_ADDR bd_addr, uint16_t lcid, uint16_t psm, uint8_t id)
{
    tMCA_HANDLE handle = mca_handle_by_cpsm(psm);
    tMCA_CCB    *p_ccb;
    tMCA_TC_TBL *p_tbl = NULL;
    uint16_t      result = L2CAP_CONN_NO_RESOURCES;
    tBTM_STATUS rc;
    tL2CAP_ERTM_INFO ertm_info, *p_ertm_info = NULL;
    tL2CAP_CFG_INFO  cfg;
    MCA_TRACE_EVENT("mca_l2c_cconn_ind_cback: lcid:x%x psm:x%x id:x%x", lcid, psm, id);

    /* do we already have a control channel for this peer? */
    if((p_ccb = mca_ccb_by_bd(handle, bd_addr)) == NULL) {
        /* no, allocate ccb */
        if((p_ccb = mca_ccb_alloc(handle, bd_addr)) != NULL) {
            /* allocate and set up entry */
            p_ccb->lcid     = lcid;
            p_tbl           = mca_tc_tbl_calloc(p_ccb);
            p_tbl->id       = id;
            p_tbl->cfg_flags = MCA_L2C_CFG_CONN_ACP;
            /* proceed with connection */
            /* Check the security */
            rc = btm_sec_mx_access_request(bd_addr, psm, FALSE, BTM_SEC_PROTO_MCA, 0,
                                           &mca_sec_check_complete_term, p_tbl);

            if(rc == BTM_CMD_STARTED) {
                /* Set the FCR options: control channel mandates ERTM */
                ertm_info.preferred_mode    = mca_l2c_fcr_opts_def.mode;
                ertm_info.allowed_modes     = L2CAP_FCR_CHAN_OPT_ERTM;
                ertm_info.user_rx_buf_size  = MCA_USER_RX_BUF_SIZE;
                ertm_info.user_tx_buf_size  = MCA_USER_TX_BUF_SIZE;
                ertm_info.fcr_rx_buf_size   = MCA_FCR_RX_BUF_SIZE;
                ertm_info.fcr_tx_buf_size   = MCA_FCR_TX_BUF_SIZE;
                p_ertm_info = &ertm_info;
                result = L2CAP_CONN_PENDING;
            } else {
                result = L2CAP_CONN_OK;
            }
        }

        /*  deal with simultaneous control channel connect case */
    }

    /* else reject their connection */

    if(!p_tbl || (p_tbl->state != MCA_TC_ST_CFG)) {
        /* Send L2CAP connect rsp */
        L2CA_ErtmConnectRsp(bd_addr, id, lcid, result, L2CAP_CONN_OK, p_ertm_info);

        /* if result ok, proceed with connection and send L2CAP
           config req */
        if(result == L2CAP_CONN_OK) {
            /* set channel state */
            p_tbl->state = MCA_TC_ST_CFG;
            /* Send L2CAP config req */
            mca_set_cfg_by_tbl(&cfg, p_tbl);
            L2CA_ConfigReq(p_tbl->lcid, &cfg);
        }
    }
}

/*******************************************************************************
**
** Function         mca_l2c_dconn_ind_cback
**
** Description      This is the L2CAP connect indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
void mca_l2c_dconn_ind_cback(BD_ADDR bd_addr, uint16_t lcid, uint16_t psm, uint8_t id)
{
    tMCA_HANDLE handle = mca_handle_by_dpsm(psm);
    tMCA_CCB    *p_ccb;
    tMCA_DCB       *p_dcb;
    tMCA_TC_TBL    *p_tbl = NULL;
    uint16_t          result;
    tL2CAP_CFG_INFO cfg;
    tL2CAP_ERTM_INFO *p_ertm_info = NULL, ertm_info;
    const tMCA_CHNL_CFG   *p_chnl_cfg;
    MCA_TRACE_EVENT("mca_l2c_dconn_ind_cback: lcid:x%x psm:x%x ", lcid, psm);

    if(((p_ccb = mca_ccb_by_bd(handle, bd_addr)) != NULL) &&        /* find the CCB */
            (p_ccb->status == MCA_CCB_STAT_PENDING) &&   /* this CCB is expecting a MDL */
            (p_ccb->p_tx_req && (p_dcb = mca_dcb_by_hdl(p_ccb->p_tx_req->dcb_idx)) != NULL)) {
        /* found the associated dcb in listening mode */
        /* proceed with connection */
        p_dcb->lcid     = lcid;
        p_tbl           = mca_tc_tbl_dalloc(p_dcb);
        p_tbl->id       = id;
        p_tbl->cfg_flags = MCA_L2C_CFG_CONN_ACP;
        p_chnl_cfg = p_dcb->p_chnl_cfg;
        /* assume that control channel has verified the security requirement */
        /* Set the FCR options: control channel mandates ERTM */
        ertm_info.preferred_mode    = p_chnl_cfg->fcr_opt.mode;
        ertm_info.allowed_modes     = (1 << p_chnl_cfg->fcr_opt.mode);
        ertm_info.user_rx_buf_size  = p_chnl_cfg->user_rx_buf_size;
        ertm_info.user_tx_buf_size  = p_chnl_cfg->user_tx_buf_size;
        ertm_info.fcr_rx_buf_size   = p_chnl_cfg->fcr_rx_buf_size;
        ertm_info.fcr_tx_buf_size   = p_chnl_cfg->fcr_tx_buf_size;
        p_ertm_info = &ertm_info;
        result = L2CAP_CONN_OK;
    } else {
        /* else we're not listening for traffic channel; reject
         * (this error code is specified by MCAP spec) */
        result = L2CAP_CONN_NO_RESOURCES;
    }

    /* Send L2CAP connect rsp */
    L2CA_ErtmConnectRsp(bd_addr, id, lcid, result, result, p_ertm_info);

    /* if result ok, proceed with connection */
    if(result == L2CAP_CONN_OK) {
        /* transition to configuration state */
        p_tbl->state = MCA_TC_ST_CFG;
        /* Send L2CAP config req */
        mca_set_cfg_by_tbl(&cfg, p_tbl);
        L2CA_ConfigReq(lcid, &cfg);
    }
}

/*******************************************************************************
**
** Function         mca_l2c_connect_cfm_cback
**
** Description      This is the L2CAP connect confirm callback function.
**
**
** Returns          void
**
*******************************************************************************/
void mca_l2c_connect_cfm_cback(uint16_t lcid, uint16_t result)
{
    tMCA_TC_TBL    *p_tbl;
    tL2CAP_CFG_INFO cfg;
    tMCA_CCB *p_ccb;
    MCA_TRACE_DEBUG("mca_l2c_connect_cfm_cback lcid: x%x, result: %d",
                    lcid, result);

    /* look up info for this channel */
    if((p_tbl = mca_tc_tbl_by_lcid(lcid)) != NULL) {
        MCA_TRACE_DEBUG("p_tbl state: %d, tcid: %d", p_tbl->state, p_tbl->tcid);

        /* if in correct state */
        if(p_tbl->state == MCA_TC_ST_CONN) {
            /* if result successful */
            if(result == L2CAP_CONN_OK) {
                if(p_tbl->tcid != 0) {
                    /* set channel state */
                    p_tbl->state = MCA_TC_ST_CFG;
                    /* Send L2CAP config req */
                    mca_set_cfg_by_tbl(&cfg, p_tbl);
                    L2CA_ConfigReq(lcid, &cfg);
                } else {
                    p_ccb = mca_ccb_by_hdl((tMCA_CL)p_tbl->cb_idx);

                    if(p_ccb == NULL) {
                        result = L2CAP_CONN_NO_RESOURCES;
                    } else {
                        /* set channel state */
                        p_tbl->state    = MCA_TC_ST_SEC_INT;
                        p_tbl->lcid     = lcid;
                        p_tbl->cfg_flags = MCA_L2C_CFG_CONN_INT;
                        /* Check the security */
                        btm_sec_mx_access_request(p_ccb->peer_addr, p_ccb->ctrl_vpsm,
                                                  TRUE, BTM_SEC_PROTO_MCA,
                                                  p_tbl->tcid,
                                                  &mca_sec_check_complete_orig, p_tbl);
                    }
                }
            }

            /* failure; notify adaption that channel closed */
            if(result != L2CAP_CONN_OK) {
                p_tbl->cfg_flags |= MCA_L2C_CFG_DISCN_INT;
                mca_tc_close_ind(p_tbl, result);
            }
        }
    }
}

/*******************************************************************************
**
** Function         mca_l2c_config_cfm_cback
**
** Description      This is the L2CAP config confirm callback function.
**
**
** Returns          void
**
*******************************************************************************/
void mca_l2c_config_cfm_cback(uint16_t lcid, tL2CAP_CFG_INFO *p_cfg)
{
    tMCA_TC_TBL    *p_tbl;

    /* look up info for this channel */
    if((p_tbl = mca_tc_tbl_by_lcid(lcid)) != NULL) {
        /* if in correct state */
        if(p_tbl->state == MCA_TC_ST_CFG) {
            /* if result successful */
            if(p_cfg->result == L2CAP_CONN_OK) {
                /* update cfg_flags */
                p_tbl->cfg_flags |= MCA_L2C_CFG_CFM_DONE;

                /* if configuration complete */
                if(p_tbl->cfg_flags & MCA_L2C_CFG_IND_DONE) {
                    mca_tc_open_ind(p_tbl);
                }
            }
            /* else failure */
            else {
                /* Send L2CAP disconnect req */
                L2CA_DisconnectReq(lcid);
            }
        }
    }
}

/*******************************************************************************
**
** Function         mca_l2c_config_ind_cback
**
** Description      This is the L2CAP config indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
void mca_l2c_config_ind_cback(uint16_t lcid, tL2CAP_CFG_INFO *p_cfg)
{
    tMCA_TC_TBL    *p_tbl;
    uint16_t          result = L2CAP_CFG_OK;

    /* look up info for this channel */
    if((p_tbl = mca_tc_tbl_by_lcid(lcid)) != NULL) {
        /* store the mtu in tbl */
        if(p_cfg->mtu_present) {
            p_tbl->peer_mtu = p_cfg->mtu;

            if(p_tbl->peer_mtu < MCA_MIN_MTU) {
                result = L2CAP_CFG_UNACCEPTABLE_PARAMS;
            }
        } else {
            p_tbl->peer_mtu = L2CAP_DEFAULT_MTU;
        }

        MCA_TRACE_DEBUG("peer_mtu: %d, lcid: x%x mtu_present:%d", p_tbl->peer_mtu, lcid,
                        p_cfg->mtu_present);
        /* send L2CAP configure response */
        wm_memset(p_cfg, 0, sizeof(tL2CAP_CFG_INFO));
        p_cfg->result = result;
        L2CA_ConfigRsp(lcid, p_cfg);

        /* if first config ind */
        if((p_tbl->cfg_flags & MCA_L2C_CFG_IND_DONE) == 0) {
            /* update cfg_flags */
            p_tbl->cfg_flags |= MCA_L2C_CFG_IND_DONE;

            /* if configuration complete */
            if(p_tbl->cfg_flags & MCA_L2C_CFG_CFM_DONE) {
                mca_tc_open_ind(p_tbl);
            }
        }
    }
}

/*******************************************************************************
**
** Function         mca_l2c_disconnect_ind_cback
**
** Description      This is the L2CAP disconnect indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
void mca_l2c_disconnect_ind_cback(uint16_t lcid, uint8_t ack_needed)
{
    tMCA_TC_TBL    *p_tbl;
    uint16_t         reason = L2CAP_DISC_TIMEOUT;
    MCA_TRACE_DEBUG("mca_l2c_disconnect_ind_cback lcid: %d, ack_needed: %d",
                    lcid, ack_needed);

    /* look up info for this channel */
    if((p_tbl = mca_tc_tbl_by_lcid(lcid)) != NULL) {
        if(ack_needed) {
            /* send L2CAP disconnect response */
            L2CA_DisconnectRsp(lcid);
        }

        p_tbl->cfg_flags = MCA_L2C_CFG_DISCN_ACP;

        if(ack_needed) {
            reason = L2CAP_DISC_OK;
        }

        mca_tc_close_ind(p_tbl, reason);
    }
}

/*******************************************************************************
**
** Function         mca_l2c_disconnect_cfm_cback
**
** Description      This is the L2CAP disconnect confirm callback function.
**
**
** Returns          void
**
*******************************************************************************/
void mca_l2c_disconnect_cfm_cback(uint16_t lcid, uint16_t result)
{
    tMCA_TC_TBL    *p_tbl;
    MCA_TRACE_DEBUG("mca_l2c_disconnect_cfm_cback lcid: x%x, result: %d",
                    lcid, result);

    /* look up info for this channel */
    if((p_tbl = mca_tc_tbl_by_lcid(lcid)) != NULL) {
        p_tbl->cfg_flags = MCA_L2C_CFG_DISCN_INT;
        mca_tc_close_ind(p_tbl, result);
    }
}


/*******************************************************************************
**
** Function         mca_l2c_congestion_ind_cback
**
** Description      This is the L2CAP congestion indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
void mca_l2c_congestion_ind_cback(uint16_t lcid, uint8_t is_congested)
{
    tMCA_TC_TBL    *p_tbl;

    /* look up info for this channel */
    if((p_tbl = mca_tc_tbl_by_lcid(lcid)) != NULL) {
        mca_tc_cong_ind(p_tbl, is_congested);
    }
}

/*******************************************************************************
**
** Function         mca_l2c_data_ind_cback
**
** Description      This is the L2CAP data indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
void mca_l2c_data_ind_cback(uint16_t lcid, BT_HDR *p_buf)
{
    tMCA_TC_TBL    *p_tbl;

    /* look up info for this channel */
    if((p_tbl = mca_tc_tbl_by_lcid(lcid)) != NULL) {
        mca_tc_data_ind(p_tbl, p_buf);
    } else { /* prevent buffer leak */
        GKI_freebuf(p_buf);
    }
}


/*******************************************************************************
**
** Function         mca_l2c_open_req
**
** Description      This function calls L2CA_ConnectReq() to initiate a L2CAP channel.
**
** Returns          void.
**
*******************************************************************************/
uint16_t mca_l2c_open_req(BD_ADDR bd_addr, uint16_t psm, const tMCA_CHNL_CFG *p_chnl_cfg)
{
    tL2CAP_ERTM_INFO ertm_info;

    if(p_chnl_cfg) {
        ertm_info.preferred_mode    = p_chnl_cfg->fcr_opt.mode;
        ertm_info.allowed_modes     = (1 << p_chnl_cfg->fcr_opt.mode);
        ertm_info.user_rx_buf_size  = p_chnl_cfg->user_rx_buf_size;
        ertm_info.user_tx_buf_size  = p_chnl_cfg->user_tx_buf_size;
        ertm_info.fcr_rx_buf_size   = p_chnl_cfg->fcr_rx_buf_size;
        ertm_info.fcr_tx_buf_size   = p_chnl_cfg->fcr_tx_buf_size;
    } else {
        ertm_info.preferred_mode    = mca_l2c_fcr_opts_def.mode;
        ertm_info.allowed_modes     = L2CAP_FCR_CHAN_OPT_ERTM;
        ertm_info.user_rx_buf_size  = MCA_USER_RX_BUF_SIZE;
        ertm_info.user_tx_buf_size  = MCA_USER_TX_BUF_SIZE;
        ertm_info.fcr_rx_buf_size   = MCA_FCR_RX_BUF_SIZE;
        ertm_info.fcr_tx_buf_size   = MCA_FCR_TX_BUF_SIZE;
    }

    return L2CA_ErtmConnectReq(psm, bd_addr, &ertm_info);
}
#endif
