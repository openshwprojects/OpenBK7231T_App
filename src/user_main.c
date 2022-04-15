/*
 * @Author: yj 
 * @email: shiliu.yang@tuya.com
 * @LastEditors: yj 
 * @file name: tuya_device.c
 * @Description: template demo for SDK WiFi & BLE for BK7231T
 * @Copyright: HANGZHOU TUYA INFORMATION TECHNOLOGY CO.,LTD
 * @Company: http://www.tuya.com
 * @Date: 2021-01-23 16:33:00
 * @LastEditTime: 2021-01-27 17:00:00
 */
//
//#include "lwip/sockets.h"
//#include "lwip/ip_addr.h"
//#include "lwip/inet.h"

#include "mem_pub.h"
#include "str_pub.h"
#include "ethernet_intf.h"
#include "driver/drv_public.h"

// Commands register, execution API and cmd tokenizer
#include "cmnds/cmd_public.h"

// overall config variables for app - like BK_LITTLEFS
#include "obk_config.h"

#include "httpserver/new_http.h"
#include "new_pins.h"
#include "new_cfg.h"
#include "logging/logging.h"
#include "httpserver/http_tcp_server.h"
#include "httpserver/rest_interface.h"
#include "printnetinfo/printnetinfo.h"
#include "mqtt/new_mqtt.h"

#include "../../beken378/app/config/param_config.h"
#include "lwip/netdb.h"
#include "littlefs/our_lfs.h"

#include "flash_config/flash_config.h"
#include "flash_config/flash_vars_vars.h"
#include "driver/drv_ntp.h"

static int g_secondsElapsed = 0;

static int g_openAP = 0;
static int g_connectToWiFi = 0;
static int bSafeMode = 0;

// reset in this number of seconds
static int g_reset = 0;
beken_timer_t g_main_timer_1s;

// save config in this number of seconds
int g_savecfg = 0;

int g_bHasWiFiConnected = 0;

#define LOG_FEATURE LOG_FEATURE_MAIN

// from wlan_ui.c
void bk_reboot(void);


void RESET_ScheduleModuleReset(int delSeconds) {
	g_reset = delSeconds;
}

int Time_getUpTimeSeconds() {
	return g_secondsElapsed;
}

// receives status change notifications about wireless - could be useful
// ctxt is pointer to a rw_evt_type
void wl_status( void *ctxt ){

    rw_evt_type stat = *((rw_evt_type*)ctxt);
    ADDLOGF_INFO("wl_status %d\r\n", stat);

    switch(stat){
        case RW_EVT_STA_IDLE:
        case RW_EVT_STA_SCANNING:
        case RW_EVT_STA_SCAN_OVER:
        case RW_EVT_STA_CONNECTING:
            PIN_set_wifi_led(0);
			g_bHasWiFiConnected = 0;
            break;
        case RW_EVT_STA_BEACON_LOSE:
        case RW_EVT_STA_PASSWORD_WRONG:
        case RW_EVT_STA_NO_AP_FOUND:
        case RW_EVT_STA_ASSOC_FULL:
        case RW_EVT_STA_DISCONNECTED:    /* disconnect with server */
            // try to connect again in few seconds
            g_connectToWiFi = 15;
            PIN_set_wifi_led(0);
			g_bHasWiFiConnected = 0;
            break;
        case RW_EVT_STA_CONNECT_FAILED:  /* authentication failed */
            PIN_set_wifi_led(0);
            // try to connect again in few seconds
            g_connectToWiFi = 60;
			g_bHasWiFiConnected = 0;
            break;
        case RW_EVT_STA_CONNECTED:        /* authentication success */    
        case RW_EVT_STA_GOT_IP: 
            PIN_set_wifi_led(1);
			g_bHasWiFiConnected = 1;
            break;
        
        /* for softap mode */
        case RW_EVT_AP_CONNECTED:          /* a client association success */
            PIN_set_wifi_led(1);
			g_bHasWiFiConnected = 1;
            break;
        case RW_EVT_AP_DISCONNECTED:       /* a client disconnect */
        case RW_EVT_AP_CONNECT_FAILED:     /* a client association failed */
            PIN_set_wifi_led(0);
			g_bHasWiFiConnected = 0;
            break;
        default:
            break;
    }

}


// from wlan_ui.c, no header
void bk_wlan_status_register_cb(FUNC_1PARAM_PTR cb);
static int setup_wifi_open_access_point(void);



void connect_to_wifi(const char *oob_ssid,const char *connect_key)
{
#if 1
	network_InitTypeDef_adv_st	wNetConfigAdv;

	os_memset( &wNetConfigAdv, 0x0, sizeof(network_InitTypeDef_adv_st) );
	
	os_strcpy((char*)wNetConfigAdv.ap_info.ssid, oob_ssid);
	hwaddr_aton("48:ee:0c:48:93:12", (unsigned char *)wNetConfigAdv.ap_info.bssid);
	wNetConfigAdv.ap_info.security = SECURITY_TYPE_WPA2_MIXED;
	wNetConfigAdv.ap_info.channel = 5;
	
	os_strcpy((char*)wNetConfigAdv.key, connect_key);
	wNetConfigAdv.key_len = os_strlen(connect_key);
	wNetConfigAdv.dhcp_mode = DHCP_CLIENT;
	wNetConfigAdv.wifi_retry_interval = 100;

	bk_wlan_start_sta_adv(&wNetConfigAdv);
#else
    network_InitTypeDef_st network_cfg;
	
    os_memset(&network_cfg, 0x0, sizeof(network_InitTypeDef_st));

    os_strcpy((char *)network_cfg.wifi_ssid, oob_ssid);
    os_strcpy((char *)network_cfg.wifi_key, connect_key);

    network_cfg.wifi_mode = STATION;
    network_cfg.dhcp_mode = DHCP_CLIENT;
    network_cfg.wifi_retry_interval = 100;

    ADDLOGF_INFO("ssid:%s key:%s\r\n", network_cfg.wifi_ssid, network_cfg.wifi_key);
			
    bk_wlan_start(&network_cfg);
#endif
}




static void app_led_timer_handler(void *data)
{
	// run_adc_test();
	MQTT_RunEverySecondUpdate();
	RepeatingEvents_OnEverySecond();
	DRV_OnEverySecond();

	g_secondsElapsed ++;
	ADDLOGF_INFO("Timer is %i free mem %d\n", g_secondsElapsed, xPortGetFreeHeapSize());

  // print network info
  if (!(g_secondsElapsed % 10)){
    print_network_info();
  }

  // when we hit 30s, mark as boot complete.
  if (g_secondsElapsed == BOOT_COMPLETE_SECONDS){
    boot_complete();
  }

  if (g_openAP){
    g_openAP--;
    if (0 == g_openAP){
      setup_wifi_open_access_point();
    }
  }
  if(g_connectToWiFi){
	g_connectToWiFi --;
	if(0 == g_connectToWiFi) {
		const char *wifi_ssid, *wifi_pass;
		wifi_ssid = CFG_GetWiFiSSID();
		wifi_pass = CFG_GetWiFiPass();
		connect_to_wifi(wifi_ssid,wifi_pass);
		// register function to get callbacks about wifi changes.
		bk_wlan_status_register_cb(wl_status);
		ADDLOGF_DEBUG("Registered for wifi changes\r\n");
		// reconnect after 10 minutes?
		//g_connectToWiFi = 60 * 10; 
	}
  }

  if (g_reset){
      g_reset--;
      if (!g_reset){
        // ensure any config changes are saved before reboot.
        config_commit(); 
        bk_reboot(); 
      }
  }

    if (g_savecfg){
        g_savecfg--;
      if (!g_savecfg){
        config_commit(); 
      }
    }

}

void app_on_generic_dbl_click(int btnIndex)
{
	if(g_secondsElapsed < 5) {
		CFG_SetOpenAccessPoint();
		CFG_SaveWiFi();
	}
}


static int setup_wifi_open_access_point(void)
{
    //#define APP_DRONE_DEF_SSID          "WIFI_UPV_000000"
    #define APP_DRONE_DEF_NET_IP        "192.168.4.1"
    #define APP_DRONE_DEF_NET_MASK      "255.255.255.0"
    #define APP_DRONE_DEF_NET_GW        "192.168.4.1"
    #define APP_DRONE_DEF_CHANNEL       1    
    
    general_param_t general;
    ap_param_t ap_info;
    network_InitTypeDef_st wNetConfig;
    int len;
    unsigned char *mac;
    
    os_memset(&general, 0, sizeof(general_param_t));
    os_memset(&ap_info, 0, sizeof(ap_param_t)); 
    os_memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_st));  
    
        general.role = 1,
        general.dhcp_enable = 1,

        os_strcpy((char *)wNetConfig.local_ip_addr, APP_DRONE_DEF_NET_IP);
        os_strcpy((char *)wNetConfig.net_mask, APP_DRONE_DEF_NET_MASK);
        os_strcpy((char *)wNetConfig.dns_server_ip_addr, APP_DRONE_DEF_NET_GW);
 

        ADDLOGF_INFO("no flash configuration, use default\r\n");
        mac = (unsigned char*)&ap_info.bssid.array;
		    // this is MAC for Access Point, it's different than Client one
		    // see wifi_get_mac_address source
        wifi_get_mac_address((char *)mac, CONFIG_ROLE_AP);
        ap_info.chann = APP_DRONE_DEF_CHANNEL;
        ap_info.cipher_suite = 0;
        //os_memcpy(ap_info.ssid.array, APP_DRONE_DEF_SSID, os_strlen(APP_DRONE_DEF_SSID));
        os_memcpy(ap_info.ssid.array, CFG_GetDeviceName(), os_strlen(CFG_GetDeviceName()));
		
        ap_info.key_len = 0;
        os_memset(&ap_info.key, 0, 65);   
  

    bk_wlan_ap_set_default_channel(ap_info.chann);

    len = os_strlen((char *)ap_info.ssid.array);

    os_strncpy((char *)wNetConfig.wifi_ssid, (char *)ap_info.ssid.array, sizeof(wNetConfig.wifi_ssid));
    os_strncpy((char *)wNetConfig.wifi_key, (char *)ap_info.key, sizeof(wNetConfig.wifi_key));
    
    wNetConfig.wifi_mode = SOFT_AP;
    wNetConfig.dhcp_mode = DHCP_SERVER;
    wNetConfig.wifi_retry_interval = 100;
    
	if(1) {
		ADDLOGF_INFO("set ip info: %s,%s,%s\r\n",
				wNetConfig.local_ip_addr,
				wNetConfig.net_mask,
				wNetConfig.dns_server_ip_addr);
	}
    
	if(1) {
	  ADDLOGF_INFO("ssid:%s  key:%s mode:%d\r\n", wNetConfig.wifi_ssid, wNetConfig.wifi_key, wNetConfig.wifi_mode);
	}
	bk_wlan_start(&wNetConfig);

  return 0;    
}


int Main_IsConnectedToWiFi() {
	return g_bHasWiFiConnected;
}

void user_main(void)
{
    OSStatus err;
	int bForceOpenAP = 0;
  int bootFailures = 0;
	const char *wifi_ssid, *wifi_pass;

  // read or initialise the boot count flash area
  increment_boot_count();

  bootFailures = boot_failures();
  if (bootFailures > 3){
    bForceOpenAP = 1;
    ADDLOGF_INFO("###### force AP mode - boot failures %d", bootFailures);
  }
  if (bootFailures > 4){
    bSafeMode = 1;
		ADDLOGF_INFO("###### safe mode activated - boot failures %d", bootFailures);
  }

	CFG_InitAndLoad();
	wifi_ssid = CFG_GetWiFiSSID();
	wifi_pass = CFG_GetWiFiPass();

#if 0
	// you can use this if you bricked your module by setting wrong access point data
	wifi_ssid = "qqqqqqqqqq";
	wifi_pass = "Fqqqqqqqqqqqqqqqqqqqqqqqqqqq"
#endif
#ifdef SPECIAL_UNBRICK_ALWAYS_OPEN_AP
	// you can use this if you bricked your module by setting wrong access point data
	bForceOpenAP = 1;
#endif

	if(*wifi_ssid == 0 || *wifi_pass == 0 || bForceOpenAP) {
		// start AP mode in 5 seconds
		g_openAP = 5;
		//setup_wifi_open_access_point();
	} else {
		g_connectToWiFi = 5;
	}

	ADDLOGF_INFO("Using SSID [%s]\r\n",wifi_ssid);
	ADDLOGF_INFO("Using Pass [%s]\r\n",wifi_pass);

	// NOT WORKING, I done it other way, see ethernetif.c
	//net_dhcp_hostname_set(g_shortDeviceName);

	start_tcp_http();
	ADDLOGF_DEBUG("Started http tcp server\r\n");

  // only initialise certain things if we are not in AP mode
  if (!bSafeMode){
    g_enable_pins = 1;
    // this actually sets the pins, moved out so we could avoid if necessary
    PIN_SetupPins();

    DRV_Generic_Init();
    RepeatingEvents_Init();

    PIN_Init();
    ADDLOGF_DEBUG("Initialised pins\r\n");

    // initialise MQTT - just sets up variables.
    // all MQTT happens in timer thread?
    MQTT_init();

    PIN_SetGenericDoubleClickCallback(app_on_generic_dbl_click);
    ADDLOGF_DEBUG("Initialised other callbacks\r\n");

#ifdef BK_LITTLEFS
    // initialise the filesystem, only if present.
    // don't create if it does not mount
    // do this for ST mode only, as it may be something in FS which is killing us,
    // and we may add a command to empty fs just be writing first sector?
    init_lfs(0);
#endif

    // initialise rest interface
    init_rest();

    // add some commands...
    taslike_commands_init();
    fortest_commands_init();
    NewLED_InitCommands();

    // NOTE: this will try to read autoexec.bat,
    // so ALL commands expected in autoexec.bat should have been registered by now...
    // but DON't run autoexec if we have had 2+ boot failures
    CMD_Init();

    if (bootFailures < 2){
      CMD_ExecuteCommand("exec autoexec.bat");
    }
  }

  err = rtos_init_timer(&g_main_timer_1s,
                        1 * 1000,
                        app_led_timer_handler,
                        (void *)0);
  ASSERT(kNoErr == err);

  err = rtos_start_timer(&g_main_timer_1s);
  ASSERT(kNoErr == err);
	ADDLOGF_DEBUG("started timer\r\n");
}

#if PLATFORM_BK7231N

// right now Free is somewhere else

#else

#undef Free
// This is needed by tuya_hal_wifi_release_ap.
// How come that the Malloc was not undefined, but Free is?
// That's because Free is defined to os_free. It would be better to fix it elsewhere
void Free(void* ptr)
{
    os_free(ptr);
}


#endif
