
#include "new_common.h"
#include "logging/logging.h"
#include "httpserver/new_http.h"
#include "new_pins.h"
#include "new_cfg.h"
#include "mqtt/new_mqtt.h"
#include "hal/hal_wifi.h"
#include "hal/hal_flashConfig.h"
#include "cmnds/cmd_public.h"
#if ENABLE_LITTLEFS
#include "littlefs/our_lfs.h"
#endif


#define DEFAULT_BOOT_SUCCESS_TIME 5

mainConfig_t g_cfg;
int g_configInitialized = 0;
int g_cfg_pendingChanges = 0;

#define CFG_IDENT_0 'C'
#define CFG_IDENT_1 'F'
#define CFG_IDENT_2 'G'

#define MAIN_CFG_VERSION_V3 3
// version 4 - bumped size by 1024,
// added alternate ssid fields
#if PLATFORM_W600
#define MAIN_CFG_VERSION 3
#else
#define MAIN_CFG_VERSION 5
#endif

static byte CFG_CalcChecksum(mainConfig_t *inf) {
	int header_size;
	int remaining_size;
	byte crc;
	int configSize;

	header_size = ((byte*)&inf->version)-((byte*)inf);

	if (inf->version == MAIN_CFG_VERSION_V3) {
		configSize = MAGIC_CONFIG_SIZE_V3;
#if ALLOW_SSID2
		// quick fix for converting
		inf->wifi_pass2[0] = 0;
		inf->wifi_ssid2[0] = 0;
#endif
	}
	else
	{
		configSize = sizeof(mainConfig_t);
	}
	remaining_size = configSize - header_size;

	ADDLOG_DEBUG(LOG_FEATURE_CFG, "CFG_CalcChecksum: header size %i, total size %i, rem size %i\n",
		header_size, configSize, remaining_size);

	// This is more flexible method and won't be affected by field offsets
	crc = Tiny_CRC8((const char*)&inf->version,remaining_size);

	return crc;
}

bool isZeroes(const byte *p, int size) {
	int i;

	for (i = 0; i < size; i++) {
		if (p[i])
			return false;
	}
	return true;
}

bool CFG_HasValidLEDCorrectionTable() {
	if (isZeroes((const byte*)&g_cfg.led_corr, sizeof(g_cfg.led_corr))) {
		return false;
	}
	else {
		return true;
	}
}
void CFG_SetDefaultLEDCorrectionTable() {
	addLogAdv(LOG_INFO, LOG_FEATURE_CFG, "CFG_SetDefaultLEDCorrectionTable: setting defaults\r\n");
	for (int c = 0; c < 3; c++) {
		g_cfg.led_corr.rgb_cal[c] = 1.0f;
	}
	g_cfg.led_corr.led_gamma = 2.2f;
	g_cfg.led_corr.rgb_bright_min = 0.1f;
	g_cfg.led_corr.cw_bright_min = 0.1f;
	g_cfg_pendingChanges++;
}
void CFG_MarkAsDirty() {
	g_cfg_pendingChanges++;
}
void CFG_ClearIO() {
	memset(&g_cfg.pins, 0, sizeof(g_cfg.pins));
	g_cfg_pendingChanges++;
}
void CFG_SetDefaultConfig() {
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
	//strcpy(g_cfg.mqtt_host, "192.168.0.113");		//Let default mqtt_host be empty
	// g_cfg.mqtt_clientId is set as shortDeviceName below
	strcpy(g_cfg.mqtt_userName, "homeassistant");
	strcpy(g_cfg.mqtt_pass, "qqqqqqqqqq");
	// already zeroed but just to remember, open AP by default
	g_cfg.wifi_ssid[0] = 0;
	g_cfg.wifi_pass[0] = 0;
	// i am not sure about this, because some platforms might have no way to store mac outside our cfg?
	memcpy(g_cfg.mac,mac,6);
	strcpy(g_cfg.webappRoot, "https://openbekeniot.github.io/webapp/");
	// Long unique device name, like OpenBK7231T_AABBCCDD
	snprintf(g_cfg.longDeviceName, sizeof(g_cfg.longDeviceName), DEVICENAME_PREFIX_FULL"_%02X%02X%02X%02X",mac[2],mac[3],mac[4],mac[5]);
	snprintf(g_cfg.shortDeviceName, sizeof(g_cfg.shortDeviceName), DEVICENAME_PREFIX_SHORT"%02X%02X%02X%02X",mac[2],mac[3],mac[4],mac[5]);
	strcpy_safe(g_cfg.mqtt_clientId, g_cfg.shortDeviceName, sizeof(g_cfg.mqtt_clientId));

	// group topic will be unique for each platform, so it's easy
	// to do group OTA without worrying about feeding wrong RBL for wrong platform
#if PLATFORM_BK7231T
	strcpy_safe(g_cfg.mqtt_group, "bekens_t", sizeof(g_cfg.mqtt_group));
#elif PLATFORM_BK7231N
	strcpy_safe(g_cfg.mqtt_group, "bekens_n", sizeof(g_cfg.mqtt_group));
#elif PLATFORM_W600
	strcpy_safe(g_cfg.mqtt_group, "w600s", sizeof(g_cfg.mqtt_group));
#elif PLATFORM_W800
	strcpy_safe(g_cfg.mqtt_group, "w800s", sizeof(g_cfg.mqtt_group));
#elif PLATFORM_XR809
	strcpy_safe(g_cfg.mqtt_group, "xr809s", sizeof(g_cfg.mqtt_group));
#elif PLATFORM_BL602
	strcpy_safe(g_cfg.mqtt_group, "bl602s", sizeof(g_cfg.mqtt_group));
#elif PLATFORM_ESPIDF
	strcpy_safe(g_cfg.mqtt_group, "esp", sizeof(g_cfg.mqtt_group));
#elif WINDOWS
	strcpy_safe(g_cfg.mqtt_group, "bekens", sizeof(g_cfg.mqtt_group));
#else
	strcpy_safe(g_cfg.mqtt_group, "obks", sizeof(g_cfg.mqtt_group));
#endif

	strcpy(g_cfg.ntpServer, DEFAULT_NTP_SERVER);

	
	// default value is 5, which means 500ms
	g_cfg.buttonHoldRepeat = CFG_DEFAULT_BTN_REPEAT;
	// default value is 3, which means 300ms
	g_cfg.buttonShortPress = CFG_DEFAULT_BTN_SHORT;
	// default value is 10, which means 1000ms
	g_cfg.buttonLongPress = CFG_DEFAULT_BTN_LONG;

	// This is helpful for users
	CFG_SetFlag(OBK_FLAG_MQTT_BROADCASTSELFSTATEONCONNECT,true);
	// this is helpful for debuging wifi issues
#if PLATFORM_BEKEN
	//CFG_SetFlag(OBK_FLAG_CMD_ACCEPT_UART_COMMANDS, true);
#endif
	
	CFG_SetDefaultLEDCorrectionTable();

	g_cfg_pendingChanges++;
}

void CFG_SetLEDRemap(int r, int g, int b, int c, int w) {
	if (g_cfg.ledRemap.r != r || g_cfg.ledRemap.g != g || g_cfg.ledRemap.b != b || g_cfg.ledRemap.c != c || g_cfg.ledRemap.w != w) {
		g_cfg.ledRemap.r = r;
		g_cfg.ledRemap.g = g;
		g_cfg.ledRemap.b = b;
		g_cfg.ledRemap.c = c;
		g_cfg.ledRemap.w = w;
		CFG_MarkAsDirty();
	}
}
void CFG_SetDefaultLEDRemap(int r, int g, int b, int c, int w) {
	if (g_cfg.ledRemap.r == 0 && g_cfg.ledRemap.g == 0 && g_cfg.ledRemap.b == 0 && g_cfg.ledRemap.c == 0 && g_cfg.ledRemap.w == 0) {
		CFG_SetLEDRemap(r, g, b, c, w);
	}
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
// called from SDK 
const char *CFG_GetOpenBekenHostName() {
	if (CFG_HasFlag(OBK_FLAG_USE_SHORT_DEVICE_NAME_AS_HOSTNAME)) {
		return CFG_GetShortDeviceName();
	}
	return CFG_GetDeviceName();
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
	static char wifi_pass[sizeof(g_cfg.wifi_pass) + 1];

	memcpy(wifi_pass, g_cfg.wifi_pass, sizeof(g_cfg.wifi_pass));
	wifi_pass[sizeof(g_cfg.wifi_pass)] = 0;
	return wifi_pass;
}
const char *CFG_GetWiFiSSID2() {
#if ALLOW_SSID2
	return g_cfg.wifi_ssid2;
#else
	return "";
#endif
}
const char *CFG_GetWiFiPass2() {
#if ALLOW_SSID2
	return g_cfg.wifi_pass2;
#else
	return "";
#endif
}
int CFG_SetWiFiSSID(const char *s) {
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.wifi_ssid, s,sizeof(g_cfg.wifi_ssid))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
		return 1;
	}
	return 0;
}
int CFG_SetWiFiPass(const char *s) {
	uint32_t len;

	len = strlen(s) + 1;
	if(len > sizeof(g_cfg.wifi_pass))
		len = sizeof(g_cfg.wifi_pass);
	if(memcmp(g_cfg.wifi_pass, s, len)) {
		memcpy(g_cfg.wifi_pass, s, len);
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
		return 1;
	}
	return 0;
}
int CFG_SetWiFiSSID2(const char *s) {
#if ALLOW_SSID2
	// this will return non-zero if there were any changes
	if (strcpy_safe_checkForChanges(g_cfg.wifi_ssid2, s, sizeof(g_cfg.wifi_ssid2))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
		return 1;
	}
#endif
	return 0;
}
int CFG_SetWiFiPass2(const char *s) {
#if ALLOW_SSID2
	if (strcpy_safe_checkForChanges(g_cfg.wifi_pass2, s, sizeof(g_cfg.wifi_pass2))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
		return 1;
	}
#endif
	return 0;
}
const char *CFG_GetMQTTHost() {
	return g_cfg.mqtt_host;
}
const char *CFG_GetMQTTClientId() {
	return g_cfg.mqtt_clientId;
}
const char *CFG_GetMQTTGroupTopic() {
	return g_cfg.mqtt_group;
}
const char *CFG_GetMQTTUserName() {
	return g_cfg.mqtt_userName;
}
const char *CFG_GetMQTTPass() {
	return g_cfg.mqtt_pass;
}

void CHANNEL_SetType(int ch, int type) {
	if (g_cfg.pins.channelTypes[ch] != type) {
		g_cfg.pins.channelTypes[ch] = type;
		g_cfg_pendingChanges++;
	}
}
int CHANNEL_GetType(int ch) {
	return g_cfg.pins.channelTypes[ch];
}
void CFG_SetMQTTHost(const char *s) {
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.mqtt_host, s,sizeof(g_cfg.mqtt_host))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetMQTTClientId(const char *s) {
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.mqtt_clientId, s,sizeof(g_cfg.mqtt_clientId))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
		g_mqtt_bBaseTopicDirty++;
	}
}
void CFG_SetMQTTGroupTopic(const char *s) {
	// this will return non-zero if there were any changes
	if (strcpy_safe_checkForChanges(g_cfg.mqtt_group, s, sizeof(g_cfg.mqtt_group))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
		g_mqtt_bBaseTopicDirty++;
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
void CFG_SetMac(char *mac) {
	if(memcmp(mac,g_cfg.mac,6)) {
		memcpy(g_cfg.mac,mac,6);
		g_cfg_pendingChanges++;
	}
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
void CFG_SetFlags(int first4bytes, int second4bytes) {
	if (g_cfg.genericFlags != first4bytes || g_cfg.genericFlags2 != second4bytes) {
		g_cfg.genericFlags = first4bytes;
		g_cfg.genericFlags2 = second4bytes;
		g_cfg_pendingChanges++;
	}
}
void CFG_SetFlag(int flag, bool bValue) {
	int *cfgValue;
	if (flag >= 32) {
		cfgValue = &g_cfg.genericFlags2;
		flag -= 32;
	}
	else {
		cfgValue = &g_cfg.genericFlags;
	}

	int nf = *cfgValue;
	if(bValue) {
		BIT_SET(nf,flag);
	} else {
		BIT_CLEAR(nf,flag);
	}
	if(nf != *cfgValue) {
		*cfgValue = nf;
		g_cfg_pendingChanges++;
		// this will start only if it wasnt running
		if(bValue && flag == OBK_FLAG_CMD_ENABLETCPRAWPUTTYSERVER) {
			CMD_StartTCPCommandLine();
		}
		if (bValue && flag == OBK_FLAG_LED_REMEMBERLASTSTATE) {
			LED_SaveStateToFlashVarsNow();
		}
	}
}
void CFG_SetLoggerFlag(int flag, bool bValue) {
	int *cfgValue;
	cfgValue = &g_cfg.loggerFlags;

	int nf = *cfgValue;
	if (bValue) {
		BIT_SET(nf, flag);
	}
	else {
		BIT_CLEAR(nf, flag);
	}
	if (nf != *cfgValue) {
		*cfgValue = nf;
		g_cfg_pendingChanges++;
	}
}
bool CFG_HasLoggerFlag(int flag) {
	return BIT_CHECK(g_cfg.loggerFlags, flag);
}
int CFG_GetFlags() {
	return g_cfg.genericFlags;
}
unsigned long CFG_GetFlags64() {
	unsigned long* pAllGenericFlags = (unsigned long*)&g_cfg.genericFlags;
	return *pAllGenericFlags;
}
bool CFG_HasFlag(int flag) {
	if (flag >= 32) {
		flag -= 32;
		return BIT_CHECK(g_cfg.genericFlags2, flag);
	}
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

const char *CFG_GetNTPServer() {
	return g_cfg.ntpServer;
}
void CFG_SetNTPServer(const char *s) {	
	if(strcpy_safe_checkForChanges(g_cfg.ntpServer, s,sizeof(g_cfg.ntpServer))) {
		g_cfg_pendingChanges++;
	}
}
int CFG_GetPowerMeasurementCalibrationInteger(int index, int def) {
	if(g_cfg.cal.values[index].i == 0) {
		return def;
	}
	return g_cfg.cal.values[index].i;
}
void CFG_SetPowerMeasurementCalibrationInteger(int index, int value) {
	if(g_cfg.cal.values[index].i != value) {
		g_cfg.cal.values[index].i = value;
		g_cfg_pendingChanges++;
	}
}
float CFG_GetPowerMeasurementCalibrationFloat(int index, float def) {
	if(g_cfg.cal.values[index].i == 0) {
		return def;
	}
	return g_cfg.cal.values[index].f;
}
void CFG_SetPowerMeasurementCalibrationFloat(int index, float value) {
	if(g_cfg.cal.values[index].f != value) {
		g_cfg.cal.values[index].f = value;
		g_cfg_pendingChanges++;
	}
}
void CFG_SetButtonLongPressTime(int value) {
	if(g_cfg.buttonLongPress != value) {
		g_cfg.buttonLongPress = value;
		g_cfg_pendingChanges++;
	}
}
void CFG_SetButtonShortPressTime(int value) {
	if(g_cfg.buttonShortPress != value) {
		g_cfg.buttonShortPress = value;
		g_cfg_pendingChanges++;
	}
}
void CFG_SetButtonRepeatPressTime(int value) {
	if(g_cfg.buttonHoldRepeat != value) {
		g_cfg.buttonHoldRepeat = value;
		g_cfg_pendingChanges++;
	}
}
const char *CFG_GetWebPassword() {
#if ALLOW_WEB_PASSWORD
	return g_cfg.webPassword;
#else
	return "";
#endif
}
void CFG_SetWebPassword(const char *s) {
#if ALLOW_WEB_PASSWORD
	// this will return non-zero if there were any changes
	if(strcpy_safe_checkForChanges(g_cfg.webPassword, s,sizeof(g_cfg.webPassword))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
#endif
}


#if ENABLE_LITTLEFS
void CFG_SetLFS_Size(uint32_t value) {
	if(g_cfg.LFS_Size != value) {
		g_cfg.LFS_Size = value;
		g_cfg_pendingChanges++;
	}
}

uint32_t CFG_GetLFS_Size() {
	uint32_t size = g_cfg.LFS_Size;
	if (size == 0){
		size = LFS_BLOCKS_DEFAULT_LEN;
	}
	return size;
}
#endif

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
#if defined(PLATFORM_XR809) || defined(PLATFORM_BL602) || defined(PLATFORM_ESPIDF)
		if (g_cfg.mac[0] == 0 && g_cfg.mac[1] == 0 && g_cfg.mac[2] == 0 && g_cfg.mac[3] == 0 && g_cfg.mac[4] == 0 && g_cfg.mac[5] == 0) {
			WiFI_GetMacAddress((char*)g_cfg.mac);
		}
		WiFI_SetMacAddress((char*)g_cfg.mac);
#endif
		addLogAdv(LOG_WARN, LOG_FEATURE_CFG, "CFG_InitAndLoad: Correct config has been loaded with %i changes count.",g_cfg.changeCounter);
	}

	// copy shortDeviceName to MQTT Client ID, set version=3
	if (g_cfg.version<3) {
		addLogAdv(LOG_WARN, LOG_FEATURE_CFG, "CFG_InitAndLoad: Old config version found, updating to v3.");
		strcpy_safe(g_cfg.mqtt_clientId, g_cfg.shortDeviceName, sizeof(g_cfg.mqtt_clientId));
		g_cfg_pendingChanges++;
	}
#if ALLOW_WEB_PASSWORD
	// add web admin password configuration
	if (g_cfg.version<5) {
		strcpy_safe(g_cfg.webPassword, "", sizeof(g_cfg.webPassword));
	}
#endif
	g_cfg.version = MAIN_CFG_VERSION;

	if(g_cfg.buttonHoldRepeat == 0) {
		// default value is 5, which means 500ms
		g_cfg.buttonHoldRepeat = CFG_DEFAULT_BTN_REPEAT;
	}
	if(g_cfg.buttonShortPress == 0) {
		// default value is 3, which means 300ms
		g_cfg.buttonShortPress = CFG_DEFAULT_BTN_SHORT;
	}
	if(g_cfg.buttonLongPress == 0) {
		// default value is 3, which means 100ms
		g_cfg.buttonLongPress = CFG_DEFAULT_BTN_LONG;
	}
	// convert to new version - add missing table
	if (CFG_HasValidLEDCorrectionTable() == false) {
		CFG_SetDefaultLEDCorrectionTable();
	}
	g_configInitialized = 1;
	CFG_Save_IfThereArePendingChanges();
}
