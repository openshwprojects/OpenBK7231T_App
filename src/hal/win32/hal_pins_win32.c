#ifdef WINDOWS

#include "../../new_common.h"
#include "../../new_pins.h"
#include "../../logging/logging.h"

int g_simulatedPinStates[PLATFORM_GPIO_MAX];

void SIM_SetSimulatedPinValue(int pinIndex, bool bHigh) {
	g_simulatedPinStates[pinIndex] = bHigh;
}

const char *HAL_PIN_GetPinNameAlias(int index) {

	return 0;
}


int HAL_PIN_CanThisPinBePWM(int index) {
	return 0;
}
void HAL_PIN_SetOutputValue(int index, int iVal) {
	g_simulatedPinStates[index] = iVal;
}

int HAL_PIN_ReadDigitalInput(int index) {
	return g_simulatedPinStates[index];
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

