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
#include "../cJSON/cJSON.h"

// Hue driver for Alexa, requiring btsimonh's SSDP to work
// Based on Tasmota approach
// The procedure is following:
// 1. first MSEARCH over UDP is done
// 2. then obk replies to MSEARCH with page details
// 3. then alexa accesses our XML pages here with GET
// 4. and can change the binary state (0 or 1) with POST

static char *buffer_out = 0;
// Filled in by HUE_Init; empty until the driver is started.
static char g_serial[13];
static char g_userID[7];
static char g_uid[37];
static char g_bridgeID[17];
static char g_mac[18];		// "AA:BB:CC:DD:EE:FF"
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

// The search-target lines that follow hue_resp. ARGUMENTS: uuid (used once or twice)
static const char *hue_resp_st[] = {
	"ST: upnp:rootdevice\r\nUSN: uuid:%s::upnp:rootdevice\r\n\r\n",
	"ST: uuid:%s\r\nUSN: uuid:%s\r\n\r\n",
	"ST: urn:schemas-upnp-org:device:basic:1\r\nUSN: uuid:%s\r\n\r\n",
};

void DRV_HUE_Send_Advert_To(struct sockaddr_in *addr) {
	int i;

	if (g_uid[0] == 0) {
		// not running
		return;
	}

	stat_searchesReceived++;

	if (buffer_out == 0) {
		outBufferLen = strlen(hue_resp) + 256;
		buffer_out = (char*)malloc(outBufferLen);
		if (buffer_out == 0)
			return;
	}
	// ARGUMENTS: first IP, then bridgeID
	snprintf(buffer_out, outBufferLen, hue_resp, HAL_GetMyIPString(), g_bridgeID);
	for (i = -1; i < 3; i++) {
		if (i >= 0)
			snprintf(buffer_out, outBufferLen, hue_resp_st[i], g_uid, g_uid);
		addLogAdv(LOG_ALL, LOG_FEATURE_HTTP, "HUE - Sending %s", buffer_out);
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
//   GET  /api/<user>/lights           -> list of lights
//   GET  /api/<user>/lights/<id>      -> one light
//   PUT  /api/<user>/lights/<id>/state -> {"on":true,"bri":127,"ct":366}
// Lights: the LED driver (if running) is light 1, then every relay channel, in channel order, as an
// on/off plug. Replies follow Tasmota's Hue emulation, which Echo devices accept.

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

#if ENABLE_LED_BASIC
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
#endif

#define HUE_MAX_LIGHTS 16

typedef struct hueLight_s {
	bool isLED;
	int channel;	// relay channel; unused for the LED light
	int relayIndex;	// 1-based number among the relays; 0 for the LED light
} hueLight_t;

// A channel becomes a Hue plug if it switches a relay (or an H-bridge), or is a toggle channel.
// Status LED pins alone are not exposed, so a plug's indicator doesn't show up as a second device.
static bool HUE_IsRelayChannel(int ch) {
	return CHANNEL_HasChannelPinWithRoleOrRole(ch, IOR_Relay, IOR_Relay_n)
		|| CHANNEL_HasChannelPinWithRoleOrRole(ch, IOR_BridgeForward, IOR_BridgeReverse)
		|| CHANNEL_GetType(ch) == ChType_Toggle;
}

static int HUE_GetLights(hueLight_t *lights) {
	int count = 0;
	int ch;

#if ENABLE_LED_BASIC
	if (LED_IsLEDRunning()) {
		lights[count].isLED = true;
		lights[count].channel = -1;
		lights[count].relayIndex = 0;
		count++;
	}
#endif
	for (ch = 0; ch < CHANNEL_MAX && count < HUE_MAX_LIGHTS; ch++) {
		if (HUE_IsRelayChannel(ch)) {
			lights[count].isLED = false;
			lights[count].channel = ch;
			lights[count].relayIndex = count + 1 - (lights[0].isLED ? 1 : 0);
			count++;
		}
	}
	return count;
}

// relays always follow the LED light, so the last light's number is the relay count
static int HUE_CountRelays(const hueLight_t *lights, int count) {
	return count ? lights[count - 1].relayIndex : 0;
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

#if ENABLE_LED_BASIC
static void HUE_PostLEDState(http_request_t* request, bool ct) {
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
}
#endif

// id is the Hue light number (1-based); relayCount is how many relays the device has.
static void HUE_PostLight(http_request_t* request, const hueLight_t *light, int id, int relayCount) {
	char name[96];
	const char *type, *modelid;

#if ENABLE_LED_BASIC
	if (light->isLED) {
		bool ct = HUE_SupportsCT();
		HUE_PostLEDState(request, ct);
		type = ct ? "Color temperature light" : "Dimmable light";
		modelid = ct ? "LTW001" : "LWB010";
		snprintf(name, sizeof(name), "%s", CFG_GetDeviceName());
	}
	else
#endif
	{
		hprintf255(request, "{\"state\":{\"on\":%s,\"alert\":\"none\",\"reachable\":true},",
			CHANNEL_Get(light->channel) ? "true" : "false");
		type = "On/Off plug-in unit";
		modelid = "LOM001";
		// one relay: the device's name; several: the channel label, or the device name plus a number
		if (CHANNEL_HasLabel(light->channel))
			snprintf(name, sizeof(name), "%s", CHANNEL_GetLabel(light->channel));
		else if (relayCount == 1)
			snprintf(name, sizeof(name), "%s", CFG_GetDeviceName());
		else
			snprintf(name, sizeof(name), "%s %i", CFG_GetDeviceName(), light->relayIndex);
	}
	hprintf255(request, "\"type\":\"%s\",\"name\":", type);
	HUE_PostJsonString(request, name);
	// uniqueid: MAC plus endpoint, the same shape Tasmota uses, with the light number as the last byte
	hprintf255(request, ",\"modelid\":\"%s\",\"manufacturername\":\"OpenBeken\","
		"\"uniqueid\":\"%s:00:11-%02x\",\"swversion\":\"1.0\"}",
		modelid, g_mac, id);
}

static void HUE_PostLights(http_request_t* request) {
	hueLight_t lights[HUE_MAX_LIGHTS];
	int count = HUE_GetLights(lights);
	int relays = HUE_CountRelays(lights, count);
	int i;

	poststr(request, "{");
	for (i = 0; i < count; i++) {
		hprintf255(request, "%s\"%i\":", i ? "," : "", i + 1);
		HUE_PostLight(request, &lights[i], i + 1, relays);
	}
	poststr(request, "}");
}

static void HUE_PostConfig(http_request_t* request) {
	hprintf255(request, "{\"name\":\"Philips hue\",\"mac\":\"%s\",", g_mac);
	// one HAL string per call: some platforms return the IP, netmask and gateway in the same buffer
	hprintf255(request, "\"dhcp\":true,\"ipaddress\":\"%s\",", HAL_GetMyIPString());
	hprintf255(request, "\"netmask\":\"%s\",", HAL_GetMyMaskString());
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

#if ENABLE_LED_BASIC
static void HUE_SetLEDState(http_request_t* request, const char *path, cJSON *json, cJSON *on) {
	cJSON *bri = cJSON_GetObjectItemCaseSensitive(json, "bri");
	cJSON *ct = cJSON_GetObjectItemCaseSensitive(json, "ct");
	bool first = true;

	if (on != 0 && cJSON_IsBool(on)) {
		LED_SetEnableAll(cJSON_IsTrue(on));
		hprintf255(request, "{\"success\":{\"%s/on\":%s}}", path, cJSON_IsTrue(on) ? "true" : "false");
		first = false;
	}
	if (bri != 0 && cJSON_IsNumber(bri)) {
		int value = bri->valueint;
		if (value < 1) value = 1;
		if (value > 254) value = 254;
		LED_SetDimmer(HUE_BriToDimmer(value));
		// Alexa's "set the lamp to 40%" may not include "on"; a real Hue bulb would stay off, but that
		// would make the request appear to do nothing, so switch on like Tasmota does.
		if (on == 0 && !LED_GetEnableAll())
			LED_SetEnableAll(1);
		hprintf255(request, "%s{\"success\":{\"%s/bri\":%i}}", first ? "" : ",", path, value);
		first = false;
	}
	if (ct != 0 && cJSON_IsNumber(ct) && HUE_SupportsCT()) {
		int value = ct->valueint;
		if (value < (int)led_temperature_min) value = (int)led_temperature_min;
		if (value > (int)led_temperature_max) value = (int)led_temperature_max;
		LED_SetTemperature(value, 1);
		hprintf255(request, "%s{\"success\":{\"%s/ct\":%i}}", first ? "" : ",", path, value);
	}
}
#endif

// PUT /api/<user>/lights/<id>/state with a body such as {"on":true,"bri":127,"ct":366}
static int HUE_SetState(http_request_t* request, const hueLight_t *light, int id) {
	char body[256];
	char path[48];
	int len = request->bodylen;
	cJSON *json, *on;

	snprintf(path, sizeof(path), "/lights/%i/state", id);
	if (request->bodystart == 0 || len <= 0)
		return HUE_Error(request, 5, path, "invalid/missing parameters in body");
	if (len >= (int)sizeof(body))
		len = sizeof(body) - 1;
	memcpy(body, request->bodystart, len);
	body[len] = 0;

	json = cJSON_Parse(body);
	if (json == 0)
		return HUE_Error(request, 2, path, "body contains invalid json");
	on = cJSON_GetObjectItemCaseSensitive(json, "on");

	stat_lightChanges++;
	http_setup(request, httpMimeTypeJson);
	poststr(request, "[");

	if (light->isLED) {
#if ENABLE_LED_BASIC
		HUE_SetLEDState(request, path, json, on);
#endif
	}
	// relays only switch on and off
	else if (on != 0 && cJSON_IsBool(on)) {
		CHANNEL_Set(light->channel, cJSON_IsTrue(on) ? 1 : 0, 0);
		hprintf255(request, "{\"success\":{\"%s/on\":%s}}", path, cJSON_IsTrue(on) ? "true" : "false");
	}
	poststr(request, "]");
	poststr(request, NULL);
	cJSON_Delete(json);
	return 1;
}

// http://192.168.0.213/api/username/lights/1/state
// http://192.168.0.213/description.xml
// Returns 1 if the request was a Hue API call and has been answered.
int HUE_APICall(http_request_t* request) {
	const char *url = request->url;
	const char *seg, *rest;
	int segLen;

	if (g_uid[0] == 0) {
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

	{
		hueLight_t lights[HUE_MAX_LIGHTS];
		int count = HUE_GetLights(lights);
		char *end;
		long id = strtol(rest, &end, 10);
		char address[24];

		if (end == rest || id < 1 || id > count || (*end != 0 && *end != '/' && *end != '?')) {
			snprintf(address, sizeof(address), "/lights/%.8s", rest);
			return HUE_Error(request, 3, address, "resource not available");
		}
		if (*end == '/' && !strncmp(end + 1, "state", 5)
			&& (request->method == HTTP_PUT || request->method == HTTP_POST)) {
			return HUE_SetState(request, &lights[id - 1], (int)id);
		}
		http_setup(request, httpMimeTypeJson);
		HUE_PostLight(request, &lights[id - 1], (int)id, HUE_CountRelays(lights, count));
		poststr(request, NULL);
		return 1;
	}
}
// backlog startDriver SSDP; startDriver HUE
// 
void HUE_Init() {
	unsigned char mac[8];

	WiFI_GetMacAddress((char*)mac);
	// username - 
	snprintf(g_userID, sizeof(g_userID), "%02X%02X%02X",  mac[3], mac[4], mac[5]);
	// SERIAL - as in Tas, full 12 chars of MAC, so 5c cf 7f 13 9f 3d
	snprintf(g_serial, sizeof(g_serial), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	// BridgeID - as in Tas, full 12 chars of MAC with FFFE inside, so 5C CF 7F FFFE 13 9F 3D
	snprintf(g_bridgeID, sizeof(g_bridgeID), "%02X%02X%02XFFFE%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	snprintf(g_mac, sizeof(g_mac), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	// uuid
	snprintf(g_uid, sizeof(g_uid), "f6543a06-da50-11ba-8d8f-%s", g_serial);


	//HTTP_RegisterCallback("/api", HTTP_ANY, HUE_APICall);
	HTTP_RegisterCallback("/description.xml", HTTP_GET, HUE_Setup, 0);
}

