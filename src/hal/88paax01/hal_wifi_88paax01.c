#include "../../new_common.h"
#include "../hal_wifi.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../httpserver/new_http.h"
#include "hal_88paax01.h"
#include "hw.h"
#include "timer.h"
#include "net.h"
#include "dhcp.h"
#include "dhcpd.h"
#include "dnsd.h"
#include "wifi.h"

static void (*g_wifi_status_cb)(int code) = NULL;
static char g_ip_str[32]   = "0.0.0.0";
static char g_gw_str[32]   = "0.0.0.0";
static char g_mask_str[32] = "255.255.255.0";
static char g_dns_str[32]  = "0.0.0.0";
static char g_mac_str[32]  = "00:00:00:00:00:00";

static int  g_wifi_hw_ready    = 0;
static int  g_wifi_ap_mode     = 0;
static int  g_wifi_sta_active  = 0;
static int  g_wifi_sta_up      = 0;
static int  g_reported_up      = 0;
static int  g_sta_has_static   = 0;
static uint8_t g_sta_static_ip[4];
static uint8_t g_sta_static_mask[4];
static uint8_t g_sta_static_gw[4];
static uint8_t g_sta_static_dns[4];

static char     g_want_ssid[64];
static char     g_want_pass[64];
static int      g_want_join        = 0;
static uint32_t g_last_join_ms     = 0;

/* Ring buffer receiving Ethernet II frames from fw/app/wifi.c's net_input() */
#define BCM_RX_RING_SLOTS 8
static uint8_t  g_rx_ring[BCM_RX_RING_SLOTS][1536];
static uint32_t g_rx_len[BCM_RX_RING_SLOTS];
static unsigned g_rx_head = 0;
static unsigned g_rx_tail = 0;

static void sync_primary_from_wifi(void)
{
	if (memcmp(net_ip(), net_wifi_ip(), 4) != 0)
		net_set_ip(net_wifi_ip());
	if (memcmp(net_netmask(), net_wifi_netmask(), 4) != 0)
		net_set_netmask(net_wifi_netmask());
	if (memcmp(net_gateway(), net_wifi_gateway(), 4) != 0)
		net_set_gateway(net_wifi_gateway());
	if (memcmp(net_dns(), net_wifi_dns(), 4) != 0)
		net_set_dns(net_wifi_dns());
}

/* Called by fw/app/wifi.c when an Ethernet II data frame arrives over SDIO */
void net_input(const uint8_t *frame, uint32_t len)
{
	if (!frame || len < 14 || len > 1536)
		return;
	unsigned next = (g_rx_head + 1u) % BCM_RX_RING_SLOTS;
	if (next == g_rx_tail)
		return;
	memcpy(g_rx_ring[g_rx_head], frame, len);
	g_rx_len[g_rx_head] = len;
	g_rx_head = next;
}

/* Called by fw/app/wifi.c when STA or AP link state changes */
void net_link_change(int up, int ap_mode)
{
	if (!up) {
		g_wifi_sta_up = 0;
		g_wifi_ap_mode = 0;
		dhcp_wifi_stop();
		static const uint8_t zero[4] = { 0, 0, 0, 0 };
		net_set_wifi_ip(zero);
		net_set_ip(zero);
		return;
	}
	if (ap_mode) {
		static const uint8_t ap_ip[4]   = { 192, 168, 4, 1 };
		static const uint8_t ap_mask[4] = { 255, 255, 255, 0 };
		net_set_wifi_ip(ap_ip);
		net_set_wifi_netmask(ap_mask);
		net_set_wifi_gateway(ap_ip);
		net_set_wifi_dns(ap_ip);
		sync_primary_from_wifi();
		dhcpd_start(ap_ip, ap_mask, 100, 8);
		dnsd_start(ap_ip);
		g_wifi_ap_mode = 1;
	} else {
		g_wifi_sta_up = 1;
		g_wifi_ap_mode = 0;
		if (g_sta_has_static) {
			net_set_wifi_ip(g_sta_static_ip);
			net_set_wifi_netmask(g_sta_static_mask);
			net_set_wifi_gateway(g_sta_static_gw);
			net_set_wifi_dns(g_sta_static_dns);
			sync_primary_from_wifi();
		} else {
			const char *hn = CFG_GetShortDeviceName();
			dhcp_wifi_start((hn && hn[0]) ? hn : "Open88PAAX01");
		}
	}
}

/* Called by fw/app/wifi.c and fw/app/ui.c */
int net_has_address(void)
{
	if (wifi_mode() == WIFI_MODE_STA && !g_sta_has_static) {
		dhcp_wifi_poll();
		if (dhcp_wifi_bound())
			sync_primary_from_wifi();
	}
	const uint8_t *wip = net_wifi_ip();
	return (wip[0] | wip[1] | wip[2] | wip[3]) != 0;
}

void net_format_ip(const uint8_t ip[4], char out[16])
{
	snprintf(out, 16, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

static int bcm_link_send(const void *frame, uint32_t len, uint8_t port_mask)
{
	(void)port_mask;
	if (!g_wifi_hw_ready)
		return -1;
	return wifi_send((const uint8_t *)frame, len);
}

static int bcm_link_recv(uint8_t **frame_out, uint32_t *len_out, int *port_out)
{
	if (!g_wifi_hw_ready)
		return 0;
	if (g_rx_tail == g_rx_head)
		wifi_poll();
	if (g_rx_tail == g_rx_head)
		return 0;
	*frame_out = g_rx_ring[g_rx_tail];
	*len_out   = g_rx_len[g_rx_tail];
	*port_out  = NET_PORT_WIFI;
	return 1;
}

static void bcm_link_recv_done(void)
{
	if (g_rx_tail != g_rx_head)
		g_rx_tail = (g_rx_tail + 1u) % BCM_RX_RING_SLOTS;
}

static const struct net_link g_bcm_link = {
	.send      = bcm_link_send,
	.recv      = bcm_link_recv,
	.recv_done = bcm_link_recv_done,
	.reclaim   = 0,
};

static int ensure_bcm_hw_up(void)
{
	if (g_wifi_hw_ready)
		return 0;
	printf("[88PAAX01] Initializing Broadcom BCM43143 SDIO Wi-Fi...\n");
	int rc = wifi_ensure_running();
	if (rc != 0) {
		printf("[88PAAX01] wifi_ensure_running() failed (%d)\n", rc);
		return rc;
	}
	const uint8_t *mac = wifi_mac();
	net_set_wifi_link(&g_bcm_link, mac);
	net_init(mac, net_ip());
	g_wifi_hw_ready = 1;
	printf("[88PAAX01] BCM43143 up, MAC=%02X:%02X:%02X:%02X:%02X:%02X\n",
	       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return 0;
}

void HAL_WiFi_Init_88PAAX01(void)
{
	ensure_bcm_hw_up();
}

int HAL_WiFi_IsApMode(void)
{
	return g_wifi_ap_mode || (wifi_mode() == WIFI_MODE_AP);
}

void HAL_WiFi_Poll_88PAAX01(void)
{
	if (!g_wifi_hw_ready)
		return;

	wifi_poll();

	if (g_wifi_sta_active && !g_wifi_ap_mode) {
		uint32_t now = timer_uptime_ms();
		if (g_want_join && now >= 2500u &&
		    (g_last_join_ms == 0 || (now - g_last_join_ms) > 15000u)) {
			g_last_join_ms = now ? now : 1;
			g_want_join = 0;
			printf("[88PAAX01] Joining SSID '%s'...\n", g_want_ssid);
			wifi_join(g_want_ssid, g_want_pass[0] ? g_want_pass : NULL);
		}
		if (wifi_mode() == WIFI_MODE_STA && !g_sta_has_static) {
			dhcp_wifi_poll();
			if (dhcp_wifi_bound())
				sync_primary_from_wifi();
		}
	}

	int is_up = (wifi_mode() == WIFI_MODE_AP) || net_has_address();
	if (is_up && !g_reported_up) {
		g_reported_up = 1;
		if (g_wifi_status_cb)
			g_wifi_status_cb(WIFI_STA_CONNECTED);
	} else if (!is_up && g_reported_up) {
		g_reported_up = 0;
		if (g_wifi_status_cb)
			g_wifi_status_cb(WIFI_STA_DISCONNECTED);
	}
}

void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifi_status_cb = cb;
}

void HAL_ConnectToWiFi(const char *oob_ssid, const char *connect_key, obkStaticIP_t *ip)
{
	if (!oob_ssid || !oob_ssid[0])
		return;

	if (ensure_bcm_hw_up() != 0)
		return;

	if (g_wifi_ap_mode) {
		dnsd_stop();
		dhcpd_stop();
		wifi_leave();
		g_wifi_ap_mode = 0;
	}

	if (ip && (ip->localIPAddr[0] | ip->localIPAddr[1] | ip->localIPAddr[2] | ip->localIPAddr[3])) {
		g_sta_has_static = 1;
		memcpy(g_sta_static_ip,   ip->localIPAddr,     4);
		memcpy(g_sta_static_mask, ip->netMask,         4);
		memcpy(g_sta_static_gw,   ip->gatewayIPAddr,   4);
		memcpy(g_sta_static_dns,  ip->dnsServerIpAddr, 4);
	} else {
		g_sta_has_static = 0;
	}

	strncpy(g_want_ssid, oob_ssid, sizeof(g_want_ssid) - 1);
	g_want_ssid[sizeof(g_want_ssid) - 1] = 0;
	if (connect_key) {
		strncpy(g_want_pass, connect_key, sizeof(g_want_pass) - 1);
		g_want_pass[sizeof(g_want_pass) - 1] = 0;
	} else {
		g_want_pass[0] = 0;
	}

	g_wifi_sta_active = 1;
	g_wifi_sta_up     = 0;
	g_want_join       = 1;
	g_last_join_ms    = 0;
	g_reported_up     = 0;
}

void HAL_DisconnectFromWifi(void)
{
	dhcp_wifi_stop();
	if (g_wifi_hw_ready)
		wifi_leave();
	g_wifi_sta_active = 0;
	g_wifi_sta_up     = 0;
	g_want_join       = 0;
	g_reported_up     = 0;
	if (g_wifi_status_cb)
		g_wifi_status_cb(WIFI_STA_DISCONNECTED);
}

int HAL_SetupWiFiOpenAccessPoint(const char *ssid)
{
	const char *ap_ssid = (ssid && ssid[0]) ? ssid : "Open88PAAX01";
	if (ensure_bcm_hw_up() != 0)
		return -1;

	dhcp_wifi_stop();
	g_wifi_sta_active = 0;
	g_wifi_sta_up     = 0;
	g_want_join       = 0;

	if (wifi_start_ap(ap_ssid, NULL, 6) == 0) {
		g_reported_up = 1;
		printf("[88PAAX01] Soft-AP '%s' active on 192.168.4.1/24\n", ap_ssid);
	}
	return 0;
}

void HAL_FastConnectToWiFi(const char *oob_ssid, const char *connect_key, obkStaticIP_t *ip)
{
	HAL_ConnectToWiFi(oob_ssid, connect_key, ip);
}

void HAL_DisableEnhancedFastConnect(void)
{
}

const char *HAL_GetMyIPString(void)
{
	const uint8_t *ip = net_wifi_ip();
	if (!(ip[0] | ip[1] | ip[2] | ip[3]))
		ip = net_ip();
	snprintf(g_ip_str, sizeof(g_ip_str), "%u.%u.%u.%u",
	         ip[0], ip[1], ip[2], ip[3]);
	return g_ip_str;
}

const char *HAL_GetMyGatewayString(void)
{
	const uint8_t *gw = net_wifi_gateway();
	if (!(gw[0] | gw[1] | gw[2] | gw[3]))
		gw = net_gateway();
	snprintf(g_gw_str, sizeof(g_gw_str), "%u.%u.%u.%u",
	         gw[0], gw[1], gw[2], gw[3]);
	return g_gw_str;
}

const char *HAL_GetMyDNSString(void)
{
	const uint8_t *dns = net_wifi_dns();
	if (!(dns[0] | dns[1] | dns[2] | dns[3]))
		dns = net_dns();
	snprintf(g_dns_str, sizeof(g_dns_str), "%u.%u.%u.%u",
	         dns[0], dns[1], dns[2], dns[3]);
	return g_dns_str;
}

const char *HAL_GetMyMaskString(void)
{
	const uint8_t *m = net_wifi_netmask();
	snprintf(g_mask_str, sizeof(g_mask_str), "%u.%u.%u.%u",
	         m[0], m[1], m[2], m[3]);
	return g_mask_str;
}

const char *HAL_GetMACStr(char *macstr)
{
	unsigned char mac[6];
	HAL_GetMACWiFi(mac);
	sprintf(macstr, "%02X%02X%02X%02X%02X%02X",
	        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return macstr;
}

void WiFI_GetMacAddress(char *mac)
{
	HAL_GetMACWiFi((unsigned char *)mac);
}

int WiFI_SetMacAddress(char *mac)
{
	(void)mac;
	return 0;
}

void HAL_PrintNetworkInfo(void)
{
	unsigned char mac[6];
	HAL_GetMACWiFi(mac);
	snprintf(g_mac_str, sizeof(g_mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
	         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	ADDLOG_INFO(LOG_FEATURE_GENERAL, "[88PAAX01] IP=%s GW=%s MASK=%s MAC=%s",
	            HAL_GetMyIPString(), HAL_GetMyGatewayString(),
	            HAL_GetMyMaskString(), g_mac_str);
}

char *HAL_GetWiFiBSSID(char *bssid)
{
	if (!bssid)
		return 0;
	unsigned char mac[6];
	HAL_GetMACWiFi(mac);
	snprintf(bssid, 20, "%02X:%02X:%02X:%02X:%02X:%02X",
	         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return bssid;
}

uint8_t HAL_GetWiFiChannel(uint8_t *chan)
{
	uint8_t ch = 6;
	if (chan)
		*chan = ch;
	return ch;
}

int wifi_rssi_get(void)
{
	int r = wifi_last_rssi();
	return r ? r : -50;
}

int HAL_GetWifiStrength(void)
{
	return wifi_rssi_get();
}

void HAL_WiFi_Scan_HTML(http_request_t *request)
{
	if (ensure_bcm_hw_up() != 0) {
		poststr(request, "Broadcom BCM43143 Wi-Fi hardware not available.<br>");
		return;
	}
	uint32_t count = 0;
	int best_rssi = 0;
	if (wifi_scan_summary(&count, &best_rssi) != 0 || count == 0) {
		poststr(request, "No networks found on 2.4 GHz (or scan busy).<br>");
		return;
	}
	hprintf255(request,
	           "<p>Broadcom BCM43143 scan found <b>%u</b> network(s), strongest signal <b>%d dBm</b>. "
	           "Run <code>wifi scan</code> on the console for full BSSID/channel list.</p>",
	           (unsigned)count, best_rssi);
}
