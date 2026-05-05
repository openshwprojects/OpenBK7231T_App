#ifndef __NEW_CFG_H__
#define __NEW_CFG_H__

#include "new_common.h"
#include "new_pins.h"

// bart.nexellent.net
#define DEFAULT_NTP_SERVER "217.147.223.78"

extern mainConfig_t g_cfg;
extern int g_cfg_pendingChanges;
extern int g_configInitialized;

static inline const char* CFG_GetDeviceName()
{
	return g_cfg.longDeviceName;
}

static inline const char* CFG_GetShortDeviceName()
{
	return g_cfg.shortDeviceName;
}

static inline int CFG_GetMQTTPort()
{
	return g_cfg.mqtt_port;
}
static inline void CFG_MarkAsDirty()
{
	g_cfg_pendingChanges++;
}
static inline const char* CFG_GetWiFiSSID()
{
	return g_cfg.wifi_ssid;
}
static inline const char* CFG_GetWiFiPass()
{
	return g_cfg.wifi_pass;
}
static inline const char* CFG_GetWiFiSSID2()
{
#if ALLOW_SSID2
	return g_cfg.wifi_ssid2;
#else
	return "";
#endif
}
static inline const char* CFG_GetWiFiPass2()
{
#if ALLOW_SSID2
	return g_cfg.wifi_pass2;
#else
	return "";
#endif
}
static inline const char* CFG_GetMQTTHost()
{
	return g_cfg.mqtt_host;
}
static inline const char* CFG_GetMQTTClientId()
{
	return g_cfg.mqtt_clientId;
}
static inline const char* CFG_GetMQTTGroupTopic()
{
	return g_cfg.mqtt_group;
}
static inline const char* CFG_GetMQTTUserName()
{
	return g_cfg.mqtt_userName;
}
static inline const char* CFG_GetMQTTPass()
{
	return g_cfg.mqtt_pass;
}
static inline const char* CFG_GetWebappRoot()
{
	return g_cfg.webappRoot;
}

static inline void CFG_IncrementOTACount()
{
	g_cfg.otaCounter++;
	g_cfg_pendingChanges++;
}

static inline const char* CFG_GetShortStartupCommand()
{
	return g_cfg.initCommandLine;
}

static inline const char* CFG_GetPingHost()
{
	return g_cfg.ping_host;
}
static inline int CFG_GetPingDisconnectedSecondsToRestart()
{
	return g_cfg.ping_seconds;
}
static inline int CFG_GetPingIntervalSeconds()
{
	return g_cfg.ping_interval;
}

static inline void CFG_SetPingHost(const char* s)
{
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.ping_host, s, sizeof(g_cfg.ping_host)))
	{
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
static inline void CFG_SetPingDisconnectedSecondsToRestart(int i)
{
	if(g_cfg.ping_seconds != i)
	{
		g_cfg.ping_seconds = i;
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
static inline void CFG_SetPingIntervalSeconds(int i)
{
	if(g_cfg.ping_interval != i)
	{
		g_cfg.ping_interval = i;
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
static inline const char* CFG_DeviceGroups_GetName()
{
	return g_cfg.dgr_name;
}
static inline int CFG_DeviceGroups_GetSendFlags()
{
	return g_cfg.dgr_sendFlags;
}
static inline int CFG_DeviceGroups_GetRecvFlags()
{
	return g_cfg.dgr_recvFlags;
}
static inline bool CFG_HasLoggerFlag(int flag)
{
	return BIT_CHECK(g_cfg.loggerFlags, flag);
}
static inline int CFG_GetFlags()
{
	return g_cfg.genericFlags;
}
static inline uint64_t CFG_GetFlags64()
{
	//unsigned long long* pAllGenericFlags = (unsigned long*)&g_cfg.genericFlags;
	//*pAllGenericFlags;
	return (uint64_t)g_cfg.genericFlags | (uint64_t)g_cfg.genericFlags2 << 32;
}
static inline const char* CFG_GetNTPServer()
{
	return g_cfg.ntpServer;
}
// BL0937, BL0942, etc constants
// Functions below assume that 0 is not a valid value, it means "use default"
// This is because we won't ever need to divide or multiply a measurement result by zero
static inline int CFG_GetPowerMeasurementCalibrationInteger(int index, int def)
{
	if(g_cfg.cal.values[index].i == 0)
	{
		return def;
	}
	return g_cfg.cal.values[index].i;
}
static inline void CFG_SetPowerMeasurementCalibrationInteger(int index, int value)
{
	if(g_cfg.cal.values[index].i != value)
	{
		g_cfg.cal.values[index].i = value;
		g_cfg_pendingChanges++;
	}
}
static inline float CFG_GetPowerMeasurementCalibrationFloat(int index, float def)
{
	if(g_cfg.cal.values[index].i == 0)
	{
		return def;
	}
	return g_cfg.cal.values[index].f;
}
static inline void CFG_SetPowerMeasurementCalibrationFloat(int index, float value)
{
	if(g_cfg.cal.values[index].f != value)
	{
		g_cfg.cal.values[index].f = value;
		g_cfg_pendingChanges++;
	}
}
static inline void CFG_SetButtonLongPressTime(int value)
{
	if(g_cfg.buttonLongPress != value)
	{
		g_cfg.buttonLongPress = value;
		g_cfg_pendingChanges++;
	}
}
static inline void CFG_SetButtonShortPressTime(int value)
{
	if(g_cfg.buttonShortPress != value)
	{
		g_cfg.buttonShortPress = value;
		g_cfg_pendingChanges++;
	}
}
static inline void CFG_SetButtonRepeatPressTime(int value)
{
	if(g_cfg.buttonHoldRepeat != value)
	{
		g_cfg.buttonHoldRepeat = value;
		g_cfg_pendingChanges++;
	}
}
static inline const char* CFG_GetWebPassword()
{
#if ALLOW_WEB_PASSWORD
	return g_cfg.webPassword;
#else
	return "";
#endif
}

static inline void CFG_ClearPins()
{
	memset(&g_cfg.pins, 0, sizeof(g_cfg.pins));
	g_cfg_pendingChanges++;
}

void CFG_SetShortDeviceName(const char* s);
void CFG_SetDeviceName(const char* s);
void CFG_SetMQTTPort(int p);
void CFG_SetOpenAccessPoint();
void CFG_ClearIO();
void CFG_SetDefaultConfig();
int CFG_SetWiFiSSID(const char* s);
int CFG_SetWiFiPass(const char* s);
int CFG_SetWiFiSSID2(const char* s);
int CFG_SetWiFiPass2(const char* s);
void CFG_SetMQTTHost(const char* s);
void CFG_SetMQTTClientId(const char* s);
void CFG_SetMQTTUserName(const char* s);
void CFG_SetMQTTGroupTopic(const char* s);
void CFG_SetMQTTPass(const char* s);
void CFG_SetLEDRemap(int r, int g, int b, int c, int w);
void CFG_SetDefaultLEDRemap(int r, int g, int b, int c, int w);
int CFG_CountLEDRemapChannels();
int CFG_SetWebappRoot(const char* s);
void CFG_InitAndLoad();
//void CFG_ApplyStartChannelValues();
void CFG_Save_IfThereArePendingChanges();
void CFG_Save_SetupTimer();
// This is a short startup command stored along with config.
// One could say that's a very crude LittleFS replacement.
// see mainConfig_t::initCommandLine
// I only added this because there was free space in the Flash sector used for config on various devices
void CFG_SetShortStartupCommand(const char* s);
void CFG_SetShortStartupCommand_AndExecuteNow(const char* s);
void CFG_SetBootOkSeconds(int v);
int CFG_GetBootOkSeconds();
void CFG_SetChannelStartupValue(int channelIndex, short newValue);
short CFG_GetChannelStartupValue(int channelIndex);
void CFG_ApplyChannelStartValues();
void CFG_DeviceGroups_SetName(const char* s);
void CFG_DeviceGroups_SetSendFlags(int newSendFlags);
void CFG_DeviceGroups_SetRecvFlags(int newSendFlags);
void CFG_SetFlags(uint32_t first4bytes, uint32_t second4bytes);
void CFG_SetFlag(int flag, bool bValue);
bool CFG_HasFlag(int flag);
void CFG_SetLoggerFlag(int flag, bool bValue);
void CFG_SetMac(char* mac);
void CFG_SetNTPServer(const char* s);
void CFG_SetWebPassword(const char* s);

#if ENABLE_LITTLEFS
void CFG_SetLFS_Size(uint32_t value);
uint32_t CFG_GetLFS_Size();
#endif 

#if MQTT_USE_TLS
void CFG_SetMQTTUseTls(byte value);
void CFG_SetMQTTVerifyTlsCert(byte value);
void CFG_SetMQTTCertFile(const char* s);
byte CFG_GetMQTTUseTls();
byte CFG_GetMQTTVerifyTlsCert();
const char* CFG_GetMQTTCertFile();
byte CFG_GetDisableWebServer();
void CFG_SetDisableWebServer(byte value);
#endif

#endif

