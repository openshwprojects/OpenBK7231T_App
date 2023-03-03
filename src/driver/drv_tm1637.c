// NOTE: qqq
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

#include "drv_tm1637.h"

#define TM1637_DELAY 120

static byte g_brightness;
static byte g_digits[] = {
	0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71,0xff
};
// I don't know why, my display has incorrect order of displayed characters
static byte g_remap[] = {
	2,1,0,5,4,3,8,7,6
};
static byte g_numDigits = sizeof(g_digits) / sizeof(g_digits[0]);
static byte *tm1638_buffer = 0;

static void TM1637_Start(softI2C_t *i2c) {
	HAL_PIN_Setup_Output(i2c->pin_data);
	HAL_PIN_Setup_Output(i2c->pin_clk);
	HAL_PIN_SetOutputValue(i2c->pin_clk, true);//send start signal to TM1637
	HAL_PIN_SetOutputValue(i2c->pin_data, true);
	HAL_PIN_SetOutputValue(i2c->pin_data, 0);
	HAL_PIN_SetOutputValue(i2c->pin_clk, 0);
}
static void TM1637_Stop(softI2C_t *i2c) {
	HAL_PIN_SetOutputValue(i2c->pin_clk, false);
	HAL_PIN_SetOutputValue(i2c->pin_data, false);
	HAL_PIN_SetOutputValue(i2c->pin_clk, true);
	HAL_PIN_SetOutputValue(i2c->pin_data, true);
}

static bool TM1637_WriteByte(softI2C_t *i2c, byte b) {
	int i, ack;
	for (i = 0; i < 8; i++) {
		HAL_PIN_SetOutputValue(i2c->pin_clk, false);
		HAL_PIN_SetOutputValue(i2c->pin_data, b & 0x01);
		b >>= 1;
		HAL_PIN_SetOutputValue(i2c->pin_clk, true);
	}
	// ACK test
	HAL_PIN_SetOutputValue(i2c->pin_clk, false);
	HAL_PIN_SetOutputValue(i2c->pin_data, true);
	HAL_PIN_SetOutputValue(i2c->pin_clk, true);
	HAL_PIN_Setup_Input(i2c->pin_data);

	usleep(TM1637_DELAY);
	ack = HAL_PIN_ReadDigitalInput(i2c->pin_data);
	if (ack == 0) {
		HAL_PIN_Setup_Output(i2c->pin_data);
		HAL_PIN_SetOutputValue(i2c->pin_data, false);
	}
	usleep(TM1637_DELAY);
	HAL_PIN_Setup_Output(i2c->pin_data);
	usleep(TM1637_DELAY);

	return ack;
}
static void TM1637_SetBrightness(byte brightness, bool on) {
	g_brightness = (brightness & 0x7) | (on ? 0x08 : 0x00);
}
static void TM1637_SendSegments(const byte *segments, byte length, byte pos) {
	int i;
	softI2C_t i2c;

	i2c.pin_clk = 24;
	i2c.pin_data = 26;
	// set COM1
	TM1637_Start(&i2c);
	TM1637_WriteByte(&i2c, TM1637_I2C_COM1);
	TM1637_Stop(&i2c);

	// set COM2 + first digit address
	TM1637_Start(&i2c);
	TM1637_WriteByte(&i2c, TM1637_I2C_COM2 + (pos & 0x03));

	// send the data bytes
	for (i = 0; i < length; i++) {
		TM1637_WriteByte(&i2c, segments[i]);
	}

	TM1637_Stop(&i2c);

	// set COM3 + brightness
	TM1637_Start(&i2c);
	TM1637_WriteByte(&i2c, TM1637_I2C_COM3 + (g_brightness & 0x0f));
	TM1637_Stop(&i2c);
}
static int TM1637_MapCharacter(int ch) {
	int ret;
	if (ch >= '0' && ch <= '9') {
		return ch - '0';
	}
	ch = tolower(ch);
	ret = 10 + (ch - 'a');
	if (ret > 16)
		ret = 16;
	return ret;
}
static void TM1637_PrintStringAt(const char *str, byte pos) {
	int i, len, idx;
	if (tm1638_buffer == 0) {
		tm1638_buffer = (byte*)malloc(TM1637_MAX_CHARS);
	}
	len = strlen(str);
	if (len > TM1637_MAX_CHARS)
		len = TM1637_MAX_CHARS;
	for (i = 0; i < len; i++) {
		idx = TM1637_MapCharacter(str[i]);
		if (idx >= g_numDigits) {
			idx = g_numDigits - 1;
		} else if (idx < 0) {
			idx = 0;
		}
		tm1638_buffer[g_remap[i]] = g_digits[idx];
	}
	TM1637_SendSegments(tm1638_buffer, len, pos);
}

static commandResult_t CMD_TM1637_Test(const void *context, const char *cmd, const char *args, int flags) {
	byte segments[8];
	int i;
	for (i = 0; i < 8; i++) {
		segments[i] = g_digits[(Time_getUpTimeSeconds() + i) % 10];
	}

	TM1637_SendSegments(segments, 6, 0);

	return CMD_RES_OK;
}

static commandResult_t CMD_TM1637_Print(const void *context, const char *cmd, const char *args, int flags) {
	int ofs;
	const char *s;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() <= 1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	ofs = Tokenizer_GetArgInteger(0);
	s = Tokenizer_GetArg(1);

	TM1637_PrintStringAt(s, ofs);

	return CMD_RES_OK;
}
static commandResult_t CMD_TM1637_Brightness(const void *context, const char *cmd, const char *args, int flags) {
	int br;
	int bOn = 1;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	br = Tokenizer_GetArgInteger(0);
	if (Tokenizer_GetArgsCount() > 1) {
		bOn = Tokenizer_GetArgInteger(1);
	}

	TM1637_SetBrightness(br, bOn);

	return CMD_RES_OK;
}
// backlog startDriver TM1637; TM1637_Test
// backlog startDriver TM1637; TM1637_Print 0 123456
// backlog TM1637_Brightness 5; TM1637_Test
void TM1637_Init() {

	TM1637_SetBrightness(0x0f, true);

	CMD_RegisterCommand("TM1637_Print", CMD_TM1637_Print, NULL);
	CMD_RegisterCommand("TM1637_Test", CMD_TM1637_Test, NULL);
	CMD_RegisterCommand("TM1637_Brightness", CMD_TM1637_Brightness, NULL);
}
