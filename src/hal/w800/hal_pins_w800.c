
#include "../../new_common.h"
#include "../../logging/logging.h"


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

	return 1;
}
void HAL_PIN_SetOutputValue(int index, int iVal) {

}

int HAL_PIN_ReadDigitalInput(int index) {
	return 0;
}
void HAL_PIN_Setup_Input_Pullup(int index) {

}
void HAL_PIN_Setup_Input(int index) {

}
void HAL_PIN_Setup_Output(int index) {

}
void HAL_PIN_PWM_Stop(int index) {

}

void HAL_PIN_PWM_Start(int index) {

}
void HAL_PIN_PWM_Update(int index, int value) {

}

