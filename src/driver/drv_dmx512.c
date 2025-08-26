#include "../new_cfg.h"
#include "../new_common.h"
#include "../new_pins.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../mqtt/new_mqtt.h"

#if ENABLE_DRIVER_DMX

#include "drv_leds_shared.h"
#include "drv_local.h"

#define DMX_BUFFER_SIZE 512

static byte *g_dmxBuffer;
static int dmx_pixelCount = 0;
static int dmx_pixelSize = 3;

void DMX_Show() {
}

byte DMX_GetByte(uint32_t idx) {
	if (idx >= DMX_BUFFER_SIZE)
		return 0;
	return g_dmxBuffer[idx];
}
void DMX_setByte(int idx, byte color) {
	if (idx >= DMX_BUFFER_SIZE)
		return 0;
	g_dmxBuffer[idx] = color;
}

void DMX_SetLEDCount(int pixel_count, int pixel_size) {
	dmx_pixelCount = pixel_count;
	dmx_pixelSize = pixel_size;
}

void DMX_Init() {
	g_dmxBuffer = (byte*)malloc(DMX_BUFFER_SIZE);
	memset(g_dmxBuffer, DMX_BUFFER_SIZE, 0);
	ledStrip_t ws_export;
	ws_export.apply = DMX_Show;
	ws_export.getByte = DMX_GetByte;
	ws_export.setByte = DMX_setByte;
	ws_export.setLEDCount = DMX_SetLEDCount;

	LEDS_InitShared(&ws_export);
}
void DMX_OnEverySecond() {

}


void DMX_Shutdown() {
	LEDS_ShutdownShared();
}
#endif




