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
 *  This contains constants definitions and other information from the MCAP
 *  specification.
 *
 ******************************************************************************/
#ifndef MCA_DEFS_H
#define MCA_DEFS_H

/*****************************************************************************
** constants
*****************************************************************************/
#define MCA_MIN_MTU                 48

/* standard op codes */
#define MCA_OP_ERROR_RSP            0x00   /* invalid opcode response */
#define MCA_OP_MDL_CREATE_REQ       0x01   /* create an MDL, wait for an associated data channel connection */
#define MCA_OP_MDL_CREATE_RSP       0x02   /* response to above request */
#define MCA_OP_MDL_RECONNECT_REQ    0x03   /* req to prepare to rvc a data channel conn associated with a prev MDL */
#define MCA_OP_MDL_RECONNECT_RSP    0x04   /* response to above request */
#define MCA_OP_MDL_ABORT_REQ        0x05   /* stop waiting for a data channel connection */
#define MCA_OP_MDL_ABORT_RSP        0x06   /* response to above request */
#define MCA_OP_MDL_DELETE_REQ       0x07   /* delete an MDL */
#define MCA_OP_MDL_DELETE_RSP       0x08   /* response to above request */
#define MCA_NUM_STANDARD_OPCODE     (1+MCA_OP_MDL_DELETE_RSP)

/* clock synchronization op codes */
#define MCA_OP_SYNC_CAP_REQ         0x11   /* request sync capabilities & requirements */
#define MCA_OP_SYNC_CAP_RSP         0x12   /* indicate completion */
#define MCA_OP_SYNC_SET_REQ         0x13   /* request to set the time-stamp clock */
#define MCA_OP_SYNC_SET_RSP         0x14   /* indicate completion */
#define MCA_OP_SYNC_INFO_IND        0x15   /* update of the actual time-stamp clock instant from the sync slave */

#define MCA_FIRST_SYNC_OP          MCA_OP_SYNC_CAP_REQ
#define MCA_LAST_SYNC_OP           MCA_OP_SYNC_INFO_IND

/* response code */
#define MCA_RSP_SUCCESS     0x00    /* The corresponding request was received and processed successfully. */
#define MCA_RSP_BAD_OPCODE  0x01    /* The Op Code received is not valid (i.e. neither a Standard Op Code nor a Clock Synchronization Protocol Op Code). */
#define MCA_RSP_BAD_PARAM   0x02    /* One or more of the values in the received request is invalid. */
/* MCA_RSP_BAD_PARAM shall be used when:
- The request length is invalid
- Some of the parameters have invalid values and none of the other defined Response Codes are more appropriate.
*/
#define MCA_RSP_BAD_MDEP    0x03    /* The MDEP ID referenced does not exist on this device. */
#define MCA_RSP_MDEP_BUSY   0x04    /* The requested MDEP currently has as many active MDLs as it can manage simultaneously. */
#define MCA_RSP_BAD_MDL     0x05    /* The MDL ID referenced is invalid. */
/* MCA_RSP_BAD_MDL shall be used when:
- A reserved or invalid value for MDL ID was used.
- The MDL ID referenced is not available (was never created, has been deleted, or was otherwise lost),
- The MDL ID referenced in the Abort request is not the same value that was used to initiate the PENDING state
*/
#define MCA_RSP_MDL_BUSY    0x06    /* The device is temporarily unable to complete the request. This is intended for reasons not related to the physical sensor (e.g. communication resources unavailable). */
#define MCA_RSP_BAD_OP      0x07    /* The received request is invalid in the current state. */
/* MCA_RSP_BAD_OP is used when
- Abort request was received while not in the PENDING state.
- Create, Reconnect, or Delete request was received while in the PENDING state.
- A response is received when a request is expected
*/
#define MCA_RSP_NO_RESOURCE 0x08    /* The device is temporarily unable to complete the request. This is intended for reasons relating to the physical sensor (e.g. hardware fault, low battery), or when processing resources are temporarily committed to other processes. */
#define MCA_RSP_ERROR       0x09    /* An internal error other than those listed in this table was encountered while processing the request. */
#define MCA_RSP_NO_SUPPORT  0x0A    /* The Op Code that was used in this request is not supported. */
#define MCA_RSP_CFG_REJ     0x0B    /* A configuration required by a MD_CREATE_MDL or MD_RECONNECT_MDL operation has been rejected. */

#define MCA_MAX_MDEP_ID     0x7F    /* the valid range for MDEP ID is 1-0x7F */
#define MCA_IS_VALID_MDL_ID(xxx)    (((xxx)>0) && ((xxx)<=0xFEFF))
#define MCA_ALL_MDL_ID      0xFFFF

#endif /* MCA_DEFS_H */
