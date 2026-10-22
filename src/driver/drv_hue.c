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
#include "../jsmn/jsmn_h.h"

// Hue driver for Alexa, requiring btsimonh's SSDP to work
// Based on Tasmota approach
// The procedure is following:
// 1. first MSEARCH over UDP is done
// 2. then obk replies to MSEARCH with page details
// 3. then alexa accesses our XML pages here with GET
// 4. Alexa reads lights with GET and changes their state with PUT.
// Relay enumeration and MAC-derived light IDs adapted from Switch-Gang.

static char *buffer_out = 0;
static char *g_serial = 0;
static char *g_userID = 0;
static char *g_uid = 0;
static char *g_bridgeID = 0;
static unsigned int g_lightBase;
static int outBufferLen = 0;
static int stat_searchesReceived = 0;
static int stat_setupXMLVisits = 0;
// stubs
static int stat_metaServiceXMLVisits = 0;
static int stat_eventsReceived = 0;
static int stat_eventServiceXMLVisits = 0;


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
const  char *hue_resp3 = "ST: urn:schemas-upnp-org:device:Basic:1\r\n"
   "USN: uuid:%s\r\n"
   "\r\n";

void DRV_HUE_Send_Advert_To(struct sockaddr_in *addr) {
	const char *tails[] = { hue_resp1, hue_resp2, hue_resp3 };
	int i, headerLen;
	if (!g_uid)
		return;
	stat_searchesReceived++;
	if (!buffer_out) {
		outBufferLen = 768;
		buffer_out = (char*)malloc(outBufferLen);
		if (!buffer_out)
			return;
	}
	// Each UDP datagram must contain both the HTTP headers and ST/USN.
	for (i = 0; i < 3; i++) {
		headerLen = snprintf(buffer_out, outBufferLen, hue_resp, HAL_GetMyIPString(), g_bridgeID);
		if (headerLen < 0 || headerLen >= outBufferLen)
			return;
		snprintf(buffer_out + headerLen, outBufferLen - headerLen, tails[i], g_uid, g_uid);
		DRV_SSDP_SendReply(addr, buffer_out);
	}
}


void HUE_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState) {
	if(bPreState)
		return;
	hprintf255(request, "<h4>HUE: searches %i, setup %i, events %i, mService %i, event %i </h4>",
		stat_searchesReceived, stat_setupXMLVisits, stat_eventsReceived, stat_metaServiceXMLVisits, stat_eventServiceXMLVisits);

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
// Expose only binary outputs, never sensor/input channels.
static bool HUE_IsLight(int ch) {
	return ch >= 0 && ch < CHANNEL_MAX
		&& (h_isChannelRelay(ch) || CHANNEL_GetType(ch) == ChType_Toggle);
}

static unsigned int HUE_LightId(int ch) {
	// Seven low bits hold channel+1 (including channel 63). The remaining
	// 24 bits come from the MAC. All IDs fit in a positive signed integer.
	return g_lightBase | (unsigned int)(ch + 1);
}

static int HUE_Error(http_request_t *request, int type, const char *description) {
	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "[{\"error\":{\"type\":%d,\"address\":\"/", type);
	poststr_escapedForJSON(request, request->url);
	poststr(request, "\",\"description\":\"");
	poststr(request, description);
	poststr(request, "\"}}]");
	poststr(request, NULL);
	return 1;
}

static void HUE_PrintState(http_request_t *request, int ch) {
	hprintf255(request, "{\"on\":%s,\"alert\":\"none\",\"reachable\":true}",
		CHANNEL_Get(ch) ? "true" : "false");
}

static void HUE_PrintLight(http_request_t *request, int ch) {
	poststr(request, "{\"state\":");
	HUE_PrintState(request, ch);
	poststr(request, ",\"type\":\"On/Off light\",\"name\":\"");
	if (CHANNEL_HasLabel(ch)) {
		poststr_escapedForJSON(request, (char*)CHANNEL_GetLabel(ch));
	} else {
		poststr_escapedForJSON(request, (char*)CFG_GetDeviceName());
		hprintf255(request, " %d", ch);
	}
	hprintf255(request, "\",\"modelid\":\"LOM001\",\"manufacturername\":\"OpenBeken\","
		"\"uniqueid\":\"%s-%02X\",\"swversion\":\"1.0\"}", g_serial, ch);
}

static void HUE_PrintLights(http_request_t *request) {
	int ch;
	bool first = true;
	poststr(request, "{");
	for (ch = 0; ch < CHANNEL_MAX; ch++) {
		if (!HUE_IsLight(ch))
			continue;
		if (!first)
			poststr(request, ",");
		first = false;
		hprintf255(request, "\"%u\":", HUE_LightId(ch));
		HUE_PrintLight(request, ch);
	}
	poststr(request, "}");
}

static void HUE_PrintConfig(http_request_t *request) {
	unsigned char mac[8];
	WiFI_GetMacAddress((char*)mac);
	poststr(request, "{\"name\":\"");
	poststr_escapedForJSON(request, (char*)CFG_GetDeviceName());
	hprintf255(request, "\",\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\",\"dhcp\":true,",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	hprintf255(request, "\"ipaddress\":\"%s\",\"bridgeid\":\"%s\",", HAL_GetMyIPString(), g_bridgeID);
	hprintf255(request, "\"swversion\":\"01041302\",\"apiversion\":\"1.17.0\","
		"\"linkbutton\":true,\"portalservices\":false,\"whitelist\":{\"%s\":{\"name\":\"Alexa\"}}}", g_userID);
}

static int HUE_SkipSpace(const char *body, int pos, int length) {
	while (pos < length && (body[pos] == ' ' || body[pos] == '\t'
		|| body[pos] == '\r' || body[pos] == '\n'))
		pos++;
	return pos;
}

static int HUE_SetState(http_request_t *request, int ch) {
	jsmn_parser parser;
	jsmntok_t tokens[32];
	const char *body = request->bodystart;
	int count, i, pos, on = -1;
	if (!body || request->bodylen <= 0 || request->bodylen < request->contentLength)
		return HUE_Error(request, 2, "Missing or incomplete JSON body");
	jsmn_init(&parser);
	count = jsmn_parse(&parser, body, request->bodylen, tokens, 32);
	if (count < 1 || tokens[0].type != JSMN_OBJECT || count != 1 + 2 * tokens[0].size)
		return HUE_Error(request, 2, "Invalid state object");
	// jsmn tokenizes permissively; also check JSON separators and trailing data.
	pos = HUE_SkipSpace(body, 0, request->bodylen);
	if (pos != tokens[0].start)
		return HUE_Error(request, 2, "Invalid JSON object");
	pos++;
	for (i = 1; i < count; i += 2) {
		pos = HUE_SkipSpace(body, pos, request->bodylen);
		if (tokens[i].type != JSMN_STRING || pos != tokens[i].start - 1)
			return HUE_Error(request, 2, "Invalid JSON property");
		pos = HUE_SkipSpace(body, tokens[i].end + 1, request->bodylen);
		if (pos >= request->bodylen || body[pos++] != ':')
			return HUE_Error(request, 2, "Missing JSON colon");
		pos = HUE_SkipSpace(body, pos, request->bodylen);
		if (pos != tokens[i + 1].start - (tokens[i + 1].type == JSMN_STRING))
			return HUE_Error(request, 2, "Invalid JSON value");
		pos = tokens[i + 1].end + (tokens[i + 1].type == JSMN_STRING);
		pos = HUE_SkipSpace(body, pos, request->bodylen);
		if (i + 2 < count) {
			if (pos >= request->bodylen || body[pos++] != ',')
				return HUE_Error(request, 2, "Missing JSON comma");
		}
	}
	pos = HUE_SkipSpace(body, pos, request->bodylen);
	if (pos >= request->bodylen || body[pos++] != '}'
		|| HUE_SkipSpace(body, pos, request->bodylen) != request->bodylen)
		return HUE_Error(request, 2, "Invalid JSON terminator");
	// Validate every field before touching the hardware. Match complete boolean
	// tokens, accepting whitespace and property ordering without substring hacks.
	for (i = 1; i < count; i += 2) {
		jsmntok_t *value = &tokens[i + 1];
		int len = value->end - value->start;
		if (jsoneq(body, &tokens[i], "on") != 0)
			return HUE_Error(request, 6, "Parameter not available for an On/Off light");
		if (on != -1 || value->type != JSMN_PRIMITIVE)
			return HUE_Error(request, 7, "Invalid on value");
		if (len == 4 && !memcmp(body + value->start, "true", 4))
			on = 1;
		else if (len == 5 && !memcmp(body + value->start, "false", 5))
			on = 0;
		else
			return HUE_Error(request, 7, "Expected a boolean on value");
	}
	if (on == -1)
		return HUE_Error(request, 6, "Missing on parameter");
	CHANNEL_Set(ch, on, 0);
	stat_eventsReceived++;
	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "[{\"success\":{\"/lights/%u/state/on\":%s}}]", HUE_LightId(ch), on ? "true" : "false");
	poststr(request, NULL);
	return 1;
}

int HUE_APICall(http_request_t *request) {
	const char *path, *resource, *end;
	unsigned int id = 0;
	int ch;
	if (!g_uid || !DRV_IsRunning("HUE") || !request->url)
		return 0;
	path = request->url;
	if (*path == '/')
		path++;
	if (strncmp(path, "api", 3) || (path[3] && path[3] != '/'))
		return 0;
	path += 3;
	if (*path == '/' && !path[1])
		path++;
	if (!*path) {
		if (request->method != HTTP_POST)
			return HUE_Error(request, 4, "Use POST to create a user");
		http_setup(request, httpMimeTypeJson);
		hprintf255(request, "[{\"success\":{\"username\":\"%s\"}}]", g_userID);
		poststr(request, NULL);
		return 1;
	}
	// Skip username without modifying the HTTP request or using global strtok.
	path++;
	resource = strchr(path, '/');
	// Native OpenBeken endpoints share /api (info, pins, ota, lfs, cmnd...).
	// Only claim the username issued by this bridge's registration response.
	if ((resource ? (size_t)(resource - path) : strlen(path)) != strlen(g_userID)
		|| strncmp(path, g_userID, strlen(g_userID)))
		return 0;
	if (resource == path || !*path)
		return HUE_Error(request, 3, "Missing username");
	resource = resource ? resource + 1 : "";
	if (!strncmp(resource, "lights/", 7)) {
		path = resource + 7;
		end = path;
		while (*end >= '0' && *end <= '9') {
			// IDs have at most 31 bits; bound each step before multiplication.
			if (id > (2147483647u - (*end - '0')) / 10)
				return HUE_Error(request, 3, "Invalid light ID");
			id = id * 10 + (*end++ - '0');
		}
		ch = (int)(id & 127) - 1;
		if (end == path || !HUE_IsLight(ch) || id != HUE_LightId(ch))
			return HUE_Error(request, 3, "Light does not exist");
		if (!strcmp(end, "/state") && request->method == HTTP_PUT)
			return HUE_SetState(request, ch);
		if (request->method != HTTP_GET)
			return HUE_Error(request, 4, "Method not available");
		if (*end && strcmp(end, "/state"))
			return HUE_Error(request, 3, "Resource does not exist");
		http_setup(request, httpMimeTypeJson);
		if (*end)
			HUE_PrintState(request, ch);
		else
			HUE_PrintLight(request, ch);
	} else {
		if (request->method != HTTP_GET)
			return HUE_Error(request, 4, "Method not available");
		if (*resource && strcmp(resource, "lights") && strcmp(resource, "config")
			&& strcmp(resource, "groups") && strcmp(resource, "schedules")
			&& strcmp(resource, "scenes") && strcmp(resource, "sensors")
			&& strcmp(resource, "rules") && strcmp(resource, "resourcelinks"))
			return HUE_Error(request, 3, "Resource does not exist");
		http_setup(request, httpMimeTypeJson);
		if (!*resource) {
			poststr(request, "{\"lights\":");
			HUE_PrintLights(request);
			poststr(request, ",\"groups\":{},\"schedules\":{},\"scenes\":{},\"sensors\":{},\"rules\":{},\"resourcelinks\":{},\"config\":");
			HUE_PrintConfig(request);
			poststr(request, "}");
		} else if (!strcmp(resource, "lights"))
			HUE_PrintLights(request);
		else if (!strcmp(resource, "config"))
			HUE_PrintConfig(request);
		else
			poststr(request, "{}");
	}
	poststr(request, NULL);
	return 1;
}

// Startup: backlog startDriver SSDP; startDriver Hue
void HUE_Init() {
	char tmp[64];
	unsigned char mac[8];
	if (g_uid)
		return;
	WiFI_GetMacAddress((char*)mac);
	g_lightBase = (((unsigned int)mac[3] << 16) | ((unsigned int)mac[4] << 8) | mac[5]) << 7;
	snprintf(tmp, sizeof(tmp), "%02X%02X%02X", mac[3], mac[4], mac[5]);
	g_userID = strdup(tmp);
	snprintf(tmp, sizeof(tmp), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	g_serial = strdup(tmp);
	snprintf(tmp, sizeof(tmp), "%02X%02X%02XFFFE%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	g_bridgeID = strdup(tmp);
	if (!g_userID || !g_serial || !g_bridgeID) {
		free(g_userID); free(g_serial); free(g_bridgeID);
		g_userID = g_serial = g_bridgeID = NULL;
		return;
	}
	snprintf(tmp, sizeof(tmp), "f6543a06-da50-11ba-8d8f-%s", g_serial);
	g_uid = strdup(tmp);
	if (!g_uid) {
		free(g_userID); free(g_serial); free(g_bridgeID);
		g_userID = g_serial = g_bridgeID = NULL;
		return;
	}
	HTTP_RegisterCallback("/description.xml", HTTP_GET, HUE_Setup, 0);
}
