#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "drv_tuyaMCU.h"
#include "../i2c/drv_i2c_public.h"
#include "drv_ntp.h"
#include "../httpserver/new_http.h"


typedef struct driver_s {
	const char *name;
	void (*initFunc)();
	void (*onEverySecond)();
	void (*appendInformationToHTTPIndexPage)(http_request_t *request);
	void (*runQuickTick)();
	void (*stopFunc)();
	void (*onChannelChanged)(int ch, int val);
	bool bLoaded;
} driver_t;

// startDriver BL0937
static driver_t g_drivers[] = {
	{ "TuyaMCU", TuyaMCU_Init, TuyaMCU_RunFrame, NULL, NULL, NULL, NULL, false },
	{ "NTP", NTP_Init, NTP_OnEverySecond, NULL, NULL, NULL, NULL, false },
	{ "I2C", DRV_I2C_Init, DRV_I2C_EverySecond, NULL, NULL, NULL, NULL, false },
	{ "BL0942", BL0942_Init, BL0942_RunFrame, BL09XX_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
	{ "BL0937", BL0937_Init, BL0937_RunFrame, BL09XX_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
	{ "CSE7766", CSE7766_Init, CSE7766_RunFrame, BL09XX_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#if PLATFORM_BEKEN
	{ "DGR", DRV_DGR_Init, NULL, NULL, DRV_DGR_RunFrame, DRV_DGR_Shutdown, DRV_DGR_OnChannelChanged, false },
#endif
	{ "SM2135", SM2135_Init, SM2135_RunFrame, NULL, NULL, NULL, SM2135_OnChannelChanged, false },
};

static int g_numDrivers = sizeof(g_drivers)/sizeof(g_drivers[0]);

bool DRV_IsRunning(const char *name) {
	int i;

	for(i = 0; i < g_numDrivers; i++) {
		if(g_drivers[i].bLoaded) {
			if(!stricmp(name,g_drivers[i].name)) {
				return true;
			}
		}
	}
	return false;
}

static SemaphoreHandle_t g_mutex = 0;

bool DRV_Mutex_Take(int del) {
	int taken;

	if(g_mutex == 0)
	{
		g_mutex = xSemaphoreCreateMutex( );
	}
    taken = xSemaphoreTake( g_mutex, del );
    if (taken == pdTRUE) {
		return true;
	}
	return false;
}
void DRV_Mutex_Free() {
	xSemaphoreGive( g_mutex );
}
void DRV_OnEverySecond() {
	int i;

	if(DRV_Mutex_Take(100)==false) {
		return;
	}
	for(i = 0; i < g_numDrivers; i++) {
		if(g_drivers[i].bLoaded) {
			if(g_drivers[i].onEverySecond != 0) {
				g_drivers[i].onEverySecond();
			}
		}
	}
	DRV_Mutex_Free();
}
void DRV_RunQuickTick() {
	int i;

	if(DRV_Mutex_Take(0)==false) {
		return;
	}
	for(i = 0; i < g_numDrivers; i++) {
		if(g_drivers[i].bLoaded) {
			if(g_drivers[i].runQuickTick != 0) {
				g_drivers[i].runQuickTick();
			}
		}
	}
	DRV_Mutex_Free();
}
void DRV_OnChannelChanged(int channel,int iVal) {
	int i;

	if(DRV_Mutex_Take(100)==false) {
		return;
	}
	for(i = 0; i < g_numDrivers; i++) {
		if(g_drivers[i].bLoaded){
			if(g_drivers[i].onChannelChanged != 0) {
				g_drivers[i].onChannelChanged(channel,iVal);
			}
		}
	}
	DRV_Mutex_Free();
}
void DRV_StopDriver(const char *name) {
	int i;

	if(DRV_Mutex_Take(100)==false) {
		return;
	}
	for(i = 0; i < g_numDrivers; i++) {
		if(!stricmp(g_drivers[i].name,name)) {
			if(g_drivers[i].bLoaded){
				if(g_drivers[i].stopFunc != 0) {
					g_drivers[i].stopFunc();
				}
				g_drivers[i].bLoaded = 0;
				addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Drv %s has been stopped.\n",name);
				break ;
			} else {
				addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Drv %s is not running.\n",name);
				break ;
			}
		}
	}
	DRV_Mutex_Free();
}
void DRV_StartDriver(const char *name) {
	int i;
	int bStarted;

	if(DRV_Mutex_Take(100)==false) {
		return;
	}
	bStarted = 0;
	for(i = 0; i < g_numDrivers; i++) {
		if(!stricmp(g_drivers[i].name,name)) {
			if(g_drivers[i].bLoaded){
				addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Drv %s is already loaded.\n",name);
				bStarted = 1;
				break ;

			} else {
				g_drivers[i].initFunc();
				g_drivers[i].bLoaded = true;
				addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Started %s.\n",name);
				bStarted = 1;
				break ;
			}
		}
	}
	if(!bStarted) {
		addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Driver %s is not known in this build.\n",name);
		addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Available drivers: ");
		for(i = 0; i < g_numDrivers; i++) {
			if(i == 0) {
				addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"%s",g_drivers[i].name);
			} else {
				addLogAdv(LOG_INFO, LOG_FEATURE_NTP,", %s",g_drivers[i].name);
			}
		}
	}
	DRV_Mutex_Free();
}
// startDriver DGR
// startDriver BL0942
// startDriver BL0937
static int DRV_Start(const void *context, const char *cmd, const char *args, int cmdFlags) {

	DRV_StartDriver(args);

	return 1;
}
void DRV_Generic_Init() {

	CMD_RegisterCommand("startDriver","",DRV_Start, "Starts driver", NULL);

}
void DRV_AppendInformationToHTTPIndexPage(http_request_t *request) {
	int i;
	int c_active = 0;

	if(DRV_Mutex_Take(100)==false) {
		return;
	}
	for(i = 0; i < g_numDrivers; i++) {
		if(g_drivers[i].bLoaded){
			c_active++;
			if(g_drivers[i].appendInformationToHTTPIndexPage) {
				g_drivers[i].appendInformationToHTTPIndexPage(request);
			}
		}
	}
	DRV_Mutex_Free();

    hprintf128(request,"<h5>%i drivers active, total %i</h5>",c_active,g_numDrivers);
}


