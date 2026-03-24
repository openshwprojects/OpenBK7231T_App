

#include "../obk_config.h"

#if ENABLE_DRIVER_PINMUTEX

#include <stdint.h>
#include "drv_local.h"
#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "../quicktick.h"
#include "../cmnds/cmd_public.h"

/*
startDriver PinMutex
// setMutex MutexIndex ChannelIndex PinUp PinDown DelayMs
setMutex 0 1 10 11 50
// now, if you set channel 1 to 0, both pins are low.
// If you set to 1, Up goes 1, if you set to 2, down goes 1
// but there is guaranted 50ms dead time
*/

// desired pin state: off, up, or down
typedef enum {
	PM_DESIRED_OFF = 0,
	PM_DESIRED_DOWN = 1,
	PM_DESIRED_UP = 2,
} pmDesired_t;

// pinMutex structure
typedef struct pinMutex_s {
	int           channel;       // which CHANNEL_Get index to watch
	int           pinUp;         // gpio pin number for "up"
	int           pinDown;       // gpio pin number for "down"
	int           deadTimeMs;    // minimum off-time when switching
	pmDesired_t   lastDesired;   // previous desired state
	int           timerMs;       // remaining dead-time countdown
} pinMutex_t;

#define MAX_PINMUTEX 4
static pinMutex_t pms[MAX_PINMUTEX];

// this must be called from the main loop at ~1 kHz (once per millisecond)
void DRV_PinMutex_RunFrame() {
	for (int i = 0; i < MAX_PINMUTEX; i++) {
		pinMutex_t *pm = &pms[i];

		// skip unused slots (both pins zero)
		if (pm->pinUp == 0 && pm->pinDown == 0) continue;

		// read desired state: 0=off, 1=up, 2=down
		int desired = CHANNEL_Get(pm->channel);

		// state changed? start dead-time
		if ((pmDesired_t)desired != pm->lastDesired) {
			// immediately turn both pins off
			HAL_PIN_SetOutputValue(pm->pinUp, 0);
			HAL_PIN_SetOutputValue(pm->pinDown, 0);
			// start delay before new direction
			if (pm->lastDesired != 0) {
				pm->timerMs = pm->deadTimeMs;
			}
			else {
				pm->timerMs = 0;
			}
			pm->lastDesired = (pmDesired_t)desired;
		}
		else if (pm->timerMs > 0) {
			// still in dead-time: count down
			pm->timerMs -= g_deltaTimeMS;
			// keep both outputs off
		}
		else {
			// dead-time expired → apply new state
			switch (pm->lastDesired) {
			case PM_DESIRED_OFF:
				HAL_PIN_SetOutputValue(pm->pinUp, 0);
				HAL_PIN_SetOutputValue(pm->pinDown, 0);
				break;
			case PM_DESIRED_UP:
				HAL_PIN_SetOutputValue(pm->pinUp, 1);
				HAL_PIN_SetOutputValue(pm->pinDown, 0);
				break;
			case PM_DESIRED_DOWN:
				HAL_PIN_SetOutputValue(pm->pinUp, 0);
				HAL_PIN_SetOutputValue(pm->pinDown, 1);
				break;
			}
		}
	}
}

// setMutex <index> <channel> <delayMs> <pinUp> <pinDown>
static commandResult_t CMD_setMutex(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);

	// expect 5 arguments
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 5) || Tokenizer_GetArgsCount() < 5) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	int idx = Tokenizer_GetArgInteger(0);
	int channel = Tokenizer_GetArgInteger(1);
	int delayMs = Tokenizer_GetArgInteger(2);
//	int pinDown = Tokenizer_GetArgInteger(3);
//	int pinUp = Tokenizer_GetArgInteger(4);
	int pinDown = Tokenizer_GetPin(3,-1);
	int pinUp = Tokenizer_GetPin(4,-1);
	if (pinDown == -1 || pinUp == -1) return CMD_RES_BAD_ARGUMENT;

	if (idx < 0 || idx >= MAX_PINMUTEX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "setMutex: index %d out of range (0..%d)", idx, MAX_PINMUTEX - 1);
		return CMD_RES_BAD_ARGUMENT;
	}
	if (delayMs < 0) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "setMutex: delay must be >= 0");
		return CMD_RES_BAD_ARGUMENT;
	}

	// initialize slot
	pms[idx].channel = channel;
	pms[idx].pinUp = pinUp;
	pms[idx].pinDown = pinDown;
	pms[idx].deadTimeMs = delayMs;
	pms[idx].lastDesired = PM_DESIRED_OFF;
	pms[idx].timerMs = 0;

	// configure gpio as outputs, start low
	HAL_PIN_Setup_Output(pinUp);
	HAL_PIN_Setup_Output(pinDown);
	HAL_PIN_SetOutputValue(pinUp, 0);
	HAL_PIN_SetOutputValue(pinDown, 0);

	addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "PinMutex[%d] = ch=%d, up=%d, down=%d, t=%dms",
		idx, channel, pinUp, pinDown, delayMs);
	return CMD_RES_OK;
}

void DRV_PinMutex_Init() {
	// mark all slots unused (both pins zero)
	for (int i = 0; i < MAX_PINMUTEX; i++) {
		pms[i].pinUp = 0;
		pms[i].pinDown = 0;
	}
	//cmddetail:{"name":"setMutex","args":"CMD_setMutex",
	//cmddetail:"descr":"pinmutex driver (potentially for shutters)",
	//cmddetail:"fn":"CMD_setMutex","file":"driver/drv_pinMutex.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("setMutex", CMD_setMutex, NULL);
}

#endif  // ENABLE_DRIVER_PINMUTEX
