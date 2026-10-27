#include "../obk_config.h"

#if ENABLE_DRIVER_GAITEKAC

#include <string.h>
#include <stdio.h>

#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "drv_uart.h"
#include "drv_gaitekAC.h"

#if ENABLE_MQTT
#include "../mqtt/new_mqtt.h"
#endif

/*
 * GAITEK AC UART protocol
 *
 * UART: 9600 8N1
 *
 * OpenRTL -> AC:
 *   "GAITEK" BD CMD VAL 00 00 00 CMD CHK
 *   CHK = XOR(BD CMD VAL 00 00 00 CMD)
 *
 * AC -> OpenRTL:
 *   "GAITEK" BD PP FF MF RT ST 20 20 TM 00 AUX CHK "KETIAG"
 *   CHK = XOR(BD..AUX)
 *
 * PP = power, 00 off, 01 on
 * FF = flags, bit 0x04 = night/sleep mode
 * MF = fan/mode, high nibble fan, low nibble mode
 *      fan: 1 high, 2 medium, 3 low
 *      mode: 1 cold/cool, 3 dry, 6 fan only
 * RT = room temperature in Celsius
 * ST = set temperature in Celsius
 * TM = timer hours, 0 disabled
 */

#define GAITEK_CH_POWER       1
#define GAITEK_CH_MODE        2
#define GAITEK_CH_FAN         3
#define GAITEK_CH_NIGHT       4
#define GAITEK_CH_SET_TEMP    5
#define GAITEK_CH_TIMER       6
#define GAITEK_CH_ROOM_TEMP   10

#define GAITEK_UART_BAUD      9600
#define GAITEK_RXBUF_SIZE     256
#define GAITEK_FRAME_LEN      24
#define GAITEK_BODY_OFF       6
#define GAITEK_BODY_LEN       12

static const byte GAITEK_HEADER[6] = { 'G', 'A', 'I', 'T', 'E', 'K' };
static const byte GAITEK_FOOTER[6] = { 'K', 'E', 'T', 'I', 'A', 'G' };

static byte g_gaitek_rx[GAITEK_FRAME_LEN];
static int g_gaitek_rxpos = 0;
static int g_gaitek_updating_from_ac = 0;
static int g_gaitek_dirty = 1;
static int g_gaitek_seconds_since_publish = 999;
static int g_gaitek_pending_mode = 0;
static int g_gaitek_pending_mode_delay = 0;

static int g_gaitek_power = 0;
static int g_gaitek_mode = 1;
static int g_gaitek_fan = 2;
static int g_gaitek_night = 0;
static int g_gaitek_set_temp_x10 = 220;
static int g_gaitek_room_temp_x10 = 0;
static int g_gaitek_timer = 0;
static int g_gaitek_have_status = 0;

static byte gaitek_xor(const byte *data, int len) {
	byte x = 0;
	for (int i = 0; i < len; i++) {
		x ^= data[i];
	}
	return x;
}

static int gaitek_clamp(int val, int minVal, int maxVal) {
	if (val < minVal) return minVal;
	if (val > maxVal) return maxVal;
	return val;
}

static int gaitek_streq(const char *a, const char *b) {
	while (*a && *b) {
		char ca = *a++;
		char cb = *b++;
		if (ca >= 'A' && ca <= 'Z') ca = ca - 'A' + 'a';
		if (cb >= 'A' && cb <= 'Z') cb = cb - 'A' + 'a';
		if (ca != cb) return 0;
	}
	return *a == 0 && *b == 0;
}

static const char *gaitek_hvac_mode(void) {
	if (!g_gaitek_power) return "off";
	switch (g_gaitek_mode) {
	case 1: return "cool";
	case 3: return "dry";
	case 6: return "fan_only";
	default: return "unknown";
	}
}

static const char *gaitek_fan_mode(void) {
	switch (g_gaitek_fan) {
	case 1: return "high";
	case 2: return "medium";
	case 3: return "low";
	default: return "unknown";
	}
}

static const char *gaitek_preset_mode(void) {
	return g_gaitek_night ? "sleep" : "none";
}

static void gaitek_mark_dirty(void) {
	g_gaitek_dirty = 1;
}

static void gaitek_write_channel(int ch, int val) {
	char tmp[48];
	snprintf(tmp, sizeof(tmp), "setChannel %i %i", ch, val);
	CMD_ExecuteCommand(tmp, 0);
}

static void gaitek_sync_channels_from_cache(void) {
	g_gaitek_updating_from_ac = 1;
	gaitek_write_channel(GAITEK_CH_POWER, g_gaitek_power);
	gaitek_write_channel(GAITEK_CH_MODE, g_gaitek_mode);
	gaitek_write_channel(GAITEK_CH_FAN, g_gaitek_fan);
	gaitek_write_channel(GAITEK_CH_NIGHT, g_gaitek_night);
	gaitek_write_channel(GAITEK_CH_SET_TEMP, g_gaitek_set_temp_x10);
	gaitek_write_channel(GAITEK_CH_TIMER, g_gaitek_timer);
	gaitek_write_channel(GAITEK_CH_ROOM_TEMP, g_gaitek_room_temp_x10);
	g_gaitek_updating_from_ac = 0;
}

static void gaitek_build_state_json(char *out, int outLen) {
	snprintf(out, outLen,
		"{\"power\":%i,\"hvac_mode\":\"%s\",\"mode\":\"%s\",\"mode_raw\":%i,"
		"\"fan\":\"%s\",\"fan_raw\":%i,\"night\":%i,\"preset\":\"%s\","
		"\"set_temp\":%i,\"setTemp\":%i,\"room_temp\":%i,\"roomTemp\":%i,"
		"\"timer\":%i,\"have_status\":%i,\"haveStatus\":%i}",
		g_gaitek_power,
		gaitek_hvac_mode(),
		gaitek_hvac_mode(),
		g_gaitek_mode,
		gaitek_fan_mode(),
		g_gaitek_fan,
		g_gaitek_night,
		gaitek_preset_mode(),
		g_gaitek_set_temp_x10 / 10,
		g_gaitek_set_temp_x10,
		g_gaitek_room_temp_x10 / 10,
		g_gaitek_room_temp_x10,
		g_gaitek_timer,
		g_gaitek_have_status,
		g_gaitek_have_status);
}

static void gaitek_publish_state(int force) {
	char json[384];

	gaitek_build_state_json(json, sizeof(json));

#if ENABLE_MQTT
	if (force || g_gaitek_dirty || g_gaitek_seconds_since_publish >= 60) {
		if (MQTT_IsReady()) {
			MQTT_PublishStat("gaitek_ac", json);
			g_gaitek_dirty = 0;
			g_gaitek_seconds_since_publish = 0;
		}
	}
#else
	(void)force;
#endif
}

static void gaitek_cache_command(byte cmd_id, byte value) {
	switch (cmd_id) {
	case 0x00:
		g_gaitek_power = value ? 1 : 0;
		break;
	case 0x05:
		g_gaitek_night = value ? 1 : 0;
		break;
	case 0x06:
		if (value == 1 || value == 3 || value == 6) g_gaitek_mode = value;
		break;
	case 0x07:
		if (value >= 1 && value <= 3) g_gaitek_fan = value;
		break;
	case 0x08:
		g_gaitek_set_temp_x10 = value * 10;
		break;
	case 0x0A:
		g_gaitek_timer = value;
		break;
	default:
		return;
	}

	gaitek_sync_channels_from_cache();
	gaitek_mark_dirty();
	gaitek_publish_state(0);
}

static void gaitek_send_command(byte cmd_id, byte value) {
	byte frame[14] = {
		'G', 'A', 'I', 'T', 'E', 'K',
		0xBD, cmd_id, value, 0x00, 0x00, 0x00, cmd_id, 0x00
	};

	if (cmd_id != 0xFF) {
		gaitek_cache_command(cmd_id, value);
	}

	frame[13] = gaitek_xor(&frame[6], 7);

	for (int i = 0; i < (int)sizeof(frame); i++) {
		UART_SendByte(frame[i]);
	}
}

static void gaitek_parse_status(const byte *frame) {
	const byte *body = frame + GAITEK_BODY_OFF;

	if (memcmp(frame, GAITEK_HEADER, 6) != 0) return;
	if (memcmp(frame + 18, GAITEK_FOOTER, 6) != 0) return;
	if (body[0] != 0xBD) return;

	byte calc = gaitek_xor(body, GAITEK_BODY_LEN - 1);
	if (calc != body[11]) return;

	int power = body[1] ? 1 : 0;
	int flags = body[2];
	int fanmode = body[3];
	int fan = (fanmode >> 4) & 0x0F;
	int mode = fanmode & 0x0F;
	int roomTempC = body[4];
	int setTempC = body[5];
	int timerH = body[8];

	g_gaitek_power = power;
	g_gaitek_mode = mode;
	g_gaitek_fan = fan;
	g_gaitek_night = (flags & 0x04) ? 1 : 0;
	g_gaitek_set_temp_x10 = setTempC * 10;
	g_gaitek_room_temp_x10 = roomTempC * 10;
	g_gaitek_timer = timerH;
	g_gaitek_have_status = 1;

	gaitek_sync_channels_from_cache();
	gaitek_mark_dirty();
	gaitek_publish_state(1);
}

static void gaitek_rx_byte(byte b) {
	if (g_gaitek_rxpos == 0 && b != 'G') {
		return;
	}

	g_gaitek_rx[g_gaitek_rxpos++] = b;

	if (g_gaitek_rxpos <= 6) {
		if (g_gaitek_rx[g_gaitek_rxpos - 1] != GAITEK_HEADER[g_gaitek_rxpos - 1]) {
			g_gaitek_rxpos = 0;
		}
		return;
	}

	if (g_gaitek_rxpos == GAITEK_FRAME_LEN) {
		gaitek_parse_status(g_gaitek_rx);
		g_gaitek_rxpos = 0;
	}
	else if (g_gaitek_rxpos > GAITEK_FRAME_LEN) {
		g_gaitek_rxpos = 0;
	}
}

void GaitekAC_RunQuickTick(void) {
	while (UART_GetDataSize() > 0) {
		byte b = UART_GetByte(0);
		UART_ConsumeBytes(1);
		gaitek_rx_byte(b);
	}
}

static void gaitek_queue_mode_after_power(byte mode) {
	g_gaitek_pending_mode = mode;
	g_gaitek_pending_mode_delay = 1;
}

void GaitekAC_RunEverySecond(void) {
	g_gaitek_seconds_since_publish++;

	if (g_gaitek_pending_mode) {
		if (g_gaitek_pending_mode_delay > 0) {
			g_gaitek_pending_mode_delay--;
		} else {
			byte mode = g_gaitek_pending_mode;
			g_gaitek_pending_mode = 0;
			gaitek_send_command(0x06, mode);
		}
	}

	gaitek_publish_state(0);
}

static int http_gaitekac_status(http_request_t *request) {
	char json[384];

	http_setup(request, httpMimeTypeJson);
	gaitek_build_state_json(json, sizeof(json));
	poststr(request, json);
	poststr(request, NULL);
	return 0;
}

static commandResult_t CMD_GaitekACPower(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	gaitek_send_command(0x00, Tokenizer_GetArgInteger(0) ? 1 : 0);
	return CMD_RES_OK;
}

static commandResult_t CMD_GaitekACNight(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	gaitek_send_command(0x05, Tokenizer_GetArgInteger(0) ? 1 : 0);
	return CMD_RES_OK;
}

static commandResult_t CMD_GaitekACMode(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1) return CMD_RES_NOT_ENOUGH_ARGUMENTS;

	const char *s = Tokenizer_GetArg(0);
	int v;

	if (gaitek_streq(s, "cold") || gaitek_streq(s, "cool")) v = 0x01;
	else if (gaitek_streq(s, "dry")) v = 0x03;
	else if (gaitek_streq(s, "fan") || gaitek_streq(s, "fan_only")) v = 0x06;
	else v = Tokenizer_GetArgInteger(0);

	if (v != 0x01 && v != 0x03 && v != 0x06) return CMD_RES_BAD_ARGUMENT;
	gaitek_send_command(0x06, v);
	return CMD_RES_OK;
}

static commandResult_t CMD_GaitekACHAMode(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1) return CMD_RES_NOT_ENOUGH_ARGUMENTS;

	const char *s = Tokenizer_GetArg(0);
	byte mode = 0;

	if (gaitek_streq(s, "off")) {
		g_gaitek_pending_mode = 0;
		g_gaitek_pending_mode_delay = 0;
		gaitek_send_command(0x00, 0);
		return CMD_RES_OK;
	}

	if (gaitek_streq(s, "cool") || gaitek_streq(s, "cold")) {
		mode = 1;
	}
	else if (gaitek_streq(s, "dry")) {
		mode = 3;
	}
	else if (gaitek_streq(s, "fan") || gaitek_streq(s, "fan_only")) {
		mode = 6;
	}
	else {
		return CMD_RES_BAD_ARGUMENT;
	}

	/*
	 * Smart power logic:
	 * If the AC is already on according to the latest cached/status state,
	 * do not send another power-on command. Just change mode.
	 *
	 * If the AC is off, power it on first and send the requested mode
	 * one second later. Some GAITEK units ignore back-to-back power+mode.
	 */
	if (g_gaitek_power) {
		g_gaitek_pending_mode = 0;
		g_gaitek_pending_mode_delay = 0;
		gaitek_send_command(0x06, mode);
	}
	else {
		gaitek_send_command(0x00, 1);
		gaitek_queue_mode_after_power(mode);
	}

	return CMD_RES_OK;
}

static commandResult_t CMD_GaitekACFan(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1) return CMD_RES_NOT_ENOUGH_ARGUMENTS;

	const char *s = Tokenizer_GetArg(0);
	int v;

	if (gaitek_streq(s, "high")) v = 0x01;
	else if (gaitek_streq(s, "medium") || gaitek_streq(s, "med")) v = 0x02;
	else if (gaitek_streq(s, "low")) v = 0x03;
	else v = Tokenizer_GetArgInteger(0);

	if (v < 1 || v > 3) return CMD_RES_BAD_ARGUMENT;
	gaitek_send_command(0x07, v);
	return CMD_RES_OK;
}

static commandResult_t CMD_GaitekACPreset(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1) return CMD_RES_NOT_ENOUGH_ARGUMENTS;

	const char *s = Tokenizer_GetArg(0);

	if (gaitek_streq(s, "none") || gaitek_streq(s, "normal") || gaitek_streq(s, "off")) {
		gaitek_send_command(0x05, 0);
		return CMD_RES_OK;
	}
	if (gaitek_streq(s, "sleep") || gaitek_streq(s, "night") || gaitek_streq(s, "on")) {
		gaitek_send_command(0x05, 1);
		return CMD_RES_OK;
	}

	return CMD_RES_BAD_ARGUMENT;
}

static commandResult_t CMD_GaitekACTemp(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	gaitek_send_command(0x08, gaitek_clamp(Tokenizer_GetArgInteger(0), 16, 31));
	return CMD_RES_OK;
}

static commandResult_t CMD_GaitekACTimer(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	gaitek_send_command(0x0A, gaitek_clamp(Tokenizer_GetArgInteger(0), 0, 24));
	return CMD_RES_OK;
}

static commandResult_t CMD_GaitekACRefresh(const void *context, const char *cmd, const char *args, int cmdFlags) {
	gaitek_send_command(0xFF, 0x01);
	return CMD_RES_OK;
}

static commandResult_t CMD_GaitekACPublish(const void *context, const char *cmd, const char *args, int cmdFlags) {
	gaitek_publish_state(1);
	return CMD_RES_OK;
}

void GaitekAC_OnChannelChanged(int ch, int val) {
	if (g_gaitek_updating_from_ac) return;

	switch (ch) {
	case GAITEK_CH_POWER:
		gaitek_send_command(0x00, val ? 1 : 0);
		break;
	case GAITEK_CH_MODE:
		if (val == 0x01 || val == 0x03 || val == 0x06) gaitek_send_command(0x06, val);
		break;
	case GAITEK_CH_FAN:
		if (val >= 1 && val <= 3) gaitek_send_command(0x07, val);
		break;
	case GAITEK_CH_NIGHT:
		gaitek_send_command(0x05, val ? 1 : 0);
		break;
	case GAITEK_CH_SET_TEMP:
		gaitek_send_command(0x08, gaitek_clamp((val + 5) / 10, 16, 31));
		break;
	case GAITEK_CH_TIMER:
		gaitek_send_command(0x0A, gaitek_clamp(val, 0, 24));
		break;
	default:
		break;
	}
}

void GaitekAC_Init(void) {
	UART_InitReceiveRingBuffer(GAITEK_RXBUF_SIZE);
	UART_InitUART(GAITEK_UART_BAUD, 0, false);

	CMD_RegisterCommand("GaitekACPower", CMD_GaitekACPower, NULL);
	CMD_RegisterCommand("GaitekACMode", CMD_GaitekACMode, NULL);
	CMD_RegisterCommand("GaitekACHAMode", CMD_GaitekACHAMode, NULL);
	CMD_RegisterCommand("GaitekACFan", CMD_GaitekACFan, NULL);
	CMD_RegisterCommand("GaitekACNight", CMD_GaitekACNight, NULL);
	CMD_RegisterCommand("GaitekACPreset", CMD_GaitekACPreset, NULL);
	CMD_RegisterCommand("GaitekACTemp", CMD_GaitekACTemp, NULL);
	CMD_RegisterCommand("GaitekACTimer", CMD_GaitekACTimer, NULL);
	CMD_RegisterCommand("GaitekACRefresh", CMD_GaitekACRefresh, NULL);
	CMD_RegisterCommand("GaitekACPublish", CMD_GaitekACPublish, NULL);

	HTTP_RegisterCallback("/gaitekac", HTTP_GET, http_gaitekac_status, 0);

	gaitek_sync_channels_from_cache();
	gaitek_mark_dirty();
}

void GaitekAC_Shutdown(void) {
	g_gaitek_rxpos = 0;
}

#endif
