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

void (*g_wifiStatusCallback)(int code);

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
void HAL_WiFi_SetupStatusCallback(void (*cb)(int code)) {
	g_wifiStatusCallback = cb;

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