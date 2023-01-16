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

#include "drv_sht3x.h"

// Some platforms have less pins than BK7231T.
// For example, BL602 doesn't have pin number 26.
// The pin code would crash BL602 while trying to access pin 26.
// This is why the default settings here a per-platform.
#if PLATFORM_BEKEN
static int g_pin_clk = 9;
static int g_pin_data = 14;
#else
static int g_pin_clk = 0;
static int g_pin_data = 1;
#endif

#define SHT3X_I2C_ADDR (0x44 << 1)

static byte channel_temp = 0, channel_humid = 0;
static float g_temp = 0.0, g_humid = 0.0;

static void SHT3X_SetLow(uint8_t pin) {
	HAL_PIN_Setup_Output(pin);
	HAL_PIN_SetOutputValue(pin, 0);
	}

static void SHT3X_SetHigh(uint8_t pin) {
	HAL_PIN_Setup_Input_Pullup(pin);
	}

static bool SHT3X_PreInit(void) {
	HAL_PIN_SetOutputValue(g_pin_data, 0);
	HAL_PIN_SetOutputValue(g_pin_clk, 0);
	SHT3X_SetHigh(g_pin_data);
	SHT3X_SetHigh(g_pin_clk);
	return (!((HAL_PIN_ReadDigitalInput(g_pin_data) == 0 || HAL_PIN_ReadDigitalInput(g_pin_clk) == 0)));
	}

static bool SHT3X_WriteByte(uint8_t value) {
	uint8_t curr;
	uint8_t ack;

	for (curr = 0x80; curr != 0; curr >>= 1)
		{
		if (curr & value)
			{
			SHT3X_SetHigh(g_pin_data);
			}
		else
			{
			SHT3X_SetLow(g_pin_data);
			}
		SHT3X_SetHigh(g_pin_clk);
		usleep(SHT3X_DELAY);
		SHT3X_SetLow(g_pin_clk);
		}
	// get Ack or Nak
	SHT3X_SetHigh(g_pin_data);
	SHT3X_SetHigh(g_pin_clk);
	usleep(SHT3X_DELAY / 2);
	ack = HAL_PIN_ReadDigitalInput(g_pin_data);
	SHT3X_SetLow(g_pin_clk);
	usleep(SHT3X_DELAY / 2);
	SHT3X_SetLow(g_pin_data);
	return (0 == ack);
	}

static bool SHT3X_Start(uint8_t addr) {
	SHT3X_SetLow(g_pin_data);
	usleep(SHT3X_DELAY);
	SHT3X_SetLow(g_pin_clk);
	return SHT3X_WriteByte(addr);
	}

static void SHT3X_Stop(void) {
	SHT3X_SetLow(g_pin_data);
	usleep(SHT3X_DELAY);
	SHT3X_SetHigh(g_pin_clk);
	usleep(SHT3X_DELAY);
	SHT3X_SetHigh(g_pin_data);
	usleep(SHT3X_DELAY);
	}

static uint8_t SHT3X_ReadByte(bool nack)
	{
	uint8_t val = 0;

	SHT3X_SetHigh(g_pin_data);
	for (int i = 0; i < 8; i++)
		{
		usleep(SHT3X_DELAY);
		SHT3X_SetHigh(g_pin_clk);
		val <<= 1;
		if (HAL_PIN_ReadDigitalInput(g_pin_data))
			{
			val |= 1;
			}
		SHT3X_SetLow(g_pin_clk);
		}
	if (nack)
		{
		SHT3X_SetHigh(g_pin_data);
		}
	else
		{
		SHT3X_SetLow(g_pin_data);
		}
	SHT3X_SetHigh(g_pin_clk);
	usleep(SHT3X_DELAY);
	SHT3X_SetLow(g_pin_clk);
	usleep(SHT3X_DELAY);
	SHT3X_SetLow(g_pin_data);

	return val;
	}


static void SHT3X_ReadBytes(uint8_t* buf, int numOfBytes)
	{

	for (int i = 0; i < numOfBytes - 1; i++)
		{

		buf[i] = SHT3X_ReadByte(false);

		}

	buf[numOfBytes - 1] = SHT3X_ReadByte(true); //Give NACK on last byte read
	}



//static commandResult_t SHT3X_GETENV(const void* context, const char* cmd, const char* args, int flags){
//	const char* c = args;
//
//	ADDLOG_DEBUG(LOG_FEATURE_CMD, "SHT3X_GETENV");
//
//	return CMD_RES_OK;
//}

static void SHT3X_ReadEnv(float* temp, float* hum)
	{
	uint8_t buff[6];
	unsigned int th, tl, hh, hl;

	SHT3X_Start(SHT3X_I2C_ADDR);
	// no clock stretching
	SHT3X_WriteByte(0x24);
        // medium repeteability
	SHT3X_WriteByte(0x16);
	SHT3X_Stop();

	rtos_delay_milliseconds(20);	//give the sensor time to do the conversion

	SHT3X_Start(SHT3X_I2C_ADDR | 1);
	SHT3X_ReadBytes(buff, 6);
	SHT3X_Stop();

	// Reset the sensor
	SHT3X_Start(SHT3X_I2C_ADDR);
	SHT3X_WriteByte(0x30);
	SHT3X_WriteByte(0xA2);
	SHT3X_Stop();

	th = buff[0];
	tl = buff[1];
	hh = buff[3];
	hl = buff[4];

	(*temp) = 175 * ( (th * 256 + tl) / 65535.0 ) - 45.0;

	(*hum) = 100 * ((hh * 256 + hl) / 65535.0);

	}

// startDriver SHT3X
void SHT3X_Init() {

	uint8_t status[2];

	SHT3X_PreInit();

	g_pin_clk = PIN_FindPinIndexForRole(IOR_SHT3X_CLK, g_pin_clk);
	g_pin_data = PIN_FindPinIndexForRole(IOR_SHT3X_DAT, g_pin_data);

	SHT3X_Start(SHT3X_I2C_ADDR);
	SHT3X_WriteByte(0x30);			//Disable Heater
	SHT3X_WriteByte(0x66);
	SHT3X_Stop();

	SHT3X_Start(SHT3X_I2C_ADDR);
	SHT3X_WriteByte(0xf3);			//Get Status
	SHT3X_WriteByte(0x2d);
	SHT3X_Stop();
	SHT3X_Start(SHT3X_I2C_ADDR | 1);
	SHT3X_ReadBytes(status, 2);
	SHT3X_Stop();

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "DRV_SHT3X_init: ID: %02X %02X\n", status[0], status[1]);



	}

void SHT3X_OnChannelChanged(int ch, int value) {
	}

void SHT3X_OnEverySecond() {

	SHT3X_ReadEnv(&g_temp, &g_humid);

	channel_temp = g_cfg.pins.channels[g_pin_data];
	channel_humid = g_cfg.pins.channels2[g_pin_data];
	CHANNEL_Set(channel_temp, (int)(g_temp * 10), 0);
	CHANNEL_Set(channel_humid, (int)(g_humid), 0);

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "DRV_SHT3X_readEnv: Temperature:%fC Humidity:%f%%\n", g_temp, g_humid);
}

void SHT3X_AppendInformationToHTTPIndexPage(http_request_t* request)
{
	hprintf255(request, "<h2>SHT3X Temperature=%f, Humidity=%f</h2>", g_temp, g_humid);
	if (channel_humid == channel_temp) {
		hprintf255(request, "WARNING: You don't have configured target channels for temp and humid results, set the first and second channel index in Pins!");
	}
}

