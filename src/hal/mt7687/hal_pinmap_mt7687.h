#if PLATFORM_MT7687

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "hal.h"
#include "hal_platform.h"

typedef struct
{
	const char* name;
	hal_gpio_pin_t pin;
	int8_t pwm_ch;
	uint32_t pwm_total;
	int8_t irq;
} mtPinMapping_t;

extern mtPinMapping_t g_pins[];
extern int g_numPins;

#endif // PLATFORM_MT7687
