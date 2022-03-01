#include "../new_common.h"
#include "ctype.h" 
#include "http_fns.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../ota/ota.h"

#ifdef WINDOWS
    // nothing
#elif PLATFORM_XR809
    #include <image/flash.h>
#elif defined(PLATFORM_BK7231N)
    // tuya-iotos-embeded-sdk-wifi-ble-bk7231n/sdk/include/tuya_hal_storage.h
    #include "tuya_hal_storage.h"
    #include "BkDriverFlash.h"
    #include "../flash_config/flash_config.h"
#else
    // REALLY? A typo in Tuya SDK? Storge?
    // tuya-iotos-embeded-sdk-wifi-ble-bk7231t/platforms/bk7231t/tuya_os_adapter/include/driver/tuya_hal_storge.h
    #include "../logging/logging.h"
    #include "tuya_hal_storge.h"
    #include "BkDriverFlash.h"
    #include "../flash_config/flash_config.h"
#endif




typedef struct template_s {
	void (*setter)();
	const char *name;
} template_t;

template_t g_templates [] = {
	{ Setup_Device_Empty, "Empty"},
	// BK7231N devices
	{ Setup_Device_BK7231N_CB2S_QiachipSmartSwitch, "[BK7231N][CB2S] QiaChip Smart Switch"},
	// BK7231T devices
	{ Setup_Device_BK7231T_WB2S_QiachipSmartSwitch, "[BK7231T][WB2S] QiaChip Smart Switch"},
	{ Setup_Device_TuyaWL_SW01_16A, "WL SW01 16A"},
	{ Setup_Device_TuyaSmartLife4CH10A, "Smart Life 4CH 10A"},
	{ Setup_Device_IntelligentLife_NF101A, "Intelligent Life NF101A"},
	{ Setup_Device_TuyaLEDDimmerSingleChannel, "Tuya LED Dimmer Single Channel PWM WB3S"},
	{ Setup_Device_CalexLEDDimmerFiveChannel, "Calex RGBWW LED Dimmer Five Channel PWM BK7231S"},
	{ Setup_Device_CalexPowerStrip_900018_1v1_0UK, "Calex UK power strip 900018.1 v1.0 UK"},
	{ Setup_Device_ArlecCCTDownlight, "Arlec CCT LED Downlight ALD029CHA"},
	{ Setup_Device_NedisWIFIPO120FWT_16A, "Nedis WIFIPO120FWT SmartPlug 16A"},
	{ Setup_Device_NedisWIFIP130FWT_10A, "Nedis WIFIP130FWT SmartPlug 10A"},
	{ Setup_Device_BK7231T_Raw_PrimeWiFiSmartOutletsOutdoor_CCWFIO232PK, "Prime SmartOutlet Outdoor 2x Costco"},
	{ Setup_Device_EmaxHome_EDU8774, "Emax Home EDU8774 SmartPlug 16A"},
	{ Setup_Device_TuyaSmartPFW02G, "Tuya Smart PFW02-G"}
};

int g_total_templates = sizeof(g_templates)/sizeof(g_templates[0]);

unsigned char hexdigit( char hex ) {
    return (hex <= '9') ? hex - '0' : 
                          toupper((unsigned char)hex) - 'A' + 10 ;
}

unsigned char hexbyte( const char* hex ) {
    return (hexdigit(*hex) << 4) | hexdigit(*(hex+1)) ;
}

int http_fn_empty_url(http_request_t *request) {
    poststr(request,"HTTP/1.1 302 OK\nLocation: /index\nConnection: close\n\n");
	poststr(request, NULL);
    return 0;
}

int http_fn_index(http_request_t *request) {
    int relayFlags;
    int pwmFlags;
    int j, i;
	char tmpA[128];

    relayFlags = 0;
    pwmFlags = 0;

    http_setup(request, httpMimeTypeHTML);
    poststr(request,htmlHeader);
    poststr(request,"<style>.r { background-color: red; } .g { background-color: green; }</style>");
    poststr(request,g_header);
    if(http_getArg(request->url,"tgl",tmpA,sizeof(tmpA))) {
        j = atoi(tmpA);
        hprintf128(request,"<h3>Toggled %i!</h3>",j);
        CHANNEL_Toggle(j);
    }
    if(http_getArg(request->url,"on",tmpA,sizeof(tmpA))) {
        j = atoi(tmpA);
        hprintf128(request,"<h3>Enabled %i!</h3>",j);
        CHANNEL_Set(j,255,1);
    }
    if(http_getArg(request->url,"off",tmpA,sizeof(tmpA))) {
        j = atoi(tmpA);
        hprintf128(request,"<h3>Disabled %i!</h3>",j);
        CHANNEL_Set(j,0,1);
    }
    if(http_getArg(request->url,"pwm",tmpA,sizeof(tmpA))) {
        int newPWMValue = atoi(tmpA);
        http_getArg(request->url,"pwmIndex",tmpA,sizeof(tmpA));
        j = atoi(tmpA);
        hprintf128(request,"<h3>Changed pwm %i to %i!</h3>",j,newPWMValue);
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
            hprintf128(request,"<input type=\"hidden\" name=\"tgl\" value=\"%i\">",i);
            hprintf128(request,"<input class=\"%s\" type=\"submit\" value=\"Toggle %i\"/></form>",c,i);
        }
        if(BIT_CHECK(pwmFlags,i)) {
            int pwmValue;

            pwmValue = CHANNEL_Get(i);
            hprintf128(request,"<form action=\"index\" id=\"form%i\">",i);
            hprintf128(request,"<input type=\"range\" min=\"0\" max=\"100\" name=\"pwm\" id=\"slider%i\" value=\"%i\">",i,pwmValue);
            hprintf128(request,"<input type=\"hidden\" name=\"pwmIndex\" value=\"%i\">",i);
            hprintf128(request,"<input  type=\"submit\" style=\"display:none;\" value=\"Toggle %i\"/></form>",i);


            poststr(request,"<script>");
            hprintf128(request,"var slider = document.getElementById(\"slider%i\");\n",i);
            poststr(request,"slider.onmouseup = function () {\n");
            hprintf128(request," document.getElementById(\"form%i\").submit();\n",i);
            poststr(request,"}\n");
            poststr(request,"</script>");
        }
    }
//	strcat(outbuf,"<button type=\"button\">Click Me!</button>");

    
    if(http_getArg(request->url,"restart",tmpA,sizeof(tmpA))) {
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

	poststr(request, NULL);
    return 0;
}

int http_fn_about(http_request_t *request){
    http_setup(request, httpMimeTypeHTML);
    poststr(request,htmlHeader);
    poststr(request,g_header);
    poststr(request,"About us page.");
    poststr(request,htmlReturnToMenu);
    HTTP_AddBuildFooter(request);
    poststr(request,htmlEnd);
	poststr(request, NULL);
    return 0;
}




int http_fn_cfg_mqtt(http_request_t *request) {
    int i;
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
    hprintf128(request, "%i", i);
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
	poststr(request, NULL);
    return 0;
}

int http_fn_cfg_mqtt_set(http_request_t *request) {
	char tmpA[128];
    http_setup(request, httpMimeTypeHTML);
    poststr(request,htmlHeader);
    poststr(request,g_header);

    if(http_getArg(request->url,"host",tmpA,sizeof(tmpA))) {
        CFG_SetMQTTHost(tmpA);
    }
    if(http_getArg(request->url,"port",tmpA,sizeof(tmpA))) {
        CFG_SetMQTTPort(atoi(tmpA));
    }
    if(http_getArg(request->url,"user",tmpA,sizeof(tmpA))) {
        CFG_SetMQTTUserName(tmpA);
    }
    if(http_getArg(request->url,"password",tmpA,sizeof(tmpA))) {
        CFG_SetMQTTPass(tmpA);
    }
    if(http_getArg(request->url,"client",tmpA,sizeof(tmpA))) {
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

	poststr(request, NULL);
    return 0;
}




int http_fn_cfg_webapp(http_request_t *request) {
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
	poststr(request, NULL);
    return 0;
}

int http_fn_cfg_webapp_set(http_request_t *request) {
	char tmpA[128];
    http_setup(request, httpMimeTypeHTML);
    poststr(request,htmlHeader);
    poststr(request,g_header);

    if(http_getArg(request->url,"url",tmpA,sizeof(tmpA))) {
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
	poststr(request, NULL);
    return 0;
}




int http_fn_cfg_wifi(http_request_t *request) {
    // for a test, show password as well...
    const char *cur_ssid, *cur_pass;
	char tmpA[128];
    int i;

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
    if(http_getArg(request->url,"scan",tmpA,sizeof(tmpA))) {
#ifdef WINDOWS

        poststr(request,"Not available on Windows<br>");
#elif PLATFORM_XR809
        poststr(request,"TODO XR809<br>");

#elif PLATFORM_BK7231T
        AP_IF_S *ar;
        uint32_t num;
        
        bk_printf("Scan begin...\r\n");
        tuya_hal_wifi_all_ap_scan(&ar,&num);
        bk_printf("Scan returned %i networks\r\n",num);
        for(i = 0; i < num; i++) {
            hprintf128(request,"[%i/%i] SSID: %s, Channel: %i, Signal %i<br>",i,(int)num,ar[i].ssid, ar[i].channel, ar[i].rssi);
        }
        tuya_hal_wifi_release_ap(ar);
#elif PLATFORM_BK7231N
        poststr(request,"TODO: BK7231N support for scan<br>");

#else
#error "Unknown platform"
        poststr(request,"Unknown platform<br>");
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

	poststr(request, NULL);
    return 0;
}

int http_fn_cfg_wifi_set(http_request_t *request) {
	char tmpA[128];
	printf("HTTP_ProcessPacket: generating cfg_wifi_set \r\n");

    http_setup(request, httpMimeTypeHTML);
    poststr(request,htmlHeader);
    poststr(request,g_header);
    if(http_getArg(request->url,"open",tmpA,sizeof(tmpA))) {
        CFG_SetWiFiSSID("");
        CFG_SetWiFiPass("");
        poststr(request,"WiFi mode set: open access point.");
    } else {
        if(http_getArg(request->url,"ssid",tmpA,sizeof(tmpA))) {
            CFG_SetWiFiSSID(tmpA);
        }
        if(http_getArg(request->url,"pass",tmpA,sizeof(tmpA))) {
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
    
	poststr(request, NULL);
    return 0;
}




int http_fn_cfg_loglevel_set(http_request_t *request) {
	char tmpA[128];
    printf("HTTP_ProcessPacket: generating cfg_loglevel_set \r\n");

    http_setup(request, httpMimeTypeHTML);
    poststr(request,htmlHeader);
    poststr(request,g_header);
    if(http_getArg(request->url,"loglevel",tmpA,sizeof(tmpA))) {
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
    hprintf128(request,"%i",loglevel);
#endif
    poststr(request,"\"><br><br>\
            <input type=\"submit\" value=\"Submit\" >\
        </form> ");
    
    poststr(request,"<br>");
    poststr(request,"<a href=\"cfg\">Return to config settings</a>");
    poststr(request,"<br>");
    poststr(request,htmlReturnToCfg);
    HTTP_AddBuildFooter(request);
    poststr(request,htmlEnd);
	poststr(request, NULL);
    return 0;
}


int http_fn_cfg_mac(http_request_t *request) {
    // must be unsigned, else print below prints negatives as e.g. FFFFFFFe
    unsigned char mac[6];
	char tmpA[128];
    int i;

    http_setup(request, httpMimeTypeHTML);
    poststr(request,htmlHeader);
    poststr(request,g_header);

    if(http_getArg(request->url,"mac",tmpA,sizeof(tmpA))) {
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


    poststr(request,"<h2> Here you can change MAC address.</h2>");
    poststr(request,"<form action=\"/cfg_mac\">\
            <label for=\"mac\">MAC:</label><br>\
            <input type=\"text\" id=\"mac\" name=\"mac\" value=\"");
    hprintf128(request,"%02X%02X%02X%02X%02X%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    poststr(request,"\"><br><br>\
            <input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure? Please check MAC hex string twice?')\">\
        </form> ");
    poststr(request,htmlReturnToCfg);
    HTTP_AddBuildFooter(request);
    poststr(request,htmlEnd);

	poststr(request, NULL);
    return 0;
}

int http_fn_flash_read_tool(http_request_t *request) {
    int len = 16;
    int ofs = 1970176;
    int res;
    int rem;
    int now;
    int nowOfs;
    int hex;
    int i;
	char tmpA[128];
	char tmpB[64];

    http_setup(request, httpMimeTypeHTML);
    poststr(request,htmlHeader);
    poststr(request,g_header);
    poststr(request,"<h4>Flash Read Tool</h4>");
    if(	http_getArg(request->url,"hex",tmpA,sizeof(tmpA))){
        hex = atoi(tmpA);
    } else {
        hex = 0;
    }

    if(	http_getArg(request->url,"offset",tmpA,sizeof(tmpA)) &&
            http_getArg(request->url,"len",tmpB,sizeof(tmpB))) {
        unsigned char buffer[128];
        len = atoi(tmpB);
        ofs = atoi(tmpA);
        hprintf128(request,"Memory at %i with len %i reads: ",ofs,len);
        poststr(request,"<br>");

        ///res = bekken_hal_flash_read (ofs, buffer,len);
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
            res = bekken_hal_flash_read (nowOfs, buffer,now);
#endif
            for(i = 0; i < now; i++) {
                unsigned char val = buffer[i];
                if(!hex && isprint(val)) {
                    hprintf128(request,"'%c' ",val);
                } else {
                    hprintf128(request,"%02X ",val);
                }
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
    hprintf128(request," value=\"%i\"><br>",ofs);
    poststr(request,"<label for=\"lenght\">lenght:</label><br>\
            <input type=\"number\" id=\"len\" name=\"len\" ");
    hprintf128(request,"value=\"%i\">",len);
    poststr(request,"<br><br>\
            <input type=\"submit\" value=\"Submit\">\
        </form> ");

    poststr(request,htmlReturnToCfg);
    HTTP_AddBuildFooter(request);
    poststr(request,htmlEnd);


	poststr(request, NULL);
    return 0;
}

int http_fn_config_dump_table(http_request_t *request) {
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
	poststr(request, NULL);
    return 0;
}




int http_fn_cfg_quick(http_request_t *request) {
	char tmpA[128];
    int j;
    http_setup(request, httpMimeTypeHTML);
    poststr(request,htmlHeader);
    poststr(request,g_header);
    poststr(request,"<h4>Quick Config</h4>");
    
    if(http_getArg(request->url,"dev",tmpA,sizeof(tmpA))) {
        j = atoi(tmpA);
        hprintf128(request,"<h3>Set dev %i!</h3>",j);
        g_templates[j].setter();
    }
    poststr(request,"<form action=\"cfg_quick\">");		
    poststr(request, "<select name=\"dev\">");
    for(j = 0; j < g_total_templates; j++) {
        hprintf128(request, "<option value=\"%i\">%s</option>",j,g_templates[j].name);
    }
    poststr(request,"</select>");
    poststr(request,"<input type=\"submit\" value=\"Set\"/></form>");
    
    poststr(request,htmlReturnToCfg);
    HTTP_AddBuildFooter(request);
    poststr(request,htmlEnd);

	poststr(request, NULL);
    return 0;
}

int http_fn_cfg_ha(http_request_t *request) {
    int relayFlags = 0;
    int pwmFlags = 0;
    int relayCount = 0;
    int pwmCount = 0;
    const char *baseName;
    int i;
	//char tmpA[128];

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
                hprintf128(request,"    name: \"%s %i\"\n",baseName,i);
                hprintf128(request,"    state_topic: \"%s/%i/get\"\n",baseName,i);
                hprintf128(request,"    command_topic: \"%s/%i/set\"\n",baseName,i);
                poststr(request,"    qos: 1\n");
                poststr(request,"    payload_on: 0\n");
                poststr(request,"    payload_off: 1\n");
                poststr(request,"    retain: true\n");
                hprintf128(request,"    availability_topic: \"%s/connected\"\n",baseName);
            }
        }
    }
    if(pwmCount > 0) {
        poststr(request,"light:\n");
        for(i = 0; i < CHANNEL_MAX; i++) {
            if(BIT_CHECK(pwmFlags,i)) {
                poststr(request,"  - platform: mqtt\n");
                hprintf128(request,"    name: \"%s %i\"\n",baseName,i);
                hprintf128(request,"    state_topic: \"%s/%i/get\"\n",baseName,i);
                hprintf128(request,"    command_topic: \"%s/%i/set\"\n",baseName,i);
                hprintf128(request,"    brightness_command_topic: \"%s/%i/set\"\n",baseName,i);
                poststr(request,"    on_command_type: \"brightness\"\n");
                poststr(request,"    brightness_scale: 99\n");
                poststr(request,"    qos: 1\n");
                poststr(request,"    payload_on: 99\n");
                poststr(request,"    payload_off: 0\n");
                poststr(request,"    retain: true\n");
                poststr(request,"    optimistic: true\n");
                hprintf128(request,"    availability_topic: \"%s/connected\"\n",baseName);
            }
        }
    }

    poststr(request,"</textarea>");

    poststr(request,htmlReturnToCfg);
    HTTP_AddBuildFooter(request);
    poststr(request,htmlEnd);

	poststr(request, NULL);
    return 0;
}

int http_fn_cfg(http_request_t *request) {
    int i,j,k;
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
    hprintf128(request,"BK_PARTITION_NET_PARAM: bOk %i, at %i, len %i<br>",k,i,j);
    k = config_get_tableOffsets(BK_PARTITION_RF_FIRMWARE,&i,&j);
    hprintf128(request,"BK_PARTITION_RF_FIRMWARE: bOk %i, at %i, len %i<br>",k,i,j);
    k = config_get_tableOffsets(BK_PARTITION_OTA,&i,&j);
    hprintf128(request,"BK_PARTITION_OTA: bOk %i, at %i, len %i<br>",k,i,j);
#endif

    poststr(request,"<a href=\"/app\" target=\"_blank\">Launch Web Application</a><br/>");

    poststr(request,htmlReturnToMenu);
    HTTP_AddBuildFooter(request);
    poststr(request,htmlEnd);
	poststr(request, NULL);
    return 0;
}

int http_fn_cfg_pins(http_request_t *request) {
    int iChanged = 0;
    int iChangedRequested = 0;
    int i;
	char tmpA[128];
	char tmpB[64];

    http_setup(request, httpMimeTypeHTML);
    poststr(request,htmlHeader);
    poststr(request,g_header);
    for(i = 0; i < GPIO_MAX; i++) {
        sprintf(tmpA, "%i",i);
        if(http_getArg(request->url,tmpA,tmpB,sizeof(tmpB))) {
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
        if(http_getArg(request->url,tmpA,tmpB,sizeof(tmpB))) {
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
        hprintf128(request, "Pins update - %i reqs, %i changed!<br><br>",iChangedRequested,iChanged);
    }
//	strcat(outbuf,"<button type=\"button\">Click Me!</button>");
    poststr(request,"<form action=\"cfg_pins\">");
    for( i = 0; i < GPIO_MAX; i++) {
        int si, ch;
        int j;

        si = PIN_GetPinRoleForPinIndex(i);
        ch = PIN_GetPinChannelForPinIndex(i);

#if PLATFORM_XR809
        poststr(request,PIN_GetPinNameAlias(i));
        poststr(request," ");
#else
        hprintf128(request, "P%i ",i);
#endif
        hprintf128(request, "<select name=\"%i\">",i);
        for(j = 0; j < IOR_Total_Options; j++) {
            if(j == si) {
                hprintf128(request, "<option value=\"%i\" selected>%s</option>",j,htmlPinRoleNames[j]);
            } else {
                hprintf128(request, "<option value=\"%i\">%s</option>",j,htmlPinRoleNames[j]);
            }
        }
        poststr(request, "</select>");
        if(ch == 0) {
            tmpB[0] = 0;
        } else {
            sprintf(tmpB,"%i",ch);
        }
        hprintf128(request, "<input name=\"r%i\" type=\"text\" value=\"%s\"/>",i,tmpB);
        poststr(request,"<br>");
    }
    poststr(request,"<input type=\"submit\" value=\"Save\"/></form>");

    poststr(request,htmlReturnToCfg);
    HTTP_AddBuildFooter(request);
    poststr(request,htmlEnd);

	poststr(request, NULL);
    return 0;
}

int http_fn_ota_exec(http_request_t *request) {
	char tmpA[128];
	//char tmpB[64];

    http_setup(request, httpMimeTypeHTML);
    poststr(request,htmlHeader);
    if(http_getArg(request->url,"host",tmpA,sizeof(tmpA))) {
        hprintf128(request,"<h3>OTA requested for %s!</h3>",tmpA);
#if WINDOWS

#elif PLATFORM_XR809
    //cmd_ota_http_exec(tmpA);
    xr809_do_ota_next_frame(tmpA);
#else
    otarequest(tmpA);
#endif
    }
    poststr(request,htmlReturnToMenu);
    HTTP_AddBuildFooter(request);
    poststr(request,htmlEnd);

	poststr(request, NULL);
    return 0;
}

int http_fn_ota(http_request_t *request) {
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

	poststr(request, NULL);
    return 0;
}

int http_fn_other(http_request_t *request) {
    http_setup(request, httpMimeTypeHTML);
    poststr(request,htmlHeader);
    poststr(request,g_header);
    poststr(request,"Not found.<br/>");
    poststr(request,htmlReturnToMenu);
    HTTP_AddBuildFooter(request);
    poststr(request,htmlEnd);
	poststr(request, NULL);
    return 0;
}
