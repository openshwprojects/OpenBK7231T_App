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
 *  This file implements utility functions for the HeaLth device profile
 *  (HL).
 *
 ******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "bt_target.h"
#include "bt_target.h"
#if defined(BTA_HL_INCLUDED) && (BTA_HL_INCLUDED == TRUE)


#include "bt_common.h"
#include "utl.h"
#include "bta_hl_int.h"
#include "bta_hl_co.h"
#include "mca_defs.h"
#include "mca_api.h"


/*******************************************************************************
**
** Function      bta_hl_set_ctrl_psm_for_dch
**
** Description    This function set the control PSM for the DCH setup
**
** Returns     uint8_t - TRUE - control PSM setting is successful
*******************************************************************************/
uint8_t bta_hl_set_ctrl_psm_for_dch(uint8_t app_idx, uint8_t mcl_idx,
                                    uint8_t mdl_idx, uint16_t ctrl_psm)
{
    tBTA_HL_MCL_CB *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    uint8_t success = TRUE, update_ctrl_psm = FALSE;
    UNUSED(mdl_idx);

    if(p_mcb->sdp.num_recs) {
        if(p_mcb->ctrl_psm != ctrl_psm) {
            /* can not use a different ctrl PSM than the current one*/
            success = FALSE;
        }
    } else {
        /* No SDP info control i.e. channel was opened by the peer */
        update_ctrl_psm = TRUE;
    }

    if(success && update_ctrl_psm) {
        p_mcb->ctrl_psm = ctrl_psm;
    }

#if BTA_HL_DEBUG == TRUE

    if(!success) {
        APPL_TRACE_DEBUG("bta_hl_set_ctrl_psm_for_dch num_recs=%d success=%d update_ctrl_psm=%d ctrl_psm=0x%x ",
                         p_mcb->sdp.num_recs, success, update_ctrl_psm, ctrl_psm);
    }

#endif
    return success;
}


/*******************************************************************************
**
** Function      bta_hl_find_sdp_idx_using_ctrl_psm
**
** Description
**
** Returns      TRUE if found
**
*******************************************************************************/
uint8_t bta_hl_find_sdp_idx_using_ctrl_psm(tBTA_HL_SDP *p_sdp,
        uint16_t ctrl_psm,
        uint8_t *p_sdp_idx)
{
    uint8_t found = FALSE;
    tBTA_HL_SDP_REC     *p_rec;
    uint8_t i;

    if(ctrl_psm != 0) {
        for(i = 0; i < p_sdp->num_recs; i++) {
            p_rec = &p_sdp->sdp_rec[i];

            if(p_rec->ctrl_psm == ctrl_psm) {
                *p_sdp_idx = i;
                found = TRUE;
                break;
            }
        }
    } else {
        *p_sdp_idx = 0;
        found = TRUE;
    }

#if BTA_HL_DEBUG == TRUE

    if(!found) {
        APPL_TRACE_DEBUG("bta_hl_find_sdp_idx_using_ctrl_psm found=%d sdp_idx=%d ctrl_psm=0x%x ",
                         found, *p_sdp_idx, ctrl_psm);
    }

#endif
    return found;
}

/*******************************************************************************
**
** Function      bta_hl_set_user_tx_buf_size
**
** Description  This function sets the user tx buffer size
**
** Returns      uint16_t buf_size
**
*******************************************************************************/

uint16_t bta_hl_set_user_tx_buf_size(uint16_t max_tx_size)
{
    if(max_tx_size > BT_DEFAULT_BUFFER_SIZE) {
        return BTA_HL_LRG_DATA_BUF_SIZE;
    }

    return L2CAP_INVALID_ERM_BUF_SIZE;
}

/*******************************************************************************
**
** Function      bta_hl_set_user_rx_buf_size
**
** Description  This function sets the user rx buffer size
**
** Returns      uint16_t buf_size
**
*******************************************************************************/

uint16_t bta_hl_set_user_rx_buf_size(uint16_t mtu)
{
    if(mtu > BT_DEFAULT_BUFFER_SIZE) {
        return BTA_HL_LRG_DATA_BUF_SIZE;
    }

    return L2CAP_INVALID_ERM_BUF_SIZE;
}



/*******************************************************************************
**
** Function      bta_hl_set_tx_win_size
**
** Description  This function sets the tx window size
**
** Returns      uint8_t tx_win_size
**
*******************************************************************************/
uint8_t bta_hl_set_tx_win_size(uint16_t mtu, uint16_t mps)
{
    uint8_t tx_win_size;

    if(mtu <= mps) {
        tx_win_size = 1;
    } else {
        if(mps > 0) {
            tx_win_size = (mtu / mps) + 1;
        } else {
            APPL_TRACE_ERROR("The MPS is zero");
            tx_win_size = 10;
        }
    }

#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_set_tx_win_size win_size=%d mtu=%d mps=%d",
                     tx_win_size, mtu, mps);
#endif
    return tx_win_size;
}

/*******************************************************************************
**
** Function      bta_hl_set_mps
**
** Description  This function sets the MPS
**
** Returns      uint16_t MPS
**
*******************************************************************************/
uint16_t bta_hl_set_mps(uint16_t mtu)
{
    uint16_t mps;

    if(mtu > BTA_HL_L2C_MPS) {
        mps = BTA_HL_L2C_MPS;
    } else {
        mps = mtu;
    }

#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_set_mps mps=%d mtu=%d",
                     mps, mtu);
#endif
    return mps;
}


/*******************************************************************************
**
** Function      bta_hl_clean_mdl_cb
**
** Description  This function clean up the specified MDL control block
**
** Returns      void
**
*******************************************************************************/
void bta_hl_clean_mdl_cb(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx)
{
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_clean_mdl_cb app_idx=%d mcl_idx=%d mdl_idx=%d",
                     app_idx, mcl_idx, mdl_idx);
#endif
    GKI_free_and_reset_buf((void **)&p_dcb->p_tx_pkt);
    GKI_free_and_reset_buf((void **)&p_dcb->p_rx_pkt);
    GKI_free_and_reset_buf((void **)&p_dcb->p_echo_tx_pkt);
    GKI_free_and_reset_buf((void **)&p_dcb->p_echo_rx_pkt);
    wm_memset((void *)p_dcb, 0, sizeof(tBTA_HL_MDL_CB));
}


/*******************************************************************************
**
** Function      bta_hl_get_buf
**
** Description  This function allocate a buffer based on the specified data size
**
** Returns      BT_HDR *.
**
*******************************************************************************/
BT_HDR *bta_hl_get_buf(uint16_t data_size, uint8_t fcs_use)
{
    size_t size = data_size + L2CAP_MIN_OFFSET + BT_HDR_SIZE + L2CAP_FCS_LEN
                  + L2CAP_EXT_CONTROL_OVERHEAD;

    if(fcs_use) {
        size += L2CAP_FCS_LEN;
    }

    BT_HDR *p_new = (BT_HDR *)GKI_getbuf(size);
    p_new->len = data_size;
    p_new->offset = L2CAP_MIN_OFFSET;
    return p_new;
}

/*******************************************************************************
**
** Function      bta_hl_find_service_in_db
**
** Description  This function check the specified service class(es) can be find in
**              the received SDP database
**
** Returns      uint8_t TRUE - found
**                      FALSE - not found
**
*******************************************************************************/
uint8_t bta_hl_find_service_in_db(uint8_t app_idx, uint8_t mcl_idx,
                                  uint16_t service_uuid,
                                  tSDP_DISC_REC **pp_rec)
{
    tBTA_HL_MCL_CB          *p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    uint8_t found = TRUE;

    switch(service_uuid) {
        case UUID_SERVCLASS_HDP_SINK:
        case UUID_SERVCLASS_HDP_SOURCE:
            if((*pp_rec = SDP_FindServiceInDb(p_mcb->p_db, service_uuid,
                                              *pp_rec)) == NULL) {
                found = FALSE;
            }

            break;

        default:
            if(((*pp_rec = bta_hl_find_sink_or_src_srv_class_in_db(p_mcb->p_db,
                           *pp_rec)) == NULL)) {
                found = FALSE;
            }

            break;
    }

    return found;
}

/*******************************************************************************
**
** Function      bta_hl_get_service_uuids
**
**
** Description  This function finds the service class(es) for both CCH and DCH oeprations
**
** Returns      uint16_t - service_id
**                       if service_uuid = 0xFFFF then it means service uuid
**                       can be either Sink or Source
**
*******************************************************************************/
uint16_t bta_hl_get_service_uuids(uint8_t sdp_oper, uint8_t app_idx, uint8_t mcl_idx,
                                  uint8_t mdl_idx)
{
    tBTA_HL_MDL_CB          *p_dcb;
    uint16_t                  service_uuid = 0xFFFF; /* both Sink and Source */

    switch(sdp_oper) {
        case BTA_HL_SDP_OP_DCH_OPEN_INIT:
        case BTA_HL_SDP_OP_DCH_RECONNECT_INIT:
            p_dcb = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);

            if(p_dcb->local_mdep_id != BTA_HL_ECHO_TEST_MDEP_ID) {
                if(p_dcb->peer_mdep_role == BTA_HL_MDEP_ROLE_SINK) {
                    service_uuid = UUID_SERVCLASS_HDP_SINK;
                } else {
                    service_uuid = UUID_SERVCLASS_HDP_SOURCE;
                }
            }

            break;

        case BTA_HL_SDP_OP_CCH_INIT:
        default:
            /* use default that is both Sink and Source */
            break;
    }

#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_get_service_uuids service_uuid=0x%x", service_uuid);
#endif
    return service_uuid;
}

/*******************************************************************************
**
** Function      bta_hl_find_echo_cfg_rsp
**
**
** Description  This function finds the configuration response for the echo test
**
** Returns      uint8_t - TRUE found
**                        FALSE not found
**
*******************************************************************************/
uint8_t bta_hl_find_echo_cfg_rsp(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdep_idx, uint8_t cfg,
                                 uint8_t *p_cfg_rsp)
{
    tBTA_HL_APP_CB      *p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MDEP        *p_mdep = &p_acb->sup_feature.mdep[mdep_idx];
    uint8_t             status = TRUE;

    if(p_mdep->mdep_id == BTA_HL_ECHO_TEST_MDEP_ID) {
        if((cfg == BTA_HL_DCH_CFG_RELIABLE) || (cfg == BTA_HL_DCH_CFG_STREAMING)) {
            *p_cfg_rsp = cfg;
        } else if(cfg == BTA_HL_DCH_CFG_NO_PREF) {
            *p_cfg_rsp = BTA_HL_DEFAULT_ECHO_TEST_SRC_DCH_CFG;
        } else {
            status = FALSE;
            APPL_TRACE_ERROR("Inavlid echo cfg value");
        }

        return status;
    }

#if BTA_HL_DEBUG == TRUE

    if(!status) {
        APPL_TRACE_DEBUG("bta_hl_find_echo_cfg_rsp status=failed app_idx=%d mcl_idx=%d mdep_idx=%d cfg=%d",
                         app_idx, mcl_idx, mdep_idx, cfg);
    }

#endif
    return status;
}

/*******************************************************************************
**
** Function      bta_hl_validate_dch_cfg
**
** Description  This function validate the DCH configuration
**
** Returns      uint8_t - TRUE cfg is valid
**                        FALSE not valid
**
*******************************************************************************/
uint8_t bta_hl_validate_cfg(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                            uint8_t cfg)
{
    tBTA_HL_MDL_CB      *p_dcb = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    uint8_t is_valid = FALSE;

    if(!bta_hl_is_the_first_reliable_existed(app_idx, mcl_idx) &&
            (cfg != BTA_HL_DCH_CFG_RELIABLE)) {
        APPL_TRACE_ERROR("the first DCH should be a reliable channel");
        return is_valid;
    }

    switch(p_dcb->local_cfg) {
        case BTA_HL_DCH_CFG_NO_PREF:
            if((cfg == BTA_HL_DCH_CFG_RELIABLE) || (cfg == BTA_HL_DCH_CFG_STREAMING)) {
                is_valid = TRUE;
            }

            break;

        case BTA_HL_DCH_CFG_RELIABLE:
        case BTA_HL_DCH_CFG_STREAMING:
            if(p_dcb->local_cfg == cfg) {
                is_valid = TRUE;
            }

            break;

        default:
            break;
    }

#if BTA_HL_DEBUG == TRUE

    if(!is_valid) {
        APPL_TRACE_DEBUG("bta_hl_validate_dch_open_cfg is_valid=%d, cfg=%d", is_valid, cfg);
    }

#endif
    return is_valid;
}

/*******************************************************************************
**
** Function       bta_hl_find_cch_cb_indexes
**
** Description  This function finds the indexes needed for the CCH state machine
**
** Returns      uint8_t - TRUE found
**                        FALSE not found
**
*******************************************************************************/
uint8_t bta_hl_find_cch_cb_indexes(tBTA_HL_DATA *p_msg,
                                   uint8_t *p_app_idx,
                                   uint8_t  *p_mcl_idx)
{
    uint8_t found = FALSE;
    tBTA_HL_MCL_CB      *p_mcb;
    uint8_t               app_idx = 0, mcl_idx = 0;

    switch(p_msg->hdr.event) {
        case BTA_HL_CCH_SDP_OK_EVT:
        case BTA_HL_CCH_SDP_FAIL_EVT:
            app_idx = p_msg->cch_sdp.app_idx;
            mcl_idx = p_msg->cch_sdp.mcl_idx;
            found = TRUE;
            break;

        case BTA_HL_MCA_CONNECT_IND_EVT:
            if(bta_hl_find_app_idx_using_handle(p_msg->mca_evt.app_handle, &app_idx)) {
                if(bta_hl_find_mcl_idx(app_idx, p_msg->mca_evt.mca_data.connect_ind.bd_addr, &mcl_idx)) {
                    /* local initiated */
                    found = TRUE;
                } else if(!bta_hl_find_mcl_idx_using_handle(p_msg->mca_evt.mcl_handle, &app_idx, &mcl_idx) &&
                          bta_hl_find_avail_mcl_idx(app_idx, &mcl_idx)) {
                    /* remote initiated */
                    p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
                    p_mcb->in_use = TRUE;
                    p_mcb->cch_oper = BTA_HL_CCH_OP_REMOTE_OPEN;
                    found = TRUE;
                }
            }

            break;

        case BTA_HL_MCA_DISCONNECT_IND_EVT:
            if(bta_hl_find_mcl_idx_using_handle(p_msg->mca_evt.mcl_handle, &app_idx,  &mcl_idx)) {
                found = TRUE;
            } else if(bta_hl_find_app_idx_using_handle(p_msg->mca_evt.app_handle, &app_idx) &&
                      bta_hl_find_mcl_idx(app_idx, p_msg->mca_evt.mca_data.disconnect_ind.bd_addr, &mcl_idx)) {
                found = TRUE;
            }

            if(found) {
                p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

                if((p_mcb->cch_oper != BTA_HL_CCH_OP_LOCAL_CLOSE)
                        && (p_mcb->cch_oper != BTA_HL_CCH_OP_LOCAL_OPEN)) {
                    p_mcb->cch_oper = BTA_HL_CCH_OP_REMOTE_CLOSE;
                }
            }

            break;

        case BTA_HL_MCA_RSP_TOUT_IND_EVT:
            if(bta_hl_find_mcl_idx_using_handle(p_msg->mca_evt.mcl_handle, &app_idx,  &mcl_idx)) {
                found = TRUE;
            }

            if(found) {
                p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

                if((p_mcb->cch_oper != BTA_HL_CCH_OP_REMOTE_CLOSE)
                        && (p_mcb->cch_oper != BTA_HL_CCH_OP_LOCAL_OPEN)) {
                    p_mcb->cch_oper = BTA_HL_CCH_OP_LOCAL_CLOSE;
                }
            }

            break;

        default:
            break;
    }

    if(found) {
        *p_app_idx = app_idx;
        *p_mcl_idx = mcl_idx;
    }

#if BTA_HL_DEBUG == TRUE

    if(!found) {
        APPL_TRACE_DEBUG("bta_hl_find_cch_cb_indexes event=%s found=%d app_idx=%d mcl_idx=%d",
                         bta_hl_evt_code(p_msg->hdr.event), found, app_idx, mcl_idx);
    }

#endif
    return found;
}

/*******************************************************************************
**
** Function       bta_hl_find_dch_cb_indexes
**
** Description  This function finds the indexes needed for the DCH state machine
**
** Returns      uint8_t - TRUE found
**                        FALSE not found
**
*******************************************************************************/
uint8_t bta_hl_find_dch_cb_indexes(tBTA_HL_DATA *p_msg,
                                   uint8_t *p_app_idx,
                                   uint8_t *p_mcl_idx,
                                   uint8_t *p_mdl_idx)
{
    uint8_t         found = FALSE;
    tBTA_HL_MCL_CB  *p_mcb;
    uint8_t           app_idx = 0, mcl_idx = 0, mdl_idx = 0;

    switch(p_msg->hdr.event) {
        case BTA_HL_MCA_CREATE_CFM_EVT:
            if(bta_hl_find_mcl_idx_using_handle(p_msg->mca_evt.mcl_handle, &app_idx, &mcl_idx) &&
                    bta_hl_find_mdl_idx(app_idx,  mcl_idx,  p_msg->mca_evt.mca_data.create_cfm.mdl_id, &mdl_idx)) {
                found = TRUE;
            }

            break;

        case BTA_HL_MCA_CREATE_IND_EVT:
        case BTA_HL_MCA_RECONNECT_IND_EVT:
            if(bta_hl_find_mcl_idx_using_handle(p_msg->mca_evt.mcl_handle, &app_idx, &mcl_idx) &&
                    bta_hl_find_avail_mdl_idx(app_idx,  mcl_idx, &mdl_idx)) {
                found = TRUE;
            }

            break;

        case BTA_HL_MCA_OPEN_CFM_EVT:
            if(bta_hl_find_mcl_idx_using_handle(p_msg->mca_evt.mcl_handle, &app_idx, &mcl_idx) &&
                    bta_hl_find_mdl_idx(app_idx,  mcl_idx,  p_msg->mca_evt.mca_data.open_cfm.mdl_id, &mdl_idx)) {
                found = TRUE;
            }

            break;

        case BTA_HL_MCA_OPEN_IND_EVT:
            if(bta_hl_find_mcl_idx_using_handle(p_msg->mca_evt.mcl_handle, &app_idx, &mcl_idx) &&
                    bta_hl_find_mdl_idx(app_idx,  mcl_idx,  p_msg->mca_evt.mca_data.open_ind.mdl_id, &mdl_idx)) {
                found = TRUE;
            }

            break;

        case BTA_HL_MCA_CLOSE_CFM_EVT:
            if(bta_hl_find_mdl_idx_using_handle((tBTA_HL_MDL_HANDLE)p_msg->mca_evt.mca_data.close_cfm.mdl,
                                                &app_idx, &mcl_idx, &mdl_idx)) {
                found = TRUE;
            }

            break;

        case BTA_HL_MCA_CLOSE_IND_EVT:
            if(bta_hl_find_mdl_idx_using_handle((tBTA_HL_MDL_HANDLE)p_msg->mca_evt.mca_data.close_ind.mdl,
                                                &app_idx, &mcl_idx, &mdl_idx)) {
                found = TRUE;
            }

            break;

        case BTA_HL_API_SEND_DATA_EVT:
            if(bta_hl_find_mdl_idx_using_handle(p_msg->api_send_data.mdl_handle,
                                                &app_idx, &mcl_idx, &mdl_idx)) {
                found = TRUE;
            }

            break;

        case BTA_HL_MCA_CONG_CHG_EVT:
            if(bta_hl_find_mdl_idx_using_handle((tBTA_HL_MDL_HANDLE)p_msg->mca_evt.mca_data.cong_chg.mdl,
                                                &app_idx, &mcl_idx, &mdl_idx)) {
                found = TRUE;
            }

            break;

        case BTA_HL_MCA_RCV_DATA_EVT:
            app_idx = p_msg->mca_rcv_data_evt.app_idx;
            mcl_idx = p_msg->mca_rcv_data_evt.mcl_idx;
            mdl_idx = p_msg->mca_rcv_data_evt.mdl_idx;
            found = TRUE;
            break;

        case BTA_HL_DCH_RECONNECT_EVT:
        case BTA_HL_DCH_OPEN_EVT:
        case BTA_HL_DCH_ECHO_TEST_EVT:
        case BTA_HL_DCH_SDP_FAIL_EVT:
            app_idx = p_msg->dch_sdp.app_idx;
            mcl_idx = p_msg->dch_sdp.mcl_idx;
            mdl_idx = p_msg->dch_sdp.mdl_idx;
            found = TRUE;
            break;

        case BTA_HL_MCA_RECONNECT_CFM_EVT:
            if(bta_hl_find_mcl_idx_using_handle(p_msg->mca_evt.mcl_handle, &app_idx, &mcl_idx) &&
                    bta_hl_find_mdl_idx(app_idx,  mcl_idx,  p_msg->mca_evt.mca_data.reconnect_cfm.mdl_id, &mdl_idx)) {
                found = TRUE;
            }

            break;

        case BTA_HL_API_DCH_CREATE_RSP_EVT:
            if(bta_hl_find_mcl_idx_using_handle(p_msg->api_dch_create_rsp.mcl_handle, &app_idx, &mcl_idx) &&
                    bta_hl_find_mdl_idx(app_idx,  mcl_idx, p_msg->api_dch_create_rsp.mdl_id, &mdl_idx)) {
                found = TRUE;
            }

            break;

        case BTA_HL_MCA_ABORT_IND_EVT:
            if(bta_hl_find_mcl_idx_using_handle(p_msg->mca_evt.mcl_handle, &app_idx, &mcl_idx) &&
                    bta_hl_find_mdl_idx(app_idx,  mcl_idx, p_msg->mca_evt.mca_data.abort_ind.mdl_id, &mdl_idx)) {
                found = TRUE;
            }

            break;

        case BTA_HL_MCA_ABORT_CFM_EVT:
            if(bta_hl_find_mcl_idx_using_handle(p_msg->mca_evt.mcl_handle, &app_idx,  &mcl_idx) &&
                    bta_hl_find_mdl_idx(app_idx,  mcl_idx,  p_msg->mca_evt.mca_data.abort_cfm.mdl_id, &mdl_idx)) {
                found = TRUE;
            }

            break;

        case BTA_HL_CI_GET_TX_DATA_EVT:
        case BTA_HL_CI_PUT_RX_DATA_EVT:
            if(bta_hl_find_mdl_idx_using_handle(p_msg->ci_get_put_data.mdl_handle, &app_idx, &mcl_idx,
                                                &mdl_idx)) {
                found = TRUE;
            }

            break;

        case BTA_HL_CI_GET_ECHO_DATA_EVT:
        case BTA_HL_CI_PUT_ECHO_DATA_EVT:
            if(bta_hl_find_mcl_idx_using_handle(p_msg->ci_get_put_echo_data.mcl_handle, &app_idx, &mcl_idx)) {
                p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
                mdl_idx = p_mcb->echo_mdl_idx;
                found = TRUE;
            }

            break;

        default:
            break;
    }

    if(found) {
        *p_app_idx = app_idx;
        *p_mcl_idx = mcl_idx;
        *p_mdl_idx = mdl_idx;
    }

#if BTA_HL_DEBUG == TRUE

    if(!found) {
        APPL_TRACE_DEBUG("bta_hl_find_dch_cb_indexes event=%s found=%d app_idx=%d mcl_idx=%d mdl_idx=%d",
                         bta_hl_evt_code(p_msg->hdr.event), found, *p_app_idx, *p_mcl_idx, *p_mdl_idx);
    }

#endif
    return found;
}

/*******************************************************************************
**
** Function      bta_hl_allocate_mdl_id
**
** Description  This function allocates a MDL ID
**
** Returns      uint16_t - MDL ID
**
*******************************************************************************/
uint16_t  bta_hl_allocate_mdl_id(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx)
{
    uint16_t  mdl_id = 0;
    tBTA_HL_MCL_CB      *p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    uint8_t duplicate_id;
    uint8_t i, mdl_cfg_idx;

    do {
        duplicate_id = FALSE;
        mdl_id = ((mdl_id + 1) & 0xFEFF);

        /* check mdl_ids that are used for the current conenctions */
        for(i = 0; i < BTA_HL_NUM_MDLS_PER_MCL; i++) {
            if(p_mcb->mdl[i].in_use &&
                    (i != mdl_idx) &&
                    (p_mcb->mdl[i].mdl_id == mdl_id)) {
                duplicate_id = TRUE;
                break;
            }
        }

        if(duplicate_id) {
            /* start from the beginning to get another MDL value*/
            continue;
        } else {
            /* check mdl_ids that are stored in the persistent memory */
            if(bta_hl_find_mdl_cfg_idx(app_idx, mcl_idx, mdl_id, &mdl_cfg_idx)) {
                duplicate_id = TRUE;
            } else {
                /* found a new MDL value */
                break;
            }
        }
    } while(TRUE);

#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_allocate_mdl OK mdl_id=%d",  mdl_id);
#endif
    return mdl_id;
}
/*******************************************************************************
**
** Function      bta_hl_find_mdl_idx
**
** Description  This function finds the MDL index based on mdl_id
**
** Returns      uint8_t TRUE-found
**
*******************************************************************************/
uint8_t bta_hl_find_mdl_idx(uint8_t app_idx, uint8_t mcl_idx, uint16_t mdl_id,
                            uint8_t *p_mdl_idx)
{
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_MDLS_PER_MCL ; i ++) {
        if(p_mcb->mdl[i].in_use  &&
                (mdl_id != 0) &&
                (p_mcb->mdl[i].mdl_id == mdl_id)) {
            found = TRUE;
            *p_mdl_idx = i;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(!found) {
        APPL_TRACE_DEBUG("bta_hl_find_mdl_idx found=%d mdl_id=%d mdl_idx=%d ",
                         found, mdl_id, i);
    }

#endif
    return found;
}

/*******************************************************************************
**
** Function      bta_hl_find_an_active_mdl_idx
**
** Description  This function finds an active MDL
**
** Returns      uint8_t TRUE-found
**
*******************************************************************************/
uint8_t bta_hl_find_an_active_mdl_idx(uint8_t app_idx, uint8_t mcl_idx,
                                      uint8_t *p_mdl_idx)
{
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_MDLS_PER_MCL ; i ++) {
        if(p_mcb->mdl[i].in_use  &&
                (p_mcb->mdl[i].dch_state == BTA_HL_DCH_OPEN_ST)) {
            found = TRUE;
            *p_mdl_idx = i;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(found) {
        APPL_TRACE_DEBUG("bta_hl_find_an_opened_mdl_idx found=%d app_idx=%d mcl_idx=%d mdl_idx=%d",
                         found, app_idx, mcl_idx, i);
    }

#endif
    return found;
}

/*******************************************************************************
**
** Function      bta_hl_find_dch_setup_mdl_idx
**
** Description  This function finds a MDL which in the DCH setup state
**
** Returns      uint8_t TRUE-found
**
*******************************************************************************/
uint8_t bta_hl_find_dch_setup_mdl_idx(uint8_t app_idx, uint8_t mcl_idx,
                                      uint8_t *p_mdl_idx)
{
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_MDLS_PER_MCL ; i ++) {
        if(p_mcb->mdl[i].in_use  &&
                (p_mcb->mdl[i].dch_state == BTA_HL_DCH_OPENING_ST)) {
            found = TRUE;
            *p_mdl_idx = i;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(found) {
        APPL_TRACE_DEBUG("bta_hl_find_dch_setup_mdl_idx found=%d app_idx=%d mcl_idx=%d mdl_idx=%d",
                         found, app_idx, mcl_idx, i);
    }

#endif
    return found;
}

/*******************************************************************************
**
** Function      bta_hl_find_an_in_use_mcl_idx
**
** Description  This function finds an in-use MCL control block index
**
** Returns      uint8_t TRUE-found
**
*******************************************************************************/
uint8_t bta_hl_find_an_in_use_mcl_idx(uint8_t app_idx,
                                      uint8_t *p_mcl_idx)
{
    tBTA_HL_MCL_CB      *p_mcb;
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_MCLS ; i ++) {
        p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, i);

        if(p_mcb->in_use  &&
                (p_mcb->cch_state != BTA_HL_CCH_IDLE_ST)) {
            found = TRUE;
            *p_mcl_idx = i;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(found) {
        APPL_TRACE_DEBUG("bta_hl_find_an_in_use_mcl_idx found=%d app_idx=%d mcl_idx=%d ",
                         found, app_idx, i);
    }

#endif
    return found;
}


/*******************************************************************************
**
** Function      bta_hl_find_an_in_use_app_idx
**
** Description  This function finds an in-use application control block index
**
** Returns      uint8_t TRUE-found
**
*******************************************************************************/
uint8_t bta_hl_find_an_in_use_app_idx(uint8_t *p_app_idx)
{
    tBTA_HL_APP_CB      *p_acb ;
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        p_acb  = BTA_HL_GET_APP_CB_PTR(i);

        if(p_acb->in_use) {
            found = TRUE;
            *p_app_idx = i;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(found) {
        APPL_TRACE_DEBUG("bta_hl_find_an_in_use_app_idx found=%d app_idx=%d ",
                         found, i);
    }

#endif
    return found;
}
/*******************************************************************************
**
** Function      bta_hl_find_app_idx
**
** Description  This function finds the application control block index based on
**              the application ID
**
** Returns      uint8_t TRUE-found
**
*******************************************************************************/
uint8_t bta_hl_find_app_idx(uint8_t app_id, uint8_t *p_app_idx)
{
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        if(bta_hl_cb.acb[i].in_use &&
                (bta_hl_cb.acb[i].app_id == app_id)) {
            found = TRUE;
            *p_app_idx = i;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_find_app_idx found=%d app_id=%d idx=%d ",
                     found, app_id, i);
#endif
    return found;
}


/*******************************************************************************
**
** Function      bta_hl_find_app_idx_using_handle
**
** Description  This function finds the application control block index based on
**              the application handle
**
** Returns      uint8_t TRUE-found
**
*******************************************************************************/
uint8_t bta_hl_find_app_idx_using_handle(tBTA_HL_APP_HANDLE app_handle,
        uint8_t *p_app_idx)
{
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        if(bta_hl_cb.acb[i].in_use &&
                (bta_hl_cb.acb[i].app_handle == app_handle)) {
            found = TRUE;
            *p_app_idx = i;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(!found) {
        APPL_TRACE_DEBUG("bta_hl_find_app_idx_using_mca_handle status=%d handle=%d app_idx=%d ",
                         found, app_handle, i);
    }

#endif
    return found;
}


/*******************************************************************************
**
** Function      bta_hl_find_mcl_idx_using_handle
**
** Description  This function finds the MCL control block index based on
**              the MCL handle
**
** Returns      uint8_t TRUE-found
**
*******************************************************************************/
uint8_t bta_hl_find_mcl_idx_using_handle(tBTA_HL_MCL_HANDLE mcl_handle,
        uint8_t *p_app_idx, uint8_t *p_mcl_idx)
{
    tBTA_HL_APP_CB  *p_acb;
    uint8_t         found = FALSE;
    uint8_t i = 0, j = 0;

    for(i = 0; i < BTA_HL_NUM_APPS; i++) {
        p_acb = BTA_HL_GET_APP_CB_PTR(i);

        if(p_acb->in_use) {
            for(j = 0; j < BTA_HL_NUM_MCLS ; j++) {
                if(p_acb->mcb[j].mcl_handle == mcl_handle) {
                    found = TRUE;
                    *p_app_idx = i;
                    *p_mcl_idx = j;
                    break;
                }
            }
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(!found) {
        APPL_TRACE_DEBUG("bta_hl_find_mcl_idx_using_handle found=%d app_idx=%d mcl_idx=%d",
                         found, i, j);
    }

#endif
    return found;
}

/*******************************************************************************
**
** Function      bta_hl_find_mcl_idx
**
** Description  This function finds the MCL control block index based on
**              the peer BD address
**
** Returns      uint8_t TRUE-found
**
*******************************************************************************/
uint8_t bta_hl_find_mcl_idx(uint8_t app_idx, BD_ADDR p_bd_addr, uint8_t *p_mcl_idx)
{
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_MCLS ; i ++) {
        if(bta_hl_cb.acb[app_idx].mcb[i].in_use &&
                (!memcmp(bta_hl_cb.acb[app_idx].mcb[i].bd_addr, p_bd_addr, BD_ADDR_LEN))) {
            found = TRUE;
            *p_mcl_idx = i;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(!found) {
        APPL_TRACE_DEBUG("bta_hl_find_mcl_idx found=%d idx=%d",
                         found, i);
    }

#endif
    return found;
}



/*******************************************************************************
**
** Function      bta_hl_find_mdl_idx_using_handle
**
** Description  This function finds the MDL control block index based on
**              the MDL handle
**
** Returns      uint8_t TRUE-found
**
*******************************************************************************/
uint8_t bta_hl_find_mdl_idx_using_handle(tBTA_HL_MDL_HANDLE mdl_handle,
        uint8_t *p_app_idx, uint8_t *p_mcl_idx,
        uint8_t *p_mdl_idx)
{
    tBTA_HL_APP_CB      *p_acb;
    tBTA_HL_MCL_CB      *p_mcb;
    tBTA_HL_MDL_CB      *p_dcb;
    uint8_t found = FALSE;
    uint8_t i, j, k;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        p_acb = BTA_HL_GET_APP_CB_PTR(i);

        if(p_acb->in_use) {
            for(j = 0; j < BTA_HL_NUM_MCLS; j++) {
                p_mcb = BTA_HL_GET_MCL_CB_PTR(i, j);

                if(p_mcb->in_use) {
                    for(k = 0; k < BTA_HL_NUM_MDLS_PER_MCL; k++) {
                        p_dcb = BTA_HL_GET_MDL_CB_PTR(i, j, k);

                        if(p_dcb->in_use) {
                            if(p_dcb->mdl_handle == mdl_handle) {
                                found = TRUE;
                                *p_app_idx = i;
                                *p_mcl_idx = j;
                                *p_mdl_idx = k;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(!found) {
        APPL_TRACE_DEBUG("bta_hl_find_mdl_idx_using_handle found=%d mdl_handle=%d  ",
                         found, mdl_handle);
    }

#endif
    return found;
}
/*******************************************************************************
**
** Function      bta_hl_is_the_first_reliable_existed
**
** Description  This function checks whether the first reliable DCH channel
**              has been setup on the MCL or not
**
** Returns      uint8_t - TRUE exist
**                        FALSE does not exist
**
*******************************************************************************/
uint8_t bta_hl_is_the_first_reliable_existed(uint8_t app_idx, uint8_t mcl_idx)
{
    tBTA_HL_MCL_CB      *p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    uint8_t is_existed = FALSE;
    uint8_t i ;

    for(i = 0; i < BTA_HL_NUM_MDLS_PER_MCL; i++) {
        if(p_mcb->mdl[i].in_use && p_mcb->mdl[i].is_the_first_reliable) {
            is_existed = TRUE;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_is_the_first_reliable_existed is_existed=%d  ", is_existed);
#endif
    return is_existed;
}

/*******************************************************************************
**
** Function      bta_hl_find_non_active_mdl_cfg
**
** Description  This function finds a valid MDL configiration index and this
**              MDL ID is not active
**
** Returns      uint8_t - TRUE found
**                        FALSE not found
**
*******************************************************************************/
uint8_t  bta_hl_find_non_active_mdl_cfg(uint8_t app_idx, uint8_t start_mdl_cfg_idx,
                                        uint8_t *p_mdl_cfg_idx)
{
    tBTA_HL_MCL_CB      *p_mcb;
    tBTA_HL_MDL_CB      *p_dcb;
    tBTA_HL_MDL_CFG     *p_mdl;
    uint8_t             mdl_in_use;
    uint8_t             found = FALSE;
    uint8_t               i, j, k;

    for(i = start_mdl_cfg_idx; i < BTA_HL_NUM_MDL_CFGS; i++) {
        mdl_in_use = FALSE;
        p_mdl = BTA_HL_GET_MDL_CFG_PTR(app_idx, i);

        for(j = 0; j < BTA_HL_NUM_MCLS; j++) {
            p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, j);

            if(p_mcb->in_use &&
                    !memcmp(p_mdl->peer_bd_addr, p_mcb->bd_addr, BD_ADDR_LEN)) {
                for(k = 0; k < BTA_HL_NUM_MDLS_PER_MCL; k++) {
                    p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, j, k);

                    if(p_dcb->in_use &&  p_mdl->mdl_id == p_dcb->mdl_id) {
                        mdl_in_use = TRUE;
                        break;
                    }
                }
            }

            if(mdl_in_use) {
                break;
            }
        }

        if(!mdl_in_use) {
            *p_mdl_cfg_idx = i;
            found = TRUE;
            break;
        }
    }

    return found;
}

/*******************************************************************************
**
** Function      bta_hl_find_mdl_cfg_idx
**
** Description  This function finds an available MDL configuration index
**
** Returns      uint8_t - TRUE found
**                        FALSE not found
**
*******************************************************************************/
uint8_t  bta_hl_find_avail_mdl_cfg_idx(uint8_t app_idx, uint8_t mcl_idx,
                                       uint8_t *p_mdl_cfg_idx)
{
    tBTA_HL_MDL_CFG     *p_mdl, *p_mdl1, *p_mdl2;
    uint8_t               i;
    uint8_t             found = FALSE;
    uint8_t               first_mdl_cfg_idx, second_mdl_cfg_idx, older_mdl_cfg_idx;
    uint8_t             done;
    UNUSED(mcl_idx);

    for(i = 0; i < BTA_HL_NUM_MDL_CFGS; i++) {
        p_mdl = BTA_HL_GET_MDL_CFG_PTR(app_idx, i);

        if(!p_mdl->active) {
            /* found an unused space to store mdl cfg*/
            found = TRUE;
            *p_mdl_cfg_idx = i;
            break;
        }
    }

    if(!found) {
        /* all available mdl cfg spaces are in use so we need to find the mdl cfg which is
        not currently in use and has the the oldest time stamp to remove*/
        found = TRUE;

        if(bta_hl_find_non_active_mdl_cfg(app_idx, 0, &first_mdl_cfg_idx)) {
            if(bta_hl_find_non_active_mdl_cfg(app_idx, (uint8_t)(first_mdl_cfg_idx + 1), &second_mdl_cfg_idx)) {
                done = FALSE;

                while(!done) {
                    p_mdl1 = BTA_HL_GET_MDL_CFG_PTR(app_idx, first_mdl_cfg_idx);
                    p_mdl2 = BTA_HL_GET_MDL_CFG_PTR(app_idx, second_mdl_cfg_idx);

                    if(p_mdl1->time < p_mdl2->time) {
                        older_mdl_cfg_idx =  first_mdl_cfg_idx;
                    } else {
                        older_mdl_cfg_idx =  second_mdl_cfg_idx;
                    }

                    if(bta_hl_find_non_active_mdl_cfg(app_idx, (uint8_t)(second_mdl_cfg_idx + 1),
                                                      &second_mdl_cfg_idx)) {
                        first_mdl_cfg_idx = older_mdl_cfg_idx;
                    } else {
                        done = TRUE;
                    }
                }

                *p_mdl_cfg_idx = older_mdl_cfg_idx;
            } else {
                *p_mdl_cfg_idx = first_mdl_cfg_idx;
            }
        } else {
            found = FALSE;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(!found) {
        APPL_TRACE_DEBUG("bta_hl_find_avail_mdl_cfg_idx found=%d mdl_cfg_idx=%d ", found, *p_mdl_cfg_idx);
    }

#endif
    return found;
}

/*******************************************************************************
**
** Function      bta_hl_find_mdl_cfg_idx
**
** Description  This function finds the MDL configuration index based on
**              the MDL ID
**
** Returns      uint8_t - TRUE found
**                        FALSE not found
**
*******************************************************************************/
uint8_t  bta_hl_find_mdl_cfg_idx(uint8_t app_idx, uint8_t mcl_idx,
                                 tBTA_HL_MDL_ID mdl_id, uint8_t *p_mdl_cfg_idx)
{
    tBTA_HL_MCL_CB      *p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CFG     *p_mdl;
    uint8_t i ;
    uint8_t found = FALSE;
    *p_mdl_cfg_idx = 0;

    for(i = 0; i < BTA_HL_NUM_MDL_CFGS; i++) {
        p_mdl = BTA_HL_GET_MDL_CFG_PTR(app_idx, i);

        if(p_mdl->active)
            APPL_TRACE_DEBUG("bta_hl_find_mdl_cfg_idx: mdl_id =%d, p_mdl->mdl_id=%d", mdl_id,
                             p_mdl->mdl_id);

        if(p_mdl->active &&
                (!memcmp(p_mcb->bd_addr, p_mdl->peer_bd_addr, BD_ADDR_LEN)) &&
                (p_mdl->mdl_id == mdl_id)) {
            found = TRUE;
            *p_mdl_cfg_idx = i;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(!found) {
        APPL_TRACE_DEBUG("bta_hl_find_mdl_cfg_idx found=%d mdl_cfg_idx=%d ", found, i);
    }

#endif
    return found;
}


/*******************************************************************************
**
** Function      bta_hl_get_cur_time
**
** Description  This function get the cuurent time value
**
** Returns      uint8_t - TRUE found
**                        FALSE not found
**
*******************************************************************************/
uint8_t  bta_hl_get_cur_time(uint8_t app_idx, uint8_t *p_cur_time)
{
    tBTA_HL_MDL_CFG     *p_mdl;
    uint8_t i, j, time_latest, time;
    uint8_t found = FALSE, result = TRUE;

    for(i = 0; i < BTA_HL_NUM_MDL_CFGS; i++) {
        p_mdl = BTA_HL_GET_MDL_CFG_PTR(app_idx, i);

        if(p_mdl->active) {
            found = TRUE;
            time_latest = p_mdl->time;

            for(j = (i + 1); j < BTA_HL_NUM_MDL_CFGS; j++) {
                p_mdl = BTA_HL_GET_MDL_CFG_PTR(app_idx, j);

                if(p_mdl->active) {
                    time = p_mdl->time;

                    if(time > time_latest) {
                        time_latest = time;
                    }
                }
            }

            break;
        }
    }

    if(found) {
        if(time_latest < BTA_HL_MAX_TIME) {
            *p_cur_time = time_latest + 1;
        } else {
            /* need to wrap around */
            result = FALSE;
        }
    } else {
        *p_cur_time = BTA_HL_MIN_TIME;
    }

#if BTA_HL_DEBUG == TRUE

    if(!result) {
        APPL_TRACE_DEBUG("bta_hl_get_cur_time result=%s cur_time=%d",
                         (result ? "OK" : "FAIL"), *p_cur_time);
    }

#endif
    return result;
}

/*******************************************************************************
**
** Function      bta_hl_sort_cfg_time_idx
**
** Description  This function sort the mdl configuration idx stored in array a
**              based on decending time value
**
** Returns      uint8_t - TRUE found
**                        FALSE not found
**
*******************************************************************************/
void bta_hl_sort_cfg_time_idx(uint8_t app_idx, uint8_t *a, uint8_t n)
{
    tBTA_HL_APP_CB      *p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);
    uint8_t temp_time, temp_idx;
    int16_t i, j;

    for(i = 1; i < n; ++i) {
        temp_idx = a[i];
        temp_time = p_acb->mdl_cfg[temp_idx].time;
        j = i - 1;

        while((j >= 0) && (temp_time < p_acb->mdl_cfg[a[j]].time)) {
            a[j + 1] = a[j];
            --j;
        }

        a[j + 1] = temp_idx;
    }
}

/*******************************************************************************
**
** Function      bta_hl_compact_mdl_cfg_time
**
** Description  This function finds the MDL configuration index based on
**              the MDL ID
**
** Returns      uint8_t - TRUE found
**                        FALSE not found
**
*******************************************************************************/
void  bta_hl_compact_mdl_cfg_time(uint8_t app_idx, uint8_t mdep_id)
{
    tBTA_HL_MDL_CFG     *p_mdl;
    uint8_t i, time_min, cnt = 0;
    uint8_t   s_arr[BTA_HL_NUM_MDL_CFGS];

    for(i = 0; i < BTA_HL_NUM_MDL_CFGS; i++) {
        p_mdl = BTA_HL_GET_MDL_CFG_PTR(app_idx, i);

        if(p_mdl->active) {
            s_arr[cnt] = i;
            cnt++;
        }
    }

#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_compact_mdl_cfg_time cnt=%d ", cnt);
#endif

    if(cnt) {
        bta_hl_sort_cfg_time_idx(app_idx, s_arr, cnt);
        time_min = BTA_HL_MIN_TIME;

        for(i = 0; i < cnt; i++) {
            p_mdl = BTA_HL_GET_MDL_CFG_PTR(app_idx, s_arr[i]);
            p_mdl->time = time_min + i;
            bta_hl_co_save_mdl(mdep_id, s_arr[i], p_mdl);
        }
    }
}



/*******************************************************************************
**
** Function      bta_hl_is_mdl_exsit_in_mcl
**
** Description  This function checks whether the MDL ID
**              has already existed in teh MCL or not
**
** Returns      uint8_t - TRUE exist
**                        FALSE does not exist
**
*******************************************************************************/
uint8_t  bta_hl_is_mdl_exsit_in_mcl(uint8_t app_idx, BD_ADDR bd_addr,
                                    tBTA_HL_MDL_ID mdl_id)
{
    tBTA_HL_MDL_CFG     *p_mdl;
    uint8_t             found = FALSE;
    uint8_t               i;

    for(i = 0; i < BTA_HL_NUM_MDL_CFGS; i++) {
        p_mdl = BTA_HL_GET_MDL_CFG_PTR(app_idx, i);

        if(p_mdl->active &&
                !memcmp(p_mdl->peer_bd_addr, bd_addr, BD_ADDR_LEN)) {
            if(mdl_id != BTA_HL_DELETE_ALL_MDL_IDS) {
                if(p_mdl->mdl_id == mdl_id) {
                    found = TRUE;
                    break;
                }
            } else {
                found = TRUE;
                break;
            }
        }
    }

    return found;
}

/*******************************************************************************
**
** Function      bta_hl_delete_mdl_cfg
**
** Description  This function delete the specified MDL ID
**
** Returns      uint8_t - TRUE Success
**                        FALSE Failed
**
*******************************************************************************/
uint8_t  bta_hl_delete_mdl_cfg(uint8_t app_idx, BD_ADDR bd_addr,
                               tBTA_HL_MDL_ID mdl_id)
{
    tBTA_HL_MDL_CFG     *p_mdl;
    uint8_t             success = FALSE;
    uint8_t               i;

    for(i = 0; i < BTA_HL_NUM_MDL_CFGS; i++) {
        p_mdl = BTA_HL_GET_MDL_CFG_PTR(app_idx, i);

        if(p_mdl->active &&
                !memcmp(p_mdl->peer_bd_addr, bd_addr, BD_ADDR_LEN)) {
            if(mdl_id != BTA_HL_DELETE_ALL_MDL_IDS) {
                if(p_mdl->mdl_id == mdl_id) {
                    bta_hl_co_delete_mdl(p_mdl->local_mdep_id, i);
                    wm_memset(p_mdl, 0, sizeof(tBTA_HL_MDL_CFG));
                    success = TRUE;
                    break;
                }
            } else {
                bta_hl_co_delete_mdl(p_mdl->local_mdep_id, i);
                wm_memset(p_mdl, 0, sizeof(tBTA_HL_MDL_CFG));
                success = TRUE;
            }
        }
    }

    return success;
}


/*******************************************************************************
**
** Function      bta_hl_is_mdl_value_valid
**
**
** Description  This function checks the specified MDL ID is in valid range or not
**
** Returns      uint8_t - TRUE Success
**                        FALSE Failed
**
** note:   mdl_id range   0x0000 reserved,
**                        0x0001-oxFEFF dynamic range,
**                        0xFF00-0xFFFE reserved,
**                        0xFFFF indicates all MDLs (for delete operation only)
**
*******************************************************************************/
uint8_t  bta_hl_is_mdl_value_valid(tBTA_HL_MDL_ID mdl_id)
{
    uint8_t             status = TRUE;

    if(mdl_id != BTA_HL_DELETE_ALL_MDL_IDS) {
        if(mdl_id != 0) {
            if(mdl_id > BTA_HL_MAX_MDL_VAL) {
                status = FALSE;
            }
        } else {
            status = FALSE;
        }
    }

    return status;
}

/*******************************************************************************
**
** Function      bta_hl_find_mdep_cfg_idx
**
** Description  This function finds the MDEP configuration index based
**                on the local MDEP ID
**
** Returns      uint8_t - TRUE found
**                        FALSE not found
**
*******************************************************************************/
uint8_t bta_hl_find_mdep_cfg_idx(uint8_t app_idx,  tBTA_HL_MDEP_ID local_mdep_id,
                                 uint8_t *p_mdep_cfg_idx)
{
    tBTA_HL_APP_CB      *p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_SUP_FEATURE     *p_sup_feature = &p_acb->sup_feature;
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < p_sup_feature->num_of_mdeps; i++) {
        if(p_sup_feature->mdep[i].mdep_id == local_mdep_id) {
            found = TRUE;
            *p_mdep_cfg_idx = i;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(!found) {
        APPL_TRACE_DEBUG("bta_hl_find_mdep_cfg_idx found=%d mdep_idx=%d local_mdep_id=%d ",
                         found, i, local_mdep_id);
    }

#endif
    return found;
}


/*******************************************************************************
**
** Function      bta_hl_find_rxtx_apdu_size
**
** Description  This function finds the maximum APDU rx and tx sizes based on
**              the MDEP configuration data
**
** Returns      void
**
*******************************************************************************/
void bta_hl_find_rxtx_apdu_size(uint8_t app_idx, uint8_t mdep_cfg_idx,
                                uint16_t *p_rx_apu_size,
                                uint16_t *p_tx_apu_size)
{
    tBTA_HL_APP_CB      *p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MDEP_CFG     *p_mdep_cfg;
    uint8_t i;
    uint16_t max_rx_apdu_size = 0, max_tx_apdu_size = 0;
    p_mdep_cfg = &p_acb->sup_feature.mdep[mdep_cfg_idx].mdep_cfg;

    for(i = 0; i < p_mdep_cfg->num_of_mdep_data_types ; i++) {
        if(max_rx_apdu_size < p_mdep_cfg->data_cfg[i].max_rx_apdu_size) {
            max_rx_apdu_size = p_mdep_cfg->data_cfg[i].max_rx_apdu_size;
        }

        if(max_tx_apdu_size < p_mdep_cfg->data_cfg[i].max_tx_apdu_size) {
            max_tx_apdu_size = p_mdep_cfg->data_cfg[i].max_tx_apdu_size;
        }
    }

    *p_rx_apu_size = max_rx_apdu_size;
    *p_tx_apu_size = max_tx_apdu_size;
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_find_rxtx_apdu_size max_rx_apdu_size=%d max_tx_apdu_size=%d ",
                     max_rx_apdu_size, max_tx_apdu_size);
#endif
}

/*******************************************************************************
**
** Function      bta_hl_validate_peer_cfg
**
** Description  This function validates the peer DCH configuration
**
** Returns      uint8_t - TRUE validation is successful
**                        FALSE validation failed
**
*******************************************************************************/
uint8_t bta_hl_validate_peer_cfg(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                                 tBTA_HL_MDEP_ID peer_mdep_id,
                                 tBTA_HL_MDEP_ROLE peer_mdep_role,
                                 uint8_t sdp_idx)
{
    tBTA_HL_MCL_CB      *p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    tBTA_HL_SDP_REC     *p_rec;
    uint8_t peer_found = FALSE;
    uint8_t i;
    APPL_TRACE_DEBUG("bta_hl_validate_peer_cfg sdp_idx=%d app_idx %d", sdp_idx, app_idx);

    if(p_dcb->local_mdep_id == BTA_HL_ECHO_TEST_MDEP_ID) {
        return TRUE;
    }

    p_rec = &p_mcb->sdp.sdp_rec[sdp_idx];

    for(i = 0; i < p_rec->num_mdeps; i++) {
        APPL_TRACE_DEBUG("mdep_id %d peer_mdep_id %d", p_rec->mdep_cfg[i].mdep_id, peer_mdep_id);
        APPL_TRACE_DEBUG("mdep_role %d peer_mdep_role %d", p_rec->mdep_cfg[i].mdep_role,
                         peer_mdep_role)

        if((p_rec->mdep_cfg[i].mdep_id == peer_mdep_id) &&
                (p_rec->mdep_cfg[i].mdep_role == peer_mdep_role)) {
            peer_found = TRUE;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(!peer_found) {
        APPL_TRACE_DEBUG("bta_hl_validate_peer_cfg failed num_mdeps=%d", p_rec->num_mdeps);
    }

#endif
    return peer_found;
}

/*******************************************************************************
**
** Function      bta_hl_chk_local_cfg
**
** Description  This function check whether the local DCH configuration is OK or not
**
** Returns      tBTA_HL_STATUS - OK - local DCH configuration is OK
**                               NO_FIRST_RELIABLE - the streaming DCH configuration
**                                                   is not OK and it needs to use
**                                                   reliable DCH configuration
**
*******************************************************************************/
tBTA_HL_STATUS bta_hl_chk_local_cfg(uint8_t app_idx, uint8_t mcl_idx,
                                    uint8_t mdep_cfg_idx,
                                    tBTA_HL_DCH_CFG local_cfg)
{
    tBTA_HL_APP_CB      *p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_STATUS status = BTA_HL_STATUS_OK;

    if(mdep_cfg_idx &&
            (p_acb->sup_feature.mdep[mdep_cfg_idx].mdep_cfg.mdep_role == BTA_HL_MDEP_ROLE_SOURCE) &&
            (!bta_hl_is_the_first_reliable_existed(app_idx, mcl_idx)) &&
            (local_cfg != BTA_HL_DCH_CFG_RELIABLE)) {
        status =  BTA_HL_STATUS_NO_FIRST_RELIABLE;
        APPL_TRACE_ERROR("BTA_HL_STATUS_INVALID_DCH_CFG");
    }

    return status;
}


/*******************************************************************************
**
** Function      bta_hl_validate_reconnect_params
**
** Description  This function validates the reconnect parameters
**
** Returns      uint8_t - TRUE validation is successful
**                        FALSE validation failed
*******************************************************************************/
uint8_t bta_hl_validate_reconnect_params(uint8_t app_idx, uint8_t mcl_idx,
        tBTA_HL_API_DCH_RECONNECT *p_reconnect,
        uint8_t *p_mdep_cfg_idx, uint8_t *p_mdl_cfg_idx)
{
    tBTA_HL_APP_CB      *p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_SUP_FEATURE *p_sup_feature = &p_acb->sup_feature;
    uint8_t               num_mdeps;
    uint8_t               mdl_cfg_idx;
    uint8_t local_mdep_id_found = FALSE;
    uint8_t mdl_cfg_found = FALSE;
    uint8_t            status = FALSE;
    uint8_t i, in_use_mdl_idx = 0;
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_validate_reconnect_params  mdl_id=%d app_idx=%d", p_reconnect->mdl_id,
                     app_idx);
#endif

    if(bta_hl_find_mdl_cfg_idx(app_idx, mcl_idx, p_reconnect->mdl_id, &mdl_cfg_idx)) {
        mdl_cfg_found = TRUE;
    }

#if BTA_HL_DEBUG == TRUE

    if(!mdl_cfg_found) {
        APPL_TRACE_DEBUG("mdl_cfg_found not found");
    }

#endif

    if(mdl_cfg_found) {
        num_mdeps = p_sup_feature->num_of_mdeps;

        for(i = 0; i < num_mdeps ; i++) {
            if(p_sup_feature->mdep[i].mdep_id == p_acb->mdl_cfg[mdl_cfg_idx].local_mdep_id) {
                local_mdep_id_found = TRUE;
                *p_mdep_cfg_idx = i;
                *p_mdl_cfg_idx = mdl_cfg_idx;
                break;
            }
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(!local_mdep_id_found) {
        APPL_TRACE_DEBUG("local_mdep_id not found");
    }

#endif

    if(local_mdep_id_found) {
        if(!bta_hl_find_mdl_idx(app_idx, mcl_idx, p_reconnect->mdl_id, &in_use_mdl_idx)) {
            status = TRUE;
        } else {
            APPL_TRACE_ERROR("mdl_id=%d is curreltly in use", p_reconnect->mdl_id);
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(!status) {
        APPL_TRACE_DEBUG("Reconnect validation failed local_mdep_id found=%d mdl_cfg_idx found=%d in_use_mdl_idx=%d ",
                         local_mdep_id_found,  mdl_cfg_found, in_use_mdl_idx);
    }

#endif
    return status;
}

/*******************************************************************************
**
** Function      bta_hl_find_avail_mcl_idx
**
** Returns      uint8_t - TRUE found
**                        FALSE not found
**
*******************************************************************************/
uint8_t bta_hl_find_avail_mcl_idx(uint8_t app_idx, uint8_t *p_mcl_idx)
{
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_MCLS ; i ++) {
        if(!bta_hl_cb.acb[app_idx].mcb[i].in_use) {
            found = TRUE;
            *p_mcl_idx = i;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(!found) {
        APPL_TRACE_DEBUG("bta_hl_find_avail_mcl_idx found=%d idx=%d",
                         found, i);
    }

#endif
    return found;
}



/*******************************************************************************
**
** Function      bta_hl_find_avail_mdl_idx
**
** Description  This function finds an available MDL control block index
**
** Returns      uint8_t - TRUE found
**                        FALSE not found
**
*******************************************************************************/
uint8_t bta_hl_find_avail_mdl_idx(uint8_t app_idx, uint8_t mcl_idx,
                                  uint8_t *p_mdl_idx)
{
    tBTA_HL_MCL_CB      *p_mcb = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_MDLS_PER_MCL ; i ++) {
        if(!p_mcb->mdl[i].in_use) {
            wm_memset((void *)&p_mcb->mdl[i], 0, sizeof(tBTA_HL_MDL_CB));
            found = TRUE;
            *p_mdl_idx = i;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(!found) {
        APPL_TRACE_DEBUG("bta_hl_find_avail_mdl_idx found=%d idx=%d",
                         found, i);
    }

#endif
    return found;
}

/*******************************************************************************
**
** Function      bta_hl_is_a_duplicate_id
**
** Description  This function finds the application has been used or not
**
** Returns      uint8_t - TRUE the app_id is a duplicate ID
**                        FALSE not a duplicate ID
*******************************************************************************/
uint8_t bta_hl_is_a_duplicate_id(uint8_t app_id)
{
    uint8_t is_duplicate = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        if(bta_hl_cb.acb[i].in_use &&
                (bta_hl_cb.acb[i].app_id == app_id)) {
            is_duplicate = TRUE;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(is_duplicate) {
        APPL_TRACE_DEBUG("bta_hl_is_a_duplicate_id app_id=%d is_duplicate=%d",
                         app_id, is_duplicate);
    }

#endif
    return is_duplicate;
}


/*******************************************************************************
**
** Function      bta_hl_find_avail_app_idx
**
** Description  This function finds an available application control block index
**
** Returns      uint8_t - TRUE found
**                        FALSE not found
**
*******************************************************************************/
uint8_t bta_hl_find_avail_app_idx(uint8_t *p_idx)
{
    uint8_t found = FALSE;
    uint8_t i;

    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        if(!bta_hl_cb.acb[i].in_use) {
            found = TRUE;
            *p_idx = i;
            break;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(!found) {
        APPL_TRACE_DEBUG("bta_hl_find_avail_app_idx found=%d app_idx=%d",
                         found, i);
    }

#endif
    return found;
}

/*******************************************************************************
**
** Function      bta_hl_app_update
**
** Description  This function registers an HDP application MCAP and DP
**
** Returns      tBTA_HL_STATUS -registration status
**
*******************************************************************************/
tBTA_HL_STATUS bta_hl_app_update(uint8_t app_id, uint8_t is_register)
{
    tBTA_HL_STATUS  status = BTA_HL_STATUS_OK;
    tBTA_HL_APP_CB  *p_acb = BTA_HL_GET_APP_CB_PTR(0);
    tMCA_CS         mca_cs;
    uint8_t           i, mdep_idx, num_of_mdeps;
    uint8_t           mdep_counter = 0;
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_app_update app_id=%d", app_id);
#endif

    if(is_register) {
        if((status == BTA_HL_STATUS_OK) &&
                bta_hl_co_get_num_of_mdep(app_id, &num_of_mdeps)) {
            for(i = 0; i < num_of_mdeps; i++) {
                mca_cs.type = MCA_TDEP_DATA;
                mca_cs.max_mdl = BTA_HL_NUM_MDLS_PER_MDEP;
                mca_cs.p_data_cback = bta_hl_mcap_data_cback;

                /* Find the first available mdep index, and create a MDL Endpoint */
                // make a function later if needed
                for(mdep_idx = 1; mdep_idx < BTA_HL_NUM_MDEPS; mdep_idx++) {
                    if(p_acb->sup_feature.mdep[mdep_idx].mdep_id == 0) {
                        break; /* We found an available index */
                    } else {
                        mdep_counter++;
                    }
                }

                /* If no available MDEPs, return error */
                if(mdep_idx == BTA_HL_NUM_MDEPS) {
                    APPL_TRACE_ERROR("bta_hl_app_update: Out of MDEP IDs");
                    status = BTA_HL_STATUS_MCAP_REG_FAIL;
                    break;
                }

                if(MCA_CreateDep((tMCA_HANDLE)p_acb->app_handle,
                                 &(p_acb->sup_feature.mdep[mdep_idx].mdep_id), &mca_cs) == MCA_SUCCESS) {
                    if(bta_hl_co_get_mdep_config(app_id,
                                                 mdep_idx,
                                                 mdep_counter,
                                                 p_acb->sup_feature.mdep[mdep_idx].mdep_id,
                                                 &p_acb->sup_feature.mdep[mdep_idx].mdep_cfg)) {
                        p_acb->sup_feature.mdep[mdep_idx].ori_app_id = app_id;
                        APPL_TRACE_DEBUG("mdep idx %d id %d ori_app_id %d num data type %d", mdep_idx,
                                         p_acb->sup_feature.mdep[mdep_idx].mdep_id,
                                         p_acb->sup_feature.mdep[mdep_idx].ori_app_id,
                                         p_acb->sup_feature.mdep[mdep_idx].mdep_cfg.num_of_mdep_data_types);

                        if(p_acb->sup_feature.mdep[mdep_idx].mdep_cfg.mdep_role ==
                                BTA_HL_MDEP_ROLE_SOURCE) {
                            p_acb->sup_feature.app_role_mask |= BTA_HL_MDEP_ROLE_MASK_SOURCE;
                        } else if(p_acb->sup_feature.mdep[mdep_idx].mdep_cfg.mdep_role ==
                                  BTA_HL_MDEP_ROLE_SINK) {
                            p_acb->sup_feature.app_role_mask |= BTA_HL_MDEP_ROLE_MASK_SINK;
                        } else {
                            APPL_TRACE_ERROR("bta_hl_app_registration: Invalid Role %d",
                                             p_acb->sup_feature.mdep[mdep_idx].mdep_cfg.mdep_role);
                            status = BTA_HL_STATUS_MDEP_CO_FAIL;
                            break;
                        }
                    } else {
                        APPL_TRACE_ERROR("bta_hl_app_registration: Cfg callout failed");
                        status = BTA_HL_STATUS_MDEP_CO_FAIL;
                        break;
                    }
                } else {
                    APPL_TRACE_ERROR("bta_hl_app_registration: MCA_CreateDep failed");
                    status = BTA_HL_STATUS_MCAP_REG_FAIL;
                    break;
                }
            }

            p_acb->sup_feature.num_of_mdeps += num_of_mdeps;
            APPL_TRACE_DEBUG("num_of_mdeps %d", p_acb->sup_feature.num_of_mdeps);

            if((status == BTA_HL_STATUS_OK) &&
                    (p_acb->sup_feature.app_role_mask == BTA_HL_MDEP_ROLE_MASK_SOURCE)) {
                p_acb->sup_feature.advertize_source_sdp =
                                bta_hl_co_advrtise_source_sdp(app_id);
            }

            if((status == BTA_HL_STATUS_OK) &&
                    (!bta_hl_co_get_echo_config(app_id, &p_acb->sup_feature.echo_cfg))) {
                status = BTA_HL_STATUS_ECHO_CO_FAIL;
            }

            if((status == BTA_HL_STATUS_OK) &&
                    (!bta_hl_co_load_mdl_config(app_id, BTA_HL_NUM_MDL_CFGS, &p_acb->mdl_cfg[0]))) {
                status = BTA_HL_STATUS_MDL_CFG_CO_FAIL;
            }
        } else {
            status = BTA_HL_STATUS_MDEP_CO_FAIL;
        }
    } else {
        for(i = 1; i < BTA_HL_NUM_MDEPS; i++) {
            if(p_acb->sup_feature.mdep[i].ori_app_id == app_id) {
                APPL_TRACE_DEBUG("Found index %", i);

                if(MCA_DeleteDep((tMCA_HANDLE)p_acb->app_handle,
                                 (p_acb->sup_feature.mdep[i].mdep_id)) != MCA_SUCCESS) {
                    APPL_TRACE_ERROR("Error deregistering");
                    status = BTA_HL_STATUS_MCAP_REG_FAIL;
                    return status;
                }

                wm_memset(&p_acb->sup_feature.mdep[i], 0, sizeof(tBTA_HL_MDEP));
            }
        }
    }

    if(status == BTA_HL_STATUS_OK) {
        /* Register/Update MDEP(s) in SDP Record */
        status = bta_hl_sdp_update(app_id);
    }

    /* else do cleanup */
    return status;
}


/*******************************************************************************
**
** Function      bta_hl_app_registration
**
** Description  This function registers an HDP application MCAP and DP
**
** Returns      tBTA_HL_STATUS -registration status
**
*******************************************************************************/
tBTA_HL_STATUS bta_hl_app_registration(uint8_t app_idx)
{
    tBTA_HL_STATUS  status = BTA_HL_STATUS_OK;
    tBTA_HL_APP_CB  *p_acb = BTA_HL_GET_APP_CB_PTR(app_idx);
    tMCA_REG        reg;
    tMCA_CS         mca_cs;
    uint8_t           i, num_of_mdeps;
    uint8_t           mdep_counter = 0;
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_app_registration app_idx=%d", app_idx);
#endif
    reg.ctrl_psm = p_acb->ctrl_psm;
    reg.data_psm = p_acb->data_psm;
    reg.sec_mask = p_acb->sec_mask;
    reg.rsp_tout = BTA_HL_MCAP_RSP_TOUT;

    if((p_acb->app_handle = (tBTA_HL_APP_HANDLE) MCA_Register(&reg, bta_hl_mcap_ctrl_cback)) != 0) {
        mca_cs.type = MCA_TDEP_ECHO;
        mca_cs.max_mdl = BTA_HL_NUM_MDLS_PER_MDEP;
        mca_cs.p_data_cback = bta_hl_mcap_data_cback;

        if(MCA_CreateDep((tMCA_HANDLE)p_acb->app_handle,
                         &(p_acb->sup_feature.mdep[0].mdep_id),
                         &mca_cs) == MCA_SUCCESS) {
            if(p_acb->sup_feature.mdep[0].mdep_id != BTA_HL_ECHO_TEST_MDEP_ID) {
                status = BTA_HL_STATUS_MCAP_REG_FAIL;
                APPL_TRACE_ERROR("BAD MDEP ID for echo test mdep_id=%d",
                                 p_acb->sup_feature.mdep[0].mdep_id);
            }
        } else {
            status = BTA_HL_STATUS_MCAP_REG_FAIL;
            APPL_TRACE_ERROR("MCA_CreateDep for echo test(mdep_id=0) failed");
        }

        if((status == BTA_HL_STATUS_OK) &&
                bta_hl_co_get_num_of_mdep(p_acb->app_id, &num_of_mdeps)) {
            p_acb->sup_feature.num_of_mdeps = num_of_mdeps + 1;

            for(i = 1; i < p_acb->sup_feature.num_of_mdeps; i++) {
                mca_cs.type = MCA_TDEP_DATA;
                mca_cs.max_mdl = BTA_HL_NUM_MDLS_PER_MDEP;
                mca_cs.p_data_cback = bta_hl_mcap_data_cback;

                if(MCA_CreateDep((tMCA_HANDLE)p_acb->app_handle,
                                 &(p_acb->sup_feature.mdep[i].mdep_id), &mca_cs) == MCA_SUCCESS) {
                    if(bta_hl_co_get_mdep_config(p_acb->app_id,
                                                 i, mdep_counter,
                                                 p_acb->sup_feature.mdep[i].mdep_id,
                                                 &p_acb->sup_feature.mdep[i].mdep_cfg)) {
                        if(p_acb->sup_feature.mdep[i].mdep_cfg.mdep_role == BTA_HL_MDEP_ROLE_SOURCE) {
                            p_acb->sup_feature.app_role_mask |= BTA_HL_MDEP_ROLE_MASK_SOURCE;
                        } else if(p_acb->sup_feature.mdep[i].mdep_cfg.mdep_role == BTA_HL_MDEP_ROLE_SINK) {
                            p_acb->sup_feature.app_role_mask |= BTA_HL_MDEP_ROLE_MASK_SINK;
                        } else {
                            status = BTA_HL_STATUS_MDEP_CO_FAIL;
                            break;
                        }

                        p_acb->sup_feature.mdep[i].ori_app_id = p_acb->app_id;
                        APPL_TRACE_DEBUG("index %d ori_app_id %d", i,
                                         p_acb->sup_feature.mdep[i].ori_app_id);
                    } else {
                        status = BTA_HL_STATUS_MDEP_CO_FAIL;
                        break;
                    }
                } else {
                    status = BTA_HL_STATUS_MCAP_REG_FAIL;
                    break;
                }
            }

            if((status == BTA_HL_STATUS_OK) &&
                    (p_acb->sup_feature.app_role_mask == BTA_HL_MDEP_ROLE_MASK_SOURCE)) {
                /* this is a source only applciation */
                p_acb->sup_feature.advertize_source_sdp =
                                bta_hl_co_advrtise_source_sdp(p_acb->app_id);
            }

            if((status == BTA_HL_STATUS_OK) &&
                    (!bta_hl_co_get_echo_config(p_acb->app_id, &p_acb->sup_feature.echo_cfg))) {
                status = BTA_HL_STATUS_ECHO_CO_FAIL;
            }

            if((status == BTA_HL_STATUS_OK) &&
                    (!bta_hl_co_load_mdl_config(p_acb->app_id, BTA_HL_NUM_MDL_CFGS, &p_acb->mdl_cfg[0]))) {
                status = BTA_HL_STATUS_MDL_CFG_CO_FAIL;
            }
        } else {
            status = BTA_HL_STATUS_MDEP_CO_FAIL;
        }
    } else {
        status = BTA_HL_STATUS_MCAP_REG_FAIL;
    }

    if(status == BTA_HL_STATUS_OK) {
        status = bta_hl_sdp_register(app_idx);
    }

    return status;
}


/*******************************************************************************
**
** Function         bta_hl_discard_data
**
** Description  This function discard an HDP event
**
** Returns     void
**
*******************************************************************************/
void bta_hl_discard_data(uint16_t event, tBTA_HL_DATA *p_data)
{
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_ERROR("BTA HL Discard event=%s", bta_hl_evt_code(event));
#endif

    switch(event) {
        case BTA_HL_API_SEND_DATA_EVT:
            break;

        case BTA_HL_MCA_RCV_DATA_EVT:
            GKI_free_and_reset_buf((void **)&p_data->mca_rcv_data_evt.p_pkt);
            break;

        default:
            /*Nothing to free*/
            break;
    }
}

/*******************************************************************************
**
** Function         bta_hl_save_mdl_cfg
**
** Description    This function saves the MDL configuration
**
** Returns     void
**
*******************************************************************************/
void bta_hl_save_mdl_cfg(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    uint8_t mdl_cfg_idx;
    tBTA_HL_MDL_ID mdl_id;
    uint8_t      found = TRUE;
    tBTA_HL_MDL_CFG mdl_cfg;
    tBTA_HL_MDEP *p_mdep_cfg;
    tBTA_HL_L2CAP_CFG_INFO l2cap_cfg;
    uint8_t time_val = 0;
    mdl_id = p_dcb->mdl_id;

    if(!bta_hl_find_mdl_cfg_idx(app_idx, mcl_idx, mdl_id, &mdl_cfg_idx)) {
        if(!bta_hl_find_avail_mdl_cfg_idx(app_idx, mcl_idx, &mdl_cfg_idx)) {
            APPL_TRACE_ERROR("No space to save the MDL config");
            found = FALSE; /*no space available*/
        }
    }

    if(found) {
        bta_hl_get_l2cap_cfg(p_dcb->mdl_handle, &l2cap_cfg);

        if(!bta_hl_get_cur_time(app_idx, &time_val)) {
            bta_hl_compact_mdl_cfg_time(app_idx, p_dcb->local_mdep_id);
            bta_hl_get_cur_time(app_idx, &time_val);
        }

        mdl_cfg.active = TRUE;
        mdl_cfg.time = time_val;
        mdl_cfg.mdl_id  = p_dcb->mdl_id;
        mdl_cfg.dch_mode = p_dcb->dch_mode;
        mdl_cfg.mtu = l2cap_cfg.mtu;
        mdl_cfg.fcs = l2cap_cfg.fcs;
        bdcpy(mdl_cfg.peer_bd_addr, p_mcb->bd_addr);
        mdl_cfg.local_mdep_id = p_dcb->local_mdep_id;
        p_mdep_cfg = &p_acb->sup_feature.mdep[p_dcb->local_mdep_cfg_idx];
        mdl_cfg.local_mdep_role = p_mdep_cfg->mdep_cfg.mdep_role;
        wm_memcpy(&p_acb->mdl_cfg[mdl_cfg_idx], &mdl_cfg, sizeof(tBTA_HL_MDL_CFG));
        bta_hl_co_save_mdl(mdl_cfg.local_mdep_id, mdl_cfg_idx, &mdl_cfg);
    }

#if BTA_HL_DEBUG == TRUE

    if(found) {
        if(p_dcb->mtu != l2cap_cfg.mtu) {
            APPL_TRACE_WARNING("MCAP and L2CAP peer mtu size out of sync from MCAP mtu=%d from l2cap mtu=%d",
                               p_dcb->mtu, l2cap_cfg.mtu);
        }

        APPL_TRACE_DEBUG("bta_hl_save_mdl_cfg saved=%d", found);
        APPL_TRACE_DEBUG("Saved. L2cap cfg mdl_id=%d mtu=%d fcs=%d dch_mode=%d",
                         mdl_cfg.mdl_id, mdl_cfg.mtu, mdl_cfg.fcs,  mdl_cfg.dch_mode);
    }

#endif
}

/*******************************************************************************
**
** Function      bta_hl_set_dch_chan_cfg
**
** Description    This function setups the L2CAP DCH channel configuration
**
** Returns     void
*******************************************************************************/
void bta_hl_set_dch_chan_cfg(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx,
                             tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MDL_CB *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    uint8_t l2cap_mode = L2CAP_FCR_ERTM_MODE;
    tBTA_HL_SUP_FEATURE *p_sup_feature = &p_acb->sup_feature;
    uint8_t local_mdep_cfg_idx = p_dcb->local_mdep_cfg_idx;

    switch(p_dcb->dch_oper) {
        case BTA_HL_DCH_OP_LOCAL_RECONNECT:
        case BTA_HL_DCH_OP_REMOTE_RECONNECT:
            if(p_dcb->dch_mode  == BTA_HL_DCH_MODE_STREAMING) {
                l2cap_mode = L2CAP_FCR_STREAM_MODE;
            }

            break;

        case BTA_HL_DCH_OP_LOCAL_OPEN:
            if(p_data->mca_evt.mca_data.create_cfm.cfg == BTA_HL_DCH_CFG_STREAMING) {
                l2cap_mode = L2CAP_FCR_STREAM_MODE;
            }

            break;

        case BTA_HL_DCH_OP_REMOTE_OPEN:
            if(p_dcb->local_cfg == BTA_HL_DCH_CFG_STREAMING) {
                l2cap_mode = L2CAP_FCR_STREAM_MODE;
            }

            break;

        default:
            APPL_TRACE_ERROR("Invalid dch oper=%d for set dch chan cfg", p_dcb->dch_oper);
            break;
    }

    p_dcb->chnl_cfg.fcr_opt.mode        = l2cap_mode;
    p_dcb->chnl_cfg.fcr_opt.mps         = bta_hl_set_mps(p_dcb->max_rx_apdu_size);
    p_dcb->chnl_cfg.fcr_opt.tx_win_sz   = bta_hl_set_tx_win_size(p_dcb->max_rx_apdu_size,
                                          p_dcb->chnl_cfg.fcr_opt.mps);
    p_dcb->chnl_cfg.fcr_opt.max_transmit = BTA_HL_L2C_MAX_TRANSMIT;
    p_dcb->chnl_cfg.fcr_opt.rtrans_tout = BTA_HL_L2C_RTRANS_TOUT;
    p_dcb->chnl_cfg.fcr_opt.mon_tout    = BTA_HL_L2C_MON_TOUT;
    p_dcb->chnl_cfg.user_rx_buf_size    = bta_hl_set_user_rx_buf_size(p_dcb->max_rx_apdu_size);
    p_dcb->chnl_cfg.user_tx_buf_size    = bta_hl_set_user_tx_buf_size(p_dcb->max_tx_apdu_size);
    p_dcb->chnl_cfg.fcr_rx_buf_size     = L2CAP_INVALID_ERM_BUF_SIZE;
    p_dcb->chnl_cfg.fcr_tx_buf_size     = L2CAP_INVALID_ERM_BUF_SIZE;
    p_dcb->chnl_cfg.data_mtu            = p_dcb->max_rx_apdu_size;
    p_dcb->chnl_cfg.fcs = BTA_HL_MCA_NO_FCS;

    if(local_mdep_cfg_idx !=  BTA_HL_ECHO_TEST_MDEP_CFG_IDX) {
        if(p_sup_feature->mdep[local_mdep_cfg_idx].mdep_cfg.mdep_role ==
                BTA_HL_MDEP_ROLE_SOURCE) {
            p_dcb->chnl_cfg.fcs = BTA_HL_DEFAULT_SOURCE_FCS;
        }
    } else {
        p_dcb->chnl_cfg.fcs = BTA_HL_MCA_USE_FCS;
    }

#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("L2CAP Params l2cap_mode[3-ERTM 4-STREAM]=%d", l2cap_mode);
    APPL_TRACE_DEBUG("Use FCS =%s mtu=%d", ((p_dcb->chnl_cfg.fcs & 1) ? "YES" : "NO"),
                     p_dcb->chnl_cfg.data_mtu);
    APPL_TRACE_DEBUG("tx_win_sz=%d, max_transmit=%d, rtrans_tout=%d, mon_tout=%d, mps=%d",
                     p_dcb->chnl_cfg.fcr_opt.tx_win_sz,
                     p_dcb->chnl_cfg.fcr_opt.max_transmit,
                     p_dcb->chnl_cfg.fcr_opt.rtrans_tout,
                     p_dcb->chnl_cfg.fcr_opt.mon_tout,
                     p_dcb->chnl_cfg.fcr_opt.mps);
    APPL_TRACE_DEBUG("USER rx_buf_size=%d, tx_buf_size=%d, FCR rx_buf_size=%d, tx_buf_size=%d",
                     p_dcb->chnl_cfg.user_rx_buf_size,
                     p_dcb->chnl_cfg.user_tx_buf_size,
                     p_dcb->chnl_cfg.fcr_rx_buf_size,
                     p_dcb->chnl_cfg.fcr_tx_buf_size);
#endif
}

/*******************************************************************************
**
** Function      bta_hl_get_l2cap_cfg
**
** Description    This function get the current L2CAP channel configuration
**
** Returns     uint8_t - TRUE - operation is successful
*******************************************************************************/
uint8_t bta_hl_get_l2cap_cfg(tBTA_HL_MDL_HANDLE mdl_hnd, tBTA_HL_L2CAP_CFG_INFO *p_cfg)
{
    uint8_t success = FALSE;
    uint16_t lcid;
    tL2CAP_CFG_INFO *p_our_cfg;
    tL2CAP_CH_CFG_BITS our_cfg_bits;
    tL2CAP_CFG_INFO *p_peer_cfg;
    tL2CAP_CH_CFG_BITS peer_cfg_bits;
    lcid = MCA_GetL2CapChannel((tMCA_DL) mdl_hnd);

    if(lcid &&
            L2CA_GetCurrentConfig(lcid, &p_our_cfg, &our_cfg_bits, &p_peer_cfg,
                                  &peer_cfg_bits)) {
        p_cfg->fcs = BTA_HL_MCA_NO_FCS;

        if(our_cfg_bits & L2CAP_CH_CFG_MASK_FCS) {
            p_cfg->fcs |= p_our_cfg->fcs;
        } else {
            p_cfg->fcs = BTA_HL_MCA_USE_FCS;
        }

        if(p_cfg->fcs != BTA_HL_MCA_USE_FCS) {
            if(peer_cfg_bits & L2CAP_CH_CFG_MASK_FCS) {
                p_cfg->fcs |= p_peer_cfg->fcs;
            } else {
                p_cfg->fcs = BTA_HL_MCA_USE_FCS;
            }
        }

        p_cfg->mtu = 0;

        if(peer_cfg_bits & L2CAP_CH_CFG_MASK_MTU) {
            p_cfg->mtu = p_peer_cfg->mtu;
        } else {
            p_cfg->mtu = L2CAP_DEFAULT_MTU;
        }

        success = TRUE;
    } else {
        p_cfg->mtu = L2CAP_DEFAULT_MTU;
        p_cfg->fcs = BTA_HL_L2C_NO_FCS;
    }

#if BTA_HL_DEBUG == TRUE

    if(!success) {
        APPL_TRACE_DEBUG("bta_hl_get_l2cap_cfg success=%d mdl=%d lcid=%d", success, mdl_hnd, lcid);
        APPL_TRACE_DEBUG("l2cap mtu=%d fcs=%d", p_cfg->mtu, p_cfg->fcs);
    }

#endif
    return success;
}

/*******************************************************************************
**
** Function      bta_hl_validate_chan_cfg
**
** Description    This function validates the L2CAP channel configuration
**
** Returns     uint8_t - TRUE - validation is successful
*******************************************************************************/
uint8_t bta_hl_validate_chan_cfg(uint8_t app_idx, uint8_t mcl_idx, uint8_t mdl_idx)
{
    tBTA_HL_APP_CB *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MDL_CB *p_dcb  = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
    uint8_t success = FALSE;
    uint8_t mdl_cfg_idx = 0;
    tBTA_HL_L2CAP_CFG_INFO l2cap_cfg;
    uint8_t get_l2cap_result, get_mdl_result;
    get_l2cap_result = bta_hl_get_l2cap_cfg(p_dcb->mdl_handle, &l2cap_cfg);
    get_mdl_result = bta_hl_find_mdl_cfg_idx(app_idx, mcl_idx, p_dcb->mdl_id, &mdl_cfg_idx);

    if(get_l2cap_result && get_mdl_result) {
        if((p_acb->mdl_cfg[mdl_cfg_idx].mtu <= l2cap_cfg.mtu) &&
                (p_acb->mdl_cfg[mdl_cfg_idx].fcs == l2cap_cfg.fcs) &&
                (p_acb->mdl_cfg[mdl_cfg_idx].dch_mode == p_dcb->dch_mode)) {
            success = TRUE;
        }
    }

#if BTA_HL_DEBUG == TRUE

    if(p_dcb->mtu != l2cap_cfg.mtu) {
        APPL_TRACE_WARNING("MCAP and L2CAP peer mtu size out of sync from MCAP mtu=%d from l2cap mtu=%d",
                           p_dcb->mtu, l2cap_cfg.mtu);
    }

    if(!success) {
        APPL_TRACE_DEBUG("bta_hl_validate_chan_cfg success=%d app_idx=%d mcl_idx=%d mdl_idx=%d", success,
                         app_idx, mcl_idx, mdl_idx);
        APPL_TRACE_DEBUG("Cur. L2cap cfg mtu=%d fcs=%d dch_mode=%d", l2cap_cfg.mtu, l2cap_cfg.fcs,
                         p_dcb->dch_mode);
        APPL_TRACE_DEBUG("From saved: L2cap cfg mtu=%d fcs=%d dch_mode=%d", p_acb->mdl_cfg[mdl_cfg_idx].mtu,
                         p_acb->mdl_cfg[mdl_cfg_idx].fcs, p_acb->mdl_cfg[mdl_cfg_idx].dch_mode);
    }

#endif
    return success;
}


/*******************************************************************************
**
** Function      bta_hl_is_cong_on
**
** Description    This function checks whether the congestion condition is on or not
**
** Returns      uint8_t - TRUE DCH is congested
**                        FALSE not congested
**
*******************************************************************************/
uint8_t  bta_hl_is_cong_on(uint8_t app_id, BD_ADDR bd_addr, tBTA_HL_MDL_ID mdl_id)

{
    tBTA_HL_MDL_CB      *p_dcb;
    uint8_t   app_idx = 0, mcl_idx, mdl_idx;
    uint8_t cong_status = TRUE;

    if(bta_hl_find_app_idx(app_id, &app_idx)) {
        if(bta_hl_find_mcl_idx(app_idx, bd_addr, &mcl_idx)) {
            if(bta_hl_find_mdl_idx(app_idx, mcl_idx, mdl_id, &mdl_idx)) {
                p_dcb = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
                cong_status  = p_dcb->cong;
            }
        }
    }

    return cong_status;
}

/*******************************************************************************
**
** Function      bta_hl_check_cch_close
**
** Description   This function checks whether there is a pending CCH close request
**               or not
**
** Returns      void
*******************************************************************************/
void bta_hl_check_cch_close(uint8_t app_idx, uint8_t mcl_idx, tBTA_HL_DATA *p_data,
                            uint8_t check_dch_setup)
{
    tBTA_HL_MCL_CB      *p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);
    tBTA_HL_MDL_CB      *p_dcb;
    uint8_t               mdl_idx;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_check_cch_close cch_close_dch_oper=%d", p_mcb->cch_close_dch_oper);
#endif

    if(p_mcb->cch_oper == BTA_HL_CCH_OP_LOCAL_CLOSE) {
        if(check_dch_setup && bta_hl_find_dch_setup_mdl_idx(app_idx, mcl_idx, &mdl_idx)) {
            p_dcb = BTA_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);

            if(!p_mcb->rsp_tout) {
                p_mcb->cch_close_dch_oper = BTA_HL_CCH_CLOSE_OP_DCH_ABORT;

                if(!p_dcb->abort_oper) {
                    p_dcb->abort_oper |= BTA_HL_ABORT_CCH_CLOSE_MASK;
                    bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_ABORT_EVT, p_data);
                }
            } else {
                p_mcb->cch_close_dch_oper = BTA_HL_CCH_CLOSE_OP_DCH_CLOSE;
                bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_CMPL_EVT, p_data);
            }
        } else if(bta_hl_find_an_active_mdl_idx(app_idx, mcl_idx, &mdl_idx)) {
            p_mcb->cch_close_dch_oper = BTA_HL_CCH_CLOSE_OP_DCH_CLOSE;
            bta_hl_dch_sm_execute(app_idx, mcl_idx, mdl_idx, BTA_HL_DCH_CLOSE_EVT, p_data);
        } else {
            p_mcb->cch_close_dch_oper = BTA_HL_CCH_CLOSE_OP_DCH_NONE;
            bta_hl_cch_sm_execute(app_idx, mcl_idx, BTA_HL_CCH_CLOSE_EVT, p_data);
        }
    }
}

/*******************************************************************************
**
** Function         bta_hl_clean_app
**
** Description      Cleans up the HDP application resources and control block
**
** Returns          void
**
*******************************************************************************/
void bta_hl_clean_app(uint8_t app_idx)
{
    tBTA_HL_APP_CB         *p_acb   = BTA_HL_GET_APP_CB_PTR(app_idx);
    int i, num_act_apps = 0;
#if BTA_HL_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hl_clean_app");
#endif
    MCA_Deregister((tMCA_HANDLE)p_acb->app_handle);

    if(p_acb->sdp_handle) {
        SDP_DeleteRecord(p_acb->sdp_handle);
    }

    wm_memset((void *) p_acb, 0, sizeof(tBTA_HL_APP_CB));

    /* check any application is still active */
    for(i = 0; i < BTA_HL_NUM_APPS ; i ++) {
        p_acb = BTA_HL_GET_APP_CB_PTR(i);

        if(p_acb->in_use) {
            num_act_apps++;
        }
    }

    if(!num_act_apps) {
        bta_sys_remove_uuid(UUID_SERVCLASS_HDP_PROFILE);
    }
}

/*******************************************************************************
**
** Function      bta_hl_check_deregistration
**
** Description   This function checks whether there is a pending deregistration
**               request or not
**
** Returns      void
*******************************************************************************/
void bta_hl_check_deregistration(uint8_t app_idx, tBTA_HL_DATA *p_data)
{
    tBTA_HL_APP_CB      *p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);
    tBTA_HL_MCL_CB      *p_mcb;
    uint8_t               mcl_idx;
    tBTA_HL             evt_data;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_check_deregistration");
#endif

    if(p_acb->deregistering) {
        if(bta_hl_find_an_in_use_mcl_idx(app_idx, &mcl_idx)) {
            p_mcb  = BTA_HL_GET_MCL_CB_PTR(app_idx, mcl_idx);

            if(p_mcb->cch_oper != BTA_HL_CCH_OP_LOCAL_CLOSE) {
                if(p_mcb->cch_state == BTA_HL_CCH_OPENING_ST) {
                    p_mcb->force_close_local_cch_opening = TRUE;
                }

                p_mcb->cch_oper = BTA_HL_CCH_OP_LOCAL_CLOSE;
                APPL_TRACE_DEBUG("p_mcb->force_close_local_cch_opening=%d", p_mcb->force_close_local_cch_opening);
                bta_hl_check_cch_close(app_idx, mcl_idx, p_data, TRUE);
            }
        } else {
            /* all cchs are closed */
            evt_data.dereg_cfm.app_handle = p_acb->app_handle;
            evt_data.dereg_cfm.app_id = p_data->api_dereg.app_id;
            evt_data.dereg_cfm.status = BTA_HL_STATUS_OK;
            p_acb->p_cback(BTA_HL_DEREGISTER_CFM_EVT, (tBTA_HL *) &evt_data);
            bta_hl_clean_app(app_idx);
            bta_hl_check_disable(p_data);
        }
    }
}


/*******************************************************************************
**
** Function      bta_hl_check_disable
**
** Description   This function checks whether there is a pending disable
**               request or not
**
** Returns      void
**
*******************************************************************************/
void bta_hl_check_disable(tBTA_HL_DATA *p_data)
{
    tBTA_HL_CB          *p_cb = &bta_hl_cb;
    tBTA_HL_APP_CB      *p_acb;
    uint8_t               app_idx;
    tBTA_HL_CTRL        evt_data;
#if (BTA_HL_DEBUG == TRUE)
    APPL_TRACE_DEBUG("bta_hl_check_disable");
#endif

    if(bta_hl_cb.disabling) {
        if(bta_hl_find_an_in_use_app_idx(&app_idx)) {
            p_acb  = BTA_HL_GET_APP_CB_PTR(app_idx);

            if(!p_acb->deregistering) {
                p_acb->deregistering = TRUE;
                bta_hl_check_deregistration(app_idx, p_data);
            }
        } else {
            /* all apps are deregistered */
            bta_sys_deregister(BTA_ID_HL);
            evt_data.disable_cfm.status = BTA_HL_STATUS_OK;

            if(p_cb->p_ctrl_cback) {
                p_cb->p_ctrl_cback(BTA_HL_CTRL_DISABLE_CFM_EVT, (tBTA_HL_CTRL *) &evt_data);
            }

            wm_memset((void *) p_cb, 0, sizeof(tBTA_HL_CB));
        }
    }
}

/*******************************************************************************
**
** Function      bta_hl_build_abort_cfm
**
** Description   This function builds the abort confirmation event data
**
** Returns      None
**
*******************************************************************************/
void  bta_hl_build_abort_cfm(tBTA_HL *p_evt_data,
                             tBTA_HL_APP_HANDLE app_handle,
                             tBTA_HL_MCL_HANDLE mcl_handle,
                             tBTA_HL_STATUS status)
{
    p_evt_data->dch_abort_cfm.status = status;
    p_evt_data->dch_abort_cfm.mcl_handle = mcl_handle;
    p_evt_data->dch_abort_cfm.app_handle = app_handle;
}

/*******************************************************************************
**
** Function      bta_hl_build_abort_ind
**
** Description   This function builds the abort indication event data
**
** Returns      None
**
*******************************************************************************/
void  bta_hl_build_abort_ind(tBTA_HL *p_evt_data,
                             tBTA_HL_APP_HANDLE app_handle,
                             tBTA_HL_MCL_HANDLE mcl_handle)
{
    p_evt_data->dch_abort_ind.mcl_handle = mcl_handle;
    p_evt_data->dch_abort_ind.app_handle = app_handle;
}
/*******************************************************************************
**
** Function      bta_hl_build_close_cfm
**
** Description   This function builds the close confirmation event data
**
** Returns      None
**
*******************************************************************************/
void  bta_hl_build_dch_close_cfm(tBTA_HL *p_evt_data,
                                 tBTA_HL_APP_HANDLE app_handle,
                                 tBTA_HL_MCL_HANDLE mcl_handle,
                                 tBTA_HL_MDL_HANDLE mdl_handle,
                                 tBTA_HL_STATUS status)
{
    p_evt_data->dch_close_cfm.status = status;
    p_evt_data->dch_close_cfm.mdl_handle = mdl_handle;
    p_evt_data->dch_close_cfm.mcl_handle = mcl_handle;
    p_evt_data->dch_close_cfm.app_handle = app_handle;
}

/*******************************************************************************
**
** Function      bta_hl_build_dch_close_ind
**
** Description   This function builds the close indication event data
**
** Returns      None
**
*******************************************************************************/
void  bta_hl_build_dch_close_ind(tBTA_HL *p_evt_data,
                                 tBTA_HL_APP_HANDLE app_handle,
                                 tBTA_HL_MCL_HANDLE mcl_handle,
                                 tBTA_HL_MDL_HANDLE mdl_handle,
                                 uint8_t intentional)
{
    p_evt_data->dch_close_ind.mdl_handle = mdl_handle;
    p_evt_data->dch_close_ind.mcl_handle = mcl_handle;
    p_evt_data->dch_close_ind.app_handle = app_handle;
    p_evt_data->dch_close_ind.intentional = intentional;
}

/*******************************************************************************
**
** Function      bta_hl_build_send_data_cfm
**
** Description   This function builds the send data confirmation event data
**
** Returns      None
**
*******************************************************************************/
void  bta_hl_build_send_data_cfm(tBTA_HL *p_evt_data,
                                 tBTA_HL_APP_HANDLE app_handle,
                                 tBTA_HL_MCL_HANDLE mcl_handle,
                                 tBTA_HL_MDL_HANDLE mdl_handle,
                                 tBTA_HL_STATUS status)
{
    p_evt_data->dch_send_data_cfm.mdl_handle = mdl_handle;
    p_evt_data->dch_send_data_cfm.mcl_handle = mcl_handle;
    p_evt_data->dch_send_data_cfm.app_handle = app_handle;
    p_evt_data->dch_send_data_cfm.status = status;
}

/*******************************************************************************
**
** Function      bta_hl_build_rcv_data_ind
**
** Description   This function builds the received data indication event data
**
** Returns      None
**
*******************************************************************************/
void  bta_hl_build_rcv_data_ind(tBTA_HL *p_evt_data,
                                tBTA_HL_APP_HANDLE app_handle,
                                tBTA_HL_MCL_HANDLE mcl_handle,
                                tBTA_HL_MDL_HANDLE mdl_handle)
{
    p_evt_data->dch_rcv_data_ind.mdl_handle = mdl_handle;
    p_evt_data->dch_rcv_data_ind.mcl_handle = mcl_handle;
    p_evt_data->dch_rcv_data_ind.app_handle = app_handle;
}


/*******************************************************************************
**
** Function      bta_hl_build_cch_open_cfm
**
** Description   This function builds the CCH open confirmation event data
**
** Returns      None
**
*******************************************************************************/
void  bta_hl_build_cch_open_cfm(tBTA_HL *p_evt_data,
                                uint8_t app_id,
                                tBTA_HL_APP_HANDLE app_handle,
                                tBTA_HL_MCL_HANDLE mcl_handle,
                                BD_ADDR bd_addr,
                                tBTA_HL_STATUS status)
{
    p_evt_data->cch_open_cfm.app_id = app_id;
    p_evt_data->cch_open_cfm.app_handle = app_handle;
    p_evt_data->cch_open_cfm.mcl_handle = mcl_handle;
    bdcpy(p_evt_data->cch_open_cfm.bd_addr, bd_addr);
    p_evt_data->cch_open_cfm.status = status;
    APPL_TRACE_DEBUG("bta_hl_build_cch_open_cfm: status=%d", status);
}

/*******************************************************************************
**
** Function      bta_hl_build_cch_open_ind
**
** Description   This function builds the CCH open indication event data
**
** Returns      None
**
*******************************************************************************/
void  bta_hl_build_cch_open_ind(tBTA_HL *p_evt_data, tBTA_HL_APP_HANDLE app_handle,
                                tBTA_HL_MCL_HANDLE mcl_handle,
                                BD_ADDR bd_addr)
{
    p_evt_data->cch_open_ind.app_handle = app_handle;
    p_evt_data->cch_open_ind.mcl_handle = mcl_handle;
    bdcpy(p_evt_data->cch_open_ind.bd_addr, bd_addr);
}

/*******************************************************************************
**
** Function      bta_hl_build_cch_close_cfm
**
** Description   This function builds the CCH close confirmation event data
**
** Returns      None
**
*******************************************************************************/
void  bta_hl_build_cch_close_cfm(tBTA_HL *p_evt_data,
                                 tBTA_HL_APP_HANDLE app_handle,
                                 tBTA_HL_MCL_HANDLE mcl_handle,
                                 tBTA_HL_STATUS status)
{
    p_evt_data->cch_close_cfm.mcl_handle = mcl_handle;
    p_evt_data->cch_close_cfm.app_handle = app_handle;
    p_evt_data->cch_close_cfm.status = status;
}


/*******************************************************************************
**
** Function      bta_hl_build_cch_close_ind
**
** Description   This function builds the CCH colse indication event data
**
** Returns      None
**
*******************************************************************************/
void  bta_hl_build_cch_close_ind(tBTA_HL *p_evt_data,
                                 tBTA_HL_APP_HANDLE app_handle,
                                 tBTA_HL_MCL_HANDLE mcl_handle,
                                 uint8_t intentional)
{
    p_evt_data->cch_close_ind.mcl_handle = mcl_handle;
    p_evt_data->cch_close_ind.app_handle = app_handle;
    p_evt_data->cch_close_ind.intentional = intentional;
}

/*******************************************************************************
**
** Function      bta_hl_build_dch_open_cfm
**
** Description   This function builds the DCH open confirmation event data
**
** Returns      None
**
*******************************************************************************/
void  bta_hl_build_dch_open_cfm(tBTA_HL *p_evt_data,
                                tBTA_HL_APP_HANDLE app_handle,
                                tBTA_HL_MCL_HANDLE mcl_handle,
                                tBTA_HL_MDL_HANDLE mdl_handle,
                                tBTA_HL_MDEP_ID local_mdep_id,
                                tBTA_HL_MDL_ID mdl_id,
                                tBTA_HL_DCH_MODE dch_mode,
                                uint8_t first_reliable,
                                uint16_t mtu,
                                tBTA_HL_STATUS status)

{
    p_evt_data->dch_open_cfm.mdl_handle = mdl_handle;
    p_evt_data->dch_open_cfm.mcl_handle = mcl_handle;
    p_evt_data->dch_open_cfm.app_handle = app_handle;
    p_evt_data->dch_open_cfm.local_mdep_id = local_mdep_id;
    p_evt_data->dch_open_cfm.mdl_id = mdl_id;
    p_evt_data->dch_open_cfm.dch_mode = dch_mode;
    p_evt_data->dch_open_cfm.first_reliable = first_reliable;
    p_evt_data->dch_open_cfm.mtu = mtu;
    p_evt_data->dch_open_cfm.status = status;
}


/*******************************************************************************
**
** Function      bta_hl_build_sdp_query_cfm
**
** Description   This function builds the SDP query indication event data
**
** Returns      None
**
*******************************************************************************/
void  bta_hl_build_sdp_query_cfm(tBTA_HL *p_evt_data,
                                 uint8_t app_id,
                                 tBTA_HL_APP_HANDLE app_handle,
                                 BD_ADDR bd_addr,
                                 tBTA_HL_SDP *p_sdp,
                                 tBTA_HL_STATUS status)

{
    APPL_TRACE_DEBUG("bta_hl_build_sdp_query_cfm: app_id = %d, app_handle=%d",
                     app_id, app_handle);
    p_evt_data->sdp_query_cfm.app_id = app_id;
    p_evt_data->sdp_query_cfm.app_handle = app_handle;
    bdcpy(p_evt_data->sdp_query_cfm.bd_addr, bd_addr);
    p_evt_data->sdp_query_cfm.p_sdp = p_sdp;
    p_evt_data->sdp_query_cfm.status = status;
}


/*******************************************************************************
**
** Function      bta_hl_build_delete_mdl_cfm
**
** Description   This function builds the delete MDL confirmation event data
**
** Returns      None
**
*******************************************************************************/
void  bta_hl_build_delete_mdl_cfm(tBTA_HL *p_evt_data,
                                  tBTA_HL_APP_HANDLE app_handle,
                                  tBTA_HL_MCL_HANDLE mcl_handle,
                                  tBTA_HL_MDL_ID mdl_id,
                                  tBTA_HL_STATUS status)

{
    p_evt_data->delete_mdl_cfm.mcl_handle = mcl_handle;
    p_evt_data->delete_mdl_cfm.app_handle = app_handle;
    p_evt_data->delete_mdl_cfm.mdl_id = mdl_id;
    p_evt_data->delete_mdl_cfm.status = status;
}

/*******************************************************************************
**
** Function      bta_hl_build_echo_test_cfm
**
** Description   This function builds the echo test confirmation event data
**
** Returns      None
**
*******************************************************************************/
void  bta_hl_build_echo_test_cfm(tBTA_HL *p_evt_data,
                                 tBTA_HL_APP_HANDLE app_handle,
                                 tBTA_HL_MCL_HANDLE mcl_handle,
                                 tBTA_HL_STATUS status)
{
    p_evt_data->echo_test_cfm.mcl_handle = mcl_handle;
    p_evt_data->echo_test_cfm.app_handle = app_handle;
    p_evt_data->echo_test_cfm.status = status;
}


/*****************************************************************************
**  Debug Functions
*****************************************************************************/
#if (BTA_HL_DEBUG == TRUE)

/*******************************************************************************
**
** Function         bta_hl_status_code
**
** Description      get the status string pointer
**
** Returns          char * - status string pointer
**
*******************************************************************************/
char *bta_hl_status_code(tBTA_HL_STATUS status)
{
    switch(status) {
        case BTA_HL_STATUS_OK:
            return "BTA_HL_STATUS_OK";

        case BTA_HL_STATUS_FAIL:
            return "BTA_HL_STATUS_FAIL";

        case BTA_HL_STATUS_ABORTED:
            return "BTA_HL_STATUS_ABORTED";

        case BTA_HL_STATUS_NO_RESOURCE:
            return "BTA_HL_STATUS_NO_RESOURCE";

        case BTA_HL_STATUS_LAST_ITEM:
            return "BTA_HL_STATUS_LAST_ITEM";

        case BTA_HL_STATUS_DUPLICATE_APP_ID:
            return "BTA_HL_STATUS_DUPLICATE_APP_ID";

        case BTA_HL_STATUS_INVALID_APP_HANDLE:
            return "BTA_HL_STATUS_INVALID_APP_HANDLE";

        case BTA_HL_STATUS_INVALID_MCL_HANDLE:
            return "BTA_HL_STATUS_INVALID_MCL_HANDLE";

        case BTA_HL_STATUS_MCAP_REG_FAIL:
            return "BTA_HL_STATUS_MCAP_REG_FAIL";

        case BTA_HL_STATUS_MDEP_CO_FAIL:
            return "BTA_HL_STATUS_MDEP_CO_FAIL";

        case BTA_HL_STATUS_ECHO_CO_FAIL:
            return "BTA_HL_STATUS_ECHO_CO_FAIL";

        case BTA_HL_STATUS_MDL_CFG_CO_FAIL:
            return "BTA_HL_STATUS_MDL_CFG_CO_FAIL";

        case BTA_HL_STATUS_SDP_NO_RESOURCE:
            return "BTA_HL_STATUS_SDP_NO_RESOURCE";

        case BTA_HL_STATUS_SDP_FAIL:
            return "BTA_HL_STATUS_SDP_FAIL";

        case BTA_HL_STATUS_NO_CCH:
            return "BTA_HL_STATUS_NO_CCH";

        case BTA_HL_STATUS_NO_MCL:
            return "BTA_HL_STATUS_NO_MCL";

        case BTA_HL_STATUS_NO_FIRST_RELIABLE:
            return "BTA_HL_STATUS_NO_FIRST_RELIABLE";

        case BTA_HL_STATUS_INVALID_DCH_CFG:
            return "BTA_HL_STATUS_INVALID_DCH_CFG";

        case BTA_HL_STATUS_INVALID_BD_ADDR:
            return "BTA_HL_STATUS_INVALID_BD_ADDR";

        case BTA_HL_STATUS_INVALID_RECONNECT_CFG:
            return "BTA_HL_STATUS_INVALID_RECONNECT_CFG";

        case BTA_HL_STATUS_ECHO_TEST_BUSY:
            return "BTA_HL_STATUS_ECHO_TEST_BUSY";

        case BTA_HL_STATUS_INVALID_LOCAL_MDEP_ID:
            return "BTA_HL_STATUS_INVALID_LOCAL_MDEP_ID";

        case BTA_HL_STATUS_INVALID_MDL_ID:
            return "BTA_HL_STATUS_INVALID_MDL_ID";

        case BTA_HL_STATUS_NO_MDL_ID_FOUND:
            return "BTA_HL_STATUS_NO_MDL_ID_FOUND";

        case BTA_HL_STATUS_DCH_BUSY:
            return "BTA_HL_STATUS_DCH_BUSY";

        default:
            return "Unknown status code";
    }
}
/*******************************************************************************
**
** Function         bta_hl_evt_code
**
** Description      Maps HL event code to the corresponding event string
**
** Returns          string pointer for the associated event name
**
*******************************************************************************/
char *bta_hl_evt_code(tBTA_HL_INT_EVT evt_code)
{
    switch(evt_code) {
        case BTA_HL_CCH_OPEN_EVT:
            return "BTA_HL_CCH_OPEN_EVT";

        case BTA_HL_CCH_SDP_OK_EVT:
            return "BTA_HL_CCH_SDP_OK_EVT";

        case BTA_HL_CCH_SDP_FAIL_EVT:
            return "BTA_HL_CCH_SDP_FAIL_EVT";

        case BTA_HL_MCA_CONNECT_IND_EVT:
            return "BTA_HL_MCA_CONNECT_IND_EVT";

        case BTA_HL_MCA_DISCONNECT_IND_EVT:
            return "BTA_HL_MCA_DISCONNECT_IND_EVT";

        case BTA_HL_CCH_CLOSE_EVT:
            return "BTA_HL_CCH_CLOSE_EVT";

        case BTA_HL_CCH_CLOSE_CMPL_EVT:
            return "BTA_HL_CCH_CLOSE_CMPL_EVT";

        case BTA_HL_DCH_OPEN_EVT:
            return "BTA_HL_DCH_OPEN_EVT";

        case BTA_HL_MCA_CREATE_IND_EVT:
            return "BTA_HL_MCA_CREATE_IND_EVT";

        case BTA_HL_MCA_CREATE_CFM_EVT:
            return "BTA_HL_MCA_CREATE_CFM_EVT";

        case BTA_HL_MCA_OPEN_IND_EVT:
            return "BTA_HL_MCA_OPEN_IND_EVT";

        case BTA_HL_MCA_OPEN_CFM_EVT:
            return "BTA_HL_MCA_OPEN_CFM_EVT";

        case BTA_HL_DCH_CLOSE_EVT:
            return "BTA_HL_DCH_CLOSE_EVT";

        case BTA_HL_MCA_CLOSE_IND_EVT:
            return "BTA_HL_MCA_CLOSE_IND_EVT";

        case BTA_HL_MCA_CLOSE_CFM_EVT:
            return "BTA_HL_MCA_CLOSE_CFM_EVT";

        case BTA_HL_API_SEND_DATA_EVT:
            return "BTA_HL_API_SEND_DATA_EVT";

        case BTA_HL_MCA_RCV_DATA_EVT:
            return "BTA_HL_MCA_RCV_DATA_EVT";

        case BTA_HL_DCH_CLOSE_CMPL_EVT:
            return "BTA_HL_DCH_CLOSE_CMPL_EVT";

        case BTA_HL_API_ENABLE_EVT:
            return "BTA_HL_API_ENABLE_EVT";

        case BTA_HL_API_DISABLE_EVT:
            return "BTA_HL_API_DISABLE_EVT";

        case BTA_HL_API_UPDATE_EVT:
            return "BTA_HL_API_UPDATE_EVT";

        case BTA_HL_API_REGISTER_EVT:
            return "BTA_HL_API_REGISTER_EVT";

        case BTA_HL_API_DEREGISTER_EVT:
            return "BTA_HL_API_DEREGISTER_EVT";

        case BTA_HL_API_CCH_OPEN_EVT:
            return "BTA_HL_API_CCH_OPEN_EVT";

        case BTA_HL_API_CCH_CLOSE_EVT:
            return "BTA_HL_API_CCH_CLOSE_EVT";

        case BTA_HL_API_DCH_OPEN_EVT:
            return "BTA_HL_API_DCH_OPEN_EVT";

        case BTA_HL_API_DCH_RECONNECT_EVT:
            return "BTA_HL_API_DCH_RECONNECT_EVT";

        case BTA_HL_API_DCH_CLOSE_EVT:
            return "BTA_HL_API_DCH_CLOSE_EVT";

        case BTA_HL_API_DELETE_MDL_EVT:
            return "BTA_HL_API_DELETE_MDL_EVT";

        case BTA_HL_API_DCH_ABORT_EVT:
            return "BTA_HL_API_DCH_ABORT_EVT";

        case BTA_HL_DCH_RECONNECT_EVT:
            return "BTA_HL_DCH_RECONNECT_EVT";

        case BTA_HL_DCH_SDP_INIT_EVT:
            return "BTA_HL_DCH_SDP_INIT_EVT";

        case BTA_HL_DCH_SDP_FAIL_EVT:
            return "BTA_HL_DCH_SDP_FAIL_EVT";

        case BTA_HL_API_DCH_ECHO_TEST_EVT:
            return "BTA_HL_API_DCH_ECHO_TEST_EVT";

        case BTA_HL_DCH_CLOSE_ECHO_TEST_EVT:
            return "BTA_HL_DCH_CLOSE_ECHO_TEST_EVT";

        case BTA_HL_MCA_RECONNECT_IND_EVT:
            return "BTA_HL_MCA_RECONNECT_IND_EVT";

        case BTA_HL_MCA_RECONNECT_CFM_EVT:
            return "BTA_HL_MCA_RECONNECT_CFM_EVT";

        case BTA_HL_API_DCH_CREATE_RSP_EVT:
            return "BTA_HL_API_DCH_CREATE_RSP_EVT";

        case BTA_HL_DCH_ABORT_EVT:
            return "BTA_HL_DCH_ABORT_EVT";

        case BTA_HL_MCA_ABORT_IND_EVT:
            return "BTA_HL_MCA_ABORT_IND_EVT";

        case BTA_HL_MCA_ABORT_CFM_EVT:
            return "BTA_HL_MCA_ABORT_CFM_EVT";

        case BTA_HL_MCA_DELETE_IND_EVT:
            return "BTA_HL_MCA_DELETE_IND_EVT";

        case BTA_HL_MCA_DELETE_CFM_EVT:
            return "BTA_HL_MCA_DELETE_CFM_EVT";

        case BTA_HL_MCA_CONG_CHG_EVT:
            return "BTA_HL_MCA_CONG_CHG_EVT";

        case BTA_HL_CI_GET_TX_DATA_EVT:
            return "BTA_HL_CI_GET_TX_DATA_EVT";

        case BTA_HL_CI_PUT_RX_DATA_EVT:
            return "BTA_HL_CI_PUT_RX_DATA_EVT";

        case BTA_HL_CI_GET_ECHO_DATA_EVT:
            return "BTA_HL_CI_GET_ECHO_DATA_EVT";

        case BTA_HL_DCH_ECHO_TEST_EVT:
            return "BTA_HL_DCH_ECHO_TEST_EVT";

        case BTA_HL_CI_PUT_ECHO_DATA_EVT:
            return "BTA_HL_CI_PUT_ECHO_DATA_EVT";

        case BTA_HL_API_SDP_QUERY_EVT:
            return "BTA_HL_API_SDP_QUERY_EVT";

        case BTA_HL_SDP_QUERY_OK_EVT:
            return "BTA_HL_SDP_QUERY_OK_EVT";

        case BTA_HL_SDP_QUERY_FAIL_EVT:
            return "BTA_HL_SDP_QUERY_FAIL_EVT";

        case BTA_HL_MCA_RSP_TOUT_IND_EVT:
            return "BTA_HL_MCA_RSP_TOUT_IND_EVT";

        default:
            return "Unknown HL event code";
    }
}

#endif  /* Debug Functions */
#endif // HL_INCLUDED








