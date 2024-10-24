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
 *  This interface file contains the interface to the Multi-Channel
 *  Adaptation Protocol (MCAP).
 *
 ******************************************************************************/
#ifndef MCA_API_H
#define MCA_API_H

#include "bt_target.h"
#include "l2c_api.h"

/* move the following to bt_target.h or other place later */
#define MCA_NUM_TC_TBL  ((MCA_NUM_REGS)*(MCA_NUM_LINKS)*(MCA_NUM_MDLS+1))
#define MCA_NUM_CCBS    ((MCA_NUM_REGS)*(MCA_NUM_LINKS))    /* Number of control channel control blocks */
#define MCA_NUM_DCBS    ((MCA_NUM_REGS)*(MCA_NUM_LINKS)*(MCA_NUM_MDLS)) /* Number of data channel control blocks */


/*****************************************************************************
** constants
*****************************************************************************/
/* API function return value result codes. */
#define MCA_SUCCESS                0       /* Function successful */
#define MCA_BAD_PARAMS             1       /* Invalid parameters */
#define MCA_NO_RESOURCES           2       /* Not enough resources */
#define MCA_BAD_HANDLE             3       /* Bad handle */
#define MCA_BUSY                   4       /* A procedure is already in progress */
#define MCA_WRITE_FAIL             5       /* Write failed */
#define MCA_BAD_MDL_ID             6       /* MDL ID is not valid for the current API */
typedef uint8_t tMCA_RESULT;

/* MDEP data type.  */
#define MCA_TDEP_ECHO              0       /* MDEP for echo test  */
#define MCA_TDEP_DATA              1       /* MDEP for normal data */

/* Control callback events. */
#define MCA_ERROR_RSP_EVT           0       /* error response */
#define MCA_CREATE_IND_EVT          1       /* create mdl indication */
#define MCA_CREATE_CFM_EVT          2       /* create mdl confirm */
#define MCA_RECONNECT_IND_EVT       3       /* reconnect mdl indication */
#define MCA_RECONNECT_CFM_EVT       4       /* reconnect mdl confirm */
#define MCA_ABORT_IND_EVT           5       /* abort mdl indication */
#define MCA_ABORT_CFM_EVT           6       /* abort mdl confirm */
#define MCA_DELETE_IND_EVT          7       /* delete mdl indication */
#define MCA_DELETE_CFM_EVT          8       /* delete mdl confirm */

#define MCA_SYNC_CAP_IND_EVT        0x11   /* request sync capabilities & requirements */
#define MCA_SYNC_CAP_CFM_EVT        0x12   /* indicate completion */
#define MCA_SYNC_SET_IND_EVT        0x13   /* request to set the time-stamp clock */
#define MCA_SYNC_SET_CFM_EVT        0x14   /* indicate completion */
#define MCA_SYNC_INFO_IND_EVT       0x15   /* update of the actual time-stamp clock instant from the sync slave */

#define MCA_CONNECT_IND_EVT         0x20    /* Control channel connected */
#define MCA_DISCONNECT_IND_EVT      0x21    /* Control channel disconnected */
#define MCA_OPEN_IND_EVT            0x22    /* Data channel open indication */
#define MCA_OPEN_CFM_EVT            0x23    /* Data channel open confirm */
#define MCA_CLOSE_IND_EVT           0x24    /* Data channel close indication */
#define MCA_CLOSE_CFM_EVT           0x25    /* Data channel close confirm */
#define MCA_CONG_CHG_EVT            0x26    /* congestion change event */
#define MCA_RSP_TOUT_IND_EVT        0x27    /* Control channel message response timeout */
/*****************************************************************************
**  Type Definitions
*****************************************************************************/
typedef uint8_t  tMCA_HANDLE; /* the handle for registration. 1 based index to rcb */
typedef uint8_t
tMCA_CL;     /* the handle for a control channel; reported at MCA_CONNECT_IND_EVT */
typedef uint8_t  tMCA_DEP;    /* the handle for MCA_CreateDep. This is also the local mdep_id */
typedef uint16_t
tMCA_DL;     /* the handle for the data channel. This is reported at MCA_OPEN_CFM_EVT or MCA_OPEN_IND_EVT */

/* This is the data callback function.  It is executed when MCAP has a data
** packet ready for the application.
*/
typedef void (tMCA_DATA_CBACK)(tMCA_DL mdl, BT_HDR *p_pkt);


/* This structure contains parameters which are set at registration. */
typedef struct {
    uint32_t      rsp_tout;   /* MCAP signaling response timeout */
    uint16_t      ctrl_psm;   /* L2CAP PSM for the MCAP control channel */
    uint16_t      data_psm;   /* L2CAP PSM for the MCAP data channel */
    uint16_t      sec_mask;   /* Security mask for BTM_SetSecurityLevel() */
} tMCA_REG;

/* This structure contains parameters to create a MDEP. */
typedef struct {
    uint8_t
    type;       /* MCA_TDEP_DATA, or MCA_TDEP_ECHO. a regiatration may have only one MCA_TDEP_ECHO MDEP */
    uint8_t           max_mdl;    /* The maximum number of MDLs for this MDEP (max is MCA_NUM_MDLS) */
    tMCA_DATA_CBACK *p_data_cback;  /* Data callback function */
} tMCA_CS;

#define MCA_FCS_NONE        0       /* fcs_present=FALSE */
#define MCA_FCS_BYPASS      0x10    /* fcs_present=TRUE, fcs=L2CAP_CFG_FCS_BYPASS */
#define MCA_FCS_USE         0x11    /* fcs_present=TRUE, fcs=L2CAP_CFG_FCS_USE */
#define MCA_FCS_PRESNT_MASK 0x10    /* fcs_present=TRUE */
#define MCA_FCS_USE_MASK    0x01    /* mask for fcs */
typedef uint8_t tMCA_FCS_OPT;

/* This structure contains L2CAP configuration parameters for the channel. */
typedef struct {
    tL2CAP_FCR_OPTS fcr_opt;
    uint16_t          user_rx_buf_size;
    uint16_t          user_tx_buf_size;
    uint16_t          fcr_rx_buf_size;
    uint16_t          fcr_tx_buf_size;
    tMCA_FCS_OPT    fcs;
    uint16_t          data_mtu;   /* L2CAP MTU of the MCAP data channel */
} tMCA_CHNL_CFG;


/* Header structure for callback event parameters. */
typedef struct {
    uint16_t          mdl_id;     /* The associated MDL ID */
    uint8_t           op_code;    /* The op (request/response) code */
} tMCA_EVT_HDR;

/* Response Header structure for callback event parameters. */
typedef struct {
    uint16_t          mdl_id;     /* The associated MDL ID */
    uint8_t           op_code;    /* The op (request/response) code */
    uint8_t           rsp_code;   /* The response code */
} tMCA_RSP_EVT;

/* This data structure is associated with the MCA_CREATE_IND_EVT. */
typedef struct {
    uint16_t          mdl_id;     /* The associated MDL ID */
    uint8_t           op_code;    /* The op (request/response) code */
    uint8_t           dep_id;     /* MDEP ID */
    uint8_t           cfg;        /* The configuration to negotiate */
} tMCA_CREATE_IND;

/* This data structure is associated with the MCA_CREATE_CFM_EVT. */
typedef struct {
    uint16_t          mdl_id;     /* The associated MDL ID */
    uint8_t           op_code;    /* The op (request/response) code */
    uint8_t           rsp_code;   /* The response code. */
    uint8_t           cfg;        /* The configuration to negotiate */
} tMCA_CREATE_CFM;

/* This data structure is associated with MCA_CONNECT_IND_EVT. */
typedef struct {
    BD_ADDR         bd_addr;    /* The peer address */
    uint16_t          mtu;        /* peer mtu */
} tMCA_CONNECT_IND;

/* This data structure is associated with MCA_DISCONNECT_IND_EVT. */
typedef struct {
    BD_ADDR         bd_addr;    /* The peer address */
    uint16_t          reason;     /* disconnect reason given by L2CAP */
} tMCA_DISCONNECT_IND;

/* This data structure is associated with MCA_OPEN_IND_EVT, and MCA_OPEN_CFM_EVT. */
typedef struct {
    uint16_t          mdl_id;     /* The associated MDL ID */
    tMCA_DL         mdl;        /* The handle for the data channel */
    uint16_t          mtu;        /* peer mtu */
} tMCA_DL_OPEN;

/* This data structure is associated with MCA_CLOSE_IND_EVT and MCA_CLOSE_CFM_EVT. */
typedef struct {
    uint16_t          mdl_id;     /* The associated MDL ID */
    tMCA_DL         mdl;        /* The handle for the data channel */
    uint16_t          reason;     /* disconnect reason given by L2CAP */
} tMCA_DL_CLOSE;

/* This data structure is associated with MCA_CONG_CHG_EVT. */
typedef struct {
    uint16_t          mdl_id;     /* N/A - This is a place holder */
    tMCA_DL         mdl;        /* The handle for the data channel */
    uint8_t         cong;       /* TRUE, if the channel is congested */
} tMCA_CONG_CHG;

/* Union of all control callback event data structures */
typedef union {
    tMCA_EVT_HDR        hdr;
    tMCA_RSP_EVT        rsp;
    tMCA_CREATE_IND     create_ind;
    tMCA_CREATE_CFM     create_cfm;
    tMCA_EVT_HDR        reconnect_ind;
    tMCA_RSP_EVT        reconnect_cfm;
    tMCA_EVT_HDR        abort_ind;
    tMCA_RSP_EVT        abort_cfm;
    tMCA_EVT_HDR        delete_ind;
    tMCA_RSP_EVT        delete_cfm;
    tMCA_CONNECT_IND    connect_ind;
    tMCA_DISCONNECT_IND disconnect_ind;
    tMCA_DL_OPEN        open_ind;
    tMCA_DL_OPEN        open_cfm;
    tMCA_DL_CLOSE       close_ind;
    tMCA_DL_CLOSE       close_cfm;
    tMCA_CONG_CHG       cong_chg;
} tMCA_CTRL;

/* This is the control callback function.  This function passes control events
** to the application.
*/
typedef void (tMCA_CTRL_CBACK)(tMCA_HANDLE handle, tMCA_CL mcl, uint8_t event,
                               tMCA_CTRL *p_data);


/*******************************************************************************
**
** Function         MCA_Init
**
** Description      Initialize MCAP internal control blocks.
**                  This function is called at stack start up.
**
** Returns          void
**
*******************************************************************************/
extern void MCA_Init(void);

/*******************************************************************************
**
** Function         MCA_SetTraceLevel
**
** Description      This function sets the debug trace level for MCA.
**                  If 0xff is passed, the current trace level is returned.
**
**                  Input Parameters:
**                      level:  The level to set the MCA tracing to:
**                      0xff-returns the current setting.
**                      0-turns off tracing.
**                      >= 1-Errors.
**                      >= 2-Warnings.
**                      >= 3-APIs.
**                      >= 4-Events.
**                      >= 5-Debug.
**
** Returns          The new trace level or current trace level if
**                  the input parameter is 0xff.
**
*******************************************************************************/
extern uint8_t MCA_SetTraceLevel(uint8_t level);

/*******************************************************************************
**
** Function         MCA_Register
**
** Description      This function registers an MCAP implementation.
**                  It is assumed that the control channel PSM and data channel
**                  PSM are not used by any other instances of the stack.
**                  If the given p_reg->ctrl_psm is 0, this handle is INT only.
**
** Returns          0, if failed. Otherwise, the MCA handle.
**
*******************************************************************************/
extern tMCA_HANDLE MCA_Register(tMCA_REG *p_reg, tMCA_CTRL_CBACK *p_cback);

/*******************************************************************************
**
** Function         MCA_Deregister
**
** Description      This function is called to deregister an MCAP implementation.
**                  Before this function can be called, all control and data
**                  channels must be removed with MCA_DisconnectReq and MCA_CloseReq.
**
** Returns          void
**
*******************************************************************************/
extern void MCA_Deregister(tMCA_HANDLE handle);

/*******************************************************************************
**
** Function         MCA_CreateDep
**
** Description      Create a data endpoint.  If the MDEP is created successfully,
**                  the MDEP ID is returned in *p_dep. After a data endpoint is
**                  created, an application can initiate a connection between this
**                  endpoint and an endpoint on a peer device.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
extern tMCA_RESULT MCA_CreateDep(tMCA_HANDLE handle, tMCA_DEP *p_dep, tMCA_CS *p_cs);

/*******************************************************************************
**
** Function         MCA_DeleteDep
**
** Description      Delete a data endpoint.  This function is called when
**                  the implementation is no longer using a data endpoint.
**                  If this function is called when the endpoint is connected
**                  the connection is closed and the data endpoint
**                  is removed.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
extern tMCA_RESULT MCA_DeleteDep(tMCA_HANDLE handle, tMCA_DEP dep);

/*******************************************************************************
**
** Function         MCA_ConnectReq
**
** Description      This function initiates an MCAP control channel connection
**                  to the peer device.  When the connection is completed, an
**                  MCA_CONNECT_IND_EVT is reported to the application via its
**                  control callback function.
**                  This control channel is identified by tMCA_CL.
**                  If the connection attempt fails, an MCA_DISCONNECT_IND_EVT is
**                  reported. The security mask parameter overrides the outgoing
**                  security mask set in MCA_Register().
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
extern tMCA_RESULT MCA_ConnectReq(tMCA_HANDLE handle, BD_ADDR bd_addr,
                                  uint16_t ctrl_psm,
                                  uint16_t sec_mask);

/*******************************************************************************
**
** Function         MCA_DisconnectReq
**
** Description      This function disconnect an MCAP control channel
**                  to the peer device.
**                  If associated data channel exists, they are disconnected.
**                  When the MCL is disconnected an MCA_DISCONNECT_IND_EVT is
**                  reported to the application via its control callback function.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
extern tMCA_RESULT MCA_DisconnectReq(tMCA_CL mcl);

/*******************************************************************************
**
** Function         MCA_CreateMdl
**
** Description      This function sends a CREATE_MDL request to the peer device.
**                  When the response is received, a MCA_CREATE_CFM_EVT is reported
**                  with the given MDL ID.
**                  If the response is successful, a data channel is open
**                  with the given p_chnl_cfg
**                  When the data channel is open successfully, a MCA_OPEN_CFM_EVT
**                  is reported. This data channel is identified as tMCA_DL.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
extern tMCA_RESULT MCA_CreateMdl(tMCA_CL mcl, tMCA_DEP dep, uint16_t data_psm,
                                 uint16_t mdl_id, uint8_t peer_dep_id,
                                 uint8_t cfg, const tMCA_CHNL_CFG *p_chnl_cfg);

/*******************************************************************************
**
** Function         MCA_CreateMdlRsp
**
** Description      This function sends a CREATE_MDL response to the peer device
**                  in response to a received MCA_CREATE_IND_EVT.
**                  If the rsp_code is successful, a data channel is open
**                  with the given p_chnl_cfg
**                  When the data channel is open successfully, a MCA_OPEN_IND_EVT
**                  is reported. This data channel is identified as tMCA_DL.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
extern tMCA_RESULT MCA_CreateMdlRsp(tMCA_CL mcl, tMCA_DEP dep,
                                    uint16_t mdl_id, uint8_t cfg, uint8_t rsp_code,
                                    const tMCA_CHNL_CFG *p_chnl_cfg);

/*******************************************************************************
**
** Function         MCA_CloseReq
**
** Description      Close a data channel.  When the channel is closed, an
**                  MCA_CLOSE_CFM_EVT is sent to the application via the
**                  control callback function for this handle.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
extern tMCA_RESULT MCA_CloseReq(tMCA_DL mdl);

/*******************************************************************************
**
** Function         MCA_ReconnectMdl
**
** Description      This function sends a RECONNECT_MDL request to the peer device.
**                  When the response is received, a MCA_RECONNECT_CFM_EVT is reported.
**                  If the response is successful, a data channel is open.
**                  When the data channel is open successfully, a MCA_OPEN_CFM_EVT
**                  is reported.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
extern tMCA_RESULT MCA_ReconnectMdl(tMCA_CL mcl, tMCA_DEP dep, uint16_t data_psm,
                                    uint16_t mdl_id, const tMCA_CHNL_CFG *p_chnl_cfg);

/*******************************************************************************
**
** Function         MCA_ReconnectMdlRsp
**
** Description      This function sends a RECONNECT_MDL response to the peer device
**                  in response to a MCA_RECONNECT_IND_EVT event.
**                  If the response is successful, a data channel is open.
**                  When the data channel is open successfully, a MCA_OPEN_IND_EVT
**                  is reported.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
extern tMCA_RESULT MCA_ReconnectMdlRsp(tMCA_CL mcl, tMCA_DEP dep,
                                       uint16_t mdl_id, uint8_t rsp_code,
                                       const tMCA_CHNL_CFG *p_chnl_cfg);

/*******************************************************************************
**
** Function         MCA_DataChnlCfg
**
** Description      This function initiates a data channel connection toward the
**                  connected peer device.
**                  When the data channel is open successfully, a MCA_OPEN_CFM_EVT
**                  is reported. This data channel is identified as tMCA_DL.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
extern tMCA_RESULT MCA_DataChnlCfg(tMCA_CL mcl, const tMCA_CHNL_CFG *p_chnl_cfg);

/*******************************************************************************
**
** Function         MCA_Abort
**
** Description      This function sends a ABORT_MDL request to the peer device.
**                  When the response is received, a MCA_ABORT_CFM_EVT is reported.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
extern tMCA_RESULT MCA_Abort(tMCA_CL mcl);

/*******************************************************************************
**
** Function         MCA_Delete
**
** Description      This function sends a DELETE_MDL request to the peer device.
**                  When the response is received, a MCA_DELETE_CFM_EVT is reported.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
extern tMCA_RESULT MCA_Delete(tMCA_CL mcl, uint16_t mdl_id);

/*******************************************************************************
**
** Function         MCA_WriteReq
**
** Description      Send a data packet to the peer device.
**
**                  The application passes the packet using the BT_HDR structure.
**                  The offset field must be equal to or greater than L2CAP_MIN_OFFSET.
**                  This allows enough space in the buffer for the L2CAP header.
**
**                  The memory pointed to by p_pkt must be a GKI buffer
**                  allocated by the application.  This buffer will be freed
**                  by the protocol stack; the application must not free
**                  this buffer.
**
** Returns          MCA_SUCCESS if successful, otherwise error.
**
*******************************************************************************/
extern tMCA_RESULT MCA_WriteReq(tMCA_DL mdl, BT_HDR *p_pkt);

/*******************************************************************************
**
** Function         MCA_GetL2CapChannel
**
** Description      Get the L2CAP CID used by the given data channel handle.
**
** Returns          L2CAP channel ID if successful, otherwise 0.
**
*******************************************************************************/
extern uint16_t MCA_GetL2CapChannel(tMCA_DL mdl);

#endif /* MCA_API_H */
