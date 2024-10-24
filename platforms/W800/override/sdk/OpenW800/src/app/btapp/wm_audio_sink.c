/*****************************************************************************
**
**  Name:           wm_audio_sink.c
**
**  Description:    This file contains the sample functions for bluetooth audio sink application
**
*****************************************************************************/

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"
#include "btif_util.h"
#if (WM_BTA_AV_SINK_INCLUDED == CFG_ON)

#include "wm_bt_av.h"
#include "wm_audio_sink.h"
#include "wm_bt_util.h"

#if (WM_AUDIO_BOARD_INCLUDED == CFG_ON)
#include "audio.h"
#endif

/**This function is the pcm output function, type is 0(PCM)*/
#if (WM_AUDIO_BOARD_INCLUDED == CFG_ON)
static uint32_t Stereo2Mono(void *audio_buf, uint32_t len, int LR)
{
    if (!audio_buf || !len || len % 4) {
        printf( "%s arg err\n", __func__);
        return 0;
    }
    int16_t *buf = audio_buf;
    uint32_t i = 0;
    LR = LR ? 1 : 0;

    for (i = 0; i < len / 4; i++) {
        buf[i] = buf[i * 2 + LR];
    }
    return len / 2;
}

int btif_co_avk_data_incoming(uint8_t type, uint8_t *p_data,uint16_t length)
{  
    Stereo2Mono(p_data, length, 1);

    tls_player_output(p_data, length>>1);
}
#else
int btif_co_avk_data_incoming(uint8_t type, uint8_t *p_data,uint16_t length)
{
	UNUSED(type);
	UNUSED(p_data);
	UNUSED(length);
	return 0;
}
#endif
static void bta2dp_connection_state_callback(tls_btav_connection_state_t state, tls_bt_addr_t *bd_addr)
{
    switch(state)
    {
        case WM_BTAV_CONNECTION_STATE_DISCONNECTED:
            TLS_BT_APPL_TRACE_DEBUG("BTAV_CONNECTION_STATE_DISCONNECTED\r\n");
            //sbc_ABV_buffer_reset();
            break;

        case WM_BTAV_CONNECTION_STATE_CONNECTING:
            TLS_BT_APPL_TRACE_DEBUG("BTAV_CONNECTION_STATE_CONNECTING\r\n");
            break;

        case WM_BTAV_CONNECTION_STATE_CONNECTED:
            TLS_BT_APPL_TRACE_DEBUG("BTAV_CONNECTION_STATE_CONNECTED\r\n");
            break;

        case WM_BTAV_CONNECTION_STATE_DISCONNECTING:
            TLS_BT_APPL_TRACE_DEBUG("BTAV_CONNECTION_STATE_DISCONNECTING\r\n");
            break;

        default:
            TLS_BT_APPL_TRACE_DEBUG("UNKNOWN BTAV_AUDIO_STATE...\r\n");
    }
}


static void bta2dp_audio_state_callback(tls_btav_audio_state_t state, tls_bt_addr_t *bd_addr)
{
    switch(state)
    {
        case WM_BTAV_AUDIO_STATE_STARTED:
            TLS_BT_APPL_TRACE_DEBUG("BTAV_AUDIO_STATE_STARTED\r\n");
            //sbc_ABV_buffer_reset();

            #if (WM_AUDIO_BOARD_INCLUDED == CFG_ON)
            tls_player_play();
			#endif

            break;

        case WM_BTAV_AUDIO_STATE_STOPPED:
            TLS_BT_APPL_TRACE_DEBUG("BTAV_AUDIO_STATE_STOPPED\r\n");
			#if (WM_AUDIO_BOARD_INCLUDED == CFG_ON)
            tls_player_stop();
			#endif
            break;

        case WM_BTAV_AUDIO_STATE_REMOTE_SUSPEND:
            TLS_BT_APPL_TRACE_DEBUG("BTAV_AUDIO_STATE_REMOTE_SUSPEND\r\n");
			#if (WM_AUDIO_BOARD_INCLUDED == CFG_ON)
            tls_player_pause();
			#endif
            break;

        default:
            TLS_BT_APPL_TRACE_DEBUG("UNKNOWN BTAV_AUDIO_STATE...\r\n");
    }
}
static void bta2dp_audio_config_callback(tls_bt_addr_t *bd_addr, uint32_t sample_rate, uint8_t channel_count)
{
    TLS_BT_APPL_TRACE_DEBUG("CBACK:%02x:%02x:%02x:%02x:%02x:%02x::sample_rate=%d, channel_count=%d\r\n",
                bd_addr->address[0], bd_addr->address[1], bd_addr->address[2], bd_addr->address[3], bd_addr->address[4], bd_addr->address[5], sample_rate, channel_count);
    
#if (WM_AUDIO_BOARD_INCLUDED == CFG_ON)
    tls_player_config(sample_rate, 16, channel_count);
#endif
	
}
static void bta2dp_audio_payload_callback(tls_bt_addr_t *bd_addr, uint8_t format, uint8_t *p_data, uint16_t length)
{
	//TLS_BT_APPL_TRACE_DEBUG("CBACK(%s): length=%d\r\n", __FUNCTION__, length);	
}

static void btavrcp_remote_features_callback(tls_bt_addr_t *bd_addr, tls_btrc_remote_features_t features)
{
    TLS_BT_APPL_TRACE_DEBUG("CBACK(%s): features:%d\r\n", __FUNCTION__, features);
}
static void btavrcp_get_play_status_callback()
{
    TLS_BT_APPL_TRACE_DEBUG("CBACK(%s): \r\n", __FUNCTION__);
}
static void btavrcp_get_element_attr_callback(uint8_t num_attr, tls_btrc_media_attr_t *p_attrs)
{
    TLS_BT_APPL_TRACE_DEBUG("CBACK(%s): num_attr:%d, param:%d\r\n", __FUNCTION__, num_attr);
}
static void btavrcp_register_notification_callback(tls_btrc_event_id_t event_id, uint32_t param)
{
    TLS_BT_APPL_TRACE_DEBUG("CBACK(%s): event_id:%d, param:%d\r\n", __FUNCTION__, event_id, param);
}

static void btavrcp_volume_change_callback(uint8_t volume, uint8_t ctype)
{
    TLS_BT_APPL_TRACE_DEBUG("CBACK: volume:%d, type:%d\r\n", volume, ctype);
}
static void btavrcp_passthrough_command_callback(int id, int pressed)
{
    TLS_BT_APPL_TRACE_DEBUG("CBACK(%s): id:%d, pressed:%d\r\n", __FUNCTION__, id, pressed);
}



static void btavrcp_passthrough_response_callback(int id, int pressed)
{
}

#if 0
static void btavrcp_connection_state_callback(bool state, tls_bt_addr_t *bd_addr)
{
    TLS_BT_APPL_TRACE_DEBUG("CBACK:%02x:%02x:%02x:%02x:%02x:%02x::state:%d\r\n",
                bd_addr->address[0], bd_addr->address[1], bd_addr->address[2], bd_addr->address[3], bd_addr->address[4], bd_addr->address[5], state);
}
#endif

static void wm_a2dp_sink_callback(tls_bt_av_evt_t evt, tls_bt_av_msg_t *msg)
{
	switch(evt)
	{
		case WMBT_A2DP_CONNECTION_STATE_EVT:
			bta2dp_connection_state_callback(msg->av_connection_state.stat, msg->av_connection_state.bd_addr);
			break;
		case WMBT_A2DP_AUDIO_STATE_EVT:
			bta2dp_audio_state_callback(msg->av_audio_state.stat, msg->av_audio_state.bd_addr);
			break;
		case WMBT_A2DP_AUDIO_CONFIG_EVT:
			bta2dp_audio_config_callback(msg->av_audio_config.bd_addr, msg->av_audio_config.sample_rate, msg->av_audio_config.channel_count);
			break;
		case WMBT_A2DP_AUDIO_PAYLOAD_EVT:
			bta2dp_audio_payload_callback(msg->av_audio_payload.bd_addr,msg->av_audio_payload.audio_format,msg->av_audio_payload.payload, msg->av_audio_payload.payload_length);
			break;
	}
}

static void wm_btrc_callback(tls_btrc_evt_t evt, tls_btrc_msg_t *msg)
{
	switch(evt)
	{
		case WM_BTRC_REMOTE_FEATURE_EVT:
			btavrcp_remote_features_callback(msg->remote_features.bd_addr, msg->remote_features.features);
			break;
		case WM_BTRC_GET_PLAY_STATUS_EVT:
			btavrcp_get_play_status_callback();
			break;
		case WM_BTRC_GET_ELEMENT_ATTR_EVT:
			btavrcp_get_element_attr_callback(msg->get_element_attr.num_attr, msg->get_element_attr.p_attrs);
			break;
		case WM_BTRC_REGISTER_NOTIFICATION_EVT:
			btavrcp_register_notification_callback(msg->register_notification.event_id, msg->register_notification.param);
			break;
		case WM_BTRC_VOLUME_CHANGED_EVT:
			btavrcp_volume_change_callback(msg->volume_change.ctype, msg->volume_change.volume);
			break;
		case WM_BTRC_PASSTHROUGH_CMD_EVT:
			btavrcp_passthrough_command_callback(msg->passthrough_cmd.id, msg->passthrough_cmd.key_state);
			break;
		default:
			TLS_BT_APPL_TRACE_VERBOSE("unhandled wm_btrc_callback, evt=%d\r\n", evt);
			break;
	}
}


static void wm_btrc_ctrl_callback(tls_btrc_ctrl_evt_t evt, tls_btrc_ctrl_msg_t *msg)
{
	switch(evt)
	{
		case WM_BTRC_PASSTHROUGH_CMD_EVT:
			btavrcp_passthrough_response_callback(msg->passthrough_rsp.id, msg->passthrough_rsp.key_state);
			break;
		default:
			TLS_BT_APPL_TRACE_VERBOSE("unhandled wm_btrc_ctrl_callback, evt=%d\r\n", evt);
			break;	
	}
}

tls_bt_status_t tls_bt_enable_a2dp_sink()
{
	tls_bt_status_t status;

	status = tls_bt_av_sink_init(wm_a2dp_sink_callback);
	if(status != TLS_BT_STATUS_SUCCESS)
	{
		TLS_BT_APPL_TRACE_ERROR("tls_bt_av_sink_init failed, status=%d\r\n", status);
		return status;
	}

	status = tls_btrc_init(wm_btrc_callback);
	if(status != TLS_BT_STATUS_SUCCESS)
	{
		TLS_BT_APPL_TRACE_ERROR("tls_btrc_init failed, status=%d\r\n", status);
		tls_bt_av_sink_deinit();
		return status;
	}
	
	status = tls_btrc_ctrl_init(wm_btrc_ctrl_callback);
	if(status != TLS_BT_STATUS_SUCCESS)
	{
		TLS_BT_APPL_TRACE_ERROR("tls_btrc_init failed, status=%d\r\n", status);
		tls_bt_av_sink_deinit();
		tls_btrc_deinit();
		return status;
	}	
	#if (WM_AUDIO_BOARD_INCLUDED == CFG_ON)
    tls_player_init();
	#endif

	return status;
}

tls_bt_status_t tls_bt_disable_a2dp_sink()
{
	tls_bt_av_sink_deinit();
	tls_btrc_deinit();
	tls_btrc_ctrl_deinit();
#if (WM_AUDIO_BOARD_INCLUDED == CFG_ON)
    tls_player_deinit();
#endif

	return TLS_BT_STATUS_SUCCESS;
}

#endif

