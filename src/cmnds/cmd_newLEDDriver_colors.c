
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
static byte g_color[][3] = {
	{ 255, 0, 0 },
	{ 0, 255, 0 },
	{ 0, 0, 255 },
	{ 255, 255, 0 },
	{ 255, 0, 255 },
	{ 0, 255, 255 },
};
static byte g_numColors = sizeof(g_color)/sizeof(g_color[0]);

void LED_NextColor() {
	char tmp[8];
	const byte *c;

	g_curColor++;
	g_curColor %= g_numColors;

	c = g_color[g_curColor];

	sprintf(tmp, "%02X%02X%02X",c[0],c[1],c[2]);

	if(LED_GetEnableAll() == 0) {
		LED_SetEnableAll(1);
	}
	LED_SetBaseColor(0,"led_basecolor_rgb",tmp,0);
}

