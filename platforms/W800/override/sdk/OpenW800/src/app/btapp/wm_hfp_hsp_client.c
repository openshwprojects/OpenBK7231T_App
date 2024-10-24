/*****************************************************************************
**
**  Name:           wm_hfp_hsp_client.c
**
**  Description:    This file contains the sample functions for bluetooth hand-free/hand-set profile client application
**
*****************************************************************************/

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"

#if (WM_BTA_HFP_HSP_INCLUDED == CFG_ON)

#include "wm_bt_hf_client.h"
#include "wm_hfp_hsp_client.h"
#include "wm_bt_util.h"
#if (WM_AUDIO_BOARD_INCLUDED == CFG_ON)
#include "audio.h"
#endif


const char *dump_hf_client_call_state(tls_bthf_client_call_state_t event);
const char *dump_hf_client_call(tls_bthf_client_call_t event);
const char *dump_hf_client_callsetup(tls_bthf_client_callsetup_t event);
const char *dump_hf_client_callheld(tls_bthf_client_callheld_t event);
const char *dump_hf_client_resp_and_hold(tls_bthf_client_resp_and_hold_t event);
const char *dump_hf_client_call_direction(uint16_t event);
const char *dump_hf_client_conn_state(tls_bthf_client_connection_state_t event);
const char *dump_hf_client_audio_state(tls_bthf_client_audio_state_t event);

#if (WM_AUDIO_BOARD_INCLUDED == CFG_ON)
static uint32_t Stereo2Mono(void *audio_buf, uint32_t len, int LR)
{
    if(!audio_buf || !len || len % 4) {
        printf("%s arg err\n", __func__);
        return 0;
    }

    int16_t *buf = audio_buf;
    uint32_t i = 0;
    LR = LR ? 1 : 0;

    for(i = 0; i < len / 4; i++) {
        buf[i] = buf[i * 2 + LR];
    }

    return len / 2;
}
static void dump_sco_data(uint8_t *p_data, uint16_t length)
{
    int i = 0;

    for(i = 0; i < 32; i++) {
        printf("%02x ", p_data[i]);
    }

    printf("\r\n");
}
/*SCO data to application*/
int btif_co_sco_data_incoming(uint8_t type, uint8_t *p_data, uint16_t length)
{
    dump_sco_data(p_data, length);
#if (WM_AUDIO_BOARD_INCLUDED == CFG_ON)
    tls_player_output(p_data, length);
#endif
    return length;
}

/*SCO data sent over HCI*/
int btif_co_sco_data_outgoing(uint8_t type, uint8_t *p_data, uint16_t length)
{
    memset(p_data, 0,  length);
    return length;
}

#else
/*SCO data to application*/
int btif_co_sco_data_incoming(uint8_t type, uint8_t *p_data, uint16_t length)
{
    return length;
}

/*SCO data sent over HCI*/
int btif_co_sco_data_outgoing(uint8_t type, uint8_t *p_data, uint16_t length)
{
    memset(p_data, 0,  length);
    return length;
}
#endif

void hfp_client_connection_state_cb(tls_bthf_client_connection_state_t state,
                                    unsigned int peer_feat,
                                    unsigned int chld_feat,
                                    tls_bt_addr_t *bd_addr)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_connection_state_cb: state=%s\r\n",
                            dump_hf_client_conn_state(state));
}

void hfp_client_audio_state_cb(tls_bthf_client_audio_state_t state,
                               tls_bt_addr_t *bd_addr)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_audio_state_cb: state=%s\r\n",
                            dump_hf_client_audio_state(state));

    switch(state) {
        case WM_BTHF_CLIENT_AUDIO_STATE_DISCONNECTED:
#if (WM_AUDIO_BOARD_INCLUDED == CFG_ON)
            tls_player_stop();
#endif
            break;

        case WM_BTHF_CLIENT_AUDIO_STATE_CONNECTING:
            break;

        case WM_BTHF_CLIENT_AUDIO_STATE_CONNECTED:

        //break;
        case WM_BTHF_CLIENT_AUDIO_STATE_CONNECTED_MSBC:
#if (WM_AUDIO_BOARD_INCLUDED == CFG_ON)
            tls_player_config(8000, 16, 2);
            tls_player_play();
#endif
            break;
    }
}

void hfp_client_vr_cmd_cb(tls_bthf_client_vr_state_t state)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_vr_cmd_cb: state=%d\r\n", state);
}

/** Callback for network state change
 */
void hfp_client_network_state_cb(tls_bthf_client_network_state_t state)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_network_state_cb: state=%d\r\n", state);
}

/** Callback for network roaming status change
 */
void hfp_client_network_roaming_cb(tls_bthf_client_service_type_t type)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_network_roaming_cb, type=%d\r\n", type);
}

/** Callback for signal strength indication
 */
void hfp_client_network_signal_cb(int signal_strength)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_network_signal_cb(%d)\r\n", signal_strength);
}

/** Callback for battery level indication
 */
void hfp_client_battery_level_cb(int battery_level)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_battery_level_cb, battery_level=%d\r\n", battery_level);
}

/** Callback for current operator name
 */
void hfp_client_current_operator_cb(const char *name)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_current_operator_cb, name=%s\r\n", name);
}

/** Callback for call indicator
 */
void hfp_client_call_cb(tls_bthf_client_call_t call)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_call_cb,call=%s\r\n", dump_hf_client_call(call));
}

/** Callback for callsetup indicator
 */
void hfp_client_callsetup_cb(tls_bthf_client_callsetup_t callsetup)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_callsetup_cb, callsetup=%s\r\n",
                            dump_hf_client_callsetup(callsetup));
    ;
}

/** Callback for callheld indicator
 */
void hfp_client_callheld_cb(tls_bthf_client_callheld_t callheld)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_callheld_cb, callheld=%s\r\n",
                            dump_hf_client_callheld(callheld));
}

/** Callback for response and hold
 */
void hfp_client_resp_and_hold_cb(tls_bthf_client_resp_and_hold_t resp_and_hold)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_resp_and_hold_cb, resp_and_hold=%s\r\n",
                            dump_hf_client_resp_and_hold(resp_and_hold));
}

/** Callback for Calling Line Identification notification
 *  Will be called only when there is an incoming call and number is provided.
 */
void hfp_client_clip_cb(const char *number)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_clip_cb, number=%s\r\n", number);
}

/**
 * Callback for Call Waiting notification
 */
void hfp_client_call_waiting_cb(const char *number)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_call_waiting_cb, number=%s\r\n", number);
}

/**
 *  Callback for listing current calls. Can be called multiple time.
 *  If number is unknown NULL is passed.
 */
void hfp_client_current_calls_cb(int index, tls_bthf_client_call_direction_t dir,
                                 tls_bthf_client_call_state_t state,
                                 tls_bthf_client_call_mpty_type_t mpty,
                                 const char *number)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_current_calls_cb, bthf_client_call_state_t=%s, number=%s\r\n",
                            dump_hf_client_call_state(state), number);
}

/** Callback for audio volume change
 */
void hfp_client_volume_change_cb(tls_bthf_client_volume_type_t type, int volume)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_volume_change_cb, type=%d, volume=%d\r\n", type, volume);
}

/** Callback for command complete event
 *  cme is valid only for BTHF_CLIENT_CMD_COMPLETE_ERROR_CME type
 */
void hfp_client_cmd_complete_cb(tls_bthf_client_cmd_complete_t type, int cme)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_cmd_complete_cb, type=%d\r\n", type);
}

/** Callback for subscriber information
 */
void hfp_client_subscriber_info_cb(const char *name,
                                   tls_bthf_client_subscriber_service_type_t type)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_subscriber_info_cb, name=%s, type=%d\r\n", name, type);
}

/** Callback for in-band ring tone settings
 */
void hfp_client_in_band_ring_tone_cb(tls_bthf_client_in_band_ring_state_t state)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_in_band_ring_tone_cb, in_band_ring_state=%d\r\n", state);
}

/**
 * Callback for requested number from AG
 */
void hfp_client_last_voice_tag_number_cb(const char *number)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_last_voice_tag_number_cb\r\n");
}

/**
 * Callback for sending ring indication to app
 */
void hfp_client_ring_indication_cb(void)
{
    TLS_BT_APPL_TRACE_DEBUG("hfp_client_ring_indication_cb\r\n");
}





static void wm_bt_hfp_client_callback(tls_bthf_client_evt_t evt, tls_bthf_client_msg_t *msg)
{
    switch(evt) {
        case WM_BTHF_CLIENT_CONNECTION_STATE_EVT:
            hfp_client_connection_state_cb(msg->connection_state_msg.state, msg->connection_state_msg.peer_feat,
                                           msg->connection_state_msg.chld_feat, msg->connection_state_msg.bd_addr);
            break;

        case WM_BTHF_CLIENT_AUDIO_STATE_EVT:
            hfp_client_audio_state_cb(msg->audio_state_msg.state, msg->audio_state_msg.bd_addr);
            break;

        case WM_BTHF_CLIENT_VR_CMD_EVT:
            hfp_client_vr_cmd_cb(msg->vr_cmd_msg.state);
            break;

        case WM_BTHF_CLIENT_NETWORK_STATE_EVT:
            hfp_client_network_state_cb(msg->network_state_msg.state);
            break;

        case WM_BTHF_CLIENT_NETWORK_ROAMING_EVT:
            hfp_client_network_roaming_cb(msg->network_roaming_msg.type);
            break;

        case WM_BTHF_CLIENT_NETWORK_SIGNAL_EVT:
            hfp_client_network_signal_cb(msg->network_signal_msg.signal_strength);
            break;

        case WM_BTHF_CLIENT_BATTERY_LEVEL_EVT:
            hfp_client_battery_level_cb(msg->battery_level_msg.battery_level);
            break;

        case WM_BTHF_CLIENT_CURRENT_OPERATOR_EVT:
            hfp_client_current_operator_cb(msg->current_operator_msg.name);
            break;

        case WM_BTHF_CLIENT_CALL_EVT:
            hfp_client_call_cb(msg->call_msg.call);
            break;

        case WM_BTHF_CLIENT_CALLSETUP_EVT:
            hfp_client_callsetup_cb(msg->callsetup_msg.callsetup);
            break;

        case WM_BTHF_CLIENT_CALLHELD_EVT:
            hfp_client_callheld_cb(msg->callheld_msg.callheld);
            break;

        case WM_BTHF_CLIENT_RESP_AND_HOLD_EVT:
            hfp_client_resp_and_hold_cb(msg->resp_and_hold_msg.resp_and_hold);
            break;

        case WM_BTHF_CLIENT_CLIP_EVT:
            hfp_client_clip_cb(msg->clip_msg.number);
            break;

        case WM_BTHF_CLIENT_CALL_WAITING_EVT:
            hfp_client_call_waiting_cb(msg->call_waiting_msg.number);
            break;

        case WM_BTHF_CLIENT_CURRENT_CALLS_EVT:
            hfp_client_current_calls_cb(msg->current_calls_msg.index, msg->current_calls_msg.dir,
                                        msg->current_calls_msg.state, msg->current_calls_msg.mpty, msg->current_calls_msg.number);
            break;

        case WM_BTHF_CLIENT_VOLUME_CHANGE_EVT:
            hfp_client_volume_change_cb(msg->volume_change_msg.type, msg->volume_change_msg.volume);
            break;

        case WM_BTHF_CLIENT_CMD_COMPLETE_EVT:
            hfp_client_cmd_complete_cb(msg->cmd_complete_msg.type, msg->cmd_complete_msg.cme);
            break;

        case WM_BTHF_CLIENT_SUBSCRIBER_INFO_EVT:
            hfp_client_subscriber_info_cb(msg->subscriber_info_msg.name, msg->subscriber_info_msg.type);
            break;

        case WM_BTHF_CLIENT_IN_BAND_RING_TONE_EVT:
            hfp_client_in_band_ring_tone_cb(msg->in_band_ring_tone_msg.state);
            break;

        case WM_BTHF_CLIENT_LAST_VOICE_TAG_NUMBER_EVT:
            hfp_client_last_voice_tag_number_cb(msg->last_voice_tag_number_msg.number);
            break;

        case WM_BTHF_CLIENT_RING_INDICATION_EVT:
            hfp_client_ring_indication_cb();
            break;

        default:
            TLS_BT_APPL_TRACE_WARNING("Unknown hfp client callback evt:%d\r\n", evt);
            break;
    }
}

tls_bt_status_t tls_bt_enable_hfp_client()
{
    tls_bt_status_t status;
#if (WM_AUDIO_BOARD_INCLUDED == CFG_ON)
    tls_player_init();
#endif
    status = tls_bt_hf_client_init(wm_bt_hfp_client_callback);
    return status;
}

tls_bt_status_t tls_bt_disable_hfp_client()
{
    tls_bt_status_t status;
    status = tls_bt_hf_client_deinit();
#if (WM_AUDIO_BOARD_INCLUDED == CFG_ON)
    tls_player_deinit();
#endif    
    return status;
}

tls_bt_status_t tls_bt_dial_number(const char *number)
{
    TLS_BT_APPL_TRACE_DEBUG("tls_bt_dial_number:%s\r\n", number);
    return tls_bt_hf_client_dial(number);
}

#ifndef CASE_RETURN_STR
#define CASE_RETURN_STR(const) case const: return #const;
#endif

const char *dump_hf_client_call_state(tls_bthf_client_call_state_t event)
{
    switch(event) {
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALL_STATE_ACTIVE)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALL_STATE_HELD)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALL_STATE_DIALING)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALL_STATE_ALERTING)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALL_STATE_INCOMING)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALL_STATE_WAITING)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALL_STATE_HELD_BY_RESP_HOLD)

        default:
            return "UNKNOWN MSG ID(call_state)";
    }
}

const char *dump_hf_client_call(tls_bthf_client_call_t event)
{
    switch(event) {
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALL_NO_CALLS_IN_PROGRESS)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALL_CALLS_IN_PROGRESS)

        default:
            return "UNKNOWN MSG ID(call)";
    }
}

const char *dump_hf_client_callsetup(tls_bthf_client_callsetup_t event)
{
    switch(event) {
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALLSETUP_NONE)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALLSETUP_INCOMING)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALLSETUP_OUTGOING)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALLSETUP_ALERTING)

        default:
            return "UNKNOWN MSG ID(callheld)";
    }
}

const char *dump_hf_client_callheld(tls_bthf_client_callheld_t event)
{
    switch(event) {
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALLHELD_NONE)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALLHELD_HOLD_AND_ACTIVE)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALLHELD_HOLD)

        default:
            return "UNKNOWN MSG ID(callheld)";
    }
}

const char *dump_hf_client_resp_and_hold(tls_bthf_client_resp_and_hold_t event)
{
    switch(event) {
            CASE_RETURN_STR(WM_BTHF_CLIENT_RESP_AND_HOLD_HELD)
            CASE_RETURN_STR(WM_BTRH_CLIENT_RESP_AND_HOLD_ACCEPT)
            CASE_RETURN_STR(WM_BTRH_CLIENT_RESP_AND_HOLD_REJECT)

        default:
            return "UNKNOWN MSG ID(hf_client_resp_and_hold)";
    }
}

const char *dump_hf_client_call_direction(uint16_t event)
{
    switch(event) {
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALL_DIRECTION_OUTGOING)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CALL_DIRECTION_INCOMING)

        default:
            return "UNKNOWN MSG ID(hf_client_call_direction)";
    }
}

const char *dump_hf_client_conn_state(tls_bthf_client_connection_state_t event)
{
    switch(event) {
            CASE_RETURN_STR(WM_BTHF_CLIENT_CONNECTION_STATE_DISCONNECTED)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CONNECTION_STATE_CONNECTING)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CONNECTION_STATE_CONNECTED)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CONNECTION_STATE_SLC_CONNECTED)
            CASE_RETURN_STR(WM_BTHF_CLIENT_CONNECTION_STATE_DISCONNECTING)

        default:
            return "UNKNOWN MSG ID(hf_client_conn_state)";
    }
}
const char *dump_hf_client_audio_state(tls_bthf_client_audio_state_t event)
{
    switch(event) {
            CASE_RETURN_STR(WM_BTHF_CLIENT_AUDIO_STATE_DISCONNECTED)
            CASE_RETURN_STR(WM_BTHF_CLIENT_AUDIO_STATE_CONNECTING)
            CASE_RETURN_STR(WM_BTHF_CLIENT_AUDIO_STATE_CONNECTED)
            CASE_RETURN_STR(WM_BTHF_CLIENT_AUDIO_STATE_CONNECTED_MSBC)

        default:
            return "UNKNOWN MSG ID(hf_client_audio_state)";
    }
}

#endif

