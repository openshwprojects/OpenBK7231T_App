#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_bl0942.h"
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
	bool bLoaded;
} driver_t;

static driver_t g_drivers[] = {
	{ "TuyaMCU", TuyaMCU_Init, TuyaMCU_RunFrame, NULL, false }, 
	{ "NTP", NTP_Init, NTP_OnEverySecond, NULL, false }, 
	{ "I2C", DRV_I2C_Init, DRV_I2C_EverySecond, NULL, false }, 
	{ "BL0942", BL0942_Init, BL0942_RunFrame, BL0942_AppendInformationToHTTPIndexPage, false }, 
};
static int g_numDrivers = sizeof(g_drivers)/sizeof(g_drivers[0]);

void DRV_OnEverySecond() {
	int i;

	for(i = 0; i < g_numDrivers; i++) {
		if(g_drivers[i].bLoaded) {
			if(g_drivers[i].onEverySecond != 0) {
				g_drivers[i].onEverySecond();
			}
		}
	}
}
// startDriver BL0942
int DRV_Start(const void *context, const char *cmd, const char *args) {
	int i;
	const char *name;

	name = args;

	for(i = 0; i < g_numDrivers; i++) {
		if(!stricmp(g_drivers[i].name,name)) {
			if(g_drivers[i].bLoaded){ 
				addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Drv %s is already loaded.\n",name);
				return 1;

			} else {
				g_drivers[i].initFunc();
				g_drivers[i].bLoaded = true;
				addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Started %s.\n",name);
				return 1;
			}
		}
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Driver %s is not known in this build.\n",name);
	return 1;
}
void DRV_Generic_Init() {

	CMD_RegisterCommand("startDriver","",DRV_Start, "Starts driver", NULL);

}
void DRV_AppendInformationToHTTPIndexPage(http_request_t *request) {
	int i;
	int c_active = 0;

	for(i = 0; i < g_numDrivers; i++) {
		if(g_drivers[i].bLoaded){ 
			c_active++;
			if(g_drivers[i].appendInformationToHTTPIndexPage) {
				g_drivers[i].appendInformationToHTTPIndexPage(request);
			}
		}
	}
	
    hprintf128(request,"<h5>%i drivers active, total %i</h5>",c_active,g_numDrivers);
}


