#ifdef PLATFORM_LN882H

#include "../hal_wifi.h"


static void (*g_wifiStatusCallback)(int code);

// lenght of "192.168.103.103" is 15 but we also need a NULL terminating character
static char g_IP[32] = "unknown";
static int g_bOpenAccessPointMode = 0;

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
}

const char* HAL_GetMACStr(char* macstr)
{
	unsigned char mac[6];
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

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	return 0;
}

#endif // PLATFORM_LN882H