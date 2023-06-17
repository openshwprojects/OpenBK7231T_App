// NOTE: based on https://github.com/RobTillaart/HT16K33.git MIT code
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
static byte g_addr = 0xE0;


//  Characters
#define HT16K33_0                0
#define HT16K33_1                1
#define HT16K33_2                2
#define HT16K33_3                3
#define HT16K33_4                4
#define HT16K33_5                5
#define HT16K33_6                6
#define HT16K33_7                7
#define HT16K33_8                8
#define HT16K33_9                9
#define HT16K33_A                10
#define HT16K33_B                11
#define HT16K33_C                12
#define HT16K33_D                13
#define HT16K33_E                14
#define HT16K33_F                15
#define HT16K33_SPACE            16
#define HT16K33_MINUS            17
#define HT16K33_TOP_C            18     //  c
#define HT16K33_DEGREE           19     //  °
#define HT16K33_NONE             99


//  Commands
#define HT16K33_ON              0x21  //  0 = off   1 = on
#define HT16K33_STANDBY         0x20  //  bit xxxxxxx0

//  bit pattern 1000 0xxy
//  y    =  display on / off
//  xx   =  00=off     01=2Hz     10 = 1Hz     11 = 0.5Hz
#define HT16K33_DISPLAYON       0x81
#define HT16K33_DISPLAYOFF      0x80
#define HT16K33_BLINKON0_5HZ    0x87
#define HT16K33_BLINKON1HZ      0x85
#define HT16K33_BLINKON2HZ      0x83
#define HT16K33_BLINKOFF        0x81

//  bit pattern 1110 xxxx
//  xxxx    =  0000 .. 1111 (0 - F)
#define HT16K33_BRIGHTNESS      0xE0

uint8_t _bright = 0x0F;

void HT16K33_WriteCmd(byte b) {
	Soft_I2C_Start(&g_softI2C, g_addr);
	Soft_I2C_WriteByte(&g_softI2C, b);
	Soft_I2C_Stop(&g_softI2C);
}

void HT16K33_brightness(uint8_t value)
{
	_bright = value;
	if (_bright > 0x0F)
		_bright = 0x0F;
	HT16K33_WriteCmd(HT16K33_BRIGHTNESS | _bright);
}
void HT16K33_displayOn()
{
	HT16K33_WriteCmd(HT16K33_ON);
	HT16K33_WriteCmd(HT16K33_DISPLAYON);
	HT16K33_brightness(_bright);
}

//
//  HEX codes 7 segment
//
//      01
//  20      02
//      40
//  10      04
//      08
//
static const uint8_t charMap7seg[] = {  //  TODO PROGMEM = slower?

  0x3F,   //  0
  0x06,   //  1
  0x5B,   //  2
  0x4F,   //  3
  0x66,   //  4
  0x6D,   //  5
  0x7D,   //  6
  0x07,   //  7
  0x7F,   //  8
  0x6F,   //  9
  0x77,   //  A
  0x7C,   //  B
  0x39,   //  C
  0x5E,   //  D
  0x79,   //  E
  0x71,   //  F
  0x00,   //  space
  0x40,   //  minus
  0x61,   //  TOP_C
  0x63,   //  degree °
};

unsigned short convert16seg(char c)
{
	unsigned short c2;
	switch (c)
	{
	case ' ': c2 = 0x0000; break;
	case '.': c2 = 0x4000; break;
	case '+': c2 = 0x12C0; break;
	case '-': c2 = 0x00C0; break;
	case '*': c2 = 0x3FC0; break;
	case '/': c2 = 0x0C00; break;
	case '=': c2 = 0x00C8; break;
	case '>': c2 = 0x0900; break;
	case '<': c2 = 0x2400; break;
	case '[': c2 = 0x0039; break;
	case ']': c2 = 0x000F; break;
		//---------------------------
	case '0': c2 = 0x0C3F; break;
	case '1': c2 = 0x0406; break;
	case '2': c2 = 0x00DB; break;
	case '3': c2 = 0x008F; break;
	case '4': c2 = 0x00E6; break;
	case '5': c2 = 0x00ED; break;
	case '6': c2 = 0x00FD; break;
	case '7': c2 = 0x0007; break;
	case '8': c2 = 0x00FF; break;
	case '9': c2 = 0x00EF; break;
		//---------------------------
	case 'A': c2 = 0x00F7; break;
	case 'B': c2 = 0x128F; break;
	case 'C': c2 = 0x0039; break;
	case 'D': c2 = 0x120F; break;
	case 'E': c2 = 0x00F9; break;
	case 'F': c2 = 0x00F1; break;
	case 'G': c2 = 0x00BD; break;
	case 'H': c2 = 0x00F6; break;
	case 'I': c2 = 0x1209; break;
	case 'J': c2 = 0x001E; break;
	case 'K': c2 = 0x2470; break;
	case 'L': c2 = 0x0038; break;
	case 'M': c2 = 0x0536; break;
	case 'N': c2 = 0x2136; break;
	case 'O': c2 = 0x003F; break;
	case 'P': c2 = 0x00F3; break;
	case 'Q': c2 = 0x203F; break;
	case 'R': c2 = 0x20F3; break;
	case 'S': c2 = 0x018D; break;
	case 'T': c2 = 0x1201; break;
	case 'U': c2 = 0x003E; break;
	case 'V': c2 = 0x0C30; break;
	case 'W': c2 = 0x2836; break;
	case 'X': c2 = 0x2D00; break;
	case 'Y': c2 = 0x1500; break;
	case 'Z': c2 = 0x0C09;
	}
	return c2;
}


void HT16K33_writePos(uint8_t pos, unsigned short mask)
{
	Soft_I2C_Start(&g_softI2C, g_addr);
	Soft_I2C_WriteByte(&g_softI2C, pos*2);
	Soft_I2C_WriteByte(&g_softI2C, mask & 0x00FF);
	Soft_I2C_WriteByte(&g_softI2C, (mask & 0xFF00) >> 8);
	Soft_I2C_Stop(&g_softI2C);
}
void HT16K33_display(const char *txt, uint8_t point)
{
	int i = 0;
	unsigned short code;

	while (*txt) {
		code = convert16seg(*txt);

		HT16K33_writePos(i,code);
		txt++;
		i++;
	}
}
void HT16K33_displayOff()
{
	HT16K33_WriteCmd(HT16K33_DISPLAYOFF);
	HT16K33_WriteCmd(HT16K33_STANDBY);
}

void HT16K33_displayClear()
{
	HT16K33_display("    ",4);
	//HT16K33_displayColon(false);
}
commandResult_t HT16K33_Test(const void* context, const char* cmd, const char* args, int cmdFlags) {


	g_softI2C.pin_clk = 26;
	g_softI2C.pin_data = 24;


	Soft_I2C_PreInit(&g_softI2C);

	rtos_delay_milliseconds(25);

	HT16K33_displayOn();
	rtos_delay_milliseconds(25);
	//HT16K33_displayClear();
	rtos_delay_milliseconds(25);
	HT16K33_display("1234", 4);
	rtos_delay_milliseconds(25);
	HT16K33_brightness(8);

	return CMD_RES_OK;
}
commandResult_t HT16K33_Print(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	const char *s = Tokenizer_GetArg(0);

	HT16K33_display(s, 4);

	return CMD_RES_OK;
}

commandResult_t HT16K33_Char(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int pos, val;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 2) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	pos = Tokenizer_GetArgInteger(0);
	val = Tokenizer_GetArgInteger(1);

	HT16K33_writePos(pos, charMap7seg[val]);

	return CMD_RES_OK;
}
commandResult_t HT16K33_Raw(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int pos, val;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 2) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	pos = Tokenizer_GetArgInteger(0);
	val = Tokenizer_GetArgInteger(1);

	HT16K33_writePos(pos, val);

	return CMD_RES_OK;
}

// backlog startDriver HT16K33; HT16K33_Test
void HT16K33_Init() {

	CMD_RegisterCommand("HT16K33_Test", HT16K33_Test, NULL);
	CMD_RegisterCommand("HT16K33_Char", HT16K33_Char, NULL);
	CMD_RegisterCommand("HT16K33_Raw", HT16K33_Raw, NULL);
	CMD_RegisterCommand("HT16K33_Print", HT16K33_Print, NULL);
}


