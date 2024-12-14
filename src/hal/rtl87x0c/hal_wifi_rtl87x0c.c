#ifdef PLATFORM_RTL87X0C

#include "../hal_wifi.h"
#include "../../new_common.h"
#include <lwip/sockets.h>
#include <lwip/netif.h>
#include <wifi/wifi_conf.h>
#include <wifi/wifi_util.h>
#include <lwip_netconf.h>
#include "tcpip.h"
#include <dhcp/dhcps.h>

extern struct netif xnetif[NET_IF_NUM];

static void (*g_wifiStatusCallback)(int code) = NULL;

bool g_STA_static_IP = 0;

static int g_bOpenAccessPointMode = 0;

const char* HAL_GetMyIPString()
{
	return ipaddr_ntoa(&xnetif[g_bOpenAccessPointMode].ip_addr.addr);
}

const char* HAL_GetMyGatewayString()
{
	return ipaddr_ntoa(&xnetif[g_bOpenAccessPointMode].gw.addr);
}

const char* HAL_GetMyDNSString()
{
	return NULL;
}

const char* HAL_GetMyMaskString()
{
	return ipaddr_ntoa(&xnetif[g_bOpenAccessPointMode].netmask.addr);
}

int WiFI_SetMacAddress(char* mac)
{
	printf("WiFI_SetMacAddress");
	return 0; // error
}

void WiFI_GetMacAddress(char* mac)
{
	//wifi_get_mac_address(mac);
	// https://github.com/libretiny-eu/libretiny/blob/0f6c31386bee9cd3f44c68a4d7edf45bec683743/cores/realtek-ambz2/base/api/lt_device.c#L7
	efuse_logical_read(0x11A, 6, mac);
}

const char* HAL_GetMACStr(char* macstr)
{
	unsigned char mac[6];
	WiFI_GetMacAddress(mac);
	sprintf(macstr, MACSTR, MAC2STR(mac));
	return macstr;
}

void HAL_PrintNetworkInfo()
{
	uint8_t mac[6];
	WiFI_GetMacAddress(mac);
	bk_printf("+--------------- net device info ------------+\r\n");
	bk_printf("|netif type    : %-16s            |\r\n", g_bOpenAccessPointMode == 0 ? "STA" : "AP");
	bk_printf("|netif ip      = %-16s            |\r\n", HAL_GetMyIPString());
	bk_printf("|netif mask    = %-16s            |\r\n", HAL_GetMyMaskString());
	bk_printf("|netif gateway = %-16s            |\r\n", HAL_GetMyGatewayString());
	bk_printf("|netif mac     : [%02X:%02X:%02X:%02X:%02X:%02X] %-7s |\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], "");
	bk_printf("+--------------------------------------------+\r\n");
}

int HAL_GetWifiStrength()
{
	int rssi;
	wext_get_rssi(WLAN0_NAME, &rssi);
	return rssi;
}

void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	printf("HAL_WiFi_SetupStatusCallback");
	g_wifiStatusCallback = cb;
}

static int _find_ap_from_scan_buf(char* buf, int buflen, char* target_ssid, void* user_data)
{
	rtw_wifi_setting_t* pwifi = (rtw_wifi_setting_t*)user_data;
	int plen = 0;

	while(plen < buflen)
	{
		u8 len, ssid_len, security_mode;
		char* ssid;

		// len offset = 0
		len = (int)*(buf + plen);
		// check end
		if(len == 0)
		{
			break;
		}
		// ssid offset = 14
		ssid_len = len - 14;
		ssid = buf + plen + 14;
		if((ssid_len == strlen(target_ssid))
			&& (!memcmp(ssid, target_ssid, ssid_len)))
		{
			strncpy((char*)pwifi->ssid, target_ssid, 33);
			// channel offset = 13
			pwifi->channel = *(buf + plen + 13);
			// security_mode offset = 11
			security_mode = (u8) * (buf + plen + 11);
			if(security_mode == IW_ENCODE_ALG_NONE)
			{
				pwifi->security_type = RTW_SECURITY_OPEN;
			}
			else if(security_mode == IW_ENCODE_ALG_WEP)
			{
				pwifi->security_type = RTW_SECURITY_WEP_PSK;
			}
			else if(security_mode == IW_ENCODE_ALG_CCMP)
			{
				pwifi->security_type = RTW_SECURITY_WPA2_AES_PSK;
			}
			else if(security_mode == IW_ENCODE_ALG_OWE)
			{
				pwifi->security_type = RTW_SECURITY_WPA3_OWE;
			}
			break;
		}
		plen += len;
	}
	return 0;
}

static int _get_ap_security_mode(char* ssid, rtw_security_t* security_mode, u8* channel)
{
	rtw_wifi_setting_t wifi;
	u32 scan_buflen = 1000;

	memset(&wifi, 0, sizeof(wifi));

	if(wifi_scan_networks_with_ssid(_find_ap_from_scan_buf, (void*)&wifi, scan_buflen, ssid, strlen(ssid)) != RTW_SUCCESS)
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
	}
}

void wifi_con_hdl(u8* buf, u32 buf_len, u32 flags, void* userdata)
{
	if(g_wifiStatusCallback != NULL)
	{
		g_wifiStatusCallback(WIFI_STA_CONNECTING);
	}
}

void wifi_conned_hdl(u8* buf, u32 buf_len, u32 flags, void* userdata)
{
	if(g_wifiStatusCallback != NULL)
	{
		g_wifiStatusCallback(WIFI_STA_CONNECTED);
	}
}

void wifi_af_hdl(u8* buf, u32 buf_len, u32 flags, void* userdata)
{
	if(g_wifiStatusCallback != NULL)
	{
		g_wifiStatusCallback(WIFI_STA_AUTH_FAILED);
	}
}

typedef struct
{
	char ssid[32];
	char pwd[64];
} wifi_data_t;
static wifi_data_t wdata = { 0 };
void ConnectToWiFiTask(void* args)
{
	rtw_security_t security_type = RTW_SECURITY_OPEN;
	int security_retry_count = 0, connect_channel = 0;
	while(1)
	{
		if(_get_ap_security_mode(wdata.ssid, &security_type, &connect_channel))
		{
			break;
		}
		security_retry_count++;
		if(security_retry_count >= 3)
		{
			printf("Can't get AP security mode and channel.\r\n");
			goto exit;
		}
	}
	if(g_wifiStatusCallback != NULL)
	{
		g_wifiStatusCallback(WIFI_STA_CONNECTING);
	}
	netif_set_hostname(&xnetif[0], CFG_GetDeviceName());
	wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, (rtw_event_handler_t)wifi_dis_hdl, NULL);
	//wifi_reg_event_handler(WIFI_EVENT_CONNECT, (rtw_event_handler_t)wifi_con_hdl, NULL);
	wifi_reg_event_handler(WIFI_EVENT_STA_GOT_IP, (rtw_event_handler_t)wifi_conned_hdl, NULL);
	wifi_reg_event_handler(WIFI_EVENT_CHALLENGE_FAIL, (rtw_event_handler_t)wifi_af_hdl, NULL);
	int ret = wifi_connect(wdata.ssid, security_type, wdata.pwd, strlen(wdata.ssid),
		strlen(wdata.pwd), NULL, NULL);
	if(ret != RTW_SUCCESS)
	{
		printf("ERROR: Can't connect to AP\r\n");
		goto exit;
	}
	LwIP_DHCP(0, DHCP_START);
exit:
	delay_ms(500);
	vTaskDelete(NULL);
}

void HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	g_bOpenAccessPointMode = 0;
	strcpy(&wdata.ssid, oob_ssid);
	strcpy(&wdata.pwd, connect_key);
	wifi_set_autoreconnect(0);

	xTaskCreate(
		(TaskFunction_t)ConnectToWiFiTask,
		"WC",
		2048 / sizeof(StackType_t),
		NULL,
		5,
		NULL);
}

void HAL_DisconnectFromWifi()
{
	printf("HAL_DisconnectFromWifi");
	wifi_disconnect();
}

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	g_bOpenAccessPointMode = 1;
	struct ip_addr ipaddr;
	struct ip_addr netmask;
	struct ip_addr gw;
	struct netif* pnetif = &xnetif[0];
	dhcps_deinit();
	wifi_off();
	vTaskDelay(20);
	if(wifi_on(RTW_MODE_AP) < 0)
	{
		printf("wifi_on failed!\r\n");
		return 0;
	}

	if(wifi_start_ap(ssid, RTW_SECURITY_OPEN, NULL, strlen(ssid), 0, 1) < 0)
	{
		printf("wifi_start_ap failed!\r\n");
	}
	IP4_ADDR(ip_2_ip4(&ipaddr), 192, 168, 4, 1);
	IP4_ADDR(ip_2_ip4(&netmask), 255, 255, 255, 0);
	IP4_ADDR(ip_2_ip4(&gw), 192, 168, 4, 1);
	netif_set_addr(pnetif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
	dhcps_init(pnetif);
	return 0;
}

#endif // PLATFORM_RTL87X0C
