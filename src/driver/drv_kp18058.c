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


void KP18058_Write(float *rgbcw) {

	int i;


	Soft_I2C_Start(&g_softI2C, 0xE1);
	Soft_I2C_WriteByte(&g_softI2C, 0x00);
	Soft_I2C_WriteByte(&g_softI2C, 0x03);
	Soft_I2C_WriteByte(&g_softI2C, 0x7D);
	for (int i = 0; i < 5; i++) {
		unsigned short cur_col_12 = MAP(rgbcw[g_cfg.ledRemap.ar[i]], 0, 255.0f, 0, 4095.0f);
		byte a, b;
		a = cur_col_12 & 0x3F;
		b = (cur_col_12 >> 6) & 0x3F;
		Soft_I2C_WriteByte(&g_softI2C, a);
		Soft_I2C_WriteByte(&g_softI2C, b);
	}
	Soft_I2C_Stop(&g_softI2C);
}

// startDriver KP18058
// KP18058_RGBCW FF00000000
void KP18058_Init() {

	g_softI2C.pin_clk = 26;
	g_softI2C.pin_data = 24;
	//g_softI2C.pin_clk = PIN_FindPinIndexForRole(IOR_KP18058_CLK, g_softI2C.pin_clk);
	//g_softI2C.pin_data = PIN_FindPinIndexForRole(IOR_KP18058_DAT, g_softI2C.pin_data);

	Soft_I2C_PreInit(&g_softI2C);

	//cmddetail:{"name":"BP5758D_RGBCW","args":"[HexColor]",
	//cmddetail:"descr":"Don't use it. It's for direct access of BP5758D driver. You don't need it because LED driver automatically calls it, so just use led_basecolor_rgb",
	//cmddetail:"fn":"BP5758D_RGBCW","file":"driver/drv_bp5758d.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("KP18058_RGBCW", CMD_LEDDriver_WriteRGBCW, NULL);
}
