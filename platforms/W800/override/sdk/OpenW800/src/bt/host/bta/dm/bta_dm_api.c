/******************************************************************************
 *
 *  Copyright (C) 2003-2014 Broadcom Corporation
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
#include "bta_sys_int.h"
#include "btm_api.h"
#include "btm_int.h"
#include <string.h>
#include "utl.h"
#include "btm_ble_api.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

static const tBTA_SYS_REG bta_dm_reg = {
    bta_dm_sm_execute,
    bta_dm_sm_disable
};

static const tBTA_SYS_REG bta_dm_search_reg = {
    bta_dm_search_sm_execute,
    bta_dm_search_sm_disable
};

extern size_t strlcpy(char *dst, const char *src, size_t siz);
/*******************************************************************************
**
** Function         BTA_EnableBluetooth
**
** Description      Enables bluetooth service.  This function must be
**                  called before any other functions in the BTA API are called.
**
**
** Returns          tBTA_STATUS
**
*******************************************************************************/
tBTA_STATUS BTA_EnableBluetooth(tBTA_DM_SEC_CBACK *p_cback)
{
    /* Bluetooth disabling is in progress */
    if(bta_dm_cb.disabling) {
        return BTA_FAILURE;
    }

    bta_dm_init_cb();
    bta_sys_register(BTA_ID_DM, &bta_dm_reg);
    bta_sys_register(BTA_ID_DM_SEARCH, &bta_dm_search_reg);
    /* if UUID list is not provided as static data */
    bta_sys_eir_register(bta_dm_eir_update_uuid);
    tBTA_DM_API_ENABLE *p_msg =
                    (tBTA_DM_API_ENABLE *)GKI_getbuf(sizeof(tBTA_DM_API_ENABLE));
    p_msg->hdr.event = BTA_DM_API_ENABLE_EVT;
    p_msg->p_sec_cback = p_cback;
    bta_sys_sendmsg(p_msg);
    return BTA_SUCCESS;
}

/*******************************************************************************
**
** Function         BTA_DisableBluetooth
**
** Description      Disables bluetooth service.  This function is called when
**                  the application no longer needs bluetooth service
**
** Returns          void
**
*******************************************************************************/
tBTA_STATUS BTA_DisableBluetooth(void)
{
    BT_HDR *p_msg = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    p_msg->event = BTA_DM_API_DISABLE_EVT;
    bta_sys_sendmsg(p_msg);
    return BTA_SUCCESS;
}

/*******************************************************************************
**
** Function         BTA_EnableTestMode
**
** Description      Enables bluetooth device under test mode
**
**
** Returns          tBTA_STATUS
**
*******************************************************************************/
tBTA_STATUS BTA_EnableTestMode(void)
{
    BT_HDR *p_msg = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    APPL_TRACE_API("%s", __func__);
    p_msg->event = BTA_DM_API_ENABLE_TEST_MODE_EVT;
    bta_sys_sendmsg(p_msg);
    return BTA_SUCCESS;
}

/*******************************************************************************
**
** Function         BTA_DisableTestMode
**
** Description      Disable bluetooth device under test mode
**
**
** Returns          None
**
*******************************************************************************/
void BTA_DisableTestMode(void)
{
    BT_HDR *p_msg = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    APPL_TRACE_API("%s", __func__);
    p_msg->event = BTA_DM_API_DISABLE_TEST_MODE_EVT;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmSetDeviceName
**
** Description      This function sets the Bluetooth name of local device
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmSetDeviceName(char *p_name)
{
    tBTA_DM_API_SET_NAME *p_msg =
                    (tBTA_DM_API_SET_NAME *)GKI_getbuf(sizeof(tBTA_DM_API_SET_NAME));
    p_msg->hdr.event = BTA_DM_API_SET_NAME_EVT;
    strlcpy((char *)p_msg->name, p_name, BD_NAME_LEN);
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmSetVisibility
**
** Description      This function sets the Bluetooth connectable,
**                  discoverable, pairable and conn paired only modes of local device
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmSetVisibility(tBTA_DM_DISC disc_mode, tBTA_DM_CONN conn_mode, uint8_t pairable_mode,
                         uint8_t conn_filter)
{
    tBTA_DM_API_SET_VISIBILITY *p_msg =
                    (tBTA_DM_API_SET_VISIBILITY *)GKI_getbuf(sizeof(tBTA_DM_MSG));
    p_msg->hdr.event = BTA_DM_API_SET_VISIBILITY_EVT;
    p_msg->disc_mode = disc_mode;
    p_msg->conn_mode = conn_mode;
    p_msg->pair_mode = pairable_mode;
    p_msg->conn_paired_only = conn_filter;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmSearch
**
** Description      This function searches for peer Bluetooth devices. It performs
**                  an inquiry and gets the remote name for devices. Service
**                  discovery is done if services is non zero
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmSearch(tBTA_DM_INQ *p_dm_inq, tBTA_SERVICE_MASK services, tBTA_DM_SEARCH_CBACK *p_cback)
{
    tBTA_DM_API_SEARCH *p_msg =
                    (tBTA_DM_API_SEARCH *)GKI_getbuf(sizeof(tBTA_DM_API_SEARCH));
    p_msg->hdr.event = BTA_DM_API_SEARCH_EVT;
    wm_memcpy(&p_msg->inq_params, p_dm_inq, sizeof(tBTA_DM_INQ));
    p_msg->services = services;
    p_msg->p_cback = p_cback;
    p_msg->rs_res  = BTA_DM_RS_NONE;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmSearchCancel
**
** Description      This function  cancels a search initiated by BTA_DmSearch
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmSearchCancel(void)
{
    BT_HDR *p_msg = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR));
    p_msg->event = BTA_DM_API_SEARCH_CANCEL_EVT;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmDiscover
**
** Description      This function does service discovery for services of a
**                  peer device
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmDiscover(BD_ADDR bd_addr, tBTA_SERVICE_MASK services,
                    tBTA_DM_SEARCH_CBACK *p_cback, uint8_t sdp_search)
{
    tBTA_DM_API_DISCOVER *p_msg =
                    (tBTA_DM_API_DISCOVER *)GKI_getbuf(sizeof(tBTA_DM_API_DISCOVER));
    p_msg->hdr.event = BTA_DM_API_DISCOVER_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->services = services;
    p_msg->p_cback = p_cback;
    p_msg->sdp_search = sdp_search;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmDiscoverUUID
**
** Description      This function does service discovery for services of a
**                  peer device
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmDiscoverUUID(BD_ADDR bd_addr, tSDP_UUID *uuid,
                        tBTA_DM_SEARCH_CBACK *p_cback, uint8_t sdp_search)
{
    tBTA_DM_API_DISCOVER *p_msg =
                    (tBTA_DM_API_DISCOVER *)GKI_getbuf(sizeof(tBTA_DM_API_DISCOVER));
    p_msg->hdr.event = BTA_DM_API_DISCOVER_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->services = BTA_USER_SERVICE_MASK; //Not exposed at API level
    p_msg->p_cback = p_cback;
    p_msg->sdp_search = sdp_search;
#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
    p_msg->num_uuid = 0;
    p_msg->p_uuid = NULL;
#endif
    wm_memcpy(&p_msg->uuid, uuid, sizeof(tSDP_UUID));
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmBond
**
** Description      This function initiates a bonding procedure with a peer
**                  device
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmBond(BD_ADDR bd_addr)
{
    tBTA_DM_API_BOND *p_msg =
                    (tBTA_DM_API_BOND *)GKI_getbuf(sizeof(tBTA_DM_API_BOND));
    p_msg->hdr.event = BTA_DM_API_BOND_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->transport = BTA_TRANSPORT_UNKNOWN;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmBondByTransports
**
** Description      This function initiates a bonding procedure with a peer
**                  device
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmBondByTransport(BD_ADDR bd_addr, tBTA_TRANSPORT transport)
{
    tBTA_DM_API_BOND *p_msg =
                    (tBTA_DM_API_BOND *)GKI_getbuf(sizeof(tBTA_DM_API_BOND));
    p_msg->hdr.event = BTA_DM_API_BOND_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->transport = transport;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmBondCancel
**
** Description      This function cancels the bonding procedure with a peer
**                  device
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmBondCancel(BD_ADDR bd_addr)
{
    tBTA_DM_API_BOND_CANCEL *p_msg =
                    (tBTA_DM_API_BOND_CANCEL *)GKI_getbuf(sizeof(tBTA_DM_API_BOND_CANCEL));
    p_msg->hdr.event = BTA_DM_API_BOND_CANCEL_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmPinReply
**
** Description      This function provides a pincode for a remote device when
**                  one is requested by DM through BTA_DM_PIN_REQ_EVT
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmPinReply(BD_ADDR bd_addr, uint8_t accept, uint8_t pin_len, uint8_t *p_pin)

{
    tBTA_DM_API_PIN_REPLY *p_msg =
                    (tBTA_DM_API_PIN_REPLY *)GKI_getbuf(sizeof(tBTA_DM_API_PIN_REPLY));
    p_msg->hdr.event = BTA_DM_API_PIN_REPLY_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->accept = accept;

    if(accept) {
        p_msg->pin_len = pin_len;
        wm_memcpy(p_msg->p_pin, p_pin, pin_len);
    }

    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmLocalOob
**
** Description      This function retrieves the OOB data from local controller.
**                  The result is reported by:
**                  - bta_dm_co_loc_oob_ext() if device supports secure
**                    connections (SC)
**                  - bta_dm_co_loc_oob() if device doesn't support SC
**
** Returns          void
**
*******************************************************************************/
void BTA_DmLocalOob(void)
{
    tBTA_DM_API_LOC_OOB *p_msg =
                    (tBTA_DM_API_LOC_OOB *)GKI_getbuf(sizeof(tBTA_DM_API_LOC_OOB));
    p_msg->hdr.event = BTA_DM_API_LOC_OOB_EVT;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmConfirm
**
** Description      This function accepts or rejects the numerical value of the
**                  Simple Pairing process on BTA_DM_SP_CFM_REQ_EVT
**
** Returns          void
**
*******************************************************************************/
void BTA_DmConfirm(BD_ADDR bd_addr, uint8_t accept)
{
    tBTA_DM_API_CONFIRM *p_msg =
                    (tBTA_DM_API_CONFIRM *)GKI_getbuf(sizeof(tBTA_DM_API_CONFIRM));
    p_msg->hdr.event = BTA_DM_API_CONFIRM_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->accept = accept;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmAddDevice
**
** Description      This function adds a device to the security database list of
**                  peer device
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmAddDevice(BD_ADDR bd_addr, DEV_CLASS dev_class, LINK_KEY link_key,
                     tBTA_SERVICE_MASK trusted_mask, uint8_t is_trusted,
                     uint8_t key_type, tBTA_IO_CAP io_cap, uint8_t pin_length)
{
    tBTA_DM_API_ADD_DEVICE *p_msg =
                    (tBTA_DM_API_ADD_DEVICE *)GKI_getbuf(sizeof(tBTA_DM_API_ADD_DEVICE));
    p_msg->hdr.event = BTA_DM_API_ADD_DEVICE_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->tm = trusted_mask;
    p_msg->is_trusted = is_trusted;
    p_msg->io_cap = io_cap;

    if(link_key) {
        p_msg->link_key_known = TRUE;
        p_msg->key_type = key_type;
        wm_memcpy(p_msg->link_key, link_key, LINK_KEY_LEN);
    }

    /* Load device class if specified */
    if(dev_class) {
        p_msg->dc_known = TRUE;
        wm_memcpy(p_msg->dc, dev_class, DEV_CLASS_LEN);
    }

    wm_memset(p_msg->bd_name, 0, BD_NAME_LEN + 1);
    wm_memset(p_msg->features, 0, sizeof(p_msg->features));
    p_msg->pin_length = pin_length;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmRemoveDevice
**
** Description      This function removes a device fromthe security database list of
**                  peer device. It manages unpairing even while connected.
**
**
** Returns          void
**
*******************************************************************************/
tBTA_STATUS BTA_DmRemoveDevice(BD_ADDR bd_addr)
{
    tBTA_DM_API_REMOVE_DEVICE *p_msg =
                    (tBTA_DM_API_REMOVE_DEVICE *)GKI_getbuf(sizeof(tBTA_DM_API_REMOVE_DEVICE));
    p_msg->hdr.event = BTA_DM_API_REMOVE_DEVICE_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    bta_sys_sendmsg(p_msg);
    return BTA_SUCCESS;
}

/*******************************************************************************
**
** Function         BTA_GetEirService
**
** Description      This function is called to get BTA service mask from EIR.
**
** Parameters       p_eir - pointer of EIR significant part
**                  p_services - return the BTA service mask
**
** Returns          None
**
*******************************************************************************/
extern const uint16_t bta_service_id_to_uuid_lkup_tbl [];
void BTA_GetEirService(uint8_t *p_eir, tBTA_SERVICE_MASK *p_services)
{
    uint8_t xx, yy;
    uint8_t num_uuid, max_num_uuid = 32;
    uint8_t uuid_list[32 * LEN_UUID_16];
    uint16_t *p_uuid16 = (uint16_t *)uuid_list;
    tBTA_SERVICE_MASK mask;
    BTM_GetEirUuidList(p_eir, LEN_UUID_16, &num_uuid, uuid_list, max_num_uuid);

    for(xx = 0; xx < num_uuid; xx++) {
        mask = 1;

        for(yy = 0; yy < BTA_MAX_SERVICE_ID; yy++) {
            if(*(p_uuid16 + xx) == bta_service_id_to_uuid_lkup_tbl[yy]) {
                *p_services |= mask;
                break;
            }

            mask <<= 1;
        }

        /* for HSP v1.2 only device */
        if(*(p_uuid16 + xx) == UUID_SERVCLASS_HEADSET_HS) {
            *p_services |= BTA_HSP_SERVICE_MASK;
        }

        if(*(p_uuid16 + xx) == UUID_SERVCLASS_HDP_SOURCE) {
            *p_services |= BTA_HL_SERVICE_MASK;
        }

        if(*(p_uuid16 + xx) == UUID_SERVCLASS_HDP_SINK) {
            *p_services |= BTA_HL_SERVICE_MASK;
        }
    }
}

/*******************************************************************************
**
** Function         BTA_DmGetConnectionState
**
** Description      Returns whether the remote device is currently connected.
**
** Returns          0 if the device is NOT connected.
**
*******************************************************************************/
uint16_t BTA_DmGetConnectionState(BD_ADDR bd_addr)
{
    tBTA_DM_PEER_DEVICE *p_dev = bta_dm_find_peer_device(bd_addr);
    return (p_dev && p_dev->conn_state == BTA_DM_CONNECTED);
}


/*******************************************************************************
**                   Device Identification (DI) Server Functions
*******************************************************************************/
/*******************************************************************************
**
** Function         BTA_DmSetLocalDiRecord
**
** Description      This function adds a DI record to the local SDP database.
**
** Returns          BTA_SUCCESS if record set sucessfully, otherwise error code.
**
*******************************************************************************/
tBTA_STATUS BTA_DmSetLocalDiRecord(tBTA_DI_RECORD *p_device_info,
                                   uint32_t *p_handle)
{
    tBTA_STATUS  status = BTA_FAILURE;

    if(bta_dm_di_cb.di_num < BTA_DI_NUM_MAX) {
        if(SDP_SetLocalDiRecord((tSDP_DI_RECORD *)p_device_info, p_handle) == SDP_SUCCESS) {
            if(!p_device_info->primary_record) {
                bta_dm_di_cb.di_handle[bta_dm_di_cb.di_num] = *p_handle;
                bta_dm_di_cb.di_num ++;
            }

            bta_sys_add_uuid(UUID_SERVCLASS_PNP_INFORMATION);
            status =  BTA_SUCCESS;
        }
    }

    return status;
}

/*******************************************************************************
**
** Function         bta_dmexecutecallback
**
** Description      This function will request BTA to execute a call back in the context of BTU task
**                  This API was named in lower case because it is only intended
**                  for the internal customers(like BTIF).
**
** Returns          void
**
*******************************************************************************/
void bta_dmexecutecallback(tBTA_DM_EXEC_CBACK *p_callback, void *p_param)
{
    tBTA_DM_API_EXECUTE_CBACK *p_msg =
                    (tBTA_DM_API_EXECUTE_CBACK *)GKI_getbuf(sizeof(tBTA_DM_MSG));
    p_msg->hdr.event = BTA_DM_API_EXECUTE_CBACK_EVT;
    p_msg->p_param = p_param;
    p_msg->p_exec_cback = p_callback;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmAddBleKey
**
** Description      Add/modify LE device information.  This function will be
**                  normally called during host startup to restore all required
**                  information stored in the NVRAM.
**
** Parameters:      bd_addr          - BD address of the peer
**                  p_le_key         - LE key values.
**                  key_type         - LE SMP key type.
**
** Returns          BTA_SUCCESS if successful
**                  BTA_FAIL if operation failed.
**
*******************************************************************************/
void BTA_DmAddBleKey(BD_ADDR bd_addr, tBTA_LE_KEY_VALUE *p_le_key, tBTA_LE_KEY_TYPE key_type)
{
#if BLE_INCLUDED == TRUE
    tBTA_DM_API_ADD_BLEKEY *p_msg =
                    (tBTA_DM_API_ADD_BLEKEY *)GKI_getbuf(sizeof(tBTA_DM_API_ADD_BLEKEY));
    p_msg->hdr.event = BTA_DM_API_ADD_BLEKEY_EVT;
    p_msg->key_type = key_type;
    bdcpy(p_msg->bd_addr, bd_addr);
    wm_memcpy(&p_msg->blekey, p_le_key, sizeof(tBTA_LE_KEY_VALUE));
    bta_sys_sendmsg(p_msg);
#endif
}

/*******************************************************************************
**
** Function         BTA_DmAddBleDevice
**
** Description      Add a BLE device.  This function will be normally called
**                  during host startup to restore all required information
**                  for a LE device stored in the NVRAM.
**
** Parameters:      bd_addr          - BD address of the peer
**                  dev_type         - Remote device's device type.
**                  addr_type        - LE device address type.
**
** Returns          void
**
*******************************************************************************/
void BTA_DmAddBleDevice(BD_ADDR bd_addr, tBLE_ADDR_TYPE addr_type, tBT_DEVICE_TYPE dev_type)
{
#if BLE_INCLUDED == TRUE
    tBTA_DM_API_ADD_BLE_DEVICE *p_msg =
                    (tBTA_DM_API_ADD_BLE_DEVICE *)GKI_getbuf(sizeof(tBTA_DM_API_ADD_BLE_DEVICE));
    p_msg->hdr.event = BTA_DM_API_ADD_BLEDEVICE_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->addr_type = addr_type;
    p_msg->dev_type = dev_type;
    bta_sys_sendmsg(p_msg);
#endif
}

/*******************************************************************************
**
** Function         BTA_DmBlePasskeyReply
**
** Description      Send BLE SMP passkey reply.
**
** Parameters:      bd_addr          - BD address of the peer
**                  accept           - passkey entry sucessful or declined.
**                  passkey          - passkey value, must be a 6 digit number,
**                                     can be lead by 0.
**
** Returns          void
**
*******************************************************************************/
void BTA_DmBlePasskeyReply(BD_ADDR bd_addr, uint8_t accept, uint32_t passkey)
{
#if BLE_INCLUDED == TRUE
    tBTA_DM_API_PASSKEY_REPLY *p_msg =
                    (tBTA_DM_API_PASSKEY_REPLY *)GKI_getbuf(sizeof(tBTA_DM_API_PASSKEY_REPLY));
    p_msg->hdr.event = BTA_DM_API_BLE_PASSKEY_REPLY_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->accept = accept;

    if(accept) {
        p_msg->passkey = passkey;
    }

    bta_sys_sendmsg(p_msg);
#endif
}

/*******************************************************************************
**
** Function         BTA_DmBleConfirmReply
**
** Description      Send BLE SMP SC user confirmation reply.
**
** Parameters:      bd_addr          - BD address of the peer
**                  accept           - numbers to compare are the same or different.
**
** Returns          void
**
*******************************************************************************/
void BTA_DmBleConfirmReply(BD_ADDR bd_addr, uint8_t accept)
{
#if BLE_INCLUDED == TRUE
    tBTA_DM_API_CONFIRM *p_msg =
                    (tBTA_DM_API_CONFIRM *)GKI_getbuf(sizeof(tBTA_DM_API_CONFIRM));
    p_msg->hdr.event = BTA_DM_API_BLE_CONFIRM_REPLY_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->accept = accept;
    bta_sys_sendmsg(p_msg);
#endif
}

/*******************************************************************************
**
** Function         BTA_DmBleSecurityGrant
**
** Description      Grant security request access.
**
** Parameters:      bd_addr          - BD address of the peer
**                  res              - security grant status.
**
** Returns          void
**
*******************************************************************************/
void BTA_DmBleSecurityGrant(BD_ADDR bd_addr, tBTA_DM_BLE_SEC_GRANT res)
{
#if BLE_INCLUDED == TRUE
    tBTA_DM_API_BLE_SEC_GRANT *p_msg =
                    (tBTA_DM_API_BLE_SEC_GRANT *)GKI_getbuf(sizeof(tBTA_DM_API_BLE_SEC_GRANT));
    p_msg->hdr.event = BTA_DM_API_BLE_SEC_GRANT_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->res = res;
    bta_sys_sendmsg(p_msg);
#endif
}

/*******************************************************************************
**
** Function         BTA_DmSetBlePrefConnParams
**
** Description      This function is called to set the preferred connection
**                  parameters when default connection parameter is not desired.
**
** Parameters:      bd_addr          - BD address of the peripheral
**                  scan_interval    - scan interval
**                  scan_window      - scan window
**                  min_conn_int     - minimum preferred connection interval
**                  max_conn_int     - maximum preferred connection interval
**                  slave_latency    - preferred slave latency
**                  supervision_tout - preferred supervision timeout
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmSetBlePrefConnParams(BD_ADDR bd_addr,
                                uint16_t min_conn_int, uint16_t max_conn_int,
                                uint16_t slave_latency, uint16_t supervision_tout)
{
#if BLE_INCLUDED == TRUE
    tBTA_DM_API_BLE_CONN_PARAMS *p_msg =
                    (tBTA_DM_API_BLE_CONN_PARAMS *)GKI_getbuf(sizeof(tBTA_DM_API_BLE_CONN_PARAMS));
    p_msg->hdr.event = BTA_DM_API_BLE_CONN_PARAM_EVT;
    wm_memcpy(p_msg->peer_bda, bd_addr, BD_ADDR_LEN);
    p_msg->conn_int_max = max_conn_int;
    p_msg->conn_int_min = min_conn_int;
    p_msg->slave_latency = slave_latency;
    p_msg->supervision_tout = supervision_tout;
    bta_sys_sendmsg(p_msg);
#endif
}

/*******************************************************************************
**
** Function         BTA_DmSetBleConnScanParams
**
** Description      This function is called to set scan parameters used in
**                  BLE connection request
**
** Parameters:      scan_interval    - scan interval
**                  scan_window      - scan window
**
** Returns          void
**
*******************************************************************************/
void BTA_DmSetBleConnScanParams(uint32_t scan_interval, uint32_t scan_window)
{
#if BLE_INCLUDED == TRUE
    tBTA_DM_API_BLE_SCAN_PARAMS *p_msg =
                    (tBTA_DM_API_BLE_SCAN_PARAMS *)GKI_getbuf(sizeof(tBTA_DM_API_BLE_SCAN_PARAMS));
    p_msg->hdr.event = BTA_DM_API_BLE_CONN_SCAN_PARAM_EVT;
    p_msg->scan_int = scan_interval;
    p_msg->scan_window = scan_window;
    bta_sys_sendmsg(p_msg);
#endif  // BLE_INCLUDED == TRUE
}

/*******************************************************************************
**
** Function         BTA_DmSetBleScanParams
**
** Description      This function is called to set scan parameters
**
** Parameters:      client_if - Client IF
**                  scan_interval - scan interval
**                  scan_window - scan window
**                  scan_mode - scan mode
**                  scan_param_setup_status_cback - Set scan param status callback
**
** Returns          void
**
*******************************************************************************/

#if BLE_INCLUDED == TRUE
void BTA_DmSetBleScanParams(tGATT_IF client_if, uint32_t scan_interval,
                            uint32_t scan_window, tBLE_SCAN_MODE scan_mode,
                            tBLE_SCAN_PARAM_SETUP_CBACK scan_param_setup_cback)
{
    tBTA_DM_API_BLE_SCAN_PARAMS *p_msg =
                    (tBTA_DM_API_BLE_SCAN_PARAMS *)GKI_getbuf(sizeof(tBTA_DM_API_BLE_SCAN_PARAMS));
    p_msg->hdr.event = BTA_DM_API_BLE_SCAN_PARAM_EVT;
    p_msg->client_if = client_if;
    p_msg->scan_int = scan_interval;
    p_msg->scan_window = scan_window;
    p_msg->scan_mode = scan_mode;
    p_msg->scan_param_setup_cback = scan_param_setup_cback;
    bta_sys_sendmsg(p_msg);
}
#endif  // BLE_INCLUDED == TRUE

#if BLE_INCLUDED == TRUE
uint8_t BTA_DmGetBleScanState()
{
    return BTM_IsBleScanActive();
}
#endif  // BLE_INCLUDED == TRUE

/*******************************************************************************
**
** Function         BTA_DmSetBleAdvParams
**
** Description      This function sets the advertising parameters BLE functionality.
**                  It is to be called when device act in peripheral or broadcaster
**                  role.
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmSetBleAdvParams(uint16_t adv_int_min, uint16_t adv_int_max,
                           tBLE_BD_ADDR *p_dir_bda)
{
#if BLE_INCLUDED == TRUE
    tBTA_DM_API_BLE_ADV_PARAMS *p_msg =
                    (tBTA_DM_API_BLE_ADV_PARAMS *)GKI_getbuf(sizeof(tBTA_DM_API_BLE_ADV_PARAMS));
    APPL_TRACE_API("BTA_DmSetBleAdvParam: %d, %d", adv_int_min, adv_int_max);
    p_msg->hdr.event = BTA_DM_API_BLE_ADV_PARAM_EVT;
    p_msg->adv_int_min = adv_int_min;
    p_msg->adv_int_max = adv_int_max;

    if(p_dir_bda != NULL) {
        p_msg->p_dir_bda = (tBLE_BD_ADDR *)(p_msg + 1);
        wm_memcpy(p_msg->p_dir_bda, p_dir_bda, sizeof(tBLE_BD_ADDR));
    }

    bta_sys_sendmsg(p_msg);
#endif
}
/*******************************************************************************
**
** Function         BTA_DmSetBleAdvExtParams
**
** Description      This function sets the advertising parameters BLE functionality.
**                  It is to be called when device act in peripheral or broadcaster
**                  role.
**
**
** Returns          void
**
*******************************************************************************/


void BTA_DmSetBleAdvExtParams(uint16_t adv_int_min, uint16_t adv_int_max, uint8_t adv_type,
                              uint8_t own_addr_type,
                              tBTM_BLE_ADV_CHNL_MAP chnl_map, uint8_t afp, uint8_t peer_addr_type, tBLE_BD_ADDR *p_dir_bda)
{
#if BLE_INCLUDED == TRUE
    tBTA_DM_API_BLE_ADV_EXT_PARAMS *p_msg =
                    (tBTA_DM_API_BLE_ADV_EXT_PARAMS *)GKI_getbuf(sizeof(tBTA_DM_API_BLE_ADV_EXT_PARAMS));
    APPL_TRACE_API("BTA_DmSetBleAdvExtParam: %d, %d", adv_int_min, adv_int_max);
    p_msg->hdr.event = BTA_DM_API_BLE_ADV_EXT_PARAM_EVT;
    p_msg->adv_int_min = adv_int_min;
    p_msg->adv_int_max = adv_int_max;
    p_msg->adv_type = adv_type;
    p_msg->afp = afp;
    p_msg->chnl_map = chnl_map;
    p_msg->own_addr_type = own_addr_type;
    p_msg->peer_addr_type = peer_addr_type;

    if(p_dir_bda != NULL) {
        p_msg->p_dir_bda = (tBLE_BD_ADDR *)(p_msg + 1);
        wm_memcpy(p_msg->p_dir_bda, p_dir_bda, sizeof(tBLE_BD_ADDR));
    }

    bta_sys_sendmsg(p_msg);
#endif
}


/*******************************************************************************
**                      BLE ADV data management API
********************************************************************************/

#if BLE_INCLUDED == TRUE

uint8_t BTA_DmGetBleAdvertiseState()
{
    return BTM_IsBleAdvertiseActive();
}

/*******************************************************************************
**
** Function         BTA_DmBleSetAdvConfig
**
** Description      This function is called to override the BTA default ADV parameters.
**
** Parameters       data_mask: adv data mask.
**                  p_adv_cfg: Pointer to User defined ADV data structure. This
**                             memory space can not be freed until p_adv_data_cback
**                             is received.
**                  p_adv_data_cback: set adv data complete callback.
**
** Returns          None
**
*******************************************************************************/
void BTA_DmBleSetAdvConfig(tBTA_BLE_AD_MASK data_mask, tBTA_BLE_ADV_DATA *p_adv_cfg,
                           tBTA_SET_ADV_DATA_CMPL_CBACK *p_adv_data_cback)
{
    tBTA_DM_API_SET_ADV_CONFIG *p_msg = GKI_getbuf(sizeof(*p_msg));
    p_msg->hdr.event = BTA_DM_API_BLE_SET_ADV_CONFIG_EVT;
    p_msg->data_mask = data_mask;
    p_msg->p_adv_data_cback = p_adv_data_cback;
    wm_memcpy(&p_msg->adv_cfg, p_adv_cfg, sizeof(p_msg->adv_cfg));
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmBleSetScanRsp
**
** Description      This function is called to override the BTA scan response.
**
** Parameters       Pointer to User defined ADV data structure
**
** Returns          None
**
*******************************************************************************/
extern void BTA_DmBleSetScanRsp(tBTA_BLE_AD_MASK data_mask, tBTA_BLE_ADV_DATA *p_adv_cfg,
                                tBTA_SET_ADV_DATA_CMPL_CBACK *p_adv_data_cback)
{
    tBTA_DM_API_SET_ADV_CONFIG *p_msg = GKI_getbuf(sizeof(*p_msg));
    p_msg->hdr.event = BTA_DM_API_BLE_SET_SCAN_RSP_EVT;
    p_msg->data_mask = data_mask;
    p_msg->p_adv_data_cback = p_adv_data_cback;
    wm_memcpy(&p_msg->adv_cfg, p_adv_cfg, sizeof(p_msg->adv_cfg));
    bta_sys_sendmsg(p_msg);
}
#if (BLE_VND_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         BTA_DmBleSetStorageParams
**
** Description      This function is called to override the BTA scan response.
**
** Parameters       batch_scan_full_max -Max storage space (in %) allocated to full scanning
**                  batch_scan_trunc_max -Max storage space (in %) allocated to truncated scanning
**                  batch_scan_notify_threshold -Setup notification level based on total space
**                  p_setup_cback - Setup callback pointer
**                  p_thres_cback - Threshold callback pointer
**                  p_rep_cback - Reports callback pointer
**                  ref_value - Ref value
**
** Returns          None
**
*******************************************************************************/
extern void BTA_DmBleSetStorageParams(uint8_t batch_scan_full_max,
                                      uint8_t batch_scan_trunc_max,
                                      uint8_t batch_scan_notify_threshold,
                                      tBTA_BLE_SCAN_SETUP_CBACK *p_setup_cback,
                                      tBTA_BLE_SCAN_THRESHOLD_CBACK *p_thres_cback,
                                      tBTA_BLE_SCAN_REP_CBACK *p_rep_cback,
                                      tBTA_DM_BLE_REF_VALUE ref_value)
{
    tBTA_DM_API_SET_STORAGE_CONFIG *p_msg =
                    (tBTA_DM_API_SET_STORAGE_CONFIG *)GKI_getbuf(sizeof(tBTA_DM_API_SET_STORAGE_CONFIG));
    bta_dm_cb.p_setup_cback = p_setup_cback;
    p_msg->hdr.event = BTA_DM_API_BLE_SETUP_STORAGE_EVT;
    p_msg->p_setup_cback = bta_ble_scan_setup_cb;
    p_msg->p_thres_cback = p_thres_cback;
    p_msg->p_read_rep_cback = p_rep_cback;
    p_msg->ref_value = ref_value;
    p_msg->batch_scan_full_max = batch_scan_full_max;
    p_msg->batch_scan_trunc_max = batch_scan_trunc_max;
    p_msg->batch_scan_notify_threshold = batch_scan_notify_threshold;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmBleEnableBatchScan
**
** Description      This function is called to enable the batch scan
**
** Parameters       scan_mode -Batch scan mode
**                  scan_interval - Scan interval
**                  scan_window - Scan window
**                  discard_rule -Discard rules
**                  addr_type - Address type
**                  ref_value - Reference value
**
** Returns          None
**
*******************************************************************************/
extern void BTA_DmBleEnableBatchScan(tBTA_BLE_BATCH_SCAN_MODE scan_mode,
                                     uint32_t scan_interval, uint32_t scan_window,
                                     tBTA_BLE_DISCARD_RULE discard_rule,
                                     tBLE_ADDR_TYPE        addr_type,
                                     tBTA_DM_BLE_REF_VALUE ref_value)
{
    tBTA_DM_API_ENABLE_SCAN *p_msg =
                    (tBTA_DM_API_ENABLE_SCAN *)GKI_getbuf(sizeof(tBTA_DM_API_ENABLE_SCAN));
    p_msg->hdr.event = BTA_DM_API_BLE_ENABLE_BATCH_SCAN_EVT;
    p_msg->scan_mode = scan_mode;
    p_msg->scan_int = scan_interval;
    p_msg->scan_window = scan_window;
    p_msg->discard_rule = discard_rule;
    p_msg->addr_type = addr_type;
    p_msg->ref_value = ref_value;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmBleDisableBatchScan
**
** Description      This function is called to disable the batch scan
**
** Parameters       ref_value - Reference value
**
** Returns          None
**
*******************************************************************************/
extern void BTA_DmBleDisableBatchScan(tBTA_DM_BLE_REF_VALUE ref_value)
{
    tBTA_DM_API_DISABLE_SCAN *p_msg =
                    (tBTA_DM_API_DISABLE_SCAN *)GKI_getbuf(sizeof(tBTA_DM_API_DISABLE_SCAN));
    p_msg->hdr.event = BTA_DM_API_BLE_DISABLE_BATCH_SCAN_EVT;
    p_msg->ref_value = ref_value;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmBleReadScanReports
**
** Description      This function is called to read scan reports
**
** Parameters       scan_type -Batch scan mode
**                  ref_value - Reference value
**
** Returns          None
**
*******************************************************************************/
extern void BTA_DmBleReadScanReports(tBTA_BLE_BATCH_SCAN_MODE scan_type,
                                     tBTA_DM_BLE_REF_VALUE ref_value)
{
    tBTA_DM_API_READ_SCAN_REPORTS *p_msg =
                    (tBTA_DM_API_READ_SCAN_REPORTS *)GKI_getbuf(sizeof(tBTA_DM_API_READ_SCAN_REPORTS));
    p_msg->hdr.event = BTA_DM_API_BLE_READ_SCAN_REPORTS_EVT;
    p_msg->scan_type = scan_type;
    p_msg->ref_value = ref_value;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmBleTrackAdvertiser
**
** Description      This function is called to track advertiser
**
** Parameters       ref_value - Reference value
**                  p_track_adv_cback - Track ADV callback
**
** Returns          None
**
*******************************************************************************/
extern void BTA_DmBleTrackAdvertiser(tBTA_DM_BLE_REF_VALUE ref_value,
                                     tBTA_BLE_TRACK_ADV_CBACK *p_track_adv_cback)
{
    tBTA_DM_API_TRACK_ADVERTISER *p_msg =
                    (tBTA_DM_API_TRACK_ADVERTISER *)GKI_getbuf(sizeof(tBTA_DM_API_TRACK_ADVERTISER));
    p_msg->hdr.event = BTA_DM_API_BLE_TRACK_ADVERTISER_EVT;
    p_msg->p_track_adv_cback = p_track_adv_cback;
    p_msg->ref_value = ref_value;
    bta_sys_sendmsg(p_msg);
}
#endif
#endif

/*******************************************************************************
**                      BLE ADV data management API
********************************************************************************/
#if BLE_INCLUDED == TRUE

/*******************************************************************************
**
** Function         BTA_DmBleBroadcast
**
** Description      This function starts or stops LE broadcasting.
**
** Parameters       start: start or stop broadcast.
**
** Returns          None
**
*******************************************************************************/
extern void BTA_DmBleBroadcast(uint8_t start, uint16_t duration, tBTA_BLE_MULTI_ADV_CBACK *p_cback)
{
    tBTA_DM_API_BLE_OBSERVE *p_msg =
                    (tBTA_DM_API_BLE_OBSERVE *)GKI_getbuf(sizeof(tBTA_DM_API_BLE_OBSERVE));
    APPL_TRACE_API("BTA_DmBleBroadcast: start = %d ", start);
    p_msg->hdr.event = BTA_DM_API_BLE_BROADCAST_EVT;
    p_msg->start = start;
    p_msg->duration = duration;
    p_msg->p_cback = p_cback;
    bta_sys_sendmsg(p_msg);
}

#endif
/*******************************************************************************
**
** Function         BTA_DmBleSetBgConnType
**
** Description      This function is called to set BLE connectable mode for a
**                  peripheral device.
**
** Parameters       bg_conn_type: it can be auto connection, or selective connection.
**                  p_select_cback: callback function when selective connection procedure
**                              is being used.
**
** Returns          void
**
*******************************************************************************/
void BTA_DmBleSetBgConnType(tBTA_DM_BLE_CONN_TYPE bg_conn_type,
                            tBTA_DM_BLE_SEL_CBACK *p_select_cback)
{
#if BLE_INCLUDED == TRUE
    tBTA_DM_API_BLE_SET_BG_CONN_TYPE *p_msg =
                    (tBTA_DM_API_BLE_SET_BG_CONN_TYPE *)GKI_getbuf(sizeof(tBTA_DM_API_BLE_SET_BG_CONN_TYPE));
    p_msg->hdr.event = BTA_DM_API_BLE_SET_BG_CONN_TYPE;
    p_msg->bg_conn_type = bg_conn_type;
    p_msg->p_select_cback = p_select_cback;
    bta_sys_sendmsg(p_msg);
#endif
}

/*******************************************************************************
**
** Function         bta_dm_discover_send_msg
**
** Description      This function send discover message to BTA task.
**
** Returns          void
**
*******************************************************************************/
#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
static void bta_dm_discover_send_msg(BD_ADDR bd_addr, tBTA_SERVICE_MASK_EXT *p_services,
                                     tBTA_DM_SEARCH_CBACK *p_cback, uint8_t sdp_search,
                                     tBTA_TRANSPORT transport)
{
    const size_t len = p_services ?
                       (sizeof(tBTA_DM_API_DISCOVER) + sizeof(tBT_UUID) * p_services->num_uuid)
                       : sizeof(tBTA_DM_API_DISCOVER);
    tBTA_DM_API_DISCOVER *p_msg = (tBTA_DM_API_DISCOVER *)GKI_getbuf(len);
    p_msg->hdr.event = BTA_DM_API_DISCOVER_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->p_cback = p_cback;
    p_msg->sdp_search = sdp_search;
    p_msg->transport    = transport;

    if(p_services != NULL) {
#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
        p_msg->services = p_services->srvc_mask;
        p_msg->num_uuid = p_services->num_uuid;

        if(p_services->num_uuid != 0) {
            p_msg->p_uuid = (tBT_UUID *)(p_msg + 1);
            wm_memcpy(p_msg->p_uuid, p_services->p_uuid,
                      sizeof(tBT_UUID) * p_services->num_uuid);
        }

#endif
    }

    bta_sys_sendmsg(p_msg);
}
#endif

/*******************************************************************************
**
** Function         BTA_DmDiscoverByTransport
**
** Description      This function does service discovery on particular transport
**                  for services of a
**                  peer device. When services.num_uuid is 0, it indicates all
**                  GATT based services are to be searched; otherwise a list of
**                  UUID of interested services should be provided through
**                  p_services->p_uuid.
**
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmDiscoverByTransport(BD_ADDR bd_addr, tBTA_SERVICE_MASK_EXT *p_services,
                               tBTA_DM_SEARCH_CBACK *p_cback, uint8_t sdp_search,
                               tBTA_TRANSPORT transport)
{
#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
    bta_dm_discover_send_msg(bd_addr, p_services, p_cback, sdp_search, transport);
#endif
}


/*******************************************************************************
**
** Function         BTA_DmDiscoverExt
**
** Description      This function does service discovery for services of a
**                  peer device. When services.num_uuid is 0, it indicates all
**                  GATT based services are to be searched; other wise a list of
**                  UUID of interested services should be provided through
**                  p_services->p_uuid.
**
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmDiscoverExt(BD_ADDR bd_addr, tBTA_SERVICE_MASK_EXT *p_services,
                       tBTA_DM_SEARCH_CBACK *p_cback, uint8_t sdp_search)
{
#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
    bta_dm_discover_send_msg(bd_addr, p_services, p_cback, sdp_search, BTA_TRANSPORT_UNKNOWN);
#endif
}

/*******************************************************************************
**
** Function         BTA_DmSearchExt
**
** Description      This function searches for peer Bluetooth devices. It performs
**                  an inquiry and gets the remote name for devices. Service
**                  discovery is done if services is non zero
**
** Parameters       p_dm_inq: inquiry conditions
**                  p_services: if service is not empty, service discovery will be done.
**                            for all GATT based service condition, put num_uuid, and
**                            p_uuid is the pointer to the list of UUID values.
**                  p_cback: callback functino when search is completed.
**
**
**
** Returns          void
**
*******************************************************************************/
void BTA_DmSearchExt(tBTA_DM_INQ *p_dm_inq, tBTA_SERVICE_MASK_EXT *p_services,
                     tBTA_DM_SEARCH_CBACK *p_cback)
{
#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
    const size_t len = p_services ?
                       (sizeof(tBTA_DM_API_SEARCH) + sizeof(tBT_UUID) * p_services->num_uuid)
                       : sizeof(tBTA_DM_API_SEARCH);
    tBTA_DM_API_SEARCH *p_msg = (tBTA_DM_API_SEARCH *)GKI_getbuf(len);
    p_msg->hdr.event = BTA_DM_API_SEARCH_EVT;
    wm_memcpy(&p_msg->inq_params, p_dm_inq, sizeof(tBTA_DM_INQ));
    p_msg->p_cback = p_cback;
    p_msg->rs_res  = BTA_DM_RS_NONE;

    if(p_services != NULL) {
        p_msg->services = p_services->srvc_mask;
        p_msg->num_uuid = p_services->num_uuid;

        if(p_services->num_uuid != 0) {
            p_msg->p_uuid = (tBT_UUID *)(p_msg + 1);
            wm_memcpy(p_msg->p_uuid, p_services->p_uuid,
                      sizeof(tBT_UUID) * p_services->num_uuid);
        } else {
            p_msg->p_uuid = NULL;
        }
    }

    bta_sys_sendmsg(p_msg);
#else
    UNUSED(p_dm_inq);
    UNUSED(p_services);
    UNUSED(p_cback);
#endif
}
/*******************************************************************************
**
** Function         BTA_DmBleUpdateConnectionParam
**
** Description      Update connection parameters, can only be used when connection is up.
**
** Parameters:      bd_addr          - BD address of the peer
**                  min_int   -     minimum connection interval, [0x0004~ 0x4000]
**                  max_int   -     maximum connection interval, [0x0004~ 0x4000]
**                  latency   -     slave latency [0 ~ 500]
**                  timeout   -     supervision timeout [0x000a ~ 0xc80]
**
** Returns          void
**
*******************************************************************************/
void BTA_DmBleUpdateConnectionParam(BD_ADDR bd_addr, uint16_t min_int,
                                    uint16_t max_int, uint16_t latency,
                                    uint16_t timeout)
{
#if BLE_INCLUDED == TRUE
    tBTA_DM_API_UPDATE_CONN_PARAM *p_msg =
                    (tBTA_DM_API_UPDATE_CONN_PARAM *)GKI_getbuf(sizeof(tBTA_DM_API_UPDATE_CONN_PARAM));
    p_msg->hdr.event = BTA_DM_API_UPDATE_CONN_PARAM_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->min_int = min_int;
    p_msg->max_int = max_int;
    p_msg->latency = latency;
    p_msg->timeout = timeout;
    bta_sys_sendmsg(p_msg);
#endif
}

/*******************************************************************************
**
** Function         BTA_DmBleConfigLocalPrivacy
**
** Description      Enable/disable privacy on the local device
**
** Parameters:      privacy_enable   - enable/disabe privacy on remote device.
**
** Returns          void
**
*******************************************************************************/
void BTA_DmBleConfigLocalPrivacy(uint8_t privacy_enable)
{
#if BLE_INCLUDED == TRUE && BLE_PRIVACY_SPT == TRUE
    tBTA_DM_API_LOCAL_PRIVACY *p_msg =
                    (tBTA_DM_API_LOCAL_PRIVACY *)GKI_getbuf(sizeof(tBTA_DM_API_ENABLE_PRIVACY));
    p_msg->hdr.event = BTA_DM_API_LOCAL_PRIVACY_EVT;
    p_msg->privacy_enable   = privacy_enable;
    bta_sys_sendmsg(p_msg);
#else
    UNUSED(privacy_enable);
#endif
}

#if BLE_INCLUDED == TRUE
/*******************************************************************************
**
** Function         BTA_BleEnableAdvInstance
**
** Description      This function enable a Multi-ADV instance with the specififed
**                  adv parameters
**
** Parameters       p_params: pointer to the adv parameter structure.
**                  p_cback: callback function associated to this adv instance.
**                  p_ref: reference data pointer to this adv instance.
**
** Returns          BTA_SUCCESS if command started sucessfully; otherwise failure.
**
*******************************************************************************/
void BTA_BleEnableAdvInstance(tBTA_BLE_ADV_PARAMS *p_params,
                              tBTA_BLE_MULTI_ADV_CBACK *p_cback,
                              void *p_ref)
{
    const size_t len = sizeof(tBTA_BLE_ADV_PARAMS) +
                       sizeof(tBTA_DM_API_BLE_MULTI_ADV_ENB);
    tBTA_DM_API_BLE_MULTI_ADV_ENB *p_msg =
                    (tBTA_DM_API_BLE_MULTI_ADV_ENB *)GKI_getbuf(len);
    APPL_TRACE_API("%s", __func__);
    p_msg->hdr.event = BTA_DM_API_BLE_MULTI_ADV_ENB_EVT;
    p_msg->p_cback = (void *)p_cback;

    if(p_params != NULL) {
        p_msg->p_params = (void *)(p_msg + 1);
        wm_memcpy(p_msg->p_params, p_params, sizeof(tBTA_BLE_ADV_PARAMS));
    }

    p_msg->p_ref = p_ref;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_BleUpdateAdvInstParam
**
** Description      This function update a Multi-ADV instance with the specififed
**                  adv parameters.
**
** Parameters       inst_id: Adv instance to update the parameter.
**                  p_params: pointer to the adv parameter structure.
**
** Returns          BTA_SUCCESS if command started sucessfully; otherwise failure.
**
*******************************************************************************/
void BTA_BleUpdateAdvInstParam(uint8_t inst_id, tBTA_BLE_ADV_PARAMS *p_params)
{
    const size_t len = sizeof(tBTA_BLE_ADV_PARAMS) +
                       sizeof(tBTA_DM_API_BLE_MULTI_ADV_PARAM);
    tBTA_DM_API_BLE_MULTI_ADV_PARAM *p_msg =
                    (tBTA_DM_API_BLE_MULTI_ADV_PARAM *)GKI_getbuf(len);
    APPL_TRACE_API("%s", __func__);
    p_msg->hdr.event = BTA_DM_API_BLE_MULTI_ADV_PARAM_UPD_EVT;
    p_msg->inst_id = inst_id;
    p_msg->p_params = (void *)(p_msg + 1);
    wm_memcpy(p_msg->p_params, p_params, sizeof(tBTA_BLE_ADV_PARAMS));
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_BleCfgAdvInstData
**
** Description      This function configure a Multi-ADV instance with the specififed
**                  adv data or scan response data.
**
** Parameter        inst_id: Adv instance to configure the adv data or scan response.
**                  is_scan_rsp: is the data scan response or adv data.
**                  data_mask: adv data type as bit mask.
**                  p_data: pointer to the ADV data structure tBTA_BLE_ADV_DATA. This
**                  memory space can not be freed until BTA_BLE_MULTI_ADV_DATA_EVT
**                  is sent to application.
**
** Returns          BTA_SUCCESS if command started sucessfully; otherwise failure.
**
*******************************************************************************/
void BTA_BleCfgAdvInstData(uint8_t inst_id, uint8_t is_scan_rsp,
                           tBTA_BLE_AD_MASK data_mask,
                           tBTA_BLE_ADV_DATA *p_data)
{
    tBTA_DM_API_BLE_MULTI_ADV_DATA *p_msg = GKI_getbuf(sizeof(*p_msg));
    p_msg->hdr.event = BTA_DM_API_BLE_MULTI_ADV_DATA_EVT;
    p_msg->inst_id = inst_id;
    p_msg->is_scan_rsp = is_scan_rsp;
    p_msg->data_mask = data_mask;
    wm_memcpy(&p_msg->data, p_data, sizeof(p_msg->data));
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_BleDisableAdvInstance
**
** Description      This function disable a Multi-ADV instance.
**
** Parameter        inst_id: instance ID to disable.
**
** Returns          BTA_SUCCESS if command started sucessfully; otherwise failure.
**
*******************************************************************************/
void BTA_BleDisableAdvInstance(uint8_t inst_id)
{
    tBTA_DM_API_BLE_MULTI_ADV_DISABLE *p_msg =
                    (tBTA_DM_API_BLE_MULTI_ADV_DISABLE *)GKI_getbuf(sizeof(tBTA_DM_API_BLE_MULTI_ADV_DISABLE));
    APPL_TRACE_API("%s: %d", __func__, inst_id);
    p_msg->hdr.event = BTA_DM_API_BLE_MULTI_ADV_DISABLE_EVT;
    p_msg->inst_id = inst_id;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmBleCfgFilterCondition
**
** Description      This function is called to configure the adv data payload filter
**                  condition.
**
** Parameters       action: to read/write/clear
**                  cond_type: filter condition type
**                  filt_index - Filter index
**                  p_cond: filter condition parameter
**                  p_cmpl_back - Command completed callback
**                  ref_value - Reference value
**
** Returns          void
**
*******************************************************************************/
void BTA_DmBleCfgFilterCondition(tBTA_DM_BLE_SCAN_COND_OP action,
                                 tBTA_DM_BLE_PF_COND_TYPE cond_type,
                                 tBTA_DM_BLE_PF_FILT_INDEX filt_index,
                                 tBTA_DM_BLE_PF_COND_PARAM *p_cond,
                                 tBTA_DM_BLE_PF_CFG_CBACK *p_cmpl_cback,
                                 tBTA_DM_BLE_REF_VALUE ref_value)
{
#if BLE_ANDROID_CONTROLLER_SCAN_FILTER == TRUE
    tBTA_DM_API_CFG_FILTER_COND *p_msg;
    APPL_TRACE_API("BTA_DmBleCfgFilterCondition: %d, %d", action, cond_type);
    uint16_t  len = sizeof(tBTA_DM_API_CFG_FILTER_COND) +
                    sizeof(tBTA_DM_BLE_PF_COND_PARAM);
    uint8_t *p;

    if(NULL != p_cond) {
        switch(cond_type) {
            case BTA_DM_BLE_PF_SRVC_DATA_PATTERN:
            case BTA_DM_BLE_PF_MANU_DATA:
                /* Length of pattern and pattern mask and other elements in */
                /* tBTA_DM_BLE_PF_MANU_COND */
                len += ((p_cond->manu_data.data_len) * 2) +
                       sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint8_t);
                break;

            case BTA_DM_BLE_PF_LOCAL_NAME:
                len += ((p_cond->local_name.data_len) + sizeof(uint8_t));
                break;

            case BTM_BLE_PF_SRVC_UUID:
            case BTM_BLE_PF_SRVC_SOL_UUID:
                len += sizeof(tBLE_BD_ADDR) + sizeof(tBTA_DM_BLE_PF_COND_MASK);
                break;

            default:
                break;
        }
    }

    p_msg = (tBTA_DM_API_CFG_FILTER_COND *)GKI_getbuf(len);
    p_msg->hdr.event = BTA_DM_API_CFG_FILTER_COND_EVT;
    p_msg->action = action;
    p_msg->cond_type = cond_type;
    p_msg->filt_index = filt_index;
    p_msg->p_filt_cfg_cback = p_cmpl_cback;
    p_msg->ref_value = ref_value;

    if(p_cond) {
        p_msg->p_cond_param = (tBTA_DM_BLE_PF_COND_PARAM *)(p_msg + 1);
        wm_memcpy(p_msg->p_cond_param, p_cond, sizeof(tBTA_DM_BLE_PF_COND_PARAM));
        p = (uint8_t *)(p_msg->p_cond_param + 1);

        if(cond_type == BTA_DM_BLE_PF_SRVC_DATA_PATTERN ||
                cond_type == BTA_DM_BLE_PF_MANU_DATA) {
            p_msg->p_cond_param->manu_data.p_pattern = p;
            p_msg->p_cond_param->manu_data.data_len = p_cond->manu_data.data_len;
            wm_memcpy(p_msg->p_cond_param->manu_data.p_pattern, p_cond->manu_data.p_pattern,
                      p_cond->manu_data.data_len);
            p += p_cond->manu_data.data_len;

            if(cond_type == BTA_DM_BLE_PF_MANU_DATA) {
                p_msg->p_cond_param->manu_data.company_id_mask =
                                p_cond->manu_data.company_id_mask;

                if(p_cond->manu_data.p_pattern_mask != NULL) {
                    p_msg->p_cond_param->manu_data.p_pattern_mask = p;
                    wm_memcpy(p_msg->p_cond_param->manu_data.p_pattern_mask,
                              p_cond->manu_data.p_pattern_mask,
                              p_cond->manu_data.data_len);
                }
            }
        } else if(cond_type == BTA_DM_BLE_PF_LOCAL_NAME) {
            p_msg->p_cond_param->local_name.p_data = p;
            p_msg->p_cond_param->local_name.data_len =
                            p_cond->local_name.data_len;
            wm_memcpy(p_msg->p_cond_param->local_name.p_data,
                      p_cond->local_name.p_data, p_cond->local_name.data_len);
        } else if(cond_type == BTM_BLE_PF_SRVC_UUID ||
                  cond_type == BTM_BLE_PF_SRVC_SOL_UUID) {
            if(p_cond->srvc_uuid.p_target_addr != NULL) {
                p_msg->p_cond_param->srvc_uuid.p_target_addr = (tBLE_BD_ADDR *)(p);
                p_msg->p_cond_param->srvc_uuid.p_target_addr->type =
                                p_cond->srvc_uuid.p_target_addr->type;
                wm_memcpy(p_msg->p_cond_param->srvc_uuid.p_target_addr->bda,
                          p_cond->srvc_uuid.p_target_addr->bda, BD_ADDR_LEN);
                p = (uint8_t *)(p_msg->p_cond_param->srvc_uuid.p_target_addr + 1);
            }

            if(p_cond->srvc_uuid.p_uuid_mask) {
                p_msg->p_cond_param->srvc_uuid.p_uuid_mask = (tBTA_DM_BLE_PF_COND_MASK *)p;
                wm_memcpy(p_msg->p_cond_param->srvc_uuid.p_uuid_mask,
                          p_cond->srvc_uuid.p_uuid_mask,
                          sizeof(tBTA_DM_BLE_PF_COND_MASK));
            }
        }
    }

    bta_sys_sendmsg(p_msg);
#else
    UNUSED(action);
    UNUSED(cond_type);
    UNUSED(filt_index);
    UNUSED(p_cond);
    UNUSED(p_cmpl_cback);
    UNUSED(ref_value);
#endif
}

/*******************************************************************************
**
** Function         BTA_DmBleScanFilterSetup
**
** Description      This function is called to setup the adv data payload filter param
**
** Parameters       p_target: enable the filter condition on a target device; if NULL
**                  filt_index - Filter index
**                  p_filt_params -Filter parameters
**                  ref_value - Reference value
**                  action - Add, delete or clear
**                  p_cmpl_back - Command completed callback
**
** Returns          void
**
*******************************************************************************/
void BTA_DmBleScanFilterSetup(uint8_t action,
                              tBTA_DM_BLE_PF_FILT_INDEX filt_index,
                              tBTA_DM_BLE_PF_FILT_PARAMS *p_filt_params,
                              tBLE_BD_ADDR *p_target,
                              tBTA_DM_BLE_PF_PARAM_CBACK *p_cmpl_cback,
                              tBTA_DM_BLE_REF_VALUE ref_value)
{
#if BLE_ANDROID_CONTROLLER_SCAN_FILTER == TRUE
    const size_t len = sizeof(tBTA_DM_API_SCAN_FILTER_PARAM_SETUP) +
                       sizeof(tBLE_BD_ADDR);
    tBTA_DM_API_SCAN_FILTER_PARAM_SETUP *p_msg =
                    (tBTA_DM_API_SCAN_FILTER_PARAM_SETUP *)GKI_getbuf(len);
    APPL_TRACE_API("%s: %d", __func__, action);
    p_msg->hdr.event = BTA_DM_API_SCAN_FILTER_SETUP_EVT;
    p_msg->action = action;
    p_msg->filt_index = filt_index;

    if(p_filt_params) {
        wm_memcpy(&p_msg->filt_params, p_filt_params,
                  sizeof(tBTA_DM_BLE_PF_FILT_PARAMS));
    }

    p_msg->p_filt_param_cback = p_cmpl_cback;
    p_msg->ref_value        = ref_value;

    if(p_target) {
        p_msg->p_target = (tBLE_BD_ADDR *)(p_msg + 1);
        wm_memcpy(p_msg->p_target, p_target, sizeof(tBLE_BD_ADDR));
    }

    bta_sys_sendmsg(p_msg);
#else
    UNUSED(action);
    UNUSED(filt_index);
    UNUSED(p_filt_params);
    UNUSED(p_target);
    UNUSED(p_cmpl_cback);
    UNUSED(ref_value);
#endif
}

/*******************************************************************************
**
** Function         BTA_DmBleGetEnergyInfo
**
** Description      This function is called to obtain the energy info
**
** Parameters       p_cmpl_cback - Command complete callback
**
** Returns          void
**
*******************************************************************************/
void BTA_DmBleGetEnergyInfo(tBTA_BLE_ENERGY_INFO_CBACK *p_cmpl_cback)
{
    const size_t len = sizeof(tBTA_DM_API_ENERGY_INFO) + sizeof(tBLE_BD_ADDR);
    tBTA_DM_API_ENERGY_INFO *p_msg = (tBTA_DM_API_ENERGY_INFO *)GKI_getbuf(len);
    APPL_TRACE_API("%s", __func__);
    p_msg->hdr.event = BTA_DM_API_BLE_ENERGY_INFO_EVT;
    p_msg->p_energy_info_cback = p_cmpl_cback;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmEnableScanFilter
**
** Description      This function is called to enable the adv data payload filter
**
** Parameters       action - enable or disable the APCF feature
**                  p_cmpl_cback - Command completed callback
**                  ref_value - Reference value
**
** Returns          void
**
*******************************************************************************/
void BTA_DmEnableScanFilter(uint8_t action, tBTA_DM_BLE_PF_STATUS_CBACK *p_cmpl_cback,
                            tBTA_DM_BLE_REF_VALUE ref_value)
{
#if BLE_ANDROID_CONTROLLER_SCAN_FILTER == TRUE
    const size_t len = sizeof(tBTA_DM_API_ENABLE_SCAN_FILTER) +
                       sizeof(tBLE_BD_ADDR);
    tBTA_DM_API_ENABLE_SCAN_FILTER *p_msg =
                    (tBTA_DM_API_ENABLE_SCAN_FILTER *)GKI_getbuf(len);
    APPL_TRACE_API("%s: %d", __func__, action);
    p_msg->hdr.event = BTA_DM_API_SCAN_FILTER_ENABLE_EVT;
    p_msg->action = action;
    p_msg->ref_value = ref_value;
    p_msg->p_filt_status_cback = p_cmpl_cback;
    bta_sys_sendmsg(p_msg);
#else
    UNUSED(action);
    UNUSED(p_cmpl_cback);
    UNUSED(ref_value);
#endif
}

/*******************************************************************************
**
** Function         BTA_DmBleUpdateConnectionParams
**
** Description      Update connection parameters, can only be used when connection is up.
**
** Parameters:      bd_addr   - BD address of the peer
**                  min_int   -     minimum connection interval, [0x0004~ 0x4000]
**                  max_int   -     maximum connection interval, [0x0004~ 0x4000]
**                  latency   -     slave latency [0 ~ 500]
**                  timeout   -     supervision timeout [0x000a ~ 0xc80]
**
** Returns          void
**
*******************************************************************************/
void BTA_DmBleUpdateConnectionParams(BD_ADDR bd_addr, uint16_t min_int, uint16_t max_int,
                                     uint16_t latency, uint16_t timeout)
{
    tBTA_DM_API_UPDATE_CONN_PARAM *p_msg =
                    (tBTA_DM_API_UPDATE_CONN_PARAM *)GKI_getbuf(sizeof(tBTA_DM_API_UPDATE_CONN_PARAM));
    p_msg->hdr.event = BTA_DM_API_UPDATE_CONN_PARAM_EVT;
    bdcpy(p_msg->bd_addr, bd_addr);
    p_msg->min_int = min_int;
    p_msg->max_int = max_int;
    p_msg->latency = latency;
    p_msg->timeout = timeout;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmBleSetDataLength
**
** Description      This function is to set maximum LE data packet size
**
** Returns          void
**
**
*******************************************************************************/
void BTA_DmBleSetDataLength(BD_ADDR remote_device, uint16_t tx_data_length)
{
    tBTA_DM_API_BLE_SET_DATA_LENGTH *p_msg =
                    (tBTA_DM_API_BLE_SET_DATA_LENGTH *)GKI_getbuf(sizeof(tBTA_DM_API_BLE_SET_DATA_LENGTH));
    bdcpy(p_msg->remote_bda, remote_device);
    p_msg->hdr.event = BTA_DM_API_SET_DATA_LENGTH_EVT;
    p_msg->tx_data_length = tx_data_length;
    bta_sys_sendmsg(p_msg);
}

#endif

/*******************************************************************************
**
** Function         BTA_DmSetEncryption
**
** Description      This function is called to ensure that connection is
**                  encrypted.  Should be called only on an open connection.
**                  Typically only needed for connections that first want to
**                  bring up unencrypted links, then later encrypt them.
**
** Parameters:      bd_addr       - Address of the peer device
**                  transport     - transport of the link to be encruypted
**                  p_callback    - Pointer to callback function to indicat the
**                                  link encryption status
**                  sec_act       - This is the security action to indicate
**                                  what knid of BLE security level is required for
**                                  the BLE link if the BLE is supported
**                                  Note: This parameter is ignored for the BR/EDR link
**                                        or the BLE is not supported
**
** Returns          void
**
*******************************************************************************/
void BTA_DmSetEncryption(BD_ADDR bd_addr, tBTA_TRANSPORT transport,
                         tBTA_DM_ENCRYPT_CBACK *p_callback,
                         tBTA_DM_BLE_SEC_ACT sec_act)
{
    tBTA_DM_API_SET_ENCRYPTION *p_msg = (tBTA_DM_API_SET_ENCRYPTION *)GKI_getbuf(sizeof(
            tBTA_DM_API_SET_ENCRYPTION));
    APPL_TRACE_API("%s", __func__);
    p_msg->hdr.event = BTA_DM_API_SET_ENCRYPTION_EVT;
    wm_memcpy(p_msg->bd_addr, bd_addr, BD_ADDR_LEN);
    p_msg->transport = transport;
    p_msg->p_callback = p_callback;
    p_msg->sec_act = sec_act;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_DmCloseACL
**
** Description      This function force to close an ACL connection and remove the
**                  device from the security database list of known devices.
**
** Parameters:      bd_addr       - Address of the peer device
**                  remove_dev    - remove device or not after link down
**
** Returns          void
**
*******************************************************************************/
void BTA_DmCloseACL(BD_ADDR bd_addr, uint8_t remove_dev, tBTA_TRANSPORT transport)
{
    tBTA_DM_API_REMOVE_ACL *p_msg =
                    (tBTA_DM_API_REMOVE_ACL *)GKI_getbuf(sizeof(tBTA_DM_API_REMOVE_ACL));
    APPL_TRACE_API("%s", __func__);
    p_msg->hdr.event = BTA_DM_API_REMOVE_ACL_EVT;
    wm_memcpy(p_msg->bd_addr, bd_addr, BD_ADDR_LEN);
    p_msg->remove_dev = remove_dev;
    p_msg->transport = transport;
    bta_sys_sendmsg(p_msg);
}

#if BLE_INCLUDED == TRUE
/*******************************************************************************
**
** Function         BTA_DmBleObserve
**
** Description      This procedure keep the device listening for advertising
**                  events from a broadcast device.
**
** Parameters       start: start or stop observe.
**
** Returns          void

**
** Returns          void.
**
*******************************************************************************/
extern void BTA_DmBleObserve(uint8_t start, uint8_t duration,
                             tBTA_DM_SEARCH_CBACK *p_results_cb)
{
    tBTA_DM_API_BLE_OBSERVE *p_msg =
                    (tBTA_DM_API_BLE_OBSERVE *)GKI_getbuf(sizeof(tBTA_DM_API_BLE_OBSERVE));
    APPL_TRACE_API("%s:start = %d ", __func__, start);
    p_msg->hdr.event = BTA_DM_API_BLE_OBSERVE_EVT;
    p_msg->start = start;
    p_msg->duration = duration;
    p_msg->p_cback = p_results_cb;
    bta_sys_sendmsg(p_msg);
}

/*******************************************************************************
**
** Function         BTA_VendorInit
**
** Description      This function initializes vendor specific
**
** Returns          void
**
*******************************************************************************/
void BTA_VendorInit(void)
{
    APPL_TRACE_API("BTA_VendorInit");
}

/*******************************************************************************
**
** Function         BTA_VendorCleanup
**
** Description      This function frees up Broadcom specific VS specific dynamic memory
**
** Returns          void
**
*******************************************************************************/
void BTA_VendorCleanup(void)
{
    tBTM_BLE_VSC_CB cmn_ble_vsc_cb;
    BTM_BleGetVendorCapabilities(&cmn_ble_vsc_cb);
#if (BLE_INCLUDED == TRUE && BLE_ANDROID_CONTROLLER_SCAN_FILTER == TRUE)

    if(cmn_ble_vsc_cb.max_filter > 0) {
        btm_ble_adv_filter_cleanup();
#if BLE_PRIVACY_SPT == TRUE
        btm_ble_resolving_list_cleanup();
#endif
    }

    if(cmn_ble_vsc_cb.tot_scan_results_strg > 0) {
        btm_ble_batchscan_cleanup();
    }

#endif
#if (BLE_VND_INCLUDED == TRUE)

    if(cmn_ble_vsc_cb.adv_inst_max > 0) {
        btm_ble_multi_adv_cleanup();
    }

#endif
}

#endif
