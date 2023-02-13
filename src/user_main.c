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

#ifdef ENABLE_LITTLEFS
#include "littlefs/our_lfs.h"
#endif


#include "driver/drv_ntp.h"
#include "driver/drv_ssdp.h"

#ifdef PLATFORM_BEKEN
#include <mcu_ps.h>
#include <fake_clock_pub.h>
void bg_register_irda_check_func(FUNCPTR func);
#endif

static int g_secondsElapsed = 0;
// open access point after this number of seconds
static int g_openAP = 0;
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
// was it ran?
static int g_bPingWatchDogStarted = 0;
// current IP string, this is compared with IP returned from HAL
// and if it changes, the MQTT publish is done
static char g_currentIPString[32] = { 0 };
static HALWifiStatus_t g_newWiFiStatus = WIFI_UNDEFINED;
static HALWifiStatus_t g_prevWiFiStatus = WIFI_UNDEFINED;

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

#ifdef PLATFORM_BK7231T
	// this function waits for the extended app functions to finish starting.
	extern void extended_app_waiting_for_launch(void);
	void extended_app_waiting_for_launch2(){
		extended_app_waiting_for_launch();
	}
#else
	void extended_app_waiting_for_launch2(void){
		// do nothing?

		// define FIXED_DELAY if delay wanted on non-beken platforms.
	#ifdef PLATFORM_BK7231N
		// wait 100ms at the start.
		// TCP is being setup in a different thread, and there does not seem to be a way to find out if it's complete yet?
		// so just wait a bit, and then start.
		int startDelay = 750;
		bk_printf("\r\ndelaying start\r\n");
		for (int i = 0; i < startDelay/10; i++){
			rtos_delay_milliseconds(10);
			bk_printf("#Startup delayed %dms#\r\n", i*10);
		}
		bk_printf("\r\nstarting....\r\n");

		// through testing, 'Initializing TCP/IP stack' appears at ~500ms
		// so we should wait at least 750?
	#endif

	}
#endif

#if defined(PLATFORM_BL602) || defined(PLATFORM_W800) || defined(PLATFORM_W600)



OSStatus rtos_create_thread( beken_thread_t* thread,
							uint8_t priority, const char* name,
							beken_thread_function_t function,
							uint32_t stack_size, beken_thread_arg_t arg ) {
    OSStatus err = kNoErr;


    err = xTaskCreate(function, name, stack_size/sizeof(StackType_t), arg, priority, thread);
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
	if(err == pdPASS){
		//printf("Thread create %s - pdPASS\n",name);
		return 0;
	} else if(err == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY ) {
		printf("Thread create %s - errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY\n",name);
	} else {
		printf("Thread create %s - err %i\n",name,err);
	}
	return 1;
}

OSStatus rtos_delete_thread( beken_thread_t* thread ) {
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

int Time_getUpTimeSeconds() {
	return g_secondsElapsed;
}


static char scheduledDriverName[4][16];
static int scheduledDelay[4] = {-1, -1, -1, -1};
void ScheduleDriverStart(const char *name, int delay) {
	int i;

	for (i = 0; i < 4; i++){
		// if already scheduled, just change delay.
		if (!strcmp(scheduledDriverName[i], name)){
			scheduledDelay[i] = delay;
			return;
		}
	}
	for (i = 0; i < 4; i++){
		// first empty slot
		if (scheduledDelay[i] == -1){
			scheduledDelay[i] = delay;
			strncpy(scheduledDriverName[i], name, 16);
			return;
		}
	}
}

void Main_OnWiFiStatusChange(int code)
{
	// careful what you do in here.
	// e.g. creata socket?  probably not....
    switch(code)
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
            g_connectToWiFi = 60;
			g_bHasWiFiConnected = 0;
			ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_STA_AUTH_FAILED - %i\r\n", code);
            break;
        case WIFI_STA_CONNECTED:
			g_bHasWiFiConnected = 1;
			ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_STA_CONNECTED - %i\r\n", code);

			if(bSafeMode == 0){
				if(strlen(CFG_DeviceGroups_GetName())>0){
					ScheduleDriverStart("DGR",5);
				}
				// if SSDP should be active, 
				// restart it now.
				if (DRV_SSDP_Active){
					ScheduleDriverStart("SSDP",5);
					//DRV_SSDP_Restart(); // this kills things
				}
			}

            break;
        /* for softap mode */
        case WIFI_AP_CONNECTED:
			g_bHasWiFiConnected = 1;
			ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_AP_CONNECTED - %i\r\n", code);
            break;
        case WIFI_AP_FAILED:
			g_bHasWiFiConnected = 0;
			ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_AP_FAILED - %i\r\n",code);
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
void Main_LogPowerSave(){
	if (!OBK_Mcu_metrics.nexttask){
		OBK_Mcu_metrics.nexttask = "unkn";
	}
	if (!OBK_Mcu_metrics.task){
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

void Main_OnEverySecond()
{
	int newMQTTState;
	const char *safe;
	int i;

#ifdef WINDOWS
	g_bHasWiFiConnected = 1;
#endif

	// run_adc_test();
	newMQTTState = MQTT_RunEverySecondUpdate();
	if(newMQTTState != bMQTTconnected) {
		bMQTTconnected = newMQTTState;
		if(newMQTTState) {
			EventHandlers_FireEvent(CMD_EVENT_MQTT_STATE,1);
		} else {
			EventHandlers_FireEvent(CMD_EVENT_MQTT_STATE,0);
		}
	}
	if (g_newWiFiStatus != g_prevWiFiStatus) {
		g_newWiFiStatus = g_prevWiFiStatus;
		// Argument type here is HALWifiStatus_t enumeration
		EventHandlers_FireEvent(CMD_EVENT_WIFI_STATE, g_newWiFiStatus);
	}
	MQTT_Dedup_Tick();
	RepeatingEvents_OnEverySecond();
#ifndef OBK_DISABLE_ALL_DRIVERS
	DRV_OnEverySecond();
#endif

#if WINDOWS
#elif PLATFORM_BL602
#elif PLATFORM_W600 || PLATFORM_W800
#elif PLATFORM_XR809
#elif PLATFORM_BK7231N || PLATFORM_BK7231T
    if (ota_progress()==-1)
#endif
    {
		CFG_Save_IfThereArePendingChanges();
    }

	if (bSafeMode == 0) {
		const char *ip = HAL_GetMyIPString();
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
	if(g_timeSinceLastPingReply != -1 && g_secondsElapsed > 60)
    {
		EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_NOPINGTIME, g_timeSinceLastPingReply, g_timeSinceLastPingReply+1);
		g_timeSinceLastPingReply++;
		if(g_timeSinceLastPingReply >= CFG_GetPingDisconnectedSecondsToRestart()) 
        {
            if (g_bHasWiFiConnected != 0)
            {
    			ADDLOGF_INFO("[Ping watchdog] No ping replies within %i seconds. Will try to reconnect.\n",g_timeSinceLastPingReply);
                HAL_DisconnectFromWifi();
		    	g_bHasWiFiConnected = 0;
			    g_connectToWiFi = 10;
                g_timeSinceLastPingReply = -1;
            }
		}
	}

	if(bSafeMode == 0) 
    {

		for(i = 0; i < PLATFORM_GPIO_MAX; i++) 
        {
			if(g_cfg.pins.roles[i] == IOR_ADC) 
            {
				int value;

				value = HAL_ADC_Read(i);

			//	ADDLOGF_INFO("ADC %i=%i\r\n", i,value);
				CHANNEL_Set(g_cfg.pins.channels[i],value, CHANNEL_SET_FLAG_SILENT);
			}
		}
	}

	// allow for up to 4 scheduled driver starts.
	for (i = 0; i < 4; i++){
		if (scheduledDelay[i] > 0){
			scheduledDelay[i]--;
			if(scheduledDelay[i] <= 0)
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

	g_secondsElapsed ++;
	if(bSafeMode) {
		safe = "[SAFE] ";
	} else {
		safe = "";
	}

	{
		//int mqtt_max, mqtt_cur, mqtt_mem;
		//MQTT_GetStats(&mqtt_cur, &mqtt_max, &mqtt_mem);
	    //ADDLOGF_INFO("mqtt req %i/%i, free mem %i\n", mqtt_cur,mqtt_max,mqtt_mem);
		ADDLOGF_INFO("%sTime %i, idle %i/s, free %d, MQTT %i(%i), bWifi %i, secondsWithNoPing %i, socks %i/%i %s\n",
			safe, g_secondsElapsed, idleCount, xPortGetFreeHeapSize(),bMQTTconnected, MQTT_GetConnectEvents(), 
			g_bHasWiFiConnected, g_timeSinceLastPingReply, LWIP_GetActiveSockets(), LWIP_GetMaxSockets(),
			g_powersave ? "POWERSAVE" : "");
		// reset so it's a per-second counter.
		idleCount = 0;
	}

#ifdef OBK_MCU_SLEEP_METRICS_ENABLE
	Main_LogPowerSave();
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
	if(g_bBootMarkedOK==false) 
    {
		int bootCompleteSeconds = CFG_GetBootOkSeconds();
		if (g_secondsElapsed > bootCompleteSeconds)
        {
			ADDLOGF_INFO("Boot complete time reached (%i seconds)\n",bootCompleteSeconds);
			HAL_FlashVars_SaveBootComplete();
			g_bootFailures = HAL_FlashVars_GetBootFailures();
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
		g_openAP--;
		if (0 == g_openAP)
        {
			HAL_SetupWiFiOpenAccessPoint(CFG_GetDeviceName());
			g_bOpenAccessPointMode = 1;
		}
	}

	//ADDLOGF_INFO("g_startPingWatchDogAfter %i, g_bPingWatchDogStarted %i ", g_startPingWatchDogAfter, g_bPingWatchDogStarted);
	if(g_bHasWiFiConnected) {
		if (g_startPingWatchDogAfter) {
			//ADDLOGF_INFO("g_startPingWatchDogAfter %i", g_startPingWatchDogAfter);
			g_startPingWatchDogAfter--;
			if (0 == g_startPingWatchDogAfter)
			{
				const char *pingTargetServer;
				//int pingInterval;
				int restartAfterNoPingsSeconds;

				g_bPingWatchDogStarted = 1;

				pingTargetServer = CFG_GetPingHost();
				//pingInterval = CFG_GetPingIntervalSeconds();
				restartAfterNoPingsSeconds = CFG_GetPingDisconnectedSecondsToRestart();

				if ((pingTargetServer != NULL) && (strlen(pingTargetServer) > 0) &&
					/*(pingInterval > 0) && */ (restartAfterNoPingsSeconds > 0))
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
	if(g_connectToWiFi)
    {
		g_connectToWiFi --;
		if(0 == g_connectToWiFi && g_bHasWiFiConnected == 0) 
        {
			const char *wifi_ssid, *wifi_pass;

			g_bOpenAccessPointMode = 0;
			wifi_ssid = CFG_GetWiFiSSID();
			wifi_pass = CFG_GetWiFiPass();
			HAL_ConnectToWiFi(wifi_ssid,wifi_pass);
			// register function to get callbacks about wifi changes.
			HAL_WiFi_SetupStatusCallback(Main_OnWiFiStatusChange);
			ADDLOGF_DEBUG("Registered for wifi changes\r\n");
		}
	}

	// config save moved here because of stack size problems
	if (g_saveCfgAfter){
		g_saveCfgAfter--;
		if (!g_saveCfgAfter){
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
	if (g_reset){
		g_reset--;
		if (!g_reset){
			// ensure any config changes are saved before reboot.
			CFG_Save_IfThereArePendingChanges();
#ifndef OBK_DISABLE_ALL_DRIVERS
            if (DRV_IsMeasuringPower()) 
            {
                BL09XX_SaveEmeteringStatistics();
            }
#endif            
			ADDLOGF_INFO("Going to call HAL_RebootModule\r\n");
			HAL_RebootModule();
		} else {

			ADDLOGF_INFO("Module reboot in %i...\r\n",g_reset);
		}
	}

#if defined(PLATFORM_BEKEN) || defined(PLATFORM_BL602) || defined(PLATFORM_W600) || defined(WINDOWS)
	if (g_dhtsCount>0) {
		if (bSafeMode == 0) {
			DHT_OnEverySecond();
		}
	}
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

/////////////////////////////////////////////////////
// this is what we do in a qucik tick
void QuickTick(void *param)
{
	if (g_bWantPinDeepSleep) {
		PINS_BeginDeepSleepWithPinWakeUp();
		g_bWantPinDeepSleep = 0;
		return;
	}

#if defined(PLATFORM_BEKEN) && defined(BEKEN_PIN_GPI_INTERRUPTS)
	// if using interrupt driven GPI for pins, don't call PIN_ticks() in QuickTick
#else
	PIN_ticks(param);
#endif

#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
	g_time = rtos_get_time();
#else
	g_time += QUICK_TMR_DURATION;
#endif
	uint32_t t_diff = g_time - g_last_time;
	// cope with wrap
	if (t_diff > 0x4000){
		t_diff = ((g_time + 0x4000) - (g_last_time + 0x4000));
	}
	g_last_time = g_time;


#if (defined WINDOWS) || (defined PLATFORM_BEKEN)
	SVM_RunThreads(t_diff);
#endif
#ifndef OBK_DISABLE_ALL_DRIVERS
	DRV_RunQuickTick();
#endif
#ifdef WINDOWS
	NewTuyaMCUSimulator_RunQuickTick(t_diff);
#endif
	CMD_RunUartCmndIfRequired();

	// process recieved messages here..
	MQTT_RunQuickTick();
	
	if(CFG_HasFlag(OBK_FLAG_LED_SMOOTH_TRANSITIONS) == true) {
		LED_RunQuickColorLerp(t_diff);
	}

	// WiFi LED
	// In Open Access point mode, fast blink
	if(Main_IsOpenAccessPointMode()) {
		g_wifiLedToggleTime += t_diff;
		if(g_wifiLedToggleTime > WIFI_LED_FAST_BLINK_DURATION) {
			g_wifi_ledState = !g_wifi_ledState;
			g_wifiLedToggleTime = 0;
			PIN_set_wifi_led(g_wifi_ledState);
		}
	} else if(Main_IsConnectedToWiFi()) {
		// In WiFi client success mode, just stay enabled
		PIN_set_wifi_led(1);
	} else {
		// in connecting mode, slow blink
		g_wifiLedToggleTime += t_diff;
		if(g_wifiLedToggleTime > WIFI_LED_SLOW_BLINK_DURATION) {
			g_wifi_ledState = !g_wifi_ledState;
			g_wifiLedToggleTime = 0;
			PIN_set_wifi_led(g_wifi_ledState);
		}
	}

}



////////////////////////////////////////////////////////
// this is the bit which runs the quick tick timer
#if WINDOWS

#elif PLATFORM_BL602
void quick_timer_thread(void *param)
{
    while(1) {
        vTaskDelay(QUICK_TMR_DURATION);
		QuickTick(0);
    }
}
#elif PLATFORM_W600 || PLATFORM_W800
void quick_timer_thread(void *param)
{
    while(1) {
        vTaskDelay(QUICK_TMR_DURATION);
		QuickTick(0);
    }
}
#elif PLATFORM_XR809
OS_Timer_t g_quick_timer;
#else
beken_timer_t g_quick_timer;
#endif
void QuickTick_StartThread(void)
{
#if WINDOWS

#elif PLATFORM_BL602

    xTaskCreate(quick_timer_thread, "quick", 1024, NULL, 15, NULL);
#elif PLATFORM_W600 || PLATFORM_W800

    xTaskCreate(quick_timer_thread, "quick", 1024, NULL, 15, NULL);
#elif PLATFORM_XR809

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
                            (void *)0);
    ASSERT(kNoErr == result);

    result = rtos_start_timer(&g_quick_timer);
    ASSERT(kNoErr == result);
#endif
}
///////////////////////////////////////////////////////


void app_on_generic_dbl_click(int btnIndex)
{
	if(g_secondsElapsed < 5) 
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

int Main_GetLastRebootBootFailures() 
{
	return g_bootFailures;
}

#define RESTARTS_REQUIRED_FOR_SAFE_MODE 4


// called from idle thread each loop.
// - just so we know it is running.
void isidle(){
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
		CMD_ExecuteCommand("exec autoexec.bat", COMMAND_FLAG_SOURCE_SCRIPT);
	}
}
void Main_Init_BeforeDelay_Unsafe(bool bAutoRunScripts) {
	g_unsafeInitDone = true;
#ifndef OBK_DISABLE_ALL_DRIVERS
	DRV_Generic_Init();
#endif
	RepeatingEvents_Init();

	// set initial values for channels.
	// this is done early so lights come on at the flick of a switch.
	CFG_ApplyChannelStartValues();
	PIN_AddCommands();
	ADDLOGF_DEBUG("Initialised pins\r\n");

#ifdef ENABLE_LITTLEFS
	// initialise the filesystem, only if present.
	// don't create if it does not mount
	// do this for ST mode only, as it may be something in FS which is killing us,
	// and we may add a command to empty fs just be writing first sector?
	init_lfs(0);
#endif

#ifdef ENABLE_LITTLEFS
	LFSAddCmds(); // setlfssize
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

	/* Automatic disable of PIN MONITOR after reboot */
	if (CFG_HasFlag(OBK_FLAG_HTTP_PINMONITOR)) {
		CFG_SetFlag(OBK_FLAG_HTTP_PINMONITOR, false);
	}

	if (bAutoRunScripts) {
		CMD_ExecuteCommand("exec early.bat", COMMAND_FLAG_SOURCE_SCRIPT);

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
		if (PIN_FindPinIndexForRole(IOR_BL0937_CF, -1) != -1 && PIN_FindPinIndexForRole(IOR_BL0937_CF1, -1) != -1 && PIN_FindPinIndexForRole(IOR_BL0937_SEL, -1) != -1) {
#ifndef OBK_DISABLE_ALL_DRIVERS
			DRV_StartDriver("BL0937");
#endif
		}
        if((PIN_FindPinIndexForRole(IOR_BridgeForward, -1) != -1) && (PIN_FindPinIndexForRole(IOR_BridgeReverse, -1) != -1))
        {
#ifndef OBK_DISABLE_ALL_DRIVERS
            DRV_StartDriver("Bridge");
#endif
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
	const char *wifi_ssid, *wifi_pass;
	ADDLOGF_INFO("Main_Init_After_Delay");

	// we can log this after delay.
	if (bSafeMode){
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

	if((*wifi_ssid == 0))
    {
		// start AP mode in 5 seconds
		g_openAP = 5;
		//HAL_SetupWiFiOpenAccessPoint();
	} else {
        if (bSafeMode)
        {
            g_openAP = 5;
        } else {
    		g_connectToWiFi = 5;
        }
	}

	ADDLOGF_INFO("Using SSID [%s]\r\n",wifi_ssid);
	ADDLOGF_INFO("Using Pass [%s]\r\n",wifi_pass);

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


