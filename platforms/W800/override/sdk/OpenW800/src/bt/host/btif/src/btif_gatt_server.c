/******************************************************************************
 *
 *  Copyright (C) 2009-2013 Broadcom Corporation
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

/************************************************************************************
 *
 *  Filename:      btif_gatt_server.c
 *
 *  Description:   GATT server implementation
 *
 ***********************************************************************************/

#define LOG_TAG "bt_btif_gatt"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "bt_target.h"
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
#include "bluetooth.h"
#include "bt_gatt.h"

#include "btif_common.h"
#include "btif_util.h"

#include "bta_api.h"
#include "bta_gatt_api.h"
#include "btif_config.h"
#include "btif_dm.h"
#include "btif_gatt.h"
#include "btif_gatt_util.h"
#include "btif_storage.h"
#include "bt_common.h"
#include "osi/include/log.h"

#include "wm_ble_gatt.h"
#include "wm_bt_def.h"

/************************************************************************************
**  Constants & Macros
************************************************************************************/

#define CHECK_BTGATT_INIT() if (tls_ble_cb == NULL)\
    {\
        LOG_WARN(LOG_TAG, "%s: BTGATT not initialized", __FUNCTION__);\
        return TLS_BT_STATUS_NOT_READY;\
    } else {\
        LOG_VERBOSE(LOG_TAG, "%s", __FUNCTION__);\
    }

typedef enum {
    BTIF_GATTS_REGISTER_APP = 2000,
    BTIF_GATTS_UNREGISTER_APP,
    BTIF_GATTS_OPEN,
    BTIF_GATTS_CLOSE,
    BTIF_GATTS_CREATE_SERVICE,
    BTIF_GATTS_ADD_INCLUDED_SERVICE,
    BTIF_GATTS_ADD_CHARACTERISTIC,
    BTIF_GATTS_ADD_DESCRIPTOR,
    BTIF_GATTS_START_SERVICE,
    BTIF_GATTS_STOP_SERVICE,
    BTIF_GATTS_DELETE_SERVICE,
    BTIF_GATTS_SEND_INDICATION,
    BTIF_GATTS_SEND_RESPONSE
} btif_gatts_event_t;

/************************************************************************************
**  Local type definitions
************************************************************************************/

typedef struct {
    uint8_t             *value;
    btgatt_srvc_id_t    srvc_id;
    tls_bt_addr_t       bd_addr;
    tls_bt_uuid_t       uuid;
    uint32_t            trans_id;
    uint16_t            conn_id;
    uint16_t            srvc_handle;
    uint16_t            incl_handle;
    uint16_t            attr_handle;
    uint16_t            permissions;
    uint16_t            len;
    uint16_t            offset;
    uint8_t             server_if;
    uint8_t             is_direct;
    uint8_t             num_handles;
    uint8_t             properties;
    uint8_t             confirm;
    uint8_t             status;
    uint8_t             auth_req;
    btgatt_transport_t  transport;

} __attribute__((packed)) btif_gatts_cb_t;

/************************************************************************************
**  Static variables
************************************************************************************/

static tls_ble_callback_t tls_ble_cb = NULL;

/************************************************************************************
**  Static functions
************************************************************************************/

static void btapp_gatts_copy_req_data(uint16_t event, char *p_dest, char *p_src)
{
    tBTA_GATTS *p_dest_data = (tBTA_GATTS *) p_dest;
    tBTA_GATTS *p_src_data = (tBTA_GATTS *) p_src;

    if(!p_src_data || !p_dest_data) {
        return;
    }

    // Copy basic structure first
    maybe_non_aligned_memcpy(p_dest_data, p_src_data, sizeof(*p_src_data));

    // Allocate buffer for request data if necessary
    switch(event) {
        case BTA_GATTS_READ_EVT:
        case BTA_GATTS_WRITE_EVT:
        case BTA_GATTS_EXEC_WRITE_EVT:
        case BTA_GATTS_MTU_EVT:
            p_dest_data->req_data.p_data = GKI_getbuf(sizeof(tBTA_GATTS_REQ_DATA));
            wm_memcpy(p_dest_data->req_data.p_data, p_src_data->req_data.p_data,
                      sizeof(tBTA_GATTS_REQ_DATA));
            break;

        default:
            break;
    }
}

static void btapp_gatts_free_req_data(uint16_t event, tBTA_GATTS *p_data)
{
    switch(event) {
        case BTA_GATTS_READ_EVT:
        case BTA_GATTS_WRITE_EVT:
        case BTA_GATTS_EXEC_WRITE_EVT:
        case BTA_GATTS_MTU_EVT:
            if(p_data != NULL) {
                GKI_free_and_reset_buf((void **)&p_data->req_data.p_data);
            }

            break;

        default:
            break;
    }
}

static void btapp_gatts_handle_cback(uint16_t event, char *p_param)
{
    LOG_VERBOSE(LOG_TAG, "%s: Event %d", __FUNCTION__, event);
    tBTA_GATTS *p_data = (tBTA_GATTS *)p_param;
    tls_ble_msg_t msg;

    switch(event) {
        case BTA_GATTS_REG_EVT: {
            bta_to_btif_uuid(&msg.ser_register.app_uuid, &p_data->reg_oper.uuid);
            msg.ser_register.server_if = p_data->reg_oper.server_if;
            msg.ser_register.status = p_data->reg_oper.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_REGISTER_EVT, &msg); }

            break;
        }

        case BTA_GATTS_DEREG_EVT:
            msg.ser_register.server_if = p_data->reg_oper.server_if;
            msg.ser_register.status = p_data->reg_oper.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_DEREGISTER_EVT, &msg); }

            break;

        case BTA_GATTS_CONNECT_EVT: {
            bdcpy(msg.ser_connect.addr, p_data->conn.remote_bda);
            msg.ser_connect.connected = true;
            msg.ser_connect.conn_id = p_data->conn.conn_id;
            msg.ser_connect.server_if = p_data->conn.server_if;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_CONNECT_EVT, &msg); }

            break;
        }

        case BTA_GATTS_DISCONNECT_EVT: {
            bdcpy(msg.ser_disconnect.addr, p_data->conn.remote_bda);
            msg.ser_disconnect.connected = false;
            msg.ser_disconnect.conn_id = p_data->conn.conn_id;
            msg.ser_disconnect.server_if = p_data->conn.server_if;
            msg.ser_disconnect.reason = p_data->conn.reason;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_DISCONNECT_EVT, &msg); }

            break;
        }

        case BTA_GATTS_CREATE_EVT: {
            msg.ser_create.inst_id = p_data->create.svc_instance;
            msg.ser_create.is_primary = p_data->create.is_primary;
            msg.ser_create.server_if = p_data->create.server_if;
            msg.ser_create.service_id = p_data->create.service_id;
            bta_to_btif_uuid(&msg.ser_create.uuid, &p_data->create.uuid);
            msg.ser_create.status = p_data->create.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_CREATE_EVT, &msg); }
        }
        break;

        case BTA_GATTS_ADD_INCL_SRVC_EVT:
            msg.ser_add_incl_srvc.attr_id = p_data->add_result.attr_id;
            msg.ser_add_incl_srvc.server_if = p_data->add_result.server_if;
            msg.ser_add_incl_srvc.service_id = p_data->add_result.service_id;
            msg.ser_add_incl_srvc.status = p_data->add_result.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_ADD_CHAR_EVT, &msg); }

            break;

        case BTA_GATTS_ADD_CHAR_EVT: {
            msg.ser_add_char.attr_id = p_data->add_result.attr_id;
            msg.ser_add_char.server_if = p_data->add_result.server_if;
            msg.ser_add_char.service_id = p_data->add_result.service_id;
            msg.ser_add_char.status = p_data->add_result.status;
            bta_to_btif_uuid(&msg.ser_add_char.uuid, &p_data->add_result.char_uuid);

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_ADD_CHAR_EVT, &msg); }

            break;
        }

        case BTA_GATTS_ADD_CHAR_DESCR_EVT: {
            msg.ser_add_char_descr.attr_id = p_data->add_result.attr_id;
            msg.ser_add_char_descr.server_if = p_data->add_result.server_if;
            msg.ser_add_char_descr.service_id = p_data->add_result.service_id;
            msg.ser_add_char_descr.status = p_data->add_result.status;
            bta_to_btif_uuid(&msg.ser_add_char_descr.uuid, &p_data->add_result.char_uuid);

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_ADD_CHAR_DESCR_EVT, &msg); }

            break;
        }

        case BTA_GATTS_START_EVT:
            msg.ser_start_srvc.server_if = p_data->srvc_oper.server_if;
            msg.ser_start_srvc.service_id = p_data->srvc_oper.service_id;
            msg.ser_start_srvc.status = p_data->srvc_oper.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_START_EVT, &msg); }

            break;

        case BTA_GATTS_STOP_EVT:
            msg.ser_stop_srvc.server_if = p_data->srvc_oper.server_if;
            msg.ser_stop_srvc.service_id = p_data->srvc_oper.service_id;
            msg.ser_stop_srvc.status = p_data->srvc_oper.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_STOP_EVT, &msg); }

            break;

        case BTA_GATTS_DELELTE_EVT:
            msg.ser_delete_srvc.server_if = p_data->srvc_oper.server_if;
            msg.ser_delete_srvc.service_id = p_data->srvc_oper.service_id;
            msg.ser_delete_srvc.status = p_data->srvc_oper.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_DELETE_EVT, &msg); }

            break;

        case BTA_GATTS_READ_EVT: {
            msg.ser_read.conn_id = p_data->req_data.conn_id;
            msg.ser_read.handle = p_data->req_data.p_data->read_req.handle;
            msg.ser_read.is_long = p_data->req_data.p_data->read_req.is_long;
            msg.ser_read.offset = p_data->req_data.p_data->read_req.offset;
            bdcpy(msg.ser_read.remote_bda, p_data->req_data.remote_bda);
            msg.ser_read.trans_id = p_data->req_data.trans_id;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_READ_EVT, &msg); }

            break;
        }

        case BTA_GATTS_WRITE_EVT: {
            msg.ser_write.conn_id = p_data->req_data.conn_id;
            msg.ser_write.handle = p_data->req_data.p_data->write_req.handle;
            msg.ser_write.is_prep = p_data->req_data.p_data->write_req.is_prep;
            msg.ser_write.len = p_data->req_data.p_data->write_req.len;
            msg.ser_write.need_rsp = p_data->req_data.p_data->write_req.need_rsp;
            msg.ser_write.offset = p_data->req_data.p_data->write_req.offset;
            bdcpy(msg.ser_write.remote_bda, p_data->req_data.remote_bda);
            msg.ser_write.trans_id = p_data->req_data.trans_id;
            msg.ser_write.value = &p_data->req_data.p_data->write_req.value[0];

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_WRITE_EVT, &msg); }

            break;
        }

        case BTA_GATTS_EXEC_WRITE_EVT: {
            msg.ser_exec_write.conn_id = p_data->req_data.conn_id;
            msg.ser_exec_write.exec_write = p_data->req_data.p_data->exec_write;
            bdcpy(msg.ser_exec_write.remote_bda,  p_data->req_data.remote_bda);
            msg.ser_exec_write.trans_id = p_data->req_data.trans_id;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_EXEC_WRITE_EVT, &msg); }

            break;
        }

        case BTA_GATTS_CONF_EVT:
            msg.ser_confirm.conn_id = p_data->req_data.conn_id;
            msg.ser_confirm.status = p_data->req_data.status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_CONFIRM_EVT, &msg); }

            break;

        case BTA_GATTS_CONGEST_EVT:
            msg.ser_congest.congested = p_data->congest.congested;
            msg.ser_congest.conn_id = p_data->congest.conn_id;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_CONGEST_EVT, &msg); }

            break;

        case BTA_GATTS_MTU_EVT:
            msg.ser_mtu.conn_id = p_data->req_data.conn_id;
            msg.ser_mtu.mtu = p_data->req_data.p_data->mtu;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_MTU_EVT, &msg); }

            break;

        case BTA_GATTS_OPEN_EVT:
        case BTA_GATTS_CANCEL_OPEN_EVT:
        case BTA_GATTS_CLOSE_EVT:
            LOG_DEBUG(LOG_TAG, "%s: Empty event (%d)!", __FUNCTION__, event);
            break;

        default:
            LOG_ERROR(LOG_TAG, "%s: Unhandled event (%d)!", __FUNCTION__, event);
            break;
    }

    btapp_gatts_free_req_data(event, p_data);
}

static void btapp_gatts_cback(tBTA_GATTS_EVT event, tBTA_GATTS *p_data)
{
    tls_bt_status_t status;
    status = btif_transfer_context(btapp_gatts_handle_cback, (uint16_t) event,
                                   (void *)p_data, sizeof(tBTA_GATTS), btapp_gatts_copy_req_data);
    ASSERTC(status == TLS_BT_STATUS_SUCCESS, "Context transfer failed!", status);
}

static void btgatts_handle_event(uint16_t event, char *p_param)
{
    btif_gatts_cb_t *p_cb = (btif_gatts_cb_t *)p_param;

    if(!p_cb) {
        return;
    }

    LOG_VERBOSE(LOG_TAG, "%s: Event %d", __FUNCTION__, event);

    switch(event) {
        case BTIF_GATTS_REGISTER_APP: {
            tBT_UUID uuid;
            btif_to_bta_uuid(&uuid, &p_cb->uuid);
            BTA_GATTS_AppRegister(&uuid, btapp_gatts_cback);
            break;
        }

        case BTIF_GATTS_UNREGISTER_APP:
            BTA_GATTS_AppDeregister(p_cb->server_if);
            break;

        case BTIF_GATTS_OPEN: {
            // Ensure device is in inquiry database
            int addr_type = 0;
            int device_type = 0;
            tBTA_GATT_TRANSPORT transport = BTA_GATT_TRANSPORT_LE;

            if(btif_get_device_type(&p_cb->bd_addr, &addr_type, &device_type) == TRUE
                    && device_type != BT_DEVICE_TYPE_BREDR) {
                BTA_DmAddBleDevice(p_cb->bd_addr.address, addr_type, device_type);
            }

            // Mark background connections
            if(!p_cb->is_direct) {
                BTA_DmBleSetBgConnType(BTM_BLE_CONN_AUTO, NULL);
            }

            // Determine transport
            if(p_cb->transport != GATT_TRANSPORT_AUTO) {
                transport = p_cb->transport;
            } else {
                switch(device_type) {
                    case BT_DEVICE_TYPE_BREDR:
                        transport = BTA_GATT_TRANSPORT_BR_EDR;
                        break;

                    case BT_DEVICE_TYPE_BLE:
                        transport = BTA_GATT_TRANSPORT_LE;
                        break;

                    case BT_DEVICE_TYPE_DUMO:
                        if(p_cb->transport == GATT_TRANSPORT_LE) {
                            transport = BTA_GATT_TRANSPORT_LE;
                        } else {
                            transport = BTA_GATT_TRANSPORT_BR_EDR;
                        }

                        break;

                    default:
                        BTIF_TRACE_ERROR("%s: Invalid device type %d", __func__, device_type);
                        return;
                }
            }

            // Connect!
            BTA_GATTS_Open(p_cb->server_if, p_cb->bd_addr.address,
                           p_cb->is_direct, transport);
            break;
        }

        case BTIF_GATTS_CLOSE:
            // Cancel pending foreground/background connections
            BTA_GATTS_CancelOpen(p_cb->server_if, p_cb->bd_addr.address, TRUE);
            BTA_GATTS_CancelOpen(p_cb->server_if, p_cb->bd_addr.address, FALSE);

            // Close active connection
            if(p_cb->conn_id != 0) {
                BTA_GATTS_Close(p_cb->conn_id);
            }

            break;

        case BTIF_GATTS_CREATE_SERVICE: {
            tBT_UUID uuid;
            btif_to_bta_uuid(&uuid, &p_cb->srvc_id.id.uuid);
            BTA_GATTS_CreateService(p_cb->server_if, &uuid,
                                    p_cb->srvc_id.id.inst_id, p_cb->num_handles,
                                    p_cb->srvc_id.is_primary);
            break;
        }

        case BTIF_GATTS_ADD_INCLUDED_SERVICE:
            BTA_GATTS_AddIncludeService(p_cb->srvc_handle, p_cb->incl_handle);
            break;

        case BTIF_GATTS_ADD_CHARACTERISTIC: {
            tBT_UUID uuid;
            btif_to_bta_uuid(&uuid, &p_cb->uuid);
            BTA_GATTS_AddCharacteristic(p_cb->srvc_handle, &uuid,
                                        p_cb->permissions, p_cb->properties);
            break;
        }

        case BTIF_GATTS_ADD_DESCRIPTOR: {
            tBT_UUID uuid;
            btif_to_bta_uuid(&uuid, &p_cb->uuid);
            BTA_GATTS_AddCharDescriptor(p_cb->srvc_handle, p_cb->permissions,
                                        &uuid);
            break;
        }

        case BTIF_GATTS_START_SERVICE:
            BTA_GATTS_StartService(p_cb->srvc_handle, p_cb->transport);
            break;

        case BTIF_GATTS_STOP_SERVICE:
            BTA_GATTS_StopService(p_cb->srvc_handle);
            break;

        case BTIF_GATTS_DELETE_SERVICE:
            BTA_GATTS_DeleteService(p_cb->srvc_handle);
            break;

        case BTIF_GATTS_SEND_INDICATION: {
            uint8_t status = BTA_GATTS_HandleValueIndication(p_cb->conn_id, p_cb->attr_handle,
                             p_cb->len, p_cb->value /*p_cb->response.attr_value.value*/, p_cb->confirm);

            if(p_cb->value) {
                GKI_freebuf(p_cb->value);
            }

            // TODO: Might need to send an ACK if handle value indication is
            //       invoked without need for confirmation.
            if(status != 0) {
                tls_ble_msg_t msg;
                msg.ser_confirm.conn_id = p_cb->conn_id;
                msg.ser_confirm.status = BTA_GATT_NO_RESOURCES;

                if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_CONFIRM_EVT, &msg); }
            }

            break;
        }

        case BTIF_GATTS_SEND_RESPONSE: {
            tBTA_GATTS_RSP rsp_struct;
            //btgatt_response_t *p_rsp = &p_cb->response;
            //btif_to_bta_response(&rsp_struct, p_rsp);
            rsp_struct.attr_value.auth_req = p_cb->auth_req;
            rsp_struct.attr_value.handle = p_cb->attr_handle;
            rsp_struct.attr_value.len = p_cb->len;
            rsp_struct.attr_value.offset = p_cb->offset;
            wm_memcpy(rsp_struct.attr_value.value, p_cb->value, p_cb->len);
            BTA_GATTS_SendRsp(p_cb->conn_id, p_cb->trans_id,
                              p_cb->status, &rsp_struct);

            if(p_cb->value) {
                GKI_freebuf(p_cb->value);
            }

            tls_ble_msg_t msg;
            msg.ser_resp.conn_id = p_cb->conn_id;
            msg.ser_resp.trans_id = p_cb->trans_id;
            msg.ser_resp.status = p_cb->status;

            if(tls_ble_cb) { tls_ble_cb(WM_BLE_SE_RESP_EVT, &msg); }

            break;
        }

        default:
            LOG_ERROR(LOG_TAG, "%s: Unknown event (%d)!", __FUNCTION__, event);
            break;
    }
}

/************************************************************************************
**  Server API Functions
************************************************************************************/

tls_bt_status_t tls_ble_server_app_init(tls_ble_callback_t callback)
{
    if(tls_ble_cb == NULL) {
        tls_ble_cb = callback;
    } else {
        return TLS_BT_STATUS_DONE;
    }

    return TLS_BT_STATUS_SUCCESS;
}
tls_bt_status_t tls_ble_server_app_deinit()
{
    if(tls_ble_cb == NULL) {
        return TLS_BT_STATUS_DONE;
    } else {
        tls_ble_cb = NULL;
    }

    return TLS_BT_STATUS_SUCCESS;
}

tls_bt_status_t tls_ble_server_app_register(tls_bt_uuid_t *uuid)
{
    CHECK_BTGATT_INIT();
    btif_gatts_cb_t btif_cb;
    wm_memcpy(&btif_cb.uuid, uuid, sizeof(tls_bt_uuid_t));
    return btif_transfer_context(btgatts_handle_event, BTIF_GATTS_REGISTER_APP,
                                 (char *) &btif_cb, sizeof(btif_gatts_cb_t), NULL);
}
tls_bt_status_t tls_ble_server_app_unregister(uint8_t server_if)
{
    CHECK_BTGATT_INIT();
    btif_gatts_cb_t btif_cb;
    btif_cb.server_if = server_if;
    return btif_transfer_context(btgatts_handle_event, BTIF_GATTS_UNREGISTER_APP,
                                 (char *) &btif_cb, sizeof(btif_gatts_cb_t), NULL);
}


tls_bt_status_t tls_ble_server_add_service(uint8_t server_if, int inst_id, int primary,
        tls_bt_uuid_t *uuid, int num_handles)
{
    CHECK_BTGATT_INIT();
    btif_gatts_cb_t btif_cb;
    btif_cb.server_if = (uint8_t) server_if;
    btif_cb.num_handles = (uint8_t) num_handles;
    btif_cb.srvc_id.is_primary = primary;
    btif_cb.srvc_id.id.inst_id = inst_id;
    wm_memcpy(&btif_cb.srvc_id.id.uuid, uuid, sizeof(tls_bt_uuid_t));
    return btif_transfer_context(btgatts_handle_event, BTIF_GATTS_CREATE_SERVICE,
                                 (char *) &btif_cb, sizeof(btif_gatts_cb_t), NULL);
}

tls_bt_status_t tls_ble_server_add_characteristic(uint8_t server_if, uint16_t service_handle,
        tls_bt_uuid_t *uuid, int properties, int permissions)
{
    CHECK_BTGATT_INIT();
    btif_gatts_cb_t btif_cb;
    btif_cb.server_if = (uint8_t) server_if;
    btif_cb.srvc_handle = (uint16_t) service_handle;
    btif_cb.properties = (uint8_t) properties;
    btif_cb.permissions = (uint16_t) permissions;
    wm_memcpy(&btif_cb.uuid, (void *)uuid, sizeof(tls_bt_uuid_t));
    return btif_transfer_context(btgatts_handle_event, BTIF_GATTS_ADD_CHARACTERISTIC,
                                 (char *) &btif_cb, sizeof(btif_gatts_cb_t), NULL);
}
tls_bt_status_t tls_ble_server_add_descriptor(uint8_t server_if, uint16_t service_handle,
        tls_bt_uuid_t *uuid, int permissions)
{
    CHECK_BTGATT_INIT();
    btif_gatts_cb_t btif_cb;
    btif_cb.server_if = (uint8_t) server_if;
    btif_cb.srvc_handle = (uint16_t) service_handle;
    btif_cb.permissions = (uint16_t) permissions;
    wm_memcpy(&btif_cb.uuid, (void *)uuid, sizeof(tls_bt_uuid_t));
    return btif_transfer_context(btgatts_handle_event, BTIF_GATTS_ADD_DESCRIPTOR,
                                 (char *) &btif_cb, sizeof(btif_gatts_cb_t), NULL);
}
tls_bt_status_t tls_ble_server_start_service(uint8_t server_if, uint16_t service_handle,
        int transport)
{
    CHECK_BTGATT_INIT();
    btif_gatts_cb_t btif_cb;
    btif_cb.server_if = (uint8_t) server_if;
    btif_cb.srvc_handle = (uint16_t) service_handle;
    btif_cb.transport = (uint8_t) transport;
    return btif_transfer_context(btgatts_handle_event, BTIF_GATTS_START_SERVICE,
                                 (char *) &btif_cb, sizeof(btif_gatts_cb_t), NULL);
}
tls_bt_status_t tls_ble_server_stop_service(uint8_t server_if, uint16_t service_handle)
{
    CHECK_BTGATT_INIT();
    btif_gatts_cb_t btif_cb;
    btif_cb.server_if = (uint8_t) server_if;
    btif_cb.srvc_handle = (uint16_t) service_handle;
    return btif_transfer_context(btgatts_handle_event, BTIF_GATTS_STOP_SERVICE,
                                 (char *) &btif_cb, sizeof(btif_gatts_cb_t), NULL);
}
tls_bt_status_t tls_ble_server_delete_service(uint8_t server_if, uint16_t service_handle)
{
    CHECK_BTGATT_INIT();
    btif_gatts_cb_t btif_cb;
    btif_cb.server_if = (uint8_t) server_if;
    btif_cb.srvc_handle = (uint16_t) service_handle;
    return btif_transfer_context(btgatts_handle_event, BTIF_GATTS_DELETE_SERVICE,
                                 (char *) &btif_cb, sizeof(btif_gatts_cb_t), NULL);
}
tls_bt_status_t tls_ble_server_connect(uint8_t server_if,
                                       const tls_bt_addr_t *bd_addr, uint8_t is_direct, int transport)
{
    CHECK_BTGATT_INIT();
    btif_gatts_cb_t btif_cb;
    btif_cb.server_if = (uint8_t) server_if;
    btif_cb.is_direct = is_direct ? 1 : 0;
    btif_cb.transport = (btgatt_transport_t)transport;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    return btif_transfer_context(btgatts_handle_event, BTIF_GATTS_OPEN,
                                 (char *) &btif_cb, sizeof(btif_gatts_cb_t), NULL);
}
tls_bt_status_t tls_ble_server_disconnect(uint8_t server_if, const tls_bt_addr_t *bd_addr,
        uint16_t conn_id)
{
    CHECK_BTGATT_INIT();
    btif_gatts_cb_t btif_cb;
    btif_cb.server_if = (uint8_t) server_if;
    btif_cb.conn_id = (uint16_t) conn_id;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    return btif_transfer_context(btgatts_handle_event, BTIF_GATTS_CLOSE,
                                 (char *) &btif_cb, sizeof(btif_gatts_cb_t), NULL);
}
tls_bt_status_t tls_ble_server_send_indication(uint8_t server_if, uint16_t attribute_handle,
        uint16_t conn_id, int len, int confirm, char *p_value)
{
    CHECK_BTGATT_INIT();
    tls_bt_status_t status;
    int send_len = 0;
    btif_gatts_cb_t btif_cb;
    btif_cb.server_if = (uint8_t) server_if;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.attr_handle = attribute_handle;
    btif_cb.confirm = confirm;
    send_len = len > BTGATT_MAX_ATTR_LEN ? BTGATT_MAX_ATTR_LEN : len;
    btif_cb.len = send_len;
    btif_cb.value = (uint8_t *)GKI_getbuf(send_len);

    if(btif_cb.value == NULL) {
        return TLS_BT_STATUS_NOMEM;
    }

    wm_memcpy(btif_cb.value, p_value, send_len);
    status = btif_transfer_context(btgatts_handle_event, BTIF_GATTS_SEND_INDICATION,
                                   (char *) &btif_cb, sizeof(btif_gatts_cb_t), NULL);

    if(status != TLS_BT_STATUS_SUCCESS) {
        GKI_freebuf(btif_cb.value);
    }

    return status;
}

tls_bt_status_t tls_ble_server_send_response(uint16_t conn_id, uint32_t trans_id, uint8_t status,
        int offset, uint16_t attr_handle, int auth_req, uint8_t *p_value, int len)
{
    CHECK_BTGATT_INIT();
    tls_bt_status_t ret;
    btif_gatts_cb_t btif_cb;
    int send_len = 0;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.trans_id = (uint32_t) trans_id;
    btif_cb.status = (uint8_t) status;
    btif_cb.attr_handle = (uint16_t)attr_handle;
    btif_cb.auth_req = (uint8_t)auth_req;
    send_len = len > BTGATT_MAX_ATTR_LEN ? BTGATT_MAX_ATTR_LEN : len;
    btif_cb.len = send_len;
    btif_cb.offset = offset;
    btif_cb.value = (uint8_t *)GKI_getbuf(send_len);

    if(btif_cb.value == NULL) {
        return TLS_BT_STATUS_NOMEM;
    }

    wm_memcpy(btif_cb.value, p_value, len);
#if 0
    btif_cb.response.handle = attr_handle;
    btif_cb.response.attr_value.auth_req = auth_req;
    btif_cb.response.attr_value.handle = attr_handle;
    btif_cb.response.attr_value.len = len;
    btif_cb.response.attr_value.offset = offset;
    wm_memcpy(btif_cb.response.attr_value.value, p_value, len);
#endif
    ret = btif_transfer_context(btgatts_handle_event, BTIF_GATTS_SEND_RESPONSE,
                                (char *) &btif_cb, sizeof(btif_gatts_cb_t), NULL);

    if(ret != TLS_BT_STATUS_SUCCESS) {
        GKI_freebuf(btif_cb.value);
    }

    return ret;
}

#endif
