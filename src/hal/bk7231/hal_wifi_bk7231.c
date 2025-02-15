#include "../hal_wifi.h"

#define LOG_FEATURE LOG_FEATURE_MAIN
#include "../../new_common.h"
#include "wlan_ui_pub.h"
#include "ethernet_intf.h"
#include "../../new_common.h"
#include "net.h"
#include "../../logging/logging.h"
#include "../../beken378/app/config/param_config.h"
#include "lwip/netdb.h"

#ifdef PLATFORM_BEKEN_NEW

#define SOFT_AP						BK_SOFT_AP
#define STATION						BK_STATION
#define SECURITY_TYPE_NONE			BK_SECURITY_TYPE_NONE
#define SECURITY_TYPE_WEP			BK_SECURITY_TYPE_WEP
#define SECURITY_TYPE_WPA_TKIP		BK_SECURITY_TYPE_WPA_TKIP
#define SECURITY_TYPE_WPA2_AES		BK_SECURITY_TYPE_WPA2_AES
#define SECURITY_TYPE_WPA2_MIXED	BK_SECURITY_TYPE_WPA2_MIXED
#define SECURITY_TYPE_AUTO			BK_SECURITY_TYPE_AUTO

#define SOFT_AP BK_SOFT_AP

#endif

static void (*g_wifiStatusCallback)(int code);

// lenght of "192.168.103.103" is 15 but we also need a NULL terminating character
static char g_IP[32] = "unknown";
static int g_bOpenAccessPointMode = 0;
char *get_security_type(int type);

IPStatusTypedef ipStatus;
// This must return correct IP for both SOFT_AP and STATION modes,
// because, for example, javascript control panel requires it
const char* HAL_GetMyIPString() {

	os_memset(&ipStatus, 0x0, sizeof(IPStatusTypedef));
	if (g_bOpenAccessPointMode) {
		bk_wlan_get_ip_status(&ipStatus, SOFT_AP);
	}
	else {
		bk_wlan_get_ip_status(&ipStatus, STATION);
	}

	strncpy(g_IP, ipStatus.ip, 16);
	return g_IP;
}
const char* HAL_GetMyGatewayString() {
	strncpy(g_IP, ipStatus.gate, 16);
	return g_IP;
}
const char* HAL_GetMyDNSString() {
	strncpy(g_IP, ipStatus.dns, 16);
	return g_IP;
}
const char* HAL_GetMyMaskString() {
	strncpy(g_IP, ipStatus.mask, 16);
	return g_IP;
}

////////////////////
// NOTE: this gets the STA mac
void getMAC(unsigned char* mac) {
	net_get_if_macaddr(mac, net_get_sta_handle());
}

int WiFI_SetMacAddress(char* mac)
{
	if (wifi_set_mac_address((char*)mac))
		return 1;
	return 0; // error
}

void WiFI_GetMacAddress(char* mac)
{
	wifi_get_mac_address((char*)mac, CONFIG_ROLE_STA);
}

const char* HAL_GetMACStr(char* macstr)
{
	unsigned char mac[6];
	getMAC(mac);
	sprintf(macstr, MACSTR, MAC2STR(mac));
	return macstr;
}

char *get_security_type(int type) {
//void print_security_type(int type) {
	switch (type)
	{
	case SECURITY_TYPE_NONE:
		return "OPEN";
		break;
	case SECURITY_TYPE_WEP:
		return "WEP";
		break;
	case SECURITY_TYPE_WPA_TKIP:
		return "TKIP";
		break;
	case SECURITY_TYPE_WPA2_AES:
		return "CCMP";
		break;
	case SECURITY_TYPE_WPA2_MIXED:
		return "MIXED";
		break;
	case SECURITY_TYPE_AUTO:
		return "AUTO";
		break;
	default:
		return "Error";
		break;
	}
}

void HAL_PrintNetworkInfo()
{
	IPStatusTypedef ipStatus;

	os_memset(&ipStatus, 0x0, sizeof(IPStatusTypedef));
	bk_wlan_get_ip_status(&ipStatus, STATION);

	char* fmt = "dhcp=%d ip=%s gate=%s mask=%s mac=" MACSTR "\r\n";
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, fmt,
		ipStatus.dhcp, ipStatus.ip, ipStatus.gate,
		ipStatus.mask, MAC2STR((unsigned char*)ipStatus.mac));

	// print wifi state
	LinkStatusTypeDef linkStatus;
	network_InitTypeDef_ap_st ap_info;
	char ssid[33] = { 0 };
#if CFG_IEEE80211N
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, 
		"sta: %d, softap: %d, b/g/n\r\n", 
		sta_ip_is_start(), 
		uap_ip_is_start());
#else
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, 	
		"sta: %d, softap: %d, b/g\r\n", 
		sta_ip_is_start(), 
		uap_ip_is_start());
#endif

	if (sta_ip_is_start())
	{
		os_memset(&linkStatus, 0x0, sizeof(LinkStatusTypeDef));
		bk_wlan_get_link_status(&linkStatus);
		memcpy(ssid, linkStatus.ssid, 32);

		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, 
			"sta:rssi=%d,ssid=%s,bssid=" MACSTR ",channel=%d,cipher_type:%s",
			linkStatus.wifi_strength, 
			ssid, 
			MAC2STR(linkStatus.bssid), 
			linkStatus.channel,
			get_security_type(bk_sta_cipher_type())
			);
	}

	if (uap_ip_is_start())
	{
		os_memset(&ap_info, 0x0, sizeof(network_InitTypeDef_ap_st));
		bk_wlan_ap_para_info_get(&ap_info);
		memcpy(ssid, ap_info.wifi_ssid, 32);
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "softap:ssid=%s,channel=%d,dhcp=%d,cipher_type:%s",
			ssid, 
			ap_info.channel, 
			ap_info.dhcp_mode,
			get_security_type(ap_info.security)
			);
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "ip=%s,gate=%s,mask=%s,dns=%s\r\n",
			ap_info.local_ip_addr,
			ap_info.gateway_ip_addr,
			ap_info.net_mask,
			ap_info.dns_server_ip_addr);
	}

}

int HAL_GetWifiStrength()
{
	LinkStatusTypeDef linkStatus;
	os_memset(&linkStatus, 0x0, sizeof(LinkStatusTypeDef));
	bk_wlan_get_link_status(&linkStatus);
	return linkStatus.wifi_strength;
}

// receives status change notifications about wireless - could be useful
// ctxt is pointer to a rw_evt_type
void wl_status(void* ctxt)
{

	rw_evt_type stat = *((rw_evt_type*)ctxt);
	//ADDLOGF_INFO("wl_status %d\r\n", stat);

	switch (stat) {
	case RW_EVT_STA_IDLE:
#ifndef PLATFORM_BEKEN_NEW
	case RW_EVT_STA_SCANNING:
	case RW_EVT_STA_SCAN_OVER:
#endif
	case RW_EVT_STA_CONNECTING:
		if (g_wifiStatusCallback != 0) {
			g_wifiStatusCallback(WIFI_STA_CONNECTING);
		}
		break;
	case RW_EVT_STA_BEACON_LOSE:
	case RW_EVT_STA_PASSWORD_WRONG:
	case RW_EVT_STA_NO_AP_FOUND:
	case RW_EVT_STA_ASSOC_FULL:
	case RW_EVT_STA_DISCONNECTED:    /* disconnect with server */
		if (g_wifiStatusCallback != 0) {
			g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
		}
		break;
	case RW_EVT_STA_CONNECT_FAILED:  /* authentication failed */
		if (g_wifiStatusCallback != 0) {
			g_wifiStatusCallback(WIFI_STA_AUTH_FAILED);
		}
		break;
	case RW_EVT_STA_CONNECTED:        /* authentication success */
	case RW_EVT_STA_GOT_IP:
		if (g_wifiStatusCallback != 0) {
			g_wifiStatusCallback(WIFI_STA_CONNECTED);
		}
		break;

		/* for softap mode */
	case RW_EVT_AP_CONNECTED:          /* a client association success */
		if (g_wifiStatusCallback != 0) {
			g_wifiStatusCallback(WIFI_AP_CONNECTED);
		}
		break;
	case RW_EVT_AP_DISCONNECTED:       /* a client disconnect */
	case RW_EVT_AP_CONNECT_FAILED:     /* a client association failed */
		if (g_wifiStatusCallback != 0) {
			g_wifiStatusCallback(WIFI_AP_FAILED);
		}
		break;
	default:
		break;
	}

}


// from wlan_ui.c, no header
void bk_wlan_status_register_cb(FUNC_1PARAM_PTR cb);


void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;

	bk_wlan_status_register_cb(wl_status);
}
void HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t *ip)
{
	g_bOpenAccessPointMode = 0;

	network_InitTypeDef_st network_cfg;

	os_memset(&network_cfg, 0x0, sizeof(network_InitTypeDef_st));

	os_strcpy((char*)network_cfg.wifi_ssid, oob_ssid);
	os_strcpy((char*)network_cfg.wifi_key, connect_key);

	network_cfg.wifi_mode = STATION;
	if (ip->localIPAddr[0] == 0) {
		network_cfg.dhcp_mode = DHCP_CLIENT;
	}
	else {
		network_cfg.dhcp_mode = DHCP_DISABLE;
		convert_IP_to_string(network_cfg.local_ip_addr, ip->localIPAddr);
		convert_IP_to_string(network_cfg.net_mask, ip->netMask);
		convert_IP_to_string(network_cfg.gateway_ip_addr, ip->gatewayIPAddr);
		convert_IP_to_string(network_cfg.dns_server_ip_addr, ip->dnsServerIpAddr);
	}
	network_cfg.wifi_retry_interval = 100;

	ADDLOGF_INFO("ssid:%s key:%s\r\n", network_cfg.wifi_ssid, network_cfg.wifi_key);

	bk_wlan_start_sta(&network_cfg);
}

void HAL_DisconnectFromWifi()
{
    bk_wlan_stop(STATION);
}

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
#define APP_DRONE_DEF_NET_IP        "192.168.4.1"
#define APP_DRONE_DEF_NET_MASK      "255.255.255.0"
#define APP_DRONE_DEF_NET_GW        "192.168.4.1"
#define APP_DRONE_DEF_CHANNEL       1

	general_param_t general;
	ap_param_t ap_info;
	network_InitTypeDef_st wNetConfig;
	int len;
	unsigned char* mac;

	os_memset(&general, 0, sizeof(general_param_t));
	os_memset(&ap_info, 0, sizeof(ap_param_t));
	os_memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_st));

	general.role = 1,
		general.dhcp_enable = 1,

		os_strcpy((char*)wNetConfig.local_ip_addr, APP_DRONE_DEF_NET_IP);
	os_strcpy((char*)wNetConfig.net_mask, APP_DRONE_DEF_NET_MASK);
	os_strcpy((char*)wNetConfig.dns_server_ip_addr, APP_DRONE_DEF_NET_GW);


	ADDLOGF_INFO("no flash configuration, use default\r\n");
	mac = (unsigned char*)&ap_info.bssid.array;
	// this is MAC for Access Point, it's different than Client one
	// see wifi_get_mac_address source
	wifi_get_mac_address((char*)mac, CONFIG_ROLE_AP);
	ap_info.chann = APP_DRONE_DEF_CHANNEL;
	ap_info.cipher_suite = 0;
	//memcpy(ap_info.ssid.array, APP_DRONE_DEF_SSID, os_strlen(APP_DRONE_DEF_SSID));
	memcpy(ap_info.ssid.array, ssid, os_strlen(ssid));

	ap_info.key_len = 0;
	os_memset(&ap_info.key, 0, 65);


	bk_wlan_ap_set_default_channel(ap_info.chann);

	len = os_strlen((char*)ap_info.ssid.array);

	os_strncpy((char*)wNetConfig.wifi_ssid, (char*)ap_info.ssid.array, sizeof(wNetConfig.wifi_ssid));
	os_strncpy((char*)wNetConfig.wifi_key, (char*)ap_info.key, sizeof(wNetConfig.wifi_key));

	wNetConfig.wifi_mode = SOFT_AP;
	wNetConfig.dhcp_mode = DHCP_SERVER;
	os_strncpy((char*)wNetConfig.gateway_ip_addr, (char*)APP_DRONE_DEF_NET_GW, sizeof(wNetConfig.gateway_ip_addr));
	os_strncpy((char*)wNetConfig.dns_server_ip_addr, (char*)APP_DRONE_DEF_NET_GW, sizeof(wNetConfig.dns_server_ip_addr));
	wNetConfig.wifi_retry_interval = 100;

	if (1)
	{
		ADDLOGF_INFO("set ip info: %s,%s,%s\r\n",
			wNetConfig.local_ip_addr,
			wNetConfig.net_mask,
			wNetConfig.dns_server_ip_addr);
	}

	if (1)
	{
		ADDLOGF_INFO("ssid:%s  key:%s mode:%d\r\n", wNetConfig.wifi_ssid, wNetConfig.wifi_key, wNetConfig.wifi_mode);
	}
	//{
	//	IPStatusTypedef ipStatus;

	//	os_memset(&ipStatus, 0x0, sizeof(IPStatusTypedef));
	//	bk_wlan_get_ip_status(&ipStatus, STATION);
	//	ipStatus.dhcp = 1;
	//  os_strcpy((char *)ipStatus.ip, APP_DRONE_DEF_NET_IP);
	//  os_strcpy((char *)ipStatus.mask, APP_DRONE_DEF_NET_MASK);
	//  os_strcpy((char *)ipStatus.gate, APP_DRONE_DEF_NET_GW);
	//  os_strcpy((char *)ipStatus.dns, APP_DRONE_DEF_NET_IP);
	//	bk_wlan_set_ip_status(&ipStatus, STATION);

	//}
	bk_wlan_start(&wNetConfig);
	g_bOpenAccessPointMode = 1;

	//dhcp_server_start(0);
	//dhcp_server_stop(void);

	return 0;
}

