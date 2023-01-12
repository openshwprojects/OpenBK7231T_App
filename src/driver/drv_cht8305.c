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

#include "drv_cht8305.h"

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

#define CHT8305_I2C_ADDR (0x40 << 1)

static byte channel_temp = 0, channel_humid = 0;
static float g_temp = 0.0, g_humid = 0.0;

static void CHT8305_SetLow(uint8_t pin) {
	HAL_PIN_Setup_Output(pin);
	HAL_PIN_SetOutputValue(pin, 0);
	}

static void CHT8305_SetHigh(uint8_t pin) {
	HAL_PIN_Setup_Input_Pullup(pin);
	}

static bool CHT8305_PreInit(void) {
	HAL_PIN_SetOutputValue(g_pin_data, 0);
	HAL_PIN_SetOutputValue(g_pin_clk, 0);
	CHT8305_SetHigh(g_pin_data);
	CHT8305_SetHigh(g_pin_clk);
	return (!((HAL_PIN_ReadDigitalInput(g_pin_data) == 0 || HAL_PIN_ReadDigitalInput(g_pin_clk) == 0)));
	}

static bool CHT8305_WriteByte(uint8_t value) {
	uint8_t curr;
	uint8_t ack;

	for (curr = 0x80; curr != 0; curr >>= 1)
		{
		if (curr & value)
			{
			CHT8305_SetHigh(g_pin_data);
			}
		else
			{
			CHT8305_SetLow(g_pin_data);
			}
		CHT8305_SetHigh(g_pin_clk);
		usleep(CHT8305_DELAY);
		CHT8305_SetLow(g_pin_clk);
		}
	// get Ack or Nak
	CHT8305_SetHigh(g_pin_data);
	CHT8305_SetHigh(g_pin_clk);
	usleep(CHT8305_DELAY / 2);
	ack = HAL_PIN_ReadDigitalInput(g_pin_data);
	CHT8305_SetLow(g_pin_clk);
	usleep(CHT8305_DELAY / 2);
	CHT8305_SetLow(g_pin_data);
	return (0 == ack);
	}

static bool CHT8305_Start(uint8_t addr) {
	CHT8305_SetLow(g_pin_data);
	usleep(CHT8305_DELAY);
	CHT8305_SetLow(g_pin_clk);
	return CHT8305_WriteByte(addr);
	}

static void CHT8305_Stop(void) {
	CHT8305_SetLow(g_pin_data);
	usleep(CHT8305_DELAY);
	CHT8305_SetHigh(g_pin_clk);
	usleep(CHT8305_DELAY);
	CHT8305_SetHigh(g_pin_data);
	usleep(CHT8305_DELAY);
	}

static uint8_t CHT8305_ReadByte(bool nack)
	{
	uint8_t val = 0;

	CHT8305_SetHigh(g_pin_data);
	for (int i = 0; i < 8; i++)
		{
		usleep(CHT8305_DELAY);
		CHT8305_SetHigh(g_pin_clk);
		val <<= 1;
		if (HAL_PIN_ReadDigitalInput(g_pin_data))
			{
			val |= 1;
			}
		CHT8305_SetLow(g_pin_clk);
		}
	if (nack)
		{
		CHT8305_SetHigh(g_pin_data);
		}
	else
		{
		CHT8305_SetLow(g_pin_data);
		}
	CHT8305_SetHigh(g_pin_clk);
	usleep(CHT8305_DELAY);
	CHT8305_SetLow(g_pin_clk);
	usleep(CHT8305_DELAY);
	CHT8305_SetLow(g_pin_data);

	return val;
	}


static void CHT8305_ReadBytes(uint8_t* buf, int numOfBytes)
	{

	for (int i = 0; i < numOfBytes - 1; i++)
		{

		buf[i] = CHT8305_ReadByte(false);

		}

	buf[numOfBytes - 1] = CHT8305_ReadByte(true); //Give NACK on last byte read
	}



//static commandResult_t CHT8305_GETENV(const void* context, const char* cmd, const char* args, int flags){
//	const char* c = args;
//
//	ADDLOG_DEBUG(LOG_FEATURE_CMD, "CHT8305_GETENV");
//
//	return CMD_RES_OK;
//}

static void CHT8305_ReadEnv(float* temp, float* hum)
	{
	uint8_t buff[4];
	unsigned int th, tl, hh, hl;

	CHT8305_Start(CHT8305_I2C_ADDR);
	CHT8305_WriteByte(0x00);
	CHT8305_Stop();

	rtos_delay_milliseconds(20);	//give the sensor time to do the conversion

	CHT8305_Start(CHT8305_I2C_ADDR | 1);
	CHT8305_ReadBytes(buff, 4);
	CHT8305_Stop();

	th = buff[0];
	tl = buff[1];
	hh = buff[2];
	hl = buff[3];

	(*temp) = (th << 8 | tl) * 165.0 / 65535.0 - 40.0;

	(*hum) = (hh << 8 | hl) * 100.0 / 65535.0;

	}

// startDriver CHT8305
void CHT8305_Init() {

	uint8_t buff[4];

	CHT8305_PreInit();

	g_pin_clk = PIN_FindPinIndexForRole(IOR_CHT8305_CLK, g_pin_clk);
	g_pin_data = PIN_FindPinIndexForRole(IOR_CHT8305_DAT, g_pin_data);

	CHT8305_Start(CHT8305_I2C_ADDR);
	CHT8305_WriteByte(0xfe);			//manufacturer ID
	CHT8305_Stop();
	CHT8305_Start(CHT8305_I2C_ADDR | 1);
	CHT8305_ReadBytes(buff, 2);
	CHT8305_Stop();

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "DRV_CHT8304_init: ID: %02X %02X\n", buff[0], buff[1]);



	}

void CHT8305_OnChannelChanged(int ch, int value) {
	}

void CHT8305_OnEverySecond() {

	CHT8305_ReadEnv(&g_temp, &g_humid);

	channel_temp = g_cfg.pins.channels[g_pin_data];
	channel_humid = g_cfg.pins.channels2[g_pin_data];
	// don't want to loose accuracy, so multiply by 10
	// We have a channel types to handle that
	CHANNEL_Set(channel_temp, (int)(g_temp * 10), 0);
	CHANNEL_Set(channel_humid, (int)(g_humid), 0);

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "DRV_CHT8304_readEnv: Temperature:%fC Humidity:%f%%\n", g_temp, g_humid);
}

void CHT8305_AppendInformationToHTTPIndexPage(http_request_t* request)
{
	hprintf255(request, "<h2>CHT8305 Temperature=%f, Humidity=%f</h2>", g_temp, g_humid);
	if (channel_humid == channel_temp) {
		hprintf255(request, "WARNING: You don't have configured target channels for temp and humid results, set the first and second channel index in Pins!");
	}
}

