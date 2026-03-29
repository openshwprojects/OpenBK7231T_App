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


uint8_t phys_min;
uint8_t phys_hour;

void PulseClock_onEverySec() {
    TimeComponents tc;
    time_t ntpTime;
    char str[64];

    ntpTime=(time_t)TIME_GetCurrentTime();
    tc=calculateComponents((uint32_t)ntpTime);

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
        sprintf(str, "New time: %i:%i, Phys time: %i:%i\n", tc.hour, tc.minute, phys_hour, phys_min);

        addLogAdv(LOG_INFO, LOG_FEATURE_DRV, str);
    }

    if (tc.minute != phys_min || tc.hour != phys_hour)
    {
        addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Advance minute");
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

void PulseClock_AppendInformationToHTTPIndexPage(http_request_t* request) {
   hprintf255(request, "<h4>PulseClock phystime=%02i:%02i</h4>", phys_hour, phys_min);
}

void PulseClock_init() {
    phys_min=phys_hour=0xff;
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Pulse Clock Init.\n");
}

