#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "drv_public.h"
#include "drv_local.h"
#include "drv_deviceclock.h"
#include "../libraries/obktime/obktime.h"	// for time functions


static uint8_t phys_min;
static uint8_t phys_hour;
static uint8_t phys_sec;
static uint8_t phys_resolution;

void PulseClock_onEverySec() {
    TimeComponents tc;
    time_t ntpTime;
    char str[64];

    ntpTime=(time_t)TIME_GetCurrentTime();
    tc=calculateComponents((uint32_t)ntpTime);

    // is device time set ?
    if (tc.year < 2026)
        return;

    if (phys_min == 0xff || phys_hour==0xff)
    {
        phys_min=tc.minute;
        phys_hour=tc.hour;
    }

    if (tc.minute != phys_min || tc.hour != phys_hour)
    {
        str[0]=0;
        sprintf(str, "PulseClock: New time: %i:%i:%i, Phys time: %i:%i:%i\n", tc.hour, tc.minute, tc.second, phys_hour, phys_min, phys_sec);

        addLogAdv(LOG_INFO, LOG_FEATURE_DRV, str);
    }

    if (tc.minute != phys_min || tc.hour != phys_hour)
    {
        addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: Advance minute");
        phys_min ++;
        if (phys_min > 59)
        {
            phys_min=0;
            phys_hour++;
            if (phys_hour > 23)
            {
                phys_hour=0;
            }
        }
    }
}

void PulseClock_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState) {
	if (bPreState)
		return;

    hprintf255(request, "<h5>PulseClock: phystime=%02i:%02i:%02i</h5>", phys_hour, phys_min, phys_sec);
}


static commandResult_t Cmd_SetPhysTime(const void* context, const char* cmd, const char* args, int cmdFlags) {
    int arg;
    char str[64];
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	phys_hour = Tokenizer_GetArgInteger(0);
	phys_min = Tokenizer_GetArgInteger(1);
	phys_sec = Tokenizer_GetArgInteger(2);

    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: PhysTime updated\n");
    return CMD_RES_OK;
}


void PulseClock_init() {
	phys_resolution = Tokenizer_GetArgIntegerDefault(1, 60);
    phys_min=phys_hour=0xff;
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: init, resolution=%i\n", phys_resolution);
	CMD_RegisterCommand("PulseClock_SetPhysTime", Cmd_SetPhysTime, NULL);
}

