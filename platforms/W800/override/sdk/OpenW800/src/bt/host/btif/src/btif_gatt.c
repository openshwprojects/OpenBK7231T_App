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

/*******************************************************************************
 *
 *  Filename:      btif_gatt.c
 *
 *  Description:   GATT Profile Bluetooth Interface
 *
 *******************************************************************************/

#define LOG_TAG "bt_btif_gatt"

#include <errno.h>
#include "bluetooth.h"
#include "bt_gatt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btif_common.h"
#include "btif_util.h"

#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))

#include "bta_api.h"
#include "bta_gatt_api.h"
#include "btif_gatt.h"
#include "btif_gatt_util.h"
#include "btif_storage.h"

#define DM_VALID_TYPE       (1<<0)
#define SERVER_VALID_TYPE   (1<<1)
#define CLIENT_VALID_TYPE   (1<<2)

/*******************************************************************************
**
** Function         btif_gatt_init
**
** Description      Initializes the GATT interface
**
** Returns          bt_status_t
**
*******************************************************************************/
static tls_bt_status_t btif_gatt_init(const btgatt_callbacks_t *callbacks, int valid_type)
{
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_gatt_cleanup
**
** Description      Closes the GATT interface
**
** Returns          void
**
*******************************************************************************/
static void  btif_gatt_cleanup(int valid_type)
{
    BTA_GATTC_Disable();
    BTA_GATTS_Disable();
}



#endif
