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
static char *g_serial = 0;
static char *g_uid = 0;
static int outBufferLen = 0;
static char *buffer_out = 0;
static int stat_searchesReceived = 0;
static int stat_setupXMLVisits = 0;
static int stat_eventsReceived = 0;

void DRV_WEMO_Send_Advert_To(struct sockaddr_in *addr) {
	char message[128];
	const char *useType;

	if (g_uid == 0) {
		// not running
		return;
	}

	stat_searchesReceived++;

	useType = "urn:Belkin:device:**";

	if (buffer_out == 0) {
		outBufferLen = strlen(g_wemo_msearch) + 256;
		buffer_out = (char*)malloc(outBufferLen);
	}
	snprintf(buffer_out, outBufferLen, g_wemo_msearch, HAL_GetMyIPString(), useType, g_uid, useType);

	DRV_SSDP_SendReply(addr, message);
}

void WEMO_AppendInformationToHTTPIndexPage(http_request_t* request) {
	hprintf255(request, "<h4>WEMO: MSEARCH received %i, setup.xml visits %i, events %i </h4>",
		stat_searchesReceived, stat_setupXMLVisits, stat_eventsReceived);

}
static int WEMO_BasicEvent1(http_request_t* request) {
	const char* cmd = request->bodystart;


	addLogAdv(LOG_INFO, LOG_FEATURE_HTTP, "Wemo post event %s", cmd);

	stat_eventsReceived++;

	return 0;
}
static int WEMO_EventService(http_request_t* request) {

	return 0;
}
static int WEMO_MetaInfoService(http_request_t* request) {

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

	HTTP_RegisterCallback("/upnp/control/basicevent1", HTTP_POST, WEMO_BasicEvent1);
	HTTP_RegisterCallback("/eventservice.xml", HTTP_GET, WEMO_EventService);
	HTTP_RegisterCallback("/metainfoservice.xml", HTTP_GET, WEMO_MetaInfoService);
	HTTP_RegisterCallback("/setup.xml", HTTP_GET, WEMO_Setup);

	//if (DRV_IsRunning("SSDP") == false) {
//	ScheduleDriverStart("SSDP", 5);
//	}
}







