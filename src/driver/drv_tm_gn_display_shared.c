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

#define GN6932_DELAY usleep(1);

static softI2C_t g_i2c;
static byte g_brightness;
static byte g_digits[] = {
	0x3f, // 0
	0x06, // 1
	0x5b, // 2
	0x4f, // 3
	0x66, // 4
	0x6d, // 5
	0x7d, // 6
	0x07, // 7
	0x7f, // 8
	0x6f, // 9
	0x77, // A
	0x7c, // B
	0x39, // C
	0x5e, // D
	0x79, // E
	0x71, // F
	0xff, // all on
	0x00, // all off
};
// I don't know why, my display has incorrect order of displayed characters
static byte g_remap[16] = {
	2,1,0,5,4,3,8,7,6
};
static byte g_numDigits = sizeof(g_digits) / sizeof(g_digits[0]);
static byte *tmgn_buffer = 0;
static int g_totalDigits = 6;

static void TM1637_SetBrightness(byte brightness, bool on) {
	g_brightness = (brightness & 0x7) | (on ? 0x08 : 0x00);
}

static bool TM_GN_WriteByte(softI2C_t *i2c, byte b) {
	int i, ack;
	if (i2c->pin_stb != -1) {
		ack = 1;
		for (i = 0; i < 8; i++) {
			HAL_PIN_SetOutputValue(i2c->pin_clk, false);
			GN6932_DELAY;
			HAL_PIN_SetOutputValue(i2c->pin_data, b & 0x01);
			GN6932_DELAY;
			b >>= 1;
			HAL_PIN_SetOutputValue(i2c->pin_clk, true);
		}
	}
	else {
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

	}
	return ack;
}
static void TM_GN_Start(softI2C_t *i2c) {
	if (i2c->pin_stb != -1) {
		HAL_PIN_SetOutputValue(i2c->pin_clk, true);
		HAL_PIN_SetOutputValue(i2c->pin_stb, false);
	}
	else {
		HAL_PIN_SetOutputValue(i2c->pin_clk, true);
		HAL_PIN_SetOutputValue(i2c->pin_data, true);
		HAL_PIN_SetOutputValue(i2c->pin_data, 0);
		HAL_PIN_SetOutputValue(i2c->pin_clk, 0);
	}
}
static void TM_GN_Stop(softI2C_t *i2c) {
	if (i2c->pin_stb != -1) {
		HAL_PIN_SetOutputValue(i2c->pin_stb, true);
	}
	else {
		HAL_PIN_SetOutputValue(i2c->pin_clk, false);
		HAL_PIN_SetOutputValue(i2c->pin_data, false);
		HAL_PIN_SetOutputValue(i2c->pin_clk, true);
		HAL_PIN_SetOutputValue(i2c->pin_data, true);
	}
}

// backlog startDriver TM1637; TM1650_Test
#define TM1650_I2C_DATA_COMMAND 0x48
#define TM1650_I2C_DIGITS 0x68
static void TM1650_SendSegments(const byte *segments, byte length, byte pos) {
	int i;
	softI2C_t i2c;
	byte brightness_and_type_byte;

	i2c.pin_clk = PIN_FindPinIndexForRole(IOR_TM1637_CLK, 16);
	i2c.pin_data = PIN_FindPinIndexForRole(IOR_TM1637_DIO, 14);

	brightness_and_type_byte = 0xff;

	TM_GN_Start(&i2c);
	TM_GN_WriteByte(&i2c, TM1650_I2C_DATA_COMMAND);
	TM_GN_WriteByte(&i2c, brightness_and_type_byte);
	TM_GN_Stop(&i2c);


	// send the data bytes
	for (i = 0; i < length; i++) {
		// set COM2 + first digit address
		TM_GN_Start(&i2c);
		TM_GN_WriteByte(&i2c, TM1650_I2C_DIGITS + pos*2);
		TM_GN_WriteByte(&i2c, segments[i]);
		TM_GN_Stop(&i2c);
	}



}




static void TM_GN_WriteCommand(softI2C_t *i2c, byte command, const byte *data, int dataSize) {
	int i;

	TM_GN_Start(i2c);
	// write command
	TM_GN_WriteByte(i2c, command);
	// write data, if available
	if (data && dataSize) {
		for (i = 0; i < dataSize; i++) {
			TM_GN_WriteByte(i2c, data[i]);
		}
	}
	TM_GN_Stop(i2c);
}
static void TM1637_SendSegments(const byte *segments, byte length, byte pos) {
	int i;

	// set COM1 (no data, just command)
	TM_GN_WriteCommand(&g_i2c, TM1637_I2C_COM1, 0, 0);

	// set COM2 + first digit address
	TM_GN_WriteCommand(&g_i2c, TM1637_I2C_COM2 + (pos & 0x03), segments, length);

	// set COM3 + brightness (no data, just command)
	TM_GN_WriteCommand(&g_i2c, TM1637_I2C_COM3 + (g_brightness & 0x0f), 0, 0);
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
static void TM1637_SetBit(int pos, int bit, int state) {
	if (state) {
		BIT_SET(tmgn_buffer[g_remap[pos]], bit);
	}
	else {
		BIT_CLEAR(tmgn_buffer[g_remap[pos]], bit);
	}
}
static void TM1637_PrintStringAt(const char *str, int pos, int maxLen) {
	int i, len, idx;
	int tgIndex;
	len = strlen(str);
	if (len > maxLen)
		len = maxLen;
	tgIndex = pos;
	for (i = 0; i < len; i++) {
		if (tgIndex >= g_totalDigits)
			break;
		if (str[i] == '.') {
			if (tgIndex - 1 >= 0) {
				tmgn_buffer[g_remap[tgIndex - 1]] |= TM1637_DOT;
			}
			continue;
		}
		idx = TM1637_MapCharacter(str[i]);
		if (idx >= g_numDigits) {
			idx = g_numDigits - 1;
		} else if (idx < 0) {
			idx = 0;
		}
		tmgn_buffer[g_remap[tgIndex]] = g_digits[idx];
		tgIndex++;
	}
}

static commandResult_t CMD_TM1637_Clear(const void *context, const char *cmd, const char *args, int flags) {
	memset(tmgn_buffer, 0x00, g_totalDigits);

	TM1637_SendSegments(tmgn_buffer, g_totalDigits, 0);

	return CMD_RES_OK;
}
static commandResult_t CMD_TM1650_Test(const void *context, const char *cmd, const char *args, int flags) {
	byte segments[8];
	int i;
	for (i = 0; i < 8; i++) {
		segments[i] = g_digits[(Time_getUpTimeSeconds() + i) % 10];
	}

	TM1650_SendSegments(segments, 4, 0);

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

// TMGN_SetBit [DigitIndex] [BitIndex] [1or0]
// TMGN_SetBit 1 0 0
// TMGN_SetBit 1 0 1
static commandResult_t CMD_TM1637_SetBit(const void *context, const char *cmd, const char *args, int flags) {
	int digitIndex;
	int bitIndex;
	int newValue;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() <= 1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	digitIndex = Tokenizer_GetArgInteger(0);
	bitIndex = Tokenizer_GetArgInteger(1);
	newValue = Tokenizer_GetArgInteger(2);

	TM1637_SetBit(digitIndex, bitIndex, newValue);
	TM1637_SendSegments(tmgn_buffer, g_totalDigits, 0);

	return CMD_RES_OK;
}
static commandResult_t CMD_TM1637_Char(const void *context, const char *cmd, const char *args, int flags) {
	int index;
	int code;
	const char *s;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() <= 1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	index = Tokenizer_GetArgInteger(0);
	code = Tokenizer_GetArgInteger(1);
	
	g_digits[index] = code;

	return CMD_RES_OK;
}
// [StartOfs] [MaxLenOr0] [StringText] [optionalBClampWithZeroesForClock]
static commandResult_t CMD_TM1637_Print(const void *context, const char *cmd, const char *args, int flags) {
	int ofs;
	int maxLen;
	const char *s;
	int sLen;
	int iPadZeroes;

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
	if (Tokenizer_GetArgInteger(3)) {
		sLen = strlen(s);
		if (sLen < maxLen) {
			iPadZeroes = maxLen - sLen;
			TM1637_PrintStringAt("00000", ofs, iPadZeroes);
			ofs += iPadZeroes;
			maxLen -= iPadZeroes;
		}
	} 
	TM1637_PrintStringAt(s, ofs, maxLen);
	TM1637_SendSegments(tmgn_buffer, g_totalDigits, 0);

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

	TM1637_SendSegments(tmgn_buffer, g_totalDigits, 0);

	return CMD_RES_OK;
}
// NOTE: TMGN_Print [Ofs] [MaxLen]
// If MaxLen=0, then no lenght limit

// NOTE: TMGN_Brightness [Brightness0to8] [bOn]
// The bOn is optional, default is 1

// backlog startDriver TM1637; TMGN_Test
// backlog startDriver TM1637; TMGN_Print 0 0 123456
// backlog TMGN_Brightness 5; TMGN_Test
// test print offset
// backlog TMGN_Clear; TMGN_Print 0 0 1
// backlog TMGN_Clear; TMGN_Print 1 0 2
// backlog TMGN_Clear; TMGN_Print 2 0 3
// backlog TMGN_Clear; TMGN_Print 3 0 4
// backlog TMGN_Clear; TMGN_Print 4 0 5
// backlog TMGN_Clear; TMGN_Print 5 0 6
/*

startDriver TM1637
TMGN_Print 0 0 123456

again:
addChannel 10 1 0 15 1
TMGN_Brightness $CH10
delay_s 0.1
goto again

*/
/*

startDriver TM1637
TMGN_Print 0 0 123456

setChannel 10 100000
again:
addChannel 10 1
TMGN_Print 0 0 $CH10
delay_s 0.25
goto again


*/
/*

//startDriver GN6932
startDriver TM1637
TMGN_Clear

setChannel 10 10
again:
addChannel 10 1 10 99 1
TMGN_Print 0 2 $CH10
delay_s 0.25
goto again


*/
/*

startDriver TM1637
TMGN_Clear

setChannel 10 12
setChannel 11 123
again:
addChannel 10 1
addChannel 11 3
TMGN_Print 0 2 $CH10
TMGN_Print 3 3 $CH11
delay_s 0.5
goto again
*/

/*

startDriver TM1637
startDriver NTP
TMGN_Clear

again:
TMGN_Print 0 2 $minute 1
TMGN_Print 2 2 $second 1
delay_s 1
goto again
*/
/*
//startDriver GN6932
startDriver TM1637
startDriver NTP
TMGN_Clear

again:
TMGN_Print 4 2 $second 1
TMGN_Print 2 2 $minute 1
TMGN_Print 0 2 $hour 1
delay_s 0.1
goto again

*/
void TM_GN_Display_SharedInit() {
	int i;

	if (PIN_FindPinIndexForRole(IOR_TM1637_CLK, -1) != -1) {
		g_i2c.pin_clk = PIN_FindPinIndexForRole(IOR_TM1637_CLK, 16);
		g_i2c.pin_data = PIN_FindPinIndexForRole(IOR_TM1637_DIO, 14);
		g_i2c.pin_stb = -1;
		addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "TM/GN driver: using I2C mode (TM1637)");

		HAL_PIN_Setup_Output(g_i2c.pin_clk);
		HAL_PIN_Setup_Output(g_i2c.pin_data);
		HAL_PIN_SetOutputValue(g_i2c.pin_clk, true);
		HAL_PIN_SetOutputValue(g_i2c.pin_data, true);

		g_totalDigits = 6; 
	}
	else {
		g_i2c.pin_clk = 17;// PIN_FindPinIndexForRole(IOR_TM1637_CLK, 16);
		g_i2c.pin_data = 15;// PIN_FindPinIndexForRole(IOR_TM1637_DIO, 14);
		g_i2c.pin_stb = 28;// PIN_FindPinIndexForRole(IOR_TM1637_DIO, 14);
		addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "TM/GN driver: using SPI mode (GN6932)");

		// GN6932 has no remap
		for (i = 0; i < sizeof(g_remap); i++) {
			g_remap[i] = i;
		}

		HAL_PIN_Setup_Output(g_i2c.pin_clk);
		HAL_PIN_Setup_Output(g_i2c.pin_stb);
		HAL_PIN_Setup_Output(g_i2c.pin_data);
		HAL_PIN_SetOutputValue(g_i2c.pin_clk, true);
		HAL_PIN_SetOutputValue(g_i2c.pin_stb, true);
		HAL_PIN_SetOutputValue(g_i2c.pin_data, true);

		g_totalDigits = 16;
	}
	
	usleep(100);

	if (tmgn_buffer == 0) {
		tmgn_buffer = (byte*)malloc(g_totalDigits);
		memset(tmgn_buffer, 0, g_totalDigits);
	}

	TM1637_SetBrightness(0x0f, true);

	//cmddetail:{"name":"TMGN_SetBit","args":"[CharIndex] [BitIndex] [BitValue]",
	//cmddetail:"descr":"Set given bit of given digit to 1 or 0.",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tm1637.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TMGN_SetBit", CMD_TM1637_SetBit, NULL);
	//cmddetail:{"name":"TMGN_Clear","args":"",
	//cmddetail:"descr":"This clears the TM1637/GN932/etc display",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tm_gn_display_shared.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TMGN_Clear", CMD_TM1637_Clear, NULL);
	//cmddetail:{"name":"TMGN_Char","args":"[CharIndex] [CharCode]",
	//cmddetail:"descr":"This allows you to set binary code for given char, valid chars range is 0 to 15, because this is 7-seg display",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tm1637.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TMGN_Char", CMD_TM1637_Char, NULL);
	//cmddetail:{"name":"TMGN_Print","args":"[StartOfs] [MaxLenOr0] [StringText] [optionalBClampWithZeroesForClock]",
	//cmddetail:"descr":"This allows you to print string on TM1637/GN932/etc display, it supports variables expansion",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tm_gn_display_shared.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TMGN_Print", CMD_TM1637_Print, NULL);
	//cmddetail:{"name":"TMGN_Test","args":"CMD_TM1637_Test",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tm_gn_display_shared.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TMGN_Test", CMD_TM1637_Test, NULL);
	//cmddetail:{"name":"TMGN_Brightness","args":"[Brigthness0to7][bOn]",
	//cmddetail:"descr":"This allows you to change brightness and state of  TM1637/GN932/etc display",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tm_gn_display_shared.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TMGN_Brightness", CMD_TM1637_Brightness, NULL);
	//cmddetail:{"name":"TMGN_Map","args":"[Map0][Map1, etc]",
	//cmddetail:"descr":"This allows you to remap characters order for TM1637/GN932/etc. My TM1637 module from Aliexpress has a strange characters order.",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tm_gn_display_shared.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TMGN_Map", CMD_TM1637_Map, NULL);
	//cmddetail:{"name":"TM1650_Test","args":"CMD_TM1650_Test",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tm1637.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TM1650_Test", CMD_TM1650_Test, NULL);
}
