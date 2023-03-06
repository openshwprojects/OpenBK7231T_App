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
	HAL_PIN_SetOutputValue(i2c->pin_clk, true);
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

	i2c.pin_clk = PIN_FindPinIndexForRole(IOR_TM1637_CLK, 0);
	i2c.pin_data = PIN_FindPinIndexForRole(IOR_TM1637_DIO, 0);

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
static void TM1637_PrintStringAt(const char *str, int pos, int maxLen) {
	int i, len, idx;
	int tgIndex;
	len = strlen(str);
	if (len > maxLen)
		len = maxLen;
	tgIndex = pos;
	for (i = 0; i < len; i++) {
		if (tgIndex >= TM1637_MAX_CHARS)
			break;
		if (str[i] == '.') {
			if (tgIndex - 1 >= 0) {
				tm1638_buffer[g_remap[tgIndex - 1]] |= TM1637_DOT;
			}
			continue;
		}
		idx = TM1637_MapCharacter(str[i]);
		if (idx >= g_numDigits) {
			idx = g_numDigits - 1;
		} else if (idx < 0) {
			idx = 0;
		}
		tm1638_buffer[g_remap[tgIndex]] = g_digits[idx];
		tgIndex++;
	}
	TM1637_SendSegments(tm1638_buffer, TM1637_MAX_CHARS, 0);
}

static commandResult_t CMD_TM1637_Clear(const void *context, const char *cmd, const char *args, int flags) {
	memset(tm1638_buffer, 0x00, TM1637_MAX_CHARS);

	TM1637_SendSegments(tm1638_buffer, TM1637_MAX_CHARS, 0);

	return CMD_RES_OK;
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
	int maxLen;
	const char *s;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() <= 2) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	ofs = Tokenizer_GetArgInteger(0);
	maxLen = Tokenizer_GetArgInteger(1);
	s = Tokenizer_GetArg(2);

	if (maxLen <= 0) {
		maxLen = 999;
	}
	TM1637_PrintStringAt(s, ofs, maxLen);

	return CMD_RES_OK;
}
static commandResult_t CMD_TM1637_Map(const void *context, const char *cmd, const char *args, int flags) {
	int i;

	Tokenizer_TokenizeString(args, 0);

	for (i = 0; i < Tokenizer_GetArgsCount(); i++) {
		g_remap[i] = Tokenizer_GetArgInteger(i);

	}

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

	TM1637_SendSegments(tm1638_buffer, TM1637_MAX_CHARS, 0);

	return CMD_RES_OK;
}
// backlog startDriver TM1637; TM1637_Test
// backlog startDriver TM1637; TM1637_Print 0 0 123456
// backlog TM1637_Brightness 5; TM1637_Test
// test print offset
// backlog TM1637_Clear; TM1637_Print 0 0 1
// backlog TM1637_Clear; TM1637_Print 1 0 2
// backlog TM1637_Clear; TM1637_Print 2 0 3
// backlog TM1637_Clear; TM1637_Print 3 0 4
// backlog TM1637_Clear; TM1637_Print 4 0 5
// backlog TM1637_Clear; TM1637_Print 5 0 6
/*

startDriver TM1637
TM1637_Print 0 0 123456

again:
addChannel 10 1 0 15 1
TM1637_Brightness $CH10
delay_s 0.1
goto again

*/
/*

startDriver TM1637
TM1637_Print 0 0 123456

setChannel 10 100000
again:
addChannel 10 1
TM1637_Print 0 0 $CH10
delay_s 0.25
goto again


*/
/*

startDriver TM1637
TM1637_Clear

setChannel 10 12
setChannel 11 123
again:
addChannel 10 1
addChannel 11 3
TM1637_Print 0 2 $CH10
TM1637_Print 3 3 $CH11
delay_s 0.5
goto again
*/
void TM1637_Init() {
	if (tm1638_buffer == 0) {
		tm1638_buffer = (byte*)malloc(TM1637_MAX_CHARS);
		memset(tm1638_buffer, 0, TM1637_MAX_CHARS);
	}

	TM1637_SetBrightness(0x0f, true);

	//cmddetail:{"name":"TM1637_Clear","args":"CMD_TM1637_Clear",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tm1637.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TM1637_Clear", CMD_TM1637_Clear, NULL);
	//cmddetail:{"name":"TM1637_Print","args":"CMD_TM1637_Print",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tm1637.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TM1637_Print", CMD_TM1637_Print, NULL);
	//cmddetail:{"name":"TM1637_Test","args":"CMD_TM1637_Test",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tm1637.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TM1637_Test", CMD_TM1637_Test, NULL);
	//cmddetail:{"name":"TM1637_Brightness","args":"CMD_TM1637_Brightness",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tm1637.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TM1637_Brightness", CMD_TM1637_Brightness, NULL);
	//cmddetail:{"name":"TM1637_Map","args":"CMD_TM1637_Map",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tm1637.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TM1637_Map", CMD_TM1637_Map, NULL);
}
