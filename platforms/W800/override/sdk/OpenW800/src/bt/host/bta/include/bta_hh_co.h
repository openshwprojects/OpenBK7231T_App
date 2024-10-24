/******************************************************************************
 *
 *  Copyright (C) 2005-2012 Broadcom Corporation
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
 *  This is the interface file for hid host call-out functions.
 *
 ******************************************************************************/
#ifndef BTA_HH_CO_H
#define BTA_HH_CO_H

#include "bta_hh_api.h"

typedef struct {
    uint16_t              rpt_uuid;
    uint8_t               rpt_id;
    tBTA_HH_RPT_TYPE    rpt_type;
    uint8_t               srvc_inst_id;
    uint8_t               char_inst_id;
} tBTA_HH_RPT_CACHE_ENTRY;

/*******************************************************************************
**
** Function         bta_hh_co_data
**
** Description      This callout function is executed by HH when data is received
**                  in interupt channel.
**
**
** Returns          void.
**
*******************************************************************************/
extern void bta_hh_co_data(uint8_t dev_handle, uint8_t *p_rpt, uint16_t len,
                           tBTA_HH_PROTO_MODE  mode, uint8_t sub_class,
                           uint8_t ctry_code, BD_ADDR peer_addr, uint8_t app_id);

/*******************************************************************************
**
** Function         bta_hh_co_open
**
** Description      This callout function is executed by HH when connection is
**                  opened, and application may do some device specific
**                  initialization.
**
** Returns          void.
**
*******************************************************************************/
extern void bta_hh_co_open(uint8_t dev_handle, uint8_t sub_class,
                           uint16_t attr_mask, uint8_t app_id);

/*******************************************************************************
**
** Function         bta_hh_co_close
**
** Description      This callout function is executed by HH when connection is
**                  closed, and device specific finalizatio nmay be needed.
**
** Returns          void.
**
*******************************************************************************/
extern void bta_hh_co_close(uint8_t dev_handle, uint8_t app_id);

#if (BLE_INCLUDED == TRUE && BTA_HH_LE_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         bta_hh_le_co_rpt_info
**
** Description      This callout function is to convey the report information on
**                  a HOGP device to the application. Application can save this
**                  information in NV if device is bonded and load it back when
**                  stack reboot.
**
** Parameters       remote_bda  - remote device address
**                  p_entry     - report entry pointer
**                  app_id      - application id
**
** Returns          void.
**
*******************************************************************************/
extern void bta_hh_le_co_rpt_info(BD_ADDR remote_bda,
                                  tBTA_HH_RPT_CACHE_ENTRY *p_entry,
                                  uint8_t app_id);

/*******************************************************************************
**
** Function         bta_hh_le_co_cache_load
**
** Description      This callout function is to request the application to load the
**                  cached HOGP report if there is any. When cache reading is completed,
**                  bta_hh_le_ci_cache_load() is called by the application.
**
** Parameters       remote_bda  - remote device address
**                  p_num_rpt: number of cached report
**                  app_id      - application id
**
** Returns          the acched report array
**
*******************************************************************************/
extern tBTA_HH_RPT_CACHE_ENTRY *bta_hh_le_co_cache_load(BD_ADDR remote_bda,
        uint8_t *p_num_rpt,
        uint8_t app_id);

/*******************************************************************************
**
** Function         bta_hh_le_co_reset_rpt_cache
**
** Description      This callout function is to reset the HOGP device cache.
**
** Parameters       remote_bda  - remote device address
**
** Returns          none
**
*******************************************************************************/
extern void bta_hh_le_co_reset_rpt_cache(BD_ADDR remote_bda, uint8_t app_id);

#endif /* #if (BLE_INCLUDED == TRUE && BTA_HH_LE_INCLUDED == TRUE) */
#endif /* BTA_HH_CO_H */
