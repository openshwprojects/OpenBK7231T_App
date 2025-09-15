// WS2812B etc animations
// For the animations themselves credit goes to https://github.com/Electriangle
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"

#if ENABLE_DRIVER_PIXELANIM

// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "../hal/hal_pins.h"
#include <math.h>

/*
// Usage:
startDriver SM16703P
SM16703P_Init 16
startDriver PixelAnim

*/

// Credit: https://github.com/Electriangle/RainbowCycle_Main
byte *RainbowWheel_Wheel(byte WheelPosition) {
	static byte c[3];

	if (WheelPosition < 85) {
		c[0] = WheelPosition * 3;
		c[1] = 255 - WheelPosition * 3;
		c[2] = 0;
	}
	else if (WheelPosition < 170) {
		WheelPosition -= 85;
		c[0] = 255 - WheelPosition * 3;
		c[1] = 0;
		c[2] = WheelPosition * 3;
	}
	else {
		WheelPosition -= 170;
		c[0] = 0;
		c[1] = WheelPosition * 3;
		c[2] = 255 - WheelPosition * 3;
	}

	return c;
}
uint16_t j = 0;
uint16_t count = 0;
int direction = 1;
void fadeToBlackBy(uint8_t fadeBy)
{
	Strip_scaleAllPixels(255-fadeBy);
}
void ShootingStar_Run() {
	int tail_length = 32;
	if (direction == -1) {        // Reverse direction option for LEDs
		if (count < pixel_count) {
			Strip_setPixelWithBrig(pixel_count - (count % (pixel_count + 1)),
				led_baseColors[0], led_baseColors[1], led_baseColors[2], led_baseColors[3], led_baseColors[4]);    // Set LEDs with the color value
		}
		count++;
	}
	else {
		if (count < pixel_count) {     // Forward direction option for LEDs
			Strip_setPixelWithBrig(count % pixel_count,
				led_baseColors[0], led_baseColors[1], led_baseColors[2], led_baseColors[3], led_baseColors[4]);    // Set LEDs with the color value
		}
		count++;
	}
	if (count > pixel_count + 16) {
		count = 0;
	}
	fadeToBlackBy(tail_length);                 // Fade the tail LEDs to black
	Strip_Apply();
}
void RainbowCycle_Run() {
	byte *c;
	uint16_t i;

	for (i = 0; i < pixel_count; i++) {
		c = RainbowWheel_Wheel(((i * 256 / pixel_count) + j) & 255);
		Strip_setPixelWithBrig(pixel_count - 1 - i, *c, *(c + 1), *(c + 2), 0, 0);
	}
	Strip_Apply();
	j++;
	j %= 256;
}

void Fire_setPixelHeatColor(int Pixel, byte temperature) {
	// Rescale heat from 0-255 to 0-191
	byte t192 = round((temperature / 255.0) * 191);

	// Calculate ramp up from
	byte heatramp = t192 & 0x3F; // 0...63
	heatramp <<= 2; // scale up to 0...252

	// Figure out which third of the spectrum we're in:
	if (t192 > 0x80) {                    // hottest
		Strip_setPixelWithBrig(Pixel, 255, 255, heatramp, 0, 0); // red to yellow
	}
	else if (t192 > 0x40) {               // middle
		Strip_setPixelWithBrig(Pixel, 255, heatramp, 0, 0, 0); // red to yellow
	}
	else {                               // coolest
		Strip_setPixelWithBrig(Pixel, heatramp, 0, 0, 0, 0);
	}
}
// FlameHeight - Use larger value for shorter flames, default=50.
// Sparks - Use larger value for more ignitions and a more active fire (between 0 to 255), default=100.
// DelayDuration - Use larger value for slower flame speed, default=10.
int FlameHeight = 50;
int Sparks = 100;
static byte *pix_workBuffer = 0;
static int pix_workBufferSize = 0;

void Pix_EnsureAllocatedWork(int bytes) {
	if (bytes < pix_workBufferSize)
		return;
	pix_workBufferSize = bytes + 16;
	pix_workBuffer = (byte*)realloc(pix_workBuffer, pix_workBufferSize);
}
int RandomRange(int min, int max) {
	int r = rand() % (max - min);
	return min + r;
}
void Fire_Run() {
	int cooldown;

	// we need a buffer for that
	Pix_EnsureAllocatedWork(pixel_count);
	// in case that realloc failed...
	if (pix_workBuffer == 0) {
		return;
	}
	// alias it as 'heat'
	byte *heat = pix_workBuffer;

	// Cool down each cell a little
	for (int i = 0; i < pixel_count; i++) {
		cooldown = RandomRange(0, ((FlameHeight * 10) / pixel_count) + 2);

		if (cooldown > heat[i]) {
			heat[i] = 0;
		}
		else {
			heat[i] = heat[i] - cooldown;
		}
	}

	// Heat from each cell drifts up and diffuses slightly
	for (int k = (pixel_count - 1); k >= 2; k--) {
		heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
	}

	// Randomly ignite new Sparks near bottom of the flame
	if (rand()%255 < Sparks) {
		int y = rand()%7;
		heat[y] = heat[y] + RandomRange(160, 255);
	}

	// Convert heat to LED colors
	for (int j = 0; j < pixel_count; j++) {
		Fire_setPixelHeatColor(j, heat[j]);
	}
	Strip_Apply();

}
static int comet_pos = 0;
static int comet_dir_local = 1;
static int comet_tail_len = 16;

void Comet_Run() {
	int head = comet_pos;
	int tail = comet_tail_len;
	if (comet_dir_local > 0) {
		comet_pos++;
		if (comet_pos >= pixel_count) {
			comet_pos = pixel_count - 1;
			comet_dir_local = -1;
		}
	}
	else {
		comet_pos--;
		if (comet_pos < 0) {
			comet_pos = 0;
			comet_dir_local = 1;
		}
	}

	fadeToBlackBy(48);

	Strip_setPixelWithBrig(head,
		led_baseColors[0], led_baseColors[1], led_baseColors[2], led_baseColors[3], led_baseColors[4]);

	for (int t = 1; t <= tail; t++) {
		int idx = head - t * comet_dir_local;
		if (idx < 0 || idx >= pixel_count) continue;
		float scale = (float)(tail - t) / (float)tail;
		if (scale < 0) scale = 0;
		byte r = (byte)round(led_baseColors[0] * scale);
		byte g = (byte)round(led_baseColors[1] * scale);
		byte b = (byte)round(led_baseColors[2] * scale);
		Strip_setPixelWithBrig(idx, r, g, b, 0, 0);
	}

	Strip_Apply();
}
static int chase_pos = 0;

void TheaterChase_Run() {
	fadeToBlackBy(200);

	for (int i = 0; i < pixel_count; i++) {
		if ((i + chase_pos) % 3 == 0) {
			Strip_setPixelWithBrig(i,
				led_baseColors[0], led_baseColors[1], led_baseColors[2],
				led_baseColors[3], led_baseColors[4]);
		}
	}
	Strip_Apply();

	chase_pos++;
	if (chase_pos >= 3) chase_pos = 0;
}

static int chase_rainbow_pos = 0;

void TheaterChaseRainbow_Run() {
	for (int i = 0; i < pixel_count; i++) {
		if ((i + chase_rainbow_pos) % 3 == 0) {
			byte *c = RainbowWheel_Wheel((i + chase_rainbow_pos * 8) & 255);
			Strip_setPixelWithBrig(i, *c, *(c + 1), *(c + 2), 0, 0);
		}
		else {
			Strip_setPixelWithBrig(i, 0, 0, 0, 0, 0);
		}
	}
	Strip_Apply();

	chase_rainbow_pos++;
	if (chase_rainbow_pos >= 3) chase_rainbow_pos = 0;
}
// startDriver PixelAnim

typedef struct ledAnim_s {
	const char *name;
	void(*runFunc)();
} ledAnim_t;

int activeAnim = -1;
ledAnim_t g_anims[] = {
	{ "Rainbow Cycle", RainbowCycle_Run },
	{ "Fire", Fire_Run },
	{ "Shooting Star", ShootingStar_Run },
	{ "Comet", Comet_Run },
	{ "Theater Chase", TheaterChase_Run },
	{ "Theater Chase Rainbow", TheaterChaseRainbow_Run }
};
int g_numAnims = sizeof(g_anims) / sizeof(g_anims[0]);
int g_speed = 0;

void PixelAnim_SetAnim(int j) {
	activeAnim = j;
	g_lightMode = Light_Anim;
	if (CFG_HasFlag(OBK_FLAG_LED_AUTOENABLE_ON_ANY_ACTION)) {
		LED_SetEnableAll(true);
	}
	apply_smart_light();
}
commandResult_t PA_Cmd_Anim(const void *context, const char *cmd, const char *args, int flags) {

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() == 0) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	PixelAnim_SetAnim(Tokenizer_GetArgInteger(0));

	return CMD_RES_OK;
}
commandResult_t PA_Cmd_AnimSpeed(const void *context, const char *cmd, const char *args, int flags) {

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() == 0) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	g_speed = Tokenizer_GetArgInteger(0);

	return CMD_RES_OK;
}
void PixelAnim_Init() {

	//cmddetail:{"name":"Anim","args":"[AnimationIndex]",
	//cmddetail:"descr":"Starts given WS2812 animation by index.",
	//cmddetail:"fn":"PA_Cmd_Anim","file":"driver/drv_pixelAnim.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("Anim", PA_Cmd_Anim, NULL);
	//cmddetail:{"name":"AnimSpeed","args":"[Interval]",
	//cmddetail:"descr":"Sets WS2812 animation speed",
	//cmddetail:"fn":"PA_Cmd_AnimSpeed","file":"driver/drv_pixelAnim.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("AnimSpeed", PA_Cmd_AnimSpeed, NULL);
}

void PixelAnim_CreatePanel(http_request_t *request) {
	const char* activeStr = "";
	char tmpA[16];
	int i;

	if (http_getArg(request->url, "an", tmpA, sizeof(tmpA))) {
		j = atoi(tmpA);
		hprintf255(request, "<h3>Ran %i!</h3>", (j));
		PixelAnim_SetAnim(j);
	}
	if (http_getArg(request->url, "spd", tmpA, sizeof(tmpA))) {
		j = atoi(tmpA);
		hprintf255(request, "<h3>Speed %i!</h3>", (j));
		g_speed = j;
	}

	poststr(request, "<tr><td>");
	hprintf255(request, "<h5>Speed</h5>");
	hprintf255(request, "<form action=\"index\">");
	hprintf255(request, "<input type=\"range\" min=\"0\" max=\"10\" name=\"spd\" id=\"spd\" value=\"%i\" onchange=\"this.form.submit()\">",
		g_speed);
	hprintf255(request, "<input  type=\"submit\" class='disp-none'></form>");
	poststr(request, "</td></tr>");

	if (g_lightMode == Light_Anim) {
		activeStr = "[ACTIVE]";
	}
	poststr(request, "<tr><td>");
	hprintf255(request, "<h5>LED Animation %s</h5>", activeStr);

	for (i = 0; i < g_numAnims; i++) {
		const char* c;
		if (i == activeAnim && g_lightMode == Light_Anim) {
			c = "bgrn";
		}
		else {
			c = "bred";
		}
		poststr(request, "<form action=\"index\">");
		hprintf255(request, "<input type=\"hidden\" name=\"an\" value=\"%i\">", i);
		hprintf255(request, "<input class=\"%s\" type=\"submit\" value=\"%s\"/></form>",
			c, g_anims[i].name);
	}
	poststr(request, "</td></tr>");
}
int g_ticks = 0;
void PixelAnim_SetAnimQuickTick() {
	if (g_lightEnableAll == 0) {
		// disabled
		return;
	}
	if (g_lightMode != Light_Anim) {
		// disabled
		return;
	}
	if (activeAnim != -1) {
		g_ticks++;
		if (g_ticks >= g_speed) {
			g_anims[activeAnim].runFunc();
			g_ticks = 0;
		}
	}
}



//ENABLE_DRIVER_PIXELANIM
#endif

