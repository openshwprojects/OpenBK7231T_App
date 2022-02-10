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

#define _TUYA_DEVICE_GLOBAL

/* Includes ------------------------------------------------------------------*/
#include "uni_log.h"
#include "tuya_iot_wifi_api.h"
#include "tuya_hal_system.h"
#include "tuya_iot_com_api.h"
#include "tuya_cloud_com_defs.h"
#include "gw_intf.h"
#include "gpio_test.h"
#include "tuya_gpio.h"
#include "tuya_key.h"
#include "tuya_led.h"
#include "wlan_ui_pub.h"

#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"

#include "mem_pub.h"
#include "str_pub.h"
#include "ethernet_intf.h"

/* Private includes ----------------------------------------------------------*/
#include "tuya_device.h"
#include "httpserver/new_http.h"
#include "new_pins.h"
#include "new_cfg.h"
#include "logging/logging.h"
#include "httpserver/http_tcp_server.h"
#include "httpserver/rest_interface.h"
#include "printnetinfo/printnetinfo.h"
#include "mqtt/new_mqtt.h"

#include "../../beken378/func/key/multi_button.h"
#include "../../beken378/app/config/param_config.h"
#include "lwip/netdb.h"



#undef Malloc
#undef Free
#define Malloc os_malloc
#define Free os_free


static int g_secondsElapsed = 0;

static int g_openAP = 0;

int Time_getUpTimeSeconds() {
	return g_secondsElapsed;
}

// from wlan_ui.c, no header
void bk_wlan_status_register_cb(FUNC_1PARAM_PTR cb);
static int setup_wifi_open_access_point(void);

int unw_recv(const int fd, void *buf, u32 nbytes)
{
    fd_set readfds, errfds;
    int ret = 0;   

    if( fd < 0 ) 
    {        
        return -1;//UNW_FAIL;
    } 

    FD_ZERO( &readfds );
    FD_ZERO( &errfds ); 
    FD_SET( fd, &readfds );
    FD_SET( fd, &errfds );

    ret = select( fd+1, &readfds, NULL, &errfds, NULL);
    ADDLOG_DEBUG(LOG_FEATURE_MAIN, "select ret:%d, %d, %d\r\n", ret, FD_ISSET( fd, &readfds ), FD_ISSET( fd, &errfds ));

    if(ret > 0 && FD_ISSET( fd, &readfds ))
        return recv(fd,buf,nbytes,0); 
    else
        return -1;//UNW_FAIL;
    
}



void connect_to_wifi(const char *oob_ssid,const char *connect_key)
{
#if 1
	network_InitTypeDef_adv_st	wNetConfigAdv;

	os_memset( &wNetConfigAdv, 0x0, sizeof(network_InitTypeDef_adv_st) );
	
	os_strcpy((char*)wNetConfigAdv.ap_info.ssid, oob_ssid);
	hwaddr_aton("48:ee:0c:48:93:12", (u8 *)wNetConfigAdv.ap_info.bssid);
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

    ADDLOG_INFO(LOG_FEATURE_MAIN, "ssid:%s key:%s\r\n", network_cfg.wifi_ssid, network_cfg.wifi_key);
			
    bk_wlan_start(&network_cfg);
#endif
}


beken_timer_t led_timer;

static void app_my_channel_toggle_callback(int channel, int iVal)
{
  ADDLOG_INFO(LOG_FEATURE_MAIN, "Channel has changed! Publishing change %i with %i \n",channel,iVal);
	example_publish(mqtt_client,channel,iVal);
}


int loopsWithDisconnected = 0;
static void app_led_timer_handler(void *data)
{
	if(mqtt_client != 0 && mqtt_client_is_connected(mqtt_client) == 0) {
		ADDLOG_INFO(LOG_FEATURE_MAIN, "Timer discovers disconnected mqtt %i\n",loopsWithDisconnected);
		loopsWithDisconnected++;
		if(loopsWithDisconnected > 10)
		{ 
			if(mqtt_client == 0)
			{
			    mqtt_client = mqtt_client_new();
			}
			example_do_connect(mqtt_client);
			loopsWithDisconnected = 0;
		}
	}

	g_secondsElapsed ++;
  ADDLOG_INFO(LOG_FEATURE_MAIN, "Timer is %i free mem %d\n", g_secondsElapsed, xPortGetFreeHeapSize());

  // print network info
  if (!(g_secondsElapsed % 10)){
    print_network_info();
  }


  if (g_openAP){
    g_openAP--;
    if (0 == g_openAP){
      setup_wifi_open_access_point();
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
    u8 *mac;
    
    os_memset(&general, 0, sizeof(general_param_t));
    os_memset(&ap_info, 0, sizeof(ap_param_t)); 
    os_memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_st));  
    
        general.role = 1,
        general.dhcp_enable = 1,

        os_strcpy((char *)wNetConfig.local_ip_addr, APP_DRONE_DEF_NET_IP);
        os_strcpy((char *)wNetConfig.net_mask, APP_DRONE_DEF_NET_MASK);
        os_strcpy((char *)wNetConfig.dns_server_ip_addr, APP_DRONE_DEF_NET_GW);
 

        ADDLOG_INFO(LOG_FEATURE_MAIN, "no flash configuration, use default\r\n");
        mac = (u8*)&ap_info.bssid.array;
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
		ADDLOG_INFO(LOG_FEATURE_MAIN, "set ip info: %s,%s,%s\r\n",
				wNetConfig.local_ip_addr,
				wNetConfig.net_mask,
				wNetConfig.dns_server_ip_addr);
	}
    
	if(1) {
	  ADDLOG_INFO(LOG_FEATURE_MAIN, "ssid:%s  key:%s mode:%d\r\n", wNetConfig.wifi_ssid, wNetConfig.wifi_key, wNetConfig.wifi_mode);
	}
	bk_wlan_start(&wNetConfig);

  return 0;    
}




// receives status change notifications about wireless - could be useful
// ctxt is pointer to a rw_evt_type
void wl_status( void *ctxt ){

    rw_evt_type stat = *((rw_evt_type*)ctxt);
    ADDLOG_INFO(LOG_FEATURE_MAIN, "wl_status %d\r\n", stat);

    switch(stat){
        case RW_EVT_STA_IDLE:
        case RW_EVT_STA_SCANNING:
        case RW_EVT_STA_SCAN_OVER:
        case RW_EVT_STA_CONNECTING:
            PIN_set_wifi_led(0);
            break;
        case RW_EVT_STA_BEACON_LOSE:
        case RW_EVT_STA_PASSWORD_WRONG:
        case RW_EVT_STA_NO_AP_FOUND:
        case RW_EVT_STA_ASSOC_FULL:
        case RW_EVT_STA_DISCONNECTED:    /* disconnect with server */
            // try to connect again in 5 seconds
            //reconnect = 5;
            PIN_set_wifi_led(0);
            break;
        case RW_EVT_STA_CONNECT_FAILED:  /* authentication failed */
            PIN_set_wifi_led(0);
            break;
        case RW_EVT_STA_CONNECTED:        /* authentication success */    
        case RW_EVT_STA_GOT_IP: 
            PIN_set_wifi_led(1);
            break;
        
        /* for softap mode */
        case RW_EVT_AP_CONNECTED:          /* a client association success */
            PIN_set_wifi_led(1);
            break;
        case RW_EVT_AP_DISCONNECTED:       /* a client disconnect */
        case RW_EVT_AP_CONNECT_FAILED:     /* a client association failed */
            PIN_set_wifi_led(0);
            break;
        default:
            break;
    }

}


void user_main(void)
//OPERATE_RET device_init(VOID)
{
    OSStatus err;
	int bForceOpenAP = 0;
	const char *wifi_ssid, *wifi_pass;

  //OPERATE_RET op_ret = OPRT_OK;

	CFG_CreateDeviceNameUnique();

	CFG_LoadWiFi();
	CFG_LoadMQTT();
	PIN_LoadFromFlash();

	wifi_ssid = CFG_GetWiFiSSID();
	wifi_pass = CFG_GetWiFiPass();

	ADDLOG_INFO(LOG_FEATURE_MAIN, "Using SSID [%s]\r\n",wifi_ssid);
	ADDLOG_INFO(LOG_FEATURE_MAIN, "Using Pass [%s]\r\n",wifi_pass);

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
		connect_to_wifi(wifi_ssid,wifi_pass);
		// register function to get callbacks about wifi changes.
		bk_wlan_status_register_cb(wl_status);
		ADDLOG_INFO(LOG_FEATURE_MAIN, "Registered for wifi changes\r\n");
	}

	// NOT WORKING, I done it other way, see ethernetif.c
	//net_dhcp_hostname_set(g_shortDeviceName);

	//demo_start_upd();
	start_tcp_http();
	ADDLOG_INFO(LOG_FEATURE_MAIN, "Started http tcp server\r\n");
	
	PIN_Init();
	ADDLOG_INFO(LOG_FEATURE_MAIN, "Initialised pins\r\n");


	PIN_SetGenericDoubleClickCallback(app_on_generic_dbl_click);
	CHANNEL_SetChangeCallback(app_my_channel_toggle_callback);
	ADDLOG_INFO(LOG_FEATURE_MAIN, "Initialised other callbacks\r\n");


    // initialise rest interface
    init_rest();

    err = rtos_init_timer(&led_timer,
                          1 * 1000,
                          app_led_timer_handler,
                          (void *)0);
    ASSERT(kNoErr == err);

    err = rtos_start_timer(&led_timer);
    ASSERT(kNoErr == err);
	ADDLOG_INFO(LOG_FEATURE_MAIN, "started timer\r\n");
}

#undef Free
// This is needed by tuya_hal_wifi_release_ap.
// How come that the Malloc was not undefined, but Free is?
// That's because Free is defined to os_free. It would be better to fix it elsewhere
void Free(void* ptr)
{
    os_free(ptr);
}

