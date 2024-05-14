// for full presentation, see related topic:
// https://www.elektroda.com/rtvforum/topic4054134.html
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"
#include "../quicktick.h"

#define SPLIT_COLOR(x) (x >> 16) & 0xFF,(x >> 8) & 0xFF,x & 0xFF

static int *g_timeOuts = 0;
static int g_numLEDs = 0;
static int g_on_color = 8900331;
static int g_off_color = 0;
static int g_on_timeout_ms;
static int g_changes = 0;
static bool g_bEnableAmbient = 0;
static int g_ambient_color = 0xFF0000;

// http://127.0.0.1/led_index?params=5
static int DR_LedIndex(http_request_t* request) {
	char tmp[8];
	if (!http_getArg(request->url, "params", tmp, sizeof(tmp))) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "DR_LedIndex: missing params\n");
		return 0;
	}
	int index = atoi(tmp);
	g_timeOuts[index] = g_on_timeout_ms;
#if ENABLE_DRIVER_SM16703P
	SM16703P_setPixel(index, SPLIT_COLOR(g_on_color));
#endif
	g_changes++;
	return 0;
}
static void applyAmbient() {
	int c = 0;
	if (g_bEnableAmbient) {
		c = g_ambient_color;
	}
	g_changes++;
	for (int i = 0; i < g_numLEDs; i++) {
#if ENABLE_DRIVER_SM16703P
		SM16703P_setPixel(i, SPLIT_COLOR(c));
#endif
	}
}
static int DR_LedEnableAmbient(http_request_t* request) {
	char tmp[16];
	if (!http_getArg(request->url, "params", tmp, sizeof(tmp))) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "DR_LedEnableAmbient: missing params\n");
		return 0;
	}
	g_bEnableAmbient = atoi(tmp);
	applyAmbient();
	return 0;
}
static int DR_LedAmbientColor(http_request_t* request) {
	char tmp[16];
	if (!http_getArg(request->url, "params", tmp, sizeof(tmp))) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "DR_LedAmbientColor: missing params\n");
		return 0;
	}
	g_ambient_color = atoi(tmp);
	applyAmbient();
	return 0;
}
static int DR_LedOnColor(http_request_t* request) {
	char tmp[16];
	if (!http_getArg(request->url, "params", tmp, sizeof(tmp))) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "DR_LedOnColor: missing params\n");
		return 0;
	}
	g_on_color = atoi(tmp);
	return 0;
}
static int DR_LedOffColor(http_request_t* request) {
	char tmp[16];
	if (!http_getArg(request->url, "params", tmp, sizeof(tmp))) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "DR_LedOffColor: missing params\n");
		return 0;
	}
	g_off_color = atoi(tmp);
	return 0;
}
static int DR_LedOnTimeout(http_request_t* request) {
	char tmp[16];
	if (!http_getArg(request->url, "params", tmp, sizeof(tmp))) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "DR_LedOnTimeout: missing params\n");
		return 0;
	}
	g_on_timeout_ms = atoi(tmp);
	return 0;
}
void Drawers_Init() {

	/*
	// Usage:
	startDriver SM16703P
	SM16703P_Init 60
	// All arguments are optional
	// startDriver Drawers [NumLEDs] [TimeoutMS] [OnColor] [OffColor] [AmbientColor]
	startDriver Drawers 60 2000 0x00FF00

	*/
	g_numLEDs = Tokenizer_GetArgIntegerDefault(1, 128);
	g_on_timeout_ms = Tokenizer_GetArgIntegerDefault(2, 1000);
	g_on_color = Tokenizer_GetArgIntegerDefault(3, 8900331);
	g_off_color = Tokenizer_GetArgIntegerDefault(4, 0);
	g_ambient_color = Tokenizer_GetArgIntegerDefault(5, 0);
	
	if (g_timeOuts) {
		free(g_timeOuts);
	}
	g_timeOuts = (int*)malloc(sizeof(int)*g_numLEDs);

	// turns on the LED
	// http://192.168.0.123/led_index?params=4
	HTTP_RegisterCallback("/led_index", HTTP_GET, DR_LedIndex, 1);
	HTTP_RegisterCallback("/led_index", HTTP_POST, DR_LedIndex, 1);
	// sets the LED on color for all LEDs
	// http://192.168.0.123/led_on_color?params=0xFF00FF
	HTTP_RegisterCallback("/led_on_color", HTTP_GET, DR_LedOnColor, 1);
	HTTP_RegisterCallback("/led_on_color", HTTP_POST, DR_LedOnColor, 1);
	// sets the LED off color for all LEDs
	// http://192.168.0.123/led_off_color?params=0x00FF00
	HTTP_RegisterCallback("/led_off_color", HTTP_GET, DR_LedOffColor, 1);
	HTTP_RegisterCallback("/led_off_color", HTTP_POST, DR_LedOffColor, 1);
	// sets the LED on timeout for all LEDs
	// http://192.168.0.123/led_on_timeout?params=5000
	HTTP_RegisterCallback("/led_on_timeout", HTTP_GET, DR_LedOnTimeout, 1);
	HTTP_RegisterCallback("/led_on_timeout", HTTP_POST, DR_LedOnTimeout, 1);
	// 
	// http://192.168.0.123/enable_ambient?params=1
	HTTP_RegisterCallback("/enable_ambient", HTTP_GET, DR_LedEnableAmbient, 1);
	HTTP_RegisterCallback("/enable_ambient", HTTP_POST, DR_LedEnableAmbient, 1);
	// sets the LED ambient color for all LEDs
	// http://192.168.0.123/led_ambient?params=0xFFFFFF
	HTTP_RegisterCallback("/led_ambient", HTTP_GET, DR_LedAmbientColor, 1);
	HTTP_RegisterCallback("/led_ambient", HTTP_POST, DR_LedAmbientColor, 1);
	
}

void Drawers_QuickTick() {
	//SM16703P_setAllPixels(SPLIT_COLOR(g_on_color));

	for (int i = 0; i < g_numLEDs; i++) {
		if (g_timeOuts[i] > 0) {
			g_timeOuts[i] -= g_deltaTimeMS;
			if (g_timeOuts[i] <= 0) {
#if ENABLE_DRIVER_SM16703P
				SM16703P_setPixel(i,SPLIT_COLOR(g_off_color));
#endif
				g_timeOuts[i] = 0;
				g_changes++;
			}
		}
	}
	if (g_changes) {
#if ENABLE_DRIVER_SM16703P
		SM16703P_Show();
#else
		ADDLOG_INFO(LOG_FEATURE_CMD, "ERROR! Drawers driver requires SM16703P\n");
#endif
		g_changes = 0;
	}
}


