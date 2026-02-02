#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../obk_config.h"
#include <ctype.h>
#include "cmd_local.h"
#include "../driver/drv_ir.h"
#include "../driver/drv_uart.h"
#if ENABLE_DRIVER_BL0942
#include "../driver/drv_bl0942.h"
#endif
#include "../driver/drv_public.h"
#include "../hal/hal_adc.h"
#include "../hal/hal_flashVars.h"
#include "../httpserver/http_tcp_server.h"
#include "../hal/hal_generic.h"

int cmd_uartInitIndex = 0;


#if ENABLE_LITTLEFS
#include "../littlefs/our_lfs.h"
#endif
#ifdef PLATFORM_BL602
#include <wifi_mgmr_ext.h>
#include "bl_flash.h"
#include "bl602_hbn.h"
#elif PLATFORM_LN882H
#include <wifi.h>
#include <power_mgmt/ln_pm.h>
#elif PLATFORM_LN8825
#include <wifi.h>
#include <hal/hal_sleep.h>
#define wifi_sta_set_powersave wifi_station_set_powersave
#define ln_pm_sleep_mode_set hal_sleep_set_mode
#define sysparam_sta_powersave_update(x)
#elif PLATFORM_ESPIDF || PLATFORM_ESP8266
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#if !PLATFORM_ESP8266
#include "esp_pm.h"
#include "driver/rtc_io.h"
#include "esp_check.h"
#endif
#elif PLATFORM_REALTEK 
#if PLATFORM_RTL8710B
#include "wlan_intf.h"
extern WLAN_LOW_PW_MODE rtw_wlan_low_pw_mode;
extern int rtw_wlan_low_pw_mode4_c1;
extern int rtw_wlan_low_pw_mode4_c2;
extern int rtw_reduce_pa_gain;
extern void rtw_enable_wlan_low_pwr_mode(WLAN_LOW_PW_MODE mode);
#elif PLATFORM_RTL8720D
extern void SystemSetCpuClk(unsigned char CpuClk);
#endif
#if !PLATFORM_REALTEK_NEW
#include "wifi_conf.h"
#endif
int g_sleepfactor = 1;
#elif PLATFORM_BEKEN_NEW
#include "co_math.h"
#include "manual_ps_pub.h"
#include "wlan_ui_pub.h"
#elif PLATFORM_XRADIO
#include "common/framework/net_ctrl.h"
#include "driver/chip/hal_wakeup.h"
#include "net/wlan/wlan_defs.h"
#include "net/wlan/wlan_ext_req.h"
#include "pm/pm.h"
#if PLATFORM_XR809
#define DEEP_SLEEP PM_MODE_POWEROFF
#else
#define DEEP_SLEEP PM_MODE_HIBERNATION
#endif
#elif PLATFORM_ECR6600
#include "psm_system.h"
#endif

#define HASH_SIZE 128

static int generateHashValue(const char* fname) {
	int		i;
	int		hash;
	int		letter;
	unsigned char* f = (unsigned char*)fname;

	hash = 0;
	i = 0;
	while (f[i]) {
		letter = tolower(f[i]);
		hash += (letter) * (i + 119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (HASH_SIZE - 1);
	return hash;
}

command_t* g_commands[HASH_SIZE] = { NULL };
bool g_powersave;

#if defined(PLATFORM_LN882H) || PLATFORM_LN8825
// this will be applied after WiFi connect
int g_ln882h_pendingPowerSaveCommand = -1;

void LN882H_ApplyPowerSave(int bOn) {
	if (bOn) {
		sysparam_sta_powersave_update(WIFI_MAX_POWERSAVE);
		wifi_sta_set_powersave(WIFI_MAX_POWERSAVE);
		if (bOn > 1) {
			ln_pm_sleep_mode_set(LIGHT_SLEEP);
		}
		else {	// to be able to switch from PowerSave from > 1 to 1 (without sleep) 
			ln_pm_sleep_mode_set(ACTIVE);
		}
	}
	else {
		sysparam_sta_powersave_update(WIFI_NO_POWERSAVE);
		wifi_sta_set_powersave(WIFI_NO_POWERSAVE);
		ln_pm_sleep_mode_set(ACTIVE);
	}
}
#endif

static commandResult_t CMD_PowerSave(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int bOn = 1;
	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() > 0) {
		bOn = Tokenizer_GetArgInteger(0);
	}
#if PLATFORM_LN882H || PLATFORM_LN8825
	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_PowerSave: will set to %i%s", bOn, Main_IsConnectedToWiFi() == 0 ? " after WiFi is connected" : "");
#else
	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_PowerSave: will set to %i", bOn);
#endif
	

#if defined(PLATFORM_BEKEN)
	extern int bk_wlan_power_save_set_level(BK_PS_LEVEL level);
	if (bOn) {
		BK_PS_LEVEL level = PS_RF_SLEEP_BIT;
		if(PIN_FindPinIndexForRole(IOR_BL0937_CF, -1) == -1) level |= PS_MCU_SLEEP_BIT;
		else bOn = 0;
		bk_wlan_power_save_set_level(level);
	}
	else {
		bk_wlan_power_save_set_level(0);
	}
#elif defined(PLATFORM_W600) || defined(PLATFORM_W800)
	if (bOn) {
		tls_wifi_set_psflag(1, 0);	//Enable powersave but don't save to flash
	}
	else {
		tls_wifi_set_psflag(0, 0);	//Disable powersave but don't save to flash
	}
#elif defined(PLATFORM_BL602)
	if (bOn) {
		wifi_mgmr_sta_ps_enter(2);
	}
	else {
		wifi_mgmr_sta_ps_exit();
	}
#elif defined(PLATFORM_LN882H) || PLATFORM_LN8825
	// this will be applied after WiFi connect
	if (Main_IsConnectedToWiFi() == 0){
		g_ln882h_pendingPowerSaveCommand = bOn;
	}
	else LN882H_ApplyPowerSave(bOn);
#elif PLATFORM_ESPIDF || PLATFORM_ESP8266
	switch(bOn)
	{
		case 1:
			ADDLOG_INFO(LOG_FEATURE_CMD, "PowerSave min_modem");
			esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
			break;
		case 2:
			ADDLOG_INFO(LOG_FEATURE_CMD, "PowerSave max_modem");
			esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
			break;
		default:
			ADDLOG_INFO(LOG_FEATURE_CMD, "Wifi powersave disabled");
			esp_wifi_set_ps(WIFI_PS_NONE);
			break;
	}
#if PLATFORM_ESPIDF
	if(Tokenizer_GetArgsCount() > 1)
	{
		int tx = Tokenizer_GetArgInteger(1);
		int8_t maxtx = 0;
		esp_wifi_get_max_tx_power(&maxtx);
		if(tx > maxtx / 4)
		{
			ADDLOG_ERROR(LOG_FEATURE_CMD, "TX power maximum is: %ddBm, entered: %idBm", maxtx / 4, tx);
		}
		else
		{
			esp_wifi_set_max_tx_power(tx * 4);
			ADDLOG_INFO(LOG_FEATURE_CMD, "Setting TX power to: %idBm", tx);
		}
	}
	if(Tokenizer_GetArgsCount() > 3)
	{
		int minfreq = Tokenizer_GetArgInteger(2);
		int maxfreq = Tokenizer_GetArgInteger(3);
		esp_pm_config_t pm_config = {
				.max_freq_mhz = maxfreq,
				.min_freq_mhz = minfreq,
		};
		esp_pm_configure(&pm_config);
		ADDLOG_INFO(LOG_FEATURE_CMD, "PowerSave freq scaling, min: %iMhz, max: %iMhz", minfreq, maxfreq);
	}
#endif
#elif PLATFORM_REALTEK
	if(!wifi_is_up(RTW_STA_INTERFACE))
	{
		ADDLOG_ERROR(LOG_FEATURE_CMD, "Wifi is not on or in AP only mode, failed setting powersave!");
		g_powersave = (bOn);
		return CMD_RES_ERROR;
	}
	if(bOn)
	{
#if PLATFORM_RTL8710B
		if(bOn >= 2)
		{
			rtw_wlan_low_pw_mode = PW_MODE_2 | PW_MODE_3 | PW_MODE_4 | PW_MODE_6;
			rtw_wlan_low_pw_mode |= bOn >= 3 ? PW_MODE_1 : PW_MODE_5;
			rtw_wlan_low_pw_mode4_c1 = 24;
			rtw_wlan_low_pw_mode4_c2 = 16;
			rtw_reduce_pa_gain = bOn == 2 ? 1 : 2; //0:not reduce, 1:reduce 2, 2:reduce 3
			rtw_enable_wlan_low_pwr_mode(rtw_wlan_low_pw_mode);
			g_sleepfactor = bOn >= 3 ? 2 : 1;
		}
		else if(g_powersave >= 2)
		{
			rtw_wlan_low_pw_mode = PW_MODE_NONE;
			rtw_wlan_low_pw_mode4_c1 = 0;
			rtw_wlan_low_pw_mode4_c2 = 0;
			rtw_reduce_pa_gain = 0;
			rtw_enable_wlan_low_pwr_mode(rtw_wlan_low_pw_mode);
			g_sleepfactor = 1;
		}
#elif PLATFORM_RTL8720D
		if(bOn >= 2)
		{
			g_sleepfactor = 2;
			SystemSetCpuClk(1);
		}
		else if(g_powersave >= 2)
		{
			g_sleepfactor = 1;
			SystemSetCpuClk(0);
		}
#endif
		wifi_enable_powersave();
	}
	else
	{
#if PLATFORM_RTL8710B
		if(g_powersave >= 2)
		{
			rtw_wlan_low_pw_mode = PW_MODE_NONE;
			rtw_wlan_low_pw_mode4_c1 = 0;
			rtw_wlan_low_pw_mode4_c2 = 0;
			rtw_reduce_pa_gain = 0;
			rtw_enable_wlan_low_pwr_mode(rtw_wlan_low_pw_mode);
		}
		g_sleepfactor = 1;
#elif PLATFORM_RTL8720D
		SystemSetCpuClk(0);
		g_sleepfactor = 1;
#endif
		wifi_disable_powersave();
	}
#elif PLATFORM_XRADIO
	if(g_powersave)
	{
		wlan_set_ps_mode(g_wlan_netif, 1);
		wlan_ext_ps_cfg_t ps_cfg;
		memset(&ps_cfg, 0, sizeof(wlan_ext_ps_cfg_t));
		ps_cfg.ps_mode = 1;
		ps_cfg.ps_idle_period = 40;
		ps_cfg.ps_change_period = 10;
		wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_PS_CFG, (uint32_t)&ps_cfg);
		if(Tokenizer_GetArgsCount() > 1)
		{
			int dtim = Tokenizer_GetArgInteger(1);
			wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_PM_DTIM, dtim);
			wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_LISTEN_INTERVAL, 0);
		}
	}
	else
	{
		wlan_set_ps_mode(g_wlan_netif, 0);
	}
#elif PLATFORM_ECR6600
	if (bOn)
	{
		psm_set_psm_enable(1);
		PSM_SLEEP_SET(MODEM_SLEEP_EN);
		if(bOn >= 2) PSM_SLEEP_SET(WFI_SLEEP_EN);
		PSM_SLEEP_CLEAR(LIGHT_SLEEP_EN);
	}
	else
	{
		psm_sleep_mode_ena_op(true, 0);
		psm_set_psm_enable(0);
		psm_pwr_mgt_ctrl(0);
		psm_sleep_mode_ena_op(true, 0);
	}
#elif PLATFORM_RDA5981
	if(bOn)
	{
		wland_set_sta_sleep(1);
	}
	else
	{
		wland_set_sta_sleep(0);
	}
#else
	ADDLOG_INFO(LOG_FEATURE_CMD, "PowerSave is not implemented on this platform");
#endif
	g_powersave = (bOn);
	return CMD_RES_OK;
}
static commandResult_t CMD_DeepSleep(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int timeMS;

	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	timeMS = Tokenizer_GetArgInteger(0);
#if defined(PLATFORM_BEKEN) && !defined(PLATFORM_BEKEN_NEW)
	// It requires a define in SDK file:
	// OpenBK7231T\platforms\bk7231t\bk7231t_os\beken378\func\include\manual_ps_pub.h
	// define there:
	// #define     PS_SUPPORT_MANUAL_SLEEP     1
	extern void bk_wlan_ps_wakeup_with_timer(UINT32 sleep_time);
	bk_wlan_ps_wakeup_with_timer(timeMS);
	return CMD_RES_OK;
#elif defined(PLATFORM_BEKEN_NEW)
	PS_DEEP_CTRL_PARAM params;
	params.sleep_mode = MANUAL_MODE_IDLE;
	params.wake_up_way = PS_DEEP_WAKEUP_RTC;
	params.sleep_time = timeMS;
	bk_enter_deep_sleep_mode(&params);
	return CMD_RES_OK;
#elif defined(PLATFORM_W600)
#elif PLATFORM_ESPIDF
	esp_sleep_enable_timer_wakeup(timeMS * 1000000);
#if CONFIG_IDF_TARGET_ESP32
	rtc_gpio_isolate(GPIO_NUM_12);
#endif
	esp_deep_sleep_start();
#elif PLATFORM_ESP8266
	esp_wifi_stop();
	esp_deep_sleep(timeMS * 1000000);
#elif PLATFORM_XRADIO
	HAL_Wakeup_SetTimer_mS(timeMS * DS_MS_TO_S);
	pm_enter_mode(DEEP_SLEEP);
#elif PLATFORM_BL602
	HBN_APP_CFG_Type cfg = {
		.useXtal32k = 0,
		.sleepTime = timeMS,
		.gpioWakeupSrc = HBN_WAKEUP_GPIO_NONE,
		.gpioTrigType = HBN_GPIO_INT_TRIGGER_ASYNC_FALLING_EDGE,
		.flashCfg = bl_flash_get_flashCfg(),
		.hbnLevel = HBN_LEVEL_1,
		.ldoLevel = HBN_LDO_LEVEL_1P10V,
	};
	HBN_Mode_Enter(&cfg);
#elif PLATFORM_ECR6600
	//unsigned int sleep_time;
	//if(timeMS < 1000)
	//{
	//	sleep_time = 1;
	//}
	//else
	//{
	//	sleep_time = (timeMS + 1000 - 1) / 1000;
	//}
	// not working
	//psm_set_deep_sleep(sleep_time);
	// not working
	//psm_enter_sleep(DEEP_SLEEP);
	// this works fine, but wifi will not receive anything after waking up, and only full power cycle will fix it up.
	drv_rtc_set_alarm_relative(timeMS * 1000 * 33);
	psm_enter_deep_sleep();
#endif

	return CMD_RES_OK;
}
#if ENABLE_HA_DISCOVERY
static commandResult_t CMD_ScheduleHADiscovery(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int delay;

	if (args && *args) {
		delay = atoi(args);
	}
	else {
		delay = 5;
	}

	Main_ScheduleHomeAssistantDiscovery(delay);

	return CMD_RES_OK;
}
#endif
static commandResult_t CMD_SetFlag(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int flag, val;

	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	flag = Tokenizer_GetArgInteger(0);
	val = Tokenizer_GetArgInteger(1);
	CFG_SetFlag(flag, val);

	return CMD_RES_OK;
}
static commandResult_t CMD_Flags(const void* context, const char* cmd, const char* args, int cmdFlags) {
	union {
		long long newValue;
		struct {
			uint32_t ints[2];
			uint32_t dummy[2]; // just to be safe
		};
	} u;
	// TODO: check on other platforms, on Beken it's 8, 64 bit
	// On Windows simulator it's 8 as well
	//ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_Flags: sizeof(newValue) = %i", sizeof(u.newValue));
	if (args && *args) {
		if (1 != sscanf(args, "%lld", &u.newValue)) {
			//ADDLOG_INFO(LOG_FEATURE_CMD, "Argument/sscanf error!");
			return CMD_RES_BAD_ARGUMENT;
		}
		//ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_Flags: 0-31: %X 32-63: %2 = %X", u.ints[0], u.ints[1]);
		u.newValue = (uint64_t)strtoull(args, NULL, 10);
		//ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_Flags (new): 0-31: %X 32-63: %2 = %X", u.ints[0], u.ints[1]);
	} else {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	CFG_SetFlags(u.ints[0], u.ints[1]);
	ADDLOG_INFO(LOG_FEATURE_CMD, "New flags set!");

	return CMD_RES_OK;
}
static commandResult_t CMD_HTTPOTA(const void* context, const char* cmd, const char* args, int cmdFlags) {

	if (args && *args) {
		OTA_RequestDownloadFromHTTP(args);
	}
	else {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Command requires 1 argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	return CMD_RES_OK;
}
static commandResult_t CMD_Restart(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int delaySeconds;

	if (args == 0 || *args == 0) {
		delaySeconds = 3;
	}
	else {
		delaySeconds = atoi(args);
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_Restart: will reboot in %i", delaySeconds);

	RESET_ScheduleModuleReset(delaySeconds);

	return CMD_RES_OK;
}
static commandResult_t CMD_ClearAll(const void* context, const char* cmd, const char* args, int cmdFlags) {

	CFG_SetDefaultConfig();
	CFG_Save_IfThereArePendingChanges();

	CHANNEL_ClearAllChannels();
	CMD_ClearAllHandlers(0, 0, 0, 0);
	RepeatingEvents_Cmd_ClearRepeatingEvents(0, 0, 0, 0);
#if defined(WINDOWS) || defined(PLATFORM_BL602) || defined(PLATFORM_BEKEN) || defined(PLATFORM_LN882H) \
 || defined(PLATFORM_ESPIDF) || defined(PLATFORM_TR6260) || defined(PLATFORM_REALTEK)
	CMD_resetSVM(0, 0, 0, 0);
#endif

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_ClearAll: all clear");

	return CMD_RES_OK;
}
static commandResult_t CMD_ClearNoPingTime(const void* context, const char* cmd, const char* args, int cmdFlags) {
	g_timeSinceLastPingReply = 0;
	return CMD_RES_OK;
}
static commandResult_t CMD_ClearConfig(const void* context, const char* cmd, const char* args, int cmdFlags) {

	CFG_SetDefaultConfig();
	CFG_Save_IfThereArePendingChanges();

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_ClearConfig: whole device config has been cleared, restart device to connect to clear AP");

	return CMD_RES_OK;
}
static commandResult_t CMD_ClearIO(const void* context, const char* cmd, const char* args, int cmdFlags) {

	CFG_ClearIO();
	CFG_Save_IfThereArePendingChanges();

	return CMD_RES_OK;
}
// setChannel 1 123
// echo First channel is $CH1 and this is the test
// will print echo First channel is 123 and this is the test
static commandResult_t CMD_Echo(const void* context, const char* cmd, const char* args, int cmdFlags) {

#if 0
	ADDLOG_INFO(LOG_FEATURE_CMD, args);
#else
	// we want $CH40 etc expanded
	Tokenizer_TokenizeString(args, TOKENIZER_ALTERNATE_EXPAND_AT_START | TOKENIZER_FORCE_SINGLE_ARGUMENT_MODE);
	ADDLOG_INFO(LOG_FEATURE_CMD, Tokenizer_GetArg(0));
#endif

	return CMD_RES_OK;
}


static commandResult_t CMD_StartupCommand(const void* context, const char* cmd, const char* args, int cmdFlags) {
	const char *cmdToSet;

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Cmd is %s",g_cfg.initCommandLine);
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	cmdToSet = Tokenizer_GetArg(0);
	if (Tokenizer_GetArgIntegerDefault(1, 0) == 1) {
		CFG_SetShortStartupCommand_AndExecuteNow(cmdToSet);
	}
	else {
		CFG_SetShortStartupCommand(cmdToSet);
	}


	return CMD_RES_OK;
}
static commandResult_t CMD_Choice(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int indexToUse;
	const char *cmdToUse;

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	indexToUse = Tokenizer_GetArgInteger(0);
	cmdToUse = Tokenizer_GetArg(1+indexToUse);

	CMD_ExecuteCommand(cmdToUse, cmdFlags);


	return CMD_RES_OK;
}
static commandResult_t CMD_PingHost(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	CFG_SetPingHost(Tokenizer_GetArg(0));

	return CMD_RES_OK;
}
static commandResult_t CMD_PingInterval(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	CFG_SetPingIntervalSeconds(Tokenizer_GetArgInteger(0));

	return CMD_RES_OK;
}
static commandResult_t CMD_SetStartValue(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int ch, val;

	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	ch = Tokenizer_GetArgInteger(0);
	val = Tokenizer_GetArgInteger(1);

	CFG_SetChannelStartupValue(ch, val);

	return CMD_RES_OK;
}
static commandResult_t CMD_OpenAP(const void* context, const char* cmd, const char* args, int cmdFlags) {

	g_openAP = 5;

	return CMD_RES_OK;
}
static commandResult_t CMD_SafeMode(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int i;
	int startSaveModeIn;

	Tokenizer_TokenizeString(args, 0);

	startSaveModeIn = Tokenizer_GetArgIntegerDefault(0, 1);

	// simulate enough boots so the reboot will go into safe mode
	for (i = 0; i <= RESTARTS_REQUIRED_FOR_SAFE_MODE; i++) {
		HAL_FlashVars_IncreaseBootCount();
	}
	if (startSaveModeIn <= 0) {
		startSaveModeIn = 1;
	}
	RESET_ScheduleModuleReset(startSaveModeIn);

	return CMD_RES_OK;
}



void CMD_UARTConsole_Init() {
#if PLATFORM_BEKEN
	UART_InitUART(115200, 0, false);
	cmd_uartInitIndex = get_g_uart_init_counter();
	UART_InitReceiveRingBuffer(512);
#endif
}
void CMD_UARTConsole_Run() {
#if PLATFORM_BEKEN
	char a;
	int i;
	int totalSize;
	char tmp[128];

	totalSize = UART_GetDataSize();
	while (totalSize) {
		a = UART_GetByte(0);
		if (a == '\n' || a == '\r' || a == ' ' || a == '\t') {
			UART_ConsumeBytes(1);
			totalSize = UART_GetDataSize();
		}
		else {
			break;
		}
	}
	if (totalSize < 2) {
		return;
	}
	// skip garbage data (should not happen)
	for (i = 0; i < totalSize; i++) {
		a = UART_GetByte(i);
		if (i + 1 < sizeof(tmp)) {
			tmp[i] = a;
			tmp[i + 1] = 0;
		}
		if (a == '\n') {
			break;
		}
	}
	UART_ConsumeBytes(i);
	CMD_ExecuteCommand(tmp, 0);
#endif
}
void CMD_RunUartCmndIfRequired() {
#if PLATFORM_BEKEN
	if (CFG_HasFlag(OBK_FLAG_CMD_ACCEPT_UART_COMMANDS)) {
		if (cmd_uartInitIndex && cmd_uartInitIndex == get_g_uart_init_counter()) {
			CMD_UARTConsole_Run();
		}
	}
#endif
}

// run an aliased command
static commandResult_t runcmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	char* c = (char*)context;
	//   char *p = c;

	//   while (*p && !isWhiteSpace(*p)) {
	//       p++;
	   //}
	//   if (*p) p++;
	if (*args) {
		return CMD_ExecuteCommandArgs(c, args, cmdFlags);
	}
	return CMD_ExecuteCommand(c, cmdFlags);
}

commandResult_t CMD_CreateAliasHelper(const char *alias, const char *ocmd) {
	char* cmdMem;
	char* aliasMem;
	command_t* existing;

	existing = CMD_Find(alias);
	if (existing != 0) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_Alias: the alias you are trying to use is already in use (as an alias or as a command)");
		return CMD_RES_BAD_ARGUMENT;
	}

	cmdMem = strdup(ocmd);
	aliasMem = strdup(alias);

	ADDLOG_INFO(LOG_FEATURE_CMD, "New alias has been set: %s runs %s", alias, ocmd);

	//cmddetail:{"name":"aliasMem","args":"",
	//cmddetail:"descr":"Internal usage only. See docs for 'alias' command.",
	//cmddetail:"fn":"runcmd","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	command_t *cmd = CMD_RegisterCommand(aliasMem, runcmd, cmdMem);
	if (cmd) {
		cmd->commandFlags |= CMD_FLAG_FREE_NAME;
		cmd->commandFlags |= CMD_FLAG_FREE_CONTEXT;
	}
	return CMD_RES_OK;
}
// run an aliased command
static commandResult_t CMD_CreateAliasForCommand(const void* context, const char* cmd, const char* args, int cmdFlags) {
	const char* alias;
	const char* ocmd;

	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	alias = Tokenizer_GetArg(0);
#if 0
	ocmd = Tokenizer_GetArgFrom(1);
#else
	while (*args && isWhiteSpace(*args)) {
		args++;
	}
	while (*args && !isWhiteSpace(*args)) {
		args++;
	}
	while (*args && isWhiteSpace(*args)) {
		args++;
	}
	ocmd = args;
#endif

	return CMD_CreateAliasHelper(alias, ocmd);
}
/*
int Flash_FindPattern(byte *data, int dataSize, int startOfs, int endOfs) {
	int i;
	float val;
	const char *stop;
	byte* buffer;
	int at;
	int res;
	int matching;

	matching = 0;

	buffer = malloc(1024);

	at = startOfs;
	while (at < endOfs) {
		int readlen = endOfs - at;
		if (readlen > 1024) {
			readlen = 1024;
		}

		res = flash_read((byte*)buffer, readlen, at);


		// Search for the pattern in the buffer
		for (i = 0; i < readlen; i++) {
			if (buffer[i] == data[matching]) {
				matching++;
				if (matching == dataSize) {
					free(buffer);
					return at + i - dataSize + 1;  // Pattern found, return the offset
				}
			}
			else {
				matching = 0;  
				if (buffer[i] == data[matching]) {
					matching++;
				}
			}
		}
		at += readlen;

	}
	free(buffer);
	return -1;
}
*/
// FindPattern 0x0 0x200000 46DCED0E672F3B70AE1276A3F8712E03
/*
commandResult_t CMD_FindPattern(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int startOfs, endOfs;
	int maxData;
	int realDataSize;
	const char *hexStr;
	byte b;
	byte *data;
	int result;

	Tokenizer_TokenizeString(args, 0);

	startOfs = Tokenizer_GetArgInteger(0);
	endOfs = Tokenizer_GetArgInteger(1);
	hexStr = Tokenizer_GetArg(2);

	maxData = strlen(hexStr);
	realDataSize = 0;

	data = malloc(maxData);

	while (hexStr[0] && hexStr[1]) {
		b = hexbyte(hexStr);

		data[realDataSize] = b;
		realDataSize++;

		hexStr += 2;
	}
	result = Flash_FindPattern(data, realDataSize, startOfs, endOfs);

	ADDLOG_INFO(LOG_FEATURE_CMD, "Pattern is at %i",result);

	return CMD_RES_OK;
}*/

#define PWM_FREQUENCY_DEFAULT 1000 //Default Frequency

int g_pwmFrequency = PWM_FREQUENCY_DEFAULT;

commandResult_t CMD_PWMFrequency(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	
	g_pwmFrequency = Tokenizer_GetArgInteger(0);

#ifdef PLATFORM_ESPIDF
	esp_err_t err = ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, (uint32_t)g_pwmFrequency);
	if(err == ESP_ERR_INVALID_ARG)
	{
		ADDLOG_ERROR(LOG_FEATURE_CMD, "ledc_set_freq: invalid arg");
		return CMD_RES_BAD_ARGUMENT;
	}
	else if(err == ESP_FAIL)
	{
		ADDLOG_ERROR(LOG_FEATURE_CMD, "ledc_set_freq: Can not find a proper pre-divider number base on the given frequency and the current duty_resolution");
		return CMD_RES_ERROR;
	}
#endif
	// reapply PWM settings
	PIN_SetupPins();

	return CMD_RES_OK;
}
commandResult_t CMD_IndexRefreshInterval(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	g_indexAutoRefreshInterval = Tokenizer_GetArgInteger(0);
	return CMD_RES_OK;
}
commandResult_t CMD_DeepSleep_SetEdge(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	// strlen("DSEdge") == 6
	if (Tokenizer_GetArgsCount() > 1) {
		// DSEdge [Edge] [Pin]
		PIN_DeepSleep_SetWakeUpEdge(Tokenizer_GetArgInteger(1),Tokenizer_GetArgInteger(0));
	}
	else {
		// DSEdge [Edge]
		PIN_DeepSleep_SetAllWakeUpEdges(Tokenizer_GetArgInteger(0));
	}

	return CMD_RES_OK;
}
static commandResult_t CMD_PowerSave_WFI(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	int bOn = 1;
	Tokenizer_TokenizeString(args, 0);

	if(Tokenizer_GetArgsCount() > 0)
	{
		bOn = Tokenizer_GetArgInteger(0);
	}
	extern bool g_use_wfi;
	g_use_wfi = (bOn);
	return CMD_RES_OK;
}

#if MQTT_USE_TLS
static commandResult_t CMD_WebServer(const void* context, const char* cmd, const char* args, int cmdFlags) {	
	int arg_count;
	Tokenizer_TokenizeString(args, 0);
	arg_count = Tokenizer_GetArgsCount();
	if (arg_count == 0)
	{
		ADDLOG_INFO(LOG_FEATURE_CMD, "WebServer:%d", !CFG_GetDisableWebServer());
		return CMD_RES_OK;
	} 
	if (arg_count == 1) {
		if (strcmp(Tokenizer_GetArg(0) , "0") == 0) {
			ADDLOG_INFO(LOG_FEATURE_CMD, "Stop WebServer");
			CFG_SetDisableWebServer(true);
			CFG_Save_IfThereArePendingChanges();
			HTTPServer_Stop();
			return CMD_RES_OK;
		}
		else if (strcmp(Tokenizer_GetArg(0), "1") == 0) {
			ADDLOG_INFO(LOG_FEATURE_CMD, "Enable WebServer and restart");
			CFG_SetDisableWebServer(false);
			CFG_Save_IfThereArePendingChanges();
			HAL_RebootModule();
			return CMD_RES_OK;
		}
	} 
	ADDLOG_ERROR(LOG_FEATURE_CMD, "Invalid Argument");
	return CMD_RES_BAD_ARGUMENT;
}
#endif

void CMD_Init_Early() {
	//cmddetail:{"name":"alias","args":"[Alias][Command with spaces]",
	//cmddetail:"descr":"add an aliased command, so a command with spaces can be called with a short, nospaced alias",
	//cmddetail:"fn":"CMD_CreateAliasForCommand","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("alias", CMD_CreateAliasForCommand, NULL);
	//cmddetail:{"name":"echo","args":"[Message]",
	//cmddetail:"descr":"Sends given message back to console. This command expands variables, so writing $CH12 will print value of channel 12, etc. Remember that you can also use special channel indices to access persistant flash variables and to access LED variables like dimmer, etc.",
	//cmddetail:"fn":"CMD_Echo","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("echo", CMD_Echo, NULL);
	//cmddetail:{"name":"restart","args":"",
	//cmddetail:"descr":"Reboots the module",
	//cmddetail:"fn":"CMD_Restart","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("restart", CMD_Restart, NULL);
	//cmddetail:{"name":"reboot","args":"",
	//cmddetail:"descr":"Same as restart. Needed for bkWriter 1.60 which sends 'reboot' cmd before trying to get bus via UART. Thanks to this, if you enable command line on UART1, you don't need to manually reboot while flashing via UART.",
	//cmddetail:"fn":"CMD_Restart","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("reboot", CMD_Restart, NULL);
	//cmddetail:{"name":"clearConfig","args":"",
	//cmddetail:"descr":"Clears all config, including WiFi data",
	//cmddetail:"fn":"CMD_ClearConfig","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("clearConfig", CMD_ClearConfig, NULL);
	//cmddetail:{"name":"clearIO","args":"",
	//cmddetail:"descr":"Clears all pins setting, channels settings",
	//cmddetail:"fn":"CMD_ClearIO","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("clearIO", CMD_ClearIO, NULL);
	//cmddetail:{"name":"clearAll","args":"",
	//cmddetail:"descr":"Clears config and all remaining features, like runtime scripts, events, etc",
	//cmddetail:"fn":"CMD_ClearAll","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("clearAll", CMD_ClearAll, NULL);
	//cmddetail:{"name":"DeepSleep","args":"[Seconds]",
	//cmddetail:"descr":"Starts deep sleep for given amount of seconds. Please remember that there is also a separate command, called PinDeepSleep, which is not using a timer, but a GPIO to wake up device.",
	//cmddetail:"fn":"CMD_DeepSleep","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("DeepSleep", CMD_DeepSleep, NULL);
	//cmddetail:{"name":"PowerSave","args":"[Optional 1 or 0, by default 1 is assumed]",
	//cmddetail:"descr":"Enables dynamic power saving mode on all platforms with the exception of TR6260. On LN882H PowerSave 1 = light sleep and Powersave >1 (eg PowerSave 2) = deeper sleep. On LN882H PowerSave 1 should be used if BL0937 metering is present. On all supported platforms PowerSave 0 can be used to disable power saving.",
	//cmddetail:"fn":"CMD_PowerSave","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("PowerSave", CMD_PowerSave, NULL);
	//cmddetail:{"name":"PowerSave_WFI","args":"[Optional 1 or 0, by default 1 is assumed]",
	//cmddetail:"descr":"Extra power save by sleeping during idle. Available on Beken, BL602, XR8xx, Realteks, W600 and W800. Reduces current by 0.001-0.002A on Beken. Enabled by default on BL602/W600/W800 (SDK behaviour).",
	//cmddetail:"fn":"CMD_PowerSave_WFI","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("PowerSave_WFI", CMD_PowerSave_WFI, NULL);
	//cmddetail:{"name":"if","args":"[Condition]['then'][CommandA]['else'][CommandB]",
	//cmddetail:"descr":"Executed a conditional. Condition should be single line. You must always use 'then' after condition. 'else' is optional. Use aliases or quotes for commands with spaces",
	//cmddetail:"fn":"CMD_If","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("if", CMD_If, NULL);
	//cmddetail:{"name":"ota_http","args":"[HTTP_URL]",
	//cmddetail:"descr":"Starts the firmware update procedure, the argument should be a reachable HTTP server file. You can easily setup HTTP server with Xampp, or Visual Code, or Python, etc. Make sure you are using OTA file for a correct platform (getting N platform RBL on T will brick device, etc etc)",
	//cmddetail:"fn":"CMD_HTTPOTA","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("ota_http", CMD_HTTPOTA, NULL);
#if ENABLE_HA_DISCOVERY
	//cmddetail:{"name":"scheduleHADiscovery","args":"[Seconds]",
	//cmddetail:"descr":"This will schedule HA discovery, the discovery will happen with given number of seconds, but timer only counts when MQTT is connected. It will not work without MQTT online, so you must set MQTT credentials first.",
	//cmddetail:"fn":"CMD_ScheduleHADiscovery","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("scheduleHADiscovery", CMD_ScheduleHADiscovery, NULL);
#endif
	//cmddetail:{"name":"flags","args":"[IntegerValue]",
	//cmddetail:"descr":"Sets the device flags",
	//cmddetail:"fn":"CMD_Flags","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("flags", CMD_Flags, NULL);
	//cmddetail:{"name":"SetFlag","args":"[FlagIndex][0or1]",
	//cmddetail:"descr":"Sets given flag",
	//cmddetail:"fn":"CMD_SetFlag","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SetFlag", CMD_SetFlag, NULL);
	//cmddetail:{"name":"ClearNoPingTime","args":"",
	//cmddetail:"descr":"Command for ping watchdog; it sets the 'time since last ping reply' to 0 again",
	//cmddetail:"fn":"CMD_ClearNoPingTime","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("ClearNoPingTime", CMD_ClearNoPingTime, NULL);
	//cmddetail:{"name":"SetStartValue","args":"[Channel][Value]",
	//cmddetail:"descr":"Sets the startup value for a channel. Used for start values for relays. Use 1 for High, 0 for low and -1 for 'remember last state'",
	//cmddetail:"fn":"CMD_SetStartValue","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SetStartValue", CMD_SetStartValue, NULL);
	//cmddetail:{"name":"OpenAP","args":"",
	//cmddetail:"descr":"Temporarily disconnects from programmed WiFi network and opens Access Point",
	//cmddetail:"fn":"CMD_OpenAP","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("OpenAP", CMD_OpenAP, NULL);
	//cmddetail:{"name":"DSEdge","args":"[edgeCode][optionalPinIndex]",
	//cmddetail:"descr":"DeepSleep (PinDeepSleep) wake configuration command. 0 means always wake up on rising edge, 1 means on falling, 2 means if state is high, use falling edge, if low, use rising. Default is 2. Second argument is optional and allows to set per-pin DSEdge instead of setting it for all pins.",
	//cmddetail:"fn":"CMD_DeepSleep_SetEdge","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("DSEdge", CMD_DeepSleep_SetEdge, NULL);
	//cmddetail:{"name":"SafeMode","args":"[OptionalDelayBeforeRestart]",
	//cmddetail:"descr":"Forces device reboot into safe mode (open ap with disabled drivers). Argument is a delay to restart in seconds, optional, minimal delay is 1",
	//cmddetail:"fn":"CMD_SafeMode","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SafeMode", CMD_SafeMode, NULL);
#if ENABLE_PING_WATCHDOG
	//cmddetail:{"name":"PingInterval","args":"[IntegerSeconds]",
	//cmddetail:"descr":"Sets the interval between ping attempts for ping watchdog mechanism",
	//cmddetail:"fn":"CMD_PingInterval","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("PingInterval", CMD_PingInterval, NULL);
	//cmddetail:{"name":"PingHost","args":"[IPStr]",
	//cmddetail:"descr":"Sets the host to ping by IP watchdog",
	//cmddetail:"fn":"CMD_PingHost","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("PingHost", CMD_PingHost, NULL);
#endif
	//cmddetail:{"name":"StartupCommand","args":"[Command in quotation marks][bRunAfter]",
	//cmddetail:"descr":"Sets the new startup command (short startup command, the one stored in config) to given string. Second argument is optional, if set to 1, command will be also executed after setting",
	//cmddetail:"fn":"CMD_StartupCommand","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("StartupCommand", CMD_StartupCommand, NULL);
	//cmddetail:{"name":"Choice","args":"[IndexToExecute][Option0][Option1][Option2][OptionN][etc]",
	//cmddetail:"descr":"This will choose a given argument by index and execute it as a command. Index to execute can be a variable like $CH1.",
	//cmddetail:"fn":"CMD_Choice","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("Choice", CMD_Choice, NULL);
	//cmddetail:{"name":"PWMFrequency","args":"[FrequencyInHz]",
	//cmddetail:"descr":"Sets the global PWM frequency.",
	//cmddetail:"fn":"CMD_PWMFrequency","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("PWMFrequency", CMD_PWMFrequency, NULL);

	//cmddetail:{"name":"IndexRefreshInterval","args":"[Interval]",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_IndexRefreshInterval","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("IndexRefreshInterval", CMD_IndexRefreshInterval, NULL);

#if MQTT_USE_TLS
	//cmddetail:{"name":"WebServer","args":"[0 - Stop / 1 - Start]",
	//cmddetail:"descr":"Setting state of WebServer",
	//cmddetail:"fn":"CMD_WebServer","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("WebServer", CMD_WebServer, NULL);
#endif
	
#if ENABLE_OBK_SCRIPTING
	CMD_InitScripting();
#endif
#if ENABLE_OBK_BERRY
	CMD_InitBerry();
#endif
	if (!bSafeMode) {
		if (CFG_HasFlag(OBK_FLAG_CMD_ACCEPT_UART_COMMANDS)) {
			CMD_UARTConsole_Init();
		}
	}
}



void CMD_Init_Delayed() {
#if ENABLE_TCP_COMMANDLINE
	if (CFG_HasFlag(OBK_FLAG_CMD_ENABLETCPRAWPUTTYSERVER)) {
		CMD_StartTCPCommandLine();
	}
#endif
#if PLATFORM_BEKEN || WINDOWS || PLATFORM_BL602 || PLATFORM_ESPIDF || PLATFORM_ESP8266 \
	|| PLATFORM_REALTEK || PLATFORM_ECR6600 || PLATFORM_XRADIO
	UART_AddCommands();
#endif
#if ENABLE_BL_TWIN
#if ENABLE_DRIVER_BL0942
	BL0942_AddCommands();
#endif
#endif
}


void CMD_ListAllCommands(void* userData, void (*callback)(command_t* cmd, void* userData)) {
	int i;
	command_t* newCmd;

	for (i = 0; i < HASH_SIZE; i++) {
		newCmd = g_commands[i];
		while (newCmd) {
			callback(newCmd, userData);
			newCmd = newCmd->next;
		}
	}

}
void CMD_FreeAllCommands() {
	int i;
	command_t* cmd, * next;

	for (i = 0; i < HASH_SIZE; i++) {
		cmd = g_commands[i];
		while (cmd) {
			next = cmd->next;
			if (cmd->commandFlags & CMD_FLAG_FREE_NAME) {
				free((char*)cmd->name);
			}
			if (cmd->commandFlags & CMD_FLAG_FREE_CONTEXT) {
				free((char*)cmd->context);
			}
			free(cmd);
			cmd = next;
		}
		g_commands[i] = 0;
	}

}
command_t *CMD_RegisterCommand(const char* name, commandHandler_t handler, void* context) {
	int hash;
	command_t* newCmd;

	// check
	newCmd = CMD_Find(name);
	if (newCmd != 0) {
		// it happens very often in Simulator due to the lack of the ability to remove commands
		if (newCmd->handler != handler) {
			ADDLOG_ERROR(LOG_FEATURE_CMD, "command with name %s already exists!", name);
		}
		return 0;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CMD, "Adding command %s", name);

	hash = generateHashValue(name);
	newCmd = (command_t*)malloc(sizeof(command_t));
	newCmd->commandFlags = 0;
	newCmd->handler = handler;
	newCmd->name = name;
	newCmd->next = g_commands[hash];
	newCmd->context = context;
	g_commands[hash] = newCmd;
	return newCmd;
}

command_t* CMD_Find(const char* name) {
	int hash;
	command_t* newCmd;

	hash = generateHashValue(name);

	newCmd = g_commands[hash];
	while (newCmd != 0) {
		if (!stricmp(newCmd->name, name)) {
			return newCmd;
		}
		newCmd = newCmd->next;
	}
	return 0;
}

// get a string up to whitespace.
// if stripnum is set, stop at numbers.
int get_cmd(const char* s, char* dest, int maxlen, int stripnum) {
	int i;
	int count = 0;
	for (i = 0; i < maxlen - 1; i++) {
		if (isWhiteSpace(*s)) {
			break;
		}
		if (*s == 0) {
			break;
		}
		if (stripnum && *s >= '0' && *s <= '9') {
			break;
		}
		*(dest++) = *(s++);
		count++;
	}
	*dest = '\0';
	return count;
}


// execute a command from cmd and args - used below and in MQTT
commandResult_t CMD_ExecuteCommandArgs(const char* cmd, const char* args, int cmdFlags) {
	command_t* newCmd;
	//int len;

	// look for complete commmand
	newCmd = CMD_Find(cmd);
	if (!newCmd) {
		// not found, so...
		char nonums[32];
		// get the complete string up to numbers.
		//len =
		get_cmd(cmd, nonums, 32, 1);
		newCmd = CMD_Find(nonums);
		if (!newCmd) {
#if ENABLE_OBK_BERRY
			static int g_guard = 0;
			if (g_guard == 0) {
				g_guard = 1;
				int c_run = CMD_Berry_RunEventHandlers_Str(CMD_EVENT_ON_CMD, cmd, args);
				g_guard = 0;
				if (c_run > 0) {
					return CMD_RES_OK;
				}
			}
#endif
			// if still not found, then error
			ADDLOG_ERROR(LOG_FEATURE_CMD, "cmd %s NOT found (args %s)", cmd, args);
			return CMD_RES_UNKNOWN_COMMAND;
		}
	}
	else {
	}

	if (newCmd->handler) {
		commandResult_t res;
		res = newCmd->handler(newCmd->context, cmd, args, cmdFlags);
		return res;
	}
	return CMD_RES_UNKNOWN_COMMAND;
}


// execute a raw command - single string
commandResult_t CMD_ExecuteCommand(const char* s, int cmdFlags) {
	const char* p;
	const char* args;

	char copy[128];
	int len;
	//const char *org;

	if (s == 0 || *s == 0) {
		return CMD_RES_EMPTY_STRING;
	}

	while (isWhiteSpace(*s)) {
		s++;
	}
	if (*s == 0) {
		return CMD_RES_EMPTY_STRING;
	}
	if ((cmdFlags & COMMAND_FLAG_SOURCE_TCP) == 0) {
		ADDLOG_DEBUG(LOG_FEATURE_CMD, "cmd [%s]", s);
	}
	//org = s;

	// get the complete string up to whitespace.
	len = get_cmd(s, copy, sizeof(copy), 0);
	s += len;

	p = s;

	while (*p && isWhiteSpace(*p)) {
		p++;
	}
	args = p;

	return CMD_ExecuteCommandArgs(copy, args, cmdFlags);
}


