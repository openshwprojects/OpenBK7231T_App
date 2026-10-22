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

static const char *g_wemo_setup_1 =
"<?xml version=\"1.0\"?>"
"<root xmlns=\"urn:Belkin:device-1-0\">"
"<device>"
"<deviceType>urn:Belkin:device:controllee:1</deviceType>"
"<friendlyName>";
static const char *g_wemo_setup_2 =
"</friendlyName>"
"<manufacturer>Belkin International Inc.</manufacturer>"
"<modelName>Socket</modelName>"
"<modelNumber>3.1415</modelNumber>"
"<UDN>uuid:";
static const char *g_wemo_setup_3 =
"</UDN>"
"<serialNumber>";
static const char *g_wemo_setup_4 =
"</serialNumber>"
"<presentationURL>http://";
static const char *g_wemo_setup_5 =
":80/</presentationURL>";
static const char *g_wemo_setup_6 =
	"<serviceList>"
		"<service>"
			"<serviceType>urn:Belkin:service:basicevent:1</serviceType>"
			"<serviceId>urn:Belkin:serviceId:basicevent1</serviceId>"
			"<controlURL>/upnp/control/basicevent1</controlURL>"
			"<eventSubURL>/upnp/event/basicevent1</eventSubURL>"
			"<SCPDURL>/eventservice.xml</SCPDURL>"
		"</service>"
		"<service>"
			"<serviceType>urn:Belkin:service:metainfo:1</serviceType>"
			"<serviceId>urn:Belkin:serviceId:metainfo1</serviceId>"
			"<controlURL>/upnp/control/metainfo1</controlURL>"
			"<eventSubURL>/upnp/event/metainfo1</eventSubURL>"
			"<SCPDURL>/metainfoservice.xml</SCPDURL>"
		"</service>"
	"</serviceList>"
	"</device>"
"</root>\r\n";

const char *g_wemo_msearch =
"HTTP/1.1 200 OK\r\n"
"CACHE-CONTROL: max-age=86400\r\n"
"EXT:\r\n"
"LOCATION: http://%s:80/setup.xml\r\n"
"OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
"01-NLS: b9200ebb-736d-4b93-bf03-835149d13983\r\n"
"SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
"ST: %s\r\n"                // type1 = urn:Belkin:device:**, type2 = upnp:rootdevice
"USN: uuid:%s::%s\r\n"      // type1 = urn:Belkin:device:**, type2 = upnp:rootdevice
"X-User-Agent: redsonic\r\n"
"\r\n";


const char *g_wemo_metaService =
"<scpd xmlns=\"urn:Belkin:service-1-0\">"
"<specVersion>"
"<major>1</major>"
"<minor>0</minor>"
"</specVersion>"
"<actionList>"
"<action>"
"<name>GetMetaInfo</name>"
"<argumentList>"
"<retval />"
"<name>GetMetaInfo</name>"
"<relatedStateVariable>MetaInfo</relatedStateVariable>"
"<direction>in</direction>"
"</argumentList>"
"</action>"
"</actionList>"
"<serviceStateTable>"
"<stateVariable sendEvents=\"yes\">"
"<name>MetaInfo</name>"
"<dataType>string</dataType>"
"<defaultValue>0</defaultValue>"
"</stateVariable>"
"</serviceStateTable>"
"</scpd>\r\n\r\n";

const char *g_wemo_eventService =
"<scpd xmlns=\"urn:Belkin:service-1-0\">"
"<actionList>"
"<action>"
"<name>SetBinaryState</name>"
"<argumentList>"
"<argument>"
"<retval/>"
"<name>BinaryState</name>"
"<relatedStateVariable>BinaryState</relatedStateVariable>"
"<direction>in</direction>"
"</argument>"
"</argumentList>"
"</action>"
"<action>"
"<name>GetBinaryState</name>"
"<argumentList>"
"<argument>"
"<retval/>"
"<name>BinaryState</name>"
"<relatedStateVariable>BinaryState</relatedStateVariable>"
"<direction>out</direction>"
"</argument>"
"</argumentList>"
"</action>"
"</actionList>"
"<serviceStateTable>"
"<stateVariable sendEvents=\"yes\">"
"<name>BinaryState</name>"
"<dataType>bool</dataType>"
"<defaultValue>0</defaultValue>"
"</stateVariable>"
"<stateVariable sendEvents=\"yes\">"
"<name>level</name>"
"<dataType>string</dataType>"
"<defaultValue>0</defaultValue>"
"</stateVariable>"
"</serviceStateTable>"
"</scpd>\r\n\r\n";
const char *g_wemo_response_1 =
"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
"<s:Body>";

const char *g_wemo_response_2_fmt =
"<u:%cetBinaryStateResponse xmlns:u=\"urn:Belkin:service:basicevent:1\">"
"<BinaryState>%i</BinaryState>"
"</u:%cetBinaryStateResponse>"
"</s:Body>"
"</s:Envelope>\r\n";

static char *g_serial = 0;
static char *g_uid = 0;
static int outBufferLen = 0;
static char *buffer_out = 0;
static int stat_searchesReceived = 0;
static int stat_setupXMLVisits = 0;
static int stat_metaServiceXMLVisits = 0;
static int stat_eventsReceived = 0;
static int stat_eventServiceXMLVisits = 0;

void DRV_WEMO_Send_Advert_To(int mode, struct sockaddr_in *addr) {
	const char *useType;

	if (g_uid == 0) {
		// not running
		return;
	}

	stat_searchesReceived++;

	if (mode == 1) {
		useType = "urn:Belkin:device:**";
	}
	else if (mode == 3) {
		useType = "urn:Belkin:device:controllee:1";
	}
	else {
		useType = "upnp:rootdevice";
	}

	addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_HTTP, "WEMO - sends reply %s",useType);

	if (buffer_out == 0) {
		outBufferLen = strlen(g_wemo_msearch) + 256;
		buffer_out = (char*)malloc(outBufferLen);
		if (!buffer_out)
			return;
	}
	snprintf(buffer_out, outBufferLen, g_wemo_msearch, HAL_GetMyIPString(), useType, g_uid, useType);

	addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_HTTP, "WEMO - Sending %s", buffer_out);
	DRV_SSDP_SendReply(addr, buffer_out);
}

void WEMO_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState) {
	if(bPreState)
		return;
	hprintf255(request, "<h4>WEMO: searches %i, setup %i, events %i, mService %i, event %i </h4>",
		stat_searchesReceived, stat_setupXMLVisits, stat_eventsReceived, stat_metaServiceXMLVisits, stat_eventServiceXMLVisits);

}


bool Main_GetFirstPowerState() {
	int i;
#if ENABLE_LED_BASIC
	if (LED_IsLEDRunning()) {
		return LED_GetEnableAll();
	}
	else 
#endif
	{
		// relays driver
		for (i = 0; i < CHANNEL_MAX; i++) {
			if (h_isChannelRelay(i) || CHANNEL_GetType(i) == ChType_Toggle) {
				return CHANNEL_Get(i);
			}
		}
	}
	return 0;
}
static int WEMO_Fault(http_request_t *request) {
	request->responseCode = HTTP_RESPONSE_SERVER_ERROR;
	http_setup(request, httpMimeTypeXML);
	poststr(request, g_wemo_response_1);
	poststr(request, "<s:Fault><faultcode>s:Client</faultcode><faultstring>UPnPError</faultstring>"
		"<detail><UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
		"<errorCode>402</errorCode><errorDescription>Invalid Args</errorDescription>"
		"</UPnPError></detail></s:Fault></s:Body></s:Envelope>");
	poststr(request, NULL);
	return 0;
}

static void WEMO_SetFirstPowerState(int on) {
	int ch;
#if ENABLE_LED_BASIC
	if (LED_IsLEDRunning()) {
		CMD_ExecuteCommand(on ? "POWER ON" : "POWER OFF", 0);
		return;
	}
#endif
	// Wemo advertises one socket: control exactly the output reported by Get.
	for (ch = 0; ch < CHANNEL_MAX; ch++) {
		if (h_isChannelRelay(ch) || CHANNEL_GetType(ch) == ChType_Toggle) {
			CHANNEL_Set(ch, on, 0);
			return;
		}
	}
}

static int WEMO_BasicEvent1(http_request_t* request) {
	char *cmd, *value, *end;
	char letter = 'G';
	int on;
	if (!request->bodystart || request->bodylen <= 0
		|| request->bodylen < request->contentLength)
		return WEMO_Fault(request);
	// HTTP buffers can contain a partial body and need not terminate at bodylen.
	cmd = malloc(request->bodylen + 1);
	if (!cmd)
		return WEMO_Fault(request);
	memcpy(cmd, request->bodystart, request->bodylen);
	cmd[request->bodylen] = 0;
	if (strstr(cmd, "SetBinaryState")) {
		letter = 'S';
		value = strstr(cmd, "<BinaryState>");
		if (!value) {
			free(cmd);
			return WEMO_Fault(request);
		}
		value += strlen("<BinaryState>");
		while (*value == ' ' || *value == '\t' || *value == '\r' || *value == '\n')
			value++;
		on = *value - '0';
		end = *value ? value + 1 : value;
		while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
			end++;
		if ((on != 0 && on != 1) || strncmp(end, "</BinaryState>", 14)) {
			free(cmd);
			return WEMO_Fault(request);
		}
		WEMO_SetFirstPowerState(on);
	} else if (!strstr(cmd, "GetBinaryState")) {
		free(cmd);
		return WEMO_Fault(request);
	}
	free(cmd);
	http_setup(request, httpMimeTypeXML);
	poststr(request, g_wemo_response_1);
	hprintf255(request, g_wemo_response_2_fmt, letter, Main_GetFirstPowerState(), letter);
	poststr(request, NULL);
	stat_eventsReceived++;
	return 0;
}
static int WEMO_EventService(http_request_t* request) {

	http_setup(request, httpMimeTypeXML);
	poststr(request, g_wemo_eventService);
	poststr(request, NULL);

	stat_eventServiceXMLVisits++;

	return 0;
}
static int WEMO_MetaInfoService(http_request_t* request) {

	http_setup(request, httpMimeTypeXML);
	poststr(request, g_wemo_metaService);
	poststr(request, NULL);

	stat_metaServiceXMLVisits++;

	return 0;
}
static int WEMO_Setup(http_request_t* request) {
	http_setup(request, httpMimeTypeXML);
	poststr(request, g_wemo_setup_1);
	// friendly name
	poststr_escaped(request, (char*)CFG_GetDeviceName());
	poststr(request, g_wemo_setup_2);
	// uuid
	poststr(request, g_uid);
	poststr(request, g_wemo_setup_3);
	// serial number
	poststr(request, g_serial);
	poststr(request, g_wemo_setup_4);
	// presentation URL
	poststr(request, HAL_GetMyIPString());
	poststr(request, g_wemo_setup_5);
	hprintf255(request, "<binaryState>%d</binaryState>", Main_GetFirstPowerState());
	poststr(request, g_wemo_setup_6);
	poststr(request, NULL);

	stat_setupXMLVisits++;

	return 0;
}
void WEMO_Init() {
	char uid[64];
	char serial[32];
	unsigned char mac[8];
	if (g_uid)
		return;

	WiFI_GetMacAddress((char*)mac);
	snprintf(serial, sizeof(serial), "201612%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
	snprintf(uid, sizeof(uid), "Socket-1_0-%s", serial);

	g_serial = strdup(serial);
	g_uid = strdup(uid);
	if (!g_serial || !g_uid) {
		free(g_serial); free(g_uid);
		g_serial = g_uid = NULL;
		return;
	}

	HTTP_RegisterCallback("/upnp/control/basicevent1", HTTP_POST, WEMO_BasicEvent1, 0);
	HTTP_RegisterCallback("/eventservice.xml", HTTP_GET, WEMO_EventService, 0);
	HTTP_RegisterCallback("/metainfoservice.xml", HTTP_GET, WEMO_MetaInfoService, 0);
	HTTP_RegisterCallback("/setup.xml", HTTP_GET, WEMO_Setup, 0);

	//if (DRV_IsRunning("SSDP") == false) {
//	ScheduleDriverStart("SSDP", 5);
//	}
}







