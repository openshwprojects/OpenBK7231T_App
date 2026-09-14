#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_flashVars.h"

/*
// sensitivity pin
setPinRole 26 PWM_scriptOnly
setPinChannel 26 10
// set full sens
setChannel 10 100

// motion read (1 or 0)
setPinRole 6 dInput
setPinChannel 6 11
setChannelType 11 Motion

// light read
setPinRole 23 ADC
setPinChannel 23 12
setChannelType 12 ReadOnly
setChannelLabel 12 Light
*/

static int g_onTime;
static int g_sensitivity;
static int g_mode;
static int g_lightLevelMargin;
static int ch_lightAdc; // light level
static int ch_motion; // pir 1 or 0
static int ch_sens; // pir sens
static int g_timeLeft = 0;
static int g_isDark = 0;

// PIR settings are kept in the retained flash variables (see hal_flashVars.h).
// NOTE: HAL_FlashVars_SaveChannel/HAL_FlashVars_GetChannelValue take a raw slot
// index in range <0, MAX_RETAIN_CHANNELS), NOT a SPECIAL_CHANNEL_FLASHVARS_*
// constant - those are only understood by CHANNEL_Set/CHANNEL_Get, which
// subtract SPECIAL_CHANNEL_FLASHVARS_FIRST before calling the HAL.
// Passing the special constants here made every save silently fail the
// "index >= MAX_RETAIN_CHANNELS" range check, so all PIR settings were lost
// on every power cycle.
// The LED driver (flag OBK_FLAG_LED_REMEMBERLASTSTATE) uses the last 4 slots,
// so use the first 4 here to avoid clashing with it.
#define VAR_TIME		0
#define VAR_SENS		1
#define VAR_MODE		2
#define VAR_LIGHTLEVEL	3

#if (VAR_LIGHTLEVEL >= (MAX_RETAIN_CHANNELS - 4))
#error "PIR flash variable slots overlap the LED driver slots"
#endif

// Used when nothing has been stored yet (fresh device / erased flash vars)
#define PIR_DEFAULT_ONTIME			60
#define PIR_DEFAULT_SENSITIVITY		50
#define PIR_DEFAULT_LIGHTLEVEL		300

// Stores a setting only if it really changed, so we don't wear out the flash
// with redundant writes (the whole flash vars blob is rewritten on each save).
static void PIR_SaveVar(int index, int *pTarget, int value) {
	if (*pTarget == value)
		return;
	*pTarget = value;
	HAL_FlashVars_SaveChannel(index, value);
}

void PIR_Init() {
	g_onTime = HAL_FlashVars_GetChannelValue(VAR_TIME);
	g_sensitivity = HAL_FlashVars_GetChannelValue(VAR_SENS);
	g_mode = HAL_FlashVars_GetChannelValue(VAR_MODE);
	g_lightLevelMargin = HAL_FlashVars_GetChannelValue(VAR_LIGHTLEVEL);
	// A zero here means "never configured" - those values are not usable
	// (on time 0 would switch the light off immediately, sensitivity 0
	// would keep the PIR blind), so fall back to sane defaults.
	if (g_onTime <= 0) {
		g_onTime = PIR_DEFAULT_ONTIME;
	}
	if (g_sensitivity <= 0) {
		g_sensitivity = PIR_DEFAULT_SENSITIVITY;
	}
	if (g_lightLevelMargin <= 0) {
		g_lightLevelMargin = PIR_DEFAULT_LIGHTLEVEL;
	}
	ch_lightAdc = CHANNEL_FindIndexForPinType(IOR_ADC);
	ch_motion = CHANNEL_FindIndexForType(ChType_Motion);
	if (ch_motion == -1) {
		ch_motion = CHANNEL_FindIndexForPinType2(IOR_DigitalInput, IOR_DigitalInput_n);
		if (ch_motion == -1) {
			ch_motion = CHANNEL_FindIndexForPinType2(IOR_DigitalInput_NoPup, IOR_DigitalInput_NoPup_n);
		}
	}
	ch_sens = CHANNEL_FindIndexForPinType2(IOR_PWM_ScriptOnly, IOR_PWM_ScriptOnly_n);
}

void PIR_OnEverySecond() {
	if (ch_sens != -1) {
		CHANNEL_Set(ch_sens,g_sensitivity, 0);
	}
	if (g_mode == 1) {
		// "Value seems to go down if MORE light is here and UP is LESS light is here"
		// If no light sensor is mapped, don't ask for channel -1, just assume it's dark
		int lightLevel = (ch_lightAdc != -1) ? CHANNEL_Get(ch_lightAdc) : (g_lightLevelMargin + 1);
		g_isDark = lightLevel > g_lightLevelMargin;
		if (g_isDark) {
			// auto mode
			int motion = (ch_motion != -1) ? CHANNEL_Get(ch_motion) : 0;
			if (motion) {
				g_timeLeft = g_onTime;
				LED_SetEnableAll(true);
			}
		}
		if (g_timeLeft > 0) {
			g_timeLeft--;
			if (g_timeLeft <= 0) {
				// turn off
				LED_SetEnableAll(false);
			}
		}
	}
	else {
		g_timeLeft = 0;
	}

}

void PIR_OnChannelChanged(int ch, int value) {

}

void PIR_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
	if (bPreState)
	{
		char tmpA[32];
		if (http_getArg(request->url, "pirTime", tmpA, sizeof(tmpA))) {
			PIR_SaveVar(VAR_TIME, &g_onTime, atoi(tmpA));
		}
		if (http_getArg(request->url, "pirSensitivity", tmpA, sizeof(tmpA))) {
			PIR_SaveVar(VAR_SENS, &g_sensitivity, atoi(tmpA));
		}
		if (http_getArg(request->url, "pirMode", tmpA, sizeof(tmpA))) {
			PIR_SaveVar(VAR_MODE, &g_mode, atoi(tmpA));
		}
		if (http_getArg(request->url, "light", tmpA, sizeof(tmpA))) {
			PIR_SaveVar(VAR_LIGHTLEVEL, &g_lightLevelMargin, atoi(tmpA));
		}

		hprintf255(request, "<h3>PIR Sensor Settings</h3>");
		hprintf255(request, "<form action=\"index\" method=\"get\">");

		hprintf255(request, "On Time (seconds): <input type=\"text\" name=\"pirTime\" value=\"%i\"/><br><br>", g_onTime);
		hprintf255(request, "PIR Sensitivity: <input type=\"range\" name=\"pirSensitivity\" min=\"1\" max=\"100\" value=\"%i\"/><br><br>", g_sensitivity);
		hprintf255(request, "Light Level Margin: <input type=\"text\" name=\"light\" value=\"%i\"/><br><br>", g_lightLevelMargin);

		hprintf255(request, "Mode:<br>");
		hprintf255(request, "<input type=\"radio\" id=\"manual\" name=\"pirMode\" value=\"0\" %s><label for=\"manual\">Manual Mode</label><br>", g_mode == 0 ? "checked" : "");
		hprintf255(request, "<input type=\"radio\" id=\"auto\" name=\"pirMode\" value=\"1\" %s><label for=\"auto\">AUTO Mode with PIR</label><br><br>", g_mode == 1 ? "checked" : "");

		hprintf255(request, "<input type=\"submit\" value=\"Save\"/>");
		hprintf255(request, "</form>");
	}
	else {
		if (g_mode == 1) {
			hprintf255(request, "PIR current state: Automatic, motion: %i, isDark %i, timeLeft: %i<br>", CHANNEL_Get(ch_motion), g_isDark, g_timeLeft);
		}
		else {
			hprintf255(request, "PIR current state: Manual<br>");
		}
	}
}
