#ifdef PLATFORM_XR809

#include "../hal_wifi.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"

#include "../../logging/logging.h"

#define LOG_FEATURE LOG_FEATURE_MAIN


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#include "common/framework/platform_init.h"
#include "common/framework/sysinfo.h"
#include "common/framework/net_ctrl.h"
#include "serial.h"
#include "kernel/os/os.h"

#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"

static void (*g_wifiStatusCallback)(int code);

// lenght of "192.168.103.103" is 15 but we also need a NULL terminating character
static char g_ipStr[32] = "unknown";

void HAL_ConnectToWiFi(const char *ssid, const char *psk, obkStaticIP_t *ip)
{
	wlan_ap_disable();
	net_switch_mode(WLAN_MODE_STA);

	/* set ssid and password to wlan */
	wlan_sta_set((uint8_t *)ssid, strlen(ssid), (uint8_t *)psk);

	/* start scan and connect to ap automatically */
	wlan_sta_enable();

	//OS_Sleep(60);
	printf("ok set wifii\n\r");
}

void HAL_DisconnectFromWifi()
{

}

int HAL_SetupWiFiOpenAccessPoint(const char *ssid) {
	char ap_psk[8] = { 0 };

	net_switch_mode(WLAN_MODE_HOSTAP);
	wlan_ap_disable();
	wlan_ap_set((uint8_t *)ssid, strlen(ssid), (uint8_t *)ap_psk);
	wlan_ap_enable();

	return 0;
}
static void wlan_msg_recv(uint32_t event, uint32_t data, void *arg)
{
	uint16_t type = EVENT_SUBTYPE(event);
	printf("%s msg type:%d\n", __func__, type);

	switch (type) {
	case NET_CTRL_MSG_WLAN_CONNECTED:
			if(g_wifiStatusCallback!=0) {
				g_wifiStatusCallback(WIFI_STA_CONNECTED);
			}
		break;
	case NET_CTRL_MSG_WLAN_DISCONNECTED:
			if(g_wifiStatusCallback!=0) {
				g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
			}
		break;
	case NET_CTRL_MSG_WLAN_SCAN_SUCCESS:
	case NET_CTRL_MSG_WLAN_SCAN_FAILED:
		break;
	case NET_CTRL_MSG_WLAN_4WAY_HANDSHAKE_FAILED:
	case NET_CTRL_MSG_WLAN_CONNECT_FAILED:
			if(g_wifiStatusCallback!=0) {
				g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
			}
		break;
	case NET_CTRL_MSG_CONNECTION_LOSS:
			if(g_wifiStatusCallback!=0) {
				g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
			}
		break;
	case NET_CTRL_MSG_NETWORK_UP:

		break;
	case NET_CTRL_MSG_NETWORK_DOWN:

		break;
#if (!defined(__CONFIG_LWIP_V1) && LWIP_IPV6)
	case NET_CTRL_MSG_NETWORK_IPV6_STATE:
		break;
#endif
	default:
		printf("unknown msg (%u, %u)\n", type, data);
		break;
	}
}
void HAL_WiFi_SetupStatusCallback(void (*cb)(int code)) {
	g_wifiStatusCallback = cb;

	observer_base *ob = sys_callback_observer_create(CTRL_MSG_TYPE_NETWORK,
	                                                 NET_CTRL_MSG_ALL,
	                                                 wlan_msg_recv,
	                                                 NULL);
	if (ob == NULL) {
		// error

		return;
	}
	if (sys_ctrl_attach(ob) != 0) {
		// error

		return;
	}
	// ok
	return;
}

int WiFI_SetMacAddress(char *mac) {
	// will be done after restarting
//	wlan_set_mac_addr(mac, 6);
	CFG_SetMac(mac);
	return 1;

}
void WiFI_GetMacAddress(char *mac) {
	memcpy(mac,g_cfg.mac,6);
}

void HAL_PrintNetworkInfo() {
/*
from LN882H

    struct netif *nif = get_connected_nif();
    if (nif != NULL) {
        netif_idx_t active = netdev_get_active();
        uint8_t * mac = nif->hwaddr;
        LOG(LOG_LVL_INFO, "+--------------- net device info ------------+\r\n");
        LOG(LOG_LVL_INFO, "|netif type    : %-16s            |\r\n", active == NETIF_IDX_STA ? "STA" : "AP");
        LOG(LOG_LVL_INFO, "|netif hostname: %-16s            |\r\n", nif->hostname);
        LOG(LOG_LVL_INFO, "|netif ip      = %-16s            |\r\n", ip4addr_ntoa(&nif->ip_addr));
        LOG(LOG_LVL_INFO, "|netif mask    = %-16s            |\r\n", ip4addr_ntoa(&nif->netmask));
        LOG(LOG_LVL_INFO, "|netif gateway = %-16s            |\r\n", ip4addr_ntoa(&nif->gw));
        LOG(LOG_LVL_INFO, "|netif mac     : [%02X:%02X:%02X:%02X:%02X:%02X] %-7s |\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], "");
        LOG(LOG_LVL_INFO, "+--------------------------------------------+\r\n");
    }


*/
	if (g_wlan_netif != NULL){
//		unsigned char mac[32];
//		WiFI_GetMacAddress((char *)mac);
		uint8_t * mac = g_wlan_netif->hwaddr;
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "+--------------- net device info ------------+\r\n");
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "|netif type    : %-16s            |\r\n", wlan_if_get_mode(g_wlan_netif) == WLAN_MODE_STA ? "STA" : "AP");
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "|netif hostname: %-16s            |\r\n", g_wlan_netif->hostname);
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "|netif ip      = %-16s            |\r\n", inet_ntoa(g_wlan_netif->ip_addr));
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "|netif mask    = %-16s            |\r\n", inet_ntoa(g_wlan_netif->netmask));
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "|netif gateway = %-16s            |\r\n", inet_ntoa(g_wlan_netif->gw));
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "|netif mac     : [%02X:%02X:%02X:%02X:%02X:%02X] %-7s |\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], "");
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "+--------------------------------------------+\r\n");

	}	

}


/*
from cmd_wlan of xr809.c

static __inline void cmd_wlan_sta_print_ap(wlan_sta_ap_t *ap)
{
	CMD_LOG(1, "%02x:%02x:%02x:%02x:%02x:%02x  ssid=%-32.32s  "
		"beacon_int=%d  freq=%d  channel=%u  rssi=%d  level=%d  "
		"flags=%#010x  wpa_key_mgmt=%#010x  wpa_cipher=%#010x  "
		"wpa2_key_mgmt=%#010x  wpa2_cipher=%#010x\n",
		ap->bssid[0], ap->bssid[1],
		ap->bssid[2], ap->bssid[3],
		ap->bssid[4], ap->bssid[5],
		ap->ssid.ssid,
		ap->beacon_int,
		ap->freq,
		ap->channel,
		ap->rssi,
		ap->level,
		ap->wpa_flags,
		ap->wpa_key_mgmt,
		ap->wpa_cipher,
		ap->wpa2_key_mgmt,
		ap->wpa2_cipher);
}addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, 

*/
int HAL_GetWifiStrength() {
	wlan_sta_ap_t *ap = malloc(sizeof(wlan_sta_ap_t));	// to hold information of connected AP	
	if (wlan_sta_ap_info(ap)){
		uint8_t rssi=ap->rssi;
		free(ap);
		return rssi;	
	}
	return; 
}

// Get WiFi Information (SSID / BSSID) - e.g. to display on status page 
/*
// ATM there is only one SSID, so need for this code
char* HAL_GetWiFiSSID(char* ssid){
	ssid[0]='\0';
	wlan_sta_ap_t *ap = malloc(sizeof(wlan_sta_ap_t));	// to hold information of connected AP	
	if (wlan_sta_ap_info(ap)){
		memcpy(ssid, ap->ssid.ssid, ap->ssid.ssid_len);
		ssid[ap->ssid.ssid_len]='\0';
		free(ap);
	}
	return ssid; 
};

*/
char* HAL_GetWiFiBSSID(char* bssid){
	bssid[0]='\0';
	wlan_sta_ap_t *ap = malloc(sizeof(wlan_sta_ap_t));	// to hold information of connected AP	
	if (wlan_sta_ap_info(ap)){
		sprintf(bssid, MACSTR, MAC2STR(ap->bssid));
		free(ap);
	}
	return bssid;
};
uint8_t HAL_GetWiFiChannel(uint8_t *chan){
	wlan_sta_ap_t *ap = malloc(sizeof(wlan_sta_ap_t));	// to hold information of connected AP	
	if (wlan_sta_ap_info(ap)){
		*chan = ap->channel ;
		free(ap);
		return *chan;
	}
	return 0; 
};



const char *HAL_GetMyIPString() {
	strcpy(g_ipStr,inet_ntoa(g_wlan_netif->ip_addr));

	return g_ipStr;
}
const char* HAL_GetMyGatewayString() {
	return "192.168.0.1";
}
const char* HAL_GetMyDNSString() {
	return "192.168.0.1";
}
const char* HAL_GetMyMaskString() {
	return "255.255.255.0";
}

const char *HAL_GetMACStr(char *macstr) {
	unsigned char mac[32];
	WiFI_GetMacAddress((char *)mac);
	sprintf(macstr,"%02X%02X%02X%02X%02X%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	return macstr;
}

#endif // PLATFORM_XR809
