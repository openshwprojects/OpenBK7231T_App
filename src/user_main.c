/*

 */
//

#include "hal/hal_wifi.h"
#include "hal/hal_generic.h"
#include "hal/hal_flashVars.h"
#include "hal/hal_adc.h"
#include "new_common.h"

#include "driver/drv_public.h"

// Commands register, execution API and cmd tokenizer
#include "cmnds/cmd_public.h"

// overall config variables for app - like BK_LITTLEFS
#include "obk_config.h"

#include "httpserver/new_http.h"
#include "new_pins.h"
#include "new_cfg.h"
#include "logging/logging.h"
#include "httpserver/http_tcp_server.h"
#include "httpserver/rest_interface.h"
#include "mqtt/new_mqtt.h"

#ifdef BK_LITTLEFS
#include "littlefs/our_lfs.h"
#endif


#include "driver/drv_ntp.h"

static int g_secondsElapsed = 0;
// open access point after this number of seconds
static int g_openAP = 0;
// connect to wifi after this number of seconds
static int g_connectToWiFi = 0;
// many boots failed? do not run pins or anything risky
static int bSafeMode = 0;
// reset after this number of seconds
static int g_reset = 0;
// is connected to WiFi?
static int g_bHasWiFiConnected = 0;
// is Open Access point or a client?
static int g_bOpenAccessPointMode = 0;

static int g_bootFailures = 0;

static int g_saveCfgAfter = 0;
static int g_startPingWatchDogAfter = 0;

// not really <time>, but rather a loop count, but it doesn't really matter much
static int g_timeSinceLastPingReply = 0;
// was it ran?
static int g_bPingWatchDogStarted = 0;

#define LOG_FEATURE LOG_FEATURE_MAIN


#if PLATFORM_XR809
size_t xPortGetFreeHeapSize() {
	return 0;
}
#endif

#if defined(PLATFORM_BL602) || defined(PLATFORM_W800)



OSStatus rtos_create_thread( beken_thread_t* thread,
							uint8_t priority, const char* name,
							beken_thread_function_t function,
							uint32_t stack_size, beken_thread_arg_t arg ) {
    OSStatus err = kNoErr;


    err = xTaskCreate(function, name, stack_size, arg, 1, thread);
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
void RESET_ScheduleModuleReset(int delSeconds) {
	g_reset = delSeconds;
}

int Time_getUpTimeSeconds() {
	return g_secondsElapsed;
}


static char scheduledDriverName[16];
static int scheduledDelay = 0;
static void ScheduleDriverStart(const char *name, int delay) {
	scheduledDelay = delay;
	strcpy(scheduledDriverName,name);
}

void Main_OnWiFiStatusChange(int code){


    switch(code){
        case WIFI_STA_CONNECTING:
			g_bHasWiFiConnected = 0;
            g_connectToWiFi = 120;
			ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_STA_CONNECTING\r\n");
            break;
        case WIFI_STA_DISCONNECTED:
            // try to connect again in few seconds
            g_connectToWiFi = 15;
			g_bHasWiFiConnected = 0;
			ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_STA_DISCONNECTED\r\n");
            break;
        case WIFI_STA_AUTH_FAILED:
            // try to connect again in few seconds
            g_connectToWiFi = 60;
			g_bHasWiFiConnected = 0;
			ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_STA_AUTH_FAILED\r\n");
            break;
        case WIFI_STA_CONNECTED:
			g_bHasWiFiConnected = 1;
			ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_STA_CONNECTED\r\n");
			
			if(bSafeMode == 0 && strlen(CFG_DeviceGroups_GetName())>0){
				ScheduleDriverStart("DGR",5);
			}

            break;
        /* for softap mode */
        case WIFI_AP_CONNECTED:
			g_bHasWiFiConnected = 1;
			ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_AP_CONNECTED\r\n");
            break;
        case WIFI_AP_FAILED:
			g_bHasWiFiConnected = 0;
			ADDLOGF_INFO("Main_OnWiFiStatusChange - WIFI_AP_FAILED\r\n");
            break;
        default:
            break;
    }

}


void CFG_Save_SetupTimer() {
	g_saveCfgAfter = 3;
}
void Main_OnPingCheckerReply(int ms) {
	g_timeSinceLastPingReply = 0;
}
int g_bBootMarkedOK = 0;


void Main_OnEverySecond()
{
	int bMQTTconnected;
	const char *safe;
	int val;

	// run_adc_test();
	bMQTTconnected = MQTT_RunEverySecondUpdate();
	RepeatingEvents_OnEverySecond();
#ifndef OBK_DISABLE_ALL_DRIVERS
	DRV_OnEverySecond();
#endif
	// some users say that despite our simple reconnect mechanism
	// there are some rare cases when devices stuck outside network
	// That is why we can also reconnect them by basing on ping
	if(g_timeSinceLastPingReply != -1 && g_secondsElapsed > 60) {
		g_timeSinceLastPingReply++;
		if(g_timeSinceLastPingReply == CFG_GetPingDisconnectedSecondsToRestart()) {
			ADDLOGF_INFO("[Ping watchdog] No ping replies within %i seconds. Will try to reconnect.\n",g_timeSinceLastPingReply);
			g_bHasWiFiConnected = 0;
			g_connectToWiFi = 10;
		}
	}

	if(bSafeMode == 0) {
		int i;

		for(i = 0; i < PLATFORM_GPIO_MAX; i++) {
			if(g_cfg.pins.roles[i] == IOR_ADC) {
				int value;

				value = HAL_ADC_Read(i);

			//	ADDLOGF_INFO("ADC %i=%i\r\n", i,value);
				CHANNEL_Set(g_cfg.pins.channels[i],value, CHANNEL_SET_FLAG_SILENT);
			}
		}
	}

	if(scheduledDelay > 0) {
		scheduledDelay--;
		if(scheduledDelay<=0){
			scheduledDelay = -1;
#ifndef OBK_DISABLE_ALL_DRIVERS
			DRV_StopDriver(scheduledDriverName);
			DRV_StartDriver(scheduledDriverName);
#endif
		}
	}

	g_secondsElapsed ++;
	if(bSafeMode) {
		safe = "[SAFE] ";
	} else {
		safe = "";
	}
#ifdef PLATFORM_BL602

		ADDLOGF_INFO("%sTime %i, free %d, MQTT %i, bWifi %i, secondsWithNoPing %i\n",
			safe, g_secondsElapsed, xPortGetFreeHeapSize(),bMQTTconnected, g_bHasWiFiConnected,g_timeSinceLastPingReply);
#else
	{
		//int mqtt_max, mqtt_cur, mqtt_mem;
		//MQTT_GetStats(&mqtt_cur, &mqtt_max, &mqtt_mem);
	//	ADDLOGF_INFO("mqtt req %i/%i, free mem %i\n", mqtt_cur,mqtt_max,mqtt_mem);
		ADDLOGF_INFO("%sTime %i, free %d, MQTT %i, bWifi %i, secondsWithNoPing %i, socks %i/%i\n",
			safe, g_secondsElapsed, xPortGetFreeHeapSize(),bMQTTconnected, g_bHasWiFiConnected,g_timeSinceLastPingReply,
			LWIP_GetActiveSockets(),LWIP_GetMaxSockets());

	}
#endif

	// print network info
	if (!(g_secondsElapsed % 10)){
		HAL_PrintNetworkInfo();
	}

	// when we hit 30s, mark as boot complete.
	if(g_bBootMarkedOK==false) {
		int bootCompleteSeconds = CFG_GetBootOkSeconds();
		if (g_secondsElapsed > bootCompleteSeconds){
			ADDLOGF_INFO("Boot complete time reached (%i seconds)\n",bootCompleteSeconds);
			HAL_FlashVars_SaveBootComplete();
			g_bootFailures = HAL_FlashVars_GetBootFailures();
			g_bBootMarkedOK = true;
		}
	}

	if (g_openAP){
		g_openAP--;
		if (0 == g_openAP){
			HAL_SetupWiFiOpenAccessPoint(CFG_GetDeviceName());
			g_bOpenAccessPointMode = 1;
		}
	}
	if(g_startPingWatchDogAfter) {
		g_startPingWatchDogAfter--;
		if(0==g_startPingWatchDogAfter) {
			const char *pingTargetServer;
			///int pingInterval;
			int restartAfterNoPingsSeconds;

			g_bPingWatchDogStarted = 1;

			pingTargetServer = CFG_GetPingHost();
			//pingInterval = CFG_GetPingIntervalSeconds();
			restartAfterNoPingsSeconds = CFG_GetPingDisconnectedSecondsToRestart();

			if(*pingTargetServer /* && pingInterval > 0*/ && restartAfterNoPingsSeconds > 0) {
				// mark as enabled
				g_timeSinceLastPingReply = 0;
			//	Main_SetupPingWatchDog(pingTargetServer,pingInterval);
				Main_SetupPingWatchDog(pingTargetServer
					/*,1*/
					);
			} else {
				// mark as disabled
				g_timeSinceLastPingReply = -1;
			}
		}
	}
	if(g_connectToWiFi){
		g_connectToWiFi --;
		if(0 == g_connectToWiFi && g_bHasWiFiConnected == 0) {
			const char *wifi_ssid, *wifi_pass;

			g_bOpenAccessPointMode = 0;
			wifi_ssid = CFG_GetWiFiSSID();
			wifi_pass = CFG_GetWiFiPass();
			HAL_ConnectToWiFi(wifi_ssid,wifi_pass);
			// register function to get callbacks about wifi changes.
			HAL_WiFi_SetupStatusCallback(Main_OnWiFiStatusChange);
			ADDLOGF_DEBUG("Registered for wifi changes\r\n");

			// it must be done with a delay
			if (g_bootFailures < 2 && g_bPingWatchDogStarted == 0){
				g_startPingWatchDogAfter = 60;
			}
		}
	}

	// config save moved here because of stack size problems
	if (g_saveCfgAfter){
		g_saveCfgAfter--;
		if (!g_saveCfgAfter){
			CFG_Save_IfThereArePendingChanges();
		}
	}
	if (g_reset){
		g_reset--;
		if (!g_reset){
			// ensure any config changes are saved before reboot.
			CFG_Save_IfThereArePendingChanges();
			ADDLOGF_INFO("Going to call HAL_RebootModule\r\n");
			HAL_RebootModule();
		} else {

			ADDLOGF_INFO("Module reboot in %i...\r\n",g_reset);
		}
	}


}

void app_on_generic_dbl_click(int btnIndex)
{
	if(g_secondsElapsed < 5) {
		CFG_SetOpenAccessPoint();
	}
}


int Main_IsOpenAccessPointMode() {
	return g_bOpenAccessPointMode;
}
int Main_IsConnectedToWiFi() {
	return g_bHasWiFiConnected;
}
int Main_GetLastRebootBootFailures() {
	return g_bootFailures;
}

#define RESTARTS_REQUIRED_FOR_SAFE_MODE 4

void Main_Init()
{
	const char *wifi_ssid, *wifi_pass;

	// read or initialise the boot count flash area
	HAL_FlashVars_IncreaseBootCount();

	g_bootFailures = HAL_FlashVars_GetBootFailures();
	if (g_bootFailures > RESTARTS_REQUIRED_FOR_SAFE_MODE){
		bSafeMode = 1;
		ADDLOGF_INFO("###### safe mode activated - boot failures %d", g_bootFailures);
	}

	CFG_InitAndLoad();
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

	if(*wifi_ssid == 0 || *wifi_pass == 0 || bSafeMode) {
		// start AP mode in 5 seconds
		g_openAP = 5;
		//HAL_SetupWiFiOpenAccessPoint();
	} else {
		g_connectToWiFi = 5;
	}

	ADDLOGF_INFO("Using SSID [%s]\r\n",wifi_ssid);
	ADDLOGF_INFO("Using Pass [%s]\r\n",wifi_pass);

	// NOT WORKING, I done it other way, see ethernetif.c
	//net_dhcp_hostname_set(g_shortDeviceName);

	HTTPServer_Start();
	ADDLOGF_DEBUG("Started http tcp server\r\n");
	// only initialise certain things if we are not in AP mode
	if (!bSafeMode){

#ifndef OBK_DISABLE_ALL_DRIVERS
		DRV_Generic_Init();
#endif
		RepeatingEvents_Init();

		CFG_ApplyChannelStartValues();

		PIN_AddCommands();

		//CFG_ApplyStartChannelValues();
		ADDLOGF_DEBUG("Initialised pins\r\n");

		// initialise MQTT - just sets up variables.
		// all MQTT happens in timer thread?
		MQTT_init();

		PIN_SetGenericDoubleClickCallback(app_on_generic_dbl_click);
		ADDLOGF_DEBUG("Initialised other callbacks\r\n");

#ifdef BK_LITTLEFS
		// initialise the filesystem, only if present.
		// don't create if it does not mount
		// do this for ST mode only, as it may be something in FS which is killing us,
		// and we may add a command to empty fs just be writing first sector?
		init_lfs(0);
#endif
		// initialise rest interface
		init_rest();

		// add some commands...
		taslike_commands_init();
		fortest_commands_init();
		NewLED_InitCommands();
#if PLATFORM_BEKEN
		CMD_InitSendCommands();
#endif
		CMD_InitChannelCommands();
		EventHandlers_Init();

		// NOTE: this will try to read autoexec.bat,
		// so ALL commands expected in autoexec.bat should have been registered by now...
		// but DON't run autoexec if we have had 2+ boot failures
		CMD_Init();

		CMD_ExecuteCommand(CFG_GetShortStartupCommand(), COMMAND_FLAG_SOURCE_SCRIPT);
		CMD_ExecuteCommand("exec autoexec.bat", COMMAND_FLAG_SOURCE_SCRIPT);

		// autostart drivers
		if(PIN_FindPinIndexForRole(IOR_SM2135_CLK,-1) != -1 && PIN_FindPinIndexForRole(IOR_SM2135_DAT,-1) != -1) {
#ifndef OBK_DISABLE_ALL_DRIVERS
			DRV_StartDriver("SM2135");
#endif
		}

		g_enable_pins = 1;
		// this actually sets the pins, moved out so we could avoid if necessary
		PIN_SetupPins();
		PIN_StartButtonScanThread();

	}

}
