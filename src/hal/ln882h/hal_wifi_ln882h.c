#if PLATFORM_LN882H || PLATFORM_LN8825

#include "../hal_wifi.h"
#include "wifi.h"
#include "dhcp.h"
#include "netif/ethernetif.h"
#include "wifi_manager.h"
#include "lwip/tcpip.h"
#include "utils/system_parameter.h"
//#include "ln_misc.h"
//#include "ln_psk_calc.h"
#include "utils/sysparam_factory_setting.h"
#include <lwip/sockets.h>
#include <stdbool.h>	// for bool "g_STA_static_IP"
#if PLATFORM_LN8825
#include "wifi_port/wifi_port.h"
#include "wifi_port/ln_wifi_err.h"
#include "ethernetif.h"
#include "hal/hal_sleep.h"
#define netdev_set_mac_addr ethernetif_set_mac_addr
#define netdev_got_ip ethernetif_got_ip
#define netdev_get_active ethernetif_get_active
#define netdev_get_netif ethernetif_get_netif
#define netdev_set_ip_info ethernetif_set_ip_info
#define netdev_set_active(x) ethernetif_set_state(x, NETIF_UP)
#define netdev_set_state ethernetif_set_state
#define sysparam_sta_mac_get(x) system_parameter_get_macaddr(STATION_IF, x)
#define sysparam_softap_mac_get(x) system_parameter_get_macaddr(SOFT_AP_IF, x)
#define sysparam_sta_mac_update(x) system_parameter_set_macaddr(STATION_IF, x)
#define sysparam_softap_mac_update(x) system_parameter_set_macaddr(SOFT_AP_IF, x)
#define netif_idx_t wifi_interface_enum_t
#define NETIF_IDX_AP SOFT_AP_IF
#define NETIF_IDX_STA STATION_IF
#define wifi_scan_cfg_t wifi_scan_config_t
#define NETDEV_UP NETIF_UP
#define SYSPARAM_ERR_NONE 0
#define LN_UNUSED(x)
#define wifi_sta_get_rssi(x) do{*x = wifi_station_get_rssi();}while(0)
#define sysparam_sta_hostname_update(x) system_parameter_set_hostname(STATION_IF, x);
#define wifi_sta_disconnect wifi_station_disconnect
#else
#include "wifi_port.h"
#include "ln_wifi_err.h"
#endif


#define PM_WIFI_DEFAULT_PS_MODE           (WIFI_NO_POWERSAVE)

static void (*g_wifiStatusCallback)(int code)  = NULL;

// if we are using a static IP, prevent using DHCP in lwips interface implementation in ethernetif.c
bool g_STA_static_IP=0;

void alert_log(const char *format, ...) {
     va_list args;

    LOG(LOG_LVL_INFO, "----------------------------------------\r\n");
    va_start(args, format);
    __sprintf(NULL, (stdio_write_fn)log_stdio_write, format, args);
    va_end(args);
    LOG(LOG_LVL_INFO, "\r\n");
    LOG(LOG_LVL_INFO, "----------------------------------------\r\n");
}

// length of "192.168.103.103" is 15 but we also need a NULL terminating character
static char g_IP[32] = "unknown";
static int g_bOpenAccessPointMode = 0;
static uint8_t psk_value[40]      = {0x0};


struct netif* get_connected_nif() {
    struct netif *nif = NULL;
    if (netdev_got_ip()) {
        netif_idx_t active = netdev_get_active();
        nif = netdev_get_netif(active);
    }
    return nif;
}

// This must return correct IP for both SOFT_AP and STATION modes,
// because, for example, javascript control panel requires it
const char* HAL_GetMyIPString() {
    struct netif *nif = get_connected_nif();
    if (nif != NULL) {
        char* ip_addr = ip4addr_ntoa(&nif->ip_addr);
        strncpy(g_IP, ip_addr, 16);
    }
	return g_IP;
}

const char* HAL_GetMyGatewayString() {
    struct netif *nif = get_connected_nif();
    if (nif != NULL) {
        char* ip_addr = ip4addr_ntoa(&nif->gw);
        strncpy(g_IP, ip_addr, 16);
    }
	return g_IP;
}

const char* HAL_GetMyDNSString() {
	return g_IP;
}

const char* HAL_GetMyMaskString() {
    struct netif *nif = get_connected_nif();
    if (nif != NULL) {
        char* ip_addr = ip4addr_ntoa(&nif->netmask);
        strncpy(g_IP, ip_addr, 16);
    }
	return g_IP;
}


int WiFI_SetMacAddress(char* mac)
{
    if (netdev_got_ip()) {
        sysparam_sta_mac_update((const uint8_t *) mac);
        if (netdev_get_active() == NETIF_IDX_STA) {
            // this probably requires a reboot. Not sure if this actually does anything
            netdev_set_mac_addr(NETIF_IDX_STA, (uint8_t *) mac);
        }
        return 1;
    }
	return 0; // error
}

void WiFI_GetMacAddress(char* mac)
{
	sysparam_sta_mac_get((unsigned char*)mac);
}

const char* HAL_GetMACStr(char* macstr)
{
	unsigned char mac[6];
    sysparam_sta_mac_get((unsigned char*)mac);
	sprintf(macstr, MACSTR, MAC2STR(mac));
	return macstr;
}

void HAL_PrintNetworkInfo()
{
    struct netif *nif = get_connected_nif();
    if (nif != NULL) {
        netif_idx_t active = netdev_get_active();
        uint8_t * mac = nif->hwaddr;
        LOG(LOG_LVL_INFO, "+--------------- net device info ------------+\r\n");
        LOG(LOG_LVL_INFO, "|netif type    : %-16s            |\r\n", active == NETIF_IDX_STA ? "STA" : "AP");
        LOG(LOG_LVL_INFO, "|netif hostname: %-16s            |\r\n", nif->hostname);
        LOG(LOG_LVL_INFO, "|netif ip      = %-16s            |\r\n", ip4addr_ntoa(&nif->ip_addr));
        LOG(LOG_LVL_INFO, "|netif mask    = %-16s            |\r\n", ip4addr_ntoa(&nif->netmask));
        LOG(LOG_LVL_INFO, "|netif gateway = %-16s            |\r\n", ip4addr_ntoa(&nif->gw));
        LOG(LOG_LVL_INFO, "|netif mac     : [%02X:%02X:%02X:%02X:%02X:%02X] %-7s |\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], "");
        LOG(LOG_LVL_INFO, "+--------------------------------------------+\r\n");
    }
}

int HAL_GetWifiStrength()
{
    int8_t val;
    wifi_sta_get_rssi(&val);
    return val;
}
// Get WiFi Information (SSID / BSSID) - e.g. to display on status page 
/*
const uint8_t * bssid;
const char ssid[33]={0};

if (wifi_get_sta_conn_info(&ssid, &bssid) == 0){
+			hprintf255(request, " --- Wifi SSID/BSSI: %s / [%02X:%02X:%02X:%02X:%02X:%02X] ", ssid, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
+		}

*/
/*
// ATM there is only one SSID, so need for this code

char* HAL_GetWiFiSSID(char* ssid){
	const uint8_t * bssid = NULL;
	const char * tempssid = NULL;
	wifi_get_sta_conn_info(&tempssid, &bssid);
	strcpy(ssid,tempssid);
	return ssid;
};
*/
#if PLATFORM_LN882H
char* HAL_GetWiFiBSSID(char* bssid){
	const uint8_t * tempbssid = NULL;
	const char * ssid = NULL;
	if (wifi_get_sta_conn_info(&ssid, &tempbssid) == 0) sprintf(bssid, MACSTR, MAC2STR(tempbssid));
	return bssid;
};
uint8_t HAL_GetWiFiChannel(uint8_t *chan){
	wifi_scan_cfg_t   scan_cfg   = {0,};
	wifi_get_sta_scan_cfg(&scan_cfg);
	*chan = scan_cfg.channel;
	return *chan;
};

#endif

void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
	g_wifiStatusCallback = cb;
}

#if PLATFORM_LN882H

static void wifi_scan_complete_cb(void * arg)
{
    LN_UNUSED(arg);

    ln_list_t *list;
    uint8_t node_count = 0;
    ap_info_node_t *pnode;

    wifi_manager_ap_list_update_enable(LN_FALSE);

    // 1.get ap info list.
    wifi_manager_get_ap_list(&list, &node_count);

    // 2.print all ap info in the list.
    LN_LIST_FOR_EACH_ENTRY(pnode, ap_info_node_t, list,list)
    {
        uint8_t * mac = (uint8_t*)pnode->info.bssid;
        ap_info_t *ap_info = &pnode->info;

        LOG(LOG_LVL_INFO, "\tCH=%2d,RSSI= %3d,", ap_info->channel, ap_info->rssi);
        LOG(LOG_LVL_INFO, "BSSID:[%02X:%02X:%02X:%02X:%02X:%02X],SSID:\"%s\"\r\n", \
                           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ap_info->ssid);
    }

    wifi_manager_ap_list_update_enable(LN_TRUE);
}

static void wifi_connect_failed_cb(void* arg) {
    wifi_sta_connect_failed_reason_t reason = *((wifi_sta_connect_failed_reason_t*) arg);
    if (reason == WIFI_STA_CONN_WRONG_PWD) {
        if (g_wifiStatusCallback != NULL) {
            g_wifiStatusCallback(WIFI_STA_AUTH_FAILED);
        }    
    }
}
#else

static void wifi_disconnected_cb(void* arg)
{
    if(g_wifiStatusCallback != NULL)
    {
        g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
    }
}

#endif

static void wifi_connected_cb(void* arg)
{
    LN_UNUSED(arg);
    if(g_wifiStatusCallback != NULL)
    {
        g_wifiStatusCallback(WIFI_STA_CONNECTED);
    }
}

void wifi_init_sta(const char* oob_ssid, const char* connect_key, obkStaticIP_t *ip)
{
#if PLATFORM_LN882H
    sta_ps_mode_t ps_mode = PM_WIFI_DEFAULT_PS_MODE;
	wifi_sta_connect_t connect = {
		.ssid    = oob_ssid,
		.pwd     = connect_key,
		.bssid   = NULL,
		.psk_value = NULL,
	};
#else
    wifi_config_t connect = { 0 };
    wifi_init_type_t init_param =
    {
        .wifi_mode = WIFI_MODE_STATION,
        .sta_ps_mode = WIFI_NO_POWERSAVE,
        .dhcp_mode = WLAN_DHCP_CLIENT,
        .scanned_ap_list_size = SCANNED_AP_LIST_SIZE,
    };
#endif
//	wifi_manager_set_ap_list_sort_rule(1);
	wifi_scan_cfg_t scan_cfg = {
        .channel   = 0,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = 20,
	};

    //1. sta mac get
    uint8_t mac_addr[6];
    if (SYSPARAM_ERR_NONE != sysparam_sta_mac_get(mac_addr)) {
        LOG(LOG_LVL_ERROR, "[%s]sta mac get failed!!!\r\n", __func__);
        return;
    }

    if (mac_addr[0] == STA_MAC_ADDR0 &&
        mac_addr[1] == STA_MAC_ADDR1 &&
        mac_addr[2] == STA_MAC_ADDR2 &&
        mac_addr[3] == STA_MAC_ADDR3 &&
        mac_addr[4] == STA_MAC_ADDR4 &&
        mac_addr[5] == STA_MAC_ADDR5) {
        ln_generate_random_mac(mac_addr);
        sysparam_sta_mac_update((const uint8_t *)mac_addr);
    }

    //2. net device(lwip)
    netdev_set_mac_addr(NETIF_IDX_STA, mac_addr);

    sysparam_sta_hostname_update(CFG_GetDeviceName());
    // static ip address
    g_STA_static_IP = (ip->localIPAddr[0] != 0) ;
       if (g_STA_static_IP){
            tcpip_ip_info_t  ip_info;
            convert_IP_to_string(g_IP, ip->localIPAddr);
            LOG(LOG_LVL_INFO, "INSIDE wifi_init_sta - setting static IP (%s)\r\n",g_IP);
            ip_info.ip.addr      = ipaddr_addr((const char *)g_IP);
            convert_IP_to_string(g_IP, ip->netMask);
            ip_info.netmask.addr = ipaddr_addr((const char *)g_IP);
            convert_IP_to_string(g_IP, ip->gatewayIPAddr);
            ip_info.gw.addr      = ipaddr_addr((const char *)g_IP);
	
            netdev_set_ip_info(NETIF_IDX_STA, &ip_info);
            dns_setserver(0,&ip->dnsServerIpAddr);
       } else  LOG(LOG_LVL_INFO, "INSIDE wifi_init_sta, no static IP - using DHCP\r\n");

#if PLATFORM_LN882H
    netdev_set_active(NETIF_IDX_STA);
#endif

    //3. wifi start
#if PLATFORM_LN882H
    wifi_manager_reg_event_callback(WIFI_MGR_EVENT_STA_SCAN_COMPLETE, &wifi_scan_complete_cb);

    if(WIFI_ERR_NONE != wifi_sta_start(mac_addr, ps_mode)){
        LOG(LOG_LVL_ERROR, "[%s]wifi sta start failed!!!\r\n", __func__);
    }

    connect.psk_value = NULL;
    if (strlen(connect.pwd) != 0) {
        if (0 == ln_psk_calc(connect.ssid, connect.pwd, psk_value, sizeof (psk_value))) {
            connect.psk_value = psk_value;
            hexdump(LOG_LVL_INFO, "psk value ", psk_value, sizeof(psk_value));
        }
    }
#endif

    if (g_wifiStatusCallback != NULL) {
        g_wifiStatusCallback(WIFI_STA_CONNECTING);
    }

#if PLATFORM_LN882H
    wifi_manager_reg_event_callback(WIFI_MGR_EVENT_STA_CONNECTED, &wifi_connected_cb);
    wifi_manager_reg_event_callback(WIFI_MGR_EVENT_STA_CONNECT_FAILED, &wifi_connect_failed_cb);
    extern void ln_wpa_sae_enable(void);
    ln_wpa_sae_enable();

    wifi_sta_connect(&connect, &scan_cfg);
#else
    netif_set_hostname(ethernetif_get_netif(STATION_IF), CFG_GetDeviceName());
    hal_sleep_set_mode(ACTIVE);
    wifi_set_mode(init_param.wifi_mode);
    wifi_set_config(STATION_IF, &connect);
    if(g_STA_static_IP)
    {
        reg_wifi_msg_callbcak(WIFI_MSG_ID_STA_CONNECTED, &wifi_connected_cb);
        reg_wifi_msg_callbcak(WIFI_MSG_ID_STA_DHCP_GOT_IP, NULL);
    }
    else
    {
        reg_wifi_msg_callbcak(WIFI_MSG_ID_STA_CONNECTED, NULL);
        reg_wifi_msg_callbcak(WIFI_MSG_ID_STA_DHCP_GOT_IP, &wifi_connected_cb);
    }
    reg_wifi_msg_callbcak(WIFI_MSG_ID_STA_DISCONNECTED, &wifi_disconnected_cb);
    reg_wifi_msg_callbcak(WIFI_MSG_ID_STA_DHCP_TIMEOUT, &wifi_disconnected_cb);
    //wifi_station_scan(&scan_cfg);

    if(!wifi_start(&init_param, true))
    {
        LOG(LOG_LVL_ERROR, "[%s, %d]wifi_start() fail.\r\n", __func__, __LINE__);
    }
    memcpy(connect.sta.ssid, oob_ssid, strlen(oob_ssid));
    memcpy(connect.sta.password, connect_key, strlen(connect_key));
    wifi_station_connect(&connect);
#endif
}

void HAL_ConnectToWiFi(const char* oob_ssid, const char* connect_key, obkStaticIP_t *ip)
{
	g_bOpenAccessPointMode = 0;
	wifi_init_sta(oob_ssid, connect_key, ip);
}

void HAL_DisconnectFromWifi()
{
    wifi_sta_disconnect();
    if (g_wifiStatusCallback != NULL) {
        g_wifiStatusCallback(WIFI_STA_DISCONNECTED);
    }
}

#if PLATFORM_LN882H

static void ap_startup_cb(void * arg)
{
    LN_UNUSED(arg);
    netdev_set_state(NETIF_IDX_AP, NETDEV_UP);
    if (g_wifiStatusCallback != NULL) {
        g_wifiStatusCallback(WIFI_AP_CONNECTED);
    }
}

void wifi_init_ap(const char* ssid)
{
    tcpip_ip_info_t  ip_info;
    server_config_t  server_config;

    ip_info.ip.addr      = ipaddr_addr((const char *)"192.168.4.1");
    ip_info.gw.addr      = ipaddr_addr((const char *)"192.168.4.1");
    ip_info.netmask.addr = ipaddr_addr((const char *)"255.255.255.0");

    server_config.server.addr   = ip_info.ip.addr;
    server_config.port          = 67;
    server_config.lease         = 2880;
    server_config.renew         = 2880;
    server_config.ip_start.addr = ipaddr_addr((const char *)"192.168.4.100");
    server_config.ip_end.addr   = ipaddr_addr((const char *)"192.168.4.150");
    server_config.client_max    = 3;
    dhcpd_curr_config_set(&server_config);

    // fix to generate unique AP MAC, mirrors STA code
    //1. AP mac get
    uint8_t mac_addr[6];
     if (SYSPARAM_ERR_NONE != sysparam_softap_mac_get(mac_addr)) {
        LOG(LOG_LVL_ERROR, "[%s]softap mac get failed!!!\r\n", __func__);
        return;
    }

    if (mac_addr[0] == SOFTAP_MAC_ADDR0 &&
        mac_addr[1] == SOFTAP_MAC_ADDR1 &&
        mac_addr[2] == SOFTAP_MAC_ADDR2 &&
        mac_addr[3] == SOFTAP_MAC_ADDR3 &&
        mac_addr[4] == SOFTAP_MAC_ADDR4 &&
        mac_addr[5] == SOFTAP_MAC_ADDR5) {
        ln_generate_random_mac(mac_addr);
        sysparam_softap_mac_update((const uint8_t *)mac_addr);
    }

    wifi_softap_cfg_t ap_cfg = {
		.ssid            = ssid,
		.pwd             = "",
		.bssid           = mac_addr,
		.ext_cfg = {
			.channel         = 6,
			.authmode        = WIFI_AUTH_OPEN,
			.ssid_hidden     = 0,
			.beacon_interval = 100,
			.psk_value = NULL,
		}
	};

    //2. net device(lwip).
    netdev_set_mac_addr(NETIF_IDX_AP, mac_addr);
    netdev_set_ip_info(NETIF_IDX_AP, &ip_info);
    netdev_set_active(NETIF_IDX_AP);
    wifi_manager_reg_event_callback(WIFI_MGR_EVENT_SOFTAP_STARTUP, &ap_startup_cb);

    ap_cfg.ext_cfg.psk_value = NULL;

    //3. wifi
    if(WIFI_ERR_NONE !=  wifi_softap_start(&ap_cfg)){
        LOG(LOG_LVL_ERROR, "[%s, %d]wifi_start() fail.\r\n", __func__, __LINE__);
        if (g_wifiStatusCallback != NULL) {
            g_wifiStatusCallback(WIFI_AP_FAILED);
        }
    }
}

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
	alert_log("Starting AP: %s", ssid);
	wifi_init_ap(ssid);

	alert_log("AP started, waiting for: netdev_got_ip()");
    while (!netdev_got_ip()) {
        OS_MsDelay(1000);
    }

	alert_log("AP started OK!");
	g_bOpenAccessPointMode = 1;

	return 0;
}

#else

int HAL_SetupWiFiOpenAccessPoint(const char* ssid)
{
    system_parameter_set_hostname(SOFT_AP_IF, CFG_GetDeviceName());

    uint8_t macaddr[6] = { 0 }, macaddr_default[6] = { 0 };
    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(ssid),
            .password = "",
            .channel = 1,
            .authmode = WIFI_AUTH_OPEN,
            .ssid_hidden = 0,
            .max_connection = 1,
            .beacon_interval = 100,
            .reserved = 0,
        },
    };
    memcpy(&wifi_config.ap.ssid, ssid, wifi_config.ap.ssid_len);
    wifi_init_type_t init_param = {
        .wifi_mode = WIFI_MODE_AP,
        .sta_ps_mode = WIFI_NO_POWERSAVE,
        .dhcp_mode = WLAN_DHCP_SERVER,
        .local_ip_addr = "192.168.4.1",
        .gateway_ip_addr = "192.168.4.1",
        .net_mask = "255.255.255.0",
    };

    wifi_set_mode(init_param.wifi_mode);

    system_parameter_get_wifi_macaddr_default(SOFT_AP_IF, macaddr_default);
    wifi_get_macaddr(SOFT_AP_IF, macaddr);
    if(ln_is_valid_mac((const char*)macaddr) && memcmp(macaddr, macaddr_default, 6) != 0)
    {
        wifi_set_macaddr_current(SOFT_AP_IF, macaddr);
    }
    else
    {
        ln_generate_random_mac(macaddr);
        wifi_set_macaddr(SOFT_AP_IF, macaddr);
    }

    wifi_set_config(SOFT_AP_IF, &wifi_config);

    netif_set_hostname(ethernetif_get_netif(SOFT_AP_IF), CFG_GetDeviceName());
    wifi_start(&init_param, true);
    g_bOpenAccessPointMode = 1;

    return 0;
}

#endif

#endif // PLATFORM_LN882H
