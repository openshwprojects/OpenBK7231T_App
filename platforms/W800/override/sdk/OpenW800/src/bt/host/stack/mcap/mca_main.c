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
 *  This is the implementation file for the MCAP Main Control Block and
 *  Utility functions.
 *
 ******************************************************************************/
#include <assert.h>
#include <string.h>

#include "bt_target.h"
#if defined(MCA_INCLUDED) &&(MCA_INCLUDED == TRUE)
#include "bt_common.h"
#include "mca_api.h"
#include "mca_defs.h"
#include "mca_int.h"
#include "l2c_api.h"

/* Main Control block for MCA */
#if MCA_DYNAMIC_MEMORY == FALSE
tMCA_CB mca_cb;
#endif

/*****************************************************************************
** constants
*****************************************************************************/

/* table of standard opcode message size */
const uint8_t mca_std_msg_len[MCA_NUM_STANDARD_OPCODE] = {
    4,          /* MCA_OP_ERROR_RSP         */
    5,          /* MCA_OP_MDL_CREATE_REQ    */
    5,          /* MCA_OP_MDL_CREATE_RSP    */
    3,          /* MCA_OP_MDL_RECONNECT_REQ */
    4,          /* MCA_OP_MDL_RECONNECT_RSP */
    3,          /* MCA_OP_MDL_ABORT_REQ     */
    4,          /* MCA_OP_MDL_ABORT_RSP     */
    3,          /* MCA_OP_MDL_DELETE_REQ    */
    4           /* MCA_OP_MDL_DELETE_RSP    */
};


/*******************************************************************************
**
** Function         mca_handle_by_cpsm
**
** Description      This function returns the handle for the given control
**                  channel PSM. 0, if not found.
**
** Returns          the MCA handle.
**
*******************************************************************************/
tMCA_HANDLE mca_handle_by_cpsm(uint16_t psm)
{
    int     i;
    tMCA_HANDLE handle = 0;
    tMCA_RCB *p_rcb = &mca_cb.rcb[0];

    for(i = 0; i < MCA_NUM_REGS; i++, p_rcb++) {
        if(p_rcb->p_cback && p_rcb->reg.ctrl_psm == psm) {
            handle = i + 1;
            break;
        }
    }

    return handle;
}

/*******************************************************************************
**
** Function         mca_handle_by_dpsm
**
** Description      This function returns the handle for the given data
**                  channel PSM. 0, if not found.
**
** Returns          the MCA handle.
**
*******************************************************************************/
tMCA_HANDLE mca_handle_by_dpsm(uint16_t psm)
{
    int     i;
    tMCA_HANDLE handle = 0;
    tMCA_RCB *p_rcb = &mca_cb.rcb[0];

    for(i = 0; i < MCA_NUM_REGS; i++, p_rcb++) {
        if(p_rcb->p_cback && p_rcb->reg.data_psm == psm) {
            handle = i + 1;
            break;
        }
    }

    return handle;
}

/*******************************************************************************
**
** Function         mca_tc_tbl_calloc
**
** Description      This function allocates a transport table for the given
**                  control channel.
**
** Returns          The tranport table.
**
*******************************************************************************/
tMCA_TC_TBL *mca_tc_tbl_calloc(tMCA_CCB *p_ccb)
{
    tMCA_TC_TBL *p_tbl = mca_cb.tc.tc_tbl;
    int             i;

    /* find next free entry in tc table */
    for(i = 0; i < MCA_NUM_TC_TBL; i++, p_tbl++) {
        if(p_tbl->state == MCA_TC_ST_UNUSED) {
            break;
        }
    }

    /* sanity check */
    assert(i != MCA_NUM_TC_TBL);
    /* initialize entry */
    p_tbl->peer_mtu = L2CAP_DEFAULT_MTU;
    p_tbl->cfg_flags = 0;
    p_tbl->cb_idx   = mca_ccb_to_hdl(p_ccb);
    p_tbl->tcid     = MCA_CTRL_TCID;
    p_tbl->my_mtu   = MCA_CTRL_MTU;
    p_tbl->state    = MCA_TC_ST_IDLE;
    p_tbl->lcid     = p_ccb->lcid;
    mca_cb.tc.lcid_tbl[p_ccb->lcid - L2CAP_BASE_APPL_CID] = i;
    MCA_TRACE_DEBUG("%s() - cb_idx: %d", __func__, p_tbl->cb_idx);
    return p_tbl;
}

/*******************************************************************************
**
** Function         mca_tc_tbl_dalloc
**
** Description      This function allocates a transport table for the given
**                  data channel.
**
** Returns          The tranport table.
**
*******************************************************************************/
tMCA_TC_TBL *mca_tc_tbl_dalloc(tMCA_DCB *p_dcb)
{
    tMCA_TC_TBL *p_tbl = mca_cb.tc.tc_tbl;
    int             i;

    /* find next free entry in tc table */
    for(i = 0; i < MCA_NUM_TC_TBL; i++, p_tbl++) {
        if(p_tbl->state == MCA_TC_ST_UNUSED) {
            break;
        }
    }

    /* sanity check */
    assert(i != MCA_NUM_TC_TBL);
    /* initialize entry */
    p_tbl->peer_mtu = L2CAP_DEFAULT_MTU;
    p_tbl->cfg_flags = 0;
    p_tbl->cb_idx   = mca_dcb_to_hdl(p_dcb);
    p_tbl->tcid     = p_dcb->p_cs->type + 1;
    p_tbl->my_mtu   = p_dcb->p_chnl_cfg->data_mtu;
    p_tbl->state    = MCA_TC_ST_IDLE;
    p_tbl->lcid     = p_dcb->lcid;
    mca_cb.tc.lcid_tbl[p_dcb->lcid - L2CAP_BASE_APPL_CID] = i;
    MCA_TRACE_DEBUG("%s() - tcid: %d, cb_idx: %d", __func__, p_tbl->tcid, p_tbl->cb_idx);
    return p_tbl;
}

/*******************************************************************************
**
** Function         mca_tc_tbl_by_lcid
**
** Description      Find the transport channel table entry by LCID.
**
**
** Returns          The tranport table.
**
*******************************************************************************/
tMCA_TC_TBL *mca_tc_tbl_by_lcid(uint16_t lcid)
{
    uint8_t idx;

    if(lcid) {
        idx = mca_cb.tc.lcid_tbl[lcid - L2CAP_BASE_APPL_CID];

        if(idx < MCA_NUM_TC_TBL) {
            return &mca_cb.tc.tc_tbl[idx];
        }
    }

    return NULL;
}

/*******************************************************************************
**
** Function         mca_free_tc_tbl_by_lcid
**
** Description      Find the  transport table entry by LCID
**                  and free the tc_tbl
**
** Returns          void.
**
*******************************************************************************/
void mca_free_tc_tbl_by_lcid(uint16_t lcid)
{
    uint8_t idx;

    if(lcid) {
        idx = mca_cb.tc.lcid_tbl[lcid - L2CAP_BASE_APPL_CID];

        if(idx < MCA_NUM_TC_TBL) {
            mca_cb.tc.tc_tbl[idx].state = MCA_TC_ST_UNUSED;
        }
    }
}


/*******************************************************************************
**
** Function         mca_set_cfg_by_tbl
**
** Description      Set the L2CAP configuration information
**
** Returns          none.
**
*******************************************************************************/
void mca_set_cfg_by_tbl(tL2CAP_CFG_INFO *p_cfg, tMCA_TC_TBL *p_tbl)
{
    tMCA_DCB   *p_dcb;
    const tL2CAP_FCR_OPTS *p_opt;
    tMCA_FCS_OPT    fcs = MCA_FCS_NONE;

    if(p_tbl->tcid == MCA_CTRL_TCID) {
        p_opt = &mca_l2c_fcr_opts_def;
    } else {
        p_dcb = mca_dcb_by_hdl(p_tbl->cb_idx);

        if(p_dcb) {
            p_opt = &p_dcb->p_chnl_cfg->fcr_opt;
            fcs   = p_dcb->p_chnl_cfg->fcs;
        }
    }

    wm_memset(p_cfg, 0, sizeof(tL2CAP_CFG_INFO));
    p_cfg->mtu_present = TRUE;
    p_cfg->mtu = p_tbl->my_mtu;
    p_cfg->fcr_present = TRUE;
    wm_memcpy(&p_cfg->fcr, p_opt, sizeof(tL2CAP_FCR_OPTS));

    if(fcs & MCA_FCS_PRESNT_MASK) {
        p_cfg->fcs_present = TRUE;
        p_cfg->fcs = (fcs & MCA_FCS_USE_MASK);
    }
}

/*******************************************************************************
**
** Function         mca_tc_close_ind
**
** Description      This function is called by the L2CAP interface when the
**                  L2CAP channel is closed.  It looks up the CCB or DCB for
**                  the channel and sends it a close event.  The reason
**                  parameter is the same value passed by the L2CAP
**                  callback function.
**
** Returns          Nothing.
**
*******************************************************************************/
void mca_tc_close_ind(tMCA_TC_TBL *p_tbl, uint16_t reason)
{
    tMCA_CCB   *p_ccb;
    tMCA_DCB   *p_dcb;
    tMCA_CLOSE  close;
    close.param  = MCA_ACP;
    close.reason = reason;
    close.lcid   = p_tbl->lcid;
    MCA_TRACE_DEBUG("%s() - tcid: %d, cb_idx:%d, old: %d", __func__,
                    p_tbl->tcid, p_tbl->cb_idx, p_tbl->state);

    /* Check if the transport channel is in use */
    if(p_tbl->state == MCA_TC_ST_UNUSED) {
        return;
    }

    /* clear mca_tc_tbl entry */
    if(p_tbl->cfg_flags & MCA_L2C_CFG_DISCN_INT) {
        close.param = MCA_INT;
    }

    p_tbl->cfg_flags = 0;
    p_tbl->peer_mtu = L2CAP_DEFAULT_MTU;

    /* if control channel, notify ccb that channel close */
    if(p_tbl->tcid == MCA_CTRL_TCID) {
        p_ccb = mca_ccb_by_hdl((tMCA_CL)p_tbl->cb_idx);
        mca_ccb_event(p_ccb, MCA_CCB_LL_CLOSE_EVT, (tMCA_CCB_EVT *)&close);
    }
    /* notify dcb that channel close */
    else {
        /* look up dcb  */
        p_dcb = mca_dcb_by_hdl(p_tbl->cb_idx);

        if(p_dcb != NULL) {
            mca_dcb_event(p_dcb, MCA_DCB_TC_CLOSE_EVT, (tMCA_DCB_EVT *) &close);
        }
    }

    p_tbl->state = MCA_TC_ST_UNUSED;
}

/*******************************************************************************
**
** Function         mca_tc_open_ind
**
** Description      This function is called by the L2CAP interface when
**                  the L2CAP channel is opened.  It looks up the CCB or DCB
**                  for the channel and sends it an open event.
**
** Returns          Nothing.
**
*******************************************************************************/
void mca_tc_open_ind(tMCA_TC_TBL *p_tbl)
{
    tMCA_CCB   *p_ccb;
    tMCA_DCB   *p_dcb;
    tMCA_OPEN  open;
    MCA_TRACE_DEBUG("mca_tc_open_ind tcid: %d, cb_idx: %d", p_tbl->tcid, p_tbl->cb_idx);
    p_tbl->state = MCA_TC_ST_OPEN;
    open.peer_mtu = p_tbl->peer_mtu;
    open.lcid = p_tbl->lcid;
    /* use param to indicate the role of connection.
     * MCA_ACP, if ACP */
    open.param = MCA_INT;

    if(p_tbl->cfg_flags & MCA_L2C_CFG_CONN_ACP) {
        open.param = MCA_ACP;
    }

    /* if control channel, notify ccb that channel open */
    if(p_tbl->tcid == MCA_CTRL_TCID) {
        p_ccb = mca_ccb_by_hdl((tMCA_CL)p_tbl->cb_idx);
        mca_ccb_event(p_ccb, MCA_CCB_LL_OPEN_EVT, (tMCA_CCB_EVT *)&open);
    }
    /* must be data channel, notify dcb that channel open */
    else {
        /* look up dcb */
        p_dcb = mca_dcb_by_hdl(p_tbl->cb_idx);

        /* put lcid in event data */
        if(p_dcb != NULL) {
            mca_dcb_event(p_dcb, MCA_DCB_TC_OPEN_EVT, (tMCA_DCB_EVT *) &open);
        }
    }
}


/*******************************************************************************
**
** Function         mca_tc_cong_ind
**
** Description      This function is called by the L2CAP interface layer when
**                  L2CAP calls the congestion callback.  It looks up the CCB
**                  or DCB for the channel and sends it a congestion event.
**                  The is_congested parameter is the same value passed by
**                  the L2CAP callback function.
**
**
** Returns          Nothing.
**
*******************************************************************************/
void mca_tc_cong_ind(tMCA_TC_TBL *p_tbl, uint8_t is_congested)
{
    tMCA_CCB   *p_ccb;
    tMCA_DCB   *p_dcb;
    MCA_TRACE_DEBUG("%s() - tcid: %d, cb_idx: %d", __func__, p_tbl->tcid, p_tbl->cb_idx);

    /* if control channel, notify ccb of congestion */
    if(p_tbl->tcid == MCA_CTRL_TCID) {
        p_ccb = mca_ccb_by_hdl((tMCA_CL)p_tbl->cb_idx);
        mca_ccb_event(p_ccb, MCA_CCB_LL_CONG_EVT, (tMCA_CCB_EVT *) &is_congested);
    }
    /* notify dcb that channel open */
    else {
        /* look up dcb by cb_idx */
        p_dcb = mca_dcb_by_hdl(p_tbl->cb_idx);

        if(p_dcb != NULL) {
            mca_dcb_event(p_dcb, MCA_DCB_TC_CONG_EVT, (tMCA_DCB_EVT *) &is_congested);
        }
    }
}


/*******************************************************************************
**
** Function         mca_tc_data_ind
**
** Description      This function is called by the L2CAP interface layer when
**                  incoming data is received from L2CAP.  It looks up the CCB
**                  or DCB for the channel and routes the data accordingly.
**
** Returns          Nothing.
**
*******************************************************************************/
void mca_tc_data_ind(tMCA_TC_TBL *p_tbl, BT_HDR *p_buf)
{
    tMCA_CCB   *p_ccb;
    tMCA_DCB   *p_dcb;
    uint8_t       event = MCA_CCB_MSG_RSP_EVT;
    uint8_t       *p;
    uint8_t       rej_rsp_code = MCA_RSP_SUCCESS;
    MCA_TRACE_DEBUG("%s() - tcid: %d, cb_idx: %d", __func__, p_tbl->tcid, p_tbl->cb_idx);

    /* if control channel, handle control message */
    if(p_tbl->tcid == MCA_CTRL_TCID) {
        p_ccb = mca_ccb_by_hdl((tMCA_CL)p_tbl->cb_idx);

        if(p_ccb) {
            p = (uint8_t *)(p_buf + 1) + p_buf->offset;

            /* all the request opcode has bit 0 set. response code has bit 0 clear */
            if((*p) & 0x01) {
                event = MCA_CCB_MSG_REQ_EVT;
            }

            if(*p < MCA_NUM_STANDARD_OPCODE) {
                if(p_buf->len != mca_std_msg_len[*p]) {
                    MCA_TRACE_ERROR("$s() - opcode: %d required len: %d, got len: %d"
                                    , __func__, *p, mca_std_msg_len[*p], p_buf->len);
                    rej_rsp_code = MCA_RSP_BAD_PARAM;
                }
            } else if((*p >= MCA_FIRST_SYNC_OP) && (*p <= MCA_LAST_SYNC_OP)) {
                MCA_TRACE_ERROR("%s() - unsupported SYNC opcode: %d len:%d"
                                , __func__, *p, p_buf->len);
                /* reject unsupported request */
                rej_rsp_code = MCA_RSP_NO_SUPPORT;
            } else {
                MCA_TRACE_ERROR("%s() - bad opcode: %d len:%d", __func__, *p, p_buf->len);
                /* reject unsupported request */
                rej_rsp_code = MCA_RSP_BAD_OPCODE;
            }

            p_buf->layer_specific = rej_rsp_code;
            /* forward the request/response to state machine */
            mca_ccb_event(p_ccb, event, (tMCA_CCB_EVT *) p_buf);
        } /* got a valid ccb */
        else {
            GKI_freebuf(p_buf);
        }
    }
    /* else send event to dcb */
    else {
        p_dcb = mca_dcb_by_hdl(p_tbl->cb_idx);

        if(p_dcb != NULL) {
            mca_dcb_event(p_dcb, MCA_DCB_TC_DATA_EVT, (tMCA_DCB_EVT *) p_buf);
        } else {
            GKI_freebuf(p_buf);
        }
    }
}

/*******************************************************************************
**
** Function         mca_rcb_alloc
**
** Description      This function allocates a registration control block.
**                  If no free RCB is available, it returns NULL.
**
** Returns          tMCA_RCB *
**
*******************************************************************************/
tMCA_RCB *mca_rcb_alloc(tMCA_REG *p_reg)
{
    int     i;
    tMCA_RCB *p_rcb = NULL;

    for(i = 0; i < MCA_NUM_REGS; i++) {
        if(mca_cb.rcb[i].p_cback == NULL) {
            p_rcb = &mca_cb.rcb[i];
            wm_memcpy(&p_rcb->reg, p_reg, sizeof(tMCA_REG));
            break;
        }
    }

    return p_rcb;
}

/*******************************************************************************
**
** Function         mca_rcb_dealloc
**
** Description      This function deallocates the RCB with the given handle.
**
** Returns          void.
**
*******************************************************************************/
void mca_rcb_dealloc(tMCA_HANDLE handle)
{
    int      i;
    uint8_t  done = TRUE;
    tMCA_RCB *p_rcb;
    tMCA_CCB *p_ccb;

    if(handle && (handle <= MCA_NUM_REGS)) {
        handle--;
        p_rcb = &mca_cb.rcb[handle];

        if(p_rcb->p_cback) {
            p_ccb = &mca_cb.ccb[handle * MCA_NUM_LINKS];

            /* check if all associated CCB are disconnected */
            for(i = 0; i < MCA_NUM_LINKS; i++, p_ccb++) {
                if(p_ccb->p_rcb) {
                    done = FALSE;
                    mca_ccb_event(p_ccb, MCA_CCB_API_DISCONNECT_EVT, NULL);
                }
            }

            if(done) {
                wm_memset(p_rcb, 0, sizeof(tMCA_RCB));
                MCA_TRACE_DEBUG("%s() - reset MCA_RCB index=%d", __func__, handle);
            }
        }
    }
}

/*******************************************************************************
**
** Function         mca_rcb_to_handle
**
** Description      This function converts a pointer to an RCB to
**                  a handle (tMCA_HANDLE).  It returns the handle.
**
** Returns          void.
**
*******************************************************************************/
tMCA_HANDLE mca_rcb_to_handle(tMCA_RCB *p_rcb)
{
    return(uint8_t)(p_rcb - mca_cb.rcb + 1);
}

/*******************************************************************************
**
** Function         mca_rcb_by_handle
**
** Description      This function finds the RCB for a handle (tMCA_HANDLE).
**                  It returns a pointer to the RCB.  If no RCB matches the
**                  handle it returns NULL.
**
** Returns          tMCA_RCB *
**
*******************************************************************************/
tMCA_RCB *mca_rcb_by_handle(tMCA_HANDLE handle)
{
    tMCA_RCB *p_rcb = NULL;

    if(handle && (handle <= MCA_NUM_REGS) && mca_cb.rcb[handle - 1].p_cback) {
        p_rcb = &mca_cb.rcb[handle - 1];
    }

    return p_rcb;
}

/*******************************************************************************
**
** Function         mca_is_valid_dep_id
**
** Description      This function checks if the given dep_id is valid.
**
** Returns          TRUE, if this is a valid local dep_id
**
*******************************************************************************/
uint8_t mca_is_valid_dep_id(tMCA_RCB *p_rcb, tMCA_DEP dep)
{
    uint8_t valid = FALSE;

    if(dep < MCA_NUM_DEPS && p_rcb->dep[dep].p_data_cback) {
        valid = TRUE;
    }

    return valid;
}
#endif
