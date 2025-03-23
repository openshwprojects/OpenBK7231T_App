// NOTE: based on https://github.com/adafruit/MAX6675-library/blob/master/max6675.cpp public domain
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "../hal/hal_pins.h"

int port_cs = 24;
int sclk = 26;
int miso = 6;
int targetChannel = -1;

byte MAX6675_ReadByte(void) {
	int i;
	byte d = 0;

	for (i = 7; i >= 0; i--) {
		HAL_PIN_SetOutputValue(sclk, 0);
		delay_ms(10);
		if (HAL_PIN_ReadDigitalInput(miso)) {
			// set the bit to 0 no matter what
			d |= (1 << i);
		}

		HAL_PIN_SetOutputValue(sclk, 1);
		delay_ms(10);
	}

	return d;
}
float MAX6675_ReadTemperature() {
	uint16_t v;

	HAL_PIN_SetOutputValue(port_cs, 0);
	delay_ms(10);

	v = MAX6675_ReadByte();
	v <<= 8;
	v |= MAX6675_ReadByte();

	HAL_PIN_SetOutputValue(port_cs, 1);

	if (v & 0x4) {
		// uh oh, no thermocouple attached!
		return -999;
		// return -100;
	}

	v >>= 3;

	return v * 0.25F;
}
void MAX6675_RunEverySecond() {
	float f = MAX6675_ReadTemperature();
	addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "T %f.", f);
	if (targetChannel != -1) {
		int type = CHANNEL_GetType(targetChannel);
		if (type == ChType_Temperature_div10) {
			CHANNEL_Set(targetChannel, f*0.1f, 0);
		}
		else {
			CHANNEL_Set(targetChannel, f, 0);
		}
	}
}
// startDriver MAX6675 24 26 6 1
void MAX6675_Init() {
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

