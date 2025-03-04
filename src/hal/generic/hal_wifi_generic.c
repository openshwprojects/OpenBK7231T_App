#include "../hal_wifi.h"
#include "../../new_common.h"

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

}

void __attribute__((weak)) HAL_DisconnectFromWifi()
{

}

int __attribute__((weak)) HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	return 0;
}
