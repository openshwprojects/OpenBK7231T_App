#ifdef WINDOWS

#include "../hal_wifi.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>


static void (*g_wifiStatusCallback)(int code);

static char g_ipStr[32];

void HAL_ConnectToWiFi(const char *ssid, const char *psk, obkStaticIP_t *ip)
{
	if (g_wifiStatusCallback) {
		g_wifiStatusCallback(WIFI_STA_CONNECTED);
	}
	else {
		printf("Win32 simulator - not calling g_wifiStatusCallback because it's null\n");
	}
}

void HAL_DisconnectFromWifi()
{

}

int HAL_SetupWiFiOpenAccessPoint(const char *ssid) {


	return 0;
}

void HAL_WiFi_SetupStatusCallback(void (*cb)(int code)) {
	g_wifiStatusCallback = cb;
	// ok
	return;
}

int WiFI_SetMacAddress(char *mac) {
	// will be done after restarting
//	wlan_set_mac_addr(mac, 6);

	return 1;

}
void WiFI_GetMacAddress(char *mac) {
	mac[0] = 0xBA;
	mac[1] = 0xDA;
	mac[2] = 0x31;
	mac[3] = 0x45;
	mac[4] = 0xCA;
	mac[5] = 0xFF;
}

void HAL_PrintNetworkInfo() {

}
int HAL_GetWifiStrength() {
    return -1;
}

const char *HAL_GetMyIPString() {
	strcpy(g_ipStr,"127.0.0.1");
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
	unsigned char mac[32]= { 0 };
//	WiFI_GetMacAddress((char *)mac);
	sprintf(macstr,"%02X%02X%02X%02X%02X%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	return macstr;
}

#endif // WINDOWS
