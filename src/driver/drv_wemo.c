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
":80/</presentationURL>"
"<binaryState>0</binaryState>";
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
"DATE: Fri, 15 Apr 2016 04:56:29 GMT\r\n"
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
	else {
		useType = "upnp:rootdevice";
	}

	addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_HTTP, "WEMO - sends reply %s",useType);

	if (buffer_out == 0) {
		outBufferLen = strlen(g_wemo_msearch) + 256;
		buffer_out = (char*)malloc(outBufferLen);
	}
	snprintf(buffer_out, outBufferLen, g_wemo_msearch, HAL_GetMyIPString(), useType, g_uid, useType);

	addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_HTTP, "WEMO - Sending %s", buffer_out);
	DRV_SSDP_SendReply(addr, buffer_out);
}

void WEMO_AppendInformationToHTTPIndexPage(http_request_t* request) {
	hprintf255(request, "<h4>WEMO: searches %i, setup %i, events %i, mService %i, event %i </h4>",
		stat_searchesReceived, stat_setupXMLVisits, stat_eventsReceived, stat_metaServiceXMLVisits, stat_eventServiceXMLVisits);

}


bool Main_GetFirstPowerState() {
	int i;
	if (LED_IsLEDRunning()) {
		return LED_GetEnableAll();
	}
	else {
		// relays driver
		for (i = 0; i < CHANNEL_MAX; i++) {
			if (h_isChannelRelay(i) || CHANNEL_GetType(i) == ChType_Toggle) {
				return CHANNEL_Get(i);
			}
		}
	}
	return 0;
}
static int WEMO_BasicEvent1(http_request_t* request) {
	const char* cmd = request->bodystart;


	addLogAdv(LOG_INFO, LOG_FEATURE_HTTP, "Wemo post event %s", cmd);
	// Sample event data taken by my user
	/*
	<?xml version="1.0" encoding="utf-8"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/"s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:GetBinaryState xmlns:u="urn:Belkin:service:basicevent:1"><BinaryState>1</BinaryState></u:GetBinaryState></s:Body></s:Envelope>
	*/
	// Set and Get are the same so we can hack just one letter
	char letter;
	// is this a Set request?
	if (strstr(cmd, "SetBinaryState")) {
		// set
		letter = 'S';
		if (strstr(cmd, "State>1</Binary")) {
			CMD_ExecuteCommand("POWER ON", 0);
		}
		if (strstr(cmd, "State>0</Binary")) {
			CMD_ExecuteCommand("POWER OFF", 0);
		}
	}
	else {
		// get
		letter = 'G';
	}
	int bMainPower = Main_GetFirstPowerState();

	// We must send a SetBinaryState or GetBinaryState response depending on what 
	// was sent to us	
	http_setup(request, httpMimeTypeXML);
	poststr(request, g_wemo_response_1);
	// letter is twice because we have two SetBinaryStateResponse tokens (opening and closing tag)
	hprintf255(request, g_wemo_response_2_fmt, letter, bMainPower, letter);
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
	poststr(request, CFG_GetDeviceName());
	poststr(request, g_wemo_setup_2);
	// uuid
	poststr(request, g_uid);
	poststr(request, g_wemo_setup_3);
	// IP str
	poststr(request, HAL_GetMyIPString());
	poststr(request, g_wemo_setup_4);
	// serial str
	poststr(request, g_serial);
	poststr(request, g_wemo_setup_5);
	poststr(request, g_wemo_setup_6);
	poststr(request, NULL);

	stat_setupXMLVisits++;

	return 0;
}
void WEMO_Init() {
	char uid[64];
	char serial[32];
	unsigned char mac[8];

	WiFI_GetMacAddress((char*)mac);
	snprintf(serial, sizeof(serial), "201612%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
	snprintf(uid, sizeof(uid), "Socket-1_0-%s", serial);

	g_serial = strdup(serial);
	g_uid = strdup(uid);

	HTTP_RegisterCallback("/upnp/control/basicevent1", HTTP_POST, WEMO_BasicEvent1, 0);
	HTTP_RegisterCallback("/eventservice.xml", HTTP_GET, WEMO_EventService, 0);
	HTTP_RegisterCallback("/metainfoservice.xml", HTTP_GET, WEMO_MetaInfoService, 0);
	HTTP_RegisterCallback("/setup.xml", HTTP_GET, WEMO_Setup, 0);

	//if (DRV_IsRunning("SSDP") == false) {
//	ScheduleDriverStart("SSDP", 5);
//	}
}







