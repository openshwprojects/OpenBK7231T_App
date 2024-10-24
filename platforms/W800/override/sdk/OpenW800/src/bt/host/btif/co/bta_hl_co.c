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
 *  This is the implementation file for the HeaLth device profile (HL)
 *  subsystem call-out functions.
 *
 ******************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bt_target.h"
#if defined(BTA_HH_INCLUDED) && (BTA_HH_INCLUDED == TRUE)
#include "osi/include/osi.h"
#include "bta_api.h"
#include "bta_hl_api.h"
#include "bta_hl_ci.h"
#include "bta_hl_co.h"
#include "bta_sys.h"
#include "btif_hl.h"
#include "btif_util.h"
#include "btm_api.h"

/*****************************************************************************
**  Constants and Data Types
*****************************************************************************/
/**************************
**  Common Definitions
***************************/




/*******************************************************************************
**
** Function        bta_hl_co_get_num_of_mdep
**
** Description     This function is called to get the number of MDEPs for this
**                 application ID
**
** Parameters      app_id - application ID
**                 p_num_of_mdep (output) - number of MDEP configurations supported
**                                          by the application
**
** Returns         Bloolean - TRUE success
**
*******************************************************************************/
uint8_t bta_hl_co_get_num_of_mdep(uint8_t app_id, uint8_t *p_num_of_mdep)
{
    uint8_t app_idx;
    uint8_t success = FALSE;

    if(btif_hl_find_app_idx(app_id, &app_idx)) {
        *p_num_of_mdep = p_btif_hl_cb->acb[app_idx].sup_feature.num_of_mdeps;
        success = TRUE;
    }

    BTIF_TRACE_DEBUG("%s success=%d num_mdeps=%d",
                     __FUNCTION__, success, *p_num_of_mdep);
    return success;
}

/*******************************************************************************
**
** Function        bta_hl_co_advrtise_source_sdp
**
** Description     This function is called to find out whether the SOURCE MDEP
**                 configuration information should be advertize in the SDP or nopt
**
** Parameters      app_id - application ID
**
** Returns         Bloolean - TRUE advertise the SOURCE MDEP configuration
**                            information
**
*******************************************************************************/
uint8_t bta_hl_co_advrtise_source_sdp(uint8_t app_id)
{
    uint8_t     advertize_source_sdp = FALSE;
    uint8_t       app_idx;

    if(btif_hl_find_app_idx(app_id, &app_idx)) {
        advertize_source_sdp = p_btif_hl_cb->acb[app_idx].sup_feature.advertize_source_sdp;
    }

    BTIF_TRACE_DEBUG("%s advertize_flag=%d", __FUNCTION__, advertize_source_sdp);
    return advertize_source_sdp;
}
/*******************************************************************************
**
** Function        bta_hl_co_get_mdep_config
**
** Description     This function is called to get the supported feature
**                 configuration for the specified mdep index and it also assigns
**                 the MDEP ID for the specified mdep index
**
** Parameters      app_id - HDP application ID
**                 mdep_idx - the mdep index
**                  mdep_counter - number of mdeps
**                 mdep_id  - the assigned MDEP ID for the specified medp_idx
**                 p_mdl_cfg (output) - pointer to the MDEP configuration
**
**
** Returns         Bloolean - TRUE success
*******************************************************************************/
uint8_t bta_hl_co_get_mdep_config(uint8_t  app_id,
                                  uint8_t mdep_idx,
                                  uint8_t mdep_counter,
                                  tBTA_HL_MDEP_ID mdep_id,
                                  tBTA_HL_MDEP_CFG *p_mdep_cfg)
{
    uint8_t       idx  ;
    uint8_t       app_idx;
    uint8_t     success = FALSE;
    BTIF_TRACE_DEBUG("%s app_id=%d mdep_idx=%d mdep_id=%d mdep_counter=%d",
                     __FUNCTION__, app_id, mdep_idx, mdep_id, mdep_counter);

    if(btif_hl_find_app_idx(app_id, &app_idx)) {
        idx = mdep_idx - mdep_counter - 1;
        p_btif_hl_cb->acb[app_idx].sup_feature.mdep[idx].mdep_id = mdep_id;
        wm_memcpy(p_mdep_cfg,
                  &p_btif_hl_cb->acb[app_idx].sup_feature.mdep[idx].mdep_cfg,
                  sizeof(tBTA_HL_MDEP_CFG));
        success = TRUE;
    }

    BTIF_TRACE_DEBUG("%s success=%d mdep_idx=%d mdep_id=%d",
                     __FUNCTION__, success, mdep_idx, mdep_id);
    return success;
}


/*******************************************************************************
**
** Function        bta_hl_co_get_echo_config
**
** Description     This function is called to get the echo test
**                 maximum APDU size configurations
**
** Parameters      app_id - HDP application ID
**                 p_echo_cfg (output) - pointer to the Echo test maximum APDU size
**                                       configuration
**
** Returns         Bloolean - TRUE success
*******************************************************************************/
uint8_t bta_hl_co_get_echo_config(uint8_t  app_id,
                                  tBTA_HL_ECHO_CFG *p_echo_cfg)
{
    uint8_t               app_idx;
    uint8_t             success = FALSE;
    btif_hl_app_cb_t    *p_acb;
    tBTA_HL_SUP_FEATURE *p_sup;
    BTIF_TRACE_DEBUG("%s app_id=%d", __FUNCTION__, app_id);

    if(btif_hl_find_app_idx(app_id, &app_idx)) {
        p_acb = BTIF_HL_GET_APP_CB_PTR(app_idx);
        p_sup = &p_acb->sup_feature;
        p_echo_cfg->max_rx_apdu_size = p_sup->echo_cfg.max_rx_apdu_size;
        p_echo_cfg->max_tx_apdu_size = p_sup->echo_cfg.max_tx_apdu_size;
        success = TRUE;
    }

    BTIF_TRACE_DEBUG("%s success=%d max tx_size=%d rx_size=%d",
                     __FUNCTION__, success, p_echo_cfg->max_tx_apdu_size,
                     p_echo_cfg->max_rx_apdu_size);
    return success;
}


/*******************************************************************************
**
** Function        bta_hl_co_save_mdl
**
** Description     This function is called to save a MDL configuration item in persistent
**                 storage
**
** Parameters      app_id - HDP application ID
**                 item_idx - the MDL configuration storage index
**                 p_mdl_cfg - pointer to the MDL configuration data
**
** Returns        void
**
*******************************************************************************/
void bta_hl_co_save_mdl(uint8_t mdep_id, uint8_t item_idx, tBTA_HL_MDL_CFG *p_mdl_cfg)
{
    BTIF_TRACE_DEBUG("%s mdep_id =%d, item_idx=%d active=%d mdl_id=%d time=%d",
                     __FUNCTION__, mdep_id, item_idx,
                     p_mdl_cfg->active,
                     p_mdl_cfg->mdl_id,
                     p_mdl_cfg->time);
    btif_hl_save_mdl_cfg(mdep_id, item_idx, p_mdl_cfg);
}

/*******************************************************************************
**
** Function        bta_hl_co_delete_mdl
**
** Description     This function is called to delete a MDL configuration item in persistent
**                 storage
**
** Parameters      app_id - HDP application ID
**                 item_idx - the MDL configuration storage index
**
** Returns          void
**
*******************************************************************************/
void bta_hl_co_delete_mdl(uint8_t mdep_id, uint8_t item_idx)
{
    BTIF_TRACE_DEBUG("%s mdep_id=%d, item_idx=%d", __FUNCTION__, mdep_id, item_idx);
    btif_hl_delete_mdl_cfg(mdep_id, item_idx);
}

/*******************************************************************************
**
** Function         bta_hl_co_get_mdl_config
**
** Description     This function is called to get the MDL configuration
**                 from the persistent memory. This function shall only be called
*8                 once after the device is powered up
**
** Parameters      app_id - HDP application ID
**                 buffer_size - the unit of the buffer size is sizeof(tBTA_HL_MDL_CFG)
**                 p_mdl_buf - Point to the starting location of the buffer
**
** Returns         uint8_t
**
**
*******************************************************************************/
uint8_t bta_hl_co_load_mdl_config(uint8_t app_id, uint8_t buffer_size,
                                  tBTA_HL_MDL_CFG *p_mdl_buf)
{
    uint8_t result = TRUE;
    uint8_t i;
    tBTA_HL_MDL_CFG *p;
    BTIF_TRACE_DEBUG("%s app_id=%d, num_items=%d",
                     __FUNCTION__, app_id, buffer_size);

    if(buffer_size > BTA_HL_NUM_MDL_CFGS) {
        result = FALSE;
        return result;
    }

    result = btif_hl_load_mdl_config(app_id, buffer_size, p_mdl_buf);

    if(result) {
        for(i = 0, p = p_mdl_buf; i < buffer_size; i++, p++) {
            if(p->active) {
                BTIF_TRACE_DEBUG("i=%d mdl_id=0x%x dch_mode=%d local mdep_role=%d mdep_id=%d mtu=%d",
                                 i, p->mdl_id, p->dch_mode, p->local_mdep_role, p->local_mdep_role, p->mtu);
            }
        }
    }

    BTIF_TRACE_DEBUG("%s success=%d num_items=%d", __FUNCTION__, result, buffer_size);
    return result;
}

/*******************************************************************************
**
** Function         bta_hl_co_get_tx_data
**
** Description     Get the data to be sent
**
** Parameters      app_id - HDP application ID
**                 mdl_handle - MDL handle
**                 buf_size - the size of the buffer
**                 p_buf - the buffer pointer
**                 evt - the evt to be passed back to the HL in the
**                       bta_hl_ci_get_tx_data call-in function
**
** Returns        Void
**
*******************************************************************************/
void bta_hl_co_get_tx_data(uint8_t app_id, tBTA_HL_MDL_HANDLE mdl_handle,
                           uint16_t buf_size, uint8_t *p_buf,  uint16_t evt)
{
    uint8_t app_idx, mcl_idx, mdl_idx;
    btif_hl_mdl_cb_t *p_dcb;
    tBTA_HL_STATUS status = BTA_HL_STATUS_FAIL;
    BTIF_TRACE_DEBUG("%s app_id=%d mdl_handle=0x%x buf_size=%d",
                     __FUNCTION__, app_id, mdl_handle, buf_size);

    if(btif_hl_find_mdl_idx_using_handle(mdl_handle, &app_idx, &mcl_idx, &mdl_idx)) {
        p_dcb = BTIF_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);

        if(p_dcb->tx_size <= buf_size) {
            wm_memcpy(p_buf, p_dcb->p_tx_pkt, p_dcb->tx_size);
            GKI_free_and_reset_buf((void **)&p_dcb->p_tx_pkt);
            p_dcb->tx_size = 0;
            status = BTA_HL_STATUS_OK;
        }
    }

    bta_hl_ci_get_tx_data(mdl_handle,  status, evt);
}


/*******************************************************************************
**
** Function        bta_hl_co_put_rx_data
**
** Description     Put the received data
**
** Parameters      app_id - HDP application ID
**                 mdl_handle - MDL handle
**                 data_size - the size of the data
**                 p_data - the data pointer
**                 evt - the evt to be passed back to the HL in the
**                       bta_hl_ci_put_rx_data call-in function
**
** Returns        Void
**
*******************************************************************************/
void bta_hl_co_put_rx_data(uint8_t app_id, tBTA_HL_MDL_HANDLE mdl_handle,
                           uint16_t data_size, uint8_t *p_data, uint16_t evt)
{
    uint8_t app_idx, mcl_idx, mdl_idx;
    btif_hl_mdl_cb_t *p_dcb;
    tBTA_HL_STATUS status = BTA_HL_STATUS_FAIL;
    BTIF_TRACE_DEBUG("%s app_id=%d mdl_handle=0x%x data_size=%d",
                     __FUNCTION__, app_id, mdl_handle, data_size);

    if(btif_hl_find_mdl_idx_using_handle(mdl_handle, &app_idx, &mcl_idx, &mdl_idx)) {
        p_dcb = BTIF_HL_GET_MDL_CB_PTR(app_idx, mcl_idx, mdl_idx);
        p_dcb->p_rx_pkt = (uint8_t *)GKI_getbuf(data_size);
        wm_memcpy(p_dcb->p_rx_pkt, p_data, data_size);

        if(p_dcb->p_scb) {
            BTIF_TRACE_DEBUG("app_idx=%d mcl_idx=0x%x mdl_idx=0x%x data_size=%d",
                             app_idx, mcl_idx, mdl_idx, data_size);
            ssize_t r;
            OSI_NO_INTR(r = send(p_dcb->p_scb->socket_id[1], p_dcb->p_rx_pkt,
                                 data_size, 0));

            if(r == data_size) {
                BTIF_TRACE_DEBUG("socket send success data_size=%d", data_size);
                status = BTA_HL_STATUS_OK;
            } else {
                BTIF_TRACE_ERROR("socket send failed r=%d data_size=%d", r,
                                 data_size);
            }
        }

        GKI_free_and_reset_buf((void **)&p_dcb->p_rx_pkt);
    }

    bta_hl_ci_put_rx_data(mdl_handle,  status, evt);
}


/*******************************************************************************
**
** Function         bta_hl_co_get_tx_data
**
** Description     Get the Echo data to be sent
**
** Parameters      app_id - HDP application ID
**                 mcl_handle - MCL handle
**                 buf_size - the size of the buffer
**                 p_buf - the buffer pointer
**                 evt - the evt to be passed back to the HL in the
**                       bta_hl_ci_get_tx_data call-in function
**
** Returns        Void
**
*******************************************************************************/
void bta_hl_co_get_echo_data(uint8_t app_id, tBTA_HL_MCL_HANDLE mcl_handle,
                             uint16_t buf_size, uint8_t *p_buf,  uint16_t evt)
{
    tBTA_HL_STATUS status = BTA_HL_STATUS_FAIL;
    UNUSED(app_id);
    UNUSED(buf_size);
    UNUSED(p_buf);
    BTIF_TRACE_ERROR("%s not supported", __FUNCTION__);
    bta_hl_ci_get_echo_data(mcl_handle,  status, evt);
}


/*******************************************************************************
**
** Function        bta_hl_co_put_echo_data
**
** Description     Put the received loopback echo data
**
** Parameters      app_id - HDP application ID
**                 mcl_handle - MCL handle
**                 data_size - the size of the data
**                 p_data - the data pointer
**                 evt - the evt to be passed back to the HL in the
**                       bta_hl_ci_put_echo_data call-in function
**
** Returns        Void
**
*******************************************************************************/
void bta_hl_co_put_echo_data(uint8_t app_id, tBTA_HL_MCL_HANDLE mcl_handle,
                             uint16_t data_size, uint8_t *p_data, uint16_t evt)
{
    tBTA_HL_STATUS status = BTA_HL_STATUS_FAIL;
    UNUSED(app_id);
    UNUSED(data_size);
    UNUSED(p_data);
    BTIF_TRACE_ERROR("%s not supported", __FUNCTION__);
    bta_hl_ci_put_echo_data(mcl_handle,  status, evt);
}
#endif
