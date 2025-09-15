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
#include "drv_leds_shared.h"

static ledStrip_t led_backend;

#define DEFAULT_PIXEL_SIZE 3
const enum ColorChannel default_color_channel_order[3] = {
	COLOR_CHANNEL_RED,
	COLOR_CHANNEL_GREEN,
	COLOR_CHANNEL_BLUE
};

enum ColorChannel *color_channel_order = default_color_channel_order;
int pixel_size = DEFAULT_PIXEL_SIZE; // default is RGB -> 3 bytes per pixel
// Number of pixels that can be addressed
uint32_t pixel_count;

bool Strip_HasChannel(ColorChannel_t ch) {
	for (int i = 0; i < pixel_size; i++) {
		if (color_channel_order[i] == ch) {
			return true;
		}
	}
	return false;
}

void Strip_GetPixel(uint32_t pixel, byte *dst) {
	int i;

	for (i = 0; i < pixel_size; i++) {
		dst[i] = led_backend.getByte(pixel * pixel_size + i);
	}
}

bool Strip_VerifyPixel(uint32_t pixel, byte r, byte g, byte b) {
	byte real[5];
	Strip_GetPixel(pixel, real);
	if (real[0] != r)
		return false;
	if (real[1] != g)
		return false;
	if (real[2] != b)
		return false;
	return true;
}
bool Strip_VerifyPixel5(uint32_t pixel, byte r, byte g, byte b, byte c, byte w) {
	byte real[5];
	Strip_GetPixel(pixel, real);
	if (real[0] != r)
		return false;
	if (real[1] != g)
		return false;
	if (real[2] != b)
		return false;
	if (real[3] != c)
		return false;
	if (real[4] != w)
		return false;
	return true;
}
bool Strip_VerifyPixel4(uint32_t pixel, byte r, byte g, byte b, byte w) {
	byte real[5];
	Strip_GetPixel(pixel, real);
	if (real[0] != r)
		return false;
	if (real[1] != g)
		return false;
	if (real[2] != b)
		return false;
	if (real[3] != w)
		return false;
	return true;
}


void Strip_setPixel(int pixel, int r, int g, int b, int c, int w) {
	if (pixel < 0 || pixel >= pixel_count) {
		return; // out of range - would crash
	}
	int i;
	for (i = 0; i < pixel_size; i++)
	{
		byte pcolor;
		switch (color_channel_order[i])
		{
		case COLOR_CHANNEL_RED:
			pcolor = r;
			break;
		case COLOR_CHANNEL_GREEN:
			pcolor = g;
			break;
		case COLOR_CHANNEL_BLUE:
			pcolor = b;
			break;
		case COLOR_CHANNEL_COLD_WHITE:
			pcolor = c;
			break;
		case COLOR_CHANNEL_WARM_WHITE:
			pcolor = w;
			break;
		default:
			ADDLOG_ERROR(LOG_FEATURE_CMD, "Unknown color channel %d at index %d", color_channel_order[i], i);
			return;
		}
		led_backend.setByte(i + (pixel * pixel_size), pcolor);

	}
}
void Strip_setMultiplePixel(uint32_t pixel, uint8_t *data, bool push) {
	// Check max pixel
	if (pixel > pixel_count)
		pixel = pixel_count;

	// Iterate over pixel
	for (uint32_t i = 0; i < pixel; i++) {
		uint8_t r, g, b;
		r = *data++;
		g = *data++;
		b = *data++;
		// TODO: Not sure how this works. Should we add Cold and Warm white here as well?
		Strip_setPixel((int)i, (int)r, (int)g, (int)b, 0, 0);
	}
	if (push) {
		Strip_Apply();
	}
}
extern float g_brightness0to100;//TODO
void Strip_setPixelWithBrig(int pixel, int r, int g, int b, int c, int w) {
	// scale brightness
#if ENABLE_LED_BASIC
	r = (int)(r * g_brightness0to100*0.01f);
	g = (int)(g * g_brightness0to100*0.01f);
	b = (int)(b * g_brightness0to100*0.01f);
	c = (int)(c * g_brightness0to100*0.01f);
	w = (int)(w * g_brightness0to100*0.01f);
#endif
	Strip_setPixel(pixel, r, g, b, c, w);
}
#define SCALE8_PIXEL(x, scale) (uint8_t)(((uint32_t)x * (uint32_t)scale) / 256)

void Strip_scaleAllPixels(int scale) {
	int pixel;
	byte b;
	int ofs;
	byte *data, *input;

	for (pixel = 0; pixel < pixel_count; pixel++) {
		for (ofs = 0; ofs < pixel_size; ofs++) {
			int byteIndex = pixel * pixel_size + ofs;
			byte b = led_backend.getByte(byteIndex);
			b = SCALE8_PIXEL(b, scale);
			led_backend.setByte(byteIndex, b);
		}
	}
}
void Strip_setAllPixels(int r, int g, int b, int c, int w) {
	int pixel;

	for (pixel = 0; pixel < pixel_count; pixel++) {
		Strip_setPixel(pixel, r, g, b, c, w);
	}
}


// SM16703P_SetRaw bUpdate byteOfs HexData
// SM16703P_SetRaw 1 0 FF000000FF000000FF
commandResult_t Strip_CMD_setRaw(const void *context, const char *cmd, const char *args, int flags) {
	int ofs, bPush;
	Tokenizer_TokenizeString(args, 0);
	bPush = Tokenizer_GetArgInteger(0);
	ofs = Tokenizer_GetArgInteger(1);
	const char *s = Tokenizer_GetArg(2);
	int i = 0;
	// parse hex string like FFAABB0011 byte by byte
	while (s[0] && s[1]) {
		led_backend.setByte(i, hexbyte(s));
		i++;
		s += 2;
	}
	if (bPush) {
		led_backend.apply();
	}
	return CMD_RES_OK;
}
commandResult_t Strip_CMD_setPixel(const void *context, const char *cmd, const char *args, int flags) {
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
	c = 0; // cold white is optional for backward compatibility]
	w = 0; // warm white is optional for backward compatibility
	int numArgs = Tokenizer_GetArgsCount();
	// this is a hack, so we can easily write commands for RGBW...
	if (numArgs == 5 && Strip_HasChannel(COLOR_CHANNEL_WARM_WHITE)) {
		// treat it as Warm (not Cool)
		w = Tokenizer_GetArgIntegerRange(4, 0, 255);
	}
	else {
		if (numArgs > 4) {
			c = Tokenizer_GetArgIntegerRange(4, 0, 255);
		}
		if (numArgs > 5) {
			w = Tokenizer_GetArgIntegerRange(5, 0, 255);
		}
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "Set Pixel %i to R %i G %i B %i C %i W %i", pixel, r, g, b, c, w);

	if (all) {
		for (i = 0; i < pixel_count; i++) {
			Strip_setPixel(i, r, g, b, c, w);
		}
	}
	else {
		Strip_setPixel(pixel, r, g, b, c, w);
	}


	return CMD_RES_OK;
}

commandResult_t Strip_CMD_InitForLEDCount(const void *context, const char *cmd, const char *args, int flags) {

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() == 0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Not Enough Arguments for init SM16703P: Amount of LEDs missing");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	//SM16703P_Shutdown();

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
		if (color_channel_order != default_color_channel_order) {
			os_free(color_channel_order);
		}
		color_channel_order = new_channel_order;
	}
	led_backend.setLEDCount(pixel_count, pixel_size);

	ADDLOG_INFO(LOG_FEATURE_CMD, "Register driver with %i LEDs", pixel_count);


	return CMD_RES_OK;
}

bool Strip_IsActive() {
	if (led_backend.apply) {
		return true;
	}
	return false;
}
void Strip_Apply() {
	led_backend.apply();
}
static commandResult_t Strip_CMD_StartTX(const void *context, const char *cmd, const char *args, int flags) {
	Strip_Apply();
	return CMD_RES_OK;
}

// startDriver SM16703P
// backlog startDriver SM16703P; SM16703P_Test
void LEDS_InitShared(ledStrip_t *api) {

	led_backend = *api;

	//cmddetail:{"name":"SM16703P_Init","args":"[NumberOfLEDs][ColorOrder]",
	//cmddetail:"descr":"This will setup LED driver for a strip with given number of LEDs. Please note that it also works for WS2812B and similiar LEDs. You can optionally set the color order with can be any combination of R, G, B, C and W (e.g. RGBW or GRBWC, default is RGB). See [tutorial](https://www.elektroda.com/rtvforum/topic4036716.html).",
	//cmddetail:"fn":"SM16703P_InitForLEDCount","file":"driver/drv_leds_shared.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_Init", Strip_CMD_InitForLEDCount, NULL);
	CMD_CreateAliasHelper("Strip_Init","SM16703P_Init");
	//cmddetail:{"name":"SM16703P_Start","args":"",
	//cmddetail:"descr":"This will send the currently set data to the strip. Please note that it also works for WS2812B and similiar LEDs. See [tutorial](https://www.elektroda.com/rtvforum/topic4036716.html).",
	//cmddetail:"fn":"SM16703P_StartTX","file":"driver/drv_leds_shared.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_Start", Strip_CMD_StartTX, NULL);
	CMD_CreateAliasHelper("Strip_Start", "SM16703P_Start");
	//cmddetail:{"name":"SM16703P_SetPixel","args":"[index/all] [R] [G] [B]",
	//cmddetail:"descr":"Sets a pixel for LED strip. Index can be a number or 'all' keyword to set all. Then, 3 integer values for R, G and B. Please note that it also works for WS2812B and similiar LEDs. See [tutorial](https://www.elektroda.com/rtvforum/topic4036716.html).",
	//cmddetail:"fn":"SM16703P_CMD_setPixel","file":"driver/drv_leds_shared.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_SetPixel", Strip_CMD_setPixel, NULL);
	CMD_CreateAliasHelper("Strip_SetPixel", "SM16703P_SetPixel");
	//cmddetail:{"name":"SM16703P_SetRaw","args":"[bUpdate] [byteOfs] [HexData]",
	//cmddetail:"descr":"Sets the raw data bytes for SPI DMA LED driver at the given offset. Hex data should be as a hex string, for example, FF00AA, etc. The bUpdate, if set to 1, will run SM16703P_Start automatically after setting data. Please note that it also works for WS2812B and similiar LEDs. See [tutorial](https://www.elektroda.com/rtvforum/topic4036716.html).",
	//cmddetail:"fn":"SM16703P_CMD_setRaw","file":"driver/drv_leds_shared.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SM16703P_SetRaw", Strip_CMD_setRaw, NULL);
	CMD_CreateAliasHelper("Strip_SetRaw", "SM16703P_SetRaw");

	//CMD_RegisterCommand("SM16703P_SendBytes", SM16703P_CMD_sendBytes, NULL);
}

void LEDS_ShutdownShared() {
	if (color_channel_order != default_color_channel_order) {
		os_free(color_channel_order);
		color_channel_order = default_color_channel_order;
	}
	pixel_size = DEFAULT_PIXEL_SIZE;
	pixel_count = 0;
	memset(&led_backend, 0, sizeof(led_backend));
}
