/******************************************************************************
 *
 *  Copyright (C) 1999-2013 Broadcom Corporation
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

#include "bt_target.h"
#if defined(SRVC_INCLUDED) &&(SRVC_INCLUDED == TRUE)
#include "bt_utils.h"
#include "gatt_api.h"
#include "gatt_int.h"
#include "srvc_eng_int.h"
#include "srvc_battery_int.h"

#if BLE_INCLUDED == TRUE

#define BA_MAX_CHAR_NUM          1
#define BA_MAX_ATTR_NUM          (BA_MAX_CHAR_NUM * 5 + 1) /* max 3 descriptors, 1 desclration and 1 value */

#ifndef BATTER_LEVEL_PROP
#define BATTER_LEVEL_PROP           (GATT_CHAR_PROP_BIT_READ|GATT_CHAR_PROP_BIT_NOTIFY)
#endif


#ifndef BATTER_LEVEL_PERM
#define BATTER_LEVEL_PERM           (GATT_PERM_READ)
#endif

tBATTERY_CB battery_cb;


/*******************************************************************************
**   battery_valid_handle_range
**
**   validate a handle to be a DIS attribute handle or not.
*******************************************************************************/
uint8_t battery_valid_handle_range(uint16_t handle)
{
    uint8_t       i = 0;
    tBA_INST    *p_inst = &battery_cb.battery_inst[0];

    for(; i < BA_MAX_INT_NUM; i ++, p_inst++) {
        if(handle == p_inst->ba_level_hdl ||
                handle == p_inst->clt_cfg_hdl ||
                handle == p_inst->rpt_ref_hdl ||
                handle == p_inst->pres_fmt_hdl) {
            return TRUE;
        }
    }

    return FALSE;
}
/*******************************************************************************
**   battery_s_write_attr_value
**
**   Process write DIS attribute request.
*******************************************************************************/
uint8_t battery_s_write_attr_value(uint8_t clcb_idx, tGATT_WRITE_REQ *p_value,
                                   tGATT_STATUS *p_status)
{
    uint8_t       *p = p_value->value, i;
    uint16_t      handle = p_value->handle;
    tBA_INST    *p_inst = &battery_cb.battery_inst[0];
    tGATT_STATUS    st = GATT_NOT_FOUND;
    tBA_WRITE_DATA   cfg;
    uint8_t       act = SRVC_ACT_RSP;

    for(i = 0; i < BA_MAX_INT_NUM; i ++, p_inst ++) {
        /* read battery level */
        if(handle == p_inst->clt_cfg_hdl) {
            wm_memcpy(cfg.remote_bda, srvc_eng_cb.clcb[clcb_idx].bda, BD_ADDR_LEN);
            STREAM_TO_UINT16(cfg.clt_cfg, p);

            if(p_inst->p_cback) {
                p_inst->pending_clcb_idx = clcb_idx;
                p_inst->pending_evt = BA_WRITE_CLT_CFG_REQ;
                p_inst->pending_handle = handle;
                cfg.need_rsp = p_value->need_rsp;
                act = SRVC_ACT_PENDING;
                (* p_inst->p_cback)(p_inst->app_id, BA_WRITE_CLT_CFG_REQ, &cfg);
            }
        } else { /* all other handle is not writable */
            st = GATT_WRITE_NOT_PERMIT;
            break;
        }
    }

    *p_status = st;
    return act;
}
/*******************************************************************************
**   BA Attributes Database Server Request callback
*******************************************************************************/
uint8_t battery_s_read_attr_value(uint8_t clcb_idx, uint16_t handle, tGATT_VALUE *p_value,
                                  uint8_t is_long, tGATT_STATUS *p_status)
{
    uint8_t       i;
    tBA_INST    *p_inst = &battery_cb.battery_inst[0];
    tGATT_STATUS    st = GATT_NOT_FOUND;
    uint8_t       act = SRVC_ACT_RSP;
    UNUSED(p_value);

    for(i = 0; i < BA_MAX_INT_NUM; i ++, p_inst ++) {
        /* read battery level */
        if(handle == p_inst->ba_level_hdl ||
                handle == p_inst->clt_cfg_hdl ||
                handle == p_inst->rpt_ref_hdl ||
                handle == p_inst->pres_fmt_hdl) {
            if(is_long) {
                st = GATT_NOT_LONG;
            }

            if(p_inst->p_cback) {
                if(handle == p_inst->ba_level_hdl) {
                    p_inst->pending_evt = BA_READ_LEVEL_REQ;
                }

                if(handle == p_inst->clt_cfg_hdl) {
                    p_inst->pending_evt = BA_READ_CLT_CFG_REQ;
                }

                if(handle == p_inst->pres_fmt_hdl) {
                    p_inst->pending_evt = BA_READ_PRE_FMT_REQ;
                }

                if(handle == p_inst->rpt_ref_hdl) {
                    p_inst->pending_evt = BA_READ_RPT_REF_REQ ;
                }

                p_inst->pending_clcb_idx = clcb_idx;
                p_inst->pending_handle = handle;
                act = SRVC_ACT_PENDING;
                (* p_inst->p_cback)(p_inst->app_id, p_inst->pending_evt, NULL);
            } else { /* application is not registered */
                st = GATT_ERR_UNLIKELY;
            }

            break;
        }

        /* else attribute not found */
    }

    *p_status = st;
    return act;
}


/*******************************************************************************
**
** Function         battery_gatt_c_read_ba_req
**
** Description      Read remote device BA level attribute request.
**
** Returns          void
**
*******************************************************************************/
uint8_t battery_gatt_c_read_ba_req(uint16_t conn_id)
{
    UNUSED(conn_id);
    return TRUE;
}

/*******************************************************************************
**
** Function         battery_c_cmpl_cback
**
** Description      Client operation complete callback.
**
** Returns          void
**
*******************************************************************************/
void battery_c_cmpl_cback(tSRVC_CLCB *p_clcb, tGATTC_OPTYPE op,
                          tGATT_STATUS status, tGATT_CL_COMPLETE *p_data)
{
    UNUSED(p_clcb);
    UNUSED(op);
    UNUSED(status);
    UNUSED(p_data);
}


/*******************************************************************************
**
** Function         Battery_Instantiate
**
** Description      Instantiate a Battery service
**
*******************************************************************************/
uint16_t Battery_Instantiate(uint8_t app_id, tBA_REG_INFO *p_reg_info)
{
    tBT_UUID            uuid = {LEN_UUID_16, {UUID_SERVCLASS_BATTERY}};
    uint16_t              srvc_hdl;
    tGATT_STATUS        status = GATT_ERROR;
    tBA_INST            *p_inst;
    tGATT_CHAR_PROP     prop = GATT_CHAR_PROP_BIT_READ;

    if(battery_cb.inst_id == BA_MAX_INT_NUM) {
        GATT_TRACE_ERROR("MAX battery service has been reached");
        return 0;
    }

    p_inst = &battery_cb.battery_inst[battery_cb.inst_id];
    srvc_hdl = GATTS_CreateService(srvc_eng_cb.gatt_if,
                                   &uuid,
                                   battery_cb.inst_id,
                                   BA_MAX_ATTR_NUM,
                                   p_reg_info->is_pri);

    if(srvc_hdl == 0) {
        GATT_TRACE_ERROR("Can not create service, Battery_Instantiate() failed!");
        return 0;
    }

    battery_cb.inst_id ++;
    p_inst->app_id  =   app_id;
    p_inst->p_cback =   p_reg_info->p_cback;
    /* add battery level
    */
    uuid.uu.uuid16 = GATT_UUID_BATTERY_LEVEL;

    if(p_reg_info->ba_level_descr & BA_LEVEL_NOTIFY) {
        prop |= GATT_CHAR_PROP_BIT_NOTIFY;
    }

    if((p_inst->ba_level_hdl  = GATTS_AddCharacteristic(srvc_hdl,
                                &uuid,
                                BATTER_LEVEL_PERM,
                                prop)) == 0) {
        GATT_TRACE_ERROR("Can not add Battery Level, Battery_Instantiate() failed!");
        status = GATT_ERROR;
    } else {
        if(p_reg_info->ba_level_descr & BA_LEVEL_NOTIFY) {
            uuid.uu.uuid16 = GATT_UUID_CHAR_CLIENT_CONFIG;
            p_inst->clt_cfg_hdl  = GATTS_AddCharDescriptor(srvc_hdl,
                                   (GATT_PERM_READ | GATT_PERM_WRITE),
                                   &uuid);

            if(p_inst->clt_cfg_hdl == 0) {
                GATT_TRACE_ERROR("Add battery level client notification FAILED!");
            }
        }

        /* need presentation format descriptor? */
        if(p_reg_info->ba_level_descr & BA_LEVEL_PRE_FMT) {
            uuid.uu.uuid16 = GATT_UUID_CHAR_PRESENT_FORMAT;

            if((p_inst->pres_fmt_hdl = GATTS_AddCharDescriptor(srvc_hdl,
                                       GATT_PERM_READ,
                                       &uuid))
                    == 0) {
                GATT_TRACE_ERROR("Add battery level presentation format descriptor FAILED!");
            }
        }

        /* need presentation format descriptor? */
        if(p_reg_info->ba_level_descr & BA_LEVEL_RPT_REF) {
            uuid.uu.uuid16 = GATT_UUID_RPT_REF_DESCR;

            if((p_inst->rpt_ref_hdl = GATTS_AddCharDescriptor(srvc_hdl,
                                      GATT_PERM_READ,
                                      &uuid))
                    == 0) {
                GATT_TRACE_ERROR("Add battery level report reference descriptor FAILED!");
            }
        }

        /* start service
        */
        status = GATTS_StartService(srvc_eng_cb.gatt_if, srvc_hdl, p_reg_info->transport);
    }

    if(status != GATT_SUCCESS) {
        battery_cb.inst_id --;
        uuid.uu.uuid16 = UUID_SERVCLASS_BATTERY;
        GATTS_DeleteService(srvc_eng_cb.gatt_if, &uuid, battery_cb.inst_id);
        srvc_hdl = 0;
    }

    return srvc_hdl;
}
/*******************************************************************************
**
** Function         Battery_Rsp
**
** Description      Respond to a battery service request
**
*******************************************************************************/
void Battery_Rsp(uint8_t app_id, tGATT_STATUS st, uint8_t event, tBA_RSP_DATA *p_rsp)
{
    tBA_INST *p_inst = &battery_cb.battery_inst[0];
    tGATTS_RSP  rsp;
    uint8_t   *pp;
    uint8_t   i = 0;

    while(i < BA_MAX_INT_NUM) {
        if(p_inst->app_id == app_id && p_inst->ba_level_hdl != 0) {
            break;
        }

        i ++;
    }

    if(i == BA_MAX_INT_NUM) {
        return;
    }

    wm_memset(&rsp, 0, sizeof(tGATTS_RSP));

    if(p_inst->pending_evt == event) {
        switch(event) {
            case BA_READ_CLT_CFG_REQ:
                rsp.attr_value.handle = p_inst->pending_handle;
                rsp.attr_value.len = 2;
                pp = rsp.attr_value.value;
                UINT16_TO_STREAM(pp, p_rsp->clt_cfg);
                srvc_sr_rsp(p_inst->pending_clcb_idx, st, &rsp);
                break;

            case BA_READ_LEVEL_REQ:
                rsp.attr_value.handle = p_inst->pending_handle;
                rsp.attr_value.len = 1;
                pp = rsp.attr_value.value;
                UINT8_TO_STREAM(pp, p_rsp->ba_level);
                srvc_sr_rsp(p_inst->pending_clcb_idx, st, &rsp);
                break;

            case BA_WRITE_CLT_CFG_REQ:
                srvc_sr_rsp(p_inst->pending_clcb_idx, st, NULL);
                break;

            case BA_READ_RPT_REF_REQ:
                rsp.attr_value.handle = p_inst->pending_handle;
                rsp.attr_value.len = 2;
                pp = rsp.attr_value.value;
                UINT8_TO_STREAM(pp, p_rsp->rpt_ref.rpt_id);
                UINT8_TO_STREAM(pp, p_rsp->rpt_ref.rpt_type);
                srvc_sr_rsp(p_inst->pending_clcb_idx, st, &rsp);
                break;

            default:
                break;
        }

        p_inst->pending_clcb_idx = 0;
        p_inst->pending_evt = 0;
        p_inst->pending_handle = 0;
    }

    return;
}
/*******************************************************************************
**
** Function         Battery_Notify
**
** Description      Send battery level notification
**
*******************************************************************************/
void Battery_Notify(uint8_t app_id, BD_ADDR remote_bda, uint8_t battery_level)
{
    tBA_INST *p_inst = &battery_cb.battery_inst[0];
    uint8_t    i = 0;

    while(i < BA_MAX_INT_NUM) {
        if(p_inst->app_id == app_id && p_inst->ba_level_hdl != 0) {
            break;
        }

        i ++;
    }

    if(i == BA_MAX_INT_NUM || p_inst->clt_cfg_hdl == 0) {
        return;
    }

    srvc_sr_notify(remote_bda, p_inst->ba_level_hdl, 1, &battery_level);
}
/*******************************************************************************
**
** Function         Battery_ReadBatteryLevel
**
** Description      Read remote device Battery Level information.
**
** Returns          void
**
*******************************************************************************/
uint8_t Battery_ReadBatteryLevel(BD_ADDR peer_bda)
{
    UNUSED(peer_bda);
    /* to be implemented */
    return TRUE;
}
#endif  /* BLE_INCLUDED */
#endif
