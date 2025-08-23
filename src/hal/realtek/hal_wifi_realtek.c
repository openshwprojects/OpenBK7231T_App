#ifdef PLATFORM_REALTEK

#include "../hal_wifi.h"
#include "../../new_cfg.h"
#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_pins.h"
#include <lwip/sockets.h>
#include <lwip/netif.h>
#include <lwip/dns.h>
#include <wifi/wifi_conf.h>
#include <wifi/wifi_util.h>
#include <lwip_netconf.h>
#include "tcpip.h"
#include <dhcp/dhcps.h>
#include <easyflash.h>

extern struct netif xnetif[NET_IF_NUM];
extern void InitEasyFlashIfNeeded();
extern int wifi_change_mac_address_from_ram(int idx, uint8_t* mac);
extern int wifi_scan_networks_with_ssid_by_extended_security(int (results_handler)(char* buf, int buflen, char* ssid, void* user_data),
	void* user_data, int scan_buflen, char* ssid, int ssid_len);
extern unsigned char psk_essid[NET_IF_NUM][32 + 4];
extern unsigned char psk_passphrase[NET_IF_NUM][128 + 1];
extern unsigned char wpa_global_PSK[NET_IF_NUM][20 * 2];

extern uint8_t wmac[6];
extern bool g_powersave;

typedef struct
{
	char ssid[32];
	char pwd[64];
	unsigned char bssid[6];
} wifi_data_t;

bool g_STA_static_IP = 0;
bool mac_init = false;

static void (*g_wifiStatusCallback)(int code) = NULL;
static int g_bOpenAccessPointMode = 0;
static wifi_data_t wdata = { 0 };
static int g_bStaticIP = 0;
static char g_IP[16] = "unknown";
static char g_GW[16] = "unknown";
static char g_MS[16] = "unknown";
obkFastConnectData_t fcdata = { 0 };

const char* HAL_GetMyIPString()
{
	return g_IP;
}

const char* HAL_GetMyGatewayString()
{
	return g_GW;
}

const char* HAL_GetMyDNSString()
{
	return ipaddr_ntoa(dns_getserver(0));
}

const char* HAL_GetMyMaskString()
{
	return g_MS;
}

#if PLATFORM_RTL8710A
extern int wifi_set_mac_address(char* mac);
#endif

int WiFI_SetMacAddress(char* mac)
{
	printf("WiFI_SetMacAddress\r\n");
#if PLATFORM_RTL8720D
	InitEasyFlashIfNeeded();
	wifi_change_mac_address_from_ram(0, (uint8_t*)mac);
	memcpy(wmac, mac, sizeof(wmac));
	return ef_set_env_blob("rtlmac", mac, sizeof(wmac));
#elif PLATFORM_RTL8710A
	InitEasyFlashIfNeeded();
	char macstr[21];
	memset(macstr, 0, sizeof(macstr));
	sprintf(macstr, "%02x%02x%02x%02x%02x%02x", \
		mac[0], mac[1], mac[2], \
		mac[3], mac[4], mac[5]);
	wifi_set_mac_address(macstr);
	wifi_off();
	vTaskDelay(20);
	wifi_on(RTW_MODE_STA);
	return ef_set_env_blob("rtlmac", mac, sizeof(wmac));
#else
	return 0; // error
#endif
}

void WiFI_GetMacAddress(char* mac)
{
#if PLATFORM_RTL8720D || PLATFORM_RTL8710A
	//if((wmac[0] == 255 && wmac[1] == 255 && wmac[2] == 255 && wmac[3] == 255 && wmac[4] == 255 && wmac[5] == 255)
	//	|| (wmac[0] == 0 && wmac[1] == 0 && wmac[2] == 0 && wmac[3] == 0 && wmac[4] == 0 && wmac[5] == 0))
	if(!mac_init)
	{
		InitEasyFlashIfNeeded();
		uint8_t fmac[6] = { 0 };
		int readLen = ef_get_env_blob("rtlmac", &fmac, sizeof(fmac), NULL);
		if(readLen)
		{
#if PLATFORM_RTL8710A
			char macstr[21];
			memset(macstr, 0, sizeof(macstr));
			sprintf(macstr, "%02x%02x%02x%02x%02x%02x", \
				fmac[0], fmac[1], fmac[2], \
				fmac[3], fmac[4], fmac[5]);
			wifi_set_mac_address(macstr);
			wifi_off();
			vTaskDelay(20);
			wifi_on(RTW_MODE_STA);
#else
			wifi_change_mac_address_from_ram(0, fmac);
#endif
			memcpy(wmac, fmac, sizeof(fmac));
		}
		mac_init = true;
	}
#endif
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
	int rssi;
	wext_get_rssi(WLAN0_NAME, &rssi);
	return rssi;
}

char* HAL_GetWiFiBSSID(char* bssid){
	uint8_t mac[6];
	wext_get_bssid(WLAN0_NAME, mac);
	sprintf(bssid, MACSTR, MAC2STR(mac));
	return bssid; 
};
uint8_t HAL_GetWiFiChannel(uint8_t *chan){
	wext_get_channel(WLAN0_NAME, chan);
	return *chan;
};

void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;
}

int obk_find_ap_from_scan_buf(char* buf, int buflen, char* target_ssid, void* user_data)
{
	rtw_wifi_setting_t* pwifi = (rtw_wifi_setting_t*)user_data;
	int plen = 0;

	while(plen < buflen)
	{
		u8 len, ssid_len;
		int security_mode;
		char* ssid;

		// len offset = 0
		len = (int)*(buf + plen);
		// check end
		if(len == 0)
		{
			break;
		}
		ssid_len = len - BUFLEN_LEN - MAC_LEN - RSSI_LEN - SECURITY_LEN_EXTENDED - WPS_ID_LEN - CHANNEL_LEN;
		ssid = buf + plen + BUFLEN_LEN + MAC_LEN + RSSI_LEN + SECURITY_LEN_EXTENDED + WPS_ID_LEN + CHANNEL_LEN;
		if((ssid_len == strlen(target_ssid))
			&& (!memcmp(ssid, target_ssid, ssid_len)))
		{
			strncpy((char*)pwifi->ssid, target_ssid, 33);
			pwifi->channel = *(buf + plen + BUFLEN_LEN + MAC_LEN + RSSI_LEN + SECURITY_LEN_EXTENDED + WPS_ID_LEN);
			security_mode = *(int*)(buf + plen + BUFLEN_LEN + MAC_LEN + RSSI_LEN);
			pwifi->security_type = security_mode;
			memcpy(wdata.bssid, buf + plen + BUFLEN_LEN, 6);
			break;
		}
		plen += len;
	}
	return 0;
}

static int _get_ap_security_mode(char* ssid, rtw_security_t* security_mode, u8* channel)
{
	rtw_wifi_setting_t wifi;
	u32 scan_buflen = 1024;

	memset(&wifi, 0, sizeof(wifi));

	if(wifi_scan_networks_with_ssid_by_extended_security(obk_find_ap_from_scan_buf, (void*)&wifi, scan_buflen, ssid, strlen(ssid)) != RTW_SUCCESS)
	{
		printf("Wifi scan failed!\n");
		return 0;
	}

	if(strcmp((char*)wifi.ssid, ssid) == 0)
	{
		*security_mode = wifi.security_type;
		*channel = wifi.channel;
		return 1;
	}

	return 0;
}

void wifi_dis_hdl(u8* buf, u32 buf_len, u32 flags, void* userdata)
{
	if(g_wifiStatusCallback != NULL)
	{
		g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
		memset(&g_IP, 0, 16);
		memset(&g_GW, 0, 16);
		memset(&g_MS, 0, 16);
		strcpy((char*)&g_IP, "unknown");
		strcpy((char*)&g_GW, "unknown");
		strcpy((char*)&g_MS, "unknown");
	}
}

void wifi_conned_hdl(u8* buf, u32 buf_len, u32 flags, void* userdata)
{
	if(g_wifiStatusCallback != NULL)
	{
		g_wifiStatusCallback(WIFI_STA_CONNECTED);
		memset(&g_IP, 0, 16);
		memset(&g_GW, 0, 16);
		memset(&g_MS, 0, 16);
		strcpy((char*)&g_IP, ipaddr_ntoa((const ip4_addr_t*)&xnetif[g_bOpenAccessPointMode].ip_addr.addr));
		strcpy((char*)&g_GW, ipaddr_ntoa((const ip4_addr_t*)&xnetif[g_bOpenAccessPointMode].gw.addr));
		strcpy((char*)&g_MS, ipaddr_ntoa((const ip4_addr_t*)&xnetif[g_bOpenAccessPointMode].netmask.addr));
	}
}

void wifi_af_hdl(u8* buf, u32 buf_len, u32 flags, void* userdata)
{
	if(g_wifiStatusCallback != NULL)
	{
		g_wifiStatusCallback(WIFI_STA_AUTH_FAILED);
	}
}

void ConnectToWiFiTask(void* args)
{
	rtw_security_t security_type = RTW_SECURITY_OPEN;
	int security_retry_count = 0;
	uint8_t connect_channel = 0;
	while(1)
	{
		if(_get_ap_security_mode(wdata.ssid, &security_type, &connect_channel))
		{
			break;
		}
		security_retry_count++;
		delay_ms(50);
		if(security_retry_count >= 5)
		{
			ADDLOG_ERROR(LOG_FEATURE_GENERAL, "Can't get AP security mode and channel.");
			goto exit;
		}
	}
	if(g_wifiStatusCallback != NULL)
	{
		g_wifiStatusCallback(WIFI_STA_CONNECTING);
	}
	int ret = wifi_connect(wdata.ssid, security_type, wdata.pwd, strlen(wdata.ssid),
		strlen(wdata.pwd), NULL, NULL);
	if(ret != RTW_SUCCESS)
	{
		ADDLOG_ERROR(LOG_FEATURE_GENERAL, "Can't connect to AP");
		goto exit;
	}
	if(g_bStaticIP == 0) 
	{
		LwIP_DHCP(0, DHCP_START);
	}
	if(wifi_is_up(RTW_STA_INTERFACE) && g_powersave) wifi_enable_powersave();

	if(CFG_HasFlag(OBK_FLAG_WIFI_ENHANCED_FAST_CONNECT))
	{
		ef_get_env_blob("fcdata", &fcdata, sizeof(obkFastConnectData_t), NULL);
		unsigned char bssid[6] = { 0 };
		if(wifi_get_ap_bssid((unsigned char*)&bssid) == -1)
		{
			ADDLOG_WARN(LOG_FEATURE_GENERAL, "Failed to get bssid, using one from scan");
			memcpy(bssid, wdata.bssid, 6);
		}
		rtw_wifi_setting_t setting;
		wifi_get_setting(WLAN0_NAME, &setting);
		if(strcmp((char*)setting.password, fcdata.pwd) != 0 ||
			memcmp(wpa_global_PSK, fcdata.wpa_global_PSK, 40) != 0 ||
			memcmp(&bssid, fcdata.bssid, 6) != 0 ||
			setting.channel != fcdata.channel ||
			setting.security_type != fcdata.security_type)
		{
			ADDLOG_INFO(LOG_FEATURE_GENERAL, "Saved fast connect data differ to current one, saving...");
			memset(&fcdata, 0, sizeof(obkFastConnectData_t));
			strcpy(fcdata.pwd, (char*)setting.password);
			memcpy(fcdata.wpa_global_PSK, wpa_global_PSK, 40);
			fcdata.channel = setting.channel;
			fcdata.security_type = setting.security_type;
			memcpy(fcdata.bssid, &bssid, sizeof(bssid));
			ef_set_env_blob("fcdata", &fcdata, sizeof(obkFastConnectData_t));
		}
	}

	vTaskDelete(NULL);
	return;
exit:
	if(g_wifiStatusCallback != NULL)
	{
		g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
	}
	vTaskDelete(NULL);
	return;
}

void ConfigureSTA(obkStaticIP_t* ip)
{
	struct ip_addr ipaddr;
	struct ip_addr netmask;
	struct ip_addr gw;
	struct ip_addr dnsserver;

	if(ip->localIPAddr[0] == 0)
	{
		//dhcps_init(&xnetif[0]);
		g_bStaticIP = 0;
	}
	else
	{
		// dhcps_deinit();
		g_bStaticIP = 1;

		IP4_ADDR(ip_2_ip4(&ipaddr), ip->localIPAddr[0], ip->localIPAddr[1], ip->localIPAddr[2], ip->localIPAddr[3]);
		IP4_ADDR(ip_2_ip4(&netmask), ip->netMask[0], ip->netMask[1], ip->netMask[2], ip->netMask[3]);
		IP4_ADDR(ip_2_ip4(&gw), ip->gatewayIPAddr[0], ip->gatewayIPAddr[1], ip->gatewayIPAddr[2], ip->gatewayIPAddr[3]);
		IP4_ADDR(ip_2_ip4(&dnsserver), ip->dnsServerIpAddr[0], ip->dnsServerIpAddr[1], ip->dnsServerIpAddr[2], ip->dnsServerIpAddr[3]);
		netif_set_addr(&xnetif[0], ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
		dns_setserver(0, &dnsserver);
	}
	if(!g_bStaticIP) wifi_config_autoreconnect(1, 2, 3);
	else wifi_set_autoreconnect(0);
	netif_set_hostname(&xnetif[0], CFG_GetDeviceName());
}

void RegisterHandlers()
{
	wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, (rtw_event_handler_t)wifi_dis_hdl, NULL);
	if(g_bStaticIP)
	{
		// with static IP, assume  that connect is enough?
		wifi_reg_event_handler(WIFI_EVENT_CONNECT, (rtw_event_handler_t)wifi_conned_hdl, NULL);
	}
	wifi_reg_event_handler(WIFI_EVENT_STA_GOT_IP, (rtw_event_handler_t)wifi_conned_hdl, NULL);
	wifi_reg_event_handler(WIFI_EVENT_CHALLENGE_FAIL, (rtw_event_handler_t)wifi_af_hdl, NULL);
}

void HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	g_bOpenAccessPointMode = 0;
	strcpy((char*)&wdata.ssid, oob_ssid);
	strncpy((char*)&wdata.pwd, connect_key, 64);
	
	ConfigureSTA(ip);
	RegisterHandlers();

	xTaskCreate(
		(TaskFunction_t)ConnectToWiFiTask,
		"WC",
		2048 / sizeof(StackType_t),
		NULL,
		9,
		NULL);
}

void HAL_FastConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	int len = ef_get_env_blob("fcdata", &fcdata, sizeof(obkFastConnectData_t), NULL);
	if(len == sizeof(obkFastConnectData_t))
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "We have fast connection data, connecting...");
		strcpy((char*)psk_essid, oob_ssid);
		strcpy((char*)psk_passphrase, fcdata.pwd);
		memcpy(wpa_global_PSK, fcdata.wpa_global_PSK, sizeof(fcdata.wpa_global_PSK));
		ConfigureSTA(ip);
		RegisterHandlers();

		g_wifiStatusCallback(WIFI_STA_CONNECTING);
		int ret = wifi_connect_bssid((unsigned char*)fcdata.bssid, (char*)oob_ssid, fcdata.security_type, fcdata.pwd,
			sizeof(fcdata.bssid), strlen(oob_ssid), strlen(fcdata.pwd), 0, NULL);
		if(ret != RTW_SUCCESS)
		{
			ADDLOG_ERROR(LOG_FEATURE_GENERAL, "Can't connect to AP");
			g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
			return;
		}
		if(g_bStaticIP == 0)
		{
			LwIP_DHCP(0, DHCP_START);
		}
		if(wifi_is_up(RTW_STA_INTERFACE) && g_powersave) wifi_enable_powersave();
		return;
	}
	else if(len)
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "Fast connect data len (%i) != saved len (%i)", sizeof(obkFastConnectData_t), len);
	}
	else
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "Fast connect data is empty, connecting normally");
	}
	HAL_ConnectToWiFi(oob_ssid, connect_key, ip);
}

void HAL_DisableEnhancedFastConnect()
{
	ef_del_env("fcdata");
}

void HAL_DisconnectFromWifi()
{
	printf("HAL_DisconnectFromWifi\r\n");
	if(wifi_is_connected_to_ap()) wifi_disconnect();
}

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	g_bOpenAccessPointMode = 1;
	rtw_mode_t mode = RTW_MODE_STA_AP;
	struct ip_addr ipaddr;
	struct ip_addr netmask;
	struct ip_addr gw;
	struct netif* pnetif = &xnetif[mode == RTW_MODE_STA_AP ? 1 : 0];
	dhcps_deinit();
	wifi_off();
	vTaskDelay(20);
	if(wifi_on(mode) < 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_GENERAL, "Failed to enable wifi");
		return 0;
	}

	if(wifi_start_ap((char*)ssid, RTW_SECURITY_OPEN, NULL, strlen(ssid), 0, 1) < 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_GENERAL, "Failed to start AP");
		return 0;
	}
	IP4_ADDR(ip_2_ip4(&ipaddr), 192, 168, 4, 1);
	IP4_ADDR(ip_2_ip4(&netmask), 255, 255, 255, 0);
	IP4_ADDR(ip_2_ip4(&gw), 192, 168, 4, 1);
	strcpy((char*)&g_IP, "192.168.4.1");
	strcpy((char*)&g_GW, "192.168.4.1");
	strcpy((char*)&g_MS, "255.255.255.0");
	netif_set_addr(pnetif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
	dhcps_init(pnetif);
	return 0;
}

#endif // PLATFORM_REALTEK
