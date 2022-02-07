

#include "../new_common.h"
#include "ctype.h" 
#ifndef WINDOWS
#include "str_pub.h"
#endif
#include "new_http.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../ota/ota.h"
#ifdef WINDOWS

#elif defined(PLATFORM_BK7231N)
// tuya-iotos-embeded-sdk-wifi-ble-bk7231n/sdk/include/tuya_hal_storage.h
#include "tuya_hal_storage.h"
#else
// REALLY? A typo in Tuya SDK? Storge?
// tuya-iotos-embeded-sdk-wifi-ble-bk7231t/platforms/bk7231t/tuya_os_adapter/include/driver/tuya_hal_storge.h

#include "tuya_hal_storge.h"
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

const char httpHeader[] = "HTTP/1.1 200 OK\nContent-type: " ;  // HTTP header
const char httpMimeTypeHTML[] = "text/html\n\n" ;              // HTML MIME type
const char httpMimeTypeText[] = "text/plain\n\n" ;           // TEXT MIME type
const char htmlHeader[] = "<!DOCTYPE html><html><body>" ;
const char htmlEnd[] = "</body></html>" ;
const char htmlReturnToMenu[] = "<a href=\"index\">Return to menu</a>";;
const char htmlReturnToCfg[] = "<a href=\"cfg\">Return to cfg</a>";;
const char *g_build_str = "Build on " __DATE__ " " __TIME__;

#if WINDOWS
#define os_free free
#define os_malloc malloc
#endif

typedef struct http_callback_tag {
    char *url;
    http_callback_fn callback;
} http_callback_t;

#define MAX_HTTP_CALLBACKS 32
static http_callback_t *callbacks[MAX_HTTP_CALLBACKS];
static int numCallbacks = 0;

int HTTP_RegisterCallback( const char *url, http_callback_fn callback){
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
	numCallbacks++;

	// success
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

void http_setup(char *o, const char *type){
	strcpy(o,httpHeader);
	strcat(o,type);
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
	{ Setup_Device_TuyaWL_SW01_16A, "WL SW01 16A"},
	{ Setup_Device_TuyaSmartLife4CH10A, "Smart Life 4CH 10A"},
	{ Setup_Device_IntelligentLife_NF101A, "Intelligent Life NF101A"},
	{ Setup_Device_TuyaLEDDimmerSingleChannel, "Tuya LED Dimmer Single Channel PWM WB3S"},
	{ Setup_Device_CalexLEDDimmerFiveChannel, "Calex RGBWW LED Dimmer Five Channel PWM BK7231S"},
	{ Setup_Device_CalexPowerStrip_900018_1v1_0UK, "Calex UK power strip 900018.1 v1.0 UK"},
	{ Setup_Device_ArlecCCTDownlight, "Arlec CCT LED Downlight ALD029CHA"}
};

int g_total_templates = sizeof(g_templates)/sizeof(g_templates[0]);

const char *g_header = "<h1><a href=\"https://github.com/openshwprojects/OpenBK7231T/\">OpenBK7231</a></h1><h3><a href=\"https://www.elektroda.com/rtvforum/viewtopic.php?p=19841301#19841301\">[Read more]</a><a href=\"https://paypal.me/openshwprojects\">[Support project]</a></h3>";


void HTTP_AddBuildFooter(char *outbuf, int outBufSize) {
	strcat_safe(outbuf,"<br>",outBufSize);
	strcat_safe(outbuf,g_build_str,outBufSize);
}
int HTTP_ProcessPacket(const char *recvbuf, char *outbuf, int outBufSize, http_send_fn sendpart, int socket) {
	int i, j;
	char tmpA[128];
	char tmpB[64];
	char tmpC[64];
	//int bChanged = 0;
	const char *urlStr;

	*outbuf = '\0';

	urlStr = recvbuf + 5;
	if(http_startsWith(recvbuf,"GET")) {
		printf("HTTP request\n");
	} else {
		printf("Other request\n");
	}
	http_getArg(urlStr,"a",tmpA,sizeof(tmpA));
	http_getArg(urlStr,"b",tmpB,sizeof(tmpB));
	http_getArg(urlStr,"c",tmpC,sizeof(tmpC));


	for (i = 0; i < numCallbacks; i++){
		char *url = callbacks[i]->url;
		if (http_checkUrlBase(urlStr, &url[1])){
			return callbacks[i]->callback(recvbuf, outbuf, outBufSize);
		}
	}

	if(http_checkUrlBase(urlStr,"about")) {
		http_setup(outbuf, httpMimeTypeHTML);
		strcat_safe(outbuf,htmlHeader,outBufSize);
		strcat_safe(outbuf,g_header,outBufSize);
		strcat_safe(outbuf,"About us page.",outBufSize);
		strcat_safe(outbuf,htmlReturnToMenu,outBufSize);
		HTTP_AddBuildFooter(outbuf,outBufSize);
		strcat_safe(outbuf,htmlEnd,outBufSize);
	} else if(http_checkUrlBase(urlStr,"cfg_mqtt")) {
		http_setup(outbuf, httpMimeTypeHTML);
		strcat_safe(outbuf,htmlHeader,outBufSize);
		strcat_safe(outbuf,g_header,outBufSize);
		strcat_safe(outbuf,"<h2> Use this to connect to your MQTT</h2>",outBufSize);
		strcat_safe(outbuf,"<form action=\"/cfg_mqtt_set\">\
			  <label for=\"host\">Host:</label><br>\
			  <input type=\"text\" id=\"host\" name=\"host\" value=\"",outBufSize);
			  
		strcat_safe(outbuf,CFG_GetMQTTHost(),outBufSize);
		strcat_safe(outbuf,"\"><br>\
			  <label for=\"port\">Port:</label><br>\
			  <input type=\"text\" id=\"port\" name=\"port\" value=\"",outBufSize);
		i = CFG_GetMQTTPort();
		sprintf(tmpA,"%i",i);
		strcat_safe(outbuf,tmpA,outBufSize);
		strcat_safe(outbuf,"\"><br><br>\
			  <label for=\"port\">Client:</label><br>\
			  <input type=\"text\" id=\"client\" name=\"client\" value=\"",outBufSize);
			  
		strcat_safe(outbuf,CFG_GetMQTTBrokerName(),outBufSize);
		strcat_safe(outbuf,"\"><br>\
			  <label for=\"user\">User:</label><br>\
			  <input type=\"text\" id=\"user\" name=\"user\" value=\"",outBufSize);
		strcat_safe(outbuf,CFG_GetMQTTUserName(),outBufSize);
		strcat_safe(outbuf,"\"><br>\
			  <label for=\"port\">Password:</label><br>\
			  <input type=\"text\" id=\"password\" name=\"password\" value=\"",outBufSize);
		strcat_safe(outbuf,CFG_GetMQTTPass(),outBufSize);
		strcat_safe(outbuf,"\"><br>\
			  <input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure? Please check MQTT data twice?')\">\
			</form> ",outBufSize);
		strcat_safe(outbuf,htmlReturnToCfg,outBufSize);
		HTTP_AddBuildFooter(outbuf,outBufSize);
		strcat_safe(outbuf,htmlEnd,outBufSize);
	} else if(http_checkUrlBase(urlStr,"cfg_mqtt_set")) {
		http_setup(outbuf, httpMimeTypeHTML);
		strcat_safe(outbuf,htmlHeader,outBufSize);
		strcat_safe(outbuf,g_header,outBufSize);
	
		if(http_getArg(recvbuf,"host",tmpA,sizeof(tmpA))) {
			CFG_SetMQTTHost(tmpA);
		}
		if(http_getArg(recvbuf,"port",tmpA,sizeof(tmpA))) {
			CFG_SetMQTTPort(atoi(tmpA));
		}
		if(http_getArg(recvbuf,"user",tmpA,sizeof(tmpA))) {
			CFG_SetMQTTUserName(tmpA);
		}
		if(http_getArg(recvbuf,"password",tmpA,sizeof(tmpA))) {
			CFG_SetMQTTPass(tmpA);
		}
		if(http_getArg(recvbuf,"client",tmpA,sizeof(tmpA))) {
			CFG_SetMQTTBrokerName(tmpA);
		}
		strcat_safe(outbuf,"MQTT mode set!",outBufSize);
		
		CFG_SaveMQTT();

		strcat_safe(outbuf,"Please wait for module to connect... if there is problem, restart it...",outBufSize);
		
		strcat_safe(outbuf,"<br>",outBufSize);
		strcat_safe(outbuf,"<a href=\"cfg_mqtt\">Return to MQTT settings</a>",outBufSize);
		strcat_safe(outbuf,"<br>",outBufSize);
		strcat_safe(outbuf,htmlReturnToCfg,outBufSize);
		HTTP_AddBuildFooter(outbuf,outBufSize);
		strcat_safe(outbuf,htmlEnd,outBufSize);
	} else if(http_checkUrlBase(urlStr,"cfg_wifi_set")) {
		http_setup(outbuf, httpMimeTypeHTML);
		strcat_safe(outbuf,htmlHeader,outBufSize);
		strcat_safe(outbuf,g_header,outBufSize);
		if(http_getArg(recvbuf,"open",tmpA,sizeof(tmpA))) {
			CFG_SetWiFiSSID("");
			CFG_SetWiFiPass("");
			strcat_safe(outbuf,"WiFi mode set: open access point.",outBufSize);
		} else {
			if(http_getArg(recvbuf,"ssid",tmpA,sizeof(tmpA))) {
				CFG_SetWiFiSSID(tmpA);
			}
			if(http_getArg(recvbuf,"pass",tmpA,sizeof(tmpA))) {
				CFG_SetWiFiPass(tmpA);
			}
			strcat_safe(outbuf,"WiFi mode set: connect to WLAN.",outBufSize);
		}
		CFG_SaveWiFi();

		strcat_safe(outbuf,"Please wait for module to reset...",outBufSize);
		
		strcat_safe(outbuf,"<br>",outBufSize);
		strcat_safe(outbuf,"<a href=\"cfg_wifi\">Return to WiFi settings</a>",outBufSize);
		strcat_safe(outbuf,"<br>",outBufSize);
		strcat_safe(outbuf,htmlReturnToCfg,outBufSize);
		HTTP_AddBuildFooter(outbuf,outBufSize);
		strcat_safe(outbuf,htmlEnd,outBufSize);
	} else if(http_checkUrlBase(urlStr,"cfg_wifi")) {
		// for a test, show password as well...
		const char *cur_ssid, *cur_pass;


		http_setup(outbuf, httpMimeTypeHTML);
		strcat_safe(outbuf,htmlHeader,outBufSize);
		strcat_safe(outbuf,g_header,outBufSize);
		/*bChanged = 0;
		if(http_getArg(recvbuf,"ssid",tmpA,sizeof(tmpA))) {
			CFG_SetWiFiSSID(tmpA);
			strcat_safe(outbuf,"<h4> WiFi SSID set!</h4>",outBufSize);
			bChanged = 1;
		}
		if(http_getArg(recvbuf,"pass",tmpA,sizeof(tmpA))) {
			CFG_SetWiFiPass(tmpA);
			strcat_safe(outbuf,"<h4> WiFi Password set!</h4>",outBufSize);
			bChanged = 1;
		}
		if(bChanged) {
			strcat_safe(outbuf,"<h4> Device will reconnect after restarting</h4>",outBufSize);
		}*/
		strcat_safe(outbuf,"<h2> Use this to disconnect from your WiFi</h2>",outBufSize);
		strcat_safe(outbuf,"<form action=\"/cfg_wifi_set\">\
			  <input type=\"hidden\" id=\"open\" name=\"open\" value=\"1\">\
			  <input type=\"submit\" value=\"Convert to open access wifi\" onclick=\"return confirm('Are you sure to convert module to open access wifi?')\">\
			</form> ",outBufSize);
		strcat_safe(outbuf,"<h2> Use this to connect to your WiFi</h2>",outBufSize);
		strcat_safe(outbuf,"<form action=\"/cfg_wifi_set\">\
			  <label for=\"ssid\">SSID:</label><br>\
			  <input type=\"text\" id=\"ssid\" name=\"ssid\" value=\"",outBufSize);
		cur_ssid = CFG_GetWiFiSSID();
		strcat_safe(outbuf,cur_ssid,outBufSize);
			  
			 strcat_safe(outbuf, "\"><br>\
			  <label for=\"pass\">Pass:</label><br>\
			  <input type=\"text\" id=\"pass\" name=\"pass\" value=\"",outBufSize);
		cur_pass = CFG_GetWiFiPass();
		strcat_safe(outbuf,cur_pass,outBufSize);
			  
		strcat_safe(outbuf,"\"><br><br>\
			  <input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure? Please check SSID and pass twice?')\">\
			</form> ",outBufSize);
		strcat_safe(outbuf,htmlReturnToCfg,outBufSize);
		HTTP_AddBuildFooter(outbuf,outBufSize);
		strcat_safe(outbuf,htmlEnd,outBufSize);
	} else if(http_checkUrlBase(urlStr,"flash_read_tool")) {
		int len = 16;
		int ofs = 1970176;
		int res;
		int rem;
		int now;
		int nowOfs;
		http_setup(outbuf, httpMimeTypeHTML);
		strcat_safe(outbuf,htmlHeader,outBufSize);
		strcat_safe(outbuf,g_header,outBufSize);
		strcat_safe(outbuf,"<h4>Flash Read Tool</h4>",outBufSize);

		if(http_getArg(recvbuf,"offset",tmpA,sizeof(tmpA))&&http_getArg(recvbuf,"len",tmpB,sizeof(tmpB))) {
			u8 buffer[128];
			len = atoi(tmpB);
			ofs = atoi(tmpA);
			sprintf(tmpA,"Memory at %i with len %i reads: ",ofs,len);
			strcat_safe(outbuf,tmpA,outBufSize);
			strcat_safe(outbuf,"<br>",outBufSize);

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
				res = tuya_hal_flash_read (nowOfs, buffer,now);
				for(i = 0; i < now; i++) {
					sprintf(tmpA,"%02X ",buffer[i]);
					strcat_safe(outbuf,tmpA,outBufSize);
				}
				rem -= now;
				nowOfs += now;
				if(rem <= 0) {
					break;
				}
			}

			strcat_safe(outbuf,"<br>",outBufSize);
		}
		strcat_safe(outbuf,"<form action=\"/flash_read_tool\">\
			  <label for=\"offset\">offset:</label><br>\
			  <input type=\"number\" id=\"offset\" name=\"offset\"",outBufSize);
		sprintf(tmpA," value=\"%i\"><br>",ofs);
		strcat_safe(outbuf,tmpA,outBufSize);
		strcat_safe(outbuf,"<label for=\"lenght\">lenght:</label><br>\
			  <input type=\"number\" id=\"len\" name=\"len\" ",outBufSize);
		sprintf(tmpA,"value=\"%i\">",len);
		strcat_safe(outbuf,tmpA,outBufSize);
		strcat_safe(outbuf,"<br><br>\
			  <input type=\"submit\" value=\"Submit\">\
			</form> ",outBufSize);

		strcat_safe(outbuf,htmlReturnToCfg,outBufSize);
		HTTP_AddBuildFooter(outbuf,outBufSize);
		strcat_safe(outbuf,htmlEnd,outBufSize);

	} else if(http_checkUrlBase(urlStr,"cfg_quick")) {
		http_setup(outbuf, httpMimeTypeHTML);
		strcat_safe(outbuf,htmlHeader,outBufSize);
		strcat_safe(outbuf,g_header,outBufSize);
		strcat_safe(outbuf,"<h4>Quick Config</h4>",outBufSize);
		
		if(http_getArg(urlStr,"dev",tmpA,sizeof(tmpA))) {
			j = atoi(tmpA);
			sprintf(tmpA,"<h3>Set dev %i!</h3>",j);
			strcat(outbuf,tmpA);
			
			g_templates[j].setter();
		}
		strcat_safe(outbuf,"<form action=\"cfg_quick\">",outBufSize);		
		sprintf(tmpA, "<select name=\"dev\">");
		strcat(outbuf,tmpA);
		for(j = 0; j < g_total_templates; j++) {
			sprintf(tmpA, "<option value=\"%i\">%s</option>",j,g_templates[j].name);
			strcat(outbuf,tmpA);
		}
		strcat(outbuf, "</select>");
		strcat_safe(outbuf,"<input type=\"submit\" value=\"Set\"/></form>",outBufSize);
		
		strcat_safe(outbuf,htmlReturnToCfg,outBufSize);
		HTTP_AddBuildFooter(outbuf,outBufSize);
		strcat_safe(outbuf,htmlEnd,outBufSize);


	} else if(http_checkUrlBase(urlStr,"cfg_ha")) {
		int relayFlags = 0;
		int pwmFlags = 0;
		int relayCount = 0;
		int pwmCount = 0;
		const char *baseName;

		baseName = CFG_GetShortDeviceName();

		http_setup(outbuf, httpMimeTypeHTML);
		strcat_safe(outbuf,htmlHeader,outBufSize);
		strcat_safe(outbuf,g_header,outBufSize);
		strcat_safe(outbuf,"<h4>Home Assistant Cfg</h4>",outBufSize);
		strcat_safe(outbuf,"<h4>Paste this to configuration yaml</h4>",outBufSize);
		
		strcat_safe(outbuf,"<textarea rows=\"40\" cols=\"50\">",outBufSize);

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
			strcat_safe(outbuf,"switch:\n",outBufSize);
			for(i = 0; i < CHANNEL_MAX; i++) {
				if(BIT_CHECK(relayFlags,i)) {
					strcat_safe(outbuf,"  - platform: mqtt\n",outBufSize);
					sprintf(tmpA,"    name: \"%s %i\"\n",baseName,i);
					strcat_safe(outbuf,tmpA,outBufSize);
					sprintf(tmpA,"    state_topic: \"%s/%i/get\"\n",baseName,i);
					strcat_safe(outbuf,tmpA,outBufSize);
					sprintf(tmpA,"    command_topic: \"%s/%i/set\"\n",baseName,i);
					strcat_safe(outbuf,tmpA,outBufSize);
					strcat_safe(outbuf,"    qos: 1\n",outBufSize);
					strcat_safe(outbuf,"    payload_on: 0\n",outBufSize);
					strcat_safe(outbuf,"    payload_off: 1\n",outBufSize);
					strcat_safe(outbuf,"    retain: true\n",outBufSize);
				}
			}
		}
		if(pwmCount > 0) {
			strcat_safe(outbuf,"light:\n",outBufSize);
			for(i = 0; i < CHANNEL_MAX; i++) {
				if(BIT_CHECK(pwmFlags,i)) {
					strcat_safe(outbuf,"  - platform: mqtt\n",outBufSize);
					sprintf(tmpA,"    name: \"%s %i\"\n",baseName,i);
					strcat_safe(outbuf,tmpA,outBufSize);
					sprintf(tmpA,"    state_topic: \"%s/%i/get\"\n",baseName,i);
					strcat_safe(outbuf,tmpA,outBufSize);
					sprintf(tmpA,"    command_topic: \"%s/%i/set\"\n",baseName,i);
					strcat_safe(outbuf,tmpA,outBufSize);
					sprintf(tmpA,"    brightness_command_topic: \"%s/%i/set\"\n",baseName,i);
					strcat_safe(outbuf,tmpA,outBufSize);
					strcat_safe(outbuf,"    on_command_type: \"brightness\"\n",outBufSize);
					strcat_safe(outbuf,"    brightness_scale: 99\n",outBufSize);
					strcat_safe(outbuf,"    qos: 1\n",outBufSize);
					strcat_safe(outbuf,"    payload_on: 99\n",outBufSize);
					strcat_safe(outbuf,"    payload_off: 0\n",outBufSize);
					strcat_safe(outbuf,"    retain: true\n",outBufSize);
					strcat_safe(outbuf,"    optimistic: true\n",outBufSize);
				}
			}
		}

		strcat_safe(outbuf,"</textarea>",outBufSize);

		strcat_safe(outbuf,htmlReturnToCfg,outBufSize);
		HTTP_AddBuildFooter(outbuf,outBufSize);
		strcat_safe(outbuf,htmlEnd,outBufSize);

		
	} else if(http_checkUrlBase(urlStr,"cfg")) {
		http_setup(outbuf, httpMimeTypeHTML);
		strcat_safe(outbuf,htmlHeader,outBufSize);
		strcat_safe(outbuf,g_header,outBufSize);
		strcat_safe(outbuf,"<form action=\"cfg_pins\"><input type=\"submit\" value=\"Configure Module\"/></form>",outBufSize);
		strcat_safe(outbuf,"<form action=\"cfg_quick\"><input type=\"submit\" value=\"Quick Config\"/></form>",outBufSize);
		strcat_safe(outbuf,"<form action=\"cfg_wifi\"><input type=\"submit\" value=\"Configure WiFi\"/></form>",outBufSize);
		strcat_safe(outbuf,"<form action=\"cfg_mqtt\"><input type=\"submit\" value=\"Configure MQTT\"/></form>",outBufSize);
		strcat_safe(outbuf,"<form action=\"cfg_ha\"><input type=\"submit\" value=\"Generate Home Assistant cfg\"/></form>",outBufSize);
		strcat_safe(outbuf,"<form action=\"ota\"><input type=\"submit\" value=\"OTA (update software by WiFi)\"/></form>",outBufSize);
		strcat_safe(outbuf,"<form action=\"cmd_single\"><input type=\"submit\" value=\"Execute custom command\"/></form>",outBufSize);
		strcat_safe(outbuf,"<form action=\"flash_read_tool\"><input type=\"submit\" value=\"Flash Read Tool\"/></form>",outBufSize);


		strcat_safe(outbuf,htmlReturnToMenu,outBufSize);
		HTTP_AddBuildFooter(outbuf,outBufSize);
		strcat_safe(outbuf,htmlEnd,outBufSize);
	//} else if(http_checkUrlBase(urlStr,"setWB2SInputs")) {
	//	http_setup(outbuf, httpMimeTypeHTML);
	//	strcat_safe(outbuf,htmlHeader,outBufSize);

	//	setupAllWB2SPinsAsButtons();

	//	http_setup(outbuf, httpMimeTypeHTML);
	//	strcat_safe(outbuf,"Set all inputs for dbg .",outBufSize);
	//	strcat_safe(outbuf,htmlReturnToMenu,outBufSize);
	//	HTTP_AddBuildFooter(outbuf,outBufSize);
	//	strcat_safe(outbuf,htmlEnd,outBufSize);
	//} else if(http_checkUrlBase(urlStr,"setAllInputs")) {
	//	http_setup(outbuf, httpMimeTypeHTML);
	//	strcat_safe(outbuf,htmlHeader,outBufSize);
	//	// it breaks UART pins as well, omg!
	//	for(i = 0; i < GPIO_MAX; i++) {
	//		PIN_SetPinRoleForPinIndex(i,IOR_Button);
	//		PIN_SetPinChannelForPinIndex(i,1);
	//	}
	//	http_setup(outbuf, httpMimeTypeHTML);
	//	strcat_safe(outbuf,"Set all inputs for dbg .",outBufSize);
	//	strcat_safe(outbuf,htmlReturnToMenu,outBufSize);
	//	HTTP_AddBuildFooter(outbuf,outBufSize);
	//	strcat_safe(outbuf,htmlEnd,outBufSize);
	} else if(http_checkUrlBase(urlStr,"cfg_pins")) {
		int iChanged = 0;
		int iChangedRequested = 0;

		http_setup(outbuf, httpMimeTypeHTML);
		strcat(outbuf,htmlHeader);
		strcat_safe(outbuf,g_header,outBufSize);
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
			if(http_getArg(recvbuf,tmpA,tmpB,sizeof(tmpB))) {
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
		if (sendpart){
			sendpart(socket, outbuf, strlen(outbuf));
			outbuf[0] = 0;
		}
		if(iChangedRequested>0) {
			PIN_SaveToFlash();
			sprintf(tmpA, "Pins update - %i reqs, %i changed!<br><br>",iChangedRequested,iChanged);
			strcat(outbuf,tmpA);
		}
	//	strcat(outbuf,"<button type=\"button\">Click Me!</button>");
		strcat(outbuf,"<form action=\"cfg_pins\">");
		for( i = 0; i < GPIO_MAX; i++) {
			int si, ch;
			si = PIN_GetPinRoleForPinIndex(i);
			ch = PIN_GetPinChannelForPinIndex(i);
			sprintf(tmpA, "P%i ",i);
			strcat(outbuf,tmpA);
			sprintf(tmpA, "<select name=\"%i\">",i);
			strcat(outbuf,tmpA);
			if (sendpart){
				sendpart(socket, outbuf, strlen(outbuf));
				outbuf[0] = 0;
			}
			for(j = 0; j < IOR_Total_Options; j++) {
				if(j == si) {
					sprintf(tmpA, "<option value=\"%i\" selected>%s</option>",j,htmlPinRoleNames[j]);
				} else {
					sprintf(tmpA, "<option value=\"%i\">%s</option>",j,htmlPinRoleNames[j]);
				}
				strcat(outbuf,tmpA);
				if (sendpart){
					sendpart(socket, outbuf, strlen(outbuf));
					outbuf[0] = 0;
				}
			}
			strcat(outbuf, "</select>");
			if(ch == 0) {
				tmpB[0] = 0;
			} else {
				sprintf(tmpB,"%i",ch);
			}
			sprintf(tmpA, "<input name=\"r%i\" type=\"text\" value=\"%s\"/>",i,tmpB);
			strcat(outbuf,tmpA);
			strcat(outbuf,"<br>");
			if (sendpart){
				sendpart(socket, outbuf, strlen(outbuf));
				outbuf[0] = 0;
			}
		}
		strcat(outbuf,"<input type=\"submit\" value=\"Save\"/></form>");

		strcat(outbuf,htmlReturnToCfg);
		HTTP_AddBuildFooter(outbuf,outBufSize);
		strcat(outbuf,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"index")) {
		int relayFlags;
		int pwmFlags;

		relayFlags = 0;
		pwmFlags = 0;

		http_setup(outbuf, httpMimeTypeHTML);
		strcat(outbuf,htmlHeader);
		strcat(outbuf,"<style>.r { background-color: red; } .g { background-color: green; }</style>");
		strcat_safe(outbuf,g_header,outBufSize);
		if(http_getArg(urlStr,"tgl",tmpA,sizeof(tmpA))) {
			j = atoi(tmpA);
			sprintf(tmpA,"<h3>Toggled %i!</h3>",j);
			strcat(outbuf,tmpA);
			CHANNEL_Toggle(j);
		}
		if(http_getArg(urlStr,"on",tmpA,sizeof(tmpA))) {
			j = atoi(tmpA);
			sprintf(tmpA,"<h3>Enabled %i!</h3>",j);
			strcat(outbuf,tmpA);
			CHANNEL_Set(j,255,1);
		}
		if(http_getArg(urlStr,"off",tmpA,sizeof(tmpA))) {
			j = atoi(tmpA);
			sprintf(tmpA,"<h3>Disabled %i!</h3>",j);
			strcat(outbuf,tmpA);
			CHANNEL_Set(j,0,1);
		}
		if(http_getArg(urlStr,"pwm",tmpA,sizeof(tmpA))) {
			int newPWMValue = atoi(tmpA);
			http_getArg(urlStr,"pwmIndex",tmpA,sizeof(tmpA));
			j = atoi(tmpA);
			sprintf(tmpA,"<h3>Changed pwm %i to %i!</h3>",j,newPWMValue);
			strcat(outbuf,tmpA);
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
				strcat(outbuf,"<form action=\"index\">");
				sprintf(tmpA,"<input type=\"hidden\" name=\"tgl\" value=\"%i\">",i);
				strcat(outbuf,tmpA);
				sprintf(tmpA,"<input class=\"%s\" type=\"submit\" value=\"Toggle %i\"/></form>",c,i);
				strcat(outbuf,tmpA);
			}
			if(BIT_CHECK(pwmFlags,i)) {
				int pwmValue;

				pwmValue = CHANNEL_Get(i);
				sprintf(tmpA,"<form action=\"index\" id=\"form%i\">",i);
				strcat(outbuf,tmpA);
				sprintf(tmpA,"<input type=\"range\" min=\"0\" max=\"100\" name=\"pwm\" id=\"slider%i\" value=\"%i\">",i,pwmValue);
				strcat(outbuf,tmpA);
				sprintf(tmpA,"<input type=\"hidden\" name=\"pwmIndex\" value=\"%i\">",i);
				strcat(outbuf,tmpA);
				sprintf(tmpA,"<input  type=\"submit\" style=\"display:none;\" value=\"Toggle %i\"/></form>",i);
				strcat(outbuf,tmpA);


				strcat(outbuf,"<script>");
				sprintf(tmpA,"var slider = document.getElementById(\"slider%i\");\n",i);
				strcat(outbuf,tmpA);
				strcat(outbuf,"slider.onmouseup = function () {\n");
				sprintf(tmpA," document.getElementById(\"form%i\").submit();\n",i);
				strcat(outbuf,tmpA);
				strcat(outbuf,"}\n");
				strcat(outbuf,"</script>");
			}
		}
	//	strcat(outbuf,"<button type=\"button\">Click Me!</button>");
		strcat(outbuf,"<form action=\"cfg\"><input type=\"submit\" value=\"Config\"/></form>");
		strcat(outbuf,"<form action=\"about\"><input type=\"submit\" value=\"About\"/></form>");


		strcat(outbuf,htmlReturnToMenu);
		HTTP_AddBuildFooter(outbuf,outBufSize);
		strcat(outbuf,htmlEnd);
    } else if(http_checkUrlBase(urlStr,"ota_exec")) {
        http_setup(outbuf, httpMimeTypeHTML);
        strcat(outbuf,htmlHeader);
		if(http_getArg(urlStr,"host",tmpA,sizeof(tmpA))) {
			sprintf(tmpB,"<h3>OTA requested for %s!</h3>",tmpA);
			strcat(outbuf,tmpB);
#if WINDOWS

#else
        otarequest(tmpA);
#endif
		}
        strcat(outbuf,htmlReturnToMenu);
		HTTP_AddBuildFooter(outbuf,outBufSize);
        strcat(outbuf,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"ota")) {
		http_setup(outbuf, httpMimeTypeHTML);
		strcat(outbuf,htmlHeader);
		strcat_safe(outbuf,"<form action=\"/ota_exec\">\
			  <label for=\"host\">URL for new bin file:</label><br>\
			  <input type=\"text\" id=\"host\" name=\"host\" value=\"",outBufSize);
		strcat_safe(outbuf,"\"><br>\
			  <input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure? Please check MQTT data twice?')\">\
			</form> ",outBufSize);
        strcat(outbuf,htmlReturnToMenu);
		HTTP_AddBuildFooter(outbuf,outBufSize);
        strcat(outbuf,htmlEnd);
	} else if(http_checkUrlBase(urlStr,"")) {
		// Redirect / to /index page
        strcat(outbuf,"HTTP/1.1 302 OK\nLocation: /index\nConnection: close\n\n");
	} else {
		http_setup(outbuf, httpMimeTypeHTML);
		strcat(outbuf,htmlHeader);
		strcat_safe(outbuf,g_header,outBufSize);
		strcat(outbuf,"Not found.<br>\n");
		strcat(outbuf,htmlReturnToMenu);
		HTTP_AddBuildFooter(outbuf,outBufSize);
		strcat(outbuf,htmlEnd);
	}
	i = strlen(outbuf);
	if(i >= outBufSize-1) {
		// Rewrite all to allow user to know that something went wrong
		http_setup(outbuf, httpMimeTypeHTML);
		strcat(outbuf,htmlHeader);
		strcat_safe(outbuf,g_header,outBufSize);
		sprintf(tmpA, "Buffer overflow occured while trying to process your request.<br>");
		strcat(outbuf,tmpA);
		strcat(outbuf,htmlReturnToMenu);
		HTTP_AddBuildFooter(outbuf,outBufSize);
		strcat(outbuf,htmlEnd);
	}
	i = strlen(outbuf);
	return i;
}