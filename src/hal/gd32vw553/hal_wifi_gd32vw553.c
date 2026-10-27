#if PLATFORM_GD32VW553

#include "wifi_management.h"
#include "wifi_init.h"
#include "wifi_net_ip.h"
#include "../hal_wifi.h"
#include "../../logging/logging.h"

extern uint8_t* g_mac;
static void (*g_wifiStatusCallback)(int code);
static int g_bAccessPointMode = 0;
static bool g_events_registered = false;
static char g_ipStr[16] = "unknown";
static char g_gwStr[16] = "unknown";
static char g_maskStr[16] = "unknown";
static char g_dnsStr[16] = "unknown";

void WiFI_GetMacAddress(char* mac)
{
	memcpy(mac, g_mac, 6);
}

const char* HAL_GetMyIPString()
{
	struct wifi_ip_addr_cfg ip_cfg;
	wifi_get_vif_ip(0, &ip_cfg);

	strcpy(g_ipStr, ip4addr_ntoa((const ip4_addr_t*)&(ip_cfg.ipv4.addr)));
	strcpy(g_gwStr, ip4addr_ntoa((const ip4_addr_t*)&(ip_cfg.ipv4.gw)));
	strcpy(g_maskStr, ip4addr_ntoa((const ip4_addr_t*)&(ip_cfg.ipv4.mask)));
	if(ip_cfg.ipv4.dns) strcpy(g_dnsStr, ip4addr_ntoa((const ip4_addr_t*)&(ip_cfg.ipv4.dns)));

	return g_ipStr;
}

const char* HAL_GetMyGatewayString()
{
	return g_gwStr;
}

const char* HAL_GetMyMaskString()
{
	return g_maskStr;
}

const char* HAL_GetMyDNSString()
{
	return g_dnsStr;
}

const char* HAL_GetMACStr(char* macstr)
{
	sprintf(macstr, MACSTR, MAC2STR(g_mac));
	return macstr;
}

int HAL_GetWifiStrength()
{
	return macif_vif_sta_rssi_get(g_bAccessPointMode);
}

void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;
}

static void sta_cb_conn_ok(void* eloop_data, void* user_ctx)
{
	if(g_wifiStatusCallback != 0)
	{
		g_wifiStatusCallback(WIFI_STA_CONNECTED);
	}
}

static void sta_cb_conn_fail(void* eloop_data, void* user_ctx)
{
	if(g_wifiStatusCallback != 0)
	{
		g_wifiStatusCallback(WIFI_STA_AUTH_FAILED);
	}
}

static void sta_cb_conn_disc(void* eloop_data, void* user_ctx)
{
	if(g_wifiStatusCallback != 0)
	{
		g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
	}
}

void HAL_ConnectToWiFi(const char* ssid, const char* pwd, obkStaticIP_t* ip)
{
	if(!g_events_registered)
	{
		eloop_event_register(WIFI_MGMT_EVENT_CONNECT_SUCCESS, sta_cb_conn_ok, NULL, NULL);
		eloop_event_register(WIFI_MGMT_EVENT_CONNECT_FAIL, sta_cb_conn_fail, NULL, NULL);
		eloop_event_register(WIFI_MGMT_EVENT_DISCONNECT, sta_cb_conn_disc, NULL, NULL);
		g_events_registered = true;
	}
	if(ip->localIPAddr[0] != 0)
	{
		struct wifi_ip_addr_cfg ip_cfg = { 0 };
		ip_cfg.mode = IP_ADDR_STATIC_IPV4;
		ip_cfg.ipv4.addr = *(uint32_t*)ip->localIPAddr;
		ip_cfg.ipv4.gw = *(uint32_t*)ip->gatewayIPAddr;
		ip_cfg.ipv4.mask = *(uint32_t*)ip->netMask;
		ip_cfg.ipv4.dns = *(uint32_t*)ip->dnsServerIpAddr;
		net_if_use_static_ip(1);
		wifi_set_vif_ip(0, &ip_cfg);
	}
	if(g_wifiStatusCallback != 0)
	{
		g_wifiStatusCallback(WIFI_STA_CONNECTING);
	}
	wifi_management_connect((char*)ssid, (char*)pwd, 0);
	g_bAccessPointMode = 0;
}

void HAL_DisconnectFromWifi()
{
	wifi_management_disconnect();
}

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	g_bAccessPointMode = 1;

	wifi_management_ap_start((char*)ssid, NULL, 1, AUTH_MODE_OPEN, 0);

	return 0;
}

void HAL_PrintNetworkInfo()
{
	uint8_t mac[6];
	WiFI_GetMacAddress((char*)mac);
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "+--------------- net device info ------------+");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif type    : %-16s            |", g_bAccessPointMode == 0 ? "STA" : "AP");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif rssi    = %-16i            |", HAL_GetWifiStrength());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif ip      = %-16s            |", HAL_GetMyIPString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif mask    = %-16s            |", HAL_GetMyMaskString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif gateway = %-16s            |", HAL_GetMyGatewayString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif mac     : "MACSTR" %-6s    |", MAC2STR(g_mac), "");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "+--------------------------------------------+");
}

#endif
