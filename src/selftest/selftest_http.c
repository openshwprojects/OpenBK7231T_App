#ifdef WINDOWS

#include "selftest_local.h"
#include "../httpserver/new_http.h"
//#define JSMN_HEADER
///#include "../jsmn/jsmn.h"
#include "../cJSON/cJSON.h"

// "GET /index?tgl=1 HTTP/1.1\r\n"
const char *http_get_template1 = "GET /%s HTTP/1.1\r\n"
"Host: 127.0.0.1\r\n"
"Connection: keep - alive\r\n"
"sec-ch-ua: \"Google Chrome\";v=\"107\", \"Chromium\";v=\"107\", \"Not=A?Brand\";v=\"24\"\r\n"
"sec-ch-ua-mobile: ? 0\r\n"
"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36\r\n"
"sec-ch-ua-platform: \"Windows\"\r\n"
"Accept: */*\r\n"
"Sec-Fetch-Site: same-origin\r\n"
"Sec-Fetch-Mode: cors\r\n"
"Sec-Fetch-Dest: empty\r\n"
"Referer: http://127.0.0.1/index\r\n"
"Accept-Encoding: gzip, deflate, br\r\n"
"Accept-Language: pl-PL,pl;q=0.9,en-US;q=0.8,en;q=0.7,de;q=0.6\r\n"
"\r\n"
"\r\n";

const char *http_post_template1 = "POST /%s HTTP/1.1\r\n"
"Host: 127.0.0.1\r\n"
"Connection: keep - alive\r\n"
"Content-Length: %i\r\n"
"sec-ch-ua: \"Google Chrome\";v=\"107\", \"Chromium\";v=\"107\", \"Not=A?Brand\";v=\"24\"\r\n"
"sec-ch-ua-mobile: ? 0\r\n"
"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36\r\n"
"sec-ch-ua-platform: \"Windows\"\r\n"
"Accept: */*\r\n"
"Sec-Fetch-Site: same-origin\r\n"
"Sec-Fetch-Mode: cors\r\n"
"Sec-Fetch-Dest: empty\r\n"
"Referer: http://127.0.0.1/app\r\n"
"Accept-Encoding: gzip, deflate, br\r\n"
"Accept-Language: pl-PL,pl;q=0.9,en-US;q=0.8,en;q=0.7,de;q=0.6\r\n"
"\r\n"
"%s";

//jsmn_parser parser;
static cJSON *g_json;
static cJSON *g_sec_power;

const char *Helper_GetPastHTTPHeader(const char *s) {
	while (*s) {
		if (!strncmp(s, "\r\n\r\n",4)) {
			return s + 4;
		}
		s++;
	}
	return 0;
}

static char outbuf[8192];
static char buffer[8192];
static const char *replyAt;
//static jsmntok_t tokens[256]; /* We expect no more than qq JSON tokens */

void Test_FakeHTTPClientPacket_Generic() {
	int iResult;
	int len;

	http_request_t request;


	iResult = strlen(buffer);

	memset(&request, 0, sizeof(request));


	request.fd = 0;
	request.received = buffer;
	request.receivedLen = iResult;
	outbuf[0] = '\0';
	request.reply = outbuf;
	request.replylen = 0;

	request.replymaxlen = sizeof(outbuf);

	printf("Test_FakeHTTPClientPacket_GET fake bytes sent: %d \n", iResult);
 	len = HTTP_ProcessPacket(&request);
	outbuf[request.replylen] = 0;
	printf("Test_FakeHTTPClientPacket_GET fake bytes received: %d \n", len);

	replyAt = Helper_GetPastHTTPHeader(outbuf);

}
void Test_FakeHTTPClientPacket_GET(const char *tg) {
	//char bufferTemp[8192];
	//va_list argList;

	//va_start(argList, tg);
	//vsnprintf(bufferTemp, sizeof(bufferTemp), tg, argList);
	//va_end(argList);

	sprintf(buffer, http_get_template1, tg);
	Test_FakeHTTPClientPacket_Generic();
}
void Test_FakeHTTPClientPacket_POST(const char *tg, const char *data) {
	int dataLen = strlen(data);

	sprintf(buffer, http_post_template1, tg, dataLen, data);
	Test_FakeHTTPClientPacket_Generic();
}
void Test_GetJSONValue_Setup(const char *text) {
	if (g_json) {
		cJSON_Delete(g_json);
	}
	printf("Received JSON: %s\n", text);
	g_json = cJSON_Parse(text);
	g_sec_power = cJSON_GetObjectItemCaseSensitive(g_json, "POWER");
}
void Test_FakeHTTPClientPacket_JSON(const char *tg) {
	/*char bufferTemp[8192];
	va_list argList;

	va_start(argList, tg);
	vsnprintf(bufferTemp, sizeof(bufferTemp), tg, argList);
	va_end(argList);
*/
	int r;
	Test_FakeHTTPClientPacket_GET(tg);

	//jsmn_init(&parser);
	//r = jsmn_parse(&parser, replyAt, strlen(replyAt), tokens, 256);
	Test_GetJSONValue_Setup(replyAt);
}
cJSON *Test_GetJSONValue_Generic_Nested2(const char *par1, const char *par2, const char *keyword) {
	cJSON *tmp;
	cJSON *parent;
	parent = cJSON_GetObjectItemCaseSensitive(g_json, par1);
	if (parent == 0)
		return 0;
	parent = cJSON_GetObjectItemCaseSensitive(parent, par2);
	if (parent == 0)
		return 0;
	tmp = cJSON_GetObjectItemCaseSensitive(parent, keyword);
	if (tmp == 0)
		return 0;
	return tmp;
}
cJSON *Test_GetJSONValue_Generic(const char *keyword, const char *obj) {
	cJSON *tmp;
	cJSON *parent;
	if (obj == 0 || obj[0] == 0) {
		parent = g_json;
	}
	else {
		parent = cJSON_GetObjectItemCaseSensitive(g_json, obj);
	}
	if (parent == 0)
		return 0;
	tmp = cJSON_GetObjectItemCaseSensitive(parent, keyword);
	if (tmp == 0)
		return 0;
	return tmp;
}
int Test_GetJSONValue_Integer_Nested2(const char *par1, const char *par2, const char *keyword) {
	cJSON *tmp;
	tmp = Test_GetJSONValue_Generic_Nested2(par1, par2, keyword);
	if (tmp == 0) {
		return -999999;
	}
	printf("Test_GetJSONValue_Integer_Nested2 will return %i for %s\n", tmp->valueint, keyword);
	return tmp->valueint;
}
//const char *Test_GetJSONValue_String_Nested(const char *par1, const char *keyword) {
//	cJSON *tmp;
//	tmp = Test_GetJSONValue_Generic_Nested(par1, keyword);
//	if (tmp == 0) {
//		return 0;
//	}
//	const char *ret = tmp->valuestring;
//	printf("Test_GetJSONValue_String_Nested will return %s for %s\n", ret, keyword);
//	return ret;
//}
const char *Test_GetJSONValue_String_Nested2(const char *par1, const char *par2, const char *keyword) {
	cJSON *tmp;
	tmp = Test_GetJSONValue_Generic_Nested2(par1, par2, keyword);
	if (tmp == 0) {
		return 0;
	}
	const char *ret = tmp->valuestring;
	printf("Test_GetJSONValue_String_Nested2 will return %s for %s\n", ret, keyword);
	return ret;
}
float Test_GetJSONValue_Float_Nested2(const char *par1, const char *par2, const char *keyword) {
	cJSON *tmp;
	tmp = Test_GetJSONValue_Generic_Nested2(par1, par2, keyword);
	if (tmp == 0) {
		return -999999;
	}
	float ret = tmp->valuedouble;
	printf("Test_GetJSONValue_Float_Nested2 will return %f for %s\n", ret, keyword);
	return ret;
}
int Test_GetJSONValue_Integer(const char *keyword, const char *obj) {
	cJSON *tmp;
	tmp = Test_GetJSONValue_Generic(keyword, obj);
	if (tmp == 0) {
		return -999999;
	}
	printf("Test_GetJSONValue_Integer will return %i for %s\n", tmp->valueint, keyword);
	return tmp->valueint;
}
const char *Test_GetJSONValue_String(const char *keyword, const char *obj) {
	cJSON *tmp;
	tmp = Test_GetJSONValue_Generic(keyword, obj);
	if (tmp == 0) {
		return "";
	}
	if (tmp->valuestring == 0) {
		printf("Test_GetJSONValue_String will return empty str for %s because it has NULL string!\n", keyword);
		return "";
	}
	printf("Test_GetJSONValue_String will return %s for %s\n", tmp->valuestring, keyword);
	return tmp->valuestring;
}
const char *Test_GetLastHTMLReply() {
	return replyAt;
}
void Test_Http_SingleRelayOnChannel1() {

	SIM_ClearOBK(0);
	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "OFF");

	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "OFF");

	CMD_ExecuteCommand("setChannel 1 1",0);
	// The empty power packet should not affect relay
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");
	CMD_ExecuteCommand("setChannel 1 0", 0);
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "OFF");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "OFF");


	Test_FakeHTTPClientPacket_GET("index?tgl=1");

	// read power
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");

	// set power ON
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER%20ON");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");

	// get power status (IT SHOULD NOT CHANGE POWER value)
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER%20Status");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");

	// set power OFF
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER%20OFF");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "OFF");

	// get power status (IT SHOULD NOT CHANGE POWER value)
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER%20Status");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "OFF");

	// set power OFF
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER%20OFF");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "OFF");

	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0);

	// set power ON
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER%20ON");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");

	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 1);

	// direct channel access - set to 0, is it 0?
	SIM_SendFakeMQTTRawChannelSet(1, "0");
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0);

	// direct channel access - set to 1, is it 1?
	SIM_SendFakeMQTTRawChannelSet(1, "1");
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 1);

	// direct channel access - set to 0, is it 0?
	SIM_SendFakeMQTTRawChannelSet(1, "0");
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0);

	// check client id with slash
	CFG_SetMQTTClientId("mainRoom/obkRelay1");
	MQTT_init();

	// direct channel access - set to 0, is it 0?
	SIM_SendFakeMQTTRawChannelSet(1, "0");
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0);

	// direct channel access - set to 1, is it 1?
	SIM_SendFakeMQTTRawChannelSet(1, "1");
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 1);

	// direct channel access - set to 0, is it 0?
	SIM_SendFakeMQTTRawChannelSet(1, "0");
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0);

	// direct channel access - set to 1, is it 1?
	SIM_SendFakeMQTTRawChannelSet(1, "1");
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 1);

}
void Test_Http_TwoRelays() {

	SIM_ClearOBK(0);
	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	PIN_SetPinRoleForPinIndex(10, IOR_Relay);
	PIN_SetPinChannelForPinIndex(10, 2);

	// Channel 1 Off
	// Channel 2 Off
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0);
		SELFTEST_ASSERT_CHANNEL(1, false);
		SELFTEST_ASSERT_CHANNEL(2, false);


	Test_FakeHTTPClientPacket_GET("index?tgl=1");

	// Channel 1 On
	// Channel 2 Off
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b1);
		SELFTEST_ASSERT_CHANNEL(1, true);
		SELFTEST_ASSERT_CHANNEL(2, false);

	Test_FakeHTTPClientPacket_GET("index?tgl=2");
	// Channel 1 On
	// Channel 2 On
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b11);
		SELFTEST_ASSERT_CHANNEL(1, true);
		SELFTEST_ASSERT_CHANNEL(2, true);


	Test_FakeHTTPClientPacket_GET("index?tgl=1");
	// Channel 1 Off
	// Channel 2 On
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b10);
		SELFTEST_ASSERT_CHANNEL(1, false);
		SELFTEST_ASSERT_CHANNEL(2, true);

	// Channel 1 should become On.
	// Both On now.
	SIM_SendFakeMQTTRawChannelSet(1, "1");
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b11);
		SELFTEST_ASSERT_CHANNEL(1, true);
		SELFTEST_ASSERT_CHANNEL(2, true);
	// Disable channel 1, should be Off again
	SIM_SendFakeMQTTRawChannelSet(1, "0");
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b10);
		SELFTEST_ASSERT_CHANNEL(1, false);
		SELFTEST_ASSERT_CHANNEL(2, true);
	// disable channel 2... both will be off
	SIM_SendFakeMQTTRawChannelSet(2, "0");
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b00);
		SELFTEST_ASSERT_CHANNEL(1, false);
		SELFTEST_ASSERT_CHANNEL(2, false);

}
void Test_Http_FourRelays() {

	SIM_ClearOBK(0);
	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	PIN_SetPinRoleForPinIndex(10, IOR_Relay);
	PIN_SetPinChannelForPinIndex(10, 2);

	PIN_SetPinRoleForPinIndex(11, IOR_Relay);
	PIN_SetPinChannelForPinIndex(11, 3);

	PIN_SetPinRoleForPinIndex(12, IOR_Relay);
	PIN_SetPinChannelForPinIndex(12, 4);

	// Channel 1 Off
	// Channel 2 Off
	// Channel 3 Off
	// Channel 4 Off
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0);
		SELFTEST_ASSERT_CHANNEL(1, false);
		SELFTEST_ASSERT_CHANNEL(2, false);
		SELFTEST_ASSERT_CHANNEL(3, false);
		SELFTEST_ASSERT_CHANNEL(4, false);

	Test_FakeHTTPClientPacket_GET("index?tgl=3");

	// Channel 1 Off
	// Channel 2 Off
	// Channel 3 On
	// Channel 4 Off
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b0100);
		SELFTEST_ASSERT_CHANNEL(1, false);
		SELFTEST_ASSERT_CHANNEL(2, false);
		SELFTEST_ASSERT_CHANNEL(3, true);
		SELFTEST_ASSERT_CHANNEL(4, false);

	Test_FakeHTTPClientPacket_GET("index?tgl=2");
	// Channel 1 Off
	// Channel 2 On
	// Channel 3 On
	// Channel 4 Off
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b0110);
		SELFTEST_ASSERT_CHANNEL(1, false);
		SELFTEST_ASSERT_CHANNEL(2, true);
		SELFTEST_ASSERT_CHANNEL(3, true);
		SELFTEST_ASSERT_CHANNEL(4, false);


	Test_FakeHTTPClientPacket_GET("index?tgl=4");
	// Channel 1 Off
	// Channel 2 On
	// Channel 3 On
	// Channel 4 On
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b1110);
		SELFTEST_ASSERT_CHANNEL(1, false);
		SELFTEST_ASSERT_CHANNEL(2, true);
		SELFTEST_ASSERT_CHANNEL(3, true);
		SELFTEST_ASSERT_CHANNEL(4, true);

	// disable channel 4..
	SIM_SendFakeMQTTRawChannelSet(4, "0");
	// Channel 1 Off
	// Channel 2 On
	// Channel 3 On
	// Channel 4 Off
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b0110);
		SELFTEST_ASSERT_CHANNEL(1, false);
		SELFTEST_ASSERT_CHANNEL(2, true);
		SELFTEST_ASSERT_CHANNEL(3, true);
		SELFTEST_ASSERT_CHANNEL(4, false);

	// disable channel 3..
	SIM_SendFakeMQTTRawChannelSet(3, "0");
	// Channel 1 Off
	// Channel 2 On
	// Channel 3 Off
	// Channel 4 Off
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b0010);
		SELFTEST_ASSERT_CHANNEL(1, false);
		SELFTEST_ASSERT_CHANNEL(2, true);
		SELFTEST_ASSERT_CHANNEL(3, false);
		SELFTEST_ASSERT_CHANNEL(4, false);

	// enable channel 3..
	SIM_SendFakeMQTTRawChannelSet(1, "1");
	// Channel 1 On
	// Channel 2 On
	// Channel 3 Off
	// Channel 4 Off
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b0011);
	SELFTEST_ASSERT_CHANNEL(1, true);
	SELFTEST_ASSERT_CHANNEL(2, true);
	SELFTEST_ASSERT_CHANNEL(3, false);
	SELFTEST_ASSERT_CHANNEL(4, false);
}
/*
StatusSTS sample from Tasmota RGB (3 PWMs set):

   "StatusSTS":{
	  "Time":"2022-11-17T09:35:58",
	  "Uptime":"5T11:36:54",
	  "UptimeSec":473814,
	  "Heap":24,
	  "SleepMode":"Dynamic",
	  "Sleep":10,
	  "LoadAvg":99,
	  "MqttCount":3,
	  "POWER":"ON",
	  "Dimmer":80,
	  "Color":"144,114,204",
	  "HSBColor":"260,44,80",
	  "Channel":[
		 57,
		 45,
		 80
	  ],
	  "Scheme":0,
	  "Fade":"OFF",
	  "Speed":1,
	  "LedTable":"ON",
	  "Wifi":{
		 "AP":1,
		 "SSId":"5G_FULL_POWER",
		 "BSSId":"30:B5:C2:5D:70:72",
		 "Channel":11,
		 "Mode":"11n",
		 "RSSI":54,
		 "Signal":-73,
		 "LinkCount":1,
		 "Downtime":"0T00:00:03"
	  }
   }

StatusSTS sample from Tasmota CW (2 PWMs set):


   "StatusSTS":{
	  "Time":"2022-11-17T09:43:50",
	  "Uptime":"0T00:00:13",
	  "UptimeSec":13,
	  "Heap":25,
	  "SleepMode":"Dynamic",
	  "Sleep":10,
	  "LoadAvg":97,
	  "MqttCount":0,
	  "POWER":"ON",
	  "Dimmer":100,
	  "Color":"255,0",
	  "White":100,
	  "CT":153,
	  "Channel":[
		 100,
		 0
	  ],
	  "Fade":"OFF",
	  "Speed":1,
	  "LedTable":"ON",
	  "Wifi":{
		 "AP":1,
		 "SSId":"5G_FULL_POWER",
		 "BSSId":"30:B5:C2:5D:70:72",
		 "Channel":11,
		 "Mode":"11n",
		 "RSSI":64,
		 "Signal":-68,
		 "LinkCount":1,
		 "Downtime":"0T00:00:03"
	  }
   }

StatusSTS sample from Tasmota RGBCW (5 PWMs set):

   "StatusSTS":{
	  "Time":"2022-11-17T10:12:49",
	  "Uptime":"0T00:00:31",
	  "UptimeSec":31,
	  "Heap":25,
	  "SleepMode":"Dynamic",
	  "Sleep":10,
	  "LoadAvg":110,
	  "MqttCount":0,
	  "POWER":"ON",
	  "Dimmer":100,
	  "Color":"0,0,0,0,255",
	  "HSBColor":"0,0,0",
	  "White":100,
	  "CT":500,
	  "Channel":[
		 0,
		 0,
		 0,
		 0,
		 100
	  ],
	  "Scheme":0,
	  "Fade":"OFF",
	  "Speed":1,
	  "LedTable":"ON",
	  "Wifi":{
		 "AP":1,
		 "SSId":"5G_FULL_POWER",
		 "BSSId":"30:B5:C2:5D:70:72",
		 "Channel":11,
		 "Mode":"11n",
		 "RSSI":62,
		 "Signal":-69,
		 "LinkCount":1,
		 "Downtime":"0T00:00:03"
	  }
   }
*/
void Test_Http_LED_SingleChannel() {

	SIM_ClearOBK(0);
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	CMD_ExecuteCommand("led_enableAll 1", 0);
	CMD_ExecuteCommand("led_dimmer 100", 0);
	
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");

	CMD_ExecuteCommand("led_enableAll 0", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "OFF");

	CMD_ExecuteCommand("led_dimmer 61", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "OFF");


	CMD_ExecuteCommand("led_enableAll 1", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");



}
void Test_Http_LED_CW() {

	SIM_ClearOBK(0);
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);

	CMD_ExecuteCommand("led_enableAll 1", 0);
	CMD_ExecuteCommand("led_dimmer 100", 0);
	CMD_ExecuteCommand("led_temperature 153", 0);

	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 153);

	CMD_ExecuteCommand("led_enableAll 0", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "OFF");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 153);

	CMD_ExecuteCommand("led_dimmer 61", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "OFF");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 153);


	CMD_ExecuteCommand("led_enableAll 1", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 153);

	CMD_ExecuteCommand("led_temperature 500", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 500);


	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer", "100");
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 500);
}
void Test_Http_LED_RGB() {

	SIM_ClearOBK(0);
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);

	PIN_SetPinRoleForPinIndex(9, IOR_PWM);
	PIN_SetPinChannelForPinIndex(9, 3);

	CMD_ExecuteCommand("led_enableAll 1", 0);
	CMD_ExecuteCommand("led_dimmer 100", 0);
	CMD_ExecuteCommand("led_basecolor_rgb FF0000", 0);

	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "255,0,0");


	CMD_ExecuteCommand("led_basecolor_rgb FFFF00", 0);

	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 100);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "255,255,0");

	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	CMD_ExecuteCommand("led_dimmer 50", 0);

	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 50);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "55,55,0");


	CMD_ExecuteCommand("led_basecolor_rgb 0000FF", 0);
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 50);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "0,0,55");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	CMD_ExecuteCommand("led_dimmer 100", 0);
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "0,0,255");

	SIM_SendFakeMQTTAndRunSimFrame_CMND("POWER", "OFF");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "OFF");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "0,0,0");

	SIM_SendFakeMQTTAndRunSimFrame_CMND("POWER", "1");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "0,0,255");


	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer", "50");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 50);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "0,0,55");

	// dimmer back to 100
	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer", "100");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "0,0,255");

	// check Tasmota HSBColor
	/*
	NOTE:
	HTTP Request: http://192.168.0.153/cm?cmnd=HSBColor%200,100,100
	Return value: {"POWER":"ON","Dimmer":100,"Color":"FF00000000","HSBColor":"0,100,100","White":0,"CT":479,"Channel":[100,0,0,0,0]}
	
	*/
	SIM_SendFakeMQTTAndRunSimFrame_CMND("HSBColor", "0,100,100");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "255,0,0");
	// check Tasmota HSBColor
	/*
	NOTE:
	HTTP Request: http://192.168.0.153/cm?cmnd=HSBColor%2090,100,100
	Return value: {"POWER":"ON","Dimmer":100,"Color":"7FFF000000","HSBColor":"90,100,100","White":0,"CT":479,"Channel":[50,100,0,0,0]}

	*/
	SIM_SendFakeMQTTAndRunSimFrame_CMND("HSBColor", "90,100,100");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "127,255,0");
	// check Tasmota HSBColor
	/*
	NOTE:
	HTTP Request: http://192.168.0.153/cm?cmnd=HSBColor%20180,100,100
	Return value: {"POWER":"ON","Dimmer":100,"Color":"00FFFF0000","HSBColor":"180,100,100","White":0,"CT":479,"Channel":[0,100,100,0,0]}
	*/
	SIM_SendFakeMQTTAndRunSimFrame_CMND("HSBColor", "180,100,100");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "0,255,255");

	/*
	CMD_ExecuteCommand("led_enableAll 0", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "OFF");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Color", "255,0,0");

	CMD_ExecuteCommand("led_dimmer 61", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "OFF");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 153);


	CMD_ExecuteCommand("led_enableAll 1", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 153);

	CMD_ExecuteCommand("led_temperature 500", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 500);
	*/

}
void Test_Http() {
	Test_Http_SingleRelayOnChannel1();
	Test_Http_TwoRelays();
	Test_Http_FourRelays();


	Test_Http_LED_SingleChannel();
	Test_Http_LED_CW();
	Test_Http_LED_RGB();
}


#endif
