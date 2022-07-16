
#include "new_common.h"
#include "logging/logging.h"
#include "httpserver/new_http.h"
#include "new_pins.h"
#include "new_cfg.h"
#include "hal/hal_wifi.h"
#include "hal/hal_flashConfig.h"
#include "cmnds/cmd_public.h"

#define DEFAULT_BOOT_SUCCESS_TIME 30

mainConfig_t g_cfg;
int g_configInitialized = 0;
int g_cfg_pendingChanges = 0;

#define CFG_IDENT_0 'C'
#define CFG_IDENT_1 'F'
#define CFG_IDENT_2 'G'

#define MAIN_CFG_VERSION 2

// version v1
// Version v2 is now flexible and doesnt have to be duplicated
// in order to support previous versions any more
typedef struct mainConfig_v1_s {
	byte ident0;
	byte ident1;
	byte ident2;
	byte crc;
	int version;
	// unused
	int genericFlags;
	// unused
	int genericFlags2;
	unsigned short changeCounter;
	unsigned short otaCounter;
	// target wifi credentials
	char wifi_ssid[64];
	char wifi_pass[64];
	// MQTT information for Home Assistant
	char mqtt_host[256];
	char mqtt_brokerName[64];
	char mqtt_userName[64];
	char mqtt_pass[128];
	int mqtt_port;
	// addon JavaScript panel is hosted on external server
	char webappRoot[64];
	// TODO?
	byte mac[6];
	// TODO?
	char shortDeviceName[32];
	char longDeviceName[64];
	pinsState_t pins;
	byte unusedSectorA[256];
	byte unusedSectorB[128];
	byte unusedSectorC[128];
	char initCommandLine[512];
} mainConfig_v1_t;

static byte CFG_CalcChecksum_V1(mainConfig_v1_t *inf) {
	byte crc = 0;
	crc ^= Tiny_CRC8((const char*)&inf->version,sizeof(inf->version));
	crc ^= Tiny_CRC8((const char*)&inf->changeCounter,sizeof(inf->changeCounter));
	crc ^= Tiny_CRC8((const char*)&inf->otaCounter,sizeof(inf->otaCounter));
	crc ^= Tiny_CRC8((const char*)&inf->genericFlags,sizeof(inf->genericFlags));
	crc ^= Tiny_CRC8((const char*)&inf->genericFlags2,sizeof(inf->genericFlags2));
	crc ^= Tiny_CRC8((const char*)&inf->wifi_ssid,sizeof(inf->wifi_ssid));
	crc ^= Tiny_CRC8((const char*)&inf->wifi_pass,sizeof(inf->wifi_pass));
	crc ^= Tiny_CRC8((const char*)&inf->mqtt_host,sizeof(inf->mqtt_host));
	crc ^= Tiny_CRC8((const char*)&inf->mqtt_brokerName,sizeof(inf->mqtt_brokerName));
	crc ^= Tiny_CRC8((const char*)&inf->mqtt_userName,sizeof(inf->mqtt_userName));
	crc ^= Tiny_CRC8((const char*)&inf->mqtt_pass,sizeof(inf->mqtt_pass));
	crc ^= Tiny_CRC8((const char*)&inf->mqtt_port,sizeof(inf->mqtt_port));
	crc ^= Tiny_CRC8((const char*)&inf->webappRoot,sizeof(inf->webappRoot));
	crc ^= Tiny_CRC8((const char*)&inf->mac,sizeof(inf->mac));
	crc ^= Tiny_CRC8((const char*)&inf->shortDeviceName,sizeof(inf->shortDeviceName));
	crc ^= Tiny_CRC8((const char*)&inf->longDeviceName,sizeof(inf->longDeviceName));
	crc ^= Tiny_CRC8((const char*)&inf->pins,sizeof(inf->pins));
	crc ^= Tiny_CRC8((const char*)&inf->unusedSectorA,sizeof(inf->unusedSectorA));
	crc ^= Tiny_CRC8((const char*)&inf->unusedSectorB,sizeof(inf->unusedSectorB));
	crc ^= Tiny_CRC8((const char*)&inf->unusedSectorC,sizeof(inf->unusedSectorC));
	crc ^= Tiny_CRC8((const char*)&inf->initCommandLine,sizeof(inf->initCommandLine));

	return crc;
}
static byte CFG_CalcChecksum(mainConfig_t *inf) {
	int header_size;
	int remaining_size;
	byte crc;

	if(inf->version <= 1) {
		return CFG_CalcChecksum_V1((mainConfig_v1_t *)inf);
	}
	header_size = ((byte*)&inf->version)-((byte*)inf);
	remaining_size = sizeof(mainConfig_t) - header_size;

	ADDLOG_DEBUG(LOG_FEATURE_CFG, "CFG_CalcChecksum: header size %i, total size %i, rem size %i\n",
		header_size, sizeof(mainConfig_t), remaining_size);

	// This is more flexible method and won't be affected by field offsets
	crc = Tiny_CRC8((const char*)&inf->version,remaining_size);

	return crc;
}
static void CFG_SetDefaultConfig() {
	// must be unsigned, else print below prints negatives as e.g. FFFFFFFe
	unsigned char mac[6] = { 0 };

#if PLATFORM_XR809
	HAL_Configuration_GenerateMACForThisModule(mac);
#else
	WiFI_GetMacAddress((char *)mac);
#endif

	g_configInitialized = 1;

	memset(&g_cfg,0,sizeof(mainConfig_t));
	g_cfg.version = MAIN_CFG_VERSION;
	g_cfg.mqtt_port = 1883;
	g_cfg.ident0 = CFG_IDENT_0;
	g_cfg.ident1 = CFG_IDENT_1;
	g_cfg.ident2 = CFG_IDENT_2;
	g_cfg.timeRequiredToMarkBootSuccessfull = DEFAULT_BOOT_SUCCESS_TIME;
	strcpy(g_cfg.ping_host,"192.168.0.1");
	strcpy(g_cfg.mqtt_host, "192.168.0.113");
	strcpy(g_cfg.mqtt_brokerName, "test");
	strcpy(g_cfg.mqtt_userName, "homeassistant");
	strcpy(g_cfg.mqtt_pass, "qqqqqqqqqq");
	// already zeroed but just to remember, open AP by default
	g_cfg.wifi_ssid[0] = 0;
	g_cfg.wifi_pass[0] = 0;
	// i am not sure about this, because some platforms might have no way to store mac outside our cfg?
	memcpy(g_cfg.mac,mac,6);
	strcpy(g_cfg.webappRoot, "https://openbekeniot.github.io/webapp/");
	// Long unique device name, like OpenBK7231T_AABBCCDD
	sprintf(g_cfg.longDeviceName,DEVICENAME_PREFIX_FULL"_%02X%02X%02X%02X",mac[2],mac[3],mac[4],mac[5]);
	sprintf(g_cfg.shortDeviceName,DEVICENAME_PREFIX_SHORT"%02X%02X%02X%02X",mac[2],mac[3],mac[4],mac[5]);

}

const char *CFG_GetWebappRoot(){
	return g_cfg.webappRoot;
}
const char *CFG_GetShortStartupCommand() {
	return g_cfg.initCommandLine;
}

void CFG_SetBootOkSeconds(int v) {
	// at least 1 second, always
	if(v < 1)
		v = 1;
	if(g_cfg.timeRequiredToMarkBootSuccessfull != v) {
		g_cfg.timeRequiredToMarkBootSuccessfull = v;
		g_cfg_pendingChanges++;
	}
}
int CFG_GetBootOkSeconds() {
	if(g_configInitialized==0){
		return DEFAULT_BOOT_SUCCESS_TIME;
	}
	if(g_cfg.timeRequiredToMarkBootSuccessfull <= 0) {
		return DEFAULT_BOOT_SUCCESS_TIME;
	}
	return g_cfg.timeRequiredToMarkBootSuccessfull;
}
const char *CFG_GetPingHost() {
	return g_cfg.ping_host;
}
int CFG_GetPingDisconnectedSecondsToRestart() {
	return g_cfg.ping_seconds;
}
int CFG_GetPingIntervalSeconds() {
	return g_cfg.ping_interval;
}
void CFG_SetPingHost(const char *s) {
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.ping_host, s,sizeof(g_cfg.ping_host))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetPingDisconnectedSecondsToRestart(int i) {
	if(g_cfg.ping_seconds != i) {
		g_cfg.ping_seconds = i;
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetPingIntervalSeconds(int i) {
	if(g_cfg.ping_interval != i) {
		g_cfg.ping_interval = i;
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetShortStartupCommand_AndExecuteNow(const char *s) {
	CFG_SetShortStartupCommand(s);
	CMD_ExecuteCommand(s,COMMAND_FLAG_SOURCE_SCRIPT);
}
void CFG_SetShortStartupCommand(const char *s) {
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.initCommandLine, s,sizeof(g_cfg.initCommandLine))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
int CFG_SetWebappRoot(const char *s) {
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.webappRoot, s,sizeof(g_cfg.webappRoot))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
	return 1;
}

const char *CFG_GetDeviceName(){
	if(g_configInitialized==0)
		return "";
	return g_cfg.longDeviceName;
}
const char *CFG_GetShortDeviceName(){
	if(g_configInitialized==0)
		return "";
	return g_cfg.shortDeviceName;
}

int CFG_GetMQTTPort() {
	return g_cfg.mqtt_port;
}
void CFG_SetShortDeviceName(const char *s) {
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.shortDeviceName, s,sizeof(g_cfg.shortDeviceName))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetDeviceName(const char *s) {
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.longDeviceName, s,sizeof(g_cfg.longDeviceName))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetMQTTPort(int p) {
	// is there a change?
	if(g_cfg.mqtt_port != p) {
		g_cfg.mqtt_port = p;
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetOpenAccessPoint() {
	// is there a change?
	if(g_cfg.wifi_ssid[0] == 0 && g_cfg.wifi_pass[0] == 0) {
		return;
	}
	g_cfg.wifi_ssid[0] = 0;
	g_cfg.wifi_pass[0] = 0;
	// mark as dirty (value has changed)
	g_cfg_pendingChanges++;
}
const char *CFG_GetWiFiSSID(){
	return g_cfg.wifi_ssid;
}
const char *CFG_GetWiFiPass(){
	return g_cfg.wifi_pass;
}
void CFG_SetWiFiSSID(const char *s) {
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.wifi_ssid, s,sizeof(g_cfg.wifi_ssid))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetWiFiPass(const char *s) {
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.wifi_pass, s,sizeof(g_cfg.wifi_pass))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
const char *CFG_GetMQTTHost() {
	return g_cfg.mqtt_host;
}
const char *CFG_GetMQTTBrokerName() {
	return g_cfg.mqtt_brokerName;
}
const char *CFG_GetMQTTUserName() {
	return g_cfg.mqtt_userName;
}
const char *CFG_GetMQTTPass() {
	return g_cfg.mqtt_pass;
}
void CFG_SetMQTTHost(const char *s) {
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.mqtt_host, s,sizeof(g_cfg.mqtt_host))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetMQTTBrokerName(const char *s) {
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.mqtt_brokerName, s,sizeof(g_cfg.mqtt_brokerName))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetMQTTUserName(const char *s) {
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.mqtt_userName, s,sizeof(g_cfg.mqtt_userName))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetMQTTPass(const char *s) {
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.mqtt_pass, s,sizeof(g_cfg.mqtt_pass))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_ClearPins() {
	memset(&g_cfg.pins,0,sizeof(g_cfg.pins));
	g_cfg_pendingChanges++;
}
void CFG_IncrementOTACount() {
	g_cfg.otaCounter++;
	g_cfg_pendingChanges++;
}
void CFG_Save_IfThereArePendingChanges() {
	if(g_cfg_pendingChanges > 0) {
		g_cfg.version = MAIN_CFG_VERSION;
		g_cfg.changeCounter++;
		g_cfg.crc = CFG_CalcChecksum(&g_cfg);
		HAL_Configuration_SaveConfigMemory(&g_cfg,sizeof(g_cfg));
		g_cfg_pendingChanges = 0;
	}
}
void CFG_DeviceGroups_SetName(const char *s) {
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.dgr_name, s,sizeof(g_cfg.dgr_name))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_DeviceGroups_SetSendFlags(int newSendFlags) {
	if(g_cfg.dgr_sendFlags != newSendFlags) {
		g_cfg.dgr_sendFlags = newSendFlags;
		g_cfg_pendingChanges++;
	}
}
void CFG_DeviceGroups_SetRecvFlags(int newRecvFlags) {
	if(g_cfg.dgr_recvFlags != newRecvFlags) {
		g_cfg.dgr_recvFlags = newRecvFlags;
		g_cfg_pendingChanges++;
	}
}
const char *CFG_DeviceGroups_GetName() {
	return g_cfg.dgr_name;
}
int CFG_DeviceGroups_GetSendFlags() {
	return g_cfg.dgr_sendFlags;
}
int CFG_DeviceGroups_GetRecvFlags() {
	return g_cfg.dgr_recvFlags;
}
void CFG_SetFlag(int flag, bool bValue) {
	int nf = g_cfg.genericFlags;
	if(bValue) {
		BIT_SET(nf,flag);
	} else {
		BIT_CLEAR(nf,flag);
	}
	if(nf != g_cfg.genericFlags) {
		g_cfg.genericFlags = nf;
		g_cfg_pendingChanges++;
		// this will start only if it wasnt running
		if(bValue && flag == OBK_FLAG_CMD_ENABLETCPRAWPUTTYSERVER) {
			CMD_StartTCPCommandLine();
		}
	}
}
int CFG_GetFlags() {
	return g_cfg.genericFlags;
}
bool CFG_HasFlag(int flag) {
	return BIT_CHECK(g_cfg.genericFlags,flag);
}

void CFG_SetChannelStartupValue(int channelIndex,short newValue) {
	if(channelIndex < 0 || channelIndex >= CHANNEL_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "CFG_SetChannelStartupValue: Channel index %i out of range <0,%i).",channelIndex,CHANNEL_MAX);
		return;
	}
	if(g_cfg.startChannelValues[channelIndex] != newValue) {
		g_cfg.startChannelValues[channelIndex] = newValue;
		g_cfg_pendingChanges++;
	}
}
short CFG_GetChannelStartupValue(int channelIndex) {
	if(channelIndex < 0 || channelIndex >= CHANNEL_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "CFG_SetChannelStartupValue: Channel index %i out of range <0,%i).",channelIndex,CHANNEL_MAX);
		return 0;
	}
	return g_cfg.startChannelValues[channelIndex];
}
void PIN_SetPinChannelForPinIndex(int index, int ch) {
	if(index < 0 || index >= PLATFORM_GPIO_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "PIN_SetPinChannelForPinIndex: Pin index %i out of range <0,%i).",index,PLATFORM_GPIO_MAX);
		return;
	}
	if(ch < 0 || ch >= CHANNEL_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "PIN_SetPinChannelForPinIndex: Channel index %i out of range <0,%i).",ch,CHANNEL_MAX);
		return;
	}
	if(g_cfg.pins.channels[index] != ch) {
		g_cfg_pendingChanges++;
		g_cfg.pins.channels[index] = ch;
	}
}
void PIN_SetPinChannel2ForPinIndex(int index, int ch) {
	if(index < 0 || index >= PLATFORM_GPIO_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "PIN_SetPinChannel2ForPinIndex: Pin index %i out of range <0,%i).",index,PLATFORM_GPIO_MAX);
		return;
	}
	if(g_cfg.pins.channels2[index] != ch) {
		g_cfg_pendingChanges++;
		g_cfg.pins.channels2[index] = ch;
	}
}
//void CFG_ApplyStartChannelValues() {
//	int i;
//
//	// apply channel values
//	for(i = 0; i < CHANNEL_MAX; i++) {
//		if(CHANNEL_IsInUse(i)) {
//			CHANNEL_Set(i,0,CHANNEL_SET_FLAG_FORCE | CHANNEL_SET_FLAG_SKIP_MQTT);
//		}
//	}
//}
void CFG_InitAndLoad() {
	byte chkSum;

	HAL_Configuration_ReadConfigMemory(&g_cfg,sizeof(g_cfg));
	chkSum = CFG_CalcChecksum(&g_cfg);
	if(g_cfg.ident0 != CFG_IDENT_0 || g_cfg.ident1 != CFG_IDENT_1 || g_cfg.ident2 != CFG_IDENT_2
		|| chkSum != g_cfg.crc) {
			addLogAdv(LOG_WARN, LOG_FEATURE_CFG, "CFG_InitAndLoad: Config crc or ident mismatch. Default config will be loaded.");
		CFG_SetDefaultConfig();
		// mark as changed
		g_cfg_pendingChanges ++;
	} else {
		addLogAdv(LOG_WARN, LOG_FEATURE_CFG, "CFG_InitAndLoad: Correct config has been loaded with %i changes count.",g_cfg.changeCounter);
	}
	g_configInitialized = 1;

}