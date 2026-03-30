#if PLATFORM_BL_NEW
#include "../hal_wifi.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "stdbool.h"
#include "stdint.h"
#include "wifi_mgmr_ext.h"
#include "async_event.h"
#if PLATFORM_BL616
#include "fhost.h"
#include "fhost_api.h"
#include "wifi_mgmr.h"
#else
static wifi_conf_t conf = {
	.country_code = "CN",
};
#endif

static char g_ipStr[16] = "unknown";
static char g_gwStr[16] = "unknown";
static char g_maskStr[16] = "unknown";
static char g_dnsStr[16] = "unknown";
static int g_bAccessPointMode = 0;
static void (*g_wifiStatusCallback)(int code);
static bool g_wifi_init = false;
static bool wifi_init_done = 0;

const char* HAL_GetMyIPString()
{
	uint32_t ip;
	uint32_t gw;
	uint32_t mask;
	uint32_t dns1;

	if(g_bAccessPointMode == 1)
	{
		struct netif* netif = fhost_to_net_if(1);
		ip = netif_ip4_addr(netif)->addr;
		mask = netif_ip4_netmask(netif)->addr;
		gw = netif_ip4_gw(netif)->addr;
	}
	else
	{
		wifi_sta_ip4_addr_get(&ip, &gw, &mask, &dns1);
	}

	strcpy(g_ipStr, inet_ntoa(ip));
	strcpy(g_gwStr, inet_ntoa(gw));
	strcpy(g_maskStr, inet_ntoa(mask));
	strcpy(g_dnsStr, inet_ntoa(dns1));

	return g_ipStr;
}

const char* HAL_GetMyGatewayString()
{
	return g_gwStr;
}

const char* HAL_GetMyDNSString()
{
	return g_dnsStr;
}

const char* HAL_GetMyMaskString()
{
	return g_maskStr;
}

void WiFI_GetMacAddress(char* mac)
{
	wifi_mgmr_sta_mac_get((uint8_t*)mac);
}

const char* HAL_GetMACStr(char* macstr)
{
	uint8_t mac[6];
	if(g_bAccessPointMode == 1)
	{
		wifi_mgmr_ap_mac_get(mac);
	}
	else
	{
		wifi_mgmr_sta_mac_get(mac);
	}
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

void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;
}

void wifi_cb_task(void* arg)
{
	int evt = (int)arg;

	if(g_wifiStatusCallback)
	{
		g_wifiStatusCallback(evt);
	}

	vTaskDelete(NULL);
}

void wifi_event_handler(async_input_event_t ev, void* priv)
{
	uint32_t code = ev->code;

	switch(code)
	{
		case CODE_WIFI_ON_INIT_DONE:
#if PLATFORM_BL616
			wifi_mgmr_task_start();
#endif
			break;
		case CODE_WIFI_ON_MGMR_DONE: wifi_init_done = 1; break;
		case CODE_WIFI_ON_SCAN_DONE: break;
		case CODE_WIFI_ON_CONNECTING:
			xTaskCreate(wifi_cb_task, "wifi_cg", 256, (void*)WIFI_STA_CONNECTING, 10, NULL);
			break;
		case CODE_WIFI_ON_CONNECTED: break;
		case CODE_WIFI_ON_GOT_IP:
			xTaskCreate(wifi_cb_task, "wifi_ci", 512, (void*)WIFI_STA_CONNECTED, 10, NULL);
			break;
#ifdef CODE_WIFI_ON_GOT_IP_ABORT
		case CODE_WIFI_ON_GOT_IP_ABORT:
#endif
#ifdef CODE_WIFI_ON_GOT_IP_TIMEOUT
		case CODE_WIFI_ON_GOT_IP_TIMEOUT:
#endif
		case CODE_WIFI_ON_DISCONNECT:
			xTaskCreate(wifi_cb_task, "wifi_ct", 256, (void*)WIFI_STA_DISCONNECTED, 10, NULL);
			break;
		case CODE_WIFI_ON_AP_STARTED:
			printf("CODE_WIFI_ON_AP_STARTED\r\n"); break;
		case CODE_WIFI_ON_AP_STOPPED:
			printf("CODE_WIFI_ON_AP_STOPPED\r\n"); break;
		case CODE_WIFI_ON_AP_STA_ADD:
			printf("CODE_WIFI_ON_AP_STA_ADD\r\n"); break;
		case CODE_WIFI_ON_AP_STA_DEL:
			printf("CODE_WIFI_ON_AP_STA_DEL\r\n"); break;
		default:
		{
			printf("[APP] [EVT] Unknown code %u \r\n", code);
		}
	}
}

void wifi_start_firmware(void)
{
	printf("Starting wifi ...\r\n");
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

void HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	if(!g_wifi_init)
	{
		wifi_start_firmware();
	}
	g_wifi_init = true;
	if(ip->localIPAddr[0] == 0)
	{
		wifi_mgmr_sta_ip_set(0, 0, 0, 0
#if PLATFORM_BL602
			, 0
#endif
		);
	}
	else
	{
		wifi_mgmr_sta_ip_set(*(int*)ip->localIPAddr, *(int*)ip->netMask, *(int*)ip->gatewayIPAddr, *(int*)ip->dnsServerIpAddr
#if PLATFORM_BL602
			, 0
#endif
		);
	}
	struct netif* netif = fhost_to_net_if(0);
	if(netif)
	{
		netif_set_hostname(netif, CFG_GetDeviceName());
	}
#if PLATFORM_BL602
	wifi_interface_t wifi_interface = wifi_mgmr_sta_enable();
	wifi_mgmr_sta_connect_mid(wifi_interface, (char*)oob_ssid, (char*)connect_key, NULL, NULL, 0, 0, ip->localIPAddr[0] == 0 ? 1 : 0, WIFI_CONNECT_PMF_CAPABLE);
#else
	wifi_sta_connect((char*)oob_ssid, (char*)connect_key, NULL, NULL, 1, 0, 0, ip->localIPAddr[0] == 0 ? 1 : 0);
#endif
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
	if(!g_wifi_init)
	{
		wifi_start_firmware();
	}
	g_wifi_init = true;
#if PLATFORM_BL602
	wifi_mgmr_sta_enable();
	wifi_interface_t wifi_if = wifi_mgmr_ap_enable();
	wifi_mgmr_ap_start(wifi_if, ssid, 0, NULL, 1);
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

#endif