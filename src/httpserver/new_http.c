

#include "../new_common.h"
#include "../logging/logging.h"
#include "ctype.h" 
#include "new_http.h"
#include "http_fns.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../ota/ota.h"
#include "../hal/hal_wifi.h"


// define the feature ADDLOGF_XXX will use
#define LOG_FEATURE LOG_FEATURE_HTTP

const char httpHeader[] = "HTTP/1.1 %d OK\nContent-type: %s" ;  // HTTP header
const char httpMimeTypeHTML[] = "text/html" ;              // HTML MIME type
const char httpMimeTypeText[] = "text/plain" ;           // TEXT MIME type
const char httpMimeTypeJson[] = "application/json" ;           // TEXT MIME type
const char httpMimeTypeBinary[] = "application/octet-stream" ;   // binary/file MIME type
const char htmlHeader[] = "<!DOCTYPE html><html><body>" ;
const char htmlEnd[] = "</body></html>" ;
const char htmlReturnToMenu[] = "<a href=\"index\">Return to menu</a>";
const char htmlReturnToCfg[] = "<a href=\"cfg\">Return to cfg</a>";

// make sure that USER_SW_VER is set on all platforms
#ifndef USER_SW_VER
    #ifdef WINDOWS
        #define USER_SW_VER "Win_Test"
    #elif PLATFORM_XR809
        #define USER_SW_VER "XR809_Test"
    #elif defined(PLATFORM_BK7231N)
        #define USER_SW_VER "BK7231N_Test"
    #elif defined(PLATFORM_BK7231T)
        #define USER_SW_VER "BK7231T_Test"
    #else
        #define USER_SW_VER "unknown"
    #endif
#endif
const char *g_build_str = "Build on " __DATE__ " " __TIME__ " version " USER_SW_VER; // Show GIT version at Build line;

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

int http_getArg(const char *base, const char *name, char *o, int maxSize) {
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
	"Btn_Tgl_All",
	"Btn_Tgl_All_n",
	"DigitalInput",
	"DigitalInput_n",
	"ToggleChannelOnToggle",
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


#if PLATFORM_XR809
const char *g_header = "<h1><a href=\"https://github.com/openshwprojects/OpenXR809/\">OpenXR809</a></h1><h3><a href=\"https://www.elektroda.com/rtvforum/viewtopic.php?p=19841301#19841301\">[Read more]</a><a href=\"https://paypal.me/openshwprojects\">[Support project]</a></h3>";

#elif PLATFORM_BK7231N

const char *g_header = "<h1><a href=\"https://github.com/openshwprojects/OpenBK7231T/\">OpenBK7231N</a></h1><h3><a href=\"https://www.elektroda.com/rtvforum/viewtopic.php?p=19841301#19841301\">[Read more]</a><a href=\"https://paypal.me/openshwprojects\">[Support project]</a></h3>";

#elif PLATFORM_BK7231T

const char *g_header = "<h1><a href=\"https://github.com/openshwprojects/OpenBK7231T/\">OpenBK7231T</a></h1><h3><a href=\"https://www.elektroda.com/rtvforum/viewtopic.php?p=19841301#19841301\">[Read more]</a><a href=\"https://paypal.me/openshwprojects\">[Support project]</a></h3>";

#elif PLATFORM_BL602

const char *g_header = "<h1><a href=\"https://github.com/openshwprojects/OpenBK7231T/\">OpenBL602</a></h1><h3><a href=\"https://www.elektroda.com/rtvforum/viewtopic.php?p=19841301#19841301\">[Read more]</a><a href=\"https://paypal.me/openshwprojects\">[Support project]</a></h3>";

#elif WINDOWS

const char *g_header = "<h1><a href=\"https://github.com/openshwprojects/OpenBK7231T/\">OpenBK7231 [Win test]</a></h1><h3><a href=\"https://www.elektroda.com/rtvforum/viewtopic.php?p=19841301#19841301\">[Read more]</a><a href=\"https://paypal.me/openshwprojects\">[Support project]</a></h3>";

#else

const char *g_header = "<h1>error</h1>";
#error "Platform not supported"
//Platform not supported
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
		ADDLOGF_ERROR("won't fit");
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


int HTTP_ProcessPacket(http_request_t *request) {
	int i;
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
		ADDLOGF_ERROR("unsupported method %7s", recvbuf);
		return 0;
	}

	if (request->method == HTTP_GET) {
		//ADDLOG_INFO(LOG_FEATURE_HTTP, "HTTP request\n");
	} else {
		//ADDLOG_INFO(LOG_FEATURE_HTTP, "Other request\n");
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
		ADDLOGF_ERROR("invalid request\n");
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
		ADDLOGF_ERROR("invalid request\n");
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

	if(http_checkUrlBase(urlStr,"")) return http_fn_empty_url(request);

	if(http_checkUrlBase(urlStr,"index")) return http_fn_index(request);

	if(http_checkUrlBase(urlStr,"about")) return http_fn_about(request);

	if(http_checkUrlBase(urlStr,"cfg_mqtt")) return http_fn_cfg_mqtt(request);
	if(http_checkUrlBase(urlStr,"cfg_mqtt_set")) return http_fn_cfg_mqtt_set(request);

	if(http_checkUrlBase(urlStr,"cfg_webapp")) return http_fn_cfg_webapp(request);
	if(http_checkUrlBase(urlStr,"cfg_webapp_set")) return http_fn_cfg_webapp_set(request); 

	if(http_checkUrlBase(urlStr,"cfg_wifi")) return http_fn_cfg_wifi(request);
	if(http_checkUrlBase(urlStr,"cfg_wifi_set")) return http_fn_cfg_wifi_set(request);

	if(http_checkUrlBase(urlStr,"cfg_loglevel_set")) return http_fn_cfg_loglevel_set(request);
	if(http_checkUrlBase(urlStr,"cfg_mac")) return http_fn_cfg_mac(request);

	if(http_checkUrlBase(urlStr,"flash_read_tool")) return http_fn_flash_read_tool(request);
	if(http_checkUrlBase(urlStr,"uart_tool")) return http_fn_uart_tool(request);
	if(http_checkUrlBase(urlStr,"cmd_tool")) return http_fn_cmd_tool(request);
	if(http_checkUrlBase(urlStr,"config_dump_table")) return http_fn_config_dump_table(request);
	if(http_checkUrlBase(urlStr,"startup_command")) return http_fn_startup_command(request);

	if(http_checkUrlBase(urlStr,"cfg_quick")) return http_fn_cfg_quick(request);
	if(http_checkUrlBase(urlStr,"cfg_ha")) return http_fn_cfg_ha(request);
	if(http_checkUrlBase(urlStr,"cfg")) return http_fn_cfg(request);

	if(http_checkUrlBase(urlStr,"cfg_pins")) return http_fn_cfg_pins(request);

	if(http_checkUrlBase(urlStr,"ota")) return http_fn_ota(request);
	if(http_checkUrlBase(urlStr,"ota_exec")) return http_fn_ota_exec(request);
	if(http_checkUrlBase(urlStr,"cm")) return http_fn_cm(request);

	return http_fn_other(request);
}
