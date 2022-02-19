

#include "../new_common.h"
#include "ctype.h" 
#if WINDOWS
//#include <windows.h>
#include <winsock2.h>
//#include <ws2tcpip.h>
#elif PLATFORM_XR809
#include "lwip/sockets.h"
#include <stdarg.h>
#include <image/flash.h>
#else
#include "lwip/sockets.h"
#include "str_pub.h"
#endif
#include "new_http.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../ota/ota.h"
#ifdef WINDOWS

#elif PLATFORM_XR809

#elif defined(PLATFORM_BK7231N)
// tuya-iotos-embeded-sdk-wifi-ble-bk7231n/sdk/include/tuya_hal_storage.h
#include "tuya_hal_storage.h"
#include "BkDriverFlash.h"
#else
// REALLY? A typo in Tuya SDK? Storge?
// tuya-iotos-embeded-sdk-wifi-ble-bk7231t/platforms/bk7231t/tuya_os_adapter/include/driver/tuya_hal_storge.h
#include "../logging/logging.h"
#include "tuya_hal_storge.h"
#include "BkDriverFlash.h"
#endif

/*
GET / HTTP/1.1
Host: 127.0.0.1
*/
/*
GET /test?a=5 HTTP/1.1
Host: 127.0.0.1
*/
/*
GET /test?a=Test%20with%20space HTTP/1.1
Host: 127.0.0.1
Connection: keep-alive
Cache-Control: max-age=0
*/
/*
GET /test?a=15&b=25 HTTP/1.1
Host: 127.0.0.1
Connection: keep-alive
*/

#define DEFAULT_OTA_URL "http://raspberrypi:1880/firmware"

const char httpHeader[] = "HTTP/1.1 %d OK\nContent-type: %s" ;  // HTTP header
const char httpMimeTypeHTML[] = "text/html" ;              // HTML MIME type
const char httpMimeTypeText[] = "text/plain" ;           // TEXT MIME type
const char httpMimeTypeJson[] = "application/json" ;           // TEXT MIME type
const char httpMimeTypeBinary[] = "application/octet-stream" ;   // binary/file MIME type
const char htmlHeader[] = "<!DOCTYPE html><html><body>" ;
const char htmlEnd[] = "</body></html>" ;
const char htmlReturnToMenu[] = "<a href=\"index\">Return to menu</a>";;
const char htmlReturnToCfg[] = "<a href=\"cfg\">Return to cfg</a>";;
const char *g_build_str = "Build on " __DATE__ " " __TIME__;

const char httpCorsHeaders[] = "Access-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept" ;           // TEXT MIME type

const char *methodNames[] = {
	"GET",
	"PUT",
	"POST",
	"OPTIONS"
};


#if WINDOWS
#define os_free free
#define os_malloc malloc
#endif

void misc_formatUpTimeString(int totalSeconds, char *o);
int Time_getUpTimeSeconds();

typedef struct http_callback_tag {
    char *url;
		int method;
    http_callback_fn callback;
} http_callback_t;

#define MAX_HTTP_CALLBACKS 32
static http_callback_t *callbacks[MAX_HTTP_CALLBACKS];
static int numCallbacks = 0;

int HTTP_RegisterCallback( const char *url, int method, http_callback_fn callback){
	if (!url || !callback){
		return -1;
	}
	if (numCallbacks >= MAX_HTTP_CALLBACKS){
		return -4;
	}
	callbacks[numCallbacks] = (http_callback_t*)os_malloc(sizeof(http_callback_t));
	if (!callbacks[numCallbacks]){
		return -2;
	}
	callbacks[numCallbacks]->url = (char *)os_malloc(strlen(url)+1);
	if (!callbacks[numCallbacks]->url){
		os_free(callbacks[numCallbacks]);
		return -3;
	}
	strcpy(callbacks[numCallbacks]->url, url);
	callbacks[numCallbacks]->callback = callback;
	callbacks[numCallbacks]->method = method;
	
	numCallbacks++;

	// success
	return 0;
}

int my_strnicmp(char *a, char *b, int len){
	int i; 
	for (i = 0; i < len; i++){
		char x = *a;
		char y = *b;
		if (!x || !y) return 1;
		if ((x | 0x20) != (y | 0x20)) return 1;
		a++;
		b++;
	}
	return 0;
}

bool http_startsWith(const char *base, const char *substr) {
	while(*substr != 0) {
		if(*base != *substr)
			return false;
		if(*base == 0)
			return false;
		base++;
		substr++;
	}
	return true;
}
bool http_checkUrlBase(const char *base, const char *fileName) {
	while(*base != 0 && *base != '?' && *base != ' ') {
		if(*base != *fileName)
			return false;
		if(*base == 0)
			return false;
		base++;
		fileName++;
	}
	if(*fileName != 0)
		return false;
	return true;
}

void http_setup(http_request_t *request, const char *type){
	hprintf128(request, httpHeader, request->responseCode, type);
	poststr(request,"\r\n"); // next header
	poststr(request,httpCorsHeaders);
	poststr(request,"\r\n"); // end headers with double CRLF
	poststr(request,"\r\n");
}

const char *http_checkArg(const char *p, const char *n) {
	while(1) {
		if(*n == 0 && (*p == 0 || *p == '='))
			return p;
		if(*p != *n)
			return 0;
		p++;
		n++;
	}
	return p;
}
void http_copyCarg(const char *atin, char *to, int maxSize) {
	int a, b;
	const unsigned char *at = (unsigned char *)atin;

	while(*at != 0 && *at != '&' && *at != ' ' && maxSize > 1) {
#if 0
		*to = *at;
		to++;
		at++;
		maxSize--;
#else
        if ((*at == '%') &&
            ((a = at[1]) && (b = at[2])) &&
            (isxdigit(a) && isxdigit(b))) {
                if (a >= 'a')
                        a -= 'a'-'A';
                if (a >= 'A')
                        a -= ('A' - 10);
                else
                        a -= '0';
                if (b >= 'a')
                        b -= 'a'-'A';
                if (b >= 'A')
                        b -= ('A' - 10);
                else
                        b -= '0';
                *to++ = 16*a+b;
                at+=3;
        } else if (*at == '+') {
                *to++ = ' ';
                at++;
        } else {
                *to++ = *at++;
        }
		maxSize--;
#endif
	}
	*to = 0;
}
bool http_getArg(const char *base, const char *name, char *o, int maxSize) {
	*o = '\0';
	while(*base != '?') {
		if(*base == 0)
			return 0;
		base++;
	}
	base++;
	while(*base) {
		const char *at = http_checkArg(base,name);
		if(at) {
			at++;
			http_copyCarg(at,o,maxSize);
			return 1;
		}
		while(*base != '&') {
			if(*base == 0) {
				return 0;
			}
			base++;
		}
		base++;
	}
	return 0;
}

const char *htmlIndex = "<select name=\"cars\" id=\"cars\">\
<option value=\"1\">qqqqqq</option>\
<option value=\"2\">qqq</option>\
</select>";
/*
const char *htmlPinRoles = "<option value=\"1\">Relay</option>\
<option value=\"2\">Relay_n</option>\
<option value=\"3\">Button</option>\
<option value=\"4\">Button_n</option>\
<option value=\"5\">LED</option>\
<option value=\"6\">LED_n</option>\
</select>";
*/

const char *htmlPinRoleNames[] = {
	" ",
	"Rel",
	"Rel_n",
	"Btn",
	"Btn_n",
	"LED",
	"LED_n",
	"PWM",
	"Wifi LED",
	"Wifi LED_n",
	"e",
	"e",
};

void setupAllWB2SPinsAsButtons() {
		PIN_SetPinRoleForPinIndex(6,IOR_Button);
		PIN_SetPinChannelForPinIndex(6,1);

		PIN_SetPinRoleForPinIndex(7,IOR_Button);
		PIN_SetPinChannelForPinIndex(7,1);

		PIN_SetPinRoleForPinIndex(8,IOR_Button);
		PIN_SetPinChannelForPinIndex(8,1);

		PIN_SetPinRoleForPinIndex(23,IOR_Button);
		PIN_SetPinChannelForPinIndex(23,1);

		PIN_SetPinRoleForPinIndex(24,IOR_Button);
		PIN_SetPinChannelForPinIndex(24,1);

		PIN_SetPinRoleForPinIndex(26,IOR_Button);
		PIN_SetPinChannelForPinIndex(26,1);

		PIN_SetPinRoleForPinIndex(27,IOR_Button);
		PIN_SetPinChannelForPinIndex(27,1);
}

typedef struct template_s {
	void (*setter)();
	const char *name;
} template_t;

template_t g_templates [] = {
	{ Setup_Device_Empty, "Empty"},
	// BK7231N devices
	{ Setup_Device_BK7231N_CB2S_QiachipSmartSwitch, "[BK7231N][CB2S] QiaChip Smart Switch"},
	// BK7231T devices
	{ Setup_Device_TuyaWL_SW01_16A, "WL SW01 16A"},
	{ Setup_Device_TuyaSmartLife4CH10A, "Smart Life 4CH 10A"},
	{ Setup_Device_IntelligentLife_NF101A, "Intelligent Life NF101A"},
	{ Setup_Device_TuyaLEDDimmerSingleChannel, "Tuya LED Dimmer Single Channel PWM WB3S"},
	{ Setup_Device_CalexLEDDimmerFiveChannel, "Calex RGBWW LED Dimmer Five Channel PWM BK7231S"},
	{ Setup_Device_CalexPowerStrip_900018_1v1_0UK, "Calex UK power strip 900018.1 v1.0 UK"},
	{ Setup_Device_ArlecCCTDownlight, "Arlec CCT LED Downlight ALD029CHA"},
	{ Setup_Device_NedisWIFIPO120FWT_16A, "Nedis WIFIPO120FWT SmartPlug 16A"},
	{ Setup_Device_NedisWIFIP130FWT_10A, "Nedis WIFIP130FWT SmartPlug 10A"},
	{ Setup_Device_EmaxHome_EDU8774, "Emax Home EDU8774 SmartPlug 16A"},
	{ Setup_Device_TuyaSmartPFW02G, "Tuya Smart PFW02-G"}
};

int g_total_templates = sizeof(g_templates)/sizeof(g_templates[0]);

#if PLATFORM_XR809
const char *g_header = "<h1><a href=\"https://github.com/openshwprojects/OpenXR809/\">OpenXR809</a></h1><h3><a href=\"https://www.elektroda.com/rtvforum/viewtopic.php?p=19841301#19841301\">[Read more]</a><a href=\"https://paypal.me/openshwprojects\">[Support project]</a></h3>";

#elif PLATFORM_BK7231N

const char *g_header = "<h1><a href=\"https://github.com/openshwprojects/OpenBK7231T/\">OpenBK7231N</a></h1><h3><a href=\"https://www.elektroda.com/rtvforum/viewtopic.php?p=19841301#19841301\">[Read more]</a><a href=\"https://paypal.me/openshwprojects\">[Support project]</a></h3>";

#elif PLATFORM_BK7231T

const char *g_header = "<h1><a href=\"https://github.com/openshwprojects/OpenBK7231T/\">OpenBK7231T</a></h1><h3><a href=\"https://www.elektroda.com/rtvforum/viewtopic.php?p=19841301#19841301\">[Read more]</a><a href=\"https://paypal.me/openshwprojects\">[Support project]</a></h3>";

#elif WINDOWS

const char *g_header = "<h1><a href=\"https://github.com/openshwprojects/OpenBK7231T/\">OpenBK7231 [Win test]</a></h1><h3><a href=\"https://www.elektroda.com/rtvforum/viewtopic.php?p=19841301#19841301\">[Read more]</a><a href=\"https://paypal.me/openshwprojects\">[Support project]</a></h3>";

#else

const char *g_header = "<h1>error</h1>";
#error "Platform not supported"
Platform not supported
#endif


void HTTP_AddBuildFooter(http_request_t *request) {
	char upTimeStr[128];
	unsigned char mac[32];
	poststr(request,"<br>");
	poststr(request,g_build_str);
	poststr(request,"<br> Online for ");
	misc_formatUpTimeString(Time_getUpTimeSeconds(), upTimeStr);
	poststr(request,upTimeStr);

	WiFI_GetMacAddress((char *)mac);

	sprintf(upTimeStr,"<br> Device MAC: %02X%02X%02X%02X%02X%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	poststr(request,upTimeStr);

}


// add some more output safely, sending if necessary.
// call with str == NULL to force send. - can be binary.
// supply length
int postany(http_request_t *request, const char *str, int len){
	int currentlen;
	int addlen = len;
	if (NULL == str){
		send(request->fd, request->reply, request->replylen, 0);
		request->reply[0] = 0;
		request->replylen = 0;
		return 0;
	}

	currentlen = request->replylen;
	if (currentlen + addlen >= request->replymaxlen){
		send(request->fd, request->reply, request->replylen, 0);
		request->reply[0] = 0;
		request->replylen = 0;
		currentlen = 0;
	}
	if (addlen > request->replymaxlen){
		printf("won't fit");
	} else {
		memcpy( request->reply+request->replylen, str, addlen );
		request->replylen += addlen;
	}
	return (currentlen + addlen);
}


// add some more output safely, sending if necessary.
// call with str == NULL to force send.
int poststr(http_request_t *request, const char *str){
	if (str == NULL){
		return postany(request, NULL, 0);
	}
	return postany(request, str, strlen(str));
}

void misc_formatUpTimeString(int totalSeconds, char *o) {
	int rem_days;
	int rem_hours;
	int rem_minutes;
	int rem_seconds;

	rem_days = totalSeconds / (24*60*60);
	totalSeconds = totalSeconds % (24*60*60);
	rem_hours = totalSeconds / (60*60);
	totalSeconds = totalSeconds % (60*60);
	rem_minutes = totalSeconds / (60);
	rem_seconds = totalSeconds % 60;

	*o = 0;
	if(rem_days > 0)
	{
		sprintf(o,"%i days, %i hours, %i minutes and %i seconds ",rem_days,rem_hours,rem_minutes,rem_seconds);
	}
	else if(rem_hours > 0)
	{
		sprintf(o,"%i hours, %i minutes and %i seconds ",rem_hours,rem_minutes,rem_seconds);
	}
	else if(rem_minutes > 0)
	{
		sprintf(o,"%i minutes and %i seconds ",rem_minutes,rem_seconds);
	}
	else
	{
		sprintf(o,"just %i seconds ",rem_seconds);
	}
}

int hprintf128(http_request_t *request, const char *fmt, ...){
  va_list argList;
  //BaseType_t taken;
	char tmp[128];
	va_start(argList, fmt);
	vsprintf(tmp, fmt, argList);
	va_end(argList);
	return postany(request, tmp, strlen(tmp));
}

uint8_t hexdigit( char hex )
{
    return (hex <= '9') ? hex - '0' : 
                          toupper(hex) - 'A' + 10 ;
}

uint8_t hexbyte( const char* hex )
{
    return (hexdigit(*hex) << 4) | hexdigit(*(hex+1)) ;
}

int HTTP_ProcessPacket(http_request_t *request) {
	int i, j, k;
	char tmpA[128];
	char tmpB[64];
	char tmpC[64];
	char *p;
	char *headers;
	char *protocol;
	//int bChanged = 0;
	char *urlStr = "";

	char *recvbuf = request->received;
	for ( i = 0; i < sizeof(methodNames)/sizeof(*methodNames); i++){
		if (http_startsWith(recvbuf, methodNames[i])){
			urlStr = recvbuf + strlen(methodNames[i]) + 2; // skip method name plus space, plus slash
			request->method = i;
			break;
		}
	}
	if (request->method == -1){
		printf("unsupported method %7s", recvbuf);
		return 0;
	}

	if (request->method == HTTP_GET) {
		printf("HTTP request\n");
	} else {
		printf("Other request\n");
	}

	// if OPTIONS, return now - for CORS
	if (request->method == HTTP_OPTIONS) {
		http_setup(request, httpMimeTypeHTML);
		i = strlen(request->reply);
		return i;
	}

	// chop URL at space
	p = strchr(urlStr, ' ');
	if (*p) {
		*p = '\0';
		p++; // past space
	}
	else {
		printf("invalid request\n");
		return 0;
	}

	request->url = urlStr;

	// protocol is next, termed by \r\n
	protocol = p;
	p = strchr(protocol, '\r');
	if (*p) {
		*p = '\0';
		p++; // past \r
		p++; // past \n
	} else {
		printf("invalid request\n");
		return 0;
	}
	// i.e. not received
	request->contentLength = -1;
	headers = p;
	do {
		p = strchr(headers, '\r');
		if (p != headers){
			if (p){
				if (request->numheaders < 16){
					request->headers[request->numheaders] = headers;
					request->numheaders++;
				}
				// pick out contentLength
				if (!my_strnicmp(headers, "Content-Length:", 15)){
					request->contentLength = atoi(headers + 15);
				}

				*p = 0;
				p++; // past \r
				p++; // past \n
				headers = p;
			} else {
				break;
			}
		}
		if (*p == '\r'){
			// end of headers
			*p = 0;
			p++;
			p++;
			break;
		}
	} while(1);

	request->bodystart = p;
	request->bodylen = request->receivedLen - (p - request->received); 

	// we will make this more general
	http_getArg(urlStr,"a",tmpA,sizeof(tmpA));
	http_getArg(urlStr,"b",tmpB,sizeof(tmpB));
	http_getArg(urlStr,"c",tmpC,sizeof(tmpC));
	if (*tmpA){
		request->querynames[request->numqueryitems] = "a";
		request->queryvalues[request->numqueryitems] = tmpA;
		request->numqueryitems++;
	}
	if (*tmpB){
		request->querynames[request->numqueryitems] = "b";
		request->queryvalues[request->numqueryitems] = tmpB;
		request->numqueryitems++;
	}
	if (*tmpC){
		request->querynames[request->numqueryitems] = "c";
		request->queryvalues[request->numqueryitems] = tmpC;
		request->numqueryitems++;
	}


	// look for a callback with this URL and method, or HTTP_ANY
	for (i = 0; i < numCallbacks; i++){
		char *url = callbacks[i]->url;
		if (http_startsWith(urlStr, &url[1])){
			int method = callbacks[i]->method;
			if(method == HTTP_ANY || method == request->method){
				return callbacks[i]->callback(request);
			}
		}
	}

	if(http_checkUrlBase(urlStr,"about")) {
		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);
		poststr(request,"About us page.");
		poststr(request,htmlReturnToMenu);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"cfg_mqtt")) {
		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);
		poststr(request,"<h2> Use this to connect to your MQTT</h2>");
		poststr(request,"<form action=\"/cfg_mqtt_set\">\
			  <label for=\"host\">Host:</label><br>\
			  <input type=\"text\" id=\"host\" name=\"host\" value=\"");
			  
		poststr(request,CFG_GetMQTTHost());
		poststr(request,"\"><br>\
			  <label for=\"port\">Port:</label><br>\
			  <input type=\"text\" id=\"port\" name=\"port\" value=\"");
		i = CFG_GetMQTTPort();
		sprintf(tmpA,"%i",i);
		poststr(request,tmpA);
		poststr(request,"\"><br><br>\
			  <label for=\"port\">Client:</label><br>\
			  <input type=\"text\" id=\"client\" name=\"client\" value=\"");
			  
		poststr(request,CFG_GetMQTTBrokerName());
		poststr(request,"\"><br>\
			  <label for=\"user\">User:</label><br>\
			  <input type=\"text\" id=\"user\" name=\"user\" value=\"");
		poststr(request,CFG_GetMQTTUserName());
		poststr(request,"\"><br>\
			  <label for=\"port\">Password:</label><br>\
			  <input type=\"text\" id=\"password\" name=\"password\" value=\"");
		poststr(request,CFG_GetMQTTPass());
		poststr(request,"\"><br>\
			  <input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure? Please check MQTT data twice?')\">\
			</form> ");
		poststr(request,htmlReturnToCfg);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"cfg_mqtt_set")) {
		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);
	
		if(http_getArg(urlStr,"host",tmpA,sizeof(tmpA))) {
			CFG_SetMQTTHost(tmpA);
		}
		if(http_getArg(urlStr,"port",tmpA,sizeof(tmpA))) {
			CFG_SetMQTTPort(atoi(tmpA));
		}
		if(http_getArg(urlStr,"user",tmpA,sizeof(tmpA))) {
			CFG_SetMQTTUserName(tmpA);
		}
		if(http_getArg(urlStr,"password",tmpA,sizeof(tmpA))) {
			CFG_SetMQTTPass(tmpA);
		}
		if(http_getArg(urlStr,"client",tmpA,sizeof(tmpA))) {
			CFG_SetMQTTBrokerName(tmpA);
		}
		if(CFG_SaveMQTT()) {
			poststr(request,"MQTT mode set!");
		} else {
			poststr(request,"Error saving MQTT settings to flash!");
		}
		

		poststr(request,"Please wait for module to connect... if there is problem, restart it...");
		
		poststr(request,"<br>");
		poststr(request,"<a href=\"cfg_mqtt\">Return to MQTT settings</a>");
		poststr(request,"<br>");
		poststr(request,htmlReturnToCfg);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"cfg_webapp")) {
		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);
		poststr(request,"<h2> Use this to set the URL of the Webapp</h2>");
		poststr(request,"<form action=\"/cfg_webapp_set\">\
			  <label for=\"url\">Url:</label><br>\
			  <input type=\"text\" id=\"url\" name=\"url\" value=\"");			  
		poststr(request,CFG_GetWebappRoot());
		poststr(request,"\"><br>\
			  <input type=\"submit\" value=\"Submit\">\
			</form> ");
		poststr(request,htmlReturnToCfg);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"config_dump_table")) {
		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);
#if WINDOWS
		poststr(request,"Not implemented <br>");
#elif PLATFORM_XR809
		poststr(request,"Not implemented <br>");
#else
		poststr(request,"Dumped to log <br>");
			config_dump_table();
#endif
		poststr(request,htmlReturnToCfg);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"cfg_webapp_set")) {
		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);
	
		if(http_getArg(urlStr,"url",tmpA,sizeof(tmpA))) {
			if(CFG_SetWebappRoot(tmpA)) {
				hprintf128(request,"Webapp url set to %s", tmpA);
			} else {
				hprintf128(request,"Webapp url change error - failed to save to flash.");
			}
		} else {
			poststr(request,"Webapp url not set because you didn't specify the argument.");
		}
		
		poststr(request,"<br>");
		poststr(request,htmlReturnToCfg);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"cfg_wifi_set")) {
		printf("HTTP_ProcessPacket: generating cfg_wifi_set \r\n");

		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);
		if(http_getArg(recvbuf,"open",tmpA,sizeof(tmpA))) {
			CFG_SetWiFiSSID("");
			CFG_SetWiFiPass("");
			poststr(request,"WiFi mode set: open access point.");
		} else {
			if(http_getArg(urlStr,"ssid",tmpA,sizeof(tmpA))) {
				CFG_SetWiFiSSID(tmpA);
			}
			if(http_getArg(urlStr,"pass",tmpA,sizeof(tmpA))) {
				CFG_SetWiFiPass(tmpA);
			}
			poststr(request,"WiFi mode set: connect to WLAN.");
		}
		printf("HTTP_ProcessPacket: calling CFG_SaveWiFi \r\n");
		CFG_SaveWiFi();
		printf("HTTP_ProcessPacket: done CFG_SaveWiFi \r\n");

		poststr(request,"Please wait for module to reset...");
		
		poststr(request,"<br>");
		poststr(request,"<a href=\"cfg_wifi\">Return to WiFi settings</a>");
		poststr(request,"<br>");
		poststr(request,htmlReturnToCfg);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"cfg_loglevel_set")) {
		printf("HTTP_ProcessPacket: generating cfg_loglevel_set \r\n");

		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);
		if(http_getArg(recvbuf,"loglevel",tmpA,sizeof(tmpA))) {
#if PLATFORM_BK7231T
			loglevel = atoi(tmpA);
#endif
			poststr(request,"LOG level changed.");
		} 
		poststr(request,"<form action=\"/cfg_loglevel_set\">\
			  <label for=\"loglevel\">loglevel:</label><br>\
			  <input type=\"text\" id=\"loglevel\" name=\"loglevel\" value=\"");
		tmpA[0] = 0;
#if PLATFORM_BK7231T
		sprintf(tmpA,"%i",loglevel);
#endif
		poststr(request,tmpA);
			  
		poststr(request,"\"><br><br>\
			  <input type=\"submit\" value=\"Submit\" >\
			</form> ");
		
		poststr(request,"<br>");
		poststr(request,"<a href=\"cfg\">Return to config settings</a>");
		poststr(request,"<br>");
		poststr(request,htmlReturnToCfg);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"cfg_wifi")) {
		// for a test, show password as well...
		const char *cur_ssid, *cur_pass;


		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);
		/*bChanged = 0;
		if(http_getArg(recvbuf,"ssid",tmpA,sizeof(tmpA))) {
			CFG_SetWiFiSSID(tmpA);
			poststr(request,"<h4> WiFi SSID set!</h4>");
			bChanged = 1;
		}
		if(http_getArg(recvbuf,"pass",tmpA,sizeof(tmpA))) {
			CFG_SetWiFiPass(tmpA);
			poststr(request,"<h4> WiFi Password set!</h4>");
			bChanged = 1;
		}
		if(bChanged) {
			poststr(request,"<h4> Device will reconnect after restarting</h4>");
		}*/
		poststr(request,"<h2> Check networks reachable by module</h2> This will lag few seconds.<br>");
		if(http_getArg(urlStr,"scan",tmpA,sizeof(tmpA))) {
#ifdef WINDOWS

			poststr(request,"Not available on Windows<br>");
#elif PLATFORM_XR809
			poststr(request,"TODO XR809<br>");

#else
			AP_IF_S *ar;
			uint32_t num;
			
			bk_printf("Scan begin...\r\n");
			tuya_hal_wifi_all_ap_scan(&ar,&num);
			bk_printf("Scan returned %i networks\r\n",num);
			for(i = 0; i < num; i++) {
				sprintf(tmpA,"[%i/%i] SSID: %s, Channel: %i, Signal %i<br>",i,(int)num,ar[i].ssid, ar[i].channel, ar[i].rssi);
				poststr(request,tmpA);
			}
			tuya_hal_wifi_release_ap(ar);
#endif
		}
		poststr(request,"<form action=\"/cfg_wifi\">\
			  <input type=\"hidden\" id=\"scan\" name=\"scan\" value=\"1\">\
			  <input type=\"submit\" value=\"Scan local networks!\">\
			</form> ");
		poststr(request,"<h2> Use this to disconnect from your WiFi</h2>");
		poststr(request,"<form action=\"/cfg_wifi_set\">\
			  <input type=\"hidden\" id=\"open\" name=\"open\" value=\"1\">\
			  <input type=\"submit\" value=\"Convert to open access wifi\" onclick=\"return confirm('Are you sure to convert module to open access wifi?')\">\
			</form> ");
		poststr(request,"<h2> Use this to connect to your WiFi</h2>");
		poststr(request,"<form action=\"/cfg_wifi_set\">\
			  <label for=\"ssid\">SSID:</label><br>\
			  <input type=\"text\" id=\"ssid\" name=\"ssid\" value=\"");
		cur_ssid = CFG_GetWiFiSSID();
		poststr(request,cur_ssid);
			  
			 poststr(request, "\"><br>\
			  <label for=\"pass\">Pass:</label><br>\
			  <input type=\"text\" id=\"pass\" name=\"pass\" value=\"");
		cur_pass = CFG_GetWiFiPass();
		poststr(request,cur_pass);
			  
		poststr(request,"\"><br><br>\
			  <input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure? Please check SSID and pass twice?')\">\
			</form> ");
		poststr(request,htmlReturnToCfg);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"cfg_mac")) {
		// must be unsigned, else print below prints negatives as e.g. FFFFFFFe
		unsigned char mac[6];

		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);

		if(http_getArg(recvbuf,"mac",tmpA,sizeof(tmpA))) {
			for( i = 0; i < 6; i++ )
			{
				mac[i] = hexbyte( &tmpA[i * 2] ) ;
			}
			//sscanf(tmpA,"%02X%02X%02X%02X%02X%02X",&mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]);
			if(WiFI_SetMacAddress((char*)mac)) {
				poststr(request,"<h4> New MAC set!</h4>");
			} else {
				poststr(request,"<h4> MAC change error?</h4>");
			}
		}

		WiFI_GetMacAddress((char *)mac);

		sprintf(tmpA,"%02X%02X%02X%02X%02X%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

		poststr(request,"<h2> Here you can change MAC address.</h2>");
		poststr(request,"<form action=\"/cfg_mac\">\
			  <label for=\"mac\">MAC:</label><br>\
			  <input type=\"text\" id=\"mac\" name=\"mac\" value=\"");
		poststr(request,tmpA);
		poststr(request,"\"><br><br>\
			  <input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure? Please check MAC hex string twice?')\">\
			</form> ");
		poststr(request,htmlReturnToCfg);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"flash_read_tool")) {
		int len = 16;
		int ofs = 1970176;
		int res;
		int rem;
		int now;
		int nowOfs;
		int hex;
		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);
		poststr(request,"<h4>Flash Read Tool</h4>");
		if(	http_getArg(urlStr,"hex",tmpA,sizeof(tmpA))){
			hex = atoi(tmpA);
		} else {
			hex = 0;
		}

		if(	http_getArg(urlStr,"offset",tmpA,sizeof(tmpA)) &&
				http_getArg(urlStr,"len",tmpB,sizeof(tmpB))) {
			u8 buffer[128];
			len = atoi(tmpB);
			ofs = atoi(tmpA);
			sprintf(tmpA,"Memory at %i with len %i reads: ",ofs,len);
			poststr(request,tmpA);
			poststr(request,"<br>");

			///res = tuya_hal_flash_read (ofs, buffer,len);
			//sprintf(tmpA,"Result %i",res);
		//	strcat(outbuf,tmpA);
		///	strcat(outbuf,"<br>");

			nowOfs = ofs;
			rem = len;
			while(1) {
				if(rem > sizeof(buffer)) {
					now = sizeof(buffer);
				} else {
					now = rem;
				}
#if PLATFORM_XR809
				//uint32_t flash_read(uint32_t flash, uint32_t addr,void *buf, uint32_t size)
				#define FLASH_INDEX_XR809 0
				res = flash_read(FLASH_INDEX_XR809, nowOfs, buffer, now);
#else
				res = tuya_hal_flash_read (nowOfs, buffer,now);
#endif
				for(i = 0; i < now; i++) {
					u8 val = buffer[i];
					if(!hex && isprint(val)) {
						sprintf(tmpA,"'%c' ",val);
					} else {
						sprintf(tmpA,"%02X ",val);
					}
					poststr(request,tmpA);
				}
				rem -= now;
				nowOfs += now;
				if(rem <= 0) {
					break;
				}
			}

			poststr(request,"<br>");
		}
		poststr(request,"<form action=\"/flash_read_tool\">");
		
		poststr(request,"<input type=\"checkbox\" id=\"hex\" name=\"hex\" value=\"1\"");
		if(hex){
			poststr(request," checked");
		}
		poststr(request,"><label for=\"hex\">Show all hex?</label><br>");
		poststr(request,"<label for=\"offset\">offset:</label><br>\
			  <input type=\"number\" id=\"offset\" name=\"offset\"");
		sprintf(tmpA," value=\"%i\"><br>",ofs);
		poststr(request,tmpA);
		poststr(request,"<label for=\"lenght\">lenght:</label><br>\
			  <input type=\"number\" id=\"len\" name=\"len\" ");
		sprintf(tmpA,"value=\"%i\">",len);
		poststr(request,tmpA);
		poststr(request,"<br><br>\
			  <input type=\"submit\" value=\"Submit\">\
			</form> ");

		poststr(request,htmlReturnToCfg);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);

	} else if(http_checkUrlBase(urlStr,"cfg_quick")) {
		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);
		poststr(request,"<h4>Quick Config</h4>");
		
		if(http_getArg(urlStr,"dev",tmpA,sizeof(tmpA))) {
			j = atoi(tmpA);
			sprintf(tmpA,"<h3>Set dev %i!</h3>",j);
			poststr(request,tmpA);
			
			g_templates[j].setter();
		}
		poststr(request,"<form action=\"cfg_quick\">");		
		sprintf(tmpA, "<select name=\"dev\">");
		poststr(request,tmpA);
		for(j = 0; j < g_total_templates; j++) {
			sprintf(tmpA, "<option value=\"%i\">%s</option>",j,g_templates[j].name);
			poststr(request,tmpA);
		}
		poststr(request,"</select>");
		poststr(request,"<input type=\"submit\" value=\"Set\"/></form>");
		
		poststr(request,htmlReturnToCfg);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);


	} else if(http_checkUrlBase(urlStr,"cfg_ha")) {
		int relayFlags = 0;
		int pwmFlags = 0;
		int relayCount = 0;
		int pwmCount = 0;
		const char *baseName;

		baseName = CFG_GetShortDeviceName();

		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);
		poststr(request,"<h4>Home Assistant Cfg</h4>");
		poststr(request,"<h4>Paste this to configuration yaml</h4>");
		poststr(request,"<h5>Make sure that you have \"switch:\" keyword only once! Home Assistant doesn't like dup keywords.</h5>");
		poststr(request,"<h5>You can also use \"switch MyDeviceName:\" to avoid keyword duplication!</h5>");
		
		poststr(request,"<textarea rows=\"40\" cols=\"50\">");

		for(i = 0; i < GPIO_MAX; i++) {
			int role = PIN_GetPinRoleForPinIndex(i);
			int ch = PIN_GetPinChannelForPinIndex(i);
			if(role == IOR_Relay || role == IOR_Relay_n || role == IOR_LED || role == IOR_LED_n) {
				BIT_SET(relayFlags,ch);
				relayCount++;
			}
			if(role == IOR_PWM) {
				BIT_SET(pwmFlags,ch);
				pwmCount++;
			}
		}
		if(relayCount > 0) {
			poststr(request,"switch:\n");
			for(i = 0; i < CHANNEL_MAX; i++) {
				if(BIT_CHECK(relayFlags,i)) {
					poststr(request,"  - platform: mqtt\n");
					sprintf(tmpA,"    name: \"%s %i\"\n",baseName,i);
					poststr(request,tmpA);
					sprintf(tmpA,"    state_topic: \"%s/%i/get\"\n",baseName,i);
					poststr(request,tmpA);
					sprintf(tmpA,"    command_topic: \"%s/%i/set\"\n",baseName,i);
					poststr(request,tmpA);
					poststr(request,"    qos: 1\n");
					poststr(request,"    payload_on: 0\n");
					poststr(request,"    payload_off: 1\n");
					poststr(request,"    retain: true\n");
					sprintf(tmpA,"    availability_topic: \"%s/connected\"\n",baseName);
					poststr(request,tmpA);
				}
			}
		}
		if(pwmCount > 0) {
			poststr(request,"light:\n");
			for(i = 0; i < CHANNEL_MAX; i++) {
				if(BIT_CHECK(pwmFlags,i)) {
					poststr(request,"  - platform: mqtt\n");
					sprintf(tmpA,"    name: \"%s %i\"\n",baseName,i);
					poststr(request,tmpA);
					sprintf(tmpA,"    state_topic: \"%s/%i/get\"\n",baseName,i);
					poststr(request,tmpA);
					sprintf(tmpA,"    command_topic: \"%s/%i/set\"\n",baseName,i);
					poststr(request,tmpA);
					sprintf(tmpA,"    brightness_command_topic: \"%s/%i/set\"\n",baseName,i);
					poststr(request,tmpA);
					poststr(request,"    on_command_type: \"brightness\"\n");
					poststr(request,"    brightness_scale: 99\n");
					poststr(request,"    qos: 1\n");
					poststr(request,"    payload_on: 99\n");
					poststr(request,"    payload_off: 0\n");
					poststr(request,"    retain: true\n");
					poststr(request,"    optimistic: true\n");
					sprintf(tmpA,"    availability_topic: \"%s/connected\"\n",baseName);
					poststr(request,tmpA);
				}
			}
		}

		poststr(request,"</textarea>");

		poststr(request,htmlReturnToCfg);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);

		
	} else if(http_checkUrlBase(urlStr,"cfg")) {
		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);
		poststr(request,"<form action=\"cfg_pins\"><input type=\"submit\" value=\"Configure Module\"/></form>");
		poststr(request,"<form action=\"cfg_quick\"><input type=\"submit\" value=\"Quick Config\"/></form>");
		poststr(request,"<form action=\"cfg_wifi\"><input type=\"submit\" value=\"Configure WiFi\"/></form>");
		poststr(request,"<form action=\"cfg_mqtt\"><input type=\"submit\" value=\"Configure MQTT\"/></form>");
		poststr(request,"<form action=\"cfg_mac\"><input type=\"submit\" value=\"Change MAC\"/></form>");
		poststr(request,"<form action=\"cfg_webapp\"><input type=\"submit\" value=\"Configure Webapp\"/></form>");
		poststr(request,"<form action=\"cfg_ha\"><input type=\"submit\" value=\"Generate Home Assistant cfg\"/></form>");
		poststr(request,"<form action=\"ota\"><input type=\"submit\" value=\"OTA (update software by WiFi)\"/></form>");
		poststr(request,"<form action=\"cmd_single\"><input type=\"submit\" value=\"Execute custom command\"/></form>");
		poststr(request,"<form action=\"flash_read_tool\"><input type=\"submit\" value=\"Flash Read Tool\"/></form>");

#if PLATFORM_BK7231T | PLATFORM_BK7231N
		k = config_get_tableOffsets(BK_PARTITION_NET_PARAM,&i,&j);
		sprintf(tmpA,"BK_PARTITION_NET_PARAM: bOk %i, at %i, len %i<br>",k,i,j);
		poststr(request,tmpA);
		k = config_get_tableOffsets(BK_PARTITION_RF_FIRMWARE,&i,&j);
		sprintf(tmpA,"BK_PARTITION_RF_FIRMWARE: bOk %i, at %i, len %i<br>",k,i,j);
		poststr(request,tmpA);
		k = config_get_tableOffsets(BK_PARTITION_OTA,&i,&j);
		sprintf(tmpA,"BK_PARTITION_OTA: bOk %i, at %i, len %i<br>",k,i,j);
		poststr(request,tmpA);
#endif


		poststr(request,"<a href=\"/app\" target=\"_blank\">Launch Web Application</a><br/>");

		poststr(request,htmlReturnToMenu);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);
	//} else if(http_checkUrlBase(urlStr,"setWB2SInputs")) {
	//	http_setup(outbuf, httpMimeTypeHTML);
	//	poststr(request,htmlHeader);

	//	setupAllWB2SPinsAsButtons();

	//	http_setup(outbuf, httpMimeTypeHTML);
	//	poststr(request,"Set all inputs for dbg .");
	//	poststr(request,htmlReturnToMenu);
	//	HTTP_AddBuildFooter(outbuf);
	//	poststr(request,htmlEnd);
	//} else if(http_checkUrlBase(urlStr,"setAllInputs")) {
	//	http_setup(outbuf, httpMimeTypeHTML);
	//	poststr(request,htmlHeader);
	//	// it breaks UART pins as well, omg!
	//	for(i = 0; i < GPIO_MAX; i++) {
	//		PIN_SetPinRoleForPinIndex(i,IOR_Button);
	//		PIN_SetPinChannelForPinIndex(i,1);
	//	}
	//	http_setup(outbuf, httpMimeTypeHTML);
	//	poststr(request,"Set all inputs for dbg .");
	//	poststr(request,htmlReturnToMenu);
	//	HTTP_AddBuildFooter(outbuf);
	//	poststr(request,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"cfg_pins")) {
		int iChanged = 0;
		int iChangedRequested = 0;

		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);
		for(i = 0; i < GPIO_MAX; i++) {
			sprintf(tmpA, "%i",i);
			if(http_getArg(recvbuf,tmpA,tmpB,sizeof(tmpB))) {
				int role;
				int pr;

				iChangedRequested++;

				role = atoi(tmpB);

				pr = PIN_GetPinRoleForPinIndex(i);
				if(pr != role) {
					PIN_SetPinRoleForPinIndex(i,role);
					iChanged++;
				}
			}
			sprintf(tmpA, "r%i",i);
			if(http_getArg(urlStr,tmpA,tmpB,sizeof(tmpB))) {
				int rel;
				int prevRel;

				iChangedRequested++;

				rel = atoi(tmpB);

				prevRel = PIN_GetPinChannelForPinIndex(i);
				if(prevRel != rel) {
					PIN_SetPinChannelForPinIndex(i,rel);
					iChanged++;
				}
			}
		}
		if(iChangedRequested>0) {
			PIN_SaveToFlash();
			sprintf(tmpA, "Pins update - %i reqs, %i changed!<br><br>",iChangedRequested,iChanged);
			poststr(request,tmpA);
		}
	//	strcat(outbuf,"<button type=\"button\">Click Me!</button>");
		poststr(request,"<form action=\"cfg_pins\">");
		for( i = 0; i < GPIO_MAX; i++) {
			int si, ch;

			si = PIN_GetPinRoleForPinIndex(i);
			ch = PIN_GetPinChannelForPinIndex(i);

#if PLATFORM_XR809
			poststr(request,PIN_GetPinNameAlias(i));
			poststr(request," ");
#else
			sprintf(tmpA, "P%i ",i);
			poststr(request,tmpA);
#endif
			sprintf(tmpA, "<select name=\"%i\">",i);
			poststr(request,tmpA);
			for(j = 0; j < IOR_Total_Options; j++) {
				if(j == si) {
					sprintf(tmpA, "<option value=\"%i\" selected>%s</option>",j,htmlPinRoleNames[j]);
				} else {
					sprintf(tmpA, "<option value=\"%i\">%s</option>",j,htmlPinRoleNames[j]);
				}
				poststr(request,tmpA);
			}
			poststr(request, "</select>");
			if(ch == 0) {
				tmpB[0] = 0;
			} else {
				sprintf(tmpB,"%i",ch);
			}
			sprintf(tmpA, "<input name=\"r%i\" type=\"text\" value=\"%s\"/>",i,tmpB);
			poststr(request,tmpA);
			poststr(request,"<br>");
		}
		poststr(request,"<input type=\"submit\" value=\"Save\"/></form>");

		poststr(request,htmlReturnToCfg);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"index")) {
		int relayFlags;
		int pwmFlags;

		relayFlags = 0;
		pwmFlags = 0;

		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,"<style>.r { background-color: red; } .g { background-color: green; }</style>");
		poststr(request,g_header);
		if(http_getArg(urlStr,"tgl",tmpA,sizeof(tmpA))) {
			j = atoi(tmpA);
			sprintf(tmpA,"<h3>Toggled %i!</h3>",j);
			poststr(request,tmpA);
			CHANNEL_Toggle(j);
		}
		if(http_getArg(urlStr,"on",tmpA,sizeof(tmpA))) {
			j = atoi(tmpA);
			sprintf(tmpA,"<h3>Enabled %i!</h3>",j);
			poststr(request,tmpA);
			CHANNEL_Set(j,255,1);
		}
		if(http_getArg(urlStr,"off",tmpA,sizeof(tmpA))) {
			j = atoi(tmpA);
			sprintf(tmpA,"<h3>Disabled %i!</h3>",j);
			poststr(request,tmpA);
			CHANNEL_Set(j,0,1);
		}
		if(http_getArg(urlStr,"pwm",tmpA,sizeof(tmpA))) {
			int newPWMValue = atoi(tmpA);
			http_getArg(urlStr,"pwmIndex",tmpA,sizeof(tmpA));
			j = atoi(tmpA);
			sprintf(tmpA,"<h3>Changed pwm %i to %i!</h3>",j,newPWMValue);
			poststr(request,tmpA);
			CHANNEL_Set(j,newPWMValue,1);
		}

		for(i = 0; i < GPIO_MAX; i++) {
			int role = PIN_GetPinRoleForPinIndex(i);
			int ch = PIN_GetPinChannelForPinIndex(i);
			if(role == IOR_Relay || role == IOR_Relay_n || role == IOR_LED || role == IOR_LED_n) {
				BIT_SET(relayFlags,ch);
			}
			if(role == IOR_PWM) {
				BIT_SET(pwmFlags,ch);
			}
		}
		for(i = 0; i < CHANNEL_MAX; i++) {
			if(BIT_CHECK(relayFlags,i)) {
				const char *c;
				if(CHANNEL_Check(i)) {
					c = "r";
				} else {
					c = "g";
				}
				poststr(request,"<form action=\"index\">");
				sprintf(tmpA,"<input type=\"hidden\" name=\"tgl\" value=\"%i\">",i);
				poststr(request,tmpA);
				sprintf(tmpA,"<input class=\"%s\" type=\"submit\" value=\"Toggle %i\"/></form>",c,i);
				poststr(request,tmpA);
			}
			if(BIT_CHECK(pwmFlags,i)) {
				int pwmValue;

				pwmValue = CHANNEL_Get(i);
				sprintf(tmpA,"<form action=\"index\" id=\"form%i\">",i);
				poststr(request,tmpA);
				sprintf(tmpA,"<input type=\"range\" min=\"0\" max=\"100\" name=\"pwm\" id=\"slider%i\" value=\"%i\">",i,pwmValue);
				poststr(request,tmpA);
				sprintf(tmpA,"<input type=\"hidden\" name=\"pwmIndex\" value=\"%i\">",i);
				poststr(request,tmpA);
				sprintf(tmpA,"<input  type=\"submit\" style=\"display:none;\" value=\"Toggle %i\"/></form>",i);
				poststr(request,tmpA);


				poststr(request,"<script>");
				sprintf(tmpA,"var slider = document.getElementById(\"slider%i\");\n",i);
				poststr(request,tmpA);
				poststr(request,"slider.onmouseup = function () {\n");
				sprintf(tmpA," document.getElementById(\"form%i\").submit();\n",i);
				poststr(request,tmpA);
				poststr(request,"}\n");
				poststr(request,"</script>");
			}
		}
	//	strcat(outbuf,"<button type=\"button\">Click Me!</button>");

		
		if(http_getArg(urlStr,"restart",tmpA,sizeof(tmpA))) {
			poststr(request,"<h5> Module will restart soon</h5>");
#if WINDOWS

#elif PLATFORM_XR809

#else
			RESET_ScheduleModuleReset(3);
#endif
		}

		poststr(request,"<form action=\"cfg\"><input type=\"submit\" value=\"Config\"/></form>");

		poststr(request,"<form action=\"/index\">\
			  <input type=\"hidden\" id=\"restart\" name=\"restart\" value=\"1\">\
			  <input type=\"submit\" value=\"Restart\" onclick=\"return confirm('Are you sure to restart module?')\">\
			</form> ");

		poststr(request,"<form action=\"about\"><input type=\"submit\" value=\"About\"/></form>");


		poststr(request,htmlReturnToMenu);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);
    } else if(http_checkUrlBase(urlStr,"ota_exec")) {
        http_setup(request, httpMimeTypeHTML);
        poststr(request,htmlHeader);
		if(http_getArg(urlStr,"host",tmpA,sizeof(tmpA))) {
			sprintf(tmpB,"<h3>OTA requested for %s!</h3>",tmpA);
			poststr(request,tmpB);
#if WINDOWS

#elif PLATFORM_XR809

#else
        otarequest(tmpA);
#endif
		}
        poststr(request,htmlReturnToMenu);
		HTTP_AddBuildFooter(request);
        poststr(request,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"ota")) {
		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,"<form action=\"/ota_exec\">\
			  <label for=\"host\">URL for new bin file:</label><br>\
			  <input type=\"text\" id=\"host\" name=\"host\" value=\"");
		poststr(request,"\"><br>\
			  <input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure?')\">\
			</form> ");
        poststr(request,htmlReturnToMenu);
		HTTP_AddBuildFooter(request);
        poststr(request,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"")) {
		// Redirect / to /index page
        poststr(request,"HTTP/1.1 302 OK\nLocation: /index\nConnection: close\n\n");
	} else {
		http_setup(request, httpMimeTypeHTML);
		poststr(request,htmlHeader);
		poststr(request,g_header);
		poststr(request,"Not found.<br/>");
		poststr(request,htmlReturnToMenu);
		HTTP_AddBuildFooter(request);
		poststr(request,htmlEnd);
	}


	// force send remaining
	poststr(request, NULL);
	// nothing more to send
	return 0;
}
