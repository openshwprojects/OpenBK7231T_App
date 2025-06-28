#if PLATFORM_XRADIO

#include "driver/chip/hal_adc.h"
#include "driver/chip/hal_gpio.h"
#include "driver/chip/hal_def.h"
#include "driver/chip/hal_pwm.h"

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"

typedef struct
{
	const char* name;
	int port;
	int pin;
	ADC_Channel adc;
	PWM_CH_ID pwm;
	uint8_t pinmux_pwm;
	int max_duty;
	int wakeup;
} xrpin_t;

extern xrpin_t g_pins[];
extern int g_numPins;

#endif // PLATFORM_XRADIO
