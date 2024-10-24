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

#ifndef SRVC_BATTERY_INT_H
#define SRVC_BATTERY_INT_H

#include "bt_target.h"
#include "srvc_api.h"
#include "gatt_api.h"

#ifndef BA_MAX_INT_NUM
#define BA_MAX_INT_NUM     4
#endif

#define BATTERY_LEVEL_SIZE      1


typedef struct {
    uint8_t           app_id;
    uint16_t          ba_level_hdl;
    uint16_t          clt_cfg_hdl;
    uint16_t          rpt_ref_hdl;
    uint16_t          pres_fmt_hdl;

    tBA_CBACK       *p_cback;

    uint16_t          pending_handle;
    uint8_t           pending_clcb_idx;
    uint8_t           pending_evt;

} tBA_INST;

typedef struct {
    tBA_INST                battery_inst[BA_MAX_INT_NUM];
    uint8_t                   inst_id;
    uint8_t                 enabled;

} tBATTERY_CB;

#ifdef __cplusplus
extern "C" {
#endif

/* Global GATT data */
#if GATT_DYNAMIC_MEMORY == FALSE
extern tBATTERY_CB battery_cb;
#else
extern tBATTERY_CB *battery_cb_ptr;
#define battery_cb (*battery_cb_ptr)
#endif


extern uint8_t battery_valid_handle_range(uint16_t handle);

extern uint8_t battery_s_write_attr_value(uint8_t clcb_idx, tGATT_WRITE_REQ *p_value,
        tGATT_STATUS *p_status);
extern uint8_t battery_s_read_attr_value(uint8_t clcb_idx, uint16_t handle, tGATT_VALUE *p_value,
        uint8_t is_long, tGATT_STATUS *p_status);



#ifdef __cplusplus
}
#endif
#endif
