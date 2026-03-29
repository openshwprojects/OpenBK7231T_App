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
//#include <time.h>
#include "../libraries/obktime/obktime.h"	// for time functions


void PulseClock_onEverySec() {
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Pulse Clock EverySec.\n");
}

void PulseClock_init() {
    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Pulse Clock Init.\n");
}

