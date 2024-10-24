/******************************************************************************
 *
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
 *  This file contains the HeaLth device profile (HL) action functions for
 *  the state machine.
 *
 ******************************************************************************/

#include <string.h>

#include "bt_target.h"
#if defined(BTA_HL_INCLUDED) && (BTA_HL_INCLUDED == TRUE)

#include "bt_common.h"
#include "sdp_api.h"
#include "bta_sys.h"
#include "port_api.h"
#include "sdp_api.h"
#include "bta_hl_api.h"
#include "bta_hl_int.h"
#include "utl.h"
#include "mca_defs.h"
#include "mca_api.h"

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
#if (BTA_HL_DEBUG == TRUE && BT_TRACE_VERBOSE == TRUE)
static char *bta_hl_mcap_evt_code(uint8_t evt_code);
static char *bta_hl_dch_oper_code(tBTA_HL_DCH_OPER oper_code);
static char *bta_hl_cback_evt_code(uint8_t evt_code);
#endif
static void bta_hl_sdp_cback(uint8_t sdp_op, uint8_t app_idx, uint8_t mcl_idx,
                             uint8_t mdl_idx, uint16_t status);
static void bta_hl_sdp_cback0(uint16_t status);
static void bta_hl_sdp_cback1(uint16_t status);
static void bta_hl_sdp_cback2(uint16_t status);
static void bta_hl_sdp_cback3(uint16_t status);
static void bta_hl_sdp_cback4(uint16_t status);
static void bta_hl_sdp_cback5(uint16_t status);
static void bta_hl_sdp_cback6(uint16_t status);


static tSDP_DISC_CMPL_CB *const bta_hl_sdp_cback_arr[] = {
    bta_hl_sdp_cback0,
    bta_hl_sdp_cback1,
    bta_hl_sdp_cback2,
    bta_hl_sdp_cback3,
    bta_hl_sdp_cback4,
    bta_hl_sdp_cback5,
    bta_hl_sdp_cback6
};



/*******************************************************************************
**
** Function         bta_hl_dch_mca_cong_change
**
** Description      Action routine for processing congestion change notification
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_cong_change(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tMCA_CONG_CHG       *p_cong_chg = &p_data->mca_evt.mca_data.cong_chg;
    tBTA_HL             evt_data;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_mca_cong_change mdl_id=%d cong=%d",
                     p_cong_chg->mdl_id,
                     p_cong_chg->cong);
#endif
    evt_data.dch_cong_ind.cong          =
                    p_dcb->cong                         = p_cong_chg->cong;
    evt_data.dch_cong_ind.mdl_handle    = p_dcb->mdl_handle;
    evt_data.dch_cong_ind.mcl_handle    = p_mcb->mcl_handle;
    evt_data.dch_cong_ind.app_handle    = p_acb->app_handle;
    p_acb->p_cback(BTA_HL_CONG_CHG_IND_EVT, (tBTA_HL *) &evt_data);
}



/*******************************************************************************
**
** Function         bta_hl_dch_echo_test
**
** Description      Action routine for processing echo test request
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_echo_test(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                          tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    UNUSED(p_data);
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_echo_test");
#endif
    p_dcb->echo_oper = BTA_HL_ECHO_OP_CI_GET_ECHO_DATA;
    p_dcb->cout_oper |= BTA_HL_CO_GET_ECHO_DATA_MASK;
    bta_hl_co_get_echo_data(p_acb->app_id, p_mcb->mcl_handle,
                            p_dcb->p_echo_tx_pkt->len,
                            BTA_HL_GET_BUF_PTR(p_dcb->p_echo_tx_pkt),
                            BTA_HL_CI_GET_ECHO_DATA_EVT);
}
/*******************************************************************************
**
** Function         bta_hl_dch_sdp_init
**
** Description      Action routine for processing DCH SDP initiation
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_sdp_init(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                         tBTA_HL_DATA *p_data)
{
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_sdp_init");
#endif

    if(p_mcb->sdp_oper == BTA_HL_SDP_OP_NONE) {
        p_mcb->sdp_mdl_idx = mdl_idx;

        if(p_dcb->dch_oper == BTA_HL_DCH_OP_LOCAL_OPEN) {
            p_mcb->sdp_oper = BTA_HL_SDP_OP_DCH_OPEN_INIT;
        } else {
            p_mcb->sdp_oper = BTA_HL_SDP_OP_DCH_RECONNECT_INIT;
        }

        if(bta_hl_init_sdp(p_mcb->sdp_oper, app_idx, mcl_idx, mdl_idx) != BTA_HL_STATUS_OK) {
            APPL_TRACE_ERROR("SDP INIT failed");
            p_mcb->sdp_oper = BTA_HL_SDP_OP_NONE;
            bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_SDP_FAIL_EVT, p_data);
        }
    } else {
        APPL_TRACE_ERROR("SDP in use");
        bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_SDP_FAIL_EVT, p_data);
    }
}


/*******************************************************************************
**
** Function         bta_hl_dch_close_echo_test
**
** Description      Action routine for processing the closing of echo test
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_close_echo_test(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                tBTA_HL_DATA *p_data)
{
    tBTA_HL_MDL_CB          *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_close_echo_test");
#endif

    switch(p_dcb->echo_oper) {
        case BTA_HL_ECHO_OP_DCH_CLOSE_CFM:
        case BTA_HL_ECHO_OP_OPEN_IND:
        case BTA_HL_ECHO_OP_ECHO_PKT:
            p_dcb->dch_oper = BTA_HL_DCH_OP_LOCAL_CLOSE_ECHO_TEST;
            break;

        case BTA_HL_ECHO_OP_MDL_CREATE_CFM:
        case BTA_HL_ECHO_OP_DCH_OPEN_CFM:
        case BTA_HL_ECHO_OP_LOOP_BACK:
        default:
            break;
    }

    if(MCA_CloseReq((tMCA_DL) p_dcb->mdl_handle) != MCA_SUCCESS) {
        bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
    }
}


/*******************************************************************************
**
** Function         bta_hl_dch_mca_rcv_data
**
** Description      Action routine for processing the received data
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_rcv_data(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                             tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_mca_rcv_data");
#endif

    if(p_dcb->local_mdep_id == BTA_HL_ECHO_TEST_MDEP_ID) {
        switch(p_dcb->echo_oper) {
            case  BTA_HL_ECHO_OP_ECHO_PKT:
                if(MCA_WriteReq((tMCA_DL) p_dcb->mdl_handle, p_data->mca_rcv_data_evt.p_pkt) != MCA_SUCCESS) {
                    GKI_free_and_reset_buf((void **)&p_data->mca_rcv_data_evt.p_pkt);
                    bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_ECHO_TEST_EVT, p_data);
                }

                break;

            case BTA_HL_ECHO_OP_LOOP_BACK:
                p_dcb->p_echo_rx_pkt = p_data->mca_rcv_data_evt.p_pkt;
                p_dcb->echo_oper = BTA_HL_ECHO_OP_CI_PUT_ECHO_DATA;
                p_dcb->cout_oper |= BTA_HL_CO_PUT_ECHO_DATA_MASK;
                p_dcb->ci_put_echo_data_status = BTA_HL_STATUS_FAIL;
                bta_hl_co_put_echo_data(p_acb->app_id, p_mcb->mcl_handle,
                                        p_dcb->p_echo_rx_pkt->len,
                                        BTA_HL_GET_BUF_PTR(p_dcb->p_echo_rx_pkt),
                                        BTA_HL_CI_PUT_ECHO_DATA_EVT);
                break;

            default:
                APPL_TRACE_ERROR("Unknonw echo_oper=%d", p_dcb->echo_oper);
                break;
        }
    } else {
        p_dcb->cout_oper |= BTA_HL_CO_PUT_RX_DATA_MASK;
        p_dcb->p_rx_pkt = p_data->mca_rcv_data_evt.p_pkt;
        bta_hl_co_put_rx_data(p_acb->app_id, p_dcb->mdl_handle,
                              p_dcb->p_rx_pkt->len,
                              BTA_HL_GET_BUF_PTR(p_dcb->p_rx_pkt),
                              BTA_HL_CI_PUT_RX_DATA_EVT);
    }
}


/*******************************************************************************
**
** Function         bta_hl_dch_ci_put_echo_data
**
** Description      Action routine for processing the call-in of the
**                  put echo data event
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_ci_put_echo_data(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                 tBTA_HL_DATA *p_data)
{
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_ci_put_echo_data");
#endif
    p_dcb->cout_oper &= ~BTA_HL_CO_PUT_ECHO_DATA_MASK;
    GKI_free_and_reset_buf((void **)&p_dcb->p_echo_rx_pkt);
    p_dcb->ci_put_echo_data_status = p_data->ci_get_put_echo_data.status;
    p_dcb->echo_oper = BTA_HL_ECHO_OP_DCH_CLOSE_CFM;
    bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_ECHO_TEST_EVT, p_data);
}



/*******************************************************************************
**
** Function         bta_hl_dch_ci_get_echo_data
**
** Description      Action routine for processing the call-in of the
**                  get echo data event
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_ci_get_echo_data(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                 tBTA_HL_DATA *p_data)
{
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tBTA_HL_STATUS      status;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_ci_get_echo_data");
#endif
    p_dcb->cout_oper &= ~BTA_HL_CO_GET_ECHO_DATA_MASK;

    if(!p_dcb->abort_oper) {
        status = p_data->ci_get_put_echo_data.status;

        if(status == BTA_HL_STATUS_OK) {
            p_dcb->echo_oper = BTA_HL_ECHO_OP_MDL_CREATE_CFM;
            bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_OPEN_EVT, p_data);
        } else {
            bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
        }
    } else {
        bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
    }
}

/*******************************************************************************
**
** Function         bta_hl_dch_ci_put_rx_data
**
** Description      Action routine for processing the call-in of the
**                  put rx data event
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_ci_put_rx_data(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                               tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tBTA_HL             evt_data;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_ci_put_rx_data");
#endif
    p_dcb->cout_oper &= ~BTA_HL_CO_PUT_RX_DATA_MASK;
    GKI_free_and_reset_buf((void **)&p_dcb->p_rx_pkt);
    bta_hl_build_rcv_data_ind(&evt_data,
                              p_acb->app_handle,
                              p_mcb->mcl_handle,
                              p_dcb->mdl_handle);
    p_acb->p_cback(BTA_HL_DCH_RCV_DATA_IND_EVT, (tBTA_HL *) &evt_data);

    if(p_dcb->close_pending) {
        if(!p_dcb->cout_oper) {
            bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_EVT, p_data);
        }
    }
}



/*******************************************************************************
**
** Function         bta_hl_dch_ci_get_tx_data
**
** Description      Action routine for processing the call-in of the
**                  get tx data event
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_ci_get_tx_data(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                               tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tMCA_RESULT         result;
    tBTA_HL_STATUS      status = BTA_HL_STATUS_OK;
    uint8_t             free_buf = FALSE;
    uint8_t             close_dch = FALSE;
    tBTA_HL             evt_data;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_ci_get_tx_data");
#endif
    p_dcb->cout_oper &= ~BTA_HL_CO_GET_TX_DATA_MASK;

    if(p_dcb->close_pending) {
        status = BTA_HL_STATUS_FAIL;
        free_buf = TRUE;

        if(!p_dcb->cout_oper) {
            close_dch = TRUE;
        }
    } else {
        if((result = MCA_WriteReq((tMCA_DL) p_dcb->mdl_handle, p_dcb->p_tx_pkt)) != MCA_SUCCESS) {
            if(result == MCA_BUSY) {
                status = BTA_HL_STATUS_DCH_BUSY;
            } else {
                status = BTA_HL_STATUS_FAIL;
            }

            free_buf = TRUE;
        } else {
            p_dcb->p_tx_pkt = NULL;
        }
    }

    if(free_buf) {
        GKI_free_and_reset_buf((void **)&p_dcb->p_tx_pkt);
    }

    bta_hl_build_send_data_cfm(&evt_data,
                               p_acb->app_handle,
                               p_mcb->mcl_handle,
                               p_dcb->mdl_handle,
                               status);
    p_acb->p_cback(BTA_HL_DCH_SEND_DATA_CFM_EVT, (tBTA_HL *) &evt_data);

    if(close_dch) {
        bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_EVT, p_data);
    }
}


/*******************************************************************************
**
** Function         bta_hl_dch_send_data
**
** Description      Action routine for processing api send data request
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_send_data(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                          tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tBTA_HL             evt_data;
    uint8_t             success = TRUE;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_send_data");
#endif

    if(!(p_dcb->cout_oper & BTA_HL_CO_GET_TX_DATA_MASK)) {
        // p_dcb->chnl_cfg.fcs may be BTA_HL_MCA_USE_FCS (0x11) or BTA_HL_MCA_NO_FCS (0x10) or BTA_HL_DEFAULT_SOURCE_FCS (1)
        uint8_t fcs_use = (uint8_t)(p_dcb->chnl_cfg.fcs & BTA_HL_MCA_FCS_USE_MASK);

        if((p_dcb->p_tx_pkt = bta_hl_get_buf(p_data->api_send_data.pkt_size, fcs_use)) != NULL) {
            bta_hl_co_get_tx_data(p_acb->app_id,
                                  p_dcb->mdl_handle,
                                  p_data->api_send_data.pkt_size,
                                  BTA_HL_GET_BUF_PTR(p_dcb->p_tx_pkt),
                                  BTA_HL_CI_GET_TX_DATA_EVT);
            p_dcb->cout_oper |= BTA_HL_CO_GET_TX_DATA_MASK;
        } else {
            success = FALSE;
        }
    } else {
        success = FALSE;
    }

    if(!success) {
        bta_hl_build_send_data_cfm(&evt_data,
                                   p_acb->app_handle,
                                   p_mcb->mcl_handle,
                                   p_dcb->mdl_handle,
                                   BTA_HL_STATUS_FAIL);
        p_acb->p_cback(BTA_HL_DCH_SEND_DATA_CFM_EVT, (tBTA_HL *) &evt_data);
    }
}

/*******************************************************************************
**
** Function         bta_hl_dch_close_cmpl
**
** Description      Action routine for processing the close complete event
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_close_cmpl(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                           tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tBTA_HL             evt_data;
    tBTA_HL_EVT         event = 0;
    uint8_t             send_evt = TRUE;
    tBTA_HL_STATUS      status;
#if (BTA_HL_DEBUG == TRUE)
#if (BT_TRACE_VERBOSE == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_close_cmpl dch oper=%s", bta_hl_dch_oper_code(p_dcb->dch_oper));
#else
    APPL_TRACE_DEBUG("bta_hl_dch_close_cmpl dch oper=%d", p_dcb->dch_oper);
#endif
#endif

    switch(p_dcb->dch_oper) {
        case BTA_HL_DCH_OP_LOCAL_OPEN:
        case BTA_HL_DCH_OP_LOCAL_RECONNECT:
            if(p_dcb->abort_oper & BTA_HL_ABORT_LOCAL_MASK) {
                bta_hl_build_abort_cfm(&evt_data,
                                       p_acb->app_handle,
                                       p_mcb->mcl_handle,
                                       BTA_HL_STATUS_OK);
                event = BTA_HL_DCH_ABORT_CFM_EVT;
            } else if(p_dcb->abort_oper & BTA_HL_ABORT_REMOTE_MASK) {
                bta_hl_build_abort_ind(&evt_data,
                                       p_acb->app_handle,
                                       p_mcb->mcl_handle);
                event = BTA_HL_DCH_ABORT_IND_EVT;
            } else {
                bta_hl_build_dch_open_cfm(&evt_data,
                                          p_acb->app_handle,
                                          p_mcb->mcl_handle,
                                          BTA_HL_INVALID_MDL_HANDLE,
                                          0, 0, 0, 0, 0, BTA_HL_STATUS_FAIL);
                event = BTA_HL_DCH_OPEN_CFM_EVT;

                if(p_dcb->dch_oper == BTA_HL_DCH_OP_LOCAL_RECONNECT) {
                    event = BTA_HL_DCH_RECONNECT_CFM_EVT;
                }
            }

            break;

        case BTA_HL_DCH_OP_LOCAL_CLOSE:
        case BTA_HL_DCH_OP_REMOTE_DELETE:
        case BTA_HL_DCH_OP_LOCAL_CLOSE_RECONNECT:
        case BTA_HL_DCH_OP_NONE:
            bta_hl_build_dch_close_cfm(&evt_data,
                                       p_acb->app_handle,
                                       p_mcb->mcl_handle,
                                       p_dcb->mdl_handle,
                                       BTA_HL_STATUS_OK);
            event = BTA_HL_DCH_CLOSE_CFM_EVT;
            break;

        case BTA_HL_DCH_OP_REMOTE_CLOSE:
            bta_hl_build_dch_close_ind(&evt_data,
                                       p_acb->app_handle,
                                       p_mcb->mcl_handle,
                                       p_dcb->mdl_handle,
                                       p_dcb->intentional_close);
            event = BTA_HL_DCH_CLOSE_IND_EVT;
            break;

        case BTA_HL_DCH_OP_REMOTE_OPEN:
            if(p_dcb->abort_oper & BTA_HL_ABORT_LOCAL_MASK) {
                bta_hl_build_abort_cfm(&evt_data,
                                       p_acb->app_handle,
                                       p_mcb->mcl_handle,
                                       BTA_HL_STATUS_OK);
                event = BTA_HL_DCH_ABORT_CFM_EVT;
            } else if(p_dcb->abort_oper & BTA_HL_ABORT_REMOTE_MASK) {
                bta_hl_build_abort_ind(&evt_data,
                                       p_acb->app_handle,
                                       p_mcb->mcl_handle);
                event = BTA_HL_DCH_ABORT_IND_EVT;
            } else {
                bta_hl_build_dch_close_ind(&evt_data,
                                           p_acb->app_handle,
                                           p_mcb->mcl_handle,
                                           p_dcb->mdl_handle,
                                           p_dcb->intentional_close);
                event = BTA_HL_DCH_CLOSE_IND_EVT;
            }

            break;

        case BTA_HL_DCH_OP_LOCAL_CLOSE_ECHO_TEST:

        /* this is normal echo test close */
        case BTA_HL_DCH_OP_REMOTE_CREATE:
        case BTA_HL_DCH_OP_REMOTE_RECONNECT:
            send_evt = FALSE;
            break;

        default:
#if (BTA_HL_DEBUG == TRUE)
#if (BT_TRACE_VERBOSE == TRUE)
            APPL_TRACE_ERROR("DCH operation not found oper=%s", bta_hl_dch_oper_code(p_dcb->dch_oper));
#else
            APPL_TRACE_ERROR("DCH operation not found oper=%d", p_dcb->dch_oper);
#endif
#endif
            send_evt = FALSE;
            break;
    }

    if(p_dcb->local_mdep_id == BTA_HL_ECHO_TEST_MDEP_ID) {
        p_mcb->echo_test = FALSE;
        send_evt = FALSE;

        if(p_dcb->dch_oper != BTA_HL_DCH_OP_LOCAL_CLOSE_ECHO_TEST) {
            switch(p_dcb->echo_oper) {
                case BTA_HL_ECHO_OP_CI_GET_ECHO_DATA:
                case BTA_HL_ECHO_OP_SDP_INIT:
                case BTA_HL_ECHO_OP_MDL_CREATE_CFM:
                case BTA_HL_ECHO_OP_DCH_OPEN_CFM:
                case BTA_HL_ECHO_OP_LOOP_BACK:
                    status = BTA_HL_STATUS_FAIL;
                    send_evt = TRUE;
                    break;

                case BTA_HL_ECHO_OP_OPEN_IND:
                case BTA_HL_ECHO_OP_ECHO_PKT:
                    break;

                default:
                    APPL_TRACE_ERROR("Invalid echo_oper=%d", p_dcb->echo_oper);
                    break;
            }
        } else {
            status = p_dcb->ci_put_echo_data_status;
            send_evt = TRUE;
        }

        if(send_evt) {
            bta_hl_build_echo_test_cfm(&evt_data,
                                       p_acb->app_handle,
                                       p_mcb->mcl_handle,
                                       status);
            event = BTA_HL_DCH_ECHO_TEST_CFM_EVT;
        }
    }

    bta_hl_clean_mdl_cb(app_idx, mcl_idx, mdl_idx);

    if(send_evt) {
        if(p_acb->p_cback) {
#if (BTA_HL_DEBUG == TRUE)
#if (BT_TRACE_VERBOSE == TRUE)
            APPL_TRACE_DEBUG("Send Event: %s",  bta_hl_cback_evt_code(event));
#else
            APPL_TRACE_DEBUG("Send Event: 0x%02x", event);
#endif
#endif
            p_acb->p_cback(event, (tBTA_HL *) &evt_data);
        }
    }

    /* check cch close is in progress or not */
    bta_hl_check_cch_close(app_idx, mcl_idx, p_data, FALSE);
}
/*******************************************************************************
**
** Function         bta_hl_dch_mca_close_ind
**
** Description      Action routine for processing the close indication
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_close_ind(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                              tBTA_HL_DATA *p_data)
{
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
#if (BTA_HL_DEBUG == TRUE)
#if (BT_TRACE_VERBOSE == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_mca_close_ind dch oper=%s", bta_hl_dch_oper_code(p_dcb->dch_oper));
#else
    APPL_TRACE_DEBUG("bta_hl_dch_mca_close_ind dch oper=%d", p_dcb->dch_oper);
#endif
#endif
    p_dcb->intentional_close = FALSE;

    if(p_data->mca_evt.mca_data.close_ind.reason == L2CAP_DISC_OK) {
        p_dcb->intentional_close = TRUE;
    }

    if(!p_dcb->cout_oper) {
        if((p_dcb->dch_oper != BTA_HL_DCH_OP_REMOTE_OPEN) &&
                (p_dcb->dch_oper != BTA_HL_DCH_OP_REMOTE_RECONNECT)) {
            p_dcb->dch_oper = BTA_HL_DCH_OP_REMOTE_CLOSE;
        }

        bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
    } else {
        p_dcb->close_pending = TRUE;
    }
}

/*******************************************************************************
**
** Function         bta_hl_dch_mca_close_cfm
**
** Description      Action routine for processing the close confirmation
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_close_cfm(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                              tBTA_HL_DATA *p_data)
{
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
#if (BTA_HL_DEBUG == TRUE)
#if (BT_TRACE_VERBOSE == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_mca_close_cfm dch_oper=%s", bta_hl_dch_oper_code(p_dcb->dch_oper));
#else
    APPL_TRACE_DEBUG("bta_hl_dch_mca_close_cfm dch_oper=%d", p_dcb->dch_oper);
#endif
#endif

    switch(p_dcb->dch_oper) {
        case BTA_HL_DCH_OP_LOCAL_CLOSE:
        case BTA_HL_DCH_OP_LOCAL_OPEN:
        case BTA_HL_DCH_OP_LOCAL_RECONNECT:
        case BTA_HL_DCH_OP_LOCAL_CLOSE_ECHO_TEST:
        case BTA_HL_DCH_OP_REMOTE_DELETE:
        case BTA_HL_DCH_OP_LOCAL_CLOSE_RECONNECT:
        case BTA_HL_DCH_OP_NONE:
            bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
            break;

        default:
#if (BTA_HL_DEBUG == TRUE)
#if (BT_TRACE_VERBOSE == TRUE)
            APPL_TRACE_ERROR("Invalid dch_oper=%s for close cfm", bta_hl_dch_oper_code(p_dcb->dch_oper));
#else
            APPL_TRACE_ERROR("Invalid dch_oper=%d for close cfm", p_dcb->dch_oper);
#endif
#endif
            break;
    }
}

/*******************************************************************************
**
** Function         bta_hl_dch_mca_close
**
** Description      Action routine for processing the DCH close request
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_close(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                          tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB          *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB          *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB          *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tBTA_HL_STATUS          status = BTA_HL_STATUS_OK;
    tBTA_HL                 evt_data;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_mca_close");
#endif

    if(!p_dcb->cout_oper) {
        p_dcb->close_pending = FALSE;

        if(MCA_CloseReq((tMCA_DL)p_dcb->mdl_handle) == MCA_SUCCESS) {
            p_dcb->dch_oper = BTA_HL_DCH_OP_LOCAL_CLOSE;
        } else {
            status = BTA_HL_STATUS_FAIL;
        }

        if((status != BTA_HL_STATUS_OK) &&
                (p_mcb->cch_close_dch_oper != BTA_HL_CCH_CLOSE_OP_DCH_CLOSE)) {
            bta_hl_build_dch_close_cfm(&evt_data,
                                       p_acb->app_handle,
                                       p_mcb->mcl_handle,
                                       p_data->api_dch_close.mdl_handle,
                                       status);
            p_acb->p_cback(BTA_HL_DCH_CLOSE_CFM_EVT, (tBTA_HL *) &evt_data);
        }
    } else {
        p_dcb->close_pending = TRUE;
    }
}


/*******************************************************************************
**
** Function         bta_hl_dch_mca_open_ind
**
** Description      Action routine for processing the open indication
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_open_ind(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                             tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tMCA_DL_OPEN        *p_open_ind = &p_data->mca_evt.mca_data.open_ind;
    tBTA_HL             evt_data;
    tBTA_HL_EVT         event;
    uint8_t               old_dch_oper = BTA_HL_DCH_OP_NONE;
    uint8_t             send_event = FALSE;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_mca_open_ind");
#endif

    if((p_dcb->dch_oper == BTA_HL_DCH_OP_REMOTE_OPEN) ||
            (p_dcb->dch_oper == BTA_HL_DCH_OP_REMOTE_RECONNECT)) {
        p_dcb->mdl_handle = (tBTA_HL_MDL_HANDLE) p_open_ind->mdl;
        p_dcb->mtu        = p_open_ind->mtu;
        evt_data.dch_open_ind.mdl_handle = p_dcb->mdl_handle;
        evt_data.dch_open_ind.mcl_handle = p_mcb->mcl_handle;
        evt_data.dch_open_ind.app_handle = p_acb->app_handle;
        evt_data.dch_open_ind.local_mdep_id = p_dcb->local_mdep_id;
        evt_data.dch_open_ind.mdl_id = p_dcb->mdl_id;
        evt_data.dch_open_ind.mtu = p_dcb->mtu;

        if(p_dcb->chnl_cfg.fcr_opt.mode == L2CAP_FCR_ERTM_MODE) {
            evt_data.dch_open_ind.dch_mode = BTA_HL_DCH_MODE_RELIABLE;

            if(!bta_hl_is_the_first_reliable_existed(app_idx, mcl_idx)) {
                p_dcb->is_the_first_reliable = TRUE;
            }
        } else {
            evt_data.dch_open_ind.dch_mode = BTA_HL_DCH_MODE_STREAMING;
        }

        evt_data.dch_open_ind.first_reliable = p_dcb->is_the_first_reliable ;
        old_dch_oper = p_dcb->dch_oper;
        p_dcb->dch_oper = BTA_HL_DCH_OP_NONE;
    }

    switch(old_dch_oper) {
        case BTA_HL_DCH_OP_REMOTE_OPEN:
            p_dcb->dch_mode = evt_data.dch_open_ind.dch_mode;

            if(p_dcb->local_mdep_id != BTA_HL_ECHO_TEST_MDEP_ID) {
                bta_hl_save_mdl_cfg(app_idx, mcl_idx, mdl_idx);
                event = BTA_HL_DCH_OPEN_IND_EVT;
                send_event = TRUE;
            } else {
                p_dcb->echo_oper = BTA_HL_ECHO_OP_ECHO_PKT;
            }

            break;

        case BTA_HL_DCH_OP_REMOTE_RECONNECT:
            if(bta_hl_validate_chan_cfg(app_idx, mcl_idx, mdl_idx)) {
                bta_hl_save_mdl_cfg(app_idx, mcl_idx, mdl_idx);
                event = BTA_HL_DCH_RECONNECT_IND_EVT;
                send_event = TRUE;
            } else {
                if(MCA_CloseReq((tMCA_DL) p_dcb->mdl_handle) == MCA_SUCCESS) {
                    p_dcb->dch_oper = BTA_HL_DCH_OP_LOCAL_CLOSE_RECONNECT;
                } else {
                    APPL_TRACE_ERROR("Unabel to close DCH for reconnect cfg mismatch");
                }
            }

            break;

        default:
            break;
    }

    if(send_event) {
        p_acb->p_cback(event, (tBTA_HL *) &evt_data);
    }
}

/*******************************************************************************
**
** Function         bta_hl_dch_mca_open_cfm
**
** Description      Action routine for processing the open confirmation
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_open_cfm(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                             tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tMCA_DL_OPEN        *p_open_cfm = &p_data->mca_evt.mca_data.open_cfm;
    tBTA_HL             evt_data;
    tBTA_HL_EVT         event;
    uint8_t               old_dch_oper = BTA_HL_DCH_OP_NONE;
    tBTA_HL_DCH_MODE    dch_mode = BTA_HL_DCH_MODE_STREAMING;
    uint8_t             send_event = FALSE;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_mca_open_cfm");
#endif

    if((p_dcb->dch_oper == BTA_HL_DCH_OP_LOCAL_OPEN) ||
            (p_dcb->dch_oper == BTA_HL_DCH_OP_LOCAL_RECONNECT)) {
        p_dcb->mdl_handle = (tBTA_HL_MDL_HANDLE) p_open_cfm->mdl;
        p_dcb->mtu        = p_open_cfm->mtu;

        /*todo verify dch_mode, mtu and fcs for reconnect */
        if(p_dcb->chnl_cfg.fcr_opt.mode == L2CAP_FCR_ERTM_MODE) {
            dch_mode = BTA_HL_DCH_MODE_RELIABLE;
        }

        if(p_dcb->local_mdep_id != BTA_HL_ECHO_TEST_MDEP_ID) {
            if(dch_mode == BTA_HL_DCH_MODE_RELIABLE) {
                if(!bta_hl_is_the_first_reliable_existed(app_idx, mcl_idx)) {
                    p_dcb->is_the_first_reliable = TRUE;
                }
            }
        }

        bta_hl_build_dch_open_cfm(&evt_data, p_acb->app_handle,
                                  p_mcb->mcl_handle,
                                  p_dcb->mdl_handle,
                                  p_dcb->local_mdep_id,
                                  p_dcb->mdl_id, dch_mode,
                                  p_dcb->is_the_first_reliable,
                                  p_dcb->mtu,
                                  BTA_HL_STATUS_OK);
        old_dch_oper = p_dcb->dch_oper;
        p_dcb->dch_oper = BTA_HL_DCH_OP_NONE;
    } else {
        APPL_TRACE_ERROR("Error dch oper =%d",  p_dcb->dch_oper);
        return;
    }

    switch(old_dch_oper) {
        case BTA_HL_DCH_OP_LOCAL_OPEN:
            p_dcb->dch_mode = dch_mode;

            if(p_dcb->local_mdep_id != BTA_HL_ECHO_TEST_MDEP_ID) {
                bta_hl_save_mdl_cfg(app_idx, mcl_idx, mdl_idx);
                event = BTA_HL_DCH_OPEN_CFM_EVT;
                send_event = TRUE;
            } else {
                p_dcb->echo_oper = BTA_HL_ECHO_OP_LOOP_BACK;

                if(MCA_WriteReq((tMCA_DL) p_dcb->mdl_handle, p_dcb->p_echo_tx_pkt) != MCA_SUCCESS) {
                    bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_ECHO_TEST_EVT, p_data);
                } else {
                    p_dcb->p_echo_tx_pkt = NULL;
                }
            }

            break;

        case BTA_HL_DCH_OP_LOCAL_RECONNECT:
            if(bta_hl_validate_chan_cfg(app_idx, mcl_idx, mdl_idx)) {
                bta_hl_save_mdl_cfg(app_idx, mcl_idx, mdl_idx);
                event = BTA_HL_DCH_RECONNECT_CFM_EVT;
                send_event = TRUE;
            } else {
                if(MCA_CloseReq((tMCA_DL) p_dcb->mdl_handle) == MCA_SUCCESS) {
                    p_dcb->dch_oper = BTA_HL_DCH_OP_LOCAL_CLOSE_RECONNECT;
                } else {
                    APPL_TRACE_ERROR("Unabel to close DCH for reconnect cfg mismatch");
                }
            }

            break;

        default:
            break;
    }

    if(send_event) {
        p_acb->p_cback(event, (tBTA_HL *) &evt_data);
    }
}


/*******************************************************************************
**
** Function         bta_hl_dch_mca_abort_ind
**
** Description      Action routine for processing the abort indication
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_abort_ind(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                              tBTA_HL_DATA *p_data)
{
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_mca_abort_ind");
#endif
    p_dcb->abort_oper |= BTA_HL_ABORT_REMOTE_MASK;
    bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
}

/*******************************************************************************
**
** Function         bta_hl_dch_mca_abort_cfm
**
** Description      Action routine for processing the abort confirmation
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_abort_cfm(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                              tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tBTA_HL             evt_data;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_mca_abort_cfm");
#endif

    if(p_dcb->abort_oper) {
        if(p_data->mca_evt.mca_data.abort_cfm.rsp_code != MCA_RSP_SUCCESS) {
            if(p_dcb->abort_oper & BTA_HL_ABORT_LOCAL_MASK) {
                bta_hl_build_abort_cfm(&evt_data,
                                       p_acb->app_handle,
                                       p_mcb->mcl_handle,
                                       BTA_HL_STATUS_FAIL);
                p_acb->p_cback(BTA_HL_DCH_ABORT_CFM_EVT, (tBTA_HL *) &evt_data);
            }
        } else {
            bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
        }
    } else {
        APPL_TRACE_ERROR("Not expecting Abort CFM ");
    }
}

/*******************************************************************************
**
** Function         bta_hl_dch_mca_abort
**
** Description      Action routine for processing the abort request
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_abort(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                          tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB          *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB          *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB          *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tMCA_RESULT             mca_result;
    tBTA_HL                 evt_data;

    if(((p_mcb->sdp_oper == BTA_HL_SDP_OP_DCH_OPEN_INIT) ||
            (p_mcb->sdp_oper == BTA_HL_SDP_OP_DCH_RECONNECT_INIT)) &&
            (p_mcb->sdp_mdl_idx == mdl_idx)) {
        p_dcb->abort_oper |= BTA_HL_ABORT_PENDING_MASK;
        return;
    } else if(p_dcb->echo_oper == BTA_HL_ECHO_OP_CI_GET_ECHO_DATA) {
        p_dcb->abort_oper |= BTA_HL_ABORT_PENDING_MASK;
        return;
    }

    p_dcb->abort_oper &= ~BTA_HL_ABORT_PENDING_MASK;

    if((mca_result = MCA_Abort((tMCA_CL) p_mcb->mcl_handle)) != MCA_SUCCESS) {
        if(mca_result == MCA_NO_RESOURCES) {
            p_dcb->abort_oper |= BTA_HL_ABORT_PENDING_MASK;
        } else {
            if(p_dcb->abort_oper & BTA_HL_ABORT_LOCAL_MASK) {
                bta_hl_build_abort_cfm(&evt_data,
                                       p_acb->app_handle,
                                       p_mcb->mcl_handle,
                                       BTA_HL_STATUS_FAIL);
                p_acb->p_cback(BTA_HL_DCH_ABORT_CFM_EVT, (tBTA_HL *) &evt_data);
            }

            bta_hl_check_cch_close(app_idx, mcl_idx, p_data, FALSE);
        }
    }

#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_mca_abort abort_oper=0x%x", p_dcb->abort_oper);
#endif
}

/*******************************************************************************
**
** Function         bta_hl_dch_mca_reconnect_ind
**
** Description      Action routine for processing the reconnect indication
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_reconnect_ind(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                  tBTA_HL_DATA *p_data)
{
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tBTA_HL_MDL_CFG     *p_mdl_cfg;
    tMCA_EVT_HDR        *p_reconnect_ind = &p_data->mca_evt.mca_data.reconnect_ind;
    uint8_t               mdl_cfg_idx, in_use_mdl_idx, mdep_cfg_idx;
    uint8_t               rsp_code = MCA_RSP_SUCCESS;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_mca_reconnect_ind mdl_id=%d", p_reconnect_ind->mdl_id);
#endif

    if(bta_hl_find_mdl_cfg_idx(app_idx, mcl_idx, p_reconnect_ind->mdl_id, &mdl_cfg_idx)) {
        if(!bta_hl_find_mdl_idx(app_idx, mcl_idx, p_reconnect_ind->mdl_id, &in_use_mdl_idx)) {
            p_mdl_cfg =  BTA_HL_GET_MDL_CFG_PTR(app_idx, mdl_cfg_idx);

            if(bta_hl_find_mdep_cfg_idx(app_idx, p_mdl_cfg->local_mdep_id, &mdep_cfg_idx)) {
                p_dcb->in_use               = TRUE;
                p_dcb->dch_oper             = BTA_HL_DCH_OP_REMOTE_RECONNECT;
                p_dcb->sec_mask             = (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT);
                p_dcb->peer_mdep_id             = 0xFF;
                p_dcb->local_mdep_id        = p_mdl_cfg->local_mdep_id  ;
                p_dcb->local_mdep_cfg_idx   = mdep_cfg_idx;
                p_dcb->local_cfg            = BTA_HL_DCH_CFG_UNKNOWN;
                p_dcb->mdl_id               = p_reconnect_ind->mdl_id;
                p_dcb->mdl_cfg_idx_included = TRUE;
                p_dcb->mdl_cfg_idx          = mdl_cfg_idx;
                p_dcb->dch_mode             = p_mdl_cfg->dch_mode;
                bta_hl_find_rxtx_apdu_size(app_idx, mdep_cfg_idx,
                                           &p_dcb->max_rx_apdu_size,
                                           &p_dcb->max_tx_apdu_size);
                bta_hl_set_dch_chan_cfg(app_idx, mcl_idx, mdl_idx, p_data);
            } else {
                rsp_code = MCA_RSP_BAD_MDL;
            }
        } else {
            rsp_code = MCA_RSP_BAD_MDL;
        }
    } else {
        rsp_code = MCA_RSP_BAD_MDL;
    }

    if(MCA_ReconnectMdlRsp((tMCA_CL) p_mcb->mcl_handle,
                           p_dcb->local_mdep_id,
                           p_dcb->mdl_id,
                           rsp_code,
                           &p_dcb->chnl_cfg) != MCA_SUCCESS) {
        MCA_Abort((tMCA_CL) p_mcb->mcl_handle);
        bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
    }
}

/*******************************************************************************
**
** Function         bta_hl_dch_mca_reconnect_cfm
**
** Description      Action routine for processing the reconenct confirmation
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_reconnect_cfm(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                  tBTA_HL_DATA *p_data)
{
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tMCA_RSP_EVT        *p_reconnect_cfm = &p_data->mca_evt.mca_data.reconnect_cfm;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_mca_reconnect_cfm");
#endif

    if(p_dcb->abort_oper & BTA_HL_ABORT_PENDING_MASK) {
        p_dcb->abort_oper &= ~BTA_HL_ABORT_PENDING_MASK;
        bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_ABORT_EVT, p_data);
        return;
    }

    if(p_dcb->dch_oper == BTA_HL_DCH_OP_LOCAL_RECONNECT) {
        if(p_reconnect_cfm->rsp_code == MCA_RSP_SUCCESS) {
            bta_hl_set_dch_chan_cfg(app_idx, mcl_idx, mdl_idx, p_data);

            if(MCA_DataChnlCfg((tMCA_CL) p_mcb->mcl_handle, &p_dcb->chnl_cfg) != MCA_SUCCESS) {
                /* should be able to abort so no checking of the return code */
                MCA_Abort((tMCA_CL) p_mcb->mcl_handle);
                bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
            }
        } else {
            bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
        }
    }
}

/*******************************************************************************
**
** Function         bta_hl_dch_mca_reconnect
**
** Description      Action routine for processing the reconnect request
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_reconnect(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                              tBTA_HL_DATA *p_data)
{
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tMCA_CHNL_CFG       *p_chnl_cfg = NULL;
    uint8_t               sdp_idx;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_mca_reconnect");
#endif

    if(bta_hl_find_sdp_idx_using_ctrl_psm(&p_mcb->sdp, p_mcb->ctrl_psm, &sdp_idx)) {
        p_mcb->data_psm = p_mcb->sdp.sdp_rec[sdp_idx].data_psm;

        if(MCA_ReconnectMdl((tMCA_CL) p_mcb->mcl_handle,
                            p_dcb->local_mdep_id,
                            p_mcb->data_psm,
                            p_dcb->mdl_id,
                            p_chnl_cfg) != MCA_SUCCESS) {
            bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
        }
    } else {
        bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
    }
}


/*******************************************************************************
**
** Function         bta_hl_dch_create_rsp
**
** Description      Action routine for processing BTA_HL_API_DCH_CREATE_RSP_EVT
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_create_rsp(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                           tBTA_HL_DATA *p_data)
{
    tBTA_HL_MCL_CB              *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB              *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tBTA_HL_API_DCH_CREATE_RSP  *p_create_rsp = &p_data->api_dch_create_rsp;
    uint8_t                       mca_rsp_code = MCA_RSP_SUCCESS;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_create_rsp");
#endif

    if(p_create_rsp->rsp_code == BTA_HL_DCH_CREATE_RSP_SUCCESS) {
        p_dcb->dch_oper             = BTA_HL_DCH_OP_REMOTE_OPEN;
        p_dcb->local_cfg            = p_create_rsp->cfg_rsp;
        bta_hl_set_dch_chan_cfg(app_idx, mcl_idx, mdl_idx, p_data);
    } else {
        mca_rsp_code = MCA_RSP_CFG_REJ;
    }

    if(MCA_CreateMdlRsp((tMCA_CL) p_mcb->mcl_handle,
                        p_dcb->local_mdep_id,
                        p_dcb->mdl_id,
                        p_dcb->local_cfg,
                        mca_rsp_code,
                        &p_dcb->chnl_cfg) != MCA_SUCCESS) {
        bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
    }
}

/*******************************************************************************
**
** Function         bta_hl_dch_mca_create_ind
**
** Description      Action routine for processing
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_create_ind(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                               tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tMCA_CREATE_IND     *p_create_ind = &p_data->mca_evt.mca_data.create_ind;
    uint8_t               mdep_cfg_idx;
    uint8_t               cfg_rsp;
    uint8_t               rsp_code = MCA_RSP_SUCCESS;
    uint8_t             send_create_ind_evt = FALSE;
    tBTA_HL             evt_data;
    tBTA_HL_ECHO_CFG    *p_echo_cfg;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_mca_create_ind");
#endif

    if(bta_hl_find_mdep_cfg_idx(app_idx, p_create_ind->dep_id, &mdep_cfg_idx)) {
        if(p_create_ind->dep_id == BTA_HL_ECHO_TEST_MDEP_ID) {
            if(bta_hl_find_echo_cfg_rsp(app_idx, mcl_idx, mdep_cfg_idx, p_create_ind->cfg, &cfg_rsp)) {
                p_dcb->in_use               = TRUE;
                p_dcb->dch_oper             = BTA_HL_DCH_OP_REMOTE_OPEN;
                p_dcb->local_mdep_id        = p_create_ind->dep_id  ;
                p_dcb->local_mdep_cfg_idx   = mdep_cfg_idx;
                p_dcb->local_cfg            = cfg_rsp;
                p_dcb->remote_cfg           = p_create_ind->cfg ;
                p_dcb->mdl_id               = p_create_ind->mdl_id;
                p_dcb->mdl_cfg_idx_included = FALSE;
                p_echo_cfg                      = BTA_HL_GET_ECHO_CFG_PTR(app_idx);
                p_dcb->max_rx_apdu_size         = p_echo_cfg->max_rx_apdu_size;
                p_dcb->max_tx_apdu_size         = p_echo_cfg->max_tx_apdu_size;
                bta_hl_set_dch_chan_cfg(app_idx, mcl_idx, mdl_idx, p_data);
            } else {
                rsp_code = MCA_RSP_CFG_REJ;
            }
        } else {
            p_dcb->in_use               = TRUE;
            p_dcb->dch_oper             = BTA_HL_DCH_OP_REMOTE_CREATE;
            p_dcb->local_mdep_id        = p_create_ind->dep_id  ;
            p_dcb->local_mdep_cfg_idx   = mdep_cfg_idx;
            p_dcb->local_cfg            = BTA_HL_DCH_CFG_UNKNOWN;
            p_dcb->remote_cfg           = p_create_ind->cfg;
            p_dcb->mdl_id               = p_create_ind->mdl_id;
            p_dcb->mdl_cfg_idx_included = FALSE;
            bta_hl_find_rxtx_apdu_size(app_idx, mdep_cfg_idx,
                                       &p_dcb->max_rx_apdu_size,
                                       &p_dcb->max_tx_apdu_size);
            send_create_ind_evt = TRUE;
        }
    } else {
        rsp_code = MCA_RSP_BAD_MDEP;
    }

    if(send_create_ind_evt) {
        evt_data.dch_create_ind.mcl_handle =  p_mcb->mcl_handle;
        evt_data.dch_create_ind.app_handle =  p_acb->app_handle;
        evt_data.dch_create_ind.local_mdep_id = p_dcb->local_mdep_id;
        evt_data.dch_create_ind.mdl_id = p_dcb->mdl_id;
        evt_data.dch_create_ind.cfg = p_dcb->remote_cfg;
        bdcpy(evt_data.dch_create_ind.bd_addr, p_mcb->bd_addr);
        p_acb->p_cback(BTA_HL_DCH_CREATE_IND_EVT, (tBTA_HL *) &evt_data);
    } else {
        if(MCA_CreateMdlRsp((tMCA_CL) p_mcb->mcl_handle,
                            p_dcb->local_mdep_id,
                            p_dcb->mdl_id,
                            p_dcb->local_cfg,
                            rsp_code,
                            &p_dcb->chnl_cfg) != MCA_SUCCESS) {
            bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
        } else {
            if(p_dcb->local_mdep_id == BTA_HL_ECHO_TEST_MDEP_ID) {
                p_mcb->echo_test = TRUE;
                p_dcb->echo_oper = BTA_HL_ECHO_OP_OPEN_IND;
            }
        }
    }
}

/*******************************************************************************
**
** Function         bta_hl_dch_mca_create_cfm
**
** Description      Action routine for processing
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_create_cfm(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                               tBTA_HL_DATA *p_data)
{
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tMCA_CREATE_CFM     *p_create_cfm = &p_data->mca_evt.mca_data.create_cfm;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_mca_create_cfm");
#endif

    if(p_dcb->abort_oper & BTA_HL_ABORT_PENDING_MASK) {
        p_dcb->abort_oper &= ~BTA_HL_ABORT_PENDING_MASK;
        bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_ABORT_EVT, p_data);
        return;
    }

    if(p_dcb->dch_oper == BTA_HL_DCH_OP_LOCAL_OPEN) {
        if(p_create_cfm->rsp_code == MCA_RSP_SUCCESS) {
            if(bta_hl_validate_cfg(app_idx, mcl_idx, mdl_idx, p_create_cfm->cfg)) {
                bta_hl_set_dch_chan_cfg(app_idx, mcl_idx, mdl_idx, p_data);

                if(MCA_DataChnlCfg((tMCA_CL) p_mcb->mcl_handle, &p_dcb->chnl_cfg) != MCA_SUCCESS) {
                    /* this should not happen */
                    APPL_TRACE_ERROR("Unable to create data channel");
                    MCA_Abort((tMCA_CL) p_mcb->mcl_handle);
                    bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
                } else {
                    if(p_dcb->local_mdep_id == BTA_HL_ECHO_TEST_MDEP_ID) {
                        p_dcb->echo_oper = BTA_HL_ECHO_OP_DCH_OPEN_CFM;
                    }
                }
            } else {
                MCA_Abort((tMCA_CL) p_mcb->mcl_handle);
                bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
            }
        } else {
            APPL_TRACE_ERROR("MCA Create- failed");
            bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
        }
    }
}

/*******************************************************************************
**
** Function         bta_hl_dch_mca_create
**
** Description      Action routine for processing the MDL create request
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_mca_create(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                           tBTA_HL_DATA *p_data)
{
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tMCA_RESULT         result;
    uint8_t               sdp_idx;
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_dch_mca_create");
#endif

    if(bta_hl_find_sdp_idx_using_ctrl_psm(&p_mcb->sdp, p_mcb->ctrl_psm, &sdp_idx) &&
            bta_hl_validate_peer_cfg(app_idx, mcl_idx, mdl_idx,
                                     p_dcb->peer_mdep_id,
                                     p_dcb->peer_mdep_role,
                                     sdp_idx)) {
        p_mcb->data_psm = p_mcb->sdp.sdp_rec[sdp_idx].data_psm;

        if((result = MCA_CreateMdl((tMCA_CL) p_mcb->mcl_handle,
                                   p_dcb->local_mdep_id,
                                   p_mcb->data_psm,
                                   p_dcb->mdl_id,
                                   p_dcb->peer_mdep_id,
                                   p_dcb->local_cfg,
                                   NULL)) != MCA_SUCCESS) {
            APPL_TRACE_ERROR("MCA_CreateMdl FAIL mca_result=%d", result);
            bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
        }
    } else {
        APPL_TRACE_ERROR("MCA Create- SDP idx or peer MDEP cfg not found");
        bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
    }
}

/*******************************************************************************
**
** Function         bta_hl_dch_sdp_fail
**
** Description      Action routine for processing the SDP failed event
**
** Returns          void
**
*******************************************************************************/
void bta_hl_dch_sdp_fail(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                         tBTA_HL_DATA *p_data)
{
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_dch_sdp_fail");
#endif
    bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
}

/******************************************************************************
**
** Function         bta_hl_sdp_cback
**
** Description      This is the SDP callback function used by HL.
**                  This function will be executed by SDP when the service
**                  search is completed.  If the search is successful, it
**                  finds the first record in the database that matches the
**                  UUID of the search.  Then retrieves the scn from the
**                  record.
**
** Returns          void.
**
******************************************************************************/
static void bta_hl_sdp_cback(uint8_t sdp_oper, uint8_t app_idx, uint8_t mcl_idx,
                             uint8_t mdl_idx, uint16_t status)
{
    tBTA_HL_MCL_CB                  *p_cb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_SDP_REC                 *p_hdp_rec;
    tBTA_HL_CCH_SDP                 *p_cch_buf;
    tBTA_HL_DCH_SDP                 *p_dch_buf;
    tSDP_DISC_REC                   *p_rec = NULL;
    tSDP_PROTOCOL_ELEM              pe;
    tSDP_DISC_ATTR                  *p_attr;
    uint8_t                           i, rec_cnt;
    tBTA_HL_SUP_FEATURE_LIST_ELEM   sup_feature;
    uint8_t                         sdp_parsing_ok = FALSE, result = FALSE;
    uint16_t                          event;
    tBTA_HL_MDL_CB                  *p_dcb;
    uint16_t                          service_uuid;
    uint16_t                          name_len;
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_sdp_cback status:%d sdp_oper=%d app_idx=%d, mcl_idx=%d,   mdl_idx=%d",
                     status, sdp_oper, app_idx, mcl_idx, mdl_idx);
#endif
    rec_cnt = 0;
    service_uuid = bta_hl_get_service_uuids(sdp_oper, app_idx, mcl_idx, mdl_idx);

    if(status == SDP_SUCCESS || status == SDP_DB_FULL) {
        wm_memset(&p_cb->sdp, 0, sizeof(tBTA_HL_SDP));

        do {
            if(bta_hl_find_service_in_db(app_idx, mcl_idx, service_uuid, &p_rec)) {
                p_hdp_rec = &p_cb->sdp.sdp_rec[rec_cnt];
                p_cb->sdp.num_recs = rec_cnt + 1;
            } else {
                break;
            }

            if(SDP_FindProtocolListElemInRec(p_rec, UUID_PROTOCOL_L2CAP, &pe)) {
                p_hdp_rec->ctrl_psm  = (uint16_t) pe.params[0];
            } else {
                APPL_TRACE_WARNING("Control PSM not found");
                break;
            }

            if(SDP_FindAddProtoListsElemInRec(p_rec, UUID_PROTOCOL_L2CAP, &pe)) {
                p_hdp_rec->data_psm = (uint16_t) pe.params[0];
            } else {
                APPL_TRACE_WARNING("Data PSM not found");
                break;
            }

            p_hdp_rec->srv_name[0] = '\0';

            if((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_SERVICE_NAME)) != NULL) {
                if(SDP_DISC_ATTR_LEN(p_attr->attr_len_type) < BT_MAX_SERVICE_NAME_LEN) {
                    name_len = (uint16_t)SDP_DISC_ATTR_LEN(p_attr->attr_len_type);
                } else {
                    name_len = BT_MAX_SERVICE_NAME_LEN;
                }

                wm_memcpy(p_hdp_rec->srv_name, p_attr->attr_value.v.array, name_len);
            }

            p_hdp_rec->srv_desp[0] = '\0';

            if((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_SERVICE_DESCRIPTION)) != NULL) {
                if(SDP_DISC_ATTR_LEN(p_attr->attr_len_type) < BT_MAX_SERVICE_NAME_LEN) {
                    name_len = (uint16_t)SDP_DISC_ATTR_LEN(p_attr->attr_len_type);
                } else {
                    name_len = BT_MAX_SERVICE_NAME_LEN;
                }

                wm_memcpy(p_hdp_rec->srv_desp, p_attr->attr_value.v.array, name_len);
            }

            p_hdp_rec->provider_name[0] = '\0';

            if((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_PROVIDER_NAME)) != NULL) {
                if(SDP_DISC_ATTR_LEN(p_attr->attr_len_type) < BT_MAX_SERVICE_NAME_LEN) {
                    name_len = (uint16_t)SDP_DISC_ATTR_LEN(p_attr->attr_len_type);
                } else {
                    name_len = BT_MAX_SERVICE_NAME_LEN;
                }

                wm_memcpy(p_hdp_rec->provider_name, p_attr->attr_value.v.array, name_len);
            }

            if((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_HDP_MCAP_SUP_PROC)) != NULL) {
                p_hdp_rec->mcap_sup_proc = p_attr->attr_value.v.u8;
            } else {
                APPL_TRACE_WARNING("MCAP SUP PROC not found");
                break;
            }

            if((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_HDP_SUP_FEAT_LIST)) != NULL) {
                if(bta_hl_fill_sup_feature_list(p_attr, &sup_feature)) {
                    p_hdp_rec->num_mdeps = (uint8_t) sup_feature.num_elems;
                    APPL_TRACE_WARNING("bta_hl_sdp_cback num_mdeps %d", sup_feature.num_elems);

                    for(i = 0; i < sup_feature.num_elems; i++) {
                        p_hdp_rec->mdep_cfg[i].data_type = sup_feature.list_elem[i].data_type;
                        p_hdp_rec->mdep_cfg[i].mdep_id = sup_feature.list_elem[i].mdep_id;
                        p_hdp_rec->mdep_cfg[i].mdep_role = sup_feature.list_elem[i].mdep_role;

                        /* Check MDEP Description pointer to prevent crash due to null pointer */
                        if(sup_feature.list_elem[i].p_mdep_desp != NULL) {
                            strlcpy(p_hdp_rec->mdep_cfg[i].mdep_desp,
                                    sup_feature.list_elem[i].p_mdep_desp,
                                    BTA_HL_MDEP_DESP_LEN);
                        } else {
                            APPL_TRACE_ERROR("bta_hl_sdp_cback Incorrect Mdep[%d] Description (Null ptr)", i);
                        }
                    }

                    sdp_parsing_ok = TRUE;
                } else {
                    APPL_TRACE_WARNING("HDP supported feature list fill failed");
                    break;
                }
            } else {
                APPL_TRACE_WARNING("HDP supported feature list not found");
                break;
            }

#if BTA_HL_DEBUG == TRUE
            APPL_TRACE_DEBUG("record=%d ctrl_psm=%0x data_psm=%x",
                             rec_cnt + 1,
                             p_hdp_rec->ctrl_psm,
                             p_hdp_rec->data_psm);
            APPL_TRACE_DEBUG("srv_name=[%s]", (p_hdp_rec->srv_name[0] != '\0') ? p_hdp_rec->srv_name : "NULL");
            APPL_TRACE_DEBUG("srv_desp=[%s]", (p_hdp_rec->srv_desp[0] != '\0') ? p_hdp_rec->srv_desp : "NULL");

            for(i = 0; i < sup_feature.num_elems; i++) {
                APPL_TRACE_DEBUG("index=0x%02x mdep_id=0x%04x data type=0x%04x mdep role=%s(0x%02x)",
                                 (i + 1),
                                 p_hdp_rec->mdep_cfg[i].mdep_id,
                                 p_hdp_rec->mdep_cfg[i].data_type,
                                 (p_hdp_rec->mdep_cfg[i].mdep_role == BTA_HL_MDEP_ROLE_SOURCE) ? "Src" : "Snk",
                                 p_hdp_rec->mdep_cfg[i].mdep_role);
            }

            APPL_TRACE_DEBUG("provider_name=[%s]",
                             (p_hdp_rec->provider_name[0] != '\0') ? p_hdp_rec->provider_name : "NULL");
            APPL_TRACE_DEBUG("found MCAP sup procedure=%d",
                             p_cb->sdp.sdp_rec[rec_cnt].mcap_sup_proc);
#endif
            rec_cnt++;

            if(rec_cnt >= BTA_HL_NUM_SDP_RECS) {
                APPL_TRACE_WARNING("No more spaces for SDP recs max_rec_cnt=%d", BTA_HL_NUM_SDP_RECS);
                break;
            }
        } while(TRUE);
    }

    GKI_free_and_reset_buf((void **)&p_cb->p_db);

    if((status == SDP_SUCCESS || status == SDP_DB_FULL) &&
            p_cb->sdp.num_recs  &&
            sdp_parsing_ok) {
        result = TRUE;
    } else {
        APPL_TRACE_WARNING("SDP Failed sdp_status=%d num_recs=%d sdp_parsing_ok=%d ",
                           status, p_cb->sdp.num_recs, sdp_parsing_ok);
    }

    p_cb->sdp_oper = BTA_HL_SDP_OP_NONE;

    switch(sdp_oper) {
        case BTA_HL_SDP_OP_CCH_INIT:
        case BTA_HL_SDP_OP_SDP_QUERY_NEW:
        case BTA_HL_SDP_OP_SDP_QUERY_CURRENT:
            /* send result in event back to BTA */
            p_cch_buf = (tBTA_HL_CCH_SDP *)GKI_getbuf(sizeof(tBTA_HL_CCH_SDP));

            if(result) {
                if(sdp_oper == BTA_HL_SDP_OP_CCH_INIT) {
                    event = BTA_HL_CCH_SDP_OK_EVT;

                    if(p_cb->close_pending) {
                        event = BTA_HL_CCH_SDP_FAIL_EVT;
                    }
                } else {
                    event = BTA_HL_SDP_QUERY_OK_EVT;
                }
            } else {
                if(sdp_oper == BTA_HL_SDP_OP_CCH_INIT) {
                    event = BTA_HL_CCH_SDP_FAIL_EVT;
                } else {
                    event = BTA_HL_SDP_QUERY_FAIL_EVT;
                }
            }

            p_cch_buf->hdr.event = event;
            p_cch_buf->app_idx = app_idx;
            p_cch_buf->mcl_idx = mcl_idx;
            p_cch_buf->release_mcl_cb = FALSE;

            if(sdp_oper == BTA_HL_SDP_OP_SDP_QUERY_NEW) {
                p_cch_buf->release_mcl_cb = TRUE;
            }

            bta_sys_sendmsg(p_cch_buf);
            break;

        case BTA_HL_SDP_OP_DCH_OPEN_INIT:
        case BTA_HL_SDP_OP_DCH_RECONNECT_INIT:
            p_dch_buf = (tBTA_HL_DCH_SDP *)GKI_getbuf(sizeof(tBTA_HL_DCH_SDP));
            p_dch_buf->hdr.event = BTA_HL_DCH_SDP_FAIL_EVT;
            p_dch_buf->app_idx = app_idx;
            p_dch_buf->mcl_idx = mcl_idx;
            p_dch_buf->mdl_idx = mdl_idx;
            p_dcb = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);

            if(p_dcb->abort_oper & BTA_HL_ABORT_PENDING_MASK) {
                p_dcb->abort_oper &= ~BTA_HL_ABORT_PENDING_MASK;
                result = FALSE;
            }

            if(result) {
                if(sdp_oper == BTA_HL_SDP_OP_DCH_OPEN_INIT) {
                    if(p_dcb->local_mdep_id == BTA_HL_ECHO_TEST_MDEP_ID) {
                        p_dch_buf->hdr.event = BTA_HL_DCH_ECHO_TEST_EVT;
                    } else {
                        p_dch_buf->hdr.event = BTA_HL_DCH_OPEN_EVT;
                    }
                } else {
                    p_dch_buf->hdr.event = BTA_HL_DCH_RECONNECT_EVT;
                }
            }

            bta_sys_sendmsg(p_dch_buf);
            break;

        default:
            break;
    }
}


/******************************************************************************
**
** Function         bta_hl_sdp_cback0
**
** Description      This is the SDP callback function used by index = 0
**
** Returns          void.
**
******************************************************************************/
static void bta_hl_sdp_cback0(uint16_t status)
{
    bta_hl_sdp_cback(bta_hl_cb.scb[0].sdp_oper,
                     bta_hl_cb.scb[0].app_idx,
                     bta_hl_cb.scb[0].mcl_idx,
                     bta_hl_cb.scb[0].mdl_idx,
                     status);
    bta_hl_deallocate_spd_cback(0);
}

/******************************************************************************
**
** Function         bta_hl_sdp_cback1
**
** Description      This is the SDP callback function used by index = 1
**
** Parameters       status  - status of the SDP callabck
**
** Returns          void.
**
******************************************************************************/
static void bta_hl_sdp_cback1(uint16_t status)
{
    bta_hl_sdp_cback(bta_hl_cb.scb[1].sdp_oper,
                     bta_hl_cb.scb[1].app_idx,
                     bta_hl_cb.scb[1].mcl_idx,
                     bta_hl_cb.scb[1].mdl_idx,
                     status);
    bta_hl_deallocate_spd_cback(1);
}

/******************************************************************************
**
** Function         bta_hl_sdp_cback2
**
** Description      This is the SDP callback function used by index = 2
**
** Returns          void.
**
******************************************************************************/
static void bta_hl_sdp_cback2(uint16_t status)
{
    bta_hl_sdp_cback(bta_hl_cb.scb[2].sdp_oper,
                     bta_hl_cb.scb[2].app_idx,
                     bta_hl_cb.scb[2].mcl_idx,
                     bta_hl_cb.scb[2].mdl_idx,
                     status);
    bta_hl_deallocate_spd_cback(2);
}

/******************************************************************************
**
** Function         bta_hl_sdp_cback3
**
** Description      This is the SDP callback function used by index = 3
**
** Returns          void.
**
******************************************************************************/
static void bta_hl_sdp_cback3(uint16_t status)
{
    bta_hl_sdp_cback(bta_hl_cb.scb[3].sdp_oper,
                     bta_hl_cb.scb[3].app_idx,
                     bta_hl_cb.scb[3].mcl_idx,
                     bta_hl_cb.scb[3].mdl_idx,
                     status);
    bta_hl_deallocate_spd_cback(3);
}

/******************************************************************************
**
** Function         bta_hl_sdp_cback4
**
** Description      This is the SDP callback function used by index = 4
**
** Parameters       status  - status of the SDP callabck
**
** Returns          void.
**
******************************************************************************/
static void bta_hl_sdp_cback4(uint16_t status)
{
    bta_hl_sdp_cback(bta_hl_cb.scb[4].sdp_oper,
                     bta_hl_cb.scb[4].app_idx,
                     bta_hl_cb.scb[4].mcl_idx,
                     bta_hl_cb.scb[4].mdl_idx,
                     status);
    bta_hl_deallocate_spd_cback(4);
}

/******************************************************************************
**
** Function         bta_hl_sdp_cback5
**
** Description      This is the SDP callback function used by index = 5
**
** Parameters       status  - status of the SDP callabck
**
** Returns          void.
**
******************************************************************************/
static void bta_hl_sdp_cback5(uint16_t status)
{
    bta_hl_sdp_cback(bta_hl_cb.scb[5].sdp_oper,
                     bta_hl_cb.scb[5].app_idx,
                     bta_hl_cb.scb[5].mcl_idx,
                     bta_hl_cb.scb[5].mdl_idx,
                     status);
    bta_hl_deallocate_spd_cback(5);
}

/******************************************************************************
**
** Function         bta_hl_sdp_cback6
**
** Description      This is the SDP callback function used by index = 6
**
** Returns          void.
**
******************************************************************************/
static void bta_hl_sdp_cback6(uint16_t status)
{
    bta_hl_sdp_cback(bta_hl_cb.scb[6].sdp_oper,
                     bta_hl_cb.scb[6].app_idx,
                     bta_hl_cb.scb[6].mcl_idx,
                     bta_hl_cb.scb[6].mdl_idx,
                     status);
    bta_hl_deallocate_spd_cback(6);
}


/*******************************************************************************
**
** Function      bta_hl_deallocate_spd_cback
**
** Description   Deallocate a SDP control block
**
** Returns      uint8_t - TRUE found
**                        FALSE not found
**
*******************************************************************************/
void bta_hl_deallocate_spd_cback(uint8_t sdp_cback_idx)
{
    tBTA_HL_SDP_CB *p_spd_cb = &bta_hl_cb.scb[sdp_cback_idx];
    wm_memset(p_spd_cb, 0, sizeof(tBTA_HL_SDP_CB));
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_deallocate_spd_cback index=%d", sdp_cback_idx);
#endif
}

/*******************************************************************************
**
** Function      bta_hl_allocate_spd_cback
**
** Description   Finds a not in used SDP control block index
**
**
** Returns      uint8_t - TRUE found
**                        FALSE not found
**
*******************************************************************************/
tSDP_DISC_CMPL_CB *bta_hl_allocate_spd_cback(tBTA_HL_SDP_OPER sdp_oper, uint8_t app_idx,
        uint8_t mcl_idx,
        uint8_t mdl_idx,
        uint8_t *p_sdp_cback_idx)
{
    uint8_t i;
    tSDP_DISC_CMPL_CB *p_cbcak = NULL;

    for(i = 0; i < BTA_HL_NUM_SDP_CBACKS ; i ++) {
        if(!bta_hl_cb.scb[i].in_use) {
            p_cbcak = bta_hl_sdp_cback_arr[i];
            bta_hl_cb.scb[i].in_use = TRUE;
            bta_hl_cb.scb[i].sdp_oper = sdp_oper;
            bta_hl_cb.scb[i].app_idx = app_idx;
            bta_hl_cb.scb[i].mcl_idx = mcl_idx;
            bta_hl_cb.scb[i].mdl_idx = mdl_idx;
            *p_sdp_cback_idx = i;
            break;
        }
    }

    if(i == BTA_HL_NUM_SDP_CBACKS) {
        APPL_TRACE_WARNING("No scb is available to allocate")
    } else {
#if BTA_HL_DEBUG == TRUE
        APPL_TRACE_DEBUG("bta_hl_allocate_spd_cback cback_idx=%d ", i);
        APPL_TRACE_DEBUG("sdp_oper=%d, app_idx=%d, mcl_idx=%d,  mdl_idx=%d",
                         bta_hl_cb.scb[i].sdp_oper,
                         bta_hl_cb.scb[i].app_idx,
                         bta_hl_cb.scb[i].mcl_idx,
                         bta_hl_cb.scb[i].mdl_idx);
#endif
    }

    return p_cbcak;
}


/*******************************************************************************
**
** Function         bta_hl_init_sdp
**
** Description      Action routine for processing the SDP initiattion request
**
** Returns          void
**
*******************************************************************************/
tBTA_HL_STATUS bta_hl_init_sdp(tBTA_HL_SDP_OPER sdp_oper, uint8_t app_idx, uint8_t mcl_idx,
                               uint8_t mdl_idx)
{
    tBTA_HL_MCL_CB      *p_cb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tSDP_UUID           uuid_list;
    uint16_t              attr_list[BTA_HL_NUM_SRCH_ATTR];
    uint16_t              num_attrs = BTA_HL_NUM_SRCH_ATTR;
    tBTA_HL_STATUS      status;
    uint8_t               sdp_cback_idx;
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_init_sdp sdp_oper=%d app_idx=%d mcl_idx=%d, mdl_idx=%d",
                     sdp_oper, app_idx, mcl_idx, mdl_idx);
#endif

    if((p_cb->sdp_cback = bta_hl_allocate_spd_cback(sdp_oper, app_idx, mcl_idx, mdl_idx,
                          &sdp_cback_idx)) != NULL) {
        if(p_cb->p_db == NULL) {
            (p_cb->p_db = (tSDP_DISCOVERY_DB *)GKI_getbuf(BTA_HL_DISC_SIZE));
        }

        attr_list[0] = ATTR_ID_SERVICE_CLASS_ID_LIST;
        attr_list[1] = ATTR_ID_PROTOCOL_DESC_LIST;
        attr_list[2] = ATTR_ID_BT_PROFILE_DESC_LIST;
        attr_list[3] = ATTR_ID_ADDITION_PROTO_DESC_LISTS;
        attr_list[4] = ATTR_ID_SERVICE_NAME;
        attr_list[5] = ATTR_ID_SERVICE_DESCRIPTION;
        attr_list[6] = ATTR_ID_PROVIDER_NAME;
        attr_list[7] = ATTR_ID_HDP_SUP_FEAT_LIST;
        attr_list[8] = ATTR_ID_HDP_DATA_EXCH_SPEC;
        attr_list[9] = ATTR_ID_HDP_MCAP_SUP_PROC;
        uuid_list.len = LEN_UUID_16;
        uuid_list.uu.uuid16 = UUID_SERVCLASS_HDP_PROFILE;
        SDP_InitDiscoveryDb(p_cb->p_db, BTA_HL_DISC_SIZE, 1, &uuid_list, num_attrs, attr_list);

        if(!SDP_ServiceSearchAttributeRequest(p_cb->bd_addr, p_cb->p_db, p_cb->sdp_cback)) {
            status = BTA_HL_STATUS_FAIL;
        } else {
            status = BTA_HL_STATUS_OK;
        }
    } else {
        status = BTA_HL_STATUS_SDP_NO_RESOURCE;
    }

    if(status != BTA_HL_STATUS_OK) {
        GKI_free_and_reset_buf((void **)&p_cb->p_db);

        if(status != BTA_HL_STATUS_SDP_NO_RESOURCE) {
            bta_hl_deallocate_spd_cback(sdp_cback_idx);
        }
    }

    return status;
}

/*******************************************************************************
**
** Function         bta_hl_cch_sdp_init
**
** Description      Action routine for processing the CCH SDP init event
**
** Returns          void
**
*******************************************************************************/
void bta_hl_cch_sdp_init(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data)
{
    tBTA_HL_MCL_CB      *p_cb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_cch_init_sdp");
#endif

    if(p_cb->sdp_oper == BTA_HL_SDP_OP_NONE) {
        p_cb->app_id = p_data->api_cch_open.app_id;
        p_cb->sdp_oper = BTA_HL_SDP_OP_CCH_INIT;

        if(bta_hl_init_sdp(p_cb->sdp_oper, app_idx, mcl_idx, 0xFF) != BTA_HL_STATUS_OK) {
            p_cb->sdp_oper = BTA_HL_SDP_OP_NONE;
            bta_hl_cch_sm_execute(app_idx, mcl_idx, BTA_HL_CCH_SDP_FAIL_EVT, p_data);
        }
    } else {
        APPL_TRACE_ERROR("SDP in use");
        bta_hl_cch_sm_execute(app_idx, mcl_idx, BTA_HL_CCH_SDP_FAIL_EVT, p_data);
    }
}

/*******************************************************************************
**
** Function         bta_hl_cch_mca_open
**
** Description      Action routine for processing the CCH open request
**
** Returns          void
**
*******************************************************************************/
void bta_hl_cch_mca_open(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    uint8_t               sdp_idx;
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_cch_mca_open");
#endif

    if(bta_hl_find_sdp_idx_using_ctrl_psm(&p_mcb->sdp, p_mcb->req_ctrl_psm, &sdp_idx)) {
        p_mcb->ctrl_psm = p_mcb->sdp.sdp_rec[sdp_idx].ctrl_psm;
        p_mcb->data_psm = p_mcb->sdp.sdp_rec[sdp_idx].data_psm;

        if(MCA_ConnectReq((tMCA_HANDLE) p_acb->app_handle,
                          p_mcb->bd_addr,
                          p_mcb->ctrl_psm,
                          p_mcb->sec_mask) != MCA_SUCCESS) {
            bta_hl_cch_sm_execute(app_idx, mcl_idx, BTA_HL_CCH_CLOSE_CMPL_EVT, p_data);
        }
    } else {
        bta_hl_cch_sm_execute(app_idx, mcl_idx, BTA_HL_CCH_CLOSE_CMPL_EVT, p_data);
    }
}

/*******************************************************************************
**
** Function         bta_hl_cch_mca_close
**
** Description      Action routine for processing the CCH close request
**
** Returns          void
**
*******************************************************************************/
void bta_hl_cch_mca_close(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data)
{
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_cch_mca_close mcl_handle=%d", p_mcb->mcl_handle);
#endif

    if(p_mcb->sdp_oper != BTA_HL_SDP_OP_CCH_INIT) {
        if(p_mcb->mcl_handle) {
            if(MCA_DisconnectReq((tMCA_HANDLE) p_mcb->mcl_handle) != MCA_SUCCESS) {
                bta_hl_cch_sm_execute(app_idx, mcl_idx, BTA_HL_CCH_CLOSE_CMPL_EVT, p_data);
            }
        } else {
            p_mcb->close_pending = TRUE;
            APPL_TRACE_DEBUG("No valid mcl_handle to stop the CCH setup now so wait until CCH is up then close it");
        }
    } else {
        p_mcb->close_pending = TRUE;
        APPL_TRACE_DEBUG("can not stop the CCH setup becasue SDP is in progress so wait until it is done");
    }
}

/*******************************************************************************
**
** Function         bta_hl_cch_close_cmpl
**
** Description      Action routine for processing the CCH close complete event
**
** Returns          void
**
*******************************************************************************/
void bta_hl_cch_close_cmpl(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL             evt_data;
    tBTA_HL_EVT         event;
    uint8_t             send_evt = TRUE;
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_cch_close_cmpl");
#endif
    bta_sys_conn_close(BTA_ID_HL, p_acb->app_id, p_mcb->bd_addr);

    if(p_mcb->cch_oper == BTA_HL_CCH_OP_LOCAL_CLOSE && p_mcb->force_close_local_cch_opening) {
        p_mcb->cch_oper = BTA_HL_CCH_OP_LOCAL_OPEN;
        APPL_TRACE_DEBUG("change cch_oper from BTA_HL_CCH_OP_LOCAL_CLOSE to BTA_HL_CCH_OP_LOCAL_OPEN");
    }

    switch(p_mcb->cch_oper) {
        case BTA_HL_CCH_OP_LOCAL_OPEN:
            bta_hl_build_cch_open_cfm(&evt_data, p_mcb->app_id, p_acb->app_handle,
                                      p_mcb->mcl_handle,
                                      p_mcb->bd_addr,
                                      BTA_HL_STATUS_FAIL);
            event = BTA_HL_CCH_OPEN_CFM_EVT;
            break;

        case BTA_HL_CCH_OP_LOCAL_CLOSE:
            bta_hl_build_cch_close_cfm(&evt_data,  p_acb->app_handle,
                                       p_mcb->mcl_handle,
                                       BTA_HL_STATUS_OK);
            event = BTA_HL_CCH_CLOSE_CFM_EVT;
            break;

        case BTA_HL_CCH_OP_REMOTE_CLOSE:
            bta_hl_build_cch_close_ind(&evt_data,
                                       p_acb->app_handle,
                                       p_mcb->mcl_handle,
                                       p_mcb->intentional_close);
            event = BTA_HL_CCH_CLOSE_IND_EVT;
            break;

        default:
            send_evt = FALSE;
            break;
    }

    wm_memset(p_mcb, 0, sizeof(tBTA_HL_MCL_CB));

    if(send_evt) {
        p_acb->p_cback(event, (tBTA_HL *) &evt_data);
    }

    bta_hl_check_deregistration(app_idx, p_data);
}

/*******************************************************************************
**
** Function         bta_hl_cch_mca_disconnect
**
** Description      Action routine for processing the CCH disconnect indication
**
** Returns          void
**
*******************************************************************************/
void bta_hl_cch_mca_disconnect(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data)
{
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb;
    uint8_t              i;
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_cch_mca_disconnect");
#endif
    p_mcb->intentional_close = FALSE;

    if(p_data->mca_evt.mca_data.disconnect_ind.reason == L2CAP_DISC_OK) {
        p_mcb->intentional_close = TRUE;
    }

    for(i = 0; i < BTA_HL_NUM_MDLS_PER_MCL; i++) {
        p_dcb = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, i);

        if(p_dcb->in_use && (p_dcb->dch_state != BTA_HL_DCH_IDLE_ST)) {
            if(p_mcb->cch_oper == BTA_HL_CCH_OP_LOCAL_CLOSE) {
                bta_hl_dch_sm_execute(app_idx, mcl_idx, i, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
            } else {
                bta_hl_dch_sm_execute(app_idx, mcl_idx, i, BTA_HL_MCA_CLOSE_IND_EVT, p_data);
            }
        }
    }

    bta_hl_cch_sm_execute(app_idx, mcl_idx, BTA_HL_CCH_CLOSE_CMPL_EVT, p_data);
}

/*******************************************************************************
**
** Function         bta_hl_cch_mca_disc_open
**
** Description      Action routine for disconnect the just opened Control channel
**
** Returns          void
**
*******************************************************************************/
void bta_hl_cch_mca_disc_open(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data)
{
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_cch_mca_disc_open mcl_handle=0x%x close_pending=%d",
                     p_data->mca_evt.mcl_handle, p_mcb->close_pending);
#endif
    p_mcb->close_pending = FALSE;
    p_mcb->mcl_handle = p_data->mca_evt.mcl_handle;
    bta_hl_cch_mca_close(app_idx, mcl_idx, p_data);
}

/*******************************************************************************
**
** Function         bta_hl_cch_mca_rsp_tout
**
** Description      Action routine for processing the MCAP response timeout
**
** Returns          void
**
*******************************************************************************/
void bta_hl_cch_mca_rsp_tout(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data)
{
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_cch_mca_rsp_tout");
#endif
    p_mcb->rsp_tout = TRUE;
    bta_hl_check_cch_close(app_idx, mcl_idx, p_data, TRUE);
}

/*******************************************************************************
**
** Function         bta_hl_cch_mca_connect
**
** Description      Action routine for processing the CCH connect indication
**
** Returns          void
**
*******************************************************************************/
void bta_hl_cch_mca_connect(uint8_t app_idx, uint8_t mcl_idx,  tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL             evt_data;
    tBTA_HL_EVT         event;
    uint8_t             send_event = TRUE;
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_cch_mca_connect mcl_handle=%d ", p_data->mca_evt.mcl_handle);
#endif
    p_mcb->mcl_handle = p_data->mca_evt.mcl_handle;
    bdcpy(p_mcb->bd_addr, p_data->mca_evt.mca_data.connect_ind.bd_addr);
    p_mcb->cch_mtu = p_data->mca_evt.mca_data.connect_ind.mtu;
    bta_sys_conn_open(BTA_ID_HL, p_acb->app_id, p_mcb->bd_addr);

    switch(p_mcb->cch_oper) {
        case BTA_HL_CCH_OP_LOCAL_OPEN:
            bta_hl_build_cch_open_cfm(&evt_data, p_mcb->app_id, p_acb->app_handle,
                                      p_mcb->mcl_handle,
                                      p_mcb->bd_addr,
                                      BTA_HL_STATUS_OK);
            event = BTA_HL_CCH_OPEN_CFM_EVT;
            break;

        case BTA_HL_CCH_OP_REMOTE_OPEN:
            bta_hl_build_cch_open_ind(&evt_data, p_acb->app_handle,
                                      p_mcb->mcl_handle,
                                      p_mcb->bd_addr);
            event = BTA_HL_CCH_OPEN_IND_EVT;
            break;

        default:
            send_event = FALSE;
            break;
    }

    p_mcb->cch_oper = BTA_HL_CCH_OP_NONE;

    if(send_event) {
        p_acb->p_cback(event, (tBTA_HL *) &evt_data);
    }
}

/*******************************************************************************
**
** Function         bta_hl_mcap_ctrl_cback
**
** Description      MCAP control callback function for HL.
**
** Returns          void
**
*******************************************************************************/
void bta_hl_mcap_ctrl_cback(tMCA_HANDLE handle, tMCA_CL mcl, uint8_t event,
                            tMCA_CTRL *p_data)
{
    uint8_t send_event = TRUE;
    uint16_t mca_event;
#if (BTA_HL_DEBUG == TRUE)
#if (BT_TRACE_VERBOSE == TRUE)
    APPL_TRACE_EVENT("bta_hl_mcap_ctrl_cback event[%s]", bta_hl_mcap_evt_code(event));
#else
    APPL_TRACE_EVENT("bta_hl_mcap_ctrl_cback event[0x%02x]", event);
#endif
#endif

    switch(event) {
        case MCA_CREATE_IND_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_CREATE_IND_EVT;
            break;

        case MCA_CREATE_CFM_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_CREATE_CFM_EVT;
            break;

        case MCA_RECONNECT_IND_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_RECONNECT_IND_EVT;
            break;

        case MCA_RECONNECT_CFM_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_RECONNECT_CFM_EVT;
            break;

        case MCA_ABORT_IND_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_ABORT_IND_EVT;
            break;

        case MCA_ABORT_CFM_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_ABORT_CFM_EVT;
            break;

        case MCA_DELETE_IND_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_DELETE_IND_EVT;
            break;

        case MCA_DELETE_CFM_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_DELETE_CFM_EVT;
            break;

        case MCA_CONNECT_IND_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_CONNECT_IND_EVT;
            break;

        case MCA_DISCONNECT_IND_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_DISCONNECT_IND_EVT;
            break;

        case MCA_OPEN_IND_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_OPEN_IND_EVT;
            break;

        case MCA_OPEN_CFM_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_OPEN_CFM_EVT;
            break;

        case MCA_CLOSE_IND_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_CLOSE_IND_EVT;
            break;

        case MCA_CLOSE_CFM_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_CLOSE_CFM_EVT;
            break;

        case MCA_CONG_CHG_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_CONG_CHG_EVT;
            break;

        case MCA_RSP_TOUT_IND_EVT:
            mca_event = (uint16_t) BTA_HL_MCA_RSP_TOUT_IND_EVT;
            break;

        case MCA_ERROR_RSP_EVT:
        default:
            send_event = FALSE;
            break;
    }

    if(send_event) {
        tBTA_HL_MCA_EVT *p_msg =
                        (tBTA_HL_MCA_EVT *)GKI_getbuf(sizeof(tBTA_HL_MCA_EVT));
        p_msg->hdr.event = mca_event;
        p_msg->app_handle = (tBTA_HL_APP_HANDLE) handle;
        p_msg->mcl_handle = (tBTA_HL_MCL_HANDLE) mcl;
        wm_memcpy(&p_msg->mca_data, p_data, sizeof(tMCA_CTRL));
        bta_sys_sendmsg(p_msg);
    }
}

/*******************************************************************************
**
** Function         bta_hl_mcap_data_cback
**
** Description      MCAP data callback function for HL.
**
** Returns          void
**
*******************************************************************************/
void bta_hl_mcap_data_cback(tMCA_DL mdl, BT_HDR *p_pkt)
{
    uint8_t app_idx, mcl_idx, mdl_idx;

    if(bta_hl_find_mdl_idx_using_handle((tBTA_HL_MDL_HANDLE)mdl, &app_idx, &mcl_idx, &mdl_idx)) {
        tBTA_HL_MCA_RCV_DATA_EVT *p_msg =
                        (tBTA_HL_MCA_RCV_DATA_EVT *)GKI_getbuf(sizeof(tBTA_HL_MCA_RCV_DATA_EVT));
        p_msg->hdr.event = BTA_HL_MCA_RCV_DATA_EVT;
        p_msg->app_idx = app_idx;
        p_msg->mcl_idx = mcl_idx;
        p_msg->mdl_idx = mdl_idx;
        p_msg->p_pkt = p_pkt;
        bta_sys_sendmsg(p_msg);
    }
}

/*****************************************************************************
**  Debug Functions
*****************************************************************************/
#if (BTA_HL_DEBUG == TRUE && BT_TRACE_VERBOSE == TRUE)

/*******************************************************************************
**
** Function         bta_hl_mcap_evt_code
**
** Description      get the MCAP event string pointer
**
** Returns          char * - event string pointer
**
*******************************************************************************/
static char *bta_hl_mcap_evt_code(uint8_t evt_code)
{
    switch(evt_code) {
        case MCA_ERROR_RSP_EVT:
            return "MCA_ERROR_RSP_EVT";

        case MCA_CREATE_IND_EVT:
            return "MCA_CREATE_IND_EVT";

        case MCA_CREATE_CFM_EVT:
            return "MCA_CREATE_CFM_EVT";

        case MCA_RECONNECT_IND_EVT:
            return "MCA_RECONNECT_IND_EVT";

        case MCA_RECONNECT_CFM_EVT:
            return "MCA_RECONNECT_CFM_EVT";

        case MCA_ABORT_IND_EVT:
            return "MCA_ABORT_IND_EVT";

        case MCA_ABORT_CFM_EVT:
            return "MCA_ABORT_CFM_EVT";

        case MCA_DELETE_IND_EVT:
            return "MCA_DELETE_IND_EVT";

        case MCA_DELETE_CFM_EVT:
            return "MCA_DELETE_CFM_EVT";

        case MCA_CONNECT_IND_EVT:
            return "MCA_CONNECT_IND_EVT";

        case MCA_DISCONNECT_IND_EVT:
            return "MCA_DISCONNECT_IND_EVT";

        case MCA_OPEN_IND_EVT:
            return "MCA_OPEN_IND_EVT";

        case MCA_OPEN_CFM_EVT:
            return "MCA_OPEN_CFM_EVT";

        case MCA_CLOSE_IND_EVT:
            return "MCA_CLOSE_IND_EVT";

        case MCA_CLOSE_CFM_EVT:
            return "MCA_CLOSE_CFM_EVT";

        case MCA_CONG_CHG_EVT:
            return "MCA_CONG_CHG_EVT";

        case MCA_RSP_TOUT_IND_EVT:
            return "MCA_RSP_TOUT_IND_EVT";

        default:
            return "Unknown MCAP event code";
    }
}


/*******************************************************************************
**
** Function         bta_hl_cback_evt_code
**
** Description      get the HDP event string pointer
**
** Returns          char * - event string pointer
**
*******************************************************************************/
static char *bta_hl_cback_evt_code(uint8_t evt_code)
{
    switch(evt_code) {
        case BTA_HL_CCH_OPEN_IND_EVT:
            return "BTA_HL_CCH_OPEN_IND_EVT";

        case BTA_HL_CCH_OPEN_CFM_EVT:
            return "BTA_HL_CCH_OPEN_CFM_EVT";

        case BTA_HL_CCH_CLOSE_IND_EVT:
            return "BTA_HL_CCH_CLOSE_IND_EVT";

        case     BTA_HL_CCH_CLOSE_CFM_EVT:
            return "BTA_HL_CCH_CLOSE_CFM_EVT";

        case BTA_HL_DCH_OPEN_IND_EVT:
            return "BTA_HL_DCH_OPEN_IND_EVT";

        case BTA_HL_DCH_OPEN_CFM_EVT:
            return "BTA_HL_DCH_OPEN_CFM_EVT";

        case BTA_HL_DCH_CLOSE_IND_EVT:
            return "BTA_HL_DCH_CLOSE_IND_EVT";

        case BTA_HL_DCH_CLOSE_CFM_EVT:
            return "BTA_HL_DCH_CLOSE_CFM_EVT";

        case BTA_HL_DCH_RCV_DATA_IND_EVT:
            return "BTA_HL_DCH_RCV_DATA_IND_EVT";

        case BTA_HL_REGISTER_CFM_EVT:
            return "BTA_HL_REGISTER_CFM_EVT";

        case BTA_HL_DEREGISTER_CFM_EVT:
            return "BTA_HL_DEREGISTER_CFM_EVT";

        case BTA_HL_DCH_RECONNECT_CFM_EVT:
            return "BTA_HL_DCH_RECONNECT_CFM_EVT";

        case BTA_HL_DCH_RECONNECT_IND_EVT:
            return "BTA_HL_DCH_RECONNECT_IND_EVT";

        case BTA_HL_DCH_ECHO_TEST_CFM_EVT:
            return "BTA_HL_DCH_ECHO_TEST_CFM_EVT";

        case BTA_HL_SDP_QUERY_CFM_EVT:
            return "BTA_HL_SDP_QUERY_CFM_EVT";

        case BTA_HL_CONG_CHG_IND_EVT:
            return "BTA_HL_CONG_CHG_IND_EVT";

        case BTA_HL_DCH_CREATE_IND_EVT:
            return "BTA_HL_DCH_CREATE_IND_EVT";

        case BTA_HL_DELETE_MDL_IND_EVT:
            return "BTA_HL_DELETE_MDL_IND_EVT";

        case BTA_HL_DELETE_MDL_CFM_EVT:
            return "BTA_HL_DELETE_MDL_CFM_EVT";

        case BTA_HL_DCH_ABORT_IND_EVT:
            return "BTA_HL_DCH_ABORT_IND_EVT";

        case BTA_HL_DCH_ABORT_CFM_EVT:
            return "BTA_HL_DCH_ABORT_CFM_EVT";

        default:
            return "Unknown HDP event code";
    }
}



/*******************************************************************************
**
** Function         bta_hl_dch_oper_code
**
** Description      Get the DCH operation string
**
** Returns          char * - DCH operation string pointer
**
*******************************************************************************/
static char *bta_hl_dch_oper_code(tBTA_HL_DCH_OPER oper_code)
{
    switch(oper_code) {
        case BTA_HL_DCH_OP_NONE:
            return "BTA_HL_DCH_OP_NONE";

        case BTA_HL_DCH_OP_REMOTE_CREATE:
            return "BTA_HL_DCH_OP_REMOTE_CREATE";

        case BTA_HL_DCH_OP_LOCAL_OPEN:
            return "BTA_HL_DCH_OP_LOCAL_OPEN";

        case BTA_HL_DCH_OP_REMOTE_OPEN:
            return "BTA_HL_DCH_OP_REMOTE_OPEN";

        case BTA_HL_DCH_OP_LOCAL_CLOSE:
            return "BTA_HL_DCH_OP_LOCAL_CLOSE";

        case BTA_HL_DCH_OP_REMOTE_CLOSE:
            return "BTA_HL_DCH_OP_REMOTE_CLOSE";

        case BTA_HL_DCH_OP_LOCAL_DELETE:
            return "BTA_HL_DCH_OP_LOCAL_DELETE";

        case BTA_HL_DCH_OP_REMOTE_DELETE:
            return "BTA_HL_DCH_OP_REMOTE_DELETE";

        case BTA_HL_DCH_OP_LOCAL_RECONNECT:
            return "BTA_HL_DCH_OP_LOCAL_RECONNECT";

        case BTA_HL_DCH_OP_REMOTE_RECONNECT:
            return "BTA_HL_DCH_OP_REMOTE_RECONNECT";

        case BTA_HL_DCH_OP_LOCAL_CLOSE_ECHO_TEST:
            return "BTA_HL_DCH_OP_LOCAL_CLOSE_ECHO_TEST";

        case BTA_HL_DCH_OP_LOCAL_CLOSE_RECONNECT:
            return "BTA_HL_DCH_OP_LOCAL_CLOSE_RECONNECT";

        default:
            return "Unknown DCH oper code";
    }
}


#endif  /* Debug Functions */
#endif /* HL_INCLUDED */
