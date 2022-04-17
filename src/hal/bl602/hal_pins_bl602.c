#ifdef PLATFORM_BL602

#include "../../new_common.h"
#include "../../logging/logging.h"


#include "bl_gpio.h"
#include <bl_pwm.h>

#define MAX_BL_PWMS 5

static int bl_pwms[MAX_BL_PWMS] = { -1, -1, -1, -1, -1 };

static int BL_FindPWMForPin(int pin) {
	int i;

	// search
	for(i = 0; i < MAX_BL_PWMS; i++) {
		if(bl_pwms[i] == pin)
			return i;
	}
	// fail
	return -1;
}
static int BL_RegisterPWMForPin(int pin) {
	int i;

	// search
	for(i = 0; i < MAX_BL_PWMS; i++) {
		if(bl_pwms[i] == pin)
			return i;
	}
	// create
	for(i = 0; i < MAX_BL_PWMS; i++) {
		if(bl_pwms[i] == -1) {
			bl_pwms[i] = pin;
			return i;
		}
	}
	// fail
	return -1;
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
void HAL_PIN_Setup_Input_Pullup(int index) {
	// int bl_gpio_enable_input(uint8_t pin, uint8_t pullup, uint8_t pulldown);
	bl_gpio_enable_input(index, 1, 0);
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

void HAL_PIN_PWM_Start(int index) {
	int pwm;
	
	pwm = BL_RegisterPWMForPin(index);

	if(pwm == -1) {
		return;
	}
	bl_pwm_init(pwm, index, 1000);
	bl_pwm_start(pwm);

}
void HAL_PIN_PWM_Update(int index, int value) {
	int pwm;
	float duty;

	pwm = BL_FindPWMForPin(index);

	if(pwm == -1) {
		return;
	}
	duty = value * 0.01f;
	bl_pwm_set_duty(pwm, duty);

}


#endif
