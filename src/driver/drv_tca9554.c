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

#define TCA9554_ADDR 0x20
#define TCA_CHANNELS 8

static softI2C_t tcI2C;
static int tca_firstChannel;
// startDriver TCA9554 [SCL] [SDA] [FirstChannel]
void TCA9554_Init() {


	tcI2C.pin_clk = Tokenizer_GetArgIntegerDefault(2, 41);
	tcI2C.pin_data = Tokenizer_GetArgIntegerDefault(3, 42);
	tca_firstChannel = Tokenizer_GetArgIntegerDefault(4, 8);

	Soft_I2C_PreInit(&tcI2C);

}
void TCA9554_OnChannelChanged(int ch, int value) {
	if (ch >= tca_firstChannel && ch < (tca_firstChannel + TCA_CHANNELS)) {
		int local = ch - tca_firstChannel;

	}
}
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

