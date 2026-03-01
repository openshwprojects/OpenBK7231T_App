#if PLATFORM_LN882H || PLATFORM_LN8825

#include "hal/hal_gpio.h"

#if PLATFORM_LN882H

typedef struct lnPinMapping_s
{
	const char* name;
	uint32_t base;
	gpio_pin_t pin;
	int8_t pwm_cha;
} lnPinMapping_t;

#else
#include "hal/syscon_types.h"

typedef struct lnPinMapping_s
{
	const char* name;
	GPIO_Num pin;
	int8_t pwm_cha;
} lnPinMapping_t;

#endif

extern lnPinMapping_t g_pins[];
extern int g_numPins;

#endif // PLATFORM_LN882H
