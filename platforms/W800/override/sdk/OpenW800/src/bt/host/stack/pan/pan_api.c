/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
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

/*****************************************************************************
 *
 *  This file contains main functions to support PAN profile
 *  commands and events.
 *
 *****************************************************************************/

#include <string.h>
#include "bt_target.h"
#if defined(PAN_INCLUDED) &&(PAN_INCLUDED == TRUE)
#include "bt_common.h"
#include "bt_types.h"
#include "bnep_api.h"
#include "pan_api.h"
#include "pan_int.h"
#include "sdp_api.h"
#include "sdpdefs.h"
#include "l2c_api.h"
#include "hcidefs.h"
#include "btm_api.h"
#include "bta_sys.h"


/*******************************************************************************
**
** Function         PAN_Register
**
** Description      This function is called by the application to register
**                  its callbacks with PAN profile. The application then
**                  should set the PAN role explicitly.
**
** Parameters:      p_register - contains all callback function pointers
**
**
** Returns          none
**
*******************************************************************************/
void PAN_Register(tPAN_REGISTER *p_register)
{
    BTM_SetDiscoverability(BTM_GENERAL_DISCOVERABLE, 0, 0);
    BTM_SetConnectability(BTM_CONNECTABLE, 0, 0);
    pan_register_with_bnep();

    if(!p_register) {
        return;
    }

    pan_cb.pan_conn_state_cb    = p_register->pan_conn_state_cb;
    pan_cb.pan_bridge_req_cb    = p_register->pan_bridge_req_cb;
    pan_cb.pan_data_buf_ind_cb  = p_register->pan_data_buf_ind_cb;
    pan_cb.pan_data_ind_cb      = p_register->pan_data_ind_cb;
    pan_cb.pan_pfilt_ind_cb     = p_register->pan_pfilt_ind_cb;
    pan_cb.pan_mfilt_ind_cb     = p_register->pan_mfilt_ind_cb;
    pan_cb.pan_tx_data_flow_cb  = p_register->pan_tx_data_flow_cb;
    return;
}



/*******************************************************************************
**
** Function         PAN_Deregister
**
** Description      This function is called by the application to de-register
**                  its callbacks with PAN profile. This will make the PAN to
**                  become inactive. This will deregister PAN services from SDP
**                  and close all active connections
**
** Parameters:      none
**
**
** Returns          none
**
*******************************************************************************/
void PAN_Deregister(void)
{
    pan_cb.pan_bridge_req_cb    = NULL;
    pan_cb.pan_data_buf_ind_cb  = NULL;
    pan_cb.pan_data_ind_cb      = NULL;
    pan_cb.pan_conn_state_cb    = NULL;
    pan_cb.pan_pfilt_ind_cb     = NULL;
    pan_cb.pan_mfilt_ind_cb     = NULL;
    PAN_SetRole(PAN_ROLE_INACTIVE, NULL, NULL, NULL, NULL);
    BNEP_Deregister();
    return;
}




/*******************************************************************************
**
** Function         PAN_SetRole
**
** Description      This function is called by the application to set the PAN
**                  profile role. This should be called after PAN_Register.
**                  This can be called any time to change the PAN role
**
** Parameters:      role        - is bit map of roles to be active
**                                      PAN_ROLE_CLIENT is for PANU role
**                                      PAN_ROLE_GN_SERVER is for GN role
**                                      PAN_ROLE_NAP_SERVER is for NAP role
**                  sec_mask    - Security mask for different roles
**                                      It is array of uint8_t. The byte represent the
**                                      security for roles PANU, GN and NAP in order
**                  p_user_name - Service name for PANU role
**                  p_gn_name   - Service name for GN role
**                  p_nap_name  - Service name for NAP role
**                                      Can be NULL if user wants it to be default
**
** Returns          PAN_SUCCESS     - if the role is set successfully
**                  PAN_FAILURE     - if the role is not valid
**
*******************************************************************************/
tPAN_RESULT PAN_SetRole(uint8_t role,
                        uint8_t *sec_mask,
                        char *p_user_name,
                        char *p_gn_name,
                        char *p_nap_name)
{
    char                *p_desc;
    uint8_t               security[3] = {PAN_PANU_SECURITY_LEVEL,
                                         PAN_GN_SECURITY_LEVEL,
                                         PAN_NAP_SECURITY_LEVEL
                                        };
    uint8_t               *p_sec;

    /* If the role is not a valid combination reject it */
    if((!(role & (PAN_ROLE_CLIENT | PAN_ROLE_GN_SERVER | PAN_ROLE_NAP_SERVER))) &&
            role != PAN_ROLE_INACTIVE) {
        PAN_TRACE_ERROR("PAN role %d is invalid", role);
        return PAN_FAILURE;
    }

    /* If the current active role is same as the role being set do nothing */
    if(pan_cb.role == role) {
        PAN_TRACE_EVENT("PAN role already was set to: %d", role);
        return PAN_SUCCESS;
    }

    if(!sec_mask) {
        p_sec = security;
    } else {
        p_sec = sec_mask;
    }

    /* Register all the roles with SDP */
    PAN_TRACE_API("PAN_SetRole() called with role 0x%x", role);
#if (defined (PAN_SUPPORTS_ROLE_NAP) && PAN_SUPPORTS_ROLE_NAP == TRUE)

    /* Check the service name */
    if((p_nap_name == NULL) || (*p_nap_name == 0)) {
        p_nap_name = PAN_NAP_DEFAULT_SERVICE_NAME;
    }

    if(role & PAN_ROLE_NAP_SERVER) {
        /* Registering for NAP service with SDP */
        p_desc = PAN_NAP_DEFAULT_DESCRIPTION;

        if(pan_cb.pan_nap_sdp_handle != 0) {
            SDP_DeleteRecord(pan_cb.pan_nap_sdp_handle);
        }

        pan_cb.pan_nap_sdp_handle = pan_register_with_sdp(UUID_SERVCLASS_NAP, p_sec[2], p_nap_name, p_desc);
        bta_sys_add_uuid(UUID_SERVCLASS_NAP);
    }
    /* If the NAP role is already active and now being cleared delete the record */
    else if(pan_cb.role & PAN_ROLE_NAP_SERVER) {
        if(pan_cb.pan_nap_sdp_handle != 0) {
            SDP_DeleteRecord(pan_cb.pan_nap_sdp_handle);
            pan_cb.pan_nap_sdp_handle = 0;
            bta_sys_remove_uuid(UUID_SERVCLASS_NAP);
        }
    }

#endif
#if (defined (PAN_SUPPORTS_ROLE_GN) && PAN_SUPPORTS_ROLE_GN == TRUE)

    /* Check the service name */
    if((p_gn_name == NULL) || (*p_gn_name == 0)) {
        p_gn_name = PAN_GN_DEFAULT_SERVICE_NAME;
    }

    if(role & PAN_ROLE_GN_SERVER) {
        /* Registering for GN service with SDP */
        p_desc = PAN_GN_DEFAULT_DESCRIPTION;

        if(pan_cb.pan_gn_sdp_handle != 0) {
            SDP_DeleteRecord(pan_cb.pan_gn_sdp_handle);
        }

        pan_cb.pan_gn_sdp_handle = pan_register_with_sdp(UUID_SERVCLASS_GN, p_sec[1], p_gn_name, p_desc);
        bta_sys_add_uuid(UUID_SERVCLASS_GN);
    }
    /* If the GN role is already active and now being cleared delete the record */
    else if(pan_cb.role & PAN_ROLE_GN_SERVER) {
        if(pan_cb.pan_gn_sdp_handle != 0) {
            SDP_DeleteRecord(pan_cb.pan_gn_sdp_handle);
            pan_cb.pan_gn_sdp_handle = 0;
            bta_sys_remove_uuid(UUID_SERVCLASS_GN);
        }
    }

#endif
#if (defined (PAN_SUPPORTS_ROLE_PANU) && PAN_SUPPORTS_ROLE_PANU == TRUE)

    /* Check the service name */
    if((p_user_name == NULL) || (*p_user_name == 0)) {
        p_user_name = PAN_PANU_DEFAULT_SERVICE_NAME;
    }

    if(role & PAN_ROLE_CLIENT) {
        /* Registering for PANU service with SDP */
        p_desc = PAN_PANU_DEFAULT_DESCRIPTION;

        if(pan_cb.pan_user_sdp_handle != 0) {
            SDP_DeleteRecord(pan_cb.pan_user_sdp_handle);
        }

        pan_cb.pan_user_sdp_handle = pan_register_with_sdp(UUID_SERVCLASS_PANU, p_sec[0], p_user_name,
                                     p_desc);
        bta_sys_add_uuid(UUID_SERVCLASS_PANU);
    }
    /* If the PANU role is already active and now being cleared delete the record */
    else if(pan_cb.role & PAN_ROLE_CLIENT) {
        if(pan_cb.pan_user_sdp_handle != 0) {
            SDP_DeleteRecord(pan_cb.pan_user_sdp_handle);
            pan_cb.pan_user_sdp_handle = 0;
            bta_sys_remove_uuid(UUID_SERVCLASS_PANU);
        }
    }

#endif

    /* Check if it is a shutdown request */
    if(role == PAN_ROLE_INACTIVE) {
        pan_close_all_connections();
    }

    pan_cb.role = role;
    PAN_TRACE_EVENT("PAN role set to: %d", role);
    return PAN_SUCCESS;
}



/*******************************************************************************
**
** Function         PAN_Connect
**
** Description      This function is called by the application to initiate a
**                  connection to the remote device
**
** Parameters:      rem_bda     - BD Addr of the remote device
**                  src_role    - Role of the local device for the connection
**                  dst_role    - Role of the remote device for the connection
**                                      PAN_ROLE_CLIENT is for PANU role
**                                      PAN_ROLE_GN_SERVER is for GN role
**                                      PAN_ROLE_NAP_SERVER is for NAP role
**                  *handle     - Pointer for returning Handle to the connection
**
** Returns          PAN_SUCCESS      - if the connection is initiated successfully
**                  PAN_NO_RESOURCES - resources are not sufficent
**                  PAN_FAILURE      - if the connection cannot be initiated
**                                           this can be because of the combination of
**                                           src and dst roles may not be valid or
**                                           allowed at that point of time
**
*******************************************************************************/
tPAN_RESULT PAN_Connect(BD_ADDR rem_bda, uint8_t src_role, uint8_t dst_role, uint16_t *handle)
{
    tPAN_CONN       *pcb;
    tBNEP_RESULT    result;
    tBT_UUID        src_uuid, dst_uuid;
    uint32_t mx_chan_id;
    /*
    ** Initialize the handle so that in case of failure return values
    ** the profile will not get confused
    */
    *handle = BNEP_INVALID_HANDLE;

    /* Check if PAN is active or not */
    if(!(pan_cb.role & src_role)) {
        PAN_TRACE_ERROR("PAN is not active for the role %d", src_role);
        return PAN_FAILURE;
    }

    /* Validate the parameters before proceeding */
    if((src_role != PAN_ROLE_CLIENT && src_role != PAN_ROLE_GN_SERVER
            && src_role != PAN_ROLE_NAP_SERVER) ||
            (dst_role != PAN_ROLE_CLIENT && dst_role != PAN_ROLE_GN_SERVER
             && dst_role != PAN_ROLE_NAP_SERVER)) {
        PAN_TRACE_ERROR("Either source %d or destination role %d is invalid", src_role, dst_role);
        return PAN_FAILURE;
    }

    /* Check if connection exists for this remote device */
    pcb = pan_get_pcb_by_addr(rem_bda);

    /* If we are PANU for this role validate destination role */
    if(src_role == PAN_ROLE_CLIENT) {
        if((pan_cb.num_conns > 1) || (pan_cb.num_conns && (!pcb))) {
            /*
            ** If the request is not for existing connection reject it
            ** because if there is already a connection we cannot accept
            ** another connection in PANU role
            */
            PAN_TRACE_ERROR("Cannot make PANU connections when there are more than one connection");
            return PAN_INVALID_SRC_ROLE;
        }

        src_uuid.uu.uuid16 = UUID_SERVCLASS_PANU;

        if(dst_role == PAN_ROLE_CLIENT) {
            dst_uuid.uu.uuid16 = UUID_SERVCLASS_PANU;
        } else if(dst_role == PAN_ROLE_GN_SERVER) {
            dst_uuid.uu.uuid16 = UUID_SERVCLASS_GN;
        } else {
            dst_uuid.uu.uuid16 = UUID_SERVCLASS_NAP;
        }

        mx_chan_id = dst_uuid.uu.uuid16;
    }
    /* If destination is PANU role validate source role */
    else if(dst_role == PAN_ROLE_CLIENT) {
        if(pan_cb.num_conns && pan_cb.active_role == PAN_ROLE_CLIENT && !pcb) {
            PAN_TRACE_ERROR("Device already have a connection in PANU role");
            return PAN_INVALID_SRC_ROLE;
        }

        dst_uuid.uu.uuid16 = UUID_SERVCLASS_PANU;

        if(src_role == PAN_ROLE_GN_SERVER) {
            src_uuid.uu.uuid16 = UUID_SERVCLASS_GN;
        } else {
            src_uuid.uu.uuid16 = UUID_SERVCLASS_NAP;
        }

        mx_chan_id = src_uuid.uu.uuid16;
    }
    /* The role combination is not valid */
    else {
        PAN_TRACE_ERROR("Source %d and Destination roles %d are not valid combination",
                        src_role, dst_role);
        return PAN_FAILURE;
    }

    /* Allocate control block and initiate connection */
    if(!pcb) {
        pcb = pan_allocate_pcb(rem_bda, BNEP_INVALID_HANDLE);
    }

    if(!pcb) {
        PAN_TRACE_ERROR("PAN Connection failed because of no resources");
        return PAN_NO_RESOURCES;
    }

    BTM_SetOutService(rem_bda, BTM_SEC_SERVICE_BNEP_PANU, mx_chan_id);
    PAN_TRACE_API("PAN_Connect() for BD Addr %x.%x.%x.%x.%x.%x",
                  rem_bda[0], rem_bda[1], rem_bda[2], rem_bda[3], rem_bda[4], rem_bda[5]);

    if(pcb->con_state == PAN_STATE_IDLE) {
        pan_cb.num_conns++;
    } else if(pcb->con_state == PAN_STATE_CONNECTED) {
        pcb->con_flags |= PAN_FLAGS_CONN_COMPLETED;
    } else
        /* PAN connection is still in progress */
    {
        return PAN_WRONG_STATE;
    }

    pcb->con_state = PAN_STATE_CONN_START;
    pcb->prv_src_uuid = pcb->src_uuid;
    pcb->prv_dst_uuid = pcb->dst_uuid;
    pcb->src_uuid     = src_uuid.uu.uuid16;
    pcb->dst_uuid     = dst_uuid.uu.uuid16;
    src_uuid.len      = 2;
    dst_uuid.len      = 2;
    result = BNEP_Connect(rem_bda, &src_uuid, &dst_uuid, &(pcb->handle));

    if(result != BNEP_SUCCESS) {
        pan_release_pcb(pcb);
        return result;
    }

    PAN_TRACE_DEBUG("PAN_Connect() current active role set to %d", src_role);
    pan_cb.prv_active_role = pan_cb.active_role;
    pan_cb.active_role = src_role;
    *handle = pcb->handle;
    return PAN_SUCCESS;
}




/*******************************************************************************
**
** Function         PAN_Disconnect
**
** Description      This is used to disconnect the connection
**
** Parameters:      handle           - handle for the connection
**
** Returns          PAN_SUCCESS      - if the connection is closed successfully
**                  PAN_FAILURE      - if the connection is not found or
**                                           there is an error in disconnecting
**
*******************************************************************************/
tPAN_RESULT PAN_Disconnect(uint16_t handle)
{
    tPAN_CONN       *pcb;
    tBNEP_RESULT    result;
    /* Check if the connection exists */
    pcb = pan_get_pcb_by_handle(handle);

    if(!pcb) {
        PAN_TRACE_ERROR("PAN connection not found for the handle %d", handle);
        return PAN_FAILURE;
    }

    result = BNEP_Disconnect(pcb->handle);

    if(pcb->con_state != PAN_STATE_IDLE) {
        pan_cb.num_conns--;
    }

    if(pan_cb.pan_bridge_req_cb && pcb->src_uuid == UUID_SERVCLASS_NAP) {
        (*pan_cb.pan_bridge_req_cb)(pcb->rem_bda, FALSE);
    }

    pan_release_pcb(pcb);

    if(result != BNEP_SUCCESS) {
        PAN_TRACE_EVENT("Error in closing PAN connection");
        return PAN_FAILURE;
    }

    PAN_TRACE_EVENT("PAN connection closed");
    return PAN_SUCCESS;
}


/*******************************************************************************
**
** Function         PAN_Write
**
** Description      This sends data over the PAN connections. If this is called
**                  on GN or NAP side and the packet is multicast or broadcast
**                  it will be sent on all the links. Otherwise the correct link
**                  is found based on the destination address and forwarded on it.
**
** Parameters:      handle   - handle for the connection
**                  dst      - MAC or BD Addr of the destination device
**                  src      - MAC or BD Addr of the source who sent this packet
**                  protocol - protocol of the ethernet packet like IP or ARP
**                  p_data   - pointer to the data
**                  len      - length of the data
**                  ext      - to indicate that extension headers present
**
** Returns          PAN_SUCCESS       - if the data is sent successfully
**                  PAN_FAILURE       - if the connection is not found or
**                                           there is an error in sending data
**
*******************************************************************************/
tPAN_RESULT PAN_Write(uint16_t handle, BD_ADDR dst, BD_ADDR src, uint16_t protocol, uint8_t *p_data,
                      uint16_t len, uint8_t ext)
{
    if(pan_cb.role == PAN_ROLE_INACTIVE || !pan_cb.num_conns) {
        PAN_TRACE_ERROR("%s PAN is not active, data write failed.", __func__);
        return PAN_FAILURE;
    }

    // If the packet is broadcast or multicast, we're going to have to create
    // a copy of the packet for each connection. We can save one extra copy
    // by fast-pathing here and calling BNEP_Write instead of placing the packet
    // in a BT_HDR buffer, calling BNEP_Write, and then freeing the buffer.
    if(dst[0] & 0x01) {
        int i;

        for(i = 0; i < MAX_PAN_CONNS; ++i) {
            if(pan_cb.pcb[i].con_state == PAN_STATE_CONNECTED) {
                BNEP_Write(pan_cb.pcb[i].handle, dst, p_data, len, protocol, src, ext);
            }
        }

        return PAN_SUCCESS;
    }

    BT_HDR *buffer = (BT_HDR *)GKI_getbuf(PAN_BUF_SIZE);
    buffer->len = len;
    buffer->offset = PAN_MINIMUM_OFFSET;
    wm_memcpy((uint8_t *)buffer + sizeof(BT_HDR) + buffer->offset, p_data, buffer->len);
    return PAN_WriteBuf(handle, dst, src, protocol, buffer, ext);
}


/*******************************************************************************
**
** Function         PAN_WriteBuf
**
** Description      This sends data over the PAN connections. If this is called
**                  on GN or NAP side and the packet is multicast or broadcast
**                  it will be sent on all the links. Otherwise the correct link
**                  is found based on the destination address and forwarded on it
**                  If the return value is not PAN_SUCCESS the application should
**                  take care of releasing the message buffer
**
** Parameters:      handle   - handle for the connection
**                  dst      - MAC or BD Addr of the destination device
**                  src      - MAC or BD Addr of the source who sent this packet
**                  protocol - protocol of the ethernet packet like IP or ARP
**                  p_buf    - pointer to the data buffer
**                  ext      - to indicate that extension headers present
**
** Returns          PAN_SUCCESS       - if the data is sent successfully
**                  PAN_FAILURE       - if the connection is not found or
**                                           there is an error in sending data
**
*******************************************************************************/
tPAN_RESULT PAN_WriteBuf(uint16_t handle, BD_ADDR dst, BD_ADDR src, uint16_t protocol,
                         BT_HDR *p_buf, uint8_t ext)
{
    tPAN_CONN       *pcb;
    uint16_t          i;
    tBNEP_RESULT    result;

    if(pan_cb.role == PAN_ROLE_INACTIVE || (!(pan_cb.num_conns))) {
        PAN_TRACE_ERROR("PAN is not active Data write failed");
        GKI_freebuf(p_buf);
        return PAN_FAILURE;
    }

    /* Check if it is broadcast or multicast packet */
    if(dst[0] & 0x01) {
        uint8_t *data = (uint8_t *)p_buf + sizeof(BT_HDR) + p_buf->offset;

        for(i = 0; i < MAX_PAN_CONNS; ++i) {
            if(pan_cb.pcb[i].con_state == PAN_STATE_CONNECTED) {
                BNEP_Write(pan_cb.pcb[i].handle, dst, data, p_buf->len, protocol, src, ext);
            }
        }

        GKI_freebuf(p_buf);
        return PAN_SUCCESS;
    }

    /* Check if the data write is on PANU side */
    if(pan_cb.active_role == PAN_ROLE_CLIENT) {
        /* Data write is on PANU connection */
        for(i = 0; i < MAX_PAN_CONNS; i++) {
            if(pan_cb.pcb[i].con_state == PAN_STATE_CONNECTED &&
                    pan_cb.pcb[i].src_uuid == UUID_SERVCLASS_PANU) {
                break;
            }
        }

        if(i == MAX_PAN_CONNS) {
            PAN_TRACE_ERROR("PAN Don't have any user connections");
            GKI_freebuf(p_buf);
            return PAN_FAILURE;
        }

        result = BNEP_WriteBuf(pan_cb.pcb[i].handle, dst, p_buf, protocol, src, ext);

        if(result == BNEP_IGNORE_CMD) {
            PAN_TRACE_DEBUG("PAN ignored data write for PANU connection");
            return result;
        } else if(result != BNEP_SUCCESS) {
            PAN_TRACE_ERROR("PAN failed to write data for the PANU connection");
            return result;
        }

        PAN_TRACE_DEBUG("PAN successfully wrote data for the PANU connection");
        return PAN_SUCCESS;
    }

    /* findout to which connection the data is meant for */
    pcb = pan_get_pcb_by_handle(handle);

    if(!pcb) {
        PAN_TRACE_ERROR("PAN Buf write for wrong handle");
        GKI_freebuf(p_buf);
        return PAN_FAILURE;
    }

    if(pcb->con_state != PAN_STATE_CONNECTED) {
        PAN_TRACE_ERROR("PAN Buf write when conn is not active");
        GKI_freebuf(p_buf);
        return PAN_FAILURE;
    }

    result = BNEP_WriteBuf(pcb->handle, dst, p_buf, protocol, src, ext);

    if(result == BNEP_IGNORE_CMD) {
        PAN_TRACE_DEBUG("PAN ignored data buf write to PANU");
        return result;
    } else if(result != BNEP_SUCCESS) {
        PAN_TRACE_ERROR("PAN failed to send data buf to the PANU");
        return result;
    }

    PAN_TRACE_DEBUG("PAN successfully sent data buf to the PANU");
    return PAN_SUCCESS;
}


/*******************************************************************************
**
** Function         PAN_SetProtocolFilters
**
** Description      This function is used to set protocol filters on the peer
**
** Parameters:      handle      - handle for the connection
**                  num_filters - number of protocol filter ranges
**                  start       - array of starting protocol numbers
**                  end         - array of ending protocol numbers
**
**
** Returns          PAN_SUCCESS        if protocol filters are set successfully
**                  PAN_FAILURE        if connection not found or error in setting
**
*******************************************************************************/
tPAN_RESULT PAN_SetProtocolFilters(uint16_t handle,
                                   uint16_t num_filters,
                                   uint16_t *p_start_array,
                                   uint16_t *p_end_array)
{
    tPAN_CONN       *pcb;
    tPAN_RESULT     result;
    /* Check if the connection exists */
    pcb = pan_get_pcb_by_handle(handle);

    if(!pcb) {
        PAN_TRACE_ERROR("PAN connection not found for the handle %d", handle);
        return PAN_FAILURE;
    }

    result = BNEP_SetProtocolFilters(pcb->handle, num_filters, p_start_array, p_end_array);

    if(result != BNEP_SUCCESS) {
        PAN_TRACE_ERROR("PAN failed to set protocol filters for handle %d", handle);
        return result;
    }

    PAN_TRACE_API("PAN successfully sent protocol filters for handle %d", handle);
    return PAN_SUCCESS;
}



/*******************************************************************************
**
** Function         PAN_SetMulticastFilters
**
** Description      This function is used to set multicast filters on the peer
**
** Parameters:      handle      - handle for the connection
**                  num_filters - number of multicast filter ranges
**                  start       - array of starting multicast filter addresses
**                  end         - array of ending multicast filter addresses
**
**
** Returns          PAN_SUCCESS        if multicast filters are set successfully
**                  PAN_FAILURE        if connection not found or error in setting
**
*******************************************************************************/
tBNEP_RESULT PAN_SetMulticastFilters(uint16_t handle,
                                     uint16_t num_mcast_filters,
                                     uint8_t *p_start_array,
                                     uint8_t *p_end_array)
{
    tPAN_CONN       *pcb;
    tPAN_RESULT     result;
    /* Check if the connection exists */
    pcb = pan_get_pcb_by_handle(handle);

    if(!pcb) {
        PAN_TRACE_ERROR("PAN connection not found for the handle %d", handle);
        return PAN_FAILURE;
    }

    result = BNEP_SetMulticastFilters(pcb->handle,
                                      num_mcast_filters, p_start_array, p_end_array);

    if(result != BNEP_SUCCESS) {
        PAN_TRACE_ERROR("PAN failed to set multicast filters for handle %d", handle);
        return result;
    }

    PAN_TRACE_API("PAN successfully sent multicast filters for handle %d", handle);
    return PAN_SUCCESS;
}


/*******************************************************************************
**
** Function         PAN_SetTraceLevel
**
** Description      This function sets the trace level for PAN. If called with
**                  a value of 0xFF, it simply reads the current trace level.
**
** Returns          the new (current) trace level
**
*******************************************************************************/
uint8_t PAN_SetTraceLevel(uint8_t new_level)
{
    if(new_level != 0xFF) {
        pan_cb.trace_level = new_level;
    } else {
        pan_dump_status();
    }

    return (pan_cb.trace_level);
}

/*******************************************************************************
**
** Function         PAN_Init
**
** Description      This function initializes the PAN module variables
**
** Parameters:      none
**
** Returns          none
**
*******************************************************************************/
void PAN_Init(void)
{
    wm_memset(&pan_cb, 0, sizeof(tPAN_CB));
#if defined(PAN_INITIAL_TRACE_LEVEL)
    pan_cb.trace_level = PAN_INITIAL_TRACE_LEVEL;
#else
    pan_cb.trace_level = BT_TRACE_LEVEL_NONE;    /* No traces */
#endif
}
#endif

