#ifdef WINDOWS

#include "../../new_common.h"
#include "../../new_pins.h"
#include "../../logging/logging.h"


typedef enum simulatedPinMode_e {
	SIM_PIN_NONE,
	SIM_PIN_OUTPUT,
	SIM_PIN_PWM,
	SIM_PIN_INPUT_PULLUP,
	SIM_PIN_INPUT,
} simulatedPinMode_t;

int g_simulatedPinStates[PLATFORM_GPIO_MAX];
int g_simulatedPWMs[PLATFORM_GPIO_MAX];
simulatedPinMode_t g_pinModes[PLATFORM_GPIO_MAX];

void SIM_SetSimulatedPinValue(int pinIndex, bool bHigh) {
	g_simulatedPinStates[pinIndex] = bHigh;
}
bool SIM_GetSimulatedPinValue(int pinIndex) {
	return g_simulatedPinStates[pinIndex];
}
bool SIM_IsPinInput(int index) {
	if (g_pinModes[index] == SIM_PIN_INPUT_PULLUP)
		return true;
	if (g_pinModes[index] == SIM_PIN_INPUT)
		return true;
	return false;
}
bool SIM_IsPinPWM(int index) {
	if (g_pinModes[index] == SIM_PIN_PWM)
		return true;
	return false;
}
int SIM_GetPWMValue(int index) {
	return g_simulatedPWMs[index];
}



int PIN_GetPWMIndexForPinIndex(int pin) {
	if (pin == 6)
		return 0;
	if (pin == 7)
		return 1;
	if (pin == 8)
		return 2;
	if (pin == 9)
		return 3;
	if (pin == 24)
		return 4;
	if (pin == 26)
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
	if (pwmIndex == -1)
		return 0;
	return 1;
}
void HAL_PIN_SetOutputValue(int index, int iVal) {
	g_simulatedPinStates[index] = iVal;
}

int HAL_PIN_ReadDigitalInput(int index) {
	return g_simulatedPinStates[index];
}
void HAL_PIN_Setup_Input_Pullup(int index) {
	g_pinModes[index] = SIM_PIN_INPUT_PULLUP;
}
void HAL_PIN_Setup_Input(int index) {
	g_pinModes[index] = SIM_PIN_INPUT;
}

void HAL_PIN_Setup_Output(int index) {
	g_pinModes[index] = SIM_PIN_OUTPUT;
}


void HAL_PIN_PWM_Stop(int pinIndex) {
}

void HAL_PIN_PWM_Start(int index) {
	g_pinModes[index] = SIM_PIN_PWM;
}
void HAL_PIN_PWM_Update(int index, int value) {
	if (value < 0)
		value = 0;
	if (value > 100)
		value = 100;
	g_simulatedPWMs[index] = value;
}



#endif

