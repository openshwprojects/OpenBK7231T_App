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

#include "drv_bp1658cj.h"




void BP1658CJ_Write(float *rgbcw) {
  unsigned short cur_col_10[5];
  int i;

  //ADDLOG_DEBUG(LOG_FEATURE_CMD, "Writing to Lamp: %f %f %f %f %f", rgbcw[0], rgbcw[1], rgbcw[2], rgbcw[3], rgbcw[4]);

	for(i = 0; i < 5; i++){
		// convert 0-255 to 0-1023
		//cur_col_10[i] = rgbcw[g_cfg.ledRemap.ar[i]] * 4;
		cur_col_10[i] = MAP(rgbcw[g_cfg.ledRemap.ar[i]], 0, 255.0f, 0, 1023.0f);
	}
  //ADDLOG_DEBUG(LOG_FEATURE_CMD, "Writing to Lamp (hex): #%02X%02X%02X%02X%02X", cur_col_10[0], cur_col_10[1], cur_col_10[2], cur_col_10[3], cur_col_10[4]);
	// If we receive 0 for all channels, we'll assume that the lightbulb is off, and activate BP1658CJ's sleep mode ([0x80] ).
	if (cur_col_10[0]==0 && cur_col_10[1]==0 && cur_col_10[2]==0 && cur_col_10[3]==0 && cur_col_10[4]==0) {
		Soft_I2C_Start(BP1658CJ_ADDR_SLEEP);
                Soft_I2C_WriteByte(BP1658CJ_SUBADDR);
                for(i = 0; i<10; ++i) //set all 10 channels to 00
                    Soft_I2C_WriteByte(0x00);
		Soft_I2C_Stop();
		return;
	}

	// Even though we could address changing channels only, in practice we observed that the lightbulb always sets all channels.
	Soft_I2C_Start(BP1658CJ_ADDR_OUT);

	// The First Byte is the Subadress
	Soft_I2C_WriteByte(BP1658CJ_SUBADDR);
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

// startDriver BP1658CJ
// BP1658CJ_RGBCW FF00000000
void BP1658CJ_Init() {
	// default map
	CFG_SetDefaultLEDRemap(1, 0, 2, 3, 4);

	g_i2c_pin_clk = PIN_FindPinIndexForRole(IOR_BP1658CJ_CLK,g_i2c_pin_clk);
	g_i2c_pin_data = PIN_FindPinIndexForRole(IOR_BP1658CJ_DAT,g_i2c_pin_data);

    Soft_I2C_PreInit();

	//cmddetail:{"name":"BP1658CJ_RGBCW","args":"[HexColor]",
	//cmddetail:"descr":"Don't use it. It's for direct access of BP1658CJ driver. You don't need it because LED driver automatically calls it, so just use led_basecolor_rgb",
	//cmddetail:"fn":"BP1658CJ_RGBCW","file":"driver/drv_bp1658cj.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("BP1658CJ_RGBCW", CMD_LEDDriver_WriteRGBCW, NULL);
	//cmddetail:{"name":"BP1658CJ_Map","args":"[Ch0][Ch1][Ch2][Ch3][Ch4]",
	//cmddetail:"descr":"Maps the RGBCW values to given indices of BP1658CJ channels. This is because BP5758D channels order is not the same for some devices. Some devices are using RGBCW order and some are using GBRCW, etc, etc. Example usage: BP1658CJ_Map 0 1 2 3 4",
	//cmddetail:"fn":"BP1658CJ_Map","file":"driver/drv_bp1658cj.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("BP1658CJ_Map", CMD_LEDDriver_Map, NULL);
}
