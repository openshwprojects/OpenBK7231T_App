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

static softI2C_t g_softI2C;
static int g_current_setting_cw = SM2135_20MA;
static int g_current_setting_rgb = SM2135_20MA;

float GetRGBCW(float *ar, int index) {
	if (index < 0 || index >= 5) {
		return 0;
	}
	return ar[index];
}
void SM2135_Write(float *rgbcw) {
	int i;
	int bRGB;
	int combinedCurrent = (g_current_setting_cw << 4) | g_current_setting_rgb;

	if(CFG_HasFlag(OBK_FLAG_SM2135_SEPARATE_MODES)) {
		bRGB = 0;
		for(i = 0; i < 3; i++){
			if(GetRGBCW(rgbcw,g_cfg.ledRemap.ar[i])!=0) {
				bRGB = 1;
				break;
			}
		}
		if(bRGB) {
			Soft_I2C_Start(&g_softI2C, SM2135_ADDR_MC);
			Soft_I2C_WriteByte(&g_softI2C, combinedCurrent);
			Soft_I2C_WriteByte(&g_softI2C, SM2135_RGB);
			Soft_I2C_WriteByte(&g_softI2C, GetRGBCW(rgbcw,g_cfg.ledRemap.r));
			Soft_I2C_WriteByte(&g_softI2C, GetRGBCW(rgbcw,g_cfg.ledRemap.g));
			Soft_I2C_WriteByte(&g_softI2C, GetRGBCW(rgbcw,g_cfg.ledRemap.b));
			Soft_I2C_Stop(&g_softI2C);
		} else {
			Soft_I2C_Start(&g_softI2C, SM2135_ADDR_MC);
			Soft_I2C_WriteByte(&g_softI2C, combinedCurrent);
			Soft_I2C_WriteByte(&g_softI2C, SM2135_CW);
			Soft_I2C_Stop(&g_softI2C);
			usleep(SM2135_DELAY);

			Soft_I2C_Start(&g_softI2C, SM2135_ADDR_C);
			Soft_I2C_WriteByte(&g_softI2C, GetRGBCW(rgbcw,g_cfg.ledRemap.c));
			Soft_I2C_WriteByte(&g_softI2C, GetRGBCW(rgbcw,g_cfg.ledRemap.w));
			Soft_I2C_Stop(&g_softI2C);

		}
	} else {
		Soft_I2C_Start(&g_softI2C, SM2135_ADDR_MC);
		Soft_I2C_WriteByte(&g_softI2C, combinedCurrent);
		Soft_I2C_WriteByte(&g_softI2C, SM2135_RGB);
		Soft_I2C_WriteByte(&g_softI2C, GetRGBCW(rgbcw,g_cfg.ledRemap.r));
		Soft_I2C_WriteByte(&g_softI2C, GetRGBCW(rgbcw,g_cfg.ledRemap.g));
		Soft_I2C_WriteByte(&g_softI2C, GetRGBCW(rgbcw,g_cfg.ledRemap.b));
		Soft_I2C_WriteByte(&g_softI2C, GetRGBCW(rgbcw,g_cfg.ledRemap.c));
		Soft_I2C_WriteByte(&g_softI2C, GetRGBCW(rgbcw,g_cfg.ledRemap.w));
		Soft_I2C_Stop(&g_softI2C);
	}
}

commandResult_t CMD_LEDDriver_WriteRGBCW(const void *context, const char *cmd, const char *args, int flags){
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
			ADDLOG_ERROR(LOG_FEATURE_CMD, "No sscanf hex result from %s", tmp);
			break;
		}

		ADDLOG_DEBUG(LOG_FEATURE_CMD, "Found chan %d -> val255 %d (from %s)", ci, val, tmp);

		col[ci] = val;

		// move to next channel.
		ci ++;
		if(ci>=5)
			break;
	}

#if ENABLE_LED_BASIC
	LED_I2CDriver_WriteRGBCW(col);
#endif

	return CMD_RES_OK;
}
// SM2135_Map is used to map the RGBCW indices to SM2135 indices
// This is how you uset RGB CW order:
// SM2135_Map 0 1 2 3 4
// This is the order used on my polish Spectrum WOJ14415 bulb:
// SM2135_Map 2 1 0 4 3 

commandResult_t CMD_LEDDriver_Map(const void *context, const char *cmd, const char *args, int flags){
	int r, g, b, c, w;
	Tokenizer_TokenizeString(args,0);

	if(Tokenizer_GetArgsCount()==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Current map is %i %i %i %i %i",
			(int)g_cfg.ledRemap.r,(int)g_cfg.ledRemap.g,(int)g_cfg.ledRemap.b,(int)g_cfg.ledRemap.c,(int)g_cfg.ledRemap.w);
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	r = Tokenizer_GetArgIntegerRange(0, -1, 4);
	g = Tokenizer_GetArgIntegerRange(1, -1, 4);
	b = Tokenizer_GetArgIntegerRange(2, -1, 4);
	c = Tokenizer_GetArgIntegerRange(3, -1, 4);
	w = Tokenizer_GetArgIntegerRange(4, -1, 4);

	CFG_SetLEDRemap(r, g, b, c, w);

	ADDLOG_INFO(LOG_FEATURE_CMD, "New map is %i %i %i %i %i",
		(int)g_cfg.ledRemap.r,(int)g_cfg.ledRemap.g,(int)g_cfg.ledRemap.b,(int)g_cfg.ledRemap.c,(int)g_cfg.ledRemap.w);

	return CMD_RES_OK;
}

static void SM2135_SetCurrent(int curValRGB, int curValCW) {
	g_current_setting_rgb = curValRGB;
	g_current_setting_cw = curValCW;
	LED_ResendCurrentColors();
}

static commandResult_t SM2135_Current(const void *context, const char *cmd, const char *args, int flags){
	int valRGB;
	int valCW;
	Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	valRGB = Tokenizer_GetArgInteger(0);
	valCW = Tokenizer_GetArgInteger(1);

	SM2135_SetCurrent(valRGB,valCW);
	return CMD_RES_OK;
}

// startDriver SM2135
// CMD_LEDDriver_WriteRGBCW FF00000000
void SM2135_Init() {

	// default setting (applied only if none was applied earlier)
	CFG_SetDefaultLEDRemap(2, 1, 0, 4, 3);

	g_softI2C.pin_clk = PIN_FindPinIndexForRole(IOR_SM2135_CLK,g_softI2C.pin_clk);
	g_softI2C.pin_data = PIN_FindPinIndexForRole(IOR_SM2135_DAT,g_softI2C.pin_data);

	Soft_I2C_PreInit(&g_softI2C);

	//cmddetail:{"name":"SM2135_RGBCW","args":"[HexColor]",
	//cmddetail:"descr":"Don't use it. It's for direct access of SM2135 driver. You don't need it because LED driver automatically calls it, so just use led_basecolor_rgb",
	//cmddetail:"fn":"CMD_LEDDriver_WriteRGBCW","file":"driver/drv_sm2135.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SM2135_RGBCW", CMD_LEDDriver_WriteRGBCW, NULL);
	//cmddetail:{"name":"SM2135_Map","args":"[Ch0][Ch1][Ch2][Ch3][Ch4]",
	//cmddetail:"descr":"Maps the RGBCW values to given indices of SM2135 channels. This is because SM2135 channels order is not the same for some devices. Some devices are using RGBCW order and some are using GBRCW, etc, etc. Example usage: SM2135_Map 0 1 2 3 4",
	//cmddetail:"fn":"CMD_LEDDriver_Map","file":"driver/drv_sm2135.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SM2135_Map", CMD_LEDDriver_Map, NULL);
	//cmddetail:{"name":"SM2135_Current","args":"[RGBLimit][CWLimit]",
	//cmddetail:"descr":"Sets the maximum current for LED driver. Please note that arguments are using SM2135 codes, see [full list of codes here](https://www.elektroda.com/rtvforum/viewtopic.php?p=20493415#20493415)",
	//cmddetail:"fn":"SM2135_Current","file":"driver/drv_sm2135.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SM2135_Current", SM2135_Current, NULL);

	// alias for LED_Map. In future we may want to migrate totally to shared LED_Map command.... 
	CMD_CreateAliasHelper("LED_Map", "SM2135_Map");
}


