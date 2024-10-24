/******************************************************************************
 *
 *  Copyright (c) 2014 The Android Open Source Project
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


#include <string.h>
#include <stdlib.h>
#include "bt_target.h"

#if defined(BTA_HFP_HSP_INCLUDED) && (BTA_HFP_HSP_INCLUDED == TRUE)

#if (BTM_SCO_HCI_INCLUDED == TRUE)

#include "bta_hf_client_co.h"
#include "hci_audio.h"
#include "bt_utils.h"

/*******************************************************************************
**
** Function         bta_hf_client_co_audio_state
**
** Description      This function is called by the HF CLIENT before the audio connection
**                  is brought up, after it comes up, and after it goes down.
**
** Parameters       handle - handle of the AG instance
**                  state - Audio state
**                  codec - if WBS support is compiled in, codec to going to be used is provided
**                      and when in SCO_STATE_SETUP, BTM_I2SPCMConfig() must be called with
**                      the correct platform parameters.
**                      in the other states codec type should not be ignored
**
** Returns          void
**
*******************************************************************************/
void bta_hf_client_co_audio_state(uint16_t handle, uint8_t state, tBTA_HFP_PEER_CODEC codec)
{
    switch(state) {
        case SCO_STATE_ON:
        case SCO_STATE_OFF:
        case SCO_STATE_OFF_TRANSFER:
        case SCO_STATE_SETUP:
        default:
            break;
    }
}

/*******************************************************************************
**
** Function         bta_hf_client_sco_co_init
**
** Description      This function can be used by the phone to initialize audio
**                  codec or for other initialization purposes before SCO connection
**                  is opened.
**
**
** Returns          Void.
**
*******************************************************************************/
tBTA_HFP_SCO_ROUTE_TYPE bta_hf_client_sco_co_init(uint32_t rx_bw, uint32_t tx_bw,
        tBTA_HFP_CODEC_INFO *p_codec_info, uint8_t app_id)
{
    APPL_TRACE_EVENT("%s rx_bw %d, tx_bw %d, codec %d", __FUNCTION__, rx_bw, tx_bw,
                     p_codec_info->codec_type);
    return BTA_HFP_SCO_ROUTE_HCI;
}

/*******************************************************************************
**
** Function         bta_hf_client_sco_co_open
**
** Description      This function is executed when a SCO connection is open.
**
**
** Returns          void
**
*******************************************************************************/
void bta_hf_client_sco_co_open(uint16_t handle, uint8_t pkt_size, uint16_t event)
{
    APPL_TRACE_EVENT("%s hdl %x, pkt_sz %u, event %u", __FUNCTION__, handle,
                     pkt_size, event);
}

/*******************************************************************************
**
** Function         bta_hf_client_sco_co_close
**
** Description      This function is called when a SCO connection is closed
**
**
** Returns          void
**
*******************************************************************************/
void bta_hf_client_sco_co_close(void)
{
    APPL_TRACE_EVENT("%s", __FUNCTION__);
}

/*SCO data to application*/
__attribute__((weak)) int btif_co_sco_data_incoming(uint8_t type, uint8_t *p_data, uint16_t length)
{
    UNUSED(type);
    UNUSED(p_data);
    UNUSED(length);
    return 0;
}

/*SCO data sent over HCI*/
__attribute__((weak)) int btif_co_sco_data_outgoing(uint8_t type, uint8_t *p_data, uint16_t length)
{
    UNUSED(type);
    UNUSED(p_data);
    UNUSED(length);
    return 0;
}

/*******************************************************************************
**
** Function         bta_hf_client_sco_co_out_data
**
** Description      This function is called to send SCO data over HCI.
**
** Returns          number of bytes got from application
**
*******************************************************************************/
uint32_t bta_hf_client_sco_co_out_data(uint8_t *p_buf, uint32_t sz)
{
    return btif_co_sco_data_outgoing(0, p_buf, sz);
}

/*******************************************************************************
**
** Function         bta_hf_client_sco_co_in_data
**
** Description      This function is called to send incoming SCO data to application.
**
** Returns          void
**
*******************************************************************************/
void bta_hf_client_sco_co_in_data(BT_HDR  *p_buf, tBTM_SCO_DATA_FLAG status)
{
    uint8_t       *p = (uint8_t *)(p_buf + 1) + p_buf->offset;
    uint8_t       pkt_size = 0;
    STREAM_SKIP_UINT16(p);
    STREAM_TO_UINT8(pkt_size, p);
    btif_co_sco_data_incoming(0, p, pkt_size);
    //BTA_HfClientCiData();
}

#endif /* #if (BTM_SCO_HCI_INCLUDED == TRUE) */

#endif /* #if (BTA_HF_INCLUDED == TRUE) */
