#ifdef PLATFORM_REALTEK

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
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

extern rtlPinMapping_t g_pins[];
extern int g_numPins;

#endif // PLATFORM_REALTEK
