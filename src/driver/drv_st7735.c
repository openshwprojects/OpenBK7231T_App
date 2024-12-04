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


#define pgm_read_byte(x) x
// Companion code to the above tables.  Reads and issues
// a series of LCD commands stored in PROGMEM byte array.
	void commandList(const uint8_t *addr)
	{

		uint8_t numCommands, numArgs;
		uint16_t ms;

		numCommands = pgm_read_byte(addr++); // Number of commands to follow
		while (numCommands--)
		{                                      // For each command...
			writecommand(pgm_read_byte(addr++)); //   Read, issue command
			numArgs = pgm_read_byte(addr++);     //   Number of args to follow
			ms = numArgs & DELAY;                //   If hibit set, delay follows args
			numArgs &= ~DELAY;                   //   Mask out delay bit
			while (numArgs--)
			{                                   //   For each argument...
				writedata(pgm_read_byte(addr++)); //     Read, issue argument
			}

			if (ms)
			{
				ms = pgm_read_byte(addr++); // Read post-command delay time (ms)
				if (ms == 255)
					ms = 500; // If 255, delay for 500 ms
				delay_ms(ms);
			}
		}
	}

	// Initialization code common to both 'B' and 'R' type displays
	void commonInit(const uint8_t *cmdList)
	{
		colstart = rowstart = 0; // May be overridden in init func

	//	pinMode(_rs, OUTPUT);
	//	pinMode(_cs, OUTPUT);
	//	csport = portOutputRegister(digitalPinToPort(_cs));
	//	rsport = portOutputRegister(digitalPinToPort(_rs));
		//cspinmask = digitalPinToBitMask(_cs);
		//rspinmask = digitalPinToBitMask(_rs);

		//pinMode(_sclk, OUTPUT);
		//pinMode(_sid, OUTPUT);
	//	clkport = portOutputRegister(digitalPinToPort(_sclk));
	//	dataport = portOutputRegister(digitalPinToPort(_sid));
	//	clkpinmask = digitalPinToBitMask(_sclk);
		//datapinmask = digitalPinToBitMask(_sid);
		*clkport &= ~clkpinmask;
		*dataport &= ~datapinmask;

		// toggle RST low to reset; CS low so it'll listen to us
		*csport &= ~cspinmask;
		if (_rst)
		{
			//pinMode(_rst, OUTPUT);
			//digitalWrite(_rst, HIGH);
			delay_ms(500);
			//digitalWrite(_rst, LOW);
			delay_ms(500);
			//digitalWrite(_rst, HIGH);
			delay_ms(500);
		}

		if (cmdList)
			commandList(cmdList);
	}


	// Initialization for ST7735B screens
	void ST7735_initB(void)
	{
		commonInit(Bcmd);
	}

	// Initialization for ST7735R screens (green or red tabs)
	void ST7735_initR(uint8_t options)
	{
		commonInit(Rcmd1);
		if (options == Adafruit_initR_GREENTAB)
		{
			commandList(Rcmd2green);
			colstart = 2;
			rowstart = 1;
		}
		else if (options == Adafruit_initR_144GREENTAB)
		{
			_height = ST7735_TFTHEIGHT_144;
			commandList(Rcmd2green144);
			colstart = 2;
			rowstart = 3;
		}
		else
		{
			// colstart, rowstart left at default '0' values
			commandList(Rcmd2red);
		}
		commandList(Rcmd3);

		// if black, change MADCTL color filter
		if (options == Adafruit_initR_BLACKTAB)
		{
			writecommand(ST7735_MADCTL);
			writedata(0xC0);
		}

		tabcolor = options;
	}


	void setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1,
		uint8_t y1)
	{

		writecommand(ST7735_CASET); // Column addr set
		writedata(0x00);
		writedata(x0 + colstart); // XSTART
		writedata(0x00);
		writedata(x1 + colstart); // XEND

		writecommand(ST7735_RASET); // Row addr set
		writedata(0x00);
		writedata(y0 + rowstart); // YSTART
		writedata(0x00);
		writedata(y1 + rowstart); // YEND

		writecommand(ST7735_RAMWR); // write to RAM
	}

	void pushColor(uint16_t color)
	{
		*rsport |= rspinmask;
		*csport &= ~cspinmask;

		spiwrite(color >> 8);
		spiwrite(color);

		*csport |= cspinmask;
	}


	void drawPixel(int16_t x, int16_t y, uint16_t color)
	{

		if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height))
			return;

		setAddrWindow(x, y, x + 1, y + 1);

		*rsport |= rspinmask;
		*csport &= ~cspinmask;

		spiwrite(color >> 8);
		spiwrite(color);

		*csport |= cspinmask;
	}

	void drawFastVLine(int16_t x, int16_t y, int16_t h,
		uint16_t color)
	{

		// Rudimentary clipping
		if ((x >= _width) || (y >= _height))
			return;
		if ((y + h - 1) >= _height)
			h = _height - y;
		setAddrWindow(x, y, x, y + h - 1);

		uint8_t hi = color >> 8, lo = color;

		*rsport |= rspinmask;
		*csport &= ~cspinmask;
		while (h--)
		{
			spiwrite(hi);
			spiwrite(lo);
		}
		*csport |= cspinmask;
	}

	void drawFastHLine(int16_t x, int16_t y, int16_t w,
		uint16_t color)
	{

		// Rudimentary clipping
		if ((x >= _width) || (y >= _height))
			return;
		if ((x + w - 1) >= _width)
			w = _width - x;
		setAddrWindow(x, y, x + w - 1, y);

		uint8_t hi = color >> 8, lo = color;

		*rsport |= rspinmask;
		*csport &= ~cspinmask;
		while (w--)
		{
			spiwrite(hi);
			spiwrite(lo);
		}
		*csport |= cspinmask;
	}


	// fill a rectangle
	void ST7735_fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
		uint16_t color)
	{

		// rudimentary clipping (drawChar w/big text requires this)
		if ((x >= _width) || (y >= _height))
			return;
		if ((x + w - 1) >= _width)
			w = _width - x;
		if ((y + h - 1) >= _height)
			h = _height - y;

		setAddrWindow(x, y, x + w - 1, y + h - 1);

		uint8_t hi = color >> 8, lo = color;

		*rsport |= rspinmask;
		*csport &= ~cspinmask;
		for (y = h; y > 0; y--)
		{
			for (x = w; x > 0; x--)
			{
				spiwrite(hi);
				spiwrite(lo);
			}
		}

		*csport |= cspinmask;
	}

	void ST7735_fillScreen(uint16_t color)
	{
		ST7735_fillRect(0, 0, _width, _height, color);
	}
	// Pass 8-bit (each) R,G,B, get back 16-bit packed color
	uint16_t Color565(uint8_t r, uint8_t g, uint8_t b)
	{
		return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
	}

#define MADCTL_MY 0x80
#define MADCTL_MX 0x40
#define MADCTL_MV 0x20
#define MADCTL_ML 0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH 0x04

	void setRotation(uint8_t m)
	{

		writecommand(ST7735_MADCTL);
		rotation = m % 4; // can't be higher than 3
		switch (rotation)
		{
		case 0:
			if (tabcolor == Adafruit_initR_BLACKTAB)
			{
				writedata(MADCTL_MX | MADCTL_MY | MADCTL_RGB);
			}
			else
			{
				writedata(MADCTL_MX | MADCTL_MY | MADCTL_BGR);
			}
			_width = ST7735_TFTWIDTH;

			if (tabcolor == Adafruit_initR_144GREENTAB)
				_height = ST7735_TFTHEIGHT_144;
			else
				_height = ST7735_TFTHEIGHT_18;

			break;
		case 1:
			if (tabcolor == Adafruit_initR_BLACKTAB)
			{
				writedata(MADCTL_MY | MADCTL_MV | MADCTL_RGB);
			}
			else
			{
				writedata(MADCTL_MY | MADCTL_MV | MADCTL_BGR);
			}

			if (tabcolor == Adafruit_initR_144GREENTAB)
				_width = ST7735_TFTHEIGHT_144;
			else
				_width = ST7735_TFTHEIGHT_18;

			_height = ST7735_TFTWIDTH;
			break;
		case 2:
			if (tabcolor == Adafruit_initR_BLACKTAB)
			{
				writedata(MADCTL_RGB);
			}
			else
			{
				writedata(MADCTL_BGR);
			}
			_width = ST7735_TFTWIDTH;
			if (tabcolor == Adafruit_initR_144GREENTAB)
				_height = ST7735_TFTHEIGHT_144;
			else
				_height = ST7735_TFTHEIGHT_18;

			break;
		case 3:
			if (tabcolor == Adafruit_initR_BLACKTAB)
			{
				writedata(MADCTL_MX | MADCTL_MV | MADCTL_RGB);
			}
			else
			{
				writedata(MADCTL_MX | MADCTL_MV | MADCTL_BGR);
			}
			if (tabcolor == Adafruit_initR_144GREENTAB)
				_width = ST7735_TFTHEIGHT_144;
			else
				_width = ST7735_TFTHEIGHT_18;

			_height = ST7735_TFTWIDTH;
			break;
		}
	}

	void invertDisplay(int i)
	{
		writecommand(i ? ST7735_INVON : ST7735_INVOFF);
	}

	// image must be uint16_t[h][w]
	void fillImage(void *image, int x, int y, int w, int h)
	{
		setAddrWindow(y, x, y + h - 1, x + w - 1);
		*rsport |= rspinmask;
		*csport &= ~cspinmask;
		uint16_t *p = (uint16_t *)image;
		for (int j = 0; j < w; ++j)
		{
			for (int i = 0; i < h; ++i)
			{
				spiwrite((uint8_t)(p[i * w + j] >> 8));
				spiwrite((uint8_t)p[i * w + j]);
			}
		}
		*csport |= cspinmask;
	}





static commandResult_t CMD_ST7735_Test(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);


	ST7735_fillScreen(ST7735_BLUE);
	return CMD_RES_OK;
}

static commandResult_t CMD_ST7735_Test2(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);


	ST7735_fillScreen(ST7735_RED);
	return CMD_RES_OK;
}

static commandResult_t Cmd_ST7735_Init(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

#define SD_CS 5
#define TFT_CS 2
#define TFT_DC 0
#define TFT_RST 5
#define MOSI_PIN 4
#define SCK_PIN 3


	ST7735_init(TFT_CS, TFT_DC, MOSI_PIN, SCK_PIN, TFT_RST);
	ST7735_initR(Adafruit_initR_BLACKTAB);
	ST7735_fillScreen(0x001F);

	return CMD_RES_OK;
}
void ST7735_Init() {
	CMD_RegisterCommand("ST7735_Test", CMD_ST7735_Test, NULL);
	CMD_RegisterCommand("ST7735_Test2", CMD_ST7735_Test2, NULL);
	CMD_RegisterCommand("ST7735_Init", Cmd_ST7735_Init, NULL);

}

