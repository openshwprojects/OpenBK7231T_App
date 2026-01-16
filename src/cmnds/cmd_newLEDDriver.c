
#include "../obk_config.h"

#if ENABLE_LED_BASIC

#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "cmd_public.h"
#include "../driver/drv_public.h"
#include "../driver/drv_local.h"
#include "../hal/hal_flashVars.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_flashConfig.h"
#include "../rgb2hsv.h"
#include <ctype.h>
#include "cmd_local.h"
#include "../mqtt/new_mqtt.h"
#include "../cJSON/cJSON.h"
#include <string.h>
#include <math.h>
#if ENABLE_LITTLEFS
	#include "../littlefs/our_lfs.h"
#endif

//  My HA config for system below:
/*
  - platform: mqtt
    name: obk8D38570E
    rgb_command_template: "{{ '#%02x%02x%02x0000' | format(red, green, blue)}}"
    rgb_state_topic: "cmnd/obk8D38570E/led_basecolor_rgb"
    rgb_command_topic: "cmnd/obk8D38570E/led_basecolor_rgb"
    command_topic: "cmnd/obk8D38570E/led_enableAll"
    availability_topic: "obk8D38570E/connected"
    payload_on: 1
    payload_off: 0
    brightness_command_topic: "cmnd/obk8D38570E/led_dimmer"
    brightness_scale: 100
    brightness_value_template: "{{ value_json.Dimmer }}"
    color_temp_command_topic: "cmnd/obk8D38570E/led_temperature"
    color_temp_state_topic: "cmnd/obk8D38570E/ctr"
    color_temp_value_template: "{{ value_json.CT }}"

	// TODO: state return topics
	*/

// NOTE: there are 2 customization commands
// They are not storing the config in flash, if you use them,
// please put them in autoexec.bat from LittleFS.
// Command 1: led_brightnessMult [floatVal]
// It sets the multipler for the dimming
// Command 2: g_cfg_colorScaleToChannel [floatVal]
// It sets the multipler for converting 0-255 range RGB to 0-100 channel value

int parsePowerArgument(const char *s);


// Those are base colors, normalized, without brightness applied
float led_baseColors[5] = { 255, 255, 255, 255, 255 };
// Those have brightness included
float finalColors[5] = { 0, 0, 0, 0, 0 };
float g_hsv_h = 0; // 0 to 360
float g_hsv_s = 0; // 0 to 1
float g_hsv_v = 1; // 0 to 1
// By default, colors are in 255 to 0 range, while our channels accept 0 to 100 range
float g_cfg_colorScaleToChannel = 100.0f/255.0f;
float g_brightness0to100 = 100.0f;
float g_brightnessScale = 1.0f;
float rgb_used_corr[3];   // RGB correction currently used
// for smart dimmer, etc
int led_defaultDimmerDeltaForHold = 10;
// how often is LED state saved (if modified)? 
// Saving LED often wears out flash memory, so it makes sense to save only from time to time
// But setting a large interval and powering off led may cause the state to not be saved.
// So it's adjustable
short led_saveStateIfModifiedInterval = 30;
short led_timeUntilNextSavePossible = 0;
byte g_ledStateSavePending = 0;
byte g_numBaseColors = 5;
byte g_lightMode = Light_RGB;

// NOTE: in this system, enabling/disabling whole led light bulb
// is not changing the stored channel and brightness values.
// They are kept intact so you can reenable the bulb and keep your color setting
byte g_lightEnableAll = 0;

// the slider control in the UI emits values
// in the range from 154-500 (defined
// in homeassistant/util/color.py as HASS_COLOR_MIN and HASS_COLOR_MAX).
float led_temperature_min = HASS_TEMPERATURE_MIN;
float led_temperature_max = HASS_TEMPERATURE_MAX;
float led_temperature_current = HASS_TEMPERATURE_MIN;

void LED_ResetGlobalVariablesToDefaults() {
	int i;

	g_lightMode = Light_RGB;
	for (i = 0; i < 5; i++) {
		led_baseColors[i] = 255;
		finalColors[i] = 0;
	}
	g_hsv_h = 0; // 0 to 360
	g_hsv_s = 0; // 0 to 1
	g_hsv_v = 1; // 0 to 1
	g_cfg_colorScaleToChannel = 100.0f / 255.0f;
	g_numBaseColors = 5;
	g_brightness0to100 = 100.0f;
	g_lightEnableAll = 0;
	led_temperature_min = HASS_TEMPERATURE_MIN;
	led_temperature_max = HASS_TEMPERATURE_MAX;
	led_temperature_current = HASS_TEMPERATURE_MIN;
}

// The color order is RGBCW.
// some people set RED to channel 0, and some of them set RED to channel 1
// Let's detect if there is a PWM on channel 0
int LED_GetFirstChannelIndex() {
#if 0
	int firstChannelIndex;
	if (CHANNEL_HasChannelPinWithRoleOrRole(0, IOR_PWM, IOR_PWM_n)) {
		firstChannelIndex = 0;
	}
	else {
		firstChannelIndex = 1;
	}
	return firstChannelIndex;
#else
	int i;
	for (i = 0; i < 5; i++) {
		if (CHANNEL_HasChannelPinWithRoleOrRole(i, IOR_PWM, IOR_PWM_n)) {
			return i;
		}
	}
	return 0;
#endif
}

bool LED_IsLedDriverChipRunning()
{
#if	ENABLE_DRIVER_TUYAMCU
	if (TuyaMCU_IsLEDRunning()) {
		return true;
	}
#endif
#ifndef OBK_DISABLE_ALL_DRIVERS
	return DRV_IsRunning("SM2135") || DRV_IsRunning("BP5758D") 
		|| DRV_IsRunning("TESTLED") || DRV_IsRunning("SM2235") || DRV_IsRunning("BP1658CJ")
		|| DRV_IsRunning("KP18058")
		|| DRV_IsRunning("SM16703P")
		|| DRV_IsRunning("SM15155E")
		|| DRV_IsRunning("DMX")
		; 
#else
	return false;
#endif
}
bool LED_IsLEDRunning()
{
	int pwmCount;

	if (LED_IsLedDriverChipRunning())
		return true;

	//pwmCount = PIN_CountPinsWithRoleOrRole(IOR_PWM, IOR_PWM_n);
	// This will treat multiple PWMs on a single channel as one.
	// Thanks to this users can turn for example RGB LED controller
	// into high power 3-outputs single colors LED controller
	PIN_get_Relay_PWM_Count(0, &pwmCount, 0);

	if (pwmCount > 0)
		return true;	
	if (CFG_HasFlag(OBK_FLAG_LED_FORCESHOWRGBCWCONTROLLER))
		return true;
	return false;
}

int isCWMode() {
	int pwmCount;

	//pwmCount = PIN_CountPinsWithRoleOrRole(IOR_PWM, IOR_PWM_n);
	// This will treat multiple PWMs on a single channel as one.
	// Thanks to this users can turn for example RGB LED controller
	// into high power 3-outputs single colors LED controller
	PIN_get_Relay_PWM_Count(0, &pwmCount, 0);

	if(pwmCount == 2)
		return 1;
	return 0;
}

int shouldSendRGB() {
	int pwmCount;

	// forced RGBCW means 'send rgb'
	// This flag should be set for SM2315 and BP5758
	// This flag also could be used for dummy Device Groups driver-module
	if(CFG_HasFlag(OBK_FLAG_LED_FORCESHOWRGBCWCONTROLLER))
		return 1;
	// assume that LED driver = RGB (at least), usually RGBCW
	if (LED_IsLedDriverChipRunning())
		return true;

	//pwmCount = PIN_CountPinsWithRoleOrRole(IOR_PWM, IOR_PWM_n);
	// This will treat multiple PWMs on a single channel as one.
	// Thanks to this users can turn for example RGB LED controller
	// into high power 3-outputs single colors LED controller
	PIN_get_Relay_PWM_Count(0, &pwmCount, 0);

	// single colors and CW don't send rgb
	if(pwmCount <= 2)
		return 0;

	return 1;
}



float led_rawLerpCurrent[5] = { 0 };
float Mathf_MoveTowards(float cur, float tg, float dt) {
	float rem = tg - cur;
	if(abs(rem) < dt) {
		return tg;
	}
	if(rem < 0) {
		return cur - dt;
	}
	return cur + dt;
}
// Colors are in 0-255 range.
// This value determines how fast color can change.
// 100 means that in one second color will go from 0 to 100
// 200 means that in one second color will go from 0 to 200
float led_lerpSpeedUnitsPerSecond = 200.f;

float led_current_value_brightness = 0;
float led_current_value_cold_or_warm = 0;


void LED_CalculateEmulatedCool(float inCool, float *outRGB) {
	outRGB[0] = inCool;
	outRGB[1] = inCool;
	outRGB[2] = inCool;
}

void LED_ApplyEmulatedCool(int firstChannelIndex, float chVal) {
	float rgb[3];
	LED_CalculateEmulatedCool(chVal, rgb);
	CHANNEL_Set_FloatPWM(firstChannelIndex + 0, rgb[0], CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set_FloatPWM(firstChannelIndex + 1, rgb[1], CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
	CHANNEL_Set_FloatPWM(firstChannelIndex + 2, rgb[2], CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
}

void LED_I2CDriver_WriteRGBCW(float* finalRGBCW) {
#ifdef ENABLE_DRIVER_GOSUNDSW2
	if (DRV_IsRunning("GosundSW2")) {
		DRV_GosundSW2_Write(finalRGBCW)
	}
#endif
#ifdef ENABLE_DRIVER_LED
	if (CFG_HasFlag(OBK_FLAG_LED_EMULATE_COOL_WITH_RGB)) {
		if (g_lightMode == Light_Temperature) {
			// the format is RGBCW
			// Emulate C with RGB
			LED_CalculateEmulatedCool(finalRGBCW[3], finalRGBCW);
			// C is unused
			finalRGBCW[3] = 0;
			// keep W unchanged
		}
	}
	if (DRV_IsRunning("SM2135")) {
		SM2135_Write(finalRGBCW);
	}
	if (DRV_IsRunning("BP5758D")) {
		BP5758D_Write(finalRGBCW);
	}
	if (DRV_IsRunning("BP1658CJ")) {
		BP1658CJ_Write(finalRGBCW);
	}
	if (DRV_IsRunning("SM2235")) {
		SM2235_Write(finalRGBCW);
	}
	if (DRV_IsRunning("KP18058")) {
		KP18058_Write(finalRGBCW);
	}
#endif
#ifdef ENABLE_DRIVER_SM15155E
	if (DRV_IsRunning("SM15155E")) {
		SM15155E_Write(finalRGBCW);
	}
#endif
}
void LED_RunOnEverySecond() {
	// can save?
	if (led_timeUntilNextSavePossible > led_saveStateIfModifiedInterval) {
		// can already save, do it if requested
		if (g_ledStateSavePending) {
			// do not save if user has turned off during the wait period
			if (CFG_HasFlag(OBK_FLAG_LED_REMEMBERLASTSTATE)) {
				LED_SaveStateToFlashVarsNow();
				// saved
			}
			g_ledStateSavePending = 0;
			led_timeUntilNextSavePossible = 0;
		}
		else {
			// otherwise don't do anything, we will save as soon as it's required
		}
	}
	else {
		// cannot save yet, bump counter up
		led_timeUntilNextSavePossible++;
	}
}
void LED_RunQuickColorLerp(int deltaMS) {
	int i;
	int firstChannelIndex;
	float deltaSeconds;
	byte finalRGBCW[5];
	int maxPossibleIndexToSet;
	int emulatedCool = -1;
	int target_value_brightness = 0;
	int target_value_cold_or_warm = 0;

	if (CFG_HasFlag(OBK_FLAG_LED_FORCE_MODE_RGB)) {
		// only allow setting pwm 0, 1 and 2, force-skip 3 and 4
		maxPossibleIndexToSet = 3;
	}
	else {
		maxPossibleIndexToSet = 5;
	}


	deltaSeconds = deltaMS * 0.001f;

	firstChannelIndex = LED_GetFirstChannelIndex();

	if (CFG_HasFlag(OBK_FLAG_LED_EMULATE_COOL_WITH_RGB)) {
		emulatedCool = firstChannelIndex + 3;
	}

	for(i = 0; i < 5; i++) {
		float ch_rgb_cal = (i < 3)? rgb_used_corr[i] : 1.0f; // adjust change rate with RGB correction in use
		// This is the most silly and primitive approach, but it works
		// In future we might implement better lerp algorithms, use HUE, etc
		led_rawLerpCurrent[i] = Mathf_MoveTowards(led_rawLerpCurrent[i],finalColors[i], deltaSeconds * led_lerpSpeedUnitsPerSecond * ch_rgb_cal);
	}

	target_value_cold_or_warm = LED_GetTemperature0to1Range() * 100.0f;
	if (g_lightEnableAll) {
		if (g_lightMode == Light_Temperature) {
			target_value_brightness = g_brightness0to100 * g_brightnessScale;
		}
	}

	led_current_value_brightness = Mathf_MoveTowards(led_current_value_brightness, target_value_brightness, deltaSeconds * led_lerpSpeedUnitsPerSecond);
	led_current_value_cold_or_warm = Mathf_MoveTowards(led_current_value_cold_or_warm, target_value_cold_or_warm, deltaSeconds * led_lerpSpeedUnitsPerSecond );

	// OBK_FLAG_LED_ALTERNATE_CW_MODE means we have a driver that takes one PWM for brightness and second for temperature
	if(isCWMode() && CFG_HasFlag(OBK_FLAG_LED_ALTERNATE_CW_MODE)) {
		CHANNEL_Set_FloatPWM(firstChannelIndex, led_current_value_cold_or_warm, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
		CHANNEL_Set_FloatPWM(firstChannelIndex+1, led_current_value_brightness, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
	} else {
		if(isCWMode()) { 
			// In CW mode, user sets just two PWMs. So we have: PWM0 and PWM1 (or maybe PWM1 and PWM2)
			// But we still have RGBCW internally
			// So, we need to map. Map component 3 of RGBCW to first channel, and component 4 to second.
			CHANNEL_Set_FloatPWM(firstChannelIndex + 0, led_rawLerpCurrent[3] * g_cfg_colorScaleToChannel, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
			CHANNEL_Set_FloatPWM(firstChannelIndex + 1, led_rawLerpCurrent[4] * g_cfg_colorScaleToChannel, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
		} else {
			// This should work for both RGB and RGBCW
			// This also could work for a SINGLE COLOR strips
			for(i = 0; i < maxPossibleIndexToSet; i++) {
				finalRGBCW[i] = led_rawLerpCurrent[i];
				float chVal = led_rawLerpCurrent[i] * g_cfg_colorScaleToChannel;
				int channelToUse = firstChannelIndex + i;
				// emulated cool is -1 by default, so this block will only execute
				// if the cool emulation was enabled
				if (channelToUse == emulatedCool && g_lightMode == Light_Temperature) {
					LED_ApplyEmulatedCool(firstChannelIndex, chVal);
				}
				else {
					if (CFG_HasFlag(OBK_FLAG_LED_ALTERNATE_CW_MODE)) {
						if (i == 3) {
							chVal = led_current_value_cold_or_warm;
						}
						else if (i == 4) {
							chVal = led_current_value_brightness;
						}
					}
					CHANNEL_Set_FloatPWM(channelToUse, chVal, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
				}
			}
		}
	}
	
	LED_I2CDriver_WriteRGBCW(led_rawLerpCurrent);
}

void LED_ResendCurrentColors() {
	if (CFG_HasFlag(OBK_FLAG_LED_SMOOTH_TRANSITIONS)) {
		LED_I2CDriver_WriteRGBCW(led_rawLerpCurrent);
	}
	else {
		LED_I2CDriver_WriteRGBCW(finalColors);
	}
}


int led_gamma_enable_channel_messages = 0;

float led_gamma_correction (int color, float iVal) { // apply LED gamma and RGB correction
	if ((color < 0) || (color > 4)) {
		return iVal;
	}
	float brightnessNormalized0to1 = g_brightness0to100 * 0.01f * g_brightnessScale;
	if (CFG_HasFlag(OBK_FLAG_LED_USE_OLD_LINEAR_MODE)) {
		return iVal * brightnessNormalized0to1;
	}

	// apply LED gamma correction:

	// this gamma correction function does not properly calculate gamma
	// float ch_bright_min = g_cfg.led_corr.rgb_bright_min / 100;
	// if (color > 2) {
	//	ch_bright_min = g_cfg.led_corr.cw_bright_min / 100;
	// }
	// float oVal = (powf (brightnessNormalized0to1, g_cfg.led_corr.led_gamma) * (1 - ch_bright_min) + ch_bright_min) * iVal;

	// color value adjusted to float 0-1, modified by brightness
	float brightnessCorrectedColor = iVal / 255.0f * brightnessNormalized0to1;

	// gamma correct the color value
	float oVal = powf (brightnessCorrectedColor,  g_cfg.led_corr.led_gamma) * 255.0f;

	// apply RGB level correction:
	if (color < 3) {
		rgb_used_corr[color] = g_cfg.led_corr.rgb_cal[color];
		oVal *= rgb_used_corr[color];
	}

	if (led_gamma_enable_channel_messages &&
			(((g_lightMode == Light_RGB) && (color < 3)) || ((g_lightMode != Light_RGB) && (color >= 3)))) {
		addLogAdv (LOG_INFO, LOG_FEATURE_CMD, "channel %i set to %.2f%%", color, oVal / 2.55);
	}
	if (oVal > 255.0f) {
		oVal = 255.0f;
	}
	return oVal;
} //


#if ENABLE_MQTT

OBK_Publish_Result sendColorChange() {
	char s[16];
	byte c[3];

	if (shouldSendRGB() == 0) {
		return OBK_PUBLISH_WAS_NOT_REQUIRED;
	}

	c[0] = (byte)(led_baseColors[0]);
	c[1] = (byte)(led_baseColors[1]);
	c[2] = (byte)(led_baseColors[2]);

	snprintf(s, sizeof(s), "%02X%02X%02X", c[0], c[1], c[2]);

	return MQTT_PublishMain_StringString_DeDuped(DEDUP_LED_BASECOLOR_RGB, DEDUP_EXPIRE_TIME, "led_basecolor_rgb", s, 0);
}
// One user requested ability to broadcast full RGBCW
static void sendFullRGBCW_IfEnabled() {
	char s[16];
	byte c[5];

	if (CFG_HasFlag(OBK_FLAG_LED_BROADCAST_FULL_RGBCW) == false) {
		return;
	}

	c[0] = (byte)(finalColors[0]);
	c[1] = (byte)(finalColors[1]);
	c[2] = (byte)(finalColors[2]);
	c[3] = (byte)(finalColors[3]);
	c[4] = (byte)(finalColors[4]);

	snprintf(s, sizeof(s), "%02X%02X%02X%02X%02X", c[0], c[1], c[2], c[3], c[4]);

	MQTT_PublishMain_StringString_DeDuped(DEDUP_LED_FINALCOLOR_RGBCW, DEDUP_EXPIRE_TIME, "led_finalcolor_rgbcw", s, 0);
}
OBK_Publish_Result LED_SendEnableAllState() {
	return MQTT_PublishMain_StringInt_DeDuped(DEDUP_LED_ENABLEALL, DEDUP_EXPIRE_TIME, "led_enableAll", g_lightEnableAll, 0);
}
OBK_Publish_Result LED_SendCurrentLightModeParam_TempOrColor() {

	if (g_lightMode == Light_Temperature) {
		return sendTemperatureChange();
	}
	else if (g_lightMode == Light_RGB) {
		return sendColorChange();
	}
	return OBK_PUBLISH_WAS_NOT_REQUIRED;
}
OBK_Publish_Result sendFinalColor() {
	char s[16];
	byte c[3];

	if (shouldSendRGB() == 0) {
		return OBK_PUBLISH_WAS_NOT_REQUIRED;
	}

	c[0] = (byte)(finalColors[0]);
	c[1] = (byte)(finalColors[1]);
	c[2] = (byte)(finalColors[2]);

	snprintf(s, sizeof(s), "%02X%02X%02X", c[0], c[1], c[2]);

	return MQTT_PublishMain_StringString_DeDuped(DEDUP_LED_FINALCOLOR_RGB, DEDUP_EXPIRE_TIME, "led_finalcolor_rgb", s, 0);
}
OBK_Publish_Result LED_SendDimmerChange() {
	int iValue;

	iValue = g_brightness0to100;

	return MQTT_PublishMain_StringInt_DeDuped(DEDUP_LED_DIMMER, DEDUP_EXPIRE_TIME, "led_dimmer", iValue, 0);
}
OBK_Publish_Result sendTemperatureChange() {
	return MQTT_PublishMain_StringInt_DeDuped(DEDUP_LED_TEMPERATURE, DEDUP_EXPIRE_TIME, "led_temperature", (int)led_temperature_current, 0);
}
#endif

void LED_SaveStateToFlashVarsNow() {
	HAL_FlashVars_SaveLED(g_lightMode, g_brightness0to100, led_temperature_current, led_baseColors[0], led_baseColors[1], led_baseColors[2], g_lightEnableAll);
}
void apply_smart_light() {
	int i;
	int firstChannelIndex;
	int channelToUse;
	byte finalRGBCW[5];
	byte baseRGBCW[5];
	int maxPossibleIndexToSet;
	int emulatedCool = -1;
	int value_brightness = 0;
	int value_cold_or_warm = 0;


	firstChannelIndex = LED_GetFirstChannelIndex();

	if (CFG_HasFlag(OBK_FLAG_LED_EMULATE_COOL_WITH_RGB)) {
		emulatedCool = firstChannelIndex + 3;
	}

	if (CFG_HasFlag(OBK_FLAG_LED_FORCE_MODE_RGB)) {
		// only allow setting pwm 0, 1 and 2, force-skip 3 and 4
		maxPossibleIndexToSet = 3;
	}
	else {
		maxPossibleIndexToSet = 5;
	}

	if (CFG_HasFlag(OBK_FLAG_LED_ALTERNATE_CW_MODE)) {
		value_cold_or_warm = LED_GetTemperature0to1Range() * 100.0f;
		if (g_lightEnableAll) {
			if (g_lightMode == Light_Temperature) {
				value_brightness = g_brightness0to100 * g_brightnessScale;
			}
		}
	}

	if(isCWMode() && CFG_HasFlag(OBK_FLAG_LED_ALTERNATE_CW_MODE)) {
		for(i = 0; i < 5; i++) {
			finalColors[i] = 0;
			baseRGBCW[i] = 0;
			finalRGBCW[i] = 0;
		}
		if(g_lightEnableAll) {
			float brightnessNormalized0to1 = g_brightness0to100 * 0.01f * g_brightnessScale;
			for(i = 3; i < 5; i++) {
				finalColors[i] = led_baseColors[i] * brightnessNormalized0to1;
				finalRGBCW[i] = led_baseColors[i] * brightnessNormalized0to1;
				baseRGBCW[i] = led_baseColors[i];
			}
		}
		if(CFG_HasFlag(OBK_FLAG_LED_SMOOTH_TRANSITIONS) == false) {
			CHANNEL_Set_FloatPWM(firstChannelIndex, value_cold_or_warm, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
			CHANNEL_Set_FloatPWM(firstChannelIndex+1, value_brightness, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
		}
	} else {
		for(i = 0; i < maxPossibleIndexToSet; i++) {
			float final = 0.0f;

			baseRGBCW[i] = led_baseColors[i];
			if(g_lightEnableAll) {
				final = led_gamma_correction (i, led_baseColors[i]);
			}
			if(g_lightMode == Light_Temperature) {
				// skip channels 0, 1, 2
				// (RGB)
				if(i < 3)
				{
					baseRGBCW[i] = 0;
					final = 0;
				}
			}
			else if (g_lightMode == Light_RGB) {
				// skip channels 3, 4
				if (i >= 3)
				{
					baseRGBCW[i] = 0;
					final = 0;
				}
			} else if(g_lightMode == Light_Anim) {
				// skip all?
				baseRGBCW[i] = 0;
				final = 0;
			} else {

			}
			finalColors[i] = final;
			finalRGBCW[i] = final;
			
			float chVal = final * g_cfg_colorScaleToChannel;
			if (chVal > 100.0f)
				chVal = 100.0f;

			channelToUse = firstChannelIndex + i;

			// log printf with %f crashes N platform?
			//ADDLOG_INFO(LOG_FEATURE_CMD, "apply_smart_light: ch %i raw is %f, bright %f, final %f, enableAll is %i",
			//	channelToUse,raw,g_brightness,final,g_lightEnableAll);

			if(CFG_HasFlag(OBK_FLAG_LED_SMOOTH_TRANSITIONS) == false) {
				if (isCWMode()) {
					// in CW mode, we have only set two channels
					// We don't have RGB channels
					// so, do simple mapping
					if (i == 3) {
						CHANNEL_Set_FloatPWM(firstChannelIndex + 0, chVal, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
					}
					else if (i == 4) {
						CHANNEL_Set_FloatPWM(firstChannelIndex + 1, chVal, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
					}
				} else {
					// emulated cool is -1 by default, so this block will only execute
					// if the cool emulation was enabled
					if (channelToUse == emulatedCool && g_lightMode == Light_Temperature) {
						LED_ApplyEmulatedCool(firstChannelIndex, chVal);
					}
					else {
						if (CFG_HasFlag(OBK_FLAG_LED_ALTERNATE_CW_MODE)) {
							if (i == 3) {
								chVal = value_cold_or_warm;
							}
							else if (i == 4) {
								chVal = value_brightness;
							}
						}
						CHANNEL_Set_FloatPWM(channelToUse, chVal, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
					}
				}
			}
		}
	}
	if(CFG_HasFlag(OBK_FLAG_LED_SMOOTH_TRANSITIONS) == false) {
		LED_I2CDriver_WriteRGBCW(finalColors);
	}

	if(CFG_HasFlag(OBK_FLAG_LED_REMEMBERLASTSTATE)) {
		// something was changed, mark as dirty
		g_ledStateSavePending = 1;
	}
#if	ENABLE_TASMOTADEVICEGROUPS
	DRV_DGR_OnLedFinalColorsChange(baseRGBCW);
#endif
#if	ENABLE_DRIVER_TUYAMCU
	TuyaMCU_OnRGBCWChange(finalColors, g_lightEnableAll, g_lightMode, g_brightness0to100*g_brightnessScale*0.01f, LED_GetTemperature0to1Range());
#endif
#if	ENABLE_DRIVER_SM16703P
	if (pixel_count > 0 && (g_lightMode != Light_Anim || g_lightEnableAll == 0)) {
		Strip_setAllPixels(finalColors[0], finalColors[1], finalColors[2], finalColors[3], finalColors[4]);
		Strip_Apply();
	}
#endif
	
	// I am not sure if it's the best place to do it
	// NOTE: this will broadcast MQTT only if a flag is set
#if ENABLE_MQTT
	sendFullRGBCW_IfEnabled();
#endif
}

void led_gamma_list (void) { // list RGB gamma settings
	led_gamma_enable_channel_messages = 1;
	addLogAdv (LOG_INFO, LOG_FEATURE_CFG, "RGB  cal %f %f %f",
		g_cfg.led_corr.rgb_cal[0], g_cfg.led_corr.rgb_cal[1], g_cfg.led_corr.rgb_cal[2]);
	addLogAdv (LOG_INFO, LOG_FEATURE_CFG, "LED  gamma %.2f  brtMinRGB %.2f%%  brtMinCW %.2f%%",
		g_cfg.led_corr.led_gamma, g_cfg.led_corr.rgb_bright_min, g_cfg.led_corr.cw_bright_min);
}

commandResult_t led_gamma_control (const void *context, const char *cmd, const char *args, int cmdFlags) {
	int c;

	if (strncmp ("cal", args, 3) == 0) { // calibrate RGB
		float cal_factor[3];
		if (args[3] == 0) { // no parameters - use led_baseColors[] to calculate calibration values
			// find color with highest start-point value:
			int ref_color = 0;
			if (led_baseColors[ref_color] < led_baseColors[1])
				ref_color = 1;
			if (led_baseColors[ref_color] < led_baseColors[2])
				ref_color = 2;
	
			// calculate RGB correction factors:
			for (c = 0; c < 3; c++) {
				cal_factor[c] = (1.0f / led_baseColors[ref_color]) * led_baseColors[c];
			}
		} else { // use parameters as calibration values
			const char *p = args;
			for (c = 0; c < 3; c++) {
				p = strstr (p + 1, " ");
				if (p != NULL) {
					cal_factor[c] = atof (p);
				} else {
					break;
				}
			}
		}
		if (c == 3) { // validate calibration values
			c = 0;
			if ((cal_factor[0] == 1.0f) || (cal_factor[1] == 1.0f) || (cal_factor[2] == 1.0f)) {
				for (; c < 3; c++) {
					if ((cal_factor[c] < 0.0f) || (cal_factor[c] > 1.0f)) {
						break;
					}
				}
			}
			if (c == 3) {
				for (c = 0; c < 3; c++) {
					g_cfg.led_corr.rgb_cal[c] = cal_factor[c];
				}
				// make sure save will happen next frame from main loop
				CFG_MarkAsDirty();
				led_gamma_list();
			}
		}

	} else if (strncmp ("gamma", args, 5) == 0) {
		float gamma_par = atof (args + 6);
		if ((gamma_par >= 1.0f) && (gamma_par <= 3.0f)) {
			g_cfg.led_corr.led_gamma = gamma_par;
			// make sure save will happen next frame from main loop
			CFG_MarkAsDirty();
			led_gamma_list();
		}

	} else if (strncmp ("brtMin", args, 6) == 0) {
		const char *p = strstr (args, " ");
		float bright_min = atof (p);
		if ((bright_min >= 0.0f) && (bright_min <= 10.0f)) {
			if (strncmp ("RGB", args + 6, 3) == 0) {
				g_cfg.led_corr.rgb_bright_min = bright_min;
			} else {
				g_cfg.led_corr.cw_bright_min = bright_min;
			}
			// make sure save will happen next frame from main loop
			CFG_MarkAsDirty();
			led_gamma_list();
		}

	} else if (strcmp ("list", args) == 0) {
		led_gamma_list ();

	} else {
		addLogAdv (LOG_INFO, LOG_FEATURE_CFG,
		           "%s sub-command NOT recognized - Use: cal [f f f], gamma <f>, brtMinRGB <f>, brtMinCW <f>, list", cmd);
	}

	return CMD_RES_OK;
} //


void LED_GetBaseColorString(char * s) {
	byte c[3];

	c[0] = (byte)(led_baseColors[0]);
	c[1] = (byte)(led_baseColors[1]);
	c[2] = (byte)(led_baseColors[2]);

	sprintf(s, "%02X%02X%02X",c[0],c[1],c[2]);
}
float LED_GetTemperature() {
	return led_temperature_current;
}
int LED_GetMode() {
	return g_lightMode;
}

const char *GetLightModeStr(int mode) {
	if(mode == Light_All)
		return "all";
	if(mode == Light_Temperature)
		return "cw";
	if(mode == Light_RGB)
		return "rgb";
	return "er";
}
void SET_LightMode(int newMode) {
	if(g_lightMode != newMode) {
        ADDLOG_DEBUG(LOG_FEATURE_CMD, "Changing LightMode from %s to %s",
			GetLightModeStr(g_lightMode),
			GetLightModeStr(newMode));
		g_lightMode = newMode;
		EventHandlers_FireEvent(CMD_EVENT_LED_MODE, newMode);
	}
}
void LED_SetBaseColorByIndex(int i, float f, bool bApply) {
	if (i < 0 || i >= 5)
		return;
	led_baseColors[i] = f;
	if (bApply) {
		if (CFG_HasFlag(OBK_FLAG_LED_AUTOENABLE_ON_ANY_ACTION)) {
			LED_SetEnableAll(true);
		}

		// set g_lightMode
		SET_LightMode(Light_RGB);
#if ENABLE_MQTT
		sendColorChange();
#endif
		apply_smart_light();
	}
}
void LED_SetTemperature0to1Range(float f) {
	led_temperature_current = led_temperature_min + (led_temperature_max-led_temperature_min) * f;
}
float LED_GetTemperature0to1Range() {
	float f;

	f = (led_temperature_current - led_temperature_min);
	f = f / (led_temperature_max - led_temperature_min);
	if(f<0)
		f = 0;
	if(f>1)
		f =1;

	return f;
}

void LED_SetTemperature(int tmpInteger, bool bApply) {
	float ww,cw,max_cw_ww;

	led_temperature_current = tmpInteger;

	ww = LED_GetTemperature0to1Range();
	cw = 1 - ww;

	//normalize to have a local max of 100%
	if (ww > cw)
		max_cw_ww = ww;
	else
		max_cw_ww = cw;

	ww = ww / max_cw_ww;
	cw = cw / max_cw_ww;

	//uncorrect for gamma to allow gamma correcting brightness later
	if (g_cfg.led_corr.led_gamma >= 1 && g_cfg.led_corr.led_gamma <= 3) {
		ww = powf(ww, 1 / g_cfg.led_corr.led_gamma);
		cw = powf(cw, 1 / g_cfg.led_corr.led_gamma);
	}

	led_baseColors[3] = (255.0f) * cw;
	led_baseColors[4] = (255.0f) * ww;


	if(bApply) {
		if (CFG_HasFlag(OBK_FLAG_LED_AUTOENABLE_ON_ANY_ACTION)) {
			LED_SetEnableAll(true);
		}

		// set g_lightMode
		SET_LightMode(Light_Temperature);
#if ENABLE_MQTT
		sendTemperatureChange();
#endif
		apply_smart_light();
	}

}

static commandResult_t temperature(const void *context, const char *cmd, const char *args, int cmdFlags){
	int tmp;
	//if (!wal_strnicmp(cmd, "POWERALL", 8)){

        ADDLOG_DEBUG(LOG_FEATURE_CMD, " temperature (%s) received with args %s",cmd,args);

		Tokenizer_TokenizeString(args, 0);

		tmp = Tokenizer_GetArgInteger(0);

		LED_SetTemperature(tmp, 1);

		return CMD_RES_OK;
	//}
	//return 0;
}
void LED_ToggleEnabled() {
	LED_SetEnableAll(!g_lightEnableAll);
}
bool g_guard_led_enable_event_cast = false;

void LED_SetStripStateOutputs() {
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		int state = g_cfg.pins.roles[i];
		if (state == IOR_StripState) {
			HAL_PIN_SetOutputValue(i, g_lightEnableAll);
		}
		else if (state == IOR_StripState_n) {
			HAL_PIN_SetOutputValue(i, !g_lightEnableAll);
		}
	}
}
void LED_SetEnableAll(int bEnable) {
	bool bEnableAllWasSetTo1;
	bool bHadChange;

	if (g_lightEnableAll == 0 && bEnable == 1) {
		bEnableAllWasSetTo1 = true;
	}
	else {
		bEnableAllWasSetTo1 = false;
	}

	// was there a change?
	if (g_lightEnableAll != bEnable) {
		// do not cast events recursively...
		// TODO: better fix
		if (g_guard_led_enable_event_cast == false) {
			g_guard_led_enable_event_cast = true;
			// cast event
			if (bEnable) {
				EventHandlers_FireEvent(CMD_EVENT_LED_STATE, 1);
			}
			else {
				EventHandlers_FireEvent(CMD_EVENT_LED_STATE, 0);
			}
			g_guard_led_enable_event_cast = false;
		}
	}
	g_lightEnableAll = bEnable;

	LED_SetStripStateOutputs();
	apply_smart_light();
#if	ENABLE_TASMOTADEVICEGROUPS
	DRV_DGR_OnLedEnableAllChange(bEnable);
#endif
#if ENABLE_MQTT
	LED_SendEnableAllState();
	if (bEnableAllWasSetTo1) {
		// if enable all was set to 1 this frame, also send dimmer
		// https://github.com/openshwprojects/OpenBK7231T_App/issues/498
		// TODO: check if it's OK 
		LED_SendDimmerChange();
	}
#endif
}
int LED_GetEnableAll() {
	return g_lightEnableAll;
}
static commandResult_t enableAll(const void *context, const char *cmd, const char *args, int cmdFlags){
	//if (!wal_strnicmp(cmd, "POWERALL", 8)){
		int bEnable;
		const char *a;
        ADDLOG_DEBUG(LOG_FEATURE_CMD, " enableAll (%s) received with args %s",cmd,args);

		Tokenizer_TokenizeString(args, 0);

		a = Tokenizer_GetArg(0);
		if (a && !stricmp(a, "toggle")) {
			bEnable = !g_lightEnableAll;
		}
		else {
			bEnable = Tokenizer_GetArgInteger(0);
		}

		LED_SetEnableAll(bEnable);


	//	sendColorChange();
	//	sendDimmerChange();
	//	sendTemperatureChange();

		return CMD_RES_OK;
	//}
	//return 0;
}

float LED_GetDimmer() {
	return g_brightness0to100;
}
int dimmer_pingPong_direction = 1;
int temperature_pingPong_direction = 1;
void LED_AddTemperature(int iVal, int wrapAroundInsteadOfClamp) {
	float cur;

	cur = led_temperature_current;

	if (wrapAroundInsteadOfClamp == 2) {
		// Ping pong mode
		cur += iVal * temperature_pingPong_direction;
		if (cur >= led_temperature_max) {
			cur = led_temperature_max;
			temperature_pingPong_direction *= -1;
		}
		if (cur <= led_temperature_min) {
			cur = led_temperature_min;
			temperature_pingPong_direction *= -1;
		}
	}
	else if (wrapAroundInsteadOfClamp == 0) {
		cur += iVal;
		// clamp
		if (cur < led_temperature_min)
			cur = led_temperature_min;
		if (cur > led_temperature_max)
			cur = led_temperature_max;
	}
	else {
		// wrap, but first always reach max/min.
		// So, if we are at min/max then wrap around, otherwise add and clamp...
		if (cur <= led_temperature_min && iVal < 0) {
			// wrapping around from min to max
			cur = led_temperature_max;
		}
		else if (cur >= led_temperature_max && iVal > 0) {
			// wrapping around from max to min
			cur = led_temperature_min;
		}
		else {
			cur += iVal;
			// clamp below is CORRECT. We want it to stop at max/min
			if (cur < led_temperature_min)
				cur = led_temperature_min;
			if (cur > led_temperature_max)
				cur = led_temperature_max;
		}
	}

	LED_SetTemperature(cur, true);
}
void LED_AddDimmer(int iVal, int addMode, int minValue) {
	int cur;

	cur = g_brightness0to100;

	if (addMode == 2) {
		// Ping pong mode
		cur += iVal * dimmer_pingPong_direction;
		if (cur >= 100) {
			cur = 100;
			dimmer_pingPong_direction *= -1;
		}
		if (cur <= minValue) {
			cur = minValue;
			dimmer_pingPong_direction *= -1;
		}
	} else if(addMode == 0) {
		// Clamp mode
		cur += iVal;
		// clamp
		if(cur < minValue)
			cur = minValue;
		if(cur > 100)
			cur = 100;
	} else {
		// wrap, but first always reach max/min.
		// So, if we are at min/max then wrap around, otherwise add and clamp...
		if (cur <= minValue && iVal < 0) {
			// wrapping around from min to max
			cur = 100;
		}
		else if (cur >= 100 && iVal > 0) {
			// wrapping around from max to min
			cur = minValue;
		}
		else {
			cur += iVal;
			if (cur < minValue)
				cur = minValue;
			if (cur > 100)
				cur = 100;
		}
	}

	LED_SetDimmer(cur);
}
void LED_NextTemperatureHold() {
	LED_AddTemperature(25, 1);
}
void LED_NextTemperature() {
	if (g_lightMode != Light_Temperature) {
		LED_SetTemperature(HASS_TEMPERATURE_MIN, true);
		return;
	}
	if (led_temperature_current == HASS_TEMPERATURE_MIN) {
		LED_SetTemperature(HASS_TEMPERATURE_CENTER, true);
	} else if (led_temperature_current == HASS_TEMPERATURE_CENTER) {
		LED_SetTemperature(HASS_TEMPERATURE_MAX, true);
	}
	else {
		LED_SetTemperature(HASS_TEMPERATURE_MIN, true);
	}
}
void LED_NextDimmerHold() {
	// dimmer hold will use some kind of min value,
	// because it's easy to get confused if we set accidentally dimmer to 0
	// and then are unable to turn on the bulb (because despite of led_enableAll 1
	// the dimmer is 0 and anyColor * 0 gives 0)
	LED_AddDimmer(led_defaultDimmerDeltaForHold, 1, 2);
}
void LED_SetDimmerForDisplayOnly(int iVal) {
	g_brightness0to100 = iVal;
}
void LED_SetDimmerIfChanged(int iVal) {
	if (g_brightness0to100 != iVal) {
		LED_SetDimmer(iVal);
	}
}
void LED_SetDimmer(int iVal) {

	g_brightness0to100 = iVal;

	if (CFG_HasFlag(OBK_FLAG_LED_AUTOENABLE_ON_ANY_ACTION)) {
		LED_SetEnableAll(true);
	}

#if	ENABLE_TASMOTADEVICEGROUPS
	DRV_DGR_OnLedDimmerChange(iVal);
#endif

	apply_smart_light();
#if ENABLE_MQTT
	LED_SendDimmerChange();

	if(shouldSendRGB()) {
		if(CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTLEDPARAMSTOGETHER)) {
			sendColorChange();
		}
		if(CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTLEDFINALCOLOR)) {
			sendFinalColor();
		}
	}
#endif

}
static commandResult_t add_temperature(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int iVal = 0;
	int bWrapAroundInsteadOfHold;

	Tokenizer_TokenizeString(args, 0);

	iVal = Tokenizer_GetArgInteger(0);
	bWrapAroundInsteadOfHold = Tokenizer_GetArgInteger(1);

	LED_AddTemperature(iVal, bWrapAroundInsteadOfHold);

	return CMD_RES_OK;
}
static commandResult_t add_dimmer(const void *context, const char *cmd, const char *args, int cmdFlags){
	int iVal = 0;
	int addMode;

	Tokenizer_TokenizeString(args, 0);

	iVal = Tokenizer_GetArgInteger(0);
	addMode = Tokenizer_GetArgInteger(1);

	LED_AddDimmer(iVal, addMode, 0);

	return CMD_RES_OK;
}
static commandResult_t dimmer(const void *context, const char *cmd, const char *args, int cmdFlags){
	//if (!wal_strnicmp(cmd, "POWERALL", 8)){
		int iVal = 0;

        ADDLOG_DEBUG(LOG_FEATURE_CMD, " dimmer (%s) received with args %s",cmd,args);

		// according to Elektroda.com users, domoticz sends following string:
		// {"brightness":52,"state":"ON"}
		if(args[0] == '{') {
			cJSON *json;
			const cJSON *brightness = NULL;
			const cJSON *state = NULL;

			json = cJSON_Parse(args);

			if(json == 0) {
				ADDLOG_INFO(LOG_FEATURE_CMD, "Dimmer - failed cJSON_Parse");
			} else {
				brightness = cJSON_GetObjectItemCaseSensitive(json, "brightness");
				if (brightness != 0 && cJSON_IsNumber(brightness))
				{
					ADDLOG_INFO(LOG_FEATURE_CMD, "Dimmer - cJSON_Parse says brightness is %i",brightness->valueint);

					LED_SetDimmer(brightness->valueint);
				}
				state = cJSON_GetObjectItemCaseSensitive(json, "state");
				if (state != 0 && cJSON_IsString(state) && (state->valuestring != NULL))
				{
					ADDLOG_INFO(LOG_FEATURE_CMD, "Dimmer - cJSON_Parse says state is %s",state->valuestring);

					if(!stricmp(state->valuestring,"ON")) {
						LED_SetEnableAll(true);
					} else if(!stricmp(state->valuestring,"OFF")) {
						LED_SetEnableAll(false);
					} else {

					}
				}
				cJSON_Delete(json);
			}
		} else {
			Tokenizer_TokenizeString(args, 0);

			iVal = Tokenizer_GetArgInteger(0);

			LED_SetDimmer(iVal);
		}

		return CMD_RES_OK;
}
void LED_SetFinalRGBCW(byte *rgbcw) {
	if(rgbcw[0] == 0 && rgbcw[1] == 0 && rgbcw[2] == 0 && rgbcw[3] == 0 && rgbcw[4] == 0) {

	}

	if(rgbcw[3] == 0 && rgbcw[4] == 0) {
		LED_SetFinalRGB(rgbcw[0],rgbcw[1],rgbcw[2]);
	} else {
		LED_SetFinalCW(rgbcw[3],rgbcw[4]);
	}
}
void LED_GetFinalChannels100(byte *rgbcw) {
	rgbcw[0] = finalColors[0] * (100.0f / 255.0f);
	rgbcw[1] = finalColors[1] * (100.0f / 255.0f);
	rgbcw[2] = finalColors[2] * (100.0f / 255.0f);
	rgbcw[3] = finalColors[3] * (100.0f / 255.0f);
	rgbcw[4] = finalColors[4] * (100.0f / 255.0f);
}
void LED_GetTasmotaHSV(int *hsv) {
	hsv[0] = g_hsv_h;
	hsv[1] = g_hsv_s * 100;
	hsv[2] = g_hsv_v * 100;
}
void LED_GetFinalRGBCW(byte *rgbcw) {
	rgbcw[0] = finalColors[0];
	rgbcw[1] = finalColors[1];
	rgbcw[2] = finalColors[2];
	rgbcw[3] = finalColors[3];
	rgbcw[4] = finalColors[4];
}
void LED_SetFinalCW(byte c, byte w) {
	float tmp;

	SET_LightMode(Light_Temperature);

	// TODO: finish the calculation,
	// the Device Group sent as White and Cool values in byte range,
	// we need to get back Temperature value
	tmp = w / 255.0f;

	LED_SetTemperature0to1Range(tmp);

	if (CFG_HasFlag(OBK_FLAG_LED_AUTOENABLE_ON_ANY_ACTION)) {
		LED_SetEnableAll(true);
	}

	led_baseColors[3] = c;
	led_baseColors[4] = w;

	apply_smart_light();
}
void LED_SetFinalRGBW(byte r, byte g, byte b, byte w) {
	SET_LightMode(Light_All);
	led_baseColors[0] = r;
	led_baseColors[1] = g;
	led_baseColors[2] = b;

	// half between Cool and Warm
    float w_half = w / 2 / 255.0f;
    LED_SetTemperature0to1Range(0.5f);

    //uncorrect for gamma to allow gamma correcting brightness later
	if (g_cfg.led_corr.led_gamma >= 1 && g_cfg.led_corr.led_gamma <= 3) {
		w_half = powf(w_half, 1 / g_cfg.led_corr.led_gamma);
	}

	led_baseColors[3] = (255.0f) * w_half;
	led_baseColors[4] = (255.0f) * w_half;

	RGBtoHSV(led_baseColors[0] / 255.0f, led_baseColors[1] / 255.0f, led_baseColors[2] / 255.0f, &g_hsv_h, &g_hsv_s, &g_hsv_v);

	if (CFG_HasFlag(OBK_FLAG_LED_AUTOENABLE_ON_ANY_ACTION)) {
		LED_SetEnableAll(true);
	}

	apply_smart_light();
}
void LED_SetFinalRGB(byte r, byte g, byte b) {
	SET_LightMode(Light_RGB);

	led_baseColors[0] = r;
	led_baseColors[1] = g;
	led_baseColors[2] = b;

	RGBtoHSV(led_baseColors[0]/255.0f, led_baseColors[1]/255.0f, led_baseColors[2]/255.0f, &g_hsv_h, &g_hsv_s, &g_hsv_v);

	if (CFG_HasFlag(OBK_FLAG_LED_AUTOENABLE_ON_ANY_ACTION)) {
		LED_SetEnableAll(true);
	}

	apply_smart_light();

#if ENABLE_MQTT
	// TODO
	if(0) {
		sendColorChange();
		if(CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTLEDPARAMSTOGETHER)) {
			LED_SendDimmerChange();
		}
		if(CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTLEDFINALCOLOR)) {
			sendFinalColor();
		}
	}
#endif
}
static void onHSVChanged() {
	float r, g, b;

	HSVtoRGB(&r, &g, &b, g_hsv_h, g_hsv_s, g_hsv_v);

	led_baseColors[0] = r * 255.0f;
	led_baseColors[1] = g * 255.0f;
	led_baseColors[2] = b * 255.0f;

	if (CFG_HasFlag(OBK_FLAG_LED_AUTOENABLE_ON_ANY_ACTION)) {
		LED_SetEnableAll(true);
	}


	apply_smart_light();


#if ENABLE_MQTT
	sendColorChange();
	if (CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTLEDFINALCOLOR)) {
		sendFinalColor();
	}
#endif
}
commandResult_t LED_SetBaseColor_HSB(const void *context, const char *cmd, const char *args, int bAll) {
	int hue, sat, bri;
	const char *p;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 1) {
		p = args;
		hue = atoi(p);
		while (*p) {
			if (*p == ',')
			{
				p++;
				break;
			}
			p++;
		}
		sat = atoi(p);
		while (*p) {
			if (*p == ',')
			{
				p++;
				break;
			}
			p++;
		}
		bri = atoi(p);
	}
	else {
		hue = Tokenizer_GetArgInteger(0);
		sat = Tokenizer_GetArgInteger(1);
		bri = Tokenizer_GetArgInteger(2);
	}

	SET_LightMode(Light_RGB);

	g_hsv_h = hue;
	g_hsv_s = sat * 0.01f;
	g_hsv_v = bri * 0.01f;

	onHSVChanged();

	return CMD_RES_OK;
}
commandResult_t LED_SetBaseColor(const void *context, const char *cmd, const char *args, int bAll){
   // support both '#' prefix and not
            const char *c = args;
            int val = 0;
            ADDLOG_DEBUG(LOG_FEATURE_CMD, " BASECOLOR got %s", args);

			// some people prefix colors with #
			if(c[0] == '#')
				c++;


			if (CFG_HasFlag(OBK_FLAG_LED_SETTING_WHITE_RGB_ENABLES_CW)) {
				if (!stricmp(c, "FFFFFF")) {
					SET_LightMode(Light_Temperature);
#if ENABLE_MQTT
					sendTemperatureChange();
#endif
					apply_smart_light();
					return CMD_RES_OK;
				}
			}

			if(bAll) {
				SET_LightMode(Light_All);
			} else {
				SET_LightMode(Light_RGB);
			}

			g_numBaseColors = 0;
			if(!stricmp(c,"rand")) {
				led_baseColors[0] = rand()%255;
				led_baseColors[1] = rand()%255;
				led_baseColors[2] = rand()%255;
				if(bAll){
					led_baseColors[3] = rand()%255;
					led_baseColors[4] = rand()%255;
				}
			}
			else if (strchr(c, ',')) {
				// parse format like: 255,50,0
				int r, g, b;
				sscanf(c, "%d,%d,%d", &r, &g, &b);
				led_baseColors[0] = r;
				led_baseColors[1] = g;
				led_baseColors[2] = b;
			} else {
				while (*c && g_numBaseColors < 5){
					char tmp[3];
					int r;
					tmp[0] = *(c++);
					if (!*c)
						break;
					tmp[1] = *(c++);
					tmp[2] = '\0';
					r = sscanf(tmp, "%x", &val);
					if (!r) {
						ADDLOG_ERROR(LOG_FEATURE_CMD, "BASECOLOR no sscanf hex result from %s", tmp);
						break;
					}


					//ADDLOG_DEBUG(LOG_FEATURE_CMD, "BASECOLOR found chan %d -> val255 %d (from %s)", g_numBaseColors, val, tmp);

					led_baseColors[g_numBaseColors] = val;
				//	baseColorChannels[g_numBaseColors] = channel;
					g_numBaseColors++;

				}
				// keep hsv in sync
			}

			RGBtoHSV(led_baseColors[0]/255.0f, led_baseColors[1]/255.0f, led_baseColors[2]/255.0f, &g_hsv_h, &g_hsv_s, &g_hsv_v);

			if (CFG_HasFlag(OBK_FLAG_LED_AUTOENABLE_ON_ANY_ACTION)) {
				LED_SetEnableAll(true);
			}
			apply_smart_light();
#if ENABLE_MQTT
			sendColorChange();
			if(CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTLEDPARAMSTOGETHER)) {
				LED_SendDimmerChange();
			}
			if(CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTLEDFINALCOLOR)) {
				sendFinalColor();
			}
#endif

        return CMD_RES_OK;
  //  }
   // return 0;
}

static commandResult_t basecolor_rgb(const void *context, const char *cmd, const char *args, int cmdFlags){
	return LED_SetBaseColor(context,cmd,args,0);
}
static commandResult_t basecolor_rgbcw(const void *context, const char *cmd, const char *args, int cmdFlags){
	return LED_SetBaseColor(context,cmd,args,1);
}

// CONFIG-ONLY command!
static commandResult_t colorMult(const void *context, const char *cmd, const char *args, int cmdFlags){
        ADDLOG_DEBUG(LOG_FEATURE_CMD, " g_cfg_colorScaleToChannel (%s) received with args %s",cmd,args);

		g_cfg_colorScaleToChannel = atof(args);

		return CMD_RES_OK;
	//}
	//return 0;
}

float LED_GetGreen255() {
	return led_baseColors[1];
}
float LED_GetRed255() {
	return led_baseColors[0];
}
float LED_GetBlue255() {
	return led_baseColors[2];
}
static void led_setBrightness(float sat) {

	g_hsv_v = sat;

	onHSVChanged();
}
static void led_setSaturation(float sat){

	g_hsv_s = sat;

	onHSVChanged();
}
static void led_setHue(float hue){

	g_hsv_h = hue;

	onHSVChanged();
}
static commandResult_t nextColor(const void *context, const char *cmd, const char *args, int cmdFlags){
   
	LED_NextColor();

	return CMD_RES_OK;
}
static commandResult_t lerpSpeed(const void *context, const char *cmd, const char *args, int cmdFlags){
	// Use tokenizer, so we can use variables (eg. $CH11 as variable)
	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}


	led_lerpSpeedUnitsPerSecond = Tokenizer_GetArgFloat(0);

	return CMD_RES_OK;
}
static commandResult_t cmdDimmerScale(const void *context, const char *cmd, const char *args, int cmdFlags) {
	// Use tokenizer, so we can use variables (eg. $CH11 as variable)
	Tokenizer_TokenizeString(args, 0);

	g_brightnessScale = Tokenizer_GetArgFloat(0);

	apply_smart_light();
	return CMD_RES_OK;
}
static commandResult_t cmdSaveStateIfModifiedInterval(const void *context, const char *cmd, const char *args, int cmdFlags) {
	// Use tokenizer, so we can use variables (eg. $CH11 as variable)
	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	led_saveStateIfModifiedInterval = Tokenizer_GetArgInteger(0);

	return CMD_RES_OK;
}
static commandResult_t dimmerDelta(const void *context, const char *cmd, const char *args, int cmdFlags) {
	// Use tokenizer, so we can use variables (eg. $CH11 as variable)
	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	led_defaultDimmerDeltaForHold = Tokenizer_GetArgInteger(0);

	return CMD_RES_OK;
}
static commandResult_t ctRange(const void *context, const char *cmd, const char *args, int cmdFlags) {
	// Use tokenizer, so we can use variables (eg. $CH11 as variable)
	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	led_temperature_min = Tokenizer_GetArgFloat(0);
	led_temperature_max = Tokenizer_GetArgFloat(1);

	return CMD_RES_OK;
}
static commandResult_t setBrightness(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float f;

	// Use tokenizer, so we can use variables (eg. $CH11 as variable)
	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	f = Tokenizer_GetArgFloat(0);

	// input is in 0-100 range
	f *= 0.01f;

	SET_LightMode(Light_RGB);

	led_setBrightness(f);

	return CMD_RES_OK;
}
static commandResult_t led_finishFullLerp(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if (CFG_HasFlag(OBK_FLAG_LED_SMOOTH_TRANSITIONS) == true) {
		LED_RunQuickColorLerp(9999.0f);
	}

	return CMD_RES_OK;
}
static commandResult_t setSaturation(const void *context, const char *cmd, const char *args, int cmdFlags){
    float f;

	// Use tokenizer, so we can use variables (eg. $CH11 as variable)
	Tokenizer_TokenizeString(args, 0);

	f = Tokenizer_GetArgFloat(0);

	// input is in 0-100 range
	f *= 0.01f;

	SET_LightMode(Light_RGB);

	led_setSaturation(f);

	return CMD_RES_OK;
}
float LED_GetSaturation() {
	return g_hsv_s * 100.0f;
}
static commandResult_t setHue(const void *context, const char *cmd, const char *args, int cmdFlags){
    float f;

	// Use tokenizer, so we can use variables (eg. $CH11 as variable)
	Tokenizer_TokenizeString(args, 0);

	f = Tokenizer_GetArgFloat(0);

	SET_LightMode(Light_RGB);

	led_setHue(f);

	return CMD_RES_OK;
}

float LED_GetHue() {
	return g_hsv_h;
}

commandResult_t commandSetPaletteColor(const void *context, const char *cmd, const char *args, int cmdFlags);

void NewLED_InitCommands(){
	int pwmCount;

	g_brightnessScale = 1.0f;

	// set, but do not apply (force a refresh)
	LED_SetTemperature(led_temperature_current,0);

	PIN_get_Relay_PWM_Count(0, &pwmCount, 0);

	// if this is CW, switch from default RGB to CW
	if (isCWMode()) {
		g_lightMode = Light_Temperature;
	}
	else if (pwmCount == 1 || pwmCount == 3) {
		// if single color or RGB, force RGB
		g_lightMode = Light_RGB;
	}

	//cmddetail:{"name":"led_dimmer","args":"[Value]",
	//cmddetail:"descr":"set output dimmer 0..100",
	//cmddetail:"fn":"dimmer","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("led_dimmer", dimmer, NULL);
	//cmddetail:{"name":"Dimmer","args":"[Value]",
	//cmddetail:"descr":"Alias for led_dimmer, added for Tasmota.",
	//cmddetail:"fn":"dimmer","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("Dimmer", dimmer, NULL);
	//cmddetail:{"name":"add_dimmer","args":"[Value][AddMode]",
	//cmddetail:"descr":"Adds a given value to current LED dimmer. AddMode 0 just adds a value (with a clamp to [0,100]), AddMode 1 will wrap around values (going under 0 goes to 100, going over 100 goes to 0), AddMode 2 will ping-pong value (going to 100 starts going back from 100 to 0, and again, going to 0 starts going up).",
	//cmddetail:"fn":"add_dimmer","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("add_dimmer", add_dimmer, NULL);
	//cmddetail:{"name":"led_enableAll","args":"[1or0orToggle]",
	//cmddetail:"descr":"Power on/off LED but remember the RGB(CW) values",
	//cmddetail:"fn":"enableAll","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("led_enableAll", enableAll, NULL);
	//cmddetail:{"name":"led_basecolor_rgb","args":"[HexValue]",
	//cmddetail:"descr":"Puts the LED driver in RGB mode and sets given color.",
	//cmddetail:"fn":"basecolor_rgb","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("led_basecolor_rgb", basecolor_rgb, NULL);
	//cmddetail:{"name":"Color","args":"[HexValue]",
	//cmddetail:"descr":"Puts the LED driver in RGB mode and sets given color.",
	//cmddetail:"fn":"basecolor_rgb","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("Color", basecolor_rgb, NULL);
	//cmddetail:{"name":"led_basecolor_rgbcw","args":"[HexValue]",
	//cmddetail:"descr":"set PWN color using #RRGGBB[cw][ww]",
	//cmddetail:"fn":"basecolor_rgbcw","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("led_basecolor_rgbcw", basecolor_rgbcw, NULL);
	//cmddetail:{"name":"add_temperature","args":"[DeltaValue][bWrapAroundInsteadOfHold]",
	//cmddetail:"descr":"Adds a given value to current LED temperature. Function can wrap or clamp if max/min is exceeded.",
	//cmddetail:"fn":"add_temperature","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("add_temperature", add_temperature, NULL);
	//cmddetail:{"name":"led_temperature","args":"[TempValue]",
	//cmddetail:"descr":"Toggles LED driver into temperature mode and sets given temperature. It using Home Assistant temperature range (in the range from 154-500 defined in homeassistant/util/color.py as HASS_COLOR_MIN and HASS_COLOR_MAX)",
	//cmddetail:"fn":"temperature","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("led_temperature", temperature, NULL);
	//cmddetail:{"name":"CT","args":"[TempValue]",
	//cmddetail:"descr":"Sets the LED temperature. Same as led_temperature but with Tasmota syntax.",
	//cmddetail:"fn":"temperature","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("CT", temperature, NULL);
	//cmddetail:{"name":"led_colorMult","args":"[Value]",
	//cmddetail:"descr":"Internal usage.",
	//cmddetail:"fn":"colorMult","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("led_colorMult", colorMult, NULL);
	//cmddetail:{"name":"led_saturation","args":"[Value]",
	//cmddetail:"descr":"This is an alternate way to set the LED color.",
	//cmddetail:"fn":"setSaturation","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("led_saturation", setSaturation, NULL);
	//cmddetail:{"name":"led_hue","args":"[Value]",
	//cmddetail:"descr":"This is an alternate way to set the LED color.",
	//cmddetail:"fn":"setHue","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("led_hue", setHue, NULL);
	//cmddetail:{"name":"led_nextColor","args":"",
	//cmddetail:"descr":"Sets the next color from predefined colours list. Our list is the same as in Tasmota.",
	//cmddetail:"fn":"nextColor","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("led_nextColor", nextColor, NULL);
	//cmddetail:{"name":"led_lerpSpeed","args":"[LerpSpeed]",
	//cmddetail:"descr":"Sets the speed of colour interpolation, where speed is defined as a number of RGB units per second, so 255 will lerp from 0 to 255 in one second",
	//cmddetail:"fn":"lerpSpeed","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("led_lerpSpeed", lerpSpeed, NULL);
	// HSBColor 360,100,100 - red
	// HSBColor 90,100,100 - green
	// HSBColor	<hue>,<sat>,<bri> = set color by hue, saturation and brightness
	//cmddetail:{"name":"HSBColor","args":"[H][S][B]",
	//cmddetail:"descr":"Tasmota-style colour access. Hue in 0-360 range, saturation in 0-100 and brightness in 0-100 range. ",
	//cmddetail:"fn":"LED_SetBaseColor_HSB","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("HSBColor", LED_SetBaseColor_HSB, NULL);
	// HSBColor1	0..360 = set hue
	//cmddetail:{"name":"HSBColor1","args":"[Hue]",
	//cmddetail:"descr":"Tasmota-style colour access. Sets hue in 0 to 360 range.",
	//cmddetail:"fn":"setHue","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("HSBColor1", setHue, NULL);
	// HSBColor2	0..100 = set saturation
	//cmddetail:{"name":"HSBColor2","args":"[Saturation]",
	//cmddetail:"descr":"Tasmota-style colour access. Set saturation in 0 to 100 range.",
	//cmddetail:"fn":"setSaturation","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("HSBColor2", setSaturation, NULL);
	// HSBColor3	0..100 = set brightness
	//cmddetail:{"name":"HSBColor3","args":"[Brightness]",
	//cmddetail:"descr":"Tasmota-style colour access. Sets brightness in 0 to 100 range.",
	//cmddetail:"fn":"setBrightness","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("HSBColor3", setBrightness, NULL);
	//cmddetail:{"name":"led_finishFullLerp","args":"",
	//cmddetail:"descr":"This will force-finish LED color interpolation. You can call it after setting the colour to skip the interpolation/smooth transition time. Of course, it makes only sense if you enabled smooth colour transitions.",
	//cmddetail:"fn":"led_finishFullLerp","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("led_finishFullLerp", led_finishFullLerp, NULL);
	//cmddetail:{"name":"led_gammaCtrl","args":"sub-cmd [par]",
	//cmddetail:"descr":"control LED Gamma Correction and Calibration",
	//cmddetail:"fn":"led_gamma_control","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":"led_gammaCtrl on"}
    CMD_RegisterCommand("led_gammaCtrl", led_gamma_control, NULL);
	//cmddetail:{"name":"CTRange","args":"[MinRange][MaxRange]",
	//cmddetail:"descr":"This sets the temperature range for display. Default is 154-500.",
	//cmddetail:"fn":"ctRange","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("CTRange", ctRange, NULL);
	//cmddetail:{"name":"DimmerDelta","args":"[DeltaValue]",
	//cmddetail:"descr":"This sets the delta value for SmartDimmer/SmartButtonForLEDs hold event. This determines the amount of change of dimmer per hold event.",
	//cmddetail:"fn":"dimmerDelta","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("DimmerDelta", dimmerDelta, NULL);
	//cmddetail:{"name":"led_saveInterval","args":"[IntervalSeconds]",
	//cmddetail:"descr":"This determines how often LED state can be saved to flash memory. The state is saved only if it was modified and if the flag for LED state save is enabled. Set this to higher value if you are changing LED states very often, for example from xLights. Saving too often could wear out flash memory too fast.",
	//cmddetail:"fn":"cmdSaveStateIfModifiedInterval","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("led_saveInterval", cmdSaveStateIfModifiedInterval, NULL);

	//cmddetail:{"name":"led_dimmerScale","args":"[Scale]",
	//cmddetail:"descr":"led_dimmerScale is a simple way of decreasing LED bulbs heating. led_dimmerScale expects argument in 0-1 range. For example, if you set led_dimmerScale to 0.8, then OBK/Home Assistant dimmer at 100% will in reality translate to 80% PWM duty cycle, so LEDs will be slightly darker, but they will heat less and it will prolong LEDs life..",
	//cmddetail:"fn":"cmdDimmerScale","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("led_dimmerScale", cmdDimmerScale, NULL);

	
	//cmddetail:{"name":"SPC","args":"[Index][RGB]",
	//cmddetail:"descr":"Sets Palette Color by index.",
	//cmddetail:"fn":"commandSetPaletteColor","file":"cmnds/cmd_newLEDDriver.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SPC", commandSetPaletteColor, NULL);
	
}

void NewLED_RestoreSavedStateIfNeeded() {
	if(CFG_HasFlag(OBK_FLAG_LED_REMEMBERLASTSTATE)) {
		short brig;
		short tmp;
		byte rgb[3];
		byte mod;
		byte bEnableAll;

		HAL_FlashVars_ReadLED(&mod, &brig, &tmp, rgb, &bEnableAll);

		g_lightEnableAll = bEnableAll;
		SET_LightMode(mod);
		g_brightness0to100 = brig;
		LED_SetTemperature(tmp,0);
		led_baseColors[0] = rgb[0];
		led_baseColors[1] = rgb[1];
		led_baseColors[2] = rgb[2];
		apply_smart_light();
	} else {
	}

	// "cmnd/obk8D38570E/led_dimmer_get""
}
#endif
