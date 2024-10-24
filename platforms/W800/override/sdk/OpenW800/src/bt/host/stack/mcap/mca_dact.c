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
 *  This is the implementation file for the MCAP Data Channel Action
 *  Functions.
 *
 ******************************************************************************/

#include <stddef.h>
#include "bt_target.h"
#if defined(MCA_INCLUDED) &&(MCA_INCLUDED == TRUE)
#include "bt_utils.h"
#include "bt_common.h"
#include "mca_api.h"
#include "mca_int.h"

/*******************************************************************************
**
** Function         mca_dcb_report_cong
**
** Description      This function is called to report the congestion flag.
**
** Returns          void.
**
*******************************************************************************/
void mca_dcb_report_cong(tMCA_DCB *p_dcb)
{
    tMCA_CTRL   evt_data;
    evt_data.cong_chg.cong   = p_dcb->cong;
    evt_data.cong_chg.mdl    = mca_dcb_to_hdl(p_dcb);
    evt_data.cong_chg.mdl_id = p_dcb->mdl_id;
    mca_ccb_report_event(p_dcb->p_ccb, MCA_CONG_CHG_EVT, &evt_data);
}

/*******************************************************************************
**
** Function         mca_dcb_tc_open
**
** Description      This function is called to report MCA_OPEN_IND_EVT or
**                  MCA_OPEN_CFM_EVT event.
**                  It also clears the congestion flag (dcb.cong).
**
** Returns          void.
**
*******************************************************************************/
void mca_dcb_tc_open(tMCA_DCB *p_dcb, tMCA_DCB_EVT *p_data)
{
    tMCA_CTRL   evt_data;
    tMCA_CCB    *p_ccb = p_dcb->p_ccb;
    uint8_t       event = MCA_OPEN_IND_EVT;

    if(p_data->open.param == MCA_INT) {
        event = MCA_OPEN_CFM_EVT;
    }

    p_dcb->cong  = FALSE;
    evt_data.open_cfm.mtu       = p_data->open.peer_mtu;
    evt_data.open_cfm.mdl_id    = p_dcb->mdl_id;
    evt_data.open_cfm.mdl       = mca_dcb_to_hdl(p_dcb);
    mca_ccb_event(p_ccb, MCA_CCB_DL_OPEN_EVT, NULL);
    mca_ccb_report_event(p_ccb, event, &evt_data);
}

/*******************************************************************************
**
** Function         mca_dcb_cong
**
** Description      This function sets the congestion state for the DCB.
**
** Returns          void.
**
*******************************************************************************/
void mca_dcb_cong(tMCA_DCB *p_dcb, tMCA_DCB_EVT *p_data)
{
    p_dcb->cong  = p_data->llcong;
    mca_dcb_report_cong(p_dcb);
}

/*******************************************************************************
**
** Function         mca_dcb_free_data
**
** Description      This function frees the received message.
**
** Returns          void.
**
*******************************************************************************/
void mca_dcb_free_data(tMCA_DCB *p_dcb, tMCA_DCB_EVT *p_data)
{
    UNUSED(p_dcb);
    GKI_freebuf(p_data);
}

/*******************************************************************************
**
** Function         mca_dcb_do_disconn
**
** Description      This function closes a data channel.
**
** Returns          void.
**
*******************************************************************************/
void mca_dcb_do_disconn(tMCA_DCB *p_dcb, tMCA_DCB_EVT *p_data)
{
    tMCA_CLOSE  close;
    UNUSED(p_data);

    if((p_dcb->lcid == 0) || (L2CA_DisconnectReq(p_dcb->lcid) == FALSE)) {
        close.param  = MCA_INT;
        close.reason = L2CAP_DISC_OK;
        close.lcid   = 0;
        mca_dcb_event(p_dcb, MCA_DCB_TC_CLOSE_EVT, (tMCA_DCB_EVT *) &close);
    }
}

/*******************************************************************************
**
** Function         mca_dcb_snd_data
**
** Description      This function sends the data from application to the peer device.
**
** Returns          void.
**
*******************************************************************************/
void mca_dcb_snd_data(tMCA_DCB *p_dcb, tMCA_DCB_EVT *p_data)
{
    uint8_t   status;
    /* do not need to check cong, because API already checked the status */
    status = L2CA_DataWrite(p_dcb->lcid, p_data->p_pkt);

    if(status == L2CAP_DW_CONGESTED) {
        p_dcb->cong = TRUE;
        mca_dcb_report_cong(p_dcb);
    }
}

/*******************************************************************************
**
** Function         mca_dcb_hdl_data
**
** Description      This function reports the received data through the data
**                  callback function.
**
** Returns          void.
**
*******************************************************************************/
void mca_dcb_hdl_data(tMCA_DCB *p_dcb, tMCA_DCB_EVT *p_data)
{
    (*p_dcb->p_cs->p_data_cback)(mca_dcb_to_hdl(p_dcb), (BT_HDR *)p_data);
}
#endif
