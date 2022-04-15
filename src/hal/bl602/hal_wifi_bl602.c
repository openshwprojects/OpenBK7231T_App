#ifdef PLATFORM_BL602

#include "../hal_wifi.h"
#include "../../new_common.h"

#include <string.h>
#include <cli.h>

#include <FreeRTOS.h>
#include <task.h>
#include <portable.h>
#include <semphr.h>
#include <hal_sys.h>
#include <bl_wifi.h>
#include <bl60x_fw_api.h>
#include <wifi_mgmr_ext.h>

char g_ipStr[64];

void HAL_ConnectToWiFi(const char *ssid, const char *psk)
{
	
}
int HAL_SetupWiFiOpenAccessPoint(const char *ssid) {
	
    uint8_t hidden_ssid = 0;
    int channel;
    wifi_interface_t wifi_interface;

    wifi_interface = wifi_mgmr_ap_enable();
    /*no password when only one param*/
    wifi_mgmr_ap_start(wifi_interface, ssid, hidden_ssid, NULL, 1);

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