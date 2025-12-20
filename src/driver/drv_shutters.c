#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../quicktick.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"
#include "drv_ntp.h"
#include "drv_deviceclock.h"

typedef enum shutterState_e {
	SHUTTER_OPEN,
	SHUTTER_CLOSED,
	SHUTTER_OPENING,
	SHUTTER_CLOSING,

} shutterState_t;
typedef struct shutter_s {
	int index;
	// current fraction
	shutterState_t state;
	float frac;
	float targetFrac;
	float openTimeSeconds;
	float closeTimeSeconds;
} shutter_t;

shutter_t *g_shutters = 0;

shutter_t *GetForIndex(int i) {
	shutter_t *s = g_shutters;
	while (s && i--) s = s->next;
	return s;
}
// startDriver Shutters
void DRV_Shutters_AddToHtmlPage(http_request_t *request, int bPreState) {
	if (bPreState)
		return;

	shutter_t *s = g_shutters;
	while (s) {
		int percent = (s->frac < 0.0f) ? -1 : (int)(s->frac * 100.0f + 0.5f);

		hprintf255(request, "<hr>");
		hprintf255(request, "<h5>Shutter %i</h5>", s->index);

		if (percent >= 0)
			hprintf255(request, "<div>Position: %i%%</div>", percent);
		else
			hprintf255(request, "<div>Position: unknown</div>");

		hprintf255(request,
			"<div>"
			"<a class='btn' href='?cmd=ShutterMoveTo+%i+1'>Open</a> "
			"<a class='btn' href='?cmd=ShutterMoveTo+%i+%.2f'>Stop</a> "
			"<a class='btn' href='?cmd=ShutterMoveTo+%i+0'>Close</a>"
			"</div>",
			s->index,
			s->index, (s->frac >= 0.0f) ? s->frac : 0.0f,
			s->index
		);

		hprintf255(request,
			"<form method='get'>"
			"<input type='hidden' name='cmd' value='ShutterMoveTo %i'>"
			"<input type='range' min='0' max='100' value='%i' "
			"onchange='this.form.cmd.value=\"ShutterMoveTo %i \" + (this.value/100); this.form.submit();'>"
			"</form>",
			s->index,
			(percent >= 0) ? percent : 0,
			s->index
		);

		s = s->next;
	}
}
static void Shutter_SetPin(shutter_t *s, int role, int val) {
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.channels[i] == s->index
			&& g_cfg.pins.roles[i] == role) {
			HAL_PIN_SetOutputValue(i, val);
		}
	}
}
static void Shutter_SetPins(shutter_t *s, shutterState_e dir) {
	// turn all off
	Shutter_SetPin(s, IOR_ShutterA, 0);
	Shutter_SetPin(s, IOR_ShutterB, 0);
	// apply new state
	switch (dir) {
	case SHUTTER_OPENING:
		Shutter_SetPin(s, IOR_ShutterA, 1);
		break;
	case SHUTTER_CLOSING:
		Shutter_SetPin(s, IOR_ShutterB, 1);
		break;
	default:
		break;
	}
}
static void Shutter_Open(shutter_t *s) {
	Shutter_SetPins(s, SHUTTER_OPENING);
	s->state = SHUTTER_OPENING;
}

static void Shutter_Close(shutter_t *s) {
	Shutter_SetPins(s, SHUTTER_CLOSING);
	s->state = SHUTTER_CLOSING;
}

static void Shutter_Stop(shutter_t *s) {
	Shutter_SetPins(s, SHUTTER_CLOSED);
	if (s->frac <= 0.0f) {
		s->frac = 0.0f;
		s->state = SHUTTER_CLOSED;
	}
	else if (s->frac >= 1.0f) {
		s->frac = 1.0f;
		s->state = SHUTTER_OPEN;
	}
	else {
		s->state = SHUTTER_UNKNOWN;
	}
}
static commandResult_t CMD_Shutter_MoveTo(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, 0);

	int index = Tokenizer_GetArgInteger(0);
	float frac = Tokenizer_GetArgFloat(1);

	if (frac < 0.0f) frac = 0.0f;
	if (frac > 1.0f) frac = 1.0f;

	shutter_t *s = GetForIndex(index);
	if (!s)
		return CMD_RES_BAD_ARGUMENT;

	s->targetFrac = frac;

	if (s->frac < 0.0f)
		s->frac = 0.0f;

	if (frac > s->frac)
		Shutter_Open(s);
	else if (frac < s->frac)
		Shutter_Close(s);

	return CMD_RES_OK;
}
void DRV_Shutter_Tick(shutter_t *s) {
	float dt = g_deltaTimeMS * 0.001f;

	switch (s->state) {
	case SHUTTER_OPENING:
		s->frac += dt / s->openTimeSeconds;
		if (s->frac >= s->targetFrac)
			Shutter_Stop(s);
		break;

	case SHUTTER_CLOSING:
		s->frac -= dt / s->closeTimeSeconds;
		if (s->frac <= s->targetFrac)
			Shutter_Stop(s);
		break;

	default:
		break;
	}
}
void DRV_Shutters_RunQuickTick() {
	shutter_t *s = g_shutters;
	while (s) {
		DRV_Shutter_Tick(s);
		s = s->next;
	}
}


void DRV_Shutters_Init() {


}

