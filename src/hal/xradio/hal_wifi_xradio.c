#if PLATFORM_XRADIO

#include "../hal_wifi.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../../logging/logging.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"
#include "net/wlan/wlan_ext_req.h"
#include "common/framework/platform_init.h"
#include "common/framework/sysinfo.h"
#include "common/framework/net_ctrl.h"
//#include "serial.h"
#include "kernel/os/os.h"

#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

static void(*g_wifiStatusCallback)(int code);

bool g_bOpenAccessPointMode = false;

void HAL_ConnectToWiFi(const char *ssid, const char *psk, obkStaticIP_t *ip)
{
	wlan_ap_disable();
	net_switch_mode(WLAN_MODE_STA);

	/* set ssid and password to wlan */
	wlan_sta_set((uint8_t *)ssid, strlen(ssid), (uint8_t *)psk);

	/* start scan and connect to ap automatically */
	wlan_sta_enable();

	netif_set_hostname(g_wlan_netif, CFG_GetDeviceName());
	if (ip->localIPAddr[0] != 0)
	{
#if !__CONFIG_LWIP_V1
		ip_addr_t ipaddr;
		ip_addr_t netmask;
		ip_addr_t gw;
		ip_addr_t dnsserver;
		IP4_ADDR(ip_2_ip4(&ipaddr), ip->localIPAddr[0], ip->localIPAddr[1], ip->localIPAddr[2], ip->localIPAddr[3]);
		IP4_ADDR(ip_2_ip4(&netmask), ip->netMask[0], ip->netMask[1], ip->netMask[2], ip->netMask[3]);
		IP4_ADDR(ip_2_ip4(&gw), ip->gatewayIPAddr[0], ip->gatewayIPAddr[1], ip->gatewayIPAddr[2], ip->gatewayIPAddr[3]);
		IP4_ADDR(ip_2_ip4(&dnsserver), ip->dnsServerIpAddr[0], ip->dnsServerIpAddr[1], ip->dnsServerIpAddr[2], ip->dnsServerIpAddr[3]);
		netif_set_addr(g_wlan_netif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
		dns_setserver(0, &dnsserver);
#endif
	}
}

void HAL_DisconnectFromWifi()
{
	wlan_sta_disconnect();
}

int HAL_SetupWiFiOpenAccessPoint(const char *ssid) {
	char ap_psk[8] = { 0 };
	g_bOpenAccessPointMode = true;
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
		if (g_wifiStatusCallback != 0) {
			g_wifiStatusCallback(WIFI_STA_CONNECTED);
		}
		break;
	case NET_CTRL_MSG_WLAN_DISCONNECTED:
		if (g_wifiStatusCallback != 0) {
			g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
		}
		break;
	case NET_CTRL_MSG_WLAN_SCAN_SUCCESS:
	case NET_CTRL_MSG_WLAN_SCAN_FAILED:
		break;

#if !PLATFORM_XR872
	case NET_CTRL_MSG_CONNECTION_LOSS:
#endif
	case NET_CTRL_MSG_WLAN_4WAY_HANDSHAKE_FAILED:
	case NET_CTRL_MSG_WLAN_CONNECT_FAILED:
		if (g_wifiStatusCallback != 0) {
			g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
		}
		break;
	case NET_CTRL_MSG_NETWORK_UP:
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
void HAL_WiFi_SetupStatusCallback(void(*cb)(int code)) {
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
	memcpy(mac, g_cfg.mac, 6);
}

void HAL_PrintNetworkInfo()
{
	uint8_t mac[6];
	WiFI_GetMacAddress((char*)mac);
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "+--------------- net device info ------------+\r\n");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif type    : %-16s            |\r\n", g_bOpenAccessPointMode == 0 ? "STA" : "AP");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif rssi    = %-16i            |\r\n", HAL_GetWifiStrength());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif ip      = %-16s            |\r\n", HAL_GetMyIPString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif mask    = %-16s            |\r\n", HAL_GetMyMaskString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif gateway = %-16s            |\r\n", HAL_GetMyGatewayString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif mac     : ["MACSTR"] %-6s  |\r\n", MAC2STR(mac), "");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "+--------------------------------------------+\r\n");
}

int HAL_GetWifiStrength()
{
	//wlan_sta_ap_info(&apinfo);
	//return apinfo.level;
	wlan_ext_signal_t signal;
	wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_SIGNAL, (int)(&signal));
	return signal.rssi / 2 + signal.noise;
}

char* HAL_GetWiFiBSSID(char* bssid) {
	wlan_sta_ap_t *ap = malloc(sizeof(wlan_sta_ap_t));	// to hold information of connected AP	
	if (!wlan_sta_ap_info(ap)) {
		sprintf(bssid, MACSTR, MAC2STR(ap->bssid));
	}
	else {
		strcpy(bssid, "wlan info failed");//must be less than 32 chars
	}
	free(ap);
	return bssid;
};
uint8_t HAL_GetWiFiChannel(uint8_t *chan) {
	wlan_sta_ap_t *ap = malloc(sizeof(wlan_sta_ap_t));	// to hold information of connected AP	
	if (!wlan_sta_ap_info(ap)) {
		*chan = ap->channel;
	}
	else {
		*chan = 0;
	}
	free(ap);
	return *chan;
};

const char *HAL_GetMyIPString()
{
	return ipaddr_ntoa(&g_wlan_netif->ip_addr);
}

const char* HAL_GetMyGatewayString()
{
	return ipaddr_ntoa(&g_wlan_netif->gw);
}

const char* HAL_GetMyDNSString()
{
#if !__CONFIG_LWIP_V1
	return ipaddr_ntoa(dns_getserver(0));
#else
	return "unknown";
#endif
}

const char* HAL_GetMyMaskString()
{
	return ipaddr_ntoa(&g_wlan_netif->netmask);
}

const char *HAL_GetMACStr(char *macstr)
{
	unsigned char mac[32];
	WiFI_GetMacAddress((char *)mac);
	sprintf(macstr, MACSTR, MAC2STR(mac));
	return macstr;
}

#endif // PLATFORM_XR809