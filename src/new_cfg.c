
#include "new_common.h"
#include "httpserver/new_http.h"
#include "new_pins.h"

#if WINDOWS

#else
#include "../../beken378/func/include/net_param_pub.h"
#include "../../beken378/app/config/param_config.h"
#endif

static int g_mqtt_port = 1883;
static char g_mqtt_host[64] = "192.168.0.113";
static char g_mqtt_brokerName[64] = "test";
static char g_mqtt_userName[64] = "homeassistant"; 
static char g_mqtt_pass[128] = "qqqqqqqqqq"; 
static char g_wifi_ssid[64] = { 0 };
static char g_wifi_pass[64] = { 0 };

// Long unique device name, like OpenBK7231T_AABBCCDD
char g_deviceName[64] = "testDev";
// Short unique device name, like obkAABBCCDD
char g_shortDeviceName[64] = "td01";

const char *CFG_GetDeviceName(){
	return g_deviceName;
}
const char *CFG_GetShortDeviceName(){
	return g_shortDeviceName;
}
void CFG_CreateDeviceNameUnique()
{
	// must be unsigned, else print below prints negatives as e.g. FFFFFFFe
	unsigned char mac[32];
#if WINDOWS

#else
    wifi_get_mac_address((char *)mac, CONFIG_ROLE_STA);
#endif
	sprintf(g_deviceName,"OpenBK7231T_%02X%02X%02X%02X",mac[2],mac[3],mac[4],mac[5]);
	sprintf(g_shortDeviceName,"obk%02X%02X%02X%02X",mac[2],mac[3],mac[4],mac[5]);

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

#else
	ITEM_NEW_WIFI_CONFIG container;
	strcpy_safe(container.ssid, g_wifi_ssid, sizeof(container.ssid));
	strcpy_safe(container.pass, g_wifi_pass, sizeof(container.pass));
	save_info_item(NEW_WIFI_CONFIG,(UINT8 *)&container, 0, 0);
#endif
}
void CFG_LoadWiFi() {
#if WINDOWS

#else
	ITEM_NEW_WIFI_CONFIG container;
	if(get_info_item(NEW_WIFI_CONFIG,(UINT8 *)&container, 0, 0) != 0) {
		strcpy_safe(g_wifi_ssid,container.ssid,sizeof(g_wifi_ssid));
		strcpy_safe(g_wifi_pass,container.pass,sizeof(g_wifi_pass));
	} 
#endif
}

void CFG_SaveMQTT() {
#if WINDOWS

#else
	ITEM_NEW_MQTT_CONFIG container;
	strcpy_safe(container.userName, g_mqtt_userName, sizeof(container.userName));
	strcpy_safe(container.pass, g_mqtt_pass, sizeof(container.pass));
	strcpy_safe(container.hostName, g_mqtt_host, sizeof(container.hostName));
	strcpy_safe(container.brokerName, g_mqtt_brokerName, sizeof(container.brokerName));
	container.port = g_mqtt_port;
	save_info_item(NEW_MQTT_CONFIG,(UINT8 *)&container, 0, 0);
	
#endif
}
void CFG_LoadMQTT() {
#if WINDOWS

#else
	ITEM_NEW_MQTT_CONFIG container;
	if(get_info_item(NEW_MQTT_CONFIG,(UINT8 *)&container, 0, 0) != 0) {
		strcpy_safe(g_mqtt_userName,container.userName,sizeof(g_mqtt_userName));
		strcpy_safe(g_mqtt_pass,container.pass,sizeof(g_mqtt_pass));
		strcpy_safe(g_mqtt_host,container.hostName,sizeof(g_mqtt_host));
		strcpy_safe(g_mqtt_brokerName,container.brokerName,sizeof(g_mqtt_brokerName));
		g_mqtt_port = container.port;
	}
#endif
}







