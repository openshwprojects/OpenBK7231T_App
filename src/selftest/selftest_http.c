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

//jsmn_parser parser;
cJSON *g_json;
cJSON *g_sec_power;

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

void Test_FakeHTTPClientPacket(const char *tg) {
	int iResult;
	int len;

	http_request_t request;

	sprintf(buffer, http_get_template1, tg);

	iResult = strlen(buffer);

	memset(&request, 0, sizeof(request));


	request.fd = 0;
	request.received = buffer;
	request.receivedLen = iResult;
	outbuf[0] = '\0';
	request.reply = outbuf;
	request.replylen = 0;

	request.replymaxlen = sizeof(outbuf);

	printf("Test_FakeHTTPClientPacket fake bytes sent: %d \n", iResult);
 	len = HTTP_ProcessPacket(&request);
	outbuf[request.replylen] = 0;
	printf("Test_FakeHTTPClientPacket fake bytes received: %d \n", len);

	replyAt = Helper_GetPastHTTPHeader(outbuf);

}
void Test_FakeHTTPClientPacket_JSON(const char *tg) {
	int r;
	Test_FakeHTTPClientPacket(tg);

	//jsmn_init(&parser);
	//r = jsmn_parse(&parser, replyAt, strlen(replyAt), tokens, 256);

	if (g_json) {
		cJSON_Delete(g_json);
	}
	g_json = cJSON_Parse(replyAt);
	g_sec_power = cJSON_GetObjectItemCaseSensitive(g_json, "POWER");
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
const char *Test_GetJSONValue_Integer(const char *keyword, const char *obj) {
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
	printf("Test_GetJSONValue_String will return %s for %s\n", tmp->valuestring, keyword);
	return tmp->valuestring;
}

void Test_Http_SingleRelayOnChannel1() {

	CMD_ExecuteCommand("clearAll", 0);
	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "OFF");

	Test_FakeHTTPClientPacket("index?tgl=1");

	// read power
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");

	// set power ON
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER%20ON");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");

	// set power OFF
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER%20OFF");
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

}
void Test_Http_TwoRelays() {

	CMD_ExecuteCommand("clearAll", 0);
	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	PIN_SetPinRoleForPinIndex(10, IOR_Relay);
	PIN_SetPinChannelForPinIndex(10, 2);

	// Channel 1 Off
	// Channel 2 Off
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0);

	Test_FakeHTTPClientPacket("index?tgl=1");

	// Channel 1 On
	// Channel 2 Off
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b1);

	Test_FakeHTTPClientPacket("index?tgl=2");
	// Channel 1 On
	// Channel 2 On
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b11);


	Test_FakeHTTPClientPacket("index?tgl=1");
	// Channel 1 Off
	// Channel 2 On
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b10);
}
void Test_Http_FourRelays() {

	CMD_ExecuteCommand("clearAll", 0);
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

	Test_FakeHTTPClientPacket("index?tgl=3");

	// Channel 1 Off
	// Channel 2 Off
	// Channel 3 On
	// Channel 4 Off
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b0100);

	Test_FakeHTTPClientPacket("index?tgl=2");
	// Channel 1 Off
	// Channel 2 On
	// Channel 3 On
	// Channel 4 Off
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b0110);


	Test_FakeHTTPClientPacket("index?tgl=4");
	// Channel 1 Off
	// Channel 2 On
	// Channel 3 On
	// Channel 4 On
	// In STATUS register, power is encoded as integer...
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("Status", "Power", 0b1110);
}
/*
StatusSTS sample from Tasmota:

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
		 "SSId":"DLINK_FastNet",
		 "BSSId":"30:B5:C2:5D:70:72",
		 "Channel":11,
		 "Mode":"11n",
		 "RSSI":54,
		 "Signal":-73,
		 "LinkCount":1,
		 "Downtime":"0T00:00:03"
	  }
   }

*/
void Test_Http_LED_SingleChannel() {

	CMD_ExecuteCommand("clearAll", 0);
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
void Test_Http() {
	Test_Http_SingleRelayOnChannel1();
	Test_Http_TwoRelays();
	Test_Http_FourRelays();


	Test_Http_LED_SingleChannel();
}


#endif
