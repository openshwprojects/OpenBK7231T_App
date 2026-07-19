#ifdef PLATFORM_GD32VW553

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "gd32vw55x_gpio.h"

typedef struct gd32PinMapping_s
{
	const char* name;
	uint32_t gpio;
	uint32_t pin;
	uint32_t pwmtimer;
	uint16_t pwmchannel;
	uint32_t pwmaf;
} gd32PinMapping_t;

extern gd32PinMapping_t g_pins[];
extern int g_numPins;

#endif // PLATFORM_GD32VW553
