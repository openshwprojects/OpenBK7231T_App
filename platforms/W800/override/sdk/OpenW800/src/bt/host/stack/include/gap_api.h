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

#ifndef GAP_API_H
#define GAP_API_H

#include "profiles_api.h"
#include "btm_api.h"
#include "l2c_api.h"

/*****************************************************************************
**  Constants
*****************************************************************************/
/*** GAP Error and Status Codes ***/
#define GAP_UNSUPPORTED     (GAP_ERR_GRP + 0x01)    /* Unsupported call */
#define GAP_EOINQDB         (GAP_ERR_GRP + 0x02)    /* End of inquiry database marker */
#define GAP_ERR_BUSY        (GAP_ERR_GRP + 0x03)    /* The requested function was busy */
#define GAP_ERR_NO_CTRL_BLK (GAP_ERR_GRP + 0x04)    /* No control blocks available */
#define GAP_ERR_STARTING_CMD (GAP_ERR_GRP + 0x05)   /* Error occurred while initiating the command */
#define GAP_NO_BDADDR_REC   (GAP_ERR_GRP + 0x06)    /* No Inquiry DB record for BD_ADDR */
#define GAP_ERR_ILL_MODE    (GAP_ERR_GRP + 0x07)    /* An illegal mode parameter was detected */
#define GAP_ERR_ILL_INQ_TIME (GAP_ERR_GRP + 0x08)   /* An illegal time parameter was detected */
#define GAP_ERR_ILL_PARM     (GAP_ERR_GRP + 0x09)   /* An illegal parameter was detected */
#define GAP_ERR_REM_NAME    (GAP_ERR_GRP + 0x0a)    /* Error starting the remote device name request */
#define GAP_CMD_INITIATED   (GAP_ERR_GRP + 0x0b)    /* The GAP command was started (result pending) */
#define GAP_DEVICE_NOT_UP   (GAP_ERR_GRP + 0x0c)    /* The device was not up; the request was not executed */
#define GAP_BAD_BD_ADDR     (GAP_ERR_GRP + 0x0d)    /* The bd addr passed in was not found or invalid */

#define GAP_ERR_BAD_HANDLE  (GAP_ERR_GRP + 0x0e)    /* Bad GAP handle                       */
#define GAP_ERR_BUF_OFFSET  (GAP_ERR_GRP + 0x0f)    /* Buffer offset invalid                */
#define GAP_ERR_BAD_STATE   (GAP_ERR_GRP + 0x10)    /* Connection is in invalid state       */
#define GAP_NO_DATA_AVAIL   (GAP_ERR_GRP + 0x11)    /* No data available                    */
#define GAP_ERR_CONGESTED   (GAP_ERR_GRP + 0x12)    /* BT stack is congested                */
#define GAP_ERR_SECURITY    (GAP_ERR_GRP + 0x13)    /* Security failed                      */

#define GAP_ERR_PROCESSING  (GAP_ERR_GRP + 0x14)    /* General error processing BTM request */
#define GAP_ERR_TIMEOUT     (GAP_ERR_GRP + 0x15)    /* Timeout occurred while processing cmd */
#define GAP_EVT_CONN_OPENED         0x0100
#define GAP_EVT_CONN_CLOSED         0x0101
#define GAP_EVT_CONN_DATA_AVAIL     0x0102
#define GAP_EVT_CONN_CONGESTED      0x0103
#define GAP_EVT_CONN_UNCONGESTED    0x0104
#define GAP_EVT_TX_EMPTY            0x0105

/* Values for 'chan_mode_mask' field */
/* GAP_ConnOpen() - optional channels to negotiate */
#define GAP_FCR_CHAN_OPT_BASIC      L2CAP_FCR_CHAN_OPT_BASIC
#define GAP_FCR_CHAN_OPT_ERTM       L2CAP_FCR_CHAN_OPT_ERTM
#define GAP_FCR_CHAN_OPT_STREAM     L2CAP_FCR_CHAN_OPT_STREAM
/*** used in connection variables and functions ***/
#define GAP_INVALID_HANDLE      0xFFFF

/* This is used to change the criteria for AMP  */
#define GAP_PROTOCOL_ID         (UUID_PROTOCOL_UDP)


#ifndef  GAP_PREFER_CONN_INT_MAX
#define  GAP_PREFER_CONN_INT_MAX         BTM_BLE_CONN_INT_MIN
#endif

#ifndef  GAP_PREFER_CONN_INT_MIN
#define  GAP_PREFER_CONN_INT_MIN         BTM_BLE_CONN_INT_MIN
#endif

#ifndef  GAP_PREFER_CONN_LATENCY
#define  GAP_PREFER_CONN_LATENCY         0
#endif

#ifndef  GAP_PREFER_CONN_SP_TOUT
#define  GAP_PREFER_CONN_SP_TOUT         2000
#endif

/*****************************************************************************
**  Type Definitions
*****************************************************************************/
/*
** Callback function for connection services
*/
typedef void (tGAP_CONN_CALLBACK)(uint16_t gap_handle, uint16_t event);

/*
** Define the callback function prototypes.  Parameters are specific
** to each event and are described below
*/
typedef void (tGAP_CALLBACK)(uint16_t event, void *p_data);


/* Definition of the GAP_FindAddrByName results structure */
typedef struct {
    uint16_t       status;
    BD_ADDR      bd_addr;
    tBTM_BD_NAME devname;
} tGAP_FINDADDR_RESULTS;

typedef struct {
    uint16_t      int_min;
    uint16_t      int_max;
    uint16_t      latency;
    uint16_t      sp_tout;
} tGAP_BLE_PREF_PARAM;

typedef union {
    tGAP_BLE_PREF_PARAM     conn_param;
    BD_ADDR                 reconn_bda;
    uint16_t                  icon;
    uint8_t                   *p_dev_name;
    uint8_t                   addr_resolution;

} tGAP_BLE_ATTR_VALUE;

typedef void (tGAP_BLE_CMPL_CBACK)(uint8_t status, BD_ADDR addr, uint16_t length, char *p_name);


/*****************************************************************************
**  External Function Declarations
*****************************************************************************/

/*** Functions for L2CAP connection interface ***/

/*******************************************************************************
**
** Function         GAP_ConnOpen
**
** Description      This function is called to open a generic L2CAP connection.
**
** Returns          handle of the connection if successful, else GAP_INVALID_HANDLE
**
*******************************************************************************/
extern uint16_t GAP_ConnOpen(char *p_serv_name, uint8_t service_id, uint8_t is_server,
                             BD_ADDR p_rem_bda, uint16_t psm, tL2CAP_CFG_INFO *p_cfg,
                             tL2CAP_ERTM_INFO *ertm_info,
                             uint16_t security, uint8_t chan_mode_mask,
                             tGAP_CONN_CALLBACK *p_cb, tBT_TRANSPORT transport);

/*******************************************************************************
**
** Function         GAP_ConnClose
**
** Description      This function is called to close a connection.
**
** Returns          BT_PASS             - closed OK
**                  GAP_ERR_BAD_HANDLE  - invalid handle
**
*******************************************************************************/
extern uint16_t GAP_ConnClose(uint16_t gap_handle);

/*******************************************************************************
**
** Function         GAP_ConnReadData
**
** Description      GKI buffer unaware application will call this function
**                  after receiving GAP_EVT_RXDATA event. A data copy is made
**                  into the receive buffer parameter.
**
** Returns          BT_PASS             - data read
**                  GAP_ERR_BAD_HANDLE  - invalid handle
**                  GAP_NO_DATA_AVAIL   - no data available
**
*******************************************************************************/
extern uint16_t GAP_ConnReadData(uint16_t gap_handle, uint8_t *p_data,
                                 uint16_t max_len, uint16_t *p_len);

/*******************************************************************************
**
** Function         GAP_GetRxQueueCnt
**
** Description      This function return number of bytes on the rx queue.
**
** Parameters:      handle     - Handle returned in the GAP_ConnOpen
**                  p_rx_queue_count - Pointer to return queue count in.
**
**
*******************************************************************************/
extern int GAP_GetRxQueueCnt(uint16_t handle, uint32_t *p_rx_queue_count);

/*******************************************************************************
**
** Function         GAP_ConnBTRead
**
** Description      GKI buffer aware applications will call this function after
**                  receiving an GAP_EVT_RXDATA event to process the incoming
**                  data buffer.
**
** Returns          BT_PASS             - data read
**                  GAP_ERR_BAD_HANDLE  - invalid handle
**                  GAP_NO_DATA_AVAIL   - no data available
**
*******************************************************************************/
extern uint16_t GAP_ConnBTRead(uint16_t gap_handle, BT_HDR **pp_buf);

/*******************************************************************************
**
** Function         GAP_ConnWriteData
**
** Description      GKI buffer unaware application will call this function
**                  to send data to the connection. A data copy is made into a GKI
**                  buffer.
**
** Returns          BT_PASS                 - data read
**                  GAP_ERR_BAD_HANDLE      - invalid handle
**                  GAP_ERR_BAD_STATE       - connection not established
**                  GAP_CONGESTION          - system is congested
**
*******************************************************************************/
extern uint16_t GAP_ConnWriteData(uint16_t gap_handle, uint8_t *p_data,
                                  uint16_t max_len, uint16_t *p_len);

/*******************************************************************************
**
** Function         GAP_ConnReconfig
**
** Description      Applications can call this function to reconfigure the connection.
**
** Returns          BT_PASS                 - config process started
**                  GAP_ERR_BAD_HANDLE      - invalid handle
**
*******************************************************************************/
extern uint16_t GAP_ConnReconfig(uint16_t gap_handle, tL2CAP_CFG_INFO *p_cfg);

/*******************************************************************************
**
** Function         GAP_ConnSetIdleTimeout
**
** Description      Higher layers call this function to set the idle timeout for
**                  a connection, or for all future connections. The "idle timeout"
**                  is the amount of time that a connection can remain up with
**                  no L2CAP channels on it. A timeout of zero means that the
**                  connection will be torn down immediately when the last channel
**                  is removed. A timeout of 0xFFFF means no timeout. Values are
**                  in seconds.
**
** Returns          BT_PASS                 - config process started
**                  GAP_ERR_BAD_HANDLE      - invalid handle
**
*******************************************************************************/
extern uint16_t GAP_ConnSetIdleTimeout(uint16_t gap_handle, uint16_t timeout);

/*******************************************************************************
**
** Function         GAP_ConnGetRemoteAddr
**
** Description      This function is called to get the remote BD address
**                  of a connection.
**
** Returns          BT_PASS             - closed OK
**                  GAP_ERR_BAD_HANDLE  - invalid handle
**
*******************************************************************************/
extern uint8_t *GAP_ConnGetRemoteAddr(uint16_t gap_handle);

/*******************************************************************************
**
** Function         GAP_ConnGetRemMtuSize
**
** Description      Returns the remote device's MTU size.
**
** Returns          uint16_t - maximum size buffer that can be transmitted to the peer
**
*******************************************************************************/
extern uint16_t GAP_ConnGetRemMtuSize(uint16_t gap_handle);

/*******************************************************************************
**
** Function         GAP_ConnGetL2CAPCid
**
** Description      Returns the L2CAP channel id
**
** Parameters:      handle      - Handle of the connection
**
** Returns          uint16_t      - The L2CAP channel id
**                  0, if error
**
*******************************************************************************/
extern uint16_t GAP_ConnGetL2CAPCid(uint16_t gap_handle);

/*******************************************************************************
**
** Function         GAP_SetTraceLevel
**
** Description      This function sets the trace level for GAP.  If called with
**                  a value of 0xFF, it simply returns the current trace level.
**
** Returns          The new or current trace level
**
*******************************************************************************/
extern uint8_t GAP_SetTraceLevel(uint8_t new_level);

/*******************************************************************************
**
** Function         GAP_Init
**
** Description      Initializes the control blocks used by GAP.
**                  This routine should not be called except once per
**                      stack invocation.
**
** Returns          Nothing
**
*******************************************************************************/
extern void GAP_Init(void);

#if (BLE_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         GAP_BleAttrDBUpdate
**
** Description      update GAP local BLE attribute database.
**
** Returns          Nothing
**
*******************************************************************************/
extern void GAP_BleAttrDBUpdate(uint16_t attr_uuid, tGAP_BLE_ATTR_VALUE *p_value);


/*******************************************************************************
**
** Function         GAP_BleReadPeerPrefConnParams
**
** Description      Start a process to read a connected peripheral's preferred
**                  connection parameters
**
** Returns          TRUE if read started, else FALSE if GAP is busy
**
*******************************************************************************/
extern uint8_t GAP_BleReadPeerPrefConnParams(BD_ADDR peer_bda);

/*******************************************************************************
**
** Function         GAP_BleReadPeerDevName
**
** Description      Start a process to read a connected peripheral's device name.
**
** Returns          TRUE if request accepted
**
*******************************************************************************/
extern uint8_t GAP_BleReadPeerDevName(BD_ADDR peer_bda, tGAP_BLE_CMPL_CBACK *p_cback);


/*******************************************************************************
**
** Function         GAP_BleReadPeerAddressResolutionCap
**
** Description      Start a process to read peer address resolution capability
**
** Returns          TRUE if request accepted
**
*******************************************************************************/
extern uint8_t GAP_BleReadPeerAddressResolutionCap(BD_ADDR peer_bda,
        tGAP_BLE_CMPL_CBACK *p_cback);

/*******************************************************************************
**
** Function         GAP_BleCancelReadPeerDevName
**
** Description      Cancel reading a peripheral's device name.
**
** Returns          TRUE if request accepted
**
*******************************************************************************/
extern uint8_t GAP_BleCancelReadPeerDevName(BD_ADDR peer_bda);


#endif

#endif  /* GAP_API_H */
