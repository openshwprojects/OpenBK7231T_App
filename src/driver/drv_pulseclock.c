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


static int32_t phys_daysec;
static int32_t phys_resolution;
static int32_t phys_pulseoffset;
static int32_t phys_pulsemillis;

int32_t HMSToDaysec(uint8_t hour, uint8_t minute, uint8_t second) {
    return (hour * 3600) + (minute * 60) + (second);
}

int32_t DaysecNormalise(int32_t daysec) {
    while (daysec < 0)
    {
        daysec += 86400;
    }
    daysec = daysec % 86400;
    return daysec;
}

uint8_t DaysecToHour(int32_t daysec) {
    return daysec / 3600;
}

uint8_t DaysecToMinute(int32_t daysec) {
    return (daysec % 3600) / 60;
}

uint8_t DaysecToSecond(int32_t daysec) {
    return (daysec % 60);
}

void PulseClock_onEverySec() {
    TimeComponents tc;
    time_t ntpTime;
    int32_t ntp_daysec;

    ntpTime=(time_t)TIME_GetCurrentTime();
    tc=calculateComponents((uint32_t)ntpTime);
    ntp_daysec=HMSToDaysec(tc.hour, tc.minute, tc.second);
    ntp_daysec += phys_pulseoffset;
    ntp_daysec -= ntp_daysec % phys_resolution;
    ntp_daysec=DaysecNormalise(ntp_daysec);

    // is device time set ?
    if (tc.year < 2026)
        return;

    if (phys_daysec == 0xffffffff)
    {
        phys_daysec=ntp_daysec;
    }

    if (phys_daysec != ntp_daysec)
    {
        addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: ntp_time=%02i:%02i:%02i, phys_time=%02i:%02i:%02i\n", 
                DaysecToHour(ntp_daysec), DaysecToMinute(ntp_daysec), DaysecToSecond(ntp_daysec), 
                DaysecToHour(phys_daysec), DaysecToMinute(phys_daysec), DaysecToSecond(phys_daysec) 
               );

        if ((phys_daysec / phys_resolution) % 2)
        {
            addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: Advance even tick begin");
            CHANNEL_Set(1, 1, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
            rtos_delay_milliseconds(phys_pulsemillis);
            addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: Advance even tick end");
            CHANNEL_Set(1, 0, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
        }
        else
        {
            addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: Advance odd tick begin");
            CHANNEL_Set(2, 1, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
            rtos_delay_milliseconds(phys_pulsemillis);
            addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: Advance odd tick end");
            CHANNEL_Set(2, 0, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
        }
        phys_daysec += phys_resolution;
        phys_daysec=DaysecNormalise(phys_daysec);
    }
    else
    {
        if (CHANNEL_Get(1))
        {
            CHANNEL_Set(1, 0, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
        }
        if (CHANNEL_Get(2))
        {
            CHANNEL_Set(2, 0, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT
        }
    }
}

void PulseClock_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState) {
	if (bPreState)
		return;

    hprintf255(request, "<h5>PulseClock: phystime=%02i:%02i:%02i</h5>", DaysecToHour(phys_daysec), DaysecToMinute(phys_daysec), DaysecToSecond(phys_daysec));
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
    phys_daysec -= phys_daysec % phys_resolution;

    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: PhysTime updated to %02i:%02i:%02i",DaysecToHour(phys_daysec), DaysecToMinute(phys_daysec), DaysecToSecond(phys_daysec));
    return CMD_RES_OK;
}


void PulseClock_init() {
	phys_resolution = Tokenizer_GetArgIntegerDefault(1, 60);
	phys_pulseoffset = Tokenizer_GetArgIntegerDefault(2, 0);
	phys_pulsemillis = Tokenizer_GetArgIntegerDefault(3, 500);
    phys_daysec=0xffffffff;
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: init, resolution=%i", phys_resolution);
	CMD_RegisterCommand("PulseClock_SetPhysTime", Cmd_SetPhysTime, NULL);
}

