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
static const uint8_t charmap[] = {  //  TODO PROGMEM = slower?

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

void HT16K33_writePos(uint8_t pos, uint8_t mask)
{
	Soft_I2C_Start(&g_softI2C, g_addr);
	Soft_I2C_WriteByte(&g_softI2C, pos*2);
	Soft_I2C_WriteByte(&g_softI2C, mask);
	Soft_I2C_Stop(&g_softI2C);
}
void HT16K33_display(uint8_t *array, uint8_t point)
{
	//  debug to Serial
	//  dumpSerial(array, point);
	//  dumpSerial();

	HT16K33_writePos(0, charmap[array[0]]);
	HT16K33_writePos(1, charmap[array[1]]);
	HT16K33_writePos(3, charmap[array[2]]);
	HT16K33_writePos(4, charmap[array[3]]);
}
void HT16K33_displayOff()
{
	HT16K33_WriteCmd(HT16K33_DISPLAYOFF);
	HT16K33_WriteCmd(HT16K33_STANDBY);
}

void HT16K33_displayClear()
{
	uint8_t x[4] = { HT16K33_SPACE, HT16K33_SPACE, HT16K33_SPACE, HT16K33_SPACE };
	HT16K33_display(x,4);
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
	uint8_t x[4] = { HT16K33_0, HT16K33_1, HT16K33_2, HT16K33_SPACE };
	rtos_delay_milliseconds(25);
	HT16K33_display(x, 4);
	rtos_delay_milliseconds(25);
	HT16K33_brightness(8);

	return CMD_RES_OK;
}
// backlog startDriver HT16K33; HT16K33_Test
void HT16K33_Init() {

	CMD_RegisterCommand("HT16K33_Test", HT16K33_Test, NULL);
}


