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


void HUE_AppendInformationToHTTPIndexPage(http_request_t* request) {
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
static int HUE_NotImplemented(http_request_t* request) {

	http_setup(request, httpMimeTypeJson);
	poststr(request, "{}");
	poststr(request, NULL);

	return 0;
}
static int HUE_Authentication(http_request_t* request) {

	http_setup(request, httpMimeTypeJson);
	hprintf255(request, "[{\"success\":{\"username\":\"%s\"}}]",g_userID);
	poststr(request, NULL);

	return 0;
}
static int HUE_Config_Internal(http_request_t* request) {

	poststr(request, "{\"name\":\"Philips hue\",\"mac\":\"");
	// TODO: mac
	poststr(request, "\",\"dhcp\":true,\"ipaddress\":\"");
	// TODO: ip
	poststr(request, "\",\"netmask\":\"");
	// TODO: mask
	poststr(request, "\",\"gateway\":\"");
	// TODO: gw
	poststr(request, "\",\"proxyaddress\":\"none\",\"proxyport\":0,\"bridgeid\":\"");
	// TODO: bridgeid
	poststr(request, "\",\"UTC\":\"{dt\",\"whitelist\":{\"");
	// TODO: id
	poststr(request, "\":{\"last use date\":\"");
	// TODO: date
	poststr(request, "\",\"create date\":\"");
	// TODO: date
	poststr(request, "\",\"name\":\"Remote\"}},\"swversion\":\"01041302\",\"apiversion\":\"1.17.0\",\"swupdate\":{\"updatestate\":0,\"url\":\"\",\"text\":\"\",\"notify\": false},\"linkbutton\":false,\"portalservices\":false}");
	poststr(request, NULL);

	return 0;
}




static int HUE_GlobalConfig(http_request_t* request) {

	http_setup(request, httpMimeTypeJson);
	poststr(request, "{\"lights\":{");
	// TODO: lights
	poststr(request, "},\"groups\":{},\"schedules\":{},\"config\":");
	HUE_Config_Internal(request);
	poststr(request, "}");
	poststr(request, NULL);

	return 0;
}


// http://192.168.0.213/api/username/lights/1/state
// http://192.168.0.213/description.xml
int HUE_APICall(http_request_t* request) {
	if (g_uid == 0) {
		// not running
		return 0;
	}
	// skip "api/"
	//const char *api = request->url + 4;
	//int urlLen = strlen(request->url);
	// TODO

	return 0;
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

