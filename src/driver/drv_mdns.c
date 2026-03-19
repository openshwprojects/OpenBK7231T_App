#include "../new_common.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../obk_config.h"
#include "drv_mdns.h"

#if (PLATFORM_BK7231N || PLATFORM_BK7231T || PLATFORM_BK7231U || PLATFORM_BK7238 || PLATFORM_BK7252 || PLATFORM_BK7252N || PLATFORM_W600 || PLATFORM_W800 || PLATFORM_BL602 || PLATFORM_LN882H || PLATFORM_LN8825 || PLATFORM_RTL87X0C || PLATFORM_TR6260 || PLATFORM_ECR6600 || (PLATFORM_XRADIO && !PLATFORM_XR872 && !__CONFIG_LWIP_V1) || PLATFORM_ESP8266 || (PLATFORM_ESPIDF && (CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32C2 || CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C5 || CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32C61 || CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3))) && ENABLE_DRIVER_MDNS

#include "lwip/apps/mdns_opts.h"

#if LWIP_MDNS_RESPONDER

#include "lwip/apps/mdns.h"
#include "lwip/err.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#if PLATFORM_ESPIDF
#include "esp_netif.h"
#include "esp_netif_net_stack.h"
#endif
#if PLATFORM_TR6260 || PLATFORM_ECR6600
#include "system_wifi.h"
#endif
#if PLATFORM_XRADIO
#include "common/framework/net_ctrl.h"
#endif
#if PLATFORM_BK7231N || PLATFORM_BK7231T || PLATFORM_BK7231U || PLATFORM_BK7238 || PLATFORM_BK7252 || PLATFORM_BK7252N
#include "net.h"
#endif
#include "lwip/tcpip.h"

#ifndef LOCK_TCPIP_CORE
#define LOCK_TCPIP_CORE()
#endif

#ifndef UNLOCK_TCPIP_CORE
#define UNLOCK_TCPIP_CORE()
#endif

extern int Main_IsConnectedToWiFi();
extern int DRV_MDNS_Active;

static int g_mdnsInitDone = 0;
static int g_mdnsNetifAdded = 0;
static s8_t g_mdnsServiceSlot = -1;

#if (LWIP_VERSION_MAJOR > 2) || (LWIP_VERSION_MAJOR == 2 && LWIP_VERSION_MINOR >= 1)
#define DRV_MDNS_HAS_RESTART_API 1
#else
#define DRV_MDNS_HAS_RESTART_API 0
#endif

static struct netif *DRV_MDNS_GetStaNetif(void) {
#if PLATFORM_LN882H || PLATFORM_LN8825
	extern struct netif *get_connected_nif(void);
	return get_connected_nif();
#elif PLATFORM_ESPIDF
	esp_netif_t *staEspNetif;
	staEspNetif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
	if (staEspNetif == 0) {
		return 0;
	}
	return (struct netif *)esp_netif_get_netif_impl(staEspNetif);
#elif PLATFORM_ESP8266
	extern struct netif *netif_default;
	return netif_default;
#elif PLATFORM_XRADIO
	return g_wlan_netif;
#elif PLATFORM_BL602
	extern struct netif *wifi_mgmr_sta_netif_get(void);
	return wifi_mgmr_sta_netif_get();
#elif PLATFORM_RTL87X0C
	extern struct netif xnetif[];
	return &xnetif[0];
#elif PLATFORM_TR6260 || PLATFORM_ECR6600
	return get_netif_by_index(STATION_IF);
#elif PLATFORM_W600 || PLATFORM_W800
	extern struct netif *tls_get_netif(void);
	return tls_get_netif();
#else
	return (struct netif *)net_get_sta_handle();
#endif
}

static void DRV_MDNS_StartOrRestart(void) {
	struct netif *netif;
	const char *hostName;
	err_t err;

	netif = DRV_MDNS_GetStaNetif();
	if (netif == 0) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_HTTP, "DRV_MDNS: no STA netif handle");
		return;
	}

	hostName = CFG_GetShortDeviceName();
	if (hostName == 0 || *hostName == 0) {
		hostName = "openbeken";
	}

	LOCK_TCPIP_CORE();

	if (!g_mdnsInitDone) {
		mdns_resp_init();
		g_mdnsInitDone = 1;
	}

	if (!g_mdnsNetifAdded) {
#if PLATFORM_ESPIDF
		err = mdns_resp_add_netif(netif, hostName);
#else
		err = mdns_resp_add_netif(netif, hostName, 120);
#endif
		if (err == ERR_OK) {
			g_mdnsNetifAdded = 1;
#if PLATFORM_ESPIDF
			g_mdnsServiceSlot = mdns_resp_add_service(netif, hostName, "_http", DNSSD_PROTO_TCP, 80, 0, 0);
#else
			g_mdnsServiceSlot = mdns_resp_add_service(netif, hostName, "_http", DNSSD_PROTO_TCP, 80, 120, 0, 0);
#endif
			if (g_mdnsServiceSlot < 0) {
				addLogAdv(LOG_ERROR, LOG_FEATURE_HTTP, "DRV_MDNS: mdns_resp_add_service failed");
			}
#if DRV_MDNS_HAS_RESTART_API
			mdns_resp_announce(netif);
#else
			mdns_resp_netif_settings_changed(netif);
#endif
			addLogAdv(LOG_INFO, LOG_FEATURE_HTTP, "DRV_MDNS: responder started (%s.local)", hostName);
		} else {
			addLogAdv(LOG_ERROR, LOG_FEATURE_HTTP, "DRV_MDNS: mdns_resp_add_netif failed %d", (int)err);
		}
	} else {
#if DRV_MDNS_HAS_RESTART_API
		mdns_resp_restart(netif);
		mdns_resp_announce(netif);
#else
		mdns_resp_netif_settings_changed(netif);
#endif
		addLogAdv(LOG_INFO, LOG_FEATURE_HTTP, "DRV_MDNS: responder restarted");
	}

	UNLOCK_TCPIP_CORE();
}

void DRV_MDNS_Init(void) {
	DRV_MDNS_Active = 1;

	if (!Main_IsConnectedToWiFi()) {
		addLogAdv(LOG_INFO, LOG_FEATURE_HTTP, "DRV_MDNS_Init - no wifi, await connection");
		return;
	}

	DRV_MDNS_StartOrRestart();
}

void DRV_MDNS_RunEverySecond(void) {
}

void DRV_MDNS_RunQuickTick(void) {
}

void DRV_MDNS_Shutdown(void) {
	struct netif *netif;

	addLogAdv(LOG_INFO, LOG_FEATURE_HTTP, "DRV_MDNS_Shutdown");

	if (g_mdnsNetifAdded) {
		netif = DRV_MDNS_GetStaNetif();
		if (netif != 0) {
			LOCK_TCPIP_CORE();
			mdns_resp_remove_netif(netif);
			UNLOCK_TCPIP_CORE();
		}
		g_mdnsNetifAdded = 0;
		g_mdnsServiceSlot = -1;
	}

	DRV_MDNS_Active = 0;
}

#else

extern int DRV_MDNS_Active;

void DRV_MDNS_Init(void) {
	addLogAdv(LOG_INFO, LOG_FEATURE_HTTP, "DRV_MDNS unavailable in this SDK build");
	DRV_MDNS_Active = 0;
}

void DRV_MDNS_RunEverySecond(void) {
}

void DRV_MDNS_RunQuickTick(void) {
}

void DRV_MDNS_Shutdown(void) {
	DRV_MDNS_Active = 0;
}

#endif

#else

extern int DRV_MDNS_Active;

void DRV_MDNS_Init(void) {
	DRV_MDNS_Active = 0;
}

void DRV_MDNS_RunEverySecond(void) {
}

void DRV_MDNS_RunQuickTick(void) {
}

void DRV_MDNS_Shutdown(void) {
	DRV_MDNS_Active = 0;
}

#endif
