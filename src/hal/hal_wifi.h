


enum HALWifiStatus {
	WIFI_STA_CONNECTING,
	WIFI_STA_DISCONNECTED,
	WIFI_STA_AUTH_FAILED,
	WIFI_STA_CONNECTED,
	WIFI_AP_CONNECTED,
	WIFI_AP_FAILED,

};


int HAL_SetupWiFiOpenAccessPoint(const char *ssid);
void HAL_ConnectToWiFi(const char *oob_ssid,const char *connect_key);
void HAL_WiFi_SetupStatusCallback(void (*cb)(int code));
// This must return correct IP for both SOFT_AP and STATION modes,
// because, for example, javascript control panel requires it
const char *HAL_GetMyIPString();
const char *HAL_GetMACStr(char *macstr);
void WiFI_GetMacAddress(char *mac);
int WiFI_SetMacAddress(char *mac);
void HAL_PrintNetworkInfo();

