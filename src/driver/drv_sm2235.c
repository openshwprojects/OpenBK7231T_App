// NOTE: this also works for SM2335, so if you have
// device with SM2335, just use SM2235 driver and
// it will work just fine.
// SM2235 and SM2335 are 10 bit LED drivers, SM2135 is 8bit
// NOTE: SM2235 driver seems to work for SM2185N
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

static softI2C_t g_softI2C;
static int g_cur_RGB = 2;
static int g_cur_CW = 4;

void SM2235_Write(float *rgbcw) {
	unsigned short cur_col_10[5];
	int i;

	//ADDLOG_DEBUG(LOG_FEATURE_CMD, "Writing to Lamp: %f %f %f %f %f", rgbcw[0], rgbcw[1], rgbcw[2], rgbcw[3], rgbcw[4]);

	for (i = 0; i < 5; i++) {
		// convert 0-255 to 0-1023
		//cur_col_10[i] = rgbcw[g_cfg.ledRemap.ar[i]] * 4;
		cur_col_10[i] = MAP(GetRGBCW(rgbcw,g_cfg.ledRemap.ar[i]), 0, 255.0f, 0, 1023.0f);
	}

#define SM2235_FIRST_BYTE(x) ((x >> 8) & 0xFF)
#define SM2235_SECOND_BYTE(x) (x & 0xFF)

	// Byte 0
	Soft_I2C_Start(&g_softI2C,SM2235_BYTE_0);
	// Byte 1
	Soft_I2C_WriteByte(&g_softI2C, g_cur_RGB << 4 | g_cur_CW);
	// Byte 2
	Soft_I2C_WriteByte(&g_softI2C, (uint8_t)(SM2235_FIRST_BYTE(cur_col_10[0])));  //Red
	// Byte 3
	Soft_I2C_WriteByte(&g_softI2C, (uint8_t)(SM2235_SECOND_BYTE(cur_col_10[0])));
	// Byte 4
	Soft_I2C_WriteByte(&g_softI2C, (uint8_t)(SM2235_FIRST_BYTE(cur_col_10[1]))); //Green
	// Byte 5
	Soft_I2C_WriteByte(&g_softI2C, (uint8_t)(SM2235_SECOND_BYTE(cur_col_10[1])));
	// Byte 6
	Soft_I2C_WriteByte(&g_softI2C, (uint8_t)(SM2235_FIRST_BYTE(cur_col_10[2]))); //Blue
	// Byte 7
	Soft_I2C_WriteByte(&g_softI2C, (uint8_t)(SM2235_SECOND_BYTE(cur_col_10[2])));
	// Byte 8
	Soft_I2C_WriteByte(&g_softI2C, (uint8_t)(SM2235_FIRST_BYTE(cur_col_10[4]))); //Cold
	// Byte 9
	Soft_I2C_WriteByte(&g_softI2C, (uint8_t)(SM2235_SECOND_BYTE(cur_col_10[4])));
	// Byte 10
	Soft_I2C_WriteByte(&g_softI2C, (uint8_t)(SM2235_FIRST_BYTE(cur_col_10[3]))); //Warm
	// Byte 11
	Soft_I2C_WriteByte(&g_softI2C, (uint8_t)(SM2235_SECOND_BYTE(cur_col_10[3])));
	Soft_I2C_Stop(&g_softI2C);

#if WINDOWS
	Simulator_StoreBP5758DColor(cur_col_10);
#endif
}

static void SM2235_SetCurrent(int curValRGB, int curValCW) {
	g_cur_RGB = curValRGB;
	g_cur_CW = curValCW;
	LED_ResendCurrentColors();
}

static commandResult_t SM2235_Current(const void *context, const char *cmd, const char *args, int flags){
	int valRGB;
	int valCW;
	Tokenizer_TokenizeString(args,0);

	if(Tokenizer_GetArgsCount()<=1) {
		ADDLOG_DEBUG(LOG_FEATURE_CMD, 
			"SM2235_Current: requires 2 arguments [RGB,CW]. Current value is: %i %i!\n",
			g_cur_RGB, g_cur_CW);
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	valRGB = Tokenizer_GetArgInteger(0);
	valCW = Tokenizer_GetArgInteger(1);

	SM2235_SetCurrent(valRGB,valCW);
	return CMD_RES_OK;
}

// startDriver SM2235
// SM2235_RGBCW FF00000000
void SM2235_Init() {

	// default setting (applied only if none was applied earlier)
	CFG_SetDefaultLEDRemap(2, 1, 0, 4, 3);

	g_softI2C.pin_clk = PIN_FindPinIndexForRole(IOR_SM2235_CLK,g_softI2C.pin_clk);
	g_softI2C.pin_data = PIN_FindPinIndexForRole(IOR_SM2235_DAT,g_softI2C.pin_data);

	Soft_I2C_PreInit(&g_softI2C);


	//cmddetail:{"name":"SM2235_RGBCW","args":"[HexColor]",
	//cmddetail:"descr":"Don't use it. It's for direct access of SM2235 driver. You don't need it because LED driver automatically calls it, so just use led_basecolor_rgb",
	//cmddetail:"fn":"CMD_LEDDriver_WriteRGBCW","file":"driver/drv_sm2235.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SM2235_RGBCW", CMD_LEDDriver_WriteRGBCW, NULL);
	//cmddetail:{"name":"SM2235_Map","args":"[Ch0][Ch1][Ch2][Ch3][Ch4]",
	//cmddetail:"descr":"Maps the RGBCW values to given indices of SM2235 channels. This is because SM2235 channels order is not the same for some devices. Some devices are using RGBCW order and some are using GBRCW, etc, etc. Example usage: SM2235_Map 0 1 2 3 4",
	//cmddetail:"fn":"CMD_LEDDriver_Map","file":"driver/drv_sm2235.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SM2235_Map", CMD_LEDDriver_Map, NULL);
	//cmddetail:{"name":"SM2235_Current","args":"[Value]",
	//cmddetail:"descr":"Sets the maximum current for LED driver.",
	//cmddetail:"fn":"SM2235_Current","file":"driver/drv_sm2235.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SM2235_Current", SM2235_Current, NULL);

	// alias for LED_Map. In future we may want to migrate totally to shared LED_Map command.... 
	CMD_CreateAliasHelper("LED_Map", "SM2235_Map");
}

