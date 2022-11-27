#ifdef WINDOWS

#include "../../new_common.h"
#include "../../new_pins.h"
#include "../../logging/logging.h"

int g_simulatedPinStates[PLATFORM_GPIO_MAX];
bool g_bInput[PLATFORM_GPIO_MAX];

void SIM_SetSimulatedPinValue(int pinIndex, bool bHigh) {
	g_simulatedPinStates[pinIndex] = bHigh;
}
bool SIM_GetSimulatedPinValue(int pinIndex) {
	return g_simulatedPinStates[pinIndex];
}
bool SIM_IsPinInput(int index) {
	return g_bInput[index];
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
	return 0;
}
void HAL_PIN_SetOutputValue(int index, int iVal) {
	g_simulatedPinStates[index] = iVal;
}

int HAL_PIN_ReadDigitalInput(int index) {
	return g_simulatedPinStates[index];
}
void HAL_PIN_Setup_Input_Pullup(int index) {
	g_bInput[index] = true;
}
void HAL_PIN_Setup_Input(int index) {
	g_bInput[index] = true;
}

void HAL_PIN_Setup_Output(int index) {
	g_bInput[index] = false;
}


void HAL_PIN_PWM_Stop(int pinIndex) {
}

void HAL_PIN_PWM_Start(int index) {
	g_bInput[index] = false;
}
void HAL_PIN_PWM_Update(int index, int value) {


}



#endif

