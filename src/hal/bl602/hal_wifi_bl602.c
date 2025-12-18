#ifdef PLATFORM_BL602

#include "../hal_wifi.h"
#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../../logging/logging.h"
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <portable.h>
#include <semphr.h>
#include <hal_sys.h>
#include <bl_wifi.h>
#include <bl60x_fw_api.h>
#include <wifi_mgmr_ext.h>
#include <aos/kernel.h>
#include <aos/yloop.h>
#include <easyflash.h>

// lenght of "192.168.103.103" is 15 but we also need a NULL terminating character
static char g_ipStr[16] = "unknown";
static char g_gwStr[16] = "unknown";
static char g_maskStr[16] = "unknown";
static char g_dnsStr[16] = "unknown";
static int g_bAccessPointMode = 1;
static void (*g_wifiStatusCallback)(int code);
extern bool g_powersave;
static obkFastConnectData_t fcdata = { 0 };
extern uint16_t phy_channel_to_freq(uint8_t band, int channel);

void HAL_ConnectToWiFi(const char *ssid, const char *psk, obkStaticIP_t *ip)
{
    wifi_interface_t wifi_interface;
    if(ip->localIPAddr[0] == 0)
    {
        wifi_mgmr_sta_ip_unset();
    }
    else
    {
        wifi_mgmr_sta_ip_set(*(int*)ip->localIPAddr, *(int*)ip->netMask, *(int*)ip->gatewayIPAddr, *(int*)ip->dnsServerIpAddr, 0);
    }
    if(g_powersave) wifi_mgmr_sta_ps_exit();
    wifi_interface = wifi_mgmr_sta_enable();

    // sending WIFI_CONNECT_PMF_CAPABLE is crucial here, without it, wpa3 or wpa2/3 mixed mode does not work and
    // connection is unstable, mqtt disconnects every few minutes
    wifi_mgmr_sta_connect_mid(wifi_interface, (char*)ssid, (char*)psk, NULL, NULL, 0, 0, ip->localIPAddr[0] == 0 ? 1 : 0, WIFI_CONNECT_PMF_CAPABLE);

    g_bAccessPointMode = 0;
}

// BL_Err_Type EF_Ctrl_Write_MAC_Address_Opt(uint8_t slot,uint8_t mac[6],uint8_t program)
// is called by 
// int8_t mfg_efuse_write_macaddr_pre(uint8_t mac[6],uint8_t program)
// is called by 
// int8_t mfg_media_write_macaddr_pre_need_lock(uint8_t mac[6],uint8_t program)
// is called by 
// int8_t mfg_media_write_macaddr_pre_with_lock(uint8_t mac[6], uint8_t program);
// from: OpenBL602\components\bl602\bl602_std\bl602_std\StdDriver\Inc\bl602_mfg_media.h
int WiFI_SetMacAddress(char *mac) {
	wifi_mgmr_sta_mac_set((uint8_t *)mac);
	CFG_SetMac(mac);
	return 1;

}
void HAL_DisconnectFromWifi()
{
    wifi_mgmr_sta_disconnect();
}

int HAL_SetupWiFiOpenAccessPoint(const char *ssid) {

    uint8_t hidden_ssid = 0;
    //int channel;
    wifi_interface_t wifi_interface;
	//struct netif *net;

    wifi_interface = wifi_mgmr_ap_enable();
    /*no password when only one param*/
    wifi_mgmr_ap_start(wifi_interface, (char*)ssid, hidden_ssid, NULL, 1);

	g_bAccessPointMode = 1;

	return 0;
}
static void event_cb_wifi_event(input_event_t *event, void *private_data)
{


    switch (event->code) {
        case CODE_WIFI_ON_INIT_DONE:
        {
            printf("[APP] [EVT] INIT DONE %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_MGMR_DONE:
        {
            printf("[APP] [EVT] MGMR DONE %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_SCAN_DONE:
        {
            printf("[APP] [EVT] SCAN Done %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_DISCONNECT:
        {
            printf("[APP] [EVT] disconnect %lld\r\n", aos_now_ms());
			if(g_wifiStatusCallback!=0) {
				g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
			}
        }
        break;
        case CODE_WIFI_ON_CONNECTING:
        {
            printf("[APP] [EVT] Connecting %lld\r\n", aos_now_ms());
			if(g_wifiStatusCallback!=0) {
				g_wifiStatusCallback(WIFI_STA_CONNECTING);
			}
        }
        break;
        case CODE_WIFI_CMD_RECONNECT:
        {
            printf("[APP] [EVT] Reconnect %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_CONNECTED:
        {
            printf("[APP] [EVT] connected %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_PRE_GOT_IP:
        {
            printf("[APP] [EVT] connected %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_GOT_IP:
        {
            printf("[APP] [EVT] GOT IP %lld\r\n", aos_now_ms());
            printf("[SYS] Memory left is %d Bytes\r\n", xPortGetFreeHeapSize());
            if(g_wifiStatusCallback!=0) {
                g_wifiStatusCallback(WIFI_STA_CONNECTED);
            }
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
        }
        break;
        case CODE_WIFI_ON_PROV_SSID:
        {
            printf("[APP] [EVT] [PROV] [SSID] %lld: %s\r\n",
                    aos_now_ms(),
                    event->value ? (const char*)event->value : "UNKNOWN"
            );
        }
        break;
        case CODE_WIFI_ON_PROV_BSSID:
        {
            printf("[APP] [EVT] [PROV] [BSSID] %lld: %s\r\n",
                    aos_now_ms(),
                    event->value ? (const char*)event->value : "UNKNOWN"
            );
        }
        break;
        case CODE_WIFI_ON_PROV_PASSWD:
        {
            printf("[APP] [EVT] [PROV] [PASSWD] %lld: %s\r\n", aos_now_ms(),
                    event->value ? (const char*)event->value : "UNKNOWN"
            );
        }
        break;
        case CODE_WIFI_ON_PROV_CONNECT:
        {
            printf("[APP] [EVT] [PROV] [CONNECT] %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_PROV_DISCONNECT:
        {
            printf("[APP] [EVT] [PROV] [DISCONNECT] %lld\r\n", aos_now_ms());
			if(g_wifiStatusCallback!=0) {
				g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
			}
        }
        break;
        default:
        {
            printf("[APP] [EVT] Unknown code %u, %lld\r\n", event->code, aos_now_ms());
            /*nothing*/
        }
    }
}
void HAL_WiFi_SetupStatusCallback(void (*cb)(int code)) {
	g_wifiStatusCallback = cb;

    aos_register_event_filter(EV_WIFI, event_cb_wifi_event, NULL);
}

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
char* HAL_GetWiFiBSSID(char* bssid){
	wifi_mgmr_sta_connect_ind_stat_info_t info;
	memset(&info, 0, sizeof(info));
	wifi_mgmr_sta_connect_ind_stat_get(&info);
//	memcpy(bssid, info.bssid, sizeof(bssid));
	sprintf(bssid, MACSTR, MAC2STR(info.bssid));
	return bssid;
};
uint8_t HAL_GetWiFiChannel(uint8_t *chan){
	wifi_mgmr_sta_connect_ind_stat_info_t info;
	memset(&info, 0, sizeof(info));
	wifi_mgmr_sta_connect_ind_stat_get(&info);
	*chan = info.chan_band;
	return *chan;
};

void HAL_PrintNetworkInfo() {

}
int HAL_GetWifiStrength() {
    int rssi = -1;
    wifi_mgmr_rssi_get(&rssi);
    return rssi;	
}

const char *HAL_GetMyIPString()
{
    uint32_t ip;
    uint32_t gw;
    uint32_t mask;

    if(g_bAccessPointMode == 1)
    {
        wifi_mgmr_ap_ip_get(&ip, &gw, &mask);
    }
    else
    {
        wifi_mgmr_sta_ip_get(&ip, &gw, &mask);
    }

    strcpy(g_ipStr,inet_ntoa(ip));
    strcpy(g_gwStr, inet_ntoa(gw));
    strcpy(g_maskStr, inet_ntoa(mask));

    return g_ipStr;
}

const char* HAL_GetMyGatewayString()
{
    return g_gwStr;
}
const char* HAL_GetMyDNSString()
{
    uint32_t dns1;
    uint32_t dns2;

    if(g_bAccessPointMode == 1)
    {
        return "none";
    }
    else
    {
        wifi_mgmr_sta_dns_get(&dns1, &dns2);
    }
    strcpy(g_dnsStr, inet_ntoa(dns1));
    return g_dnsStr;
}
const char* HAL_GetMyMaskString()
{
    return g_maskStr;
}

void WiFI_GetMacAddress(char *mac) {
	// FOR station mode
	wifi_mgmr_sta_mac_get((uint8_t*)mac);
}
const char *HAL_GetMACStr(char *macstr) {
	uint8_t mac[6];
	if(g_bAccessPointMode == 1) {
		wifi_mgmr_ap_mac_get(mac);
	} else {
		wifi_mgmr_sta_mac_get(mac);
	}
	sprintf(macstr, MACSTR, MAC2STR(mac));
	return macstr;
}

void HAL_FastConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t* ip)
{
    int len = ef_get_env_blob("fcdata", &fcdata, sizeof(obkFastConnectData_t), NULL);
    if(len == sizeof(obkFastConnectData_t))
    {
        ADDLOG_INFO(LOG_FEATURE_GENERAL, "We have fast connection data, connecting...");
        if(ip->localIPAddr[0] == 0)
        {
            wifi_mgmr_sta_ip_unset();
        }
        else
        {
            wifi_mgmr_sta_ip_set(*(int*)ip->localIPAddr, *(int*)ip->netMask, *(int*)ip->gatewayIPAddr, *(int*)ip->dnsServerIpAddr, 0);
        }

        struct ap_connect_adv ext_param = { 0 };
        char psk[65] = { 0 };
        memcpy(psk, fcdata.psk, 64);
        printf("strlen psk = %i\r\n", strlen(psk));
        ext_param.psk = psk;
        ext_param.ap_info.type = AP_INFO_TYPE_SUGGEST;
        ext_param.ap_info.time_to_live = 30;
        ext_param.ap_info.bssid = (uint8_t*)fcdata.bssid;
        ext_param.ap_info.band = 0;
        ext_param.ap_info.freq = phy_channel_to_freq(0, fcdata.channel);
        ext_param.ap_info.use_dhcp = ip->localIPAddr[0] == 0 ? 1 : 0;
        ext_param.flags = WIFI_CONNECT_PMF_CAPABLE | WIFI_CONNECT_STOP_SCAN_ALL_CHANNEL_IF_TARGET_AP_FOUND | WIFI_CONNECT_STOP_SCAN_CURRENT_CHANNEL_IF_TARGET_AP_FOUND;

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

#endif // PLATFORM_BL602
