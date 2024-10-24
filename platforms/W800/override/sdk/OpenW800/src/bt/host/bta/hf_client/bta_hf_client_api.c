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

/******************************************************************************
 *
 *  This is the implementation of the API for the handsfree (HF role)
 *  subsystem of BTA
 *
 ******************************************************************************/

#include <string.h>
#include "bt_target.h"
#if defined(BTA_HFP_HSP_INCLUDED) && (BTA_HFP_HSP_INCLUDED == TRUE)
#include "bta_hf_client_api.h"
#include "bta_hf_client_int.h"
#include "osi/include/compat.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/
static const tBTA_SYS_REG bta_hf_client_reg = {
    bta_hf_client_hdl_event,
    BTA_HfClientDisable
};


/*****************************************************************************
**  External Function Declarations
*****************************************************************************/

/*******************************************************************************
**
** Function         BTA_HfClientEnable
**
** Description      Enable the HF CLient service. When the enable
**                  operation is complete the callback function will be
**                  called with a BTA_HF_CLIENT_ENABLE_EVT. This function must
**                  be called before other function in the HF CLient API are
**                  called.
**
** Returns          BTA_SUCCESS if OK, BTA_FAILURE otherwise.
**
*******************************************************************************/
tBTA_STATUS BTA_HfClientEnable(tBTA_HF_CLIENT_CBACK *p_cback)
{
    if(bta_sys_is_register(BTA_ID_HS)) {
        APPL_TRACE_ERROR("BTA HF Client is already enabled, ignoring ...");
        return BTA_FAILURE;
    }

    /* register with BTA system manager */
    bta_sys_register(BTA_ID_HS, &bta_hf_client_reg);
    tBTA_HF_CLIENT_API_ENABLE *p_buf =
                    (tBTA_HF_CLIENT_API_ENABLE *)GKI_getbuf(sizeof(tBTA_HF_CLIENT_API_ENABLE));
    p_buf->hdr.event = BTA_HF_CLIENT_API_ENABLE_EVT;
    p_buf->p_cback = p_cback;
    bta_sys_sendmsg(p_buf);
    return BTA_SUCCESS;
}

/*******************************************************************************
**
** Function         BTA_HfClientDisable
**
** Description      Disable the HF Client service
**
**
** Returns          void
**
*******************************************************************************/
void BTA_HfClientDisable(void)
{
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    p_buf->event = BTA_HF_CLIENT_API_DISABLE_EVT;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HfClientRegister
**
** Description      Register an HF Client service.
**
**
** Returns          void
**
*******************************************************************************/
void BTA_HfClientRegister(tBTA_SEC sec_mask, tBTA_HF_CLIENT_FEAT features,
                          char *p_service_name)
{
    tBTA_HF_CLIENT_API_REGISTER *p_buf =
                    (tBTA_HF_CLIENT_API_REGISTER *)GKI_getbuf(sizeof(tBTA_HF_CLIENT_API_REGISTER));
    p_buf->hdr.event = BTA_HF_CLIENT_API_REGISTER_EVT;
    p_buf->features = features;
    p_buf->sec_mask = sec_mask;

    if(p_service_name) {
        strlcpy(p_buf->name, p_service_name, BTA_SERVICE_NAME_LEN);
    } else {
        p_buf->name[0] = 0;
    }

    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HfClientDeregister
**
** Description      Deregister an HF Client service.
**
**
** Returns          void
**
*******************************************************************************/
void BTA_HfClientDeregister(uint16_t handle)
{
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    p_buf->event = BTA_HF_CLIENT_API_DEREGISTER_EVT;
    p_buf->layer_specific = handle;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HfClientOpen
**
** Description      Opens a connection to an audio gateway.
**                  When connection is open callback function is called
**                  with a BTA_AG_OPEN_EVT. Only the data connection is
**                  opened. The audio connection is not opened.
**
**
** Returns          void
**
*******************************************************************************/
void BTA_HfClientOpen(uint16_t handle, BD_ADDR bd_addr, tBTA_SEC sec_mask)
{
    tBTA_HF_CLIENT_API_OPEN *p_buf =
                    (tBTA_HF_CLIENT_API_OPEN *)GKI_getbuf(sizeof(tBTA_HF_CLIENT_API_OPEN));
    p_buf->hdr.event = BTA_HF_CLIENT_API_OPEN_EVT;
    p_buf->hdr.layer_specific = handle;
    bdcpy(p_buf->bd_addr, bd_addr);
    p_buf->sec_mask = sec_mask;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HfClientClose
**
** Description      Close the current connection to an audio gateway.
**                  Any current audio connection will also be closed
**
**
** Returns          void
**
*******************************************************************************/
void BTA_HfClientClose(uint16_t handle)
{
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    p_buf->event = BTA_HF_CLIENT_API_CLOSE_EVT;
    p_buf->layer_specific = handle;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HfCllientAudioOpen
**
** Description      Opens an audio connection to the currently connected
**                 audio gateway
**
**
** Returns          void
**
*******************************************************************************/
void BTA_HfClientAudioOpen(uint16_t handle)
{
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    p_buf->event = BTA_HF_CLIENT_API_AUDIO_OPEN_EVT;
    p_buf->layer_specific = handle;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HfClientAudioClose
**
** Description      Close the currently active audio connection to an audio
**                  gateway. The data connection remains open
**
**
** Returns          void
**
*******************************************************************************/
void BTA_HfClientAudioClose(uint16_t handle)
{
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    p_buf->event = BTA_HF_CLIENT_API_AUDIO_CLOSE_EVT;
    p_buf->layer_specific = handle;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         BTA_HfClientSendAT
**
** Description      send AT command
**
**
** Returns          void
**
*******************************************************************************/
void BTA_HfClientSendAT(uint16_t handle, tBTA_HF_CLIENT_AT_CMD_TYPE at, uint32_t val1,
                        uint32_t val2, const char *str)
{
    tBTA_HF_CLIENT_DATA_VAL *p_buf =
                    (tBTA_HF_CLIENT_DATA_VAL *)GKI_getbuf(sizeof(tBTA_HF_CLIENT_DATA_VAL));
    p_buf->hdr.event = BTA_HF_CLIENT_SEND_AT_CMD_EVT;
    p_buf->uint8_val = at;
    p_buf->uint32_val1 = val1;
    p_buf->uint32_val2 = val2;

    if(str) {
        strlcpy(p_buf->str, str, BTA_HF_CLIENT_NUMBER_LEN + 1);
        p_buf->str[BTA_HF_CLIENT_NUMBER_LEN] = '\0';
    } else {
        p_buf->str[0] = '\0';
    }

    p_buf->hdr.layer_specific = handle;
    bta_sys_sendmsg(p_buf);
}
#if (BTM_SCO_HCI_INCLUDED == TRUE )
void BTA_HfClientCiData(void)
{
    BT_HDR *p_buf;

    if((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL) {
        p_buf->event = BTA_HF_CLIENT_CI_SCO_DATA_EVT;
        bta_sys_sendmsg(p_buf);
    }
}
#endif /* #if (BTM_SCO_HCI_INCLUDED == TRUE ) */

#endif
