#ifdef PLATFORM_RTL87X0C

#include <gpio_api.h>
#include "pwmout_api.h"

typedef struct rtlPinMapping_s
{
	const char* name;
	PinName pin;
	gpio_t* gpio;
	gpio_irq_t* irq;
	pwmout_t* pwm;
} rtlPinMapping_t;

#endif // PLATFORM_RTL87X0C
