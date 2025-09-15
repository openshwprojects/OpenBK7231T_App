#include "../new_cfg.h"
#include "../new_common.h"
#include "../new_pins.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../mqtt/new_mqtt.h"

#if ENABLE_DRIVER_SM16703P

#include "drv_leds_shared.h"
#include "drv_local.h"
#include "drv_spiLED.h"

void SM16703P_Show() {
	if (spiLED.ready == 0)
		return;
	SPIDMA_StartTX(spiLED.msg);
}

byte SM16703P_GetByte(uint32_t idx) {
	if (spiLED.msg == 0)
		return 0;
	if (spiLED.ready == 0)
		return 0;
	byte *input = spiLED.buf + spiLED.ofs + idx * 4;
	byte ret = reverse_translate_byte(input);
	return ret;
}
void SM16703P_setByte(int index, byte color) {
	if (spiLED.buf == 0)
		return;
	if (spiLED.ready == 0)
		return;
	translate_byte(color, spiLED.buf + (spiLED.ofs + index * 4));
}

void SM16703P_SetLEDCount(int pixel_count, int pixel_size) {
	// Third arg (optional, default "0"): spiLED.ofs to prepend to each transmission
	if (Tokenizer_GetArgsCount() > 2) {
		spiLED.ofs = Tokenizer_GetArgIntegerRange(2, 0, 255);
	}
	// Fourth arg (optional, default "64"): spiLED.padding to append to each transmission
	//spiLED.padding = 64;
	//if (Tokenizer_GetArgsCount() > 3) {
	//	spiLED.padding = Tokenizer_GetArgIntegerRange(3, 0, 255);
	//}
	// each pixel is RGB, so 3 bytes per pixel
	SPILED_InitDMA(pixel_count * pixel_size);
}

// startDriver SM16703P
// backlog startDriver SM16703P; SM16703P_Test
void SM16703P_Init() {

	int pin = PIN_FindPinIndexForRole(IOR_SM16703P_DIN, -1);
	SPILED_Init(pin);

	ledStrip_t ws_export;
	ws_export.apply = SM16703P_Show;
	ws_export.getByte = SM16703P_GetByte;
	ws_export.setByte = SM16703P_setByte;
	ws_export.setLEDCount = SM16703P_SetLEDCount;

	LEDS_InitShared(&ws_export);
}

void SM16703P_Shutdown() {
	LEDS_ShutdownShared();
}
#endif
