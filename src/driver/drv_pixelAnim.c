#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "../hal/hal_pins.h"

int numLEDs = 16;

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
// startDriver PixelAnim

void PixelAnim_Init() {

}
void PixelAnim_RunQuickTick() {
	byte *c;
	uint16_t i;

	for (i = 0; i < numLEDs; i++) {
		c = RainbowWheel_Wheel(((i * 256 / numLEDs) + j) & 255);
		SM16703P_setPixel(numLEDs - 1 - i,*c, *(c + 1), *(c + 2));
	}
	SM16703P_Show();
	j++;
	j %= 256;


}


