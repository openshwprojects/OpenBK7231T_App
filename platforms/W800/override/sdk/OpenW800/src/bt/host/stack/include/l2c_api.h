/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
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
 *  this file contains the L2CAP API definitions
 *
 ******************************************************************************/
#ifndef L2C_API_H
#define L2C_API_H

#include <stdbool.h>

#include "bt_target.h"
#include "l2cdefs.h"
#include "hcidefs.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

/* Define the minimum offset that L2CAP needs in a buffer. This is made up of
** HCI type(1), len(2), handle(2), L2CAP len(2) and CID(2) => 9
*/
#define L2CAP_MIN_OFFSET    13     /* plus control(2), SDU length(2) */

#define L2CAP_LCC_SDU_LENGTH    2
#define L2CAP_LCC_OFFSET        (L2CAP_MIN_OFFSET + L2CAP_LCC_SDU_LENGTH)  /* plus SDU length(2) */

/* ping result codes */
#define L2CAP_PING_RESULT_OK        0       /* Ping reply received OK     */
#define L2CAP_PING_RESULT_NO_LINK   1       /* Link could not be setup    */
#define L2CAP_PING_RESULT_NO_RESP   2       /* Remote L2CAP did not reply */

/* result code for L2CA_DataWrite() */
#define L2CAP_DW_FAILED        FALSE
#define L2CAP_DW_SUCCESS       TRUE
#define L2CAP_DW_CONGESTED     2

/* Values for priority parameter to L2CA_SetAclPriority */
#define L2CAP_PRIORITY_NORMAL       0
#define L2CAP_PRIORITY_HIGH         1

/* Values for priority parameter to L2CA_SetTxPriority */
#define L2CAP_CHNL_PRIORITY_HIGH    0
#define L2CAP_CHNL_PRIORITY_MEDIUM  1
#define L2CAP_CHNL_PRIORITY_LOW     2

typedef uint8_t tL2CAP_CHNL_PRIORITY;

/* Values for Tx/Rx data rate parameter to L2CA_SetChnlDataRate */
#define L2CAP_CHNL_DATA_RATE_HIGH       3
#define L2CAP_CHNL_DATA_RATE_MEDIUM     2
#define L2CAP_CHNL_DATA_RATE_LOW        1
#define L2CAP_CHNL_DATA_RATE_NO_TRAFFIC 0

typedef uint8_t tL2CAP_CHNL_DATA_RATE;

/* Data Packet Flags  (bits 2-15 are reserved) */
/* layer specific 14-15 bits are used for FCR SAR */
#define L2CAP_FLUSHABLE_MASK        0x0003
#define L2CAP_FLUSHABLE_CH_BASED    0x0000
#define L2CAP_FLUSHABLE_PKT         0x0001
#define L2CAP_NON_FLUSHABLE_PKT     0x0002


/* L2CA_FlushChannel num_to_flush definitions */
#define L2CAP_FLUSH_CHANS_ALL       0xffff
#define L2CAP_FLUSH_CHANS_GET       0x0000


/* special CID for Multi-AV for reporting congestion */
#define L2CAP_MULTI_AV_CID          0

/* length of the HCI header block */
/* HCI header(4) + SNK count(1) + FCR bits(1) + AV data length(2) */
#define L2CAP_MULTI_AV_HCI_HDR_LEN  8

/* length of padding for 4 bytes align */
#define L2CAP_MULTI_AV_PADDING_LEN  2

/* length of the HCI header block with padding for FCR */
/* HCI header(4) + SNK count(1) + FCR bits(1) + AV data length(2) + padding(2) */
#define L2CAP_MULTI_AV_HCI_HDR_LEN_WITH_PADDING 10

/* length of the L2CAP header block */
/* HCI header(4) + L2CAP header(4) + padding(4) or control word(2) + FCS(2) */
#define L2CAP_MULTI_AV_L2C_HDR_LEN  12

/* definition used for L2CA_SetDesireRole */
#define L2CAP_ROLE_SLAVE            HCI_ROLE_SLAVE
#define L2CAP_ROLE_MASTER           HCI_ROLE_MASTER
#define L2CAP_ROLE_ALLOW_SWITCH     0x80    /* set this bit to allow switch at create conn */
#define L2CAP_ROLE_DISALLOW_SWITCH  0x40    /* set this bit to disallow switch at create conn */
#define L2CAP_ROLE_CHECK_SWITCH     0xC0


/* Values for 'allowed_modes' field passed in structure tL2CAP_ERTM_INFO
*/
#define L2CAP_FCR_CHAN_OPT_BASIC    (1 << L2CAP_FCR_BASIC_MODE)
#define L2CAP_FCR_CHAN_OPT_ERTM     (1 << L2CAP_FCR_ERTM_MODE)
#define L2CAP_FCR_CHAN_OPT_STREAM   (1 << L2CAP_FCR_STREAM_MODE)

#define L2CAP_FCR_CHAN_OPT_ALL_MASK (L2CAP_FCR_CHAN_OPT_BASIC | L2CAP_FCR_CHAN_OPT_ERTM | L2CAP_FCR_CHAN_OPT_STREAM)

/* Validity check for PSM.  PSM values must be odd.  Also, all PSM values must
** be assigned such that the least significant bit of the most sigificant
** octet equals zero.
*/
#define L2C_INVALID_PSM(psm)       (((psm) & 0x0101) != 0x0001)
#define L2C_IS_VALID_PSM(psm)      (((psm) & 0x0101) == 0x0001)
#define L2C_IS_VALID_LE_PSM(psm)   (((psm) > 0x0000) && ((psm) < 0x0100))

/*****************************************************************************
**  Type Definitions
*****************************************************************************/

typedef struct {
#define L2CAP_FCR_BASIC_MODE    0x00
#define L2CAP_FCR_ERTM_MODE     0x03
#define L2CAP_FCR_STREAM_MODE   0x04
#define L2CAP_FCR_LE_COC_MODE   0x05

    uint8_t  mode;

    uint8_t  tx_win_sz;
    uint8_t  max_transmit;
    uint16_t rtrans_tout;
    uint16_t mon_tout;
    uint16_t mps;
} tL2CAP_FCR_OPTS;

/* Define a structure to hold the configuration parameters. Since the
** parameters are optional, for each parameter there is a boolean to
** use to signify its presence or absence.
*/
typedef struct {
    uint16_t      result;                 /* Only used in confirm messages */
    uint8_t     mtu_present;
    uint16_t      mtu;
    uint8_t     qos_present;
    FLOW_SPEC   qos;
    uint8_t     flush_to_present;
    uint16_t      flush_to;
    uint8_t     fcr_present;
    tL2CAP_FCR_OPTS fcr;
    uint8_t     fcs_present;            /* Optionally bypasses FCS checks */
    uint8_t       fcs;                    /* '0' if desire is to bypass FCS, otherwise '1' */
    uint8_t               ext_flow_spec_present;
    tHCI_EXT_FLOW_SPEC    ext_flow_spec;
    uint16_t      flags;                  /* bit 0: 0-no continuation, 1-continuation */
} tL2CAP_CFG_INFO;

/* Define a structure to hold the configuration parameter for LE L2CAP connection
** oriented channels.
*/
typedef struct {
    uint16_t  mtu;
    uint16_t  mps;
    uint16_t  credits;
} tL2CAP_LE_CFG_INFO;

/* L2CAP channel configured field bitmap */
#define L2CAP_CH_CFG_MASK_MTU           0x0001
#define L2CAP_CH_CFG_MASK_QOS           0x0002
#define L2CAP_CH_CFG_MASK_FLUSH_TO      0x0004
#define L2CAP_CH_CFG_MASK_FCR           0x0008
#define L2CAP_CH_CFG_MASK_FCS           0x0010
#define L2CAP_CH_CFG_MASK_EXT_FLOW_SPEC 0x0020

typedef uint16_t tL2CAP_CH_CFG_BITS;

/*********************************
**  Callback Functions Prototypes
**********************************/

/* Connection indication callback prototype. Parameters are
**              BD Address of remote
**              Local CID assigned to the connection
**              PSM that the remote wants to connect to
**              Identifier that the remote sent
*/
typedef void (tL2CA_CONNECT_IND_CB)(BD_ADDR, uint16_t, uint16_t, uint8_t);


/* Connection confirmation callback prototype. Parameters are
**              Local CID
**              Result - 0 = connected, non-zero means failure reason
*/
typedef void (tL2CA_CONNECT_CFM_CB)(uint16_t, uint16_t);


/* Connection pending callback prototype. Parameters are
**              Local CID
*/
typedef void (tL2CA_CONNECT_PND_CB)(uint16_t);


/* Configuration indication callback prototype. Parameters are
**              Local CID assigned to the connection
**              Pointer to configuration info
*/
typedef void (tL2CA_CONFIG_IND_CB)(uint16_t, tL2CAP_CFG_INFO *);


/* Configuration confirm callback prototype. Parameters are
**              Local CID assigned to the connection
**              Pointer to configuration info
*/
typedef void (tL2CA_CONFIG_CFM_CB)(uint16_t, tL2CAP_CFG_INFO *);


/* Disconnect indication callback prototype. Parameters are
**              Local CID
**              Boolean whether upper layer should ack this
*/
typedef void (tL2CA_DISCONNECT_IND_CB)(uint16_t, uint8_t);


/* Disconnect confirm callback prototype. Parameters are
**              Local CID
**              Result
*/
typedef void (tL2CA_DISCONNECT_CFM_CB)(uint16_t, uint16_t);


/* QOS Violation indication callback prototype. Parameters are
**              BD Address of violating device
*/
typedef void (tL2CA_QOS_VIOLATION_IND_CB)(BD_ADDR);


/* Data received indication callback prototype. Parameters are
**              Local CID
**              Address of buffer
*/
typedef void (tL2CA_DATA_IND_CB)(uint16_t, BT_HDR *);


/* Echo response callback prototype. Note that this is not included in the
** registration information, but is passed to L2CAP as part of the API to
** actually send an echo request. Parameters are
**              Result
*/
typedef void (tL2CA_ECHO_RSP_CB)(uint16_t);


/* Callback function prototype to pass broadcom specific echo response  */
/* to the upper layer                                                   */
typedef void (tL2CA_ECHO_DATA_CB)(BD_ADDR, uint16_t, uint8_t *);


/* Congestion status callback protype. This callback is optional. If
** an application tries to send data when the transmit queue is full,
** the data will anyways be dropped. The parameter is:
**              Local CID
**              TRUE if congested, FALSE if uncongested
*/
typedef void (tL2CA_CONGESTION_STATUS_CB)(uint16_t, uint8_t);

/* Callback prototype for number of packets completed events.
** This callback notifies the application when Number of Completed Packets
** event has been received.
** This callback is originally designed for 3DG devices.
** The parameter is:
**          peer BD_ADDR
*/
typedef void (tL2CA_NOCP_CB)(BD_ADDR);

/* Transmit complete callback protype. This callback is optional. If
** set, L2CAP will call it when packets are sent or flushed. If the
** count is 0xFFFF, it means all packets are sent for that CID (eRTM
** mode only). The parameters are:
**              Local CID
**              Number of SDUs sent or dropped
*/
typedef void (tL2CA_TX_COMPLETE_CB)(uint16_t, uint16_t);

/* Define the structure that applications use to register with
** L2CAP. This structure includes callback functions. All functions
** MUST be provided, with the exception of the "connect pending"
** callback and "congestion status" callback.
*/
typedef struct {
    tL2CA_CONNECT_IND_CB        *pL2CA_ConnectInd_Cb;
    tL2CA_CONNECT_CFM_CB        *pL2CA_ConnectCfm_Cb;
    tL2CA_CONNECT_PND_CB        *pL2CA_ConnectPnd_Cb;
    tL2CA_CONFIG_IND_CB         *pL2CA_ConfigInd_Cb;
    tL2CA_CONFIG_CFM_CB         *pL2CA_ConfigCfm_Cb;
    tL2CA_DISCONNECT_IND_CB     *pL2CA_DisconnectInd_Cb;
    tL2CA_DISCONNECT_CFM_CB     *pL2CA_DisconnectCfm_Cb;
    tL2CA_QOS_VIOLATION_IND_CB  *pL2CA_QoSViolationInd_Cb;
    tL2CA_DATA_IND_CB           *pL2CA_DataInd_Cb;
    tL2CA_CONGESTION_STATUS_CB  *pL2CA_CongestionStatus_Cb;
    tL2CA_TX_COMPLETE_CB        *pL2CA_TxComplete_Cb;

} tL2CAP_APPL_INFO;

/* Define the structure that applications use to create or accept
** connections with enhanced retransmission mode.
*/
typedef struct {
    uint8_t       preferred_mode;
    uint8_t       allowed_modes;
    uint16_t      user_rx_buf_size;
    uint16_t      user_tx_buf_size;
    uint16_t      fcr_rx_buf_size;
    uint16_t      fcr_tx_buf_size;

} tL2CAP_ERTM_INFO;

#define L2CA_REGISTER(a,b,c)              L2CA_Register(a,(tL2CAP_APPL_INFO *)b)
#define L2CA_DEREGISTER(a)                L2CA_Deregister(a)
#define L2CA_CONNECT_REQ(a,b,c)           L2CA_ErtmConnectReq(a,b,c)
#define L2CA_CONNECT_RSP(a,b,c,d,e,f)     L2CA_ErtmConnectRsp(a,b,c,d,e,f)
#define L2CA_CONFIG_REQ(a,b)              L2CA_ConfigReq(a,b)
#define L2CA_CONFIG_RSP(a,b)              L2CA_ConfigRsp(a,b)
#define L2CA_DISCONNECT_REQ(a)            L2CA_DisconnectReq(a)
#define L2CA_DISCONNECT_RSP(a)            L2CA_DisconnectRsp(a)
#define L2CA_DATA_WRITE(a, b)             L2CA_DataWrite(a, b)
#define L2CA_REGISTER_COC(a,b,c)          L2CA_RegisterLECoc(a,(tL2CAP_APPL_INFO *)b)
#define L2CA_DEREGISTER_COC(a)            L2CA_DeregisterLECoc(a)
#define L2CA_CONNECT_COC_REQ(a,b,c)       L2CA_ConnectLECocReq(a,b,c)
#define L2CA_CONNECT_COC_RSP(a,b,c,d,e,f) L2CA_ConnectLECocRsp(a,b,c,d,e,f)
#define L2CA_GET_PEER_COC_CONFIG(a, b)    L2CA_GetPeerLECocConfig(a, b)

/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
**
** Function         L2CA_Register
**
** Description      Other layers call this function to register for L2CAP
**                  services.
**
** Returns          PSM to use or zero if error. Typically, the PSM returned
**                  is the same as was passed in, but for an outgoing-only
**                  connection to a dynamic PSM, a "virtual" PSM is returned
**                  and should be used in the calls to L2CA_ConnectReq() and
**                  BTM_SetSecurityLevel().
**
*******************************************************************************/
extern uint16_t L2CA_Register(uint16_t psm, tL2CAP_APPL_INFO *p_cb_info);

/*******************************************************************************
**
** Function         L2CA_Deregister
**
** Description      Other layers call this function to deregister for L2CAP
**                  services.
**
** Returns          void
**
*******************************************************************************/
extern void L2CA_Deregister(uint16_t psm);

/*******************************************************************************
**
** Function         L2CA_AllocatePSM
**
** Description      Other layers call this function to find an unused PSM for L2CAP
**                  services.
**
** Returns          PSM to use.
**
*******************************************************************************/
extern uint16_t L2CA_AllocatePSM(void);

/*******************************************************************************
**
** Function         L2CA_ConnectReq
**
** Description      Higher layers call this function to create an L2CAP connection.
**                  Note that the connection is not established at this time, but
**                  connection establishment gets started. The callback function
**                  will be invoked when connection establishes or fails.
**
** Returns          the CID of the connection, or 0 if it failed to start
**
*******************************************************************************/
extern uint16_t L2CA_ConnectReq(uint16_t psm, BD_ADDR p_bd_addr);

/*******************************************************************************
**
** Function         L2CA_ConnectRsp
**
** Description      Higher layers call this function to accept an incoming
**                  L2CAP connection, for which they had gotten an connect
**                  indication callback.
**
** Returns          TRUE for success, FALSE for failure
**
*******************************************************************************/
extern uint8_t L2CA_ConnectRsp(BD_ADDR p_bd_addr, uint8_t id, uint16_t lcid,
                               uint16_t result, uint16_t status);

/*******************************************************************************
**
** Function         L2CA_ErtmConnectReq
**
** Description      Higher layers call this function to create an L2CAP connection
**                  that needs to use Enhanced Retransmission Mode.
**                  Note that the connection is not established at this time, but
**                  connection establishment gets started. The callback function
**                  will be invoked when connection establishes or fails.
**
** Returns          the CID of the connection, or 0 if it failed to start
**
*******************************************************************************/
extern uint16_t L2CA_ErtmConnectReq(uint16_t psm, BD_ADDR p_bd_addr,
                                    tL2CAP_ERTM_INFO *p_ertm_info);
#if (BLE_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         L2CA_RegisterLECoc
**
** Description      Other layers call this function to register for L2CAP
**                  Connection Oriented Channel.
**
** Returns          PSM to use or zero if error. Typically, the PSM returned
**                  is the same as was passed in, but for an outgoing-only
**                  connection to a dynamic PSM, a "virtual" PSM is returned
**                  and should be used in the calls to L2CA_ConnectLECocReq()
**                  and BTM_SetSecurityLevel().
**
*******************************************************************************/
extern uint16_t L2CA_RegisterLECoc(uint16_t psm, tL2CAP_APPL_INFO *p_cb_info);

/*******************************************************************************
**
** Function         L2CA_DeregisterLECoc
**
** Description      Other layers call this function to deregister for L2CAP
**                  Connection Oriented Channel.
**
** Returns          void
**
*******************************************************************************/
extern void L2CA_DeregisterLECoc(uint16_t psm);

/*******************************************************************************
**
** Function         L2CA_ConnectLECocReq
**
** Description      Higher layers call this function to create an L2CAP LE COC.
**                  Note that the connection is not established at this time, but
**                  connection establishment gets started. The callback function
**                  will be invoked when connection establishes or fails.
**
** Returns          the CID of the connection, or 0 if it failed to start
**
*******************************************************************************/
extern uint16_t L2CA_ConnectLECocReq(uint16_t psm, BD_ADDR p_bd_addr, tL2CAP_LE_CFG_INFO *p_cfg);

/*******************************************************************************
**
** Function         L2CA_ConnectLECocRsp
**
** Description      Higher layers call this function to accept an incoming
**                  L2CAP LE COC connection, for which they had gotten an connect
**                  indication callback.
**
** Returns          TRUE for success, FALSE for failure
**
*******************************************************************************/
extern uint8_t L2CA_ConnectLECocRsp(BD_ADDR p_bd_addr, uint8_t id, uint16_t lcid, uint16_t result,
                                    uint16_t status, tL2CAP_LE_CFG_INFO *p_cfg);

/*******************************************************************************
**
**  Function         L2CA_GetPeerLECocConfig
**
**  Description      Get peers configuration for LE Connection Oriented Channel.
**
**  Return value:    TRUE if peer is connected
**
*******************************************************************************/
extern uint8_t L2CA_GetPeerLECocConfig(uint16_t lcid, tL2CAP_LE_CFG_INFO *peer_cfg);
#endif

// This function sets the callback routines for the L2CAP connection referred to by
// |local_cid|. The callback routines can only be modified for outgoing connections
// established by |L2CA_ConnectReq| or accepted incoming connections. |callbacks|
// must not be NULL. This function returns true if the callbacks could be updated,
// false if not (e.g. |local_cid| was not found).
uint8_t L2CA_SetConnectionCallbacks(uint16_t local_cid, const tL2CAP_APPL_INFO *callbacks);

/*******************************************************************************
**
** Function         L2CA_ErtmConnectRsp
**
** Description      Higher layers call this function to accept an incoming
**                  L2CAP connection, for which they had gotten an connect
**                  indication callback, and for which the higher layer wants
**                  to use Enhanced Retransmission Mode.
**
** Returns          TRUE for success, FALSE for failure
**
*******************************************************************************/
extern uint8_t  L2CA_ErtmConnectRsp(BD_ADDR p_bd_addr, uint8_t id, uint16_t lcid,
                                    uint16_t result, uint16_t status,
                                    tL2CAP_ERTM_INFO *p_ertm_info);

/*******************************************************************************
**
** Function         L2CA_ConfigReq
**
** Description      Higher layers call this function to send configuration.
**
** Returns          TRUE if configuration sent, else FALSE
**
*******************************************************************************/
extern uint8_t L2CA_ConfigReq(uint16_t cid, tL2CAP_CFG_INFO *p_cfg);

/*******************************************************************************
**
** Function         L2CA_ConfigRsp
**
** Description      Higher layers call this function to send a configuration
**                  response.
**
** Returns          TRUE if configuration response sent, else FALSE
**
*******************************************************************************/
extern uint8_t L2CA_ConfigRsp(uint16_t cid, tL2CAP_CFG_INFO *p_cfg);

/*******************************************************************************
**
** Function         L2CA_DisconnectReq
**
** Description      Higher layers call this function to disconnect a channel.
**
** Returns          TRUE if disconnect sent, else FALSE
**
*******************************************************************************/
extern uint8_t L2CA_DisconnectReq(uint16_t cid);

/*******************************************************************************
**
** Function         L2CA_DisconnectRsp
**
** Description      Higher layers call this function to acknowledge the
**                  disconnection of a channel.
**
** Returns          void
**
*******************************************************************************/
extern uint8_t L2CA_DisconnectRsp(uint16_t cid);

/*******************************************************************************
**
** Function         L2CA_DataWrite
**
** Description      Higher layers call this function to write data.
**
** Returns          L2CAP_DW_SUCCESS, if data accepted, else FALSE
**                  L2CAP_DW_CONGESTED, if data accepted and the channel is congested
**                  L2CAP_DW_FAILED, if error
**
*******************************************************************************/
extern uint8_t L2CA_DataWrite(uint16_t cid, BT_HDR *p_data);

/*******************************************************************************
**
** Function         L2CA_Ping
**
** Description      Higher layers call this function to send an echo request.
**
** Returns          TRUE if echo request sent, else FALSE.
**
*******************************************************************************/
extern uint8_t L2CA_Ping(BD_ADDR p_bd_addr, tL2CA_ECHO_RSP_CB *p_cb);

/*******************************************************************************
**
** Function         L2CA_Echo
**
** Description      Higher layers call this function to send an echo request
**                  with application-specific data.
**
** Returns          TRUE if echo request sent, else FALSE.
**
*******************************************************************************/
extern uint8_t  L2CA_Echo(BD_ADDR p_bd_addr, BT_HDR *p_data, tL2CA_ECHO_DATA_CB *p_callback);

// Given a local channel identifier, |lcid|, this function returns the bound remote
// channel identifier, |rcid|, and the ACL link handle, |handle|. If |lcid| is not
// known or is invalid, this function returns false and does not modify the values
// pointed at by |rcid| and |handle|. |rcid| and |handle| may be NULL.
uint8_t L2CA_GetIdentifiers(uint16_t lcid, uint16_t *rcid, uint16_t *handle);

/*******************************************************************************
**
** Function         L2CA_SetIdleTimeout
**
** Description      Higher layers call this function to set the idle timeout for
**                  a connection, or for all future connections. The "idle timeout"
**                  is the amount of time that a connection can remain up with
**                  no L2CAP channels on it. A timeout of zero means that the
**                  connection will be torn down immediately when the last channel
**                  is removed. A timeout of 0xFFFF means no timeout. Values are
**                  in seconds.
**
** Returns          TRUE if command succeeded, FALSE if failed
**
*******************************************************************************/
extern uint8_t L2CA_SetIdleTimeout(uint16_t cid, uint16_t timeout,
                                   uint8_t is_global);

/*******************************************************************************
**
** Function         L2CA_SetIdleTimeoutByBdAddr
**
** Description      Higher layers call this function to set the idle timeout for
**                  a connection. The "idle timeout" is the amount of time that
**                  a connection can remain up with no L2CAP channels on it.
**                  A timeout of zero means that the connection will be torn
**                  down immediately when the last channel is removed.
**                  A timeout of 0xFFFF means no timeout. Values are in seconds.
**                  A bd_addr is the remote BD address. If bd_addr = BT_BD_ANY,
**                  then the idle timeouts for all active l2cap links will be
**                  changed.
**
** Returns          TRUE if command succeeded, FALSE if failed
**
** NOTE             This timeout applies to all logical channels active on the
**                  ACL link.
*******************************************************************************/
extern uint8_t L2CA_SetIdleTimeoutByBdAddr(BD_ADDR bd_addr, uint16_t timeout,
        tBT_TRANSPORT transport);

/*******************************************************************************
**
** Function         L2CA_SetTraceLevel
**
** Description      This function sets the trace level for L2CAP. If called with
**                  a value of 0xFF, it simply reads the current trace level.
**
** Returns          the new (current) trace level
**
*******************************************************************************/
extern uint8_t L2CA_SetTraceLevel(uint8_t trace_level);

/*******************************************************************************
**
** Function     L2CA_SetDesireRole
**
** Description  This function sets the desire role for L2CAP.
**              If the new role is L2CAP_ROLE_ALLOW_SWITCH, allow switch on
**              HciCreateConnection.
**              If the new role is L2CAP_ROLE_DISALLOW_SWITCH, do not allow switch on
**              HciCreateConnection.
**
**              If the new role is a valid role (HCI_ROLE_MASTER or HCI_ROLE_SLAVE),
**              the desire role is set to the new value. Otherwise, it is not changed.
**
** Returns      the new (current) role
**
*******************************************************************************/
extern uint8_t L2CA_SetDesireRole(uint8_t new_role);

/*******************************************************************************
**
** Function     L2CA_LocalLoopbackReq
**
** Description  This function sets up a CID for local loopback
**
** Returns      CID of 0 if none.
**
*******************************************************************************/
extern uint16_t L2CA_LocalLoopbackReq(uint16_t psm, uint16_t handle, BD_ADDR p_bd_addr);

/*******************************************************************************
**
** Function     L2CA_FlushChannel
**
** Description  This function flushes none, some or all buffers queued up
**              for xmission for a particular CID. If called with
**              L2CAP_FLUSH_CHANS_GET (0), it simply returns the number
**              of buffers queued for that CID L2CAP_FLUSH_CHANS_ALL (0xffff)
**              flushes all buffers.  All other values specifies the maximum
**              buffers to flush.
**
** Returns      Number of buffers left queued for that CID
**
*******************************************************************************/
extern uint16_t   L2CA_FlushChannel(uint16_t lcid, uint16_t num_to_flush);


/*******************************************************************************
**
** Function         L2CA_SetAclPriority
**
** Description      Sets the transmission priority for an ACL channel.
**                  (For initial implementation only two values are valid.
**                  L2CAP_PRIORITY_NORMAL and L2CAP_PRIORITY_HIGH).
**
** Returns          TRUE if a valid channel, else FALSE
**
*******************************************************************************/
extern uint8_t L2CA_SetAclPriority(BD_ADDR bd_addr, uint8_t priority);

/*******************************************************************************
**
** Function         L2CA_FlowControl
**
** Description      Higher layers call this function to flow control a channel.
**
**                  data_enabled - TRUE data flows, FALSE data is stopped
**
** Returns          TRUE if valid channel, else FALSE
**
*******************************************************************************/
extern uint8_t L2CA_FlowControl(uint16_t cid, uint8_t data_enabled);

/*******************************************************************************
**
** Function         L2CA_SendTestSFrame
**
** Description      Higher layers call this function to send a test S-frame.
**
** Returns          TRUE if valid Channel, else FALSE
**
*******************************************************************************/
extern uint8_t L2CA_SendTestSFrame(uint16_t cid, uint8_t sup_type,
                                   uint8_t back_track);

/*******************************************************************************
**
** Function         L2CA_SetTxPriority
**
** Description      Sets the transmission priority for a channel. (FCR Mode)
**
** Returns          TRUE if a valid channel, else FALSE
**
*******************************************************************************/
extern uint8_t L2CA_SetTxPriority(uint16_t cid, tL2CAP_CHNL_PRIORITY priority);

/*******************************************************************************
**
** Function         L2CA_RegForNoCPEvt
**
** Description      Register callback for Number of Completed Packets event.
**
** Input Param      p_cb - callback for Number of completed packets event
**                  p_bda - BT address of remote device
**
** Returns
**
*******************************************************************************/
extern uint8_t L2CA_RegForNoCPEvt(tL2CA_NOCP_CB *p_cb, BD_ADDR p_bda);

/*******************************************************************************
**
** Function         L2CA_SetChnlDataRate
**
** Description      Sets the tx/rx data rate for a channel.
**
** Returns          TRUE if a valid channel, else FALSE
**
*******************************************************************************/
extern uint8_t L2CA_SetChnlDataRate(uint16_t cid, tL2CAP_CHNL_DATA_RATE tx,
                                    tL2CAP_CHNL_DATA_RATE rx);

typedef void (tL2CA_RESERVE_CMPL_CBACK)(void);

/*******************************************************************************
**
** Function         L2CA_SetFlushTimeout
**
** Description      This function set the automatic flush time out in Baseband
**                  for ACL-U packets.
**                  BdAddr : the remote BD address of ACL link. If it is BT_DB_ANY
**                           then the flush time out will be applied to all ACL link.
**                  FlushTimeout: flush time out in ms
**                           0x0000 : No automatic flush
**                           L2CAP_NO_RETRANSMISSION : No retransmission
**                           0x0002 - 0xFFFE : flush time out, if (flush_tout*8)+3/5)
**                                    <= HCI_MAX_AUTO_FLUSH_TOUT (in 625us slot).
**                                    Otherwise, return FALSE.
**                           L2CAP_NO_AUTOMATIC_FLUSH : No automatic flush
**
** Returns          TRUE if command succeeded, FALSE if failed
**
** NOTE             This flush timeout applies to all logical channels active on the
**                  ACL link.
*******************************************************************************/
extern uint8_t L2CA_SetFlushTimeout(BD_ADDR bd_addr, uint16_t flush_tout);

/*******************************************************************************
**
** Function         L2CA_DataWriteEx
**
** Description      Higher layers call this function to write data with extended
**                  flags.
**                  flags : L2CAP_FLUSHABLE_CH_BASED
**                          L2CAP_FLUSHABLE_PKT
**                          L2CAP_NON_FLUSHABLE_PKT
**
** Returns          L2CAP_DW_SUCCESS, if data accepted, else FALSE
**                  L2CAP_DW_CONGESTED, if data accepted and the channel is congested
**                  L2CAP_DW_FAILED, if error
**
*******************************************************************************/
extern uint8_t L2CA_DataWriteEx(uint16_t cid, BT_HDR *p_data, uint16_t flags);

/*******************************************************************************
**
** Function         L2CA_SetChnlFlushability
**
** Description      Higher layers call this function to set a channels
**                  flushability flags
**
** Returns          TRUE if CID found, else FALSE
**
*******************************************************************************/
extern uint8_t L2CA_SetChnlFlushability(uint16_t cid, uint8_t is_flushable);

/*******************************************************************************
**
**  Function         L2CA_GetPeerFeatures
**
**  Description      Get a peers features and fixed channel map
**
**  Parameters:      BD address of the peer
**                   Pointers to features and channel mask storage area
**
**  Return value:    TRUE if peer is connected
**
*******************************************************************************/
extern uint8_t L2CA_GetPeerFeatures(BD_ADDR bd_addr, uint32_t *p_ext_feat, uint8_t *p_chnl_mask);

/*******************************************************************************
**
**  Function         L2CA_GetBDAddrbyHandle
**
**  Description      Get BD address for the given HCI handle
**
**  Parameters:      HCI handle
**                   BD address of the peer
**
**  Return value:    TRUE if found lcb for the given handle, FALSE otherwise
**
*******************************************************************************/
extern uint8_t L2CA_GetBDAddrbyHandle(uint16_t handle, BD_ADDR bd_addr);

/*******************************************************************************
**
**  Function         L2CA_GetChnlFcrMode
**
**  Description      Get the channel FCR mode
**
**  Parameters:      Local CID
**
**  Return value:    Channel mode
**
*******************************************************************************/
extern uint8_t L2CA_GetChnlFcrMode(uint16_t lcid);


/*******************************************************************************
**
**                      UCD callback prototypes
**
*******************************************************************************/

/* UCD discovery. Parameters are
**      BD Address of remote
**      Data Type
**      Data
*/
#define L2CAP_UCD_INFO_TYPE_RECEPTION   0x01
#define L2CAP_UCD_INFO_TYPE_MTU         0x02

typedef void (tL2CA_UCD_DISCOVER_CB)(BD_ADDR, uint8_t, uint32_t);

/* UCD data received. Parameters are
**      BD Address of remote
**      Pointer to buffer with data
*/
typedef void (tL2CA_UCD_DATA_CB)(BD_ADDR, BT_HDR *);

/* Congestion status callback protype. This callback is optional. If
** an application tries to send data when the transmit queue is full,
** the data will anyways be dropped. The parameter is:
**              remote BD_ADDR
**              TRUE if congested, FALSE if uncongested
*/
typedef void (tL2CA_UCD_CONGESTION_STATUS_CB)(BD_ADDR, uint8_t);

/* UCD registration info (the callback addresses and PSM)
*/
typedef struct {
    tL2CA_UCD_DISCOVER_CB           *pL2CA_UCD_Discover_Cb;
    tL2CA_UCD_DATA_CB               *pL2CA_UCD_Data_Cb;
    tL2CA_UCD_CONGESTION_STATUS_CB  *pL2CA_UCD_Congestion_Status_Cb;
} tL2CAP_UCD_CB_INFO;

/*******************************************************************************
**
**  Function        L2CA_UcdRegister
**
**  Description     Register PSM on UCD.
**
**  Parameters:     tL2CAP_UCD_CB_INFO
**
**  Return value:   TRUE if successs
**
*******************************************************************************/
extern uint8_t L2CA_UcdRegister(uint16_t psm, tL2CAP_UCD_CB_INFO *p_cb_info);

/*******************************************************************************
**
**  Function        L2CA_UcdDeregister
**
**  Description     Deregister PSM on UCD.
**
**  Parameters:     PSM
**
**  Return value:   TRUE if successs
**
*******************************************************************************/
extern uint8_t L2CA_UcdDeregister(uint16_t psm);

/*******************************************************************************
**
**  Function        L2CA_UcdDiscover
**
**  Description     Discover UCD of remote device.
**
**  Parameters:     PSM
**                  BD_ADDR of remote device
**                  info_type : L2CAP_UCD_INFO_TYPE_RECEPTION
**                              L2CAP_UCD_INFO_TYPE_MTU
**
**
**  Return value:   TRUE if successs
**
*******************************************************************************/
extern uint8_t L2CA_UcdDiscover(uint16_t psm, BD_ADDR rem_bda, uint8_t info_type);

/*******************************************************************************
**
**  Function        L2CA_UcdDataWrite
**
**  Description     Send UCD to remote device
**
**  Parameters:     PSM
**                  BD Address of remote
**                  Pointer to buffer of type BT_HDR
**                  flags : L2CAP_FLUSHABLE_CH_BASED
**                          L2CAP_FLUSHABLE_PKT
**                          L2CAP_NON_FLUSHABLE_PKT
**
** Return value     L2CAP_DW_SUCCESS, if data accepted
**                  L2CAP_DW_FAILED,  if error
**
*******************************************************************************/
extern uint16_t L2CA_UcdDataWrite(uint16_t psm, BD_ADDR rem_bda, BT_HDR *p_buf, uint16_t flags);

/*******************************************************************************
**
**  Function        L2CA_UcdSetIdleTimeout
**
**  Description     Set UCD Idle timeout.
**
**  Parameters:     BD Addr
**                  Timeout in second
**
**  Return value:   TRUE if successs
**
*******************************************************************************/
extern uint8_t L2CA_UcdSetIdleTimeout(BD_ADDR rem_bda, uint16_t timeout);

/*******************************************************************************
**
** Function         L2CA_UCDSetTxPriority
**
** Description      Sets the transmission priority for a connectionless channel.
**
** Returns          TRUE if a valid channel, else FALSE
**
*******************************************************************************/
extern uint8_t L2CA_UCDSetTxPriority(BD_ADDR rem_bda, tL2CAP_CHNL_PRIORITY priority);


/*******************************************************************************
**
**                      Fixed Channel callback prototypes
**
*******************************************************************************/

/* Fixed channel connected and disconnected. Parameters are
**      channel
**      BD Address of remote
**      TRUE if channel is connected, FALSE if disconnected
**      Reason for connection failure
**      transport : physical transport, BR/EDR or LE
*/
typedef void (tL2CA_FIXED_CHNL_CB)(uint16_t, BD_ADDR, uint8_t, uint16_t, tBT_TRANSPORT);

/* Signalling data received. Parameters are
**      channel
**      BD Address of remote
**      Pointer to buffer with data
*/
typedef void (tL2CA_FIXED_DATA_CB)(uint16_t, BD_ADDR, BT_HDR *);

/* Congestion status callback protype. This callback is optional. If
** an application tries to send data when the transmit queue is full,
** the data will anyways be dropped. The parameter is:
**      remote BD_ADDR
**      TRUE if congested, FALSE if uncongested
*/
typedef void (tL2CA_FIXED_CONGESTION_STATUS_CB)(BD_ADDR, uint8_t);

/* Fixed channel registration info (the callback addresses and channel config)
*/
typedef struct {
    tL2CA_FIXED_CHNL_CB    *pL2CA_FixedConn_Cb;
    tL2CA_FIXED_DATA_CB    *pL2CA_FixedData_Cb;
    tL2CA_FIXED_CONGESTION_STATUS_CB *pL2CA_FixedCong_Cb;
    tL2CAP_FCR_OPTS         fixed_chnl_opts;

    uint16_t                  default_idle_tout;
    tL2CA_TX_COMPLETE_CB    *pL2CA_FixedTxComplete_Cb; /* fixed channel tx complete callback */
} tL2CAP_FIXED_CHNL_REG;


#if (L2CAP_NUM_FIXED_CHNLS > 0)
/*******************************************************************************
**
**  Function        L2CA_RegisterFixedChannel
**
**  Description     Register a fixed channel.
**
**  Parameters:     Fixed Channel #
**                  Channel Callbacks and config
**
**  Return value:   TRUE if registered OK
**
*******************************************************************************/
extern uint8_t  L2CA_RegisterFixedChannel(uint16_t fixed_cid, tL2CAP_FIXED_CHNL_REG *p_freg);

/*******************************************************************************
**
**  Function        L2CA_ConnectFixedChnl
**
**  Description     Connect an fixed signalling channel to a remote device.
**
**  Parameters:     Fixed CID
**                  BD Address of remote
**
**  Return value:   TRUE if connection started
**
*******************************************************************************/
extern uint8_t L2CA_ConnectFixedChnl(uint16_t fixed_cid, BD_ADDR bd_addr);

/*******************************************************************************
**
**  Function        L2CA_SendFixedChnlData
**
**  Description     Write data on a fixed signalling channel.
**
**  Parameters:     Fixed CID
**                  BD Address of remote
**                  Pointer to buffer of type BT_HDR
**
** Return value     L2CAP_DW_SUCCESS, if data accepted
**                  L2CAP_DW_FAILED,  if error
**
*******************************************************************************/
extern uint16_t L2CA_SendFixedChnlData(uint16_t fixed_cid, BD_ADDR rem_bda, BT_HDR *p_buf);

/*******************************************************************************
**
**  Function        L2CA_RemoveFixedChnl
**
**  Description     Remove a fixed channel to a remote device.
**
**  Parameters:     Fixed CID
**                  BD Address of remote
**                  Idle timeout to use (or 0xFFFF if don't care)
**
**  Return value:   TRUE if channel removed
**
*******************************************************************************/
extern uint8_t L2CA_RemoveFixedChnl(uint16_t fixed_cid, BD_ADDR rem_bda);

/*******************************************************************************
**
** Function         L2CA_SetFixedChannelTout
**
** Description      Higher layers call this function to set the idle timeout for
**                  a fixed channel. The "idle timeout" is the amount of time that
**                  a connection can remain up with no L2CAP channels on it.
**                  A timeout of zero means that the connection will be torn
**                  down immediately when the last channel is removed.
**                  A timeout of 0xFFFF means no timeout. Values are in seconds.
**                  A bd_addr is the remote BD address. If bd_addr = BT_BD_ANY,
**                  then the idle timeouts for all active l2cap links will be
**                  changed.
**
** Returns          TRUE if command succeeded, FALSE if failed
**
*******************************************************************************/
extern uint8_t L2CA_SetFixedChannelTout(BD_ADDR rem_bda, uint16_t fixed_cid, uint16_t idle_tout);

#endif /* (L2CAP_NUM_FIXED_CHNLS > 0) */

/*******************************************************************************
**
** Function     L2CA_GetCurrentConfig
**
** Description  This function returns configurations of L2CAP channel
**              pp_our_cfg : pointer of our saved configuration options
**              p_our_cfg_bits : valid config in bitmap
**              pp_peer_cfg: pointer of peer's saved configuration options
**              p_peer_cfg_bits : valid config in bitmap
**
** Returns      TRUE if successful
**
*******************************************************************************/
extern uint8_t L2CA_GetCurrentConfig(uint16_t lcid,
                                     tL2CAP_CFG_INFO **pp_our_cfg,  tL2CAP_CH_CFG_BITS *p_our_cfg_bits,
                                     tL2CAP_CFG_INFO **pp_peer_cfg, tL2CAP_CH_CFG_BITS *p_peer_cfg_bits);

/*******************************************************************************
**
** Function     L2CA_GetConnectionConfig
**
** Description  This function polulates the mtu, remote cid & lm_handle for
**              a given local L2CAP channel
**
** Returns      TRUE if successful
**
*******************************************************************************/
extern uint8_t L2CA_GetConnectionConfig(uint16_t lcid, uint16_t *mtu, uint16_t *rcid,
                                        uint16_t *handle);

#if (BLE_INCLUDED == TRUE)
/*******************************************************************************
**
**  Function        L2CA_CancelBleConnectReq
**
**  Description     Cancel a pending connection attempt to a BLE device.
**
**  Parameters:     BD Address of remote
**
**  Return value:   TRUE if connection was cancelled
**
*******************************************************************************/
extern uint8_t L2CA_CancelBleConnectReq(BD_ADDR rem_bda);

/*******************************************************************************
**
**  Function        L2CA_UpdateBleConnParams
**
**  Description     Update BLE connection parameters.
**
**  Parameters:     BD Address of remote
**
**  Return value:   TRUE if update started
**
*******************************************************************************/
extern uint8_t L2CA_UpdateBleConnParams(BD_ADDR rem_bdRa, uint16_t min_int,
                                        uint16_t max_int, uint16_t latency, uint16_t timeout);

/*******************************************************************************
**
**  Function        L2CA_EnableUpdateBleConnParams
**
**  Description     Update BLE connection parameters.
**
**  Parameters:     BD Address of remote
**                  enable flag
**
**  Return value:   TRUE if update started
**
*******************************************************************************/
extern uint8_t L2CA_EnableUpdateBleConnParams(BD_ADDR rem_bda, uint8_t enable);

/*******************************************************************************
**
** Function         L2CA_GetBleConnRole
**
** Description      This function returns the connection role.
**
** Returns          link role.
**
*******************************************************************************/
extern uint8_t L2CA_GetBleConnRole(BD_ADDR bd_addr);

/*******************************************************************************
**
** Function         L2CA_GetDisconnectReason
**
** Description      This function returns the disconnect reason code.
**
**  Parameters:     BD Address of remote
**                         Physical transport for the L2CAP connection (BR/EDR or LE)
**
** Returns          disconnect reason
**
*******************************************************************************/
extern uint16_t L2CA_GetDisconnectReason(BD_ADDR remote_bda, tBT_TRANSPORT transport);

#endif /* (BLE_INCLUDED == TRUE) */

#ifdef __cplusplus
}
#endif

#endif  /* L2C_API_H */
