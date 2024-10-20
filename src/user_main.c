/*

 */
 //

#include "hal/hal_wifi.h"
#include "hal/hal_generic.h"
#include "hal/hal_flashVars.h"
#include "hal/hal_adc.h"
#include "new_common.h"

//#include "driver/drv_ir.h"
#include "driver/drv_public.h"
//#include "ir/ir_local.h"

// Commands register, execution API and cmd tokenizer
#include "cmnds/cmd_public.h"

// overall config variables for app - like ENABLE_LITTLEFS
#include "obk_config.h"

#include "httpserver/new_http.h"
#include "httpserver/http_fns.h"
#include "new_pins.h"
#include "quicktick.h"
#include "new_cfg.h"
#include "logging/logging.h"
#include "httpserver/http_tcp_server.h"
#include "httpserver/rest_interface.h"
#include "mqtt/new_mqtt.h"
#include "ota/ota.h"

#if ENABLE_LITTLEFS
#include "littlefs/our_lfs.h"
#endif


#include "driver/drv_ntp.h"
#include "driver/drv_ssdp.h"
#include "driver/drv_uart.h"

#if PLATFORM_BEKEN
#include <mcu_ps.h>
#include <fake_clock_pub.h>
#include <BkDriverWdg.h>
#include "temp_detect_pub.h"
void bg_register_irda_check_func(FUNCPTR func);
#elif PLATFORM_BL602
#include <bl_sys.h>
#include <bl_adc.h>     //  For BL602 ADC HAL
#include <bl602_adc.h>  //  For BL602 ADC Standard Driver
#include <bl602_glb.h>  //  For BL602 Global Register Standard Driver
#include <bl_wdt.h>
#elif PLATFORM_W600 || PLATFORM_W800
#include "wm_watchdog.h"
#elif PLATFORM_LN882H
#include "hal/hal_wdt.h"
#include "hal/hal_gpio.h"
#elif PLATFORM_ESPIDF
#include "esp_timer.h"
#endif

int g_secondsElapsed = 0;
// open access point after this number of seconds
int g_openAP = 0;
// connect to wifi after this number of seconds
static int g_connectToWiFi = 0;
// reset after this number of seconds
static int g_reset = 0;
// is connected to WiFi?
static int g_bHasWiFiConnected = 0;
// is Open Access point or a client?
static int g_bOpenAccessPointMode = 0;
// in safe mode, user can press a button to enter the unsafe one
static int g_doUnsafeInitIn = 0;
int g_bootFailures = 0;
static int g_saveCfgAfter = 0;
int g_startPingWatchDogAfter = 60;
// many boots failed? do not run pins or anything risky
int bSafeMode = 0;
// not really <time>, but rather a loop count, but it doesn't really matter much
// start disabled.
int g_timeSinceLastPingReply = -1;
int g_prevTimeSinceLastPingReply = -1;
// was it ran?
static int g_bPingWatchDogStarted = 0;
// current IP string, this is compared with IP returned from HAL
// and if it changes, the MQTT publish is done
static char g_currentIPString[32] = { 0 };
static HALWifiStatus_t g_newWiFiStatus = WIFI_UNDEFINED;
static HALWifiStatus_t g_prevWiFiStatus = WIFI_UNDEFINED;
static int g_noMQTTTime = 0;

uint8_t g_StartupDelayOver = 0;

uint32_t idleCount = 0;

int DRV_SSDP_Active = 0;

#define LOG_FEATURE LOG_FEATURE_MAIN

void Main_ForceUnsafeInit();

#if PLATFORM_XR809
size_t xPortGetFreeHeapSize() {
	return 0;
}
#endif

#if PLATFORM_BL602
/// Read the Internal Temperature Sensor as Float. Returns 0 if successful.
/// Based on bl_tsen_adc_get in https://github.com/lupyuen/bl_iot_sdk/blob/master/components/hal_drv/bl602_hal/bl_adc.c#L224-L282
static int get_tsen_adc(
	float *temp,      //  Pointer to float to store the temperature
	uint8_t log_flag  //  0 to disable logging, 1 to enable logging
) {
	static uint16_t tsen_offset = 0xFFFF;
	float val = 0.0;

	//  If the offset has not been fetched...
	if (0xFFFF == tsen_offset) {
		//  Define the ADC configuration
		tsen_offset = 0;
		ADC_CFG_Type adcCfg = {
		  .v18Sel = ADC_V18_SEL_1P82V,                /*!< ADC 1.8V select */
		  .v11Sel = ADC_V11_SEL_1P1V,                 /*!< ADC 1.1V select */
		  .clkDiv = ADC_CLK_DIV_32,                   /*!< Clock divider */
		  .gain1 = ADC_PGA_GAIN_1,                    /*!< PGA gain 1 */
		  .gain2 = ADC_PGA_GAIN_1,                    /*!< PGA gain 2 */
		  .chopMode = ADC_CHOP_MOD_AZ_PGA_ON,         /*!< ADC chop mode select */
		  .biasSel = ADC_BIAS_SEL_MAIN_BANDGAP,       /*!< ADC current form main bandgap or aon bandgap */
		  .vcm = ADC_PGA_VCM_1V,                      /*!< ADC VCM value */
		  .vref = ADC_VREF_2V,                        /*!< ADC voltage reference */
		  .inputMode = ADC_INPUT_SINGLE_END,          /*!< ADC input signal type */
		  .resWidth = ADC_DATA_WIDTH_16_WITH_256_AVERAGE,  /*!< ADC resolution and oversample rate */
		  .offsetCalibEn = 0,                         /*!< Offset calibration enable */
		  .offsetCalibVal = 0,                        /*!< Offset calibration value */
		};
		ADC_FIFO_Cfg_Type adcFifoCfg = {
		  .fifoThreshold = ADC_FIFO_THRESHOLD_1,
		  .dmaEn = DISABLE,
		};

		//  Enable and reset the ADC
		GLB_Set_ADC_CLK(ENABLE, GLB_ADC_CLK_96M, 7);
		ADC_Disable();
		ADC_Enable();
		ADC_Reset();

		//  Configure the ADC and Internal Temperature Sensor
		ADC_Init(&adcCfg);
		ADC_Channel_Config(ADC_CHAN_TSEN_P, ADC_CHAN_GND, 0);
		ADC_Tsen_Init(ADC_TSEN_MOD_INTERNAL_DIODE);
		ADC_FIFO_Cfg(&adcFifoCfg);

		//  Fetch the offset
		BL_Err_Type rc = ADC_Trim_TSEN(&tsen_offset);

		//  Must wait 100 milliseconds or returned temperature will be negative
		rtos_delay_milliseconds(100);
	}
	//  Read the temperature based on the offset
	val = TSEN_Get_Temp(tsen_offset);
	if (log_flag) {
		printf("offset = %d\r\n", tsen_offset);
		printf("temperature = %f Celsius\r\n", val);
	}
	//  Return the temperature
	*temp = val;
	return 0;
}
#endif

#ifdef PLATFORM_BK7231T
// this function waits for the extended app functions to finish starting.
extern void extended_app_waiting_for_launch(void);
void extended_app_waiting_for_launch2() {
	extended_app_waiting_for_launch();
}
#else
void extended_app_waiting_for_launch2(void) {
	// do nothing?

	// define FIXED_DELAY if delay wanted on non-beken platforms.
#ifdef PLATFORM_BK7231N
	// wait 100ms at the start.
	// TCP is being setup in a different thread, and there does not seem to be a way to find out if it's complete yet?
	// so just wait a bit, and then start.
	int startDelay = 750;
	bk_printf("\r\ndelaying start\r\n");
	for (int i = 0; i < startDelay / 10; i++) {
		rtos_delay_milliseconds(10);
		bk_printf("#Startup delayed %dms#\r\n", i * 10);
	}
	bk_printf("\r\nstarting....\r\n");

	// through testing, 'Initializing TCP/IP stack' appears at ~500ms
	// so we should wait at least 750?
#endif

}
#endif


#if defined(PLATFORM_LN882H) || defined(PLATFORM_ESPIDF)

int LWIP_GetMaxSockets() {
	return 0;
}
int LWIP_GetActiveSockets() {
	return 0;
}
#endif

#if defined(PLATFORM_BL602) || defined(PLATFORM_W800) || defined(PLATFORM_W600)|| defined(PLATFORM_LN882H) || defined(PLATFORM_ESPIDF)

OSStatus rtos_create_thread(beken_thread_t* thread,
	uint8_t priority, const char* name,
	beken_thread_function_t function,
	uint32_t stack_size, beken_thread_arg_t arg) {
	OSStatus err = kNoErr;


	err = xTaskCreate(function, name, stack_size / sizeof(StackType_t), arg, priority, thread);
	/*
	 BaseType_t xTaskCreate(
								  TaskFunction_t pvTaskCode,
								  const char * const pcName,
								  configSTACK_DEPTH_TYPE usStackDepth,
								  void *pvParameters,
								  UBaseType_t uxPriority,
								  TaskHandle_t *pvCreatedTask
							  );
	*/
	if (err == pdPASS) {
		//printf("Thread create %s - pdPASS\n",name);
		return 0;
	}
	else if (err == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY) {
		printf("Thread create %s - errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY\n", name);
	}
	else {
		printf("Thread create %s - err %i\n", name, err);
	}
	return 1;
}

OSStatus rtos_delete_thread(beken_thread_t* thread) {
	vTaskDelete(thread);
	return kNoErr;
}
#endif
void MAIN_ScheduleUnsafeInit(int delSeconds) {
	g_doUnsafeInitIn = delSeconds;
}
void RESET_ScheduleModuleReset(int delSeconds) {
	g_reset = delSeconds;
}


static char scheduledDriverName[4][16];
static int scheduledDelay[4] = { -1, -1, -1, -1 };
void ScheduleDriverStart(const char* name, int delay) {
	int i;

	for (i = 0; i < 4; i++) {
		// if already scheduled, just change delay.
		if (!strcmp(scheduledDriverName[i], name)) {
			scheduledDelay[i] = delay;
			return;
		}
	}
	for (i = 0; i < 4; i++) {
		// first empty slot
		if (scheduledDelay[i] == -1) {
			scheduledDelay[i] = delay;
			strncpy(scheduledDriverName[i], name, 16);
			return;
		}
	}
}

#if defined(PLATFORM_LN882H)
// LN882H hack, maybe place somewhere else?
// this will be applied after WiFi connect
extern int g_ln882h_pendingPowerSaveCommand;
void LN882H_ApplyPowerSave(int bOn);
#endif

// SSID switcher by xjikka 20240525
#if ALLOW_SSID2
static int g_SSIDactual = 0;        // 0=SSID1 1=SSID2
static int g_SSIDSwitchAfterTry = 3;// switch to opposite SSID after
static int g_SSIDSwitchCnt = 0;     // switch counter
#endif

void CheckForSSID12_Switch() {
#if ALLOW_SSID2
	// nothing to do if SSID2 is unset 
	if (CFG_GetWiFiSSID2()[0] == 0) return;
	if (g_SSIDSwitchCnt++ < g_SSIDSwitchAfterTry) {
		ADDLOGF_INFO("WiFi SSID: waiting for SSID switch %d/%d (using SSID%d)\r\n", g_SSIDSwitchCnt, g_SSIDSwitchAfterTry, g_SSIDactual+1);
		return;
	}
	g_SSIDSwitchCnt = 0;
	g_SSIDactual ^= 1;	// toggle SSID 
	ADDLOGF_INFO("WiFi SSID: switching to SSID%i\r\n", g_SSIDactual + 1);
#endif
}

const char* CFG_GetWiFiSSIDX() {
#if ALLOW_SSID2
	if (g_SSIDactual) {
		return CFG_GetWiFiSSID2();
	}
	else {
		return CFG_GetWiFiSSID();
	}
#else
	return CFG_GetWiFiSSID();
#endif
}

const char* CFG_GetWiFiPassX() {
#if ALLOW_SSID2
	if (g_SSIDactual) {
		return CFG_GetWiFiPass2();
	}
	else {
		return CFG_GetWiFiPass();
	}
#else
	return CFG_GetWiFiPass();
#endif
}

void Main_OnWiFiStatusChange(int code)
{
	// careful what you do in here.
	// e.g. creata socket?  probably not....
	switch (code)
	{
	case WIFI_STA_CONNECTING:
		g_bHasWiFiConnected = 0;
		g_connectToWiFi = 120;
		ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_STA_CONNECTING - %i\r\n", code);
		break;
	case WIFI_STA_DISCONNECTED:
		// try to connect again in few seconds
		if (g_bHasWiFiConnected != 0)
		{
			HAL_DisconnectFromWifi();
		}
		g_connectToWiFi = 15;
		g_bHasWiFiConnected = 0;
		g_timeSinceLastPingReply = -1;
		ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_STA_DISCONNECTED - %i\r\n", code);
		break;
	case WIFI_STA_AUTH_FAILED:
		// try to connect again in few seconds
		// for me first auth will often fail, so retry more aggressively during startup
		// the maximum of 6 tries during first 30 seconds should be acceptable
		if (g_secondsElapsed < 30) {
			g_connectToWiFi = 5;
		}
		else {
			g_connectToWiFi = 60;
		}
		g_bHasWiFiConnected = 0;
		ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_STA_AUTH_FAILED - %i\r\n", code);
		break;
	case WIFI_STA_CONNECTED:
		g_bHasWiFiConnected = 1;
#if ALLOW_SSID2
		g_SSIDSwitchCnt = 0;
#endif
		ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_STA_CONNECTED - %i\r\n", code);

		if (bSafeMode == 0) {
			if (strlen(CFG_DeviceGroups_GetName()) > 0) {
				ScheduleDriverStart("DGR", 5);
			}
			// if SSDP should be active, 
			// restart it now.
			if (DRV_SSDP_Active) {
				ScheduleDriverStart("SSDP", 5);
				//DRV_SSDP_Restart(); // this kills things
			}
		}
#if defined(PLATFORM_LN882H)
		// LN882H hack, maybe place somewhere else?
		// this will be applied only if WiFi is connected
		if (g_ln882h_pendingPowerSaveCommand != -1) {
			ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_PowerSave: applying delayed setting. PowerSave will set to %i", g_ln882h_pendingPowerSaveCommand);
			LN882H_ApplyPowerSave(g_ln882h_pendingPowerSaveCommand);
			g_ln882h_pendingPowerSaveCommand = -1;
		}
#endif
		break;
		/* for softap mode */
	case WIFI_AP_CONNECTED:
		g_bHasWiFiConnected = 1;
		ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_AP_CONNECTED - %i\r\n", code);
		break;
	case WIFI_AP_FAILED:
		g_bHasWiFiConnected = 0;
		ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_AP_FAILED - %i\r\n", code);
		break;
	default:
		break;
	}
	g_newWiFiStatus = code;
}


void CFG_Save_SetupTimer()
{
	g_saveCfgAfter = 3;
}

void Main_OnPingCheckerReply(int ms)
{
	g_timeSinceLastPingReply = 0;
}

int g_doHomeAssistantDiscoveryIn = 0;
int g_bBootMarkedOK = 0;
int g_rebootReason = 0;
static int bMQTTconnected = 0;

int Main_HasMQTTConnected()
{
	return bMQTTconnected;
}

int Main_HasWiFiConnected()
{
	return g_bHasWiFiConnected;
}

#ifdef OBK_MCU_SLEEP_METRICS_ENABLE
extern OBK_MCU_SLEEP_METRICS OBK_Mcu_metrics;
void Main_LogPowerSave() {
	if (!OBK_Mcu_metrics.nexttask) {
		OBK_Mcu_metrics.nexttask = "unkn";
	}
	if (!OBK_Mcu_metrics.task) {
		OBK_Mcu_metrics.task = "unkn2";
	}
	ADDLOGF_DEBUG("PS: %ums/%ums longests:%ums/%ums req:%ums/%ums %s %s",
		BK_TICKS_TO_MS(OBK_Mcu_metrics.slept_ticks),
		BK_TICKS_TO_MS(OBK_Mcu_metrics.sleep_requested_ticks),
		BK_TICKS_TO_MS(OBK_Mcu_metrics.longest_sleep_1s),
		BK_TICKS_TO_MS(OBK_Mcu_metrics.longest_sleep),
		BK_TICKS_TO_MS(OBK_Mcu_metrics.longest_sleep_req_1s),
		BK_TICKS_TO_MS(OBK_Mcu_metrics.longest_sleep_req),
		OBK_Mcu_metrics.nexttask,
		OBK_Mcu_metrics.task
	);
	// see mcu_ps.c for reasons...
	ADDLOGF_DEBUG("PS: nosleep reasons %d %d %d %d %d",
		OBK_Mcu_metrics.reasons[0], // not enabled
		OBK_Mcu_metrics.reasons[1], // peri_busy
		OBK_Mcu_metrics.reasons[2], // mcu_prevent
		OBK_Mcu_metrics.reasons[3], // txl_sleep_check
		OBK_Mcu_metrics.reasons[4] // too short (<=2ms?)
	);
	memset(OBK_Mcu_metrics.reasons, 0, sizeof(OBK_Mcu_metrics.reasons));

	OBK_Mcu_metrics.slept_ticks = 0;
	OBK_Mcu_metrics.sleep_requested_ticks = 0;
	OBK_Mcu_metrics.longest_sleep_1s = 0;
	OBK_Mcu_metrics.longest_sleep_req_1s = 0;

	/*
		extern int bk_wlan_power_save_set_level(BK_PS_LEVEL level);

		if (g_sleep == 1){
			OBK_Mcu_metrics.longest_sleep = 0;
			g_sleep = 2;
			bk_wlan_power_save_set_level(
				// PS_DEEP_SLEEP_BIT |
				PS_RF_SLEEP_BIT |
				PS_MCU_SLEEP_BIT);
		}
	*/
}
#endif

/// @brief Schedule HomeAssistant discovery. The caller should check OBK_FLAG_AUTOMAIC_HASS_DISCOVERY if necessary.
/// @param seconds 
void Main_ScheduleHomeAssistantDiscovery(int seconds) {
	g_doHomeAssistantDiscoveryIn = seconds;
}

void Main_ConnectToWiFiNow() {
	const char* wifi_ssid, * wifi_pass;

	g_bOpenAccessPointMode = 0;
	CheckForSSID12_Switch();
	wifi_ssid = CFG_GetWiFiSSIDX();
	wifi_pass = CFG_GetWiFiPassX();
	// register function to get callbacks about wifi changes .. 
	// ... but do it, before calling HAL_ConnectToWiFi(), 
	// otherwise callbacks are not possible (e.g. WIFI_STA_CONNECTING can never be called )!!
	HAL_WiFi_SetupStatusCallback(Main_OnWiFiStatusChange);
	ADDLOGF_INFO("Registered for wifi changes\r\n");
	ADDLOGF_INFO("Connecting to SSID [%s]\r\n", wifi_ssid);
	HAL_ConnectToWiFi(wifi_ssid, wifi_pass, &g_cfg.staticIP);
	// don't set g_connectToWiFi = 0; here!
	// this would overwrite any changes, e.g. from Main_OnWiFiStatusChange !
	// so don't do this here, but e.g. set in Main_OnWiFiStatusChange if connected!!!
}
bool Main_HasFastConnect() {
	if (g_bootFailures > 2)
		return false;
	if (CFG_HasFlag(OBK_FLAG_WIFI_FAST_CONNECT)) {
		return true;
	}
	if ((PIN_FindPinIndexForRole(IOR_DoorSensorWithDeepSleep, -1) != -1) ||
		(PIN_FindPinIndexForRole(IOR_DoorSensorWithDeepSleep_NoPup, -1) != -1))
	{
		return true;
	}
	return false;
}
#if PLATFORM_LN882H || PLATFORM_ESPIDF
// Quick hack to display LN-only temperature,
// we may improve it in the future
extern float g_wifi_temperature;
#else
float g_wifi_temperature = 0;
#endif

static byte g_secondsSpentInLowMemoryWarning = 0;
void Main_OnEverySecond()
{
	int newMQTTState;
	const char* safe;
	int i;

#ifdef WINDOWS
	g_bHasWiFiConnected = 1;
#endif

	// display temperature - thanks to giedriuslt
// only in Normal mode, and if boot is not failing
	if (!bSafeMode && g_bootFailures <= 1)
	{
#if PLATFORM_BEKEN
		UINT32 temperature;
		temp_single_get_current_temperature(&temperature);
#if PLATFORM_BK7231T
		g_wifi_temperature = 2.21f * (temperature / 25.0f) - 65.91f;
#else
		g_wifi_temperature = (-0.457f * temperature) + 188.474f;
#endif
#elif PLATFORM_BL602
		get_tsen_adc(&g_wifi_temperature, 0);
#elif PLATFORM_LN882H
		// this is set externally, I am just leaving comment here
#endif
	}
	// run_adc_test();
	newMQTTState = MQTT_RunEverySecondUpdate();
	if (newMQTTState != bMQTTconnected) {
		bMQTTconnected = newMQTTState;
		if (newMQTTState) {
			EventHandlers_FireEvent(CMD_EVENT_MQTT_STATE, 1);
		}
		else {
			EventHandlers_FireEvent(CMD_EVENT_MQTT_STATE, 0);
		}
	}
	if (g_newWiFiStatus != g_prevWiFiStatus) {
		g_prevWiFiStatus = g_newWiFiStatus;
		// Argument type here is HALWifiStatus_t enumeration
		EventHandlers_FireEvent(CMD_EVENT_WIFI_STATE, g_newWiFiStatus);
	}
	// Update time with no MQTT
	if (bMQTTconnected) {
		i = 0;
	} else {
		i = g_noMQTTTime + 1;
	}
	// 'i' is a new value
	EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_NOMQTTTIME, g_noMQTTTime, i);
	// save new value
	g_noMQTTTime = i;

	MQTT_Dedup_Tick();
	LED_RunOnEverySecond();
#ifndef OBK_DISABLE_ALL_DRIVERS
	DRV_OnEverySecond();
#if defined(PLATFORM_BEKEN) || defined(WINDOWS) || defined(PLATFORM_BL602) || defined(PLATFORM_ESPIDF)
	UART_RunEverySecond();
#endif
#endif

#if WINDOWS
#elif PLATFORM_BL602
#elif PLATFORM_W600 || PLATFORM_W800
#elif PLATFORM_XR809
#elif PLATFORM_BK7231N || PLATFORM_BK7231T
	if (ota_progress() == -1)
#endif
	{
		CFG_Save_IfThereArePendingChanges();
	}

	// On Beken, do reboot if we ran into heap size problem
#if PLATFORM_BEKEN
	if (xPortGetFreeHeapSize() < 25 * 1000) {
		g_secondsSpentInLowMemoryWarning++;
		ADDLOGF_ERROR("Low heap warning!\n");
		if (g_secondsSpentInLowMemoryWarning > 5) {
			HAL_RebootModule();
		}
	}
	else {
		g_secondsSpentInLowMemoryWarning = 0;
	}
#endif
	if (bSafeMode == 0) {
		const char* ip = HAL_GetMyIPString();
		// this will return non-zero if there were any changes
		if (strcpy_safe_checkForChanges(g_currentIPString, ip, sizeof(g_currentIPString))) {
			if (MQTT_IsReady()) {
				MQTT_DoItemPublish(PUBLISHITEM_SELF_IP);
			}
			EventHandlers_FireEvent(CMD_EVENT_IPCHANGE, 0);

			//Invoke Hass discovery if ipaddr changed
			if (CFG_HasFlag(OBK_FLAG_AUTOMAIC_HASS_DISCOVERY)) {
				Main_ScheduleHomeAssistantDiscovery(1);
			}
		}
	}

	// some users say that despite our simple reconnect mechanism
	// there are some rare cases when devices stuck outside network
	// That is why we can also reconnect them by basing on ping
	if (g_timeSinceLastPingReply != -1 && g_secondsElapsed > 60)
	{
		// cast event so users can script anything easily, run custom commands
		// Usage: addChangeHandler noPingTime > 600 reboot
		g_timeSinceLastPingReply++;
		EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_NOPINGTIME, g_prevTimeSinceLastPingReply, g_timeSinceLastPingReply);
		g_prevTimeSinceLastPingReply = g_timeSinceLastPingReply;
		// this is an old mechanism that just tries to reconnect (but without reboot)
		if (CFG_GetPingDisconnectedSecondsToRestart() > 0 && g_timeSinceLastPingReply >= CFG_GetPingDisconnectedSecondsToRestart())
		{
			if (g_bHasWiFiConnected != 0)
			{
				ADDLOGF_INFO("[Ping watchdog] No ping replies within %i seconds. Will try to reconnect.\n", g_timeSinceLastPingReply);
				HAL_DisconnectFromWifi();
				g_bHasWiFiConnected = 0;
				g_connectToWiFi = 10;
				g_timeSinceLastPingReply = -1;
			}
		}
	}

	if (bSafeMode == 0)
	{

		for (i = 0; i < PLATFORM_GPIO_MAX; i++)
		{
			if (g_cfg.pins.roles[i] == IOR_ADC)
			{
				int value;

				value = HAL_ADC_Read(i);

				//	ADDLOGF_INFO("ADC %i=%i\r\n", i,value);
				CHANNEL_Set(g_cfg.pins.channels[i], value, CHANNEL_SET_FLAG_SILENT);
			}
		}
	}

	// allow for up to 4 scheduled driver starts.
	for (i = 0; i < 4; i++) {
		if (scheduledDelay[i] > 0) {
			scheduledDelay[i]--;
			if (scheduledDelay[i] <= 0)
			{
				scheduledDelay[i] = -1;
#ifndef OBK_DISABLE_ALL_DRIVERS
				DRV_StopDriver(scheduledDriverName[i]);
				DRV_StartDriver(scheduledDriverName[i]);
#endif
				scheduledDriverName[i][0] = 0;
			}
		}
	}

	g_secondsElapsed++;
	if (bSafeMode) {
		safe = "[SAFE] ";
	}
	else {
		safe = "";
	}

	{
		//int mqtt_max, mqtt_cur, mqtt_mem;
		//MQTT_GetStats(&mqtt_cur, &mqtt_max, &mqtt_mem);
		//ADDLOGF_INFO("mqtt req %i/%i, free mem %i\n", mqtt_cur,mqtt_max,mqtt_mem);
		ADDLOGF_INFO("%sTime %i, idle %i/s, free %d, MQTT %i(%i), bWifi %i, secondsWithNoPing %i, socks %i/%i %s\n",
			safe, g_secondsElapsed, idleCount, xPortGetFreeHeapSize(), bMQTTconnected, MQTT_GetConnectEvents(),
			g_bHasWiFiConnected, g_timeSinceLastPingReply, LWIP_GetActiveSockets(), LWIP_GetMaxSockets(),
			g_powersave ? "POWERSAVE" : "");
		// reset so it's a per-second counter.
		idleCount = 0;
	}

#ifdef OBK_MCU_SLEEP_METRICS_ENABLE
	if (g_powersave && CFG_HasLoggerFlag(LOGGER_FLAG_POWER_SAVE)) {
		Main_LogPowerSave();
	}
#endif


	// print network info
	if (!(g_secondsElapsed % 10))
	{
		HAL_PrintNetworkInfo();

	}
	// IR TESTING ONLY!!!!
#ifdef PLATFORM_BK7231T
	//DRV_IR_Print();
#endif

	// when we hit 30s, mark as boot complete.
	if (g_bBootMarkedOK == false)
	{
		int bootCompleteSeconds = CFG_GetBootOkSeconds();
		if (g_secondsElapsed > bootCompleteSeconds)
		{
			ADDLOGF_INFO("Boot complete time reached (%i seconds)\n", bootCompleteSeconds);
			HAL_FlashVars_SaveBootComplete();
			//g_bootFailures = HAL_FlashVars_GetBootFailures();
			g_bBootMarkedOK = true;
		}
	}

	if (g_doHomeAssistantDiscoveryIn) {
		if (MQTT_IsReady()) {
			g_doHomeAssistantDiscoveryIn--;
			if (g_doHomeAssistantDiscoveryIn == 0) {
				ADDLOGF_INFO("Will do request HA discovery now.\n");
				doHomeAssistantDiscovery(0, 0);
			}
			else {
				ADDLOGF_INFO("Will scheduled HA discovery in %i seconds\n", g_doHomeAssistantDiscoveryIn);
			}
		}
		else {
			ADDLOGF_INFO("HA discovery is scheduled, but MQTT connection is not present yet\n");
		}
	}
	if (g_openAP)
	{
		if (g_bHasWiFiConnected)
		{
			HAL_DisconnectFromWifi();
			g_bHasWiFiConnected = 0;
		}
		g_openAP--;
		if (0 == g_openAP)
		{
			HAL_SetupWiFiOpenAccessPoint(CFG_GetDeviceName());
			g_bOpenAccessPointMode = 1;
		}
	}

	//ADDLOGF_INFO("g_startPingWatchDogAfter %i, g_bPingWatchDogStarted %i ", g_startPingWatchDogAfter, g_bPingWatchDogStarted);
	if (g_bHasWiFiConnected) {
		if (g_startPingWatchDogAfter) {
			//ADDLOGF_INFO("g_startPingWatchDogAfter %i", g_startPingWatchDogAfter);
			g_startPingWatchDogAfter--;
			if (0 == g_startPingWatchDogAfter)
			{
				const char* pingTargetServer;

				g_bPingWatchDogStarted = 1;

				pingTargetServer = CFG_GetPingHost();

				if ((pingTargetServer != NULL) && (strlen(pingTargetServer) > 0))
				{
					// mark as enabled
					g_timeSinceLastPingReply = 0;
					//Main_SetupPingWatchDog(pingTargetServer,pingInterval);
					Main_SetupPingWatchDog(pingTargetServer);
				}
				else {
					// mark as disabled
					g_timeSinceLastPingReply = -1;
				}
			}
		}

	}
	if (g_connectToWiFi)
	{
		g_connectToWiFi--;
		if (0 == g_connectToWiFi && g_bHasWiFiConnected == 0)
		{
			Main_ConnectToWiFiNow();
		}
	}

	// config save moved here because of stack size problems
	if (g_saveCfgAfter) {
		g_saveCfgAfter--;
		if (!g_saveCfgAfter) {
			CFG_Save_IfThereArePendingChanges();
		}
	}
	if (g_doUnsafeInitIn) {
		g_doUnsafeInitIn--;
		if (!g_doUnsafeInitIn) {
			ADDLOGF_INFO("Going to call Main_ForceUnsafeInit\r\n");
			Main_ForceUnsafeInit();
		}
	}
	if (g_reset) {
		g_reset--;
		if (!g_reset) {
			// ensure any config changes are saved before reboot.
			CFG_Save_IfThereArePendingChanges();
#ifdef ENABLE_DRIVER_BL0937
			if (DRV_IsMeasuringPower())
			{
				BL09XX_SaveEmeteringStatistics();
			}
#endif            
			ADDLOGF_INFO("Going to call HAL_RebootModule\r\n");
			HAL_RebootModule();
		}
		else {

			ADDLOGF_INFO("Module reboot in %i...\r\n", g_reset);
		}
	}

#if ENABLE_DRIVER_DHT
	if (g_dhtsCount > 0) {
		if (bSafeMode == 0) {
			DHT_OnEverySecond();
		}
	}
#endif
#ifdef PLATFORM_BEKEN
	bk_wdg_reload();
#elif PLATFORM_BL602
	bl_wdt_feed();
#elif PLATFORM_W600 || PLATFORM_W800
	tls_watchdog_clr();
#elif PLATFORM_LN882H
	hal_wdt_cnt_restart(WDT_BASE);
#endif
	// force it to sleep...  we MUST have some idle task processing
	// else task memory doesn't get freed
	rtos_delay_milliseconds(1);

}


//////////////////////////////////////////////////////
// Quick tick

#define WIFI_LED_FAST_BLINK_DURATION 250
#define WIFI_LED_SLOW_BLINK_DURATION 500



static int g_wifiLedToggleTime = 0;
static int g_wifi_ledState = 0;
static uint32_t g_time = 0;
static uint32_t g_last_time = 0;
int g_bWantPinDeepSleep;
int g_pinDeepSleepWakeUp = 0;
unsigned int g_deltaTimeMS;

/////////////////////////////////////////////////////
// this is what we do in a qucik tick
void QuickTick(void* param)
{
	if (g_bWantPinDeepSleep) {
		g_bWantPinDeepSleep = 0;
		PINS_BeginDeepSleepWithPinWakeUp(g_pinDeepSleepWakeUp);
		return;
	}

#if defined(PLATFORM_BEKEN) && defined(BEKEN_PIN_GPI_INTERRUPTS)
	// if using interrupt driven GPI for pins, don't call PIN_ticks() in QuickTick
#else
	PIN_ticks(param);
#endif

#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
	g_time = rtos_get_time();
#elif defined (PLATFORM_ESPIDF)
	g_time = esp_timer_get_time() / 1000;
#else
	g_time += QUICK_TMR_DURATION;
#endif
	g_deltaTimeMS = g_time - g_last_time;
	// cope with wrap
	if (g_deltaTimeMS > 0x4000) {
		g_deltaTimeMS = ((g_time + 0x4000) - (g_last_time + 0x4000));
	}
	g_last_time = g_time;


#if (defined WINDOWS) || (defined PLATFORM_BEKEN) || (defined PLATFORM_BL602) || (defined PLATFORM_LN882H) || (defined PLATFORM_ESPIDF)
	SVM_RunThreads(g_deltaTimeMS);
#endif
	RepeatingEvents_RunUpdate(g_deltaTimeMS * 0.001f);
#ifndef OBK_DISABLE_ALL_DRIVERS
	DRV_RunQuickTick();
#endif
#ifdef WINDOWS
	NewTuyaMCUSimulator_RunQuickTick(g_deltaTimeMS);
#endif
	CMD_RunUartCmndIfRequired();

	// process recieved messages here..
	MQTT_RunQuickTick();

	if (CFG_HasFlag(OBK_FLAG_LED_SMOOTH_TRANSITIONS) == true) {
		LED_RunQuickColorLerp(g_deltaTimeMS);
	}

	// WiFi LED
	// In Open Access point mode, fast blink
	if (Main_IsOpenAccessPointMode()) {
		g_wifiLedToggleTime += g_deltaTimeMS;
		if (g_wifiLedToggleTime > WIFI_LED_FAST_BLINK_DURATION) {
			g_wifi_ledState = !g_wifi_ledState;
			g_wifiLedToggleTime = 0;
			PIN_set_wifi_led(g_wifi_ledState);
		}
	}
	else if (Main_IsConnectedToWiFi()) {
		// In WiFi client success mode, just stay enabled
		PIN_set_wifi_led(1);
	}
	else {
		// in connecting mode, slow blink
		g_wifiLedToggleTime += g_deltaTimeMS;
		if (g_wifiLedToggleTime > WIFI_LED_SLOW_BLINK_DURATION) {
			g_wifi_ledState = !g_wifi_ledState;
			g_wifiLedToggleTime = 0;
			PIN_set_wifi_led(g_wifi_ledState);
		}
	}

}



////////////////////////////////////////////////////////
// this is the bit which runs the quick tick timer
#if WINDOWS

#elif PLATFORM_BL602 || PLATFORM_W600 || PLATFORM_W800
void quick_timer_thread(void* param)
{
	while (1) {
		vTaskDelay(QUICK_TMR_DURATION);
		QuickTick(0);
	}
}
#elif PLATFORM_ESPIDF
esp_timer_handle_t g_quick_timer;
#elif PLATFORM_XR809 || PLATFORM_LN882H
OS_Timer_t g_quick_timer;
#else
beken_timer_t g_quick_timer;
#endif
void QuickTick_StartThread(void)
{
#if WINDOWS

#elif PLATFORM_BL602 || PLATFORM_W600 || PLATFORM_W800

	xTaskCreate(quick_timer_thread, "quick", 1024, NULL, 15, NULL);
#elif PLATFORM_ESPIDF
	const esp_timer_create_args_t g_quick_timer_args =
	{
			.callback = &QuickTick,
			.name = "quick"
	};

	esp_timer_create(&g_quick_timer_args, &g_quick_timer);
	esp_timer_start_periodic(g_quick_timer, QUICK_TMR_DURATION * 1000);
#elif PLATFORM_XR809 || PLATFORM_LN882H

	OS_TimerSetInvalid(&g_quick_timer);
	if (OS_TimerCreate(&g_quick_timer, OS_TIMER_PERIODIC, QuickTick, NULL,
		QUICK_TMR_DURATION) != OS_OK) {
		printf("Quick timer create failed\n");
		return;
	}

	OS_TimerStart(&g_quick_timer); /* start OS timer to feed watchdog */
#else
	OSStatus result;

	result = rtos_init_timer(&g_quick_timer,
		QUICK_TMR_DURATION,
		QuickTick,
		(void*)0);
	ASSERT(kNoErr == result);

	result = rtos_start_timer(&g_quick_timer);
	ASSERT(kNoErr == result);
#endif
}
///////////////////////////////////////////////////////


void app_on_generic_dbl_click(int btnIndex)
{
	if (g_secondsElapsed < 5)
	{
		CFG_SetOpenAccessPoint();
	}
}


int Main_IsOpenAccessPointMode()
{
	return g_bOpenAccessPointMode;
}

int Main_IsConnectedToWiFi()
{
	return g_bHasWiFiConnected;
}


// called from idle thread each loop.
// - just so we know it is running.
void isidle() {
	idleCount++;
}

bool g_unsafeInitDone = false;

void Main_Init_AfterDelay_Unsafe(bool bStartAutoRunScripts) {

	// initialise MQTT - just sets up variables.
	// all MQTT happens in timer thread?
	MQTT_init();

	CMD_Init_Delayed();

	if (bStartAutoRunScripts) {
		if (PIN_FindPinIndexForRole(IOR_IRRecv, -1) != -1 || PIN_FindPinIndexForRole(IOR_IRSend, -1) != -1) {
			// start IR driver 5 seconds after boot.  It may affect wifi connect?
			// yet we also want it to start if no wifi for IR control...
#ifndef OBK_DISABLE_ALL_DRIVERS
			DRV_StartDriver("IR");
			//ScheduleDriverStart("IR",5);
#endif
		}

		// NOTE: this will try to read autoexec.bat,
		// so ALL commands expected in autoexec.bat should have been registered by now...
		CMD_ExecuteCommand(CFG_GetShortStartupCommand(), COMMAND_FLAG_SOURCE_SCRIPT);
		CMD_ExecuteCommand("startScript autoexec.bat", COMMAND_FLAG_SOURCE_SCRIPT);
	}
#ifdef PLATFORM_BEKEN
	bk_wdg_initialize(10000);
#elif PLATFORM_BL602
	// max is 4 seconds or so...
	// #define MAX_MS_WDT (65535/16)
	bl_wdt_init(3000);
#elif PLATFORM_W600 || PLATFORM_W800
	tls_watchdog_init(5*1000*1000);
#elif PLATFORM_LN882H
	/* Watchdog initialization */
	wdt_init_t_def wdt_init;
	memset(&wdt_init,0,sizeof(wdt_init));
	wdt_init.wdt_rmod = WDT_RMOD_1;         // When equal to 0, the counter is reset directly when it overflows; when equal to 1, an interrupt is generated first when the counter overflows, and if it overflows again, it resets.
	wdt_init.wdt_rpl = WDT_RPL_32_PCLK;     // Set the reset delay time
	wdt_init.top = WDT_TOP_VALUE_9;         //wdt cnt value = 0x1FFFF   Time = 4.095 s
	hal_wdt_init(WDT_BASE, &wdt_init);
    
	/* Configure watchdog interrupt */
	NVIC_SetPriority(WDT_IRQn,     4);
	NVIC_EnableIRQ(WDT_IRQn);
    
	/* Enable watchdog */
	hal_wdt_en(WDT_BASE,HAL_ENABLE);
#endif
}
void Main_Init_BeforeDelay_Unsafe(bool bAutoRunScripts) {
	g_unsafeInitDone = true;
#ifndef OBK_DISABLE_ALL_DRIVERS
	DRV_Generic_Init();
#endif
#ifdef PLATFORM_BEKEN
	int bk_misc_get_start_type();
	g_rebootReason = bk_misc_get_start_type();
#endif

	RepeatingEvents_Init();

	// set initial values for channels.
	// this is done early so lights come on at the flick of a switch.
	CFG_ApplyChannelStartValues();
	PIN_AddCommands();
	ADDLOGF_DEBUG("Initialised pins\r\n");

#if ENABLE_LITTLEFS
	// initialise the filesystem, only if present.
	// don't create if it does not mount
	// do this for ST mode only, as it may be something in FS which is killing us,
	// and we may add a command to empty fs just be writing first sector?
	init_lfs(0);
#endif

	PIN_SetGenericDoubleClickCallback(app_on_generic_dbl_click);
	ADDLOGF_DEBUG("Initialised other callbacks\r\n");

	// initialise rest interface
	init_rest();

	// add some commands...
	taslike_commands_init();
#if ENABLE_TEST_COMMANDS
	CMD_InitTestCommands();
#endif
	NewLED_InitCommands();
#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
	CMD_InitSendCommands();
#endif
	CMD_InitChannelCommands();
	EventHandlers_Init();

	// CMD_Init() is now split into Early and Delayed
	// so ALL commands expected in autoexec.bat should have been registered by now...
	// but DON't run autoexec if we have had 2+ boot failures
	CMD_Init_Early();
#if WINDOWS
	CMD_InitSimulatorOnlyCommands();
#endif

	/* Automatic disable of PIN MONITOR after reboot */
	if (CFG_HasFlag(OBK_FLAG_HTTP_PINMONITOR)) {
		CFG_SetFlag(OBK_FLAG_HTTP_PINMONITOR, false);
	}

	if (bAutoRunScripts) {
		CMD_ExecuteCommand("exec early.bat", COMMAND_FLAG_SOURCE_SCRIPT);
		if (!CFG_HasFlag(OBK_FLAG_DRV_DISABLE_AUTOSTART)) {
			// autostart drivers
			if (PIN_FindPinIndexForRole(IOR_SM2135_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_SM2135_DAT, -1) != -1)
			{
#ifndef OBK_DISABLE_ALL_DRIVERS
				DRV_StartDriver("SM2135");
#endif
			}
			if (PIN_FindPinIndexForRole(IOR_SM2235_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_SM2235_DAT, -1) != -1)
			{
#ifndef OBK_DISABLE_ALL_DRIVERS
				DRV_StartDriver("SM2235");
#endif
			}
			if (PIN_FindPinIndexForRole(IOR_BP5758D_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_BP5758D_DAT, -1) != -1)
			{
#ifndef OBK_DISABLE_ALL_DRIVERS
				DRV_StartDriver("BP5758D");
#endif
			}
			if (PIN_FindPinIndexForRole(IOR_BP1658CJ_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_BP1658CJ_DAT, -1) != -1) {
#ifndef OBK_DISABLE_ALL_DRIVERS
				DRV_StartDriver("BP1658CJ");
#endif
			}
			if (PIN_FindPinIndexForRole(IOR_KP18058_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_KP18058_DAT, -1) != -1) {
#ifndef OBK_DISABLE_ALL_DRIVERS
				DRV_StartDriver("KP18058");
#endif
			}
			if (PIN_FindPinIndexForRole(IOR_BL0937_CF, -1) != -1 && PIN_FindPinIndexForRole(IOR_BL0937_CF1, -1) != -1
				&& (PIN_FindPinIndexForRole(IOR_BL0937_SEL, -1) != -1 || PIN_FindPinIndexForRole(IOR_BL0937_SEL_n, -1) != -1)) {
#ifndef OBK_DISABLE_ALL_DRIVERS
				DRV_StartDriver("BL0937");
#endif
			}
			if ((PIN_FindPinIndexForRole(IOR_BridgeForward, -1) != -1) && (PIN_FindPinIndexForRole(IOR_BridgeReverse, -1) != -1))
			{
#ifndef OBK_DISABLE_ALL_DRIVERS
				DRV_StartDriver("Bridge");
#endif
			}
			if ((PIN_FindPinIndexForRole(IOR_DoorSensorWithDeepSleep, -1) != -1) ||
				(PIN_FindPinIndexForRole(IOR_DoorSensorWithDeepSleep_NoPup, -1) != -1) ||
				(PIN_FindPinIndexForRole(IOR_DoorSensorWithDeepSleep_pd, -1) != -1))
			{
#ifndef OBK_DISABLE_ALL_DRIVERS
				DRV_StartDriver("DoorSensor");
#endif
			}
			if (PIN_FindPinIndexForRole(IOR_CHT83XX_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_CHT83XX_DAT, -1) != -1) {
#ifndef OBK_DISABLE_ALL_DRIVERS
				DRV_StartDriver("CHT83XX");
#endif
			}
			if (PIN_FindPinIndexForRole(IOR_SHT3X_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_SHT3X_DAT, -1) != -1) {
#ifndef OBK_DISABLE_ALL_DRIVERS
				DRV_StartDriver("SHT3X");
#endif
			}
			if (PIN_FindPinIndexForRole(IOR_SGP_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_SGP_DAT, -1) != -1) {
#ifndef OBK_DISABLE_ALL_DRIVERS
				DRV_StartDriver("SGP");
#endif
			}
			if (PIN_FindPinIndexForRole(IOR_BAT_ADC, -1) != -1) {
#ifndef OBK_DISABLE_ALL_DRIVERS
				DRV_StartDriver("Battery");
#endif
			}
			if (PIN_FindPinIndexForRole(IOR_TM1637_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_TM1637_DIO, -1) != -1) {
#ifndef OBK_DISABLE_ALL_DRIVERS
				DRV_StartDriver("TM1637");
#endif
			}
			if ((PIN_FindPinIndexForRole(IOR_GN6932_CLK, -1) != -1) &&
				(PIN_FindPinIndexForRole(IOR_GN6932_DAT, -1) != -1) &&
				(PIN_FindPinIndexForRole(IOR_GN6932_STB, -1) != -1))
			{
#ifndef OBK_DISABLE_ALL_DRIVERS
				DRV_StartDriver("GN6932");
#endif
			}
//			if ((PIN_FindPinIndexForRole(IOR_TM1638_CLK, -1) != -1) &&
//				(PIN_FindPinIndexForRole(IOR_TM1638_DAT, -1) != -1) &&
//				(PIN_FindPinIndexForRole(IOR_TM1638_STB, -1) != -1))
//			{
//#ifndef OBK_DISABLE_ALL_DRIVERS
//				DRV_StartDriver("TM1638");
//#endif
//			}
		}
	}

	g_enable_pins = 1;
	// this actually sets the pins, moved out so we could avoid if necessary
	PIN_SetupPins();
	QuickTick_StartThread();

	NewLED_RestoreSavedStateIfNeeded();
}
void Main_ForceUnsafeInit() {
	if (g_unsafeInitDone) {
		ADDLOGF_INFO("It was already done.\r\n");
		return;
	}
	Main_Init_BeforeDelay_Unsafe(false);
	Main_Init_AfterDelay_Unsafe(false);
	bSafeMode = 0;
}
//////////////////////////////////////////////////////
// do things which should happen BEFORE we delay at Startup
// e.g. set lights to last value, so we get immediate response at
// power on.
void Main_Init_Before_Delay()
{
	ADDLOGF_INFO("Main_Init_Before_Delay");
	// read or initialise the boot count flash area
	HAL_FlashVars_IncreaseBootCount();

#ifdef PLATFORM_BEKEN
	// this just increments our idle counter variable.
	// it registers a cllback from RTOS IDLE function.
	// why is it called IRDA??  is this where they check for IR?
	bg_register_irda_check_func(isidle);
#endif

	g_bootFailures = HAL_FlashVars_GetBootFailures();
	if (g_bootFailures > RESTARTS_REQUIRED_FOR_SAFE_MODE)
	{
		bSafeMode = 1;
		ADDLOGF_INFO("###### safe mode activated - boot failures %d", g_bootFailures);
	}
	CFG_InitAndLoad();

#if ENABLE_LITTLEFS
	LFSAddCmds();
#endif

	// only initialise certain things if we are not in AP mode
	if (!bSafeMode)
	{
		Main_Init_BeforeDelay_Unsafe(true);
	}

	ADDLOGF_INFO("Main_Init_Before_Delay done");
	bk_printf("\r\nMain_Init_Before_Delay done\r\n");
}

// a fixed delay of 750ms to wait for calibration routines in core thread,
// which must complete before LWIP etc. are intialised,
// and crucially before we use certain TCP/IP features.
// it would be nicer to understand more about why, and to wait for TCPIP to be ready
// rather than use a fixed delay.
// (e.g. are we delayed by it reading temperature?)
void Main_Init_Delay()
{
	ADDLOGF_INFO("Main_Init_Delay");
	bk_printf("\r\nMain_Init_Delay\r\n");

	extended_app_waiting_for_launch2();

	ADDLOGF_INFO("Main_Init_Delay done");
	bk_printf("\r\nMain_Init_Delay done\r\n");

	// use this variable wherever to determine if we have TCP/IP features.
	// e.g. in logging to determine if we can start TCP thread
	g_StartupDelayOver = 1;
}


// do things after the start delay.
// i.e. things that use TCP/IP like NTP, SSDP, DGR
void Main_Init_After_Delay()
{
	const char* wifi_ssid, * wifi_pass;
	ADDLOGF_INFO("Main_Init_After_Delay");

	// we can log this after delay.
	if (bSafeMode) {
		ADDLOGF_INFO("###### safe mode activated - boot failures %d", g_bootFailures);
	}

	wifi_ssid = CFG_GetWiFiSSID();
	wifi_pass = CFG_GetWiFiPass();

#if 0
	// you can use this if you bricked your module by setting wrong access point data
	wifi_ssid = "qqqqqqqqqq";
	wifi_pass = "Fqqqqqqqqqqqqqqqqqqqqqqqqqqq"
#endif
#ifdef SPECIAL_UNBRICK_ALWAYS_OPEN_AP
		// you can use this if you bricked your module by setting wrong access point data
		bForceOpenAP = 1;
#endif

	if ((*wifi_ssid == 0))
	{
		// start AP mode in 5 seconds
		g_openAP = 5;
		//HAL_SetupWiFiOpenAccessPoint();
	}
	else {
		if (bSafeMode)
		{
			g_openAP = 5;
		}
		else {
			if (Main_HasFastConnect()) {
				mqtt_loopsWithDisconnected = 9999;
				Main_ConnectToWiFiNow();
			}
			else {
				g_connectToWiFi = 5;
			}
		}
	}

	ADDLOGF_INFO("Using SSID [%s]\r\n", wifi_ssid);
	ADDLOGF_INFO("Using Pass [%s]\r\n", wifi_pass);

	// NOT WORKING, I done it other way, see ethernetif.c
	//net_dhcp_hostname_set(g_shortDeviceName);

	HTTPServer_Start();
	ADDLOGF_DEBUG("Started http tcp server\r\n");

	// only initialise certain things if we are not in AP mode
	if (!bSafeMode)
	{
		//Always invoke discovery on startup. This accounts for change in ipaddr before startup and firmware update.
		if (CFG_HasFlag(OBK_FLAG_AUTOMAIC_HASS_DISCOVERY)) {
			Main_ScheduleHomeAssistantDiscovery(1);
		}

		Main_Init_AfterDelay_Unsafe(true);
	}

	ADDLOGF_INFO("Main_Init_After_Delay done");
}



void Main_Init()
{
	g_unsafeInitDone = false;

#ifdef WINDOWS
	// on windows, Main_Init may happen multiple time so we need to reset variables
	LED_ResetGlobalVariablesToDefaults();
	// on windows, we don't want to remember commands from previous session
	CMD_FreeAllCommands();
#endif

	// do things we want to happen immediately on boot
	Main_Init_Before_Delay();
	// delay until TCP/IP stack is ready
	Main_Init_Delay();
	// do things we want after TCP/IP stack is ready
	Main_Init_After_Delay();

}


