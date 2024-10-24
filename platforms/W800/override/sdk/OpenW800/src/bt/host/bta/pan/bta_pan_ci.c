/******************************************************************************
 *
 *  Copyright (C) 2004-2012 Broadcom Corporation
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
 *  This is the implementation file for data gateway call-in functions.
 *
 ******************************************************************************/

#include "bt_target.h"
#if defined(BTA_PAN_INCLUDED) && (BTA_PAN_INCLUDED == TRUE)
#include <string.h>

#include "bt_common.h"
#include "pan_api.h"
#include "bta_api.h"
#include "bta_pan_api.h"
#include "bta_pan_ci.h"
#include "bta_pan_int.h"
#include "bt_utils.h"

as a
/*******************************************************************************
**
** Function         bta_pan_ci_tx_ready
**
** Description      This function sends an event to PAN indicating the phone is
**                  ready for more data and PAN should call bta_pan_co_tx_path().
**                  This function is used when the TX data path is configured
**                  to use a pull interface.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_ci_tx_ready(uint16_t handle)
{
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    p_buf->layer_specific = handle;
    p_buf->event = BTA_PAN_CI_TX_READY_EVT;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         bta_pan_ci_rx_ready
**
** Description      This function sends an event to PAN indicating the phone
**                  has data available to send to PAN and PAN should call
**                  bta_pan_co_rx_path().  This function is used when the RX
**                  data path is configured to use a pull interface.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_ci_rx_ready(uint16_t handle)
{
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    p_buf->layer_specific = handle;
    p_buf->event = BTA_PAN_CI_RX_READY_EVT;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         bta_pan_ci_tx_flow
**
** Description      This function is called to enable or disable data flow on
**                  the TX path.  The phone should call this function to
**                  disable data flow when it is congested and cannot handle
**                  any more data sent by bta_pan_co_tx_write() or
**                  bta_pan_co_tx_writebuf().  This function is used when the
**                  TX data path is configured to use a push interface.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_ci_tx_flow(uint16_t handle, uint8_t enable)
{
    tBTA_PAN_CI_TX_FLOW  *p_buf =
                    (tBTA_PAN_CI_TX_FLOW *)GKI_getbuf(sizeof(tBTA_PAN_CI_TX_FLOW));
    p_buf->hdr.layer_specific = handle;
    p_buf->hdr.event = BTA_PAN_CI_TX_FLOW_EVT;
    p_buf->enable = enable;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         bta_pan_ci_rx_write
**
** Description      This function is called to send data to PAN when the RX path
**                  is configured to use a push interface.  The function copies
**                  data to an event buffer and sends it to PAN.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_ci_rx_write(uint16_t handle, BD_ADDR dst, BD_ADDR src, uint16_t protocol,
                         uint8_t *p_data, uint16_t len, uint8_t ext)
{
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(PAN_BUF_SIZE);
    p_buf->offset = PAN_MINIMUM_OFFSET;
    /* copy all other params before the data */
    bdcpy(((tBTA_PAN_DATA_PARAMS *)p_buf)->src, src);
    bdcpy(((tBTA_PAN_DATA_PARAMS *)p_buf)->dst, dst);
    ((tBTA_PAN_DATA_PARAMS *)p_buf)->protocol = protocol;
    ((tBTA_PAN_DATA_PARAMS *)p_buf)->ext = ext;
    p_buf->len = len;
    /* copy data */
    wm_memcpy((uint8_t *)(p_buf + 1) + p_buf->offset, p_data, len);
    p_buf->layer_specific = handle;
    p_buf->event = BTA_PAN_CI_RX_WRITEBUF_EVT;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         bta_pan_ci_rx_writebuf
**
** Description      This function is called to send data to the phone when
**                  the RX path is configured to use a push interface with
**                  zero copy.  The function sends an event to PAN containing
**                  the data buffer. The buffer will be freed by BTA; the
**                  phone must not free the buffer.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_ci_rx_writebuf(uint16_t handle, BD_ADDR dst, BD_ADDR src, uint16_t protocol,
                            BT_HDR *p_buf, uint8_t ext)
{
    /* copy all other params before the data */
    bdcpy(((tBTA_PAN_DATA_PARAMS *)p_buf)->src, src);
    bdcpy(((tBTA_PAN_DATA_PARAMS *)p_buf)->dst, dst);
    ((tBTA_PAN_DATA_PARAMS *)p_buf)->protocol = protocol;
    ((tBTA_PAN_DATA_PARAMS *)p_buf)->ext = ext;
    p_buf->layer_specific = handle;
    p_buf->event = BTA_PAN_CI_RX_WRITEBUF_EVT;
    bta_sys_sendmsg(p_buf);
}




/*******************************************************************************
**
** Function         bta_pan_ci_readbuf
**
** Description
**
**
** Returns          void
**
*******************************************************************************/
BT_HDR *bta_pan_ci_readbuf(uint16_t handle, BD_ADDR src, BD_ADDR dst, uint16_t *p_protocol,
                           uint8_t *p_ext, uint8_t *p_forward)
{
    tBTA_PAN_SCB *p_scb;
    BT_HDR *p_buf;
    p_scb = bta_pan_scb_by_handle(handle);
    p_buf = (BT_HDR *)fixed_queue_try_dequeue(p_scb->data_queue);

    if(p_buf != NULL) {
        bdcpy(src, ((tBTA_PAN_DATA_PARAMS *)p_buf)->src);
        bdcpy(dst, ((tBTA_PAN_DATA_PARAMS *)p_buf)->dst);
        *p_protocol = ((tBTA_PAN_DATA_PARAMS *)p_buf)->protocol;
        *p_ext = ((tBTA_PAN_DATA_PARAMS *)p_buf)->ext;
        *p_forward = ((tBTA_PAN_DATA_PARAMS *)p_buf)->forward;
    }

    return p_buf;
}


/*******************************************************************************
**
** Function         bta_pan_ci_set_mfilters
**
** Description      This function is called to set multicast filters
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_ci_set_mfilters(uint16_t handle, uint16_t num_mcast_filters, uint8_t *p_start_array,
                             uint8_t *p_end_array)
{
    PAN_SetMulticastFilters(handle, num_mcast_filters, p_start_array, p_end_array);
}


/*******************************************************************************
**
** Function         bta_pan_ci_set_mfilters
**
** Description      This function is called to set protocol filters
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_ci_set_pfilters(uint16_t handle, uint16_t num_filters, uint16_t *p_start_array,
                             uint16_t *p_end_array)
{
    PAN_SetProtocolFilters(handle, num_filters, p_start_array, p_end_array);
}
#endif

