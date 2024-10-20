#ifdef PLATFORM_ESPIDF

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
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"
#include "lwip/err.h"
#include "lwip/sys.h"

static void (*g_wifiStatusCallback)(int code);
static int g_bOpenAccessPointMode = 0;
static esp_netif_ip_info_t g_ip_info;
static esp_netif_t* sta_netif = NULL;
static esp_netif_t* ap_netif = NULL;
static char g_ip[16];
static char g_gw[16];
static char g_ms[16];

// This must return correct IP for both SOFT_AP and STATION modes,
// because, for example, javascript control panel requires it
const char* HAL_GetMyIPString()
{
	sprintf(g_ip, IPSTR, IP2STR(&g_ip_info.ip));
	return g_ip;
}

const char* HAL_GetMyGatewayString()
{
	sprintf(g_gw, IPSTR, IP2STR(&g_ip_info.gw));
	return g_gw;
}

const char* HAL_GetMyDNSString()
{
	return "error";
}

const char* HAL_GetMyMaskString()
{
	sprintf(g_ms, IPSTR, IP2STR(&g_ip_info.netmask));
	return g_ms;
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
	esp_read_mac(mac, ESP_MAC_BASE);
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
	wifi_ap_record_t ap;
	esp_wifi_sta_get_ap_info(&ap);
	return ap.rssi;
}

void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;
}

void event_handler(void* arg, esp_event_base_t event_base,
	int32_t event_id, void* event_data)
{
	if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START && !g_bOpenAccessPointMode)
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
	if(sta_netif != NULL)
	{
		esp_wifi_stop();
		esp_netif_destroy(sta_netif);
		esp_wifi_deinit();
		delay_ms(10);
	}
	else
	{
		esp_event_handler_instance_t instance_any_id, instance_got_ip;

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
		wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
		strncpy((char*)wifi_config.sta.ssid, (char*)oob_ssid, 32);
		strncpy((char*)wifi_config.sta.password, (char*)connect_key, 64);
	}

	esp_netif_set_hostname(sta_netif, CFG_GetDeviceName());

	esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
	esp_wifi_set_mode(WIFI_MODE_STA);

	esp_wifi_start();
}

void HAL_DisconnectFromWifi()
{
	esp_wifi_disconnect();
}

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	g_bOpenAccessPointMode = 1;
	ap_netif = esp_netif_create_default_wifi_ap();
	esp_netif_create_default_wifi_sta();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	cfg.nvs_enable = false;
	esp_wifi_init(&cfg);

	esp_event_handler_instance_t instance_any_id;

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
			.pmf_cfg =
			{
				.required = false,
			},
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
}

#endif // PLATFORM_ESPIDF
