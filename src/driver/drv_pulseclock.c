#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_flashVars.h"
#include "drv_public.h"
#include "drv_local.h"
#include "drv_deviceclock.h"
#include "../libraries/obktime/obktime.h"	// for time functions

#define PHYS_UNKNOWN 999999
#define PHYS_DAYSEC_FV (SPECIAL_CHANNEL_FLASHVARS_LAST-4)

static int32_t phys_daysec;
static int32_t phys_resolution;
static int32_t phys_pulseoffset;
static int32_t phys_pulsemillis;
static int32_t phys_maxsec;

int32_t HMSToDaysec(uint8_t hour, uint8_t minute, uint8_t second) {
    return (hour * 3600) + (minute * 60) + (second);
}

int32_t DaysecNormalise(int32_t daysec) {
    daysec=daysec % phys_maxsec;
    if (daysec < 0)
    {
        daysec += phys_maxsec;
    }
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

static void PulseClock_SetPin(int role, int val) {
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.channels[i] == 1
			&& g_cfg.pins.roles[i] == role) {
            HAL_PIN_Setup_Output(i);
            HAL_PIN_SetOutputValue(i, val);
		}
	}
}

static bool PulseClock_CanRev() {
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.roles[i] == IOR_PulseClock_Rev 
                || g_cfg.pins.roles[i] == IOR_PulseClock_RevPos 
                || g_cfg.pins.roles[i] == IOR_PulseClock_RevNeg
                ) {
            return true;
        }
    }
    return false;
}

static void PulseClock_SetAllOff() {
    PulseClock_SetPin(IOR_PulseClock_Fwd, 0);
    PulseClock_SetPin(IOR_PulseClock_FwdPos, 0);
    PulseClock_SetPin(IOR_PulseClock_FwdNeg, 0);
    PulseClock_SetPin(IOR_PulseClock_Rev, 0);
    PulseClock_SetPin(IOR_PulseClock_RevPos, 0);
    PulseClock_SetPin(IOR_PulseClock_RevNeg, 0);
}

void PulseClock_onEverySec() {
    TimeComponents tc;
    time_t ntpTime;
    int32_t want_daysec;

    PulseClock_SetAllOff();

    ntpTime=(time_t)TIME_GetCurrentTime();
    tc=calculateComponents((uint32_t)ntpTime);
    want_daysec=HMSToDaysec(tc.hour, tc.minute, tc.second);
    want_daysec += phys_pulseoffset;
    want_daysec -= want_daysec % phys_resolution;
    want_daysec=DaysecNormalise(want_daysec);

    // is device time set ?
    if (tc.year < 2026)
        return;

    if (phys_daysec == PHYS_UNKNOWN)
    {
        phys_daysec=want_daysec;
    }

    int32_t diff=(phys_daysec - want_daysec);
    if (diff)
    {
        addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: ntp_time=%02i:%02i:%02i, want_time=%02i:%02i:%02i, phys_time=%02i:%02i:%02i", 
                tc.hour, tc.minute, tc.second,
                DaysecToHour(want_daysec), DaysecToMinute(want_daysec), DaysecToSecond(want_daysec), 
                DaysecToHour(phys_daysec), DaysecToMinute(phys_daysec), DaysecToSecond(phys_daysec) 
               );

        if ((DaysecNormalise(diff) < phys_maxsec/2) && PulseClock_CanRev())
        {
            if ((phys_daysec / phys_resolution) % 2)
            {
                addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: Reverse (neg)");
                PulseClock_SetPin(IOR_PulseClock_RevNeg,1);
                PulseClock_SetPin(IOR_PulseClock_Rev,1);
            }
            else
            {
                addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: Reverse (pos)");
                PulseClock_SetPin(IOR_PulseClock_RevPos,1);
                PulseClock_SetPin(IOR_PulseClock_Rev,1);
            }
            phys_daysec -= phys_resolution;
        }
        else
        {
            if ((phys_daysec / phys_resolution) % 2)
            {
                addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: Forward (pos)");
                PulseClock_SetPin(IOR_PulseClock_FwdPos,1);
                PulseClock_SetPin(IOR_PulseClock_Fwd,1);
            }
            else
            {
                addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: Forward (neg)");
                PulseClock_SetPin(IOR_PulseClock_FwdNeg,1);
                PulseClock_SetPin(IOR_PulseClock_Fwd,1);
            }
            phys_daysec += phys_resolution;
        }

        rtos_delay_milliseconds(phys_pulsemillis);
        PulseClock_SetAllOff();
        phys_daysec=DaysecNormalise(phys_daysec);
//        HAL_FlashVars_SaveChannel(PHYS_DAYSEC_FV, (int)phys_daysec);
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
	phys_maxsec = Tokenizer_GetArgIntegerDefault(4, 86400/2);
//	phys_daysec=(int32_t) HAL_FlashVars_GetChannelValue(PHYS_DAYSEC_FV);
//    if (phys_daysec != DaysecNormalise(phys_daysec))
//    {
        phys_daysec=PHYS_UNKNOWN;
//    }
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "PulseClock: init, resolution=%i, pulseoffset=%i, pulsemillis=%i, maxsec=%i, phystime=%02i:%02i:%02i", 
            phys_resolution, 
            phys_pulseoffset,
            phys_pulsemillis,
            phys_maxsec,
            DaysecToHour(phys_daysec), DaysecToMinute(phys_daysec), DaysecToSecond(phys_daysec));
	CMD_RegisterCommand("PulseClock_SetPhysTime", Cmd_SetPhysTime, NULL);
}

