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
 *  This is the public interface file for the object push (OP) client and
 *  server subsystem of BTA, Broadcom's Bluetooth application layer for
 *  mobile phones.
 *
 ******************************************************************************/
#ifndef BTA_OP_API_H
#define BTA_OP_API_H

#include "bta_api.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/
/* Extra Debug Code */
#ifndef BTA_OPS_DEBUG
#define BTA_OPS_DEBUG           FALSE
#endif

#ifndef BTA_OPC_DEBUG
#define BTA_OPC_DEBUG           FALSE
#endif


/* Object format */
#define BTA_OP_VCARD21_FMT          1       /* vCard 2.1 */
#define BTA_OP_VCARD30_FMT          2       /* vCard 3.0 */
#define BTA_OP_VCAL_FMT             3       /* vCal 1.0 */
#define BTA_OP_ICAL_FMT             4       /* iCal 2.0 */
#define BTA_OP_VNOTE_FMT            5       /* vNote */
#define BTA_OP_VMSG_FMT             6       /* vMessage */
#define BTA_OP_OTHER_FMT            0xFF    /* other format */

typedef uint8_t tBTA_OP_FMT;

/* Object format mask */
#define BTA_OP_VCARD21_MASK         0x01    /* vCard 2.1 */
#define BTA_OP_VCARD30_MASK         0x02    /* vCard 3.0 */
#define BTA_OP_VCAL_MASK            0x04    /* vCal 1.0 */
#define BTA_OP_ICAL_MASK            0x08    /* iCal 2.0 */
#define BTA_OP_VNOTE_MASK           0x10    /* vNote */
#define BTA_OP_VMSG_MASK            0x20    /* vMessage */
#define BTA_OP_ANY_MASK             0x40    /* Any type of object. */

typedef uint8_t tBTA_OP_FMT_MASK;

#endif /* BTA_OP_API_H */

