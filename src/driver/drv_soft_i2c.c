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

// Some platforms have less pins than BK7231T.
// For example, BL602 doesn't have pin number 26.
// The pin code would crash BL602 while trying to access pin 26.
// This is why the default settings here a per-platform.
#if PLATFORM_BEKEN
int g_i2c_pin_clk = 26;
int g_i2c_pin_data = 24;
#else
int g_i2c_pin_clk = 0;
int g_i2c_pin_data = 1;
#endif

void usleep(int r) //delay function do 10*r nops, because rtos_delay_milliseconds is too much
{
#ifdef WIN32
	// not possible on Windows port
#else
	for (volatile int i = 0; i < r; i++)
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
#endif
}

void Soft_I2C_SetLow(uint8_t pin) {
	HAL_PIN_Setup_Output(pin);
	HAL_PIN_SetOutputValue(pin, 0);
}

void Soft_I2C_SetHigh(uint8_t pin) {
	HAL_PIN_Setup_Input_Pullup(pin);
}

bool Soft_I2C_PreInit(void) {
	HAL_PIN_SetOutputValue(g_i2c_pin_data, 0);
	HAL_PIN_SetOutputValue(g_i2c_pin_clk, 0);
	Soft_I2C_SetHigh(g_i2c_pin_data);
	Soft_I2C_SetHigh(g_i2c_pin_clk);
	return (!((HAL_PIN_ReadDigitalInput(g_i2c_pin_data) == 0 || HAL_PIN_ReadDigitalInput(g_i2c_pin_clk) == 0)));
}

bool Soft_I2C_WriteByte(uint8_t value) {
	uint8_t curr;
	uint8_t ack;

	for (curr = 0X80; curr != 0; curr >>= 1) {
		if (curr & value) {
			Soft_I2C_SetHigh(g_i2c_pin_data);
		}
		else {
			Soft_I2C_SetLow(g_i2c_pin_data);
		}
		Soft_I2C_SetHigh(g_i2c_pin_clk);
		usleep(SM2135_DELAY);
		Soft_I2C_SetLow(g_i2c_pin_clk);
	}
	// get Ack or Nak
	Soft_I2C_SetHigh(g_i2c_pin_data);
	Soft_I2C_SetHigh(g_i2c_pin_clk);
	usleep(SM2135_DELAY / 2);
	ack = HAL_PIN_ReadDigitalInput(g_i2c_pin_data);
	Soft_I2C_SetLow(g_i2c_pin_clk);
	usleep(SM2135_DELAY / 2);
	Soft_I2C_SetLow(g_i2c_pin_data);
	return (0 == ack);
}

bool Soft_I2C_Start(uint8_t addr) {
	Soft_I2C_SetLow(g_i2c_pin_data);
	usleep(SM2135_DELAY);
	Soft_I2C_SetLow(g_i2c_pin_clk);
	return Soft_I2C_WriteByte(addr);
}

void Soft_I2C_Stop(void) {
	Soft_I2C_SetLow(g_i2c_pin_data);
	usleep(SM2135_DELAY);
	Soft_I2C_SetHigh(g_i2c_pin_clk);
	usleep(SM2135_DELAY);
	Soft_I2C_SetHigh(g_i2c_pin_data);
	usleep(SM2135_DELAY);
}


void Soft_I2C_ReadBytes(uint8_t* buf, int numOfBytes)
{

	for (int i = 0; i < numOfBytes - 1; i++)
	{

		buf[i] = Soft_I2C_ReadByte(false);

	}

	buf[numOfBytes - 1] = Soft_I2C_ReadByte(true); //Give NACK on last byte read
}
uint8_t Soft_I2C_ReadByte(bool nack)
{
	uint8_t val = 0;

	Soft_I2C_SetHigh(g_i2c_pin_data);
	for (int i = 0; i < 8; i++)
	{
		usleep(SM2135_DELAY);
		Soft_I2C_SetHigh(g_i2c_pin_clk);
		val <<= 1;
		if (HAL_PIN_ReadDigitalInput(g_i2c_pin_data))
		{
			val |= 1;
		}
		Soft_I2C_SetLow(g_i2c_pin_clk);
	}
	if (nack)
	{
		Soft_I2C_SetHigh(g_i2c_pin_data);
	}
	else
	{
		Soft_I2C_SetLow(g_i2c_pin_data);
	}
	Soft_I2C_SetHigh(g_i2c_pin_clk);
	usleep(SM2135_DELAY);
	Soft_I2C_SetLow(g_i2c_pin_clk);
	usleep(SM2135_DELAY);
	Soft_I2C_SetLow(g_i2c_pin_data);

	return val;
}

