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
 *  This is the implementation file for the MCAP Data chahnel state machine.
 *
 ******************************************************************************/
#include <string.h>

#include "bt_target.h"
#if defined(MCA_INCLUDED) &&(MCA_INCLUDED == TRUE)
#include "mca_api.h"
#include "mca_defs.h"
#include "mca_int.h"

/*****************************************************************************
** data channel state machine constants and types
*****************************************************************************/
enum {
    MCA_DCB_TC_OPEN,
    MCA_DCB_CONG,
    MCA_DCB_FREE_DATA,
    MCA_DCB_DEALLOC,
    MCA_DCB_DO_DISCONN,
    MCA_DCB_SND_DATA,
    MCA_DCB_HDL_DATA,
    MCA_DCB_NUM_ACTIONS
};
#define MCA_DCB_IGNORE     MCA_DCB_NUM_ACTIONS

/* action function list */
const tMCA_DCB_ACTION mca_dcb_action[] = {
    mca_dcb_tc_open,
    mca_dcb_cong,
    mca_dcb_free_data,
    mca_dcb_dealloc,
    mca_dcb_do_disconn,
    mca_dcb_snd_data,
    mca_dcb_hdl_data
};

/* state table information */
#define MCA_DCB_ACTIONS            1       /* number of actions */
#define MCA_DCB_ACT_COL            0       /* position of action function */
#define MCA_DCB_NEXT_STATE         1       /* position of next state */
#define MCA_DCB_NUM_COLS           2       /* number of columns in state tables */

/* state table for opening state */
const uint8_t mca_dcb_st_opening[][MCA_DCB_NUM_COLS] = {
    /* Event                            Action              Next State */
    /* MCA_DCB_API_CLOSE_EVT    */   {MCA_DCB_DO_DISCONN,   MCA_DCB_CLOSING_ST},
    /* MCA_DCB_API_WRITE_EVT    */   {MCA_DCB_IGNORE,       MCA_DCB_OPENING_ST},
    /* MCA_DCB_TC_OPEN_EVT      */   {MCA_DCB_TC_OPEN,      MCA_DCB_OPEN_ST},
    /* MCA_DCB_TC_CLOSE_EVT     */   {MCA_DCB_DEALLOC,      MCA_DCB_NULL_ST},
    /* MCA_DCB_TC_CONG_EVT      */   {MCA_DCB_CONG,         MCA_DCB_OPENING_ST},
    /* MCA_DCB_TC_DATA_EVT      */   {MCA_DCB_FREE_DATA,    MCA_DCB_OPENING_ST}
};

/* state table for open state */
const uint8_t mca_dcb_st_open[][MCA_DCB_NUM_COLS] = {
    /* Event                            Action              Next State */
    /* MCA_DCB_API_CLOSE_EVT    */   {MCA_DCB_DO_DISCONN,   MCA_DCB_CLOSING_ST},
    /* MCA_DCB_API_WRITE_EVT    */   {MCA_DCB_SND_DATA,     MCA_DCB_OPEN_ST},
    /* MCA_DCB_TC_OPEN_EVT      */   {MCA_DCB_IGNORE,       MCA_DCB_OPEN_ST},
    /* MCA_DCB_TC_CLOSE_EVT     */   {MCA_DCB_DEALLOC,      MCA_DCB_NULL_ST},
    /* MCA_DCB_TC_CONG_EVT      */   {MCA_DCB_CONG,         MCA_DCB_OPEN_ST},
    /* MCA_DCB_TC_DATA_EVT      */   {MCA_DCB_HDL_DATA,     MCA_DCB_OPEN_ST}
};

/* state table for closing state */
const uint8_t mca_dcb_st_closing[][MCA_DCB_NUM_COLS] = {
    /* Event                            Action              Next State */
    /* MCA_DCB_API_CLOSE_EVT    */   {MCA_DCB_IGNORE,       MCA_DCB_CLOSING_ST},
    /* MCA_DCB_API_WRITE_EVT    */   {MCA_DCB_IGNORE,       MCA_DCB_CLOSING_ST},
    /* MCA_DCB_TC_OPEN_EVT      */   {MCA_DCB_TC_OPEN,      MCA_DCB_OPEN_ST},
    /* MCA_DCB_TC_CLOSE_EVT     */   {MCA_DCB_DEALLOC,      MCA_DCB_NULL_ST},
    /* MCA_DCB_TC_CONG_EVT      */   {MCA_DCB_IGNORE,       MCA_DCB_CLOSING_ST},
    /* MCA_DCB_TC_DATA_EVT      */   {MCA_DCB_FREE_DATA,    MCA_DCB_CLOSING_ST}
};

/* type for state table */
typedef const uint8_t (*tMCA_DCB_ST_TBL)[MCA_DCB_NUM_COLS];

/* state table */
const tMCA_DCB_ST_TBL mca_dcb_st_tbl[] = {
    mca_dcb_st_opening,
    mca_dcb_st_open,
    mca_dcb_st_closing
};

#if (BT_TRACE_VERBOSE == TRUE)
/* verbose event strings for trace */
const char *const mca_dcb_evt_str[] = {
    "API_CLOSE_EVT",
    "API_WRITE_EVT",
    "TC_OPEN_EVT",
    "TC_CLOSE_EVT",
    "TC_CONG_EVT",
    "TC_DATA_EVT"
};
/* verbose state strings for trace */
const char *const mca_dcb_st_str[] = {
    "NULL_ST",
    "OPENING_ST",
    "OPEN_ST",
    "CLOSING_ST"
};
#endif

/*******************************************************************************
**
** Function         mca_dcb_event
**
** Description      This function is the DCB state machine main function.
**                  It uses the state and action function tables to execute
**                  action functions.
**
** Returns          void.
**
*******************************************************************************/
void mca_dcb_event(tMCA_DCB *p_dcb, uint8_t event, tMCA_DCB_EVT *p_data)
{
    tMCA_DCB_ST_TBL    state_table;
    uint8_t              action;

    if(p_dcb == NULL) {
        return;
    }

#if (BT_TRACE_VERBOSE == TRUE)
    MCA_TRACE_EVENT("DCB dcb=%d event=%s state=%s", mca_dcb_to_hdl(p_dcb), mca_dcb_evt_str[event],
                    mca_dcb_st_str[p_dcb->state]);
#else
    MCA_TRACE_EVENT("DCB dcb=%d event=%d state=%d", mca_dcb_to_hdl(p_dcb), event, p_dcb->state);
#endif
    /* look up the state table for the current state */
    state_table = mca_dcb_st_tbl[p_dcb->state - 1];
    /* set next state */
    p_dcb->state = state_table[event][MCA_DCB_NEXT_STATE];

    /* execute action functions */
    if((action = state_table[event][MCA_DCB_ACT_COL]) != MCA_DCB_IGNORE) {
        (*mca_dcb_action[action])(p_dcb, p_data);
    }
}

/*******************************************************************************
**
** Function         mca_dcb_alloc
**
** Description      This function is called to allocate an DCB.
**                  It initializes the DCB with the data passed to the function.
**
** Returns          tMCA_DCB *
**
*******************************************************************************/
tMCA_DCB *mca_dcb_alloc(tMCA_CCB *p_ccb, tMCA_DEP dep)
{
    tMCA_DCB *p_dcb = NULL, *p_dcb_tmp;
    tMCA_RCB *p_rcb = p_ccb->p_rcb;
    tMCA_CS  *p_cs;
    int       i, max;

    if(dep < MCA_NUM_DEPS) {
        p_cs = &p_rcb->dep[dep];
        i = mca_ccb_to_hdl(p_ccb) - 1;
        p_dcb_tmp = &mca_cb.dcb[i * MCA_NUM_MDLS];
        /* make sure p_cs->max_mdl is smaller than MCA_NUM_MDLS at MCA_CreateDep */
        max = p_cs->max_mdl;

        for(i = 0; i < max; i++, p_dcb_tmp++) {
            if(p_dcb_tmp->state == MCA_DCB_NULL_ST) {
                p_dcb_tmp->p_ccb = p_ccb;
                p_dcb_tmp->state = MCA_DCB_OPENING_ST;
                p_dcb_tmp->cong  = TRUE;
                p_dcb_tmp->p_cs  = p_cs;
                p_dcb = p_dcb_tmp;
                break;
            }
        }
    }

    return p_dcb;
}

/*******************************************************************************
**
** Function         mca_dep_free_mdl
**
** Description      This function is called to check the number of free mdl for
**                  the given dep.
**
** Returns          the number of free mdl for the given dep
**
*******************************************************************************/
uint8_t mca_dep_free_mdl(tMCA_CCB *p_ccb, tMCA_DEP dep)
{
    tMCA_DCB *p_dcb;
    tMCA_RCB *p_rcb = p_ccb->p_rcb;
    tMCA_CS  *p_cs;
    int       i, max;
    uint8_t   count = 0;
    uint8_t   left;

    if(dep < MCA_NUM_DEPS) {
        p_cs = &p_rcb->dep[dep];
        i = mca_ccb_to_hdl(p_ccb) - 1;
        p_dcb = &mca_cb.dcb[i * MCA_NUM_MDLS];
        /* make sure p_cs->max_mdl is smaller than MCA_NUM_MDLS at MCA_CreateDep */
        max = p_cs->max_mdl;

        for(i = 0; i < max; i++, p_dcb++) {
            if((p_dcb->state != MCA_DCB_NULL_ST) && (p_dcb->p_cs == p_cs)) {
                count++;
                break;
            }
        }
    } else {
        max = 0;
        MCA_TRACE_WARNING("Invalid Dep ID");
    }

    left = max - count;
    return left;
}

/*******************************************************************************
**
** Function         mca_dcb_dealloc
**
** Description      This function deallocates an DCB.
**
** Returns          void.
**
*******************************************************************************/
void mca_dcb_dealloc(tMCA_DCB *p_dcb, tMCA_DCB_EVT *p_data)
{
    tMCA_CCB *p_ccb = p_dcb->p_ccb;
    uint8_t    event = MCA_CLOSE_IND_EVT;
    tMCA_CTRL   evt_data;
    MCA_TRACE_DEBUG("mca_dcb_dealloc");
    GKI_free_and_reset_buf((void **)&p_dcb->p_data);

    if(p_data) {
        /* non-NULL -> an action function -> report disconnect event */
        evt_data.close_cfm.mdl      = mca_dcb_to_hdl(p_dcb);
        evt_data.close_cfm.reason   = p_data->close.reason;
        evt_data.close_cfm.mdl_id   = p_dcb->mdl_id;

        if(p_data->close.param == MCA_INT) {
            event = MCA_CLOSE_CFM_EVT;
        }

        if(p_data->close.lcid) {
            mca_ccb_report_event(p_ccb, event, &evt_data);
        }
    }

    mca_free_tc_tbl_by_lcid(p_dcb->lcid);
    wm_memset(p_dcb, 0, sizeof(tMCA_DCB));
}

/*******************************************************************************
**
** Function         mca_dcb_to_hdl
**
** Description      This function converts a pointer to an DCB to a handle (tMCA_DL).
**                  It returns the handle.
**
** Returns          tMCA_DL.
**
*******************************************************************************/
tMCA_DL mca_dcb_to_hdl(tMCA_DCB *p_dcb)
{
    return (uint8_t)(p_dcb - mca_cb.dcb + 1);
}

/*******************************************************************************
**
** Function         mca_dcb_by_hdl
**
** Description      This function finds the DCB for a handle (tMCA_DL).
**                  It returns a pointer to the DCB.
**                  If no DCB matches the handle it returns NULL.
**
** Returns          tMCA_DCB *
**
*******************************************************************************/
tMCA_DCB *mca_dcb_by_hdl(tMCA_DL hdl)
{
    tMCA_DCB *p_dcb = NULL;

    if(hdl && hdl <= MCA_NUM_DCBS && mca_cb.dcb[hdl - 1].state) {
        p_dcb = &mca_cb.dcb[hdl - 1];
    }

    return p_dcb;
}

/*******************************************************************************
**
** Function         mca_dcb_close_by_mdl_id
**
** Description      This function finds the DCB for a mdl_id and
**                  disconnect the mdl
**
** Returns          void
**
*******************************************************************************/
void mca_dcb_close_by_mdl_id(tMCA_CCB *p_ccb, uint16_t mdl_id)
{
    tMCA_DCB *p_dcb;
    int       i;
    MCA_TRACE_DEBUG("mca_dcb_close_by_mdl_id mdl_id=%d", mdl_id);
    i = mca_ccb_to_hdl(p_ccb) - 1;
    p_dcb = &mca_cb.dcb[i * MCA_NUM_MDLS];

    for(i = 0; i < MCA_NUM_MDLS; i++, p_dcb++) {
        if(p_dcb->state) {
            if(p_dcb->mdl_id == mdl_id) {
                mca_dcb_event(p_dcb, MCA_DCB_API_CLOSE_EVT, NULL);
                break;
            } else if(mdl_id == MCA_ALL_MDL_ID) {
                mca_dcb_event(p_dcb, MCA_DCB_API_CLOSE_EVT, NULL);
            }
        }
    }
}
#endif
