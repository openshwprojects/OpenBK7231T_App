#if PLATFORM_ESPIDF || PLATFORM_ESP8266

#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../new_common.h"
#include "../hal_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "dhcpserver/dhcpserver.h"

#if PLATFORM_ESPIDF
#include "esp_mac.h"
#include "esp_netif_net_stack.h"
static esp_netif_t* sta_netif = NULL;
static esp_netif_t* ap_netif = NULL;
#else
#include "tcpip_adapter.h"
#define ESP_MAC_BASE ESP_MAC_WIFI_STA
#define esp_netif_ip_info_t tcpip_adapter_ip_info_t
#define esp_event_handler_instance_t esp_event_handler_t
#define esp_event_handler_instance_register(a,b,c,d,e) esp_event_handler_register(a,b,c,e)
#define esp_event_handler_instance_unregister(a,b,c) esp_event_handler_unregister(a,b,c)
#define esp_netif_t tcpip_adapter_if_t
#define esp_netif_set_hostname tcpip_adapter_set_hostname
#define esp_netif_get_ip_info tcpip_adapter_get_ip_info
#define esp_netif_create_default_wifi_ap() TCPIP_ADAPTER_IF_AP
#define esp_netif_create_default_wifi_sta() TCPIP_ADAPTER_IF_STA
#define esp_netif_destroy(...)
static tcpip_adapter_if_t sta_netif = TCPIP_ADAPTER_IF_STA;
static tcpip_adapter_if_t ap_netif = TCPIP_ADAPTER_IF_AP;
#endif

static void (*g_wifiStatusCallback)(int code);
// is (Open-) Access point or a client?
// included as "extern uint8_t g_WifiMode;" from new_common.h
// initilized in user_main.c
// values:	0 = STA	1 = OpenAP	2 = WAP-AP
static esp_netif_ip_info_t g_ip_info;
esp_event_handler_instance_t instance_any_id, instance_got_ip;
bool handlers_registered = false;

// This must return correct IP for both SOFT_AP and STATION modes,
// because, for example, javascript control panel requires it
const char* HAL_GetMyIPString()
{
	return ipaddr_ntoa((ip4_addr_t*)&g_ip_info.ip);
}

const char* HAL_GetMyGatewayString()
{
	return ipaddr_ntoa((ip4_addr_t*)&g_ip_info.gw);
}

const char* HAL_GetMyDNSString()
{
#if PLATFORM_ESP8266
	ip4_addr_t dns = dhcps_dns_getserver();
	return ipaddr_ntoa(&dns);
#else
	return "error";
#endif
}

const char* HAL_GetMyMaskString()
{
	return ipaddr_ntoa((ip4_addr_t*)&g_ip_info.netmask);
}

int WiFI_SetMacAddress(char* mac)
{
	return 0;
}

void WiFI_GetMacAddress(char* mac)
{
	esp_read_mac((unsigned char*)mac, ESP_MAC_BASE);
}

const char* HAL_GetMACStr(char* macstr)
{
	uint8_t mac[6];
	esp_read_mac(mac, ESP_MAC_BASE);
	sprintf(macstr, MACSTR, MAC2STR(mac));
	return macstr;
}

void HAL_PrintNetworkInfo()
{
	uint8_t mac[6];
	WiFI_GetMacAddress((char*)&mac);
	bk_printf("+--------------- net device info ------------+\r\n");
	bk_printf("|netif type    : %-16s            |\r\n", g_WifiMode == 0 ? "STA" : "AP");
	bk_printf("|netif rssi    = %-16i            |\r\n", HAL_GetWifiStrength());
	bk_printf("|netif ip      = %-16s            |\r\n", HAL_GetMyIPString());
	bk_printf("|netif mask    = %-16s            |\r\n", HAL_GetMyMaskString());
	bk_printf("|netif gateway = %-16s            |\r\n", HAL_GetMyGatewayString());
	bk_printf("|netif mac     : ["MACSTR"] %-6s  |\r\n", MAC2STR(mac), "");
	bk_printf("+--------------------------------------------+\r\n");
}

int HAL_GetWifiStrength()
{
	wifi_ap_record_t ap;
	esp_wifi_sta_get_ap_info(&ap);
	return ap.rssi;
}

char* HAL_GetWiFiBSSID(char* bssid){
	wifi_ap_record_t ap_info;
	memset((void*)&ap_info, 0, sizeof(wifi_ap_record_t));
	if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK){	
		sprintf(bssid, MACSTR, MAC2STR(ap_info.bssid));
		return bssid;
	}
	bssid[0]='\0';
	return bssid; 
};
uint8_t HAL_GetWiFiChannel(uint8_t *chan){
	wifi_ap_record_t ap_info;
	memset((void*)&ap_info, 0, sizeof(wifi_ap_record_t));
	if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK){
		*chan = ap_info.primary;
		return *chan;
	}
	return 0;
};


void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;
}

void event_handler(void* arg, esp_event_base_t event_base,
	int32_t event_id, void* event_data)
{
	if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START && g_WifiMode==0)
	{
		if(g_wifiStatusCallback != NULL)
		{
			g_wifiStatusCallback(WIFI_STA_CONNECTING);
		}
		ADDLOG_INFO(LOG_FEATURE_MAIN, "WiFi Connecting...");
		esp_wifi_connect();
	}
	else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
	{
		if(g_wifiStatusCallback != NULL)
		{
			g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
		}
		ADDLOG_INFO(LOG_FEATURE_MAIN, "WiFi Disconnected");
	}
	else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
	{
		ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
		g_ip_info = event->ip_info;
		if(g_wifiStatusCallback != NULL)
		{
			g_wifiStatusCallback(WIFI_STA_CONNECTED);
		}
	}
	else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
	{
		if(g_wifiStatusCallback != NULL)
		{
			g_wifiStatusCallback(WIFI_AP_CONNECTED);
		}
	}
}

void HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
#if PLATFORM_ESPIDF
	if(sta_netif != NULL)
#else
	if(1)
#endif
	{
		esp_wifi_stop();
		esp_netif_destroy(sta_netif);
		esp_wifi_deinit();
		delay_ms(50);
	}

	if(!handlers_registered)
	{
		esp_event_handler_instance_register(WIFI_EVENT,
			ESP_EVENT_ANY_ID,
			&event_handler,
			NULL,
			&instance_any_id);
		esp_event_handler_instance_register(IP_EVENT,
			IP_EVENT_STA_GOT_IP,
			&event_handler,
			NULL,
			&instance_got_ip);
		handlers_registered = true;
	}

	sta_netif = esp_netif_create_default_wifi_sta();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);

	wifi_config_t wifi_config;
	esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
	if(strcmp((char*)wifi_config.sta.ssid, oob_ssid) != 0 || strcmp((char*)wifi_config.sta.password, connect_key) != 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "WiFi saved ssid/pass != current, resetting");
		memset(&wifi_config.sta, 0, sizeof(wifi_sta_config_t));
		//wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
		strncpy((char*)wifi_config.sta.ssid, (char*)oob_ssid, 32);
		strncpy((char*)wifi_config.sta.password, (char*)connect_key, 64);
	}
#if CONFIG_ESP8266_WIFI_ENABLE_WPA3_SAE
	wifi_config.pmf_cfg.capable = true;
#endif
	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

	esp_wifi_start();

	esp_netif_set_hostname(sta_netif, CFG_GetDeviceName());
}

void HAL_DisconnectFromWifi()
{
	esp_wifi_disconnect();
}

#if ENABLE_WPA_AP
int HAL_SetupWiFiAccessPoint(const char* ssid, const char* key)
{
#if PLATFORM_ESPIDF
	if(sta_netif != NULL)
#else
	if(1)
#endif
	{
		esp_wifi_stop();
		esp_netif_destroy(sta_netif);
		esp_wifi_deinit();
		delay_ms(50);
	}
	ap_netif = esp_netif_create_default_wifi_ap();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	cfg.nvs_enable = false;
	esp_wifi_init(&cfg);

	if(!handlers_registered)
	{
		esp_event_handler_instance_register(WIFI_EVENT,
			ESP_EVENT_ANY_ID,
			&event_handler,
			NULL,
			&instance_any_id);
		handlers_registered = true;
	}
	wifi_config_t wifi_ap_config =
	{
		.ap =
		{
			.ssid_len = strlen(ssid),
			.channel = g_wifi_channel,
			.max_connection = AP_STA_CLIENTS,
			.authmode = (! key || key[0] == 0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_PSK,
#if !PLATFORM_ESP8266
			.pmf_cfg =
			{
				.required = false,
			},
#endif
		},
	};

	wifi_config_t wifi_sta_config =
	{
		.sta = { },
	};
	strncpy((char*)wifi_ap_config.ap.ssid, (char*)ssid, 32);
	if ( key && key[0] != 0 ) strncpy((char*)wifi_ap_config.ap.password, (char*)key, 64);
	esp_netif_set_hostname(ap_netif, CFG_GetDeviceName());

	esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config);
	esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config);
	esp_wifi_set_mode(WIFI_MODE_AP);
	esp_wifi_set_max_tx_power(10 * 4);
	esp_wifi_start();

	//esp_netif_set_default_netif(ap_netif);

	esp_netif_get_ip_info(ap_netif, &g_ip_info);

	return 1;
}
#endif

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
#if !ENABLE_WPA_AP

	g_WifiMode = 1; 	// 0 = STA	1 = OpenAP	2 = WAP-AP 
	ap_netif = esp_netif_create_default_wifi_ap();
	sta_netif = esp_netif_create_default_wifi_sta();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	cfg.nvs_enable = false;
	esp_wifi_init(&cfg);

	esp_event_handler_instance_register(WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&event_handler,
		NULL,
		&instance_any_id);

	wifi_config_t wifi_ap_config =
	{
		.ap =
		{
			.ssid_len = strlen(ssid),
			.channel = 1,
			.max_connection = 1,
			.authmode = WIFI_AUTH_OPEN,
#if !PLATFORM_ESP8266
			.pmf_cfg =
			{
				.required = false,
			},
#endif
		},
	};

	wifi_config_t wifi_sta_config =
	{
		.sta = { },
	};
	strncpy((char*)wifi_ap_config.ap.ssid, (char*)ssid, 32);
	esp_netif_set_hostname(ap_netif, CFG_GetDeviceName());

	esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config);
	esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config);
	esp_wifi_set_mode(WIFI_MODE_APSTA);
	esp_wifi_set_max_tx_power(10 * 4);
	esp_wifi_start();

	//esp_netif_set_default_netif(ap_netif);

	esp_netif_get_ip_info(ap_netif, &g_ip_info);

	return 1;
#else
	return HAL_SetupWiFiAccessPoint(ssid, NULL);
#endif
}

#endif // PLATFORM_ESPIDF
