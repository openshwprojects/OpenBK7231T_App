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

static byte g_chosenCurrent = BP5758D_14MA;

// allow user to select current by index? maybe, not yet
//static byte g_currentTable[] = { BP5758D_2MA, BP5758D_5MA, BP5758D_8MA, BP5758D_10MA, BP5758D_14MA, BP5758D_15MA, BP5758D_65MA, BP5758D_90MA };

bool bIsSleeping = false; //Save sleep state of Lamp

static void BP5758D_SetCurrent(byte curVal) {
	SM2135_Stop();

	usleep(SM2135_DELAY);
	
	// here is a conversion from human-readable format to BP's format
	g_chosenCurrent = (curVal>63) ? (curVal+34) : curVal;
	// That assumed that user knows the strange BP notation
	//g_chosenCurrent = curVal;

    // For it's init sequence, BP5758D just sets all fields
    SM2135_Start(BP5758D_ADDR_SETUP);
    // Output enabled: enable all outputs since we're using a RGBCW light
    SM2135_WriteByte(BP5758D_ENABLE_OUTPUTS_ALL);
    // Set currents for OUT1-OUT5
    SM2135_WriteByte(g_chosenCurrent);
    SM2135_WriteByte(g_chosenCurrent);
    SM2135_WriteByte(g_chosenCurrent);
    SM2135_WriteByte(g_chosenCurrent);
    SM2135_WriteByte(g_chosenCurrent);
    SM2135_Stop();
	usleep(SM2135_DELAY);
}
static void BP5758D_PreInit() {
	HAL_PIN_Setup_Output(g_i2c_pin_clk);
	HAL_PIN_Setup_Output(g_i2c_pin_data);

	SM2135_Stop();

	usleep(SM2135_DELAY);

    // For it's init sequence, BP5758D just sets all fields
    SM2135_Start(BP5758D_ADDR_SETUP);
    // Output enabled: enable all outputs since we're using a RGBCW light
    SM2135_WriteByte(BP5758D_ENABLE_OUTPUTS_ALL);
    // Set currents for OUT1-OUT5
    SM2135_WriteByte(g_chosenCurrent); //TODO: Make this configurable from webapp / console
    SM2135_WriteByte(g_chosenCurrent);
    SM2135_WriteByte(g_chosenCurrent);
    SM2135_WriteByte(g_chosenCurrent);
    SM2135_WriteByte(g_chosenCurrent);
    // Set grayscale levels ouf all outputs to 0
    SM2135_WriteByte(0x00);
    SM2135_WriteByte(0x00);
    SM2135_WriteByte(0x00);
    SM2135_WriteByte(0x00);
    SM2135_WriteByte(0x00);
    SM2135_WriteByte(0x00);
    SM2135_WriteByte(0x00);
    SM2135_WriteByte(0x00);
    SM2135_WriteByte(0x00);
    SM2135_WriteByte(0x00);
    SM2135_Stop();
}



void BP5758D_Write(float *rgbcw) {
	int i;
	unsigned short cur_col_10[5];

	for(i = 0; i < 5; i++){
		// convert 0-255 to 0-1023
		//cur_col_10[i] = rgbcw[g_channelOrder[i]] * 4;
		cur_col_10[i] = MAP(rgbcw[g_channelOrder[i]], 0, 255.0f, 0, 1023.0f);

	}

	// If we receive 0 for all channels, we'll assume that the lightbulb is off, and activate BP5758d's sleep mode.
	if (cur_col_10[0]==0 && cur_col_10[1]==0 && cur_col_10[2]==0 && cur_col_10[3]==0 && cur_col_10[4]==0) {
		bIsSleeping = true;
		SM2135_Start(BP5758D_ADDR_SETUP); 		//Select B1: Output enable setup
		SM2135_WriteByte(BP5758D_DISABLE_OUTPUTS_ALL); //Set all outputs to OFF
		SM2135_Stop(); 				//Stop transmission since we have to set Sleep mode (can probably be removed)
		SM2135_Start(BP5758D_ADDR_SLEEP); 		//Enable sleep mode
		SM2135_Stop();
		return;
	}

	if(bIsSleeping) {
		bIsSleeping = false;				//No need to run it every time a val gets changed
		SM2135_Start(BP5758D_ADDR_SETUP);		//Sleep mode gets disabled too since bits 5:6 get set to 01
		SM2135_WriteByte(BP5758D_ENABLE_OUTPUTS_ALL);	//Set all outputs to ON
		SM2135_Stop();
	}

	// Even though we could address changing channels only, in practice we observed that the lightbulb always sets all channels.
	SM2135_Start(BP5758D_ADDR_OUT1_GL);
	// Brigtness values are transmitted as two bytes. The light-bulb accepts a 10-bit integer (0-1023) as an input value.
	// The first 5bits of this input are transmitted in second byte, the second 5bits in the first byte.
	SM2135_WriteByte((uint8_t)(cur_col_10[0] & 0x1F));  //Red
	SM2135_WriteByte((uint8_t)(cur_col_10[0] >> 5));
	SM2135_WriteByte((uint8_t)(cur_col_10[1] & 0x1F)); //Green
	SM2135_WriteByte((uint8_t)(cur_col_10[1] >> 5));
	SM2135_WriteByte((uint8_t)(cur_col_10[2] & 0x1F)); //Blue
	SM2135_WriteByte((uint8_t)(cur_col_10[2] >> 5));
	SM2135_WriteByte((uint8_t)(cur_col_10[4] & 0x1F)); //Cold
	SM2135_WriteByte((uint8_t)(cur_col_10[4] >> 5));
	SM2135_WriteByte((uint8_t)(cur_col_10[3] & 0x1F)); //Warm
	SM2135_WriteByte((uint8_t)(cur_col_10[3] >> 5));

	SM2135_Stop();
}

// see drv_bp5758d.h for sample values
// Also see here for Datasheet table:
// https://user-images.githubusercontent.com/19175445/193464004-d5e8072b-d7a8-4950-8f06-118c01796616.png
// https://imgur.com/a/VKM6jOb
static commandResult_t BP5758D_Current(const void *context, const char *cmd, const char *args, int flags){
	byte val;
	Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	val = Tokenizer_GetArgInteger(0);
	// reinit bulb
	BP5758D_SetCurrent(val);
	return CMD_RES_OK;
}

static commandResult_t BP5758D_RGBCW(const void *context, const char *cmd, const char *args, int flags){
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

	return CMD_RES_OK;
}
// BP5758D_Map is used to map the RGBCW indices to BP5758D indices
// This is how you uset RGB CW order:
// BP5758D_Map 0 1 2 3 4
// This is the order used on my polish Spectrum WOJ14415 bulb:
// BP5758D_Map 2 1 0 4 3

static commandResult_t BP5758D_Map(const void *context, const char *cmd, const char *args, int flags){

	Tokenizer_TokenizeString(args,0);

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

	return CMD_RES_OK;
}


// startDriver BP5758D
// BP5758D_RGBCW FF00000000
//
// to init a current value at startup - short startup command
// backlog startDriver BP5758D; BP5758D_Current 14; 
void BP5758D_Init() {
	int i;
	// default map
	for (i = 0; i < 5; i++) {
		g_channelOrder[i] = i;
	}

	g_i2c_pin_clk = PIN_FindPinIndexForRole(IOR_BP5758D_CLK,g_i2c_pin_clk);
	g_i2c_pin_data = PIN_FindPinIndexForRole(IOR_BP5758D_DAT,g_i2c_pin_data);

    BP5758D_PreInit();

	//cmddetail:{"name":"BP5758D_RGBCW","args":"[HexColor]",
	//cmddetail:"descr":"Don't use it. It's for direct access of BP5758D driver. You don't need it because LED driver automatically calls it, so just use led_basecolor_rgb",
	//cmddetail:"fn":"BP5758D_RGBCW","file":"driver/drv_bp5758d.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("BP5758D_RGBCW", "", BP5758D_RGBCW, NULL, NULL);
	//cmddetail:{"name":"BP5758D_Map","args":"[Ch0][Ch1][Ch2][Ch3][Ch4]",
	//cmddetail:"descr":"Maps the RGBCW values to given indices of BP5758D channels. This is because BP5758D channels order is not the same for some devices. Some devices are using RGBCW order and some are using GBRCW, etc, etc. Example usage: BP5758D_Map 0 1 2 3 4",
	//cmddetail:"fn":"BP5758D_Map","file":"driver/drv_bp5758d.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("BP5758D_Map", "", BP5758D_Map, NULL, NULL);
	//cmddetail:{"name":"BP5758D_Current","args":"[MaxCurrent]",
	//cmddetail:"descr":"Sets the maximum current limit for BP5758D driver",
	//cmddetail:"fn":"BP5758D_Current","file":"driver/drv_bp5758d.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("BP5758D_Current", "", BP5758D_Current, NULL, NULL);
}
