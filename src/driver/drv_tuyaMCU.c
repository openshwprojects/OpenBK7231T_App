
//
// Generic TuyaMCU information
//
/*
There are two versions of TuyaMCU that I am aware of.
TuyaMCU version 3, the one that is supported by Tasmota and documented here:

TuyaMCU version 0, aka low power protocol, documented here:
(Tuya IoT Development PlatformProduct DevelopmentLow-Code Development (MCU)Wi-Fi for Low-PowerSerial Port Protocol)
https://developer.tuya.com/en/docs/iot/tuyacloudlowpoweruniversalserialaccessprotocol?id=K95afs9h4tjjh
*/

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
#include "drv_tuyaMCU.h"
#include "drv_uart.h"
#include "drv_public.h"
#include <time.h>
#include "drv_deviceclock.h"
#include "../rgb2hsv.h"


#define TUYA_CMD_HEARTBEAT     0x00
#define TUYA_CMD_QUERY_PRODUCT 0x01
#define TUYA_CMD_MCU_CONF      0x02
#define TUYA_CMD_WIFI_STATE    0x03
#define TUYA_CMD_WIFI_RESET    0x04
#define TUYA_CMD_WIFI_SELECT   0x05
#define TUYA_CMD_SET_DP        0x06
#define TUYA_CMD_STATE         0x07
#define TUYA_CMD_QUERY_STATE   0x08
#define TUYA_CMD_SET_TIME      0x1C
#define TUYA_CMD_WEATHERDATA   0x21
#define TUYA_CMD_SET_RSSI      0x24
#define TUYA_CMD_NETWORK_STATUS 0x2B
#define TUYA_CMD_REPORT_STATUS_RECORD_TYPE		0x34 

#define TUYA_V0_CMD_PRODUCTINFORMATION      0x01
#define TUYA_V0_CMD_NETWEORKSTATUS          0x02
#define TUYA_V0_CMD_RESETWIFI               0x03
#define TUYA_V0_CMD_RESETWIFI_AND_SEL_CONF  0x04
#define TUYA_V0_CMD_REALTIMESTATUS          0x05
#define TUYA_V0_CMD_OBTAINLOCALTIME         0x06
#define TUYA_V0_CMD_RECORDSTATUS            0x08
#define TUYA_V0_CMD_OBTAINDPCACHE           0x10
#define TUYA_V0_CMD_QUERYSIGNALSTRENGTH		0x0B

#define TUYA_NETWORK_STATUS_SMART_CONNECT_SETUP 0x00
#define TUYA_NETWORK_STATUS_AP_MODE             0x01
#define TUYA_NETWORK_STATUS_NOT_CONNECTED       0x02
#define TUYA_NETWORK_STATUS_CONNECTED_TO_ROUTER 0x03
#define TUYA_NETWORK_STATUS_CONNECTED_TO_CLOUD  0x04
#define TUYA_NETWORK_STATUS_LOW_POWER_MODE      0x05

void TuyaMCU_RunFrame();

static byte g_tuyaMCUConfirmationsToSend_0x08 = 0;
static byte g_tuyaMCUConfirmationsToSend_0x05 = 0;
static byte g_resetWiFiEvents = 0;

#define OBKTM_FLAG_DPCACHE 1
#define OBKTM_FLAG_NOREAD 2

//=============================================================================
//dp data point type
//=============================================================================
#define         DP_TYPE_RAW                     0x00        //RAW type
#define         DP_TYPE_BOOL                    0x01        //bool type
#define         DP_TYPE_VALUE                   0x02        //value type
#define         DP_TYPE_STRING                  0x03        //string type
#define         DP_TYPE_ENUM                    0x04        //enum type
#define         DP_TYPE_BITMAP                  0x05        //fault type

// Subtypes of raw - added for OpenBeken - not Tuya standard
#define         DP_TYPE_RAW_DDS238Packet        200
#define			DP_TYPE_RAW_TAC2121C_VCP		201
#define			DP_TYPE_RAW_TAC2121C_YESTERDAY	202
#define			DP_TYPE_RAW_TAC2121C_LASTMONTH	203
#define			DP_TYPE_PUBLISH_TO_MQTT			204
#define			DP_TYPE_RAW_VCPPfF				205
#define			DP_TYPE_RAW_V2C3P3				206

const char* TuyaMCU_GetDataTypeString(int dpId) {
	if (DP_TYPE_RAW == dpId)
		return "raw";
	if (DP_TYPE_BOOL == dpId)
		return "bool";
	if (DP_TYPE_VALUE == dpId)
		return "val";
	if (DP_TYPE_STRING == dpId)
		return "str";
	if (DP_TYPE_ENUM == dpId)
		return "enum";
	if (DP_TYPE_BITMAP == dpId)
		return "bitmap";
	return "error";
}




const char* TuyaMCU_GetCommandTypeLabel(int t) {
	if (t == TUYA_CMD_HEARTBEAT)
		return "Hearbeat";
	if (t == TUYA_CMD_QUERY_PRODUCT)
		return "QueryProductInformation";
	if (t == TUYA_CMD_MCU_CONF)
		return "MCUconf";
	if (t == TUYA_CMD_WIFI_STATE)
		return "WiFiState";
	if (t == TUYA_CMD_WIFI_RESET)
		return "WiFiReset";
	if (t == TUYA_CMD_WIFI_SELECT)
		return "WiFiSelect";
	if (t == TUYA_CMD_SET_DP)
		return "SetDP";
	if (t == TUYA_CMD_STATE)
		return "State";
	if (t == TUYA_CMD_QUERY_STATE)
		return "QueryState";
	if (t == TUYA_CMD_SET_TIME)
		return "SetTime";
	if (t == TUYA_CMD_WEATHERDATA)
		return "WeatherData";
	if (t == TUYA_CMD_NETWORK_STATUS)
		return "NetworkStatus";
	if (t == TUYA_CMD_SET_RSSI)
		return "SetRSSI";
	if (t == TUYA_V0_CMD_QUERYSIGNALSTRENGTH)
		return "QuerySignalStrngth";
	if (t == TUYA_CMD_REPORT_STATUS_RECORD_TYPE)
		return "TUYA_CMD_REPORT_STATUS_RECORD_TYPE";
	return "Unknown";
}

typedef struct rtcc_s {
	uint8_t       second;
	uint8_t       minute;
	uint8_t       hour;
	uint8_t       day_of_week;               // sunday is day 1
	uint8_t       day_of_month;
	uint8_t       month;
	char          name_of_month[4];
	uint16_t      day_of_year;
	uint16_t      year;
	uint32_t      days;
	uint32_t      valid;
} rtcc_t;

typedef struct tuyaMCUMapping_s {
	// internal Tuya variable index
	byte dpId;
	// data point type (one of the DP_TYPE_xxx defines)
	byte dpType;
	// true if it's supposed to be sent in dp cache
	byte obkFlags;
	// could be renamed to flags later?
	byte inv;
	// target channel
	short channel;
	// store last channel value to avoid sending it again
	int prevValue;
	// allow storing raw data for later usage
	byte *rawData;
	int rawBufferSize;
	int rawDataLen;
	// not really useful as long as we have integer channels
	float mult;
	float delta;
	float delta2;
	float delta3;
	// TODO
	//int mode;
	// list
	struct tuyaMCUMapping_s* next;
} tuyaMCUMapping_t;

tuyaMCUMapping_t* g_tuyaMappings = 0;

/**
 * Dimmer range
 *
 * Map OpenBK7231T_App's dimmer range of 0..100 to the dimmer range used by TuyMCU.
 * Use tuyaMCU_setDimmerrange command to set range used by TuyaMCU.
 */
 // minimum dimmer value as reported by TuyaMCU dimmer
static int g_dimmerRangeMin = 0;
// maximum dimmer value as reported by TuyaMCU dimmer
static int g_dimmerRangeMax = 100;

// serial baud rate used to communicate with the TuyaMCU
// common baud rates are 9600 bit/s and 115200 bit/s
static int g_baudRate = 9600;

// global mcu time
static int g_tuyaNextRequestDelay;
static bool g_sensorMode = 0;

// Flag to prevent automatic sending during boot/initialization
static bool g_tuyaMCU_allowAutomaticSending = true;

static bool heartbeat_valid = false;
static int heartbeat_timer = 0;
static int heartbeat_counter = 0;
static bool product_information_valid = false;
// ?? it's unused atm
//static char *prod_info = NULL;
static bool working_mode_valid = false;
static bool wifi_state_valid = false;
static bool wifi_state = false;
static int wifi_state_timer = 0;
static bool self_processing_mode = true;
static bool state_updated = false;
static int g_sendQueryStatePackets = 0;
static int g_tuyaMCUBatteryAckDelay = 0;

// wifistate to send when not online
// See: https://imgur.com/a/mEfhfiA
static byte g_defaultTuyaMCUWiFiState = 0x00;

// LED lighitng 
// See: https://www.elektroda.com/rtvforum/viewtopic.php?p=20646359#20646359
static short g_tuyaMCUled_id_color = -1;
static short g_tuyaMCUled_format = -1;
static short g_tuyaMCUled_id_cw_brightness = -1;
static short g_tuyaMCUled_id_power = -1;
static short g_tuyaMCUled_id_cw_temperature = -1;

#define TUYAMCU_BUFFER_SIZE		256

static byte *g_tuyaMCUpayloadBuffer = 0;
static int g_tuyaMCUpayloadBufferSize = 0;

// battery powered device state
enum TuyaMCUV0State {
	TM0_STATE_AWAITING_INFO,
	TM0_STATE_AWAITING_WIFI,
	TM0_STATE_AWAITING_MQTT,
	TM0_STATE_AWAITING_STATES,
};
static byte g_tuyaBatteryPoweredState = 0;
static byte g_hello[] = { 0x55, 0xAA, 0x00, 0x01, 0x00, 0x00, 0x00 };
//static byte g_request_state[] = { 0x55, 0xAA, 0x00, 0x02, 0x00, 0x01, 0x04, 0x06 };
static bool g_tuyaMCU_batteryPoweredMode = false;

typedef struct tuyaMCUPacket_s {
	byte *data;
	int size;
	int allocated;
	struct tuyaMCUPacket_s *next;
} tuyaMCUPacket_t;

tuyaMCUPacket_t *tm_emptyPackets = 0;
tuyaMCUPacket_t *tm_sendPackets = 0;

tuyaMCUPacket_t *TUYAMCU_AddToQueue(int len) {
	tuyaMCUPacket_t *toUse;
	if (tm_emptyPackets) {
		toUse = tm_emptyPackets;
		tm_emptyPackets = toUse->next;

		if (len > toUse->allocated) {
			toUse->data = realloc(toUse->data, len);
			toUse->allocated = len;
		}
	}
	else {
		toUse = malloc(sizeof(tuyaMCUPacket_t));
		int toAlloc = 128;
		if (len > toAlloc)
			toAlloc = len;
		toUse->allocated = toAlloc;
		toUse->data = malloc(toUse->allocated);
	}
	toUse->size = len;
	if (tm_sendPackets == 0) {
		tm_sendPackets = toUse;
	}
	else {
		tuyaMCUPacket_t *p = tm_sendPackets;
		while (p->next) {
			p = p->next;
		}
		p->next = toUse;
	}
	toUse->next = 0;
	return toUse;
}
bool TUYAMCU_SendFromQueue() {
	tuyaMCUPacket_t *toUse;
	if (tm_sendPackets == 0)
		return false;
	toUse = tm_sendPackets;
	tm_sendPackets = toUse->next;

	UART_SendByte(0x55);
	UART_SendByte(0xAA);
	UART_SendByte(0x00);
	for (int i = 0; i < toUse->size; i++) {
		UART_SendByte(toUse->data[i]);
	}

	toUse->next = tm_emptyPackets;
	tm_emptyPackets = toUse;
	return true;
}

tuyaMCUMapping_t* TuyaMCU_FindDefForID(int dpId) {
	tuyaMCUMapping_t* cur;

	cur = g_tuyaMappings;
	while (cur) {
		if (cur->dpId == dpId)
			return cur;
		cur = cur->next;
	}
	return 0;
}

tuyaMCUMapping_t* TuyaMCU_FindDefForChannel(int channel) {
	tuyaMCUMapping_t* cur;

	cur = g_tuyaMappings;
	while (cur) {
		if (cur->channel == channel)
			return cur;
		cur = cur->next;
	}
	return 0;
}

tuyaMCUMapping_t* TuyaMCU_MapIDToChannel(int dpId, int dpType, int channel, int obkFlags, float mul, int inv, float delta, float delta2, float delta3) {
	tuyaMCUMapping_t* cur;

	cur = TuyaMCU_FindDefForID(dpId);

	if (cur == 0) {
		cur = (tuyaMCUMapping_t*)malloc(sizeof(tuyaMCUMapping_t));
		cur->next = g_tuyaMappings;
		cur->rawData = 0;
		cur->rawDataLen = 0;
		cur->rawBufferSize = 0;
		g_tuyaMappings = cur;
	}
	cur->dpId = dpId;
	cur->dpType = dpType;
	cur->obkFlags = obkFlags;
	cur->mult = mul;
	cur->delta = delta;
	cur->delta2 = delta2;
	cur->delta3 = delta3;
	cur->inv = inv;
	cur->prevValue = 0;
	cur->channel = channel;
	return cur;
}

// now you can detect TuyaMCU faults with event handler
// addChangeHandler MissedHeartbeats > 4 setChannel 0 1
void TuyaMCU_SetHeartbeatCounter(int v) {
	EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_MISSEDHEARTBEATS, heartbeat_counter, v);
	heartbeat_counter = v;
}

// header version command lenght data checksum
// 55AA     00      00      0000   xx   00

#define MIN_TUYAMCU_PACKET_SIZE (2+1+1+2+1)
int UART_TryToGetNextTuyaPacket(byte* out, int maxSize) {
	int cs;
	int len, i;
	int c_garbage_consumed = 0;
	byte a, b, version, command, lena, lenb;
	char printfSkipDebug[256];
	char buffer2[8];

	printfSkipDebug[0] = 0;

	cs = UART_GetDataSize();

	if (cs < MIN_TUYAMCU_PACKET_SIZE) {
		return 0;
	}
	// skip garbage data (should not happen)
	while (cs > 0) {
		a = UART_GetByte(0);
		b = UART_GetByte(1);
		if (a != 0x55 || b != 0xAA) {
			UART_ConsumeBytes(1);
			if (c_garbage_consumed + 2 < sizeof(printfSkipDebug)) {
				snprintf(buffer2, sizeof(buffer2), "%02X ", a);
				strcat_safe(printfSkipDebug, buffer2, sizeof(printfSkipDebug));
			}
			c_garbage_consumed++;
			cs--;
		}
		else {
			break;
		}
	}
	if (c_garbage_consumed > 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "Consumed %i unwanted non-header byte in Tuya MCU buffer\n", c_garbage_consumed);
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "Skipped data (part) %s\n", printfSkipDebug);
	}
	if (cs < MIN_TUYAMCU_PACKET_SIZE) {
		return 0;
	}
	a = UART_GetByte(0);
	b = UART_GetByte(1);
	if (a != 0x55 || b != 0xAA) {
		return 0;
	}
	version = UART_GetByte(2);
	command = UART_GetByte(3);
	lena = UART_GetByte(4); // hi
	lenb = UART_GetByte(5); // lo
	len = lenb | lena >> 8;
	// now check if we have received whole packet
	len += 2 + 1 + 1 + 2 + 1; // header 2 bytes, version, command, lenght, chekcusm
	if (cs >= len) {
		int ret;
		// can packet fit into the buffer?
		if (len <= maxSize) {
			for (i = 0; i < len; i++) {
				out[i] = UART_GetByte(i);
			}
			ret = len;
		}
		else {
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU packet too large, %i > %i\n", len, maxSize);
			ret = 0;
		}
		// consume whole packet (but don't touch next one, if any)
		UART_ConsumeBytes(len);
		return ret;
	}
	return 0;
}



// append header, len, everything, checksum
void TuyaMCU_SendCommandWithData(byte cmdType, byte* data, int payload_len) {
	int i;
	
	byte check_sum = (0xFF + cmdType + (payload_len >> 8) + (payload_len & 0xFF));

	//UART_InitUART(g_baudRate, 0, false);
	if (CFG_HasFlag(OBK_FLAG_TUYAMCU_USE_QUEUE)) {
		tuyaMCUPacket_t *p = TUYAMCU_AddToQueue(payload_len + 4);
		p->data[0] = cmdType;
		p->data[1] = payload_len >> 8;
		p->data[2] = payload_len & 0xFF;
		memcpy(p->data + 3, data, payload_len);
		for (i = 0; i < payload_len; i++) {
			byte b = data[i];
			check_sum += b;
		}
		p->data[3+payload_len] = check_sum;
	}
	else {
		UART_SendByte(0x55);
		UART_SendByte(0xAA);
		UART_SendByte(0x00);         // version 00
		UART_SendByte(cmdType);         // version 00
		UART_SendByte(payload_len >> 8);      // following data length (Hi)
		UART_SendByte(payload_len & 0xFF);    // following data length (Lo)
		for (i = 0; i < payload_len; i++) {
			byte b = data[i];
			check_sum += b;
			UART_SendByte(b);
		}
		UART_SendByte(check_sum);
	}
}
int TuyaMCU_AppendStateInternal(byte *buffer, int bufferMax, int currentLen, uint8_t id, int8_t type, void* value, int dataLen) {
	if (currentLen + 4 + dataLen >= bufferMax) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "Tuya buff overflow");
		return 0;
	}
	buffer[currentLen + 0] = id;
	buffer[currentLen + 1] = type;
	buffer[currentLen + 2] = 0x00;
	buffer[currentLen + 3] = dataLen;
	memcpy(buffer + (currentLen + 4), value, dataLen);
	return currentLen + 4 + dataLen;
}
void TuyaMCU_SendStateInternal(uint8_t id, uint8_t type, void* value, int dataLen)
{
	uint16_t payload_len = 0;
	
	payload_len = TuyaMCU_AppendStateInternal(g_tuyaMCUpayloadBuffer, g_tuyaMCUpayloadBufferSize,
		payload_len, id, type, value, dataLen);

	TuyaMCU_SendCommandWithData(TUYA_CMD_SET_DP, g_tuyaMCUpayloadBuffer, payload_len);
}
void TuyaMCU_SendTwoVals(byte idA, int valA, byte idB, int valB)
{
	byte swap[4];
	uint16_t payload_len = 0;

	swap[0] = ((byte*)&valA)[3];
	swap[1] = ((byte*)&valA)[2];
	swap[2] = ((byte*)&valA)[1];
	swap[3] = ((byte*)&valA)[0];
	payload_len = TuyaMCU_AppendStateInternal(g_tuyaMCUpayloadBuffer, g_tuyaMCUpayloadBufferSize,
		payload_len, idA, DP_TYPE_VALUE, swap, 4);
	swap[0] = ((byte*)&valB)[3];
	swap[1] = ((byte*)&valB)[2];
	swap[2] = ((byte*)&valB)[1];
	swap[3] = ((byte*)&valB)[0];
	payload_len = TuyaMCU_AppendStateInternal(g_tuyaMCUpayloadBuffer, g_tuyaMCUpayloadBufferSize,
		payload_len, idB, DP_TYPE_VALUE, swap, 4);

	TuyaMCU_SendCommandWithData(TUYA_CMD_SET_DP, g_tuyaMCUpayloadBuffer, payload_len);
}
void TuyaMCU_SendState(uint8_t id, uint8_t type, uint8_t* value)
{
	byte swap[4];

	switch (type) {
	case DP_TYPE_BOOL:
	case DP_TYPE_ENUM:
		TuyaMCU_SendStateInternal(id, type, value, 1);
		break;
	case DP_TYPE_VALUE:
		swap[0] = value[3];
		swap[1] = value[2];
		swap[2] = value[1];
		swap[3] = value[0];
		TuyaMCU_SendStateInternal(id, type, swap, 4);
		break;
	case DP_TYPE_STRING:
		TuyaMCU_SendStateInternal(id, type, value, strlen((const char*)value));
		break;
	case DP_TYPE_RAW:
		break;
	}
}

void TuyaMCU_SendBool(uint8_t id, bool value)
{
	TuyaMCU_SendState(id, DP_TYPE_BOOL, (uint8_t*)&value);
}

void TuyaMCU_SendValue(uint8_t id, uint32_t value)
{
	TuyaMCU_SendState(id, DP_TYPE_VALUE, (uint8_t*)(&value));
}

void TuyaMCU_SendEnum(uint8_t id, uint32_t value)
{
	TuyaMCU_SendState(id, DP_TYPE_ENUM, (uint8_t*)(&value));
}

static uint16_t convertHexStringtoBytes(uint8_t* dest, char src[], uint16_t src_len) {
	char hexbyte[3];
	uint16_t i;

	if (NULL == dest || NULL == src || 0 == src_len) {
		return 0;
	}

	hexbyte[2] = 0;

	for (i = 0; i < src_len; i++) {
		hexbyte[0] = src[2 * i];
		hexbyte[1] = src[2 * i + 1];
		dest[i] = strtol(hexbyte, NULL, 16);
	}

	return i;
}

void TuyaMCU_SendHexString(uint8_t id, char data[]) {
	uint16_t len = strlen(data) / 2;
	uint16_t payload_len = 4 + len;
	g_tuyaMCUpayloadBuffer[0] = id;
	g_tuyaMCUpayloadBuffer[1] = DP_TYPE_STRING;
	g_tuyaMCUpayloadBuffer[2] = len >> 8;
	g_tuyaMCUpayloadBuffer[3] = len & 0xFF;

	convertHexStringtoBytes(&g_tuyaMCUpayloadBuffer[4], data, len);

	TuyaMCU_SendCommandWithData(TUYA_CMD_SET_DP, g_tuyaMCUpayloadBuffer, payload_len);
}

void TuyaMCU_SendString(uint8_t id, char data[]) {
	uint16_t len = strlen(data);
	uint16_t payload_len = 4 + len;
	g_tuyaMCUpayloadBuffer[0] = id;
	g_tuyaMCUpayloadBuffer[1] = DP_TYPE_STRING;
	g_tuyaMCUpayloadBuffer[2] = len >> 8;
	g_tuyaMCUpayloadBuffer[3] = len & 0xFF;

	for (uint16_t i = 0; i < len; i++) {
		g_tuyaMCUpayloadBuffer[4 + i] = data[i];
	}

	TuyaMCU_SendCommandWithData(TUYA_CMD_SET_DP, g_tuyaMCUpayloadBuffer, payload_len);
}

void TuyaMCU_SendRaw(uint8_t id, char data[]) {
	char* beginPos = strchr(data, 'x');
	if (!beginPos) {
		beginPos = strchr(data, 'X');
	}
	if (!beginPos) {
		beginPos = data;
	}
	else {
		beginPos += 1;
	}
	uint16_t strSize = strlen(beginPos);
	uint16_t len = strSize / 2;
	uint16_t payload_len = 4 + len;
	g_tuyaMCUpayloadBuffer[0] = id;
	g_tuyaMCUpayloadBuffer[1] = DP_TYPE_RAW;
	g_tuyaMCUpayloadBuffer[2] = len >> 8;
	g_tuyaMCUpayloadBuffer[3] = len & 0xFF;

	convertHexStringtoBytes(&g_tuyaMCUpayloadBuffer[4], beginPos, len);

	TuyaMCU_SendCommandWithData(TUYA_CMD_SET_DP, g_tuyaMCUpayloadBuffer, payload_len);
}

/*
For setting the Wifi Signal Strength. I tested by using the following.
Take the RSSI for the front web interface (eg -54), calculate the 2's complement (0xCA),
and manually calculate the checksum and it works.
"uartSendHex 55AA 00 24 0001 CA EE"
*/
void TuyaMCU_Send_RSSI(int rssi) {
	char payload_signedByte;

	payload_signedByte = rssi;

	TuyaMCU_SendCommandWithData(TUYA_CMD_SET_RSSI, (byte*)&payload_signedByte, 1);
}
// See this post to find about about arguments of WiFi state
// https://www.elektroda.com/rtvforum/viewtopic.php?p=20483899#20483899
commandResult_t Cmd_TuyaMCU_Set_DefaultWiFiState(const void* context, const char* cmd, const char* args, int cmdFlags) {

	g_defaultTuyaMCUWiFiState = atoi(args);

	return CMD_RES_OK;
}
void TuyaMCU_SendColor(int dpID, float fR, float fG, float fB, int tuyaRGB) {
	char str[16];
	float hue, sat, val;

	RGBtoHSV(fR, fG, fB, &hue, &sat, &val);
	int iHue, iSat, iVal;
	iHue = (int)hue; //0,359
	iSat = sat * 100; // 0-100
	iVal = val * 100; // 0-100

	// this is based on formats figured out in Tasmota
	switch (tuyaRGB) {
	case 0: // Uppercase Type 1 payload
		sprintf(str, "%04X%04X%04X", iHue, iSat * 10, iVal * 10);
		break;
	case 1: // Lowercase Type 1 payload
		// SAMPLE: 00e403c0000a 
		// which is 00e4 03c0 000a
		sprintf(str, "%04x%04x%04x", iHue, iSat * 10, iVal * 10);
		break;
	case 2: // Uppercase Type 2 payload
		// RRGGBBFFFF6464 
		//snprintf(str, sizeof(str), ("%sFFFF6464"), scolor);
		break;
	case 3: // Lowercase Type 2 payload
		//snprintf(str, sizeof(str), ("%sffff6464"), tolower(scolor, scolor));
		break;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "Color is sent as %s\n", str);
	TuyaMCU_SendString(dpID, str);
}
// tuyaMCU_sendColor dpID red01 green01 blue01 tuyaRGB
// tuyaMCU_sendColor 24 1 0 0 1
// tuyaMCU_sendColor 24 1 0 0 1
// green:
// tuyaMCU_sendColor 24 0 1 0 1
commandResult_t Cmd_TuyaMCU_SendColor(const void* context, const char* cmd, const char* args, int cmdFlags) {
	float fR, fG, fB;
	int dpID, tuyaRGB;

	Tokenizer_TokenizeString(args, 0);

	dpID = Tokenizer_GetArgInteger(0);
	fR = Tokenizer_GetArgFloat(1);
	fG = Tokenizer_GetArgFloat(2);
	fB = Tokenizer_GetArgFloat(3);
	tuyaRGB = Tokenizer_GetArgIntegerDefault(4, 1);

	TuyaMCU_SendColor(dpID, fR, fG, fB, tuyaRGB);

	return CMD_RES_OK;
}
// tuyaMCU_setupLED dpIDColor TasFormat dpIDPower
commandResult_t Cmd_TuyaMCU_SetupLED(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	g_tuyaMCUled_id_color = Tokenizer_GetArgInteger(0);
	g_tuyaMCUled_format = Tokenizer_GetArgInteger(1);
	g_tuyaMCUled_id_power = Tokenizer_GetArgIntegerDefault(2, 20);
	g_tuyaMCUled_id_cw_brightness = Tokenizer_GetArgIntegerDefault(3, 22);
	g_tuyaMCUled_id_cw_temperature = Tokenizer_GetArgIntegerDefault(4, 23);

	return CMD_RES_OK;
}
commandResult_t Cmd_TuyaMCU_Send_RSSI(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int toSend;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 1) {
		toSend = HAL_GetWifiStrength();
	}
	else {
		toSend = Tokenizer_GetArgInteger(0);
	}
	TuyaMCU_Send_RSSI(toSend);
	return CMD_RES_OK;
}
void TuyaMCU_Send_SignalStrength(byte state, byte strength) {
	byte payload_buffer[2];
	payload_buffer[0] = state;
	payload_buffer[1] = strength;
	TuyaMCU_SendCommandWithData(TUYA_V0_CMD_QUERYSIGNALSTRENGTH, payload_buffer, sizeof(payload_buffer));
}
void TuyaMCU_Send_SetTime(struct tm* pTime, bool bSensorMode) {
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
	if (bSensorMode) {
		// https://developer.tuya.com/en/docs/iot/tuyacloudlowpoweruniversalserialaccessprotocol?id=K95afs9h4tjjh
		TuyaMCU_SendCommandWithData(0x06, payload_buffer, 8);
	}
	else {
		TuyaMCU_SendCommandWithData(TUYA_CMD_SET_TIME, payload_buffer, 8);
	}
}
struct tm* TuyaMCU_Get_NTP_Time() {
	struct tm* ptm;
	time_t ntpTime;

	ntpTime=(time_t)TIME_GetCurrentTime();
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "MCU time to set: %i\n", ntpTime);
	ptm = gmtime(&ntpTime);
	if (ptm != 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ptime ->gmtime => tm_hour: %i\n", ptm->tm_hour);
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ptime ->gmtime => tm_min: %i\n", ptm->tm_min);
	}
	return ptm;
}
void TuyaMCU_Send_RawBuffer(byte* data, int len) {
	int i;

	for (i = 0; i < len; i++) {
		UART_SendByte(data[i]);
	}
}
//battery-powered water sensor with TyuaMCU request to get somo response
// uartSendHex 55AA0001000000 - this will get reply:
//Info:TuyaMCU:TuyaMCU_ParseQueryProductInformation: received {"p":"j53rkdu55ydc0fkq","v":"1.0.0"}
//
// and this will get states: 0x55 0xAA 0x00 0x02 0x00 0x01 0x04 0x06
// uartSendHex 55AA000200010406
/*Info:MAIN:Time 143, free 88864, MQTT 1, bWifi 1, secondsWithNoPing -1, socks 2/38

Info:TuyaMCU:TUYAMCU received: 55 AA 00 08 00 0C 00 02 02 02 02 02 02 01 04 00 01 00 25

Info:TuyaMCU:TuyaMCU_ProcessIncoming: processing V0 command 8 with 19 bytes

Info:TuyaMCU:TuyaMCU_V0_ParseRealTimeWithRecordStorage: processing dpId 1, dataType 4-DP_TYPE_ENUM and 1 data bytes

Info:TuyaMCU:TuyaMCU_V0_ParseRealTimeWithRecordStorage: raw data 1 byte:
Info:GEN:No change in channel 1 (still set to 0) - ignoring
*/

int TuyaMCU_ParseDPType(const char *dpTypeString) {
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
	else if (!stricmp(dpTypeString, "RAW_DDS238")) {
		// linkTuyaMCUOutputToChannel 6 RAW_DDS238
		dpType = DP_TYPE_RAW_DDS238Packet;
	}
	else if (!stricmp(dpTypeString, "RAW_TAC2121C_VCP")) {
		// linkTuyaMCUOutputToChannel 6 RAW_TAC2121C_VCP
		dpType = DP_TYPE_RAW_TAC2121C_VCP;
	}
	else if (!stricmp(dpTypeString, "RAW_V2C3P3")) {
		dpType = DP_TYPE_RAW_V2C3P3;
	}
	else if (!stricmp(dpTypeString, "RAW_VCPPfF")) {
		dpType = DP_TYPE_RAW_VCPPfF;
	}
	else if (!stricmp(dpTypeString, "RAW_TAC2121C_Yesterday")) {
		dpType = DP_TYPE_RAW_TAC2121C_YESTERDAY;
	}
	else if (!stricmp(dpTypeString, "RAW_TAC2121C_LastMonth")) {
		dpType = DP_TYPE_RAW_TAC2121C_LASTMONTH;
	}
	else if (!stricmp(dpTypeString, "MQTT")) {
		// linkTuyaMCUOutputToChannel 6 MQTT
		dpType = DP_TYPE_PUBLISH_TO_MQTT;
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

commandResult_t TuyaMCU_LinkTuyaMCUOutputToChannel(const void* context, const char* cmd, const char* args, int cmdFlags) {
	const char* dpTypeString;
	byte dpId;
	byte dpType;
	int channelID;
	byte argsCount;
	byte obkFlags;
	float mult, delta, delta2, delta3;
	byte inv;

	// linkTuyaMCUOutputToChannel [dpId] [varType] [channelID] [obkFlags] [mult] [inv] [delta]
	// linkTuyaMCUOutputToChannel 1 val 1
	Tokenizer_TokenizeString(args, 0);

	argsCount = Tokenizer_GetArgsCount();
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	dpId = Tokenizer_GetArgInteger(0);
	dpTypeString = Tokenizer_GetArg(1);
	dpType = TuyaMCU_ParseDPType(dpTypeString);
	if (argsCount < 3) {
		channelID = -999;
	}
	else {
		channelID = Tokenizer_GetArgInteger(2);
	}
	obkFlags = Tokenizer_GetArgInteger(3);
	mult = Tokenizer_GetArgFloatDefault(4, 1.0f);
	inv = Tokenizer_GetArgInteger(5);
	delta = Tokenizer_GetArgFloatDefault(6, 0.0f);
	delta2 = Tokenizer_GetArgFloatDefault(7, 0.0f);
	delta3 = Tokenizer_GetArgFloatDefault(8, 0.0f);

	TuyaMCU_MapIDToChannel(dpId, dpType, channelID, obkFlags, mult, inv, delta, delta2, delta3);

	return CMD_RES_OK;
}

commandResult_t TuyaMCU_Send_SetTime_Current(const void* context, const char* cmd, const char* args, int cmdFlags) {

	TuyaMCU_Send_SetTime(TuyaMCU_Get_NTP_Time(),g_sensorMode);

	return CMD_RES_OK;
}
commandResult_t TuyaMCU_Send_SetTime_Example(const void* context, const char* cmd, const char* args, int cmdFlags) {
	struct tm testTime;

	testTime.tm_year = 2012;
	testTime.tm_mon = 7;
	testTime.tm_mday = 15;
	testTime.tm_wday = 4;
	testTime.tm_hour = 6;
	testTime.tm_min = 54;
	testTime.tm_sec = 32;

	TuyaMCU_Send_SetTime(&testTime,g_sensorMode);
	return CMD_RES_OK;
}

void TuyaMCU_Send(byte* data, int size) {
	int i;
	unsigned char check_sum;

	check_sum = 0;
	for (i = 0; i < size; i++) {
		byte b = data[i];
		check_sum += b;
		UART_SendByte(b);
	}
	UART_SendByte(check_sum);

	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "\nWe sent %i bytes to Tuya MCU\n", size + 1);
}

commandResult_t TuyaMCU_SetDimmerRange(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	g_dimmerRangeMin = Tokenizer_GetArgInteger(0);
	g_dimmerRangeMax = Tokenizer_GetArgInteger(1);

	return CMD_RES_OK;
}

commandResult_t TuyaMCU_SendHeartbeat(const void* context, const char* cmd, const char* args, int cmdFlags) {
	TuyaMCU_SendCommandWithData(TUYA_CMD_HEARTBEAT, NULL, 0);

	return CMD_RES_OK;
}

commandResult_t TuyaMCU_SendQueryProductInformation(const void* context, const char* cmd, const char* args, int cmdFlags) {
	TuyaMCU_SendCommandWithData(TUYA_CMD_QUERY_PRODUCT, NULL, 0);

	return CMD_RES_OK;
}
commandResult_t TuyaMCU_SendQueryState(const void* context, const char* cmd, const char* args, int cmdFlags) {
	TuyaMCU_SendCommandWithData(TUYA_CMD_QUERY_STATE, NULL, 0);

	return CMD_RES_OK;
}

void TuyaMCU_SendStateRawFromString(int dpId, const char *args) {
	int cur = 0;
	byte buffer[64];

	while (*args) {
		if (cur >= sizeof(buffer)) {
			addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "Tuya raw buff overflow");
			return;
		}
		buffer[cur] = CMD_ParseOrExpandHexByte(&args);
		cur++;
	}
	TuyaMCU_SendStateInternal(dpId, DP_TYPE_RAW, buffer, cur);
}
const char *STR_FindArg(const char *s, int arg) {
	while (1) {
		while (isspace((int)*s)) {
			if (*s == 0)
				return "";
			s++;
		}
		arg--;
		if (arg < 0) {
			return s;
		}
		while (isspace((int)*s) == false) {
			if (*s == 0)
				return "";
			s++;
		}
	}
	return 0;
}
// tuyaMcu_sendState id type value
// send boolean true
// tuyaMcu_sendState 25 1 1
// send boolean false
// tuyaMcu_sendState 25 1 0
// send value 87
// tuyaMcu_sendState 25 2 87
// send string 
// tuyaMcu_sendState 25 3 ff0000646464ff 
// send raw 
// tuyaMcu_sendState 25 0 ff$CH3$00646464ff 
// tuyaMcu_sendState 2 0 0404010C$CH9$$CH10$$CH11$FF04FEFF0031
// send enum (type 4)
// tuyaMcu_sendState 14 4 2
commandResult_t TuyaMCU_SendStateCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int dpId;
	int dpType;
	int value;
	const char *valStr;

	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	dpId = Tokenizer_GetArgInteger(0);
	dpType = TuyaMCU_ParseDPType(Tokenizer_GetArg(1)); ;
	if (dpType == DP_TYPE_STRING) {
		valStr = Tokenizer_GetArg(2);
		TuyaMCU_SendState(dpId, DP_TYPE_STRING, (uint8_t*)valStr);
	}
	else if (dpType == DP_TYPE_RAW) {
		//valStr = Tokenizer_GetArg(2);
		valStr = STR_FindArg(args, 2);
		TuyaMCU_SendStateRawFromString(dpId, valStr);
	}
	else {
		value = Tokenizer_GetArgInteger(2);
		TuyaMCU_SendState(dpId, dpType, (uint8_t*)&value);
	}

	return CMD_RES_OK;
}

commandResult_t TuyaMCU_SendMCUConf(const void* context, const char* cmd, const char* args, int cmdFlags) {
	TuyaMCU_SendCommandWithData(TUYA_CMD_MCU_CONF, NULL, 0);

	return CMD_RES_OK;
}

void Tuya_SetWifiState(uint8_t state)
{
	TuyaMCU_SendCommandWithData(TUYA_CMD_WIFI_STATE, &state, 1);
}
void Tuya_SetWifiState_V0(uint8_t state)
{
	TuyaMCU_SendCommandWithData(0x02, &state, 1);
}

void TuyaMCU_SendNetworkStatus()
{
	uint8_t state = TUYA_NETWORK_STATUS_NOT_CONNECTED;
	if (Main_IsOpenAccessPointMode() != 0) {
		state = TUYA_NETWORK_STATUS_AP_MODE;
	}
	else if (Main_HasWiFiConnected() != 0) {
		if (g_cfg.mqtt_host[0] == 0) {
			// if not MQTT configured at all
			state = TUYA_NETWORK_STATUS_CONNECTED_TO_CLOUD;
		} else {
			// otherwise, wait for MQTT connection
			state = Main_HasMQTTConnected() != 0 ? TUYA_NETWORK_STATUS_CONNECTED_TO_CLOUD : TUYA_NETWORK_STATUS_CONNECTED_TO_ROUTER;
		}
	}
	// allow override
	if (state < g_defaultTuyaMCUWiFiState) {
		state = g_defaultTuyaMCUWiFiState;
	}
	addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU, "SendNetworkStatus: sending status 0x%X to MCU \n", state);
	TuyaMCU_SendCommandWithData(0x2B, &state, 1);
}
void TuyaMCU_ForcePublishChannelValues() {
#if ENABLE_MQTT
	tuyaMCUMapping_t* cur;

	cur = g_tuyaMappings;
	while (cur) {
		MQTT_ChannelPublish(cur->channel, 0);
		cur = cur->next;
	}
#endif
}
// ntp_timeZoneOfs 2
// addRepeatingEvent 10 -1 uartSendHex 55AA0008000007
// setChannelType 1 temperature_div10
// setChannelType 2 humidity
// linkTuyaMCUOutputToChannel 1 1
// linkTuyaMCUOutputToChannel 2 2
// addRepeatingEvent 10 -1 tuyaMcu_sendCurTime
//
// same as above but merged
// backlog ntp_timeZoneOfs 2; addRepeatingEvent 10 -1 uartSendHex 55AA0008000007; setChannelType 1 temperature_div10; setChannelType 2 humidity; linkTuyaMCUOutputToChannel 1 1; linkTuyaMCUOutputToChannel 2 2; addRepeatingEvent 10 -1 tuyaMcu_sendCurTime
//
void TuyaMCU_ApplyMapping(tuyaMCUMapping_t* mapping, int dpID, int value) {
	int mappedValue = value;

	// hardcoded values
#if ENABLE_LED_BASIC
	if (dpID == g_tuyaMCUled_id_power) {
		LED_SetEnableAll(value);
	}
	if (dpID == g_tuyaMCUled_id_cw_temperature) {
		float temperatureRange01 = 1.0f - ((value - 10) / 980.0f);
		LED_SetTemperature0to1Range(temperatureRange01);
	}
	if (dpID == g_tuyaMCUled_id_cw_brightness) {
		// TuyaMCU sends in 0-1000 range, we need 0-100
		LED_SetDimmerForDisplayOnly(value*0.1f);
	}
#endif

	if (mapping == 0) {
		addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU, "ApplyMapping: id %i (val %i) not mapped\n", dpID, value);
		return;
	}
	if (mapping->channel < 0) {
		return;
	}
	if (mapping->obkFlags & OBKTM_FLAG_NOREAD) {
		// ignore
		return;
	}

	// map value depending on channel type
	switch (CHANNEL_GetType(mapping->channel))
	{
	case ChType_Dimmer:
		// map TuyaMCU's dimmer range to OpenBK7231T_App's dimmer range 0..100
		mappedValue = ((value - g_dimmerRangeMin) * 100) / (g_dimmerRangeMax - g_dimmerRangeMin);
		break;
	case ChType_Dimmer256:
		// map TuyaMCU's dimmer range to OpenBK7231T_App's dimmer range 0..256
		mappedValue = ((value - g_dimmerRangeMin) * 256) / (g_dimmerRangeMax - g_dimmerRangeMin);
		break;
	case ChType_Dimmer1000:
		// map TuyaMCU's dimmer range to OpenBK7231T_App's dimmer range 0..1000
		mappedValue = ((value - g_dimmerRangeMin) * 1000) / (g_dimmerRangeMax - g_dimmerRangeMin);
		break;
	default:
		break;
	}
	if (mapping->inv) {
		mappedValue = !mappedValue;
	}

	if (value != mappedValue) {
		addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU, "ApplyMapping: mapped dp %i value %d to %d\n", dpID, value, mappedValue);
	}

	mapping->prevValue = mappedValue;

	CHANNEL_Set(mapping->channel, ((mappedValue + mapping->delta) * mapping->mult), 0);
}

bool TuyaMCU_IsChannelUsedByTuyaMCU(int channel) {
	tuyaMCUMapping_t* mapping;

	// find mapping
	mapping = TuyaMCU_FindDefForChannel(channel);

	if (mapping == 0) {
		return false;
	}
	return true;
}
void TuyaMCU_OnChannelChanged(int channel, int iVal) {
	tuyaMCUMapping_t* mapping;
	int mappediVal = iVal;

	// find mapping
	mapping = TuyaMCU_FindDefForChannel(channel);

	if (mapping == 0) {
		return;
	}

	if (mapping->inv) {
		iVal = !iVal;
	}

	// this might be a callback from CHANNEL_Set in TuyaMCU_ApplyMapping. If we should set exactly the
	// same value, skip it
	if (mapping->prevValue == iVal) {
		return;
	}

	// dpCaches are sent when requested - TODO - is it correct?
	if (mapping->obkFlags & OBKTM_FLAG_DPCACHE) {
		return;
	}

	// map value depending on channel type
	switch (CHANNEL_GetType(mapping->channel))
	{
	case ChType_Dimmer:
		// map OpenBK7231T_App's dimmer range 0..100 to TuyaMCU's dimmer range
		mappediVal = (((g_dimmerRangeMax - g_dimmerRangeMin) * iVal) / 100) + g_dimmerRangeMin;
		break;
	case ChType_Dimmer256:
		// map OpenBK7231T_App's dimmer range 0..256 to TuyaMCU's dimmer range
		mappediVal = (((g_dimmerRangeMax - g_dimmerRangeMin) * iVal) / 256) + g_dimmerRangeMin;
		break;
	case ChType_Dimmer1000:
		// map OpenBK7231T_App's dimmer range 0..256 to TuyaMCU's dimmer range
		mappediVal = (((g_dimmerRangeMax - g_dimmerRangeMin) * iVal) / 1000) + g_dimmerRangeMin;
		break;
	default:
		break;
	}

	if (iVal != mappediVal) {
		addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU, "OnChannelChanged: mapped value %d (OpenBK7231T_App range) to %d (TuyaMCU range)\n", iVal, mappediVal);
	}
	// send value to TuyaMCU
	switch (mapping->dpType)
	{
	case DP_TYPE_BOOL:
		TuyaMCU_SendBool(mapping->dpId, mappediVal != 0);
		break;

	case DP_TYPE_ENUM:
		TuyaMCU_SendEnum(mapping->dpId, mappediVal);
		break;

	case DP_TYPE_VALUE:
		TuyaMCU_SendValue(mapping->dpId, mappediVal);
		break;

	default:
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "OnChannelChanged: channel %d: unsupported data point type %d-%s\n", channel, mapping->dpType, TuyaMCU_GetDataTypeString(mapping->dpType));
		break;
	}
	//mapping->prevValue = iVal;
}

void TuyaMCU_ParseQueryProductInformation(const byte* data, int len) {
	char name[256];
	int useLen;

	useLen = len - 1;
	if (useLen > sizeof(name) - 1)
		useLen = sizeof(name) - 1;
	memcpy(name, data, useLen);
	name[useLen] = 0;

	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ParseQueryProductInformation: received %s\n", name);

	if (g_sensorMode) {
		if (g_tuyaBatteryPoweredState == TM0_STATE_AWAITING_INFO) {
			g_tuyaBatteryPoweredState = TM0_STATE_AWAITING_WIFI;
			g_tuyaNextRequestDelay = 0;
		}
	}
}
// See: https://www.elektroda.com/rtvforum/viewtopic.php?p=20345606#20345606
void TuyaMCU_ParseWeatherData(const byte* data, int len) {
	int ofs;
	byte bValid;
	//int checkLen;
	int iValue;
	byte stringLen;
	byte varType;
	char buffer[64];
	const char* stringData;
	//const char *stringDataValue;
	ofs = 0;

	while (ofs + 4 < len) {
		bValid = data[ofs];
		stringLen = data[ofs + 1];
		stringData = (const char*)(data + (ofs + 2));
		if (stringLen >= (sizeof(buffer) - 1))
			stringLen = sizeof(buffer) - 2;
		memcpy(buffer, stringData, stringLen);
		buffer[stringLen] = 0;
		varType = data[ofs + 2 + stringLen];
		// T: 0x00 indicates an integer and 0x01 indicates a string.
		ofs += (2 + stringLen);
		ofs++;
		stringLen = data[ofs];
		stringData = (const char*)(data + (ofs + 1));
		if (varType == 0x00) {
			// integer
			if (stringLen == 4) {
				iValue = data[ofs + 1] << 24 | data[ofs + 2] << 16 | data[ofs + 3] << 8 | data[ofs + 4];
			}
			else if (stringLen == 2) {
				iValue = data[ofs + 1] << 8 | data[ofs + 2];
			}
			else if (stringLen == 1) {
				iValue = data[ofs + 1];
			}
			else {
				iValue = 0;
			}
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ParseWeatherData: key %s, val integer %i\n", buffer, iValue);
		}
		else {
			// string
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ParseWeatherData: key %s, string not yet handled\n", buffer);
		}
		ofs += stringLen;
	}
}



int http_obk_json_dps(int id, void* request, jsonCb_t printer) {
	int i;
	int iCnt = 0;
	char tmp[8];
	tuyaMCUMapping_t* cur;

	printer(request, "[");


	cur = g_tuyaMappings;
	while (cur) {
		if (id == -1 || id == cur->dpId) {
			if (iCnt) {
				printer(request, ",");
			}
			iCnt++;
			printer(request, "{\"id\":%i,\"type\":%i,\"data\":", cur->dpId, cur->dpType);
			if (cur->rawData == 0) {
				printer(request, "0}", cur->rawData);
			}
			else {
				if (cur->dpType == DP_TYPE_BOOL || cur->dpType == DP_TYPE_ENUM
					|| cur->dpType == DP_TYPE_VALUE) {
					if (cur->rawDataLen == 1) {
						i = cur->rawData[0];
					}
					else if (cur->rawDataLen == 4) {
						i = cur->rawData[0] << 24 | cur->rawData[1] << 16 | cur->rawData[2] << 8 | cur->rawData[3];
					}
					else {
						i = 0;
					}
					if (cur->mult != 1.0f) {
						printer(request, "%f}", (float)(i*cur->mult));
					}
					else {
						printer(request, "%i}", i);
					}
				}
				else if (cur->dpType == DP_TYPE_STRING) {
					printer(request, "\"%s\"}", cur->rawData);
				}
				else {
					printer(request, "\"");
					for (i = 0; i < cur->rawDataLen; i++) {
						sprintf(tmp, "%02X", cur->rawData[i]);
						printer(request, "%s", tmp);
					}
					printer(request, "\"}");
				}
			}
		}
		cur = cur->next;
	}


	printer(request, "]");
	return 0;
}

// Protocol version - 0x00 (not 0x03)
// Used for battery powered devices, eg. door sensor.
// Packet ID: 0x08


// When bIncludesDate = true
//55AA 00 08 000C  00 02 02 02 02 02 02 01 01 00 01 01 23
//Head v0 ID lengh bV YY MM DD HH MM SS                CHKSUM
// after that, there are status data uniys
// 01   01   0001   01
// dpId Type Len    Value
//
// When bIncludesDate = false
// 55AA 00 06 0005  10   0100 01 00 1C
// Head v0 ID lengh dpId leen tp vl CHKSUM

void TuyaMCU_V0_ParseRealTimeWithRecordStorage(const byte* data, int len, bool bIncludesDate) {
	int ofs;
	int sectorLen;
	int dpId;
	int dataType;
	tuyaMCUMapping_t* mapping;

	if (bIncludesDate) {
		//data[0]; // bDateValid
		//data[1]; //  year
		//data[2]; //  month
		//data[3]; //  day
		//data[4]; //  hour
		//data[5]; //  minute
		//data[6]; //  second

		ofs = 7;
	}
	else {
		ofs = 0;
	}

	while (ofs + 4 < len) {
		sectorLen = data[ofs + 2] << 8 | data[ofs + 3];
		dpId = data[ofs];
		dataType = data[ofs + 1];
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "V0_ParseRealTimeWithRecordStorage: processing id %i, dataType %i-%s and %i data bytes\n",
			dpId, dataType, TuyaMCU_GetDataTypeString(dataType), sectorLen);

		// find mapping (where to save received data)
		mapping = TuyaMCU_FindDefForID(dpId);

		if (sectorLen == 1) {
			int iVal = (int)data[ofs + 4];
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "V0_ParseRealTimeWithRecordStorage: byte %i\n", iVal);
			// apply to channels
			TuyaMCU_ApplyMapping(mapping, dpId, iVal);
		}
		if (sectorLen == 4) {
			int iVal = data[ofs + 4] << 24 | data[ofs + 5] << 16 | data[ofs + 6] << 8 | data[ofs + 7];
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "V0_ParseRealTimeWithRecordStorage: int32 %i\n", iVal);
			// apply to channels
			TuyaMCU_ApplyMapping(mapping, dpId, iVal);
		}

		// size of header (type, datatype, len 2 bytes) + data sector size
		ofs += (4 + sectorLen);
	}
	if (g_sensorMode) {
		if (g_tuyaBatteryPoweredState == TM0_STATE_AWAITING_STATES) {
			if (bIncludesDate) {
				g_tuyaMCUConfirmationsToSend_0x08++;
			}
			else {
				g_tuyaMCUConfirmationsToSend_0x05++;
			}
			g_tuyaNextRequestDelay++;
		}
	}
}
void TuyaMCU_PublishDPToMQTT(const byte *data, int ofs) {
	int sectorLen;
	int dpId;
	int dataType;
	char sName[32];
	int strLen;
	char *s;
	const byte *payload;
	int index;

	sectorLen = data[ofs + 2] << 8 | data[ofs + 3];
	dpId = data[ofs];
	dataType = data[ofs + 1];

	// really it's just +1 for NULL character but let's keep more space
	strLen = sectorLen * 2 + 16;
	if (g_tuyaMCUpayloadBufferSize < strLen) {
		g_tuyaMCUpayloadBuffer = realloc(g_tuyaMCUpayloadBuffer, strLen);
		g_tuyaMCUpayloadBufferSize = strLen;
	}
	s = (char*)g_tuyaMCUpayloadBuffer;
	*s = 0;

	const char *typeStr;
	typeStr = TuyaMCU_GetDataTypeString(dataType);

	sprintf(sName, "tm/%s/%i", typeStr, dpId);
	payload = data + (ofs + 4);
	switch (dataType) 
	{
	case DP_TYPE_BOOL:
	case DP_TYPE_VALUE:
	case DP_TYPE_ENUM:
		if (sectorLen == 4){
			index = payload[0] << 24 | payload[1] << 16 | payload[2] << 8 | payload[3];
		} else if (sectorLen == 1) {
			index = (int)payload[0];
		}
		else if (sectorLen == 2) {
			index =  payload[1] << 8 | payload[0];
		}
		else {
			index = 0;
		}
		sprintf(s, "%i", index);
		break;
	case DP_TYPE_STRING:
		memcpy(s, payload, sectorLen);
		s[sectorLen] = 0;
		break;
	case DP_TYPE_RAW:
	case DP_TYPE_BITMAP:
		// convert payload bytes string like FFAABB in s
		index = 0;
		if (0) {
			*s = '0';
			s++; index++;
			*s = 'x';
			s++; index++;
		}
		for (int i = 0; i < sectorLen; i++) {
			// convert each byte to two hexadecimal characters
			s[index++] = "0123456789ABCDEF"[(*payload >> 4) & 0xF];
			s[index++] = "0123456789ABCDEF"[*payload & 0xF];
			payload++;
		}
		s[index] = 0; // null-terminate the string
		break;
		
	}
	if (*s == 0)
		return;

#if ENABLE_MQTT
	MQTT_PublishMain_StringString(sName, s, OBK_PUBLISH_FLAG_FORCE_REMOVE_GET);
#endif
}
void TuyaMCU_PublishDPToBerry(const byte *data, int ofs) {
	int sectorLen;
	int dpId;
	int dataType;
	int strLen;
	char *s;
	const byte *payload;
	int index;

	sectorLen = data[ofs + 2] << 8 | data[ofs + 3];
	dpId = data[ofs];
	dataType = data[ofs + 1];

	// really it's just +1 for NULL character but let's keep more space
	strLen = sectorLen * 2 + 16;
	if (g_tuyaMCUpayloadBufferSize < strLen) {
		g_tuyaMCUpayloadBuffer = realloc(g_tuyaMCUpayloadBuffer, strLen);
		g_tuyaMCUpayloadBufferSize = strLen;
	}
	s = (char*)g_tuyaMCUpayloadBuffer;

	payload = data + (ofs + 4);
	switch (dataType)
	{
	case DP_TYPE_BOOL:
	case DP_TYPE_VALUE:
	case DP_TYPE_ENUM:
		if (sectorLen == 4) {
			index = payload[0] << 24 | payload[1] << 16 | payload[2] << 8 | payload[3];
		}
		else if (sectorLen == 1) {
			index = (int)payload[0];
		}
		else if (sectorLen == 2) {
			index = payload[1] << 8 | payload[0];
		}
		else {
			index = 0;
		}
		CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_ON_DP, dpId, index);
		break;
	case DP_TYPE_STRING:
		memcpy(s, payload, sectorLen);
		s[sectorLen] = 0;
		//CMD_Berry_RunEventHandlers_IntStr(CMD_EVENT_ON_DP, dpId, s);
		break;
	case DP_TYPE_RAW:
	case DP_TYPE_BITMAP:
		CMD_Berry_RunEventHandlers_IntBytes(CMD_EVENT_ON_DP, dpId, payload, sectorLen);
		break;

	}
}
#define CALIB_IF_NONZERO(x,d) if(x) { x += d; }

void TuyaMCU_ParseStateMessage(const byte* data, int len) {
	tuyaMCUMapping_t* mapping;
	int ofs;
	int sectorLen;
	int dpId;
	int dataType;
	int day, month, year;
	//int channelType;
	int iVal;

	ofs = 0;

	while (ofs + 4 < len) {
		sectorLen = data[ofs + 2] << 8 | data[ofs + 3];
		dpId = data[ofs];
		dataType = data[ofs + 1];
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ParseState: id %i type %i-%s len %i\n",
			dpId, dataType, TuyaMCU_GetDataTypeString(dataType), sectorLen);

		mapping = TuyaMCU_FindDefForID(dpId);
		if (mapping && mapping->dpType == DP_TYPE_PUBLISH_TO_MQTT) {
			TuyaMCU_PublishDPToMQTT(data, ofs);
		}

#if ENABLE_OBK_BERRY
		TuyaMCU_PublishDPToBerry(data, ofs);
#endif
		if (CFG_HasFlag(OBK_FLAG_TUYAMCU_STORE_RAW_DATA)) {
			if (CFG_HasFlag(OBK_FLAG_TUYAMCU_STORE_ALL_DATA)) {
				if (mapping == 0) {
					mapping = TuyaMCU_MapIDToChannel(dpId, dataType, -1, 0, 1.0f, 0, 0, 0, 0);
				}
			}
			if (mapping) {
				// add space for NULL terminating character
				int useLen = sectorLen + 1;
				if (mapping->rawBufferSize < useLen) {
					mapping->rawData = realloc(mapping->rawData, useLen);
					mapping->rawBufferSize = useLen;
				}
				mapping->rawDataLen = sectorLen;
				memcpy(mapping->rawData, data + ofs + 4, sectorLen);
				// TuyaMCU strings are without NULL terminating character
				mapping->rawData[sectorLen] = 0;
			}
		}

		if (sectorLen == 1) {
			iVal = (int)data[ofs + 4];
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ParseState: byte %i\n", iVal);
			// apply to channels
			TuyaMCU_ApplyMapping(mapping, dpId, iVal);
		}
		else if (sectorLen == 4) {
			iVal = data[ofs + 4] << 24 | data[ofs + 5] << 16 | data[ofs + 6] << 8 | data[ofs + 7];
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ParseState: int32 %i\n", iVal);
			// apply to channels
			TuyaMCU_ApplyMapping(mapping, dpId, iVal);
		}
		else {

			if (mapping != 0) {
				switch (mapping->dpType) {
				case DP_TYPE_RAW_TAC2121C_YESTERDAY:
				{
					if (sectorLen != 8) {

					}
					else {
						month = data[ofs + 4];
						day = data[ofs + 4 + 1];
						// consumption
						iVal = data[ofs + 6 + 4] << 8 | data[ofs + 7 + 4];
						addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TAC2121C_YESTERDAY: day %i, month %i, val %i\n",
							day, month, iVal);

					}
				}
				break;
				case DP_TYPE_RAW_TAC2121C_LASTMONTH:
				{
					if (sectorLen != 8) {

					}
					else {
						year = data[ofs + 4];
						month = data[ofs + 4 + 1];
						// consumption
						iVal = data[ofs + 6 + 4] << 8 | data[ofs + 7 + 4];
						addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "DP_TYPE_RAW_TAC2121C_LASTMONTH: month %i, year %i, val %i\n",
							month, year, iVal);

					}
				}
				break;
				case DP_TYPE_RAW_V2C3P3:
				{
					// see: https://www.elektroda.com/rtvforum/viewtopic.php?p=21569350#21569350
					if (sectorLen == 8) {
						int iV, iC, iP;
						iV = data[ofs + 4] << 8 | data[ofs + 5];
						iC = (data[ofs + 6] << 16) | (data[ofs + 7] << 8) | data[ofs + 8];
						iP = (data[ofs + 9] << 16) | (data[ofs + 10] << 8) | data[ofs + 11];
						// calibration
						CALIB_IF_NONZERO(iV, mapping->delta);
						CALIB_IF_NONZERO(iC, mapping->delta2);
						CALIB_IF_NONZERO(iP, mapping->delta3);
						if (mapping->channel < 0) {
							CHANNEL_SetFirstChannelByType(ChType_Voltage_div10, iV);
							CHANNEL_SetFirstChannelByType(ChType_Current_div1000, iC);
							CHANNEL_SetFirstChannelByType(ChType_Power, iP);
						}
						else {
							CHANNEL_Set(mapping->channel, iV, 0);
							CHANNEL_Set(mapping->channel + 1, iC, 0);
							CHANNEL_Set(mapping->channel + 2, iP, 0);
						}
					}
					else {

					}
				}
				break;
				case DP_TYPE_RAW_VCPPfF:
				{
					if (sectorLen == 15) {
						int iV, iC, iP, iPf, iF;
						// voltage
						iV = data[ofs + 0 + 4] << 8 | data[ofs + 1 + 4];
						// current
						iC = data[ofs + 3 + 4] << 8 | data[ofs + 4 + 4];
						// power
						iP = data[ofs + 6 + 4] << 8 | data[ofs + 7 + 4];
						// pf
						iPf = data[ofs + 11 + 4] << 8 | data[ofs + 12 + 4];
						// freq
						iF = data[ofs + 13 + 4] << 8 | data[ofs + 14 + 4];
						// calibration
						CALIB_IF_NONZERO(iV, mapping->delta);
						CALIB_IF_NONZERO(iC, mapping->delta2);
						CALIB_IF_NONZERO(iP, mapping->delta3);
						if (mapping->channel < 0) {
							CHANNEL_SetFirstChannelByType(ChType_Voltage_div10, iV);
							CHANNEL_SetFirstChannelByType(ChType_Current_div1000, iC);
							CHANNEL_SetFirstChannelByType(ChType_Power, iP);
							CHANNEL_SetFirstChannelByType(ChType_PowerFactor_div1000, iPf);
							CHANNEL_SetFirstChannelByType(ChType_Frequency_div1000, iF);
						}
						else {
							CHANNEL_Set(mapping->channel, iV, 0);
							CHANNEL_Set(mapping->channel + 1, iC, 0);
							CHANNEL_Set(mapping->channel + 2, iP, 0);
							CHANNEL_Set(mapping->channel + 3, iPf, 0);
							CHANNEL_Set(mapping->channel + 4, iF, 0);
						}
					}
					else {

					}
				}
				break;
				case DP_TYPE_RAW_TAC2121C_VCP:
				{
					if (sectorLen == 6) {
						int iV, iC, iP;
						// voltage
						iV = data[ofs + 0 + 4] << 8 | data[ofs + 1 + 4];
						// current
						iC = data[ofs + 2 + 4] << 8 | data[ofs + 3 + 4];
						// power
						iP = data[ofs + 4 + 4] << 8 | data[ofs + 5 + 4];
						// calibration
						CALIB_IF_NONZERO(iV, mapping->delta);
						CALIB_IF_NONZERO(iC, mapping->delta2);
						CALIB_IF_NONZERO(iP, mapping->delta3);
						if (mapping->channel < 0) {
							CHANNEL_SetFirstChannelByType(ChType_Voltage_div10, iV);
							CHANNEL_SetFirstChannelByType(ChType_Current_div10, iC);
							CHANNEL_SetFirstChannelByType(ChType_Power_div10, iP);
						}
						else {
							CHANNEL_Set(mapping->channel, iV, 0);
							CHANNEL_Set(mapping->channel + 1, iC, 0);
							CHANNEL_Set(mapping->channel + 2, iP, 0);
						}
					}
					else if (sectorLen == 8 || sectorLen == 10) {
						int iV, iC, iP;
						// voltage
						iV = data[ofs + 0 + 4] << 8 | data[ofs + 1 + 4];
						// current
						iC = data[ofs + 3 + 4] << 8 | data[ofs + 4 + 4];
						// power
						iP = data[ofs + 6 + 4] << 8 | data[ofs + 7 + 4];
						// calibration
						CALIB_IF_NONZERO(iV, mapping->delta);
						CALIB_IF_NONZERO(iC, mapping->delta2);
						CALIB_IF_NONZERO(iP, mapping->delta3);
						if (mapping->channel < 0) {
							CHANNEL_SetFirstChannelByType(ChType_Voltage_div10, iV);
							CHANNEL_SetFirstChannelByType(ChType_Current_div1000, iC);
							CHANNEL_SetFirstChannelByType(ChType_Power, iP);
						}
						else {
							CHANNEL_Set(mapping->channel, iV, 0);
							CHANNEL_Set(mapping->channel+1, iC, 0);
							CHANNEL_Set(mapping->channel+2, iP, 0);
						}
					}
					else {

					}
				}
				break;
				case DP_TYPE_RAW_DDS238Packet:
				{
					if (sectorLen != 15) {

					}
					else {
						// FREQ??
						iVal = data[ofs + 8 + 4] << 8 | data[ofs + 9 + 4];
						//CHANNEL_SetFirstChannelByType(QQQQQQ, iVal);
						// 06 46 = 1606 => A x 100? ?
						iVal = data[ofs + 11 + 4] << 8 | data[ofs + 12 + 4];
						CHANNEL_SetFirstChannelByType(ChType_Current_div1000, iVal);
						// Voltage?
						iVal = data[ofs + 13 + 4] << 8 | data[ofs + 14 + 4];
						CHANNEL_SetFirstChannelByType(ChType_Voltage_div10, iVal);
					}
				}
				break;
				}
			}
		}

		// size of header (type, datatype, len 2 bytes) + data sector size
		ofs += (4 + sectorLen);
	}

}

int TuyaMCU_WiFiInReset() {
	return g_resetWiFiEvents >= 3;
}

void TuyaMCU_ResetWiFi() {
	g_resetWiFiEvents++;

	if (TuyaMCU_WiFiInReset()) {
		g_openAP = 1;
	}
}
void TuyaMCU_V0_SendDPCacheReply() {
#if 0
	// send empty?
	byte dat = 0x00;
	TuyaMCU_SendCommandWithData(0x10, &dat, 1);
#else
	int dataLen;
	int value;
	int writtenCount;
	byte *p;
	byte buffer[64];
	tuyaMCUMapping_t *map;

	buffer[0] = 0x01; // OK
	buffer[1] = 0x00; // number of variables, will update later

	writtenCount = 0;
	dataLen = 2;
	p = buffer + dataLen;

	map = g_tuyaMappings;
	// packet is the following:
	// 01 02 
	// result 1 (OK), numVars 2
	// 11 02 0004 00000001  		
	// dpId = 17 Len = 0004 Val V = 1
	// 12 02 0004 00000001
	// dpId = 18 Len = 0004 Val V = 1	
	// 
	while (map) {
		if (map->obkFlags & OBKTM_FLAG_DPCACHE) {
			writtenCount++;
			// dpID
			*p = map->dpId;
			p++;
			// type
			*p = map->dpType;
			p++;
			// lenght
			*p = 0;
			p++;
			value = CHANNEL_Get(map->channel);
			switch (map->dpType) {
			case DP_TYPE_ENUM:
				// set len
				*p = 1;
				p++;
				// set data 
				*p = value;
				p++;
				break;
			case DP_TYPE_VALUE:
				// set len
				*p = 4;
				p++;
				// set data 
				p[3] = (value >> 0) & 0xFF; // first byte (lowest bits)
				p[2] = (value >> 8) & 0xFF; // first byte (lowest bits)
				p[1] = (value >> 16) & 0xFF; // first byte (lowest bits)
				p[0] = (value >> 24) & 0xFF; // first byte (lowest bits)
				p += 4;
				break;
			default:
			case DP_TYPE_BOOL:
				// set len
				*p = 1;
				p++;
				// set data 
				*p = value;
				p++;
				break;
			}
		}
		map = map->next;
	}
	dataLen = p - buffer;
	buffer[1] = writtenCount;

	TuyaMCU_SendCommandWithData(0x10, buffer, dataLen);
#endif
}
void TuyaMCU_ParseReportStatusType(const byte *value, int len) {
	int subcommand = value[0];
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "0x%X command, subcommand 0x%X\n", TUYA_CMD_REPORT_STATUS_RECORD_TYPE, subcommand);
	byte reply[2] = { 0x00, 0x00 };
	reply[0] = subcommand;
	switch (subcommand)
	{
	case 0x04:
		// query: 55 AA 03 34 00 01 04 3B
		// reply: 55 aa 00 34 00 02 04 00 39
		// No processing, just reply
		break;
	
	case 0x0B:
		// TuyaMCU version 3 equivalent packet to version 0 0x08 packet
		// This packet includes first DateTime (skip past), then DataUnits
		TuyaMCU_ParseStateMessage(value + 9, len - 9);
		state_updated = true;
		g_sendQueryStatePackets = 0;
		break;

	default:
		// Unknown subcommand, ignore
		return;
	}
	TuyaMCU_SendCommandWithData(TUYA_CMD_REPORT_STATUS_RECORD_TYPE, reply, 2);
}
void TuyaMCU_ProcessIncoming(const byte* data, int len) {
	int checkLen;
	int i;
	byte checkCheckSum;
	byte cmd;
	byte version;

	if (data[0] != 0x55 || data[1] != 0xAA) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ProcessIncoming: discarding packet with bad ident and len %i\n", len);
		return;
	}
	version = data[2];
	checkLen = data[5] | data[4] >> 8;
	checkLen = checkLen + 2 + 1 + 1 + 2 + 1;
	if (checkLen != len) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ProcessIncoming: discarding packet bad expected len, expected %i and got len %i\n", checkLen, len);
		return;
	}
	checkCheckSum = 0;
	for (i = 0; i < len - 1; i++) {
		checkCheckSum += data[i];
	}
	if (checkCheckSum != data[len - 1]) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ProcessIncoming: discarding packet bad expected checksum, expected %i and got checksum %i\n", (int)data[len - 1], (int)checkCheckSum);
		return;
	}
	cmd = data[3];
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ProcessIncoming[v=%i]: cmd %i (%s) len %i\n", version, cmd, TuyaMCU_GetCommandTypeLabel(cmd), len);
	switch (cmd)
	{
	case TUYA_CMD_HEARTBEAT:
		heartbeat_valid = true;
		TuyaMCU_SetHeartbeatCounter(0);
		break;
	case TUYA_CMD_MCU_CONF:
		working_mode_valid = true;
		int dataCount;
		// https://github.com/openshwprojects/OpenBK7231T_App/issues/291
		// header	ver	TUYA_CMD_MCU_CONF	LENGHT							Chksum
		// Pushing
		// 55 AA	01	02					00 03	FF 01 01			06 
		// 55 AA	01	02					00 03	FF 01 00			05 
		// Rotating down
		// 55 AA	01	02					00 05	01 24 02 01 0A		39 
		// 55 AA	01	02					00 03	01 09 00			0F 
		// Rotating up
		// 55 AA	01	02					00 05	01 24 01 01 0A		38 
		// 55 AA	01	02					00 03	01 09 01			10 
		dataCount = data[5];
		if (dataCount == 0)
		{
			self_processing_mode = true;
		}
		else if (dataCount == 2)
		{
			self_processing_mode = false;
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "IMPORTANT!!! mcu conf pins: %i %i",
				(int)(data[6]), (int)(data[7]));
		}
		if (5 + dataCount + 2 != len) {
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ProcessIncoming: TUYA_CMD_MCU_CONF had wrong data lenght?");
		}
		else {
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ProcessIncoming: TUYA_CMD_MCU_CONF, TODO!");
		}
		if (g_sensorMode) {
			if (g_tuyaBatteryPoweredState == TM0_STATE_AWAITING_WIFI) {
				g_tuyaBatteryPoweredState = TM0_STATE_AWAITING_MQTT;
			}
			else if (g_tuyaBatteryPoweredState == TM0_STATE_AWAITING_MQTT) {
				g_tuyaBatteryPoweredState = TM0_STATE_AWAITING_STATES;
				g_tuyaNextRequestDelay = 0;
			}
		}
		break;
	case TUYA_CMD_WIFI_STATE:
		if (g_sensorMode) {
			// for TuyaMCU v0, 0x03 is a pair button hold event
			TuyaMCU_ResetWiFi();
		}
		else {
			wifi_state_valid = true;
		}
		break;
	case TUYA_CMD_WIFI_SELECT:
		{
			// This was added for this user:
			// https://www.elektroda.com/rtvforum/topic3937723.html
			if (version == 0) {
				// 0x05 packet for version 0 (not 0x03) of TuyaMCU
				// This packet has no datetime stamp
				TuyaMCU_V0_ParseRealTimeWithRecordStorage(data + 6, len - 6, false);
			}
			else {
				// TUYA_CMD_WIFI_SELECT
				// it should have 1 payload byte, AP mode or EZ mode, but does it make difference for us?
				g_openAP = 1;
			}
			break;
		}
		break;
	case TUYA_CMD_WIFI_RESET:
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ProcessIncoming: 0x04 replying");
		// added for https://www.elektroda.com/rtvforum/viewtopic.php?p=21095905#21095905
		TuyaMCU_SendCommandWithData(0x04, 0, 0);
		break;
	case 0x22:
		{
			TuyaMCU_ParseStateMessage(data + 6, len - 6);

			byte data23[1] = { 1 };
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ProcessIncoming: 0x22 replying");
			// For example, the module returns 55 aa 00 23 00 01 01 24
			TuyaMCU_SendCommandWithData(0x23, data23, 1);
		}
		break;

		
	case TUYA_CMD_STATE:
		TuyaMCU_ParseStateMessage(data + 6, len - 6);
		state_updated = true;
		g_sendQueryStatePackets = 0;
		break;
	case TUYA_CMD_SET_TIME:
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ProcessIncoming: received TUYA_CMD_SET_TIME, so sending back time");
		TuyaMCU_Send_SetTime(TuyaMCU_Get_NTP_Time(), false);
		break;
		// 55 AA 00 01 00 ${"p":"e7dny8zvmiyhqerw","v":"1.0.0"}$
		// uartFakeHex 55AA000100247B2270223A226537646E79387A766D69796871657277222C2276223A22312E302E30227D24
	case TUYA_CMD_QUERY_PRODUCT:
		TuyaMCU_ParseQueryProductInformation(data + 6, len - 6);
		product_information_valid = true;
		break;
		// this name seems invalid for Version 0 of TuyaMCU
	case TUYA_CMD_QUERY_STATE:
		if (version == 0) {
			// 0x08 packet for version 0 (not 0x03) of TuyaMCU
			// This packet includes first a DateTime, then RealTimeDataStorage
			TuyaMCU_V0_ParseRealTimeWithRecordStorage(data + 6, len - 6, true);
		}
		else {

		}
		break;
	case TUYA_V0_CMD_QUERYSIGNALSTRENGTH:
		if (version == 0) {
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ProcessIncoming: received TUYA_V0_CMD_QUERYSIGNALSTRENGTH, so sending back signal");

			TuyaMCU_Send_SignalStrength(1,80);
		}
		else {

		}
		break;
	case TUYA_V0_CMD_OBTAINLOCALTIME:
		if (version == 0) {
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ProcessIncoming: received TUYA_V0_CMD_OBTAINLOCALTIME, so sending back time");
			TuyaMCU_Send_SetTime(TuyaMCU_Get_NTP_Time(),true);
		}
		else {

		}
		break;
	case TUYA_V0_CMD_OBTAINDPCACHE:
		// This is sent by TH01
		// Info:TuyaMCU:TUYAMCU received: 55 AA 00 10 00 02 01 09 1B
	{
		TuyaMCU_V0_SendDPCacheReply();
	}
	break;
	case TUYA_CMD_WEATHERDATA:
		TuyaMCU_ParseWeatherData(data + 6, len - 6);
		break;

	case TUYA_CMD_SET_RSSI:
		// This is send by TH06, S09
		// Info:TuyaMCU:TUYAMCU received: 55 AA 03 24 00 00 26
		if (version == 3) {
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ProcessIncoming: received TUYA_CMD_SET_RSSI, so sending back signal strength");
			TuyaMCU_Send_RSSI(HAL_GetWifiStrength());
		}
		break;
	case TUYA_CMD_REPORT_STATUS_RECORD_TYPE:
		//This is sent by https://www.elektroda.pl/rtvforum/viewtopic.php?p=20941591#20941591
		//Info:TuyaMCU:TUYAMCU received: 55 AA 03 34 00 01 04 3B
		// uartFakeHex 55AA03340001043B
		if (version == 3) {
			TuyaMCU_ParseReportStatusType(data + 6, len - 6);
		}
		break;
	case TUYA_CMD_NETWORK_STATUS:
		//This is sent by S09
		//Info:TuyaMCU:TUYAMCU received: 55 AA 03 2B 00 00 2D
		//
		if (version == 3) {
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ProcessIncoming: (test for S09 calendar/IR device) received TUYA_CMD_NETWORK_STATUS 0x2B ");
			TuyaMCU_SendNetworkStatus();
		}
		break;
	default:
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "ProcessIncoming: unhandled type %i", cmd);
		break;
	}
	EventHandlers_FireEvent(CMD_EVENT_TUYAMCU_PARSED, cmd);
}
// tuyaMcu_sendCmd 0x30 000000
// This will send 55 AA 00 30 00 03 00 00 00 32
// 
commandResult_t TuyaMCU_SendUserCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	byte packet[128];
	int c = 0;

	Tokenizer_TokenizeString(args, 0);

	int command = Tokenizer_GetArgInteger(0);
	//XJIKKA 20250327 tuyaMcu_sendCmd without second param bug
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
	TuyaMCU_SendCommandWithData(command,packet, c);
	return CMD_RES_OK;
}

commandResult_t TuyaMCU_FakePacket(const void* context, const char* cmd, const char* args, int cmdFlags) {
	byte packet[256];
	int c = 0;
	if (!(*args)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "FakePacket: requires 1 argument (hex string, like FFAABB00CCDD\n");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	while (*args) {
		byte b;
		b = hexbyte(args);

		if (sizeof(packet) > c + 1) {
			packet[c] = b;
			c++;
		}
		args += 2;
	}
	TuyaMCU_ProcessIncoming(packet, c);
	return CMD_RES_OK;
}

commandResult_t Cmd_TuyaMCU_SetBatteryAckDelay(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int delay;

	Tokenizer_TokenizeString(args, 0);
	Tokenizer_CheckArgsCountAndPrintWarning(args, 1);

	delay = Tokenizer_GetArgInteger(0);

	if (!Tokenizer_IsArgInteger(0)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "SetBatteryAckDelay: requires 1 argument [delay in seconds]\n");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	if (delay < 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "SetBatteryAckDelay: delay must be positive\n");
		return CMD_RES_BAD_ARGUMENT;
	}

	if (delay > 60) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "SetBatteryAckDelay: delay too long, max 60 seconds\n");
		return CMD_RES_BAD_ARGUMENT;
	}

	g_tuyaMCUBatteryAckDelay = delay;

	return CMD_RES_OK;
}

commandResult_t Cmd_TuyaMCU_EnableAutoSend(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int enable;

	Tokenizer_TokenizeString(args, 0);
	Tokenizer_CheckArgsCountAndPrintWarning(args, 1);

	enable = Tokenizer_GetArgInteger(0);

	if (!Tokenizer_IsArgInteger(0)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "EnableAutoSend: requires 1 argument [0/1]\n");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	if (enable != 0 && enable != 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "EnableAutoSend: argument must be 0 or 1\n");
		return CMD_RES_BAD_ARGUMENT;
	}

	TuyaMCU_EnableAutomaticSending(enable != 0);
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU automatic sending %s\n", enable ? "enabled" : "disabled");

	return CMD_RES_OK;
}

commandResult_t Cmd_TuyaMCU_BatteryPoweredMode(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int enable = 1;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() > 0) {
		enable = Tokenizer_GetArgInteger(0);
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU power saving %s\n", enable ? "enabled" : "disabled");
	TuyaMCU_BatteryPoweredMode(enable != 0);

	return CMD_RES_OK;
}

void TuyaMCU_RunWiFiUpdateAndPackets() {
	//addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"WifiCheck %d ", wifi_state_timer);
	/* Monitor WIFI and MQTT connection and apply Wifi state
	 * State is updated when change is detected or after timeout */
	if ((Main_HasWiFiConnected() != 0) && (Main_HasMQTTConnected() != 0))
	{
		if ((wifi_state == false) || (wifi_state_timer == 0))
		{
			addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_TUYAMCU, "Will send SetWiFiState 4.\n");
			Tuya_SetWifiState(4);
			wifi_state = true;
			wifi_state_timer++;
		}
	}
	else {
		if ((wifi_state == true) || (wifi_state_timer == 0))
		{
			addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_TUYAMCU, "Will send SetWiFiState %i.\n", (int)g_defaultTuyaMCUWiFiState);

			Tuya_SetWifiState(g_defaultTuyaMCUWiFiState);
			wifi_state = false;
			wifi_state_timer++;
		}
	}
	/* wifi state timer */
	if (wifi_state_timer > 0)
		wifi_state_timer++;
	if (wifi_state_timer >= 60)
	{
		/* timeout after ~1 minute */
		wifi_state_timer = 0;
		//addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"Wifi_State timer");
	}
}
void TuyaMCU_PrintPacket(byte *data, int len) {
	int i;
	char buffer_for_log[256];
	char buffer2[4];
	
			buffer_for_log[0] = 0;
			for (i = 0; i < len; i++) {
				snprintf(buffer2, sizeof(buffer2), "%02X ", data[i]);
				strcat_safe(buffer_for_log, buffer2, sizeof(buffer_for_log));
			}
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "Received: %s\n", buffer_for_log);
#if 1
			// redo sprintf without spaces
			buffer_for_log[0] = 0;
			for (i = 0; i < len; i++) {
				snprintf(buffer2, sizeof(buffer2), "%02X", data[i]);
				strcat_safe(buffer_for_log, buffer2, sizeof(buffer_for_log));
			}
			// fire string event, as we already have it sprintfed
			// This is so we can have event handlers that fire
			// when an UART string is received...
			EventHandlers_FireEvent_String(CMD_EVENT_ON_UART, buffer_for_log);
#endif
}
void TuyaMCU_RunReceive() {
	byte data[192];
	int len;
	while (1)
	{
		len = UART_TryToGetNextTuyaPacket(data, sizeof(data));
		if (len > 0) {
			TuyaMCU_PrintPacket(data,len);
			TuyaMCU_ProcessIncoming(data, len);
		}
		else {
			break;
		}
	}
}
void TuyaMCU_RunStateMachine_V3() {

	/* For power saving mode */
	/* Devices are powered by the TuyaMCU, transmit information and get turned off */
	/* Use the minimal amount of communications */
	if (g_tuyaMCU_batteryPoweredMode) {
		/* Don't worry about connection after state is updated device will be turned off */
		if (!state_updated) {
			/* Don't send heartbeats just work on product information */
			heartbeat_valid = true;
			if (product_information_valid == false)
			{
				addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_TUYAMCU, "Will send TUYA_CMD_QUERY_PRODUCT.\n");
				/* Request production information */
				TuyaMCU_SendCommandWithData(TUYA_CMD_QUERY_PRODUCT, NULL, 0);
			}
			else 
			{
				/* Don't bother with MCU config */
				working_mode_valid = true;
				/* No query state. Will be updated when connected to wifi/mqtt */
				TuyaMCU_RunWiFiUpdateAndPackets();
			}
		}
		return;
	}

	//addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"UART ring buffer state: %i %i\n",g_recvBufIn,g_recvBufOut);

	// extraDebug log level
	addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_TUYAMCU, "TuyaMCU heartbeat_valid = %i, product_information_valid=%i,"
		" self_processing_mode = %i, wifi_state_valid = %i, wifi_state_timer=%i\n",
		(int)heartbeat_valid, (int)product_information_valid, (int)self_processing_mode,
		(int)wifi_state_valid, (int)wifi_state_timer);
	/* Command controll */
	if (heartbeat_timer == 0)
	{
		/* Generate heartbeat to keep communication alove */
		TuyaMCU_SendCommandWithData(TUYA_CMD_HEARTBEAT, NULL, 0);
		heartbeat_timer = 3;
		TuyaMCU_SetHeartbeatCounter(heartbeat_counter+1);
		if (heartbeat_counter >= 4)
		{
			/* unanswerred heartbeats -> lost communication */
			heartbeat_valid = false;
			product_information_valid = false;
			working_mode_valid = false;
			wifi_state_valid = false;
			state_updated = false;
			g_sendQueryStatePackets = 0;
		}
		//addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "WFS: %d H%d P%d M%d W%d S%d", wifi_state_timer,
		//        heartbeat_valid, product_information_valid, working_mode_valid, wifi_state_valid,
		//        state_updated);
	}
	else {
		/* Heartbeat timer - sent every 3 seconds */
		if (heartbeat_timer > 0)
		{
			heartbeat_timer--;
		}
		else {
			heartbeat_timer = 0;
		}
		if (heartbeat_valid == true)
		{
			/* Connection Active */
			if (product_information_valid == false)
			{
				addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_TUYAMCU, "Will send TUYA_CMD_QUERY_PRODUCT.\n");
				/* Request production information */
				TuyaMCU_SendCommandWithData(TUYA_CMD_QUERY_PRODUCT, NULL, 0);
			}
			else if (working_mode_valid == false)
			{
				addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_TUYAMCU, "Will send TUYA_CMD_MCU_CONF.\n");
				/* Request working mode */
				TuyaMCU_SendCommandWithData(TUYA_CMD_MCU_CONF, NULL, 0);
			}
			else if ((wifi_state_valid == false) && (self_processing_mode == false))
			{
				/* Reset wifi state -> Aquirring network connection */
				Tuya_SetWifiState(g_defaultTuyaMCUWiFiState);
				addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_TUYAMCU, "Will send TUYA_CMD_WIFI_STATE.\n");
				TuyaMCU_SendCommandWithData(TUYA_CMD_WIFI_STATE, NULL, 0);
			}
			else if (state_updated == false)
			{
				// fix for this device getting stuck?
				// https://www.elektroda.com/rtvforum/topic3936455.html
				if (g_sendQueryStatePackets > 1 && (g_sendQueryStatePackets % 2 == 0)) {
					TuyaMCU_RunWiFiUpdateAndPackets();
				}
				else {
					/* Request first state of all DP - this should list all existing DP */
					addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_TUYAMCU, "Will send TUYA_CMD_QUERY_STATE (state_updated==false, try %i).\n",
						g_sendQueryStatePackets);
					TuyaMCU_SendCommandWithData(TUYA_CMD_QUERY_STATE, NULL, 0);
				}
				g_sendQueryStatePackets++;
			}
			else
			{
				TuyaMCU_RunWiFiUpdateAndPackets();
			}
		}
	}
}
void TuyaMCU_RunStateMachine_BatteryPowered() {
	if (TuyaMCU_WiFiInReset()) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "User requested Reset - Operating Open AP\n");
		/* Set current state to Setup */
		Tuya_SetWifiState_V0(TUYA_NETWORK_STATUS_SMART_CONNECT_SETUP);

		/* Don't interact further with MCU when in reset */
		return;
	}

	g_tuyaNextRequestDelay--;
	switch (g_tuyaBatteryPoweredState) {
	case TM0_STATE_AWAITING_INFO:
		if (g_tuyaNextRequestDelay <= 0) {
			TuyaMCU_Send_RawBuffer(g_hello, sizeof(g_hello));
			// retry 
			g_tuyaNextRequestDelay = 3;
		}
		break;
	case TM0_STATE_AWAITING_WIFI:
		if (g_tuyaNextRequestDelay <= 0) {
			if (Main_IsConnectedToWiFi()) {
				Tuya_SetWifiState_V0(TUYA_NETWORK_STATUS_CONNECTED_TO_ROUTER);
				// retry
				g_tuyaNextRequestDelay = 3;
			}
		}
		break;
	case TM0_STATE_AWAITING_MQTT:
		if (g_tuyaNextRequestDelay <= 0) {
			
#if WINDOWS
			if (true)
#else
			if (Main_HasMQTTConnected()
				||
				g_defaultTuyaMCUWiFiState == 0x04
				||
				g_cfg.mqtt_host[0] == 0
				)
#endif
			{
				Tuya_SetWifiState_V0(TUYA_NETWORK_STATUS_CONNECTED_TO_CLOUD);
				// retry
				g_tuyaNextRequestDelay = 3;
			}
		}
		break;
	case TM0_STATE_AWAITING_STATES:
		if (g_tuyaNextRequestDelay <= 0) {
			if (g_tuyaMCUBatteryAckDelay > 0) {
				g_tuyaMCUBatteryAckDelay--;
				break;
			}

			byte dat = 0x00;
			if (g_tuyaMCUConfirmationsToSend_0x08 > 0) {
				g_tuyaMCUConfirmationsToSend_0x08--;
				TuyaMCU_SendCommandWithData(0x08, &dat, 1);
				g_tuyaNextRequestDelay = 2;
			}
			else if (g_tuyaMCUConfirmationsToSend_0x05 > 0) {
				g_tuyaMCUConfirmationsToSend_0x05--;
				TuyaMCU_SendCommandWithData(0x05, &dat, 1);
				g_tuyaNextRequestDelay = 2;
			}
			break;
		}
	}
}
int timer_send = 0;
void TuyaMCU_RunFrame() {
	TuyaMCU_RunReceive();


	if (timer_send > 0) {
		timer_send -= g_deltaTimeMS;
	}
	else {
		if (TUYAMCU_SendFromQueue()) {
			timer_send = 100;
		}
	}
}
void TuyaMCU_RunEverySecond() {
	g_sensorMode = DRV_IsRunning("tmSensor");
	if (g_sensorMode) {
		TuyaMCU_RunStateMachine_BatteryPowered();
	}
	else {
		TuyaMCU_RunStateMachine_V3();
	}
}




commandResult_t TuyaMCU_SetBaudRate(const void* context, const char* cmd, const char* args, int cmdFlags) {
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

static SemaphoreHandle_t g_mutex = 0;
static int g_previousLEDPower = -1;

void TuyaMCU_EnableAutomaticSending(bool enable) {
	g_tuyaMCU_allowAutomaticSending = enable;
}

void TuyaMCU_BatteryPoweredMode(bool enable) {
	g_tuyaMCU_batteryPoweredMode = enable;
}

void TuyaMCU_OnRGBCWChange(const float *rgbcw, int bLightEnableAll, int iLightMode, float brightnessRange01, float temperatureRange01) {
	if (g_tuyaMCUled_id_color == -1) {
		return;
	}
	
	// Only send commands if automatic sending is enabled
	if (!g_tuyaMCU_allowAutomaticSending) {
		return;
	}
	if (g_mutex == 0)
	{
		g_mutex = xSemaphoreCreateMutex();
	}
	// temporary fix?
	bool taken = xSemaphoreTake(g_mutex, 1000);
	if (taken == false) {
		return;
	}
	if (g_previousLEDPower != bLightEnableAll) {
		rtos_delay_milliseconds(50);
		// dpID 20: switch light on and off
		TuyaMCU_SendBool(g_tuyaMCUled_id_power, bLightEnableAll);
		g_previousLEDPower = bLightEnableAll;
	}
	if (bLightEnableAll == false) {
		xSemaphoreGive(g_mutex);
		// nothing else to do!
		return;
	}
	rtos_delay_milliseconds(50);
	if (iLightMode == Light_RGB) {
		// dpID 21: switch between RGB and white mode : 0->white, 1->RGB
		TuyaMCU_SendBool(21, 1);
		rtos_delay_milliseconds(50);

		// dpID 24: color(and indirectly brightness) in RGB mode, handled by your code already fine
		TuyaMCU_SendColor(g_tuyaMCUled_id_color, rgbcw[0] / 255.0f, rgbcw[1] / 255.0f, rgbcw[2] / 255.0f, g_tuyaMCUled_format);
	}
	else {
		// dpID 21: switch between RGB and white mode : 0->white, 1->RGB
		//TuyaMCU_SendBool(21, 0);
		// dpID 22: brightness only in white mode: The range is from 50 (dark) to 1000 (light), 
		// NOTE: when changing this value, dpID 21 is set to 0 -> white automatically
		int mcu_brightness = brightnessRange01 * 1000.0f;
		TuyaMCU_SendValue(g_tuyaMCUled_id_cw_brightness, mcu_brightness);
		rtos_delay_milliseconds(50);
		// dpID 23: Kelvin value of white : The range is from 10 (ww)to 990 (cw), 
		// NOTE : when changing this value, dpID 21 is set to 0->white automatically
		int mcu_temperature = 10 + (1.0f - temperatureRange01) * 980.0f;
		TuyaMCU_SendValue(g_tuyaMCUled_id_cw_temperature, mcu_temperature);
		//TuyaMCU_SendTwoVals(22, mcu_brightness, 23, mcu_temperature);
	}
	xSemaphoreGive(g_mutex);


}
bool TuyaMCU_IsLEDRunning() {
	if (g_tuyaMCUled_id_color == -1)
		return false;
	return true;
}
void TuyaMCU_Shutdown() {
	tuyaMCUMapping_t *tmp, *nxt;
	tuyaMCUPacket_t *packet, *next_packet;

	// free the tuyaMCUMapping_t linked list
	tmp = g_tuyaMappings;
	while (tmp) {
		nxt = tmp->next;
		// free rawData if allocated
		if (tmp->rawData) {
			free(tmp->rawData);
			tmp->rawData = NULL;
			tmp->rawBufferSize = 0;
			tmp->rawDataLen = 0;
		}
		free(tmp);
		tmp = nxt;
	}
	g_tuyaMappings = NULL;

	// free the tuyaMCUpayloadBuffer
	if (g_tuyaMCUpayloadBuffer) {
		free(g_tuyaMCUpayloadBuffer);
		g_tuyaMCUpayloadBuffer = NULL;
		g_tuyaMCUpayloadBufferSize = 0;
	}

	// free the tm_emptyPackets queue
	packet = tm_emptyPackets;
	while (packet) {
		next_packet = packet->next;
		if (packet->data) {
			free(packet->data);
			packet->data = NULL;
			packet->allocated = 0;
			packet->size = 0;
		}
		free(packet);
		packet = next_packet;
	}
	tm_emptyPackets = NULL;

	// free the tm_sendPackets queue
	packet = tm_sendPackets;
	while (packet) {
		next_packet = packet->next;
		if (packet->data) {
			free(packet->data);
			packet->data = NULL;
			packet->allocated = 0;
			packet->size = 0;
		}
		free(packet);
		packet = next_packet;
	}
	tm_sendPackets = NULL;

	// free the mutex
	if (g_mutex) {
		//vSemaphoreDelete(g_mutex);
		g_mutex = NULL;
	}
}
void TuyaMCU_Init()
{
	g_resetWiFiEvents = 0;
	g_tuyaNextRequestDelay = 1;
	g_tuyaBatteryPoweredState = 0;
	g_tuyaMCUConfirmationsToSend_0x05 = 0;
	g_tuyaMCUConfirmationsToSend_0x08 = 0;
	if (g_tuyaMCUpayloadBuffer == 0) {
		g_tuyaMCUpayloadBufferSize = TUYAMCU_BUFFER_SIZE;
		g_tuyaMCUpayloadBuffer = (byte*)malloc(TUYAMCU_BUFFER_SIZE);
	}

	UART_InitUART(g_baudRate, 0, false);
	UART_InitReceiveRingBuffer(1024);
	// uartSendHex 55AA0008000007
	//cmddetail:{"name":"tuyaMcu_testSendTime","args":"",
	//cmddetail:"descr":"Sends a example date by TuyaMCU to clock/callendar MCU",
	//cmddetail:"fn":"TuyaMCU_Send_SetTime_Example","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_testSendTime", TuyaMCU_Send_SetTime_Example, NULL);
	//cmddetail:{"name":"tuyaMcu_sendCurTime","args":"",
	//cmddetail:"descr":"Sends a current date by TuyaMCU to clock/callendar MCU. Time is taken from NTP driver, so NTP also should be already running.",
	//cmddetail:"fn":"TuyaMCU_Send_SetTime_Current","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_sendCurTime", TuyaMCU_Send_SetTime_Current, NULL);
	//cmddetail:{"name":"linkTuyaMCUOutputToChannel","args":"[dpId][varType][channelID][obkFlags-Optional][mult-optional][bInverse-Optional][delta-Optional][delta2][delta3]",
	//cmddetail:"descr":"Used to map between TuyaMCU dpIDs and our internal channels. Mult, inverse and delta are for calibration, they are optional. obkFlags is also optional, you can set it to 1 for battery powered devices, so a variable is set with DPCache, for example a sampling interval for humidity/temperature sensor. Mapping works both ways. DpIDs are per-device, you can get them by sniffing UART communication. Vartypes can also be sniffed from Tuya. VarTypes can be following: 0-raw, 1-bool, 2-value, 3-string, 4-enum, 5-bitmap. Please see [Tuya Docs](https://developer.tuya.com/en/docs/iot/tuya-cloud-universal-serial-port-access-protocol?id=K9hhi0xxtn9cb) for info about TuyaMCU. You can also see our [TuyaMCU Analyzer Tool](https://www.elektroda.com/rtvforum/viewtopic.php?p=20528459#20528459)",
	//cmddetail:"fn":"TuyaMCU_LinkTuyaMCUOutputToChannel","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("linkTuyaMCUOutputToChannel", TuyaMCU_LinkTuyaMCUOutputToChannel, NULL);
	//cmddetail:{"name":"tuyaMcu_setDimmerRange","args":"[Min][Max]",
	//cmddetail:"descr":"Set dimmer range used by TuyaMCU",
	//cmddetail:"fn":"TuyaMCU_SetDimmerRange","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_setDimmerRange", TuyaMCU_SetDimmerRange, NULL);
	//cmddetail:{"name":"tuyaMcu_sendHeartbeat","args":"",
	//cmddetail:"descr":"Send heartbeat to TuyaMCU",
	//cmddetail:"fn":"TuyaMCU_SendHeartbeat","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_sendHeartbeat", TuyaMCU_SendHeartbeat, NULL);
	//cmddetail:{"name":"tuyaMcu_sendQueryState","args":"",
	//cmddetail:"descr":"Send query state command. No arguments needed.",
	//cmddetail:"fn":"TuyaMCU_SendQueryState","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_sendQueryState", TuyaMCU_SendQueryState, NULL);
	//cmddetail:{"name":"tuyaMcu_sendProductInformation","args":"",
	//cmddetail:"descr":"Send query packet (0x01). No arguments needed.",
	//cmddetail:"fn":"TuyaMCU_SendQueryProductInformation","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_sendProductInformation", TuyaMCU_SendQueryProductInformation, NULL);
	//cmddetail:{"name":"tuyaMcu_sendState","args":"[dpID][dpType][dpValue]",
	//cmddetail:"descr":"Manually send set state command. Do not use it. Use mapping, so communication is bidirectional and automatic.",
	//cmddetail:"fn":"TuyaMCU_SendStateCmd","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_sendState", TuyaMCU_SendStateCmd, NULL);
	//cmddetail:{"name":"tuyaMcu_sendMCUConf","args":"",
	//cmddetail:"descr":"Send MCU conf command",
	//cmddetail:"fn":"TuyaMCU_SendMCUConf","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_sendMCUConf", TuyaMCU_SendMCUConf, NULL);
	//cmddetail:{"name":"tuyaMcu_sendCmd","args":"[CommandIndex] [HexPayloadNBytes]",
	//cmddetail:"descr":"This will automatically calculate TuyaMCU checksum and length for given command ID and payload, then it will send a command. It's better to use it than uartSendHex",
	//cmddetail:"fn":"TuyaMCU_SendUserCmd","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_sendCmd", TuyaMCU_SendUserCmd, NULL);
	//cmddetail:{"name":"fakeTuyaPacket","args":"[HexString]",
	//cmddetail:"descr":"This simulates packet being sent from TuyaMCU to our OBK device.",
	//cmddetail:"fn":"TuyaMCU_FakePacket","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("fakeTuyaPacket", TuyaMCU_FakePacket, NULL);
	//cmddetail:{"name":"tuyaMcu_setBaudRate","args":"[BaudValue]",
	//cmddetail:"descr":"Sets the baud rate used by TuyaMCU UART communication. Default value is 9600. Some other devices require 115200.",
	//cmddetail:"fn":"TuyaMCU_SetBaudRate","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_setBaudRate", TuyaMCU_SetBaudRate, NULL);
	//cmddetail:{"name":"tuyaMcu_sendRSSI","args":"",
	//cmddetail:"descr":"Command sends the specific RSSI value to TuyaMCU (it will send current RSSI if no argument is set)",
	//cmddetail:"fn":"Cmd_TuyaMCU_Send_RSSI","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_sendRSSI", Cmd_TuyaMCU_Send_RSSI, NULL);
	//cmddetail:{"name":"tuyaMcu_defWiFiState","args":"",
	//cmddetail:"descr":"Command sets the default WiFi state for TuyaMCU when device is not online. It may be required for some devices to work, because Tuya designs them to ignore touch buttons or beep when not paired. Please see [values table and description here](https://www.elektroda.com/rtvforum/viewtopic.php?p=20483899#20483899).",
	//cmddetail:"fn":"Cmd_TuyaMCU_Set_DefaultWiFiState","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_defWiFiState", Cmd_TuyaMCU_Set_DefaultWiFiState, NULL);

	//cmddetail:{"name":"tuyaMcu_sendColor","args":"[dpID] [red01] [green01] [blue01] [tuyaRGBformat]",
	//cmddetail:"descr":"This sends a TuyaMCU color string of given format for given RGB values, where each value is in [0,1] range as float",
	//cmddetail:"fn":"Cmd_TuyaMCU_SendColor","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_sendColor", Cmd_TuyaMCU_SendColor, NULL);

	//cmddetail:{"name":"tuyaMcu_setupLED","args":"[dpIDColor] [TasFormat] [dpIDPower]",
	//cmddetail:"descr":"Setups the TuyaMCU LED driver for given color dpID and power dpID. Also allows you to choose color format.",
	//cmddetail:"fn":"Cmd_TuyaMCU_SetupLED","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_setupLED", Cmd_TuyaMCU_SetupLED, NULL);

	//cmddetail:{"name":"tuyaMcu_setBatteryAckDelay","args":"[ackDelay]",
	//cmddetail:"descr":"Defines the delay before the ACK is sent to TuyaMCU sending the device to sleep. Default value is 0 seconds.",
	//cmddetail:"fn":"Cmd_TuyaMCU_SetBatteryAckDelay","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_setBatteryAckDelay", Cmd_TuyaMCU_SetBatteryAckDelay, NULL);
	
	//cmddetail:{"name":"tuyaMcu_enableAutoSend","args":"[0/1]",
	//cmddetail:"descr":"Enable or disable automatic sending of commands to TuyaMCU during boot/initialization. Use 0 to disable, 1 to enable.",
	//cmddetail:"fn":"Cmd_TuyaMCU_EnableAutoSend","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":"tuyaMcu_enableAutoSend 0"}
	CMD_RegisterCommand("tuyaMcu_enableAutoSend", Cmd_TuyaMCU_EnableAutoSend, NULL);

	//cmddetail:{"name":"tuyaMcu_batteryPoweredMode","args": "[Optional 1 or 0, by default 1 is assumed]",
	//cmddetail:"descr":"Enables battery mode communications for version 3 TuyaMCU. tuyaMcu_batteryPoweredMode 0 can be used to disable the mode.",
	//cmddetail:"fn":"Cmd_TuyaMCU_BatteryPoweredMode","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":"tuyaMcu_batteryPoweredMode"}
	CMD_RegisterCommand("tuyaMcu_batteryPoweredMode", Cmd_TuyaMCU_BatteryPoweredMode, NULL);
}



// Door sensor with TuyaMCU version 0 (not 3), so all replies have x00 and not 0x03 byte
// fakeTuyaPacket 55AA0008000C00010101010101030400010223
// fakeTuyaPacket 55AA0008000C00020202020202010100010123
// https://developer.tuya.com/en/docs/iot/tuyacloudlowpoweruniversalserialaccessprotocol?id=K95afs9h4tjjh
/// 55AA 00 08 000C 00 02 0202 020202010100010123
/// head vr id size dp dt dtln ????
// https://github.com/esphome/feature-requests/issues/497
/// 55AA 00 08 000C 00 02 02 02 02 02 02 01 01 0001 01 23
/// head vr id size FL YY MM DD HH MM SS ID TP SIZE VL CK
/// 55AA 00 08 000C 00 01 01 01 01 01 01 03 04 0001 02 23
// TP = 0x01    bool    1   Value range: 0x00/0x01.
// TP = 0x04    enum    1   Enumeration type, ranging from 0 to 255.


