#ifdef PLATFORM_REALTEK_NEW

#include "../hal_wifi.h"
#include "../../new_cfg.h"
#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_pins.h"
#include <lwip/sockets.h>
#include <lwip/netif.h>
#include <lwip/dns.h>
#include <lwip_netconf.h>
#include <dhcp/dhcps.h>
#include "wifi_api.h"
#include "wifi_fast_connect.h"

extern struct netif xnetif[NET_IF_NUM];
extern void InitEasyFlashIfNeeded();
extern int wifi_do_fast_connect(void);

typedef struct
{
	char ssid[32];
	char pwd[64];
	unsigned char bssid[6];
} wifi_data_t;

extern uint8_t wmac[6];
extern bool g_powersave;
extern int (*p_wifi_do_fast_connect)(void);
extern int (*p_store_fast_connect_info)(unsigned int data1, unsigned int data2);

bool g_STA_static_IP = 0;

static void (*g_wifiStatusCallback)(int code) = NULL;
static int g_bOpenAccessPointMode = 0;
static wifi_data_t wdata = { 0 };
static int g_bStaticIP = 0;
obkFastConnectData_t fcdata = { 0 };
struct static_ip_config user_static_ip = { 0 };

const char* HAL_GetMyIPString()
{
	return ipaddr_ntoa(&xnetif[g_bOpenAccessPointMode].ip_addr);
}

const char* HAL_GetMyGatewayString()
{
	return ipaddr_ntoa(&xnetif[g_bOpenAccessPointMode].gw);
}

const char* HAL_GetMyDNSString()
{
	return ipaddr_ntoa(dns_getserver(0));
}

const char* HAL_GetMyMaskString()
{
	return ipaddr_ntoa(&xnetif[g_bOpenAccessPointMode].netmask);
}

int WiFI_SetMacAddress(char* mac)
{
	return 0;
}

void WiFI_GetMacAddress(char* mac)
{
	memcpy(mac, (char*)wmac, sizeof(wmac));
}

const char* HAL_GetMACStr(char* macstr)
{
	unsigned char mac[6];
	WiFI_GetMacAddress((char*)mac);
	sprintf(macstr, MACSTR, MAC2STR(mac));
	return macstr;
}

void HAL_PrintNetworkInfo()
{
	uint8_t mac[6];
	WiFI_GetMacAddress((char*)mac);
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "+--------------- net device info ------------+\r\n");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif type    : %-16s            |\r\n", g_bOpenAccessPointMode == 0 ? "STA" : "AP");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif rssi    = %-16i            |\r\n", HAL_GetWifiStrength());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif ip      = %-16s            |\r\n", HAL_GetMyIPString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif mask    = %-16s            |\r\n", HAL_GetMyMaskString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif gateway = %-16s            |\r\n", HAL_GetMyGatewayString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif mac     : [%02X:%02X:%02X:%02X:%02X:%02X] %-7s |\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], "");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "+--------------------------------------------+\r\n");
}

int HAL_GetWifiStrength()
{
	union rtw_phy_stats phy_stats;
	wifi_get_phy_stats(STA_WLAN_INDEX, NULL, &phy_stats);
	return -((uint8_t)(0xFF - phy_stats.sta.rssi + 1));
}

void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;
}

void obk_wifi_hdl_new(u8* buf, s32 buf_len, s32 flags, void* userdata)
{
	UNUSED(buf_len);
	UNUSED(userdata);
	u8 join_status = (u8)flags;
	struct rtw_event_info_joinstatus_joinfail* fail_info = (struct rtw_event_info_joinstatus_joinfail*)buf;
	struct rtw_event_info_joinstatus_disconn* disconn_info = (struct rtw_event_info_joinstatus_disconn*)buf;

	if(join_status == RTW_JOINSTATUS_SUCCESS)
	{
#if LWIP_NETIF_HOSTNAME
		netif_set_hostname(&xnetif[0], CFG_GetDeviceName());
#endif
		return;
	}

	if(join_status == RTW_JOINSTATUS_FAIL)
	{
		switch(fail_info->fail_reason)
		{
			case -RTK_ERR_WIFI_CONN_INVALID_KEY:
			case -RTK_ERR_WIFI_CONN_AUTH_PASSWORD_WRONG:
			case -RTK_ERR_WIFI_CONN_4WAY_PASSWORD_WRONG:
			case -RTK_ERR_WIFI_CONN_AUTH_FAIL:
				if(g_wifiStatusCallback != NULL)
				{
					g_wifiStatusCallback(WIFI_STA_AUTH_FAILED);
				}
				break;
			case -RTK_ERR_WIFI_CONN_SCAN_FAIL:
			case -RTK_ERR_WIFI_CONN_ASSOC_FAIL:
			case -RTK_ERR_WIFI_CONN_4WAY_HANDSHAKE_FAIL:
			default:
				if(g_wifiStatusCallback != NULL)
				{
					g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
				}
				break;
		}
		return;
	}

	if(join_status == RTW_JOINSTATUS_DISCONNECT)
	{
		if(g_wifiStatusCallback != NULL)
		{
			g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
		}
	}
}

void ConnectToWiFiTask(void* args)
{
	struct rtw_network_info connect_param = { 0 };

	memcpy(connect_param.ssid.val, wdata.ssid, strlen(wdata.ssid));
	connect_param.ssid.len = strlen(wdata.ssid);
	connect_param.password = (unsigned char*)wdata.pwd;
	connect_param.password_len = strlen(wdata.pwd);

	if(g_wifiStatusCallback != NULL)
	{
		g_wifiStatusCallback(WIFI_STA_CONNECTING);
	}
	wifi_connect(&connect_param, 1);
	if(!g_bStaticIP) LwIP_DHCP(0, DHCP_START);
	if(g_wifiStatusCallback != NULL)
	{
		g_wifiStatusCallback(WIFI_STA_CONNECTED);
	}
	vTaskDelete(NULL);
}

void ConfigureSTA(obkStaticIP_t* ip)
{
	struct ip_addr ipaddr;
	struct ip_addr netmask;
	struct ip_addr gw;
	struct ip_addr dnsserver;

	wifi_set_autoreconnect(0);
	wifi_reg_event_handler(RTW_EVENT_JOIN_STATUS, obk_wifi_hdl_new, NULL);

	if(ip->localIPAddr[0] == 0)
	{
		g_bStaticIP = 0;
	}
	else
	{
		g_bStaticIP = 1;

		IP4_ADDR(ip_2_ip4(&ipaddr), ip->localIPAddr[0], ip->localIPAddr[1], ip->localIPAddr[2], ip->localIPAddr[3]);
		IP4_ADDR(ip_2_ip4(&netmask), ip->netMask[0], ip->netMask[1], ip->netMask[2], ip->netMask[3]);
		IP4_ADDR(ip_2_ip4(&gw), ip->gatewayIPAddr[0], ip->gatewayIPAddr[1], ip->gatewayIPAddr[2], ip->gatewayIPAddr[3]);
		IP4_ADDR(ip_2_ip4(&dnsserver), ip->dnsServerIpAddr[0], ip->dnsServerIpAddr[1], ip->dnsServerIpAddr[2], ip->dnsServerIpAddr[3]);
		netif_set_addr(&xnetif[STA_WLAN_INDEX], ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
		dns_setserver(0, &dnsserver);
	}
}

void HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	g_bOpenAccessPointMode = 0;
	strcpy((char*)&wdata.ssid, oob_ssid);
	strncpy((char*)&wdata.pwd, connect_key, 64);
	
	ConfigureSTA(ip);

	xTaskCreate(
		(TaskFunction_t)ConnectToWiFiTask,
		"WC",
		4096 / sizeof(StackType_t),
		NULL,
		9,
		NULL);
}

void HAL_DisconnectFromWifi()
{
	wifi_disconnect();
}

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	g_bOpenAccessPointMode = 1;
	rtw_mode_t mode = RTW_MODE_STA_AP;
	struct ip_addr ipaddr;
	struct ip_addr netmask;
	struct ip_addr gw;
	struct netif* pnetif = &xnetif[SOFTAP_WLAN_INDEX];
	dhcps_deinit();
	wifi_stop_ap();
	struct rtw_softap_info connect_param = { 0 };
	memcpy(connect_param.ssid.val, ssid, strlen(ssid));
	connect_param.ssid.len = strlen(ssid);
	connect_param.security_type = RTW_SECURITY_OPEN;
	connect_param.channel = 1;

	if(wifi_start_ap(&connect_param) < 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_GENERAL, "Failed to start AP");
		return 0;
	}
	IP4_ADDR(ip_2_ip4(&ipaddr), 192, 168, 4, 1);
	IP4_ADDR(ip_2_ip4(&netmask), 255, 255, 255, 0);
	IP4_ADDR(ip_2_ip4(&gw), 192, 168, 4, 1);
	netifapi_netif_set_addr(pnetif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));

	dhcps_init(pnetif);
	return 0;
}

void FastConnectToWiFiTask(void* args)
{
	struct wlan_fast_reconnect* data = (struct wlan_fast_reconnect*)malloc(sizeof(struct wlan_fast_reconnect));
	struct wlan_fast_reconnect* empty_data = (struct wlan_fast_reconnect*)malloc(sizeof(struct wlan_fast_reconnect));
	memset(data, 0xff, sizeof(struct wlan_fast_reconnect));
	memset(empty_data, 0xff, sizeof(struct wlan_fast_reconnect));
	int ret = rt_kv_get("wlan_data", (uint8_t*)data, sizeof(struct wlan_fast_reconnect));
	if(ret < 0 || memcmp(empty_data, data, sizeof(struct wlan_fast_reconnect)) == 0)
	{
		ConnectToWiFiTask(args);
		goto exit;
	}
	if(g_wifiStatusCallback != NULL)
	{
		g_wifiStatusCallback(WIFI_STA_CONNECTING);
	}
	wifi_do_fast_connect();
	if(g_wifiStatusCallback != NULL)
	{
		g_wifiStatusCallback(WIFI_STA_CONNECTED);
	}
exit:
	free(data);
	free(empty_data);
	vTaskDelete(NULL);
}

void HAL_FastConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	g_bOpenAccessPointMode = 0;
	strcpy((char*)&wdata.ssid, oob_ssid);
	strncpy((char*)&wdata.pwd, connect_key, 64);

	ConfigureSTA(ip);

	xTaskCreate(
		(TaskFunction_t)FastConnectToWiFiTask,
		"WFC",
		4096 / sizeof(StackType_t),
		NULL,
		9,
		NULL);
}

void HAL_DisableEnhancedFastConnect()
{
	rt_kv_delete("wlan_data");
}

#endif // PLATFORM_REALTEK_NEW
