#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "../hal/hal_pins.h"

/*

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

void RainbowWheel_Run() {
	byte *c;
	uint16_t i;

	for (i = 0; i < pixel_count; i++) {
		c = RainbowWheel_Wheel(((i * 256 / pixel_count) + j) & 255);
		SM16703P_setPixel(pixel_count - 1 - i, *c, *(c + 1), *(c + 2));
	}
	SM16703P_Show();
	j++;
	j %= 256;
}
// startDriver PixelAnim

typedef struct ledAnim_s {
	const char *name;
	void(*runFunc)();
} ledAnim_t;

int activeAnim = -1;
ledAnim_t g_anims[] = {
	{ "Rainbow Wheel", RainbowWheel_Run },
	{ "Test123", RainbowWheel_Run }
};
int g_numAnims = sizeof(g_anims) / sizeof(g_anims[0]);

void PixelAnim_Init() {

}
extern byte g_lightEnableAll;
extern byte g_lightMode;
void PixelAnim_CreatePanel(http_request_t *request) {
	const char* activeStr = "";
	int i;

	if (g_lightMode == Light_Anim) {
		activeStr = "[ACTIVE]";
	}
	poststr(request, "<tr><td>");
	hprintf255(request, "<h5>LED Animation %s</h5>", activeStr);

	for (i = 0; i < g_numAnims; i++) {
		const char* c;
		if (i == activeAnim) {
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
void PixelAnim_Run(int j) {
	activeAnim = j;
	g_lightMode = Light_Anim;
}
void PixelAnim_RunQuickTick() {
	if (g_lightEnableAll == 0) {
		// disabled
		return;
	}
	if (g_lightMode != Light_Anim) {
		// disabled
		return;
	}
	if (activeAnim != -1) {
		g_anims[activeAnim].runFunc();
	}
}


