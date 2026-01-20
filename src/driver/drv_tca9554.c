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

// default for ESP32-S3-ETH-8DI-8RO
static int tca_adr = (0x20 << 1);
#define TCA_CHANNELS 8

static softI2C_t tcI2C;
static int tca_firstChannel;
static int tca_values = 0;

// startDriver TCA9554 [SCL] [SDA] [FirstChannel] [Adr]
// startDriver TCA9554 41 42 8
// backlog stopDriver *; startDriver TCA9554 41 42 8 64
/*
startDriver TCA9554 41 42 0
setChannelType 0 Toggle
setChannelType 1 Toggle
setChannelType 2 Toggle
setChannelType 3 Toggle
setChannelType 4 Toggle
setChannelType 5 Toggle
setChannelType 6 Toggle
setChannelType 7 Toggle


*/
void TCA9554_Init() {
//	tcI2C.pin_clk = Tokenizer_GetArgIntegerDefault(1, 41);
//	tcI2C.pin_data = Tokenizer_GetArgIntegerDefault(2, 42);
	tcI2C.pin_clk = Tokenizer_GetPin(1, 41);
	tcI2C.pin_data = Tokenizer_GetPin(2, 42);
	tca_firstChannel = Tokenizer_GetArgIntegerDefault(3, 8);
	tca_adr = Tokenizer_GetArgIntegerDefault(4, tca_adr);

	Soft_I2C_PreInit(&tcI2C);

	rtos_delay_milliseconds(1);
	Soft_I2C_Start(&tcI2C, tca_adr);
	Soft_I2C_WriteByte(&tcI2C, 0x03);
	Soft_I2C_WriteByte(&tcI2C, 0x00);
	Soft_I2C_Stop(&tcI2C);
}
void TCA9954_ApplyChanges() {
	Soft_I2C_Start(&tcI2C, tca_adr);
	Soft_I2C_WriteByte(&tcI2C, 0x01);
	Soft_I2C_WriteByte(&tcI2C, tca_values);
	Soft_I2C_Stop(&tcI2C);
}
void TCA9554_OnChannelChanged(int ch, int value) {
	if (ch >= tca_firstChannel && ch < (tca_firstChannel + TCA_CHANNELS)) {
		int local = ch - tca_firstChannel;
		if (value) {
			tca_values |= (1 << local);  // set bit
		}
		else {
			tca_values &= ~(1 << local); // clear bit
		}
		TCA9954_ApplyChanges(); // write changes to TCA9554
	}
}

void TCA9554_OnEverySecond()
{
	//static int x = 0;
	//// launch measurement on sensor. 
	//Soft_I2C_Start(&tcI2C, tca_adr);
	//Soft_I2C_WriteByte(&tcI2C, 0x03);
	//Soft_I2C_WriteByte(&tcI2C, 0x00);
	//Soft_I2C_Stop(&tcI2C);

	//rtos_delay_milliseconds(12);
	//Soft_I2C_Start(&tcI2C, tca_adr);
	//Soft_I2C_WriteByte(&tcI2C, 0x01);
	//Soft_I2C_WriteByte(&tcI2C, x);
	//Soft_I2C_Stop(&tcI2C);

	//x = ~x;
}

