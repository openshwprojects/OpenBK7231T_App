
#include "../../new_common.h"
#include "../../logging/logging.h"

#include <gpio_pub.h>

#include "../../beken378/func/include/net_param_pub.h"
#include "../../beken378/func/user_driver/BkDriverPwm.h"
#include "../../beken378/func/user_driver/BkDriverI2c.h"
#include "../../beken378/driver/i2c/i2c1.h"
#include "../../beken378/driver/gpio/gpio.h"


static int PIN_GetPWMIndexForPinIndex(int pin) {
	if(pin == 6)
		return 0;
	if(pin == 7)
		return 1;
	if(pin == 8)
		return 2;
	if(pin == 9)
		return 3;
	if(pin == 24)
		return 4;
	if(pin == 26)
		return 5;
	return -1;
}

const char *HAL_PIN_GetPinNameAlias(int index) {
	return 0;
}

int HAL_PIN_CanThisPinBePWM(int index) {
	int pwmIndex = PIN_GetPWMIndexForPinIndex(index);
	if(pwmIndex == -1)
		return 0;
	return 1;
}
void HAL_PIN_SetOutputValue(int index, int iVal) {
	bk_gpio_output(index, iVal);
}

int HAL_PIN_ReadDigitalInput(int index) {
	return bk_gpio_input(index);
}
void HAL_PIN_Setup_Input_Pullup(int index) {
	bk_gpio_config_input_pup(index);
}
void HAL_PIN_Setup_Output(int index) {
	bk_gpio_config_output(index);
	bk_gpio_output(index, 0);
}
void HAL_PIN_PWM_Stop(int index) {
	int pwmIndex;

	pwmIndex = PIN_GetPWMIndexForPinIndex(index);

	// is this pin capable of PWM?
	if(pwmIndex == -1) {
		return;
	}

	bk_pwm_stop(pwmIndex);
}

void HAL_PIN_PWM_Start(int index) {
	int pwmIndex;

	pwmIndex = PIN_GetPWMIndexForPinIndex(index);

	// is this pin capable of PWM?
	if(pwmIndex == -1) {
		return;
	}

	// they are using 1kHz PWM
	// See: https://www.elektroda.pl/rtvforum/topic3798114.html
#if PLATFORM_BK7231N
	// OSStatus bk_pwm_initialize(bk_pwm_t pwm, uint32_t frequency, uint32_t duty_cycle);
	bk_pwm_initialize(pwmIndex, 1000, 0, 0, 0);
#else
	bk_pwm_initialize(pwmIndex, 1000, 0);
#endif
	bk_pwm_start(pwmIndex);
}
void HAL_PIN_PWM_Update(int index, int value) {
	int pwmIndex;

	pwmIndex = PIN_GetPWMIndexForPinIndex(index);

	// is this pin capable of PWM?
	if(pwmIndex == -1) {
		return;
	}
	if(value<0)
		value = 0;
	if(value>100)
		value = 100;
#if PLATFORM_BK7231N
	bk_pwm_update_param(pwmIndex, 1000, value * 10.0f,0,0); // Duty cycle 0...100 * 10.0 = 0...1000
#else
	bk_pwm_update_param(pwmIndex, 1000, value * 10.0f); // Duty cycle 0...100 * 10.0 = 0...1000
#endif
}

