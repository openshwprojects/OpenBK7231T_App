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

#include "drv_bp5758d.h"

static int g_pin_clk = 26;
static int g_pin_data = 24;
// Mapping between RGBCW to current BP5758D channels
static byte g_channelOrder[5] = { 0, 1, 2, 3, 4 };

const byte BP5758D_DELAY = 2;

static void BP5758D_Stop() {
	HAL_PIN_SetOutputValue(g_pin_clk, 1);
	rtos_delay_milliseconds(BP5758D_DELAY);
	HAL_PIN_SetOutputValue(g_pin_data, 1);
	rtos_delay_milliseconds(BP5758D_DELAY);
}


static void BP5758D_WriteByte(uint8_t value) {
	int bit_idx;
	bool bit;

	for (bit_idx = 7; bit_idx >= 0; bit_idx--) {
		bit = BIT_CHECK(value, bit_idx);
		HAL_PIN_SetOutputValue(g_pin_data, bit);
		rtos_delay_milliseconds(BP5758D_DELAY);
		HAL_PIN_SetOutputValue(g_pin_clk, 1);
		rtos_delay_milliseconds(BP5758D_DELAY);
		HAL_PIN_SetOutputValue(g_pin_clk, 0);
		rtos_delay_milliseconds(BP5758D_DELAY);
	}
	// Wait for ACK
	// TODO: pullup?
	HAL_PIN_Setup_Input(g_pin_data);
	HAL_PIN_SetOutputValue(g_pin_clk, 1);
	rtos_delay_milliseconds(BP5758D_DELAY);
	HAL_PIN_SetOutputValue(g_pin_clk, 0);
	rtos_delay_milliseconds(BP5758D_DELAY);
	HAL_PIN_Setup_Output(g_pin_data);
}

static void BP5758D_Start(uint8_t addr) {
	HAL_PIN_SetOutputValue(g_pin_data, 0);
	rtos_delay_milliseconds(BP5758D_DELAY);
	HAL_PIN_SetOutputValue(g_pin_clk, 0);
	rtos_delay_milliseconds(BP5758D_DELAY);
	BP5758D_Write(addr);
}

static void BP5758D_PreInit() {
	HAL_PIN_Setup_Output(g_pin_clk);
	HAL_PIN_Setup_Output(g_pin_data);
	BP5758D_Stop();

	rtos_delay_milliseconds(BP5758D_DELAY);
	
    // For it's init sequence, BP5758D just sets all fields
    BP5758D_Init();
    BP5758D_Start(BP5758D_ADDR_SETUP);
    // Output enabled: enable all outputs since we're using a RGBCW light
    BP5758D_Write(BP5758D_ENABLE_OUTPUTS_ALL);
    // Set currents for OUT1-OUT5
    BP5758D_Write(BP5758D_14MA);
    BP5758D_Write(BP5758D_14MA);
    BP5758D_Write(BP5758D_14MA);
    BP5758D_Write(BP5758D_14MA);
    BP5758D_Write(BP5758D_14MA);
    // Set grayscale levels ouf all outputs to 0
    BP5758D_Write(0x00);
    BP5758D_Write(0x00);
    BP5758D_Write(0x00);
    BP5758D_Write(0x00);
    BP5758D_Write(0x00);
    BP5758D_Write(0x00);
    BP5758D_Write(0x00);
    BP5758D_Write(0x00);
    BP5758D_Write(0x00);
    BP5758D_Write(0x00);
    BP5758D_Stop();
}



void BP5758D_Write(byte *rgbcw) {
	int i;
	unsigned short cur_col_10[5];

	for(i = 0; i < 5; i++){
		// convert 0-255 to 0-1023
		cur_col_10[i] = rgbcw[g_channelOrder[i]] * 4;
	}

	// If we receive 0 for all channels, we'll assume that the lightbulb is off, and activate BP5758d's sleep mode.
	if (cur_col_10[0]==0 && cur_col_10[1]==0 && cur_col_10[2]==0 && cur_col_10[3]==0 && cur_col_10[4]==0) {
		BP5758D_Start(BP5758D_ADDR_SLEEP);
		BP5758D_Stop();
		return;
	}

	// Even though we could address changing channels only, in practice we observed that the lightbulb always sets all channels.
	BP5758D_Start(BP5758D_ADDR_OUT1_GL);
	// Brigtness values are transmitted as two bytes. The light-bulb accepts a 10-bit integer (0-1023) as an input value.
	// The first 5bits of this input are transmitted in second byte, the second 5bits in the first byte.  
	BP5758D_WriteByte((uint8_t)(cur_col_10[0] & 0x1F));  //Red
	BP5758D_WriteByte((uint8_t)(cur_col_10[0] >> 5));
	BP5758D_WriteByte((uint8_t)(cur_col_10[1] & 0x1F)); //Green
	BP5758D_WriteByte((uint8_t)(cur_col_10[1] >> 5));
	BP5758D_WriteByte((uint8_t)(cur_col_10[2] & 0x1F)); //Blue
	BP5758D_WriteByte((uint8_t)(cur_col_10[2] >> 5));
	BP5758D_WriteByte((uint8_t)(cur_col_10[4] & 0x1F)); //Cold
	BP5758D_WriteByte((uint8_t)(cur_col_10[4] >> 5));
	BP5758D_WriteByte((uint8_t)(cur_col_10[3] & 0x1F)); //Warm
	BP5758D_WriteByte((uint8_t)(cur_col_10[3] >> 5));

	BP5758D_Stop();
	return true;
}


static int BP5758D_RGBCW(const void *context, const char *cmd, const char *args, int flags){
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
			ADDLOG_ERROR(LOG_FEATURE_CMD, "BP5758D_RGBCW no sscanf hex result from %s", tmp);
			break;
		}

		ADDLOG_DEBUG(LOG_FEATURE_CMD, "BP5758D_RGBCW found chan %d -> val255 %d (from %s)", ci, val, tmp);

		col[ci] = val;

		// move to next channel.
		ci ++;
		if(ci>=5)
			break;
	}

	BP5758D_Write(col);

	return 0;
}
// BP5758D_Map is used to map the RGBCW indices to BP5758D indices
// This is how you uset RGB CW order:
// BP5758D_Map 0 1 2 3 4
// This is the order used on my polish Spectrum WOJ14415 bulb:
// BP5758D_Map 2 1 0 4 3 

static int BP5758D_Map(const void *context, const char *cmd, const char *args, int flags){
	
	Tokenizer_TokenizeString(args);

	if(Tokenizer_GetArgsCount()==0) {
		ADDLOG_DEBUG(LOG_FEATURE_CMD, "BP5758D_Map current order is %i %i %i    %i %i! ",
			(int)g_channelOrder[0],(int)g_channelOrder[1],(int)g_channelOrder[2],(int)g_channelOrder[3],(int)g_channelOrder[4]);
		return 0;
	}

	g_channelOrder[0] = Tokenizer_GetArgIntegerRange(0, 0, 4);
	g_channelOrder[1] = Tokenizer_GetArgIntegerRange(1, 0, 4);
	g_channelOrder[2] = Tokenizer_GetArgIntegerRange(2, 0, 4);
	g_channelOrder[3] = Tokenizer_GetArgIntegerRange(3, 0, 4);
	g_channelOrder[4] = Tokenizer_GetArgIntegerRange(4, 0, 4);

	ADDLOG_DEBUG(LOG_FEATURE_CMD, "BP5758D_Map new order is %i %i %i    %i %i! ",
		(int)g_channelOrder[0],(int)g_channelOrder[1],(int)g_channelOrder[2],(int)g_channelOrder[3],(int)g_channelOrder[4]);

	return 0;
}


// startDriver BP5758D
// BP5758D_RGBCW FF00000000
void BP5758D_Init() {

    BP5758D_PreInit();

	g_pin_clk = PIN_FindPinIndexForRole(IOR_BP5758D_CLK,g_pin_clk);
	g_pin_data = PIN_FindPinIndexForRole(IOR_BP5758D_DAT,g_pin_data);

    CMD_RegisterCommand("BP5758D_RGBCW", "", BP5758D_RGBCW, "qq", NULL);
    CMD_RegisterCommand("BP5758D_Map", "", BP5758D_Map, "qq", NULL);
}

void BP5758D_RunFrame() {

}


void BP5758D_OnChannelChanged(int ch, int value) {
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

	BP5758D_Write(col);
#endif
}




