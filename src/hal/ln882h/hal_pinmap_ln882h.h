#ifdef PLATFORM_LN882H

#include "hal/hal_gpio.h"

typedef struct lnPinMapping_s
{
	const char* name;
	uint32_t base;
	gpio_pin_t pin;
	int8_t pwm_cha;
} lnPinMapping_t;

extern lnPinMapping_t g_pins[];
extern int g_numPins;

#endif // PLATFORM_LN882H
