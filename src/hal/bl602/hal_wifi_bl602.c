#if PLATFORM_BL602 || PLATFORM_BL616

#include "../hal_wifi.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../../logging/logging.h"
#include <easyflash.h>

#include <wifi_mgmr_ext.h>
#if PLATFORM_BL_NEW
#include "async_event.h"
#define input_event_t async_input_event_t
extern void wifi_task_create(void);
#if PLATFORM_BL616
#include "fhost.h"
#include "fhost_api.h"
#include "wifi_mgmr.h"
#define MGMR_IP_SET(a,b,c,d,e) wifi_mgmr_sta_ip_set(a,b,c,d)
#else
#define MGMR_IP_SET(a,b,c,d,e) wifi_mgmr_sta_ip_set(a,b,c,d,e)
static wifi_conf_t conf = {
	.country_code = "CN",
};
extern int fhost_init();
#endif
#else
#include <hal_sys.h>
#include <bl_wifi.h>
#include <bl60x_fw_api.h>
#include <aos/kernel.h>
#include <aos/yloop.h>
#define MGMR_IP_SET(a,b,c,d,e) wifi_mgmr_sta_ip_set(a,b,c,d,e)
#endif

static char g_ipStr[16] = "unknown";
static char g_gwStr[16] = "unknown";
static char g_maskStr[16] = "unknown";
static char g_dnsStr[16] = "unknown";

static int g_bAccessPointMode = 0;
static void (*g_wifiStatusCallback)(int code);

extern bool g_powersave;
__attribute__((aligned(32))) static obkFastConnectData_t fcdata = { 0 };
extern uint16_t phy_channel_to_freq(uint8_t band, int channel);

#if PLATFORM_BL_NEW
static bool g_wifi_init = false;
static bool wifi_init_done = 0;
static void wifi_event_handler(async_input_event_t event, void* private_data);

void wifi_start_firmware(void)
{
	async_register_event_filter(EV_WIFI, wifi_event_handler, NULL);
	wifi_task_create();
	fhost_init();
#if PLATFORM_BL616
	wifi_mgmr_init();
#else
	wifi_mgmr_init(&conf);
#endif
	while(!wifi_init_done)
	{
		vTaskDelay(1);
	}
}

void wifi_cb_task(void* arg)
{
	if(g_wifiStatusCallback)
	{
		g_wifiStatusCallback((int)arg);
	}
	vTaskDelete(NULL);
}

#endif

const char* HAL_GetMyIPString()
{
	uint32_t ip, gw, mask, dns1 = 0;

	if(g_bAccessPointMode)
	{
#if PLATFORM_BL602
		wifi_mgmr_ap_ip_get(&ip, &gw, &mask);
#else
		struct netif* netif = fhost_to_net_if(1);
		ip = netif_ip4_addr(netif)->addr;
		mask = netif_ip4_netmask(netif)->addr;
		gw = netif_ip4_gw(netif)->addr;
#endif
	}
	else
	{
		wifi_sta_ip4_addr_get(&ip, &gw, &mask, &dns1);
	}

	strcpy(g_ipStr, inet_ntoa(ip));
	strcpy(g_gwStr, inet_ntoa(gw));
	strcpy(g_maskStr, inet_ntoa(mask));
	if(dns1) strcpy(g_dnsStr, inet_ntoa(dns1));

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

void WiFI_GetMacAddress(char* mac)
{
	wifi_mgmr_sta_mac_get((uint8_t*)mac);
}

const char* HAL_GetMACStr(char* macstr)
{
	uint8_t mac[6];
	if(g_bAccessPointMode)
		wifi_mgmr_ap_mac_get(mac);
	else
		wifi_mgmr_sta_mac_get(mac);

	sprintf(macstr, MACSTR, MAC2STR(mac));
	return macstr;
}

int HAL_GetWifiStrength()
{
	int rssi = -1;
#if PLATFORM_BL602
	wifi_mgmr_rssi_get(&rssi);
#else
	wifi_mgmr_sta_rssi_get(&rssi);
#endif
	return rssi;
}

#if PLATFORM_BL602
// Get WiFi Information (SSID / BSSID) - e.g. to display on status page 
// use bl_wifi_sta_info_get(bl_wifi_ap_info_t* sta_info); or bl_wifi_ap_info_get(bl_wifi_ap_info_t* ap_info);
//        ef_get_env_blob((const char *)WIFI_AP_PSM_INFO_CHANNEL, val_buf, val_len, NULL);
//        ef_get_env_blob((const char *)WIFI_AP_PSM_INFO_BSSID, val_buf, val_len, NULL);
//
/*
// ATM there is only one SSID, so need for this code
char* HAL_GetWiFiSSID(char* ssid){
	wifi_mgmr_sta_connect_ind_stat_info_t info;
	memset(&info, 0, sizeof(info));
	wifi_mgmr_sta_connect_ind_stat_get(&info);
//	memcpy(ssid, info.ssid, sizeof(ssid));
	strcpy(ssid, info.ssid);
	return ssid;
};
*/
char* HAL_GetWiFiBSSID(char* bssid)
{
	wifi_mgmr_sta_connect_ind_stat_info_t info;
	memset(&info, 0, sizeof(info));
	wifi_mgmr_sta_connect_ind_stat_get(&info);
	//	memcpy(bssid, info.bssid, sizeof(bssid));
	sprintf(bssid, MACSTR, MAC2STR(info.bssid));
	return bssid;
};
uint8_t HAL_GetWiFiChannel(uint8_t* chan)
{
	wifi_mgmr_sta_connect_ind_stat_info_t info;
	memset(&info, 0, sizeof(info));
	wifi_mgmr_sta_connect_ind_stat_get(&info);
	*chan = info.chan_band;
	return *chan;
};
int WiFI_SetMacAddress(char* mac)
{
	wifi_mgmr_sta_mac_set((uint8_t*)mac);
	CFG_SetMac(mac);
	return 1;

}
#endif

#if PLATFORM_BL_NEW
static void wifi_event_handler(async_input_event_t event, void* private_data)
#else
static void wifi_event_handler(input_event_t* event, void* private_data)
#endif
{
	uint32_t code = event->code;

	switch(code)
	{
		case CODE_WIFI_ON_INIT_DONE:
#if PLATFORM_BL616
			wifi_mgmr_task_start();
#endif
			break;
		case CODE_WIFI_ON_MGMR_DONE:
#if PLATFORM_BL_NEW
			wifi_init_done = 1;
#endif
			break;
		case CODE_WIFI_ON_SCAN_DONE:
			break;
		case CODE_WIFI_ON_CONNECTING:
		{
#if !PLATFORM_BL616
			if(g_wifiStatusCallback != 0)
			{
				g_wifiStatusCallback(WIFI_STA_CONNECTING);
			}
#else
			xTaskCreate(wifi_cb_task, "wifi_cg", 256, (void*)WIFI_STA_CONNECTING, 10, NULL);
#endif
		}
		break;

		case CODE_WIFI_ON_CONNECTED:
			break;
		case CODE_WIFI_ON_GOT_IP:
		{
#if !PLATFORM_BL616
			if(g_wifiStatusCallback != 0)
			{
				g_wifiStatusCallback(WIFI_STA_CONNECTED);
			}
#else
			xTaskCreate(wifi_cb_task, "wifi_ci", 512, (void*)WIFI_STA_CONNECTED, 10, NULL);
#endif

#if PLATFORM_BL602
			if(g_powersave) wifi_mgmr_sta_ps_enter(2);

			if(CFG_HasFlag(OBK_FLAG_WIFI_ENHANCED_FAST_CONNECT))
			{
				wifi_mgmr_sta_connect_ind_stat_info_t info;
				wifi_mgmr_sta_connect_ind_stat_get(&info);
				ef_get_env_blob("fcdata", &fcdata, sizeof(obkFastConnectData_t), NULL);
				if(strcmp((const char*)info.passphr, fcdata.pwd) != 0 ||
					memcmp(&info.bssid, fcdata.bssid, 6) != 0 ||
					info.chan_id != fcdata.channel)
				{
					ADDLOG_INFO(LOG_FEATURE_GENERAL, "Saved fast connect data differ to current one, saving...");
					memset(&fcdata, 0, sizeof(obkFastConnectData_t));
					strcpy(fcdata.pwd, (const char*)info.passphr);
					fcdata.channel = info.chan_id;
					wifi_mgmr_psk_cal(info.passphr, info.ssid, strlen(info.ssid), fcdata.psk);
					memcpy(&fcdata.bssid, (char*)&info.bssid, sizeof(fcdata.bssid));
					ef_set_env_blob("fcdata", &fcdata, sizeof(obkFastConnectData_t));
				}
			}
#endif
		}
		break;

#ifdef CODE_WIFI_ON_GOT_IP_ABORT
		case CODE_WIFI_ON_GOT_IP_ABORT:
#endif
#ifdef CODE_WIFI_ON_GOT_IP_TIMEOUT
		case CODE_WIFI_ON_GOT_IP_TIMEOUT:
#endif
		case CODE_WIFI_ON_DISCONNECT:
		{
#if !PLATFORM_BL616
			if(g_wifiStatusCallback != 0)
			{
				g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
			}
#else
			xTaskCreate(wifi_cb_task, "wifi_ct", 256, (void*)WIFI_STA_DISCONNECTED, 10, NULL);
#endif
		}
		break;

		case CODE_WIFI_CMD_RECONNECT:
		case CODE_WIFI_ON_PRE_GOT_IP:
		case CODE_WIFI_ON_PROV_SSID:
		case CODE_WIFI_ON_PROV_BSSID:
		case CODE_WIFI_ON_PROV_PASSWD:
		case CODE_WIFI_ON_PROV_CONNECT:
		case CODE_WIFI_ON_PROV_DISCONNECT:
			break;
		case CODE_WIFI_ON_AP_STARTED:
		case CODE_WIFI_ON_AP_STOPPED:
		case CODE_WIFI_ON_AP_STA_ADD:
		case CODE_WIFI_ON_AP_STA_DEL:
			break;
		default:
		{
			printf("[APP] [EVT] Unknown code %u\r\n", (unsigned int)code);
		}
	}
}

void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;

#if !PLATFORM_BL_NEW
	aos_register_event_filter(EV_WIFI, wifi_event_handler, NULL);
#endif
}

void HAL_ConnectToWiFi(const char* ssid, const char* psk, obkStaticIP_t* ip)
{
#if PLATFORM_BL_NEW
	if(!g_wifi_init)
	{
		wifi_start_firmware();
		g_wifi_init = true;
	}
#endif

	if(ip->localIPAddr[0] == 0)
	{
		MGMR_IP_SET(0, 0, 0, 0, 0);
	}
	else
	{
		MGMR_IP_SET(*(int*)ip->localIPAddr, *(int*)ip->netMask, *(int*)ip->gatewayIPAddr, *(int*)ip->dnsServerIpAddr, 0);
	}

#if PLATFORM_BL602
	if(g_powersave) wifi_mgmr_sta_ps_exit();

	wifi_interface_t wifi_interface = wifi_mgmr_sta_enable();
	wifi_mgmr_sta_connect_mid(wifi_interface,
		(char*)ssid,
		(char*)psk,
		NULL, NULL,
		0, 0,
		ip->localIPAddr[0] == 0 ? 1 : 0,
		WIFI_CONNECT_PMF_CAPABLE);
#else
	wifi_sta_connect((char*)ssid, (char*)psk, NULL, NULL, 1, 0, 0, ip->localIPAddr[0] == 0 ? 1 : 0);
#endif

	g_bAccessPointMode = 0;
}

void HAL_DisconnectFromWifi()
{
#if PLATFORM_BL602
	wifi_mgmr_sta_disconnect();
#else
	wifi_sta_disconnect();
#endif
}

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	g_bAccessPointMode = 1;

#if PLATFORM_BL_NEW
	if(!g_wifi_init)
	{
		wifi_start_firmware();
		g_wifi_init = true;
	}
#endif

#if PLATFORM_BL602
	wifi_interface_t wifi_if = wifi_mgmr_ap_enable();
	wifi_mgmr_ap_start(wifi_if, (char*)ssid, 0, NULL, 1);
#else
	wifi_mgmr_ap_params_t config = { 0 };
	config.ssid = (char*)ssid;
	config.key = "";
	//config.akm = NULL;
	config.channel = 1;
	config.ap_ipaddr = inet_addr("192.168.4.1");
	config.ap_mask = inet_addr("255.255.255.0");
	config.use_dhcpd = true;
	config.use_ipcfg = true;
	config.start = 100;
	config.limit = 100;
	wifi_mgmr_ap_start(&config);
#endif
	return 0;
}

#if PLATFORM_BL602
void HAL_FastConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	int len = ef_get_env_blob("fcdata", &fcdata, sizeof(obkFastConnectData_t), NULL);
	if(len == sizeof(obkFastConnectData_t))
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "We have fast connection data, connecting...");
		if(ip->localIPAddr[0] == 0)
		{
			MGMR_IP_SET(0, 0, 0, 0, 0);
		}
		else
		{
			MGMR_IP_SET(*(int*)ip->localIPAddr, *(int*)ip->netMask, *(int*)ip->gatewayIPAddr, *(int*)ip->dnsServerIpAddr, 0);
		}

		struct ap_connect_adv ext_param = { 0 };
		char psk[65] = { 0 };
		memcpy(psk, fcdata.psk, 64);
		ext_param.psk = psk;
		ext_param.ap_info.type = AP_INFO_TYPE_SUGGEST;
		ext_param.ap_info.time_to_live = 30;
		ext_param.ap_info.bssid = (uint8_t*)fcdata.bssid;
		ext_param.ap_info.band = 0;
		ext_param.ap_info.freq = phy_channel_to_freq(0, fcdata.channel);
		ext_param.ap_info.use_dhcp = ip->localIPAddr[0] == 0 ? 1 : 0;
		ext_param.flags = WIFI_CONNECT_PMF_CAPABLE | WIFI_CONNECT_STOP_SCAN_ALL_CHANNEL_IF_TARGET_AP_FOUND | WIFI_CONNECT_STOP_SCAN_CURRENT_CHANNEL_IF_TARGET_AP_FOUND;

#if PLATFORM_BL_NEW
		if(!g_wifi_init)
		{
			wifi_start_firmware();
			g_wifi_init = true;
		}
#endif
		if(g_powersave) wifi_mgmr_sta_ps_exit();
		wifi_interface_t wifi_interface;
		wifi_interface = wifi_mgmr_sta_enable();
		wifi_mgmr_sta_connect_ext(wifi_interface, (char*)oob_ssid, (char*)connect_key, &ext_param);
		g_bAccessPointMode = 0;
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
#endif

#endif