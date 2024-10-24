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

#ifndef SRVC_ENG_INT_H
#define SRVC_ENG_INT_H

#include "bt_target.h"
#include "gatt_api.h"
#include "srvc_api.h"

#define SRVC_MAX_APPS                  GATT_CL_MAX_LCB

#define SRVC_ID_NONE            0
#define SRVC_ID_DIS             1
#define SRVC_ID_MAX             SRVC_ID_DIS

#define SRVC_ACT_IGNORE     0
#define SRVC_ACT_RSP        1
#define SRVC_ACT_PENDING    2

typedef struct {
    uint8_t         in_use;
    uint16_t          conn_id;
    uint8_t         connected;
    BD_ADDR         bda;
    uint32_t          trans_id;
    uint8_t           cur_srvc_id;

    tDIS_VALUE      dis_value;

} tSRVC_CLCB;


/* service engine control block */
typedef struct {
    tSRVC_CLCB              clcb[SRVC_MAX_APPS]; /* connection link*/
    tGATT_IF                gatt_if;
    uint8_t                 enabled;

} tSRVC_ENG_CB;



#ifdef __cplusplus
extern "C" {
#endif

/* Global GATT data */
#if GATT_DYNAMIC_MEMORY == FALSE
extern tSRVC_ENG_CB srvc_eng_cb;
#else
extern tSRVC_ENG_CB srvc_eng_cb_ptr;
#define srvc_eng_cb (*srvc_eng_cb_ptr)

#endif

extern tSRVC_CLCB *srvc_eng_find_clcb_by_conn_id(uint16_t conn_id);
extern tSRVC_CLCB *srvc_eng_find_clcb_by_bd_addr(BD_ADDR bda);
extern uint16_t srvc_eng_find_conn_id_by_bd_addr(BD_ADDR bda);


extern void srvc_eng_release_channel(uint16_t conn_id) ;
extern uint8_t srvc_eng_request_channel(BD_ADDR remote_bda, uint8_t srvc_id);
extern void srvc_sr_rsp(uint8_t clcb_idx, tGATT_STATUS st, tGATTS_RSP *p_rsp);
extern void srvc_sr_notify(BD_ADDR remote_bda, uint16_t handle, uint16_t len, uint8_t *p_value);


#ifdef __cplusplus
}
#endif
#endif
