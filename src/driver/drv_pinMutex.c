// based on info from here:
// https://www.elektroda.com/rtvforum/viewtopic.php?p=21593497#21593497
#include "../obk_config.h"

#if ENABLE_DRIVER_PINMUTEX

#include <math.h>
#include <stdint.h>

#include "drv_local.h"
#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"
#include "drv_uart.h"

/*
startDriver PinMutex
// setMutex MutexIndex ChannelIndex PinUp PinDown DelayMs
setMutex 0 1 10 11 50
// now, if you set channel 1 to 0, both pins are low.
// If you set to 1, Up goes 1, if you set to 2, down goes 1
// but there is guaranted 50ms dead time
*/

typedef struct pinMutex_s {
	int pinUp, pinDown;
	int interval;
	int channel;
} pinMutex_t;


#define MAX_PINMUTEX 4
pinMutex_t pms[MAX_PINMUTEX];

void DRV_PinMutex_RunFrame() {

	for (int i = 0; i < MAX_PINMUTEX; i++) {
		int desired = Channel_Get(pms[i].channel);

	}

}

void DRV_PinMutex_Init() {


}

#endif
