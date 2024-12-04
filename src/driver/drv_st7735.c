// This is based on ADAFRUIT Library:
/***************************************************
  This is a library for the Adafruit 1.8" SPI display.

This library works with the Adafruit 1.8" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/358
The 1.8" TFT shield
  ----> https://www.adafruit.com/product/802
The 1.44" TFT breakout
  ----> https://www.adafruit.com/product/2088
as well as Adafruit raw 1.8" TFT display
  ----> http://www.adafruit.com/products/618

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"

#include "drv_st7735.h"

static uint8_t tabcolor;
static volatile uint32_t *dataport, *clkport, *csport, *rsport;
static uint32_t _cs, _rs, _rst, _sid, _sclk,
datapinmask, clkpinmask, cspinmask, rspinmask,
colstart, rowstart; // some displays need this changed

static int16_t WIDTH, HEIGHT;   // This is the 'raw' display w/h - never changes
static int16_t _width, _height; // Display w/h as modified by current rotation
static uint8_t rotation;

uint16_t swapcolor(uint16_t x)
{
	return (x << 11) | (x & 0x07E0) | (x >> 11);
}

void spiwrite(uint8_t),
writecommand(uint8_t c),
writedata(uint8_t d),
commandList(const uint8_t *addr),
commonInit(const uint8_t *cmdList);

// Constructor when using software SPI.  All output pins are configurable.
void ST7735_init(int8_t cs, int8_t rs, int8_t sid, int8_t sclk, int8_t rst)
{
	WIDTH = ST7735_TFTWIDTH;
	HEIGHT = ST7735_TFTHEIGHT_18;
	_width = WIDTH;
	_height = HEIGHT;
	rotation = 0;
	_cs = cs;
	_rs = rs;
	_sid = sid;
	_sclk = sclk;
	_rst = rst;
}

inline void spiwrite(uint8_t c)
{

	// Fast SPI bitbang swiped from LPD8806 library
	for (uint8_t bit = 0x80; bit; bit >>= 1)
	{
		if (c & bit)
			*dataport |= datapinmask;
		else
			*dataport &= ~datapinmask;
		*clkport |= clkpinmask;
		*clkport &= ~clkpinmask;
	}
}

void writecommand(uint8_t c)
{
	*rsport &= ~rspinmask;
	*csport &= ~cspinmask;

	// Serial.print("C ");
	spiwrite(c);

	*csport |= cspinmask;
}

void writedata(uint8_t c)
{
	*rsport |= rspinmask;
	*csport &= ~cspinmask;

	// Serial.print("D ");
	spiwrite(c);

	*csport |= cspinmask;
}

#ifndef PROGMEM
#define PROGMEM
#endif

// Rather than a bazillion writecommand() and writedata() calls, screen
// initialization commands and arguments are organized in these tables
// stored in PROGMEM.  The table may look bulky, but that's mostly the
// formatting -- storage-wise this is hundreds of bytes more compact
// than the equivalent code.  Companion function follows.
#define DELAY 0x80
static const uint8_t PROGMEM
Bcmd[] = {                     // Initialization commands for 7735B screens
	18,                        // 18 commands in list:
	ST7735_SWRESET, DELAY,     //  1: Software reset, no args, w/delay
	50,                        //     50 ms delay
	ST7735_SLPOUT, DELAY,      //  2: Out of sleep mode, no args, w/delay
	255,                       //     255 = 500 ms delay
	ST7735_COLMOD, 1 + DELAY,  //  3: Set color mode, 1 arg + delay:
	0x05,                      //     16-bit color
	10,                        //     10 ms delay
	ST7735_FRMCTR1, 3 + DELAY, //  4: Frame rate control, 3 args + delay:
	0x00,                      //     fastest refresh
	0x06,                      //     6 lines front porch
	0x03,                      //     3 lines back porch
	10,                        //     10 ms delay
	ST7735_MADCTL, 1,          //  5: Memory access ctrl (directions), 1 arg:
	0x08,                      //     Row addr/col addr, bottom to top refresh
	ST7735_DISSET5, 2,         //  6: Display settings #5, 2 args, no delay:
	0x15,                      //     1 clk cycle nonoverlap, 2 cycle gate
							   //     rise, 3 cycle osc equalize
	0x02,                      //     Fix on VTL
	ST7735_INVCTR, 1,          //  7: Display inversion control, 1 arg:
	0x0,                       //     Line inversion
	ST7735_PWCTR1, 2 + DELAY,  //  8: Power control, 2 args + delay:
	0x02,                      //     GVDD = 4.7V
	0x70,                      //     1.0uA
	10,                        //     10 ms delay
	ST7735_PWCTR2, 1,          //  9: Power control, 1 arg, no delay:
	0x05,                      //     VGH = 14.7V, VGL = -7.35V
	ST7735_PWCTR3, 2,          // 10: Power control, 2 args, no delay:
	0x01,                      //     Opamp current small
	0x02,                      //     Boost frequency
	ST7735_VMCTR1, 2 + DELAY,  // 11: Power control, 2 args + delay:
	0x3C,                      //     VCOMH = 4V
	0x38,                      //     VCOML = -1.1V
	10,                        //     10 ms delay
	ST7735_PWCTR6, 2,          // 12: Power control, 2 args, no delay:
	0x11, 0x15,
	ST7735_GMCTRP1, 16,     // 13: Magical unicorn dust, 16 args, no delay:
	0x09, 0x16, 0x09, 0x20, //     (seriously though, not sure what
	0x21, 0x1B, 0x13, 0x19, //      these config values represent)
	0x17, 0x15, 0x1E, 0x2B,
	0x04, 0x05, 0x02, 0x0E,
	ST7735_GMCTRN1, 16 + DELAY, // 14: Sparkles and rainbows, 16 args + delay:
	0x0B, 0x14, 0x08, 0x1E,     //     (ditto)
	0x22, 0x1D, 0x18, 0x1E,
	0x1B, 0x1A, 0x24, 0x2B,
	0x06, 0x06, 0x02, 0x0F,
	10,                   //     10 ms delay
	ST7735_CASET, 4,      // 15: Column addr set, 4 args, no delay:
	0x00, 0x02,           //     XSTART = 2
	0x00, 0x81,           //     XEND = 129
	ST7735_RASET, 4,      // 16: Row addr set, 4 args, no delay:
	0x00, 0x02,           //     XSTART = 1
	0x00, 0x81,           //     XEND = 160
	ST7735_NORON, DELAY,  // 17: Normal display on, no args, w/delay
	10,                   //     10 ms delay
	ST7735_DISPON, DELAY, // 18: Main screen turn on, no args, w/delay
	255 },                 //     255 = 500 ms delay

	Rcmd1[] = {                // Init for 7735R, part 1 (red or green tab)
		15,                    // 15 commands in list:
		ST7735_SWRESET, DELAY, //  1: Software reset, 0 args, w/delay
		150,                   //     150 ms delay
		ST7735_SLPOUT, DELAY,  //  2: Out of sleep mode, 0 args, w/delay
		255,                   //     500 ms delay
		ST7735_FRMCTR1, 3,     //  3: Frame rate ctrl - normal mode, 3 args:
		0x01, 0x2C, 0x2D,      //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
		ST7735_FRMCTR2, 3,     //  4: Frame rate control - idle mode, 3 args:
		0x01, 0x2C, 0x2D,      //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
		ST7735_FRMCTR3, 6,     //  5: Frame rate ctrl - partial mode, 6 args:
		0x01, 0x2C, 0x2D,      //     Dot inversion mode
		0x01, 0x2C, 0x2D,      //     Line inversion mode
		ST7735_INVCTR, 1,      //  6: Display inversion ctrl, 1 arg, no delay:
		0x07,                  //     No inversion
		ST7735_PWCTR1, 3,      //  7: Power control, 3 args, no delay:
		0xA2,
		0x02,                         //     -4.6V
		0x84,                         //     AUTO mode
		ST7735_PWCTR2, 1,             //  8: Power control, 1 arg, no delay:
		0xC5,                         //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
		ST7735_PWCTR3, 2,             //  9: Power control, 2 args, no delay:
		0x0A,                         //     Opamp current small
		0x00,                         //     Boost frequency
		ST7735_PWCTR4, 2,             // 10: Power control, 2 args, no delay:
		0x8A,                         //     BCLK/2, Opamp current small & Medium low
		0x2A, ST7735_PWCTR5, 2,       // 11: Power control, 2 args, no delay:
		0x8A, 0xEE, ST7735_VMCTR1, 1, // 12: Power control, 1 arg, no delay:
		0x0E, ST7735_INVOFF, 0,       // 13: Don't invert display, no args, no delay
		ST7735_MADCTL, 1,             // 14: Memory access control (directions), 1 arg:
		0xC8,                         //     row addr/col addr, bottom to top refresh
		ST7735_COLMOD, 1,             // 15: set color mode, 1 arg, no delay:
		0x05 },                        //     16-bit color

		Rcmd2green[] = {        // Init for 7735R, part 2 (green tab only)
			2,                  //  2 commands in list:
			ST7735_CASET, 4,    //  1: Column addr set, 4 args, no delay:
			0x00, 0x02,         //     XSTART = 0
			0x00, 0x7F + 0x02,  //     XEND = 127
			ST7735_RASET, 4,    //  2: Row addr set, 4 args, no delay:
			0x00, 0x01,         //     XSTART = 0
			0x00, 0x9F + 0x01 }, //     XEND = 159
			Rcmd2red[] = {          // Init for 7735R, part 2 (red tab only)
				2,                  //  2 commands in list:
				ST7735_CASET, 4,    //  1: Column addr set, 4 args, no delay:
				0x00, 0x00,         //     XSTART = 0
				0x00, 0x7F,         //     XEND = 127
				ST7735_RASET, 4,    //  2: Row addr set, 4 args, no delay:
				0x00, 0x00,         //     XSTART = 0
				0x00, 0x9F },        //     XEND = 159

				Rcmd2green144[] = {  // Init for 7735R, part 2 (green 1.44 tab)
					2,               //  2 commands in list:
					ST7735_CASET, 4, //  1: Column addr set, 4 args, no delay:
					0x00, 0x00,      //     XSTART = 0
					0x00, 0x7F,      //     XEND = 127
					ST7735_RASET, 4, //  2: Row addr set, 4 args, no delay:
					0x00, 0x00,      //     XSTART = 0
					0x00, 0x7F },     //     XEND = 127

					Rcmd3[] = {                                                                                                              // Init for 7735R, part 3 (red or green tab)
						4,                                                                                                                   //  4 commands in list:
						ST7735_GMCTRP1, 16,                                                                                                  //  1: Magical unicorn dust, 16 args, no delay:
						0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10, ST7735_GMCTRN1, 16,  //  2: Sparkles and rainbows, 16 args, no delay:
						0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10, ST7735_NORON, DELAY, //  3: Normal display on, no args, w/delay
						10,                                                                                                                  //     10 ms delay
						ST7735_DISPON, DELAY,                                                                                                //  4: Main screen turn on, no args w/delay
						100 };                                                                                                                //     100 ms delay


static commandResult_t CMD_ST7735_Test(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);


	return CMD_RES_OK;
}

void ST7735_Init() {
	CMD_RegisterCommand("ST7735_Test", CMD_ST7735_Test, NULL);


}

