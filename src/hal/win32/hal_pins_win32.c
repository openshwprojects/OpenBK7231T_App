#ifdef WINDOWS

#include "../../new_common.h"
#include "../../logging/logging.h"


const char *HAL_PIN_GetPinNameAlias(int index) {

	return 0;
}



// XR809 - PWM todo
int HAL_PIN_CanThisPinBePWM(int index) {
	return 0;
}
void HAL_PIN_SetOutputValue(int index, int iVal) {

}

int HAL_PIN_ReadDigitalInput(int index) {

	return 1;
}
void HAL_PIN_Setup_Input_Pullup(int index) {

}
void HAL_PIN_Setup_Input(int index) {

}

void HAL_PIN_Setup_Output(int index) {

}


void HAL_PIN_PWM_Stop(int pinIndex) {
}

void HAL_PIN_PWM_Start(int index) {

}
void HAL_PIN_PWM_Update(int index, int value) {


}



#endif

