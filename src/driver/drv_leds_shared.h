#ifndef __DRV_LED_SHARED_H__
#define __DRV_LED_SHARED_H__

#include "../new_cfg.h"
#include "../new_common.h"
#include "../new_pins.h"

typedef struct ledStrip_s {
	byte (*getByte)(uint32_t pixel);
	void (*setByte)(uint32_t idx, byte val);
	void (*apply)();
	void (*setLEDCount)(int pixel_count, int pixel_size);
} ledStrip_t;

typedef enum ColorChannel {
	COLOR_CHANNEL_RED,
	COLOR_CHANNEL_GREEN,
	COLOR_CHANNEL_BLUE,
	COLOR_CHANNEL_COLD_WHITE,
	COLOR_CHANNEL_WARM_WHITE
} ColorChannel_t;

void LEDS_InitShared(ledStrip_t *api);
void LEDS_ShutdownShared();

#endif
