#if PLATFORM_LN882H

#include "../new_cfg.h"
#include "../new_common.h"
#include "../new_pins.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../mqtt/new_mqtt.h"
#include "./drv_sm16703P.h"

#include "hal/hal_gpio.h"
#include "hal/hal_dma.h"
#include "hal/hal_ws2811.h"
#include "../hal/ln882h/pin_mapping_ln882h.h"

int color_order = SM16703P_COLOR_ORDER_RGB; // default to RGB
// Number of pixels that can be addressed
uint32_t pixel_count = 0;

// LN882H specific defines
#define WS2811_DMA_CH DMA_CH_2
#define WS2811_BAUD_RATE 16
// 6 is the default pin number as used for the device in https://www.elektroda.com/rtvforum/topic4083817.html
#define WS2811_DEFAULT_DATA_PIN 6

typedef struct ws2811Data_s {
	uint8_t *buf;
	bool ready;
	int pin_data;
} ws2811Data_t;

extern ws2811Data_t ws2811Data;

/*
WS2811 peripheral test instructions:
1. For WS2811, the most important thing is the time of T0L, T1L, T0H, T1H. The following is a brief description of how to set these values.
First of all, we need to know that we have artificially fixed the ratio of T0H and T0L to 1:4, and the ratio of T1H and T1L to 4:1, (according to the WS2811 manual, this ratio is valid)
The baud rate here refers to the time of a single 0 (T0) or a single 1 (T1), so T0 = T0H + T0L, T1 = T1H + T1L,
According to the WS2811 manual and the ratio between T0H, T0L, T1H, and T1L, we generally believe (the following data are measured values ​​and can be used directly):
T0 = 1125 ns, T0L = 900ns, T0H = 225ns
T1 = 1125 ns, T1L = 225ns, T1H = 900ns
And T0 = T1 = (br + 1) * 5 * (1 / pclk)
Where br is WS2811 BR register, pclk is the peripheral clock, pclk = 40M on FPGA, so br = 8; pclk = 80M on the actual chip, so br = 16
2. Generally, WS2811 is used with DMA, and DMA uses channel 2.
3. When WS2811 does not send data, the pin will default to low level. According to the WS2811 protocol, if the bus low level lasts for more than 280us, the bus will be RESET to complete the transmission.
4. When using WS2811 + DMA, WS2811 Empty Interrupt will not respond. (The interrupt function conflicts with DMA, so the WS2811 DMA EN must be turned off)
*/


gpio_direction_t currentDirection = GPIO_INPUT;

ws2811Data_t ws2811Data = {
	.ready = false,
	.pin_data = WS2811_DEFAULT_DATA_PIN
};

// LN882H specific function (roughly the same as SPIDMA_StartTX in BK7231N)
void WS2811_StartTX(uint8_t *send_data, unsigned int data_len)
{
	// Configure DMA transfer parameters
	hal_dma_set_mem_addr(WS2811_DMA_CH,(uint32_t)send_data);
	hal_dma_set_data_num(WS2811_DMA_CH,data_len);

	// Start data transfer
	hal_dma_en(WS2811_DMA_CH,HAL_ENABLE);

	// Wait for transfer to complete
	while(hal_dma_get_data_num(WS2811_DMA_CH) != 0);

	// After sending, close DMA in time to prepare for the next configuration of DMA parameters.
	hal_dma_en(WS2811_DMA_CH,HAL_DISABLE);
}

// LN882H specific function (roughly the same as SPIDMA_InitDMA in BK7231N)
void WS2811_InitAll()
{
	lnPinMapping_t *pin = g_pins + ws2811Data.pin_data;
	// Initialize the buffer
	ws2811Data.buf = (uint8_t *)os_malloc(sizeof(uint8_t) * pixel_count * 3);

	// 1. Configure WS2811 pin multiplexing
	hal_gpio_pin_afio_select(pin->base,pin->pin,WS2811_OUT);
	hal_gpio_pin_afio_en(pin->base,pin->pin,HAL_ENABLE);

	// 2. Initialize WS2811 configuration
	ws2811_init_t_def ws2811_init;

	ws2811_init.br = WS2811_BAUD_RATE;					// baud rate = (br+1)*5 * (1 / pclk)
	hal_ws2811_init(WS2811_BASE,&ws2811_init);			// Initialize WS2811

	hal_ws2811_en(WS2811_BASE,HAL_ENABLE);				// Enable WS2811
	hal_ws2811_dma_en(WS2811_BASE,HAL_ENABLE);			// Enable WS2811 DMA

	hal_ws2811_it_cfg(WS2811_BASE,WS2811_IT_EMPTY_FLAG,HAL_ENABLE); // Configure WS2811 interrupts

	NVIC_EnableIRQ(WS2811_IRQn);						// Enable WS2811 interrupt
	NVIC_SetPriority(WS2811_IRQn,	 1);				// Set WS2811 interrupt priority

	// 3. Configure DMA
	dma_init_t_def dma_init;	
	memset(&dma_init,0,sizeof(dma_init));

	dma_init.dma_mem_addr = (uint32_t)ws2811Data.buf;	// Configure memory address
	dma_init.dma_data_num = 0;							// Set the number of transfers
	dma_init.dma_dir = DMA_READ_FORM_MEM;				// Set the transfer direction
	dma_init.dma_mem_inc_en = DMA_MEM_INC_EN;			// Enable memory to increase automatically
	dma_init.dma_p_addr = WS2811_DATA_REG;				// Set peripheral address

	hal_dma_init(WS2811_DMA_CH,&dma_init);				// Initialize DMA
	hal_dma_en(WS2811_DMA_CH,HAL_DISABLE);				// Enable DMA

	ws2811Data.ready = true;
}

void SM16703P_Show() {
	WS2811_StartTX(ws2811Data.buf, pixel_count * 3);
}

void WS2811_SetRawHexString(int pixel_offset, const char *s, int push) {
	if (!ws2811Data.ready)
		return;

	// every pixel is 3 bytes, so *3
	uint8_t *dst = ws2811Data.buf + pixel_offset * 3;
	// parse hex string like FFAABB0011 byte by byte
	while (s[0] && s[1] && s[2] && s[3] && s[4] && s[5]) {
		uint8_t b0, b1, b2;
		uint8_t r, g, b;
		r = hexbyte(s);
		s+=2;
		g = hexbyte(s);
		s+=2;
		b = hexbyte(s);
		s+=2;

		if (color_order == SM16703P_COLOR_ORDER_RGB) {
			b0 = r;
			b1 = g;
			b2 = b;
		} else if (color_order == SM16703P_COLOR_ORDER_RBG) {
			b0 = r;
			b1 = b;
			b2 = g;
		} else if (color_order == SM16703P_COLOR_ORDER_BRG) {
			b0 = b;
			b1 = r;
			b2 = g;
		} else if (color_order == SM16703P_COLOR_ORDER_BGR) {
			b0 = b;
			b1 = g;
			b2 = r;
		} else if (color_order == SM16703P_COLOR_ORDER_GRB) {
			b0 = g;
			b1 = r;
			b2 = b;
		} else if (color_order == SM16703P_COLOR_ORDER_GBR) {
			b0 = g;
			b1 = b;
			b2 = r;
		}

		*dst++ = b0;
		*dst++ = b1;
		*dst++ = b2;
	}
	if (push) {
		SM16703P_Show();
	}
}

void SM16703P_GetPixel(uint32_t pixel, byte *dst) {

}

bool SM16703P_VerifyPixel(uint32_t pixel, byte r, byte g, byte b) {
	return true;
}

void WS2811_setMultiplePixel(int pixel, const uint8_t *data, bool push) {
	// Return if driver is not loaded
	if (!ws2811Data.ready)
		return;

	// Check max pixel
	if (pixel > pixel_count)
		pixel = pixel_count;

	uint8_t *dst = ws2811Data.buf;

	// Iterate over pixel
	for (int i = 0; i < pixel; i++) {
		uint8_t b0, b1, b2;
		if (color_order == SM16703P_COLOR_ORDER_RGB) {
			b0 = *data++;
			b1 = *data++;
			b2 = *data++;
		}
		if (color_order == SM16703P_COLOR_ORDER_RBG) {
			b0 = *data++;
			b2 = *data++;
			b1 = *data++;
		}
		if (color_order == SM16703P_COLOR_ORDER_BRG) {
			b2 = *data++;
			b0 = *data++;
			b1 = *data++;
		}
		if (color_order == SM16703P_COLOR_ORDER_BGR) {
			b2 = *data++;
			b1 = *data++;
			b0 = *data++;
		}
		if (color_order == SM16703P_COLOR_ORDER_GRB) {
			b1 = *data++;
			b0 = *data++;
			b2 = *data++;
		}
		if (color_order == SM16703P_COLOR_ORDER_GBR) {
			b1 = *data++;
			b2 = *data++;
			b0 = *data++;
		}

		*dst++ = b0;
		*dst++ = b1;
		*dst++ = b2;
	}
	if (push) {
		SM16703P_Show();
	}
}

void SM16703P_setPixel(int pixel, int r, int g, int b) {
	if (!ws2811Data.ready)
		return;
	if (pixel >= pixel_count) return;

	int b0, b1, b2;

	if (color_order == SM16703P_COLOR_ORDER_RGB) {
		b0 = r;
		b1 = g;
		b2 = b;
	}
	if (color_order == SM16703P_COLOR_ORDER_RBG) {
		b0 = r;
		b1 = b;
		b2 = g;
	}
	if (color_order == SM16703P_COLOR_ORDER_BRG) {
		b0 = b;
		b1 = r;
		b2 = g;
	}
	if (color_order == SM16703P_COLOR_ORDER_BGR) {
		b0 = b;
		b1 = g;
		b2 = r;
	}
	if (color_order == SM16703P_COLOR_ORDER_GRB) {
		b0 = g;
		b1 = r;
		b2 = b;
	}
	if (color_order == SM16703P_COLOR_ORDER_GBR) {
		b0 = g;
		b1 = b;
		b2 = r;
	}

	ws2811Data.buf[pixel * 3] = b0;
	ws2811Data.buf[pixel * 3 + 1] = b1;
	ws2811Data.buf[pixel * 3 + 2] = b2;
}
extern float g_brightness0to100;//TODO
void SM16703P_setPixelWithBrig(int pixel, int r, int g, int b) {
#if ENABLE_LED_BASIC
	r = (int)(r * g_brightness0to100*0.01f);
	g = (int)(g * g_brightness0to100*0.01f);
	b = (int)(b * g_brightness0to100*0.01f);
#endif
	SM16703P_setPixel(pixel,r, g, b);
}
#define SCALE8_PIXEL(x, scale) (uint8_t)(((uint32_t)x * (uint32_t)scale) / 256)

// TODO: implement
void SM16703P_scaleAllPixels(int scale) {}

void SM16703P_setAllPixels(int r, int g, int b) {
	if (!ws2811Data.ready)
		return;

	int b0, b1, b2;

	if (color_order == SM16703P_COLOR_ORDER_RGB) {
		b0 = r;
		b1 = g;
		b2 = b;
	}
	if (color_order == SM16703P_COLOR_ORDER_RBG) {
		b0 = r;
		b1 = b;
		b2 = g;
	}
	if (color_order == SM16703P_COLOR_ORDER_BRG) {
		b0 = b;
		b1 = r;
		b2 = g;
	}
	if (color_order == SM16703P_COLOR_ORDER_BGR) {
		b0 = b;
		b1 = g;
		b2 = r;
	}
	if (color_order == SM16703P_COLOR_ORDER_GRB) {
		b0 = g;
		b1 = r;
		b2 = b;
	}
	if (color_order == SM16703P_COLOR_ORDER_GBR) {
		b0 = g;
		b1 = b;
		b2 = r;
	}

	for (int i = 0; i < pixel_count; i++) {
		ws2811Data.buf[i * 3] = b0;
		ws2811Data.buf[i * 3 + 1] = b1;
		ws2811Data.buf[i * 3 + 2] = b2;
	}
}

// SM16703P_SetRaw bUpdate pixelOfs HexData
// SM16703P_SetRaw 1 0 FF000000FF000000FF
commandResult_t SM16703P_CMD_setRaw(const void *context, const char *cmd, const char *args, int flags) {
	int ofs, bPush;
	Tokenizer_TokenizeString(args, 0);
	bPush = Tokenizer_GetArgInteger(0);
	ofs = Tokenizer_GetArgInteger(1);
	WS2811_SetRawHexString(ofs, Tokenizer_GetArg(2), bPush);
	return CMD_RES_OK;
}

commandResult_t SM16703P_CMD_setPixel(const void *context, const char *cmd, const char *args, int flags) {
	int pixel = 0;
	const char *all = 0;
	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() != 4) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Not Enough Arguments for init SM16703P: Amount of LEDs missing");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	all = Tokenizer_GetArg(0);
	if (*all == 'a') {

	}
	else {
		pixel = Tokenizer_GetArgInteger(0);
		all = 0;
	}
	uint8_t r = (uint8_t) Tokenizer_GetArgIntegerRange(1, 0, 255);
	uint8_t g = (uint8_t) Tokenizer_GetArgIntegerRange(2, 0, 255);
	uint8_t b = (uint8_t) Tokenizer_GetArgIntegerRange(3, 0, 255);

	ADDLOG_INFO(LOG_FEATURE_CMD, "Set Pixel %i to R %i G %i B %i", pixel, r, g, b);

	if (all) {
		for (int i = 0; i < pixel_count; i++) {
			SM16703P_setPixel(i, r, g, b);
		}
	}
	else {
		SM16703P_setPixel(pixel, r, g, b);
	}

	return CMD_RES_OK;
}

commandResult_t SM16703P_InitForLEDCount(const void *context, const char *cmd, const char *args, int flags) {

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() == 0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Not Enough Arguments for init SM16703P: Amount of LEDs missing");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	// First arg: number of pixel to address
	pixel_count = Tokenizer_GetArgIntegerRange(0, 0, 255);

	// Second arg (optional, default "RGB"): pixel format of "RGB" or "GRB"
	if (Tokenizer_GetArgsCount() > 1) {
		const char *format = Tokenizer_GetArg(1);
		if (!stricmp(format, "RGB")) {
			color_order = SM16703P_COLOR_ORDER_RGB;
		}
		else if (!stricmp(format, "RBG")) {
			color_order = SM16703P_COLOR_ORDER_RBG;
		}
		else if (!stricmp(format, "BRG")) {
			color_order = SM16703P_COLOR_ORDER_BRG;
		}
		else if (!stricmp(format, "BGR")) {
			color_order = SM16703P_COLOR_ORDER_BGR;
		}
		else if (!stricmp(format, "GRB")) {
			color_order = SM16703P_COLOR_ORDER_GRB;
		}
		else if (!stricmp(format, "GBR")) {
			color_order = SM16703P_COLOR_ORDER_GBR;
		}
		else {
			ADDLOG_INFO(LOG_FEATURE_CMD, "Invalid color format, should be combination of R,G,B", format);
			return CMD_RES_ERROR;
		}
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "Register driver with %i LEDs", pixel_count);

	WS2811_InitAll();

	return CMD_RES_OK;
}

static commandResult_t SM16703P_StartTX(const void *context, const char *cmd, const char *args, int flags) {
	if (!ws2811Data.ready)
		return CMD_RES_ERROR;
	SM16703P_Show();
	return CMD_RES_OK;
}

void WS2811_IRQHandler(void)
{
	hal_ws2811_set_data(WS2811_BASE, 11);
}

void SM16703P_Init() {
	ws2811Data.pin_data = PIN_FindPinIndexForRole(IOR_SM16703P_DIN, WS2811_DEFAULT_DATA_PIN);

	//cmddetail:{"name":"SM16703P_Init","args":"[NumberOfLEDs][ColorOrder]",
	//cmddetail:"descr":"This will setup LED driver for a strip with given number of LEDs. Please note that it also works for WS2812B and similiar LEDs. You can optionally set the color order with either RGB, RBG, BRG, BGB, GRB or GBR (default RGB). See [tutorial](https://www.elektroda.com/rtvforum/topic4036716.html).",
	//cmddetail:"fn":"NULL);","file":"driver/drv_sm16703P.h","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_Init", SM16703P_InitForLEDCount, NULL);
	//cmddetail:{"name":"SM16703P_Start","args":"",
	//cmddetail:"descr":"This will send the currently set data to the strip. Please note that it also works for WS2812B and similiar LEDs. See [tutorial](https://www.elektroda.com/rtvforum/topic4036716.html).",
	//cmddetail:"fn":"NULL);","file":"driver/drv_sm16703P.h","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_Start", SM16703P_StartTX, NULL);
	//cmddetail:{"name":"SM16703P_SetPixel","args":"[index/all] [R] [G] [B]",
	//cmddetail:"descr":"Sets a pixel for LED strip. Index can be a number or 'all' keyword to set all. Then, 3 integer values for R, G and B. Please note that it also works for WS2812B and similiar LEDs. See [tutorial](https://www.elektroda.com/rtvforum/topic4036716.html).",
	//cmddetail:"fn":"NULL);","file":"driver/drv_sm16703P.h","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_SetPixel", SM16703P_CMD_setPixel, NULL);
	// For the current LN882H version, the ofs parameter is unused.
	//cmddetail:{"name":"SM16703P_SetRaw","args":"[bUpdate] [byteOfs] [HexData]",
	//cmddetail:"descr":"Sets the raw data bytes for SPI DMA LED driver at the given offset. Hex data should be as a hex string, for example, FF00AA, etc. The bUpdate, if set to 1, will run SM16703P_Start automatically after setting data. Please note that it also works for WS2812B and similiar LEDs. See [tutorial](https://www.elektroda.com/rtvforum/topic4036716.html).",
	//cmddetail:"fn":"NULL);","file":"driver/drv_sm16703P.h","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_SetRaw", SM16703P_CMD_setRaw, NULL);

	//CMD_RegisterCommand("SM16703P_SendBytes", SM16703P_CMD_sendBytes, NULL);
}

#endif