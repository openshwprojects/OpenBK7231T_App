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

void SM2135_Write(float *rgbcw) {
			//Soft_I2C_Start(&g_softI2C, SM2135_ADDR_MC);
			//Soft_I2C_WriteByte(&g_softI2C, combinedCurrent);
			//Soft_I2C_WriteByte(&g_softI2C, SM2135_RGB);
			//Soft_I2C_WriteByte(&g_softI2C, GetRGBCW(rgbcw,g_cfg.ledRemap.r));
			//Soft_I2C_WriteByte(&g_softI2C, GetRGBCW(rgbcw,g_cfg.ledRemap.g));
			//Soft_I2C_WriteByte(&g_softI2C, GetRGBCW(rgbcw,g_cfg.ledRemap.b));
			//Soft_I2C_Stop(&g_softI2C);
}


void SSD1306_Init() {


	g_softI2C.pin_clk = 16;
	g_softI2C.pin_data = 20;

	Soft_I2C_PreInit(&g_softI2C);

}

void SSD1306_OnEverySecond() {


}

