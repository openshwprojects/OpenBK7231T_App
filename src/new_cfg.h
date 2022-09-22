#ifndef __NEW_CFG_H__
#define __NEW_CFG_H__

#include "new_common.h"


extern int g_cfg_pendingChanges;
typedef struct
{
   bool  red_led;
  bool green_led;
  bool blue_led;
   bool white_led;
   bool led_offall;
   uint8_t r_brightness;
   uint8_t b_brightness;
   uint8_t g_brightness;
   uint8_t w_brightness;
   uint8_t r_pin;
   uint8_t b_pin;
   uint8_t g_pin;
   uint8_t w_pin;
   uint8_t r_channel;
   uint8_t b_channel;
   uint8_t g_channel;
   uint8_t w_channel;
   // target wifi credentials
   	char blub_wifi_ssid[64];
   	char blub_wifi_pass[64];
   	char blub_device_id[64];
   	// MQTT information for Home Assistant
   	char blub_mqtt_host[256];
   	char blub_mqtt_brokerName[64];
   	char blub_mqtt_userName[64];
   	char blub_mqtt_pass[128];
   	int blub_mqtt_port;
   	char blub_mqtt_topic[128];

  } smartblub_config_data_t;

const char *CFG_GetDeviceName();
const char *CFG_GetShortDeviceName();
void CFG_SetShortDeviceName(const char *s);
void CFG_SetDeviceName(const char *s);
void CFG_CreateDeviceNameUnique();
int CFG_GetMQTTPort();
void CFG_SetMQTTPort(int p);
void CFG_SetOpenAccessPoint();
void CFG_SetDefaultConfig();
const char *CFG_GetWiFiSSID();
const char *CFG_GetWiFiPass();
void CFG_SetWiFiSSID(const char *s);
void CFG_SetWiFiPass(const char *s);
const char *CFG_GetMQTTHost();
const char *CFG_GetMQTTBrokerName();
const char *CFG_GetMQTTUserName();
const char *CFG_GetMQTTPass();
void CFG_SetMQTTHost(const char *s);
void CFG_SetMQTTBrokerName(const char *s);
void CFG_SetMQTTUserName(const char *s);
void CFG_SetMQTTPass(const char *s);
const char *CFG_GetWebappRoot();
int CFG_SetWebappRoot(const char *s);
void CFG_InitAndLoad();
//void CFG_ApplyStartChannelValues();
void CFG_Save_IfThereArePendingChanges();
void CFG_Save_SetupTimer();
void CFG_IncrementOTACount();
// This is a short startup command stored along with config.
// One could say that's a very crude LittleFS replacement.
// see mainConfig_t::initCommandLine
// I only added this because there was free space in the Flash sector used for config on various devices
void CFG_SetShortStartupCommand(const char *s);
void CFG_SetShortStartupCommand_AndExecuteNow(const char *s);
const char *CFG_GetShortStartupCommand();
const char *CFG_GetPingHost();
int CFG_GetPingDisconnectedSecondsToRestart();
int CFG_GetPingIntervalSeconds();
void CFG_SetPingHost(const char *s);
void CFG_SetPingDisconnectedSecondsToRestart(int i);
void CFG_SetPingIntervalSeconds(int i);
void CFG_SetBootOkSeconds(int v);
int CFG_GetBootOkSeconds();
void CFG_SetChannelStartupValue(int channelIndex,short newValue);
short CFG_GetChannelStartupValue(int channelIndex);
void CFG_ApplyChannelStartValues();
void CFG_DeviceGroups_SetName(const char *s);
void CFG_DeviceGroups_SetSendFlags(int newSendFlags);
void CFG_DeviceGroups_SetRecvFlags(int newSendFlags);
const char *CFG_DeviceGroups_GetName();
int CFG_DeviceGroups_GetSendFlags();
int CFG_DeviceGroups_GetRecvFlags();
void CFG_SetFlag(int flag, bool bValue);
bool CFG_HasFlag(int flag);
int CFG_GetFlags();
const char* CFG_GetNTPServer();
void CFG_SetNTPServer(const char *s);


#endif 


