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

#define LOG_TAG "bt_btif_bta_ag"
#include "bt_target.h"
#if defined(BTA_HFP_HSP_AG_INCLUDED) && (BTA_HFP_HSP_AG_INCLUDED == TRUE)
#include "gki.h"

#include "bta_ag_api.h"
#include "hci/include/hci_audio.h"
#include "osi/include/osi.h"

/*******************************************************************************
**
** Function         bta_ag_co_init
**
** Description      This callout function is executed by AG when it is
**                  started by calling BTA_AgEnable().  This function can be
**                  used by the phone to initialize audio paths or for other
**                  initialization purposes.
**
**
** Returns          Void.
**
*******************************************************************************/
void bta_ag_co_init(void)
{
    BTM_WriteVoiceSettings(AG_VOICE_SETTINGS);
}


/*******************************************************************************
**
** Function         bta_ag_co_audio_state
**
** Description      This function is called by the AG before the audio connection
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
#if (BTM_WBS_INCLUDED == TRUE )
void bta_ag_co_audio_state(uint16_t handle, uint8_t app_id, uint8_t state, tBTA_AG_PEER_CODEC codec)
#else
void bta_ag_co_audio_state(uint16_t handle, uint8_t app_id, uint8_t state)
#endif
{
    BTIF_TRACE_DEBUG("bta_ag_co_audio_state: handle %d, state %d", handle, state);

    switch(state) {
        case SCO_STATE_OFF:
#if (BTM_WBS_INCLUDED == TRUE )
            BTIF_TRACE_DEBUG("bta_ag_co_audio_state(handle %d)::Closed (OFF), codec: 0x%x",
                             handle, codec);
            set_audio_state(handle, codec, state);
#else
            BTIF_TRACE_DEBUG("bta_ag_co_audio_state(handle %d)::Closed (OFF)",
                             handle);
#endif
            break;

        case SCO_STATE_OFF_TRANSFER:
            BTIF_TRACE_DEBUG("bta_ag_co_audio_state(handle %d)::Closed (XFERRING)", handle);
            break;

        case SCO_STATE_SETUP:
#if (BTM_WBS_INCLUDED == TRUE )
            set_audio_state(handle, codec, state);
#else
            set_audio_state(handle, BTA_AG_CODEC_CVSD, state);
#endif
            break;

        default:
            break;
    }

#if (BTM_WBS_INCLUDED == TRUE )
    APPL_TRACE_DEBUG("bta_ag_co_audio_state(handle %d, app_id: %d, state %d, codec: 0x%x)",
                     handle, app_id, state, codec);
#else
    APPL_TRACE_DEBUG("bta_ag_co_audio_state(handle %d, app_id: %d, state %d)", \
                     handle, app_id, state);
#endif
}


/*******************************************************************************
**
** Function         bta_ag_co_data_open
**
** Description      This function is executed by AG when a service level connection
**                  is opened.  The phone can use this function to set
**                  up data paths or perform any required initialization or
**                  set up particular to the connected service.
**
**
** Returns          void
**
*******************************************************************************/
void bta_ag_co_data_open(uint16_t handle, tBTA_SERVICE_ID service)
{
    BTIF_TRACE_DEBUG("bta_ag_co_data_open handle:%d service:%d", handle, service);
}

/*******************************************************************************
**
** Function         bta_ag_co_data_close
**
** Description      This function is called by AG when a service level
**                  connection is closed
**
**
** Returns          void
**
*******************************************************************************/
void bta_ag_co_data_close(uint16_t handle)
{
    BTIF_TRACE_DEBUG("bta_ag_co_data_close handle:%d", handle);
}


/*******************************************************************************
 **
 ** Function         bta_ag_co_tx_write
 **
 ** Description      This function is called by the AG to send data to the
 **                  phone when the AG is configured for AT command pass-through.
 **                  The implementation of this function must copy the data to
 **                  the phones memory.
 **
 ** Returns          void
 **
 *******************************************************************************/
void bta_ag_co_tx_write(uint16_t handle, UNUSED_ATTR uint8_t *p_data, uint16_t len)
{
    BTIF_TRACE_DEBUG("bta_ag_co_tx_write: handle: %d, len: %d", handle, len);
}
#endif
