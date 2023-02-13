#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "drv_tuyaMCU.h"
#include "drv_ir.h"
#include "../i2c/drv_i2c_public.h"
#include "drv_ntp.h"
#include "../httpserver/new_http.h"
#include "drv_public.h"
#include "drv_ssdp.h"

const char* sensor_mqttNames[OBK_NUM_MEASUREMENTS] = {
	"voltage",
	"current",
	"power"
};

//sensor_device_classes just so happens to be the same as sensor_mqttNames.
const char* sensor_mqtt_device_classes[OBK_NUM_MEASUREMENTS] = {
	"voltage",
	"current",
	"power"
};

const char* sensor_mqtt_device_units[OBK_NUM_MEASUREMENTS] = {
	"V",
	"A",
	"W"
};

const char* counter_mqttNames[OBK_NUM_COUNTERS] = {
	"energycounter",
	"energycounter_last_hour",
	"consumption_stats",
	"energycounter_yesterday",
	"energycounter_today",
	"energycounter_clear_date",
};

const char* counter_devClasses[OBK_NUM_COUNTERS] = {
	"energy",
	"energy",
	"",
	"energy",
	"energy",
	"timestamp"
};

typedef struct driver_s {
	const char* name;
	void (*initFunc)();
	void (*onEverySecond)();
	void (*appendInformationToHTTPIndexPage)(http_request_t* request);
	void (*runQuickTick)();
	void (*stopFunc)();
	void (*onChannelChanged)(int ch, int val);
	bool bLoaded;
} driver_t;


// startDriver BL0937
static driver_t g_drivers[] = {

#ifdef ENABLE_DRIVER_TUYAMCU
	{ "TuyaMCU",	TuyaMCU_Init,		TuyaMCU_RunFrame,			NULL, NULL, NULL, NULL, false },
	{ "tmSensor",	TuyaMCU_Sensor_Init, TuyaMCU_Sensor_RunFrame,	NULL, NULL, NULL, NULL, false },
#endif

#ifdef ENABLE_BASIC_DRIVERS
	{ "NTP",		NTP_Init,			NTP_OnEverySecond,			NTP_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
	{ "TESTPOWER",	Test_Power_Init,	 Test_Power_RunFrame,		BL09XX_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
	{ "TESTLED",	Test_LED_Driver_Init, Test_LED_Driver_RunFrame, NULL, NULL, NULL, Test_LED_Driver_OnChannelChanged, false },
	{ "HTTPButtons",	DRV_InitHTTPButtons, NULL, NULL, NULL, NULL, NULL, false },
#endif

#if ENABLE_I2C
	{ "I2C",		DRV_I2C_Init,		DRV_I2C_EverySecond,		NULL, NULL, NULL, NULL, false },
#endif

#ifdef ENABLE_DRIVER_BL0942
	{ "BL0942",		BL0942_Init,		BL0942_RunFrame,			BL09XX_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#endif

#ifdef ENABLE_DRIVER_BL0937	
	{ "BL0937",		BL0937_Init,		BL0937_RunFrame,			BL09XX_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#endif

#ifdef ENABLE_DRIVER_CSE7766
	{ "CSE7766",	CSE7766_Init,		CSE7766_RunFrame,			BL09XX_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#endif

#if PLATFORM_BEKEN	
	{ "SM16703P",	SM16703P_Init,		NULL,						NULL, NULL, NULL, NULL, false },
	{ "IR",			DRV_IR_Init,		 NULL,						NULL, DRV_IR_RunFrame, NULL, NULL, false },
#endif
#if defined(PLATFORM_BEKEN) || defined(WINDOWS)	
	{ "DDP",		DRV_DDP_Init,		NULL,						NULL, DRV_DDP_RunFrame, DRV_DDP_Shutdown, NULL, false },
	{ "SSDP",		DRV_SSDP_Init,		DRV_SSDP_RunEverySecond,	NULL, DRV_SSDP_RunQuickTick, DRV_SSDP_Shutdown, NULL, false },
	{ "Wemo",		WEMO_Init,		NULL,		WEMO_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
	{ "PWMToggler",	DRV_InitPWMToggler, NULL, DRV_Toggler_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
	{ "DGR",		DRV_DGR_Init,		DRV_DGR_RunEverySecond,		NULL, DRV_DGR_RunQuickTick, DRV_DGR_Shutdown, DRV_DGR_OnChannelChanged, false },
#endif

#ifdef ENABLE_DRIVER_LED
	{ "SM2135",		SM2135_Init,		NULL,			NULL, NULL, NULL, NULL, false },
	{ "BP5758D",	BP5758D_Init,		NULL,			NULL, NULL, NULL, NULL, false },
	{ "BP1658CJ",	BP1658CJ_Init,		NULL,			NULL, NULL, NULL, NULL, false },
	{ "SM2235",		SM2235_Init,		NULL,			NULL, NULL, NULL, NULL, false },
#endif	
#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
	{ "CHT8305",	CHT8305_Init,		CHT8305_OnEverySecond,		CHT8305_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
	{ "MAX72XX",	DRV_MAX72XX_Init,		NULL,		NULL, NULL, NULL, NULL, false },
	{ "SHT3X",	SHT3X_Init,		NULL,		SHT3X_AppendInformationToHTTPIndexPage, NULL, SHT3X_StopDriver, NULL, false },
#endif
    { "Bridge",  Bridge_driver_Init, NULL,                       NULL, Bridge_driver_QuickFrame, Bridge_driver_DeInit, Bridge_driver_OnChannelChanged, false }
};

static const int g_numDrivers = sizeof(g_drivers) / sizeof(g_drivers[0]);

bool DRV_IsRunning(const char* name) {
	int i;

	for (i = 0; i < g_numDrivers; i++) {
		if (g_drivers[i].bLoaded) {
			if (!stricmp(name, g_drivers[i].name)) {
				return true;
			}
		}
	}
	return false;
}

static SemaphoreHandle_t g_mutex = 0;

bool DRV_Mutex_Take(int del) {
	int taken;

	if (g_mutex == 0)
	{
		g_mutex = xSemaphoreCreateMutex();
	}
	taken = xSemaphoreTake(g_mutex, del);
	if (taken == pdTRUE) {
		return true;
	}
	return false;
}
void DRV_Mutex_Free() {
	xSemaphoreGive(g_mutex);
}
void DRV_OnEverySecond() {
	int i;

	if (DRV_Mutex_Take(100) == false) {
		return;
	}
	for (i = 0; i < g_numDrivers; i++) {
		if (g_drivers[i].bLoaded) {
			if (g_drivers[i].onEverySecond != 0) {
				g_drivers[i].onEverySecond();
			}
		}
	}
	DRV_Mutex_Free();
}
void DRV_RunQuickTick() {
	int i;

	if (DRV_Mutex_Take(0) == false) {
		return;
	}
	for (i = 0; i < g_numDrivers; i++) {
		if (g_drivers[i].bLoaded) {
			if (g_drivers[i].runQuickTick != 0) {
				g_drivers[i].runQuickTick();
			}
		}
	}
	DRV_Mutex_Free();
}
void DRV_OnChannelChanged(int channel, int iVal) {
	int i;

	//if(DRV_Mutex_Take(100)==false) {
	//	return;
	//}
	for (i = 0; i < g_numDrivers; i++) {
		if (g_drivers[i].bLoaded) {
			if (g_drivers[i].onChannelChanged != 0) {
				g_drivers[i].onChannelChanged(channel, iVal);
			}
		}
	}
	//DRV_Mutex_Free();
}
// right now only used by simulator
void DRV_ShutdownAllDrivers() {
	int i;
	for (i = 0; i < g_numDrivers; i++) {
		if (g_drivers[i].bLoaded) {
			DRV_StopDriver(g_drivers[i].name);
		}
	}
}
void DRV_StopDriver(const char* name) {
	int i;

	if (DRV_Mutex_Take(100) == false) {
		return;
	}
	for (i = 0; i < g_numDrivers; i++) {
		if (!stricmp(g_drivers[i].name, name)) {
			if (g_drivers[i].bLoaded) {
				if (g_drivers[i].stopFunc != 0) {
					g_drivers[i].stopFunc();
				}
				g_drivers[i].bLoaded = false;
				addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Drv %s stopped.", name);
				break;
			}
			else {
				addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Drv %s not running.", name);
				break;
			}
		}
	}
	DRV_Mutex_Free();
}
void DRV_StartDriver(const char* name) {
	int i;
	int bStarted;

	if (DRV_Mutex_Take(100) == false) {
		return;
	}
	bStarted = 0;
	for (i = 0; i < g_numDrivers; i++) {
		if (!stricmp(g_drivers[i].name, name)) {
			if (g_drivers[i].bLoaded) {
				addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Drv %s is already loaded.\n", name);
				bStarted = 1;
				break;

			}
			else {
				g_drivers[i].initFunc();
				g_drivers[i].bLoaded = true;
				addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Started %s.\n", name);
				bStarted = 1;
				break;
			}
		}
	}
	if (!bStarted) {
		addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Driver %s is not known in this build.\n", name);
		addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Available drivers: ");
		for (i = 0; i < g_numDrivers; i++) {
			if (i == 0) {
				addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "%s", g_drivers[i].name);
			}
			else {
				addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, ", %s", g_drivers[i].name);
			}
		}
	}
	DRV_Mutex_Free();
}
// startDriver DGR
// startDriver BL0942
// startDriver BL0937
static commandResult_t DRV_Start(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	DRV_StartDriver(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}
static commandResult_t DRV_Stop(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	DRV_StopDriver(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}

void DRV_Generic_Init() {
	//cmddetail:{"name":"startDriver","args":"[DriverName]",
	//cmddetail:"descr":"Starts driver",
	//cmddetail:"fn":"DRV_Start","file":"driver/drv_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("startDriver", DRV_Start, NULL);
	//cmddetail:{"name":"stopDriver","args":"[DriverName]",
	//cmddetail:"descr":"Stops driver",
	//cmddetail:"fn":"DRV_Stop","file":"driver/drv_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("stopDriver", DRV_Stop, NULL);
}
void DRV_AppendInformationToHTTPIndexPage(http_request_t* request) {
	int i, j;
	int c_active = 0;

	if (DRV_Mutex_Take(100) == false) {
		return;
	}
	for (i = 0; i < g_numDrivers; i++) {
		if (g_drivers[i].bLoaded) {
			c_active++;
			if (g_drivers[i].appendInformationToHTTPIndexPage) {
				g_drivers[i].appendInformationToHTTPIndexPage(request);
			}
		}
	}
	DRV_Mutex_Free();

	hprintf255(request, "<h5>%i drivers active", c_active);
	if (c_active > 0) {
		j = 0;// printed 0 names so far
		// generate active drivers list in (  )
		hprintf255(request, " (");
		for (i = 0; i < g_numDrivers; i++) {
			if (g_drivers[i].bLoaded) {
				// if at least one name printed, add separator
				if (j != 0) {
					hprintf255(request, ",");
				}
				hprintf255(request, g_drivers[i].name);
				// one more name printed
				j++;
			}
		}
		hprintf255(request, ")");
	}
	hprintf255(request, ", total %i</h5>", g_numDrivers);
}

bool DRV_IsMeasuringPower() {
#ifndef OBK_DISABLE_ALL_DRIVERS
	return DRV_IsRunning("BL0937") || DRV_IsRunning("BL0942") || DRV_IsRunning("CSE7766") || DRV_IsRunning("TESTPOWER");
#else
	return false;
#endif
}
