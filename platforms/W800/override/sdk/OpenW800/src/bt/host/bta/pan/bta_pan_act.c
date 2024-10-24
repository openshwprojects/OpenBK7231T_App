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
 *  This file contains the pan action functions for the state machine.
 *
 ******************************************************************************/

#include "bt_target.h"

#if defined(BTA_PAN_INCLUDED) && (BTA_PAN_INCLUDED == TRUE)
AA
#include "bta_api.h"
#include "bta_sys.h"
#include "bt_common.h"
#include "pan_api.h"
#include "bta_pan_api.h"
#include "bta_pan_int.h"
#include "bta_pan_co.h"
#include <string.h>
#include "utl.h"


/* RX and TX data flow mask */
#define BTA_PAN_RX_MASK              0x0F
#define BTA_PAN_TX_MASK              0xF0

/*******************************************************************************
 **
 ** Function    bta_pan_pm_conn_busy
 **
 ** Description set pan pm connection busy state
 **
 ** Params      p_scb: state machine control block of pan connection
 **
 ** Returns     void
 **
 *******************************************************************************/
static void bta_pan_pm_conn_busy(tBTA_PAN_SCB *p_scb)
{
    if((p_scb != NULL) && (p_scb->state != BTA_PAN_IDLE_ST)) {
        bta_sys_busy(BTA_ID_PAN, p_scb->app_id, p_scb->bd_addr);
    }
}

/*******************************************************************************
 **
 ** Function    bta_pan_pm_conn_idle
 **
 ** Description set pan pm connection idle state
 **
 ** Params      p_scb: state machine control block of pan connection
 **
 ** Returns     void
 **
 *******************************************************************************/
static void bta_pan_pm_conn_idle(tBTA_PAN_SCB *p_scb)
{
    if((p_scb != NULL) && (p_scb->state != BTA_PAN_IDLE_ST)) {
        bta_sys_idle(BTA_ID_PAN, p_scb->app_id, p_scb->bd_addr);
    }
}

/*******************************************************************************
**
** Function         bta_pan_conn_state_cback
**
** Description      Connection state callback from Pan profile
**
**
** Returns          void
**
*******************************************************************************/
static void bta_pan_conn_state_cback(uint16_t handle, BD_ADDR bd_addr, tPAN_RESULT state,
                                     uint8_t is_role_change, uint8_t src_role, uint8_t dst_role)
{
    tBTA_PAN_SCB *p_scb;
    tBTA_PAN_CONN *p_buf = (tBTA_PAN_CONN *)GKI_getbuf(sizeof(tBTA_PAN_CONN));

    if((state == PAN_SUCCESS) && !is_role_change) {
        p_buf->hdr.event = BTA_PAN_CONN_OPEN_EVT;

        if((p_scb = bta_pan_scb_by_handle(handle)) == NULL) {
            /* allocate an scb */
            p_scb = bta_pan_scb_alloc();
        }

        /* we have exceeded maximum number of connections */
        if(!p_scb) {
            PAN_Disconnect(handle);
            return;
        }

        p_scb->handle = handle;
        p_scb->local_role = src_role;
        p_scb->peer_role = dst_role;
        p_scb->pan_flow_enable = TRUE;
        bdcpy(p_scb->bd_addr, bd_addr);
        p_scb->data_queue = fixed_queue_new(SIZE_MAX);

        if(src_role == PAN_ROLE_CLIENT) {
            p_scb->app_id = bta_pan_cb.app_id[0];
        } else if(src_role == PAN_ROLE_GN_SERVER) {
            p_scb->app_id = bta_pan_cb.app_id[1];
        } else if(src_role == PAN_ROLE_NAP_SERVER) {
            p_scb->app_id = bta_pan_cb.app_id[2];
        }
    } else if((state != PAN_SUCCESS) && !is_role_change) {
        p_buf->hdr.event = BTA_PAN_CONN_CLOSE_EVT;
    } else {
        return;
    }

    p_buf->result = state;
    p_buf->hdr.layer_specific = handle;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         bta_pan_data_flow_cb
**
** Description      Data flow status callback from PAN
**
**
** Returns          void
**
*******************************************************************************/
static void bta_pan_data_flow_cb(uint16_t handle, tPAN_RESULT result)
{
    tBTA_PAN_SCB *p_scb;

    if((p_scb = bta_pan_scb_by_handle(handle)) == NULL) {
        return;
    }

    if(result == PAN_TX_FLOW_ON) {
        BT_HDR *p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR));
        p_buf->layer_specific = handle;
        p_buf->event = BTA_PAN_BNEP_FLOW_ENABLE_EVT;
        bta_sys_sendmsg(p_buf);
        bta_pan_co_rx_flow(handle, p_scb->app_id, TRUE);
    } else if(result == PAN_TX_FLOW_OFF) {
        p_scb->pan_flow_enable = FALSE;
        bta_pan_co_rx_flow(handle, p_scb->app_id, FALSE);
    }
}

/*******************************************************************************
**
** Function         bta_pan_data_buf_ind_cback
**
** Description      data indication callback from pan profile
**
**
** Returns          void
**
*******************************************************************************/
static void bta_pan_data_buf_ind_cback(uint16_t handle, BD_ADDR src, BD_ADDR dst, uint16_t protocol,
                                       BT_HDR *p_buf,
                                       uint8_t ext, uint8_t forward)
{
    tBTA_PAN_SCB *p_scb;
    BT_HDR *p_new_buf;

    if(sizeof(tBTA_PAN_DATA_PARAMS) > p_buf->offset) {
        /* offset smaller than data structure in front of actual data */
        p_new_buf = (BT_HDR *)GKI_getbuf(PAN_BUF_SIZE);
        wm_memcpy((uint8_t *)(p_new_buf + 1) + sizeof(tBTA_PAN_DATA_PARAMS),
                  (uint8_t *)(p_buf + 1) + p_buf->offset, p_buf->len);
        p_new_buf->len    = p_buf->len;
        p_new_buf->offset = sizeof(tBTA_PAN_DATA_PARAMS);
        GKI_freebuf(p_buf);
    } else {
        p_new_buf = p_buf;
    }

    /* copy params into the space before the data */
    bdcpy(((tBTA_PAN_DATA_PARAMS *)p_new_buf)->src, src);
    bdcpy(((tBTA_PAN_DATA_PARAMS *)p_new_buf)->dst, dst);
    ((tBTA_PAN_DATA_PARAMS *)p_new_buf)->protocol = protocol;
    ((tBTA_PAN_DATA_PARAMS *)p_new_buf)->ext = ext;
    ((tBTA_PAN_DATA_PARAMS *)p_new_buf)->forward = forward;

    if((p_scb = bta_pan_scb_by_handle(handle)) == NULL) {
        GKI_freebuf(p_new_buf);
        return;
    }

    fixed_queue_enqueue(p_scb->data_queue, p_new_buf);
    BT_HDR *p_event = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    p_event->layer_specific = handle;
    p_event->event = BTA_PAN_RX_FROM_BNEP_READY_EVT;
    bta_sys_sendmsg(p_event);
}

/*******************************************************************************
**
** Function         bta_pan_pfilt_ind_cback
**
** Description
**
**
** Returns          void
**
*******************************************************************************/
static void bta_pan_pfilt_ind_cback(uint16_t handle, uint8_t indication, tBNEP_RESULT result,
                                    uint16_t num_filters, uint8_t *p_filters)
{
    bta_pan_co_pfilt_ind(handle, indication,
                         (tBTA_PAN_STATUS)((result == BNEP_SUCCESS) ? BTA_PAN_SUCCESS : BTA_PAN_FAIL),
                         num_filters, p_filters);
}


/*******************************************************************************
**
** Function         bta_pan_mfilt_ind_cback
**
** Description
**
**
** Returns          void
**
*******************************************************************************/
static void bta_pan_mfilt_ind_cback(uint16_t handle, uint8_t indication, tBNEP_RESULT result,
                                    uint16_t num_mfilters, uint8_t *p_mfilters)
{
    bta_pan_co_mfilt_ind(handle, indication,
                         (tBTA_PAN_STATUS)((result == BNEP_SUCCESS) ? BTA_PAN_SUCCESS : BTA_PAN_FAIL),
                         num_mfilters, p_mfilters);
}



/*******************************************************************************
**
** Function         bta_pan_has_multiple_connections
**
** Description      Check whether there are multiple GN/NAP connections to
**                  different devices
**
**
** Returns          uint8_t
**
*******************************************************************************/
static uint8_t bta_pan_has_multiple_connections(uint8_t app_id)
{
    tBTA_PAN_SCB *p_scb = NULL;
    uint8_t     found = FALSE;
    BD_ADDR     bd_addr;

    for(uint8_t index = 0; index < BTA_PAN_NUM_CONN; index++) {
        p_scb = &bta_pan_cb.scb[index];

        if(p_scb->in_use == TRUE && app_id == p_scb->app_id) {
            /* save temp bd_addr */
            bdcpy(bd_addr, p_scb->bd_addr);
            found = TRUE;
            break;
        }
    }

    /* If cannot find a match then there is no connection at all */
    if(found == FALSE) {
        return FALSE;
    }

    /* Find whether there is another connection with different device other than PANU.
        Could be same service or different service */
    for(uint8_t index = 0; index < BTA_PAN_NUM_CONN; index++) {
        p_scb = &bta_pan_cb.scb[index];

        if(p_scb->in_use == TRUE && p_scb->app_id != bta_pan_cb.app_id[0] &&
                bdcmp(bd_addr, p_scb->bd_addr)) {
            return TRUE;
        }
    }

    return FALSE;
}

/*******************************************************************************
**
** Function         bta_pan_enable
**
** Description
**
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_enable(tBTA_PAN_DATA *p_data)
{
    tPAN_REGISTER reg_data;
    uint16_t  initial_discoverability;
    uint16_t  initial_connectability;
    uint16_t  d_window;
    uint16_t  d_interval;
    uint16_t  c_window;
    uint16_t  c_interval;
    bta_pan_cb.p_cback = p_data->api_enable.p_cback;
    reg_data.pan_conn_state_cb  = bta_pan_conn_state_cback;
    reg_data.pan_bridge_req_cb  = NULL;
    reg_data.pan_data_buf_ind_cb = bta_pan_data_buf_ind_cback;
    reg_data.pan_data_ind_cb = NULL;
    reg_data.pan_pfilt_ind_cb = bta_pan_pfilt_ind_cback;
    reg_data.pan_mfilt_ind_cb = bta_pan_mfilt_ind_cback;
    reg_data.pan_tx_data_flow_cb = bta_pan_data_flow_cb;
    /* read connectability and discoverability settings.
    Pan profile changes the settings. We have to change it back to
    be consistent with other bta subsystems */
    initial_connectability = BTM_ReadConnectability(&c_window, &c_interval);
    initial_discoverability = BTM_ReadDiscoverability(&d_window, &d_interval);
    PAN_Register(&reg_data);
    /* set it back to original value */
    BTM_SetDiscoverability(initial_discoverability, d_window, d_interval);
    BTM_SetConnectability(initial_connectability, c_window, c_interval);
    bta_pan_cb.flow_mask = bta_pan_co_init(&bta_pan_cb.q_level);
    bta_pan_cb.p_cback(BTA_PAN_ENABLE_EVT, NULL);
}

/*******************************************************************************
**
** Function         bta_pan_set_role
**
** Description
**
** Returns          void
**
*******************************************************************************/
void bta_pan_set_role(tBTA_PAN_DATA *p_data)
{
    tPAN_RESULT status;
    tBTA_PAN_SET_ROLE set_role;
    uint8_t  sec[3];
    bta_pan_cb.app_id[0] = p_data->api_set_role.user_app_id;
    bta_pan_cb.app_id[1] = p_data->api_set_role.gn_app_id;
    bta_pan_cb.app_id[2] = p_data->api_set_role.nap_app_id;
    sec[0] = p_data->api_set_role.user_sec_mask;
    sec[1] = p_data->api_set_role.gn_sec_mask;
    sec[2] = p_data->api_set_role.nap_sec_mask;
    /* set security correctly in api and here */
    status = PAN_SetRole(p_data->api_set_role.role, sec,
                         p_data->api_set_role.user_name,
                         p_data->api_set_role.gn_name,
                         p_data->api_set_role.nap_name);
    set_role.role = p_data->api_set_role.role;

    if(status == PAN_SUCCESS) {
        if(p_data->api_set_role.role & PAN_ROLE_NAP_SERVER) {
            bta_sys_add_uuid(UUID_SERVCLASS_NAP);
        } else {
            bta_sys_remove_uuid(UUID_SERVCLASS_NAP);
        }

        if(p_data->api_set_role.role & PAN_ROLE_GN_SERVER) {
            bta_sys_add_uuid(UUID_SERVCLASS_GN);
        } else {
            bta_sys_remove_uuid(UUID_SERVCLASS_GN);
        }

        if(p_data->api_set_role.role & PAN_ROLE_CLIENT) {
            bta_sys_add_uuid(UUID_SERVCLASS_PANU);
        } else {
            bta_sys_remove_uuid(UUID_SERVCLASS_PANU);
        }

        set_role.status = BTA_PAN_SUCCESS;
    }
    /* if status is not success clear everything */
    else {
        PAN_SetRole(0, 0, NULL, NULL, NULL);
        bta_sys_remove_uuid(UUID_SERVCLASS_NAP);
        bta_sys_remove_uuid(UUID_SERVCLASS_GN);
        bta_sys_remove_uuid(UUID_SERVCLASS_PANU);
        set_role.status = BTA_PAN_FAIL;
    }

    bta_pan_cb.p_cback(BTA_PAN_SET_ROLE_EVT, (tBTA_PAN *)&set_role);
}



/*******************************************************************************
**
** Function         bta_pan_disable
**
** Description
**
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_disable(void)
{
    BT_HDR *p_buf;
    tBTA_PAN_SCB *p_scb = &bta_pan_cb.scb[0];
    uint8_t i;
    /* close all connections */
    PAN_SetRole(0, NULL, NULL, NULL, NULL);
#if (BTA_EIR_CANNED_UUID_LIST != TRUE)
    bta_sys_remove_uuid(UUID_SERVCLASS_NAP);
    bta_sys_remove_uuid(UUID_SERVCLASS_GN);
    bta_sys_remove_uuid(UUID_SERVCLASS_PANU);
#endif // BTA_EIR_CANNED_UUID_LIST

    /* free all queued up data buffers */
    for(i = 0; i < BTA_PAN_NUM_CONN; i++, p_scb++) {
        if(p_scb->in_use) {
            while((p_buf = (BT_HDR *)fixed_queue_try_dequeue(p_scb->data_queue)) != NULL) {
                GKI_freebuf(p_buf);
            }

            bta_pan_co_close(p_scb->handle, p_scb->app_id);
        }
    }

    PAN_Deregister();
}

/*******************************************************************************
**
** Function         bta_pan_open
**
** Description
**
** Returns          void
**
*******************************************************************************/
void bta_pan_open(tBTA_PAN_SCB *p_scb, tBTA_PAN_DATA *p_data)
{
    tPAN_RESULT status;
    tBTA_PAN_OPEN data;
    tBTA_PAN_OPENING    opening;
    status = PAN_Connect(p_data->api_open.bd_addr, p_data->api_open.local_role,
                         p_data->api_open.peer_role,
                         &p_scb->handle);
    APPL_TRACE_DEBUG("%s pan connect status: %d", __func__, status);

    if(status == PAN_SUCCESS) {
        bdcpy(p_scb->bd_addr, p_data->api_open.bd_addr);
        p_scb->local_role = p_data->api_open.local_role;
        p_scb->peer_role = p_data->api_open.peer_role;
        bdcpy(opening.bd_addr, p_data->api_open.bd_addr);
        opening.handle = p_scb->handle;
        bta_pan_cb.p_cback(BTA_PAN_OPENING_EVT, (tBTA_PAN *)&opening);
    } else {
        bta_pan_scb_dealloc(p_scb);
        bdcpy(data.bd_addr, p_data->api_open.bd_addr);
        data.status = BTA_PAN_FAIL;
        data.local_role = p_data->api_open.local_role;
        data.peer_role = p_data->api_open.peer_role;
        bta_pan_cb.p_cback(BTA_PAN_OPEN_EVT, (tBTA_PAN *)&data);
    }
}


/*******************************************************************************
**
** Function         bta_pan_close
**
** Description
**
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_api_close(tBTA_PAN_SCB *p_scb, tBTA_PAN_DATA *p_data)
{
    tBTA_PAN_CONN *p_buf = (tBTA_PAN_CONN *)GKI_getbuf(sizeof(tBTA_PAN_CONN));
    UNUSED(p_data);
    PAN_Disconnect(p_scb->handle);
    /*
     * Send an event to BTA so that application will get the connection
     * close event.
     */
    p_buf->hdr.event = BTA_PAN_CONN_CLOSE_EVT;
    p_buf->hdr.layer_specific = p_scb->handle;
    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         bta_pan_conn_open
**
** Description      process connection open event
**
** Returns          void
**
*******************************************************************************/
void bta_pan_conn_open(tBTA_PAN_SCB *p_scb, tBTA_PAN_DATA *p_data)
{
    tBTA_PAN_OPEN data;
    APPL_TRACE_DEBUG("%s pan connection result: %d", __func__, p_data->conn.result);
    bdcpy(data.bd_addr, p_scb->bd_addr);
    data.handle = p_scb->handle;
    data.local_role = p_scb->local_role;
    data.peer_role = p_scb->peer_role;

    if(p_data->conn.result == PAN_SUCCESS) {
        data.status = BTA_PAN_SUCCESS;
        p_scb->pan_flow_enable = TRUE;
        p_scb->app_flow_enable = TRUE;
        bta_sys_conn_open(BTA_ID_PAN, p_scb->app_id, p_scb->bd_addr);
    } else {
        bta_pan_scb_dealloc(p_scb);
        data.status = BTA_PAN_FAIL;
    }

    p_scb->pan_flow_enable = TRUE;
    p_scb->app_flow_enable = TRUE;

    /* If app_id is NAP/GN, check whether there are multiple connections.
       If there are, provide a special app_id to dm to enforce master role only. */
    if((p_scb->app_id == bta_pan_cb.app_id[1] || p_scb->app_id == bta_pan_cb.app_id[2]) &&
            bta_pan_has_multiple_connections(p_scb->app_id)) {
        p_scb->app_id = BTA_APP_ID_PAN_MULTI;
    }

    bta_sys_conn_open(BTA_ID_PAN, p_scb->app_id, p_scb->bd_addr);
    bta_pan_cb.p_cback(BTA_PAN_OPEN_EVT, (tBTA_PAN *)&data);
}

/*******************************************************************************
**
** Function         bta_pan_conn_close
**
** Description      process connection close event
**
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_conn_close(tBTA_PAN_SCB *p_scb, tBTA_PAN_DATA *p_data)
{
    tBTA_PAN_CLOSE data;
    BT_HDR *p_buf;
    data.handle = p_data->hdr.layer_specific;
    bta_sys_conn_close(BTA_ID_PAN, p_scb->app_id, p_scb->bd_addr);

    /* free all queued up data buffers */
    while((p_buf = (BT_HDR *)fixed_queue_try_dequeue(p_scb->data_queue)) != NULL) {
        GKI_freebuf(p_buf);
    }

    bta_pan_scb_dealloc(p_scb);
    bta_pan_cb.p_cback(BTA_PAN_CLOSE_EVT, (tBTA_PAN *)&data);
}




/*******************************************************************************
**
** Function         bta_pan_rx_path
**
** Description      Handle data on the RX path (data sent from the phone to
**                  BTA).
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_rx_path(tBTA_PAN_SCB *p_scb, tBTA_PAN_DATA *p_data)
{
    UNUSED(p_data);

    /* if data path configured for rx pull */
    if((bta_pan_cb.flow_mask & BTA_PAN_RX_MASK) == BTA_PAN_RX_PULL) {
        /* if we can accept data */
        if(p_scb->pan_flow_enable == TRUE) {
            /* call application callout function for rx path */
            bta_pan_co_rx_path(p_scb->handle, p_scb->app_id);
        }
    }
    /* else data path configured for rx push */
    else {
    }
}

/*******************************************************************************
**
** Function         bta_pan_tx_path
**
** Description      Handle the TX data path (data sent from BTA to the phone).
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_tx_path(tBTA_PAN_SCB *p_scb, tBTA_PAN_DATA *p_data)
{
    UNUSED(p_data);

    /* if data path configured for tx pull */
    if((bta_pan_cb.flow_mask & BTA_PAN_TX_MASK) == BTA_PAN_TX_PULL) {
        bta_pan_pm_conn_busy(p_scb);
        /* call application callout function for tx path */
        bta_pan_co_tx_path(p_scb->handle, p_scb->app_id);

        /* free data that exceeds queue level */
        while(fixed_queue_length(p_scb->data_queue) > bta_pan_cb.q_level) {
            GKI_freebuf(fixed_queue_try_dequeue(p_scb->data_queue));
        }

        bta_pan_pm_conn_idle(p_scb);
    }
    /* if configured for zero copy push */
    else if((bta_pan_cb.flow_mask & BTA_PAN_TX_MASK) == BTA_PAN_TX_PUSH_BUF) {
        /* if app can accept data */
        if(p_scb->app_flow_enable == TRUE) {
            BT_HDR *p_buf;

            /* read data from the queue */
            if((p_buf = (BT_HDR *)fixed_queue_try_dequeue(p_scb->data_queue)) != NULL) {
                /* send data to application */
                bta_pan_co_tx_writebuf(p_scb->handle,
                                       p_scb->app_id,
                                       ((tBTA_PAN_DATA_PARAMS *)p_buf)->src,
                                       ((tBTA_PAN_DATA_PARAMS *)p_buf)->dst,
                                       ((tBTA_PAN_DATA_PARAMS *)p_buf)->protocol,
                                       p_buf,
                                       ((tBTA_PAN_DATA_PARAMS *)p_buf)->ext,
                                       ((tBTA_PAN_DATA_PARAMS *)p_buf)->forward);
            }

            /* free data that exceeds queue level  */
            while(fixed_queue_length(p_scb->data_queue) > bta_pan_cb.q_level) {
                GKI_freebuf(fixed_queue_try_dequeue(p_scb->data_queue));
            }

            /* if there is more data to be passed to
            upper layer */
            if(!fixed_queue_is_empty(p_scb->data_queue)) {
                p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
                p_buf->layer_specific = p_scb->handle;
                p_buf->event = BTA_PAN_RX_FROM_BNEP_READY_EVT;
                bta_sys_sendmsg(p_buf);
            }
        }
    }
}

/*******************************************************************************
**
** Function         bta_pan_tx_flow
**
** Description      Set the application flow control state.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_tx_flow(tBTA_PAN_SCB *p_scb, tBTA_PAN_DATA *p_data)
{
    p_scb->app_flow_enable = p_data->ci_tx_flow.enable;
}

/*******************************************************************************
**
** Function         bta_pan_write_buf
**
** Description      Handle a bta_pan_ci_rx_writebuf() and send data to PAN.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_write_buf(tBTA_PAN_SCB *p_scb, tBTA_PAN_DATA *p_data)
{
    if((bta_pan_cb.flow_mask & BTA_PAN_RX_MASK) == BTA_PAN_RX_PUSH_BUF) {
        bta_pan_pm_conn_busy(p_scb);
        PAN_WriteBuf(p_scb->handle,
                     ((tBTA_PAN_DATA_PARAMS *)p_data)->dst,
                     ((tBTA_PAN_DATA_PARAMS *)p_data)->src,
                     ((tBTA_PAN_DATA_PARAMS *)p_data)->protocol,
                     (BT_HDR *)p_data,
                     ((tBTA_PAN_DATA_PARAMS *)p_data)->ext);
        bta_pan_pm_conn_idle(p_scb);
    }
}

/*******************************************************************************
**
** Function         bta_pan_free_buf
**
** Description      Frees the data buffer during closing state
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_free_buf(tBTA_PAN_SCB *p_scb, tBTA_PAN_DATA *p_data)
{
    UNUSED(p_scb);
    GKI_freebuf(p_data);
}

#endif /* PAN_INCLUDED */
