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

// Wemo driver for Alexa, requiring btsimonh's SSDP to work
// Based on Tasmota approach
// The procedure is following:
// 1. first MSEARCH over UDP is done
// 2. then obk replies to MSEARCH with page details
// 3. then alexa accesses our XML pages here with GET
// 4. and can change the binary state (0 or 1) with POST
//
// Note: OpenBeken HTTP server does not support SUBSCRIBE/NOTIFY, so state changes made
// outside Alexa may not update in Alexa until queried.

static const char *g_wemo_setup_1 =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
"<root xmlns=\"urn:Belkin:device-1-0\">\r\n"
"<device>\r\n"
"<deviceType>urn:Belkin:device:controllee:1</deviceType>\r\n"
"<friendlyName>";
static const char *g_wemo_setup_2 =
"</friendlyName>\r\n"
"<manufacturer>Belkin International Inc.</manufacturer>\r\n"
"<modelName>Socket</modelName>\r\n"
"<modelNumber>3.1415</modelNumber>\r\n"
"<UDN>uuid:";
static const char *g_wemo_setup_3 =
"</UDN>\r\n"
"<serialNumber>";
static const char *g_wemo_setup_4 =
"</serialNumber>\r\n"
"<presentationURL>http://";
static const char *g_wemo_setup_5 =
":80/</presentationURL>\r\n"
"<binaryState>0</binaryState>\r\n";
static const char *g_wemo_setup_6 =
	"<serviceList>\r\n"
		"<service>\r\n"
			"<serviceType>urn:Belkin:service:basicevent:1</serviceType>\r\n"
			"<serviceId>urn:Belkin:serviceId:basicevent1</serviceId>\r\n"
			"<controlURL>/upnp/control/basicevent1</controlURL>\r\n"
			"<eventSubURL>/upnp/event/basicevent1</eventSubURL>\r\n"
			"<SCPDURL>/eventservice.xml</SCPDURL>\r\n"
		"</service>\r\n"
		"<service>\r\n"
			"<serviceType>urn:Belkin:service:metainfo:1</serviceType>\r\n"
			"<serviceId>urn:Belkin:serviceId:metainfo1</serviceId>\r\n"
			"<controlURL>/upnp/control/metainfo1</controlURL>\r\n"
			"<eventSubURL>/upnp/event/metainfo1</eventSubURL>\r\n"
			"<SCPDURL>/metainfoservice.xml</SCPDURL>\r\n"
		"</service>\r\n"
	"</serviceList>\r\n"
	"</device>\r\n"
"</root>\r\n";

// SSDP reply template (unicast reply to M-SEARCH)
static const char *g_wemo_msearch =
"HTTP/1.1 200 OK\r\n"
"CACHE-CONTROL: max-age=86400\r\n"
"DATE: Fri, 15 Apr 2016 04:56:29 GMT\r\n"
"EXT:\r\n"
"LOCATION: http://%s:80/setup.xml\r\n"
"OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
"01-NLS: b9200ebb-736d-4b93-bf03-835149d13983\r\n"
"SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
"ST: %s\r\n"
"USN: uuid:%s::%s\r\n"
"X-User-Agent: redsonic\r\n"
"\r\n";

static const char *g_wemo_metaService =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
"<scpd xmlns=\"urn:Belkin:service-1-0\">\r\n"
"<specVersion>\r\n"
"<major>1</major>\r\n"
"<minor>0</minor>\r\n"
"</specVersion>\r\n"
"<actionList>\r\n"
"<action>\r\n"
"<name>GetMetaInfo</name>\r\n"
"<argumentList>\r\n"
"<argument>\r\n"
"<name>MetaInfo</name>\r\n"
"<direction>out</direction>\r\n"
"<relatedStateVariable>MetaInfo</relatedStateVariable>\r\n"
"</argument>\r\n"
"</argumentList>\r\n"
"</action>\r\n"
"<action>\r\n"
"<name>GetExtMetaInfo</name>\r\n"
"<argumentList>\r\n"
"<argument>\r\n"
"<name>ExtMetaInfo</name>\r\n"
"<direction>out</direction>\r\n"
"<relatedStateVariable>ExtMetaInfo</relatedStateVariable>\r\n"
"</argument>\r\n"
"</argumentList>\r\n"
"</action>\r\n"
"</actionList>\r\n"
"<serviceStateTable>\r\n"
"<stateVariable sendEvents=\"no\">\r\n"
"<name>MetaInfo</name>\r\n"
"<dataType>string</dataType>\r\n"
"<defaultValue>0</defaultValue>\r\n"
"</stateVariable>\r\n"
"<stateVariable sendEvents=\"no\">\r\n"
"<name>ExtMetaInfo</name>\r\n"
"<dataType>string</dataType>\r\n"
"<defaultValue>0</defaultValue>\r\n"
"</stateVariable>\r\n"
"</serviceStateTable>\r\n"
"</scpd>\r\n";

static const char *g_wemo_eventService =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
"<scpd xmlns=\"urn:Belkin:service-1-0\">\r\n"
"<specVersion><major>1</major><minor>0</minor></specVersion>\r\n"
"<actionList>\r\n"
"<action>\r\n"
"<name>SetBinaryState</name>\r\n"
"<argumentList>\r\n"
"<argument>\r\n"
"<retval/>\r\n"
"<name>BinaryState</name>\r\n"
"<relatedStateVariable>BinaryState</relatedStateVariable>\r\n"
"<direction>in</direction>\r\n"
"</argument>\r\n"
"</argumentList>\r\n"
"</action>\r\n"
"<action>\r\n"
"<name>GetBinaryState</name>\r\n"
"<argumentList>\r\n"
"<argument>\r\n"
"<retval/>\r\n"
"<name>BinaryState</name>\r\n"
"<relatedStateVariable>BinaryState</relatedStateVariable>\r\n"
"<direction>out</direction>\r\n"
"</argument>\r\n"
"</argumentList>\r\n"
"</action>\r\n"
"</actionList>\r\n"
"<serviceStateTable>\r\n"
"<stateVariable sendEvents=\"yes\">\r\n"
"<name>BinaryState</name>\r\n"
"<dataType>bool</dataType>\r\n"
"<defaultValue>0</defaultValue>\r\n"
"</stateVariable>\r\n"
"<stateVariable sendEvents=\"yes\">\r\n"
"<name>level</name>\r\n"
"<dataType>string</dataType>\r\n"
"<defaultValue>0</defaultValue>\r\n"
"</stateVariable>\r\n"
"</serviceStateTable>\r\n"
"</scpd>\r\n";

static const char *g_wemo_response_1 =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
"<s:Body>";

static const char *g_wemo_response_2_fmt =
"<u:%cetBinaryStateResponse xmlns:u=\"urn:Belkin:service:basicevent:1\">"
"<BinaryState>%i</BinaryState>"
"</u:%cetBinaryStateResponse>"
"</s:Body>"
"</s:Envelope>\r\n";

// Minimal SOAP response templates for MetaInfo (used by /upnp/control/metainfo1)
static const char *g_wemo_metainfo_response_1 =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
"<s:Body>";

static const char *g_wemo_metainfo_response_2_fmt =
"<u:%sResponse xmlns:u=\"urn:Belkin:service:metainfo:1\">"
"<%s>%s</%s>"
"</u:%sResponse>"
"</s:Body>"
"</s:Envelope>";

// ------------------------------------------------------------
// Globals / stats
// ------------------------------------------------------------

static char *g_serial = 0;
static char *g_uid = 0;
static int g_wemo_enabled = 0;
static int outBufferLen = 0;
static char *buffer_out = 0;

static int stat_searchesReceived = 0;
static int stat_setupXMLVisits = 0;
static int stat_eventsReceived = 0;
static int stat_metaServiceXMLVisits = 0;
static int stat_eventServiceXMLVisits = 0;

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

static const char *WEMO_FindHeader(http_request_t *request, const char *namePrefix) {
	int i;
	if (!request || !namePrefix) return 0;
	for (i = 0; i < request->numheaders; i++) {
		const char *h = request->headers[i];
		if (!h) continue;
		if (!my_strnicmp(h, namePrefix, strlen(namePrefix))) {
			return h;
		}
	}
	return 0;
}

static void WEMO_PostXMLSafe(http_request_t *request, const char *s) {
	const char *p;
	if (!s) return;
	for (p = s; *p; p++) {
		switch (*p) {
			case '&': poststr(request, "&amp;"); break;
			case '<': poststr(request, "&lt;"); break;
			case '>': poststr(request, "&gt;"); break;
			case '"': poststr(request, "&quot;"); break;
			case '\'': poststr(request, "&apos;"); break;
			default: {
				char c[2];
				c[0] = *p;
				c[1] = 0;
				poststr(request, c);
			} break;
		}
	}
}

static int WEMO_FindMainChannel(void) {
	int i;
	// Prefer a relay-type channel; fallback to toggle type.
	for (i = 0; i < CHANNEL_MAX; i++) {
		if (h_isChannelRelay(i) || CHANNEL_IsPowerRelayChannel(i)) {
			return i;
		}
	}
	for (i = 0; i < CHANNEL_MAX; i++) {
		if (CHANNEL_GetType(i) == ChType_Toggle) {
			return i;
		}
	}
	return -1;
}

static int WEMO_GetMainPowerState(void) {
	int ch;
#if ENABLE_LED_BASIC
	if (LED_IsLEDRunning()) {
		return LED_GetEnableAll() ? 1 : 0;
	}
#endif
	ch = WEMO_FindMainChannel();
	if (ch >= 0) {
		return CHANNEL_Get(ch) ? 1 : 0;
	}
	return 0;
}

static void WEMO_SetMainPowerState(int v) {
	int ch;
#if ENABLE_LED_BASIC
	if (LED_IsLEDRunning()) {
		LED_SetEnableAll(v ? 1 : 0);
		return;
	}
#endif
	ch = WEMO_FindMainChannel();
	if (ch >= 0) {
		CHANNEL_Set(ch, v ? 1 : 0, 0);
	}
}

static int WEMO_ParseBinaryStateFromXML(const char *xml, int *outState) {
	const char *p;
	if (!xml || !outState) return 0;

	// Common forms:
	//  - <BinaryState>1</BinaryState>
	//  - ...State>1</Binary...
	p = strstr(xml, "<BinaryState>");
	if (p) {
		p += strlen("<BinaryState>");
		*outState = atoi(p) ? 1 : 0;
		return 1;
	}
	p = strstr(xml, "State>1</Binary");
	if (p) { *outState = 1; return 1; }
	p = strstr(xml, "State>0</Binary");
	if (p) { *outState = 0; return 1; }

	return 0;
}

static char WEMO_DetermineSoapVerb(http_request_t *request, const char *xmlBody) {
	const char *soap = 0;
	// Prefer body detection
	if (xmlBody) {
		if (strstr(xmlBody, "SetBinaryState")) return 'S';
		if (strstr(xmlBody, "GetBinaryState")) return 'G';
	}
	// Fallback to SOAPAction header
	soap = WEMO_FindHeader(request, "SOAPACTION:");
	if (soap) {
		if (strstr(soap, "SetBinaryState")) return 'S';
		if (strstr(soap, "GetBinaryState")) return 'G';
	}
	// Default to Get
	return 'G';
}

// ------------------------------------------------------------
// SSDP integration
// ------------------------------------------------------------

void DRV_WEMO_Send_Advert_To(int mode, struct sockaddr_in *addr) {
	const char *useType;

	if (g_uid == 0) {
		// not running
		return;
	}

	if (!g_wemo_enabled) {
		return;
	}

	stat_searchesReceived++;

	// mode is decided by SSDP driver (historically: 1=Belkin device wildcard, 0=rootdevice).
	// For compatibility, when asked for Belkin wildcard, also reply for explicit controllee.
	if (mode == 1) {
		// 1) urn:Belkin:device:**
		useType = "urn:Belkin:device:**";

		if (buffer_out == 0) {
			outBufferLen = (int)strlen(g_wemo_msearch) + 256;
			buffer_out = (char*)malloc(outBufferLen);
		}
		snprintf(buffer_out, outBufferLen, g_wemo_msearch, HAL_GetMyIPString(), useType, g_uid, useType);
		DRV_SSDP_SendReply(addr, buffer_out);

		// 2) urn:Belkin:device:controllee:1
		useType = "urn:Belkin:device:controllee:1";
		snprintf(buffer_out, outBufferLen, g_wemo_msearch, HAL_GetMyIPString(), useType, g_uid, useType);
		DRV_SSDP_SendReply(addr, buffer_out);
	}
	else {
		useType = "upnp:rootdevice";

		if (buffer_out == 0) {
			outBufferLen = (int)strlen(g_wemo_msearch) + 256;
			buffer_out = (char*)malloc(outBufferLen);
		}
		snprintf(buffer_out, outBufferLen, g_wemo_msearch, HAL_GetMyIPString(), useType, g_uid, useType);
		DRV_SSDP_SendReply(addr, buffer_out);
	}

	addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_HTTP, "WEMO - sent SSDP reply %s", useType);
}

void WEMO_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState) {
	if (bPreState)
		return;
	hprintf255(request, "<h4>WEMO: searches %i, setup %i, events %i, mService %i, event %i </h4>",
		stat_searchesReceived, stat_setupXMLVisits, stat_eventsReceived, stat_metaServiceXMLVisits, stat_eventServiceXMLVisits);
}

// ------------------------------------------------------------
// HTTP callbacks
// ------------------------------------------------------------

static int WEMO_BasicEvent1(http_request_t* request) {
	if (!g_wemo_enabled) {
		return http_rest_error(request, 404, "Not Found");
	}
	const char* cmd = request->bodystart;
	char verb;
	int desiredState = -1;

	// Avoid log spam at INFO; SOAP bodies are large.
	addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_HTTP, "Wemo post event %s", cmd ? cmd : "(null)");

	verb = WEMO_DetermineSoapVerb(request, cmd);

	if (verb == 'S') {
		if (WEMO_ParseBinaryStateFromXML(cmd, &desiredState)) {
			WEMO_SetMainPowerState(desiredState);
		}
	}

	// Always respond with the live state.
	http_setup(request, httpMimeTypeXML);
	poststr(request, g_wemo_response_1);
	hprintf255(request, g_wemo_response_2_fmt, verb, WEMO_GetMainPowerState(), verb);
	poststr(request, NULL);

	stat_eventsReceived++;
	return 0;
}

static int WEMO_MetaInfo1(http_request_t* request)
{
	if (!g_wemo_enabled) {
		return http_rest_error(request, 404, "Not Found");
	}

	const char *cmd = request->bodystart;
	if (!cmd) {
		return http_rest_error(request, 400, "Bad Request");
	}

	// Alexa typically doesn't call this, but some UPnP stacks will.
	// We implement minimal GetMetaInfo / GetExtMetaInfo for correctness.
	const char *action = "GetMetaInfo";
	const char *tag = "MetaInfo";
	if (strstr(cmd, "GetExtMetaInfo") != 0) {
		action = "GetExtMetaInfo";
		tag = "ExtMetaInfo";
	}

	http_setup(request, httpMimeTypeXML);
	poststr(request, g_wemo_metainfo_response_1);
	// Value kept intentionally simple; this endpoint is informational.
	hprintf255(request, g_wemo_metainfo_response_2_fmt, action, tag, "0", tag, action);
	poststr(request, NULL);
	return 0;
}

static int WEMO_EventService(http_request_t* request) {
	if (!g_wemo_enabled) {
		return http_rest_error(request, 404, "Not Found");
	}
	http_setup(request, httpMimeTypeXML);
	poststr(request, g_wemo_eventService);
	poststr(request, NULL);

	stat_eventServiceXMLVisits++;
	return 0;
}

static int WEMO_MetaInfoService(http_request_t* request) {
	if (!g_wemo_enabled) {
		return http_rest_error(request, 404, "Not Found");
	}
	http_setup(request, httpMimeTypeXML);
	poststr(request, g_wemo_metaService);
	poststr(request, NULL);

	stat_metaServiceXMLVisits++;
	return 0;
}

static int WEMO_Setup(http_request_t* request) {
	if (!g_wemo_enabled) {
		return http_rest_error(request, 404, "Not Found");
	}
	const char *fname;
	int ch;

	http_setup(request, httpMimeTypeXML);

	// Prefer the label of the main relay channel (if present); fallback to device name.
	ch = WEMO_FindMainChannel();
	fname = 0;
	if (ch >= 0 && CHANNEL_HasLabel(ch)) {
		fname = CHANNEL_GetLabel(ch);
	}
	if (!fname || !*fname) {
		fname = CFG_GetDeviceName();
	}

	poststr(request, g_wemo_setup_1);
	WEMO_PostXMLSafe(request, fname);
	poststr(request, g_wemo_setup_2);
	poststr(request, g_uid);
	poststr(request, g_wemo_setup_3);
	poststr(request, g_serial);
	poststr(request, g_wemo_setup_4);
	poststr(request, HAL_GetMyIPString());
	poststr(request, g_wemo_setup_5);
	poststr(request, g_wemo_setup_6);
	poststr(request, NULL);

	stat_setupXMLVisits++;
	return 0;
}

void WEMO_Init() {
	// (Re)enable Wemo emulation.
	g_wemo_enabled = 1;
	char uid[64];
	char serial[32];
	unsigned char mac[8];

	WiFI_GetMacAddress((char*)mac);
	snprintf(serial, sizeof(serial), "201612%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
	snprintf(uid, sizeof(uid), "Socket-1_0-%s", serial);

	g_serial = strdup(serial);
	g_uid = strdup(uid);

	HTTP_RegisterCallback("/upnp/control/basicevent1", HTTP_POST, WEMO_BasicEvent1, 0);
	HTTP_RegisterCallback("/upnp/control/metainfo1", HTTP_POST, WEMO_MetaInfo1, 0);
	HTTP_RegisterCallback("/eventservice.xml", HTTP_GET, WEMO_EventService, 0);
	HTTP_RegisterCallback("/metainfoservice.xml", HTTP_GET, WEMO_MetaInfoService, 0);
	HTTP_RegisterCallback("/setup.xml", HTTP_GET, WEMO_Setup, 0);
}

void WEMO_Shutdown(void) {
	// OpenBeken cannot unregister HTTP callbacks at runtime.
	// Disable responses/SSDP adverts without freeing identity strings (callbacks may still be hit).
	g_wemo_enabled = 0;
}
