#include "../hal_wifi.h"

#define LOG_FEATURE LOG_FEATURE_MAIN

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "lwip/netdb.h"

static void (*g_wifiStatusCallback)(int code);


#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x "

static char g_IP[3+3+3+3+1+1+1] = "unknown";
static int g_bOpenAccessPointMode = 0;

// This must return correct IP for both SOFT_AP and STATION modes,
// because, for example, javascript control panel requires it
const char *HAL_GetMyIPString(){

    return g_IP;
}

////////////////////
// NOTE: this gets the STA mac
void getMAC(unsigned char *mac){
    net_get_if_macaddr(mac, net_get_sta_handle());
}
int WiFI_SetMacAddress(char *mac) {

	return 0;
}

void WiFI_GetMacAddress(char *mac) {

}
const char *HAL_GetMACStr(char *macstr){
    unsigned char mac[6] = { 0, 1, 2, 3, 4, 5 };

    sprintf(macstr, MACSTR, MAC2STR(mac));
    return macstr;
}

void HAL_PrintNetworkInfo(){

}


void HAL_WiFi_SetupStatusCallback(void (*cb)(int code)) {
	g_wifiStatusCallback = cb;

}
void HAL_ConnectToWiFi(const char *oob_ssid,const char *connect_key)
{
	g_bOpenAccessPointMode = 0;

}


int HAL_SetupWiFiOpenAccessPoint(const char *ssid)
{


	//dhcp_server_start(0);
	//dhcp_server_stop(void);

  return 0;
}

