#if (PLATFORM_BK7231N || WINDOWS) && !PLATFORM_BEKEN_NEW


#include "../new_cfg.h"
#include "../new_common.h"
#include "../new_pins.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../mqtt/new_mqtt.h"

#include "drv_local.h"
#include "drv_spiLED.h"

// Number of pixels that can be addressed
uint32_t pixel_count;

void SM16703P_GetPixel(uint32_t pixel, byte *dst) {
	int i;
	uint8_t *input;

	if (spiLED.msg == 0)
		return;
	
	input = spiLED.buf + spiLED.ofs + (pixel * 3 * 4);

	for (i = 0; i < 3; i++) {
		*dst++ = reverse_translate_byte(input + i * 4);
	}
}

enum ColorChannel {
	COLOR_CHANNEL_RED,
	COLOR_CHANNEL_GREEN,
	COLOR_CHANNEL_BLUE,
	COLOR_CHANNEL_COLD_WHITE,
	COLOR_CHANNEL_WARM_WHITE
};
const enum ColorChannel default_color_channel_order[3] = {
	COLOR_CHANNEL_RED,
	COLOR_CHANNEL_GREEN,
	COLOR_CHANNEL_BLUE
};
enum ColorChannel *color_channel_order = default_color_channel_order;
int pixel_size = 3; // default is RGB -> 3 bytes per pixel

bool SM16703P_VerifyPixel(uint32_t pixel, byte r, byte g, byte b) {
	byte real[3];
	SM16703P_GetPixel(pixel, real);
	if (real[0] != r)
		return false;
	if (real[1] != g)
		return false;
	if (real[2] != b)
		return false;
	return true;
}


void SM16703P_setPixel(int pixel, int r, int g, int b, int c, int w) {
	if (!spiLED.ready)
		return;
	int i;
	for(i = 0; i < pixel_size; i++) 
	{
		switch (color_channel_order[i])
		{
			case COLOR_CHANNEL_RED:
				translate_byte(r, spiLED.buf + (spiLED.ofs + i * 4 + (pixel * pixel_size * 4)));
				break;
			case COLOR_CHANNEL_GREEN:
				translate_byte(g, spiLED.buf + (spiLED.ofs + i * 4 + (pixel * pixel_size * 4)));
				break;
			case COLOR_CHANNEL_BLUE:
				translate_byte(b, spiLED.buf + (spiLED.ofs + i * 4 + (pixel * pixel_size * 4)));
				break;
			case COLOR_CHANNEL_COLD_WHITE:
				translate_byte(c, spiLED.buf + (spiLED.ofs + i * 4 + (pixel * pixel_size * 4)));
				break;
			case COLOR_CHANNEL_WARM_WHITE:
				translate_byte(w, spiLED.buf + (spiLED.ofs + i * 4 + (pixel * pixel_size * 4)));
				break;
			default:
				ADDLOG_ERROR(LOG_FEATURE_CMD, "Unknown color channel %d at index %d", color_channel_order[i], i);
				return;
		}
	}
}
void SM16703P_setMultiplePixel(uint32_t pixel, uint8_t *data, bool push) {

	// Return if driver is not loaded
	if (!spiLED.ready)
		return;

	// Check max pixel
	if (pixel > pixel_count)
		pixel = pixel_count;

	// Iterate over pixel
	uint8_t *dst = spiLED.buf + spiLED.ofs;
	for (uint32_t i = 0; i < pixel; i++) {
		uint8_t r, g, b;
		r = *data++;
		g = *data++;
		b = *data++;
		// TODO: Not sure how this works. Should we add Cold and Warm white here as well?
		SM16703P_setPixel((int)i, (int)r, (int)g, (int)b, 0, 0);
	}
	if (push) {
		SPIDMA_StartTX(spiLED.msg);
	}
}
extern float g_brightness0to100;//TODO
void SM16703P_setPixelWithBrig(int pixel, int r, int g, int b, int c, int w) {
	// scale brightness
#if ENABLE_LED_BASIC
	r = (int)(r * g_brightness0to100*0.01f);
	g = (int)(g * g_brightness0to100*0.01f);
	b = (int)(b * g_brightness0to100*0.01f);
	c = (int)(c * g_brightness0to100*0.01f);
	w = (int)(w * g_brightness0to100*0.01f);
#endif
	SM16703P_setPixel(pixel,r, g, b, c, w);
}
#define SCALE8_PIXEL(x, scale) (uint8_t)(((uint32_t)x * (uint32_t)scale) / 256)

void SM16703P_scaleAllPixels(int scale) {
	int pixel;
	byte b;
	int ofs;
	byte *data, *input;

	for (pixel = 0; pixel < pixel_count; pixel++) {
		for (ofs = 0; ofs < 3; ofs++) {
			data = spiLED.buf + (spiLED.ofs + ofs * 4 + (pixel * 3 * 4));
			b = reverse_translate_byte(data);
			b = SCALE8_PIXEL(b, scale);
			translate_byte(b, data);
		}
	}
}
void SM16703P_setAllPixels(int r, int g, int b, int c, int w) {
	int pixel;
	if (!spiLED.ready)
		return;
	
	for (pixel = 0; pixel < pixel_count; pixel++) {
		SM16703P_setPixel(pixel, r, g, b, c, w);
	}
}


// SM16703P_SetRaw bUpdate byteOfs HexData
// SM16703P_SetRaw 1 0 FF000000FF000000FF
commandResult_t SM16703P_CMD_setRaw(const void *context, const char *cmd, const char *args, int flags) {
	int ofs, bPush;
	Tokenizer_TokenizeString(args, 0);
	bPush = Tokenizer_GetArgInteger(0);
	ofs = Tokenizer_GetArgInteger(1);
	SPILED_SetRawHexString(ofs, Tokenizer_GetArg(2), bPush);
	return CMD_RES_OK;
}
commandResult_t SM16703P_CMD_setPixel(const void *context, const char *cmd, const char *args, int flags) {
	int i, r, g, b, c, w;
	int pixel = 0;
	const char *all = 0;
	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 4) {
		// We need at least 4 arguments: pixel, red, green, blue - cold and warm white are optional
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
	r = Tokenizer_GetArgIntegerRange(1, 0, 255);
	g = Tokenizer_GetArgIntegerRange(2, 0, 255);
	b = Tokenizer_GetArgIntegerRange(3, 0, 255);
	c = 0; // cold white is optional for backward compatibility
	if (Tokenizer_GetArgsCount() > 4) {
		c = Tokenizer_GetArgIntegerRange(4, 0, 255);
	}
	w = 0; // warm white is optional for backward compatibility
	if (Tokenizer_GetArgsCount() > 5) {
		w = Tokenizer_GetArgIntegerRange(5, 0, 255);
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "Set Pixel %i to R %i G %i B %i C %i W %i", pixel, r, g, b, c, w);

	if (all) {
		for (i = 0; i < pixel_count; i++) {
			SM16703P_setPixel(i, r, g, b, c, w);
		}
	}
	else {
		SM16703P_setPixel(pixel, r, g, b, c, w);

		ADDLOG_INFO(LOG_FEATURE_CMD, "Raw Data 0x%02x 0x%02x 0x%02x 0x%02x - 0x%02x 0x%02x 0x%02x 0x%02x - 0x%02x 0x%02x 0x%02x 0x%02x",
			spiLED.buf[spiLED.ofs + 0 + (pixel * 3 * 4)],
			spiLED.buf[spiLED.ofs + 1 + (pixel * 3 * 4)],
			spiLED.buf[spiLED.ofs + 2 + (pixel * 3 * 4)],
			spiLED.buf[spiLED.ofs + 3 + (pixel * 3 * 4)],
			spiLED.buf[spiLED.ofs + 4 + (pixel * 3 * 4)],
			spiLED.buf[spiLED.ofs + 5 + (pixel * 3 * 4)],
			spiLED.buf[spiLED.ofs + 6 + (pixel * 3 * 4)],
			spiLED.buf[spiLED.ofs + 7 + (pixel * 3 * 4)],
			spiLED.buf[spiLED.ofs + 8 + (pixel * 3 * 4)],
			spiLED.buf[spiLED.ofs + 9 + (pixel * 3 * 4)],
			spiLED.buf[spiLED.ofs + 10 + (pixel * 3 * 4)],
			spiLED.buf[spiLED.ofs + 11 + (pixel * 3 * 4)]);
	}


	return CMD_RES_OK;
}

commandResult_t SM16703P_InitForLEDCount(const void *context, const char *cmd, const char *args, int flags) {

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() == 0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Not Enough Arguments for init SM16703P: Amount of LEDs missing");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	
	SM16703P_Shutdown();

	// First arg: number of pixel to address
	pixel_count = Tokenizer_GetArgIntegerRange(0, 0, 255);
	// Second arg (optional, default "RGB"): pixel format of "RGB" or "GRB"
	if (Tokenizer_GetArgsCount() > 1) {
		const char *format = Tokenizer_GetArg(1);
		size_t format_length = strlen(format);
		enum ColorChannel *new_channel_order = os_malloc(sizeof(enum ColorChannel) * (format_length + 1));
		if (!new_channel_order) {
			ADDLOG_ERROR(LOG_FEATURE_CMD, "Failed to allocate memory for color channel order");
			return CMD_RES_ERROR;
		}
		int i = 0;
		for (const char *p = format; *p; p++) {
			switch (*p) {
				case 'R':
					new_channel_order[i++] = COLOR_CHANNEL_RED;
					break;
				case 'G':
					new_channel_order[i++] = COLOR_CHANNEL_GREEN;
					break;
				case 'B':
					new_channel_order[i++] = COLOR_CHANNEL_BLUE;
					break;
				case 'C':
					new_channel_order[i++] = COLOR_CHANNEL_COLD_WHITE;
					break;
				case 'W':
					new_channel_order[i++] = COLOR_CHANNEL_WARM_WHITE;
					break;
				default:
					ADDLOG_ERROR(LOG_FEATURE_CMD, "Invalid color '%c' in format '%s', should be combination of R,G,B,C,W", *p, format);
					os_free(new_channel_order);
					return CMD_RES_ERROR;
			}
		}
		pixel_size = i; // number of color channels
		color_channel_order = new_channel_order;
	}
	// Third arg (optional, default "0"): spiLED.ofs to prepend to each transmission
	if (Tokenizer_GetArgsCount() > 2) {
		spiLED.ofs = Tokenizer_GetArgIntegerRange(2, 0, 255);
	}
	// Fourth arg (optional, default "64"): spiLED.padding to append to each transmission
	//spiLED.padding = 64;
	//if (Tokenizer_GetArgsCount() > 3) {
	//	spiLED.padding = Tokenizer_GetArgIntegerRange(3, 0, 255);
	//}

	ADDLOG_INFO(LOG_FEATURE_CMD, "Register driver with %i LEDs", pixel_count);
	
	// each pixel is RGB, so 3 bytes per pixel
	SPILED_InitDMA(pixel_count * pixel_size);

	return CMD_RES_OK;
}

void SM16703P_Show() {
	SPIDMA_StartTX(spiLED.msg);
}
static commandResult_t SM16703P_StartTX(const void *context, const char *cmd, const char *args, int flags) {
	if (!spiLED.ready)
		return CMD_RES_ERROR;
	SM16703P_Show();
	return CMD_RES_OK;
}
//static commandResult_t SM16703P_CMD_sendBytes(const void *context, const char *cmd, const char *args, int flags) {
//	if (!spiLED.ready)
//		return CMD_RES_ERROR;
//	const char *s = args;
//	int i = spiLED.ofs;
//	while (*s && s[1]) {
//		*(spiLED.buf + (i)) = hexbyte(s);
//		s += 2;
//		i++;
//	}
//	while (i < spiLED.msg->send_len) {
//		*(spiLED.buf + (i)) = 0;
//	}
//	SPIDMA_StartTX(spiLED.msg);
//	return CMD_RES_OK;
//}

// startDriver SM16703P
// backlog startDriver SM16703P; SM16703P_Test
void SM16703P_Init() {
	SPILED_Init();

	//cmddetail:{"name":"SM16703P_Init","args":"[NumberOfLEDs][ColorOrder]",
	//cmddetail:"descr":"This will setup LED driver for a strip with given number of LEDs. Please note that it also works for WS2812B and similiar LEDs. You can optionally set the color order with can be any combination of R, G, B, C and W (e.g. RGBW or GRBWC, default is RGB). See [tutorial](https://www.elektroda.com/rtvforum/topic4036716.html).",
	//cmddetail:"fn":"NULL);","file":"driver/drv_sm16703P.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_Init", SM16703P_InitForLEDCount, NULL);
	//cmddetail:{"name":"SM16703P_Start","args":"",
	//cmddetail:"descr":"This will send the currently set data to the strip. Please note that it also works for WS2812B and similiar LEDs. See [tutorial](https://www.elektroda.com/rtvforum/topic4036716.html).",
	//cmddetail:"fn":"NULL);","file":"driver/drv_sm16703P.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_Start", SM16703P_StartTX, NULL);
	//cmddetail:{"name":"SM16703P_SetPixel","args":"[index/all] [R] [G] [B]",
	//cmddetail:"descr":"Sets a pixel for LED strip. Index can be a number or 'all' keyword to set all. Then, 3 integer values for R, G and B. Please note that it also works for WS2812B and similiar LEDs. See [tutorial](https://www.elektroda.com/rtvforum/topic4036716.html).",
	//cmddetail:"fn":"NULL);","file":"driver/drv_sm16703P.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_SetPixel", SM16703P_CMD_setPixel, NULL);
	//cmddetail:{"name":"SM16703P_SetRaw","args":"[bUpdate] [byteOfs] [HexData]",
	//cmddetail:"descr":"Sets the raw data bytes for SPI DMA LED driver at the given offset. Hex data should be as a hex string, for example, FF00AA, etc. The bUpdate, if set to 1, will run SM16703P_Start automatically after setting data. Please note that it also works for WS2812B and similiar LEDs. See [tutorial](https://www.elektroda.com/rtvforum/topic4036716.html).",
	//cmddetail:"fn":"NULL);","file":"driver/drv_sm16703P.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_SetRaw", SM16703P_CMD_setRaw, NULL);

	//CMD_RegisterCommand("SM16703P_SendBytes", SM16703P_CMD_sendBytes, NULL);
}

void SM16703P_Shutdown() {
	if (color_channel_order != default_color_channel_order) {
		os_free(color_channel_order);
		color_channel_order = default_color_channel_order;
	}
}
#endif
