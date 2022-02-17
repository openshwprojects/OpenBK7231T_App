
#include "new_common.h"
#include "httpserver/new_http.h"
#include "new_pins.h"
#include "new_cfg.h"

#if WINDOWS

#elif PLATFORM_XR809
// XR809 sysinfo is used to save configuration to flash
#include "common/framework/sysinfo.h"
#else
#include "flash_config/flash_config.h"
#include "../../beken378/func/include/net_param_pub.h"
#include "../../beken378/app/config/param_config.h"
#endif


// added for OpenBK7231T
#define NEW_WEBAPP_CONFIG_SIZE 64
//NEW_PINS_CONFIG

static int g_mqtt_port = 1883;
static char g_mqtt_host[64] = "192.168.0.113";
static char g_mqtt_brokerName[64] = "test";
static char g_mqtt_userName[64] = "homeassistant"; 
static char g_mqtt_pass[128] = "qqqqqqqqqq"; 
static char g_wifi_ssid[64] = { 0 };
static char g_wifi_pass[64] = { 0 };

#if PLATFORM_XR809 || WINDOWS
#define CONFIG_URL_SIZE_MAX 64
#endif

static char g_webappRoot[CONFIG_URL_SIZE_MAX] = "https://openbekeniot.github.io/webapp/";

// Long unique device name, like OpenBK7231T_AABBCCDD
char g_deviceName[64] = "testDev";
// Short unique device name, like obkAABBCCDD
char g_shortDeviceName[64] = "td01";

const char *CFG_LoadWebappRoot(){
#if WINDOWS

#elif PLATFORM_XR809

#else
	ITEM_URL_CONFIG item;
	int res;
	CONFIG_INIT_ITEM(CONFIG_TYPE_WEBAPP_ROOT, &item);
	res = config_get_item(&item);
	if (res) 
		strcpy_safe(g_webappRoot, item.url,sizeof(g_webappRoot));
#endif
	return g_webappRoot;
}

const char *CFG_GetWebappRoot(){
	return g_webappRoot;
}

int CFG_SetWebappRoot(const char *s) {
#if WINDOWS
	strcpy_safe(g_webappRoot, s,sizeof(g_webappRoot));
	return 1; // ok
#elif PLATFORM_XR809
	strcpy_safe(g_webappRoot, s,sizeof(g_webappRoot));
	return 1; // ok

#else
	ITEM_URL_CONFIG item;
	int res;
	CONFIG_INIT_ITEM(CONFIG_TYPE_WEBAPP_ROOT, &item);
	res = config_get_item(&item);
	strcpy_safe(item.url, s,sizeof(item.url));
	strcpy_safe(g_webappRoot, item.url,sizeof(g_webappRoot));
	
	if(config_save_item(&item)) {
		return 1;
	}
	return 0;
#endif
}

const char *CFG_GetDeviceName(){
	return g_deviceName;
}
const char *CFG_GetShortDeviceName(){
	return g_shortDeviceName;
}

#if WINDOWS
#define DEVICENAME_PREFIX_FULL "WinTest"
#define DEVICENAME_PREFIX_SHORT "WT"
#elif PLATFORM_XR809
#define DEVICENAME_PREFIX_FULL "OpenXR809"
#define DEVICENAME_PREFIX_SHORT "oxr"
#elif PLATFORM_BK7231N
#define DEVICENAME_PREFIX_FULL "OpenBK7231N"
#define DEVICENAME_PREFIX_SHORT "obk"
#elif PLATFORM_BK7231T
#define DEVICENAME_PREFIX_FULL "OpenBK7231T"
#define DEVICENAME_PREFIX_SHORT "obk"
#else
#error "You must define a platform.."
This platform is not supported, error!
#endif

void WiFI_GetMacAddress(char *mac) {
#if WINDOWS

#elif PLATFORM_XR809
	sysinfo_t *inf;
	inf = sysinfo_get();
	if(inf == 0) {
		mac[0] = 'E'; mac[1] = 'R'; mac[2] = 'R'; mac[3] = 'O'; mac[4] = 'R'; mac[5] = '!';
		printf("WiFI_GetMacAddress: sysinfo_get returned 0!\n\r");
		return;
	}
	memcpy(mac,inf->mac_addr,6);
#else
    wifi_get_mac_address((char *)mac, CONFIG_ROLE_STA);
#endif
}
int WiFI_SetMacAddress(char *mac) {
#if WINDOWS
	return 0;
#elif PLATFORM_XR809
	sysinfo_t *inf;
	int res;
	inf = sysinfo_get();
	if(inf == 0) {
		printf("WiFI_SetMacAddress: sysinfo_get returned 0!\n\r");
		return 0; // error
	}
	memcpy(inf->mac_addr,mac,6);
	res = sysinfo_save_wrapper();
	if(res != 0) {
		printf("WiFI_SetMacAddress: sysinfo_save error - %i!\n\r",res);
		return 0; // error
	}
	return 1;
#else
   if(wifi_set_mac_address((char *)mac))
	   return 1;
   return 0; // error
#endif
}
void CFG_CreateDeviceNameUnique()
{
	// must be unsigned, else print below prints negatives as e.g. FFFFFFFe
	unsigned char mac[32];

	WiFI_GetMacAddress((char *)mac);

	sprintf(g_deviceName,DEVICENAME_PREFIX_FULL"_%02X%02X%02X%02X",mac[2],mac[3],mac[4],mac[5]);
	sprintf(g_shortDeviceName,DEVICENAME_PREFIX_SHORT"%02X%02X%02X%02X",mac[2],mac[3],mac[4],mac[5]);

		// NOT WORKING, I done it other way, see ethernetif.c
	//net_dhcp_hostname_set(g_shortDeviceName);
}

int CFG_GetMQTTPort() {
	return g_mqtt_port;
}
void CFG_SetMQTTPort(int p) {
	g_mqtt_port = p;
}
void CFG_SetOpenAccessPoint() {
	g_wifi_ssid[0] = 0;
	g_wifi_pass[0] = 0;
}
const char *CFG_GetWiFiSSID(){
	return g_wifi_ssid;
}
const char *CFG_GetWiFiPass(){
	return g_wifi_pass;
}
void CFG_SetWiFiSSID(const char *s) {
	strcpy_safe(g_wifi_ssid,s,sizeof(g_wifi_ssid));
}
void CFG_SetWiFiPass(const char *s) {
	strcpy_safe(g_wifi_pass,s,sizeof(g_wifi_pass));
}
const char *CFG_GetMQTTHost() {
	return g_mqtt_host;
}
const char *CFG_GetMQTTBrokerName() {
	return g_mqtt_brokerName;
}
const char *CFG_GetMQTTUserName() {
	return g_mqtt_userName;
}
const char *CFG_GetMQTTPass() {
	return g_mqtt_pass;
}
void CFG_SetMQTTHost(const char *s) {
	strcpy_safe(g_mqtt_host,s,sizeof(g_mqtt_host));
}
void CFG_SetMQTTBrokerName(const char *s) {
	strcpy_safe(g_mqtt_brokerName,s,sizeof(g_mqtt_brokerName));
}
void CFG_SetMQTTUserName(const char *s) {
	strcpy_safe(g_mqtt_userName,s,sizeof(g_mqtt_userName));
}
void CFG_SetMQTTPass(const char *s) {
	strcpy_safe(g_mqtt_pass,s,sizeof(g_mqtt_pass));
}
void CFG_SaveWiFi() {
#if WINDOWS

#elif PLATFORM_XR809
	sysinfo_t *inf;
	int res;
	inf = sysinfo_get();
	if(inf == 0) {
		printf("CFG_SaveWiFi: sysinfo_get returned 0!\n\r");
		return;
	}
	strcpy_safe((char*)inf->wlan_sta_param.ssid, g_wifi_ssid, sizeof(inf->wlan_sta_param.ssid));
	strcpy_safe((char*)inf->wlan_sta_param.psk, g_wifi_pass, sizeof(inf->wlan_sta_param.psk));

	res = sysinfo_save_wrapper();
	if(res != 0) {
		printf("CFG_SaveWiFi: sysinfo_save error - %i!\n\r",res);
	}
#else
	ITEM_NEW_WIFI_CONFIG container;
	os_memset(&container, 0, sizeof(container));
	CONFIG_INIT_ITEM(CONFIG_TYPE_WIFI, &container);
	strcpy_safe(container.ssid, g_wifi_ssid, sizeof(container.ssid));
	strcpy_safe(container.pass, g_wifi_pass, sizeof(container.pass));
	config_save_item(&container);
#endif
}
void CFG_LoadWiFi() {
#if WINDOWS

#elif PLATFORM_XR809
	sysinfo_t *inf;
	inf = sysinfo_get();
	if(inf == 0) {
		printf("CFG_LoadWiFi: sysinfo_get returned 0!\n\r");
		return;
	}
	strcpy_safe(g_wifi_ssid,(char*)inf->wlan_sta_param.ssid,sizeof(g_wifi_ssid));
	strcpy_safe(g_wifi_pass,(char*)inf->wlan_sta_param.psk,sizeof(g_wifi_pass));
#else
	{
		// try to read 'old' structure with extra 8 bytes
		// if we find it, delete and re-save with new structure
		ITEM_NEW_WIFI_CONFIG2 container;
		CONFIG_INIT_ITEM(OLD_WIFI_CONFIG, &container);
		if (config_get_item(&container) != 0){
			strcpy_safe(g_wifi_ssid,container.ssid,sizeof(g_wifi_ssid));
			strcpy_safe(g_wifi_pass,container.pass,sizeof(g_wifi_pass));
			// delete and re-save
			config_delete_item(OLD_WIFI_CONFIG);
			CFG_SaveWiFi();
		} 
	}
	{
		ITEM_NEW_NEW_WIFI_CONFIG container;
		CONFIG_INIT_ITEM(CONFIG_TYPE_WIFI, &container);
		if (config_get_item(&container) != 0){
			strcpy_safe(g_wifi_ssid,container.ssid,sizeof(g_wifi_ssid));
			strcpy_safe(g_wifi_pass,container.pass,sizeof(g_wifi_pass));
		} 
	}
#endif
}
#if PLATFORM_XR809
int sysinfo_checksum(sysinfo_t *inf) {
	int crc = 0;
	crc ^= Tiny_CRC8((const char*)&inf->mac_addr,sizeof(inf->mac_addr));
	crc ^= Tiny_CRC8((const char*)&inf->wlan_mode,sizeof(inf->wlan_mode));
	crc ^= Tiny_CRC8((const char*)&inf->wlan_sta_param,sizeof(inf->wlan_sta_param));
	crc ^= Tiny_CRC8((const char*)&inf->wlan_ap_param,sizeof(inf->wlan_ap_param));
	crc ^= Tiny_CRC8((const char*)&inf->netif_sta_param,sizeof(inf->netif_sta_param));
	crc ^= Tiny_CRC8((const char*)&inf->netif_ap_param,sizeof(inf->netif_ap_param));
	crc ^= Tiny_CRC8((const char*)&inf->mqtt_param,sizeof(inf->mqtt_param));
	crc ^= Tiny_CRC8((const char*)&inf->pins,sizeof(inf->pins));

	return crc;
}
int sysinfo_save_wrapper() {
	sysinfo_t *inf;
	int res;
	inf = sysinfo_get();
	if(inf == 0) {
		printf("sysinfo_save_wrapper: sysinfo_get returned 0!\n\r");
		return -1;
	}
	printf("sysinfo_save_wrapper: going to calc checksum!\n\r");
	inf->checksum = sysinfo_checksum(inf);
	printf("sysinfo_save_wrapper: going to call save!\n\r");
	res = sysinfo_save();
	if(res != 0) {
		printf("sysinfo_save_wrapper: sysinfo_save returned error!\n\r");
	}
	printf("sysinfo_save_wrapper: done!\n\r");
	return 0;
}
#endif
int CFG_SaveMQTT() {
#if WINDOWS
	return 0;
#elif PLATFORM_XR809
	sysinfo_t *inf;
	int res;
	inf = sysinfo_get();
	if(inf == 0) {
		printf("CFG_SaveMQTT: sysinfo_get returned 0!\n\r");
		return 0;
	}
	strcpy_safe(inf->mqtt_param.userName, g_mqtt_userName, sizeof(inf->mqtt_param.userName));
	strcpy_safe(inf->mqtt_param.pass, g_mqtt_pass, sizeof(inf->mqtt_param.pass));
	strcpy_safe(inf->mqtt_param.hostName, g_mqtt_host, sizeof(inf->mqtt_param.hostName));
	strcpy_safe(inf->mqtt_param.brokerName, g_mqtt_brokerName, sizeof(inf->mqtt_param.brokerName));
	inf->mqtt_param.port = g_mqtt_port;

	printf("CFG_SaveMQTT: sysinfo will save inf->mqtt_param.userName - %s!\n\r",inf->mqtt_param.userName);
	printf("CFG_SaveMQTT: sysinfo will save inf->mqtt_param.hostName - %s!\n\r",inf->mqtt_param.hostName);
	res = sysinfo_save_wrapper();
	if(res != 0) {
		printf("CFG_SaveMQTT: sysinfo_save error - %i!\n\r",res);
		return 0;
	}
#else
	ITEM_NEW_MQTT_CONFIG container;
	os_memset(&container, 0, sizeof(container));
	CONFIG_INIT_ITEM(CONFIG_TYPE_MQTT, &container);
	strcpy_safe(container.userName, g_mqtt_userName, sizeof(container.userName));
	strcpy_safe(container.pass, g_mqtt_pass, sizeof(container.pass));
	strcpy_safe(container.hostName, g_mqtt_host, sizeof(container.hostName));
	strcpy_safe(container.brokerName, g_mqtt_brokerName, sizeof(container.brokerName));
	container.port = g_mqtt_port;
	if(config_save_item(&container))
		return 1;
	return 0;
	
#endif
}
void CFG_LoadMQTT() {
#if WINDOWS

#elif PLATFORM_XR809
	sysinfo_t *inf;
	inf = sysinfo_get();
	if(inf == 0) {
		printf("CFG_LoadMQTT: sysinfo_get returned 0!\n\r");
		return;
	}
	strcpy_safe(g_mqtt_userName,inf->mqtt_param.userName,sizeof(g_mqtt_userName));
	strcpy_safe(g_mqtt_pass,inf->mqtt_param.pass,sizeof(g_mqtt_pass));
	strcpy_safe(g_mqtt_host,inf->mqtt_param.hostName,sizeof(g_mqtt_host));
	strcpy_safe(g_mqtt_brokerName,inf->mqtt_param.brokerName,sizeof(g_mqtt_brokerName));
	g_mqtt_port = inf->mqtt_param.port;
	
	printf("CFG_LoadMQTT: sysinfo has been loaded!\n\r");
	printf("CFG_LoadMQTT: SYSINFO_SIZE is %i!\n\r",SYSINFO_SIZE);
	printf("CFG_LoadMQTT: g_mqtt_userName is %s!\n\r",g_mqtt_userName);
	printf("CFG_LoadMQTT: g_mqtt_host is %s!\n\r",g_mqtt_host);

#else
	{
		ITEM_NEW_MQTT_CONFIG2 container;
		CONFIG_INIT_ITEM(NEW_MQTT_CONFIG, &container);
		if (config_get_item(&container) != 0){
			strcpy_safe(g_mqtt_userName,container.userName,sizeof(g_mqtt_userName));
			strcpy_safe(g_mqtt_pass,container.pass,sizeof(g_mqtt_pass));
			strcpy_safe(g_mqtt_host,container.hostName,sizeof(g_mqtt_host));
			strcpy_safe(g_mqtt_brokerName,container.brokerName,sizeof(g_mqtt_brokerName));
			g_mqtt_port = container.port;

			// delete and re-save
			config_delete_item(OLD_MQTT_CONFIG);
			CFG_SaveMQTT();
		}
	}
	{
		ITEM_NEW_NEW_MQTT_CONFIG container;
		CONFIG_INIT_ITEM(CONFIG_TYPE_MQTT, &container);
		if (config_get_item(&container) != 0){
			strcpy_safe(g_mqtt_userName,container.userName,sizeof(g_mqtt_userName));
			strcpy_safe(g_mqtt_pass,container.pass,sizeof(g_mqtt_pass));
			strcpy_safe(g_mqtt_host,container.hostName,sizeof(g_mqtt_host));
			strcpy_safe(g_mqtt_brokerName,container.brokerName,sizeof(g_mqtt_brokerName));
			g_mqtt_port = container.port;
		}
	}
#endif
}

void CFG_InitAndLoad() {
	CFG_CreateDeviceNameUnique();
    CFG_LoadWebappRoot();
	CFG_LoadWiFi();
	CFG_LoadMQTT();
	PIN_LoadFromFlash();
}
