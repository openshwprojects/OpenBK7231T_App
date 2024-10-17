#ifdef WINDOWS

#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#include <crtdbg.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "obk_config.h"
#include "new_common.h"
#include "quicktick.h"
#include "driver\drv_public.h"
#include "cmnds\cmd_public.h"
#include "httpserver\new_http.h"
#include "hal\hal_flashVars.h"
#include "selftest\selftest_local.h"
#include "new_pins.h"
#include <timeapi.h>

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "Winmm.lib")

int accum_time = 0;
int win_frameNum = 0;
// this time counter is simulated, I need this for unit tests to work
int g_simulatedTimeNow = 0;
extern int g_httpPort;
#define DEFAULT_FRAME_TIME 5


void strcat_safe_test(){
	char tmpA[16];
	char tmpB[16];
	char buff[128];
	int res0, res1, res2, res3, res4, res5;
	tmpA[0] = 0;
	res0 = strcat_safe(tmpA,"Test1",sizeof(tmpA));
	res1 = strcat_safe(tmpA," ",sizeof(tmpA));
	res2 = strcat_safe(tmpA,"is now processing",sizeof(tmpA));
	res3 = strcat_safe(tmpA," very long string",sizeof(tmpA));
	res4 = strcat_safe(tmpA," and it",sizeof(tmpA));
	res5 = strcat_safe(tmpA," and it",sizeof(tmpA));
	tmpB[0] = 0;
	res0 = strcat_safe(tmpB,"Test1",sizeof(tmpB));
	res1 = strcat_safe(tmpB," ",sizeof(tmpB));
	res2 = strcat_safe(tmpB,"is now processing",sizeof(tmpB));
	res3 = strcat_safe(tmpB," very long string",sizeof(tmpB));
	res4 = strcat_safe(tmpB," and it",sizeof(tmpB));
	res5 = strcat_safe(tmpB," and it",sizeof(tmpB));

	urldecode2_safe(buff,"qqqqqq%40qqqq",sizeof(buff));
	urldecode2_safe(buff,"qqqqqq%40qqqq",sizeof(buff));


}

void Sim_RunFrame(int frameTime) {
	//printf("Sim_RunFrame: frametime %i\n", frameTime);
	win_frameNum++;
	// this time counter is simulated, I need this for unit tests to work
	g_simulatedTimeNow += frameTime;
	accum_time += frameTime;
	QuickTick(0);
	WIN_RunMQTTFrame();
	HTTPServer_RunQuickTick();
	if (accum_time > 1000) {
		accum_time -= 1000;
		Main_OnEverySecond();
	}
}
void Sim_RunMiliseconds(int ms, bool bApplyRealtimeWait) {
	while (ms > 0) {
		if (bApplyRealtimeWait) {
			Sleep(DEFAULT_FRAME_TIME);
		}
		Sim_RunFrame(DEFAULT_FRAME_TIME);
		ms -= DEFAULT_FRAME_TIME;
	}
}
void Sim_RunSeconds(float f, bool bApplyRealtimeWait) {
	int ms = (int)(f * 1000);
	Sim_RunMiliseconds(ms, bApplyRealtimeWait);
}
void Sim_RunFrames(int n, bool bApplyRealtimeWait) {
	int i;

	for (i = 0; i < n; i++) {
		if (bApplyRealtimeWait) {
			Sleep(DEFAULT_FRAME_TIME);
		}
		Sim_RunFrame(DEFAULT_FRAME_TIME);
	}
}

bool bObkStarted = false;
void SIM_Hack_ClearSimulatedPinRoles();


void SIM_ClearOBK(const char *flashPath) {
	if (bObkStarted) {
		DRV_ShutdownAllDrivers();
#if ENABLE_LITTLEFS
		release_lfs();
#endif
		SIM_Hack_ClearSimulatedPinRoles();
		WIN_ResetMQTT();
		UART_ResetForSimulator();
		CMD_ExecuteCommand("clearAll", 0);
		CMD_ExecuteCommand("led_expoMode", 0);
		// LOG deinit after main init so commands will be re-added
		LOG_DeInit();
	}
	if (flashPath) {
		SIM_SetupFlashFileReading(flashPath);
	}
	bObkStarted = true;
	Main_Init();
}
void Win_DoUnitTests() {
	Test_TuyaMCU_Boolean();
	Test_TuyaMCU_DP22();


	Test_Expressions_RunTests_Braces();
	Test_Expressions_RunTests_Basic();
	//Test_Enums();
	Test_Backlog();
	Test_DoorSensor();
	Test_WS2812B();
	Test_Command_If_Else();
	Test_MQTT();
	Test_ChargeLimitDriver();
	// this is slowest
	Test_TuyaMCU_Basic();
	Test_TuyaMCU_Mult();
	Test_TuyaMCU_RawAccess();
	Test_Battery();
	Test_TuyaMCU_BatteryPowered();
	Test_JSON_Lib();
	Test_MQTT_Get_LED_EnableAll();
	Test_MQTT_Get_Relay();
	Test_Commands_Startup();
	Test_IF_Inside_Backlog();
	Test_WaitFor();
	Test_TwoPWMsOneChannel();
	Test_ClockEvents();
	Test_HassDiscovery_Base();
	Test_HassDiscovery();
	Test_HassDiscovery_Ext();
	Test_Role_ToggleAll_2();
	Test_Demo_ButtonToggleGroup();
	Test_Demo_ButtonScrollingChannelValues();
	Test_CFG_Via_HTTP();
	Test_Commands_Calendar();
	Test_Commands_Generic();
	Test_Demo_SimpleShuttersScript();
	Test_Role_ToggleAll();
	Test_Demo_FanCyclingRelays();
	Test_Demo_MapFanSpeedToRelays();
	Test_MapRanges();
	Test_Demo_ExclusiveRelays();
	Test_MultiplePinsOnChannel();
	Test_Flags();
	Test_DHT();
	Test_EnergyMeter();
	Test_Tasmota();
	Test_NTP();
	Test_NTP_SunsetSunrise();
	Test_HTTP_Client();
	Test_ExpandConstant();
	Test_ChangeHandlers_MQTT();
	Test_ChangeHandlers();
	Test_ChangeHandlers_EnsureThatChannelVariableIsExpandedAtHandlerRunTime();
	Test_RepeatingEvents();
	Test_ButtonEvents();
	Test_Commands_Alias();
	Test_Demo_SignAndValue();
	Test_LEDDriver();
	Test_LFS();
	Test_Scripting();
	Test_Commands_Channels();
	Test_Command_If();
	Test_Tokenizer();
	Test_Http();
	Test_Http_LED();
	Test_DeviceGroups();





	// Just to be sure
	// Must be last step
	// reset whole device
	SIM_ClearOBK(0);
}
long g_delta;
float SIM_GetDeltaTimeSeconds() {
	return g_delta * 0.001f;
}
long start_time = 0;
bool bStartTimeSet = false;
long SIM_GetTime() {
	long cur = timeGetTime();
	if (bStartTimeSet == false) {
		start_time = cur;
		bStartTimeSet = true;
	}
	return cur - start_time;
}
// this time counter is simulated, I need this for unit tests to work
int rtos_get_time() {
	return g_simulatedTimeNow;
}
int g_bDoingUnitTestsNow = 0;

#include "sim/sim_public.h"

int SelfTest_GetNumErrors();
extern int g_selfTestsMode;

int __cdecl main(int argc, char **argv)
{

	// clear debug data
	if (1) {
		FILE *f = fopen("sim_lastPublishes.txt", "wb");
		if (f != 0) {
			fprintf(f, "\n\n");
			fclose(f);
		}
	}
	if (argc > 1) {
		int value;

		for (int i = 1; i < argc; i++) {
			if (argv[i][0] == '-') {
				if (wal_strnicmp(argv[i] + 1, "port", 4) == 0) {
					i++;

					if (i < argc && sscanf(argv[i], "%d", &value) == 1) {
						g_httpPort = value;
					}
				} else if (wal_strnicmp(argv[i] + 1, "w", 1) == 0) {
					i++;

					if (i < argc && sscanf(argv[i], "%d", &value) == 1) {
						SIM_SetWindowW(value);
					}
				}
				else if (wal_strnicmp(argv[i] + 1, "h", 1) == 0) {
					i++;

					if (i < argc && sscanf(argv[i], "%d", &value) == 1) {
						SIM_SetWindowH(value);
					}
				}
				else if (wal_strnicmp(argv[i] + 1, "runUnitTests", 12) == 0) {
					i++;

					if (i < argc && sscanf(argv[i], "%d", &value) == 1) {
						// 0 = don't run, 1 = run with system pause, 2 - run without system pause
						g_selfTestsMode = value;
					}
				}
			}
		}
	}

    WSADATA wsaData;
    int iResult;

#if 0
	int maxTest = 100;
	for (int i = 0; i <= maxTest; i++) {
		float frac = (float)i / (float)(maxTest);
		float in = frac;
		float res = LED_BrightnessMapping(255, in);
		printf("Brightness %f with color %f gives %f\n", in, 255.0f, res);
	}
#endif
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }
	printf("sizeof(short) = %d\n", (int)sizeof(short));
	printf("sizeof(int) = %d\n", (int)sizeof(int));
	printf("sizeof(long) = %d\n", (int)sizeof(long));
	printf("sizeof(unsigned long) = %d\n", (int)sizeof(unsigned long));
	printf("sizeof(float) = %d\n", (int)sizeof(float));
	printf("sizeof(double) = %d\n", (int)sizeof(double));
	printf("sizeof(long double) = %d\n", (int)sizeof(long double));
	printf("sizeof(led_corr_t) = %d\n", (int)sizeof(led_corr_t));
	
	if (sizeof(FLASH_VARS_STRUCTURE) != MAGIC_FLASHVARS_SIZE) {
		printf("sizeof(FLASH_VARS_STRUCTURE) != MAGIC_FLASHVARS_SIZE!: %i\n", sizeof(FLASH_VARS_STRUCTURE));
		system("pause");
	}
	if (sizeof(ledRemap_t) != MAGIC_LED_REMAP_SIZE) {
		printf("sizeof(ledRemap_t) != MAGIC_LED_REMAP_SIZE!: %i\n", sizeof(ledRemap_t));
		system("pause");
	}
	if (sizeof(led_corr_t) != MAGIC_LED_CORR_SIZE) {
		printf("sizeof(led_corr_t) != MAGIC_LED_CORR_SIZE!: %i\n", sizeof(led_corr_t));
		system("pause");
	}
	//printf("Offset MQTT Group: %i", OFFSETOF(mainConfig_t, mqtt_group));
	if (sizeof(mainConfig_t) != MAGIC_CONFIG_SIZE_V4) {
		printf("sizeof(mainConfig_t) != MAGIC_CONFIG_SIZE!: %i\n", sizeof(mainConfig_t));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, staticIP) != 0x00000527) {
		printf("OFFSETOF(mainConfig_t, staticIP) != 0x00000527z: %i\n", OFFSETOF(mainConfig_t, staticIP));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, wifi_ssid) != 0x00000014) {
		printf("OFFSETOF(mainConfig_t, wifi_ssid) != 0x00000014: %i\n", OFFSETOF(mainConfig_t, wifi_ssid));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, wifi_pass) != 0x00000054) {
		printf("OFFSETOF(mainConfig_t, wifi_pass) != 0x00000054: %i\n", OFFSETOF(mainConfig_t, wifi_pass));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, unused_fill) != 0x0000045E) {
		printf("OFFSETOF(mainConfig_t, unused_fill) != 0x0000045E: %i\n", OFFSETOF(mainConfig_t, unused_fill));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, buttonHoldRepeat) != 0x000004BA) {
		printf("OFFSETOF(mainConfig_t, buttonHoldRepeat) != 0x000004BA: %i\n", OFFSETOF(mainConfig_t, buttonHoldRepeat));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, shortDeviceName) != 0x2DE) {
		printf("OFFSETOF(mainConfig_t, shortDeviceName) != 0x2DE: %i\n", OFFSETOF(mainConfig_t, shortDeviceName));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, timeRequiredToMarkBootSuccessfull) != 0x00000597) {
		printf("OFFSETOF(mainConfig_t, timeRequiredToMarkBootSuccessfull) != 0x00000597: %i\n", OFFSETOF(mainConfig_t, timeRequiredToMarkBootSuccessfull));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, dgr_sendFlags) != 0x00000460) {
		printf("OFFSETOF(mainConfig_t, dgr_sendFlags) != 0x00000460: %i\n", OFFSETOF(mainConfig_t, dgr_sendFlags));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, mqtt_host) != 0x00000094) {
		printf("OFFSETOF(mainConfig_t, mqtt_host) != 0x00000094: %i\n", OFFSETOF(mainConfig_t, mqtt_host));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, mqtt_userName) != 0x1D4) {
		printf("OFFSETOF(mainConfig_t, mqtt_userName) != 0x1D4: %i\n", OFFSETOF(mainConfig_t, mqtt_userName));
		system("pause");
}
	if (OFFSETOF(mainConfig_t, mqtt_port) != 0x294) {
		printf("OFFSETOF(mainConfig_t, mqtt_port) != 0x294: %i\n", OFFSETOF(mainConfig_t, mqtt_port));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, mqtt_group) != 0x00000554) {
		printf("OFFSETOF(mainConfig_t, mqtt_group) != 0x00000554: %i\n", OFFSETOF(mainConfig_t, mqtt_group));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, LFS_Size) != 0x000004BC) {
		printf("OFFSETOF(mainConfig_t, LFS_Size) != 0x000004BC: %i\n", OFFSETOF(mainConfig_t, LFS_Size));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, ping_host) != 0x000005A0) {
		printf("OFFSETOF(mainConfig_t, ping_host) != 0x000005A0: %i\n", OFFSETOF(mainConfig_t, ping_host));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, buttonShortPress) != 0x000004B8) {
		printf("OFFSETOF(mainConfig_t, buttonShortPress) != 0x000004B8: %i\n", OFFSETOF(mainConfig_t, buttonShortPress));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, pins) != 0x0000033E) {
		printf("OFFSETOF(mainConfig_t, pins) != 0x0000033E: %i\n", OFFSETOF(mainConfig_t, pins));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, version) != 0x00000004) {
		printf("OFFSETOF(mainConfig_t, version) != 0x00000004: %i\n", OFFSETOF(mainConfig_t, version));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, initCommandLine) != 0x000005E0) {
		printf("OFFSETOF(mainConfig_t, initCommandLine) != 0x000005E0: %i\n", OFFSETOF(mainConfig_t, initCommandLine));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, wifi_ssid2) != 0x00000C00) {
		printf("OFFSETOF(mainConfig_t, wifi_ssid2) != 0x00000C00: %i\n", OFFSETOF(mainConfig_t, wifi_ssid2));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, wifi_pass2) != 0x00000C40) {
		printf("OFFSETOF(mainConfig_t, wifi_pass2) != 0x00000C40: %i\n", OFFSETOF(mainConfig_t, wifi_pass2));
		system("pause");
	}
	if (OFFSETOF(mainConfig_t, unused) != 0x00000CA5) {
		printf("OFFSETOF(mainConfig_t, unused) != 0x00000CA5: %i\n", OFFSETOF(mainConfig_t, unused));
		system("pause");
	}
	// Test expansion
	//CMD_UART_Send_Hex(0,0,"FFAA$CH1$BB",0);

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

	if (g_selfTestsMode) {
		g_bDoingUnitTestsNow = 1;
		SIM_ClearOBK(0);
		// let things warm up a little
		Sim_RunFrames(50, false);
		// run tests
		Win_DoUnitTests();
		Sim_RunFrames(50, false);
		g_bDoingUnitTestsNow = 0;
		if (g_selfTestsMode > 1) {
			return SelfTest_GetNumErrors();
		}
	}


	SIM_CreateWindow(argc, argv);
#if 1
	CMD_ExecuteCommand("MQTTHost 192.168.0.113", 0);
	CMD_ExecuteCommand("MqttPassword ma1oovoo0pooTie7koa8Eiwae9vohth1vool8ekaej8Voohi7beif5uMuph9Diex", 0);
	CMD_ExecuteCommand("MqttClient WindowsOBK", 0);
	CMD_ExecuteCommand("MqttUser homeassistant", 0);
#else
	CMD_ExecuteCommand("MQTTHost 192.168.0.118", 0);
	CMD_ExecuteCommand("MqttPassword Test1", 0);
	CMD_ExecuteCommand("MqttClient WindowsOBK", 0);
	CMD_ExecuteCommand("MqttUser homeassistant", 0);
#endif
	CMD_ExecuteCommand("reboot", 0);
	//CMD_ExecuteCommand("addRepeatingEvent 1 -1 backlog addChannel 1 1; publishInt myTestTopic $CH1", 0);
	
	if (false) {
		while (1) {
			Sleep(DEFAULT_FRAME_TIME);
			Sim_RunFrame(DEFAULT_FRAME_TIME);
			SIM_RunWindow();
		}
	}
	else {
		long prev_time = SIM_GetTime();
		while (1) {
			long cur_time = SIM_GetTime();
			g_delta = cur_time - prev_time;
			if (g_delta <= 0)
				continue;
			Sim_RunFrame(g_delta);
			SIM_RunWindow();
			prev_time = cur_time;
		}
	}
	return 0;
}

// initialise OTA flash starting at startaddr
int init_ota(unsigned int startaddr) {
	return 0;
}

// add any length of data to OTA
void add_otadata(unsigned char *data, int len) {
	return;
}

// finalise OTA flash (write last sector if incomplete)
void close_ota() {
	return;
}

void otarequest(const char *urlin) {
	return;
}

int ota_progress() {
	return 0;
}
int ota_total_bytes() {
	return 0;
}

#endif

