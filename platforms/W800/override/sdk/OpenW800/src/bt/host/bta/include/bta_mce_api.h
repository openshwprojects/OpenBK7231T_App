/******************************************************************************
 *
 *  Copyright (C) 2014 The Android Open Source Project
 *  Copyright (C) 2006-2012 Broadcom Corporation
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
 *  This is the public interface file the BTA MCE I/F
 *
 ******************************************************************************/
#ifndef BTA_MCE_API_H
#define BTA_MCE_API_H

#include "bt_target.h"
#include "bt_types.h"
#include "bta_api.h"
#include "btm_api.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/
/* status values */
#define BTA_MCE_SUCCESS             0            /* Successful operation. */
#define BTA_MCE_FAILURE             1            /* Generic failure. */
#define BTA_MCE_BUSY                2            /* Temporarily can not handle this request. */

typedef uint8_t tBTA_MCE_STATUS;

/* MCE I/F callback events */
/* events received by tBTA_MCE_DM_CBACK */
#define BTA_MCE_ENABLE_EVT               0  /* MCE enabled */
#define BTA_MCE_MAS_DISCOVERY_COMP_EVT   1  /* SDP MAS discovery complete */
#define BTA_MCE_MAX_EVT                  2  /* max number of MCE events */

#define BTA_MCE_MAX_MAS_INSTANCES 12

typedef uint16_t tBTA_MCE_EVT;

typedef struct {
    uint8_t   scn;
    char    *p_srv_name;
    uint16_t  srv_name_len;
    uint8_t   instance_id;
    uint8_t   msg_type;
} tBTA_MCE_MAS_INFO;

/* data associated with BTA_MCE_MAS_DISCOVERY_COMP_EVT */
typedef struct {
    tBTA_MCE_STATUS    status;
    BD_ADDR            remote_addr;
    int                num_mas;
    tBTA_MCE_MAS_INFO  mas[BTA_MCE_MAX_MAS_INSTANCES];
} tBTA_MCE_MAS_DISCOVERY_COMP;

/* union of data associated with MCE callback */
typedef union {
    tBTA_MCE_STATUS              status;         /* BTA_MCE_ENABLE_EVT */
    tBTA_MCE_MAS_DISCOVERY_COMP  mas_disc_comp;  /* BTA_MCE_MAS_DISCOVERY_COMP_EVT */
} tBTA_MCE;

/* MCE DM Interface callback */
typedef void (tBTA_MCE_DM_CBACK)(tBTA_MCE_EVT event, tBTA_MCE *p_data, void *user_data);

/* MCE configuration structure */
typedef struct {
    uint16_t  sdp_db_size;            /* The size of p_sdp_db */
    tSDP_DISCOVERY_DB   *p_sdp_db;  /* The data buffer to keep SDP database */
} tBTA_MCE_CFG;

/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
**
** Function         BTA_MceEnable
**
** Description      Enable the MCE I/F service. When the enable
**                  operation is complete the callback function will be
**                  called with a BTA_MCE_ENABLE_EVT. This function must
**                  be called before other functions in the MCE API are
**                  called.
**
** Returns          BTA_MCE_SUCCESS if successful.
**                  BTA_MCE_FAIL if internal failure.
**
*******************************************************************************/
extern tBTA_MCE_STATUS BTA_MceEnable(tBTA_MCE_DM_CBACK *p_cback);

/*******************************************************************************
**
** Function         BTA_MceGetRemoteMasInstances
**
** Description      This function performs service discovery for the MAS service
**                  by the given peer device. When the operation is completed
**                  the tBTA_MCE_DM_CBACK callback function will be  called with
**                  a BTA_MCE_MAS_DISCOVERY_COMP_EVT.
**
** Returns          BTA_MCE_SUCCESS, if the request is being processed.
**                  BTA_MCE_FAILURE, otherwise.
**
*******************************************************************************/
extern tBTA_MCE_STATUS BTA_MceGetRemoteMasInstances(BD_ADDR bd_addr);

#ifdef __cplusplus
}
#endif

#endif /* BTA_MCE_API_H */
