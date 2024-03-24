
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
#if ENABLE_LITTLEFS
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
	// 0 = idk?
	// CHANGED NOW. LET'S ASSUME THAT INDEXING STARTS AT 0
	// SO 0 = RED, 1 = GREEN, 2 = BLUE...
	//{ 125, 0, 255 },
	// 0 = red
	{ 255, 0, 0 }, 
	// 1 = green
	{ 0, 255, 0 },
	// 2 = blue
	{ 0, 0, 255 },
	// 3 = orange
	{ 255, 165, 0 },
	// 4 = light green
	{ 144, 238, 144 },
	// 5 = light blue
	{ 173, 216, 230 },
	// 6 = amber
	{ 255, 191, 0 },
	// 7 = cyan
	{ 0, 255, 255 },
	// 8 = purple
	{ 221, 160, 221 },
	// 9 = yellow
	{ 255, 255, 153 },
	// 10 = pink
	{ 255, 192, 203 },
	// 11 = white (using RGB channels CT 153)
	{ 255, 255, 255 },
	// 12	= 	COLD WHITE (CT 153)
#define SPECIAL_INDEX_CT153				12
	// 13	= WARM white (CT 500)
#define SPECIAL_INDEX_CT500				13
	// 14	= CT 327
#define SPECIAL_INDEX_CT327				13

};
// https://www.elektroda.com/rtvforum/viewtopic.php?p=20280817#20280817
static byte g_numColors = sizeof(g_color)/sizeof(g_color[0]);

commandResult_t commandSetPaletteColor(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int index;
	const char *s;

	// Use tokenizer, so we can use variables (eg. $CH11 as variable)
	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	index = Tokenizer_GetArgInteger(0);
	// string is in format like FF00FF
	s = Tokenizer_GetArg(1);
	// convert 0xFF00AA to FF00AA
	if (s[1] == 'x') {
		s += 2;
	}
	sscanf(s, "%02hhx%02hhx%02hhx", 
		&g_color[index][0], &g_color[index][1], &g_color[index][2]);

	return CMD_RES_OK;
}

void LED_SetColorByIndex(int index) {
	char tmp[8];
	const byte *c;

	// special CT indices
	if (index == SPECIAL_INDEX_CT500) {
		LED_SetTemperature(500, true);
		if (LED_GetEnableAll() == 0) {
			LED_SetEnableAll(1);
		}
		return;
	}
	if (index == SPECIAL_INDEX_CT327) {
		LED_SetTemperature(327, true);
		if (LED_GetEnableAll() == 0) {
			LED_SetEnableAll(1);
		}
		return;
	}
	if (index == SPECIAL_INDEX_CT153) {
		LED_SetTemperature(153, true);
		if (LED_GetEnableAll() == 0) {
			LED_SetEnableAll(1);
		}
		return;
	}
	// roll indices
	if (index < 0)
		index = g_numColors - 1;
	if (index >= g_numColors)
		index = 0;

	g_curColor = index;

	c = g_color[g_curColor];

	snprintf(tmp, sizeof(tmp), "%02X%02X%02X", c[0], c[1], c[2]);

	LED_SetBaseColor(0, "led_basecolor_rgb", tmp, 0);
	if (LED_GetEnableAll() == 0) {
		LED_SetEnableAll(1);
	}
}
void LED_NextColor() {

	g_curColor++;
	g_curColor %= g_numColors;

	LED_SetColorByIndex(g_curColor);
}

