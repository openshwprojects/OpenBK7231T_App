#ifdef PLATFORM_BL602

#include "../../new_pins.h"
#include "../../new_common.h"
#include "../../logging/logging.h"


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
	
	pwm = BL_FindPWMForPin(index);

	if(pwm == -1) {
		return;
	}
	
	//addLogAdv(LOG_INFO, LOG_FEATURE_MAIN,"HAL_PIN_PWM_Start: pin %i chose pwm %i\r\n",index,pwm);
    //  Frequency must be between 2000 and 800000
	bl_pwm_init(pwm, index, 2000);
	bl_pwm_start(pwm);

}
void HAL_PIN_PWM_Update(int index, int value) {
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


#endif
