#ifdef PLATFORM_XR809

#include "../hal_wifi.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"


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

static char g_ipStr[32];

void HAL_ConnectToWiFi(const char *ssid, const char *psk)
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

void WiFI_GetMacAddress(char *mac) {
	memcpy(mac,g_cfg.mac,6);
}

void HAL_PrintNetworkInfo() {

}
const char *HAL_GetMyIPString() {
	strcpy(g_ipStr,inet_ntoa(g_wlan_netif->ip_addr));

	return g_ipStr;
}
const char *HAL_GetMACStr(char *macstr) {
	unsigned char mac[32];
	WiFI_GetMacAddress((char *)mac);
	sprintf(macstr,"%02X%02X%02X%02X%02X%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	return macstr;
}

#endif // PLATFORM_XR809