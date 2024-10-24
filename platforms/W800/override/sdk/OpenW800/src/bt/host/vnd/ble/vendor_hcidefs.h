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

/*****************************************************************************
**
**  Name        vendor_hcidefs.h
**
**  Function    This file contains Broadcom Specific Host Controller Interface
**              definitions.
**
******************************************************************************/

#ifndef VENDOR_HCIDEFS_H
#define VENDOR_HCIDEFS_H

/*****************************************************************************
** Private address resolution VSC
******************************************************************************/

/* VSC */
#define HCI_VENDOR_BLE_RPA_VSC                (0x0155 | HCI_GRP_VENDOR_SPECIFIC)

/* Sub codes */
#define HCI_VENDOR_BLE_RPA_ENABLE       0x01
#define HCI_VENDOR_BLE_RPA_ADD_IRK      0x02
#define HCI_VENDOR_BLE_RPA_REMOVE_IRK   0x03
#define HCI_VENDOR_BLE_RPA_CLEAR_IRK    0x04
#define HCI_VENDOR_BLE_RPA_READ_IRK     0x05


/*****************************************************************************
** Advertising data payload filter VSC
******************************************************************************/

/* VSC */
#define HCI_VENDOR_BLE_PCF_VSC                (0x0157 | HCI_GRP_VENDOR_SPECIFIC)

#endif

