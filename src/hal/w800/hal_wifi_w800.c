#if defined(PLATFORM_W800) || defined(PLATFORM_W600)

#include "../hal_wifi.h"

#define LOG_FEATURE LOG_FEATURE_MAIN

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "lwip/netdb.h"
#include "lwip/netif.h"
#include "wm_netif.h"

static void (*g_wifiStatusCallback)(int code);

// lenght of "192.168.103.103" is 15 but we also need a NULL terminating character
static char g_IP[32] = "unknown";
static int g_bOpenAccessPointMode = 0;

const char* security_names[] = {
	"OPEN",
	"WEP40",
	"WEP104",
	"WPA_PSK_TKIP",
	"WPA_PSK_CCMP",
	"WPA2_PSK_TKIP",
	"WPA2_PSK_CCMP",
	"WPA_PSK_TKIP&AES",
	"WPA2_PSK_TKIP&AES",
};

// This must return correct IP for both SOFT_AP and STATION modes,
// because, for example, javascript control panel requires it
const char* HAL_GetMyIPString()
{
	struct netif* netif = tls_get_netif();
	strcpy(g_IP, inet_ntoa(netif->ip_addr));
	return g_IP;
}

const char* HAL_GetMyGatewayString() {
	return "192.168.0.1";
}
const char* HAL_GetMyDNSString() {
	return "192.168.0.1";
}
const char* HAL_GetMyMaskString() {
	return "255.255.255.0";
}

////////////////////
// NOTE: this gets the STA mac

int WiFI_SetMacAddress(char* mac)
{
	return 0;
}

void WiFI_GetMacAddress(char* mac)
{
	struct netif* netif = tls_get_netif();
	MEMCPY(mac, &netif->hwaddr[0], ETH_ALEN);
}
const char* HAL_GetMACStr(char* macstr)
{
	unsigned char mac[6] = { 0, 1, 2, 3, 4, 5 };
	WiFI_GetMacAddress(mac);

	sprintf(macstr, MACSTR, MAC2STR(mac));
	return macstr;
}

void print_security_type(int type)
{
	if (type >= IEEE80211_ENCRYT_NONE && type <= IEEE80211_ENCRYT_AUTO_WPA2) {
		bk_printf("%s\r\n", security_names[type]);
	}
	else {
		bk_printf("Error\r\n");
	}
}

void HAL_PrintNetworkInfo()
{
	char macstr[3 * 6 + 1];
	struct tls_curr_bss_t bss;
	unsigned char mac[6] = { 0, 1, 2, 3, 4, 5 };

	struct netif* netif = tls_get_netif();
	MEMCPY(mac, &netif->hwaddr, ETH_ALEN);
	snprintf(macstr, sizeof(macstr), MACSTR, MAC2STR(mac));

	tls_wifi_get_current_bss(&bss);
	bss.ssid[bss.ssid_len]=0;

	struct tls_ethif* tmpethif = tls_netif_get_ethif();
	char buffer[256];
	char ip[16] = {0};
	strcpy(ip, inet_ntoa(tmpethif->ip_addr));
	char gw[16] = {0};
	strcpy(gw, inet_ntoa(tmpethif->gw));
	char netmask[16] = {0};
	strcpy(netmask, inet_ntoa(tmpethif->netmask));
	char dns[16] = {0};
	strcpy(dns, inet_ntoa(tmpethif->dns1));
	snprintf(buffer, 256, 	"Network info:\r\n"
				"\tsta:rssi=%d, SSID=%s, BSSID=" MACSTR ", channel=%d, encr=%s\r\n"
				"\tIP=%s, GW=%s, MASK=%s, MAC=%s, DNS=%s\r\n",
				bss.rssi, bss.ssid, MAC2STR(bss.bssid), bss.channel, 
				( bss.encryptype >=  IEEE80211_ENCRYT_NONE && bss.encryptype <= IEEE80211_ENCRYT_AUTO_WPA2) ? security_names[bss.encryptype] : "-",
				 ip, gw, netmask, macstr, dns );
	bk_printf(buffer);
	// do we need this in web Log?
	// disable for now
//	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, buffer);
}

int HAL_GetWifiStrength()
{
	struct tls_curr_bss_t bss;
	tls_wifi_get_current_bss(&bss);
	return (signed char)(0x100-bss.rssi); //this is adjusted according to example
}

static void apsta_net_status(u8 status)
{
	switch (status)
	{
	case NETIF_WIFI_JOIN_SUCCESS:
		// wm_printf("apsta_net_status: NETIF_WIFI_JOIN_SUCCESS\n");
		if (g_wifiStatusCallback != 0)
		{
			g_wifiStatusCallback(WIFI_STA_CONNECTED);
		}
		break;

	case NETIF_WIFI_JOIN_FAILED:
		// wm_printf("apsta_net_status: sta join net failed\n");
		if (g_wifiStatusCallback != 0)
		{
			g_wifiStatusCallback(WIFI_STA_AUTH_FAILED);
		}
		break;
	case NETIF_WIFI_DISCONNECTED:
		// wm_printf("apsta_net_status: sta net disconnected\n");
		if (g_wifiStatusCallback != 0)
		{
			g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
		}
		// tls_os_queue_send(ApstaDemoTaskQueue, (void *)APSTA_DEMO_CMD_SOFTAP_CLOSE, 0);
		break;
	case NETIF_IP_NET_UP:
		// wm_printf("\napsta_net_status: sta ip: %v %v %v\n", netif->ip_addr.addr, netif->netmask, netif->gw);
		// tls_os_queue_send(ApstaDemoTaskQueue, (void *)APSTA_DEMO_CMD_SOFTAP_CREATE, 0);
		break;
	case NETIF_WIFI_SOFTAP_FAILED:
		// wm_printf("apsta_net_status: softap create failed\n");
		break;
	case NETIF_WIFI_SOFTAP_CLOSED:
		// wm_printf("apsta_net_status: softap closed\n");
		break;
	case NETIF_IP_NET2_UP:
		// wm_printf("\napsta_net_status: softap ip: %v\n", netif->next->ip_addr.addr);
		break;
	default:
		break;
	}
}

void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;
	tls_netif_add_status_event(apsta_net_status);
}
static int connect_wifi_demo(char* ssid, char* pwd, obkStaticIP_t *ip)
{
	int ret;
	struct tls_param_ip* ip_param = NULL;
	u8 wireless_protocol = 0;

	tls_wifi_disconnect();
	tls_wifi_softap_destroy();

	#if defined(PLATFORM_W800)
	tls_wifi_set_oneshot_flag(0);
	#endif

	tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void*)&wireless_protocol, TRUE);
	if (TLS_PARAM_IEEE80211_INFRA != wireless_protocol)
	{
		tls_wifi_softap_destroy();
		wireless_protocol = TLS_PARAM_IEEE80211_INFRA;
		tls_param_set(TLS_PARAM_ID_WPROTOCOL, (void*)&wireless_protocol, FALSE);
	}

	ip_param = tls_mem_alloc(sizeof(struct tls_param_ip));
	if (ip_param)
	{
		tls_param_get(TLS_PARAM_ID_IP, ip_param, FALSE);
		if (ip->localIPAddr[0] == 0) {
			ip_param->dhcp_enable = TRUE;
		}
		else {
			ip_param->dhcp_enable = FALSE;
			MEMCPY(ip_param->ip, ip->localIPAddr, 4);
			MEMCPY(ip_param->netmask, ip->netMask, 4);
			MEMCPY(ip_param->gateway, ip->gatewayIPAddr, 4);
			MEMCPY(ip_param->dns1, ip->dnsServerIpAddr, 4);
			MEMCPY(ip_param->dns2, ip->dnsServerIpAddr, 4);
		}
		tls_param_set(TLS_PARAM_ID_IP, ip_param, FALSE);
		tls_mem_free(ip_param);
	}

	ret = tls_wifi_connect((u8*)ssid, strlen(ssid), (u8*)pwd, strlen(pwd));
	if (WM_SUCCESS == ret)
		wm_printf("\nplease wait connect net......\n");
	else
		wm_printf("\napsta connect net failed, please check configure......\n");

	return ret;
}
void HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t *ip)
{
	g_bOpenAccessPointMode = 0;
	connect_wifi_demo(oob_ssid, connect_key, ip);
}

void HAL_DisconnectFromWifi()
{

}


/*1)Add sta add callback function
  2)Add sta list monitor task*/
static tls_os_timer_t* sta_monitor_tim = NULL;
static u32 totalstanum = 0;
static void demo_monitor_stalist_tim(void* ptmr, void* parg)
{
	u8* stabuf = NULL;
	u32 stanum = 0;
	int i = 0;
	stabuf = tls_mem_alloc(1024);
	if (stabuf)
	{
		tls_wifi_get_authed_sta_info(&stanum, stabuf, 1024);
		if (totalstanum != stanum)
		{
			wm_printf("sta mac:\n");
			for (i = 0; i < stanum; i++)
			{
				wm_printf("%M\n", &stabuf[i * 6]);
			}
		}
		totalstanum = stanum;
		tls_mem_free(stabuf);
		stabuf = NULL;
	}
}

int demo_create_softap(u8* ssid, u8* key, int chan, int encrypt, int format)
{
	struct tls_softap_info_t* apinfo;
	struct tls_ip_info_t* ipinfo;
	u8 ret = 0;
	u8 ssid_set = 0;
	u8 wireless_protocol = 0;

	u8 ssid_len = 0;
	if (!ssid)
	{
		return WM_FAILED;
	}

	ipinfo = tls_mem_alloc(sizeof(struct tls_ip_info_t));
	if (!ipinfo)
	{
		return WM_FAILED;
	}
	apinfo = tls_mem_alloc(sizeof(struct tls_softap_info_t));
	if (!apinfo)
	{
		tls_mem_free(ipinfo);
		return WM_FAILED;
	}

	tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void*)&wireless_protocol, TRUE);
	if (TLS_PARAM_IEEE80211_SOFTAP != wireless_protocol)
	{
		wireless_protocol = TLS_PARAM_IEEE80211_SOFTAP;
		tls_param_set(TLS_PARAM_ID_WPROTOCOL, (void*)&wireless_protocol, FALSE);
	}

	#if defined(PLATFORM_W800)
	tls_wifi_set_oneshot_flag(0); /*disable oneshot*/
	#endif

	tls_param_get(TLS_PARAM_ID_BRDSSID, (void*)&ssid_set, (bool)0);
	if (0 == ssid_set)
	{
		ssid_set = 1;
		tls_param_set(TLS_PARAM_ID_BRDSSID, (void*)&ssid_set, (bool)1); /*set bssid broadcast flag*/
	}

	tls_wifi_disconnect();

	ssid_len = strlen((const char*)ssid);
	MEMCPY(apinfo->ssid, ssid, ssid_len);
	apinfo->ssid[ssid_len] = '\0';

	apinfo->encrypt = encrypt;                           /*0:open, 1:wep64, 2:wep128,3:TKIP WPA ,4: CCMP WPA, 5:TKIP WPA2 ,6: CCMP WPA2*/
	apinfo->channel = chan;                              /*channel*/
	apinfo->keyinfo.format = format;                     /*key's format:0-HEX, 1-ASCII*/
	apinfo->keyinfo.index = 1;                           /*wep key index*/
	apinfo->keyinfo.key_len = strlen((const char*)key); /*key length*/
	MEMCPY(apinfo->keyinfo.key, key, apinfo->keyinfo.key_len);
	/*ip info:ipaddress, netmask, dns*/
	ipinfo->ip_addr[0] = 192;
	ipinfo->ip_addr[1] = 168;
	ipinfo->ip_addr[2] = 4;
	ipinfo->ip_addr[3] = 1;
	ipinfo->netmask[0] = 255;
	ipinfo->netmask[1] = 255;
	ipinfo->netmask[2] = 255;
	ipinfo->netmask[3] = 0;
	MEMCPY(ipinfo->dnsname, "local.wm", sizeof("local.wm"));

	ret = tls_wifi_softap_create(apinfo, ipinfo);
	wm_printf("\n ap create %s ! \n", (ret == WM_SUCCESS) ? "Successfully" : "Error");

	if (WM_SUCCESS == ret)
	{
		if (sta_monitor_tim)
		{
			tls_os_timer_delete(sta_monitor_tim);
			sta_monitor_tim = NULL;
		}
		tls_os_timer_create(&sta_monitor_tim, demo_monitor_stalist_tim, NULL, 500, TRUE, NULL);
		tls_os_timer_start(sta_monitor_tim);
	}

	tls_mem_free(ipinfo);
	tls_mem_free(apinfo);
	return ret;
}

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	demo_create_softap(ssid, "", 15, 0, 1);

	// dhcp_server_start(0);
	// dhcp_server_stop(void);

	return 0;
}

#endif
