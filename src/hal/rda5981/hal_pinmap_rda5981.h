#ifdef PLATFORM_RDA5981

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include <gpio_api.h>
#include "pwmout_api.h"
#include "gpio_irq_api.h"

typedef struct rdaPinMapping_s
{
	const char* name;
	PinName pin;
	gpio_t* gpio;
	gpio_irq_t* irq;
	pwmout_t* pwm;
} rdaPinMapping_t;

extern rdaPinMapping_t g_pins[];
extern int g_numPins;

#endif // PLATFORM_RDA5981
