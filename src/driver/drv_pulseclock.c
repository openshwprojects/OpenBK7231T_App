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

char *my_strcat(char *p, const char *s) {
	strcat(p, s);
	return p + strlen(s);
}
char *add_padded(char *o, int i) {
	if (i < 10) {
		*o = '0';
		o++;
	}
	sprintf(o, "%i", i);
	return o + strlen(o);
}

void PulseClock_onEverySec() {
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Pulse Clock EverySec.\n");

    TimeComponents tc;
    time_t ntpTime;
    char str[64];


    ntpTime=(time_t)TIME_GetCurrentTime();
    tc=calculateComponents((uint32_t)ntpTime);

    str[0]=0;
    sprintf(str, "%i %i\n", tc.hour, tc.minute);

    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, str);
                
}

void PulseClock_init() {
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Pulse Clock Init.\n");
}

