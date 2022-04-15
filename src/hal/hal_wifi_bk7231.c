#include "hal_wifi.h"

#define LOG_FEATURE LOG_FEATURE_MAIN

#include "mem_pub.h"
#include "str_pub.h"
#include "wlan_ui_pub.h"
#include "ethernet_intf.h"
#include "../logging/logging.h"
#include "../../beken378/app/config/param_config.h"
#include "lwip/netdb.h"

void (*g_wifiStatusCallback)(int code);


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
			if(g_wifiStatusCallback!=0) {
				g_wifiStatusCallback(WIFI_STA_CONNECTING);
			}
            break;
        case RW_EVT_STA_BEACON_LOSE:
        case RW_EVT_STA_PASSWORD_WRONG:
        case RW_EVT_STA_NO_AP_FOUND:
        case RW_EVT_STA_ASSOC_FULL:
        case RW_EVT_STA_DISCONNECTED:    /* disconnect with server */
			if(g_wifiStatusCallback!=0) {
				g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
			}
            break;
        case RW_EVT_STA_CONNECT_FAILED:  /* authentication failed */
			if(g_wifiStatusCallback!=0) {
				g_wifiStatusCallback(WIFI_STA_AUTH_FAILED);
			}
            break;
        case RW_EVT_STA_CONNECTED:        /* authentication success */    
        case RW_EVT_STA_GOT_IP: 
			if(g_wifiStatusCallback!=0) {
				g_wifiStatusCallback(WIFI_STA_CONNECTED);
			}
            break;
        
        /* for softap mode */
        case RW_EVT_AP_CONNECTED:          /* a client association success */
			if(g_wifiStatusCallback!=0) {
				g_wifiStatusCallback(WIFI_AP_CONNECTED);
			}
            break;
        case RW_EVT_AP_DISCONNECTED:       /* a client disconnect */
        case RW_EVT_AP_CONNECT_FAILED:     /* a client association failed */
			if(g_wifiStatusCallback!=0) {
				g_wifiStatusCallback(WIFI_AP_FAILED);
			}
            break;
        default:
            break;
    }

}


// from wlan_ui.c, no header
void bk_wlan_status_register_cb(FUNC_1PARAM_PTR cb);


void HAL_WiFi_SetupStatusCallback(void (*cb)(int code)) {
	bk_wlan_status_register_cb(wl_status);
}
void HAL_ConnectToWiFi(const char *oob_ssid,const char *connect_key)
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


int HAL_SetupWiFiOpenAccessPoint(const char *ssid)
{
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
        os_memcpy(ap_info.ssid.array, ssid, os_strlen(ssid));
		
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

