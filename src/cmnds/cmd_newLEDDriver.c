
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../obk_config.h"
#include <ctype.h>
#include "cmd_local.h"
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

int parsePowerArgument(const char *s);

// In general, LED can be in two modes:
// - Temperature (Cool and Warm LEDs are on) 
// - RGB (R G B LEDs are on)
// This is just like in Tuya.
// The third mode, "All", was added by me for testing.
enum LightMode {
	Light_Temperature,
	Light_RGB,
	Light_All,
};

int g_lightMode = Light_RGB;
// Those are base colors, normalized, without brightness applied
float baseColors[5] = { 255, 255, 255, 255, 255 };
// this is not really needed, as we could easily change channel indices to anything we want...
int baseColorChannels[5] = { 1, 2, 3, 4, 5 };
int g_numBaseColors = 5;
float g_brightness = 1.0f;

// NOTE: in this system, enabling/disabling whole led light bulb
// is not changing the stored channel and brightness values.
// They are kept intact so you can reenable the bulb and keep your color setting
int g_lightEnableAll = 1;
// config only stuff
float g_cfg_brightnessMult = 0.01f;

void apply_smart_light() {
	int i;

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

	////	ADDLOG_INFO(LOG_FEATURE_CMD, "apply_smart_light: ch %i raw is %f, scaled by brightness is %f, enableAll is %i",i,raw,final,g_lightEnableAll);

		CHANNEL_Set(baseColorChannels[i], final, false);
	}
}
static int temperature(const void *context, const char *cmd, const char *args){
	int tmp;
	float f;
	//if (!wal_strnicmp(cmd, "POWERALL", 8)){

        ADDLOG_INFO(LOG_FEATURE_CMD, " temperature (%s) received with args %s",cmd,args);

		tmp = atoi(args);

		g_lightMode = Light_Temperature;
// the slider control in the UI emits values 
//in the range from 154-500 (defined 
//in homeassistant/util/color.py as HASS_COLOR_MIN and HASS_COLOR_MAX).

		f = (tmp - 154);
		f = f / (500.0f - 154.0f);
		if(f<0)
			f = 0;
		if(f>1)
			f =1;

     ///   ADDLOG_INFO(LOG_FEATURE_CMD, "tasCmnd temperature frac %f",f);

		baseColors[3] = (255.0f) * (1-f);
		baseColors[4] = (255.0f) * f;

		apply_smart_light();

		return 1;
	//}
	//return 0;
}
static int enableAll(const void *context, const char *cmd, const char *args){
	//if (!wal_strnicmp(cmd, "POWERALL", 8)){

        ADDLOG_INFO(LOG_FEATURE_CMD, " enableAll (%s) received with args %s",cmd,args);

		g_lightEnableAll = parsePowerArgument(args);

		apply_smart_light();

		return 1;
	//}
	//return 0;
}

static int dimmer(const void *context, const char *cmd, const char *args){
	//if (!wal_strnicmp(cmd, "POWERALL", 8)){
		int iVal = 0;

        ADDLOG_INFO(LOG_FEATURE_CMD, " dimmer (%s) received with args %s",cmd,args);

		iVal = parsePowerArgument(args);

		g_brightness = iVal * g_cfg_brightnessMult;

		apply_smart_light();

		return 1;
	//}
	//return 0;
}
static int basecolor(const void *context, const char *cmd, const char *args, int bAll){
   // if (!wal_strnicmp(cmd, "COLOR", 5)){
        if (args[0] != '#'){
            ADDLOG_ERROR(LOG_FEATURE_CMD, " BASECOLOR expected a # prefixed color, you sent %s",args);
            return 0;
        } else {
            const char *c = args;
            int val = 0;
            int channel = 0;
            ADDLOG_DEBUG(LOG_FEATURE_CMD, " BASECOLOR got %s", args);
            c++;

			if(bAll) {
				g_lightMode = Light_All;
			} else {
				g_lightMode = Light_RGB;
			}

			g_numBaseColors = 0;
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
                // if this channel is not PWM, find a PWM channel;
                while ((channel < 32) && (IOR_PWM != CHANNEL_GetRoleForOutputChannel(channel))) {
                    channel ++;
                }

                if (channel >= 32) {
                    ADDLOG_ERROR(LOG_FEATURE_CMD, "BASECOLOR channel >= 32");
                    break;
                }

                ADDLOG_DEBUG(LOG_FEATURE_CMD, "BASECOLOR found chan %d -> val255 %d(from %s)", channel, val, tmp);

				baseColors[g_numBaseColors] = val;
				baseColorChannels[g_numBaseColors] = channel;
				g_numBaseColors++;

                // move to next channel.
                channel ++;
            }
		apply_smart_light();
            if (!(*c)){
                ADDLOG_DEBUG(LOG_FEATURE_CMD, "BASECOLOR arg ended");
            }
        }
        return 1;
  //  }
   // return 0;
}

static int basecolor_rgb(const void *context, const char *cmd, const char *args){
	return basecolor(context,cmd,args,0);
}
static int basecolor_rgbcw(const void *context, const char *cmd, const char *args){
	return basecolor(context,cmd,args,1);
}

// CONFIG-ONLY command!
static int brightnessMult(const void *context, const char *cmd, const char *args){
        ADDLOG_INFO(LOG_FEATURE_CMD, " brightnessMult (%s) received with args %s",cmd,args);

		g_cfg_brightnessMult = atof(args);

		return 1;
	//}
	//return 0;
}


int NewLED_InitCommands(){
    CMD_RegisterCommand("led_dimmer", "", dimmer, "set output dimmer 0..100", NULL);
    CMD_RegisterCommand("led_enableAll", "", enableAll, "qqqq", NULL);
    CMD_RegisterCommand("led_basecolor_rgb", "", basecolor_rgb, "set PWN color using #RRGGBB", NULL);
    CMD_RegisterCommand("led_basecolor_rgbcw", "", basecolor_rgbcw, "set PWN color using #RRGGBB[cw][ww]", NULL);
    CMD_RegisterCommand("led_temperature", "", temperature, "set qqqq", NULL);
    CMD_RegisterCommand("led_brightnessMult", "", brightnessMult, "set qqqq", NULL);

	// "cmnd/obk8D38570E/led_dimmer_get""
    return 0;
}
