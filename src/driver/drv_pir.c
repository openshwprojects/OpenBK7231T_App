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

#define VAR_TIME SPECIAL_CHANNEL_FLASHVARS_LAST
#define VAR_SENS (SPECIAL_CHANNEL_FLASHVARS_LAST-1)
#define VAR_MODE (SPECIAL_CHANNEL_FLASHVARS_LAST-2)
#define VAR_LIGHTLEVEL (SPECIAL_CHANNEL_FLASHVARS_LAST-3)

void PIR_Init() {
	g_onTime = HAL_FlashVars_GetChannelValue(VAR_TIME);
	g_sensitivity = HAL_FlashVars_GetChannelValue(VAR_SENS);
	g_mode = HAL_FlashVars_GetChannelValue(VAR_MODE);
	g_lightLevelMargin = HAL_FlashVars_GetChannelValue(VAR_LIGHTLEVEL);
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
		int lightLevel = CHANNEL_Get(ch_lightAdc);
		g_isDark = lightLevel > g_lightLevelMargin;
		if (g_isDark) {
			// auto mode
			int motion = CHANNEL_Get(ch_motion);
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
			g_onTime = atoi(tmpA);
			HAL_FlashVars_SaveChannel(VAR_TIME, g_onTime);
		}
		if (http_getArg(request->url, "pirSensitivity", tmpA, sizeof(tmpA))) {
			g_sensitivity = atoi(tmpA);
			HAL_FlashVars_SaveChannel(VAR_SENS, g_sensitivity);
		}
		if (http_getArg(request->url, "pirMode", tmpA, sizeof(tmpA))) {
			g_mode = atoi(tmpA);
			HAL_FlashVars_SaveChannel(VAR_MODE, g_mode);
		}
		if (http_getArg(request->url, "light", tmpA, sizeof(tmpA))) {
			g_lightLevelMargin = atoi(tmpA);
			HAL_FlashVars_SaveChannel(VAR_LIGHTLEVEL, g_lightLevelMargin);
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
