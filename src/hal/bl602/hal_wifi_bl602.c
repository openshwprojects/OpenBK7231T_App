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
#include <aos/kernel.h>
#include <aos/yloop.h>

static char g_ipStr[64];
static int g_bAccessPointMode = 1;
static void (*g_wifiStatusCallback)(int code);

void HAL_ConnectToWiFi(const char *ssid, const char *psk)
{
    wifi_interface_t wifi_interface;

    wifi_interface = wifi_mgmr_sta_enable();
    wifi_mgmr_sta_connect(wifi_interface, ssid, psk, NULL, NULL, 0, 0);

	g_bAccessPointMode = 0;
}
int HAL_SetupWiFiOpenAccessPoint(const char *ssid) {
	
    uint8_t hidden_ssid = 0;
    int channel;
    wifi_interface_t wifi_interface;
	struct netif *net;
	
    wifi_interface = wifi_mgmr_ap_enable();
    /*no password when only one param*/
    wifi_mgmr_ap_start(wifi_interface, ssid, hidden_ssid, NULL, 1);

	g_bAccessPointMode = 1;

	return 0;
}
static void event_cb_wifi_event(input_event_t *event, void *private_data)
{


    switch (event->code) {
        case CODE_WIFI_ON_INIT_DONE:
        {
            printf("[APP] [EVT] INIT DONE %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_MGMR_DONE:
        {
            printf("[APP] [EVT] MGMR DONE %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_SCAN_DONE:
        {
            printf("[APP] [EVT] SCAN Done %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_DISCONNECT:
        {
            printf("[APP] [EVT] disconnect %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_CONNECTING:
        {
            printf("[APP] [EVT] Connecting %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_CMD_RECONNECT:
        {
            printf("[APP] [EVT] Reconnect %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_CONNECTED:
        {
            printf("[APP] [EVT] connected %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_PRE_GOT_IP:
        {
            printf("[APP] [EVT] connected %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_GOT_IP:
        {
            printf("[APP] [EVT] GOT IP %lld\r\n", aos_now_ms());
            printf("[SYS] Memory left is %d Bytes\r\n", xPortGetFreeHeapSize());
        }
        break;
        case CODE_WIFI_ON_PROV_SSID:
        {
            printf("[APP] [EVT] [PROV] [SSID] %lld: %s\r\n",
                    aos_now_ms(),
                    event->value ? (const char*)event->value : "UNKNOWN"
            );
        }
        break;
        case CODE_WIFI_ON_PROV_BSSID:
        {
            printf("[APP] [EVT] [PROV] [BSSID] %lld: %s\r\n",
                    aos_now_ms(),
                    event->value ? (const char*)event->value : "UNKNOWN"
            );
        }
        break;
        case CODE_WIFI_ON_PROV_PASSWD:
        {
            printf("[APP] [EVT] [PROV] [PASSWD] %lld: %s\r\n", aos_now_ms(),
                    event->value ? (const char*)event->value : "UNKNOWN"
            );
        }
        break;
        case CODE_WIFI_ON_PROV_CONNECT:
        {
            printf("[APP] [EVT] [PROV] [CONNECT] %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_PROV_DISCONNECT:
        {
            printf("[APP] [EVT] [PROV] [DISCONNECT] %lld\r\n", aos_now_ms());
        }
        break;
        default:
        {
            printf("[APP] [EVT] Unknown code %u, %lld\r\n", event->code, aos_now_ms());
            /*nothing*/
        }
    }
}
void HAL_WiFi_SetupStatusCallback(void (*cb)(int code)) {
	g_wifiStatusCallback = cb;

    aos_register_event_filter(EV_WIFI, event_cb_wifi_event, NULL);
}


void HAL_PrintNetworkInfo() {

}
const char *HAL_GetMyIPString() {
	uint32_t ip;
	uint32_t gw;
	uint32_t mask;

	if(g_bAccessPointMode == 1) {
		wifi_mgmr_ap_ip_get(&ip, &gw, &mask);
	} else {
		wifi_mgmr_sta_ip_get(&ip, &gw, &mask);
	}

	strcpy(g_ipStr,inet_ntoa(ip));

	return g_ipStr;
}
void WiFI_GetMacAddress(char *mac) {
	// FOR station mode
	wifi_mgmr_sta_mac_get((uint8_t*)mac);
}
const char *HAL_GetMACStr(char *macstr) {
	uint8_t mac[6];
	if(g_bAccessPointMode == 1) {
		wifi_mgmr_ap_mac_get(mac);
	} else {
		wifi_mgmr_sta_mac_get(mac);
	}
	sprintf(macstr,"%02X%02X%02X%02X%02X%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	return macstr;
}

#endif // PLATFORM_BL602