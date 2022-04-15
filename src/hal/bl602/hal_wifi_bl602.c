#ifdef PLATFORM_BL602

#include "../hal_wifi.h"

char g_ipStr[64];

void HAL_ConnectToWiFi(const char *ssid, const char *psk)
{
	
}
int HAL_SetupWiFiOpenAccessPoint(const char *ssid) {
	

	return 0;
}
void HAL_WiFi_SetupStatusCallback(void (*cb)(int code)) {

}


void HAL_PrintNetworkInfo() {

}
const char *HAL_GetMyIPString() {

	return g_ipStr;
}
const char *HAL_GetMACStr(char *macstr) {

	return macstr;
}

#endif // PLATFORM_BL602