#if PLATFORM_ARMINO

#include "../hal_wifi.h"
#include "../src/new_pins.h"
#include "../src/new_cfg.h"
#include "../../new_common.h"
#include "../../logging/logging.h"
#include "bk_wifi.h"
#include "wifi_v2.h"
#include "easyflash.h"
#include <components/event.h>
#include <components/netif.h>

static void (*g_wifiStatusCallback)(int code);
static int g_bOpenAccessPointMode = 0;
bool g_bStaticIP = false;
static uint8_t* g_mac = NULL;

extern bk_err_t bk_wifi_get_ip_status(IPStatusTypedef* outNetpara, WiFi_Interface inInterface);
extern uint8_t* wpas_get_sta_psk(void);
static obkFastConnectData_t fcdata = {0};

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
	wifi_link_status_t link_status;
	memset(&link_status, 0x0, sizeof(link_status));
	bk_wifi_sta_get_link_status(&link_status);
	return link_status.rssi;
	//return bk_wifi_get_beacon_rssi();
}

/*
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
			wifi_link_status_t link_status = {0};
			bk_wifi_sta_get_link_status(&link_status);
			//ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "state %i aid %i rssi %i ssid %s bssid " MACSTR " channel %i security %i password %s",
			//	link_status.state, link_status.aid, link_status.rssi, link_status.ssid, MAC2STR(link_status.bssid), link_status.channel, link_status.security, link_status.password);

			if (CFG_HasFlag(OBK_FLAG_WIFI_ENHANCED_FAST_CONNECT))
			{
				char psks[65];
				memset(&psks, 0, sizeof(psks));
				if(link_status.security > WIFI_SECURITY_WEP && link_status.security < WIFI_SECURITY_WPA3_SAE)
				{
					uint8_t* psk = wpas_get_sta_psk();
					for(int i = 0; i < 32 && sprintf(psks + i * 2, "%02x", psk[i]) == 2; i++);
				}

				if(memcmp((char*)psks, fcdata.psk, 64) != 0 ||
					memcmp(fcdata.bssid, link_status.bssid, 6) != 0 ||
					link_status.channel != fcdata.channel ||
					link_status.security != fcdata.security_type)
				{
					ADDLOG_INFO(LOG_FEATURE_GENERAL, "Saved fast connect data differ to current one, saving...");
					memcpy(fcdata.bssid, link_status.bssid, 6);
					fcdata.channel = link_status.channel;
					fcdata.security_type = link_status.security;
					memcpy(fcdata.psk, psks, sizeof(fcdata.psk));
					ef_set_env_blob("fcdata", &fcdata, sizeof(obkFastConnectData_t));
				}
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
		default:
			break;
	}
}
*/

static int wl_status2(void* arg, event_module_t event_module,
	int event_id, void* event_data)
{
	switch(event_id)
	{
		//case EVENT_WIFI_STA_CONNECTED: if(!g_bStaticIP) break;
		case EVENT_NETIF_GOT_IP4:
			if(g_wifiStatusCallback != 0)
			{
				g_wifiStatusCallback(WIFI_STA_CONNECTED);
			}
			wifi_link_status_t link_status = { 0 };
			bk_wifi_sta_get_link_status(&link_status);
			//ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "state %i aid %i rssi %i ssid %s bssid " MACSTR " channel %i security %i password %s",
			//	link_status.state, link_status.aid, link_status.rssi, link_status.ssid, MAC2STR(link_status.bssid), link_status.channel, link_status.security, link_status.password);

			if(CFG_HasFlag(OBK_FLAG_WIFI_ENHANCED_FAST_CONNECT))
			{
				char psks[65];
				memset(&psks, 0, sizeof(psks));
				if(link_status.security > WIFI_SECURITY_WEP && link_status.security < WIFI_SECURITY_WPA3_SAE)
				{
					uint8_t* psk = wpas_get_sta_psk();
					for(int i = 0; i < 32 && sprintf(psks + i * 2, "%02x", psk[i]) == 2; i++);
				}

				if(memcmp((char*)psks, fcdata.psk, 64) != 0 ||
					memcmp(fcdata.bssid, link_status.bssid, 6) != 0 ||
					link_status.channel != fcdata.channel ||
					link_status.security != fcdata.security_type)
				{
					ADDLOG_INFO(LOG_FEATURE_GENERAL, "Saved fast connect data differ to current one, saving...");
					memcpy(fcdata.bssid, link_status.bssid, 6);
					fcdata.channel = link_status.channel;
					fcdata.security_type = link_status.security;
					memcpy(fcdata.psk, psks, sizeof(fcdata.psk));
					ef_set_env_blob("fcdata", &fcdata, sizeof(obkFastConnectData_t));
				}
			}
			//bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x1, 0x0); // significantly reduces power draw, but causes PWM to glitch
			break;
		case EVENT_NETIF_DHCP_TIMEOUT:
		case EVENT_WIFI_STA_DISCONNECTED:
			HAL_DisconnectFromWifi(); // TODO: discover better way to disable auto reconnect (it breaks static ip) 
			// (? https://github.com/NonPIayerCharacter/OpenBK7239N/blob/3f1b4f4ce1798a7d062467160f64dea37020f9ee/components/bk_netif/bk_netif.c#L70)
			if(g_wifiStatusCallback != 0)
			{
				g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
			}
			break;
		case EVENT_WIFI_AP_CONNECTED:
			if(g_wifiStatusCallback != 0)
			{
				g_wifiStatusCallback(WIFI_AP_CONNECTED);
			}
			break;
		case EVENT_WIFI_AP_DISCONNECTED:
			if(g_wifiStatusCallback != 0)
			{
				g_wifiStatusCallback(WIFI_AP_FAILED);
			}
			break;
		default: break;
	}
	return BK_OK;
}
void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;
	//bk_wlan_status_register_cb(wl_status);
	bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wl_status2, NULL);
	bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, wl_status2, NULL);
}

void HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	g_bOpenAccessPointMode = 0;
	wifi_sta_config_t sta_cfg;
	//wlan_auto_reconnect_t arc_cfg;
	//arc_cfg.max_count = -1;
	//arc_cfg.timeout = -1;
	//arc_cfg.disable_reconnect_when_disconnect = 1;
	memset(&sta_cfg, 0, sizeof(sta_cfg));
	sta_cfg.auto_reconnect_count = -1;
	sta_cfg.auto_reconnect_timeout = -1;
	sta_cfg.disable_auto_reconnect_after_disconnect = false;
	sta_cfg.is_user_fast_connect = 0;
	sta_cfg.is_not_support_auto_fci = 1;
	strcpy((char *)sta_cfg.ssid, oob_ssid);
	strcpy((char *)sta_cfg.password, connect_key);
	g_bStaticIP = false;
	if(ip->localIPAddr[0] != 0)
	{
		netif_ip4_config_t ip4_config;
		convert_IP_to_string(ip4_config.ip, ip->localIPAddr);
		convert_IP_to_string(ip4_config.mask, ip->netMask);
		convert_IP_to_string(ip4_config.gateway, ip->gatewayIPAddr);
		convert_IP_to_string(ip4_config.dns, ip->dnsServerIpAddr);
		bk_netif_static_ip(ip4_config);
		g_bStaticIP = true;
	}
	if(g_wifiStatusCallback != 0)
	{
		g_wifiStatusCallback(WIFI_STA_CONNECTING);
	}
	bk_wifi_sta_set_config(&sta_cfg);
	bk_wifi_sta_start();
	//wlan_sta_set_autoreconnect(&arc_cfg);
}

void HAL_FastConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	int len = ef_get_env_blob("fcdata", &fcdata, sizeof(obkFastConnectData_t), NULL);
	if(len == sizeof(obkFastConnectData_t) && fcdata.channel != 0)
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "We have fast connection data, connecting...");
		
		wifi_sta_config_t sta_cfg;
		//wlan_auto_reconnect_t arc_cfg;
		//arc_cfg.max_count = -1;
		//arc_cfg.timeout = -1;
		//arc_cfg.disable_reconnect_when_disconnect = 1;
		memset(&sta_cfg, 0, sizeof(sta_cfg));
		sta_cfg.auto_reconnect_count = -1;
		sta_cfg.auto_reconnect_timeout = -1;
		sta_cfg.disable_auto_reconnect_after_disconnect = false;
		sta_cfg.channel = fcdata.channel;
		sta_cfg.security = fcdata.security_type;
		sta_cfg.is_user_fast_connect = 1;
		sta_cfg.is_not_support_auto_fci = 1;
		sta_cfg.psk_len = 64;
		sta_cfg.psk_calculated = true;
		strcpy((char *)sta_cfg.ssid, oob_ssid);
		strcpy((char *)sta_cfg.password, connect_key);
		memcpy(sta_cfg.bssid, fcdata.bssid, sizeof(fcdata.bssid));
		memcpy(sta_cfg.psk, fcdata.psk, 64);
		g_bStaticIP = false;
		if(ip->localIPAddr[0] != 0)
		{
			netif_ip4_config_t ip4_config;
			convert_IP_to_string(ip4_config.ip, ip->localIPAddr);
			convert_IP_to_string(ip4_config.mask, ip->netMask);
			convert_IP_to_string(ip4_config.gateway, ip->gatewayIPAddr);
			convert_IP_to_string(ip4_config.dns, ip->dnsServerIpAddr);
			bk_netif_static_ip(ip4_config);
			g_bStaticIP = true;
		}
		if(g_wifiStatusCallback != 0)
		{
			g_wifiStatusCallback(WIFI_STA_CONNECTING);
		}
		bk_wifi_sta_set_config(&sta_cfg);
		bk_wifi_sta_start();
		//wlan_sta_set_autoreconnect(&arc_cfg);
	}
	else
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "Fast connect data is empty, connecting normally");
		HAL_ConnectToWiFi(oob_ssid, connect_key, ip);
	}
}

void HAL_DisableEnhancedFastConnect()
{
	ef_del_env("fcdata");
	ef_del_env("fast_connect_id");
}

void HAL_DisconnectFromWifi()
{
	bk_wifi_sta_disconnect();
}

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	wifi_ap_config_t ap_config;
	netif_ip4_config_t ip4_config;
	memset(&ap_config, 0, sizeof(ap_config));
	memset(&ip4_config, 0, sizeof(ip4_config));
	ap_config.channel = 1;
	strcpy(ip4_config.ip, "192.168.4.1");
	strcpy(ip4_config.mask, "255.255.255.0");
	strcpy(ip4_config.gateway, "192.168.4.1");
	strcpy(ip4_config.dns, "192.168.4.1");
	bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config);
	strcpy((char*)ap_config.ssid, ssid);
	bk_wifi_ap_set_config(&ap_config);
	bk_wifi_ap_start();
	g_bOpenAccessPointMode = 1;
	return 0;
}

#endif
