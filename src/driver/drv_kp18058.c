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


static softI2C_t g_softI2C;
static int g_current_RGB = 14;
static int g_current_CW = 30;

byte GetParityBit(byte b) {
	byte sum;
	int i;

	sum = 0;
	for (i = 1; i < 8; i++) {
		if (BIT_CHECK(b, i)) {
			sum++;
		}
	}
	if (sum % 2 == 0) {
		return 0;
	}
	return 1;
}

void KP18058_Write(float *rgbcw) {
	bool bAllZero = true;

	for (int i = 0; i < 5; i++) {
		if (rgbcw[i] > 0.01f) {
			bAllZero = false;
		}
	}

	// RGB current
	byte byte2 = (rgbcw[0] || rgbcw[1] || rgbcw[2]) ? g_current_RGB : 1;
	byte2 = byte2 << 1;
	byte2 |= GetParityBit(byte2);

	// Bit 7: RGB PWM, Bit 6: Unknown, Bit 5-1: CW current
	byte byte3 = (1 << 7) | (1 << 6) | (g_current_CW << 1);
	byte3 |= GetParityBit(byte3);

	if (bAllZero) {
		Soft_I2C_Start(&g_softI2C, 0x81);
		Soft_I2C_WriteByte(&g_softI2C, 0x00);
		Soft_I2C_WriteByte(&g_softI2C, 0x03);
		Soft_I2C_WriteByte(&g_softI2C, byte3);
		for (int i = 0; i < 10; i++) {
			Soft_I2C_WriteByte(&g_softI2C, 0x00);
		}
	}
	else {
		Soft_I2C_Start(&g_softI2C, 0xE1);
		Soft_I2C_WriteByte(&g_softI2C, 0x00);
		Soft_I2C_WriteByte(&g_softI2C, byte2);
		Soft_I2C_WriteByte(&g_softI2C, byte3);
		for (int i = 0; i < 5; i++) {
			float useVal = GetRGBCW(rgbcw, g_cfg.ledRemap.ar[i]);
			unsigned short cur_col_10 = MAP(useVal, 0, 255.0f, 0, 1023.0f);
			byte a, b;
			a = cur_col_10 & 0x1F;
			b = (cur_col_10 >> 5) & 0x1F;
			a = a << 1;
			b = b << 1;
			a |= GetParityBit(a);
			b |= GetParityBit(b);
			Soft_I2C_WriteByte(&g_softI2C, b);
			Soft_I2C_WriteByte(&g_softI2C, a);
		}
	}
	Soft_I2C_Stop(&g_softI2C);
}

commandResult_t KP18058_Current(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	g_current_RGB = Tokenizer_GetArgIntegerRange(0, 0, 31);
	g_current_CW = Tokenizer_GetArgIntegerRange(1, 0, 31);
	LED_ResendCurrentColors();

	return CMD_RES_OK;
}

// startDriver KP18058
// KP18058_Map 0 1 2 3 4
// KP18058_RGBCW FF00000000
void KP18058_Init() {
	// default map
	CFG_SetDefaultLEDRemap(2, 0, 1, 3, 4);

	g_softI2C.pin_clk = 7;
	g_softI2C.pin_data = 8;
	g_softI2C.pin_clk = PIN_FindPinIndexForRole(IOR_KP18058_CLK, g_softI2C.pin_clk);
	g_softI2C.pin_data = PIN_FindPinIndexForRole(IOR_KP18058_DAT, g_softI2C.pin_data);

	Soft_I2C_PreInit(&g_softI2C);

	//cmddetail:{"name":"KP18058_RGBCW","args":"[HexColor]",
	//cmddetail:"descr":"Don't use it. It's for direct access of KP18058 driver. You don't need it because LED driver automatically calls it, so just use led_basecolor_rgb",
	//cmddetail:"fn":"CMD_LEDDriver_WriteRGBCW","file":"driver/drv_kp18058.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("KP18058_RGBCW", CMD_LEDDriver_WriteRGBCW, NULL);
	//cmddetail:{"name":"KP18058_Map","args":"[Ch0][Ch1][Ch2][Ch3][Ch4]",
	//cmddetail:"descr":"Maps KP18058_Map RGBCW values to given indices of KP18058 channels. This is because KP18058 channels order is not the same for some devices. Some devices are using RGBCW order and some are using GBRCW, etc, etc. Example usage: KP18058_Map 0 1 2 3 4",
	//cmddetail:"fn":"CMD_LEDDriver_Map","file":"driver/drv_kp18058.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("KP18058_Map", CMD_LEDDriver_Map, NULL);
	//cmddetail:{"name":"KP18058_Current","args":"[RGBLimit][CWLimit]",
	//cmddetail:"descr":"Sets the maximum current for LED driver. Values 0-31. Example usage: KP18058_Current 14 30",
	//cmddetail:"fn":"KP18058_Current","file":"driver/drv_kp18058.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("KP18058_Current", KP18058_Current, NULL);
}
