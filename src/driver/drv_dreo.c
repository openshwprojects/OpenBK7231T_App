// Dreo Heater UART Protocol Driver for OpenBeken
// Robust version - simplified packet parser with large linear buffer

#include "../obk_config.h"
#include "../logging/logging.h"
#include "../new_common.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../httpserver/new_http.h"
#include "drv_uart.h"

// Dreo DP types (same as Tuya standard)
#define DREO_DP_TYPE_RAW    0x00
#define DREO_DP_TYPE_BOOL   0x01
#define DREO_DP_TYPE_VALUE  0x02
#define DREO_DP_TYPE_STRING 0x03
#define DREO_DP_TYPE_ENUM   0x04

// Dreo command IDs
#define DREO_CMD_HEARTBEAT     0x00
#define DREO_CMD_QUERY_PRODUCT 0x01
#define DREO_CMD_MCU_CONF      0x02
#define DREO_CMD_WIFI_STATE    0x03
#define DREO_CMD_SET_DP        0x06
#define DREO_CMD_STATE         0x07
#define DREO_CMD_QUERY_STATE   0x08

// Mapping structure
typedef struct dreoMapping_s {
	byte dpId;
	byte dpType;
	short channel;
	int lastValue;
	byte hasData;
	struct dreoMapping_s *next;
} dreoMapping_t;

static dreoMapping_t *g_dreoMappings = NULL;
static byte g_dreoSeq = 0;
static int g_dreoHeartbeatTimer = 0;
static int g_dreoInitState = 0;
static int g_dreoInitTimer = 0;
static int g_dreoBytesSent = 0;
static int g_dreoBytesReceived = 0;
static int g_dreoBytesInvalid = 0;

// -----------------------------------------------------------------------
// Robust linear receive buffer (4096 bytes - enough for many back-to-back packets)
static byte g_dreoRecvBuf[4096];
static int  g_dreoRecvLen = 0;

// -----------------------------------------------------------------------
// Mapping Management (unchanged)
static dreoMapping_t *Dreo_FindByDPId(int dpId) {
	dreoMapping_t *cur = g_dreoMappings;
	while (cur) {
		if (cur->dpId == dpId) return cur;
		cur = cur->next;
	}
	return NULL;
}

static dreoMapping_t *Dreo_FindByChannel(int channel) {
	dreoMapping_t *cur = g_dreoMappings;
	while (cur) {
		if (cur->channel == channel) return cur;
		cur = cur->next;
	}
	return NULL;
}

static dreoMapping_t *Dreo_MapDPToChannel(int dpId, int dpType, int channel) {
	dreoMapping_t *cur = Dreo_FindByDPId(dpId);
	if (cur == NULL) {
		cur = (dreoMapping_t *)malloc(sizeof(dreoMapping_t));
		memset(cur, 0, sizeof(dreoMapping_t));
		cur->next = g_dreoMappings;
		g_dreoMappings = cur;
	}
	cur->dpId = dpId;
	cur->dpType = dpType;
	cur->channel = channel;
	cur->lastValue = 0;
	cur->hasData = 0;
	return cur;
}

static dreoMapping_t *Dreo_AutoStore(int dpId, int dpType) {
	dreoMapping_t *cur = Dreo_FindByDPId(dpId);
	if (cur == NULL) {
		cur = (dreoMapping_t *)malloc(sizeof(dreoMapping_t));
		memset(cur, 0, sizeof(dreoMapping_t));
		cur->dpId = dpId;
		cur->dpType = dpType;
		cur->channel = -1;
		cur->lastValue = 0;
		cur->hasData = 0;
		cur->next = g_dreoMappings;
		g_dreoMappings = cur;
	}
	return cur;
}

// -----------------------------------------------------------------------
// Send functions (unchanged)
static void Dreo_SendRaw(byte cmd, const byte *payload, int payloadLen) {
	int i;
	uint32_t sum = 0;

	UART_SendByte(0x55);
	UART_SendByte(0xAA);
	byte ver = 0x00; UART_SendByte(ver); sum += ver;
	byte seq = g_dreoSeq++; UART_SendByte(seq); sum += seq;
	UART_SendByte(cmd); sum += cmd;
	byte reserved = 0x00; UART_SendByte(reserved); sum += reserved;
	byte lenH = (payloadLen >> 8) & 0xFF; UART_SendByte(lenH); sum += lenH;
	byte lenL = payloadLen & 0xFF; UART_SendByte(lenL); sum += lenL;

	for (i = 0; i < payloadLen; i++) {
		UART_SendByte(payload[i]);
		sum += payload[i];
	}

	byte checksum = (byte)((sum - 1) & 0xFF);
	UART_SendByte(checksum);

	g_dreoBytesSent += 8 + payloadLen + 1;
	addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Dreo: sent cmd 0x%02X, %i payload bytes", cmd, payloadLen);
}

static void Dreo_SendDP(byte dpId, byte dpType, const byte *value, int valueLen) {
	byte data[64];
	int idx = 0;
	data[idx++] = dpId;
	data[idx++] = 0x01;
	data[idx++] = dpType;
	data[idx++] = (valueLen >> 8) & 0xFF;
	data[idx++] = valueLen & 0xFF;
	for (int i = 0; i < valueLen && idx < (int)sizeof(data); i++) {
		data[idx++] = value[i];
	}
	Dreo_SendRaw(DREO_CMD_SET_DP, data, idx);
}

static void Dreo_SendBool(byte dpId, int value) { byte v = value ? 1 : 0; Dreo_SendDP(dpId, DREO_DP_TYPE_BOOL, &v, 1); }
static void Dreo_SendValue(byte dpId, uint32_t value) {
	byte v[4]; v[0] = (value >> 24) & 0xFF; v[1] = (value >> 16) & 0xFF; v[2] = (value >> 8) & 0xFF; v[3] = value & 0xFF;
	Dreo_SendDP(dpId, DREO_DP_TYPE_VALUE, v, 4);
}
static void Dreo_SendEnum(byte dpId, uint32_t value) { byte v = value & 0xFF; Dreo_SendDP(dpId, DREO_DP_TYPE_ENUM, &v, 1); }

// -----------------------------------------------------------------------
// ROBUST PACKET PARSER (this is the important change)
static void Dreo_ProcessDPStatus(const byte *data, int startOfs, int dataLen);
static void Dreo_ProcessPacket(const byte *data, int len);

static int Dreo_TryGetPacket() {
	int cs = UART_GetDataSize();
	if (cs == 0 && g_dreoRecvLen == 0) return 0;

	// Append all new bytes
	while (cs > 0 && g_dreoRecvLen < (int)sizeof(g_dreoRecvBuf)) {
		g_dreoRecvBuf[g_dreoRecvLen++] = UART_GetByte(0);
		UART_ConsumeBytes(1);
		cs--;
	}

	// Process as many complete valid packets as possible from the front
	int ofs = 0;
	while (ofs + 9 <= g_dreoRecvLen) {   // minimum packet size = 9 bytes
		if (g_dreoRecvBuf[ofs] != 0x55 || g_dreoRecvBuf[ofs + 1] != 0xAA) {
			ofs++;   // skip garbage
			continue;
		}

		int payloadLen = ((int)g_dreoRecvBuf[ofs + 6] << 8) | g_dreoRecvBuf[ofs + 7];
		int packetLen = 8 + payloadLen + 1;

		if (ofs + packetLen > g_dreoRecvLen) break;   // not enough data yet

		// Verify checksum
		uint32_t sum = 0;
		for (int i = 2; i < ofs + packetLen - 1; i++) {
			sum += g_dreoRecvBuf[i];
		}
		byte expected = (byte)((sum - 1) & 0xFF);

		if (g_dreoRecvBuf[ofs + packetLen - 1] == expected) {
			// VALID PACKET
			g_dreoBytesReceived += packetLen;
			addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL,
				"Dreo: received cmd=0x%02X payload=%i bytes (valid packet)", g_dreoRecvBuf[ofs+4], payloadLen);

			Dreo_ProcessPacket(g_dreoRecvBuf + ofs, packetLen);

			// Remove this packet from the front of the buffer
			int remaining = g_dreoRecvLen - (ofs + packetLen);
			if (remaining > 0) {
				memmove(g_dreoRecvBuf, g_dreoRecvBuf + ofs + packetLen, remaining);
			}
			g_dreoRecvLen = remaining;
			ofs = 0;   // start scanning again from the new front
			continue;
		}

		// Bad checksum - treat as garbage
		ofs++;
	}

	// If we have too much garbage at the front, drop the oldest 512 bytes
	if (g_dreoRecvLen > 3000) {
		memmove(g_dreoRecvBuf, g_dreoRecvBuf + 512, g_dreoRecvLen - 512);
		g_dreoRecvLen -= 512;
		g_dreoBytesInvalid += 512;
	}

	return 0;
}

static void Dreo_ProcessDPStatus(const byte *data, int startOfs, int dataLen) {
	int idx = startOfs;
	int endIdx = startOfs + dataLen;

	while (idx + 5 <= endIdx) {
		byte dpId = data[idx];
		byte dpType = data[idx + 2];
		int dpLen = (data[idx + 3] << 8) | data[idx + 4];

		if (idx + 5 + dpLen > endIdx) break;

		int val = 0;
		if (dpLen == 1) val = data[idx + 5];
		else if (dpLen == 4) val = ((uint32_t)data[idx + 5] << 24) | ((uint32_t)data[idx + 6] << 16) |
		                           ((uint32_t)data[idx + 7] << 8) | data[idx + 8];
		else {
			for (int i = 0; i < dpLen; i++) val = (val << 8) | data[idx + 5 + i];
		}

		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: DP id=%i type=%i len=%i val=%i", dpId, dpType, dpLen, val);

		dreoMapping_t *mapping = Dreo_AutoStore(dpId, dpType);
		mapping->lastValue = val;
		mapping->hasData = 1;

		if (mapping->channel >= 0) {
			CHANNEL_Set(mapping->channel, val, 0);
		}

		idx += 5 + dpLen;
	}
}

static void Dreo_ProcessPacket(const byte *data, int len) {
	byte cmd = data[4];
	int payloadLen = (data[6] << 8) | data[7];

	addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Dreo: received cmd=0x%02X payload=%i bytes", cmd, payloadLen);

	switch (cmd) {
	case DREO_CMD_HEARTBEAT:
		addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Dreo: heartbeat ACK");
		break;
	case DREO_CMD_QUERY_PRODUCT:
	case DREO_CMD_MCU_CONF:
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: product/MCU response");
		break;
	case 0x0E:
		addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Dreo: received unknown cmd 0x0E");
		break;
	case DREO_CMD_STATE:
	case DREO_CMD_QUERY_STATE:
		if (payloadLen > 0) Dreo_ProcessDPStatus(data, 8, payloadLen);
		break;
	default:
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: unknown cmd 0x%02X", cmd);
		break;
	}
}

// -----------------------------------------------------------------------
// Rest of the driver (unchanged except RunFrame)
static int Dreo_ParseDPType(const char *typeStr) {
	if (!stricmp(typeStr, "bool")) return DREO_DP_TYPE_BOOL;
	if (!stricmp(typeStr, "val"))  return DREO_DP_TYPE_VALUE;
	if (!stricmp(typeStr, "enum")) return DREO_DP_TYPE_ENUM;
	if (!stricmp(typeStr, "str"))  return DREO_DP_TYPE_STRING;
	if (!stricmp(typeStr, "raw"))  return DREO_DP_TYPE_RAW;
	if (strIsInteger(typeStr)) return atoi(typeStr);
	return DREO_DP_TYPE_VALUE;
}

static commandResult_t Dreo_LinkOutputToChannel(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int dpId, dpType, channelID = -1;
	const char *dpTypeString;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;

	dpId = Tokenizer_GetArgInteger(0);
	dpTypeString = Tokenizer_GetArg(1);
	dpType = Dreo_ParseDPType(dpTypeString);
	if (Tokenizer_GetArgsCount() >= 3) channelID = Tokenizer_GetArgInteger(2);

	Dreo_MapDPToChannel(dpId, dpType, channelID);
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: mapped dpId %i (type %i) -> channel %i", dpId, dpType, channelID);
	return CMD_RES_OK;
}

static commandResult_t Dreo_SendStateCmd(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int dpId, dpType, value;
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3)) return CMD_RES_NOT_ENOUGH_ARGUMENTS;

	dpId = Tokenizer_GetArgInteger(0);
	dpType = Dreo_ParseDPType(Tokenizer_GetArg(1));
	value = Tokenizer_GetArgInteger(2);

	switch (dpType) {
	case DREO_DP_TYPE_BOOL: Dreo_SendBool(dpId, value); break;
	case DREO_DP_TYPE_VALUE: Dreo_SendValue(dpId, value); break;
	case DREO_DP_TYPE_ENUM: Dreo_SendEnum(dpId, value); break;
	default: Dreo_SendEnum(dpId, value); break;
	}
	return CMD_RES_OK;
}

void Dreo_Init(void) {
	g_dreoSeq = 0;
	g_dreoHeartbeatTimer = 0;
	g_dreoInitState = 0;
	g_dreoInitTimer = 0;
	g_dreoRecvLen = 0;

	UART_InitUART(115200, 0, false);
	UART_InitReceiveRingBuffer(1024);

	CMD_RegisterCommand("linkDreoOutputToChannel", Dreo_LinkOutputToChannel, NULL);
	CMD_RegisterCommand("dreo_sendState", Dreo_SendStateCmd, NULL);

	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: driver initialized (115200 baud)");
}

void Dreo_Shutdown(void) {
	dreoMapping_t *cur = g_dreoMappings;
	while (cur) {
		dreoMapping_t *nxt = cur->next;
		free(cur);
		cur = nxt;
	}
	g_dreoMappings = NULL;
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: driver shut down");
}

void Dreo_RunEverySecond(void) {
	g_dreoHeartbeatTimer++;
	if (g_dreoHeartbeatTimer >= 10) {
		Dreo_SendRaw(DREO_CMD_HEARTBEAT, NULL, 0);
		g_dreoHeartbeatTimer = 0;
	}

	if (g_dreoInitState < 3) {
		g_dreoInitTimer++;
		switch (g_dreoInitState) {
		case 0: Dreo_SendRaw(DREO_CMD_HEARTBEAT, NULL, 0); g_dreoInitState = 1; break;
		case 1: if (g_dreoInitTimer >= 1) { byte initData[] = {0x02,0x05,0x00}; Dreo_SendRaw(DREO_CMD_WIFI_STATE, initData, sizeof(initData)); g_dreoInitState = 2; } break;
		case 2: if (g_dreoInitTimer >= 2) { Dreo_SendRaw(DREO_CMD_MCU_CONF, NULL, 0); g_dreoInitState = 3; } break;
		}
	}
}

void Dreo_RunFrame(void) {
	while (1) {
		if (Dreo_TryGetPacket() == 0) break;
	}
}

void Dreo_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
	if (bPreState) return;
	hprintf255(request, "<h5>Dreo Heater (sent: %i, recv: %i, invalid: %i bytes)</h5>", g_dreoBytesSent, g_dreoBytesReceived, g_dreoBytesInvalid);
	poststr(request, "<table border=1 style='border-collapse:collapse;'>");
	poststr(request, "<tr><th>dpID</th><th>Type</th><th>Value</th><th>Channel</th></tr>");

	dreoMapping_t *cur = g_dreoMappings;
	while (cur) {
		const char *typeName = "?";
		switch (cur->dpType) {
		case DREO_DP_TYPE_BOOL:   typeName = "bool"; break;
		case DREO_DP_TYPE_VALUE:  typeName = "val";  break;
		case DREO_DP_TYPE_ENUM:   typeName = "enum"; break;
		case DREO_DP_TYPE_STRING: typeName = "str";  break;
		case DREO_DP_TYPE_RAW:    typeName = "raw";  break;
		}
		if (cur->hasData) {
			hprintf255(request, "<tr><td>%i</td><td>%s</td><td>%i</td><td>%s</td></tr>",
				cur->dpId, typeName, cur->lastValue, cur->channel >= 0 ? "CH" : "none");
		} else {
			hprintf255(request, "<tr><td>%i</td><td>%s</td><td>n/a</td><td>%s</td></tr>",
				cur->dpId, typeName, cur->channel >= 0 ? "CH" : "none");
		}
		cur = cur->next;
	}
	poststr(request, "</table>");
}

void Dreo_OnChannelChanged(int ch, int value) {
	dreoMapping_t *mapping = Dreo_FindByChannel(ch);
	if (mapping == NULL) return;
	if (mapping->lastValue == value) return;

	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: channel %i changed to %i, sending dpId %i", ch, value, mapping->dpId);

	switch (mapping->dpType) {
	case DREO_DP_TYPE_BOOL: Dreo_SendBool(mapping->dpId, value); break;
	case DREO_DP_TYPE_VALUE: Dreo_SendValue(mapping->dpId, value); break;
	case DREO_DP_TYPE_ENUM: Dreo_SendEnum(mapping->dpId, value); break;
	default: Dreo_SendEnum(mapping->dpId, value); break;
	}
	mapping->lastValue = value;
}

void Dreo_OnHassDiscovery(const char *topic) {}