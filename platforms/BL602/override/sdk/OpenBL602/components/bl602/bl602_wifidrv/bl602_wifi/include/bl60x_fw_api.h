/*
 * Copyright (c) 2016-2022 Bouffalolab.
 *
 * This file is part of
 *     *** Bouffalolab Software Dev Kit ***
 *      (see www.bouffalolab.com).
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of Bouffalo Lab nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __BL60x_FW_API_H__
#define __BL60x_FW_API_H__

//#include <bl_ro_params_mgt.h>
#include <stdint.h>
#include "bl_os_private.h"

/// Tasks types.
typedef enum wifi_fw_task_id
{
    TASK_NONE = (uint8_t) -1,

    /// MAC Management task.
    TASK_MM = 0,
    /// SCAN task
    TASK_SCAN,
    /// SCAN task
    TASK_SCANU,
    /// SCAN task
    TASK_ME,
    /// SM task
    TASK_SM,
    /// APM task
    TASK_APM,
    /// BAM task
    TASK_BAM,
    /// RXU task
    TASK_RXU,

    TASK_CFG,

    // This is used to define the last task that is running on the EMB processor
    TASK_LAST_EMB = TASK_CFG,

    // nX API task
    TASK_API,
    TASK_MAX,
} ke_task_id_t;

/// For MAC HW States.
typedef enum hw_state_tag
{
    /// MAC HW IDLE State.
    HW_IDLE = 0,
    /// MAC HW RESERVED State.
    HW_RESERVED,
    /// MAC HW DOZE State.
    HW_DOZE,
    /// MAC HW ACTIVE State.
    HW_ACTIVE
} hw_state_tag_t;

/// Possible States of the MM STA Task.
typedef enum mm_state_tag
{
    /// MAC IDLE State.
    MM_IDLE,
    /// MAC ACTIVE State.
    MM_ACTIVE,
    /// MAC is going to IDLE
    MM_GOING_TO_IDLE,
    /// IDLE state internally controlled
    MM_HOST_BYPASSED,
    /// MAC Max Number of states.
    MM_STATE_MAX
} mm_state_tag_t;

/**
 ****************************************************************************************
 * @brief Builds the first message ID of a task
 * @param[in] task Task identifier
 * @return The identifier of the first message of the task
 ****************************************************************************************
 */
#define KE_FIRST_MSG(task) (((task) << 10))

/// Message Identifier. The number of messages is limited to 0xFFFF.
/// The message ID is divided in two parts:
/// bits[15~8]: task index (no more than 255 tasks support)
/// bits[7~0]: message index(no more than 255 messages per task)
typedef enum wifi_fw_event_id
{
    /// RESET Request.
    MM_RESET_REQ = KE_FIRST_MSG(TASK_MM),
    /// RESET Confirmation.
    MM_RESET_CFM,
    /// START Request.
    MM_START_REQ,
    /// START Confirmation.
    MM_START_CFM,
    /// Read Version Request.
    MM_VERSION_REQ,
    /// Read Version Confirmation.
    MM_VERSION_CFM,
    /// ADD INTERFACE Request.
    MM_ADD_IF_REQ,
    /// ADD INTERFACE Confirmation.
    MM_ADD_IF_CFM,
    /// REMOVE INTERFACE Request.
    MM_REMOVE_IF_REQ,
    /// REMOVE INTERFACE Confirmation.
    MM_REMOVE_IF_CFM,
    /// STA ADD Request.
    MM_STA_ADD_REQ,
    /// STA ADD Confirm.
    MM_STA_ADD_CFM,
    /// STA DEL Request.
    MM_STA_DEL_REQ,
    /// STA DEL Confirm.
    MM_STA_DEL_CFM,
    /// CHANNEL CONFIGURATION Request.
    MM_SET_CHANNEL_REQ,
    /// CHANNEL CONFIGURATION Confirmation.
    MM_SET_CHANNEL_CFM,
    /// BEACON INTERVAL CONFIGURATION Request.
    MM_SET_BEACON_INT_REQ,
    /// BEACON INTERVAL CONFIGURATION Confirmation.
    MM_SET_BEACON_INT_CFM,
    /// BASIC RATES CONFIGURATION Request.
    MM_SET_BASIC_RATES_REQ,
    /// BASIC RATES CONFIGURATION Confirmation.
    MM_SET_BASIC_RATES_CFM,
    /// BSSID CONFIGURATION Request.
    MM_SET_BSSID_REQ,
    /// BSSID CONFIGURATION Confirmation.
    MM_SET_BSSID_CFM,
    /// EDCA PARAMETERS CONFIGURATION Request.
    MM_SET_EDCA_REQ,
    /// EDCA PARAMETERS CONFIGURATION Confirmation.
    MM_SET_EDCA_CFM,
    /// Request setting the VIF active state (i.e associated or AP started)
    MM_SET_VIF_STATE_REQ,
    /// Confirmation of the @ref MM_SET_VIF_STATE_REQ message.
    MM_SET_VIF_STATE_CFM,
    /// IDLE mode change Request.
    MM_SET_IDLE_REQ,
    /// IDLE mode change Confirm.
    MM_SET_IDLE_CFM,
    /// Indication of the primary TBTT to the upper MAC. Upon the reception of this
    /// message the upper MAC has to push the beacon(s) to the beacon transmission queue.
    MM_PRIMARY_TBTT_IND,
    /// Indication of the secondary TBTT to the upper MAC. Upon the reception of this
    /// message the upper MAC has to push the beacon(s) to the beacon transmission queue.
    MM_SECONDARY_TBTT_IND,
    /// Request to the LMAC to trigger the embedded logic analyzer and forward the debug
    /// dump.
    MM_DENOISE_REQ,
    /// Set Power Save mode
    MM_SET_PS_MODE_REQ,
    /// Set Power Save mode confirmation
    MM_SET_PS_MODE_CFM,
    /// Request to add a channel context
    MM_CHAN_CTXT_ADD_REQ,
    /// Confirmation of the channel context addition
    MM_CHAN_CTXT_ADD_CFM,
    /// Request to delete a channel context
    MM_CHAN_CTXT_DEL_REQ,
    /// Confirmation of the channel context deletion
    MM_CHAN_CTXT_DEL_CFM,
    /// Request to link a channel context to a VIF
    MM_CHAN_CTXT_LINK_REQ,
    /// Confirmation of the channel context link
    MM_CHAN_CTXT_LINK_CFM,
    /// Request to unlink a channel context from a VIF
    MM_CHAN_CTXT_UNLINK_REQ,
    /// Confirmation of the channel context unlink
    MM_CHAN_CTXT_UNLINK_CFM,
    /// Request to update a channel context
    MM_CHAN_CTXT_UPDATE_REQ,
    /// Confirmation of the channel context update
    MM_CHAN_CTXT_UPDATE_CFM,
    /// Request to schedule a channel context
    MM_CHAN_CTXT_SCHED_REQ,
    /// Confirmation of the channel context scheduling
    MM_CHAN_CTXT_SCHED_CFM,
    /// Request to change the beacon template in LMAC
    MM_BCN_CHANGE_REQ,
    /// Confirmation of the beacon change
    MM_BCN_CHANGE_CFM,
    /// Request to update the TIM in the beacon (i.e to indicate traffic bufferized at AP)
    MM_TIM_UPDATE_REQ,
    /// Confirmation of the TIM update
    MM_TIM_UPDATE_CFM,
    /// Connection loss indication
    MM_CONNECTION_LOSS_IND,
    /// Channel context switch indication to the upper layers
    MM_CHANNEL_SWITCH_IND,
    /// Channel context pre-switch indication to the upper layers
    MM_CHANNEL_PRE_SWITCH_IND,
    /// Request to remain on channel or cancel remain on channel
    MM_REMAIN_ON_CHANNEL_REQ,
    /// Confirmation of the (cancel) remain on channel request
    MM_REMAIN_ON_CHANNEL_CFM,
    /// Remain on channel expired indication
    MM_REMAIN_ON_CHANNEL_EXP_IND,
    /// Indication of a PS state change of a peer device
    MM_PS_CHANGE_IND,
    /// Indication that some buffered traffic should be sent to the peer device
    MM_TRAFFIC_REQ_IND,
    /// Request to modify the STA Power-save mode options
    MM_SET_PS_OPTIONS_REQ,
    /// Confirmation of the PS options setting
    MM_SET_PS_OPTIONS_CFM,
    /// Indication of PS state change for a P2P VIF
    MM_P2P_VIF_PS_CHANGE_IND,
    /// Message containing channel information
    MM_CHANNEL_SURVEY_IND,
    /// Message containing Beamformer information
    MM_BFMER_ENABLE_REQ,
    /// Request to Start/Stop NOA - GO Only
    MM_SET_P2P_NOA_REQ,
    /// Request to Start/Stop Opportunistic PS - GO Only
    MM_SET_P2P_OPPPS_REQ,
    /// Start/Stop NOA Confirmation
    MM_SET_P2P_NOA_CFM,
    /// Start/Stop Opportunistic PS Confirmation
    MM_SET_P2P_OPPPS_CFM,
    /// P2P NoA Update Indication - GO Only
    MM_P2P_NOA_UPD_IND,
    /// Indication that RSSI is below or above the threshold
    MM_RSSI_STATUS_IND,
    /// Request to update the group information of a station
    MM_MU_GROUP_UPDATE_REQ,
    /// Confirmation of the @ref MM_MU_GROUP_UPDATE_REQ message
    MM_MU_GROUP_UPDATE_CFM,
    //Enable monitor mode
    MM_MONITOR_REQ,
    MM_MONITOR_CFM,
    //Channel set under monitor mode
    MM_MONITOR_CHANNEL_REQ,
    MM_MONITOR_CHANNEL_CFM,

    /*
     * Section of internal MM messages. No MM API messages should be defined below this point
     */
    /// Internal request to force the HW going to IDLE
    MM_FORCE_IDLE_REQ,
    /// Message indicating that the switch to the scan channel is done
    MM_SCAN_CHANNEL_START_IND,
    /// Message indicating that the scan can end early
    MM_SCAN_CHANNEL_END_EARLY,
    /// Message indicating that the scan on the channel is finished
    MM_SCAN_CHANNEL_END_IND,

    /// MAX number of messages
    MM_MAX,

    /// Request to start the AP.
    CFG_START_REQ = KE_FIRST_MSG(TASK_CFG),
    CFG_START_CFM,
    CFG_MAX,


    /// Scanning start Request.
    SCAN_START_REQ = KE_FIRST_MSG(TASK_SCAN),
    /// Scanning start Confirmation.
    SCAN_START_CFM,
    /// End of scanning indication.
    SCAN_DONE_IND,
    /// Cancel scan request
    SCAN_CANCEL_REQ,
    /// Cancel scan confirmation
    SCAN_CANCEL_CFM,

    /// Abort scan request
    SCAN_ABORT_REQ,
    SCAN_ABORT_CFM,

    /*
     * Section of internal SCAN messages. No SCAN API messages should be defined below this point
     */
    SCAN_TIMER,

    /// MAX number of messages
    SCAN_MAX,

    /// Request to start the AP.
    APM_START_REQ = KE_FIRST_MSG(TASK_APM),
    /// Confirmation of the AP start.
    APM_START_CFM,
    /// Request to stop the AP.
    APM_STOP_REQ,
    /// Confirmation of the AP stop.
    APM_STOP_CFM,
    /// Nofity host that a station has joined the network
    APM_STA_ADD_IND,
    /// Nofity host that a station has left the network
    APM_STA_DEL_IND,
    /// Check sta connect timeout
    APM_STA_CONNECT_TIMEOUT_IND,
    /// Request to delete STA
    APM_STA_DEL_REQ,
    /// Confirmation of delete STA
    APM_STA_DEL_CFM,
    /// CONF MAX STA Request
    APM_CONF_MAX_STA_REQ,
    /// CONF MAX STA Confirm
    APM_CONF_MAX_STA_CFM,
    /// MAX number of messages
    APM_MAX,

    /// BAM addition response timeout
    BAM_ADD_BA_RSP_TIMEOUT_IND  = KE_FIRST_MSG(TASK_BAM),
    /// BAM Inactivity timeout
    BAM_INACTIVITY_TIMEOUT_IND,

    /// Configuration request from host.
    ME_CONFIG_REQ = KE_FIRST_MSG(TASK_ME),
    /// Configuration confirmation.
    ME_CONFIG_CFM,
    /// Configuration request from host.
    ME_CHAN_CONFIG_REQ,
    /// Configuration confirmation.
    ME_CHAN_CONFIG_CFM,
    /// TKIP MIC failure indication.
    ME_TKIP_MIC_FAILURE_IND,
    /// Add a station to the FW (AP mode)
    ME_STA_ADD_REQ,
    /// Confirmation of the STA addition
    ME_STA_ADD_CFM,
    /// Delete a station from the FW (AP mode)
    ME_STA_DEL_REQ,
    /// Confirmation of the STA deletion
    ME_STA_DEL_CFM,
    /// Indication of a TX RA/TID queue credit update
    ME_TX_CREDITS_UPDATE_IND,
    /// Request indicating to the FW that there is traffic buffered on host
    ME_TRAFFIC_IND_REQ,
    /// Confirmation that the @ref ME_TRAFFIC_IND_REQ has been executed
    ME_TRAFFIC_IND_CFM,
    /// Request RC fixed rate
    ME_RC_SET_RATE_REQ,

    /*
     * Section of internal ME messages. No ME API messages should be defined below this point
     */
    /// Internal request to indicate that a VIF needs to get the HW going to ACTIVE or IDLE
    ME_SET_ACTIVE_REQ,
    /// Confirmation that the switch to ACTIVE or IDLE has been executed
    ME_SET_ACTIVE_CFM,
    /// Internal request to indicate that a VIF desires to de-activate/activate the Power-Save mode
    ME_SET_PS_DISABLE_REQ,
    /// Confirmation that the PS state de-activate/activate has been executed
    ME_SET_PS_DISABLE_CFM,
    /// MAX number of messages
    ME_MAX,

    /// Management frame reception indication
    RXU_MGT_IND = KE_FIRST_MSG(TASK_RXU),
    /// NULL frame reception indication
    RXU_NULL_DATA,

    /// Scan request from host.
    SCANU_START_REQ = KE_FIRST_MSG(TASK_SCANU),
    /// Scanning start Confirmation.
    SCANU_START_CFM,
    /// Join request
    SCANU_JOIN_REQ,
    /// Join confirmation.
    SCANU_JOIN_CFM,
    /// Scan result indication.
    SCANU_RESULT_IND,
    /// Scan RAW Data Send from host
    SCANU_RAW_SEND_REQ,
    /// Scan result indication.
    SCANU_RAW_SEND_CFM,

    /// MAX number of messages
    SCANU_MAX,

    /// Request to connect to an AP
    SM_CONNECT_REQ = KE_FIRST_MSG(TASK_SM),
    /// Confirmation of connection
    SM_CONNECT_CFM,
    /// Indicates that the SM associated to the AP
    SM_CONNECT_IND,
    /// Request to disconnect
    SM_DISCONNECT_REQ,
    /// Confirmation of disconnection
    SM_DISCONNECT_CFM,
    /// Indicates that the SM disassociated the AP
    SM_DISCONNECT_IND,
    /// Timeout message for procedures requiring a response from peer
    SM_RSP_TIMEOUT_IND,
    /// Request to cancel connect when connecting
    SM_CONNECT_ABORT_REQ,
    /// Confirmation of connect abort
    SM_CONNECT_ABORT_CFM,
    /// Timeout message for requiring a SA Query Response from AP
    SM_SA_QUERY_TIMEOUT_IND,
    /// MAX number of messages
    SM_MAX,
} ke_msg_id_t;

/*--------------------------------------------------------------------*/
/* Status Codes - these codes are used in bouffalolab fw actions      */
/* when an error action is taking place the status code can indicate  */
/* what the status code.                                              */
/* It is defined by bouffaloalb                                       */
/*--------------------------------------------------------------------*/
#define WLAN_FW_SUCCESSFUL                                        0 
#define WLAN_FW_TX_AUTH_FRAME_ALLOCATE_FAIILURE                   1 
#define WLAN_FW_AUTHENTICATION_FAIILURE                           2
#define WLAN_FW_AUTH_ALGO_FAIILURE                                3
#define WLAN_FW_TX_ASSOC_FRAME_ALLOCATE_FAIILURE                  4 
#define WLAN_FW_ASSOCIATE_FAIILURE                                5
#define WLAN_FW_DEAUTH_BY_AP_WHEN_NOT_CONNECTION                  6
#define WLAN_FW_DEAUTH_BY_AP_WHEN_CONNECTION                      7
#define WLAN_FW_4WAY_HANDSHAKE_ERROR_PSK_TIMEOUT_FAILURE          8
#define WLAN_FW_4WAY_HANDSHAKE_TX_DEAUTH_FRAME_TRANSMIT_FAILURE   9
#define WLAN_FW_4WAY_HANDSHAKE_TX_DEAUTH_FRAME_ALLOCATE_FAIILURE 10
#define WLAN_FW_AUTH_OR_ASSOC_RESPONSE_TIMEOUT_FAILURE           11
#define WLAN_FW_SCAN_NO_BSSID_AND_CHANNEL                        12
#define WLAN_FW_CREATE_CHANNEL_CTX_FAILURE_WHEN_JOIN_NETWORK     13
#define WLAN_FW_JOIN_NETWORK_FAILURE                             14
#define WLAN_FW_ADD_STA_FAILURE                                  15
#define WLAN_FW_BEACON_LOSS                                      16
#define WLAN_FW_JOIN_NETWORK_SECURITY_NOMATCH                    17
#define WLAN_FW_JOIN_NETWORK_WEPLEN_ERROR                        18
#define WLAN_FW_DISCONNECT_BY_USER_WITH_DEAUTH                   19
#define WLAN_FW_DISCONNECT_BY_USER_NO_DEAUTH                     20
#define WLAN_FW_DISCONNECT_BY_FW_PS_TX_NULLFRAME_FAILURE         21
#define WLAN_FW_TRAFFIC_LOSS                                     22
#define WLAN_FW_CONNECT_ABORT_BY_USER_WITH_DEAUTH                23
#define WLAN_FW_CONNECT_ABORT_BY_USER_NO_DEAUTH                  24
#define WLAN_FW_CONNECT_ABORT_WHEN_JOINING_NETWORK               25
#define WLAN_FW_CONNECT_ABORT_WHEN_SCANNING                      26 


/*--------------------------------------------------------------------*/
/* AP Mode Status Codes - these codes are used in bouffalolab fw actions      */
/* when an error action is taking place the status code can indicate  */
/* what the status code.                                              */
/* It is defined by bouffaloalb                                       */
/*--------------------------------------------------------------------*/
#define WLAN_FW_APM_SUCCESSFUL                                        0 
#define WLAN_FW_APM_DELETESTA_BY_USER                                 1
#define WLAN_FW_APM_DEATUH_BY_STA                                     2
#define WLAN_FW_APM_DISASSOCIATE_BY_STA                               3
#define WLAN_FW_APM_DELETECONNECTION_TIMEOUT                          4
#define WLAN_FW_APM_DELETESTA_FOR_NEW_CONNECTION                      5
#define WLAN_FW_APM_DEAUTH_BY_AUTHENTICATOR                           6

void bl60x_fw_xtal_capcode_set(uint8_t cap_in, uint8_t cap_out, uint8_t enable, uint8_t cap_in_max, uint8_t cap_out_max);
void bl60x_fw_xtal_capcode_update(uint8_t cap_in, uint8_t cap_out);
void bl60x_fw_xtal_capcode_restore(void);
void bl60x_fw_xtal_capcode_autofit(void);
void bl60x_fw_xtal_capcode_get(uint8_t *cap_in, uint8_t *cap_out);
//void bl60x_fw_rf_tx_power_table_set(bl_tx_pwr_tbl_t* pwr_table);
int bl60x_fw_password_hash(char *password, char *ssid, int ssidlength, char *output);
void bl60x_fw_rf_table_set(uint32_t* channel_div_table_in, uint16_t* channel_cnt_table_in,
                                                uint16_t lo_fcal_div);

void bl60x_fw_dump_data(void);
void bl60x_fw_dump_statistic(int forced);

int bl60x_check_mac_status(int *is_ok);

enum {
    /// Background
    API_AC_BK = 0,
    /// Best-effort
    API_AC_BE,
    /// Video
    API_AC_VI,
    /// Voice
    API_AC_VO,
    /// Number of access categories
    API_AC_MAX
};

int bl60x_edca_get(int ac, uint8_t *aifs, uint8_t *cwmin, uint8_t *cwmax, uint16_t *txop);

/*Wi-Fi Firmware Entry*/
void wifi_main(void *param);

void bl_tpc_update_power_table(int8_t power_table_config[38]);
void bl_tpc_update_power_table_rate(int8_t power_table[24]);
void bl_tpc_update_power_table_channel_offset(int8_t power_table[14]);
void bl_tpc_update_power_rate_11b(int8_t power_rate_table[4]);
void bl_tpc_update_power_rate_11g(int8_t power_rate_table[8]);
void bl_tpc_update_power_rate_11n(int8_t power_rate_table[8]);
void bl_tpc_power_table_get(int8_t power_table_config[38]);

void phy_cli_register(void);



enum task_mm_cfg {
    TASK_MM_CFG_KEEP_ALIVE_STATUS_ENABLED,
    TASK_MM_CFG_KEEP_ALIVE_TIME_LAST_RECEIVED,
    TASK_MM_CFG_KEEP_ALIVE_PACKET_COUNTER,
};

enum task_sm_cfg {
    TASK_SM_CFG_AUTH_ASSOC_RETRY_LIMIT,
};

enum task_scan_cfg {
    TASK_SCAN_CFG_DURATION_SCAN_PASSIVE,
    TASK_SCAN_CFG_DURATION_SCAN_ACTIVE,
    TASK_SCAN_CFG_DURATION_SCAN_JOIN_ACTIVE,
};

typedef enum{
    /**version part**/
    SM_CONNECTION_DATA_TLV_ID_VERSION,
    /**Status part**/
    SM_CONNECTION_DATA_TLV_ID_STATUS_CODE,
    SM_CONNECTION_DATA_TLV_ID_DHCPSTATUS,
    /**frame part**/
    SM_CONNECTION_DATA_TLV_ID_AUTH_1,
    SM_CONNECTION_DATA_TLV_ID_AUTH_2,
    SM_CONNECTION_DATA_TLV_ID_AUTH_3,
    SM_CONNECTION_DATA_TLV_ID_AUTH_4,
    SM_CONNECTION_DATA_TLV_ID_ASSOC_REQ,
    SM_CONNECTION_DATA_TLV_ID_ASSOC_RSP,
    SM_CONNECTION_DATA_TLV_ID_4WAY_1,
    SM_CONNECTION_DATA_TLV_ID_4WAY_2,
    SM_CONNECTION_DATA_TLV_ID_4WAY_3,
    SM_CONNECTION_DATA_TLV_ID_4WAY_4,
    SM_CONNECTION_DATA_TLV_ID_2WAY_1,
    SM_CONNECTION_DATA_TLV_ID_2WAY_2,
    SM_CONNECTION_DATA_TLV_ID_DEAUTH,
    /**striped frame part**/
    SM_CONNECTION_DATA_TLV_ID_STRIPED_AUTH_1,
    SM_CONNECTION_DATA_TLV_ID_STRIPED_AUTH_2,
    SM_CONNECTION_DATA_TLV_ID_STRIPED_AUTH_3,
    SM_CONNECTION_DATA_TLV_ID_STRIPED_AUTH_4,
    SM_CONNECTION_DATA_TLV_ID_STRIPED_AUTH_UNVALID,
    SM_CONNECTION_DATA_TLV_ID_STRIPED_ASSOC_REQ,
    SM_CONNECTION_DATA_TLV_ID_STRIPED_ASSOC_RSP,
    SM_CONNECTION_DATA_TLV_ID_STRIPED_DEAUTH_FROM_REMOTE,
    SM_CONNECTION_DATA_TLV_ID_STRIPED_DEASSOC_FROM_REMOTE,
    SM_CONNECTION_DATA_TLV_ID_RESERVED,
} sm_connection_data_tlv_id_t;

/* structure of a list element header */
struct sm_tlv_list_hdr
{
    struct sm_tlv_list_hdr *next;
};

/* structure of a list */
struct sm_tlv_list
{
    struct sm_tlv_list_hdr *first;
    struct sm_tlv_list_hdr *last;
};

/*
 * TLV ID数据以链表的形式存储在struct sm_connect_tlv_desc中，
 * callback需要遍历这个链表来获取所有的数据
 */
struct sm_connect_tlv_desc {
    struct sm_tlv_list_hdr list_hdr;
    sm_connection_data_tlv_id_t id;
    uint16_t len;
    uint8_t data[0];
};

#ifdef TD_DIAGNOSIS_STA
typedef struct wifi_diagnosis_info
{
    uint32_t beacon_recv;
    uint32_t beacon_loss;
    uint32_t beacon_total;
    uint64_t unicast_recv;
    uint64_t unicast_send;
    uint64_t multicast_recv;
    uint64_t multicast_send;
}wifi_diagnosis_info_t;

wifi_diagnosis_info_t *bl_diagnosis_get(void);
int wifi_td_diagnosis_init(void);
int wifi_td_diagnosis_deinit(void);
#endif

#endif /*__BL60x_FW_API_H__*/
