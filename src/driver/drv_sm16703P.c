#if PLATFORM_BK7231N || WINDOWS


#include "../new_cfg.h"
#include "../new_common.h"
#include "../new_pins.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../mqtt/new_mqtt.h"

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

#define SM16703P_COLOR_ORDER_RGB         0x00
#define SM16703P_COLOR_ORDER_RBG         0x01
#define SM16703P_COLOR_ORDER_BRG         0x02
#define SM16703P_COLOR_ORDER_BGR         0x03
#define SM16703P_COLOR_ORDER_GRB         0x04
#define SM16703P_COLOR_ORDER_GBR         0x05
int color_order = SM16703P_COLOR_ORDER_RGB; // default to RGB


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
		if (color_order == SM16703P_COLOR_ORDER_RGB) {
			r = *data++;
			g = *data++;
			b = *data++;
		}
		if (color_order == SM16703P_COLOR_ORDER_RBG) {
			r = *data++;
			b = *data++;
			g = *data++;
		}
		if (color_order == SM16703P_COLOR_ORDER_BRG) {
			b = *data++;
			r = *data++;
			g = *data++;
		}
		if (color_order == SM16703P_COLOR_ORDER_BGR) {
			b = *data++;
			g = *data++;
			r = *data++;
		}
		if (color_order == SM16703P_COLOR_ORDER_GRB) {
			g = *data++;
			r = *data++;
			b = *data++;
		}
		if (color_order == SM16703P_COLOR_ORDER_GBR) {
			g = *data++;
			b = *data++;
			r = *data++;
		}

		*dst++ = translate_2bit((r >> 6));
		*dst++ = translate_2bit((r >> 4));
		*dst++ = translate_2bit((r >> 2));
		*dst++ = translate_2bit(r);
		*dst++ = translate_2bit((g >> 6));
		*dst++ = translate_2bit((g >> 4));
		*dst++ = translate_2bit((g >> 2));
		*dst++ = translate_2bit(g);
		*dst++ = translate_2bit((b >> 6));
		*dst++ = translate_2bit((b >> 4));
		*dst++ = translate_2bit((b >> 2));
		*dst++ = translate_2bit(b);
	}
	if (push) {
		SPIDMA_StartTX(spiLED.msg);
	}
}
void SM16703P_setPixel(int pixel, int r, int g, int b) {
	if (!spiLED.ready)
		return;
	// Load data in correct format
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
	translate_byte(b0, spiLED.buf + (spiLED.ofs + 0 + (pixel * 3 * 4)));
	translate_byte(b1, spiLED.buf + (spiLED.ofs + 4 + (pixel * 3 * 4)));
	translate_byte(b2, spiLED.buf + (spiLED.ofs + 8 + (pixel * 3 * 4)));
}
extern float g_brightness0to100;//TODO
void SM16703P_setPixelWithBrig(int pixel, int r, int g, int b) {
	r = (int)(r * g_brightness0to100*0.01f);
	g = (int)(g * g_brightness0to100*0.01f);
	b = (int)(b * g_brightness0to100*0.01f);
	SM16703P_setPixel(pixel,r, g, b);
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
void SM16703P_setAllPixels(int r, int g, int b) {
	int pixel;
	if (!spiLED.ready)
		return;
	// Load data in correct format
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
	for (pixel = 0; pixel < pixel_count; pixel++) {
		translate_byte(b0, spiLED.buf + (spiLED.ofs + 0 + (pixel * 3 * 4)));
		translate_byte(b1, spiLED.buf + (spiLED.ofs + 4 + (pixel * 3 * 4)));
		translate_byte(b2, spiLED.buf + (spiLED.ofs + 8 + (pixel * 3 * 4)));
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
	int i, r, g, b;
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
	r = Tokenizer_GetArgIntegerRange(1, 0, 255);
	g = Tokenizer_GetArgIntegerRange(2, 0, 255);
	b = Tokenizer_GetArgIntegerRange(3, 0, 255);

	ADDLOG_INFO(LOG_FEATURE_CMD, "Set Pixel %i to R %i G %i B %i", pixel, r, g, b);

	if (all) {
		for (i = 0; i < pixel_count; i++) {
			SM16703P_setPixel(i, r, g, b);
		}
	}
	else {
		SM16703P_setPixel(pixel, r, g, b);

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
	SPILED_InitDMA(pixel_count * 3);

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
	//cmddetail:"descr":"This will setup LED driver for a strip with given number of LEDs. Please note that it also works for WS2812B and similiar LEDs. You can optionally set the color order with either RGB, RBG, BRG, BGB, GRB or GBR (default RGB). See [tutorial](https://www.elektroda.com/rtvforum/topic4036716.html).",
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
#endif
