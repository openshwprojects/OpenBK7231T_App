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

// Maximum reasonable payload size (Dreo packets are very small; prevents parser lockup on false 55 AA headers
// that claim huge payloadLen and cause the "incomplete packet -> break" to skip real packets forever)
#define DREO_MAX_PAYLOAD 180

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
static int g_dreoConsecutiveBad = 0;

// -----------------------------------------------------------------------
// Ring-buffer receive (true circular buffer for reliable reassembly)
// -----------------------------------------------------------------------

#define DREO_RX_BUF_SIZE 1024
static byte g_dreoRxBuf[DREO_RX_BUF_SIZE];
static int g_dreoRxHead = 0;   // write pointer (UART data goes here)
static int g_dreoRxTail = 0;   // read pointer  (parser consumes from here)

// Number of bytes currently in the ring buffer
static int Dreo_RxAvailable(void) {
	return (g_dreoRxHead - g_dreoRxTail + DREO_RX_BUF_SIZE) % DREO_RX_BUF_SIZE;
}

// Read one byte from ring (does NOT advance tail)
static byte Dreo_RxPeek(int offset) {
	int idx = (g_dreoRxTail + offset) % DREO_RX_BUF_SIZE;
	return g_dreoRxBuf[idx];
}

// Advance tail by N bytes after successful packet processing
static void Dreo_RxConsume(int n) {
	g_dreoRxTail = (g_dreoRxTail + n) % DREO_RX_BUF_SIZE;
}

// -----------------------------------------------------------------------
// Debug helper: dump a range of bytes from the ring buffer (used when discarding garbage)
// -----------------------------------------------------------------------
static void Dreo_DumpRingBytes(int startOfs, int count, const char *msg) {
	char tmp[256];
	int pos = 0;
	pos += snprintf(tmp + pos, sizeof(tmp) - pos, "Dreo: %s (%d bytes): ", msg, count);
	for (int i = 0; i < count && pos < (int)sizeof(tmp) - 6; i++) {
		pos += snprintf(tmp + pos, sizeof(tmp) - pos, "%02X ", Dreo_RxPeek(startOfs + i));
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "%s", tmp);
}

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
// Checksum = (sum(bytes from ver to last payload byte) - 1) & 0xFF
static void Dreo_SendRaw(byte cmd, const byte *payload, int payloadLen) {
	int i;
	uint32_t sum = 0;

	UART_SendByte(0x55);
	UART_SendByte(0xAA);
	byte ver = 0x00;
	UART_SendByte(ver);           // version
	sum += ver;

	byte seq = g_dreoSeq++;
	UART_SendByte(seq);            // sequence
	sum += seq;

	UART_SendByte(cmd);            // command
	sum += cmd;

	byte reserved = 0x00;
	UART_SendByte(reserved);       // reserved zero
	sum += reserved;

	byte lenH = (payloadLen >> 8) & 0xFF;
	byte lenL = payloadLen & 0xFF;
	UART_SendByte(lenH);           // length high
	UART_SendByte(lenL);           // length low
	sum += lenH;
	sum += lenL;

	for (i = 0; i < payloadLen; i++) {
		UART_SendByte(payload[i]);
		sum += payload[i];
	}

	byte checksum = (byte)((sum - 1) & 0xFF);
	UART_SendByte(checksum);

	g_dreoBytesSent += 8 + payloadLen + 1;  // header(8) + payload + checksum(1)

	addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Dreo: sent cmd 0x%02X, seq=0x%02X, %i payload bytes", cmd, seq, payloadLen);
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
// Dreo UART Packet Receive (ring-buffer version)
// -----------------------------------------------------------------------

// Dreo packet format: 55 AA [ver] [seq] [cmd] [00] [lenH] [lenL] [payload...] [checksum]
// Minimum packet: 8 header + 1 checksum = 9 bytes
#define DREO_MIN_PACKET_SIZE 9

// Helper: dump raw bytes when we see a potential header (very useful for desync debugging)
static void Dreo_DumpBytes(const byte *buf, int len, const char *msg) {
	char tmp[256];
	int pos = 0;
	pos += snprintf(tmp + pos, sizeof(tmp) - pos, "Dreo: %s: ", msg);
	for (int i = 0; i < len && pos < (int)sizeof(tmp) - 4; i++) {
		pos += snprintf(tmp + pos, sizeof(tmp) - pos, "%02X ", buf[i]);
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "%s", tmp);
}

static int Dreo_TryGetPacket(byte *out, int maxSize) {
	int avail = Dreo_RxAvailable();

	// Copy new UART data into our ring buffer (done once per call - ring buffer stays stable)
	int cs = UART_GetDataSize();
	if (cs > 0) {
		if (cs > DREO_RX_BUF_SIZE - avail) {
			// buffer full - discard oldest data
			int keep = DREO_RX_BUF_SIZE - 400;
			if (keep < 0) keep = 0;
			g_dreoRxTail = (g_dreoRxTail + (DREO_RX_BUF_SIZE - keep)) % DREO_RX_BUF_SIZE;
		}

		for (int i = 0; i < cs; i++) {
			g_dreoRxBuf[g_dreoRxHead] = UART_GetByte(i);
			g_dreoRxHead = (g_dreoRxHead + 1) % DREO_RX_BUF_SIZE;
		}
		UART_ConsumeBytes(cs);
	}

	avail = Dreo_RxAvailable();
	if (avail < DREO_MIN_PACKET_SIZE)
		return 0;

	// Search for 55 AA header
	for (int ofs = 0; ofs <= avail - DREO_MIN_PACKET_SIZE; ofs++) {
		if (Dreo_RxPeek(ofs) != 0x55 || Dreo_RxPeek(ofs + 1) != 0xAA)
			continue;

		// Scenario 3: garbage bytes at the beginning - discard them now so a real header becomes aligned
		if (ofs > 0) {
			Dreo_DumpRingBytes(0, ofs, "discarded leading garbage bytes before valid header");
			addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: discarding %d leading garbage bytes before header", ofs);
			Dreo_RxConsume(ofs);
			return 0;   // re-evaluate the buffer in the next TryGetPacket call (RunFrame while-loop will continue)
		}

		// Header is now aligned at position 0 (ofs == 0)
		byte ver = Dreo_RxPeek(2);
		byte seq = Dreo_RxPeek(3);
		byte cmd = Dreo_RxPeek(4);
		byte lenH = Dreo_RxPeek(6);
		byte lenL = Dreo_RxPeek(7);
		int payloadLen = ((int)lenH << 8) | lenL;
		int packetLen = 8 + payloadLen + 1;

		// Reject obviously bogus headers
		if (payloadLen > DREO_MAX_PAYLOAD) {
			addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: bogus large header (payloadLen=%d > %d) at ofs=0 - skipping bad sync", payloadLen, DREO_MAX_PAYLOAD);
			Dreo_RxConsume(1); // discard the bad 55 AA
			continue;
		}

		if (packetLen > avail) {
			// Scenario 2: valid header but packet still incomplete - leave the ring buffer untouched and wait for next frame
			addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Dreo: incomplete packet at ofs=0 (need %d bytes, have %d)", packetLen, avail);
			return 0;
		}

		// Scenario 1: complete valid-looking packet at the front - verify checksum using only Peek (no extra copy yet)
		uint32_t calcSum = 0;
		for (int i = 2; i < packetLen - 1; i++) {
			calcSum += Dreo_RxPeek(i);
		}
		byte expected = (byte)((calcSum - 1) & 0xFF);
		byte actual = Dreo_RxPeek(packetLen - 1);

		if (actual != expected) {
			g_dreoBytesInvalid++;
			g_dreoConsecutiveBad++;

			addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
				"Dreo: checksum FAIL ofs=0 seq=0x%02X cmd=0x%02X expected=0x%02X got=0x%02X (sum=0x%02X)",
				seq, cmd, expected, actual, (byte)calcSum);

			if (g_dreoConsecutiveBad > 5) {
				addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: too many consecutive bad packets - forcing resync (discarding 30 bytes)");
				Dreo_RxConsume(30);
				g_dreoConsecutiveBad = 0;
			} else {
				Dreo_RxConsume(1); // discard just the leading 55 for faster resync
			}
			continue;
		}

		// VALID PACKET FOUND (scenario 1)
		g_dreoConsecutiveBad = 0;

		// Copy only the confirmed valid packet to linear buffer for processing
		if (packetLen > maxSize) {
			addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: packet too large (%i > %i)", packetLen, maxSize);
			Dreo_RxConsume(packetLen);
			return 0;
		}
		for (int i = 0; i < packetLen; i++) {
			out[i] = Dreo_RxPeek(i);
		}

		Dreo_DumpBytes(out, packetLen, "found potential packet");

		Dreo_RxConsume(packetLen);

		g_dreoBytesReceived += packetLen;

		addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL,
			"Dreo: VALID packet seq=0x%02X cmd=0x%02X payload=%i bytes", seq, cmd, payloadLen);

		return packetLen;
	}

	// No 55 AA header found at all (pure garbage scenario)
	// Prevent the ring buffer from filling up forever with garbage
	if (avail >= DREO_MIN_PACKET_SIZE * 2) {
		Dreo_DumpRingBytes(0, 1, "discarded single garbage byte (no header found)");
		addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Dreo: no header found - discarding 1 garbage byte for resync");
		Dreo_RxConsume(1);
	}

	return 0;
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
	byte seq = data[3];
	int payloadLen = (data[6] << 8) | data[7];

	addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL,
		"Dreo: received cmd=0x%02X seq=0x%02X payload=%i bytes", cmd, seq, payloadLen);

	switch (cmd) {
	case DREO_CMD_HEARTBEAT:
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: heartbeat ACK (seq=0x%02X)", seq);
		break;

	case DREO_CMD_QUERY_PRODUCT:
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: product info response (seq=0x%02X)", seq);
		break;

	case DREO_CMD_MCU_CONF:
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: MCU conf response (seq=0x%02X)", seq);
		break;

	case DREO_CMD_WIFI_STATE:   // 0x03 - response to our init WiFi state command
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Dreo: WiFi state response (seq=0x%02X)", seq);
		break;

	case 0x0E:   // unknown command seen in logs (MCU reply, 3-byte payload)
		addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Dreo: received unknown cmd 0x0E (3-byte payload, seq=0x%02X)", seq);
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
			"Dreo: unknown cmd 0x%02X (seq=0x%02X)", cmd, seq);
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
	g_dreoConsecutiveBad = 0;
	g_dreoRxHead = 0;
	g_dreoRxTail = 0;

	UART_InitUART(115200, 0, false);
	UART_InitReceiveRingBuffer(1024);   // increased from 512 to prevent buffer overflow / desync on ESP-IDF

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
	// Heartbeat every 10 seconds (countdown prevents double-sending seen in previous logs)
	static int g_heartbeatCountdown = 10;
	g_heartbeatCountdown--;
	if (g_heartbeatCountdown <= 0) {
		Dreo_SendRaw(DREO_CMD_HEARTBEAT, NULL, 0);
		g_heartbeatCountdown = 10;
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