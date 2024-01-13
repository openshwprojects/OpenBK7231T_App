#ifdef PLATFORM_LN882H

#include "../hal_wifi.h"
#include "wifi.h"
#include "wifi_port.h"
#include "netif/ethernetif.h"
#include "wifi_manager.h"
#include "lwip/tcpip.h"
#include "utils/system_parameter.h"
#include "ln_wifi_err.h"

static void (*g_wifiStatusCallback)(int code);

// lenght of "192.168.103.103" is 15 but we also need a NULL terminating character
static char g_IP[32] = "unknown";
static int g_bOpenAccessPointMode = 0;
static uint8_t mac_addr[6]        = {0x00, 0x50, 0xC2, 0x5E, 0x88, 0x99};

// This must return correct IP for both SOFT_AP and STATION modes,
// because, for example, javascript control panel requires it
const char* HAL_GetMyIPString() {


	return g_IP;
}
const char* HAL_GetMyGatewayString() {
	return g_IP;
}
const char* HAL_GetMyDNSString() {
	return g_IP;
}
const char* HAL_GetMyMaskString() {
	return g_IP;
}


int WiFI_SetMacAddress(char* mac)
{
	return 0; // error
}

void WiFI_GetMacAddress(char* mac)
{
	sysparam_sta_mac_get((unsigned char*)mac);
}

const char* HAL_GetMACStr(char* macstr)
{
	unsigned char mac[6];
	sysparam_sta_mac_get((unsigned char*)mac);
	sprintf(macstr, MACSTR, MAC2STR(mac));
	return macstr;
}


void HAL_PrintNetworkInfo()
{


}

int HAL_GetWifiStrength()
{
	return 0;
}


void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;

}
void HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t *ip)
{
	g_bOpenAccessPointMode = 0;

	
}

void HAL_DisconnectFromWifi()
{
}


static void ap_startup_cb(void * arg)
{
    netdev_set_state(NETIF_IDX_AP, NETDEV_UP);
}

void wifi_init_ap(const char* ssid)
{
    tcpip_ip_info_t  ip_info;
    server_config_t  server_config;
	wifi_softap_cfg_t ap_cfg = {
		.ssid            = ssid,
		.pwd             = "",
		.bssid           = mac_addr,
		.ext_cfg = {
			.channel         = 6,
			.authmode        = WIFI_AUTH_OPEN,
			.ssid_hidden     = 0,
			.beacon_interval = 100,
			.psk_value = NULL,
		}
	};

    ip_info.ip.addr      = ipaddr_addr((const char *)"192.168.4.1");
    ip_info.gw.addr      = ipaddr_addr((const char *)"192.168.4.1");
    ip_info.netmask.addr = ipaddr_addr((const char *)"255.255.255.0");

    server_config.server.addr   = ip_info.ip.addr;
    server_config.port          = 67;
    server_config.lease         = 2880;
    server_config.renew         = 2880;
    server_config.ip_start.addr = ipaddr_addr((const char *)"192.168.4.100");
    server_config.ip_end.addr   = ipaddr_addr((const char *)"192.168.4.150");
    server_config.client_max    = 3;
    dhcpd_curr_config_set(&server_config);

    //1. net device(lwip).
    netdev_set_mac_addr(NETIF_IDX_AP, mac_addr);
    netdev_set_ip_info(NETIF_IDX_AP, &ip_info);
    netdev_set_active(NETIF_IDX_AP);
    wifi_manager_reg_event_callback(WIFI_MGR_EVENT_SOFTAP_STARTUP, &ap_startup_cb);

    sysparam_softap_mac_update((const uint8_t *)mac_addr);

    ap_cfg.ext_cfg.psk_value = NULL;

    //2. wifi
    if(WIFI_ERR_NONE !=  wifi_softap_start(&ap_cfg)){
        LOG(LOG_LVL_ERROR, "[%s, %d]wifi_start() fail.\r\n", __func__, __LINE__);
    }
}

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	LOG(LOG_LVL_INFO, "Starting AP: %s\r\n", ssid);
	wifi_init_ap(ssid);

	LOG(LOG_LVL_INFO, "AP started, waiting for: netdev_got_ip()\r\n");
    while (!netdev_got_ip()) {
        OS_MsDelay(1000);
    }

	LOG(LOG_LVL_INFO, "AP started OK!\r\n");
	g_bOpenAccessPointMode = 1;

	return 0;
}

#endif // PLATFORM_LN882H