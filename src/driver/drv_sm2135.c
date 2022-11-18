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

#include "drv_sm2135.h"

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

static int g_current_setting_cw = SM2135_20MA;
static int g_current_setting_rgb = SM2135_20MA;
// Mapping between RGBCW to current SM2135 channels
static byte g_channelOrder[5] = { 2, 1, 0, 4, 3 };

static void SM2135_SetLow(uint8_t pin) {
	HAL_PIN_Setup_Output(pin);
	HAL_PIN_SetOutputValue(pin, 0);
}

static void SM2135_SetHigh(uint8_t pin) {
	HAL_PIN_Setup_Input_Pullup(pin);
}

static bool SM2135_PreInit(void) {
	HAL_PIN_SetOutputValue(g_pin_data, 0);
	HAL_PIN_SetOutputValue(g_pin_clk, 0);
	SM2135_SetHigh(g_pin_data);
	SM2135_SetHigh(g_pin_clk);
	return (!((HAL_PIN_ReadDigitalInput(g_pin_data) == 0 || HAL_PIN_ReadDigitalInput(g_pin_clk) == 0)));
}

static bool SM2135_WriteByte(uint8_t value) {
	uint8_t curr;
	uint8_t ack;

	for (curr = 0X80; curr != 0; curr >>= 1) {
		if (curr & value) {
			SM2135_SetHigh(g_pin_data);
		} else {
			SM2135_SetLow(g_pin_data);
		}
		SM2135_SetHigh(g_pin_clk);
		usleep(SM2135_DELAY);
		SM2135_SetLow(g_pin_clk);
	}
	// get Ack or Nak
	SM2135_SetHigh(g_pin_data);
	SM2135_SetHigh(g_pin_clk);
	usleep(SM2135_DELAY / 2);
	ack = HAL_PIN_ReadDigitalInput(g_pin_data);
	SM2135_SetLow(g_pin_clk);
	usleep(SM2135_DELAY / 2);
	SM2135_SetLow(g_pin_data);
	return (0 == ack);
}

static bool SM2135_Start(uint8_t addr) {
	SM2135_SetLow(g_pin_data);
	usleep(SM2135_DELAY);
	SM2135_SetLow(g_pin_clk);
	return SM2135_WriteByte(addr);
}

static void SM2135_Stop(void) {
	SM2135_SetLow(g_pin_data);
	usleep(SM2135_DELAY);
	SM2135_SetHigh(g_pin_clk);
	usleep(SM2135_DELAY);
	SM2135_SetHigh(g_pin_data);
	usleep(SM2135_DELAY);
}

void SM2135_Write(byte *rgbcw) {
	int i;
	int bRGB;

	if(CFG_HasFlag(OBK_FLAG_SM2135_SEPARATE_MODES)) {
		bRGB = 0;
		for(i = 0; i < 3; i++){
			if(rgbcw[g_channelOrder[i]]!=0) {
				bRGB = 1;
				break;
			}
		}
		if(bRGB) {
			SM2135_Start(SM2135_ADDR_MC);
			SM2135_WriteByte(g_current_setting_rgb);
			SM2135_WriteByte(SM2135_RGB);
			SM2135_WriteByte(rgbcw[g_channelOrder[0]]);
			SM2135_WriteByte(rgbcw[g_channelOrder[1]]);
			SM2135_WriteByte(rgbcw[g_channelOrder[2]]); 
			SM2135_Stop();
		} else {
			SM2135_Start(SM2135_ADDR_MC);
			SM2135_WriteByte(g_current_setting_cw);
			SM2135_WriteByte(SM2135_CW);
			SM2135_Stop();
			usleep(SM2135_DELAY);

			SM2135_Start(SM2135_ADDR_C);
			SM2135_WriteByte(rgbcw[g_channelOrder[3]]);
			SM2135_WriteByte(rgbcw[g_channelOrder[4]]); 
			SM2135_Stop();

		}
	} else {
		SM2135_Start(SM2135_ADDR_MC);
		SM2135_WriteByte(g_current_setting_rgb);
		SM2135_WriteByte(SM2135_RGB);
		SM2135_WriteByte(rgbcw[g_channelOrder[0]]);
		SM2135_WriteByte(rgbcw[g_channelOrder[1]]);
		SM2135_WriteByte(rgbcw[g_channelOrder[2]]); 
		SM2135_WriteByte(rgbcw[g_channelOrder[3]]); 
		SM2135_WriteByte(rgbcw[g_channelOrder[4]]); 
		SM2135_Stop();
	}
}

static int SM2135_RGBCW(const void *context, const char *cmd, const char *args, int flags){
	const char *c = args;
	byte col[5] = { 0, 0, 0, 0, 0 };
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
			ADDLOG_ERROR(LOG_FEATURE_CMD, "SM2135_RGBCW no sscanf hex result from %s", tmp);
			break;
		}

		ADDLOG_DEBUG(LOG_FEATURE_CMD, "SM2135_RGBCW found chan %d -> val255 %d (from %s)", ci, val, tmp);

		col[ci] = val;

		// move to next channel.
		ci ++;
		if(ci>=5)
			break;
	}

	SM2135_Write(col);

	return 0;
}
// SM2135_Map is used to map the RGBCW indices to SM2135 indices
// This is how you uset RGB CW order:
// SM2135_Map 0 1 2 3 4
// This is the order used on my polish Spectrum WOJ14415 bulb:
// SM2135_Map 2 1 0 4 3 

static int SM2135_Map(const void *context, const char *cmd, const char *args, int flags){
	
	Tokenizer_TokenizeString(args,0);

	if(Tokenizer_GetArgsCount()==0) {
		ADDLOG_DEBUG(LOG_FEATURE_CMD, "SM2135_Map current order is %i %i %i    %i %i! ",
			(int)g_channelOrder[0],(int)g_channelOrder[1],(int)g_channelOrder[2],(int)g_channelOrder[3],(int)g_channelOrder[4]);
		return 0;
	}

	g_channelOrder[0] = Tokenizer_GetArgIntegerRange(0, 0, 4);
	g_channelOrder[1] = Tokenizer_GetArgIntegerRange(1, 0, 4);
	g_channelOrder[2] = Tokenizer_GetArgIntegerRange(2, 0, 4);
	g_channelOrder[3] = Tokenizer_GetArgIntegerRange(3, 0, 4);
	g_channelOrder[4] = Tokenizer_GetArgIntegerRange(4, 0, 4);

	ADDLOG_DEBUG(LOG_FEATURE_CMD, "SM2135_Map new order is %i %i %i    %i %i! ",
		(int)g_channelOrder[0],(int)g_channelOrder[1],(int)g_channelOrder[2],(int)g_channelOrder[3],(int)g_channelOrder[4]);

	return 0;
}

static void SM2135_SetCurrent(int curValRGB, int curValCW) {
	g_current_setting_rgb = curValRGB;
	g_current_setting_cw = curValCW;
}

static int SM2135_Current(const void *context, const char *cmd, const char *args, int flags){
	int valRGB;
	int valCW;
	Tokenizer_TokenizeString(args,0);

	if(Tokenizer_GetArgsCount()<=1) {
		ADDLOG_DEBUG(LOG_FEATURE_CMD, "SM2135_Current: requires 2 arguments [RGB,CW]. Current value is: %i %i!\n",g_current_setting_rgb,g_current_setting_cw);
		return 0;
	}
	valRGB = Tokenizer_GetArgInteger(0);
	valCW = Tokenizer_GetArgInteger(1);

	SM2135_SetCurrent(valRGB,valCW);
	return 1;
}

// startDriver SM2135
// SM2135_RGBCW FF00000000
void SM2135_Init() {

    SM2135_PreInit();

	g_pin_clk = PIN_FindPinIndexForRole(IOR_SM2135_CLK,g_pin_clk);
	g_pin_data = PIN_FindPinIndexForRole(IOR_SM2135_DAT,g_pin_data);

    CMD_RegisterCommand("SM2135_RGBCW", "", SM2135_RGBCW, "qq", NULL);
    CMD_RegisterCommand("SM2135_Map", "", SM2135_Map, "qq", NULL);
    CMD_RegisterCommand("SM2135_Current", "", SM2135_Current, "qq", NULL);
}

void SM2135_RunFrame() {

}


void SM2135_OnChannelChanged(int ch, int value) {
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

	SM2135_Write(col);
#endif
}


