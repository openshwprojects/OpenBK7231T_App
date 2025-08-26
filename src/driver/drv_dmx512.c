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

void DMX_Show() {
}

byte DMX_GetByte(uint32_t idx) {
	byte ret = 0;
	return ret;
}
void DMX_setByte(int index, byte color) {

}

void DMX_SetLEDCount(int pixel_count, int pixel_size) {

}

void DMX_Init() {

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




