// NOTE: based on https://github.com/adafruit/MAX31855-library/blob/master/MAX31855.cpp public domain
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "../hal/hal_pins.h"

static int port_cs = 24;
static int sclk = 26;
static int miso = 6;
static int targetChannel = -1;

#define MAX31855_THERMOCOUPLE_RESOLUTION    0.25   //in °C per dac step
#define MAX31855_COLD_JUNCTION_RESOLUTION   0.0625 //in °C per dac step

static int stage = 0;
int MAX31855_ReadRaw(void) {
	int i;
	int d = 0;

	//	delay_ms(100);
	HAL_PIN_SetOutputValue(port_cs, 0);
	usleep(10);
	for (i = 31; i >= 0; i--) {
		HAL_PIN_SetOutputValue(sclk, 1);
		usleep(10);
		if (HAL_PIN_ReadDigitalInput(miso)) {
			d |= (1 << i);
		}
		HAL_PIN_SetOutputValue(sclk, 0);
		usleep(10);
	}
	HAL_PIN_SetOutputValue(port_cs, 1);

	return d;
}

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)

void MAX31855_ReadTemperature() {
	int raw;

	stage = !stage;

	// request data
	if (stage) {
		HAL_PIN_SetOutputValue(port_cs, 0);
		usleep(10);
		HAL_PIN_SetOutputValue(port_cs, 1);
		return;
	}
	// read data
	raw = MAX31855_ReadRaw();

	// print it like 0xFFAABBCC
	//addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "0x%08X", raw);

	if (raw == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "MAX31855 read fail");
		return;
	}

	if (bitRead(raw, 17) == 0 && bitRead(raw, 3) == 0) {

	}
	else {
		addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "MAX31855 bad ID");
		return;
	}

	// clear D17..D0 bits
	float temp = (float)(raw >> 18) * MAX31855_THERMOCOUPLE_RESOLUTION;
	float cjTemp = (float)((raw & 0x0000FFFF) >> 4) * MAX31855_COLD_JUNCTION_RESOLUTION;

	addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "T %f, cjT %f", temp, cjTemp);
}
void MAX31855_RunEverySecond() {
	MAX31855_ReadTemperature();
}
// startDriver MAX31855 24 26 6 1
void MAX31855_Init() {
	port_cs = Tokenizer_GetArgIntegerDefault(1, port_cs);
	sclk = Tokenizer_GetArgIntegerDefault(2, sclk);
	miso = Tokenizer_GetArgIntegerDefault(3, miso);
	targetChannel = Tokenizer_GetArgIntegerDefault(4, targetChannel);

	// define pin modes
	HAL_PIN_Setup_Output(port_cs);
	HAL_PIN_Setup_Output(sclk);
	HAL_PIN_Setup_Input(miso);

	HAL_PIN_SetOutputValue(port_cs, 1);
}

