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
#include "../../new_pins.h"
#include "../src/new_cfg.h"

#ifdef PLATFORM_BEKEN_NEW

#define SOFT_AP						BK_SOFT_AP
#define STATION						BK_STATION
#define SECURITY_TYPE_NONE			BK_SECURITY_TYPE_NONE
#define SECURITY_TYPE_WEP			BK_SECURITY_TYPE_WEP
#define SECURITY_TYPE_WPA_TKIP		BK_SECURITY_TYPE_WPA_TKIP
#define SECURITY_TYPE_WPA_AES		BK_SECURITY_TYPE_WPA_AES
#define SECURITY_TYPE_WPA2_TKIP		BK_SECURITY_TYPE_WPA2_TKIP
#define SECURITY_TYPE_WPA2_AES		BK_SECURITY_TYPE_WPA2_AES
#define SECURITY_TYPE_WPA2_MIXED	BK_SECURITY_TYPE_WPA2_MIXED
#define SECURITY_TYPE_AUTO			BK_SECURITY_TYPE_AUTO
#define RW_EVT_STA_CONNECT_FAILED	RW_EVT_STA_AUTH_FAILED
#endif

//extern int pbkdf2_sha1(const char* passphrase, const u8* ssid, size_t ssid_len,
//	int iterations, u8* buf, size_t buflen);
extern u8* wpas_get_sta_psk(void);

static void (*g_wifiStatusCallback)(int code);

// lenght of "192.168.103.103" is 15 but we also need a NULL terminating character
static char g_IP[32] = "unknown";
static int g_bOpenAccessPointMode = 0;
char *get_security_type(int type);
bool g_bStaticIP = false, g_needFastConnectSave = false;

IPStatusTypedef ipStatus;
// This must return correct IP for both SOFT_AP and STATION modes,
// because, for example, javascript control panel requires it
const char* HAL_GetMyIPString() {

	memset(&ipStatus, 0x0, sizeof(IPStatusTypedef));
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
	case SECURITY_TYPE_WPA2_TKIP:
		return "TKIP";
		break;
	case SECURITY_TYPE_WPA_AES:
	case SECURITY_TYPE_WPA2_AES:
		return "CCMP";
		break;
#ifdef PLATFORM_BEKEN_NEW
	case BK_SECURITY_TYPE_WPA_MIXED:
#endif
	case SECURITY_TYPE_WPA2_MIXED:
		return "MIXED";
		break;
	case SECURITY_TYPE_AUTO:
		return "AUTO";
		break;
#ifdef PLATFORM_BEKEN_NEW
	case BK_SECURITY_TYPE_WPA3_SAE:
		return "SAE";
		break;
	case BK_SECURITY_TYPE_WPA3_WPA2_MIXED:
		return "MIXED3";
		break;
	case BK_SECURITY_TYPE_OWE:
		return "OWE";
		break;
#endif
	default:
		return "Error";
		break;
	}
}

void HAL_PrintNetworkInfo()
{
	IPStatusTypedef ipStatus;

	memset(&ipStatus, 0x0, sizeof(IPStatusTypedef));
	bk_wlan_get_ip_status(&ipStatus, STATION);

	char* fmt = "dhcp=%d ip=%s gate=%s mask=%s mac=" MACSTR "\r\n";
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, fmt,
		ipStatus.dhcp, ipStatus.ip, ipStatus.gate,
		ipStatus.mask, MAC2STR((unsigned char*)ipStatus.mac));

	// print wifi state
	LinkStatusTypeDef linkStatus;
	network_InitTypeDef_ap_st ap_info;
	char ssid[33] = { 0 };
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
#if CFG_IEEE80211N
		"sta: %d, softap: %d, b/g/n\r\n",
#else
		"sta: %d, softap: %d, b/g\r\n",
#endif
		sta_ip_is_start(), 
		uap_ip_is_start());

	if (sta_ip_is_start())
	{
		memset(&linkStatus, 0x0, sizeof(LinkStatusTypeDef));
		bk_wlan_get_link_status(&linkStatus);
		memcpy(ssid, linkStatus.ssid, 32);

		int cipher = bk_sta_cipher_type();

		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, 
			"sta:rssi=%d,ssid=%s,bssid=" MACSTR ",channel=%d,cipher_type:%s",
			linkStatus.wifi_strength, 
			ssid, 
			MAC2STR(linkStatus.bssid), 
			linkStatus.channel,
			get_security_type(cipher)
			);

		// bk_wlan_get_link_status doesn't work in handler when static ip is configured
		if(g_needFastConnectSave && CFG_HasFlag(OBK_FLAG_WIFI_ENHANCED_FAST_CONNECT) && cipher != SECURITY_TYPE_AUTO)
		{
			// if we have SAE, mixed WPA3 (BK uses SAE not PSK in that case), WEP, Open, OWE or auto, then disable connection via psk
#if PLATFORM_BEKEN_NEW
			if(cipher <= SECURITY_TYPE_WEP || cipher >= BK_SECURITY_TYPE_WPA3_SAE)
#else
			if(cipher <= SECURITY_TYPE_WEP)
#endif
			{
				// if we ignore that, and use psk - bk will fail to connect.
				// If we use password instead of psk, then first attempt to connect will almost always fail.
				// And even if it succeeds, first connect is not faster.
				ADDLOG_WARN(LOG_FEATURE_GENERAL, "Fast connect is not supported with current AP encryption.");
				if(g_cfg.fcdata.channel != 0)
				{
					g_cfg.fcdata.channel = 0;
					g_cfg_pendingChanges++;
				}
			}
			else
			{
				char psks[65];
				//const char* wifi_ssid, * wifi_pass;
				//wifi_ssid = CFG_GetWiFiSSID();
				//wifi_pass = CFG_GetWiFiPass();
				//unsigned char psk[32];
				//pbkdf2_sha1(wifi_pass, (u8*)wifi_ssid, strlen(wifi_ssid), 4096, psk, 32);
				uint8_t* psk = wpas_get_sta_psk();

				for(int i = 0; i < 32 && sprintf(psks + i * 2, "%02x", psk[i]) == 2; i++);

				if(memcmp((char*)psks, g_cfg.fcdata.psk, 64) != 0 ||
					memcmp(g_cfg.fcdata.bssid, linkStatus.bssid, 6) != 0 ||
					linkStatus.channel != g_cfg.fcdata.channel ||
					linkStatus.security != g_cfg.fcdata.security_type)
				{
					ADDLOG_INFO(LOG_FEATURE_GENERAL, "Saved fast connect data differ to current one, saving...");
					memcpy(g_cfg.fcdata.bssid, linkStatus.bssid, 6);
					g_cfg.fcdata.channel = linkStatus.channel;
					g_cfg.fcdata.security_type = linkStatus.security;
					memcpy(g_cfg.fcdata.psk, psks, sizeof(g_cfg.fcdata.psk));
					g_cfg_pendingChanges++;
				}
			}
			g_needFastConnectSave = false;
		}
	}

	if (uap_ip_is_start())
	{
		memset(&ap_info, 0x0, sizeof(network_InitTypeDef_ap_st));
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
	memset(&linkStatus, 0x0, sizeof(LinkStatusTypeDef));
	bk_wlan_get_link_status(&linkStatus);
	return linkStatus.wifi_strength;
}
// Get WiFi Information (SSID / BSSID) - e.g. to display on status page 
/*
// ATM there is only one SSID, so need for this code
char* HAL_GetWiFiSSID(char* ssid){
	if (sta_ip_is_start()){
		LinkStatusTypeDef linkStatus;
		bk_wlan_get_link_status(&linkStatus);
//		memcpy(ssid, linkStatus.ssid, sizeof(ssid)-1);
		strcpy(ssid,linkStatus.ssid);
		return ssid;	
	}
	ssid[0]='\0';
	return ssid; 
};
*/
char* HAL_GetWiFiBSSID(char* bssid){
	if (sta_ip_is_start()){
		LinkStatusTypeDef linkStatus;
		bk_wlan_get_link_status(&linkStatus);
		sprintf(bssid, MACSTR, MAC2STR(linkStatus.bssid));
		return bssid;
	}
	bssid[0]='\0';
	return bssid; 
};
uint8_t HAL_GetWiFiChannel(uint8_t *chan){
	if (sta_ip_is_start()){
		LinkStatusTypeDef linkStatus;
		bk_wlan_get_link_status(&linkStatus);
		*chan = linkStatus.channel ;
		return *chan;
	}
	return 0;
};

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
#ifdef PLATFORM_BEKEN_NEW
	case RW_EVT_STA_DEAUTH:
	case RW_EVT_STA_DISASSOC:
	case RW_EVT_STA_ASSOC_FAILED:
	case RW_EVT_STA_ACTIVE_DISCONNECTED:
#else
	case RW_EVT_STA_DISCONNECTED:    /* disconnect with server */
#endif
		if (g_wifiStatusCallback != 0) {
			g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
		}
		break;
	case RW_EVT_STA_CONNECT_FAILED:  /* authentication failed */
		if (g_wifiStatusCallback != 0) {
			g_wifiStatusCallback(WIFI_STA_AUTH_FAILED);
		}
		break;
	case RW_EVT_STA_CONNECTED: if(!g_bStaticIP) break;
	case RW_EVT_STA_GOT_IP:          /* authentication success */
		if (g_wifiStatusCallback != 0) {
			g_wifiStatusCallback(WIFI_STA_CONNECTED);
		}
		g_needFastConnectSave = true;
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

	memset(&network_cfg, 0x0, sizeof(network_InitTypeDef_st));

	strcpy((char*)network_cfg.wifi_ssid, oob_ssid);
	strcpy((char*)network_cfg.wifi_key, connect_key);

	network_cfg.wifi_mode = STATION;
	if (ip->localIPAddr[0] == 0) {
		network_cfg.dhcp_mode = DHCP_CLIENT;
		g_bStaticIP = false;
	}
	else {
		network_cfg.dhcp_mode = DHCP_DISABLE;
		convert_IP_to_string(network_cfg.local_ip_addr, ip->localIPAddr);
		convert_IP_to_string(network_cfg.net_mask, ip->netMask);
		convert_IP_to_string(network_cfg.gateway_ip_addr, ip->gatewayIPAddr);
		convert_IP_to_string(network_cfg.dns_server_ip_addr, ip->dnsServerIpAddr);
		g_bStaticIP = true;
	}
	network_cfg.wifi_retry_interval = 100;

	//ADDLOGF_INFO("ssid:%s key:%s\r\n", network_cfg.wifi_ssid, network_cfg.wifi_key);

	bk_wlan_start_sta(&network_cfg);
}

void HAL_FastConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
	if(strnlen(g_cfg.fcdata.psk, 64) == 64 &&
		g_cfg.fcdata.channel != 0 &&
		g_cfg.fcdata.security_type != 0 &&
		!(g_cfg.fcdata.bssid[0] == 0 && g_cfg.fcdata.bssid[1] == 0 && g_cfg.fcdata.bssid[2] == 0 &&
		g_cfg.fcdata.bssid[3] == 0 && g_cfg.fcdata.bssid[4] == 0 && g_cfg.fcdata.bssid[5] == 0))
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "We have fast connection data, connecting...");
		network_InitTypeDef_adv_st network_cfg;
		memset(&network_cfg, 0, sizeof(network_InitTypeDef_adv_st));
		strcpy(network_cfg.ap_info.ssid, oob_ssid);
		memcpy(network_cfg.key, g_cfg.fcdata.psk, 64);
		network_cfg.key_len = 64;
		memcpy(network_cfg.ap_info.bssid, g_cfg.fcdata.bssid, sizeof(g_cfg.fcdata.bssid));

		if(ip->localIPAddr[0] == 0)
		{
			network_cfg.dhcp_mode = DHCP_CLIENT;
			g_bStaticIP = false;
		}
		else
		{
			network_cfg.dhcp_mode = DHCP_DISABLE;
			convert_IP_to_string(network_cfg.local_ip_addr, ip->localIPAddr);
			convert_IP_to_string(network_cfg.net_mask, ip->netMask);
			convert_IP_to_string(network_cfg.gateway_ip_addr, ip->gatewayIPAddr);
			convert_IP_to_string(network_cfg.dns_server_ip_addr, ip->dnsServerIpAddr);
			g_bStaticIP = true;
		}
		network_cfg.ap_info.channel = g_cfg.fcdata.channel;
		network_cfg.ap_info.security = g_cfg.fcdata.security_type;
		network_cfg.wifi_retry_interval = 100;

		bk_wlan_start_sta_adv(&network_cfg);
	}
	else
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "Fast connect data is empty, connecting normally");
		HAL_ConnectToWiFi(oob_ssid, connect_key, ip);
	}
}

void HAL_DisableEnhancedFastConnect()
{
	if(g_cfg.fcdata.channel != 0)
	{
		g_cfg.fcdata.channel = 0;
		g_cfg_pendingChanges++;
	}
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
	unsigned char* mac;

	memset(&general, 0, sizeof(general_param_t));
	memset(&ap_info, 0, sizeof(ap_param_t));
	memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_st));

	general.role = 1,
	general.dhcp_enable = 1,

	strcpy((char*)wNetConfig.local_ip_addr, APP_DRONE_DEF_NET_IP);
	strcpy((char*)wNetConfig.net_mask, APP_DRONE_DEF_NET_MASK);
	strcpy((char*)wNetConfig.dns_server_ip_addr, APP_DRONE_DEF_NET_GW);


	ADDLOGF_INFO("no flash configuration, use default\r\n");
	mac = (unsigned char*)&ap_info.bssid.array;
	// this is MAC for Access Point, it's different than Client one
	// see wifi_get_mac_address source
	wifi_get_mac_address((char*)mac, CONFIG_ROLE_AP);
	ap_info.chann = APP_DRONE_DEF_CHANNEL;
	ap_info.cipher_suite = 0;
	//memcpy(ap_info.ssid.array, APP_DRONE_DEF_SSID, strlen(APP_DRONE_DEF_SSID));
	memcpy(ap_info.ssid.array, ssid, strlen(ssid));

	ap_info.key_len = 0;
	memset(&ap_info.key, 0, 65);


	bk_wlan_ap_set_default_channel(ap_info.chann);

	//int len = strlen((char*)ap_info.ssid.array);

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

	//	memset(&ipStatus, 0x0, sizeof(IPStatusTypedef));
	//	bk_wlan_get_ip_status(&ipStatus, STATION);
	//	ipStatus.dhcp = 1;
	//  strcpy((char *)ipStatus.ip, APP_DRONE_DEF_NET_IP);
	//  strcpy((char *)ipStatus.mask, APP_DRONE_DEF_NET_MASK);
	//  strcpy((char *)ipStatus.gate, APP_DRONE_DEF_NET_GW);
	//  strcpy((char *)ipStatus.dns, APP_DRONE_DEF_NET_IP);
	//	bk_wlan_set_ip_status(&ipStatus, STATION);

	//}
	bk_wlan_start(&wNetConfig);
	g_bOpenAccessPointMode = 1;

	//dhcp_server_start(0);
	//dhcp_server_stop(void);

	return 0;
}

