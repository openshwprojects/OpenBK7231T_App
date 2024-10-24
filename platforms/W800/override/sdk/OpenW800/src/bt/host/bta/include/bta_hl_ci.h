/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
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
 *  This is the interface file for the HL (HeaLth device profile) subsystem
 *  call-in functions.
 *
 ******************************************************************************/
#ifndef BTA_HL_CI_H
#define BTA_HL_CI_H

#include "bta_api.h"
#include "bta_hl_api.h"


/*****************************************************************************
**  Constants and Data Types
*****************************************************************************/
/**************************
**  Common Definitions
***************************/
/* Read Ready Event */
/*****************************************************************************
**  Function Declarations
*****************************************************************************/
/**************************
**  Common Functions
***************************/
/*******************************************************************************
**
** Function         bta_hl_ci_get_tx_data
**
** Description      This function is called in response to the
**                  bta_hl_co_get_tx_data call-out function.
**
** Parameters       mdl_handle -MDL handle
**                  status - BTA_MA_STATUS_OK if operation is successful
**                           BTA_MA_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void
**
*******************************************************************************/
extern  void bta_hl_ci_get_tx_data(tBTA_HL_MDL_HANDLE mdl_handle,
                                   tBTA_HL_STATUS status,
                                   uint16_t evt);

/*******************************************************************************
**
** Function         bta_hl_ci_put_rx_data
**
** Description      This function is called in response to the
**                  bta_hl_co_put_rx_data call-out function.
**
** Parameters       mdl_handle -MDL handle
**                  status - BTA_MA_STATUS_OK if operation is successful
**                           BTA_MA_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void
**
*******************************************************************************/
extern  void bta_hl_ci_put_rx_data(tBTA_HL_MDL_HANDLE mdl_handle,
                                   tBTA_HL_STATUS status,
                                   uint16_t evt);



/*******************************************************************************
**
** Function         bta_hl_ci_get_echo_data
**
** Description      This function is called in response to the
**                  bta_hl_co_get_echo_data call-out function.
**
** Parameters       mcl_handle -MCL handle
**                  status - BTA_MA_STATUS_OK if operation is successful
**                           BTA_MA_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void
**
*******************************************************************************/
extern  void bta_hl_ci_get_echo_data(tBTA_HL_MCL_HANDLE mcl_handle,
                                     tBTA_HL_STATUS status,
                                     uint16_t evt);


/*******************************************************************************
**
** Function         bta_hl_ci_put_echo_data
**
** Description      This function is called in response to the
**                  bta_hl_co_put_echo_data call-out function.
**
** Parameters       mcl_handle -MCL handle
**                  status - BTA_MA_STATUS_OK if operation is successful
**                           BTA_MA_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void
**
*******************************************************************************/
extern void bta_hl_ci_put_echo_data(tBTA_HL_MCL_HANDLE mcl_handle,
                                    tBTA_HL_STATUS status,
                                    uint16_t evt);
#endif /* BTA_HL_CI_H */
