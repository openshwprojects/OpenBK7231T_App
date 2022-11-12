
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "cmd_public.h"
#include "../obk_config.h"
#include "../driver/drv_public.h"
#include "../hal/hal_flashVars.h"
#include "../rgb2hsv.h"
#include <ctype.h>
#include "cmd_local.h"
#include "../mqtt/new_mqtt.h"
#include "../cJSON/cJSON.h"
#ifdef BK_LITTLEFS
	#include "../littlefs/our_lfs.h"
#endif

static byte g_curColor = 0;
// TODO: make this list consistent with
// Tasmota colors standard:
/*
Set color to
1 = red
2 = green
3 = blue
4 = orange
5 = light green
6 = light blue
7 = amber
8 = cyan
9 = purple
10 = yellow
11 = pink
12 = white (using RGB channels)
+ = next color
- = previous color
*/
static byte g_color[][3] = {
	{ 255, 0, 0 },
	{ 0, 255, 0 },
	{ 0, 0, 255 },
	{ 255, 255, 0 },
	{ 255, 0, 255 },
	{ 0, 255, 255 },
};
static byte g_numColors = sizeof(g_color)/sizeof(g_color[0]);


void LED_SetColorByIndex(int index) {
	char tmp[8];
	const byte *c;

	if (index < 0)
		index = g_numColors - 1;
	if (index >= g_numColors)
		index = 0;

	c = g_color[g_curColor];

	sprintf(tmp, "%02X%02X%02X", c[0], c[1], c[2]);

	if (LED_GetEnableAll() == 0) {
		LED_SetEnableAll(1);
	}
	LED_SetBaseColor(0, "led_basecolor_rgb", tmp, 0);
}
void LED_NextColor() {

	g_curColor++;
	g_curColor %= g_numColors;

	LED_SetColorByIndex(g_curColor);
}

