/******************************************************************************
 *
 *  Copyright (C) 2003-2012 Broadcom Corporation
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
 *  This is the interface file for audio gateway call-in functions.
 *
 ******************************************************************************/
#ifndef BTA_AG_CI_H
#define BTA_AG_CI_H

#include "bta_ag_api.h"

/*****************************************************************************
**  Function Declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
**
** Function         bta_ag_ci_rx_write
**
** Description      This function is called to send data to the AG when the AG
**                  is configured for AT command pass-through.  The function
**                  copies data to an event buffer and sends it.
**
** Returns          void
**
*******************************************************************************/
extern void bta_ag_ci_rx_write(uint16_t handle, char *p_data, uint16_t len);

/******************************************************************************
**
** Function         bta_ag_ci_slc_ready
**
** Description      This function is called to notify AG that SLC is up at
**                  the application. This funcion is only used when the app
**                  is running in pass-through mode.
**
** Returns          void
**
******************************************************************************/
extern void bta_ag_ci_slc_ready(uint16_t handle);

/******************************************************************************
**
** Function         bta_ag_ci_wbs_command
**
** Description      This function is called to notify AG that a WBS command is
**                  received
**
** Returns          void
**
******************************************************************************/
extern void bta_ag_ci_wbs_command(uint16_t handle, char *p_data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* BTA_AG_CI_H */
