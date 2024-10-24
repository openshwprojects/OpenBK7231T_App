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
 *  This is the implementation file for the MCAP Control channel state
 *  machine.
 *
 ******************************************************************************/
#include <string.h>

#include "bt_target.h"
#if defined(MCA_INCLUDED) &&(MCA_INCLUDED == TRUE)
#include "mca_api.h"
#include "mca_defs.h"
#include "mca_int.h"
#include  "btu.h"

/*****************************************************************************
** data channel state machine constants and types
*****************************************************************************/
enum {
    MCA_CCB_FREE_MSG,
    MCA_CCB_SND_REQ,
    MCA_CCB_SND_RSP,
    MCA_CCB_DO_DISCONN,
    MCA_CCB_CONG,
    MCA_CCB_HDL_REQ,
    MCA_CCB_HDL_RSP,
    MCA_CCB_LL_OPEN,
    MCA_CCB_DL_OPEN,
    MCA_CCB_DEALLOC,
    MCA_CCB_RSP_TOUT,
    MCA_CCB_NUM_ACTIONS
};
#define MCA_CCB_IGNORE     MCA_CCB_NUM_ACTIONS

/* action function list */
const tMCA_CCB_ACTION mca_ccb_action[] = {
    mca_ccb_free_msg,
    mca_ccb_snd_req,
    mca_ccb_snd_rsp,
    mca_ccb_do_disconn,
    mca_ccb_cong,
    mca_ccb_hdl_req,
    mca_ccb_hdl_rsp,
    mca_ccb_ll_open,
    mca_ccb_dl_open,
    mca_ccb_dealloc,
    mca_ccb_rsp_tout,
};

/* state table information */
#define MCA_CCB_ACTIONS            1       /* number of actions */
#define MCA_CCB_ACT_COL            0       /* position of action function */
#define MCA_CCB_NEXT_STATE         1       /* position of next state */
#define MCA_CCB_NUM_COLS           2       /* number of columns in state tables */

/* state table for opening state */
const uint8_t mca_ccb_st_opening[][MCA_CCB_NUM_COLS] = {
    /* Event                            Action              Next State */
    /* MCA_CCB_API_CONNECT_EVT    */   {MCA_CCB_IGNORE,     MCA_CCB_OPENING_ST},
    /* MCA_CCB_API_DISCONNECT_EVT */   {MCA_CCB_DO_DISCONN, MCA_CCB_CLOSING_ST},
    /* MCA_CCB_API_REQ_EVT        */   {MCA_CCB_FREE_MSG,   MCA_CCB_OPENING_ST},
    /* MCA_CCB_API_RSP_EVT        */   {MCA_CCB_IGNORE,     MCA_CCB_OPENING_ST},
    /* MCA_CCB_MSG_REQ_EVT        */   {MCA_CCB_FREE_MSG,   MCA_CCB_OPENING_ST},
    /* MCA_CCB_MSG_RSP_EVT        */   {MCA_CCB_FREE_MSG,   MCA_CCB_OPENING_ST},
    /* MCA_CCB_DL_OPEN_EVT        */   {MCA_CCB_IGNORE,     MCA_CCB_OPENING_ST},
    /* MCA_CCB_LL_OPEN_EVT        */   {MCA_CCB_LL_OPEN,    MCA_CCB_OPEN_ST},
    /* MCA_CCB_LL_CLOSE_EVT       */   {MCA_CCB_DEALLOC,    MCA_CCB_NULL_ST},
    /* MCA_CCB_LL_CONG_EVT        */   {MCA_CCB_CONG,       MCA_CCB_OPENING_ST},
    /* MCA_CCB_RSP_TOUT_EVT       */   {MCA_CCB_IGNORE,     MCA_CCB_OPENING_ST}
};

/* state table for open state */
const uint8_t mca_ccb_st_open[][MCA_CCB_NUM_COLS] = {
    /* Event                            Action              Next State */
    /* MCA_CCB_API_CONNECT_EVT    */   {MCA_CCB_IGNORE,     MCA_CCB_OPEN_ST},
    /* MCA_CCB_API_DISCONNECT_EVT */   {MCA_CCB_DO_DISCONN, MCA_CCB_CLOSING_ST},
    /* MCA_CCB_API_REQ_EVT        */   {MCA_CCB_SND_REQ,    MCA_CCB_OPEN_ST},
    /* MCA_CCB_API_RSP_EVT        */   {MCA_CCB_SND_RSP,    MCA_CCB_OPEN_ST},
    /* MCA_CCB_MSG_REQ_EVT        */   {MCA_CCB_HDL_REQ,    MCA_CCB_OPEN_ST},
    /* MCA_CCB_MSG_RSP_EVT        */   {MCA_CCB_HDL_RSP,    MCA_CCB_OPEN_ST},
    /* MCA_CCB_DL_OPEN_EVT        */   {MCA_CCB_DL_OPEN,    MCA_CCB_OPEN_ST},
    /* MCA_CCB_LL_OPEN_EVT        */   {MCA_CCB_IGNORE,     MCA_CCB_OPEN_ST},
    /* MCA_CCB_LL_CLOSE_EVT       */   {MCA_CCB_DEALLOC,    MCA_CCB_NULL_ST},
    /* MCA_CCB_LL_CONG_EVT        */   {MCA_CCB_CONG,       MCA_CCB_OPEN_ST},
    /* MCA_CCB_RSP_TOUT_EVT       */   {MCA_CCB_RSP_TOUT,   MCA_CCB_OPEN_ST}
};

/* state table for closing state */
const uint8_t mca_ccb_st_closing[][MCA_CCB_NUM_COLS] = {
    /* Event                            Action              Next State */
    /* MCA_CCB_API_CONNECT_EVT    */   {MCA_CCB_IGNORE,     MCA_CCB_CLOSING_ST},
    /* MCA_CCB_API_DISCONNECT_EVT */   {MCA_CCB_IGNORE,     MCA_CCB_CLOSING_ST},
    /* MCA_CCB_API_REQ_EVT        */   {MCA_CCB_FREE_MSG,   MCA_CCB_CLOSING_ST},
    /* MCA_CCB_API_RSP_EVT        */   {MCA_CCB_IGNORE,     MCA_CCB_CLOSING_ST},
    /* MCA_CCB_MSG_REQ_EVT        */   {MCA_CCB_FREE_MSG,   MCA_CCB_CLOSING_ST},
    /* MCA_CCB_MSG_RSP_EVT        */   {MCA_CCB_FREE_MSG,   MCA_CCB_CLOSING_ST},
    /* MCA_CCB_DL_OPEN_EVT        */   {MCA_CCB_IGNORE,     MCA_CCB_CLOSING_ST},
    /* MCA_CCB_LL_OPEN_EVT        */   {MCA_CCB_IGNORE,     MCA_CCB_CLOSING_ST},
    /* MCA_CCB_LL_CLOSE_EVT       */   {MCA_CCB_DEALLOC,    MCA_CCB_NULL_ST},
    /* MCA_CCB_LL_CONG_EVT        */   {MCA_CCB_IGNORE,     MCA_CCB_CLOSING_ST},
    /* MCA_CCB_RSP_TOUT_EVT       */   {MCA_CCB_IGNORE,     MCA_CCB_CLOSING_ST}
};

/* type for state table */
typedef const uint8_t (*tMCA_CCB_ST_TBL)[MCA_CCB_NUM_COLS];

/* state table */
const tMCA_CCB_ST_TBL mca_ccb_st_tbl[] = {
    mca_ccb_st_opening,
    mca_ccb_st_open,
    mca_ccb_st_closing
};

#if (BT_TRACE_VERBOSE == TRUE)
/* verbose event strings for trace */
static const char *const mca_ccb_evt_str[] = {
    "API_CONNECT_EVT",
    "API_DISCONNECT_EVT",
    "API_REQ_EVT",
    "API_RSP_EVT",
    "MSG_REQ_EVT",
    "MSG_RSP_EVT",
    "DL_OPEN_EVT",
    "LL_OPEN_EVT",
    "LL_CLOSE_EVT",
    "LL_CONG_EVT",
    "RSP_TOUT_EVT"
};
/* verbose state strings for trace */
static const char *const mca_ccb_st_str[] = {
    "NULL_ST",
    "OPENING_ST",
    "OPEN_ST",
    "CLOSING_ST"
};
#endif

/*******************************************************************************
**
** Function         mca_stop_timer
**
** Description      This function is stop a MCAP timer
**
**                  This function is for use internal to MCAP only.
**
** Returns          void
**
*******************************************************************************/
void mca_stop_timer(tMCA_CCB *p_ccb)
{
    alarm_cancel(p_ccb->mca_ccb_timer);
}

/*******************************************************************************
**
** Function         mca_ccb_event
**
** Description      This function is the CCB state machine main function.
**                  It uses the state and action function tables to execute
**                  action functions.
**
** Returns          void.
**
*******************************************************************************/
void mca_ccb_event(tMCA_CCB *p_ccb, uint8_t event, tMCA_CCB_EVT *p_data)
{
    tMCA_CCB_ST_TBL    state_table;
    uint8_t              action;
#if (BT_TRACE_VERBOSE == TRUE)
    MCA_TRACE_EVENT("CCB ccb=%d event=%s state=%s", mca_ccb_to_hdl(p_ccb), mca_ccb_evt_str[event],
                    mca_ccb_st_str[p_ccb->state]);
#else
    MCA_TRACE_EVENT("CCB ccb=%d event=%d state=%d", mca_ccb_to_hdl(p_ccb), event, p_ccb->state);
#endif
    /* look up the state table for the current state */
    state_table = mca_ccb_st_tbl[p_ccb->state - 1];
    /* set next state */
    p_ccb->state = state_table[event][MCA_CCB_NEXT_STATE];

    /* execute action functions */
    if((action = state_table[event][MCA_CCB_ACT_COL]) != MCA_CCB_IGNORE) {
        (*mca_ccb_action[action])(p_ccb, p_data);
    }
}

/*******************************************************************************
**
** Function         mca_ccb_by_bd
**
** Description      This function looks up the CCB based on the BD address.
**                  It returns a pointer to the CCB.
**                  If no CCB is found it returns NULL.
**
** Returns          void.
**
*******************************************************************************/
tMCA_CCB *mca_ccb_by_bd(tMCA_HANDLE handle, BD_ADDR bd_addr)
{
    tMCA_CCB *p_ccb = NULL;
    tMCA_RCB *p_rcb = mca_rcb_by_handle(handle);
    tMCA_CCB *p_ccb_tmp;
    int       i;

    if(p_rcb) {
        i = handle - 1;
        p_ccb_tmp = &mca_cb.ccb[i * MCA_NUM_LINKS];

        for(i = 0; i < MCA_NUM_LINKS; i++, p_ccb_tmp++) {
            if(p_ccb_tmp->state != MCA_CCB_NULL_ST && memcmp(p_ccb_tmp->peer_addr, bd_addr, BD_ADDR_LEN) == 0) {
                p_ccb = p_ccb_tmp;
                break;
            }
        }
    }

    return p_ccb;
}

/*******************************************************************************
**
** Function         mca_ccb_alloc
**
** Description      This function allocates a CCB and copies the BD address to
**                  the CCB.  It returns a pointer to the CCB.  If no CCB can
**                  be allocated it returns NULL.
**
** Returns          void.
**
*******************************************************************************/
tMCA_CCB *mca_ccb_alloc(tMCA_HANDLE handle, BD_ADDR bd_addr)
{
    tMCA_CCB *p_ccb = NULL;
    tMCA_RCB *p_rcb = mca_rcb_by_handle(handle);
    tMCA_CCB *p_ccb_tmp;
    int       i;
    MCA_TRACE_DEBUG("mca_ccb_alloc handle:0x%x", handle);

    if(p_rcb) {
        i = handle - 1;
        p_ccb_tmp = &mca_cb.ccb[i * MCA_NUM_LINKS];

        for(i = 0; i < MCA_NUM_LINKS; i++, p_ccb_tmp++) {
            if(p_ccb_tmp->state == MCA_CCB_NULL_ST) {
                p_ccb_tmp->p_rcb = p_rcb;
                p_ccb_tmp->mca_ccb_timer = alarm_new("mca.mca_ccb_timer");
                p_ccb_tmp->state = MCA_CCB_OPENING_ST;
                p_ccb_tmp->cong  = TRUE;
                wm_memcpy(p_ccb_tmp->peer_addr, bd_addr, BD_ADDR_LEN);
                p_ccb = p_ccb_tmp;
                break;
            }
        }
    }

    return p_ccb;
}


/*******************************************************************************
**
** Function         mca_ccb_dealloc
**
** Description      This function deallocates a CCB.
**
** Returns          void.
**
*******************************************************************************/
void mca_ccb_dealloc(tMCA_CCB *p_ccb, tMCA_CCB_EVT *p_data)
{
    tMCA_CTRL   evt_data;
    MCA_TRACE_DEBUG("mca_ccb_dealloc ctrl_vpsm:0x%x", p_ccb->ctrl_vpsm);
    mca_dcb_close_by_mdl_id(p_ccb, MCA_ALL_MDL_ID);

    if(p_ccb->ctrl_vpsm) {
        L2CA_Deregister(p_ccb->ctrl_vpsm);
    }

    if(p_ccb->data_vpsm) {
        L2CA_Deregister(p_ccb->data_vpsm);
    }

    GKI_free_and_reset_buf((void **)&p_ccb->p_rx_msg);
    GKI_free_and_reset_buf((void **)&p_ccb->p_tx_req);
    mca_stop_timer(p_ccb);

    if(p_data) {
        /* non-NULL -> an action function -> report disconnect event */
        wm_memcpy(evt_data.disconnect_ind.bd_addr, p_ccb->peer_addr, BD_ADDR_LEN);
        evt_data.disconnect_ind.reason = p_data->close.reason;
        mca_ccb_report_event(p_ccb, MCA_DISCONNECT_IND_EVT, &evt_data);
    }

    mca_free_tc_tbl_by_lcid(p_ccb->lcid);
    alarm_free(p_ccb->mca_ccb_timer);
    wm_memset(p_ccb, 0, sizeof(tMCA_CCB));
}

/*******************************************************************************
**
** Function         mca_ccb_to_hdl
**
** Description      This function converts a pointer to a CCB to a tMCA_CL
**                  and returns the value.
**
** Returns          void.
**
*******************************************************************************/
tMCA_CL mca_ccb_to_hdl(tMCA_CCB *p_ccb)
{
    return (uint8_t)(p_ccb - mca_cb.ccb + 1);
}

/*******************************************************************************
**
** Function         mca_ccb_by_hdl
**
** Description      This function converts an index value to a CCB.  It returns
**                  a pointer to the CCB.  If no valid CCB matches the index it
**                  returns NULL.
**
** Returns          void.
**
*******************************************************************************/
tMCA_CCB *mca_ccb_by_hdl(tMCA_CL mcl)
{
    tMCA_CCB *p_ccb = NULL;

    if(mcl && mcl <= MCA_NUM_CCBS && mca_cb.ccb[mcl - 1].state) {
        p_ccb = &mca_cb.ccb[mcl - 1];
    }

    return p_ccb;
}


/*******************************************************************************
**
** Function         mca_ccb_uses_mdl_id
**
** Description      This function checkes if a given mdl_id is in use.
**
** Returns          TRUE, if the given mdl_id is currently used in the MCL.
**
*******************************************************************************/
uint8_t mca_ccb_uses_mdl_id(tMCA_CCB *p_ccb, uint16_t mdl_id)
{
    uint8_t uses = FALSE;
    tMCA_DCB *p_dcb;
    int       i;
    i = mca_ccb_to_hdl(p_ccb) - 1;
    p_dcb = &mca_cb.dcb[i * MCA_NUM_MDLS];

    for(i = 0; i < MCA_NUM_MDLS; i++, p_dcb++) {
        if(p_dcb->state != MCA_DCB_NULL_ST && p_dcb->mdl_id == mdl_id) {
            uses = TRUE;
            break;
        }
    }

    return uses;
}
#endif
