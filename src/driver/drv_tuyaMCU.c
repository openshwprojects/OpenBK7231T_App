
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
#include "drv_ntp.h"


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

#define TUYA_NETWORK_STATUS_AP_MODE             0x01
#define TUYA_NETWORK_STATUS_NOT_CONNECTED       0x02
#define TUYA_NETWORK_STATUS_CONNECTED_TO_ROUTER 0x03
#define TUYA_NETWORK_STATUS_CONNECTED_TO_CLOUD  0x04
#define TUYA_NETWORK_STATUS_LOW_POWER_MODE      0x05

void TuyaMCU_RunFrame();

static byte g_tuyaMCUConfirmationsToSend_0x08 = 0;
static byte g_tuyaMCUConfirmationsToSend_0x05 = 0;
static byte g_resetWiFiEvents = 0;



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

const char* TuyaMCU_GetDataTypeString(int dpId) {
	if (DP_TYPE_RAW == dpId)
		return "DP_TYPE_RAW";
	if (DP_TYPE_BOOL == dpId)
		return "DP_TYPE_BOOL";
	if (DP_TYPE_VALUE == dpId)
		return "DP_TYPE_VALUE";
	if (DP_TYPE_STRING == dpId)
		return "DP_TYPE_STRING";
	if (DP_TYPE_ENUM == dpId)
		return "DP_TYPE_ENUM";
	if (DP_TYPE_BITMAP == dpId)
		return "DP_TYPE_BITMAP";
	return "DP_ERROR";
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
	byte fnId;
	// target channel
	byte channel;
	// data point type (one of the DP_TYPE_xxx defines)
	byte dpType;
	// true if it's supposed to be sent in dp cache
	byte bDPCache;
	// store last channel value to avoid sending it again
	int prevValue;
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

// wifistate to send when not online
// See: https://imgur.com/a/mEfhfiA
static byte g_defaultTuyaMCUWiFiState = 0x00;


// battery powered device state
enum TuyaMCUV0State {
	TM0_STATE_AWAITING_INFO,
	TM0_STATE_AWAITING_WIFI,
	TM0_STATE_AWAITING_MQTT,
	TM0_STATE_AWAITING_STATES,
};
static byte g_tuyaBatteryPoweredState = 0;
static byte g_hello[] = { 0x55, 0xAA, 0x00, 0x01, 0x00, 0x00, 0x00 };
static byte g_request_state[] = { 0x55, 0xAA, 0x00, 0x02, 0x00, 0x01, 0x04, 0x06 };

tuyaMCUMapping_t* TuyaMCU_FindDefForID(int fnId) {
	tuyaMCUMapping_t* cur;

	cur = g_tuyaMappings;
	while (cur) {
		if (cur->fnId == fnId)
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

void TuyaMCU_MapIDToChannel(int fnId, int dpType, int channel, int bDPCache) {
	tuyaMCUMapping_t* cur;

	cur = TuyaMCU_FindDefForID(fnId);

	if (cur == 0) {
		cur = (tuyaMCUMapping_t*)malloc(sizeof(tuyaMCUMapping_t));
		cur->fnId = fnId;
		cur->dpType = dpType;
		cur->bDPCache = bDPCache;
		cur->prevValue = 0;
		cur->next = g_tuyaMappings;
		g_tuyaMappings = cur;
	}

	cur->channel = channel;
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
	UART_InitUART(g_baudRate);
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

void TuyaMCU_SendState(uint8_t id, uint8_t type, uint8_t* value)
{
	uint16_t payload_len = 4;
	uint8_t payload_buffer[8];
	payload_buffer[0] = id;
	payload_buffer[1] = type;
	switch (type) {
	case DP_TYPE_BOOL:
	case DP_TYPE_ENUM:
		payload_len += 1;
		payload_buffer[2] = 0x00;
		payload_buffer[3] = 0x01;
		payload_buffer[4] = value[0];
		break;
	case DP_TYPE_VALUE:
		payload_len += 4;
		payload_buffer[2] = 0x00;
		payload_buffer[3] = 0x04;
		payload_buffer[4] = value[3];
		payload_buffer[5] = value[2];
		payload_buffer[6] = value[1];
		payload_buffer[7] = value[0];
		break;

	}

	TuyaMCU_SendCommandWithData(TUYA_CMD_SET_DP, payload_buffer, payload_len);
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
#ifdef WIN32

#else

	uint16_t len = strlen(data) / 2;
	uint16_t payload_len = 4 + len;
	uint8_t payload_buffer[payload_len];
	payload_buffer[0] = id;
	payload_buffer[1] = DP_TYPE_STRING;
	payload_buffer[2] = len >> 8;
	payload_buffer[3] = len & 0xFF;

	convertHexStringtoBytes(&payload_buffer[4], data, len);

	TuyaMCU_SendCommandWithData(TUYA_CMD_SET_DP, payload_buffer, payload_len);
#endif
}

void TuyaMCU_SendString(uint8_t id, char data[]) {
#ifdef WIN32

#else
	uint16_t len = strlen(data);
	uint16_t payload_len = 4 + len;
	uint8_t payload_buffer[payload_len];
	payload_buffer[0] = id;
	payload_buffer[1] = DP_TYPE_STRING;
	payload_buffer[2] = len >> 8;
	payload_buffer[3] = len & 0xFF;

	for (uint16_t i = 0; i < len; i++) {
		payload_buffer[4 + i] = data[i];
	}

	TuyaMCU_SendCommandWithData(TUYA_CMD_SET_DP, payload_buffer, payload_len);
#endif
}

void TuyaMCU_SendRaw(uint8_t id, char data[]) {
#ifdef WIN32

#else
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
	uint8_t payload_buffer[payload_len];
	payload_buffer[0] = id;
	payload_buffer[1] = DP_TYPE_RAW;
	payload_buffer[2] = len >> 8;
	payload_buffer[3] = len & 0xFF;

	convertHexStringtoBytes(&payload_buffer[4], beginPos, len);

	TuyaMCU_SendCommandWithData(TUYA_CMD_SET_DP, payload_buffer, payload_len);
#endif
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
void TuyaMCU_Send_SetTime(struct tm* pTime) {
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

	TuyaMCU_SendCommandWithData(TUYA_CMD_SET_TIME, payload_buffer, 8);
}
struct tm* TuyaMCU_Get_NTP_Time() {
	struct tm* ptm;

	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "MCU time to set: %i\n", g_ntpTime);
	ptm = gmtime((time_t*)&g_ntpTime);
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


commandResult_t TuyaMCU_LinkTuyaMCUOutputToChannel(const void* context, const char* cmd, const char* args, int cmdFlags) {
	const char* dpTypeString;
	byte dpId;
	byte dpType;
	byte channelID;
	byte argsCount;
	byte bDPCache;

	// linkTuyaMCUOutputToChannel dpId varType channelID
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
	else if (!stricmp(dpTypeString, "RAW_TAC2121C_Yesterday")) {
		dpType = DP_TYPE_RAW_TAC2121C_YESTERDAY;
	}
	else if (!stricmp(dpTypeString, "RAW_TAC2121C_LastMonth")) {
		dpType = DP_TYPE_RAW_TAC2121C_LASTMONTH;
	}
	else {
		if (strIsInteger(dpTypeString)) {
			dpType = atoi(dpTypeString);
		}
		else {
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_LinkTuyaMCUOutputToChannel: %s is not a valid var type\n", dpTypeString);
			return CMD_RES_BAD_ARGUMENT;
		}
	}
	if (argsCount < 2) {
		channelID = -999;
	}
	else {
		channelID = Tokenizer_GetArgInteger(2);
	}
	bDPCache = Tokenizer_GetArgInteger(3);

	TuyaMCU_MapIDToChannel(dpId, dpType, channelID, bDPCache);

	return CMD_RES_OK;
}

commandResult_t TuyaMCU_Send_SetTime_Current(const void* context, const char* cmd, const char* args, int cmdFlags) {

	TuyaMCU_Send_SetTime(TuyaMCU_Get_NTP_Time());

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

	TuyaMCU_Send_SetTime(&testTime);
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


commandResult_t TuyaMCU_SendStateCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int dpId;
	int dpType;
	int value;

	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	dpId = Tokenizer_GetArgInteger(0);
	dpType = Tokenizer_GetArgInteger(1);
	value = Tokenizer_GetArgInteger(2);

	TuyaMCU_SendState(dpId, dpType, (uint8_t*)&value);

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
		state = Main_HasMQTTConnected() != 0 ? TUYA_NETWORK_STATUS_CONNECTED_TO_CLOUD : TUYA_NETWORK_STATUS_CONNECTED_TO_ROUTER;
	}
	addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU, "TuyaMCU_SendNetworkStatus: sending status 0x%X to MCU \n", state);
	TuyaMCU_SendCommandWithData(0x2B, &state, 1);
}
void TuyaMCU_ForcePublishChannelValues() {
	tuyaMCUMapping_t* cur;

	cur = g_tuyaMappings;
	while (cur) {
		MQTT_ChannelPublish(cur->channel, 0);
		cur = cur->next;
	}
	return 0;
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
void TuyaMCU_ApplyMapping(int fnID, int value) {
	tuyaMCUMapping_t* mapping;
	int mappedValue = value;

	// find mapping (where to save received data)
	mapping = TuyaMCU_FindDefForID(fnID);

	if (mapping == 0) {
		addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU, "TuyaMCU_ApplyMapping: id %i with value %i is not mapped\n", fnID, value);
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

	if (value != mappedValue) {
		addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU, "TuyaMCU_ApplyMapping: mapped value %d (TuyaMCU range) to %d (OpenBK7321T_App range)\n", value, mappedValue);
	}

	mapping->prevValue = mappedValue;

	CHANNEL_Set(mapping->channel, mappedValue, 0);
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

	// this might be a callback from CHANNEL_Set in TuyaMCU_ApplyMapping. If we should set exactly the
	// same value, skip it
	if (mapping->prevValue == iVal) {
		return;
	}

	// dpCaches are sent when requested - TODO - is it correct?
	if (mapping->bDPCache) {
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
		addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU, "TuyaMCU_OnChannelChanged: mapped value %d (OpenBK7321T_App range) to %d (TuyaMCU range)\n", iVal, mappediVal);
	}

	// send value to TuyaMCU
	switch (mapping->dpType)
	{
	case DP_TYPE_BOOL:
		TuyaMCU_SendBool(mapping->fnId, mappediVal != 0);
		break;

	case DP_TYPE_ENUM:
		TuyaMCU_SendEnum(mapping->fnId, mappediVal);
		break;

	case DP_TYPE_VALUE:
		TuyaMCU_SendValue(mapping->fnId, mappediVal);
		break;

	default:
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_OnChannelChanged: channel %d: unsupported data point type %d-%s\n", channel, mapping->dpType, TuyaMCU_GetDataTypeString(mapping->dpType));
		break;
	}
}

void TuyaMCU_ParseQueryProductInformation(const byte* data, int len) {
	char name[256];
	int useLen;

	useLen = len - 1;
	if (useLen > sizeof(name) - 1)
		useLen = sizeof(name) - 1;
	memcpy(name, data, useLen);
	name[useLen] = 0;

	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ParseQueryProductInformation: received %s\n", name);

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
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ParseWeatherData: key %s, val integer %i\n", buffer, iValue);
		}
		else {
			// string
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ParseWeatherData: key %s, string not yet handled\n", buffer);
		}
		ofs += stringLen;
	}
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
// Head v0 ID lengh fnId leen tp vl CHKSUM

void TuyaMCU_V0_ParseRealTimeWithRecordStorage(const byte* data, int len, bool bIncludesDate) {
	int ofs;
	int sectorLen;
	int fnId;
	int dataType;

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
		fnId = data[ofs];
		dataType = data[ofs + 1];
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_V0_ParseRealTimeWithRecordStorage: processing dpId %i, dataType %i-%s and %i data bytes\n",
			fnId, dataType, TuyaMCU_GetDataTypeString(dataType), sectorLen);


		if (sectorLen == 1) {
			int iVal = (int)data[ofs + 4];
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_V0_ParseRealTimeWithRecordStorage: raw data 1 byte: %c\n", iVal);
			// apply to channels
			TuyaMCU_ApplyMapping(fnId, iVal);
		}
		if (sectorLen == 4) {
			int iVal = data[ofs + 4] << 24 | data[ofs + 5] << 16 | data[ofs + 6] << 8 | data[ofs + 7];
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_V0_ParseRealTimeWithRecordStorage: raw data 4 int: %i\n", iVal);
			// apply to channels
			TuyaMCU_ApplyMapping(fnId, iVal);
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
void TuyaMCU_ParseStateMessage(const byte* data, int len) {
	int ofs;
	int sectorLen;
	int fnId;
	int dataType;
	int day, month, year;
	//int channelType;
	int iVal;

	ofs = 0;

	while (ofs + 4 < len) {
		sectorLen = data[ofs + 2] << 8 | data[ofs + 3];
		fnId = data[ofs];
		dataType = data[ofs + 1];
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ParseStateMessage: processing dpId %i, dataType %i-%s and %i data bytes\n",
			fnId, dataType, TuyaMCU_GetDataTypeString(dataType), sectorLen);


		if (sectorLen == 1) {
			iVal = (int)data[ofs + 4];
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ParseStateMessage: raw data 1 byte: %c\n", iVal);
			// apply to channels
			TuyaMCU_ApplyMapping(fnId, iVal);
		}
		else if (sectorLen == 4) {
			iVal = data[ofs + 4] << 24 | data[ofs + 5] << 16 | data[ofs + 6] << 8 | data[ofs + 7];
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ParseStateMessage: raw data 4 int: %i\n", iVal);
			// apply to channels
			TuyaMCU_ApplyMapping(fnId, iVal);
		}
		else {
			tuyaMCUMapping_t* mapping;

			mapping = TuyaMCU_FindDefForID(fnId);

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
				case DP_TYPE_RAW_TAC2121C_VCP:
				{
					if (sectorLen == 8 || sectorLen == 10) {
						// voltage
						iVal = data[ofs + 0 + 4] << 8 | data[ofs + 1 + 4];
						CHANNEL_SetAllChannelsByType(ChType_Voltage_div10, iVal);
						// current
						iVal = data[ofs + 3 + 4] << 8 | data[ofs + 4 + 4];
						CHANNEL_SetAllChannelsByType(ChType_Current_div1000, iVal);
						// power
						iVal = data[ofs + 6 + 4] << 8 | data[ofs + 7 + 4];
						CHANNEL_SetAllChannelsByType(ChType_Power, iVal);
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
						//CHANNEL_SetAllChannelsByType(QQQQQQ, iVal);
						// 06 46 = 1606 => A x 100? ?
						iVal = data[ofs + 11 + 4] << 8 | data[ofs + 12 + 4];
						CHANNEL_SetAllChannelsByType(ChType_Current_div1000, iVal);
						// Voltage?
						iVal = data[ofs + 13 + 4] << 8 | data[ofs + 14 + 4];
						CHANNEL_SetAllChannelsByType(ChType_Voltage_div10, iVal);
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
void TuyaMCU_ResetWiFi() {
	g_resetWiFiEvents++;

	if (g_resetWiFiEvents >= 3) {
		g_openAP = 1;
	}
}
#define TUYA_V0_CMD_PRODUCTINFORMATION      0x01
#define TUYA_V0_CMD_NETWEORKSTATUS          0x02
#define TUYA_V0_CMD_RESETWIFI               0x03
#define TUYA_V0_CMD_RESETWIFI_AND_SEL_CONF  0x04
#define TUYA_V0_CMD_REALTIMESTATUS          0x05
#define TUYA_V0_CMD_RECORDSTATUS            0x08
#define TUYA_V0_CMD_OBTAINDPCACHE           0x10

void TuyaMCU_V0_SendDPCacheReply() {
#if 0
	// send empty?
	byte dat = 0x00;
	TuyaMCU_SendCommandWithData(0x10, &dat, 1);
#else
	int i;
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
	// fnId = 17 Len = 0004 Val V = 1
	// 12 02 0004 00000001
	// fnId = 18 Len = 0004 Val V = 1	
	// 
	while(map) {
		if (map->bDPCache) {
			writtenCount++;
			// dpID
			*p = map->fnId;
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
void TuyaMCU_ProcessIncoming(const byte* data, int len) {
	int checkLen;
	int i;
	byte checkCheckSum;
	byte cmd;
	byte version;

	if (data[0] != 0x55 || data[1] != 0xAA) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ProcessIncoming: discarding packet with bad ident and len %i\n", len);
		return;
	}
	version = data[2];
	checkLen = data[5] | data[4] >> 8;
	checkLen = checkLen + 2 + 1 + 1 + 2 + 1;
	if (checkLen != len) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ProcessIncoming: discarding packet bad expected len, expected %i and got len %i\n", checkLen, len);
		return;
	}
	checkCheckSum = 0;
	for (i = 0; i < len - 1; i++) {
		checkCheckSum += data[i];
	}
	if (checkCheckSum != data[len - 1]) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ProcessIncoming: discarding packet bad expected checksum, expected %i and got checksum %i\n", (int)data[len - 1], (int)checkCheckSum);
		return;
	}
	cmd = data[3];
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ProcessIncoming[ver=%i]: processing command %i (%s) with %i bytes\n", version, cmd, TuyaMCU_GetCommandTypeLabel(cmd), len);
	switch (cmd)
	{
	case TUYA_CMD_HEARTBEAT:
		heartbeat_valid = true;
		heartbeat_counter = 0;
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
		}
		if (5 + dataCount + 2 != len) {
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ProcessIncoming: TUYA_CMD_MCU_CONF had wrong data lenght?");
		}
		else {
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ProcessIncoming: TUYA_CMD_MCU_CONF, TODO!");
		}
		if (g_sensorMode) {
			if (g_tuyaBatteryPoweredState == TM0_STATE_AWAITING_WIFI) {
				g_tuyaBatteryPoweredState = TM0_STATE_AWAITING_MQTT;
			} else if (g_tuyaBatteryPoweredState == TM0_STATE_AWAITING_MQTT) {
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

	case TUYA_CMD_STATE:
		TuyaMCU_ParseStateMessage(data + 6, len - 6);
		state_updated = true;
		g_sendQueryStatePackets = 0;
		break;
	case TUYA_CMD_SET_TIME:
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ProcessIncoming: received TUYA_CMD_SET_TIME, so sending back time");
		TuyaMCU_Send_SetTime(TuyaMCU_Get_NTP_Time());
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
	case TUYA_V0_CMD_OBTAINDPCACHE:
		// This is sent by TH01
		// Info:TuyaMCU:TUYAMCU received: 55 AA 00 10 00 02 01 09 1B
		{
			TuyaMCU_V0_SendDPCacheReply();
		}
		break;
	case 0x05:
		// This was added for this user:
		// https://www.elektroda.com/rtvforum/topic3937723.html
		if (version == 0) {
			// 0x05 packet for version 0 (not 0x03) of TuyaMCU
			// This packet has no datetime stamp
			TuyaMCU_V0_ParseRealTimeWithRecordStorage(data + 6, len - 6, false);
		}
		else {
		}
		break;
	case TUYA_CMD_WEATHERDATA:
		TuyaMCU_ParseWeatherData(data + 6, len - 6);
		break;

	case TUYA_CMD_SET_RSSI:
		// This is send by TH06, S09
		// Info:TuyaMCU:TUYAMCU received: 55 AA 03 24 00 00 26
		if (version == 3) {
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ProcessIncoming: received TUYA_CMD_SET_RSSI, so sending back signal strength");
			TuyaMCU_Send_RSSI(HAL_GetWifiStrength());
		}
		break;

	case TUYA_CMD_NETWORK_STATUS:
		//This is sent by S09
		//Info:TuyaMCU:TUYAMCU received: 55 AA 03 2B 00 00 2D
		//
		if (version == 3) {
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ProcessIncoming: (test for S09 calendar/IR device) received TUYA_CMD_NETWORK_STATUS 0x2B ");
			TuyaMCU_SendNetworkStatus();
		}
		break;
	default:
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_ProcessIncoming: unhandled type %i", cmd);
		break;
	}
	EventHandlers_FireEvent(CMD_EVENT_TUYAMCU_PARSED, cmd);
}

commandResult_t TuyaMCU_FakePacket(const void* context, const char* cmd, const char* args, int cmdFlags) {
	byte packet[256];
	int c = 0;
	if (!(*args)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TuyaMCU_FakePacket: requires 1 argument (hex string, like FFAABB00CCDD\n");
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
void TuyaMCU_RunWiFiUpdateAndPackets() {
	//addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"TuyaMCU_WifiCheck %d ", wifi_state_timer);
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
		//addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"TuyaMCU_Wifi_State timer");
	}
}
void TuyaMCU_RunReceive() {
	byte data[128];
	char buffer_for_log[256];
	char buffer2[4];
	int len, i;
	while (1)
	{
		len = UART_TryToGetNextTuyaPacket(data, sizeof(data));
		if (len > 0) {
			buffer_for_log[0] = 0;
			for (i = 0; i < len; i++) {
				snprintf(buffer2, sizeof(buffer2), "%02X ", data[i]);
				strcat_safe(buffer_for_log, buffer2, sizeof(buffer_for_log));
			}
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "TUYAMCU received: %s\n", buffer_for_log);
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
			TuyaMCU_ProcessIncoming(data, len);
		}
		else {
			break;
		}
	}
}
void TuyaMCU_RunStateMachine_V3() {

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
		heartbeat_counter++;
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
				// send wifi state 0x03 
				Tuya_SetWifiState_V0(0x03);
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
			if (Main_HasMQTTConnected() || g_defaultTuyaMCUWiFiState == 0x04)
#endif
			{
				// send wifi state 0x04 
				Tuya_SetWifiState_V0(0x04);
				// retry
				g_tuyaNextRequestDelay = 3;
			}
		}
		break;
	case TM0_STATE_AWAITING_STATES:
		if (g_tuyaNextRequestDelay <= 0) {
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
void TuyaMCU_RunFrame() {
	TuyaMCU_RunReceive();

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
	UART_InitUART(g_baudRate);

	return CMD_RES_OK;
}


void TuyaMCU_Init()
{
	// this is only for simulator, where multiple sessions can happen...
	tuyaMCUMapping_t *tmp, *nxt;
	tmp = g_tuyaMappings;
	while (tmp) {
		nxt = tmp->next;
		free(tmp);
		tmp = nxt;
	}
	g_tuyaMappings = 0;

	g_resetWiFiEvents = 0;
	g_tuyaNextRequestDelay = 1;
	g_tuyaBatteryPoweredState = 0; 
	g_tuyaMCUConfirmationsToSend_0x05 = 0;
	g_tuyaMCUConfirmationsToSend_0x08 = 0;

	UART_InitUART(g_baudRate);
	UART_InitReceiveRingBuffer(256);
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
	///CMD_RegisterCommand("tuyaMcu_sendSimple","",TuyaMCU_Send_Simple, "Appends a 0x55 0xAA header to a data, append a checksum at end and send");
	//cmddetail:{"name":"linkTuyaMCUOutputToChannel","args":"[dpId][varType][channelID][bDPCache-Optional]",
	//cmddetail:"descr":"Used to map between TuyaMCU dpIDs and our internal channels. Last argument is optional and 0 by default. You can set it to 1 for battery powered devices, so a variable is set with DPCache, for example a sampling interval for humidity/temperature sensor. Mapping works both ways. DpIDs are per-device, you can get them by sniffing UART communication. Vartypes can also be sniffed from Tuya. VarTypes can be following: 0-raw, 1-bool, 2-value, 3-string, 4-enum, 5-bitmap. Please see [Tuya Docs](https://developer.tuya.com/en/docs/iot/tuya-cloud-universal-serial-port-access-protocol?id=K9hhi0xxtn9cb) for info about TuyaMCU. You can also see our [TuyaMCU Analyzer Tool](https://www.elektroda.com/rtvforum/viewtopic.php?p=20528459#20528459)",
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
	//cmddetail:"fn":"Cmd_TuyaMCU_Send_RSSI","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_defWiFiState", Cmd_TuyaMCU_Set_DefaultWiFiState, NULL);
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


