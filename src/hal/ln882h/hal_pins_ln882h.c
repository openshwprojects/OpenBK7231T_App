
#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"


int PIN_GetPWMIndexForPinIndex(int pin) {
	return -1;
}

const char *HAL_PIN_GetPinNameAlias(int index) {
	
	return "N/A";
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
void HAL_PIN_Setup_Input_Pulldown(int index) {
}
void HAL_PIN_Setup_Input(int index) {
}
void HAL_PIN_Setup_Output(int index) {
}
void HAL_PIN_PWM_Stop(int index) {

}

void HAL_PIN_PWM_Start(int index) {
	
}
void HAL_PIN_PWM_Update(int index, float value) {

}

unsigned int HAL_GetGPIOPin(int index) {
	return index;
}
