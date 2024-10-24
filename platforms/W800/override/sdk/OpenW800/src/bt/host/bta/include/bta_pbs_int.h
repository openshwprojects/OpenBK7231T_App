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
 *  This is the private file for the phone book access server (PBS).
 *
 ******************************************************************************/
#ifndef BTA_PBS_INT_H
#define BTA_PBS_INT_H

#include "bt_target.h"
#include "bta_sys.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/

/* Profile supported features */
#define BTA_PBS_SUPF_DOWNLOAD     0x0001
#define BTA_PBS_SURF_BROWSE       0x0002

/* Profile supported repositories */
#define BTA_PBS_REPOSIT_LOCAL      0x01    /* Local PhoneBook */
#define BTA_PBS_REPOSIT_SIM        0x02    /* SIM card PhoneBook */

#define BTA_PBS_TARGET_UUID "\x79\x61\x35\xf0\xf0\xc5\x11\xd8\x09\x66\x08\x00\x20\x0c\x9a\x66"
#define BTA_PBS_UUID_LENGTH                 16
#define BTA_PBS_MAX_AUTH_KEY_SIZE           16  /* Must not be greater than OBX_MAX_AUTH_KEY_SIZE */

#define BTA_PBS_DEFAULT_VERSION             0x0101  /* for PBAP PSE version 1.1 */


/* Configuration structure */
typedef struct {
    uint8_t       realm_charset;          /* Server only */
    uint8_t
    userid_req;             /* TRUE if user id is required during obex authentication (Server only) */
    uint8_t       supported_features;     /* Server supported features */
    uint8_t       supported_repositories; /* Server supported repositories */

} tBTA_PBS_CFG;

#endif /* BTA_PBS_INT_H */
