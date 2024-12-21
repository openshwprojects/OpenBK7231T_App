// NOTE: qqq
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"

typedef  {
	SHUTTER_CLOSED,
	SHUTTER_OPENING_START,
	SHUTTER_OPENING,
	SHUTTER_CLOSING,
	SHUTTER_CLOSING_STOP,
} shuttersState_t;

// default values
static float time_open_start = 1.0f;
static float time_open = 10.0f;
static float time_close = 5.0f;
static float time_close_finish = 0.5f;
static float percentage;
static shuttersState_t state;

void Shutters_RunQuickTick() {


}
void Shutters_Init() {


}
