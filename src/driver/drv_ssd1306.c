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


static softI2C_t g_softI2C;

static int ssd1306_addr = 0x3C;

#define SSD1306_CMD  0x00
#define SSD1306_DATA 0x40


static void SSD1306_WriteCmd(byte c) {
	Soft_I2C_Start(&g_softI2C, ssd1306_addr << 1);
	Soft_I2C_WriteByte(&g_softI2C, SSD1306_CMD);
	Soft_I2C_WriteByte(&g_softI2C, c);
	Soft_I2C_Stop(&g_softI2C);
}

static void SSD1306_WriteData(byte d) {
	Soft_I2C_Start(&g_softI2C, ssd1306_addr << 1);
	Soft_I2C_WriteByte(&g_softI2C, SSD1306_DATA);
	Soft_I2C_WriteByte(&g_softI2C, d);
	Soft_I2C_Stop(&g_softI2C);
}

void SSD1306_Fill(byte v) {
	int i;
	SSD1306_WriteCmd(0x21); // Set column range
	SSD1306_WriteCmd(0x00);
	SSD1306_WriteCmd(0x7F);
	SSD1306_WriteCmd(0x22); // Set page range (for 128x32: 0-3)
	SSD1306_WriteCmd(0x00);
	SSD1306_WriteCmd(0x03);
	for (i = 0; i < 512; i++)
		SSD1306_WriteData(v);
}
void SSD1306_SetOn(bool b) {
	if (b) {
		SSD1306_WriteCmd(0xAF);
	}
	else {
		SSD1306_WriteCmd(0xAE);
	}
}
void SSD1306_SetPos(byte x, byte page) {
	SSD1306_WriteCmd(0xB0 | page);
	SSD1306_WriteCmd(0x00 | (x & 0x0F));
	SSD1306_WriteCmd(0x10 | (x >> 4));
}

void SSD1306_DrawRect(byte x, byte y, byte w, byte h, byte fill) {
	byte px, py;
	byte page_start = y >> 3;
	byte page_end = (y + h - 1) >> 3;

	for (py = page_start; py <= page_end; py++) {
		SSD1306_SetPos(x, py);
		for (px = 0; px < w; px++) {

			byte mask = 0x00;

			if (fill) {
				mask = 0xFF;
			}
			else {
				if (py == page_start || py == page_end)
					mask = 0xFF;
				else if (px == 0 || px == w - 1)
					mask = 0xFF;
			}

			SSD1306_WriteData(mask);
		}
	}
}
// Standard 5x7 ASCII font
static const byte font5x7[][5] = {
	{0x00, 0x00, 0x00, 0x00, 0x00}, // space
	{0x00, 0x00, 0x5F, 0x00, 0x00}, // !
	{0x00, 0x07, 0x00, 0x07, 0x00}, // "
	{0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
	{0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
	{0x23, 0x13, 0x08, 0x64, 0x62}, // %
	{0x36, 0x49, 0x55, 0x22, 0x50}, // &
	{0x00, 0x05, 0x03, 0x00, 0x00}, // '
	{0x00, 0x1C, 0x22, 0x41, 0x00}, // (
	{0x00, 0x41, 0x22, 0x1C, 0x00}, // )
	{0x14, 0x08, 0x3E, 0x08, 0x14}, // *
	{0x08, 0x08, 0x3E, 0x08, 0x08}, // +
	{0x00, 0x50, 0x60, 0x00, 0x00}, // ,
	{0x08, 0x08, 0x08, 0x08, 0x08}, // -
	{0x00, 0x60, 0x60, 0x00, 0x00}, // .
	{0x20, 0x10, 0x08, 0x04, 0x02}, // /
	{0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
	{0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
	{0x42, 0x61, 0x51, 0x49, 0x46}, // 2
	{0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
	{0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
	{0x27, 0x45, 0x45, 0x45, 0x39}, // 5
	{0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
	{0x01, 0x71, 0x09, 0x05, 0x03}, // 7
	{0x36, 0x49, 0x49, 0x49, 0x36}, // 8
	{0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
	{0x00, 0x36, 0x36, 0x00, 0x00}, // :
	{0x00, 0x56, 0x36, 0x00, 0x00}, // ;
	{0x08, 0x14, 0x22, 0x41, 0x00}, // <
	{0x14, 0x14, 0x14, 0x14, 0x14}, // =
	{0x00, 0x41, 0x22, 0x14, 0x08}, // >
	{0x02, 0x01, 0x51, 0x09, 0x06}, // ?
	{0x32, 0x49, 0x79, 0x41, 0x3E}, // @
	{0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
	{0x7F, 0x49, 0x49, 0x49, 0x36}, // B
	{0x3E, 0x41, 0x41, 0x41, 0x22}, // C
	{0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
	{0x7F, 0x49, 0x49, 0x49, 0x41}, // E
	{0x7F, 0x09, 0x09, 0x09, 0x01}, // F
	{0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
	{0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
	{0x00, 0x41, 0x7F, 0x41, 0x00}, // I
	{0x20, 0x40, 0x41, 0x3F, 0x01}, // J
	{0x7F, 0x08, 0x14, 0x22, 0x41}, // K
	{0x7F, 0x40, 0x40, 0x40, 0x40}, // L
	{0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
	{0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
	{0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
	{0x7F, 0x09, 0x09, 0x09, 0x06}, // P
	{0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
	{0x7F, 0x09, 0x19, 0x29, 0x46}, // R
	{0x46, 0x49, 0x49, 0x49, 0x31}, // S
	{0x01, 0x01, 0x7F, 0x01, 0x01}, // T
	{0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
	{0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
	{0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
	{0x63, 0x14, 0x08, 0x14, 0x63}, // X
	{0x07, 0x08, 0x70, 0x08, 0x07}, // Y
	{0x61, 0x51, 0x49, 0x45, 0x43}, // Z
};
void SSD1306_WriteChar(char c) {
	// Convert lowercase to uppercase
	if (c >= 'a' && c <= 'z') {
		c = c - 'a' + 'A';
	}
	if (c < 32 || c > 90) c = 32; // Basic bounds check, map unknown to space
	c -= 32; // Offset to match array index

	Soft_I2C_Start(&g_softI2C, ssd1306_addr << 1);
	Soft_I2C_WriteByte(&g_softI2C, SSD1306_DATA);

	for (int i = 0; i < 5; i++) {
		Soft_I2C_WriteByte(&g_softI2C, font5x7[(uint8_t)c][i]);
	}
	Soft_I2C_WriteByte(&g_softI2C, 0x00); // 1px spacing between chars
	Soft_I2C_Stop(&g_softI2C);
}

void SSD1306_String(const char *str) {
	while (*str) {
		SSD1306_WriteChar(*str++);
	}
}

//int x = 1000;
void SSD1306_OnEverySecond() {
	//SSD1306_SetOn(true);
	//x++;
	//char tmp[8];
	//sprintf(tmp, "%i", x);
	//SSD1306_SetPos(0, 0);
	//SSD1306_String(tmp);
}


commandResult_t SSD1306_Cmd_Print(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	const char *s = Tokenizer_GetArg(0);
	SSD1306_String(s);
	return CMD_RES_OK;
}
commandResult_t SSD1306_Cmd_GoTo(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int x = Tokenizer_GetArgInteger(0);
	int y = Tokenizer_GetArgInteger(1);
	SSD1306_SetPos(x, y);
	return CMD_RES_OK;
}
commandResult_t SSD1306_Cmd_On(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int bOn = Tokenizer_GetArgInteger(0);
	SSD1306_SetOn(bOn);
	return CMD_RES_OK;
}
commandResult_t SSD1306_Cmd_Clear(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int val = Tokenizer_GetArgInteger(0);
	SSD1306_Fill(val);
	return CMD_RES_OK;
}

commandResult_t SSD1306_Cmd_Rect(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 4)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int x = Tokenizer_GetArgInteger(0);
	int y = Tokenizer_GetArgInteger(1);
	int w = Tokenizer_GetArgInteger(2);
	int h = Tokenizer_GetArgInteger(3);
	int fill = Tokenizer_GetArgIntegerDefault(4, 0xff);
	SSD1306_DrawRect(x, y, w, h, fill);
	return CMD_RES_OK;
}
// startDriver SSD1306 16 20 0x3C
// backlog stopdriver SSD1306 ; startDriver SSD1306 16 20 0x3C
/*




backlog stopdriver SSD1306 ; startDriver SSD1306 16 20 0x3C
ssd1306_clear 0
ssd1306_on 1
setChannel 12 0
again:
delay_s 1
ssd1306_goto 0 0
ssd1306_print "Hello "
ssd1306_print  $CH12
addChannel 12 1
goto again


*/
void SSD1306_DRV_Init() {

	g_softI2C.pin_clk = Tokenizer_GetPin(1, 16);
	g_softI2C.pin_data = Tokenizer_GetPin(2, 20);
	ssd1306_addr = Tokenizer_GetArgIntegerDefault(3, 0x3C);

	CMD_RegisterCommand("ssd1306_clear", SSD1306_Cmd_Clear, NULL);
	CMD_RegisterCommand("ssd1306_on", SSD1306_Cmd_On, NULL);
	CMD_RegisterCommand("ssd1306_print", SSD1306_Cmd_Print, NULL);
	CMD_RegisterCommand("ssd1306_rect", SSD1306_Cmd_Rect, NULL);
	CMD_RegisterCommand("ssd1306_goto", SSD1306_Cmd_GoTo, NULL);

	Soft_I2C_PreInit(&g_softI2C);

	SSD1306_WriteCmd(0xAE);
	SSD1306_WriteCmd(0xD5);
	SSD1306_WriteCmd(0x80);
	SSD1306_WriteCmd(0xA8);
	SSD1306_WriteCmd(0x1F);
	SSD1306_WriteCmd(0xD3);
	SSD1306_WriteCmd(0x00);
	SSD1306_WriteCmd(0x40);
	SSD1306_WriteCmd(0x8D);
	SSD1306_WriteCmd(0x14);
	SSD1306_WriteCmd(0x20);
	SSD1306_WriteCmd(0x00);
	SSD1306_WriteCmd(0xA1);
	SSD1306_WriteCmd(0xC8);
	SSD1306_WriteCmd(0xDA);
	SSD1306_WriteCmd(0x02);
	SSD1306_WriteCmd(0x81);
	SSD1306_WriteCmd(0x8F);
	SSD1306_WriteCmd(0xD9);
	SSD1306_WriteCmd(0xF1);
	SSD1306_WriteCmd(0xDB);
	SSD1306_WriteCmd(0x40);
	SSD1306_WriteCmd(0xA4);
	SSD1306_WriteCmd(0xA6);
	SSD1306_WriteCmd(0xAF);

	SSD1306_Fill(0x00);
}