#include "../hal_wifi.h"
#include "../../new_common.h"
#include "../../logging/logging.h"

//uint8_t HAL_AP_Wifi_Channel = 1;	// use channel 1 as default
// let's use (existing) g_wifi_channel - we can only be AP or STA, so we can share it ...
#ifndef AP_STA_CLIENTS
#define AP_STA_CLIENTS 3
#endif
const char* __attribute__((weak)) HAL_GetMyIPString()
{
	return "error";
}

const char* __attribute__((weak)) HAL_GetMyGatewayString()
{
	return "error";
}

const char* __attribute__((weak)) HAL_GetMyDNSString()
{
	return "error";
}

const char* __attribute__((weak)) HAL_GetMyMaskString()
{
	return "error";;
}

int __attribute__((weak)) WiFI_SetMacAddress(char* mac)
{

	return 0; // error
}

void __attribute__((weak)) WiFI_GetMacAddress(char* mac)
{

}
char* __attribute__((weak)) HAL_GetWiFiBSSID(char* bssid) {
	strcpy(bssid, "30:B5:C2:5D:70:72");
	return bssid;
};
uint8_t __attribute__((weak)) HAL_GetWiFiChannel(uint8_t *chan) {
	*chan = 12;
	return *chan;
};
const char* __attribute__((weak)) HAL_GetMACStr(char* macstr)
{
	return "error";
}

void __attribute__((weak)) HAL_PrintNetworkInfo()
{

}

int __attribute__((weak)) HAL_GetWifiStrength()
{
	return 0;
}

void __attribute__((weak)) HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{

}

void __attribute__((weak)) HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	ADDLOG_ERROR(LOG_FEATURE_GENERAL, "Generic %s called", __func__);
}

void __attribute__((weak)) HAL_FastConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	ADDLOG_ERROR(LOG_FEATURE_GENERAL, "Enhanced fast connect is not implemented on "PLATFORM_MCU_NAME".");
	HAL_ConnectToWiFi(oob_ssid, connect_key, ip);
}

void __attribute__((weak)) HAL_DisableEnhancedFastConnect()
{

}

void __attribute__((weak)) HAL_DisconnectFromWifi()
{

}

int __attribute__((weak)) HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	ADDLOG_ERROR(LOG_FEATURE_GENERAL, "Generic %s called", __func__);
	return 0;
}

int __attribute__((weak)) HAL_SetupWiFiAccessPoint(const char* ssid, const char* key)
{
	ADDLOG_ERROR(LOG_FEATURE_GENERAL, "Generic %s called", __func__);
	return 0;
}
