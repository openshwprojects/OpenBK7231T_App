#ifdef PLATFORM_BL602

#include "../../new_pins.h"
#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../hal_pins.h"


#include "bl_gpio.h"
#include <bl_pwm.h>

int BL_FindPWMForPin(int index){
	return index % 5;
}


void HAL_PIN_SetOutputValue(int index, int iVal) {
    bl_gpio_output_set(index, iVal ? 1 : 0);
}

const char *HAL_PIN_GetPinNameAlias(int index) {
	return 0;
}
// BL602 - any pin can be pwm
int HAL_PIN_CanThisPinBePWM(int index) {
	return 1;
}
int HAL_PIN_ReadDigitalInput(int index) {
	uint8_t iVal;
    bl_gpio_input_get(index, &iVal);
	return iVal;
}
void HAL_PIN_Setup_Input_Pulldown(int index) {
	bl_gpio_enable_input(index, 0, 1);
}
void HAL_PIN_Setup_Input_Pullup(int index) {
	// int bl_gpio_enable_input(uint8_t pin, uint8_t pullup, uint8_t pulldown);
	bl_gpio_enable_input(index, 1, 0);
}
void HAL_PIN_Setup_Input(int index) {
	// int bl_gpio_enable_input(uint8_t pin, uint8_t pullup, uint8_t pulldown);
	bl_gpio_enable_input(index, 0, 0);
}
void HAL_PIN_Setup_Output(int index) {
	bl_gpio_enable_output(index, 1,0);
	bl_gpio_output_set(index, 0);
}

void HAL_PIN_PWM_Stop(int index) {
	int pwm;

	pwm = BL_FindPWMForPin(index);

	if(pwm == -1) {
		return;
	}
	bl_pwm_stop(pwm);
}

void HAL_PIN_PWM_Start(int index, int freq) {
	int pwm;

	pwm = BL_FindPWMForPin(index);

	if(pwm == -1) {
		return;
	}

	//addLogAdv(LOG_INFO, LOG_FEATURE_MAIN,"HAL_PIN_PWM_Start: pin %i chose pwm %i\r\n",index,pwm);
    //  Frequency must be between 2000 and 800000
	if(freq < 2000) freq = 2000;
	bl_pwm_init(pwm, index, freq);
	bl_pwm_start(pwm);

}
void HAL_PIN_PWM_Update(int index, float value) {
	int pwm;
	float duty;

	pwm = BL_FindPWMForPin(index);

	if(pwm == -1) {
		return;
	}
	if(value<0)
		value = 0;
	if(value>100)
		value = 100;
	duty = value;
/*duty is the PWM Duty Cycle (0 to 100). When duty=25, it means that in every PWM Cycle...
--> PWM Ouput is 1 (High) for the initial 25% of the PWM Cycle
--> Followed by PWM Output 0 (Low) for the remaining 75% of the PWM Cycle
*/

	//addLogAdv(LOG_INFO, LOG_FEATURE_MAIN,"HAL_PIN_PWM_Update: pin %i had pwm %i, set %i\r\n",index,pwm,value);
	bl_pwm_set_duty(pwm, duty);

}

unsigned int HAL_GetGPIOPin(int index) {
	return index;
}

OBKInterruptHandler g_handlers[PLATFORM_GPIO_MAX];
OBKInterruptType g_modes[PLATFORM_GPIO_MAX];

#include "hal_gpio.h"

void BL602_Interrupt(gpio_ctx_t* context) {
	int obkPinNum = (int)context->arg;
	if (g_handlers[obkPinNum]) {
		g_handlers[obkPinNum](obkPinNum);
	}
	bl_gpio_intmask(obkPinNum, 0);
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {
	g_handlers[pinIndex] = function;
	int bl_mode;
	switch(mode)
	{
		case INTERRUPT_RISING: bl_mode = GPIO_INT_TRIG_POS_PULSE; break;
		case INTERRUPT_FALLING: bl_mode = GPIO_INT_TRIG_NEG_PULSE; break;
		default: bl_mode = GPIO_INT_TRIG_NEG_PULSE; break;
	}
	hal_gpio_register_handler(BL602_Interrupt, pinIndex,
		GPIO_INT_CONTROL_ASYNC, bl_mode, (void*)pinIndex);
}
void HAL_DetachInterrupt(int pinIndex) {
	if (g_handlers[pinIndex] == 0) {
		return; // already removed;
	}
	g_handlers[pinIndex] = 0;
}

#endif
