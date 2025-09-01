#include "../new_cfg.h"
#include "../new_common.h"
#include "../new_pins.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../mqtt/new_mqtt.h"

#if ENABLE_DRIVER_WS2811_LN882H

#include "drv_leds_shared.h"
#include "drv_local.h"

#include "hal/hal_gpio.h"
#include "hal/hal_dma.h"
#include "hal/hal_ws2811.h"
#include "../hal/ln882h/pin_mapping_ln882h.h"
#include <ln_drv_ws2811.h>

static byte *pixels;
static int g_pixelSize, g_pixelCount;

void WS2811_LN882H_Show() {
	ln_drv_ws2811_send_data(pixels, LED_AMOUNT * 3);
}

byte WS2811_LN882H_GetByte(uint32_t idx) {


}
void WS2811_LN882H_setByte(int idx, byte color) {



}

void WS2811_LN882H_SetLEDCount(int pixel_count, int pixel_size) {
	g_pixelSize = pixel_size;
	g_pixelCount = pixel_count;
	ln_drv_ws2811_init(GPIO_A, GPIO_PIN_7);
	pixels = (byte*)malloc(pixel_count*pixel_size);

}

void WS2811_LN882H_Init() {


	ledStrip_t ws_export;
	ws_export.apply = WS2811_LN882H_Show;
	ws_export.getByte = WS2811_LN882H_GetByte;
	ws_export.setByte = WS2811_LN882H_setByte;
	ws_export.setLEDCount = WS2811_LN882H_SetLEDCount;

	LEDS_InitShared(&ws_export);
}


void WS2811_LN882H_Shutdown() {


	LEDS_ShutdownShared();
}
#endif





