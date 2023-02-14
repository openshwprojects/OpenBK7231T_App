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
	Soft_I2C_Stop();

	usleep(SM2135_DELAY);
	
	// here is a conversion from human-readable format to BP's format
	g_chosenCurrent = (curVal>63) ? (curVal+34) : curVal;
	// That assumed that user knows the strange BP notation
	//g_chosenCurrent = curVal;

    // For it's init sequence, BP5758D just sets all fields
    Soft_I2C_Start(BP5758D_ADDR_SETUP);
    // Output enabled: enable all outputs since we're using a RGBCW light
    Soft_I2C_WriteByte(BP5758D_ENABLE_OUTPUTS_ALL);
    // Set currents for OUT1-OUT5
    Soft_I2C_WriteByte(g_chosenCurrent);
    Soft_I2C_WriteByte(g_chosenCurrent);
    Soft_I2C_WriteByte(g_chosenCurrent);
    Soft_I2C_WriteByte(g_chosenCurrent);
    Soft_I2C_WriteByte(g_chosenCurrent);
    Soft_I2C_Stop();
	usleep(SM2135_DELAY);
}
static void BP5758D_PreInit() {
	HAL_PIN_Setup_Output(g_i2c_pin_clk);
	HAL_PIN_Setup_Output(g_i2c_pin_data);

	Soft_I2C_Stop();

	usleep(SM2135_DELAY);

    // For it's init sequence, BP5758D just sets all fields
    Soft_I2C_Start(BP5758D_ADDR_SETUP);
    // Output enabled: enable all outputs since we're using a RGBCW light
    Soft_I2C_WriteByte(BP5758D_ENABLE_OUTPUTS_ALL);
    // Set currents for OUT1-OUT5
    Soft_I2C_WriteByte(g_chosenCurrent); //TODO: Make this configurable from webapp / console
    Soft_I2C_WriteByte(g_chosenCurrent);
    Soft_I2C_WriteByte(g_chosenCurrent);
    Soft_I2C_WriteByte(g_chosenCurrent);
    Soft_I2C_WriteByte(g_chosenCurrent);
    // Set grayscale levels ouf all outputs to 0
    Soft_I2C_WriteByte(0x00);
    Soft_I2C_WriteByte(0x00);
    Soft_I2C_WriteByte(0x00);
    Soft_I2C_WriteByte(0x00);
    Soft_I2C_WriteByte(0x00);
    Soft_I2C_WriteByte(0x00);
    Soft_I2C_WriteByte(0x00);
    Soft_I2C_WriteByte(0x00);
    Soft_I2C_WriteByte(0x00);
    Soft_I2C_WriteByte(0x00);
    Soft_I2C_Stop();
}



void BP5758D_Write(float *rgbcw) {
	int i;
	unsigned short cur_col_10[5];

	for(i = 0; i < 5; i++){
		// convert 0-255 to 0-1023
		//cur_col_10[i] = rgbcw[g_cfg.ledRemap.ar[i]] * 4;
		cur_col_10[i] = MAP(rgbcw[g_cfg.ledRemap.ar[i]], 0, 255.0f, 0, 1023.0f);

	}

	// If we receive 0 for all channels, we'll assume that the lightbulb is off, and activate BP5758d's sleep mode.
	if (cur_col_10[0]==0 && cur_col_10[1]==0 && cur_col_10[2]==0 && cur_col_10[3]==0 && cur_col_10[4]==0) {
		bIsSleeping = true;
		Soft_I2C_Start(BP5758D_ADDR_SETUP); 		//Select B1: Output enable setup
		Soft_I2C_WriteByte(BP5758D_DISABLE_OUTPUTS_ALL); //Set all outputs to OFF
		Soft_I2C_Stop(); 				//Stop transmission since we have to set Sleep mode (can probably be removed)
		Soft_I2C_Start(BP5758D_ADDR_SLEEP); 		//Enable sleep mode
		Soft_I2C_Stop();
		return;
	}

	if(bIsSleeping) {
		bIsSleeping = false;				//No need to run it every time a val gets changed
		Soft_I2C_Start(BP5758D_ADDR_SETUP);		//Sleep mode gets disabled too since bits 5:6 get set to 01
		Soft_I2C_WriteByte(BP5758D_ENABLE_OUTPUTS_ALL);	//Set all outputs to ON
		Soft_I2C_Stop();
	}

	// Even though we could address changing channels only, in practice we observed that the lightbulb always sets all channels.
	Soft_I2C_Start(BP5758D_ADDR_OUT1_GL);
	// Brigtness values are transmitted as two bytes. The light-bulb accepts a 10-bit integer (0-1023) as an input value.
	// The first 5bits of this input are transmitted in second byte, the second 5bits in the first byte.
	Soft_I2C_WriteByte((uint8_t)(cur_col_10[0] & 0x1F));  //Red
	Soft_I2C_WriteByte((uint8_t)(cur_col_10[0] >> 5));
	Soft_I2C_WriteByte((uint8_t)(cur_col_10[1] & 0x1F)); //Green
	Soft_I2C_WriteByte((uint8_t)(cur_col_10[1] >> 5));
	Soft_I2C_WriteByte((uint8_t)(cur_col_10[2] & 0x1F)); //Blue
	Soft_I2C_WriteByte((uint8_t)(cur_col_10[2] >> 5));
	Soft_I2C_WriteByte((uint8_t)(cur_col_10[4] & 0x1F)); //Cold
	Soft_I2C_WriteByte((uint8_t)(cur_col_10[4] >> 5));
	Soft_I2C_WriteByte((uint8_t)(cur_col_10[3] & 0x1F)); //Warm
	Soft_I2C_WriteByte((uint8_t)(cur_col_10[3] >> 5));

	Soft_I2C_Stop();
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

// startDriver BP5758D
// BP5758D_RGBCW FF00000000
//
// to init a current value at startup - short startup command
// backlog startDriver BP5758D; BP5758D_Current 14; 
void BP5758D_Init() {
	int i;
	// default setting (applied only if none was applied earlier)
	CFG_SetDefaultLEDRemap(0, 1, 2, 3, 4);

	g_i2c_pin_clk = PIN_FindPinIndexForRole(IOR_BP5758D_CLK,g_i2c_pin_clk);
	g_i2c_pin_data = PIN_FindPinIndexForRole(IOR_BP5758D_DAT,g_i2c_pin_data);

    BP5758D_PreInit();

	//cmddetail:{"name":"BP5758D_RGBCW","args":"[HexColor]",
	//cmddetail:"descr":"Don't use it. It's for direct access of BP5758D driver. You don't need it because LED driver automatically calls it, so just use led_basecolor_rgb",
	//cmddetail:"fn":"BP5758D_RGBCW","file":"driver/drv_bp5758d.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("BP5758D_RGBCW", CMD_LEDDriver_WriteRGBCW, NULL);
	//cmddetail:{"name":"BP5758D_Map","args":"[Ch0][Ch1][Ch2][Ch3][Ch4]",
	//cmddetail:"descr":"Maps the RGBCW values to given indices of BP5758D channels. This is because BP5758D channels order is not the same for some devices. Some devices are using RGBCW order and some are using GBRCW, etc, etc. Example usage: BP5758D_Map 0 1 2 3 4",
	//cmddetail:"fn":"BP5758D_Map","file":"driver/drv_bp5758d.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("BP5758D_Map", CMD_LEDDriver_Map, NULL);
	//cmddetail:{"name":"BP5758D_Current","args":"[MaxCurrent]",
	//cmddetail:"descr":"Sets the maximum current limit for BP5758D driver",
	//cmddetail:"fn":"BP5758D_Current","file":"driver/drv_bp5758d.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("BP5758D_Current", BP5758D_Current, NULL);
}
