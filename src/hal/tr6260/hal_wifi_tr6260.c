#ifdef PLATFORM_TR6260

#include "../hal_wifi.h"
#include "../../new_common.h"
#include "../../logging/logging.h"
#include <lwip/sockets.h>
#include <stdbool.h>
#include "system_event.h"
#include "system_wifi.h"
#include "system_wifi_def.h"

static void (*g_wifiStatusCallback)(int code) = NULL;
extern system_event_cb_t s_event_handler_cb;

bool g_bStaticIP = 0;

static struct ip_info if_ip;
static int g_bOpenAccessPointMode = 0;

const char* HAL_GetMyIPString()
{
	return ipaddr_ntoa(&if_ip.ip);
}

const char* HAL_GetMyGatewayString()
{
	return ipaddr_ntoa(&if_ip.gw);
}

const char* HAL_GetMyDNSString()
{
	return ipaddr_ntoa(&if_ip.dns1);
}

const char* HAL_GetMyMaskString()
{
	return ipaddr_ntoa(&if_ip.netmask);
}

int WiFI_SetMacAddress(char* mac)
{
	char macstr[18] = { 0 };
	sprintf(macstr, MAC_STR, MAC_VALUE((uint8_t*)mac));
	return !amt_mac_write(&macstr);
}

void WiFI_GetMacAddress(char* mac)
{
	get_netif_mac(STATION_IF, mac);
}

const char* HAL_GetMACStr(char* macstr)
{
	unsigned char mac[6];
	get_netif_mac(STATION_IF, mac);
	sprintf(macstr, MACSTR, MAC2STR(mac));
	return macstr;
}

void HAL_PrintNetworkInfo()
{
	uint8_t mac[6];
	WiFI_GetMacAddress(mac);
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "+--------------- net device info ------------+\r\n");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif type    : %-16s            |\r\n", g_bOpenAccessPointMode == 0 ? "STA" : "AP");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif rssi    = %-16i            |\r\n", HAL_GetWifiStrength());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif ip      = %-16s            |\r\n", HAL_GetMyIPString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif mask    = %-16s            |\r\n", HAL_GetMyMaskString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif gateway = %-16s            |\r\n", HAL_GetMyGatewayString());
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "|netif mac     : ["MAC_STR"] %-7s |\r\n", MAC_VALUE(mac), "");
	ADDLOG_DEBUG(LOG_FEATURE_GENERAL, "+--------------------------------------------+\r\n");
}

int HAL_GetWifiStrength()
{
	wifi_info_t ap_info;
	memset((void*)&ap_info, 0, sizeof(wifi_info_t));
	wifi_get_wifi_info(&ap_info);
	return ap_info.rssi;
}

char* HAL_GetWiFiBSSID(char* bssid){
	wifi_info_t ap_info;
	memset((void*)&ap_info, 0, sizeof(wifi_info_t));
	if (wifi_get_wifi_info(&ap_info) == SYS_OK){	
		sprintf(bssid, MACSTR, MAC2STR(ap_info.bssid));
		return bssid;
	}
	bssid[0]='\0';
	return bssid; 
};
uint8_t HAL_GetWiFiChannel(uint8_t *chan){
	wifi_info_t ap_info;
	memset((void*)&ap_info, 0, sizeof(wifi_info_t));
	if (wifi_get_wifi_info(&ap_info) == SYS_OK){
		*chan = ap_info.channel;
		return *chan;
	}
	return 0;
};


static sys_err_t handle_wifi_event(void* ctx, system_event_t* event)
{
	int vif;

	vif = event->vif;
	switch(event->event_id)
	{
		case SYSTEM_EVENT_STA_START:
		{
			if(g_wifiStatusCallback != NULL)
			{
				g_wifiStatusCallback(WIFI_STA_CONNECTING);
			}
		}
		case SYSTEM_EVENT_STA_CONNECTED: if(!g_bStaticIP) break;
		case SYSTEM_EVENT_STA_GOT_IP:
		{
			if(g_wifiStatusCallback != NULL)
			{
				g_wifiStatusCallback(WIFI_STA_CONNECTED);
			}
			wifi_get_ip_info(STATION_IF, &if_ip);
			break;
		}
		case SYSTEM_EVENT_STA_LOST_IP:
		case SYSTEM_EVENT_STA_DISCONNECTED:
		{
			if(g_wifiStatusCallback != NULL)
			{
				g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
			}
			//HAL_DisconnectFromWifi();
			//hal_lmac_reset_all();
			//HAL_RebootModule();
			break;
		}
		case SYSTEM_EVENT_STA_ASSOC_REJECT:
		case SYSTEM_EVENT_STA_4WAY_HS_FAIL:
		{
			if(g_wifiStatusCallback != NULL)
			{
				g_wifiStatusCallback(WIFI_STA_AUTH_FAILED);
			}
			break;
		}
		case SYSTEM_EVENT_AP_STACONNECTED:
		{
			if(g_wifiStatusCallback != NULL)
			{
				g_wifiStatusCallback(WIFI_AP_CONNECTED);
			}
			break;
		}
		default: break;
	}

	return SYS_OK;
}

void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;
	sys_event_loop_set_cb(handle_wifi_event, NULL);
}

void HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	g_bOpenAccessPointMode = 0;
	unsigned int sta_ip = 0, count = 0;
	while(!wifi_is_ready_full())
	{
		sys_delay_ms(10);
		system_printf("wifi not ready!\n");
	}
	wifi_set_opmode(WIFI_MODE_STA);
	wifi_remove_config_all(STATION_IF);
	wifi_remove_config_all(SOFTAP_IF);
	wifi_add_config(STATION_IF);
	wifi_config_ssid(STATION_IF, (unsigned char*)oob_ssid);
	wifi_set_password(STATION_IF, connect_key);
	wifi_config_commit(STATION_IF);
	if(ip->localIPAddr[0] == 0)
	{
		g_bStaticIP = 0;
	}
	else
	{
		g_bStaticIP = 1;

		IP4_ADDR(&if_ip.ip, ip->localIPAddr[0], ip->localIPAddr[1], ip->localIPAddr[2], ip->localIPAddr[3]);
		IP4_ADDR(&if_ip.gw, ip->gatewayIPAddr[0], ip->gatewayIPAddr[1], ip->gatewayIPAddr[2], ip->gatewayIPAddr[3]);
		IP4_ADDR(&if_ip.netmask, ip->netMask[0], ip->netMask[1], ip->netMask[2], ip->netMask[3]);
		IP4_ADDR(&if_ip.dns1, ip->dnsServerIpAddr[0], ip->dnsServerIpAddr[1], ip->dnsServerIpAddr[2], ip->dnsServerIpAddr[3]);
		set_sta_ipconfig(&if_ip);
	}
	wifi_set_status(STATION_IF, STA_STATUS_START);
	struct netif* nif = NULL;
	nif = get_netif_by_index(STATION_IF);
	netif_set_hostname(nif, CFG_GetDeviceName());
}

void HAL_DisconnectFromWifi()
{
	wifi_disconnect();
}

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	wifi_set_opmode(WIFI_MODE_AP_STA);
	int channel = 1, ret;
	ip_info_t ip_info;
	wifi_config_u config;
	uint8_t mac[6] = { 0 }, ip_part2, ip_part3;

	wifi_get_mac_addr(SOFTAP_IF, mac);
	ip_part2 = mac[4];
	ip_part3 = mac[5];

	memset(&config, 0, sizeof(config));
	strlcpy((char*)config.ap.ssid, ssid, sizeof(config.ap.ssid));
	config.ap.channel = channel;
	config.ap.authmode = AUTH_OPEN;

	while(!wifi_is_ready())
	{
		system_printf("wifi not ready!\n");
		delay_ms(10);
	}

	ret = wifi_start_softap(&config);
	if(SYS_OK != ret)
	{
		system_printf("HAL_SetupWiFiOpenAccessPoint failed, err: %d\n", ret);
		if(g_wifiStatusCallback != NULL)
		{
			g_wifiStatusCallback(WIFI_AP_FAILED);
		}
		return ret;
	}

	memset(&ip_info, 0, sizeof(ip_info));
	ip_info.ip.addr = ipaddr_addr((const char*)"192.168.4.1");
	ip_info.gw.addr = ipaddr_addr((const char*)"192.168.4.1");
	ip_info.netmask.addr = ipaddr_addr((const char*)"255.255.255.0");
	set_softap_ipconfig(&ip_info);

	struct dhcps_lease dhcp_cfg_info;
	dhcp_cfg_info.enable = true;
	dhcp_cfg_info.start_ip.addr = ipaddr_addr((const char*)"192.168.4.100");
	dhcp_cfg_info.end_ip.addr = ipaddr_addr((const char*)"192.168.4.150");

	wifi_softap_set_dhcps_lease(&dhcp_cfg_info);
	wifi_get_ip_info(SOFTAP_IF, &if_ip);
	if(g_wifiStatusCallback != NULL)
	{
		g_wifiStatusCallback(WIFI_AP_CONNECTED);
	}
	return 0;
}

#endif // PLATFORM_TR6260
