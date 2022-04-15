#ifdef PLATFORM_XR809

#include "hal_wifi.h"

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
	char ap_psk[] = "12345678";

	net_switch_mode(WLAN_MODE_HOSTAP);
	wlan_ap_disable();
	wlan_ap_set((uint8_t *)ssid, strlen(ssid), (uint8_t *)ap_psk);
	wlan_ap_enable();

	return 0;
}
void HAL_WiFi_SetupStatusCallback(void (*cb)(int code)) {

}

#endif // PLATFORM_XR809