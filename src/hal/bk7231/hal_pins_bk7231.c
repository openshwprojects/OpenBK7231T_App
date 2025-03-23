
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

// must fit all pwm indexes
static uint32_t g_periods[6];

int PIN_GetPWMIndexForPinIndex(int pin) 
{
	switch(pin)
	{
		case 6:		return 0;
		case 7:		return 1;
		case 8:		return 2;
		case 9:		return 3;
		case 24:	return 4;
		case 26:	return 5;
		default:	return -1;
	}
}

const char *HAL_PIN_GetPinNameAlias(int index) 
{
	// some of pins have special roles
	switch(index)
	{
#ifndef PLATFORM_BK7238
		case 1:		return "RXD2";
		case 10:	return "RXD1";
		case 23:	return "ADC3";
		case 24:	return "PWM4";
		case 26:	return "PWM5";
#else
		case 1:		return "RXD2/ADC5";
		case 10:	return "RXD1/ADC6";
		case 26:	return "PWM5/ADC1";
		case 24:	return "PWM4/ADC2";
		case 20:	return "ADC3";
		case 28:	return "ADC4";
#endif
		case 0:		return "TXD2";
		case 11:	return "TXD1";
		case 6:		return "PWM0";
		case 7:		return "PWM1";
		case 8:		return "PWM2";
		case 9:		return "PWM3";
		default:	return "N/A";
	}
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

void HAL_PIN_PWM_Start(int index, int freq) {
	int pwmIndex;

	pwmIndex = PIN_GetPWMIndexForPinIndex(index);

	// is this pin capable of PWM?
	if(pwmIndex == -1) {
		return;
	}

	uint32_t period = (26000000 / freq);
	g_periods[pwmIndex] = period;
#if defined(PLATFORM_BK7231N) && !defined(PLATFORM_BEKEN_NEW)
	// OSStatus bk_pwm_initialize(bk_pwm_t pwm, uint32_t frequency, uint32_t duty_cycle);
	bk_pwm_initialize(pwmIndex, period, 0, 0, 0);
#else
	bk_pwm_initialize(pwmIndex, period, 0);
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
	uint32_t period = g_periods[pwmIndex];
	uint32_t duty = (value / 100.0 * period); //No need to use upscaled variable
#if defined(PLATFORM_BK7231N) && !defined(PLATFORM_BEKEN_NEW)
	bk_pwm_update_param(pwmIndex, period, duty,0,0);
#else
	bk_pwm_update_param(pwmIndex, period, duty);
#endif
}

unsigned int HAL_GetGPIOPin(int index) {
	return index;
}
