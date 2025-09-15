//
// Low-Power TuyaMCU information
//

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../quicktick.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "../hal/hal_wifi.h"
#include "../mqtt/new_mqtt.h"
#include "drv_public.h"
#include "drv_tuyaMCULE.h"
#include "drv_uart.h"
#include "drv_public.h"
#include "drv_ntp.h"

#define TUYAMCULE_CMD_PRODUCT_INFO                   0x01
#define TUYAMCULE_CMD_WIFI_STATE                     0x02
#define TUYAMCULE_CMD_WIFI_RESET                     0x03
#define TUYAMCULE_CMD_WIFI_MODE                      0x04
#define TUYAMCULE_CMD_STATE                          0x05
#define TUYAMCULE_CMD_GET_LOCAL_TIME                 0x06
#define TUYAMCULE_CMD_WIFI_TEST                      0x07
#define TUYAMCULE_CMD_STATE_RC                       0x08
#define TUYAMCULE_CMD_SET_DATA_POINT                 0x09
#define TUYAMCULE_CMD_WIFI_UPDATE                    0x0a
#define TUYAMCULE_CMD_GET_RSSI                       0x0b
#define TUYAMCULE_CMD_REQUEST_MCU_UG                 0x0c
#define TUYAMCULE_CMD_UPDATE_START                   0x0d
#define TUYAMCULE_CMD_UPDATE_TRANS                   0x0e
#define TUYAMCULE_CMD_GET_DP_CACHE                   0x10

#define TUYAMCULE_NETWORK_STATUS_SMART_CONNECT_SETUP 0x00
#define TUYAMCULE_NETWORK_STATUS_AP_MODE             0x01
#define TUYAMCULE_NETWORK_STATUS_NOT_CONNECTED       0x02
#define TUYAMCULE_NETWORK_STATUS_CONNECTED_TO_ROUTER 0x03
#define TUYAMCULE_NETWORK_STATUS_CONNECTED_TO_CLOUD  0x04

#define DP_TYPE_RAW    0x00
#define DP_TYPE_BOOL   0x01
#define DP_TYPE_VALUE  0x02
#define DP_TYPE_STRING 0x03
#define DP_TYPE_ENUM   0x04
#define DP_TYPE_BITMAP 0x05

enum TuyaMCULEState {
	TUYAMCULE_STATE_AWAITING_INFO,
	TUYAMCULE_STATE_AWAITING_WIFI,
	TUYAMCULE_STATE_AWAITING_MQTT,
	TUYAMCULE_STATE_AWAITING_STATES,
};

typedef struct TuyaMCULE_dpConfig_s {
	byte dpId;
	byte dpType;
	short channel;
	struct TuyaMCULE_dpConfig_s *next;
} TuyaMCULE_dpConfig_t;

static int g_baudRate = 9600;

static bool g_wifiReset = false;
static int g_tuyaNextRequestDelay;
static byte g_tuyaBatteryPoweredState = TUYAMCULE_STATE_AWAITING_INFO;
static byte g_defaultTuyaMCUWiFiState = TUYAMCULE_NETWORK_STATUS_SMART_CONNECT_SETUP;

static TuyaMCULE_dpConfig_t *g_dpConfigs = 0;

static int g_HandleStateRcCbEpectedCount = 0;

const char* TuyaMCULE_GetDataTypeString(int dpId) {
	switch (dpId) {
		case DP_TYPE_RAW: return "raw";
		case DP_TYPE_BOOL: return "bool";
		case DP_TYPE_VALUE: return "val";
		case DP_TYPE_STRING: return "str";
		case DP_TYPE_ENUM: return "enum";
		case DP_TYPE_BITMAP: return "bitmap";
	}

	return "unknown";
}

const char* TuyaMCULE_GetCommandTypeLabel(int t) {
	switch (t) {
		case TUYAMCULE_CMD_PRODUCT_INFO: return "ProductInfo";
		case TUYAMCULE_CMD_WIFI_STATE: return "WifiState";
		case TUYAMCULE_CMD_WIFI_RESET: return "WifiReset";
		case TUYAMCULE_CMD_WIFI_MODE: return "WifiMode";
		case TUYAMCULE_CMD_STATE: return "State";
		case TUYAMCULE_CMD_GET_LOCAL_TIME: return "GetLocalTime";
		case TUYAMCULE_CMD_WIFI_TEST: return "WifiTest";
		case TUYAMCULE_CMD_STATE_RC: return "StateRC";
		case TUYAMCULE_CMD_SET_DATA_POINT: return "SetDataPoint";
		case TUYAMCULE_CMD_WIFI_UPDATE: return "WifiUpdate";
		case TUYAMCULE_CMD_GET_RSSI: return "Rssi";
		case TUYAMCULE_CMD_REQUEST_MCU_UG: return "RequestMCUUG";
		case TUYAMCULE_CMD_UPDATE_START: return "UpdateStart";
		case TUYAMCULE_CMD_UPDATE_TRANS: return "UprateTrans";
		case TUYAMCULE_CMD_GET_DP_CACHE: return "GetDPCache";
	}

	return "Unknown";
}

TuyaMCULE_dpConfig_t *TuyaMCULE_getDPConfig(int dpId) {
	for (TuyaMCULE_dpConfig_t *cur = g_dpConfigs; cur; cur = cur->next) {
		if (cur->dpId == dpId)
			return cur;
	}
	return 0;
}

void TuyaMCULE_SendCommand(byte cmd, byte* data, int len) {
	UART_SendByte(0x55);
	UART_SendByte(0xAA);
	UART_SendByte(0x00); // version
	UART_SendByte(cmd);
	UART_SendByte(len >> 8);
	UART_SendByte(len & 0xFF);
	byte check_sum = (0xFF + cmd + (len >> 8) + (len & 0xFF));
	for (int i = 0; i < len; i++) {
		byte b = data[i];
		check_sum += b;
		UART_SendByte(b);
	}
	UART_SendByte(check_sum);

	char buffer_for_log[256] = { 0 };
	for (int i = 0; i < len; i++)
		snprintf(buffer_for_log, sizeof(buffer_for_log), "%s%02X ", buffer_for_log, data[i]);
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "Sent: 55 AA 00 %02X %02X %02X %s%02X\n", cmd, len >> 8, len & 0xFF, buffer_for_log, check_sum);
}

void TuyaMCULE_GetProductInfo()
{
	TuyaMCULE_SendCommand(TUYAMCULE_CMD_PRODUCT_INFO, NULL, 0);
}

void TuyaMCULE_SetWifiState(uint8_t state)
{
	TuyaMCULE_SendCommand(TUYAMCULE_CMD_WIFI_STATE, &state, 1);
}

void TuyaMCULE_SetLocalTime(struct tm* pTime) {
	byte payload_buffer[8];

	if (pTime == 0) {
		memset(payload_buffer, 0, sizeof(payload_buffer));
	}
	else {
		byte tuya_day_of_week;

		// convert from [0, 6] days since Sunday range to [1, 7] where 1 indicates monday
		if (pTime->tm_wday == 0) {
			tuya_day_of_week = 7;
		}
		else {
			tuya_day_of_week = pTime->tm_wday;
		}
		// valid flag
		payload_buffer[0] = 0x01;
		// datetime
		payload_buffer[1] = pTime->tm_year % 100;
		// tm uses: int tm_mon;   // months since January - [0, 11]
		// Tuya uses:  Data[1] indicates the month, ranging from 1 to 12.
		payload_buffer[2] = pTime->tm_mon + 1;
		payload_buffer[3] = pTime->tm_mday;
		payload_buffer[4] = pTime->tm_hour;
		payload_buffer[5] = pTime->tm_min;
		payload_buffer[6] = pTime->tm_sec;
		// Data[7]: indicates the week, ranging from 1 to 7. 1 indicates Monday.
		payload_buffer[7] = tuya_day_of_week; //1 for Monday in TUYA Doc
	}

	TuyaMCULE_SendCommand(TUYAMCULE_CMD_GET_LOCAL_TIME, payload_buffer, 8);
}

// TUYAMCULE_CMD_SET_DATA_POINT

void TuyaMCULE_SetRSSI(byte rssi) {
	byte buf[] = { 1, rssi };
	TuyaMCULE_SendCommand(TUYAMCULE_CMD_GET_RSSI, buf, sizeof(buf));
}

// TUYAMCULE_CMD_UPDATE_TRANS

void TuyaMCULE_RunEverySecond() {
	if (g_wifiReset) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "User requested Reset - Operating Open AP\n");
		/* Set current state to Setup */
		TuyaMCULE_SetWifiState(TUYAMCULE_NETWORK_STATUS_SMART_CONNECT_SETUP);

		/* Don't interact further with MCU when in reset */
		return;
	}

	if (--g_tuyaNextRequestDelay > 0)
		return;

	switch (g_tuyaBatteryPoweredState) {
	case TUYAMCULE_STATE_AWAITING_INFO:
		TuyaMCULE_GetProductInfo();
		g_tuyaNextRequestDelay = 3;
		break;
	case TUYAMCULE_STATE_AWAITING_WIFI:
		if (Main_IsConnectedToWiFi()) {
			TuyaMCULE_SetWifiState(TUYAMCULE_NETWORK_STATUS_CONNECTED_TO_ROUTER);
			// retry
			g_tuyaNextRequestDelay = 3;
		}
		break;
	case TUYAMCULE_STATE_AWAITING_MQTT:
#if !WINDOWS
		if (g_defaultTuyaMCUWiFiState == TUYAMCULE_NETWORK_STATUS_CONNECTED_TO_CLOUD ||
			Main_HasMQTTConnected() || g_cfg.mqtt_host[0] == 0
			)
#endif
		{
			TuyaMCULE_SetWifiState(TUYAMCULE_NETWORK_STATUS_CONNECTED_TO_CLOUD);
			// retry
			g_tuyaNextRequestDelay = 3;
		}
		break;
	case TUYAMCULE_STATE_AWAITING_STATES:
		break;
	}
}

void TuyaMCULE_PrintPacket(byte *data, int len) {
	char buffer_for_log[256] = { 0 };
	for (int i = 0; i < len; i++)
		snprintf(buffer_for_log, sizeof(buffer_for_log), "%s%02X ", buffer_for_log, data[i]);
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "Received: %s\n", buffer_for_log);

#if 1
	// redo sprintf without spaces
	buffer_for_log[0] = 0;
	for (int i = 0; i < len; i++)
		snprintf(buffer_for_log, sizeof(buffer_for_log), "%s%02X", buffer_for_log, data[i]);

	// fire string event, as we already have it sprintfed
	// This is so we can have event handlers that fire
	// when an UART string is received...
	EventHandlers_FireEvent_String(CMD_EVENT_ON_UART, buffer_for_log);
#endif
}

void TuyaMCULE_HandleProductInformation(const byte* data, int len) {
	char name[256];
	int useLen = len;
	if (useLen > sizeof(name) - 1)
		useLen = sizeof(name) - 1;
	memcpy(name, data, useLen);
	name[useLen] = 0;

	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "HandleProductInformation: received %s\n", name);

	if (g_tuyaBatteryPoweredState == TUYAMCULE_STATE_AWAITING_INFO) {
		g_tuyaBatteryPoweredState = TUYAMCULE_STATE_AWAITING_WIFI;
		g_tuyaNextRequestDelay = 0;
	}
}

void TuyaMCULE_HandleWifiState(const byte* data, int len) {
	switch (g_tuyaBatteryPoweredState)
	{
		case TUYAMCULE_STATE_AWAITING_WIFI:
			g_tuyaBatteryPoweredState = TUYAMCULE_STATE_AWAITING_MQTT;
			break;
		case TUYAMCULE_STATE_AWAITING_MQTT:
			g_tuyaBatteryPoweredState = TUYAMCULE_STATE_AWAITING_STATES;
			g_tuyaNextRequestDelay = 0;
			break;
	}
}

void TuyaMCULE_HandleWifiReset(const byte* data, int len) {
	g_wifiReset = true;

	TuyaMCULE_SendCommand(TUYAMCULE_CMD_WIFI_RESET, 0, 0);
}

/*
TUYAMCULE_CMD_WIFI_MODE
*/

void TuyaMCULE_HandleStateCb(void *arg) {
	byte zero = 0;
	TuyaMCULE_SendCommand(TUYAMCULE_CMD_STATE, &zero, 1);
}

void TuyaMCULE_HandleState(const byte* data, int len) {
	byte dpId = data[0];
	byte dpType = data[1];
	unsigned short dpLen = ((data[2] << 8) | data[3]);

	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "HandleState: dpId %d, dpType %s (0x%x) and %i data bytes\n",
			dpId, TuyaMCULE_GetDataTypeString(dpType), dpType, dpLen);

	char buffer_for_log[256] = { 0 };
	for (int i = 0; i < dpLen; i++)
		snprintf(buffer_for_log, sizeof(buffer_for_log), "%s%02X ", buffer_for_log, data[4 + i]);
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "HandleState: dpValue: %s\n", buffer_for_log);

	TuyaMCULE_dpConfig_t *dpConfig = TuyaMCULE_getDPConfig(dpId);
	if (!dpConfig) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "HandleState: Missing conbig for dpId: %d\n", dpId);
		TuyaMCULE_HandleStateCb(0);
		return;
	}

	if (dpConfig->dpType != dpType) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "HandleState: dpType is different then expected for dpId %d: %s (0x%x) != %s (0x%x)\n", dpId, TuyaMCULE_GetDataTypeString(dpConfig->dpType), dpConfig->dpType, TuyaMCULE_GetDataTypeString(dpType), dpType);
		TuyaMCULE_HandleStateCb(0);
		return;
	}

	int value = 0;
	switch (dpLen)
	{
		case 1: value = data[4]; break;
		case 2: value = (data[4] << 8) | data[5]; break;
		case 4: value = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7]; break;
		default:
			addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "HandleState: supported dpLen (%d) for dpId %d\n", dpLen, dpId);
			TuyaMCULE_HandleStateCb(0);
			return;
	}

	CHANNEL_SetWithCB(dpConfig->channel, value, 0, TuyaMCULE_HandleStateCb, 0);
}

void TuyaMCULE_HandleGetLocalTime(const byte* data, int len) {
	TuyaMCULE_SetLocalTime(gmtime(&g_ntpTime));
}

/*
TUYAMCULE_CMD_WIFI_TEST
*/

void TuyaMCULE_HandleStateRCCb(void *arg) {
	--g_HandleStateRcCbEpectedCount;
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "HandleStateRCCb: g_HandleStateRcCbEpectedCount: %d\n", g_HandleStateRcCbEpectedCount);
	if (g_HandleStateRcCbEpectedCount == 0) {
		byte zero = 0;
		TuyaMCULE_SendCommand(TUYAMCULE_CMD_STATE_RC, &zero, 1);
	}
}

void TuyaMCULE_HandleStateRC(const byte* data, int len) {
	if (g_HandleStateRcCbEpectedCount > 0) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "HandleStateRC: Wtf?! g_HandleStateRcCbEpectedCount > 0 (%d)\n", g_HandleStateRcCbEpectedCount);
	}

	byte time_valid = data[0];
	int year = 2000 + data[1];
	byte month = data[2];
	byte day = data[3];
	byte hour = data[4];
	byte min = data[5];
	byte sec = data[6];

	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "HandleStateRC: Time: Valid: %d | %d. %d. %d. %02d:%02d:%02d\n",
			time_valid, year, month, day, hour, min, sec);

	data += 7;
	len -= 7;

	int setCount = 0;

	while (len >= 4)
	{
		byte dpId = data[0];
		byte dpType = data[1];
		unsigned short dpLen = ((data[2] << 8) | data[3]);

		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "HandleStateRC: dpId %i, dpType %s (0x%x) and %i data bytes\n",
				dpId, TuyaMCULE_GetDataTypeString(dpType), dpType, dpLen);

		if (dpLen > len) {
			addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "HandleStateRC: dpLen (%d) > len (%d)\n", dpLen, len);
			break;
		}

		char buffer_for_log[256] = { 0 };
		for (int i = 0; i < dpLen; i++)
			snprintf(buffer_for_log, sizeof(buffer_for_log), "%s%02X ", buffer_for_log, data[4 + i]);
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "HandleStateRC: dpValue: %s\n", buffer_for_log);

		TuyaMCULE_dpConfig_t *dpConfig = TuyaMCULE_getDPConfig(dpId);
		if (!dpConfig) {
			addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "HandleStateRC: Missing conbig for dpId: %d\n", dpId);
			continue;
		}

		if (dpConfig->dpType != dpType) {
			addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "HandleStateRC: dpType is different then expected for dpId %d: %s (0x%x) != %s (0x%x)\n", dpId, TuyaMCULE_GetDataTypeString(dpConfig->dpType), dpConfig->dpType, TuyaMCULE_GetDataTypeString(dpType), dpType);
			continue;
		}

		int value = 0;
		switch (dpLen)
		{
			case 1: value = data[4]; break;
			case 2: value = (data[4] << 8) | data[5]; break;
			case 4: value = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7]; break;
			default:
				addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "HandleStateRC: supported dpLen (%d) for dpId %d\n", dpLen, dpId);
				continue;
		}

		++setCount;
		++g_HandleStateRcCbEpectedCount;
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "HandleStateRC: g_HandleStateRcCbEpectedCount: %d\n", g_HandleStateRcCbEpectedCount);
		CHANNEL_SetWithCB(dpConfig->channel, value, 0, TuyaMCULE_HandleStateRCCb, 0);

		data += 4 + dpLen;
		len -= 4 + dpLen;
	}

	if (setCount == 0) {
		byte zero = 0;
		TuyaMCULE_SendCommand(TUYAMCULE_CMD_STATE_RC, &zero, 1);
	}
}

/*
TUYAMCULE_CMD_WIFI_UPDATE
*/

void TuyaMCULE_HandleGetRssi(const byte* data, int len) {
	TuyaMCULE_SetRSSI(HAL_GetWifiStrength());
}

/*
TUYAMCULE_CMD_REQUEST_MCU_UG
TUYAMCULE_CMD_UPDATE_START
*/

void TuyaMCULE_HandleGetDpCache(const byte* data, int len) {
	byte dp_num = data[0];

	if (len <= dp_num) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "HandleGetDpCache: dp_num (%d) > len (%d)\n", dp_num, len);
		return;
	}

	char buffer_for_log[256] = { 0 };
	for (int i = 0; i < dp_num; i++)
		snprintf(buffer_for_log, sizeof(buffer_for_log), "%s%02X ", buffer_for_log, data[1 + i]);
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "HandleGetDpCache: dpId table: %s\n", buffer_for_log);

	byte *resp = NULL;
	int resp_len = 2;

	if (dp_num == 0) {
		byte dp_count = 0;
		for (TuyaMCULE_dpConfig_t *cur = g_dpConfigs; cur; cur = cur->next) {
			byte dpLen = 0;
			switch (cur->dpType)
			{
				// case DP_TYPE_RAW: break; // TODO
				case DP_TYPE_BOOL: dpLen = 1; break;
				case DP_TYPE_VALUE: dpLen = 4; break;
				// case DP_TYPE_STRING: break; // TODO
				case DP_TYPE_ENUM: dpLen = 1; break;
				// case DP_TYPE_BITMAP: break; // TODO
				default: continue;
			}
			resp_len += 4 + dpLen;
			++dp_count;
		}

		resp = malloc(resp_len);
		resp[0] = 1;
		resp[1] = dp_count;

		byte *ptr = &resp[2];
		for (TuyaMCULE_dpConfig_t *cur = g_dpConfigs; cur; cur = cur->next) {
			byte dpLen = 0;
			switch (cur->dpType)
			{
				// case DP_TYPE_RAW: break; // TODO
				case DP_TYPE_BOOL: dpLen = 1; break;
				case DP_TYPE_VALUE: dpLen = 4; break;
				// case DP_TYPE_STRING: break; // TODO
				case DP_TYPE_ENUM: dpLen = 1; break;
				// case DP_TYPE_BITMAP: break; // TODO
				default: continue;
			}

			ptr[0] = cur->dpId;
			ptr[1] = cur->dpType;
			ptr[2] = 0;
			ptr[3] = dpLen;

			int value = CHANNEL_Get(cur->channel);

			switch (cur->dpType)
			{
				// case DP_TYPE_RAW: break; // TODO
				case DP_TYPE_BOOL: ptr[4] = value; break;
				case DP_TYPE_VALUE: ptr[4] = value >> 24; ptr[5] = value >> 16; ptr[6] = value >> 8; ptr[7] = value; break;
				// case DP_TYPE_STRING: break; // TODO
				case DP_TYPE_ENUM: ptr[4] = value; break;
				// case DP_TYPE_BITMAP: break; // TODO
				default: continue;
			}

			ptr += 4 + dpLen;
		}
	} else {
		byte dp_count = 0;
		for (int i = 0; i < dp_num; i++)
		{
			byte needDpId = data[1 + i];
			for (TuyaMCULE_dpConfig_t *cur = g_dpConfigs; cur; cur = cur->next) {
				if (cur->dpId != needDpId)
					continue;
				byte dpLen = 0;
				switch (cur->dpType)
				{
					// case DP_TYPE_RAW: break; // TODO
					case DP_TYPE_BOOL: dpLen = 1; break;
					case DP_TYPE_VALUE: dpLen = 4; break;
					// case DP_TYPE_STRING: break; // TODO
					case DP_TYPE_ENUM: dpLen = 1; break;
					// case DP_TYPE_BITMAP: break; // TODO
					default: break;
				}
				resp_len += 4 + dpLen;
				++dp_count;
				break;
			}
		}

		resp = malloc(resp_len);
		resp[0] = 1;
		resp[1] = dp_count;

		byte *ptr = &resp[2];
		for (int i = 0; i < dp_num; i++)
		{
			byte needDpId = data[1 + i];
			for (TuyaMCULE_dpConfig_t *cur = g_dpConfigs; cur; cur = cur->next) {
				if (cur->dpId != needDpId)
					continue;
				byte dpLen = 0;
				switch (cur->dpType)
				{
					// case DP_TYPE_RAW: break; // TODO
					case DP_TYPE_BOOL: dpLen = 1; break;
					case DP_TYPE_VALUE: dpLen = 4; break;
					// case DP_TYPE_STRING: break; // TODO
					case DP_TYPE_ENUM: dpLen = 1; break;
					// case DP_TYPE_BITMAP: break; // TODO
					default: continue;
				}

				ptr[0] = cur->dpId;
				ptr[1] = cur->dpType;
				ptr[2] = 0;
				ptr[3] = dpLen;

				int value = CHANNEL_Get(cur->channel);

				switch (cur->dpType)
				{
					// case DP_TYPE_RAW: break; // TODO
					case DP_TYPE_BOOL: ptr[4] = value; break;
					case DP_TYPE_VALUE: ptr[4] = value >> 24; ptr[5] = value >> 16; ptr[6] = value >> 8; ptr[7] = value; break;
					// case DP_TYPE_STRING: break; // TODO
					case DP_TYPE_ENUM: ptr[4] = value; break;
					// case DP_TYPE_BITMAP: break; // TODO
					default: continue;
				}

				ptr += 4 + dpLen;
				break;
			}
		}
	}

	if (!resp) {
		byte zero = 0;
		TuyaMCULE_SendCommand(TUYAMCULE_CMD_GET_DP_CACHE, &zero, 1);
	} else {
		TuyaMCULE_SendCommand(TUYAMCULE_CMD_GET_DP_CACHE, resp, resp_len);
		free(resp);
	}
}

void TuyaMCULE_ProcessIncoming(const byte *packet, int len) {
	if (packet[0] != 0x55 || packet[1] != 0xAA) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "ProcessIncoming: discarding packet with bad ident and len %i\n", len);
		return;
	}

	byte version = packet[2];
	if (version != 0) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "ProcessIncoming: Version not 0. Use TuyaMCU instead!\n");
		return;
	}

	int datalen = (packet[4] << 8) | packet[5];
	if (datalen + 2 + 1 + 1 + 2 + 1 != len) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "ProcessIncoming: discarding packet bad expected len, expected %i and got len %i\n", datalen + 2 + 1 + 1 + 2 + 1, len);
		return;
	}

	byte checkCheckSum = 0;
	for (int i = 0; i < len - 1; i++)
		checkCheckSum += packet[i];

	if (checkCheckSum != packet[len - 1]) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "ProcessIncoming: discarding packet bad expected checksum, expected %i and got checksum %i\n", (int)packet[len - 1], (int)checkCheckSum);
		return;
	}

	byte cmd = packet[3];
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ProcessIncoming[v=%i]: cmd %s (0x%x) len %i\n", version, TuyaMCULE_GetCommandTypeLabel(cmd), cmd, len);

	const byte *data = &packet[6];

	switch (cmd)
	{
		case TUYAMCULE_CMD_PRODUCT_INFO:
			TuyaMCULE_HandleProductInformation(data, datalen);
			break;

		case TUYAMCULE_CMD_WIFI_STATE:
			TuyaMCULE_HandleWifiState(data, datalen);
			break;

		case TUYAMCULE_CMD_WIFI_RESET:
			TuyaMCULE_HandleWifiReset(data, datalen);
			break;

#if 0
		case TUYAMCULE_CMD_WIFI_MODE:
			break;
#endif

		case TUYAMCULE_CMD_STATE:
			TuyaMCULE_HandleState(data, datalen);
			break;

		case TUYAMCULE_CMD_GET_LOCAL_TIME:
			TuyaMCULE_HandleGetLocalTime(data, datalen);
			break;

#if 0
		case TUYAMCULE_CMD_WIFI_TEST:
			break;
#endif

		case TUYAMCULE_CMD_STATE_RC:
			TuyaMCULE_HandleStateRC(data, datalen);
			break;

#if 0
		case TUYAMCULE_CMD_WIFI_UPDATE:
			break;
#endif

		case TUYAMCULE_CMD_GET_RSSI:
			TuyaMCULE_HandleGetRssi(data, datalen);
			break;

#if 0
		case TUYAMCULE_CMD_REQUEST_MCU_UG:
			break;

		case TUYAMCULE_CMD_UPDATE_START:
			break;

		case TUYAMCULE_CMD_UPDATE_TRANS: // ACK
			break;
#endif

		case TUYAMCULE_CMD_GET_DP_CACHE:
			TuyaMCULE_HandleGetDpCache(data, datalen);
			break;

		default:
			addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "ProcessIncoming: unhandled type %s (0x%x)", TuyaMCULE_GetCommandTypeLabel(cmd), cmd);
			break;
	}

	EventHandlers_FireEvent(CMD_EVENT_TUYAMCU_PARSED, cmd);
}

#define MIN_TUYAMCU_PACKET_SIZE (2+1+1+2+1)
int TuyaMCULE_GetPacketFromUart(byte* out, int maxSize) {
	char skippedBytes[256] = { 0 };
	int c_garbage_consumed = 0;
	while (1) {
		if (UART_GetDataSize() < 2)
			return 0;

		if (UART_GetByte(0) == 0x55 && UART_GetByte(1) == 0xAA)
			break;

		snprintf(skippedBytes, sizeof(skippedBytes), "%s%02X ", skippedBytes, UART_GetByte(0));
		UART_ConsumeBytes(1);
		c_garbage_consumed++;
	}

	if (c_garbage_consumed > 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "Consumed %i unwanted non-header byte in Tuya MCU buffer\n", c_garbage_consumed);
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "Skipped data (part) %s\n", skippedBytes);
	}

	if (UART_GetDataSize() < MIN_TUYAMCU_PACKET_SIZE)
		return 0;

	// byte version = UART_GetByte(2);
	// byte command = UART_GetByte(3);
	int len = ((UART_GetByte(4) << 8) | UART_GetByte(5)) + 2 + 1 + 1 + 2 + 1; // header 2 bytes, version, command, lenght, chekcusm
	if (len >= maxSize) {
		UART_ConsumeBytes(1);
		addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "TuyaMCU packet too large, %i > %i\n", len, maxSize);
		return 0;
	}

	if (UART_GetDataSize() < len)
		return 0;

	for (int i = 0; i < len; i++) {
		out[i] = UART_GetByte(i);
	}

	UART_ConsumeBytes(len);

	return len;
}

void TuyaMCULE_RunFrame() {
	byte packet[192];
	int len;
	while ((len = TuyaMCULE_GetPacketFromUart(packet, sizeof(packet))) > 0)
	{
		TuyaMCULE_PrintPacket(packet,len);
		TuyaMCULE_ProcessIncoming(packet, len);
	}
}

commandResult_t TuyaMCULE_SetBaudRate(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	g_baudRate = Tokenizer_GetArgInteger(0);
	UART_InitUART(g_baudRate, 0, false);

	return CMD_RES_OK;
}

commandResult_t TuyaMCULE_SetDefaultWiFiState(const void* context, const char* cmd, const char* args, int cmdFlags) {
	g_defaultTuyaMCUWiFiState = atoi(args);

	return CMD_RES_OK;
}

int TuyaMCULE_ParseDPType(const char *dpTypeString) {
	int dpType;
	if (!stricmp(dpTypeString, "bool")) {
		dpType = DP_TYPE_BOOL;
	}
	else if (!stricmp(dpTypeString, "val")) {
		dpType = DP_TYPE_VALUE;
	}
	else if (!stricmp(dpTypeString, "str")) {
		dpType = DP_TYPE_STRING;
	}
	else if (!stricmp(dpTypeString, "enum")) {
		dpType = DP_TYPE_ENUM;
	}
	else if (!stricmp(dpTypeString, "raw")) {
		dpType = DP_TYPE_RAW;
	}
	else {
		if (strIsInteger(dpTypeString)) {
			dpType = atoi(dpTypeString);
		}
		else {
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "%s is not a valid var type\n", dpTypeString);
			return DP_TYPE_VALUE;
		}
	}
	return dpType;
}

commandResult_t TuyaMCULE_SetDpConfig(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	byte dpId = Tokenizer_GetArgInteger(0);

	const char *dpTypeString = Tokenizer_GetArg(1);
	byte dpType = TuyaMCULE_ParseDPType(dpTypeString);

	short channel = Tokenizer_GetArgIntegerDefault(2, -999);

	TuyaMCULE_dpConfig_t *dpConfig = TuyaMCULE_getDPConfig(dpId);
	if (!dpConfig) {
		dpConfig = (TuyaMCULE_dpConfig_t *) malloc(sizeof(TuyaMCULE_dpConfig_t));
		dpConfig->dpId = dpId;
		dpConfig->next = g_dpConfigs;
		g_dpConfigs = dpConfig;
	}

	dpConfig->dpType = dpType;
	dpConfig->channel = channel;

	return CMD_RES_OK;
}

commandResult_t TuyaMCULE_SendCurSetTime(const void* context, const char* cmd, const char* args, int cmdFlags) {
	TuyaMCULE_SetLocalTime(gmtime(&g_ntpTime));

	return CMD_RES_OK;
}

commandResult_t TuyaMCULE_GetProductInformation(const void* context, const char* cmd, const char* args, int cmdFlags) {
	TuyaMCULE_GetProductInfo();

	return CMD_RES_OK;
}

commandResult_t TuyaMCULE_SendUserCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	byte packet[128];
	int c = 0;

	Tokenizer_TokenizeString(args, 0);

	int command = Tokenizer_GetArgInteger(0);

	if (Tokenizer_GetArgsCount() >= 2) {
		const char* s = Tokenizer_GetArg(1);
		if (s) {
			while (*s) {
				byte b;
				b = CMD_ParseOrExpandHexByte(&s);

				if (sizeof(packet) > c + 1) {
					packet[c] = b;
					c++;
				}
			}
		}
	}

	TuyaMCULE_SendCommand(command,packet, c);

	return CMD_RES_OK;
}

commandResult_t TuyaMCULE_SendRSSI(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int toSend;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 1) {
		toSend = HAL_GetWifiStrength();
	}
	else {
		toSend = Tokenizer_GetArgInteger(0);
	}

	TuyaMCULE_SetRSSI(toSend);

	return CMD_RES_OK;
}

void TuyaMCULE_Init() {
	UART_InitUART(g_baudRate, 0, false);
	UART_InitReceiveRingBuffer(1024);

	//cmddetail:{"name":"tuyaMcuLE_setBaudRate","args":"[BaudValue]",
	//cmddetail:"descr":"Sets the baud rate used by TuyaMCU UART communication. Default value is 9600. Some other devices require 115200.",
	//cmddetail:"fn":"TuyaMCULE_SetBaudRate","file":"driver/drv_tuyaMCULE.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcuLE_setBaudRate", TuyaMCULE_SetBaudRate, NULL);

	//cmddetail:{"name":"tuyaMcuLE_defWiFiState","args":"",
	//cmddetail:"descr":"Command sets the default WiFi state for TuyaMCU when device is not online. It may be required for some devices to work, because Tuya designs them to ignore touch buttons or beep when not paired. Please see [values table and description here](https://www.elektroda.com/rtvforum/viewtopic.php?p=20483899#20483899).",
	//cmddetail:"fn":"TuyaMCULE_SetDefaultWiFiState","file":"driver/drv_tuyaMCULE.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcuLE_defWiFiState", TuyaMCULE_SetDefaultWiFiState, NULL);

	//cmddetail:{"name":"linkTuyaMCUOutputToChannel","args":"[dpId][varType][channelID]",
	//cmddetail:"descr":"Used to map between TuyaMCU dpIDs and our internal channels. Mult, inverse and delta are for calibration, they are optional. bDPCache is also optional, you can set it to 1 for battery powered devices, so a variable is set with DPCache, for example a sampling interval for humidity/temperature sensor. Mapping works both ways. DpIDs are per-device, you can get them by sniffing UART communication. Vartypes can also be sniffed from Tuya. VarTypes can be following: 0-raw, 1-bool, 2-value, 3-string, 4-enum, 5-bitmap. Please see [Tuya Docs](https://developer.tuya.com/en/docs/iot/tuya-cloud-universal-serial-port-access-protocol?id=K9hhi0xxtn9cb) for info about TuyaMCU. You can also see our [TuyaMCU Analyzer Tool](https://www.elektroda.com/rtvforum/viewtopic.php?p=20528459#20528459)",
	//cmddetail:"fn":"TuyaMCULE_SetDpConfig","file":"driver/drv_tuyaMCULE.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMCULE_SetDpConfig", TuyaMCULE_SetDpConfig, NULL);

	//cmddetail:{"name":"tuyaMcuLE_sendCurTime","args":"",
	//cmddetail:"descr":"Sends a current date by TuyaMCU to clock/callendar MCU. Time is taken from NTP driver, so NTP also should be already running.",
	//cmddetail:"fn":"TuyaMCULE_SendCurSetTime","file":"driver/drv_tuyaMCULE.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcuLE_sendCurTime", TuyaMCULE_SendCurSetTime, NULL);

	//cmddetail:{"name":"tuyaMcuLE_getProductInformation","args":"",
	//cmddetail:"descr":"Send query packet (0x01). No arguments needed.",
	//cmddetail:"fn":"TuyaMCULE_GetProductInformation","file":"driver/drv_tuyaMCULE.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcuLE_getProductInformation", TuyaMCULE_GetProductInformation, NULL);

	//cmddetail:{"name":"tuyaMcuLE_sendCmd","args":"[CommandIndex] [HexPayloadNBytes]",
	//cmddetail:"descr":"This will automatically calculate TuyaMCU checksum and length for given command ID and payload, then it will send a command. It's better to use it than uartSendHex",
	//cmddetail:"fn":"TuyaMCULE_SendUserCmd","file":"driver/drv_tuyaMCULE.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcuLE_sendCmd", TuyaMCULE_SendUserCmd, NULL);

	//cmddetail:{"name":"tuyaMcuLE_sendRSSI","args":"",
	//cmddetail:"descr":"Command sends the specific RSSI value to TuyaMCU (it will send current RSSI if no argument is set)",
	//cmddetail:"fn":"TuyaMCULE_SendRSSI","file":"driver/drv_tuyaMCULE.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcuLE_sendRSSI", TuyaMCULE_SendRSSI, NULL);
}

void TuyaMCULE_Shutdown() {
	g_baudRate = 9600;

	g_wifiReset = false;
	g_tuyaNextRequestDelay;
	g_tuyaBatteryPoweredState = TUYAMCULE_STATE_AWAITING_INFO;
	g_defaultTuyaMCUWiFiState = TUYAMCULE_NETWORK_STATUS_SMART_CONNECT_SETUP;

	TuyaMCULE_dpConfig_t *tmp = g_dpConfigs;
	while (tmp) {
		TuyaMCULE_dpConfig_t *nxt = tmp->next;
		free(tmp);
		tmp = nxt;
	}
	g_dpConfigs = NULL;

	g_HandleStateRcCbEpectedCount = 0;
}
