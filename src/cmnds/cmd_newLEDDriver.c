
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "cmd_public.h"
#include "../obk_config.h"
#include "../driver/drv_public.h"
#include "../rgb2hsv.h"
#include <ctype.h>
#include "cmd_local.h"
#include "../mqtt/new_mqtt.h"
#ifdef BK_LITTLEFS
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


int g_lightMode = Light_RGB;
// Those are base colors, normalized, without brightness applied
float baseColors[5] = { 255, 255, 255, 255, 255 };
float finalColors[5] = { 255, 255, 255, 255, 255 };
float g_hsv_h = 0; // 0 to 360
float g_hsv_s = 0; // 0 to 1
float g_hsv_v = 1; // 0 to 1
// By default, colors are in 255 to 0 range, while our channels accept 0 to 100 range
float g_cfg_colorScaleToChannel = 100.0f/255.0f;
int g_numBaseColors = 5;
float g_brightness = 1.0f;

// NOTE: in this system, enabling/disabling whole led light bulb
// is not changing the stored channel and brightness values.
// They are kept intact so you can reenable the bulb and keep your color setting
int g_lightEnableAll = 1;
// config only stuff
float g_cfg_brightnessMult = 0.01f;

// the slider control in the UI emits values
//in the range from 154-500 (defined
//in homeassistant/util/color.py as HASS_COLOR_MIN and HASS_COLOR_MAX).
float led_temperature_min = HASS_TEMPERATURE_MIN;
float led_temperature_max = HASS_TEMPERATURE_MAX;
float led_temperature_current = 0;

void apply_smart_light() {
	int i;
	int firstChannelIndex;
	int channelToUse;
	byte finalRGBCW[5];

	// The color order is RGBCW.
	// some people set RED to channel 0, and some of them set RED to channel 1
	// Let's detect if there is a PWM on channel 0
	if(CHANNEL_HasChannelPinWithRole(0, IOR_PWM)) {
		firstChannelIndex = 0;
	} else {
		firstChannelIndex = 1;
	}

	for(i = 0; i < 5; i++) {
		float raw, final;

		raw = baseColors[i];

		if(g_lightEnableAll) {
			final = raw * g_brightness;
		} else {
			final = 0;
		}
		if(g_lightMode == Light_Temperature) {
			// skip channels 0, 1, 2
			// (RGB)
			if(i < 3)
			{
				final = 0;
			}
		} else if(g_lightMode == Light_RGB) {
			// skip channels 3, 4
			if(i >= 3)
			{
				final = 0;
			}
		} else {

		}
		finalColors[i] = final;
		finalRGBCW[i] = final;

		channelToUse = firstChannelIndex + i;

		// log printf with %f crashes N platform?
		//ADDLOG_INFO(LOG_FEATURE_CMD, "apply_smart_light: ch %i raw is %f, bright %f, final %f, enableAll is %i",
		//	channelToUse,raw,g_brightness,final,g_lightEnableAll);

		CHANNEL_Set(channelToUse, final * g_cfg_colorScaleToChannel, CHANNEL_SET_FLAG_SKIP_MQTT);
	}
#ifndef OBK_DISABLE_ALL_DRIVERS
	if(DRV_IsRunning("SM2135")) {
		SM2135_Write(finalRGBCW);
	}
#endif
}
static void sendColorChange() {
	char s[16];
	byte c[3];

	c[0] = (byte)(baseColors[0]);
	c[1] = (byte)(baseColors[1]);
	c[2] = (byte)(baseColors[2]);
	
	sprintf(s,"%02X%02X%02X",c[0],c[1],c[2]);

	MQTT_PublishMain_StringString("led_basecolor_rgb",s);
}
void LED_GetBaseColorString(char * s) {
	byte c[3];

	c[0] = (byte)(baseColors[0]);
	c[1] = (byte)(baseColors[1]);
	c[2] = (byte)(baseColors[2]);
	
	sprintf(s,"%02X%02X%02X",c[0],c[1],c[2]);
}
static void sendFinalColor() {
	char s[16];
	byte c[3];

	c[0] = (byte)(finalColors[0]);
	c[1] = (byte)(finalColors[1]);
	c[2] = (byte)(finalColors[2]);
	
	sprintf(s,"%02X%02X%02X",c[0],c[1],c[2]);

	MQTT_PublishMain_StringString("led_finalcolor_rgb",s);
}
static void sendDimmerChange() {
	int iValue;

	iValue = g_brightness / g_cfg_brightnessMult;

	MQTT_PublishMain_StringInt("led_dimmer", iValue);
}
static void sendTemperatureChange(){
	MQTT_PublishMain_StringInt("led_temperature", (int)led_temperature_current);
}
float LED_GetTemperature() {
	return led_temperature_current;
}
int LED_GetMode() {
	return g_lightMode;
}

void LED_SetTemperature(int tmpInteger, bool bApply) {
	float f;
	
	led_temperature_current = tmpInteger;

	f = (tmpInteger - led_temperature_min);
	f = f / (led_temperature_max - led_temperature_min);
	if(f<0)
		f = 0;
	if(f>1)
		f =1;

     ///   ADDLOG_INFO(LOG_FEATURE_CMD, "tasCmnd temperature frac %f",f);

	baseColors[3] = (255.0f) * (1-f);
	baseColors[4] = (255.0f) * f;

	if(bApply) {
		g_lightMode = Light_Temperature;
		sendTemperatureChange();
		apply_smart_light();
	}

}

static int temperature(const void *context, const char *cmd, const char *args, int cmdFlags){
	int tmp;
	//if (!wal_strnicmp(cmd, "POWERALL", 8)){

        ADDLOG_INFO(LOG_FEATURE_CMD, " temperature (%s) received with args %s",cmd,args);


		tmp = atoi(args);

		LED_SetTemperature(tmp, 1);

		return 1;
	//}
	//return 0;
}
void LED_SetEnableAll(int bEnable) {
	g_lightEnableAll = bEnable;

	apply_smart_light();
#ifndef OBK_DISABLE_ALL_DRIVERS
	DRV_DGR_OnLedEnableAllChange(bEnable);
#endif

	MQTT_PublishMain_StringInt("led_enableAll",g_lightEnableAll);
}
int LED_GetEnableAll() {
	return g_lightEnableAll;
}
static int enableAll(const void *context, const char *cmd, const char *args, int cmdFlags){
	//if (!wal_strnicmp(cmd, "POWERALL", 8)){
		int bEnable;
        ADDLOG_INFO(LOG_FEATURE_CMD, " enableAll (%s) received with args %s",cmd,args);

		bEnable = parsePowerArgument(args);

		LED_SetEnableAll(bEnable);

	
	//	sendColorChange();
	//	sendDimmerChange();
	//	sendTemperatureChange();

		return 1;
	//}
	//return 0;
}
float LED_GetDimmer() {
	return g_brightness / g_cfg_brightnessMult;
}
void LED_SetDimmer(int iVal) {

	g_brightness = iVal * g_cfg_brightnessMult;

#ifndef OBK_DISABLE_ALL_DRIVERS
	DRV_DGR_OnLedDimmerChange(iVal);
#endif

	apply_smart_light();
	sendDimmerChange();
	if(CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTLEDPARAMSTOGETHER)) {
		sendColorChange();
	}
	if(CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTLEDFINALCOLOR)) {
		sendFinalColor();
	}

}
static int dimmer(const void *context, const char *cmd, const char *args, int cmdFlags){
	//if (!wal_strnicmp(cmd, "POWERALL", 8)){
		int iVal = 0;

        ADDLOG_INFO(LOG_FEATURE_CMD, " dimmer (%s) received with args %s",cmd,args);

		iVal = parsePowerArgument(args);

		LED_SetDimmer(iVal);

		return 1;
	//}
	//return 0;
}
int LED_SetBaseColor(const void *context, const char *cmd, const char *args, int bAll){
   // support both '#' prefix and not
            const char *c = args;
            int val = 0;
            ADDLOG_DEBUG(LOG_FEATURE_CMD, " BASECOLOR got %s", args);

			// some people prefix colors with #
			if(c[0] == '#')
				c++;

			if(bAll) {
				g_lightMode = Light_All;
			} else {
				g_lightMode = Light_RGB;
			}

			g_numBaseColors = 0;
			if(!stricmp(c,"rand")) {
				baseColors[0] = rand()%255;
				baseColors[1] = rand()%255;
				baseColors[2] = rand()%255;
				if(bAll){
					baseColors[3] = rand()%255;
					baseColors[4] = rand()%255;
				}
			} else {
				while (*c){
					char tmp[3];
					int r;
					tmp[0] = *(c++);
					if (!*c) break;
					tmp[1] = *(c++);
					tmp[2] = '\0';
					r = sscanf(tmp, "%x", &val);
					if (!r) {
						ADDLOG_ERROR(LOG_FEATURE_CMD, "BASECOLOR no sscanf hex result from %s", tmp);
						break;
					}


					//ADDLOG_DEBUG(LOG_FEATURE_CMD, "BASECOLOR found chan %d -> val255 %d (from %s)", g_numBaseColors, val, tmp);

					baseColors[g_numBaseColors] = val;
				//	baseColorChannels[g_numBaseColors] = channel;
					g_numBaseColors++;

				}
				// keep hsv in sync
			}
			
			RGBtoHSV(baseColors[0]/255.0f, baseColors[1]/255.0f, baseColors[2]/255.0f, &g_hsv_h, &g_hsv_s, &g_hsv_v);

			apply_smart_light();
			sendColorChange();
			if(CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTLEDPARAMSTOGETHER)) {
				sendDimmerChange();
			}
			if(CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTLEDFINALCOLOR)) {
				sendFinalColor();
			}


            //if (!(*c)){
                //ADDLOG_DEBUG(LOG_FEATURE_CMD, "BASECOLOR arg ended");
           // }
        return 1;
  //  }
   // return 0;
}

static int basecolor_rgb(const void *context, const char *cmd, const char *args, int cmdFlags){
	return LED_SetBaseColor(context,cmd,args,0);
}
static int basecolor_rgbcw(const void *context, const char *cmd, const char *args, int cmdFlags){
	return LED_SetBaseColor(context,cmd,args,1);
}

// CONFIG-ONLY command!
static int colorMult(const void *context, const char *cmd, const char *args, int cmdFlags){
        ADDLOG_INFO(LOG_FEATURE_CMD, " g_cfg_colorScaleToChannel (%s) received with args %s",cmd,args);

		g_cfg_colorScaleToChannel = atof(args);

		return 1;
	//}
	//return 0;
}
// CONFIG-ONLY command!
static int brightnessMult(const void *context, const char *cmd, const char *args, int cmdFlags){
        ADDLOG_INFO(LOG_FEATURE_CMD, " brightnessMult (%s) received with args %s",cmd,args);

		g_cfg_brightnessMult = atof(args);

		return 1;
	//}
	//return 0;
}
static void onHSVChanged() {
	float r, g, b;

	HSVtoRGB(&r,&g,&b, g_hsv_h,g_hsv_s,g_hsv_v);

	baseColors[0] = r * 255.0f;
	baseColors[1] = g * 255.0f;
	baseColors[2] = b * 255.0f;

	sendColorChange();

	apply_smart_light();

	
	if(CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTLEDFINALCOLOR)) {
		sendFinalColor();
	}
}
static void led_setSaturation(float sat){

	g_hsv_s = sat;

	onHSVChanged();
}
static void led_setHue(float hue){
	
	g_hsv_h = hue;

	onHSVChanged();
}
static int setSaturation(const void *context, const char *cmd, const char *args, int cmdFlags){
    float f;

	f = atof(args);

	// input is in 0-100 range 
	f *= 0.01f;

	led_setSaturation(f);

	return 1;
}
static int setHue(const void *context, const char *cmd, const char *args, int cmdFlags){
    float f;

	f = atof(args);

	led_setHue(f);

	return 1;
}

int NewLED_InitCommands(){
    CMD_RegisterCommand("led_dimmer", "", dimmer, "set output dimmer 0..100", NULL);
    CMD_RegisterCommand("led_enableAll", "", enableAll, "qqqq", NULL);
    CMD_RegisterCommand("led_basecolor_rgb", "", basecolor_rgb, "set PWN color using #RRGGBB", NULL);
    CMD_RegisterCommand("led_basecolor_rgbcw", "", basecolor_rgbcw, "set PWN color using #RRGGBB[cw][ww]", NULL);
    CMD_RegisterCommand("led_temperature", "", temperature, "set qqqq", NULL);
    CMD_RegisterCommand("led_brightnessMult", "", brightnessMult, "set qqqq", NULL);
    CMD_RegisterCommand("led_colorMult", "", colorMult, "set qqqq", NULL);
    CMD_RegisterCommand("led_saturation", "", setSaturation, "set qqqq", NULL);
    CMD_RegisterCommand("led_hue", "", setHue, "set qqqq", NULL);

	// set, but do not apply
	LED_SetTemperature(250,0);

	// "cmnd/obk8D38570E/led_dimmer_get""
    return 0;
}
