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
 *  This is the API implementation file for the BTA device manager.
 *
 ******************************************************************************/

#include "bt_common.h"
#include "bta_sys.h"
#include "bta_api.h"
#include "bta_dm_int.h"
#include <string.h>
#include "bta_dm_ci.h"


/*******************************************************************************
**
** Function         bta_dm_ci_io_req
**
** Description      This function must be called in response to function
**                  bta_dm_co_io_req(), if *p_oob_data to BTA_OOB_UNKNOWN
**                  by bta_dm_co_io_req().
**
** Returns          void
**
*******************************************************************************/
void bta_dm_ci_io_req(BD_ADDR bd_addr, tBTA_IO_CAP io_cap, tBTA_OOB_DATA oob_data,
                      tBTA_AUTH_REQ auth_req)

{
    tBTA_DM_CI_IO_REQ *p_msg =
                    (tBTA_DM_CI_IO_REQ *)GKI_getbuf(sizeof(tBTA_DM_CI_IO_REQ));
    p_msg->hdr.event = BTA_DM_CI_IO_REQ_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->io_cap = io_cap;
    p_msg->oob_data = oob_data;
    p_msg->auth_req = auth_req;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         bta_dm_ci_rmt_oob
**
** Description      This function must be called in response to function
**                  bta_dm_co_rmt_oob() to provide the OOB data associated
**                  with the remote device.
**
** Returns          void
**
*******************************************************************************/
void bta_dm_ci_rmt_oob(uint8_t accept, BD_ADDR bd_addr, BT_OCTET16 c, BT_OCTET16 r)
{
    tBTA_DM_CI_RMT_OOB *p_msg =
                    (tBTA_DM_CI_RMT_OOB *)GKI_getbuf(sizeof(tBTA_DM_CI_RMT_OOB));
    p_msg->hdr.event = BTA_DM_CI_RMT_OOB_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->accept = accept;
    wm_memcpy(p_msg->c, c, BT_OCTET16_LEN);
    wm_memcpy(p_msg->r, r, BT_OCTET16_LEN);
    bta_sys_sendmsg(p_msg);
}

#if (BTM_SCO_HCI_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         bta_dm_sco_ci_data_ready
**
** Description      This function sends an event to indicating that the phone
**                  has SCO data ready.
**
** Parameters       event: is obtained from bta_dm_sco_co_open() function, which
**                          is the BTA event we want to send back to BTA module
**                          when there is encoded data ready.
**                  sco_handle: is the BTA sco handle which indicate a specific
**                           SCO connection.
** Returns          void
**
*******************************************************************************/
void bta_dm_sco_ci_data_ready(uint16_t event, uint16_t sco_handle)
{
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    p_buf->event = event;
    p_buf->layer_specific = sco_handle;
    bta_sys_sendmsg(p_buf);
}
#endif
