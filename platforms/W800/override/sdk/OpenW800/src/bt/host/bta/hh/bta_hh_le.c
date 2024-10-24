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

#define LOG_TAG "bt_bta_hh"

#include <string.h>
#include "bt_target.h"
#if (defined BTA_HH_LE_INCLUDED && BTA_HH_LE_INCLUDED == TRUE)

#include "bta_api.h"
#include "bta_hh_int.h"
#include "bta_api.h"
#include "bta_gatt_api.h"
#include "bta_hh_co.h"
#include "btm_api.h"
#include "btm_ble_api.h"
#include "btm_int.h"
#include "osi/include/log.h"
#include "srvc_api.h"
#include "stack/include/l2c_api.h"
#include "utl.h"

#ifndef BTA_HH_LE_RECONN
#define BTA_HH_LE_RECONN    TRUE
#endif

#define BTA_HH_APP_ID_LE            0xff

#define BTA_HH_LE_RPT_TYPE_VALID(x)     ((x) <= BTA_LE_HID_RPT_FEATURE && (x)>=BTA_LE_HID_RPT_INPUT)

#define BTA_HH_LE_PROTO_BOOT_MODE      0x00
#define BTA_HH_LE_PROTO_REPORT_MODE      0x01

#define BTA_LE_HID_RTP_UUID_MAX     5
static const uint16_t bta_hh_uuid_to_rtp_type[BTA_LE_HID_RTP_UUID_MAX][2] = {
    {GATT_UUID_HID_REPORT,       BTA_HH_RPTT_INPUT},
    {GATT_UUID_HID_BT_KB_INPUT,  BTA_HH_RPTT_INPUT},
    {GATT_UUID_HID_BT_KB_OUTPUT, BTA_HH_RPTT_OUTPUT},
    {GATT_UUID_HID_BT_MOUSE_INPUT, BTA_HH_RPTT_INPUT},
    {GATT_UUID_BATTERY_LEVEL,      BTA_HH_RPTT_INPUT}
};


static void bta_hh_gattc_callback(tBTA_GATTC_EVT event, tBTA_GATTC *p_data);
static void bta_hh_le_register_scpp_notif(tBTA_HH_DEV_CB *p_dev_cb, tBTA_GATT_STATUS status);
static void bta_hh_le_register_scpp_notif_cmpl(tBTA_HH_DEV_CB *p_dev_cb, tBTA_GATT_STATUS status);
static void bta_hh_le_add_dev_bg_conn(tBTA_HH_DEV_CB *p_cb, uint8_t check_bond);
//TODO(jpawlowski): uncomment when fixed
// static void bta_hh_process_cache_rpt (tBTA_HH_DEV_CB *p_cb,
//                                       tBTA_HH_RPT_CACHE_ENTRY *p_rpt_cache,
//                                       uint8_t num_rpt);

#define GATT_READ_CHAR 0
#define GATT_READ_DESC 1
#define GATT_WRITE_CHAR 2
#define GATT_WRITE_DESC 3

/* Holds pending GATT operations */
typedef struct {
    uint8_t type;
    uint16_t conn_id;
    uint16_t handle;

    /* write-specific fields */
    tBTA_GATTC_WRITE_TYPE write_type;
    uint16_t  len;
    uint8_t   p_value[GATT_MAX_ATTR_LEN];
} gatt_operation;

static list_t *gatt_op_queue = NULL; // list of gatt_operation
static list_t *gatt_op_queue_executing =
                NULL; // list of uint16_t connection ids that currently execute

static void mark_as_executing(uint16_t conn_id)
{
    uint16_t *executing_conn_id = GKI_getbuf(sizeof(uint16_t));
    *executing_conn_id = conn_id;

    if(!gatt_op_queue_executing) {
        gatt_op_queue_executing = list_new(GKI_freebuf);
    }

    list_append(gatt_op_queue_executing, executing_conn_id);
}

static uint8_t rm_exec_conn_id(void *data, void *context)
{
    uint16_t *conn_id = context;
    uint16_t *conn_id2 = data;

    if(*conn_id == *conn_id2) {
        list_remove(gatt_op_queue_executing, data);
    }

    return TRUE;
}

static void mark_as_not_executing(uint16_t conn_id)
{
    if(gatt_op_queue_executing) {
        list_foreach(gatt_op_queue_executing, rm_exec_conn_id, &conn_id);
    }
}

static uint8_t exec_list_contains(void *data, void *context)
{
    uint16_t *conn_id = context;
    uint16_t *conn_id2 = data;

    if(*conn_id == *conn_id2) {
        return FALSE;
    }

    return TRUE;
}

static uint8_t rm_op_by_conn_id(void *data, void *context)
{
    uint16_t *conn_id = context;
    gatt_operation *op = data;

    if(op->conn_id == *conn_id) {
        list_remove(gatt_op_queue, data);
    }

    return TRUE;
}

static void gatt_op_queue_clean(uint16_t conn_id)
{
    if(gatt_op_queue) {
        list_foreach(gatt_op_queue, rm_op_by_conn_id, &conn_id);
    }

    mark_as_not_executing(conn_id);
}

static uint8_t find_op_by_conn_id(void *data, void *context)
{
    uint16_t *conn_id = context;
    gatt_operation *op = data;

    if(op->conn_id == *conn_id) {
        return FALSE;
    }

    return TRUE;
}

static void gatt_execute_next_op(uint16_t conn_id)
{
    APPL_TRACE_DEBUG("%s:", __func__, conn_id);

    if(!gatt_op_queue || list_is_empty(gatt_op_queue)) {
        APPL_TRACE_DEBUG("%s: op queue is empty", __func__);
        return;
    }

    list_node_t *op_node = list_foreach(gatt_op_queue, find_op_by_conn_id, &conn_id);

    if(op_node == NULL) {
        APPL_TRACE_DEBUG("%s: no more operations queued for conn_id %d", __func__, conn_id);
        return;
    }

    gatt_operation *op = list_node(op_node);

    if(gatt_op_queue_executing && list_foreach(gatt_op_queue_executing, exec_list_contains, &conn_id)) {
        APPL_TRACE_DEBUG("%s: can't enqueue next op, already executing", __func__);
        return;
    }

    if(op->type == GATT_READ_CHAR) {
        const tBTA_GATTC_CHARACTERISTIC *p_char = BTA_GATTC_GetCharacteristic(op->conn_id, op->handle);
        mark_as_executing(conn_id);
        BTA_GATTC_ReadCharacteristic(op->conn_id, p_char->handle, BTA_GATT_AUTH_REQ_NONE);
        list_remove(gatt_op_queue, op);
    } else if(op->type == GATT_READ_DESC) {
        const tBTA_GATTC_DESCRIPTOR *p_desc = BTA_GATTC_GetDescriptor(op->conn_id, op->handle);
        mark_as_executing(conn_id);
        BTA_GATTC_ReadCharDescr(op->conn_id, p_desc->handle, BTA_GATT_AUTH_REQ_NONE);
        list_remove(gatt_op_queue, op);
    } else if(op->type == GATT_WRITE_CHAR) {
        const tBTA_GATTC_CHARACTERISTIC *p_char = BTA_GATTC_GetCharacteristic(op->conn_id, op->handle);
        mark_as_executing(conn_id);
        BTA_GATTC_WriteCharValue(op->conn_id, p_char->handle, op->write_type, op->len,
                                 op->p_value, BTA_GATT_AUTH_REQ_NONE);
        list_remove(gatt_op_queue, op);
    } else if(op->type == GATT_WRITE_DESC) {
        const tBTA_GATTC_DESCRIPTOR *p_desc = BTA_GATTC_GetDescriptor(op->conn_id, op->handle);
        tBTA_GATT_UNFMT value;
        value.len = op->len;
        value.p_value = op->p_value;
        mark_as_executing(conn_id);
        BTA_GATTC_WriteCharDescr(op->conn_id, p_desc->handle, BTA_GATTC_TYPE_WRITE,
                                 &value, BTA_GATT_AUTH_REQ_NONE);
        list_remove(gatt_op_queue, op);
    }
}

static void gatt_queue_read_op(uint8_t op_type, uint16_t conn_id, uint16_t handle)
{
    if(gatt_op_queue == NULL) {
        gatt_op_queue = list_new(GKI_freebuf);
    }

    gatt_operation *op = GKI_getbuf(sizeof(gatt_operation));
    op->type = op_type;
    op->conn_id = conn_id;
    op->handle = handle;
    list_append(gatt_op_queue, op);
    gatt_execute_next_op(conn_id);
}

static void gatt_queue_write_op(uint8_t op_type, uint16_t conn_id, uint16_t handle, uint16_t len,
                                uint8_t *p_value, tBTA_GATTC_WRITE_TYPE write_type)
{
    if(gatt_op_queue == NULL) {
        gatt_op_queue = list_new(GKI_freebuf);
    }

    gatt_operation *op = GKI_getbuf(sizeof(gatt_operation));
    op->type = op_type;
    op->conn_id = conn_id;
    op->handle = handle;
    op->write_type = write_type;
    op->len = len;
    wm_memcpy(op->p_value, p_value, len);
    list_append(gatt_op_queue, op);
    gatt_execute_next_op(conn_id);
}

#if BTA_HH_DEBUG == TRUE
static const char *bta_hh_le_rpt_name[4] = {
    "UNKNOWN",
    "INPUT",
    "OUTPUT",
    "FEATURE"
};

/*******************************************************************************
**
** Function         bta_hh_le_hid_report_dbg
**
** Description      debug function to print out all HID report available on remote
**                  device.
**
** Returns          void
**
*******************************************************************************/
static void bta_hh_le_hid_report_dbg(tBTA_HH_DEV_CB *p_cb)
{
    APPL_TRACE_DEBUG("%s: HID Report DB", __func__);

    if(!p_cb->hid_srvc.in_use) {
        return;
    }

    tBTA_HH_LE_RPT  *p_rpt = &p_cb->hid_srvc.report[0];

    for(int j = 0; j < BTA_HH_LE_RPT_MAX; j ++, p_rpt++) {
        char   *rpt_name = "Unknown";

        if(!p_rpt->in_use) {
            break;
        }

        if(p_rpt->uuid == GATT_UUID_HID_REPORT) {
            rpt_name = "Report";
        }

        if(p_rpt->uuid == GATT_UUID_HID_BT_KB_INPUT) {
            rpt_name = "Boot KB Input";
        }

        if(p_rpt->uuid == GATT_UUID_HID_BT_KB_OUTPUT) {
            rpt_name = "Boot KB Output";
        }

        if(p_rpt->uuid == GATT_UUID_HID_BT_MOUSE_INPUT) {
            rpt_name = "Boot MI Input";
        }

        APPL_TRACE_DEBUG("\t\t [%s- 0x%04x] [Type: %s], [ReportID: %d] [srvc_inst_id: %d] [char_inst_id: %d] [Clt_cfg: %d]",
                         rpt_name,
                         p_rpt->uuid,
                         ((p_rpt->rpt_type < 4) ? bta_hh_le_rpt_name[p_rpt->rpt_type] : "UNKNOWN"),
                         p_rpt->rpt_id,
                         p_rpt->srvc_inst_id,
                         p_rpt->char_inst_id,
                         p_rpt->client_cfg_value);
    }
}

/*******************************************************************************
**
** Function         bta_hh_uuid_to_str
**
** Description
**
** Returns          void
**
*******************************************************************************/
static char *bta_hh_uuid_to_str(uint16_t uuid)
{
    switch(uuid) {
        case GATT_UUID_HID_INFORMATION:
            return "GATT_UUID_HID_INFORMATION";

        case GATT_UUID_HID_REPORT_MAP:
            return "GATT_UUID_HID_REPORT_MAP";

        case GATT_UUID_HID_CONTROL_POINT:
            return "GATT_UUID_HID_CONTROL_POINT";

        case GATT_UUID_HID_REPORT:
            return "GATT_UUID_HID_REPORT";

        case GATT_UUID_HID_PROTO_MODE:
            return "GATT_UUID_HID_PROTO_MODE";

        case GATT_UUID_HID_BT_KB_INPUT:
            return "GATT_UUID_HID_BT_KB_INPUT";

        case GATT_UUID_HID_BT_KB_OUTPUT:
            return "GATT_UUID_HID_BT_KB_OUTPUT";

        case GATT_UUID_HID_BT_MOUSE_INPUT:
            return "GATT_UUID_HID_BT_MOUSE_INPUT";

        case GATT_UUID_CHAR_CLIENT_CONFIG:
            return "GATT_UUID_CHAR_CLIENT_CONFIG";

        case GATT_UUID_EXT_RPT_REF_DESCR:
            return "GATT_UUID_EXT_RPT_REF_DESCR";

        case GATT_UUID_RPT_REF_DESCR:
            return "GATT_UUID_RPT_REF_DESCR";

        default:
            return "Unknown UUID";
    }
}

#endif
/*******************************************************************************
**
** Function         bta_hh_le_enable
**
** Description      initialize LE HID related functionality
**
**
** Returns          void
**
*******************************************************************************/
void bta_hh_le_enable(void)
{
    char       app_name[LEN_UUID_128 + 1];
    tBT_UUID    app_uuid = {LEN_UUID_128, {0}};
    uint8_t       xx;
    bta_hh_cb.gatt_if = BTA_GATTS_INVALID_IF;

    for(xx = 0; xx < BTA_HH_MAX_DEVICE; xx ++) {
        bta_hh_cb.le_cb_index[xx]       = BTA_HH_IDX_INVALID;
    }

    wm_memset(app_name, 0, LEN_UUID_128 + 1);
    strncpy(app_name, "BTA HH OVER LE", LEN_UUID_128);
    wm_memcpy((void *)app_uuid.uu.uuid128, (void *)app_name, LEN_UUID_128);
    BTA_GATTC_AppRegister(&app_uuid, bta_hh_gattc_callback);
    return;
}

/*******************************************************************************
**
** Function         bta_hh_le_register_cmpl
**
** Description      BTA HH register with BTA GATTC completed
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_register_cmpl(tBTA_GATTC_REG *p_reg)
{
    tBTA_HH_STATUS      status = BTA_HH_ERR;

    if(p_reg->status == BTA_GATT_OK) {
        bta_hh_cb.gatt_if = p_reg->client_if;
        status = BTA_HH_OK;
    } else {
        bta_hh_cb.gatt_if = BTA_GATTS_INVALID_IF;
    }

    /* signal BTA call back event */
    (* bta_hh_cb.p_cback)(BTA_HH_ENABLE_EVT, (tBTA_HH *)&status);
}

/*******************************************************************************
**
** Function         bta_hh_le_is_hh_gatt_if
**
** Description      Check to see if client_if is BTA HH LE GATT interface
**
**
** Returns          whether it is HH GATT IF
**
*******************************************************************************/
uint8_t bta_hh_le_is_hh_gatt_if(tBTA_GATTC_IF client_if)
{
    return (bta_hh_cb.gatt_if == client_if);
}

/*******************************************************************************
**
** Function         bta_hh_le_deregister
**
** Description      De-register BTA HH from BTA GATTC
**
**
** Returns          void
**
*******************************************************************************/
void bta_hh_le_deregister(void)
{
    BTA_GATTC_AppDeregister(bta_hh_cb.gatt_if);
}

/*******************************************************************************
**
** Function         bta_hh_is_le_device
**
** Description      Check to see if the remote device is a LE only device
**
** Parameters:
**
*******************************************************************************/
uint8_t bta_hh_is_le_device(tBTA_HH_DEV_CB *p_cb, BD_ADDR remote_bda)
{
    p_cb->is_le_device = BTM_UseLeLink(remote_bda);
    return p_cb->is_le_device;
}

/*******************************************************************************
**
** Function         bta_hh_le_open_conn
**
** Description      open a GATT connection first.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_open_conn(tBTA_HH_DEV_CB *p_cb, BD_ADDR remote_bda)
{
    /* update cb_index[] map */
    p_cb->hid_handle = BTA_HH_GET_LE_DEV_HDL(p_cb->index);
    wm_memcpy(p_cb->addr, remote_bda, BD_ADDR_LEN);
    bta_hh_cb.le_cb_index[BTA_HH_GET_LE_CB_IDX(p_cb->hid_handle)] = p_cb->index;
    p_cb->in_use = TRUE;
    BTA_GATTC_Open(bta_hh_cb.gatt_if, remote_bda, TRUE, BTA_GATT_TRANSPORT_LE);
}

/*******************************************************************************
**
** Function         bta_hh_le_find_dev_cb_by_conn_id
**
** Description      Utility function find a device control block by connection ID.
**
*******************************************************************************/
tBTA_HH_DEV_CB *bta_hh_le_find_dev_cb_by_conn_id(uint16_t conn_id)
{
    uint8_t   i;
    tBTA_HH_DEV_CB *p_dev_cb = &bta_hh_cb.kdev[0];

    for(i = 0; i < BTA_HH_MAX_DEVICE; i ++, p_dev_cb ++) {
        if(p_dev_cb->in_use  && p_dev_cb->conn_id == conn_id) {
            return p_dev_cb;
        }
    }

    return NULL;
}

/*******************************************************************************
**
** Function         bta_hh_le_find_dev_cb_by_bda
**
** Description      Utility function find a device control block by BD address.
**
*******************************************************************************/
tBTA_HH_DEV_CB *bta_hh_le_find_dev_cb_by_bda(BD_ADDR bda)
{
    uint8_t   i;
    tBTA_HH_DEV_CB *p_dev_cb = &bta_hh_cb.kdev[0];

    for(i = 0; i < BTA_HH_MAX_DEVICE; i ++, p_dev_cb ++) {
        if(p_dev_cb->in_use  &&
                memcmp(p_dev_cb->addr, bda, BD_ADDR_LEN) == 0) {
            return p_dev_cb;
        }
    }

    return NULL;
}

/*******************************************************************************
**
** Function         bta_hh_le_find_service_inst_by_battery_inst_id
**
** Description      find HID service instance ID by battery service instance ID
**
*******************************************************************************/
uint8_t bta_hh_le_find_service_inst_by_battery_inst_id(tBTA_HH_DEV_CB *p_cb, uint8_t ba_inst_id)
{
    if(p_cb->hid_srvc.in_use &&
            p_cb->hid_srvc.incl_srvc_inst == ba_inst_id) {
        return p_cb->hid_srvc.srvc_inst_id;
    }

    return BTA_HH_IDX_INVALID;
}

/*******************************************************************************
**
** Function         bta_hh_le_find_report_entry
**
** Description      find the report entry by service instance and report UUID and
**                  instance ID
**
*******************************************************************************/
tBTA_HH_LE_RPT *bta_hh_le_find_report_entry(tBTA_HH_DEV_CB *p_cb,
        uint8_t  srvc_inst_id,  /* service instance ID */
        uint16_t rpt_uuid,
        uint8_t  char_inst_id)
{
    uint8_t   i;
    uint8_t   hid_inst_id = srvc_inst_id;
    tBTA_HH_LE_RPT *p_rpt;

    if(rpt_uuid == GATT_UUID_BATTERY_LEVEL) {
        hid_inst_id = bta_hh_le_find_service_inst_by_battery_inst_id(p_cb, srvc_inst_id);

        if(hid_inst_id == BTA_HH_IDX_INVALID) {
            return NULL;
        }
    }

    p_rpt = &p_cb->hid_srvc.report[0];

    for(i = 0; i < BTA_HH_LE_RPT_MAX; i ++, p_rpt ++) {
        if(p_rpt->uuid == rpt_uuid &&
                p_rpt->srvc_inst_id == srvc_inst_id &&
                p_rpt->char_inst_id == char_inst_id) {
            return p_rpt;
        }
    }

    return NULL;
}

/*******************************************************************************
**
** Function         bta_hh_le_find_rpt_by_idtype
**
** Description      find a report entry by report ID and protocol mode
**
** Returns          void
**
*******************************************************************************/
tBTA_HH_LE_RPT *bta_hh_le_find_rpt_by_idtype(tBTA_HH_LE_RPT *p_head, uint8_t mode,
        tBTA_HH_RPT_TYPE r_type, uint8_t rpt_id)
{
    tBTA_HH_LE_RPT *p_rpt = p_head;
    uint8_t   i;
#if BTA_HH_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hh_le_find_rpt_by_idtype: r_type: %d rpt_id: %d", r_type, rpt_id);
#endif

    for(i = 0 ; i < BTA_HH_LE_RPT_MAX; i ++, p_rpt++) {
        if(p_rpt->in_use && p_rpt->rpt_id == rpt_id && r_type == p_rpt->rpt_type) {
            /* return battery report w/o condition */
            if(p_rpt->uuid == GATT_UUID_BATTERY_LEVEL) {
                return p_rpt;
            }

            if(mode == BTA_HH_PROTO_RPT_MODE && p_rpt->uuid == GATT_UUID_HID_REPORT) {
                return p_rpt;
            }

            if(mode == BTA_HH_PROTO_BOOT_MODE &&
                    (p_rpt->uuid >= GATT_UUID_HID_BT_KB_INPUT && p_rpt->uuid <= GATT_UUID_HID_BT_MOUSE_INPUT)) {
                return p_rpt;
            }
        }
    }

    return NULL;
}

/*******************************************************************************
**
** Function         bta_hh_le_find_alloc_report_entry
**
** Description      find or allocate a report entry in the HID service report list.
**
*******************************************************************************/
tBTA_HH_LE_RPT *bta_hh_le_find_alloc_report_entry(tBTA_HH_DEV_CB *p_cb,
        uint8_t srvc_inst_id,
        uint16_t rpt_uuid,
        uint8_t  inst_id)
{
    uint8_t   i, hid_inst_id = srvc_inst_id;
    tBTA_HH_LE_RPT *p_rpt;

    if(rpt_uuid == GATT_UUID_BATTERY_LEVEL) {
        hid_inst_id = bta_hh_le_find_service_inst_by_battery_inst_id(p_cb, srvc_inst_id);

        if(hid_inst_id == BTA_HH_IDX_INVALID) {
            return NULL;
        }
    }

    p_rpt = &p_cb->hid_srvc.report[0];

    for(i = 0; i < BTA_HH_LE_RPT_MAX; i ++, p_rpt ++) {
        if(!p_rpt->in_use ||
                (p_rpt->uuid == rpt_uuid &&
                 p_rpt->srvc_inst_id == srvc_inst_id &&
                 p_rpt->char_inst_id == inst_id)) {
            if(!p_rpt->in_use) {
                p_rpt->in_use   = TRUE;
                p_rpt->index    = i;
                p_rpt->srvc_inst_id = srvc_inst_id;
                p_rpt->char_inst_id = inst_id;
                p_rpt->uuid     = rpt_uuid;

                /* assign report type */
                for(i = 0; i < BTA_LE_HID_RTP_UUID_MAX; i ++) {
                    if(bta_hh_uuid_to_rtp_type[i][0] == rpt_uuid) {
                        p_rpt->rpt_type = (tBTA_HH_RPT_TYPE)bta_hh_uuid_to_rtp_type[i][1];

                        if(rpt_uuid == GATT_UUID_HID_BT_KB_INPUT || rpt_uuid == GATT_UUID_HID_BT_KB_OUTPUT) {
                            p_rpt->rpt_id = BTA_HH_KEYBD_RPT_ID;
                        }

                        if(rpt_uuid == GATT_UUID_HID_BT_MOUSE_INPUT) {
                            p_rpt->rpt_id = BTA_HH_MOUSE_RPT_ID;
                        }

                        break;
                    }
                }
            }

            return p_rpt;
        }
    }

    return NULL;
}

static tBTA_GATTC_DESCRIPTOR *find_descriptor_by_short_uuid(uint16_t conn_id,
        uint16_t char_handle,
        uint16_t short_uuid)
{
    const tBTA_GATTC_CHARACTERISTIC *p_char =
                    BTA_GATTC_GetCharacteristic(conn_id, char_handle);

    if(!p_char) {
        LOG_WARN(LOG_TAG, "%s No such characteristic: %d", __func__, char_handle);
        return NULL;
    }

    if(!p_char->descriptors || list_is_empty(p_char->descriptors)) {
        return NULL;
    }

    for(list_node_t *dn = list_begin(p_char->descriptors);
            dn != list_end(p_char->descriptors); dn = list_next(dn)) {
        tBTA_GATTC_DESCRIPTOR *p_desc = list_node(dn);

        if(p_char->uuid.len == LEN_UUID_16 &&
                p_desc->uuid.uu.uuid16 == short_uuid) {
            return p_desc;
        }
    }

    return NULL;
}

/*******************************************************************************
**
** Function         bta_hh_le_read_char_dscrpt
**
** Description      read characteristic descriptor
**
*******************************************************************************/
static tBTA_HH_STATUS bta_hh_le_read_char_dscrpt(tBTA_HH_DEV_CB *p_cb, uint16_t char_handle,
        uint16_t short_uuid)
{
    const tBTA_GATTC_DESCRIPTOR *p_desc = find_descriptor_by_short_uuid(p_cb->conn_id, char_handle,
                                          short_uuid);

    if(!p_desc) {
        return BTA_HH_ERR;
    }

    gatt_queue_read_op(GATT_READ_DESC, p_cb->conn_id, p_desc->handle);
    return BTA_HH_OK;
}

/*******************************************************************************
**
** Function         bta_hh_le_save_rpt_ref
**
** Description      save report reference information and move to next one.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_save_rpt_ref(tBTA_HH_DEV_CB *p_dev_cb, tBTA_HH_LE_RPT  *p_rpt,
                            tBTA_GATTC_READ *p_data)
{
    /* if the length of the descriptor value is right, parse it */
    if(p_data->status == BTA_GATT_OK &&
            p_data->p_value && p_data->p_value->len == 2) {
        uint8_t *pp = p_data->p_value->p_value;
        STREAM_TO_UINT8(p_rpt->rpt_id, pp);
        STREAM_TO_UINT8(p_rpt->rpt_type, pp);

        if(p_rpt->rpt_type > BTA_HH_RPTT_FEATURE) { /* invalid report type */
            p_rpt->rpt_type = BTA_HH_RPTT_RESRV;
        }

#if BTA_HH_DEBUG == TRUE
        APPL_TRACE_DEBUG("%s: report ID: %d", __func__, p_rpt->rpt_id);
#endif
        tBTA_HH_RPT_CACHE_ENTRY rpt_entry;
        rpt_entry.rpt_id    = p_rpt->rpt_id;
        rpt_entry.rpt_type  = p_rpt->rpt_type;
        rpt_entry.rpt_uuid  = p_rpt->uuid;
        rpt_entry.srvc_inst_id = p_rpt->srvc_inst_id;
        rpt_entry.char_inst_id = p_rpt->char_inst_id;
        bta_hh_le_co_rpt_info(p_dev_cb->addr,
                              &rpt_entry,
                              p_dev_cb->app_id);
    } else if(p_data->status == BTA_GATT_INSUF_AUTHENTICATION) {
        /* close connection right away */
        p_dev_cb->status = BTA_HH_ERR_AUTH_FAILED;
        /* close the connection and report service discovery complete with error */
        bta_hh_le_api_disc_act(p_dev_cb);
        return;
    }

    if(p_rpt->index < BTA_HH_LE_RPT_MAX - 1) {
        p_rpt ++;
    } else {
        p_rpt = NULL;
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_save_rpt_ref
**
** Description      save report reference information and move to next one.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_save_ext_rpt_ref(tBTA_HH_DEV_CB *p_dev_cb,
                                tBTA_GATTC_READ *p_data)
{
    /* if the length of the descriptor value is right, parse it
      assume it's a 16 bits UUID */
    if(p_data->status == BTA_GATT_OK &&
            p_data->p_value && p_data->p_value->len == 2) {
        uint8_t *pp = p_data->p_value->p_value;
        STREAM_TO_UINT16(p_dev_cb->hid_srvc.ext_rpt_ref, pp);
#if BTA_HH_DEBUG == TRUE
        APPL_TRACE_DEBUG("%s: External Report Reference UUID 0x%04x", __func__,
                         p_dev_cb->hid_srvc.ext_rpt_ref);
#endif
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_register_input_notif
**
** Description      Register for all notifications for the report applicable
**                  for the protocol mode.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_register_input_notif(tBTA_HH_DEV_CB *p_dev_cb, uint8_t proto_mode,
                                    uint8_t register_ba)
{
    tBTA_HH_LE_RPT  *p_rpt = &p_dev_cb->hid_srvc.report[0];
#if BTA_HH_DEBUG == TRUE
    APPL_TRACE_DEBUG("%s: bta_hh_le_register_input_notif mode: %d", __func__, proto_mode);
#endif

    for(int i = 0; i < BTA_HH_LE_RPT_MAX; i ++, p_rpt ++) {
        if(p_rpt->rpt_type == BTA_HH_RPTT_INPUT) {
            if(register_ba && p_rpt->uuid == GATT_UUID_BATTERY_LEVEL) {
                BTA_GATTC_RegisterForNotifications(bta_hh_cb.gatt_if, p_dev_cb->addr,
                                                   p_rpt->char_inst_id);
            }
            /* boot mode, deregister report input notification */
            else if(proto_mode == BTA_HH_PROTO_BOOT_MODE) {
                if(p_rpt->uuid == GATT_UUID_HID_REPORT &&
                        p_rpt->client_cfg_value == BTA_GATT_CLT_CONFIG_NOTIFICATION) {
                    APPL_TRACE_DEBUG("---> Deregister Report ID: %d", p_rpt->rpt_id);
                    BTA_GATTC_DeregisterForNotifications(bta_hh_cb.gatt_if, p_dev_cb->addr,
                                                         p_rpt->char_inst_id);
                }
                /* register boot reports notification */
                else if(p_rpt->uuid == GATT_UUID_HID_BT_KB_INPUT ||
                        p_rpt->uuid == GATT_UUID_HID_BT_MOUSE_INPUT) {
                    APPL_TRACE_DEBUG("<--- Register Boot Report ID: %d", p_rpt->rpt_id);
                    BTA_GATTC_RegisterForNotifications(bta_hh_cb.gatt_if, p_dev_cb->addr,
                                                       p_rpt->char_inst_id);
                }
            } else if(proto_mode == BTA_HH_PROTO_RPT_MODE) {
                if((p_rpt->uuid == GATT_UUID_HID_BT_KB_INPUT ||
                        p_rpt->uuid == GATT_UUID_HID_BT_MOUSE_INPUT) &&
                        p_rpt->client_cfg_value == BTA_GATT_CLT_CONFIG_NOTIFICATION) {
                    APPL_TRACE_DEBUG("---> Deregister Boot Report ID: %d", p_rpt->rpt_id);
                    BTA_GATTC_DeregisterForNotifications(bta_hh_cb.gatt_if, p_dev_cb->addr,
                                                         p_rpt->char_inst_id);
                } else if(p_rpt->uuid == GATT_UUID_HID_REPORT &&
                          p_rpt->client_cfg_value == BTA_GATT_CLT_CONFIG_NOTIFICATION) {
                    APPL_TRACE_DEBUG("<--- Register Report ID: %d", p_rpt->rpt_id);
                    BTA_GATTC_RegisterForNotifications(bta_hh_cb.gatt_if, p_dev_cb->addr,
                                                       p_rpt->char_inst_id);
                }
            }

            /*
            else unknow protocol mode */
        }
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_open_cmpl
**
** Description      HID over GATT connection sucessfully opened
**
*******************************************************************************/
void bta_hh_le_open_cmpl(tBTA_HH_DEV_CB *p_cb)
{
    if(p_cb->disc_active == BTA_HH_LE_DISC_NONE) {
#if BTA_HH_DEBUG
        bta_hh_le_hid_report_dbg(p_cb);
#endif
        bta_hh_le_register_input_notif(p_cb, p_cb->mode, TRUE);
        bta_hh_sm_execute(p_cb, BTA_HH_OPEN_CMPL_EVT, NULL);
#if (BTA_HH_LE_RECONN == TRUE)

        if(p_cb->status == BTA_HH_OK) {
            bta_hh_le_add_dev_bg_conn(p_cb, TRUE);
        }

#endif
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_write_char_clt_cfg
**
** Description      Utility function to find and write client configuration of
**                  a characteristic
**
*******************************************************************************/
uint8_t bta_hh_le_write_char_clt_cfg(tBTA_HH_DEV_CB *p_cb,
                                     uint8_t char_handle,
                                     uint16_t clt_cfg_value)
{
    uint8_t                      buf[2], *pp = buf;
    tBTA_GATTC_DESCRIPTOR *p_desc = find_descriptor_by_short_uuid(p_cb->conn_id,
                                    char_handle, GATT_UUID_CHAR_CLIENT_CONFIG);

    if(!p_desc) {
        return FALSE;
    }

    UINT16_TO_STREAM(pp, clt_cfg_value);
    gatt_queue_write_op(GATT_WRITE_DESC, p_cb->conn_id,
                        p_desc->handle, 2, buf, BTA_GATTC_TYPE_WRITE);
    return TRUE;
}

/*******************************************************************************
**
** Function         bta_hh_le_write_rpt_clt_cfg
**
** Description      write client configuration. This is only for input report
**                  enable all input notification upon connection open.
**
*******************************************************************************/
uint8_t bta_hh_le_write_rpt_clt_cfg(tBTA_HH_DEV_CB *p_cb, uint8_t srvc_inst_id)
{
    uint8_t           i;
    tBTA_HH_LE_RPT  *p_rpt = &p_cb->hid_srvc.report[p_cb->clt_cfg_idx];

    for(i = p_cb->clt_cfg_idx; i < BTA_HH_LE_RPT_MAX && p_rpt->in_use; i ++, p_rpt ++) {
        /* enable notification for all input report, regardless mode */
        if(p_rpt->rpt_type == BTA_HH_RPTT_INPUT) {
            if(bta_hh_le_write_char_clt_cfg(p_cb,
                                            p_rpt->char_inst_id,
                                            BTA_GATT_CLT_CONFIG_NOTIFICATION)) {
                p_cb->clt_cfg_idx = i;
                return TRUE;
            }
        }
    }

    p_cb->clt_cfg_idx = 0;

    /* client configuration is completed, send open callback */
    if(p_cb->state == BTA_HH_W4_CONN_ST) {
        p_cb->disc_active &= ~BTA_HH_LE_DISC_HIDS;
        bta_hh_le_open_cmpl(p_cb);
    }

    return FALSE;
}

/*******************************************************************************
**
** Function         bta_hh_le_set_protocol_mode
**
** Description      Set remote device protocol mode.
**
*******************************************************************************/
uint8_t bta_hh_le_set_protocol_mode(tBTA_HH_DEV_CB *p_cb, tBTA_HH_PROTO_MODE mode)
{
    tBTA_HH_CBDATA      cback_data;
    uint8_t             exec = FALSE;
    APPL_TRACE_DEBUG("%s attempt mode: %s", __func__,
                     (mode == BTA_HH_PROTO_RPT_MODE) ? "Report" : "Boot");
    cback_data.handle  = p_cb->hid_handle;

    /* boot mode is not supported in the remote device */
    if(p_cb->hid_srvc.proto_mode_handle == 0) {
        p_cb->mode  = BTA_HH_PROTO_RPT_MODE;

        if(mode == BTA_HH_PROTO_BOOT_MODE) {
            APPL_TRACE_ERROR("Set Boot Mode failed!! No PROTO_MODE Char!");
            cback_data.status = BTA_HH_ERR;
        } else {
            /* if set to report mode, need to de-register all input report notification */
            bta_hh_le_register_input_notif(p_cb, p_cb->mode, FALSE);
            cback_data.status = BTA_HH_OK;
        }

        if(p_cb->state == BTA_HH_W4_CONN_ST) {
            p_cb->status = (cback_data.status == BTA_HH_OK) ? BTA_HH_OK : BTA_HH_ERR_PROTO;
        } else {
            (* bta_hh_cb.p_cback)(BTA_HH_SET_PROTO_EVT, (tBTA_HH *)&cback_data);
        }
    } else if(p_cb->mode != mode) {
        p_cb->mode = mode;
        mode = (mode == BTA_HH_PROTO_BOOT_MODE) ? BTA_HH_LE_PROTO_BOOT_MODE : BTA_HH_LE_PROTO_REPORT_MODE;
        gatt_queue_write_op(GATT_WRITE_CHAR, p_cb->conn_id, p_cb->hid_srvc.proto_mode_handle, 1,
                            &mode, BTA_GATTC_TYPE_WRITE_NO_RSP);
        exec        = TRUE;
    }

    return exec;
}

/*******************************************************************************
**
** Function         bta_hh_le_get_protocol_mode
**
** Description      Get remote device protocol mode.
**
*******************************************************************************/
void bta_hh_le_get_protocol_mode(tBTA_HH_DEV_CB *p_cb)
{
    tBTA_HH_HSDATA    hs_data;
    p_cb->w4_evt = BTA_HH_GET_PROTO_EVT;

    if(p_cb->hid_srvc.in_use && p_cb->hid_srvc.proto_mode_handle != 0) {
        gatt_queue_read_op(GATT_READ_CHAR, p_cb->conn_id, p_cb->hid_srvc.proto_mode_handle);
        return;
    }

    /* no service support protocol_mode, by default report mode */
    hs_data.status  = BTA_HH_OK;
    hs_data.handle  = p_cb->hid_handle;
    hs_data.rsp_data.proto_mode = BTA_HH_PROTO_RPT_MODE;
    p_cb->w4_evt = 0;
    (* bta_hh_cb.p_cback)(BTA_HH_GET_PROTO_EVT, (tBTA_HH *)&hs_data);
}

/*******************************************************************************
**
** Function         bta_hh_le_dis_cback
**
** Description      DIS read complete callback
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_dis_cback(BD_ADDR addr, tDIS_VALUE *p_dis_value)
{
    tBTA_HH_DEV_CB *p_cb = bta_hh_le_find_dev_cb_by_bda(addr);

    if(p_cb == NULL || p_dis_value == NULL) {
        APPL_TRACE_ERROR("received unexpected/error DIS callback");
        return;
    }

    p_cb->disc_active &= ~BTA_HH_LE_DISC_DIS;

    /* plug in the PnP info for this device */
    if(p_dis_value->attr_mask & DIS_ATTR_PNP_ID_BIT) {
#if BTA_HH_DEBUG == TRUE
        APPL_TRACE_DEBUG("Plug in PnP info: product_id = %02x, vendor_id = %04x, version = %04x",
                         p_dis_value->pnp_id.product_id,
                         p_dis_value->pnp_id.vendor_id,
                         p_dis_value->pnp_id.product_version);
#endif
        p_cb->dscp_info.product_id = p_dis_value->pnp_id.product_id;
        p_cb->dscp_info.vendor_id  = p_dis_value->pnp_id.vendor_id;
        p_cb->dscp_info.version    = p_dis_value->pnp_id.product_version;
    }

    bta_hh_le_open_cmpl(p_cb);
}

/*******************************************************************************
**
** Function         bta_hh_le_pri_service_discovery
**
** Description      Initialize GATT discovery on the remote LE HID device by opening
**                  a GATT connection first.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_pri_service_discovery(tBTA_HH_DEV_CB *p_cb)
{
    tBT_UUID        pri_srvc;
    bta_hh_le_co_reset_rpt_cache(p_cb->addr, p_cb->app_id);
    p_cb->disc_active |= (BTA_HH_LE_DISC_HIDS | BTA_HH_LE_DISC_DIS);

    /* read DIS info */
    if(!DIS_ReadDISInfo(p_cb->addr, bta_hh_le_dis_cback, DIS_ATTR_PNP_ID_BIT)) {
        APPL_TRACE_ERROR("read DIS failed");
        p_cb->disc_active &= ~BTA_HH_LE_DISC_DIS;
    }

    /* in parallel */
    /* start primary service discovery for HID service */
    pri_srvc.len        = LEN_UUID_16;
    pri_srvc.uu.uuid16  = UUID_SERVCLASS_LE_HID;
    BTA_GATTC_ServiceSearchRequest(p_cb->conn_id, &pri_srvc);
    return;
}

/*******************************************************************************
**
** Function         bta_hh_le_encrypt_cback
**
** Description      link encryption complete callback for bond verification.
**
** Returns          None
**
*******************************************************************************/
void bta_hh_le_encrypt_cback(BD_ADDR bd_addr, tBTA_GATT_TRANSPORT transport,
                             void *p_ref_data, tBTM_STATUS result)
{
    uint8_t   idx = bta_hh_find_cb(bd_addr);
    tBTA_HH_DEV_CB *p_dev_cb;
    UNUSED(p_ref_data);
    UNUSED(transport);

    if(idx != BTA_HH_IDX_INVALID) {
        p_dev_cb = &bta_hh_cb.kdev[idx];
    } else {
        APPL_TRACE_ERROR("unexpected encryption callback, ignore");
        return;
    }

    p_dev_cb->status = (result == BTM_SUCCESS) ? BTA_HH_OK : BTA_HH_ERR_SEC;
    p_dev_cb->reason = result;
    bta_hh_sm_execute(p_dev_cb, BTA_HH_ENC_CMPL_EVT, NULL);
}

/*******************************************************************************
**
** Function         bta_hh_security_cmpl
**
** Description      Security check completed, start the service discovery
**                  if no cache available, otherwise report connection open completed
**
** Parameters:
**
*******************************************************************************/
void bta_hh_security_cmpl(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf)
{
    UNUSED(p_buf);
    APPL_TRACE_DEBUG("%s", __func__);

    if(p_cb->status == BTA_HH_OK) {
        if(!p_cb->hid_srvc.in_use) {
            APPL_TRACE_DEBUG("bta_hh_security_cmpl no reports loaded, try to load");
            /* start loading the cache if not in stack */
            //TODO(jpawlowski): cache storage is broken, fix it
            // tBTA_HH_RPT_CACHE_ENTRY     *p_rpt_cache;
            // uint8_t                       num_rpt = 0;
            // if ((p_rpt_cache = bta_hh_le_co_cache_load(p_cb->addr, &num_rpt, p_cb->app_id)) != NULL)
            // {
            //     bta_hh_process_cache_rpt(p_cb, p_rpt_cache, num_rpt);
            // }
        }

        /*  discovery has been done for HID service */
        if(p_cb->app_id != 0 && p_cb->hid_srvc.in_use) {
            APPL_TRACE_DEBUG("%s: discovery has been done for HID service", __func__);

            /* configure protocol mode */
            if(bta_hh_le_set_protocol_mode(p_cb, p_cb->mode) == FALSE) {
                bta_hh_le_open_cmpl(p_cb);
            }
        }
        /* start primary service discovery for HID service */
        else {
            APPL_TRACE_DEBUG("%s: Starting service discovery", __func__);
            bta_hh_le_pri_service_discovery(p_cb);
        }
    } else {
        APPL_TRACE_ERROR("%s() - encryption failed; status=0x%04x, reason=0x%04x",
                         __FUNCTION__, p_cb->status, p_cb->reason);

        if(!(p_cb->status == BTA_HH_ERR_SEC && p_cb->reason == BTM_ERR_PROCESSING)) {
            bta_hh_le_api_disc_act(p_cb);
        }
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_notify_enc_cmpl
**
** Description      process GATT encryption complete event
**
** Returns
**
*******************************************************************************/
void bta_hh_le_notify_enc_cmpl(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf)
{
    if(p_cb == NULL || p_cb->security_pending == FALSE ||
            p_buf == NULL || p_buf->le_enc_cmpl.client_if != bta_hh_cb.gatt_if) {
        return;
    }

    p_cb->security_pending = FALSE;
    bta_hh_start_security(p_cb, NULL);
}

/*******************************************************************************
**
** Function         bta_hh_clear_service_cache
**
** Description      clear the service cache
**
** Parameters:
**
*******************************************************************************/
void bta_hh_clear_service_cache(tBTA_HH_DEV_CB *p_cb)
{
    tBTA_HH_LE_HID_SRVC     *p_hid_srvc = &p_cb->hid_srvc;
    p_cb->app_id = 0;
    p_cb->dscp_info.descriptor.dsc_list = NULL;
    GKI_free_and_reset_buf((void **)&p_hid_srvc->rpt_map);
    wm_memset(p_hid_srvc, 0, sizeof(tBTA_HH_LE_HID_SRVC));
}

/*******************************************************************************
**
** Function         bta_hh_start_security
**
** Description      start the security check of the established connection
**
** Parameters:
**
*******************************************************************************/
void bta_hh_start_security(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf)
{
    uint8_t           sec_flag = 0;
    tBTM_SEC_DEV_REC  *p_dev_rec;
    UNUSED(p_buf);
    p_dev_rec = btm_find_dev(p_cb->addr);

    if(p_dev_rec) {
        if(p_dev_rec->sec_state == BTM_SEC_STATE_ENCRYPTING ||
                p_dev_rec->sec_state == BTM_SEC_STATE_AUTHENTICATING) {
            /* if security collision happened, wait for encryption done */
            p_cb->security_pending = TRUE;
            return;
        }
    }

    /* verify bond */
    BTM_GetSecurityFlagsByTransport(p_cb->addr, &sec_flag, BT_TRANSPORT_LE);

    /* if link has been encrypted */
    if(sec_flag & BTM_SEC_FLAG_ENCRYPTED) {
        bta_hh_sm_execute(p_cb, BTA_HH_ENC_CMPL_EVT, NULL);
    }
    /* if bonded and link not encrypted */
    else if(sec_flag & BTM_SEC_FLAG_LKEY_KNOWN) {
        sec_flag = BTM_BLE_SEC_ENCRYPT;
        p_cb->status = BTA_HH_ERR_AUTH_FAILED;
        BTM_SetEncryption(p_cb->addr, BTA_TRANSPORT_LE, bta_hh_le_encrypt_cback, NULL, sec_flag);
    }
    /* unbonded device, report security error here */
    else if(p_cb->sec_mask != BTA_SEC_NONE) {
        sec_flag = BTM_BLE_SEC_ENCRYPT_NO_MITM;
        p_cb->status = BTA_HH_ERR_AUTH_FAILED;
        bta_hh_clear_service_cache(p_cb);
        BTM_SetEncryption(p_cb->addr, BTA_TRANSPORT_LE, bta_hh_le_encrypt_cback, NULL, sec_flag);
    }
    /* otherwise let it go through */
    else {
        bta_hh_sm_execute(p_cb, BTA_HH_ENC_CMPL_EVT, NULL);
    }
}

/*******************************************************************************
**
** Function         bta_hh_gatt_open
**
** Description      process GATT open event.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_gatt_open(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_buf)
{
    tBTA_GATTC_OPEN *p_data = &p_buf->le_open;
    uint8_t           *p2;
    tHID_STATUS     status = BTA_HH_ERR;

    /* if received invalid callback data , ignore it */
    if(p_cb == NULL || p_data == NULL) {
        return;
    }

    p2 = p_data->remote_bda;
    APPL_TRACE_DEBUG("bta_hh_gatt_open BTA_GATTC_OPEN_EVT bda= [%08x%04x] status =%d",
                     ((p2[0]) << 24) + ((p2[1]) << 16) + ((p2[2]) << 8) + (p2[3]),
                     ((p2[4]) << 8) + p2[5], p_data->status);

    if(p_data->status == BTA_GATT_OK) {
        p_cb->is_le_device  = TRUE;
        p_cb->in_use    = TRUE;
        p_cb->conn_id   = p_data->conn_id;
        p_cb->hid_handle = BTA_HH_GET_LE_DEV_HDL(p_cb->index);
        bta_hh_cb.le_cb_index[BTA_HH_GET_LE_CB_IDX(p_cb->hid_handle)] = p_cb->index;
        gatt_op_queue_clean(p_cb->conn_id);
#if BTA_HH_DEBUG == TRUE
        APPL_TRACE_DEBUG("hid_handle = %2x conn_id = %04x cb_index = %d", p_cb->hid_handle, p_cb->conn_id,
                         p_cb->index);
#endif
        bta_hh_sm_execute(p_cb, BTA_HH_START_ENC_EVT, NULL);
    } else { /* open failure */
        bta_hh_sm_execute(p_cb, BTA_HH_SDP_CMPL_EVT, (tBTA_HH_DATA *)&status);
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_close
**
** Description      This function process the GATT close event and post it as a
**                  BTA HH internal event
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_close(tBTA_GATTC_CLOSE *p_data)
{
    tBTA_HH_DEV_CB *p_dev_cb = bta_hh_le_find_dev_cb_by_bda(p_data->remote_bda);
    uint16_t  sm_event = BTA_HH_GATT_CLOSE_EVT;

    if(p_dev_cb != NULL) {
        tBTA_HH_LE_CLOSE *p_buf =
                        (tBTA_HH_LE_CLOSE *)GKI_getbuf(sizeof(tBTA_HH_LE_CLOSE));
        p_buf->hdr.event = sm_event;
        p_buf->hdr.layer_specific = (uint16_t)p_dev_cb->hid_handle;
        p_buf->conn_id = p_data->conn_id;
        p_buf->reason = p_data->reason;
        p_dev_cb->conn_id = BTA_GATT_INVALID_CONN_ID;
        p_dev_cb->security_pending = FALSE;
        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_gatt_disc_cmpl
**
** Description      Check to see if the remote device is a LE only device
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_gatt_disc_cmpl(tBTA_HH_DEV_CB *p_cb, tBTA_HH_STATUS status)
{
    APPL_TRACE_DEBUG("bta_hh_le_gatt_disc_cmpl ");

    /* if open sucessful or protocol mode not desired, keep the connection open but inform app */
    if(status == BTA_HH_OK || status == BTA_HH_ERR_PROTO) {
        /* assign a special APP ID temp, since device type unknown */
        p_cb->app_id = BTA_HH_APP_ID_LE;
        /* set report notification configuration */
        p_cb->clt_cfg_idx = 0;
        bta_hh_le_write_rpt_clt_cfg(p_cb, p_cb->hid_srvc.srvc_inst_id);
    } else { /* error, close the GATT connection */
        /* close GATT connection if it's on */
        bta_hh_le_api_disc_act(p_cb);
    }
}

void process_included(tBTA_HH_DEV_CB *p_dev_cb, tBTA_GATTC_SERVICE *service)
{
    if(!service->included_svc || list_is_empty(service->included_svc)) {
        APPL_TRACE_ERROR("%s: Remote device does not have battery level", __func__);
        return;
    }

    for(list_node_t *isn = list_begin(service->included_svc);
            isn != list_end(service->included_svc); isn = list_next(isn)) {
        tBTA_GATTC_INCLUDED_SVC *p_isvc = list_node(isn);
        tBTA_GATTC_SERVICE *incl_svc = p_isvc->included_service;

        if(incl_svc->uuid.uu.uuid16 == UUID_SERVCLASS_BATTERY &&
                incl_svc->uuid.len == LEN_UUID_16) {
            /* read include service UUID */
            p_dev_cb->hid_srvc.incl_srvc_inst = incl_svc->handle;

            for(list_node_t *cn = list_begin(incl_svc->characteristics);
                    cn != list_end(incl_svc->characteristics); cn = list_next(cn)) {
                tBTA_GATTC_CHARACTERISTIC *p_char = list_node(cn);

                if(p_char->uuid.uu.uuid16 == GATT_UUID_BATTERY_LEVEL &&
                        p_char->uuid.len == LEN_UUID_16) {
                    if(bta_hh_le_find_alloc_report_entry(p_dev_cb,
                                                         incl_svc->s_handle,
                                                         GATT_UUID_BATTERY_LEVEL,
                                                         p_char->handle) == NULL) {
                        APPL_TRACE_ERROR("Add battery report entry failed !!!");
                    }

                    gatt_queue_read_op(GATT_READ_CHAR, p_dev_cb->conn_id, p_char->handle);
                    return;
                }
            }
        }
    }

    APPL_TRACE_ERROR("%s: Remote device does not have battery level", __func__);
}

/*******************************************************************************
**
** Function         bta_hh_le_search_hid_chars
**
** Description      This function discover all characteristics a service and
**                  all descriptors available.
**
** Parameters:
**
*******************************************************************************/
static void bta_hh_le_search_hid_chars(tBTA_HH_DEV_CB *p_dev_cb, tBTA_GATTC_SERVICE *service)
{
    tBTA_HH_LE_RPT *p_rpt;

    for(list_node_t *cn = list_begin(service->characteristics);
            cn != list_end(service->characteristics); cn = list_next(cn)) {
        tBTA_GATTC_CHARACTERISTIC *p_char = list_node(cn);

        if(p_char->uuid.len != LEN_UUID_16) {
            continue;
        }

        LOG_DEBUG(LOG_TAG, "%s: %s 0x%04d", __func__, bta_hh_uuid_to_str(p_char->uuid.uu.uuid16),
                  p_char->uuid.uu.uuid16);

        switch(p_char->uuid.uu.uuid16) {
            case GATT_UUID_HID_CONTROL_POINT:
                p_dev_cb->hid_srvc.control_point_handle = p_char->handle;
                break;

            case GATT_UUID_HID_INFORMATION:
            case GATT_UUID_HID_REPORT_MAP:
                /* read the char value */
                gatt_queue_read_op(GATT_READ_CHAR, p_dev_cb->conn_id, p_char->handle);
                bta_hh_le_read_char_dscrpt(p_dev_cb, p_char->handle,
                                           GATT_UUID_EXT_RPT_REF_DESCR);
                break;

            case GATT_UUID_HID_REPORT:
                p_rpt = bta_hh_le_find_alloc_report_entry(p_dev_cb,
                        p_dev_cb->hid_srvc.srvc_inst_id,
                        GATT_UUID_HID_REPORT,
                        p_char->handle);

                if(p_rpt == NULL) {
                    APPL_TRACE_ERROR("%s: Add report entry failed !!!", __func__);
                    break;
                }

                if(p_rpt->rpt_type != BTA_HH_RPTT_INPUT) {
                    break;
                }

                bta_hh_le_read_char_dscrpt(p_dev_cb, p_char->handle, GATT_UUID_RPT_REF_DESCR);
                break;

            /* found boot mode report types */
            case GATT_UUID_HID_BT_KB_OUTPUT:
            case GATT_UUID_HID_BT_MOUSE_INPUT:
            case GATT_UUID_HID_BT_KB_INPUT:
                if(bta_hh_le_find_alloc_report_entry(p_dev_cb,
                                                     service->handle,
                                                     p_char->uuid.uu.uuid16,
                                                     p_char->handle) == NULL) {
                    APPL_TRACE_ERROR("%s: Add report entry failed !!!", __func__);
                }

                break;

            default:
                APPL_TRACE_DEBUG("%s: not processing %s 0x%04d", __func__,
                                 bta_hh_uuid_to_str(p_char->uuid.uu.uuid16),
                                 p_char->uuid.uu.uuid16);
        }
    }

    /* Make sure PROTO_MODE is processed as last */
    for(list_node_t *cn = list_begin(service->characteristics);
            cn != list_end(service->characteristics); cn = list_next(cn)) {
        tBTA_GATTC_CHARACTERISTIC *p_char = list_node(cn);

        if(p_char->uuid.len != LEN_UUID_16 &&
                p_char->uuid.uu.uuid16 == GATT_UUID_HID_PROTO_MODE) {
            p_dev_cb->hid_srvc.proto_mode_handle = p_char->handle;
            bta_hh_le_set_protocol_mode(p_dev_cb, p_dev_cb->mode);
            break;
        }
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_srvc_search_cmpl
**
** Description      This function process the GATT service search complete.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_srvc_search_cmpl(tBTA_GATTC_SEARCH_CMPL *p_data)
{
    tBTA_HH_DEV_CB *p_dev_cb = bta_hh_le_find_dev_cb_by_conn_id(p_data->conn_id);

    /* service search exception or no HID service is supported on remote */
    if(p_dev_cb == NULL) {
        return;
    }

    if(p_data->status != BTA_GATT_OK) {
        p_dev_cb->status = BTA_HH_ERR_SDP;
        /* close the connection and report service discovery complete with error */
        bta_hh_le_api_disc_act(p_dev_cb);
        return;
    }

    const list_t *services = BTA_GATTC_GetServices(p_data->conn_id);
    uint8_t have_hid = false;

    for(list_node_t *sn = list_begin(services);
            sn != list_end(services); sn = list_next(sn)) {
        tBTA_GATTC_SERVICE *service = list_node(sn);

        if(service->uuid.uu.uuid16 == UUID_SERVCLASS_LE_HID &&
                service->is_primary && !have_hid) {
            have_hid = true;
            /* found HID primamry service */
            p_dev_cb->hid_srvc.in_use = TRUE;
            p_dev_cb->hid_srvc.srvc_inst_id = service->handle;
            p_dev_cb->hid_srvc.proto_mode_handle = 0;
            p_dev_cb->hid_srvc.control_point_handle = 0;
            process_included(p_dev_cb, service);
            bta_hh_le_search_hid_chars(p_dev_cb, service);
            APPL_TRACE_DEBUG("%s: have HID service inst_id= %d", __func__, p_dev_cb->hid_srvc.srvc_inst_id);
        } else if(service->uuid.uu.uuid16 == UUID_SERVCLASS_SCAN_PARAM) {
            p_dev_cb->scan_refresh_char_handle = 0;

            for(list_node_t *cn = list_begin(service->characteristics);
                    cn != list_end(service->characteristics); cn = list_next(cn)) {
                tBTA_GATTC_CHARACTERISTIC *p_char = list_node(cn);

                if(p_char->uuid.len == LEN_UUID_16 &&
                        p_char->uuid.uu.uuid16 == GATT_UUID_SCAN_REFRESH) {
                    p_dev_cb->scan_refresh_char_handle = p_char->handle;

                    if(p_char->properties & BTA_GATT_CHAR_PROP_BIT_NOTIFY) {
                        p_dev_cb->scps_notify |= BTA_HH_LE_SCPS_NOTIFY_SPT;
                    } else {
                        p_dev_cb->scps_notify = BTA_HH_LE_SCPS_NOTIFY_NONE;
                    }

                    break;
                }
            }
        } else if(service->uuid.uu.uuid16 == UUID_SERVCLASS_GAP_SERVER) {
            //TODO(jpawlowski): this should be done by GAP profile, remove when GAP is fixed.
            for(list_node_t *cn = list_begin(service->characteristics);
                    cn != list_end(service->characteristics); cn = list_next(cn)) {
                tBTA_GATTC_CHARACTERISTIC *p_char = list_node(cn);

                if(p_char->uuid.len == LEN_UUID_16 &&
                        p_char->uuid.uu.uuid16 == GATT_UUID_GAP_PREF_CONN_PARAM) {
                    /* read the char value */
                    gatt_queue_read_op(GATT_READ_CHAR, p_dev_cb->conn_id, p_char->handle);
                    break;
                }
            }
        }
    }

    bta_hh_le_gatt_disc_cmpl(p_dev_cb, p_dev_cb->status);
}

/*******************************************************************************
**
** Function         bta_hh_read_battery_level_cmpl
**
** Description      This function process the battery level read
**
** Parameters:
**
*******************************************************************************/
void bta_hh_read_battery_level_cmpl(uint8_t status, tBTA_HH_DEV_CB *p_dev_cb,
                                    tBTA_GATTC_READ *p_data)
{
    UNUSED(status);
    UNUSED(p_data);
    p_dev_cb->hid_srvc.expl_incl_srvc = TRUE;
}


/*******************************************************************************
**
** Function         bta_hh_le_save_rpt_map
**
** Description      save the report map into the control block.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_save_rpt_map(tBTA_HH_DEV_CB *p_dev_cb, tBTA_GATTC_READ *p_data)
{
    tBTA_HH_LE_HID_SRVC *p_srvc = &p_dev_cb->hid_srvc;
    GKI_free_and_reset_buf((void **)&p_srvc->rpt_map);

    if(p_data->p_value->len > 0) {
        p_srvc->rpt_map = (uint8_t *)GKI_getbuf(p_data->p_value->len);
    }

    if(p_srvc->rpt_map != NULL) {
        uint8_t *pp = p_data->p_value->p_value;
        STREAM_TO_ARRAY(p_srvc->rpt_map, pp, p_data->p_value->len);
        p_srvc->descriptor.dl_len = p_data->p_value->len;
        p_srvc->descriptor.dsc_list = p_dev_cb->hid_srvc.rpt_map;
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_proc_get_rpt_cmpl
**
** Description      Process the Read report complete, send GET_REPORT_EVT to application
**                  with the report data.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_proc_get_rpt_cmpl(tBTA_HH_DEV_CB *p_dev_cb, tBTA_GATTC_READ *p_data)
{
    BT_HDR              *p_buf = NULL;
    tBTA_HH_LE_RPT      *p_rpt;
    tBTA_HH_HSDATA      hs_data;
    uint8_t               *pp ;

    if(p_dev_cb->w4_evt != BTA_HH_GET_RPT_EVT) {
        APPL_TRACE_ERROR("Unexpected READ cmpl, w4_evt = %d", p_dev_cb->w4_evt);
        return;
    }

    const tBTA_GATTC_CHARACTERISTIC *p_char = BTA_GATTC_GetCharacteristic(p_dev_cb->conn_id,
            p_data->handle);
    wm_memset(&hs_data, 0, sizeof(hs_data));
    hs_data.status  = BTA_HH_ERR;
    hs_data.handle  = p_dev_cb->hid_handle;

    if(p_data->status == BTA_GATT_OK) {
        p_rpt = bta_hh_le_find_report_entry(p_dev_cb,
                                            p_char->service->handle,
                                            p_char->uuid.uu.uuid16,
                                            p_char->handle);

        if(p_rpt != NULL && p_data->p_value != NULL) {
            p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) + p_data->p_value->len + 1);
            /* pack data send to app */
            hs_data.status  = BTA_HH_OK;
            p_buf->len = p_data->p_value->len + 1;
            p_buf->layer_specific = 0;
            p_buf->offset = 0;
            /* attach report ID as the first byte of the report before sending it to USB HID driver */
            pp = (uint8_t *)(p_buf + 1);
            UINT8_TO_STREAM(pp, p_rpt->rpt_id);
            wm_memcpy(pp, p_data->p_value->p_value, p_data->p_value->len);
            hs_data.rsp_data.p_rpt_data = p_buf;
        }
    }

    p_dev_cb->w4_evt = 0;
    (* bta_hh_cb.p_cback)(BTA_HH_GET_RPT_EVT, (tBTA_HH *)&hs_data);
    GKI_free_and_reset_buf((void **)&p_buf);
}

/*******************************************************************************
**
** Function         bta_hh_le_proc_read_proto_mode
**
** Description      Process the Read protocol mode, send GET_PROTO_EVT to application
**                  with the protocol mode.
**
*******************************************************************************/
void bta_hh_le_proc_read_proto_mode(tBTA_HH_DEV_CB *p_dev_cb, tBTA_GATTC_READ *p_data)
{
    tBTA_HH_HSDATA      hs_data;
    hs_data.status  = BTA_HH_ERR;
    hs_data.handle  = p_dev_cb->hid_handle;
    hs_data.rsp_data.proto_mode = p_dev_cb->mode;

    if(p_data->status == BTA_GATT_OK && p_data->p_value) {
        hs_data.status  = BTA_HH_OK;
        /* match up BTE/BTA report/boot mode def*/
        hs_data.rsp_data.proto_mode = *(p_data->p_value->p_value);

        /* LE repot mode is the opposite value of BR/EDR report mode, flip it here */
        if(hs_data.rsp_data.proto_mode == 0) {
            hs_data.rsp_data.proto_mode = BTA_HH_PROTO_BOOT_MODE;
        } else {
            hs_data.rsp_data.proto_mode = BTA_HH_PROTO_RPT_MODE;
        }

        p_dev_cb->mode = hs_data.rsp_data.proto_mode;
    }

#if BTA_HH_DEBUG
    APPL_TRACE_DEBUG("LE GET_PROTOCOL Mode = [%s]",
                     (hs_data.rsp_data.proto_mode == BTA_HH_PROTO_RPT_MODE) ? "Report" : "Boot");
#endif
    p_dev_cb->w4_evt = 0;
    (* bta_hh_cb.p_cback)(BTA_HH_GET_PROTO_EVT, (tBTA_HH *)&hs_data);
}

/*******************************************************************************
**
** Function         bta_hh_w4_le_read_char_cmpl
**
** Description      process the GATT read complete in W4_CONN state.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_w4_le_read_char_cmpl(tBTA_HH_DEV_CB *p_dev_cb, tBTA_HH_DATA *p_buf)
{
    tBTA_GATTC_READ      *p_data = (tBTA_GATTC_READ *)p_buf;
    uint8_t               *pp ;
    const tBTA_GATTC_CHARACTERISTIC *p_char = BTA_GATTC_GetCharacteristic(p_dev_cb->conn_id,
            p_data->handle);
    uint16_t char_uuid = p_char->uuid.uu.uuid16;

    if(char_uuid == GATT_UUID_BATTERY_LEVEL) {
        bta_hh_read_battery_level_cmpl(p_data->status, p_dev_cb, p_data);
    } else if(char_uuid == GATT_UUID_GAP_PREF_CONN_PARAM) {
        //TODO(jpawlowski): this should be done by GAP profile, remove when GAP is fixed.
        uint8_t *pp = p_data->p_value->p_value;
        uint16_t min, max, latency, tout;
        STREAM_TO_UINT16(min, pp);
        STREAM_TO_UINT16(max, pp);
        STREAM_TO_UINT16(latency, pp);
        STREAM_TO_UINT16(tout, pp);

        // Make sure both min, and max are bigger than 11.25ms, lower values can introduce
        // audio issues if A2DP is also active.
        if(min < BTM_BLE_CONN_INT_MIN_LIMIT) {
            min = BTM_BLE_CONN_INT_MIN_LIMIT;
        }

        if(max < BTM_BLE_CONN_INT_MIN_LIMIT) {
            max = BTM_BLE_CONN_INT_MIN_LIMIT;
        }

        if(tout < BTM_BLE_CONN_TIMEOUT_MIN_DEF) {
            tout = BTM_BLE_CONN_TIMEOUT_MIN_DEF;
        }

        BTM_BleSetPrefConnParams(p_dev_cb->addr, min, max, latency, tout);
        L2CA_UpdateBleConnParams(p_dev_cb->addr, min, max, latency, tout);
    } else {
        if(p_data->status == BTA_GATT_OK && p_data->p_value) {
            pp = p_data->p_value->p_value;

            switch(char_uuid) {
                /* save device information */
                case GATT_UUID_HID_INFORMATION:
                    STREAM_TO_UINT16(p_dev_cb->dscp_info.version, pp);
                    STREAM_TO_UINT8(p_dev_cb->dscp_info.ctry_code, pp);
                    STREAM_TO_UINT8(p_dev_cb->dscp_info.flag, pp);
                    break;

                case GATT_UUID_HID_REPORT_MAP:
                    bta_hh_le_save_rpt_map(p_dev_cb, p_data);
                    return;

                default:
#if BTA_HH_DEBUG == TRUE
                    APPL_TRACE_ERROR("Unexpected read %s(0x%04x)",
                                     bta_hh_uuid_to_str(char_uuid), char_uuid);
#endif
                    break;
            }
        } else {
#if BTA_HH_DEBUG == TRUE
            APPL_TRACE_ERROR("read uuid %s[0x%04x] error: %d", bta_hh_uuid_to_str(char_uuid),
                             char_uuid, p_data->status);
#else
            APPL_TRACE_ERROR("read uuid [0x%04x] error: %d", char_uuid, p_data->status);
#endif
        }
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_read_char_cmpl
**
** Description      a characteristic value is received.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_read_char_cmpl(tBTA_HH_DEV_CB *p_dev_cb, tBTA_HH_DATA *p_buf)
{
    tBTA_GATTC_READ *p_data = (tBTA_GATTC_READ *)p_buf;
    const tBTA_GATTC_CHARACTERISTIC *p_char = BTA_GATTC_GetCharacteristic(p_dev_cb->conn_id,
            p_data->handle);
    uint16_t char_uuid = p_char->uuid.uu.uuid16;

    switch(char_uuid) {
        /* GET_REPORT */
        case GATT_UUID_HID_REPORT:
        case GATT_UUID_HID_BT_KB_INPUT:
        case GATT_UUID_HID_BT_KB_OUTPUT:
        case GATT_UUID_HID_BT_MOUSE_INPUT:
        case GATT_UUID_BATTERY_LEVEL: /* read battery level */
            bta_hh_le_proc_get_rpt_cmpl(p_dev_cb, p_data);
            break;

        case GATT_UUID_HID_PROTO_MODE:
            bta_hh_le_proc_read_proto_mode(p_dev_cb, p_data);
            break;

        default:
            APPL_TRACE_ERROR("Unexpected Read UUID: 0x%04x", char_uuid);
            break;
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_read_descr_cmpl
**
** Description      read characteristic descriptor is completed in CONN st.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_read_descr_cmpl(tBTA_HH_DEV_CB *p_dev_cb, tBTA_HH_DATA *p_buf)
{
    tBTA_HH_LE_RPT  *p_rpt;
    tBTA_GATTC_READ *p_data = (tBTA_GATTC_READ *)p_buf;
    uint8_t   *pp;
    const tBTA_GATTC_DESCRIPTOR *p_desc = BTA_GATTC_GetDescriptor(p_data->conn_id, p_data->handle);

    /* if a report client configuration */
    if(p_desc->uuid.uu.uuid16 == GATT_UUID_CHAR_CLIENT_CONFIG) {
        if((p_rpt = bta_hh_le_find_report_entry(p_dev_cb,
                                                p_desc->characteristic->service->handle,
                                                p_desc->characteristic->uuid.uu.uuid16,
                                                p_desc->characteristic->handle)) != NULL) {
            pp = p_data->p_value->p_value;
            STREAM_TO_UINT16(p_rpt->client_cfg_value, pp);
            APPL_TRACE_DEBUG("Read Client Configuration: 0x%04x", p_rpt->client_cfg_value);
        }
    }
}

/*******************************************************************************
**
** Function         bta_hh_w4_le_read_descr_cmpl
**
** Description      read characteristic descriptor is completed in W4_CONN st.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_w4_le_read_descr_cmpl(tBTA_HH_DEV_CB *p_dev_cb, tBTA_HH_DATA *p_buf)
{
    tBTA_HH_LE_RPT  *p_rpt;
    tBTA_GATTC_READ *p_data = (tBTA_GATTC_READ *)p_buf;
    uint16_t char_uuid16;

    if(p_data == NULL) {
        return;
    }

    const tBTA_GATTC_DESCRIPTOR *p_desc = BTA_GATTC_GetDescriptor(p_data->conn_id, p_data->handle);

    if(p_desc == NULL) {
        APPL_TRACE_ERROR("%s: p_descr is NULL %d", __func__, p_data->handle);
        return;
    }

    char_uuid16 = p_desc->characteristic->uuid.uu.uuid16;
#if BTA_HH_DEBUG == TRUE
    APPL_TRACE_DEBUG("bta_hh_w4_le_read_descr_cmpl uuid: %s(0x%04x)",
                     bta_hh_uuid_to_str(p_desc->uuid.uu.uuid16),
                     p_desc->uuid.uu.uuid16);
#endif

    switch(char_uuid16) {
        case GATT_UUID_HID_REPORT:
            if((p_rpt = bta_hh_le_find_report_entry(p_dev_cb,
                                                    p_desc->characteristic->service->handle,
                                                    GATT_UUID_HID_REPORT,
                                                    p_desc->characteristic->handle))) {
                bta_hh_le_save_rpt_ref(p_dev_cb, p_rpt, p_data);
            }

            break;

        case GATT_UUID_HID_REPORT_MAP:
            bta_hh_le_save_ext_rpt_ref(p_dev_cb, p_data);
            break;

        case GATT_UUID_BATTERY_LEVEL:
            if((p_rpt = bta_hh_le_find_report_entry(p_dev_cb,
                                                    p_desc->characteristic->service->handle,
                                                    GATT_UUID_BATTERY_LEVEL,
                                                    p_desc->characteristic->handle))) {
                bta_hh_le_save_rpt_ref(p_dev_cb, p_rpt, p_data);
            }

            break;

        default:
            APPL_TRACE_ERROR("unknown descriptor read complete for uuid: 0x%04x", char_uuid16);
            break;
    }
}

/*******************************************************************************
**
** Function         bta_hh_w4_le_write_cmpl
**
** Description      Write charactersitic complete event at W4_CONN st.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_w4_le_write_cmpl(tBTA_HH_DEV_CB *p_dev_cb, tBTA_HH_DATA *p_buf)
{
    tBTA_GATTC_WRITE    *p_data = (tBTA_GATTC_WRITE *)p_buf;

    if(p_data == NULL) {
        return;
    }

    const tBTA_GATTC_CHARACTERISTIC *p_char = BTA_GATTC_GetCharacteristic(p_dev_cb->conn_id,
            p_data->handle);

    if(p_char->uuid.uu.uuid16 == GATT_UUID_HID_PROTO_MODE) {
        p_dev_cb->status = (p_data->status == BTA_GATT_OK) ? BTA_HH_OK : BTA_HH_ERR_PROTO;

        if((p_dev_cb->disc_active & BTA_HH_LE_DISC_HIDS) == 0) {
            bta_hh_le_open_cmpl(p_dev_cb);
        }
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_write_cmpl
**
** Description      Write charactersitic complete event at CONN st.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_write_cmpl(tBTA_HH_DEV_CB *p_dev_cb, tBTA_HH_DATA *p_buf)
{
    tBTA_GATTC_WRITE    *p_data = (tBTA_GATTC_WRITE *)p_buf;
    tBTA_HH_CBDATA      cback_data ;
    uint16_t              cb_evt = p_dev_cb->w4_evt;

    if(p_data == NULL  || cb_evt == 0) {
        return;
    }

    const tBTA_GATTC_CHARACTERISTIC *p_char = BTA_GATTC_GetCharacteristic(p_dev_cb->conn_id,
            p_data->handle);
#if BTA_HH_DEBUG
    APPL_TRACE_DEBUG("bta_hh_le_write_cmpl w4_evt: %d", p_dev_cb->w4_evt);
#endif

    switch(p_char->uuid.uu.uuid16) {
        /* Set protocol finished */
        case GATT_UUID_HID_PROTO_MODE:
            cback_data.handle  = p_dev_cb->hid_handle;

            if(p_data->status == BTA_GATT_OK) {
                bta_hh_le_register_input_notif(p_dev_cb, p_dev_cb->mode, FALSE);
                cback_data.status = BTA_HH_OK;
            } else {
                cback_data.status =  BTA_HH_ERR;
            }

            p_dev_cb->w4_evt = 0;
            (* bta_hh_cb.p_cback)(cb_evt, (tBTA_HH *)&cback_data);
            break;

        /* Set Report finished */
        case GATT_UUID_HID_REPORT:
        case GATT_UUID_HID_BT_KB_INPUT:
        case GATT_UUID_HID_BT_MOUSE_INPUT:
        case GATT_UUID_HID_BT_KB_OUTPUT:
            cback_data.handle  = p_dev_cb->hid_handle;
            cback_data.status = (p_data->status == BTA_GATT_OK) ? BTA_HH_OK : BTA_HH_ERR;
            p_dev_cb->w4_evt = 0;
            (* bta_hh_cb.p_cback)(cb_evt, (tBTA_HH *)&cback_data);
            break;

        case GATT_UUID_SCAN_INT_WINDOW:
            bta_hh_le_register_scpp_notif(p_dev_cb, p_data->status);
            break;

        default:
            break;
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_write_char_descr_cmpl
**
** Description      Write charactersitic descriptor complete event
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_write_char_descr_cmpl(tBTA_HH_DEV_CB *p_dev_cb, tBTA_HH_DATA *p_buf)
{
    tBTA_GATTC_WRITE    *p_data = (tBTA_GATTC_WRITE *)p_buf;
    uint8_t   srvc_inst_id, hid_inst_id;
    const tBTA_GATTC_DESCRIPTOR *p_desc = BTA_GATTC_GetDescriptor(p_dev_cb->conn_id,
                                          p_data->handle);
    uint16_t desc_uuid = p_desc->uuid.uu.uuid16;
    uint16_t char_uuid = p_desc->characteristic->uuid.uu.uuid16;

    /* only write client configuration possible */
    if(desc_uuid == GATT_UUID_CHAR_CLIENT_CONFIG) {
        srvc_inst_id = p_desc->characteristic->service->handle;
        hid_inst_id = srvc_inst_id;

        switch(char_uuid) {
            case GATT_UUID_BATTERY_LEVEL: /* battery level clt cfg registered */
                hid_inst_id = bta_hh_le_find_service_inst_by_battery_inst_id(p_dev_cb, srvc_inst_id);

            /* fall through */
            case GATT_UUID_HID_BT_KB_INPUT:
            case GATT_UUID_HID_BT_MOUSE_INPUT:
            case GATT_UUID_HID_REPORT:
                if(p_data->status == BTA_GATT_OK)
                    p_dev_cb->hid_srvc.report[p_dev_cb->clt_cfg_idx].client_cfg_value =
                                    BTA_GATT_CLT_CONFIG_NOTIFICATION;

                p_dev_cb->clt_cfg_idx ++;
                bta_hh_le_write_rpt_clt_cfg(p_dev_cb, hid_inst_id);
                break;

            case GATT_UUID_SCAN_REFRESH:
                bta_hh_le_register_scpp_notif_cmpl(p_dev_cb, p_data->status);
                break;

            default:
                APPL_TRACE_ERROR("Unknown char ID clt cfg: 0x%04x", char_uuid);
        }
    } else {
#if BTA_HH_DEBUG == TRUE
        APPL_TRACE_ERROR("Unexpected write to %s(0x%04x)",
                         bta_hh_uuid_to_str(desc_uuid), desc_uuid);
#else
        APPL_TRACE_ERROR("Unexpected write to (0x%04x)", desc_uuid);
#endif
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_input_rpt_notify
**
** Description      process the notificaton event, most likely for input report.
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_input_rpt_notify(tBTA_GATTC_NOTIFY *p_data)
{
    tBTA_HH_DEV_CB       *p_dev_cb = bta_hh_le_find_dev_cb_by_conn_id(p_data->conn_id);
    uint8_t           app_id;
    uint8_t           *p_buf;
    tBTA_HH_LE_RPT  *p_rpt;

    if(p_dev_cb == NULL) {
        APPL_TRACE_ERROR("notification received from Unknown device");
        return;
    }

    const tBTA_GATTC_CHARACTERISTIC *p_char = BTA_GATTC_GetCharacteristic(p_dev_cb->conn_id,
            p_data->handle);
    app_id = p_dev_cb->app_id;
    p_rpt = bta_hh_le_find_report_entry(p_dev_cb,
                                        p_dev_cb->hid_srvc.srvc_inst_id,
                                        p_char->uuid.uu.uuid16,
                                        p_char->handle);

    if(p_rpt == NULL) {
        APPL_TRACE_ERROR("notification received for Unknown Report");
        return;
    }

    if(p_char->uuid.uu.uuid16 == GATT_UUID_HID_BT_MOUSE_INPUT) {
        app_id = BTA_HH_APP_ID_MI;
    } else if(p_char->uuid.uu.uuid16 == GATT_UUID_HID_BT_KB_INPUT) {
        app_id = BTA_HH_APP_ID_KB;
    }

    APPL_TRACE_DEBUG("Notification received on report ID: %d", p_rpt->rpt_id);

    /* need to append report ID to the head of data */
    if(p_rpt->rpt_id != 0) {
        p_buf = (uint8_t *)GKI_getbuf(p_data->len + 1);
        p_buf[0] = p_rpt->rpt_id;
        wm_memcpy(&p_buf[1], p_data->value, p_data->len);
        ++p_data->len;
    } else {
        p_buf = p_data->value;
    }

    bta_hh_co_data((uint8_t)p_dev_cb->hid_handle,
                   p_buf,
                   p_data->len,
                   p_dev_cb->mode,
                   0,  /* no sub class*/
                   p_dev_cb->dscp_info.ctry_code,
                   p_dev_cb->addr,
                   app_id);

    if(p_buf != p_data->value) {
        GKI_freebuf(p_buf);
    }
}

/*******************************************************************************
**
** Function         bta_hh_gatt_open_fail
**
** Description      action function to process the open fail
**
** Returns          void
**
*******************************************************************************/
void bta_hh_le_open_fail(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data)
{
    tBTA_HH_CONN            conn_dat ;

    /* open failure in the middle of service discovery, clear all services */
    if(p_cb->disc_active & BTA_HH_LE_DISC_HIDS) {
        bta_hh_clear_service_cache(p_cb);
    }

    p_cb->disc_active = BTA_HH_LE_DISC_NONE;
    /* Failure in opening connection or GATT discovery failure */
    conn_dat.handle = p_cb->hid_handle;
    wm_memcpy(conn_dat.bda, p_cb->addr, BD_ADDR_LEN);
    conn_dat.le_hid = TRUE;
    conn_dat.scps_supported = p_cb->scps_supported;

    if(p_cb->status == BTA_HH_OK) {
        conn_dat.status = (p_data->le_close.reason == BTA_GATT_CONN_UNKNOWN) ? p_cb->status : BTA_HH_ERR;
    } else {
        conn_dat.status = p_cb->status;
    }

    /* Report OPEN fail event */
    (*bta_hh_cb.p_cback)(BTA_HH_OPEN_EVT, (tBTA_HH *)&conn_dat);
}

/*******************************************************************************
**
** Function         bta_hh_gatt_close
**
** Description      action function to process the GATT close int he state machine.
**
** Returns          void
**
*******************************************************************************/
void bta_hh_gatt_close(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data)
{
    tBTA_HH_CBDATA          disc_dat = {BTA_HH_OK, 0};
    /* finaliza device driver */
    bta_hh_co_close(p_cb->hid_handle, p_cb->app_id);
    /* update total conn number */
    bta_hh_cb.cnt_num --;
    disc_dat.handle = p_cb->hid_handle;
    disc_dat.status = p_cb->status;
    (*bta_hh_cb.p_cback)(BTA_HH_CLOSE_EVT, (tBTA_HH *)&disc_dat);

    /* if no connection is active and HH disable is signaled, disable service */
    if(bta_hh_cb.cnt_num == 0 && bta_hh_cb.w4_disable) {
        bta_hh_disc_cmpl();
    } else {
#if (BTA_HH_LE_RECONN == TRUE)

        if(p_data->le_close.reason == BTA_GATT_CONN_TIMEOUT) {
            bta_hh_le_add_dev_bg_conn(p_cb, FALSE);
        }

#endif
    }

    return;
}

/*******************************************************************************
**
** Function         bta_hh_le_api_disc_act
**
** Description      initaite a Close API to a remote HID device
**
** Returns          void
**
*******************************************************************************/
void bta_hh_le_api_disc_act(tBTA_HH_DEV_CB *p_cb)
{
    if(p_cb->conn_id != BTA_GATT_INVALID_CONN_ID) {
        gatt_op_queue_clean(p_cb->conn_id);
        BTA_GATTC_Close(p_cb->conn_id);
        /* remove device from background connection if intended to disconnect,
           do not allow reconnection */
        bta_hh_le_remove_dev_bg_conn(p_cb);
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_get_rpt
**
** Description      GET_REPORT on a LE HID Report
**
** Returns          void
**
*******************************************************************************/
void bta_hh_le_get_rpt(tBTA_HH_DEV_CB *p_cb, tBTA_HH_RPT_TYPE r_type, uint8_t rpt_id)
{
    tBTA_HH_LE_RPT  *p_rpt = bta_hh_le_find_rpt_by_idtype(p_cb->hid_srvc.report, p_cb->mode, r_type,
                             rpt_id);

    if(p_rpt == NULL) {
        APPL_TRACE_ERROR("%s: no matching report", __func__);
        return;
    }

    p_cb->w4_evt = BTA_HH_GET_RPT_EVT;
    gatt_queue_read_op(GATT_READ_CHAR, p_cb->conn_id, p_rpt->char_inst_id);
}

/*******************************************************************************
**
** Function         bta_hh_le_write_rpt
**
** Description      SET_REPORT/or DATA output on a LE HID Report
**
** Returns          void
**
*******************************************************************************/
void bta_hh_le_write_rpt(tBTA_HH_DEV_CB *p_cb,
                         tBTA_HH_RPT_TYPE r_type,
                         BT_HDR *p_buf, uint16_t w4_evt)
{
    tBTA_HH_LE_RPT  *p_rpt;
    uint8_t   *p_value, rpt_id;

    if(p_buf == NULL || p_buf->len == 0) {
        APPL_TRACE_ERROR("bta_hh_le_write_rpt: Illegal data");
        return;
    }

    /* strip report ID from the data */
    p_value = (uint8_t *)(p_buf + 1) + p_buf->offset;
    STREAM_TO_UINT8(rpt_id, p_value);
    p_buf->len -= 1;
    p_rpt = bta_hh_le_find_rpt_by_idtype(p_cb->hid_srvc.report, p_cb->mode, r_type, rpt_id);

    if(p_rpt == NULL) {
        APPL_TRACE_ERROR("%s: no matching report", __func__);
        GKI_freebuf(p_buf);
        return;
    }

    p_cb->w4_evt = w4_evt;
    const tBTA_GATTC_CHARACTERISTIC *p_char = BTA_GATTC_GetCharacteristic(p_cb->conn_id,
            p_rpt->char_inst_id);
    tBTA_GATTC_WRITE_TYPE write_type = BTA_GATTC_TYPE_WRITE;

    if(p_char && (p_char->properties & BTA_GATT_CHAR_PROP_BIT_WRITE_NR)) {
        write_type = BTA_GATTC_TYPE_WRITE_NO_RSP;
    }

    gatt_queue_write_op(GATT_WRITE_CHAR, p_cb->conn_id, p_rpt->char_inst_id,
                        p_buf->len, p_value, write_type);
}

/*******************************************************************************
**
** Function         bta_hh_le_suspend
**
** Description      send LE suspend or exit suspend mode to remote device.
**
** Returns          void
**
*******************************************************************************/
void bta_hh_le_suspend(tBTA_HH_DEV_CB *p_cb, tBTA_HH_TRANS_CTRL_TYPE ctrl_type)
{
    ctrl_type -= BTA_HH_CTRL_SUSPEND;
    gatt_queue_write_op(GATT_WRITE_CHAR, p_cb->conn_id, p_cb->hid_srvc.control_point_handle, 1,
                        &ctrl_type, BTA_GATTC_TYPE_WRITE_NO_RSP);
}

/*******************************************************************************
**
** Function         bta_hh_le_write_dev_act
**
** Description      Write LE device action. can be SET/GET/DATA transaction.
**
** Returns          void
**
*******************************************************************************/
void bta_hh_le_write_dev_act(tBTA_HH_DEV_CB *p_cb, tBTA_HH_DATA *p_data)
{
    switch(p_data->api_sndcmd.t_type) {
        case HID_TRANS_SET_PROTOCOL:
            p_cb->w4_evt = BTA_HH_SET_PROTO_EVT;
            bta_hh_le_set_protocol_mode(p_cb, p_data->api_sndcmd.param);
            break;

        case HID_TRANS_GET_PROTOCOL:
            bta_hh_le_get_protocol_mode(p_cb);
            break;

        case HID_TRANS_GET_REPORT:
            bta_hh_le_get_rpt(p_cb,
                              p_data->api_sndcmd.param,
                              p_data->api_sndcmd.rpt_id);
            break;

        case HID_TRANS_SET_REPORT:
            bta_hh_le_write_rpt(p_cb,
                                p_data->api_sndcmd.param,
                                p_data->api_sndcmd.p_data,
                                BTA_HH_SET_RPT_EVT);
            break;

        case HID_TRANS_DATA:  /* output report */
            bta_hh_le_write_rpt(p_cb,
                                p_data->api_sndcmd.param,
                                p_data->api_sndcmd.p_data,
                                BTA_HH_DATA_EVT);
            break;

        case HID_TRANS_CONTROL:

            /* no handshake event will be generated */
            /* if VC_UNPLUG is issued, set flag */
            if(p_data->api_sndcmd.param == BTA_HH_CTRL_SUSPEND ||
                    p_data->api_sndcmd.param == BTA_HH_CTRL_EXIT_SUSPEND) {
                bta_hh_le_suspend(p_cb, p_data->api_sndcmd.param);
            }

            break;

        default:
            APPL_TRACE_ERROR("%s unsupported transaction for BLE HID device: %d",
                             __func__, p_data->api_sndcmd.t_type);
            break;
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_get_dscp_act
**
** Description      Send ReportDescriptor to application for all HID services.
**
** Returns          void
**
*******************************************************************************/
void bta_hh_le_get_dscp_act(tBTA_HH_DEV_CB *p_cb)
{
    if(p_cb->hid_srvc.in_use) {
        p_cb->dscp_info.descriptor.dl_len = p_cb->hid_srvc.descriptor.dl_len;
        p_cb->dscp_info.descriptor.dsc_list = p_cb->hid_srvc.descriptor.dsc_list;
        (*bta_hh_cb.p_cback)(BTA_HH_GET_DSCP_EVT, (tBTA_HH *)&p_cb->dscp_info);
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_add_dev_bg_conn
**
** Description      Remove a LE HID device from back ground connection procedure.
**
** Returns          void
**
*******************************************************************************/
static void bta_hh_le_add_dev_bg_conn(tBTA_HH_DEV_CB *p_cb, uint8_t check_bond)
{
    uint8_t           sec_flag = 0;
    uint8_t         to_add = TRUE;

    if(check_bond) {
        /* start reconnection if remote is a bonded device */
        /* verify bond */
        BTM_GetSecurityFlagsByTransport(p_cb->addr, &sec_flag, BT_TRANSPORT_LE);

        if((sec_flag & BTM_SEC_FLAG_LKEY_KNOWN) == 0) {
            to_add = FALSE;
        }
    }

    if( /*p_cb->dscp_info.flag & BTA_HH_LE_NORMAL_CONN &&*/
                    !p_cb->in_bg_conn && to_add) {
        /* add device into BG connection to accept remote initiated connection */
        BTA_GATTC_Open(bta_hh_cb.gatt_if, p_cb->addr, FALSE, BTA_GATT_TRANSPORT_LE);
        p_cb->in_bg_conn = TRUE;
        BTA_DmBleSetBgConnType(BTA_DM_BLE_CONN_AUTO, NULL);
    }

    return;
}

/*******************************************************************************
**
** Function         bta_hh_le_add_device
**
** Description      Add a LE HID device as a known device, and also add the address
**                  into back ground connection WL for incoming connection.
**
** Returns          void
**
*******************************************************************************/
uint8_t bta_hh_le_add_device(tBTA_HH_DEV_CB *p_cb, tBTA_HH_MAINT_DEV *p_dev_info)
{
    p_cb->hid_handle = BTA_HH_GET_LE_DEV_HDL(p_cb->index);
    bta_hh_cb.le_cb_index[BTA_HH_GET_LE_CB_IDX(p_cb->hid_handle)] = p_cb->index;
    /* update DI information */
    bta_hh_update_di_info(p_cb,
                          p_dev_info->dscp_info.vendor_id,
                          p_dev_info->dscp_info.product_id,
                          p_dev_info->dscp_info.version,
                          p_dev_info->dscp_info.flag);
    /* add to BTA device list */
    bta_hh_add_device_to_list(p_cb, p_cb->hid_handle,
                              p_dev_info->attr_mask,
                              &p_dev_info->dscp_info.descriptor,
                              p_dev_info->sub_class,
                              p_dev_info->dscp_info.ssr_max_latency,
                              p_dev_info->dscp_info.ssr_min_tout,
                              p_dev_info->app_id);
    bta_hh_le_add_dev_bg_conn(p_cb, FALSE);
    return p_cb->hid_handle;
}

/*******************************************************************************
**
** Function         bta_hh_le_remove_dev_bg_conn
**
** Description      Remove a LE HID device from back ground connection procedure.
**
** Returns          void
**
*******************************************************************************/
void bta_hh_le_remove_dev_bg_conn(tBTA_HH_DEV_CB *p_dev_cb)
{
    if(p_dev_cb->in_bg_conn) {
        p_dev_cb->in_bg_conn = FALSE;
        BTA_GATTC_CancelOpen(bta_hh_cb.gatt_if, p_dev_cb->addr, FALSE);
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_update_scpp
**
** Description      action function to update the scan parameters on remote HID
**                  device
**
** Parameters:
**
*******************************************************************************/
void bta_hh_le_update_scpp(tBTA_HH_DEV_CB *p_dev_cb, tBTA_HH_DATA *p_buf)
{
    uint8_t   value[4], *p = value;
    tBTA_HH_CBDATA      cback_data;

    if(!p_dev_cb->is_le_device ||
            p_dev_cb->mode != BTA_HH_PROTO_RPT_MODE ||
            p_dev_cb->scps_supported == FALSE) {
        APPL_TRACE_ERROR("Can not set ScPP scan paramter as boot host, or remote does not support ScPP ");
        cback_data.handle = p_dev_cb->hid_handle;
        cback_data.status = BTA_HH_ERR;
        (* bta_hh_cb.p_cback)(BTA_HH_UPDATE_SCPP_EVT, (tBTA_HH *)&cback_data);
        return;
    }

    p_dev_cb->w4_evt = BTA_HH_UPDATE_SCPP_EVT;
    UINT16_TO_STREAM(p, p_buf->le_scpp_update.scan_int);
    UINT16_TO_STREAM(p, p_buf->le_scpp_update.scan_win);
    gatt_queue_write_op(GATT_WRITE_CHAR, p_dev_cb->conn_id, p_dev_cb->scan_refresh_char_handle, 2,
                        value, BTA_GATTC_TYPE_WRITE_NO_RSP);
}

/*******************************************************************************
**
** Function         bta_hh_gattc_callback
**
** Description      This is GATT client callback function used in BTA HH.
**
** Parameters:
**
*******************************************************************************/
static void bta_hh_gattc_callback(tBTA_GATTC_EVT event, tBTA_GATTC *p_data)
{
    tBTA_HH_DEV_CB *p_dev_cb;
    uint16_t          evt;
#if BTA_HH_DEBUG
    APPL_TRACE_DEBUG("bta_hh_gattc_callback event = %d", event);
#endif

    if(p_data == NULL) {
        return;
    }

    switch(event) {
        case BTA_GATTC_REG_EVT: /* 0 */
            bta_hh_le_register_cmpl(&p_data->reg_oper);
            break;

        case BTA_GATTC_DEREG_EVT: /* 1 */
            bta_hh_cleanup_disable(p_data->reg_oper.status);
            break;

        case BTA_GATTC_OPEN_EVT: /* 2 */
            p_dev_cb = bta_hh_le_find_dev_cb_by_bda(p_data->open.remote_bda);

            if(p_dev_cb) {
                bta_hh_sm_execute(p_dev_cb, BTA_HH_GATT_OPEN_EVT, (tBTA_HH_DATA *)&p_data->open);
            }

            break;

        case BTA_GATTC_READ_CHAR_EVT: /* 3 */
        case BTA_GATTC_READ_DESCR_EVT: /* 8 */
            mark_as_not_executing(p_data->read.conn_id);
            gatt_execute_next_op(p_data->read.conn_id);
            p_dev_cb = bta_hh_le_find_dev_cb_by_conn_id(p_data->read.conn_id);

            if(event == BTA_GATTC_READ_CHAR_EVT) {
                evt = BTA_HH_GATT_READ_CHAR_CMPL_EVT;
            } else {
                evt = BTA_HH_GATT_READ_DESCR_CMPL_EVT;
            }

            bta_hh_sm_execute(p_dev_cb, evt, (tBTA_HH_DATA *)&p_data->read);
            break;

        case BTA_GATTC_WRITE_DESCR_EVT: /* 9 */
        case BTA_GATTC_WRITE_CHAR_EVT: /* 4 */
            mark_as_not_executing(p_data->write.conn_id);
            gatt_execute_next_op(p_data->write.conn_id);
            p_dev_cb = bta_hh_le_find_dev_cb_by_conn_id(p_data->write.conn_id);

            if(event == BTA_GATTC_WRITE_CHAR_EVT) {
                evt = BTA_HH_GATT_WRITE_CHAR_CMPL_EVT;
            } else {
                evt = BTA_HH_GATT_WRITE_DESCR_CMPL_EVT;
            }

            bta_hh_sm_execute(p_dev_cb, evt, (tBTA_HH_DATA *)&p_data->write);
            break;

        case BTA_GATTC_CLOSE_EVT: /* 5 */
            bta_hh_le_close(&p_data->close);
            break;

        case BTA_GATTC_SEARCH_CMPL_EVT: /* 6 */
            bta_hh_le_srvc_search_cmpl(&p_data->search_cmpl);
            break;

        case BTA_GATTC_NOTIF_EVT: /* 10 */
            bta_hh_le_input_rpt_notify(&p_data->notify);
            break;

        case BTA_GATTC_ENC_CMPL_CB_EVT: /* 17 */
            p_dev_cb = bta_hh_le_find_dev_cb_by_bda(p_data->enc_cmpl.remote_bda);

            if(p_dev_cb) {
                bta_hh_sm_execute(p_dev_cb, BTA_HH_GATT_ENC_CMPL_EVT,
                                  (tBTA_HH_DATA *)&p_data->enc_cmpl);
            }

            break;

        default:
            break;
    }
}

/*******************************************************************************
**
** Function         bta_hh_le_hid_read_rpt_clt_cfg
**
** Description      a test command to read report descriptor client configuration
**
** Returns          void
**
*******************************************************************************/
void bta_hh_le_hid_read_rpt_clt_cfg(BD_ADDR bd_addr, uint8_t rpt_id)
{
    tBTA_HH_DEV_CB *p_cb = NULL;
    tBTA_HH_LE_RPT *p_rpt ;
    uint8_t           index = BTA_HH_IDX_INVALID;
    index = bta_hh_find_cb(bd_addr);

    if((index = bta_hh_find_cb(bd_addr)) == BTA_HH_IDX_INVALID) {
        APPL_TRACE_ERROR("unknown device");
        return;
    }

    p_cb = &bta_hh_cb.kdev[index];
    p_rpt = bta_hh_le_find_rpt_by_idtype(p_cb->hid_srvc.report, p_cb->mode, BTA_HH_RPTT_INPUT, rpt_id);

    if(p_rpt == NULL) {
        APPL_TRACE_ERROR("bta_hh_le_write_rpt: no matching report");
        return;
    }

    bta_hh_le_read_char_dscrpt(p_cb, p_rpt->char_inst_id, GATT_UUID_CHAR_CLIENT_CONFIG);
    return;
}

/*******************************************************************************
**
** Function         bta_hh_le_register_scpp_notif
**
** Description      register scan parameter refresh notitication complete
**
**
** Parameters:
**
*******************************************************************************/
static void bta_hh_le_register_scpp_notif(tBTA_HH_DEV_CB *p_dev_cb, tBTA_GATT_STATUS status)
{
    uint8_t               sec_flag = 0;

    /* if write scan parameter sucessful */
    /* if bonded and notification is not enabled, configure the client configuration */
    if(status == BTA_GATT_OK &&
            (p_dev_cb->scps_notify & BTA_HH_LE_SCPS_NOTIFY_SPT) != 0 &&
            (p_dev_cb->scps_notify & BTA_HH_LE_SCPS_NOTIFY_ENB) == 0) {
        BTM_GetSecurityFlagsByTransport(p_dev_cb->addr, &sec_flag, BT_TRANSPORT_LE);

        if((sec_flag & BTM_SEC_FLAG_LKEY_KNOWN)) {
            if(bta_hh_le_write_char_clt_cfg(p_dev_cb, p_dev_cb->scan_refresh_char_handle,
                                            BTA_GATT_CLT_CONFIG_NOTIFICATION)) {
                BTA_GATTC_RegisterForNotifications(bta_hh_cb.gatt_if, p_dev_cb->addr,
                                                   p_dev_cb->scan_refresh_char_handle);
                return;
            }
        }
    }

    bta_hh_le_register_scpp_notif_cmpl(p_dev_cb, status);
}

/*******************************************************************************
**
** Function         bta_hh_le_register_scpp_notif_cmpl
**
** Description      action function to register scan parameter refresh notitication
**
** Parameters:
**
*******************************************************************************/
static void bta_hh_le_register_scpp_notif_cmpl(tBTA_HH_DEV_CB *p_dev_cb, tBTA_GATT_STATUS status)
{
    tBTA_HH_CBDATA      cback_data ;
    uint16_t              cb_evt = p_dev_cb->w4_evt;

    if(status == BTA_GATT_OK) {
        p_dev_cb->scps_notify = (BTA_HH_LE_SCPS_NOTIFY_ENB | BTA_HH_LE_SCPS_NOTIFY_SPT);
    }

    cback_data.handle = p_dev_cb->hid_handle;
    cback_data.status = (status == BTA_GATT_OK) ? BTA_HH_OK : BTA_HH_ERR;
    p_dev_cb->w4_evt = 0;
    (* bta_hh_cb.p_cback)(cb_evt, (tBTA_HH *)&cback_data);
}

/*******************************************************************************
**
** Function         bta_hh_process_cache_rpt
**
** Description      Process the cached reports
**
** Parameters:
**
*******************************************************************************/
//TODO(jpawlowski): uncomment when fixed
// static void bta_hh_process_cache_rpt (tBTA_HH_DEV_CB *p_cb,
//                                       tBTA_HH_RPT_CACHE_ENTRY *p_rpt_cache,
//                                       uint8_t num_rpt)
// {
//     uint8_t                       i = 0;
//     tBTA_HH_LE_RPT              *p_rpt;

//     if (num_rpt != 0)  /* no cache is found */
//     {
//         p_cb->hid_srvc.in_use = TRUE;

//         /* set the descriptor info */
//         p_cb->hid_srvc.descriptor.dl_len =
//                 p_cb->dscp_info.descriptor.dl_len;
//         p_cb->hid_srvc.descriptor.dsc_list =
//                     p_cb->dscp_info.descriptor.dsc_list;

//         for (; i <num_rpt; i ++, p_rpt_cache ++)
//         {
//             if ((p_rpt = bta_hh_le_find_alloc_report_entry (p_cb,
//                                                p_rpt_cache->srvc_inst_id,
//                                                p_rpt_cache->rpt_uuid,
//                                                p_rpt_cache->char_inst_id,
//                                                p_rpt_cache->prop))  == NULL)
//             {
//                 APPL_TRACE_ERROR("bta_hh_process_cache_rpt: allocation report entry failure");
//                 break;
//             }
//             else
//             {
//                 p_rpt->rpt_type =  p_rpt_cache->rpt_type;
//                 p_rpt->rpt_id   =  p_rpt_cache->rpt_id;

//                 if(p_rpt->uuid == GATT_UUID_HID_BT_KB_INPUT ||
//                     p_rpt->uuid == GATT_UUID_HID_BT_MOUSE_INPUT ||
//                     (p_rpt->uuid == GATT_UUID_HID_REPORT && p_rpt->rpt_type == BTA_HH_RPTT_INPUT))
//                 {
//                     p_rpt->client_cfg_value = BTA_GATT_CLT_CONFIG_NOTIFICATION;
//                 }
//             }
//         }
//     }
// }

#endif




