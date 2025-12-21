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

typedef enum shutterState_t {
	SHUTTER_OPEN,
	SHUTTER_CLOSED,
	SHUTTER_OPENING,
	SHUTTER_CLOSING,
	SHUTTER_UNKNOWN,

} shutterState_t;
typedef struct shutter_s {
	int channel;
	// current fraction
	shutterState_t state;
	float frac;
	float targetFrac;
	float openTimeSeconds;
	float closeTimeSeconds;
	struct shutter_s *next;
} shutter_t;

shutter_t *g_shutters = 0;

shutter_t *GetForChannel(int i) {
	shutter_t *s = g_shutters;
	while (s) {
		if (i == s->channel) {
			return s;
		}
		s = s->next;
	}
	return s;
}
// startDriver Shutters
void DRV_Shutters_AddToHtmlPage(http_request_t *request, int bPreState) {
	if (bPreState) {
		hprintf255(request, "<script>"
			"function sendCmd(cmd) {"
			"  fetch('/cm?cmnd=' + encodeURIComponent(cmd));"
			"}"
			"</script>");

		hprintf255(request, "<h1>Shutters Calibration Section</h1>");

		shutter_t *s = g_shutters;
		while (s) {
			hprintf255(request, "<hr>");
			hprintf255(request, "<h5>Shutter %i</h5>", s->channel);

			hprintf255(request,
				"<form onsubmit='return false;'>"
				"<label>Open time (s):</label> "
				"<input id='ot%i' type='number' step='0.1' value='%.2f'> ",
				s->channel, s->openTimeSeconds
			);

			hprintf255(request,
				"<label>Close time (s):</label> "
				"<input id='ct%i' type='number' step='0.1' value='%.2f'> ",
				s->channel, s->closeTimeSeconds
			);

			hprintf255(request,
				"<button class='btn' onclick='sendCmd(\"ShutterCalibrate %i \" + "
				"document.getElementById(\"ot%i\").value + \" \" + "
				"document.getElementById(\"ct%i\").value)'>Set</button>"
				"</form>",
				s->channel,
				s->channel,
				s->channel
			);


			s = s->next;
		}
		return;
	}

	hprintf255(request, "<h1>Shutters Controls Section</h1>");
	shutter_t *s = g_shutters;
	while (s) {
		int currentPercent = (s->frac < 0.0f) ? -1 : (int)(s->frac * 100.0f + 0.5f);
		int targetPercent = (int)(s->targetFrac * 100.0f + 0.5f);

		hprintf255(request, "<hr>");
		hprintf255(request, "<h5>Shutter %i</h5>", s->channel);

		if (currentPercent >= 0)
			hprintf255(request, "<div>Current Position: %i%%</div>", currentPercent);
		else
			hprintf255(request, "<div>Current Position: unknown</div>");

		hprintf255(request, "<div>Target Position: %i%%</div>", targetPercent);

		hprintf255(request,
			"<div>"
			"<button class='btn' onclick='sendCmd(\"ShutterMoveTo %i 1\")'>Open</button> "
			"<button class='btn' onclick='sendCmd(\"ShutterMoveTo %i %.2f\")'>Stop</button> "
			"<button class='btn' onclick='sendCmd(\"ShutterMoveTo %i 0\")'>Close</button>"
			"</div>",
			s->channel,
			s->channel, (s->frac >= 0.0f) ? s->frac : 0.0f,
			s->channel
		);

		// Target slider (read/write)
		hprintf255(request,
			"<form onsubmit='return false;'>"
			"<label>Set Target:</label>"
			"<input type='range' min='0' max='100' value='%i' "
			"onchange='sendCmd(\"ShutterMoveTo %i \" + (this.value/100))'>"
			"</form>",
			targetPercent,
			s->channel
		);

		// Current slider (read-only)
		hprintf255(request,
			"<form onsubmit='return false;'>"
			"<label>Current Position:</label>"
			"<input type='range' min='0' max='100' value='%i' disabled>"
			"</form>",
			(currentPercent >= 0) ? currentPercent : 0
		);

		s = s->next;
	}
}

static void Shutter_SetPin(shutter_t *s, int role, int val) {
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.channels[i] == s->channel
			&& g_cfg.pins.roles[i] == role) {
			HAL_PIN_SetOutputValue(i, val);
		}
	}
}
static void Shutter_SetPins(shutter_t *s, shutterState_t dir) {
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


static void Shutter_Stop(shutter_t *s) {
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
	Shutter_SetPins(s, s->state);
}
static commandResult_t CMD_Shutter_MoveTo(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, 0);

	int index = Tokenizer_GetArgInteger(0);
	float frac = Tokenizer_GetArgFloat(1);

	if (frac < 0.0f) frac = 0.0f;
	if (frac > 1.0f) frac = 1.0f;

	shutter_t *s = GetForChannel(index);
	if (!s)
		return CMD_RES_BAD_ARGUMENT;

	s->targetFrac = frac;

	if (s->frac < 0.0f)
		s->frac = 0.0f;

	if (frac > s->frac) {
		s->state = SHUTTER_OPENING;
	}
	else if (frac < s->frac) {
		s->state = SHUTTER_CLOSING;
	}
	Shutter_SetPins(s, s->state);

	return CMD_RES_OK;
}
void Shutter_SetTimes(int channel, float timeOpen, float timeClose) {
	shutter_t *s = GetForChannel(channel);
	if (!s)
		return;

	if (timeOpen > 0.0f)
		s->openTimeSeconds = timeOpen;

	if (timeClose > 0.0f)
		s->closeTimeSeconds = timeClose;
}

static commandResult_t CMD_Shutter_Calibrate(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, 0);

	int index = Tokenizer_GetArgInteger(0);
	// can be - or -1 or 0 to mean 'skip'
	float timeOpen = Tokenizer_GetArgFloat(1);
	float timeClose = Tokenizer_GetArgFloat(2);

	Shutter_SetTimes(index, timeOpen, timeClose);

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
static void RegisterShutterForChannel(int channel) {
	shutter_t *s = GetForChannel(channel);
	if (!s) {
		s = malloc(sizeof(shutter_t));
		memset(s, 0, sizeof(shutter_t));
		s->channel = channel;
		s->frac = 0.0f;  // unknown position
		s->state = SHUTTER_OPEN;
		s->targetFrac = 0.0f;
		s->openTimeSeconds = 10.0f;  // default
		s->closeTimeSeconds = 10.0f; // default
		s->next = g_shutters;
		g_shutters = s;
	}
}
// backlog setPinRole 5 ShutterA; startDriver Shutters
void DRV_Shutters_Init() {
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		int r = g_cfg.pins.roles[i];
		if (r == IOR_ShutterA || r == IOR_ShutterB) {
			RegisterShutterForChannel(g_cfg.pins.channels[i]);
		}
	}
	CMD_RegisterCommand("ShutterMoveTo", CMD_Shutter_MoveTo, NULL);
	CMD_RegisterCommand("ShutterCalibrate", CMD_Shutter_Calibrate, NULL);
}

