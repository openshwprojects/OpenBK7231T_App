#if PLATFORM_RDA5981

#include "rda59xx_daemon.h"
#include "WiFiStackInterface.h"
#undef WEAK
WiFiStackInterface wifi;

extern "C" {
#include "../hal_wifi.h"
#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#undef connect

// is (Open-) Access point or a client?
// included as "extern short g_WifiMode;" from new_common.h
// initilized in user_main.c
// values:	0 = STA	1 = OpenAP	2 = WAP-AP



static void (*g_wifiStatusCallback)(int code);
extern struct netif lwip_sta_netif;
extern uint8_t macaddr[6];

const char* HAL_GetMyIPString()
{
	return g_WifiMode ? wifi.get_ip_address_ap() : wifi.get_ip_address();
}

const char* HAL_GetMyGatewayString()
{
	return g_WifiMode ? wifi.get_gateway_ap() : wifi.get_gateway();
}

const char* HAL_GetMyDNSString()
{
	return "error";
}

const char* HAL_GetMyMaskString()
{
	return g_WifiMode ? wifi.get_netmask_ap() : wifi.get_netmask();
}

void WiFI_GetMacAddress(char* mac)
{
	memcpy(mac, macaddr, sizeof(mac));
}

int WiFI_SetMacAddress(char* mac)
{
	return rda5981_flash_write_mac_addr((uint8_t*)mac) == 0;
}

uint8_t HAL_GetWiFiChannel(uint8_t *chan)
{
	*chan = wifi.get_channel();
	return *chan;
};

const char* HAL_GetMACStr(char* macstr)
{
	unsigned char mac[32];
	WiFI_GetMacAddress((char*)mac);
	sprintf(macstr, MACSTR, MAC2STR(mac));
	return macstr;
}

void HAL_PrintNetworkInfo()
{
	uint8_t mac[6];
	WiFI_GetMacAddress((char*)mac);
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "+--------------- net device info ------------+\r\n");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif type    : %-16s            |\r\n", g_WifiMode == 0 ? "STA" : "AP");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif rssi    = %-16i            |\r\n", HAL_GetWifiStrength());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif ip      = %-16s            |\r\n", HAL_GetMyIPString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif mask    = %-16s            |\r\n", HAL_GetMyMaskString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif gateway = %-16s            |\r\n", HAL_GetMyGatewayString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif mac     : ["MACSTR"] %-6s  |\r\n", MAC2STR(mac), "");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "+--------------------------------------------+\r\n");
}

int HAL_GetWifiStrength()
{
	wifi.update_rssi();
	return wifi.get_rssi();
}

void obk_wifi_cb(WIFI_EVENT evt, void* info)
{
	switch(evt)
	{
		case EVENT_STA_DISCONNECTTED:
		case EVENT_STA_NOFOUND_AP:
			if(g_wifiStatusCallback != NULL)
			{
				g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
			}
			break;
		case EVENT_STA_CONNECT_FAIL:
			if(g_wifiStatusCallback != NULL)
			{
				g_wifiStatusCallback(WIFI_STA_AUTH_FAILED);
			}
			break;
		case EVENT_STA_GOT_IP:
			if(g_wifiStatusCallback != NULL)
			{
				g_wifiStatusCallback(WIFI_STA_CONNECTED);
			}
		default:
			break;
	}
}
void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;
	rda59xx_wifi_set_event_cb(obk_wifi_cb);
}

void HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	if(g_wifiStatusCallback != NULL)
	{
		g_wifiStatusCallback(WIFI_STA_CONNECTING);
	}
	wifi.set_dhcp(1);
	netif_set_hostname(&lwip_sta_netif, CFG_GetDeviceName());
	wifi.connect(oob_ssid, connect_key, NULL, NSAPI_SECURITY_NONE, 0);
}

void HAL_DisconnectFromWifi()
{
	wifi.disconnect();
}

#if ENABLE_WPA_AP
int HAL_SetupWiFiAccessPoint(const char* ssid, const char *key)
{
	char dhcpend[16]={0};	//4 * up to 3 digits (=12) + 3 "." (=15) + "\0" --> 16
	sprintf(dhcpend,"192.168.4.%i",2 + AP_STA_CLIENTS);
	
	wifi.set_network_ap("192.168.4.1", "255.255.255.0", "192.168.4.1", "192.168.4.2", dhcpend);
	wifi.start_ap(ssid, key, g_wifi_channel);
	return 0;
}
#endif
int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	wifi.set_network_ap("192.168.4.1", "255.255.255.0", "192.168.4.1", "192.168.4.2", "192.168.4.254");
	wifi.start_ap(ssid, "", g_wifi_channel);
	return 0;
}

}
#endif
