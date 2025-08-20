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

#define SGP_I2C_ADDRESS (0x58 << 1)

static softI2C_t tcI2C;

// startDriver TCA9554
void TCA9554_Init() {


	tcI2C.pin_clk = 9;
	tcI2C.pin_data = 14;

	Soft_I2C_PreInit(&tcI2C);

}
#define TCA9554_ADDR 0x20
int x = 0;
void TCA9554_OnEverySecond()
{
	// launch measurement on sensor. 
	Soft_I2C_Start(&tcI2C, TCA9554_ADDR);
	Soft_I2C_WriteByte(&tcI2C, 0x03);
	Soft_I2C_WriteByte(&tcI2C, 0x00);
	Soft_I2C_Stop(&tcI2C);

	rtos_delay_milliseconds(12);
	Soft_I2C_Start(&tcI2C, TCA9554_ADDR);
	Soft_I2C_WriteByte(&tcI2C, 0x01);
	Soft_I2C_WriteByte(&tcI2C, x);
	Soft_I2C_Stop(&tcI2C);

	x = ~x;

}

