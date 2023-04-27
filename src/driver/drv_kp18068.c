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

void KP18068_WriteByte(softI2C_t *i2c, byte b) {
	int i;
	byte sum;

	sum = 0;
	for (i = 1; i < 8; i++) {
		if (BIT_CHECK(b,i)) {
			sum++;
		}
	}
	if (sum % 2 == 0) {
		BIT_CLEAR(b, 0);
	}
	else {
		BIT_SET(b, 0);
	}
	Soft_I2C_WriteByte(i2c, b);
}
void KP18068_Write(float *rgbcw) {

	//Soft_I2C_Start(&g_softI2C, SM2135_ADDR_MC);
	//Soft_I2C_Stop(&g_softI2C);
}

// startDriver KP18068
// KP18068_RGBCW FF00000000
void KP18068_Init() {

	g_softI2C.pin_clk = 1111;
	g_softI2C.pin_data = 111111111;
	//g_softI2C.pin_clk = PIN_FindPinIndexForRole(IOR_KP18068_CLK, g_softI2C.pin_clk);
	//g_softI2C.pin_data = PIN_FindPinIndexForRole(IOR_KP18068_DAT, g_softI2C.pin_data);

	Soft_I2C_PreInit(&g_softI2C);


}
