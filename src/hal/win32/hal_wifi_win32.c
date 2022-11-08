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

void HAL_ConnectToWiFi(const char *ssid, const char *psk)
{

}

void HAL_DisconnectFromWifi()
{

}

int HAL_SetupWiFiOpenAccessPoint(const char *ssid) {


	return 0;
}

void HAL_WiFi_SetupStatusCallback(void (*cb)(int code)) {
	
	// ok
	return;
}

int WiFI_SetMacAddress(char *mac) {
	// will be done after restarting
//	wlan_set_mac_addr(mac, 6);

	return 1;

}
void WiFI_GetMacAddress(char *mac) {

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
const char *HAL_GetMACStr(char *macstr) {
	unsigned char mac[32]= { 0 };
//	WiFI_GetMacAddress((char *)mac);
	sprintf(macstr,"%02X%02X%02X%02X%02X%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	return macstr;
}

#endif // WINDOWS
