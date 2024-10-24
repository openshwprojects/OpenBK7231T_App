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
 *  This is the implementation file for the MCAP Control Channel Action
 *  Functions.
 *
 ******************************************************************************/
#include <string.h>
#include "bt_target.h"
#if defined(MCA_INCLUDED) &&(MCA_INCLUDED == TRUE)
#include "bt_utils.h"
#include "bt_common.h"
#include "btm_api.h"
#include "mca_api.h"
#include "mca_defs.h"
#include "mca_int.h"


#include  "btu.h"

extern fixed_queue_t *btu_general_alarm_queue;

/*****************************************************************************
** constants
*****************************************************************************/
/*******************************************************************************
**
** Function         mca_ccb_rsp_tout
**
** Description      This function processes the response timeout.
**
** Returns          void.
**
*******************************************************************************/
void mca_ccb_rsp_tout(tMCA_CCB *p_ccb, tMCA_CCB_EVT *p_data)
{
    tMCA_CTRL   evt_data;
    UNUSED(p_data);
    mca_ccb_report_event(p_ccb, MCA_RSP_TOUT_IND_EVT, &evt_data);
}

/*******************************************************************************
**
** Function         mca_ccb_report_event
**
** Description      This function reports the given event.
**
** Returns          void.
**
*******************************************************************************/
void mca_ccb_report_event(tMCA_CCB *p_ccb, uint8_t event, tMCA_CTRL *p_data)
{
    if(p_ccb && p_ccb->p_rcb && p_ccb->p_rcb->p_cback) {
        (*p_ccb->p_rcb->p_cback)(mca_rcb_to_handle(p_ccb->p_rcb), mca_ccb_to_hdl(p_ccb), event, p_data);
    }
}

/*******************************************************************************
**
** Function         mca_ccb_free_msg
**
** Description      This function frees the received message.
**
** Returns          void.
**
*******************************************************************************/
void mca_ccb_free_msg(tMCA_CCB *p_ccb, tMCA_CCB_EVT *p_data)
{
    UNUSED(p_ccb);
    GKI_freebuf(p_data);
}

/*******************************************************************************
**
** Function         mca_ccb_snd_req
**
** Description      This function builds a request and sends it to the peer.
**
** Returns          void.
**
*******************************************************************************/
void mca_ccb_snd_req(tMCA_CCB *p_ccb, tMCA_CCB_EVT *p_data)
{
    tMCA_CCB_MSG *p_msg = (tMCA_CCB_MSG *)p_data;
    uint8_t   *p, *p_start;
    uint8_t is_abort = FALSE;
    tMCA_DCB *p_dcb;
    MCA_TRACE_DEBUG("mca_ccb_snd_req cong=%d req=%d", p_ccb->cong, p_msg->op_code);

    /* check for abort request */
    if((p_ccb->status == MCA_CCB_STAT_PENDING) && (p_msg->op_code == MCA_OP_MDL_ABORT_REQ)) {
        p_dcb = mca_dcb_by_hdl(p_ccb->p_tx_req->dcb_idx);
        /* the Abort API does not have the associated mdl_id.
         * Get the mdl_id in dcb to compose the request */
        p_msg->mdl_id = p_dcb->mdl_id;
        mca_dcb_event(p_dcb, MCA_DCB_API_CLOSE_EVT, NULL);
        GKI_free_and_reset_buf((void **)&p_ccb->p_tx_req);
        p_ccb->status = MCA_CCB_STAT_NORM;
        is_abort = TRUE;
    }

    /* no pending outgoing messages or it's an abort request for a pending data channel */
    if((!p_ccb->p_tx_req) || is_abort) {
        p_ccb->p_tx_req = p_msg;

        if(!p_ccb->cong) {
            BT_HDR *p_pkt = (BT_HDR *)GKI_getbuf(MCA_CTRL_MTU);
            p_pkt->offset = L2CAP_MIN_OFFSET;
            p = p_start = (uint8_t *)(p_pkt + 1) + L2CAP_MIN_OFFSET;
            *p++ = p_msg->op_code;
            UINT16_TO_BE_STREAM(p, p_msg->mdl_id);

            if(p_msg->op_code == MCA_OP_MDL_CREATE_REQ) {
                *p++ = p_msg->mdep_id;
                *p++ = p_msg->param;
            }

            p_msg->hdr.layer_specific = TRUE;   /* mark this message as sent */
            p_pkt->len = p - p_start;
            L2CA_DataWrite(p_ccb->lcid, p_pkt);
            uint64_t interval_ms = p_ccb->p_rcb->reg.rsp_tout * 1000;
            alarm_set_on_queue(p_ccb->mca_ccb_timer, interval_ms,
                               mca_ccb_timer_timeout, p_ccb,
                               btu_general_alarm_queue);
        }

        /* else the L2CAP channel is congested. keep the message to be sent later */
    } else {
        MCA_TRACE_WARNING("dropping api req");
        GKI_freebuf(p_data);
    }
}

/*******************************************************************************
**
** Function         mca_ccb_snd_rsp
**
** Description      This function builds a response and sends it to
**                  the peer.
**
** Returns          void.
**
*******************************************************************************/
void mca_ccb_snd_rsp(tMCA_CCB *p_ccb, tMCA_CCB_EVT *p_data)
{
    tMCA_CCB_MSG *p_msg = (tMCA_CCB_MSG *)p_data;
    uint8_t   *p, *p_start;
    uint8_t chk_mdl = FALSE;
    BT_HDR *p_pkt = (BT_HDR *)GKI_getbuf(MCA_CTRL_MTU);
    MCA_TRACE_DEBUG("%s cong=%d req=%d", __func__, p_ccb->cong, p_msg->op_code);
    /* assume that API functions verified the parameters */
    p_pkt->offset = L2CAP_MIN_OFFSET;
    p = p_start = (uint8_t *)(p_pkt + 1) + L2CAP_MIN_OFFSET;
    *p++ = p_msg->op_code;
    *p++ = p_msg->rsp_code;
    UINT16_TO_BE_STREAM(p, p_msg->mdl_id);

    if(p_msg->op_code == MCA_OP_MDL_CREATE_RSP) {
        *p++ = p_msg->param;
        chk_mdl = TRUE;
    } else if(p_msg->op_code == MCA_OP_MDL_RECONNECT_RSP) {
        chk_mdl = TRUE;
    }

    if(chk_mdl && p_msg->rsp_code == MCA_RSP_SUCCESS) {
        mca_dcb_by_hdl(p_msg->dcb_idx);
        BTM_SetSecurityLevel(FALSE, "", BTM_SEC_SERVICE_MCAP_DATA,
                             p_ccb->sec_mask,
                             p_ccb->p_rcb->reg.data_psm, BTM_SEC_PROTO_MCA,
                             p_msg->dcb_idx);
        p_ccb->status = MCA_CCB_STAT_PENDING;
        /* set p_tx_req to block API_REQ/API_RSP before DL is up */
        GKI_free_and_reset_buf((void **)&p_ccb->p_tx_req);
        p_ccb->p_tx_req = p_ccb->p_rx_msg;
        p_ccb->p_rx_msg = NULL;
        p_ccb->p_tx_req->dcb_idx = p_msg->dcb_idx;
    }

    GKI_free_and_reset_buf((void **)&p_ccb->p_rx_msg);
    p_pkt->len = p - p_start;
    L2CA_DataWrite(p_ccb->lcid, p_pkt);
}

/*******************************************************************************
**
** Function         mca_ccb_do_disconn
**
** Description      This function closes a control channel.
**
** Returns          void.
**
*******************************************************************************/
void mca_ccb_do_disconn(tMCA_CCB *p_ccb, tMCA_CCB_EVT *p_data)
{
    UNUSED(p_data);
    mca_dcb_close_by_mdl_id(p_ccb, MCA_ALL_MDL_ID);
    L2CA_DisconnectReq(p_ccb->lcid);
}

/*******************************************************************************
**
** Function         mca_ccb_cong
**
** Description      This function sets the congestion state for the CCB.
**
** Returns          void.
**
*******************************************************************************/
void mca_ccb_cong(tMCA_CCB *p_ccb, tMCA_CCB_EVT *p_data)
{
    MCA_TRACE_DEBUG("mca_ccb_cong cong=%d/%d", p_ccb->cong, p_data->llcong);
    p_ccb->cong = p_data->llcong;

    if(!p_ccb->cong) {
        /* if there's a held packet, send it now */
        if(p_ccb->p_tx_req && !p_ccb->p_tx_req->hdr.layer_specific) {
            p_data = (tMCA_CCB_EVT *)p_ccb->p_tx_req;
            p_ccb->p_tx_req = NULL;
            mca_ccb_snd_req(p_ccb, p_data);
        }
    }
}

/*******************************************************************************
**
** Function         mca_ccb_hdl_req
**
** Description      This function is called when a MCAP request is received from
**                  the peer. It calls the application callback function to
**                  report the event.
**
** Returns          void.
**
*******************************************************************************/
void mca_ccb_hdl_req(tMCA_CCB *p_ccb, tMCA_CCB_EVT *p_data)
{
    BT_HDR  *p_pkt = &p_data->hdr;
    uint8_t   *p, *p_start;
    tMCA_DCB    *p_dcb;
    tMCA_CTRL       evt_data;
    tMCA_CCB_MSG    *p_rx_msg = NULL;
    uint8_t           reject_code = MCA_RSP_NO_RESOURCE;
    uint8_t         send_rsp = FALSE;
    uint8_t         check_req = FALSE;
    uint8_t           reject_opcode;
    MCA_TRACE_DEBUG("mca_ccb_hdl_req status:%d", p_ccb->status);
    p_rx_msg = (tMCA_CCB_MSG *)p_pkt;
    p = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
    evt_data.hdr.op_code = *p++;
    BE_STREAM_TO_UINT16(evt_data.hdr.mdl_id, p);
    reject_opcode = evt_data.hdr.op_code + 1;
    MCA_TRACE_DEBUG("received mdl id: %d ", evt_data.hdr.mdl_id);

    if(p_ccb->status == MCA_CCB_STAT_PENDING) {
        MCA_TRACE_DEBUG("received req inpending state");

        /* allow abort in pending state */
        if((p_ccb->status == MCA_CCB_STAT_PENDING) && (evt_data.hdr.op_code == MCA_OP_MDL_ABORT_REQ)) {
            reject_code = MCA_RSP_SUCCESS;
            send_rsp = TRUE;
            /* clear the pending status */
            p_ccb->status = MCA_CCB_STAT_NORM;

            if(p_ccb->p_tx_req && ((p_dcb = mca_dcb_by_hdl(p_ccb->p_tx_req->dcb_idx)) != NULL)) {
                mca_dcb_dealloc(p_dcb, NULL);
                GKI_free_and_reset_buf((void **)&p_ccb->p_tx_req);
            }
        } else {
            reject_code = MCA_RSP_BAD_OP;
        }
    } else if(p_ccb->p_rx_msg) {
        MCA_TRACE_DEBUG("still handling prev req");
        /* still holding previous message, reject this new one ?? */
    } else if(p_ccb->p_tx_req) {
        MCA_TRACE_DEBUG("still waiting for a response ctrl_vpsm:0x%x", p_ccb->ctrl_vpsm);

        /* sent a request; waiting for response */
        if(p_ccb->ctrl_vpsm == 0) {
            MCA_TRACE_DEBUG("local is ACP. accept the cmd from INT");
            /* local is acceptor, need to handle the request */
            check_req = TRUE;
            reject_code = MCA_RSP_SUCCESS;

            /* drop the previous request */
            if((p_ccb->p_tx_req->op_code == MCA_OP_MDL_CREATE_REQ) &&
                    ((p_dcb = mca_dcb_by_hdl(p_ccb->p_tx_req->dcb_idx)) != NULL)) {
                mca_dcb_dealloc(p_dcb, NULL);
            }

            GKI_free_and_reset_buf((void **)&p_ccb->p_tx_req);
            mca_stop_timer(p_ccb);
        } else {
            /*  local is initiator, ignore the req */
            GKI_freebuf(p_pkt);
            return;
        }
    } else if(p_pkt->layer_specific != MCA_RSP_SUCCESS) {
        reject_code = (uint8_t)p_pkt->layer_specific;

        if(((evt_data.hdr.op_code >= MCA_NUM_STANDARD_OPCODE) &&
                (evt_data.hdr.op_code < MCA_FIRST_SYNC_OP)) ||
                (evt_data.hdr.op_code > MCA_LAST_SYNC_OP)) {
            /* invalid op code */
            reject_opcode = MCA_OP_ERROR_RSP;
            evt_data.hdr.mdl_id = 0;
        }
    } else {
        check_req = TRUE;
        reject_code = MCA_RSP_SUCCESS;
    }

    if(check_req) {
        if(reject_code == MCA_RSP_SUCCESS) {
            reject_code = MCA_RSP_BAD_MDL;

            if(MCA_IS_VALID_MDL_ID(evt_data.hdr.mdl_id) ||
                    ((evt_data.hdr.mdl_id == MCA_ALL_MDL_ID) && (evt_data.hdr.op_code == MCA_OP_MDL_DELETE_REQ))) {
                reject_code = MCA_RSP_SUCCESS;

                /* mdl_id is valid according to the spec */
                switch(evt_data.hdr.op_code) {
                    case MCA_OP_MDL_CREATE_REQ:
                        evt_data.create_ind.dep_id = *p++;
                        evt_data.create_ind.cfg = *p++;
                        p_rx_msg->mdep_id = evt_data.create_ind.dep_id;

                        if(!mca_is_valid_dep_id(p_ccb->p_rcb, p_rx_msg->mdep_id)) {
                            MCA_TRACE_ERROR("not a valid local mdep id");
                            reject_code = MCA_RSP_BAD_MDEP;
                        } else if(mca_ccb_uses_mdl_id(p_ccb, evt_data.hdr.mdl_id)) {
                            MCA_TRACE_DEBUG("the mdl_id is currently used in the CL(create)");
                            mca_dcb_close_by_mdl_id(p_ccb, evt_data.hdr.mdl_id);
                        } else {
                            /* check if this dep still have MDL available */
                            if(mca_dep_free_mdl(p_ccb, evt_data.create_ind.dep_id) == 0) {
                                MCA_TRACE_ERROR("the mdep is currently using max_mdl");
                                reject_code = MCA_RSP_MDEP_BUSY;
                            }
                        }

                        break;

                    case MCA_OP_MDL_RECONNECT_REQ:
                        if(mca_ccb_uses_mdl_id(p_ccb, evt_data.hdr.mdl_id)) {
                            MCA_TRACE_ERROR("the mdl_id is currently used in the CL(reconn)");
                            reject_code = MCA_RSP_MDL_BUSY;
                        }

                        break;

                    case MCA_OP_MDL_ABORT_REQ:
                        reject_code = MCA_RSP_BAD_OP;
                        break;

                    case MCA_OP_MDL_DELETE_REQ:
                        /* delete the associated mdl */
                        mca_dcb_close_by_mdl_id(p_ccb, evt_data.hdr.mdl_id);
                        send_rsp = TRUE;
                        break;
                }
            }
        }
    }

    if(((reject_code != MCA_RSP_SUCCESS) && (evt_data.hdr.op_code != MCA_OP_SYNC_INFO_IND))
            || send_rsp) {
        BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(MCA_CTRL_MTU);
        p_buf->offset = L2CAP_MIN_OFFSET;
        p = p_start = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
        *p++ = reject_opcode;
        *p++ = reject_code;
        UINT16_TO_BE_STREAM(p, evt_data.hdr.mdl_id);
        /*
          if (((*p_start) == MCA_OP_MDL_CREATE_RSP) && (reject_code == MCA_RSP_SUCCESS))
          {
          *p++ = evt_data.create_ind.cfg;
          }
        */
        p_buf->len = p - p_start;
        L2CA_DataWrite(p_ccb->lcid, p_buf);
    }

    if(reject_code == MCA_RSP_SUCCESS) {
        /* use the received GKI buffer to store information to double check response API */
        p_rx_msg->op_code = evt_data.hdr.op_code;
        p_rx_msg->mdl_id = evt_data.hdr.mdl_id;
        p_ccb->p_rx_msg = p_rx_msg;

        if(send_rsp) {
            GKI_freebuf(p_pkt);
            p_ccb->p_rx_msg = NULL;
        }

        mca_ccb_report_event(p_ccb, evt_data.hdr.op_code, &evt_data);
    } else {
        GKI_freebuf(p_pkt);
    }
}

/*******************************************************************************
**
** Function         mca_ccb_hdl_rsp
**
** Description      This function is called when a MCAP response is received from
**                  the peer.  It calls the application callback function with
**                  the results.
**
** Returns          void.
**
*******************************************************************************/
void mca_ccb_hdl_rsp(tMCA_CCB *p_ccb, tMCA_CCB_EVT *p_data)
{
    BT_HDR  *p_pkt = &p_data->hdr;
    uint8_t   *p;
    tMCA_CTRL   evt_data;
    uint8_t     chk_mdl = FALSE;
    tMCA_DCB    *p_dcb;
    tMCA_RESULT result = MCA_BAD_HANDLE;
    tMCA_TC_TBL *p_tbl;

    if(p_ccb->p_tx_req) {
        /* verify that the received response matches the sent request */
        p = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
        evt_data.hdr.op_code = *p++;

        if((evt_data.hdr.op_code == 0) ||
                ((p_ccb->p_tx_req->op_code + 1) == evt_data.hdr.op_code)) {
            evt_data.rsp.rsp_code = *p++;
            mca_stop_timer(p_ccb);
            BE_STREAM_TO_UINT16(evt_data.hdr.mdl_id, p);

            if(evt_data.hdr.op_code == MCA_OP_MDL_CREATE_RSP) {
                evt_data.create_cfm.cfg = *p++;
                chk_mdl = TRUE;
            } else if(evt_data.hdr.op_code == MCA_OP_MDL_RECONNECT_RSP) {
                chk_mdl = TRUE;
            }

            if(chk_mdl) {
                p_dcb = mca_dcb_by_hdl(p_ccb->p_tx_req->dcb_idx);

                if(evt_data.rsp.rsp_code == MCA_RSP_SUCCESS) {
                    if(evt_data.hdr.mdl_id != p_dcb->mdl_id) {
                        MCA_TRACE_ERROR("peer's mdl_id=%d != our mdl_id=%d", evt_data.hdr.mdl_id, p_dcb->mdl_id);

                        /* change the response code to be an error */
                        if(evt_data.rsp.rsp_code == MCA_RSP_SUCCESS) {
                            evt_data.rsp.rsp_code = MCA_RSP_BAD_MDL;
                            /* send Abort */
                            p_ccb->status = MCA_CCB_STAT_PENDING;
                            MCA_Abort(mca_ccb_to_hdl(p_ccb));
                        }
                    } else if(p_dcb->p_chnl_cfg) {
                        /* the data channel configuration is known. Proceed with data channel initiation */
                        BTM_SetSecurityLevel(TRUE, "", BTM_SEC_SERVICE_MCAP_DATA, p_ccb->sec_mask,
                                             p_ccb->data_vpsm, BTM_SEC_PROTO_MCA, p_ccb->p_tx_req->dcb_idx);
                        p_dcb->lcid = mca_l2c_open_req(p_ccb->peer_addr, p_ccb->data_vpsm, p_dcb->p_chnl_cfg);

                        if(p_dcb->lcid) {
                            p_tbl = mca_tc_tbl_dalloc(p_dcb);

                            if(p_tbl) {
                                p_tbl->state = MCA_TC_ST_CONN;
                                p_ccb->status = MCA_CCB_STAT_PENDING;
                                result = MCA_SUCCESS;
                            }
                        }
                    } else {
                        /* mark this MCL as pending and wait for MCA_DataChnlCfg */
                        p_ccb->status = MCA_CCB_STAT_PENDING;
                        result = MCA_SUCCESS;
                    }
                }

                if(result != MCA_SUCCESS && p_dcb) {
                    mca_dcb_dealloc(p_dcb, NULL);
                }
            } /* end of chk_mdl */

            if(p_ccb->status != MCA_CCB_STAT_PENDING) {
                GKI_free_and_reset_buf((void **)&p_ccb->p_tx_req);
            }

            mca_ccb_report_event(p_ccb, evt_data.hdr.op_code, &evt_data);
        }

        /* else a bad response is received */
    } else {
        /* not expecting any response. drop it */
        MCA_TRACE_WARNING("dropping received rsp (not expecting a response)");
    }

    GKI_freebuf(p_data);
}

/*******************************************************************************
**
** Function         mca_ccb_ll_open
**
** Description      This function is called to report MCA_CONNECT_IND_EVT event.
**                  It also clears the congestion flag (ccb.cong).
**
** Returns          void.
**
*******************************************************************************/
void mca_ccb_ll_open(tMCA_CCB *p_ccb, tMCA_CCB_EVT *p_data)
{
    tMCA_CTRL    evt_data;
    p_ccb->cong  = FALSE;
    evt_data.connect_ind.mtu = p_data->open.peer_mtu;
    wm_memcpy(evt_data.connect_ind.bd_addr, p_ccb->peer_addr, BD_ADDR_LEN);
    mca_ccb_report_event(p_ccb, MCA_CONNECT_IND_EVT, &evt_data);
}

/*******************************************************************************
**
** Function         mca_ccb_dl_open
**
** Description      This function is called when data channel is open.
**                  It clears p_tx_req to allow other message exchage on this CL.
**
** Returns          void.
**
*******************************************************************************/
void mca_ccb_dl_open(tMCA_CCB *p_ccb, tMCA_CCB_EVT *p_data)
{
    UNUSED(p_data);
    GKI_free_and_reset_buf((void **)&p_ccb->p_tx_req);
    GKI_free_and_reset_buf((void **)&p_ccb->p_rx_msg);
    p_ccb->status = MCA_CCB_STAT_NORM;
}
#endif
