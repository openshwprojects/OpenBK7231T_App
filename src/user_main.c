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
#include "driver/drv_bl_shared.h"
#include "driver/drv_hlw8112.h"
//#include "ir/ir_local.h"

#include "driver/drv_deviceclock.h"

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
#include "hal/hal_ota.h"

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
#include "BkDriverWdg.h"

void bg_register_irda_check_func(FUNCPTR func);
extern void WFI(void);
#elif PLATFORM_BL602
#include <bl_sys.h>
#include <hosal_adc.h>
#include <bl_wdt.h>
#elif PLATFORM_W600 || PLATFORM_W800
#include "wm_watchdog.h"
#elif PLATFORM_LN882H
#include "hal/hal_wdt.h"
#include "hal/hal_gpio.h"
#elif PLATFORM_ESPIDF
#include "esp_timer.h"
#elif PLATFORM_ECR6600
#include "hal_adc.h"
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
char g_wifi_bssid[33] = { "30:B5:C2:5D:70:72" };
uint8_t g_wifi_channel = 12;
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

#if PLATFORM_BL602 || PLATFORM_W600 || PLATFORM_W800
#define DEF_USE_WFI 1
#else
#define DEF_USE_WFI 0
#endif
#if PLATFORM_BEKEN
#define WFI_FUNC WFI
#elif PLATFORM_BL602 || PLATFORM_REALTEK || PLATFORM_XRADIO || PLATFORM_W600 || PLATFORM_RDA5981 || PLATFORM_LN8825 || PLATFORM_LN882H
#define WFI_FUNC() __asm volatile("wfi")
#elif PLATFORM_W800
#define WFI_FUNC __WFI
#endif
bool g_use_wfi = DEF_USE_WFI;


// TEMPORARY
int ota_status = -1;
int total_bytes = 0;

int OTA_GetProgress()
{
	return ota_status;
}

void OTA_ResetProgress()
{
	ota_status = -1;
}

void OTA_IncrementProgress(int value)
{
	ota_status += value;
}

int OTA_GetTotalBytes()
{
	return total_bytes;
}

void OTA_SetTotalBytes(int value)
{
	total_bytes = value;
}




#if PLATFORM_XR806 || PLATFORM_XR872

size_t xPortGetFreeHeapSize()
{
	return sram_free_heap_size();
}

#elif PLATFORM_RDA5981

#include "hal/api/mbed_stats.h"
extern uint32_t mbed_heap_size;
size_t xPortGetFreeHeapSize()
{
	mbed_stats_heap_t heap_stats;
	mbed_stats_heap_get(&heap_stats);
	//ADDLOGF_DEBUG("mbed_heap_size: %i\n heap_stats.current_size: %i\nheap_stats.max_size: %i\nheap_stats.total_size: %i\nheap_stats.alloc_cnt: %i\nheap_stats.alloc_fail_cnt: %i\n",
	//	mbed_heap_size, heap_stats.current_size, heap_stats.max_size, heap_stats.total_size, heap_stats.alloc_cnt, heap_stats.alloc_fail_cnt);
	return mbed_heap_size - heap_stats.current_size;
}
int _kill(int pid, int sig)
{
	errno = EINVAL;
	return -1;
}

pid_t _getpid()
{
	return 1;
}

#endif

#if PLATFORM_BL602
/// Read the Internal Temperature Sensor as Float. Returns 0 if successful.
/// Based on bl_tsen_adc_get in https://github.com/lupyuen/bl_iot_sdk/blob/master/components/hal_drv/bl602_hal/bl_adc.c#L224-L282
static int get_tsen_adc(
	float *temp,      //  Pointer to float to store the temperature
	uint8_t log_flag  //  0 to disable logging, 1 to enable logging
) {
	
	
	//  Return the temperature
	*temp = hosal_adc_tsen_value_get_f(hosal_adc_device_get());
	return 0;
}
#endif

#if PLATFORM_BEKEN
#if (OBK_VARIANT == OBK_VARIANT_BATTERY)
	#if PLATFORM_BEKEN_NEW
		#define START_MS_DELAY 10;
	#else
		#define START_MS_DELAY 0;
	#endif
#else
	#define START_MS_DELAY 250;
#endif
// this function waits for the extended app functions to finish starting.
extern void extended_app_waiting_for_launch(void);
void extended_app_waiting_for_launch2()
{
	// 3.0.76 'broke' it. It is now called in init_app_thread, which will later call user_main
#ifndef PLATFORM_BEKEN_NEW
	extended_app_waiting_for_launch();
#endif

	// define FIXED_DELAY if delay wanted on non-beken platforms.
#if PLATFORM_BK7231N || PLATFORM_BEKEN_NEW
	// wait 100ms at the start.
	// TCP is being setup in a different thread, and there does not seem to be a way to find out if it's complete yet?
	// so just wait a bit, and then start.
	uint8_t startDelay = START_MS_DELAY;
	bk_printf("\r\ndelaying start\r\n");
	for(uint8_t i = 0; i < startDelay / 10; i++)
	{
		rtos_delay_milliseconds(10);
		bk_printf("#Startup delayed %dms#\r\n", i * 10);
	}
	bk_printf("\r\nstarting....\r\n");

	// through testing, 'Initializing TCP/IP stack' appears at ~500ms
	// so we should wait at least 750?
#endif
}
#else
void extended_app_waiting_for_launch2(void) {
	// do nothing?
}
#endif


#if PLATFORM_ESPIDF || PLATFORM_REALTEK_NEW

int LWIP_GetMaxSockets() {
	return 0;
}
int LWIP_GetActiveSockets() {
	return 0;
}
#endif

#if PLATFORM_BL602 || PLATFORM_W800 || PLATFORM_W600 || PLATFORM_LN882H || PLATFORM_LN8825 \
	|| PLATFORM_ESPIDF || PLATFORM_TR6260 || PLATFORM_REALTEK || PLATFORM_ECR6600 \
	|| PLATFORM_XRADIO || PLATFORM_ESP8266

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
	if(thread == NULL) vTaskDelete(NULL);
	else vTaskDelete(*thread);
	return kNoErr;
}

OSStatus rtos_suspend_thread(beken_thread_t* thread)
{
	if(thread == NULL) vTaskSuspend(NULL);
	else vTaskSuspend(*thread);
	return kNoErr;
}

#elif PLATFORM_TXW81X

OSStatus rtos_create_thread(beken_thread_t* thread,
	uint8_t priority, const char* name,
	beken_thread_function_t function,
	uint32_t stack_size, beken_thread_arg_t arg)
{
	OSStatus err = kNoErr;

	*thread = os_task_create(name, function, arg, priority, 0, NULL, stack_size);
	if(*thread != NULL)
	{
		return 0;
	}
	else
	{
		printf("Thread create %s - err %i\n", name, err);
	}
	return 1;
}

OSStatus rtos_delete_thread(beken_thread_t* thread)
{
	if(thread == NULL)
	{
		k_task_handle_t hdl = os_task_current();
		os_task_destroy(hdl);
	}
	else os_task_destroy(*thread);
	return kNoErr;
}

OSStatus rtos_suspend_thread(beken_thread_t* thread)
{
	if(thread == NULL)
	{
		k_task_handle_t hdl = os_task_current();
		os_task_suspend2(hdl);
	}
	else os_task_suspend2(*thread);
	return kNoErr;
}

#elif PLATFORM_RDA5981

#include "rt_TypeDef.h"

OSStatus rtos_create_thread(beken_thread_t thread,
	uint8_t priority, const char* name,
	beken_thread_function_t function,
	uint32_t stack_size, beken_thread_arg_t arg)
{
	osThreadDef_t def;
	osThreadId     id;

	def.pthread = (os_pthread)function;
	def.tpriority = osPriorityNormal;
	def.stacksize = stack_size;
	def.stack_pointer = malloc(stack_size);
	if(def.stack_pointer == NULL)
	{
		printf("Error allocating the stack memory");
		return 1;
	}
	thread = osThreadCreate(&def, arg);
	if(thread == NULL)
	{
		free(def.stack_pointer);
		printf("Thread create %s - err\n", name);
		return 1;
	}
	//printf("Thread stack at 0x%lx %lu\n", (uint32_t)def.stack_pointer, stack_size);
	return 0;
}

OSStatus rtos_delete_thread(beken_thread_t thread)
{
	if(thread == NULL)
	{
		thread = osThreadGetId();
	}
	P_TCB tcb = rt_tid2ptcb(thread);
	uint32_t* stk = tcb->stack;
	if(stk == NULL) printf("rtos_delete_thread stk is null\n");
	//printf("Freeing stack at 0x%lx %u\n", (uint32_t)stk, tcb->priv_stack);
	free(stk);
	osThreadTerminate(thread);
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

#if defined(PLATFORM_LN882H) || PLATFORM_LN8825
// LN882H hack, maybe place somewhere else?
// this will be applied after WiFi connect
extern int g_ln882h_pendingPowerSaveCommand;
void LN882H_ApplyPowerSave(int bOn);
#endif

// SSID switcher by xjikka 20240525
#if ALLOW_SSID2
#define SSID_USE_SSID1  0
#define SSID_USE_SSID2  1
static int g_SSIDactual = SSID_USE_SSID1;       // -1 not initialized,  0=SSID1 1=SSID2
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
	if(CFG_HasFlag(OBK_FLAG_WIFI_ENHANCED_FAST_CONNECT)) HAL_DisableEnhancedFastConnect();
#endif
}

//20241125 XJIKKA Init last stored SSID from RetailChannel if set
//Note that it must be set in early.bat using CMD_setStartupSSIDChannel
void Init_WiFiSSIDactual_FromChannelIfSet(void) {
#if ALLOW_SSID2
	g_SSIDactual = FV_GetStartupSSID_StoredValue(SSID_USE_SSID1);
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
		ADDLOGF_INFO("%s - WIFI_STA_CONNECTING - %i\r\n", __func__, code);
		break;
	case WIFI_STA_DISCONNECTED:
		// try to connect again in few seconds
		// if we are already disconnected, why must we call disconnect again?
#if PLATFORM_BEKEN
		if (g_bHasWiFiConnected != 0)
		{
			HAL_DisconnectFromWifi();
		}
#endif
		if(g_secondsElapsed < 30)
		{
			g_connectToWiFi = 5;
		}
		else
		{
			g_connectToWiFi = 15;
		}
		g_bHasWiFiConnected = 0;
		g_timeSinceLastPingReply = -1;
		ADDLOGF_INFO("%s - WIFI_STA_DISCONNECTED - %i\r\n", __func__, code);
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
		ADDLOGF_INFO("%s - WIFI_STA_AUTH_FAILED - %i\r\n", __func__, code);
		break;
	case WIFI_STA_CONNECTED:
#if ALLOW_SSID2
		if (!g_bHasWiFiConnected) FV_UpdateStartupSSIDIfChanged_StoredValue(g_SSIDactual);	//update ony on first connect
#endif		

		g_bHasWiFiConnected = 1;
		ADDLOGF_INFO("%s - WIFI_STA_CONNECTED - %i\r\n", __func__, code);

#if ALLOW_SSID2
		g_SSIDSwitchCnt = 0;
#endif

		if (bSafeMode == 0) {
			HAL_GetWiFiBSSID(g_wifi_bssid);
			HAL_GetWiFiChannel(&g_wifi_channel);


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
#if defined(PLATFORM_LN882H) || PLATFORM_LN8825
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
		ADDLOGF_INFO("%s - WIFI_AP_CONNECTED - %i\r\n", __func__, code);
		break;
	case WIFI_AP_FAILED:
		g_bHasWiFiConnected = 0;
		ADDLOGF_INFO("%s - WIFI_AP_FAILED - %i\r\n", __func__, code);
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
#if ENABLE_HA_DISCOVERY
void Main_ScheduleHomeAssistantDiscovery(int seconds) {
	g_doHomeAssistantDiscoveryIn = seconds;
}
#endif


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
	if(CFG_HasFlag(OBK_FLAG_WIFI_ENHANCED_FAST_CONNECT))
	{
		HAL_FastConnectToWiFi(wifi_ssid, wifi_pass, &g_cfg.staticIP);
	}
	else
	{
		HAL_ConnectToWiFi(wifi_ssid, wifi_pass, &g_cfg.staticIP);
	}
	// don't set g_connectToWiFi = 0; here!
	// this would overwrite any changes, e.g. from Main_OnWiFiStatusChange !
	// so don't do this here, but e.g. set in Main_OnWiFiStatusChange if connected!!!
}
bool Main_HasFastConnect() {
	if(g_bootFailures > 2)
	{
		HAL_DisableEnhancedFastConnect();
		return false;
	}
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
#if PLATFORM_LN882H || PLATFORM_ESPIDF || PLATFORM_ESP8266 || PLATFORM_LN8825
// Quick hack to display LN-only temperature,
// we may improve it in the future
extern float g_wifi_temperature;
#else
float g_wifi_temperature = 0;
#endif

static byte g_secondsSpentInLowMemoryWarning = 0;
void Main_OnEverySecond()
{
#if PLATFORM_W600 || PLATFORM_W800
#define TimeOut_t xTimeOutType 
#endif
#if ! ( WINDOWS || PLATFORM_TXW81X  || PLATFORM_RDA5981) 
	TimeOut_t myTimeout;	// to get uptime from xTicks - not working on WINDOWS and TXW81X and RDA5981
#endif
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
#if PLATFORM_BEKEN_NEW
	#if PLATFORM_BK7231N
		g_wifi_temperature = (-0.38f * temperature) + 156.0f;
	#elif PLATFORM_BK7238 || PLATFORM_BK7252N
		g_wifi_temperature = (-0.4f * temperature) + 131.0f;
	#else
		g_wifi_temperature = temperature * 0.128f;
	#endif
#elif PLATFORM_BK7231T
		g_wifi_temperature = 2.21f * (temperature / 25.0f) - 65.91f;
#else
		g_wifi_temperature = (-0.457f * temperature) + 188.474f;
#endif
#elif PLATFORM_BL602
		get_tsen_adc(&g_wifi_temperature, 0);
#elif PLATFORM_LN882H
		// this is set externally, I am just leaving comment here
#elif PLATFORM_W800 || PLATFORM_W600
		g_wifi_temperature = HAL_ADC_Temp();
#elif PLATFORM_ECR6600
		g_wifi_temperature = hal_adc_tempsensor();
#endif
	}

#if ENABLE_MQTT
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
#endif
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


#if ENABLE_MQTT
	MQTT_Dedup_Tick();
#endif
#if ENABLE_LED_BASIC
	LED_RunOnEverySecond();
#endif
#ifndef OBK_DISABLE_ALL_DRIVERS
	DRV_OnEverySecond();
#if defined(PLATFORM_BEKEN) || defined(WINDOWS) || defined(PLATFORM_BL602) || defined(PLATFORM_ESPIDF) \
 || defined (PLATFORM_RTL87X0C) || PLATFORM_ESP8266
	UART_RunEverySecond();
#endif
#endif

	if (OTA_GetProgress() == -1)
	{
		CFG_Save_IfThereArePendingChanges();
	}

	// On Beken, do reboot if we ran into heap size problem
#if PLATFORM_BEKEN || PLATFORM_W800
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
#if ENABLE_MQTT
			if (MQTT_IsReady()) {
				MQTT_DoItemPublish(PUBLISHITEM_SELF_IP);
			}
#endif
			EventHandlers_FireEvent(CMD_EVENT_IPCHANGE, 0);
#if ENABLE_HA_DISCOVERY
			//Invoke Hass discovery if ipaddr changed
			if (CFG_HasFlag(OBK_FLAG_AUTOMAIC_HASS_DISCOVERY)) {
				Main_ScheduleHomeAssistantDiscovery(1);
			}
#endif
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
#if (WINDOWS || PLATFORM_TXW81X || PLATFORM_RDA5981)
	g_secondsElapsed++;
#elif defined(PLATFORM_ESPIDF)
	g_secondsElapsed = (int)(esp_timer_get_time() / 1000000);
#else
	vTaskSetTimeOutState( &myTimeout );
	g_secondsElapsed = (int)((((uint64_t) myTimeout.xOverflowCount << (sizeof(portTickType)*8) | myTimeout.xTimeOnEntering)*portTICK_RATE_MS ) / 1000 );
#endif
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
#if ENABLE_MQTT
		ADDLOGF_INFO("%sTime %i, idle %i/s, free %d, MQTT %i(%i), bWifi %i, secondsWithNoPing %i, socks %i/%i %s\n",
			safe, g_secondsElapsed, idleCount, xPortGetFreeHeapSize(), bMQTTconnected,
			MQTT_GetConnectEvents(),g_bHasWiFiConnected, g_timeSinceLastPingReply, LWIP_GetActiveSockets(), LWIP_GetMaxSockets(),
			g_powersave ? "POWERSAVE" : "");
#else
		ADDLOGF_INFO("%sTime %i, idle %i/s, free %d,  bWifi %i, secondsWithNoPing %i, socks %i/%i %s\n",
			safe, g_secondsElapsed, idleCount, xPortGetFreeHeapSize(),g_bHasWiFiConnected, g_timeSinceLastPingReply, LWIP_GetActiveSockets(), LWIP_GetMaxSockets(),
			g_powersave ? "POWERSAVE" : "");
#endif
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


#if ENABLE_HA_DISCOVERY
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
#endif
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
#if ENABLE_PING_WATCHDOG
					Main_SetupPingWatchDog(pingTargetServer);
#endif
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
#if ENABLE_BL_SHARED
			if (DRV_IsMeasuringPower())
			{
				BL09XX_SaveEmeteringStatistics();
			}
#endif       
#if ENABLE_DRIVER_HLW8112SPI
			HLW8112_Save_Statistics();
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
	HAL_Run_WDT();
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
unsigned int g_timeMs = 0;
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
	g_timeMs = rtos_get_time();
#elif defined(PLATFORM_ESPIDF) //|| defined(PLATFORM_ESP8266)
	g_timeMs = esp_timer_get_time() / 1000;
#else
	g_timeMs += QUICK_TMR_DURATION;
#endif
	g_deltaTimeMS = g_timeMs - g_last_time;
	// cope with wrap
	if (g_deltaTimeMS > 0x4000) {
		g_deltaTimeMS = ((g_timeMs + 0x4000) - (g_last_time + 0x4000));
	}
	g_last_time = g_timeMs;

#if ENABLE_OBK_SCRIPTING
	SVM_RunThreads(g_deltaTimeMS);
#endif
#if ENABLE_OBK_BERRY
	extern void Berry_RunThreads(int deltaMS);
	Berry_RunThreads(g_deltaTimeMS);
#endif
	RepeatingEvents_RunUpdate(g_deltaTimeMS * 0.001f);
#ifndef OBK_DISABLE_ALL_DRIVERS
	DRV_RunQuickTick();
#endif
#ifdef WINDOWS
	NewTuyaMCUSimulator_RunQuickTick(g_deltaTimeMS);
#endif
	CMD_RunUartCmndIfRequired();

	// process received messages here..
#if ENABLE_MQTT
	MQTT_RunQuickTick();
#endif

#if ENABLE_LED_BASIC
	if (CFG_HasFlag(OBK_FLAG_LED_SMOOTH_TRANSITIONS) == true) {
		LED_RunQuickColorLerp(g_deltaTimeMS);
	}
#endif

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

#define QT_STACK_SIZE 2048

////////////////////////////////////////////////////////
// this is the bit which runs the quick tick timer
#if WINDOWS

#elif PLATFORM_BL602 || PLATFORM_W600 || PLATFORM_W800 || PLATFORM_TR6260 || defined(PLATFORM_REALTEK) || PLATFORM_ECR6600 \
	|| PLATFORM_ESP8266 || PLATFORM_ESPIDF || PLATFORM_XRADIO || PLATFORM_LN882H || PLATFORM_TXW81X || PLATFORM_RDA5981 || PLATFORM_LN8825
void quick_timer_thread(void* param)
{
	while (1) {
		rtos_delay_milliseconds(QUICK_TMR_DURATION);
		QuickTick(0);
	}
}
#else
beken_timer_t g_quick_timer;
#endif
void QuickTick_StartThread(void)
{
#if WINDOWS

#elif PLATFORM_BL602 || PLATFORM_W600 || PLATFORM_W800 || PLATFORM_TR6260 || defined(PLATFORM_REALTEK) || PLATFORM_ECR6600 \
	|| PLATFORM_ESP8266 || PLATFORM_ESPIDF || PLATFORM_XRADIO || PLATFORM_LN882H || PLATFORM_LN8825
	xTaskCreate(quick_timer_thread, "quick", QT_STACK_SIZE, NULL, 15, NULL);
#elif PLATFORM_TXW81X
	os_task_create("quick", quick_timer_thread, NULL, 15, 0, NULL, QT_STACK_SIZE);
#elif PLATFORM_RDA5981
	rda_thread_new("quick", quick_timer_thread, NULL, QT_STACK_SIZE, osPriorityNormal);
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
#if PLATFORM_ESPIDF || PLATFORM_ESP8266 || PLATFORM_BL602 || (PLATFORM_REALTEK && !PLATFORM_REALTEK_NEW) \
|| PLATFORM_XRADIO || PLATFORM_LN8825 || PLATFORM_LN882H
inline __attribute__((always_inline))
#endif
void isidle() {
	idleCount++;
#ifdef WFI_FUNC
	if(g_use_wfi) WFI_FUNC();
#endif
}

bool g_unsafeInitDone = false;

void Main_Init_AfterDelay_Unsafe(bool bStartAutoRunScripts) {

	// initialise MQTT - just sets up variables.
	// all MQTT happens in timer thread?
#if ENABLE_MQTT
	MQTT_init();
#endif

	CMD_Init_Delayed();

	if (bStartAutoRunScripts) {
		if (PIN_FindPinIndexForRole(IOR_IRRecv, -1) != -1 || PIN_FindPinIndexForRole(IOR_IRSend, -1) != -1
			|| PIN_FindPinIndexForRole(IOR_IRRecv_nPup, -1) != -1) {
			// start IR driver 5 seconds after boot.  It may affect wifi connect?
			// yet we also want it to start if no wifi for IR control...
#ifndef OBK_DISABLE_ALL_DRIVERS
			DRV_StartDriver("IR");
			//ScheduleDriverStart("IR",5);
#endif
		}

		// NOTE: this will try to read autoexec.bat,
		// so ALL commands expected in autoexec.bat should have been registered by now...
#if ENABLE_OBK_SCRIPTING
		SVM_RunStartupCommandAsScript();
#else
		CMD_ExecuteCommand(CFG_GetShortStartupCommand(), COMMAND_FLAG_SOURCE_SCRIPT);
#endif
		CMD_ExecuteCommand("startScript autoexec.bat", COMMAND_FLAG_SOURCE_SCRIPT);
#if ENABLE_OBK_BERRY
		CMD_ExecuteCommand("berry import autoexec", COMMAND_FLAG_SOURCE_SCRIPT);
#endif
	}
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
#if ENABLE_LED_BASIC
	NewLED_InitCommands();
#endif
#if ENABLE_SEND_POSTANDGET
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
#ifndef OBK_DISABLE_ALL_DRIVERS
		if (!CFG_HasFlag(OBK_FLAG_DRV_DISABLE_AUTOSTART)) {
			// autostart drivers
			if (PIN_FindPinIndexForRole(IOR_SM2135_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_SM2135_DAT, -1) != -1)
			{
				DRV_StartDriver("SM2135");
			}
			if (PIN_FindPinIndexForRole(IOR_SM2235_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_SM2235_DAT, -1) != -1)
			{
				DRV_StartDriver("SM2235");
			}
			if (PIN_FindPinIndexForRole(IOR_BP5758D_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_BP5758D_DAT, -1) != -1)
			{
				DRV_StartDriver("BP5758D");
			}
			if (PIN_FindPinIndexForRole(IOR_BP1658CJ_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_BP1658CJ_DAT, -1) != -1) {
				DRV_StartDriver("BP1658CJ");
			}
			if (PIN_FindPinIndexForRole(IOR_KP18058_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_KP18058_DAT, -1) != -1) {
				DRV_StartDriver("KP18058");
			}
			if (PIN_FindPinIndexForRole(IOR_BL0937_CF, -1) != -1 && PIN_FindPinIndexForRole(IOR_BL0937_CF1, -1) != -1
				&& (PIN_FindPinIndexForRole(IOR_BL0937_SEL, -1) != -1 || PIN_FindPinIndexForRole(IOR_BL0937_SEL_n, -1) != -1)) {
				DRV_StartDriver("BL0937");
			}
			if ((PIN_FindPinIndexForRole(IOR_BridgeForward, -1) != -1) && (PIN_FindPinIndexForRole(IOR_BridgeReverse, -1) != -1))
			{
				DRV_StartDriver("Bridge");
			}
			if ((PIN_FindPinIndexForRole(IOR_DoorSensorWithDeepSleep, -1) != -1) ||
				(PIN_FindPinIndexForRole(IOR_DoorSensorWithDeepSleep_NoPup, -1) != -1) ||
				(PIN_FindPinIndexForRole(IOR_DoorSensorWithDeepSleep_pd, -1) != -1))
			{
				DRV_StartDriver("DoorSensor");
			}
			if (PIN_FindPinIndexForRole(IOR_CHT83XX_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_CHT83XX_DAT, -1) != -1) {
				DRV_StartDriver("CHT83XX");
			}
			if (PIN_FindPinIndexForRole(IOR_SHT3X_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_SHT3X_DAT, -1) != -1) {
				DRV_StartDriver("SHT3X");
			}
			if (PIN_FindPinIndexForRole(IOR_SGP_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_SGP_DAT, -1) != -1) {
				DRV_StartDriver("SGP");
			}
			if (PIN_FindPinIndexForRole(IOR_BAT_ADC, -1) != -1) {
				DRV_StartDriver("Battery");
			}
			if (PIN_FindPinIndexForRole(IOR_TM1637_CLK, -1) != -1 && PIN_FindPinIndexForRole(IOR_TM1637_DIO, -1) != -1) {
				DRV_StartDriver("TM1637");
			}
			if ((PIN_FindPinIndexForRole(IOR_GN6932_CLK, -1) != -1) &&
				(PIN_FindPinIndexForRole(IOR_GN6932_DAT, -1) != -1) &&
				(PIN_FindPinIndexForRole(IOR_GN6932_STB, -1) != -1))
			{
				DRV_StartDriver("GN6932");
			}
			if (PIN_FindPinIndexForRole(IOR_HLW8112_SCSN, -1) != -1) {
				DRV_StartDriver("HLW8112SPI");
			}
//			if ((PIN_FindPinIndexForRole(IOR_TM1638_CLK, -1) != -1) &&
//				(PIN_FindPinIndexForRole(IOR_TM1638_DAT, -1) != -1) &&
//				(PIN_FindPinIndexForRole(IOR_TM1638_STB, -1) != -1))
//			{
//				DRV_StartDriver("TM1638");
//			}
		}
#endif
	}

	g_enable_pins = 1;
	// this actually sets the pins, moved out so we could avoid if necessary
	PIN_SetupPins();
	QuickTick_StartThread();

#if ENABLE_LED_BASIC
	NewLED_RestoreSavedStateIfNeeded();
#endif
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
	ADDLOGF_INFO("%s", __func__);
	// read or initialise the boot count flash area
	HAL_FlashVars_IncreaseBootCount();

#if defined(PLATFORM_BEKEN)
	// this just increments our idle counter variable.
	// it registers a cllback from RTOS IDLE function.
	// why is it called IRDA??  is this where they check for IR?
	bg_register_irda_check_func(isidle);
#elif PLATFORM_TR6260
	system_register_idle_callback(isidle);
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

	ADDLOGF_INFO("%s done", __func__);
	bk_printf("\r\%s done\r\n", __func__);
}

// a fixed delay of 750ms to wait for calibration routines in core thread,
// which must complete before LWIP etc. are intialised,
// and crucially before we use certain TCP/IP features.
// it would be nicer to understand more about why, and to wait for TCPIP to be ready
// rather than use a fixed delay.
// (e.g. are we delayed by it reading temperature?)
void Main_Init_Delay()
{
	ADDLOGF_INFO("%s", __func__);
	bk_printf("\r\%s\r\n", __func__);

	extended_app_waiting_for_launch2();

	ADDLOGF_INFO("%s done", __func__);
	bk_printf("\r\%s done\r\n", __func__);

	// use this variable wherever to determine if we have TCP/IP features.
	// e.g. in logging to determine if we can start TCP thread
	g_StartupDelayOver = 1;
}


// do things after the start delay.
// i.e. things that use TCP/IP like NTP, SSDP, DGR
void Main_Init_After_Delay()
{
	const char* wifi_ssid, * wifi_pass;
	ADDLOGF_INFO("%s", __func__);

	// we can log this after delay.
	if (bSafeMode) {
		ADDLOGF_INFO("###### safe mode activated - boot failures %d", g_bootFailures);
	}
#if ALLOW_SSID2
	Init_WiFiSSIDactual_FromChannelIfSet();//Channel must be set in early.bat using CMD_setStartupSSIDChannel
#endif
	wifi_ssid = CFG_GetWiFiSSIDX();
	wifi_pass = CFG_GetWiFiPassX();

#if 0
	// you can use this if you bricked your module by setting wrong access point data
	wifi_ssid = "qqqqqqqqqq";
	wifi_pass = "Fqqqqqqqqqqqqqqqqqqqqqqqqqqq"
#endif
#ifdef SPECIAL_UNBRICK_ALWAYS_OPEN_AP
		// you can use this if you bricked your module by setting wrong access point data
		bForceOpenAP = 1;
#endif

	HAL_Configure_WDT();

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
#if ENABLE_MQTT
				mqtt_loopsWithDisconnected = 9999;
#endif
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

#if MQTT_USE_TLS
	if (!CFG_GetDisableWebServer() || bSafeMode) {
#endif		
		HTTPServer_Start();
		ADDLOGF_DEBUG("Started http tcp server\r\n");
#if MQTT_USE_TLS
	} 
#endif		

	// only initialise certain things if we are not in AP mode
	if (!bSafeMode)
	{
#if ENABLE_HA_DISCOVERY
		//Always invoke discovery on startup. This accounts for change in ipaddr before startup and firmware update.
		if (CFG_HasFlag(OBK_FLAG_AUTOMAIC_HASS_DISCOVERY)) {
			Main_ScheduleHomeAssistantDiscovery(1);
		}
#endif
		Main_Init_AfterDelay_Unsafe(true);
	}

	ADDLOGF_INFO("%s done", __func__);
}

// to be overriden
// Translate name like RB5 for OBK pin index
#ifdef _MSC_VER
///#pragma comment(linker, "/alternatename:HAL_PIN_Find=Default_HAL_PIN_Find")
#else
int HAL_PIN_Find(const char *name) __attribute__((weak));
int HAL_PIN_Find(const char *name) {
	return atoi(name);
}
#endif


void Main_Init()
{
	g_unsafeInitDone = false;
	bk_printf("%s, version %s\r\n", DEVICENAME_PREFIX_FULL, USER_SW_VER);

#ifdef WINDOWS
#if ENABLE_LED_BASIC
	// on windows, Main_Init may happen multiple time so we need to reset variables
	LED_ResetGlobalVariablesToDefaults();
#endif
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

#if PLATFORM_ESPIDF || PLATFORM_ESP8266 || PLATFORM_BL602 || (PLATFORM_REALTEK && !PLATFORM_REALTEK_NEW) \
|| PLATFORM_XRADIO || PLATFORM_W600 || PLATFORM_W800 || PLATFORM_LN8825 || PLATFORM_LN882H

void vApplicationIdleHook(void)
{
	isidle();
}

#endif
