
#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
//#include "../../new_pins.h"
#include <gpio_pub.h>

#include "../../beken378/func/include/net_param_pub.h"
#include "../../beken378/func/user_driver/BkDriverPwm.h"
#include "../../beken378/func/user_driver/BkDriverI2c.h"
#include "../../beken378/driver/i2c/i2c1.h"
#include "../../beken378/driver/gpio/gpio.h"

//100hz to 20000hz according to tuya code
#define PWM_FREQUENCY_SLOW 600 //Slow frequency for LED Drivers requiring slower PWM Freq

extern int g_pwmFrequency;

int PIN_GetPWMIndexForPinIndex(int pin) {
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
	// some of pins have special roles
	if (index == 23)
		return "ADC3";
	if (index == 26)
		return "PWM5";
	if (index == 24)
		return "PWM4";
	if (index == 6)
		return "PWM0";
	if (index == 7)
		return "PWM1";
	if (index == 0)
		return "TXD2";
	if (index == 1)
		return "RXD2";
	if (index == 9)
		return "PWM3";
	if (index == 8)
		return "PWM2";
	if (index == 10)
		return "RXD1";
	if (index == 11)
		return "TXD1";
	return "N/A";
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
void HAL_PIN_Setup_Input_Pulldown(int index) {
	bk_gpio_config_input_pdwn(index);
}
void HAL_PIN_Setup_Input(int index) {
	bk_gpio_config_input(index);
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
	int useFreq;

	pwmIndex = PIN_GetPWMIndexForPinIndex(index);

	// is this pin capable of PWM?
	if(pwmIndex == -1) {
		return;
	}
	useFreq = g_pwmFrequency;
	//Use slow pwm if user has set checkbox in webif
	if(CFG_HasFlag(OBK_FLAG_SLOW_PWM))
		useFreq = PWM_FREQUENCY_SLOW;

	uint32_t frequency = (26000000 / useFreq);
#if PLATFORM_BK7231N
	// OSStatus bk_pwm_initialize(bk_pwm_t pwm, uint32_t frequency, uint32_t duty_cycle);
	bk_pwm_initialize(pwmIndex, frequency, 0, 0, 0);
#else
	bk_pwm_initialize(pwmIndex, frequency, 0);
#endif
	bk_pwm_start(pwmIndex);
}
void HAL_PIN_PWM_Update(int index, float value) {
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

	//uint32_t value_upscaled = value * 10.0f; //Duty cycle 0...100 -> 0...1000
	uint32_t period = (26000000 / g_pwmFrequency); //TODO: Move to global variable and set in init func so it does not have to be recalculated every time...
	uint32_t duty = (value / 100.0 * period); //No need to use upscaled variable
#if PLATFORM_BK7231N
	bk_pwm_update_param(pwmIndex, period, duty,0,0);
#else
	bk_pwm_update_param(pwmIndex, period, duty);
#endif
}

unsigned int HAL_GetGPIOPin(int index) {
	return index;
}
