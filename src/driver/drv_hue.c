#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_wifi.h"
#include "drv_public.h"
#include "drv_local.h"
#include "drv_ssdp.h"
#include "../httpserver/new_http.h"

// Hue driver for Alexa, requiring btsimonh's SSDP to work
// Based on Tasmota approach
// The procedure is following:
// 1. first MSEARCH over UDP is done
// 2. then obk replies to MSEARCH with page details
// 3. then alexa accesses our XML pages here with GET
// 4. and can change the binary state (0 or 1) with POST

static char *buffer_out = 0;
static char *g_serial = 0;
static char *g_userID = 0;
static char *g_uid = 0;
static char *g_bridgeID = 0;
static int outBufferLen = 0;
static int stat_searchesReceived = 0;
static int stat_setupXMLVisits = 0;
static int stat_apiCalls = 0;
static int stat_lightChanges = 0;


// ARGUMENTS: first IP, then bridgeID
const  char *hue_resp = "HTTP/1.1 200 OK\r\n"
   "HOST: 239.255.255.250:1900\r\n"
   "CACHE-CONTROL: max-age=100\r\n"
   "EXT:\r\n"
   "LOCATION: http://%s:80/description.xml\r\n"
   "SERVER: Linux/3.14.0 UPnP/1.0 IpBridge/1.24.0\r\n"  // was 1.17
   "hue-bridgeid: %s\r\n";

// ARGUMENTS: uuid
const  char *hue_resp1 = "ST: upnp:rootdevice\r\n"
  "USN: uuid:%s::upnp:rootdevice\r\n"
  "\r\n";

// ARGUMENTS: uuid and uuid
const  char *hue_resp2 =  "ST: uuid:%s\r\n"
   "USN: uuid:%s\r\n"
   "\r\n";

// ARGUMENTS: uuid
const  char *hue_resp3 = "ST: urn:schemas-upnp-org:device:basic:1\r\n"
   "USN: uuid:%s\r\n"
   "\r\n";

void DRV_HUE_Send_Advert_To(struct sockaddr_in *addr) {
	//const char *useType;

	if (g_uid == 0) {
		// not running
		return;
	}

	stat_searchesReceived++;

	if (buffer_out == 0) {
		outBufferLen = strlen(hue_resp) + 256;
		buffer_out = (char*)malloc(outBufferLen);
	}
	{
		// ARGUMENTS: first IP, then bridgeID
		snprintf(buffer_out, outBufferLen, hue_resp, HAL_GetMyIPString(), g_bridgeID);

		addLogAdv(LOG_ALL, LOG_FEATURE_HTTP, "HUE - Sending[0] %s", buffer_out);
		DRV_SSDP_SendReply(addr, buffer_out);
	}
	{
		// ARGUMENTS: uuid
		snprintf(buffer_out, outBufferLen, hue_resp1, g_uid);

		addLogAdv(LOG_ALL, LOG_FEATURE_HTTP, "HUE - Sending[1] %s", buffer_out);
		DRV_SSDP_SendReply(addr, buffer_out);
	}
	{
		// ARGUMENTS: uuid and uuid
		snprintf(buffer_out, outBufferLen, hue_resp2, g_uid, g_uid);

		addLogAdv(LOG_ALL, LOG_FEATURE_HTTP, "HUE - Sending[2] %s", buffer_out);
		DRV_SSDP_SendReply(addr, buffer_out);
	}
	{
		// ARGUMENTS: uuid
		snprintf(buffer_out, outBufferLen, hue_resp3, g_uid);

		addLogAdv(LOG_ALL, LOG_FEATURE_HTTP, "HUE - Sending[3] %s", buffer_out);
		DRV_SSDP_SendReply(addr, buffer_out);
	}
}


void HUE_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState) {
	if(bPreState)
		return;
	hprintf255(request, "<h4>HUE: searches %i, setup %i, api %i, changes %i</h4>",
		stat_searchesReceived, stat_setupXMLVisits, stat_apiCalls, stat_lightChanges);

}

const char *g_hue_setup_1 = "<?xml version=\"1.0\"?>"
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
"<specVersion>"
"<major>1</major>"
"<minor>0</minor>"
"</specVersion>"
"<URLBase>http://";
// IP ADDR HERE
const char *g_hue_setup_2 = ":80/</URLBase>"
"<device>"
"<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
"<friendlyName>Amazon-Echo-HA-Bridge (";
// IP ADDR HERE
const char *g_hue_setup_3 = ")</friendlyName>"
"<manufacturer>Royal Philips Electronics</manufacturer>"
"<manufacturerURL>http://www.philips.com</manufacturerURL>"
"<modelDescription>Philips hue Personal Wireless Lighting</modelDescription>"
"<modelName>Philips hue bridge 2012</modelName>"
"<modelNumber>929000226503</modelNumber>"
"<serialNumber>";
// SERIAL here
const char *g_hue_setup_4 = "</serialNumber>"
"<UDN>uuid:";
// UID HERE
const char *g_hue_setup_5 = "</UDN>"
   "</device>"
   "</root>\r\n"
   "\r\n";

static int HUE_Setup(http_request_t* request) {
	http_setup(request, httpMimeTypeXML);
	poststr(request, g_hue_setup_1);
	poststr(request, HAL_GetMyIPString());
	poststr(request, g_hue_setup_2);
	poststr(request, HAL_GetMyIPString());
	poststr(request, g_hue_setup_3);
	poststr(request, g_serial);
	poststr(request, g_hue_setup_4);
	poststr(request, g_uid);
	poststr(request, g_hue_setup_5);
	poststr(request, NULL);

	stat_setupXMLVisits++;

	return 0;
}
// Hue API for Alexa (and other Hue clients). The Echo, after finding description.xml, does:
//   POST /api                         -> register, we hand out our fixed username
//   GET  /api/<user>                  -> whole bridge state (lights + config)
//   GET  /api/<user>/lights           -> list of lights; we expose the LED driver as light "1"
//   GET  /api/<user>/lights/1         -> one light
//   PUT  /api/<user>/lights/1/state   -> {"on":true,"bri":127,"ct":366}
// Replies follow Tasmota's Hue emulation, which Echo devices accept.

// OpenBeken's own REST API also lives under /api/ - never claim those requests.
static const char *g_obkRestNames[] = {
	"bt_scan", "channels", "channelTypes", "cmnd", "del", "flash", "fsblock", "info",
	"lfs", "logconfig", "ota", "pins", "reboot", "run", "seriallog", 0
};

static bool HUE_IsOBKRestName(const char *seg, int len) {
	int i;
	for (i = 0; g_obkRestNames[i]; i++) {
		if ((int)strlen(g_obkRestNames[i]) == len && !strncmp(seg, g_obkRestNames[i], len))
			return true;
	}
	return false;
}

// Hue bri is 1..254, OpenBeken dimmer is 0..100
static int HUE_DimmerToBri(float dimmer) {
	int bri = (int)(dimmer * 254.0f / 100.0f + 0.5f);
	if (bri < 1) bri = 1;
	if (bri > 254) bri = 254;
	return bri;
}
static int HUE_BriToDimmer(int bri) {
	int dimmer = (bri * 100 + 127) / 254;
	if (dimmer < 1) dimmer = 1;
	if (dimmer > 100) dimmer = 100;
	return dimmer;
}

// Lights with warm and cool white (2 PWMs, RGBCW, or a LED driver chip) can do colour temperature.
static bool HUE_SupportsCT() {
	int pwmCount = 0;
	if (LED_IsLedDriverChipRunning())
		return true;
	PIN_get_Relay_PWM_Count(0, &pwmCount, 0);
	return pwmCount == 2 || pwmCount >= 4;
}

static void HUE_PostJsonString(http_request_t* request, const char *s) {
	char tmp[2] = { 0, 0 };
	poststr(request, "\"");
	for (; s && *s; s++) {
		if (*s == '"' || *s == '\\') {
			poststr(request, "\\");
		}
		if ((unsigned char)*s < 0x20)
			continue;
		tmp[0] = *s;
		poststr(request, tmp);
	}
	poststr(request, "\"");
}

static void HUE_PostLight(http_request_t* request) {
	bool ct = HUE_SupportsCT();
	int mireds;

	hprintf255(request, "{\"state\":{\"on\":%s,\"bri\":%i,",
		LED_GetEnableAll() ? "true" : "false", HUE_DimmerToBri(LED_GetDimmer()));
	if (ct) {
		mireds = (int)LED_GetTemperature();
		if (mireds < 153) mireds = 153;
		if (mireds > 500) mireds = 500;
		hprintf255(request, "\"ct\":%i,\"colormode\":\"ct\",", mireds);
	}
	poststr(request, "\"alert\":\"none\",\"effect\":\"none\",\"reachable\":true},");
	hprintf255(request, "\"type\":\"%s\",\"name\":", ct ? "Color temperature light" : "Dimmable light");
	HUE_PostJsonString(request, CFG_GetDeviceName());
	// uniqueid: MAC plus endpoint, the same shape Tasmota uses
	hprintf255(request, ",\"modelid\":\"%s\",\"manufacturername\":\"OpenBeken\",\"productname\":\"%s\","
		"\"uniqueid\":\"%c%c:%c%c:%c%c:%c%c:%c%c:%c%c:00:11-01\",\"swversion\":\"1.0\"}",
		ct ? "LTW001" : "LWB010", ct ? "Hue white ambiance lamp" : "Hue white lamp",
		g_serial[0], g_serial[1], g_serial[2], g_serial[3], g_serial[4], g_serial[5],
		g_serial[6], g_serial[7], g_serial[8], g_serial[9], g_serial[10], g_serial[11]);
}

static void HUE_PostLights(http_request_t* request) {
	poststr(request, "{");
	if (LED_IsLEDRunning()) {
		poststr(request, "\"1\":");
		HUE_PostLight(request);
	}
	poststr(request, "}");
}

static void HUE_PostConfig(http_request_t* request) {
	hprintf255(request, "{\"name\":\"Philips hue\",\"mac\":\"%c%c:%c%c:%c%c:%c%c:%c%c:%c%c\",",
		g_serial[0], g_serial[1], g_serial[2], g_serial[3], g_serial[4], g_serial[5],
		g_serial[6], g_serial[7], g_serial[8], g_serial[9], g_serial[10], g_serial[11]);
	hprintf255(request, "\"dhcp\":true,\"ipaddress\":\"%s\",\"netmask\":\"%s\",",
		HAL_GetMyIPString(), HAL_GetMyMaskString());
	hprintf255(request, "\"gateway\":\"%s\",\"proxyaddress\":\"none\",\"proxyport\":0,\"bridgeid\":\"%s\",",
		HAL_GetMyGatewayString(), g_bridgeID);
	hprintf255(request, "\"UTC\":\"2024-01-01T00:00:00\",\"whitelist\":{\"%s\":{\"last use date\":\"2024-01-01T00:00:00\","
		"\"create date\":\"2024-01-01T00:00:00\",\"name\":\"Remote\"}},", g_userID);
	poststr(request, "\"swversion\":\"01041302\",\"apiversion\":\"1.17.0\",\"swupdate\":{\"updatestate\":0,\"url\":\"\","
		"\"text\":\"\",\"notify\":false},\"linkbutton\":false,\"portalservices\":false}");
}

static int HUE_Reply_Json(http_request_t* request, void (*body)(http_request_t*)) {
	http_setup(request, httpMimeTypeJson);
	body(request);
	poststr(request, NULL);
	return 1;
}

static void HUE_PostGlobal(http_request_t* request) {
	poststr(request, "{\"lights\":");
	HUE_PostLights(request);
	poststr(request, ",\"groups\":{},\"schedules\":{},\"config\":");
	HUE_PostConfig(request);
	poststr(request, "}");
}

static int HUE_Error(http_request_t* request, int type, const char *address, const char *description) {
	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "[{\"error\":{\"type\":%i,\"address\":\"%s\",\"description\":\"%s\"}}]", type, address, description);
	poststr(request, NULL);
	return 1;
}

// Minimal JSON field readers for the small bodies Hue clients send, e.g. {"on":true,"bri":127}
static const char *HUE_FindJsonValue(const char *json, const char *key) {
	char pattern[16];
	const char *p;
	snprintf(pattern, sizeof(pattern), "\"%s\"", key);
	p = strstr(json, pattern);
	if (p == 0)
		return 0;
	p += strlen(pattern);
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
		p++;
	if (*p != ':')
		return 0;
	p++;
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
		p++;
	return p;
}

static int HUE_SetState(http_request_t* request) {
	char body[256];
	const char *v;
	int len = request->bodylen;
	bool first = true;

	if (request->bodystart == 0 || len <= 0)
		return HUE_Error(request, 5, "/lights/1/state", "invalid/missing parameters in body");
	if (len >= (int)sizeof(body))
		len = sizeof(body) - 1;
	memcpy(body, request->bodystart, len);
	body[len] = 0;

	stat_lightChanges++;
	http_setup(request, httpMimeTypeJson);
	poststr(request, "[");

	v = HUE_FindJsonValue(body, "on");
	if (v) {
		int on = (*v == 't');
		LED_SetEnableAll(on);
		hprintf255(request, "{\"success\":{\"/lights/1/state/on\":%s}}", on ? "true" : "false");
		first = false;
	}
	v = HUE_FindJsonValue(body, "bri");
	if (v) {
		int bri = atoi(v);
		if (bri < 1) bri = 1;
		if (bri > 254) bri = 254;
		LED_SetDimmer(HUE_BriToDimmer(bri));
		// Alexa's "set the lamp to 40%" may not include "on"; a real Hue bulb would stay off, but that
		// would make the request appear to do nothing, so switch on like Tasmota does.
		if (HUE_FindJsonValue(body, "on") == 0 && !LED_GetEnableAll())
			LED_SetEnableAll(1);
		hprintf255(request, "%s{\"success\":{\"/lights/1/state/bri\":%i}}", first ? "" : ",", bri);
		first = false;
	}
	v = HUE_FindJsonValue(body, "ct");
	if (v && HUE_SupportsCT()) {
		int ct = atoi(v);
		if (ct < (int)led_temperature_min) ct = (int)led_temperature_min;
		if (ct > (int)led_temperature_max) ct = (int)led_temperature_max;
		LED_SetTemperature(ct, 1);
		hprintf255(request, "%s{\"success\":{\"/lights/1/state/ct\":%i}}", first ? "" : ",", ct);
		first = false;
	}
	poststr(request, "]");
	poststr(request, NULL);
	return 1;
}

// http://192.168.0.213/api/username/lights/1/state
// http://192.168.0.213/description.xml
// Returns 1 if the request was a Hue API call and has been answered.
int HUE_APICall(http_request_t* request) {
	const char *url = request->url;
	const char *seg, *rest;
	int segLen;

	if (g_uid == 0) {
		// not running
		return 0;
	}
	if (url == 0 || strncmp(url, "api", 3) || (url[3] != 0 && url[3] != '/' && url[3] != '?'))
		return 0;

	// "api" itself: registration (POST) or a client probing us (GET)
	if (url[3] == 0 || url[3] == '?' || (url[3] == '/' && (url[4] == 0 || url[4] == '?'))) {
		if (request->method != HTTP_POST && request->method != HTTP_GET)
			return 0;
		stat_apiCalls++;
		http_setup(request, httpMimeTypeJson);
		hprintf255(request, "[{\"success\":{\"username\":\"%s\"}}]", g_userID);
		poststr(request, NULL);
		return 1;
	}

	seg = url + 4;
	rest = strchr(seg, '/');
	segLen = rest ? (int)(rest - seg) : (int)strcspn(seg, "?");
	if (HUE_IsOBKRestName(seg, segLen))
		return 0;

	stat_apiCalls++;
	// "api/config" without a username
	if (segLen == 6 && !strncmp(seg, "config", 6) && rest == 0)
		return HUE_Reply_Json(request, HUE_PostConfig);

	// "api/<user>" - the whole bridge
	if (rest == 0 || rest[1] == 0)
		return HUE_Reply_Json(request, HUE_PostGlobal);
	rest++;

	if (!strncmp(rest, "config", 6))
		return HUE_Reply_Json(request, HUE_PostConfig);
	if (!strncmp(rest, "groups", 6) || !strncmp(rest, "schedules", 9) || !strncmp(rest, "scenes", 6)
		|| !strncmp(rest, "sensors", 7) || !strncmp(rest, "rules", 5)) {
		http_setup(request, httpMimeTypeJson);
		poststr(request, "{}");
		poststr(request, NULL);
		return 1;
	}
	if (strncmp(rest, "lights", 6))
		return HUE_Error(request, 4, "/", "method, GET, not available for resource");
	rest += 6;
	if (*rest == 0 || ((*rest == '/' || *rest == '?') && (rest[1] == 0 || *rest == '?')))
		return HUE_Reply_Json(request, HUE_PostLights);
	if (*rest == '/')
		rest++;

	// only light "1" exists
	if (rest[0] != '1' || (rest[1] != 0 && rest[1] != '/' && rest[1] != '?') || !LED_IsLEDRunning())
		return HUE_Error(request, 3, "/lights", "resource, /lights, not available");
	rest++;
	if (*rest == '/' && !strncmp(rest + 1, "state", 5)) {
		if (request->method == HTTP_PUT || request->method == HTTP_POST)
			return HUE_SetState(request);
	}
	return HUE_Reply_Json(request, HUE_PostLight);
}
// backlog startDriver SSDP; startDriver HUE
// 
void HUE_Init() {
	char tmp[64];
	unsigned char mac[8];

	WiFI_GetMacAddress((char*)mac);
	// username - 
	snprintf(tmp, sizeof(tmp), "%02X%02X%02X",  mac[3], mac[4], mac[5]);
	g_userID = strdup(tmp);
	// SERIAL - as in Tas, full 12 chars of MAC, so 5c cf 7f 13 9f 3d
	snprintf(tmp, sizeof(tmp), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	g_serial = strdup(tmp);
	// BridgeID - as in Tas, full 12 chars of MAC with FFFE inside, so 5C CF 7F FFFE 13 9F 3D
	snprintf(tmp, sizeof(tmp), "%02X%02X%02XFFFE%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	g_bridgeID = strdup(tmp);
	// uuid
	snprintf(tmp, sizeof(tmp), "f6543a06-da50-11ba-8d8f-%s", g_serial);
	g_uid = strdup(tmp);


	//HTTP_RegisterCallback("/api", HTTP_ANY, HUE_APICall);
	HTTP_RegisterCallback("/description.xml", HTTP_GET, HUE_Setup, 0);
}

