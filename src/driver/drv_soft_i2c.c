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

static int g_clk_period = SM2135_DELAY;

#if !PLATFORM_ESPIDF && !PLATFORM_XR806 && !PLATFORM_XR872 && !PLATFORM_ESP8266 && !PLATFORM_REALTEK_NEW && !PLATFORM_TXW81X
void usleep(int r) //delay function do 10*r nops, because rtos_delay_milliseconds is too much
{
#ifdef WIN32
	// not possible on Windows port
#else
	for (volatile int i = 0; i < r; i++)
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
#endif
}
#endif

void Soft_I2C_SetLow(uint8_t pin) {
	HAL_PIN_Setup_Output(pin);
	HAL_PIN_SetOutputValue(pin, 0);
}

void Soft_I2C_SetHigh(uint8_t pin) {
	HAL_PIN_Setup_Input_Pullup(pin);
}

static commandResult_t CMD_SoftI2C_SetClkPeriod(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	g_clk_period = Tokenizer_GetArgInteger(0);
	return CMD_RES_OK;
}

bool Soft_I2C_PreInit(softI2C_t *i2c) {
	//cmddetail:{"name":"SoftI2C_SetClkPeriod","args":"[period]",
	//cmddetail:"descr":"Sets the clock period in number of nop delay cycles (times 10)",
	//cmddetail:"fn":"CMD_SoftI2C_SetClkPeriod","file":"driver/drv_soft_i2c.c","requires":"",
	//cmddetail:"examples":"SoftI2C_SetClkPeriod 50"}
	CMD_RegisterCommand("SoftI2C_SetClkPeriod", CMD_SoftI2C_SetClkPeriod, NULL);

	HAL_PIN_SetOutputValue(i2c->pin_data, 0);
	HAL_PIN_SetOutputValue(i2c->pin_clk, 0);
	Soft_I2C_SetHigh(i2c->pin_data);
	Soft_I2C_SetHigh(i2c->pin_clk);
	return (!((HAL_PIN_ReadDigitalInput(i2c->pin_data) == 0 || HAL_PIN_ReadDigitalInput(i2c->pin_clk) == 0)));
}

bool Soft_I2C_WriteByte(softI2C_t *i2c, uint8_t value) {
	uint8_t curr;
	uint8_t ack;

	for (curr = 0x80; curr != 0; curr >>= 1) {
		if (curr & value) {
			Soft_I2C_SetHigh(i2c->pin_data);
		}
		else {
			Soft_I2C_SetLow(i2c->pin_data);
		}
		Soft_I2C_SetHigh(i2c->pin_clk);
		usleep(g_clk_period);
		Soft_I2C_SetLow(i2c->pin_clk);
	}
	// get Ack or Nak
	Soft_I2C_SetHigh(i2c->pin_data);
	Soft_I2C_SetHigh(i2c->pin_clk);
	usleep(g_clk_period / 2);
	ack = HAL_PIN_ReadDigitalInput(i2c->pin_data);
	Soft_I2C_SetLow(i2c->pin_clk);
	usleep(g_clk_period / 2);
	Soft_I2C_SetLow(i2c->pin_data);
	return (0 == ack);
}

void Soft_I2C_Start_Internal(softI2C_t *i2c) {
	Soft_I2C_SetLow(i2c->pin_data);
	usleep(g_clk_period);
	Soft_I2C_SetLow(i2c->pin_clk);
}
bool Soft_I2C_Start(softI2C_t *i2c, uint8_t addr) {
	Soft_I2C_SetLow(i2c->pin_data);
	usleep(g_clk_period);
	Soft_I2C_SetLow(i2c->pin_clk);
	return Soft_I2C_WriteByte(i2c,addr);
}

void Soft_I2C_Stop(softI2C_t *i2c) {
	Soft_I2C_SetLow(i2c->pin_data);
	usleep(g_clk_period);
	Soft_I2C_SetHigh(i2c->pin_clk);
	usleep(g_clk_period);
	Soft_I2C_SetHigh(i2c->pin_data);
	usleep(g_clk_period);
}


void Soft_I2C_ReadBytes(softI2C_t *i2c, uint8_t* buf, int numOfBytes)
{

	for (int i = 0; i < numOfBytes - 1; i++)
	{

		buf[i] = Soft_I2C_ReadByte(i2c,false);

	}

	buf[numOfBytes - 1] = Soft_I2C_ReadByte(i2c,true); //Give NACK on last byte read
}
uint8_t Soft_I2C_ReadByte(softI2C_t *i2c, bool nack)
{
	uint8_t val = 0;

	Soft_I2C_SetHigh(i2c->pin_data);
	for (int i = 0; i < 8; i++)
	{
		usleep(g_clk_period);
		Soft_I2C_SetHigh(i2c->pin_clk);
		val <<= 1;
		if (HAL_PIN_ReadDigitalInput(i2c->pin_data))
		{
			val |= 1;
		}
		Soft_I2C_SetLow(i2c->pin_clk);
	}
	if (nack)
	{
		Soft_I2C_SetHigh(i2c->pin_data);
	}
	else
	{
		Soft_I2C_SetLow(i2c->pin_data);
	}
	Soft_I2C_SetHigh(i2c->pin_clk);
	usleep(g_clk_period);
	Soft_I2C_SetLow(i2c->pin_clk);
	usleep(g_clk_period);
	Soft_I2C_SetLow(i2c->pin_data);

	return val;
}

