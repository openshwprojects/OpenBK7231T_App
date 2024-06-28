// freeze
#include "drv_local.h"
#include "../new_common.h"

#if PLATFORM_BEKEN

extern "C" {

#include "include.h"
#include "arm_arch.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../obk_config.h"
#include "../cmnds/cmd_public.h"
#include "bk_timer_pub.h"
#include "drv_model_pub.h"

#include <gpio_pub.h>
//#include "pwm.h"
#include "pwm_pub.h"

#include "../../beken378/func/include/net_param_pub.h"
#include "../../beken378/func/user_driver/BkDriverPwm.h"
#include "../../beken378/func/user_driver/BkDriverI2c.h"
#include "../../beken378/driver/i2c/i2c1.h"
#include "../../beken378/driver/gpio/gpio.h"

#endif

void IR2_Init() {
	int txpin = 26;
	int pwmIndex = PIN_GetPWMIndexForPinIndex(txpin);
	// is this pin capable of PWM?
	if (pwmIndex != -1) {
		uint32_t pwmfrequency = 38000;
		uint32_t period = (26000000 / pwmfrequency);
		uint32_t duty = period / 2;
#if PLATFORM_BK7231N
		// OSStatus bk_pwm_initialize(bk_pwm_t pwm, uint32_t frequency, uint32_t duty_cycle);
		bk_pwm_initialize((bk_pwm_t)pwmIndex, period, duty, 0, 0);
#else
		bk_pwm_initialize((bk_pwm_t)pwmIndex, period, duty);
#endif
		bk_pwm_start((bk_pwm_t)pwmIndex);
	}
}



