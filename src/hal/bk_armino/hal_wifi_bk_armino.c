#if PLATFORM_ARMINO

#include "../hal_wifi.h"
#include "../../new_common.h"
#include "../../logging/logging.h"
#include "bk_wifi.h"

static void (*g_wifiStatusCallback)(int code);
static int g_bOpenAccessPointMode = 0;
bool g_bStaticIP = false;
static uint8_t* g_mac = NULL;

static char g_IP[16] = "unknown";
IPStatusTypedef ipStatus;
const char* HAL_GetMyIPString()
{
	memset(&ipStatus, 0x0, sizeof(IPStatusTypedef));
	if(g_bOpenAccessPointMode)
	{
		bk_wifi_get_ip_status(&ipStatus, BK_SOFT_AP);
	}
	else
	{
		bk_wifi_get_ip_status(&ipStatus, BK_STATION);
	}

	strncpy(g_IP, ipStatus.ip, 16);
	return g_IP;
}

const char* HAL_GetMyGatewayString()
{
	strncpy(g_IP, ipStatus.gate, 16);
	return g_IP;
}

const char* HAL_GetMyDNSString()
{
	strncpy(g_IP, ipStatus.dns, 16);
	return g_IP;
}

const char* HAL_GetMyMaskString()
{
	strncpy(g_IP, ipStatus.mask, 16);
	return g_IP;
}

void WiFI_GetMacAddress(char* mac)
{
	if(g_mac == NULL)
	{
		g_mac = os_malloc(6);
		bk_get_mac((uint8_t*)g_mac, MAC_TYPE_STA);
	}
	if(mac) memcpy(mac, g_mac, 6);
}

const char* HAL_GetMACStr(char* macstr)
{
	WiFI_GetMacAddress(NULL);
	sprintf(macstr, MACSTR, MAC2STR(g_mac));
	return macstr;
}

void HAL_PrintNetworkInfo()
{
	uint8_t mac[6];
	WiFI_GetMacAddress((char*)mac);
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "+--------------- net device info ------------+");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif type    : %-16s            |", g_bOpenAccessPointMode == 0 ? "STA" : "AP");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif rssi    = %-16i            |", HAL_GetWifiStrength());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif ip      = %-16s            |", HAL_GetMyIPString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif mask    = %-16s            |", HAL_GetMyMaskString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif gateway = %-16s            |", HAL_GetMyGatewayString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif mac     : "MACSTR" %-6s    |", MAC2STR(mac), "");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "+--------------------------------------------+");
}

int HAL_GetWifiStrength()
{
	//wifi_link_status_t link_status;
	//os_memset(&link_status, 0x0, sizeof(link_status));
	//bk_wifi_sta_get_link_status(&link_status);
	//return link_status.rssi;
	return bk_wifi_get_beacon_rssi();
}

void wl_status(void* ctxt)
{
	wifi_linkstate_reason_t info = *((wifi_linkstate_reason_t*)ctxt);
	switch(info.state)
	{
		case WIFI_LINKSTATE_STA_CONNECTING:
			if(g_wifiStatusCallback != 0)
			{
				g_wifiStatusCallback(WIFI_STA_CONNECTING);
			}
			break;
		case WIFI_LINKSTATE_STA_DISCONNECTED:
			if(g_wifiStatusCallback != 0)
			{
				g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
			}
			break;
		case WIFI_LINKSTATE_STA_CONNECT_FAILED:
			if(g_wifiStatusCallback != 0)
			{
				g_wifiStatusCallback(WIFI_STA_AUTH_FAILED);
			}
			break;
		case WIFI_LINKSTATE_STA_CONNECTED: if(!g_bStaticIP) break;
		case WIFI_LINKSTATE_STA_GOT_IP:
			if(g_wifiStatusCallback != 0)
			{
				g_wifiStatusCallback(WIFI_STA_CONNECTED);
			}
			break;
		case WIFI_LINKSTATE_AP_CONNECTED:
			if(g_wifiStatusCallback != 0)
			{
				g_wifiStatusCallback(WIFI_AP_CONNECTED);
			}
			break;
		case WIFI_LINKSTATE_AP_DISCONNECTED:
		case WIFI_LINKSTATE_AP_CONNECT_FAILED:
			if(g_wifiStatusCallback != 0)
			{
				g_wifiStatusCallback(WIFI_AP_FAILED);
			}
			break;
	}
}
void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;
	bk_wlan_status_register_cb(wl_status);
}

void HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	g_bOpenAccessPointMode = 0;

	network_InitTypeDef_st network_cfg;

	memset(&network_cfg, 0x0, sizeof(network_InitTypeDef_st));

	strcpy((char*)network_cfg.wifi_ssid, oob_ssid);
	strcpy((char*)network_cfg.wifi_key, connect_key);

	network_cfg.wifi_mode = BK_STATION;
	if(ip->localIPAddr[0] == 0)
	{
		network_cfg.dhcp_mode = DHCP_CLIENT;
		g_bStaticIP = false;
	}
	else
	{
		network_cfg.dhcp_mode = DHCP_DISABLE;
		convert_IP_to_string(network_cfg.local_ip_addr, ip->localIPAddr);
		convert_IP_to_string(network_cfg.net_mask, ip->netMask);
		convert_IP_to_string(network_cfg.gateway_ip_addr, ip->gatewayIPAddr);
		convert_IP_to_string(network_cfg.dns_server_ip_addr, ip->dnsServerIpAddr);
		g_bStaticIP = true;
	}
	network_cfg.wifi_retry_interval = 100;

	bk_wlan_start_sta(&network_cfg);
}

void HAL_DisconnectFromWifi()
{
	bk_wifi_disable();
}

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	//network_InitTypeDef_st wNetConfig;
	//
	//os_memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_st));
	//
	//os_strcpy((char*)wNetConfig.wifi_ssid, ssid);
	//
	//wNetConfig.wifi_mode = BK_SOFT_AP;
	//wNetConfig.dhcp_mode = DHCP_SERVER;
	//wNetConfig.wifi_retry_interval = 100;
	//os_strcpy((char*)wNetConfig.local_ip_addr, "192.168.4.1");
	//os_strcpy((char*)wNetConfig.net_mask, "255.255.255.0");
	//os_strcpy((char*)wNetConfig.dns_server_ip_addr, "192.168.4.1");
	//
	//bk_wlan_ap_set_default_channel(1);
	//bk_wlan_start_ap(&wNetConfig);

	wifi_ap_config_t ap_config;
	netif_ip4_config_t ip4_config;
	os_memset(&ap_config, 0, sizeof(ap_config));
	os_memset(&ip4_config, 0, sizeof(ip4_config));
	ap_config.channel = 1;
	os_strcpy(ip4_config.ip, "192.168.4.1");
	os_strcpy(ip4_config.mask, "255.255.255.0");
	os_strcpy(ip4_config.gateway, "192.168.4.1");
	os_strcpy(ip4_config.dns, "192.168.4.1");
	bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config);
	os_strcpy((char*)ap_config.ssid, ssid);
	bk_wifi_ap_set_config(&ap_config);
	bk_wifi_ap_start();
	g_bOpenAccessPointMode = 1;
	return 0;
}

#endif
