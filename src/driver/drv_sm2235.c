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

#include "drv_sm2235.h"

// Some platforms have less pins than BK7231T.
// For example, BL602 doesn't have pin number 26.
// The pin code would crash BL602 while trying to access pin 26.
// This is why the default settings here a per-platform.
#if PLATFORM_BEKEN
static int g_pin_clk = 26;
static int g_pin_data = 24;
#else
static int g_pin_clk = 0;
static int g_pin_data = 1;
#endif

// Mapping between RGBCW to current SM2235 channels
static byte g_channelOrder[5] = { 2, 1, 0, 4, 3 };

static void SM2235_SetLow(uint8_t pin) {
	HAL_PIN_Setup_Output(pin);
	HAL_PIN_SetOutputValue(pin, 0);
}

static void SM2235_SetHigh(uint8_t pin) {
	HAL_PIN_Setup_Input_Pullup(pin);
}

static bool SM2235_PreInit(void) {
	HAL_PIN_SetOutputValue(g_pin_data, 0);
	HAL_PIN_SetOutputValue(g_pin_clk, 0);
	SM2235_SetHigh(g_pin_data);
	SM2235_SetHigh(g_pin_clk);
	return (!((HAL_PIN_ReadDigitalInput(g_pin_data) == 0 || HAL_PIN_ReadDigitalInput(g_pin_clk) == 0)));
}

static bool SM2235_WriteByte(uint8_t value) {
	uint8_t curr;
	uint8_t ack;

	for (curr = 0X80; curr != 0; curr >>= 1) {
		if (curr & value) {
			SM2235_SetHigh(g_pin_data);
		} else {
			SM2235_SetLow(g_pin_data);
		}
		SM2235_SetHigh(g_pin_clk);
		usleep(SM2235_DELAY);
		SM2235_SetLow(g_pin_clk);
	}
	// get Ack or Nak
	SM2235_SetHigh(g_pin_data);
	SM2235_SetHigh(g_pin_clk);
	usleep(SM2235_DELAY / 2);
	ack = HAL_PIN_ReadDigitalInput(g_pin_data);
	SM2235_SetLow(g_pin_clk);
	usleep(SM2235_DELAY / 2);
	SM2235_SetLow(g_pin_data);
	return (0 == ack);
}

static bool SM2235_Start(uint8_t addr) {
	SM2235_SetLow(g_pin_data);
	usleep(SM2235_DELAY);
	SM2235_SetLow(g_pin_clk);
	return SM2235_WriteByte(addr);
}

static void SM2235_Stop(void) {
	SM2235_SetLow(g_pin_data);
	usleep(SM2235_DELAY);
	SM2235_SetHigh(g_pin_clk);
	usleep(SM2235_DELAY);
	SM2235_SetHigh(g_pin_data);
	usleep(SM2235_DELAY);
}

void SM2235_Write(float *rgbcw) {
	unsigned short cur_col_10[5];
	int i;

	//ADDLOG_DEBUG(LOG_FEATURE_CMD, "Writing to Lamp: %f %f %f %f %f", rgbcw[0], rgbcw[1], rgbcw[2], rgbcw[3], rgbcw[4]);

	for (i = 0; i < 5; i++) {
		// convert 0-255 to 0-1023
		//cur_col_10[i] = rgbcw[g_channelOrder[i]] * 4;
		cur_col_10[i] = MAP(rgbcw[g_channelOrder[i]], 0, 255.0f, 0, 1023.0f);
	}

#define SM2235_FIRST_BYTE(x) ((x >> 8) & 0xFF)
#define SM2235_SECOND_BYTE(x) (x & 0xFF)

	// Byte 0
	SM2235_Start(SM2235_BYTE_0);
	// Byte 1
	SM2235_WriteByte(SM2235_BYTE_1);
	// Byte 2
	SM2235_WriteByte((uint8_t)(SM2235_FIRST_BYTE(cur_col_10[0])));  //Red
	// Byte 3
	SM2235_WriteByte((uint8_t)(SM2235_SECOND_BYTE(cur_col_10[0])));
	// Byte 4
	SM2235_WriteByte((uint8_t)(SM2235_FIRST_BYTE(cur_col_10[1]))); //Green
	// Byte 5
	SM2235_WriteByte((uint8_t)(SM2235_SECOND_BYTE(cur_col_10[1])));
	// Byte 6
	SM2235_WriteByte((uint8_t)(SM2235_FIRST_BYTE(cur_col_10[2]))); //Blue
	// Byte 7
	SM2235_WriteByte((uint8_t)(SM2235_SECOND_BYTE(cur_col_10[2])));
	// Byte 8
	SM2235_WriteByte((uint8_t)(SM2235_FIRST_BYTE(cur_col_10[4]))); //Cold
	// Byte 9
	SM2235_WriteByte((uint8_t)(SM2235_SECOND_BYTE(cur_col_10[4])));
	// Byte 10
	SM2235_WriteByte((uint8_t)(SM2235_FIRST_BYTE(cur_col_10[3]))); //Warm
	// Byte 11
	SM2235_WriteByte((uint8_t)(SM2235_SECOND_BYTE(cur_col_10[3])));
	SM2235_Stop();

}

static commandResult_t SM2235_RGBCW(const void *context, const char *cmd, const char *args, int flags){
	const char *c = args;
	float col[5] = { 0, 0, 0, 0, 0 };
	int ci;
	int val;

	ci = 0;

	// some people prefix colors with #
	if(c[0] == '#')
		c++;
	while (*c){
		char tmp[3];
		int r;
		tmp[0] = *(c++);
		if (!*c)
			break;
		tmp[1] = *(c++);
		tmp[2] = '\0';
		r = sscanf(tmp, "%x", &val);
		if (!r) {
			ADDLOG_ERROR(LOG_FEATURE_CMD, "SM2235_RGBCW no sscanf hex result from %s", tmp);
			break;
		}

		ADDLOG_DEBUG(LOG_FEATURE_CMD, "SM2235_RGBCW found chan %d -> val255 %d (from %s)", ci, val, tmp);

		col[ci] = val;

		// move to next channel.
		ci ++;
		if(ci>=5)
			break;
	}

	SM2235_Write(col);

	return CMD_RES_OK;
}
// SM2235_Map is used to map the RGBCW indices to SM2235 indices
// This is how you uset RGB CW order:
// SM2235_Map 0 1 2 3 4
// This is the order used on my polish Spectrum WOJ14415 bulb:
// SM2235_Map 2 1 0 4 3 

static commandResult_t SM2235_Map(const void *context, const char *cmd, const char *args, int flags){
	
	Tokenizer_TokenizeString(args,0);

	if(Tokenizer_GetArgsCount()==0) {
		ADDLOG_DEBUG(LOG_FEATURE_CMD, "SM2235_Map current order is %i %i %i    %i %i! ",
			(int)g_channelOrder[0],(int)g_channelOrder[1],(int)g_channelOrder[2],(int)g_channelOrder[3],(int)g_channelOrder[4]);
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	g_channelOrder[0] = Tokenizer_GetArgIntegerRange(0, 0, 4);
	g_channelOrder[1] = Tokenizer_GetArgIntegerRange(1, 0, 4);
	g_channelOrder[2] = Tokenizer_GetArgIntegerRange(2, 0, 4);
	g_channelOrder[3] = Tokenizer_GetArgIntegerRange(3, 0, 4);
	g_channelOrder[4] = Tokenizer_GetArgIntegerRange(4, 0, 4);

	ADDLOG_DEBUG(LOG_FEATURE_CMD, "SM2235_Map new order is %i %i %i    %i %i! ",
		(int)g_channelOrder[0],(int)g_channelOrder[1],(int)g_channelOrder[2],(int)g_channelOrder[3],(int)g_channelOrder[4]);

	return CMD_RES_OK;
}

static void SM2235_SetCurrent(int curValRGB, int curValCW) {
	//g_current_setting_rgb = curValRGB;
	//g_current_setting_cw = curValCW;
}

static commandResult_t SM2235_Current(const void *context, const char *cmd, const char *args, int flags){
	/*int valRGB;
	int valCW;
	Tokenizer_TokenizeString(args,0);

	if(Tokenizer_GetArgsCount()<=1) {
		ADDLOG_DEBUG(LOG_FEATURE_CMD, "SM2235_Current: requires 2 arguments [RGB,CW]. Current value is: %i %i!\n",g_current_setting_rgb,g_current_setting_cw);
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	valRGB = Tokenizer_GetArgInteger(0);
	valCW = Tokenizer_GetArgInteger(1);

	SM2235_SetCurrent(valRGB,valCW);*/
	return CMD_RES_OK;
}

// startDriver SM2235
// SM2235_RGBCW FF00000000
void SM2235_Init() {

    SM2235_PreInit();

	g_pin_clk = PIN_FindPinIndexForRole(IOR_SM2235_CLK,g_pin_clk);
	g_pin_data = PIN_FindPinIndexForRole(IOR_SM2235_DAT,g_pin_data);

	//cmddetail:{"name":"SM2235_RGBCW","args":"[HexColor]",
	//cmddetail:"descr":"Don't use it. It's for direct access of SM2235 driver. You don't need it because LED driver automatically calls it, so just use led_basecolor_rgb",
	//cmddetail:"fn":"SM2235_RGBCW","file":"driver/drv_sm2235.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SM2235_RGBCW", "", SM2235_RGBCW, NULL, NULL);
	//cmddetail:{"name":"SM2235_Map","args":"[Ch0][Ch1][Ch2][Ch3][Ch4]",
	//cmddetail:"descr":"Maps the RGBCW values to given indices of SM2235 channels. This is because SM2235 channels order is not the same for some devices. Some devices are using RGBCW order and some are using GBRCW, etc, etc. Example usage: SM2235_Map 0 1 2 3 4",
	//cmddetail:"fn":"SM2235_Map","file":"driver/drv_sm2235.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SM2235_Map", "", SM2235_Map, NULL, NULL);
	//cmddetail:{"name":"SM2235_Current","args":"[Value]",
	//cmddetail:"descr":"Sets the maximum current for LED driver.",
	//cmddetail:"fn":"SM2235_Current","file":"driver/drv_sm2235.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SM2235_Current", "", SM2235_Current, NULL, NULL);
}

void SM2235_RunFrame() {

}


void SM2235_OnChannelChanged(int ch, int value) {
#if 0
	byte col[5];
	int channel;
	int c;

	for(channel = 0; channel < CHANNEL_MAX; channel++){
		if(IOR_PWM == CHANNEL_GetRoleForOutputChannel(channel)){
			col[c] = CHANNEL_Get(channel);
			c++;
		}
	}
	for( ; c < 5; c++){
		col[c] = 0;
	}

	SM2235_Write(col);
#endif
}


