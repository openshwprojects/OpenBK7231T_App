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

static uint8_t tabcolor;
static volatile uint32_t *dataport, *clkport, *csport, *rsport;
static uint32_t _cs, _rs, _rst, _sid, _sclk,
datapinmask, clkpinmask, cspinmask, rspinmask,
colstart, rowstart; // some displays need this changed

static int16_t WIDTH, HEIGHT;   // This is the 'raw' display w/h - never changes
static int16_t _width, _height; // Display w/h as modified by current rotation
static uint8_t rotation;


static commandResult_t CMD_ST7735_Test(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);


	return CMD_RES_OK;
}

void ST7735_Init() {
	CMD_RegisterCommand("ST7735_Test", CMD_ST7735_Test, NULL);


}

