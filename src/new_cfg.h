#ifndef __NEW_CFG_H__
#define __NEW_CFG_H__

#include "new_common.h"

// bart.nexellent.net
#define DEFAULT_NTP_SERVER "217.147.223.78"

extern int g_cfg_pendingChanges;

const char *CFG_GetDeviceName();
const char *CFG_GetShortDeviceName();
void CFG_SetShortDeviceName(const char *s);
void CFG_SetDeviceName(const char *s);
int CFG_GetMQTTPort();
void CFG_SetMQTTPort(int p);
void CFG_SetOpenAccessPoint();
void CFG_MarkAsDirty();
void CFG_ClearIO();
void CFG_SetDefaultConfig();
const char *CFG_GetWiFiSSID();
const char *CFG_GetWiFiPass();
const char *CFG_GetWiFiSSID2();
const char *CFG_GetWiFiPass2();
int CFG_SetWiFiSSID(const char *s);
int CFG_SetWiFiPass(const char *s);
int CFG_SetWiFiSSID2(const char *s);
int CFG_SetWiFiPass2(const char *s);
const char *CFG_GetMQTTHost();
const char *CFG_GetMQTTClientId();
const char *CFG_GetMQTTGroupTopic();
const char *CFG_GetMQTTUserName();
const char *CFG_GetMQTTPass();
void CFG_SetMQTTHost(const char *s);
void CFG_SetMQTTClientId(const char *s);
void CFG_SetMQTTUserName(const char *s);
void CFG_SetMQTTGroupTopic(const char *s);
void CFG_SetMQTTPass(const char *s);
const char *CFG_GetWebappRoot();
void CFG_SetLEDRemap(int r, int g, int b, int c, int w);
void CFG_SetDefaultLEDRemap(int r, int g, int b, int c, int w);
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
void CFG_SetFlags(int first4bytes, int second4bytes);
void CFG_SetFlag(int flag, bool bValue);
bool CFG_HasFlag(int flag);
void CFG_SetLoggerFlag(int flag, bool bValue);
bool CFG_HasLoggerFlag(int flag);
void CFG_SetMac(char *mac);
int CFG_GetFlags();
unsigned long CFG_GetFlags64();
const char* CFG_GetNTPServer();
void CFG_SetNTPServer(const char *s);
// BL0937, BL0942, etc constants
// Functions below assume that 0 is not a valid value, it means "use default"
// This is because we won't ever need to divide or multiply a measurement result by zero
int CFG_GetPowerMeasurementCalibrationInteger(int index, int def);
void CFG_SetPowerMeasurementCalibrationInteger(int index, int value);
float CFG_GetPowerMeasurementCalibrationFloat(int index, float def);
void CFG_SetPowerMeasurementCalibrationFloat(int index, float value);
void CFG_SetButtonLongPressTime(int value);
void CFG_SetButtonShortPressTime(int value);
void CFG_SetButtonRepeatPressTime(int value);
const char *CFG_GetWebPassword();
void CFG_SetWebPassword(const char *s);

#if ENABLE_LITTLEFS
    void CFG_SetLFS_Size(uint32_t value);
    uint32_t CFG_GetLFS_Size();
#endif 

#endif

