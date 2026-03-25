// Dreo Heater UART Protocol Driver for OpenBeken
// Based on ESPHome dreo_heater.h reference implementation
// Uses modified Tuya UART protocol with custom checksum
//
// Protocol format:
//   [55 AA] [ver] [seq] [cmd] [00] [lenH] [lenL] [payload] [checksum]
//   Checksum: (sum(bytes[2..end-1]) - 1) & 0xFF
//
// DP payload format (same as standard Tuya):
//   [dpId] [dpType:0x01] [type] [lenH] [lenL] [value...]
//
// Known dpIDs from Dreo heater:
//   1  = Power (bool)          2  = Mode (enum: 1=Manual,2=Eco,3=Fan)
//   3  = Heat Level (enum 1-3) 4  = Target Temp (value)
//   6  = Sound (bool, inv)     7  = Current Temp (value, read-only)
//   8  = Screen Display (bool) 9  = Timer (value, 4 bytes)
//   15 = Calibration (value)   16 = Child Lock (bool)
//   17 = Temp Unit Alias       19 = Heating Status (bool, read-only)
//   20 = Window Detection      22 = Temp Unit (enum: 1=F, 2=C)

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

// Mapping structure: links a Dreo dpID to an OBK channel
typedef struct dreoMapping_s {
	byte dpId;
	byte dpType;
	short channel;   // -1 = no channel mapping (autostore only)
	int lastValue;   // last received value
	byte hasData;     // set to 1 once we receive data for this dpId
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
// Mapping Management
// -----------------------------------------------------------------------

static dreoMapping_t *Dreo_FindByDPId(int dpId) {
	dreoMapping_t *cur = g_dreoMappings;
	while (cur) {
		if (cur->dpId == dpId)
			return cur;
		cur = cur->next;
	}
	return NULL;
}

static dreoMapping_t *Dreo_FindByChannel(int channel) {
	dreoMapping_t *cur = g_dreoMappings;
	while (cur) {
		if (cur->channel == channel)
			return cur;
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

// Auto-store: create a mapping entry if one doesn't exist (no channel, just for display)
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
// Dreo UART Packet Send
// -----------------------------------------------------------------------

// Send a raw Dreo packet:  55 AA [ver=0x00] [seq] [cmd] [0x00] [lenH] [lenL] [payload] [checksum]
// Checksum = (sum(bytes from seq to last payload byte) - 1) & 0xFF
static void Dreo_SendRaw(byte cmd, const byte *payload, int payloadLen) {
	int i;
	uint32_t sum = 0;

	UART_SendByte(0x55);
	UART_SendByte(0xAA);
	UART_SendByte(0x00);           // version

	byte seq = g_dreoSeq++;
	UART_SendByte(seq);            // sequence
	sum += seq;

	UART_SendByte(cmd);            // command
	UART_SendByte(0x00);           // reserved zero

	byte lenH = (payloadLen >> 8) & 0xFF;
	byte lenL = payloadLen & 0xFF;
	UART_SendByte(lenH);           // length high
	UART_SendByte(lenL);           // length low

	for (i = 0; i < payloadLen; i++) {
		UART_SendByte(payload[i]);
		sum += payload[i];
	}

	byte checksum = (byte)((sum - 1) & 0xFF);
	UART_SendByte(checksum);

	g_dreoBytesSent += 8 + payloadLen + 1;  // header(8) + payload + checksum(1)

	addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Dreo: sent cmd 0x%02X, %i payload bytes", cmd, payloadLen);
}

// Send a DP value to the Dreo MCU
// DP payload: [dpId] [0x01] [type] [lenH] [lenL] [value...]
static void Dreo_SendDP(byte dpId, byte dpType, const byte *value, int valueLen) {
	byte data[64];
	int idx = 0;

	data[idx++] = dpId;
	data[idx++] = 0x01;               // protocol byte (always 0x01 in Dreo)
	data[idx++] = dpType;
	data[idx++] = (valueLen >> 8) & 0xFF;
	data[idx++] = valueLen & 0xFF;

	for (int i = 0; i < valueLen && idx < (int)sizeof(data); i++) {
		data[idx++] = value[i];
	}

	Dreo_SendRaw(DREO_CMD_SET_DP, data, idx);
}

static void Dreo_SendBool(byte dpId, int value) {
	byte v = value ? 1 : 0;
	Dreo_SendDP(dpId, DREO_DP_TYPE_BOOL, &v, 1);
}

static void Dreo_SendValue(byte dpId, uint32_t value) {
	byte v[4];
	v[0] = (value >> 24) & 0xFF;
	v[1] = (value >> 16) & 0xFF;
	v[2] = (value >> 8) & 0xFF;
	v[3] = value & 0xFF;
	Dreo_SendDP(dpId, DREO_DP_TYPE_VALUE, v, 4);
}

static void Dreo_SendEnum(byte dpId, uint32_t value) {
	byte v = value & 0xFF;
	Dreo_SendDP(dpId, DREO_DP_TYPE_ENUM, &v, 1);
}

// -----------------------------------------------------------------------
// Dreo UART Packet Receive
// -----------------------------------------------------------------------

// Dreo packet format: 55 AA [ver] [seq] [cmd] [00] [lenH] [lenL] [payload...] [checksum]
// Minimum packet: 8 header + 1 checksum = 9 bytes
#define DREO_MIN_PACKET_SIZE 9

// Try to extract the next complete Dreo packet from the UART ring buffer.
// Returns total packet length if a valid packet was extracted into 'out', or 0 if not ready.
static int Dreo_TryGetPacket(byte *out, int maxSize) {
	int cs;
	int c_garbage = 0;

	cs = UART_GetDataSize();
	if (cs < DREO_MIN_PACKET_SIZE)
		return 0;

	// Skip non-header bytes
	while (cs > 1) {
		if (UART_GetByte(0) == 0x55 && UART_GetByte(1) == 0xAA)
			break;
		UART_ConsumeBytes(1);
		c_garbage++;
		cs--;
	}
	if (c_garbage > 0) {
		g_dreoBytesInvalid += c_garbage;
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: skipped %i garbage bytes", c_garbage);
	}
	if (cs < DREO_MIN_PACKET_SIZE)
		return 0;

	// Verify header
	if (UART_GetByte(0) != 0x55 || UART_GetByte(1) != 0xAA)
		return 0;

	// Parse lengths: bytes [6] and [7] are lenH and lenL
	byte lenH = UART_GetByte(6);
	byte lenL = UART_GetByte(7);
	int payloadLen = (lenH << 8) | lenL;
	int packetLen = 8 + payloadLen + 1;  // header(8) + payload + checksum(1)

	if (cs < packetLen)
		return 0;  // incomplete

	// Verify checksum: sum = seq + sum(payload bytes), then (sum-1)&0xFF
	uint32_t calcSum = UART_GetByte(3);           // seq byte
	for (int i = 8; i < packetLen - 1; i++) {     // payload starts at index 8
		calcSum += UART_GetByte(i);
	}
	byte expectedChecksum = (byte)((calcSum - 1) & 0xFF);
	byte receivedChecksum = UART_GetByte(packetLen - 1);

	if (receivedChecksum != expectedChecksum) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
			"Dreo: checksum mismatch, got 0x%02X expected 0x%02X (seq=0x%02X), dropping packet",
			receivedChecksum, expectedChecksum, UART_GetByte(3));
		g_dreoBytesInvalid++;
		UART_ConsumeBytes(packetLen);   // consume whole bad packet to avoid desync
		return 0;
	}

	// Valid packet — copy to output buffer
	if (packetLen > maxSize) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: packet too large (%i > %i)", packetLen, maxSize);
		UART_ConsumeBytes(packetLen);
		return 0;
	}

	for (int i = 0; i < packetLen; i++) {
		out[i] = UART_GetByte(i);
	}
	UART_ConsumeBytes(packetLen);
	g_dreoBytesReceived += packetLen;
	return packetLen;
}

// -----------------------------------------------------------------------
// DP Status Processing
// -----------------------------------------------------------------------

// Process DP status data from within a received packet.
// The DP data starts at 'data + startOfs' and spans 'dataLen' bytes.
// DP format: [dpId] [dpType:0x01] [type] [lenH] [lenL] [value...]
static void Dreo_ProcessDPStatus(const byte *data, int startOfs, int dataLen) {
	int idx = startOfs;
	int endIdx = startOfs + dataLen;

	while (idx + 5 <= endIdx) {
		byte dpId = data[idx];
		// data[idx+1] is always 0x01 (protocol byte)
		byte dpType = data[idx + 2];
		int dpLen = (data[idx + 3] << 8) | data[idx + 4];

		if (idx + 5 + dpLen > endIdx) {
			addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
				"Dreo: truncated DP at id=%i, need %i bytes", dpId, dpLen);
			break;
		}

		// Extract value
		int val = 0;
		if (dpLen == 1) {
			val = data[idx + 5];
		} else if (dpLen == 4) {
			val = ((uint32_t)data[idx + 5] << 24) |
				  ((uint32_t)data[idx + 6] << 16) |
				  ((uint32_t)data[idx + 7] << 8) |
				  ((uint32_t)data[idx + 8]);
		} else {
			// generic: treat as big-endian integer
			for (int i = 0; i < dpLen; i++) {
				val = (val << 8) | data[idx + 5 + i];
			}
		}

		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
			"Dreo: DP id=%i type=%i len=%i val=%i", dpId, dpType, dpLen, val);

		// Auto-store: ensure a mapping entry exists for this dpId
		dreoMapping_t *mapping = Dreo_AutoStore(dpId, dpType);
		mapping->lastValue = val;
		mapping->hasData = 1;

		// If this dpId is mapped to a channel, set it
		if (mapping->channel >= 0) {
			CHANNEL_Set(mapping->channel, val, 0);
		}

		idx += 5 + dpLen;
	}
}

// Process a complete received Dreo packet
static void Dreo_ProcessPacket(const byte *data, int len) {
	byte cmd = data[4];
	int payloadLen = (data[6] << 8) | data[7];

	addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL,
		"Dreo: received cmd=0x%02X payload=%i bytes", cmd, payloadLen);

	switch (cmd) {
	case DREO_CMD_HEARTBEAT:
		addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Dreo: heartbeat ACK");
		break;

	case DREO_CMD_QUERY_PRODUCT:
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: product info response");
		break;

	case DREO_CMD_MCU_CONF:
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: MCU conf response");
		break;

	case DREO_CMD_STATE:
	case DREO_CMD_QUERY_STATE:
		// DP status data starts at byte 8 (after the 8-byte header)
		if (payloadLen > 0) {
			Dreo_ProcessDPStatus(data, 8, payloadLen);
		}
		break;

	default:
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
			"Dreo: unknown cmd 0x%02X", cmd);
		break;
	}
}

// -----------------------------------------------------------------------
// Command: linkDreoOutputToChannel
// -----------------------------------------------------------------------

static int Dreo_ParseDPType(const char *typeStr) {
	if (!stricmp(typeStr, "bool")) return DREO_DP_TYPE_BOOL;
	if (!stricmp(typeStr, "val"))  return DREO_DP_TYPE_VALUE;
	if (!stricmp(typeStr, "enum")) return DREO_DP_TYPE_ENUM;
	if (!stricmp(typeStr, "str"))  return DREO_DP_TYPE_STRING;
	if (!stricmp(typeStr, "raw"))  return DREO_DP_TYPE_RAW;
	// numeric fallback
	if (strIsInteger(typeStr))
		return atoi(typeStr);
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: '%s' is not a valid var type", typeStr);
	return DREO_DP_TYPE_VALUE;
}

// linkDreoOutputToChannel [dpId] [varType] [channelID]
static commandResult_t Dreo_LinkOutputToChannel(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int dpId, dpType, channelID;
	const char *dpTypeString;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	dpId = Tokenizer_GetArgInteger(0);
	dpTypeString = Tokenizer_GetArg(1);
	dpType = Dreo_ParseDPType(dpTypeString);

	if (Tokenizer_GetArgsCount() < 3) {
		channelID = -1;
	} else {
		channelID = Tokenizer_GetArgInteger(2);
	}

	Dreo_MapDPToChannel(dpId, dpType, channelID);

	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
		"Dreo: mapped dpId %i (type %i) -> channel %i", dpId, dpType, channelID);

	return CMD_RES_OK;
}

// dreo_sendState [dpId] [dpType] [value]
static commandResult_t Dreo_SendStateCmd(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int dpId, dpType, value;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	dpId = Tokenizer_GetArgInteger(0);
	dpType = Dreo_ParseDPType(Tokenizer_GetArg(1));
	value = Tokenizer_GetArgInteger(2);

	switch (dpType) {
	case DREO_DP_TYPE_BOOL:
		Dreo_SendBool(dpId, value);
		break;
	case DREO_DP_TYPE_VALUE:
		Dreo_SendValue(dpId, value);
		break;
	case DREO_DP_TYPE_ENUM:
		Dreo_SendEnum(dpId, value);
		break;
	default:
		Dreo_SendEnum(dpId, value);
		break;
	}

	return CMD_RES_OK;
}

// -----------------------------------------------------------------------
// Driver Entry Points
// -----------------------------------------------------------------------

void Dreo_Init(void) {
	g_dreoSeq = 0;
	g_dreoHeartbeatTimer = 0;
	g_dreoInitState = 0;
	g_dreoInitTimer = 0;

	UART_InitUART(115200, 0, false);
	UART_InitReceiveRingBuffer(512);

	//cmddetail:{"name":"linkDreoOutputToChannel","args":"[dpId][varType][channelID]",
	//cmddetail:"descr":"Map a Dreo dpId to an OBK channel. VarTypes: bool, val, enum, str, raw. Syntax is same as linkTuyaMCUOutputToChannel.",
	//cmddetail:"fn":"Dreo_LinkOutputToChannel","file":"driver/drv_dreo.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("linkDreoOutputToChannel", Dreo_LinkOutputToChannel, NULL);

	//cmddetail:{"name":"dreo_sendState","args":"[dpId][dpType][value]",
	//cmddetail:"descr":"Send a DP value to the Dreo MCU. Types: bool, val, enum.",
	//cmddetail:"fn":"Dreo_SendStateCmd","file":"driver/drv_dreo.c","requires":"",
	//cmddetail:"examples":""}
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
	// Heartbeat every 10 seconds
	g_dreoHeartbeatTimer++;
	if (g_dreoHeartbeatTimer >= 10) {
		Dreo_SendRaw(DREO_CMD_HEARTBEAT, NULL, 0);
		g_dreoHeartbeatTimer = 0;
	}

	// Init sequence (mimics dreo.h setup)
	// State 0: send heartbeat (cmd 0x00) — already done above on first call
	// State 1: after ~1s, send MCU conf with init data
	// State 2: after ~2s, send query state (cmd 0x02)
	if (g_dreoInitState < 3) {
		g_dreoInitTimer++;
		switch (g_dreoInitState) {
		case 0:
			// Send initial heartbeat
			Dreo_SendRaw(DREO_CMD_HEARTBEAT, NULL, 0);
			g_dreoInitState = 1;
			break;
		case 1:
			if (g_dreoInitTimer >= 1) {
				// Send MCU conf init data: {0x02, 0x05, 0x00}
				byte initData[] = { 0x02, 0x05, 0x00 };
				Dreo_SendRaw(DREO_CMD_WIFI_STATE, initData, sizeof(initData));
				g_dreoInitState = 2;
			}
			break;
		case 2:
			if (g_dreoInitTimer >= 2) {
				// Send query state
				Dreo_SendRaw(DREO_CMD_MCU_CONF, NULL, 0);
				g_dreoInitState = 3;
			}
			break;
		}
	}
}

void Dreo_RunFrame(void) {
	byte pkt[192];
	int len;

	while (1) {
		len = Dreo_TryGetPacket(pkt, sizeof(pkt));
		if (len > 0) {
			Dreo_ProcessPacket(pkt, len);
		} else {
			break;
		}
	}
}

void Dreo_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
	dreoMapping_t *cur;

	if (bPreState)
		return;

	hprintf255(request, "<h5>Dreo Heater (sent: %i, recv: %i, invalid: %i bytes)</h5>",
		g_dreoBytesSent, g_dreoBytesReceived, g_dreoBytesInvalid);
	poststr(request, "<table border=1 style='border-collapse:collapse;'>");
	poststr(request, "<tr><th>dpID</th><th>Type</th><th>Value</th><th>Channel</th></tr>");

	cur = g_dreoMappings;
	while (cur) {
		const char *typeName;
		switch (cur->dpType) {
		case DREO_DP_TYPE_BOOL:   typeName = "bool"; break;
		case DREO_DP_TYPE_VALUE:  typeName = "val";  break;
		case DREO_DP_TYPE_ENUM:   typeName = "enum"; break;
		case DREO_DP_TYPE_STRING: typeName = "str";  break;
		case DREO_DP_TYPE_RAW:    typeName = "raw";  break;
		default:                  typeName = "?";    break;
		}

		if (cur->hasData) {
			if (cur->channel >= 0) {
				hprintf255(request, "<tr><td>%i</td><td>%s</td><td>%i</td><td>CH%i</td></tr>",
					cur->dpId, typeName, cur->lastValue, cur->channel);
			} else {
				hprintf255(request, "<tr><td>%i</td><td>%s</td><td>%i</td><td><i>none</i></td></tr>",
					cur->dpId, typeName, cur->lastValue);
			}
		} else {
			if (cur->channel >= 0) {
				hprintf255(request, "<tr><td>%i</td><td>%s</td><td><i>n/a</i></td><td>CH%i</td></tr>",
					cur->dpId, typeName, cur->channel);
			} else {
				hprintf255(request, "<tr><td>%i</td><td>%s</td><td><i>n/a</i></td><td><i>none</i></td></tr>",
					cur->dpId, typeName);
			}
		}
		cur = cur->next;
	}

	poststr(request, "</table>");
}

void Dreo_OnChannelChanged(int ch, int value) {
	dreoMapping_t *mapping = Dreo_FindByChannel(ch);
	if (mapping == NULL)
		return;

	// Avoid echo: if we just set this value from a received DP, skip
	if (mapping->lastValue == value)
		return;

	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
		"Dreo: channel %i changed to %i, sending dpId %i", ch, value, mapping->dpId);

	switch (mapping->dpType) {
	case DREO_DP_TYPE_BOOL:
		Dreo_SendBool(mapping->dpId, value);
		break;
	case DREO_DP_TYPE_VALUE:
		Dreo_SendValue(mapping->dpId, value);
		break;
	case DREO_DP_TYPE_ENUM:
		Dreo_SendEnum(mapping->dpId, value);
		break;
	default:
		Dreo_SendEnum(mapping->dpId, value);
		break;
	}

	mapping->lastValue = value;
}

void Dreo_OnHassDiscovery(const char *topic) {
	// Placeholder for future HA discovery support
}
