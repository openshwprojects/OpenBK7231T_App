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


static uint32_t phys_daysec;
static uint32_t phys_resolution;

uint32_t HMSToDaysec(uint8_t hour, uint8_t minute, uint8_t second) {
    return (hour * 60 * 60) + (minute * 60) + (second);
}

uint8_t DaysecToHour(uint32_t daysec) {
    return daysec / 3600;
}

uint8_t DaysecToMinute(uint32_t daysec) {
    return (daysec % 3660) / 60;
}

uint8_t DaysecToSecond(uint32_t daysec) {
    return (daysec % 60);
}

void PulseClock_onEverySec() {
    TimeComponents tc;
    time_t ntpTime;
    char str[64];
    uint32_t ntp_daysec;

    ntpTime=(time_t)TIME_GetCurrentTime();
    tc=calculateComponents((uint32_t)ntpTime);
    ntp_daysec=HMSToDaysec(tc.hour, tc.minute, tc.second);
    ntp_daysec -= ntp_daysec % phys_resolution;

    // is device time set ?
    if (tc.year < 2026)
        return;

    if (phys_daysec == 0xffffffff)
    {
        phys_daysec=ntp_daysec;
    }

    if (phys_daysec != ntp_daysec)
    {
        str[0]=0;
        sprintf(str, "PulseClock: NTP time: %i, Phys time: %i\n", ntp_daysec, phys_daysec);

        addLogAdv(LOG_INFO, LOG_FEATURE_DRV, str);
    }

    if (phys_daysec != ntp_daysec)
    {
        addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: Advance");
        phys_daysec += phys_resolution;
        phys_daysec %= 86400;
    }
}

void PulseClock_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState) {
	if (bPreState)
		return;

    hprintf255(request, "<h5>PulseClock: phystime=%i</h5>", phys_daysec);
}


static commandResult_t Cmd_SetPhysTime(const void* context, const char* cmd, const char* args, int cmdFlags) {
    int arg;
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
    
    phys_daysec=HMSToDaysec(
            Tokenizer_GetArgInteger(0),
            Tokenizer_GetArgInteger(1),
            Tokenizer_GetArgInteger(2)
            );

    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: PhysTime updated to %i\n",phys_daysec);
    return CMD_RES_OK;
}


void PulseClock_init() {
	phys_resolution = Tokenizer_GetArgIntegerDefault(1, 60);
    phys_daysec=0xffffffff;
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: init, resolution=%i\n", phys_resolution);
	CMD_RegisterCommand("PulseClock_SetPhysTime", Cmd_SetPhysTime, NULL);
}

