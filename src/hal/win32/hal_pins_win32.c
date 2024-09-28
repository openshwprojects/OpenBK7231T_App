#ifdef WINDOWS

#include "../../new_common.h"
#include "../../new_pins.h"
#include "../../logging/logging.h"
#include "../hal_pins.h"

typedef enum simulatedPinMode_e {
	SIM_PIN_NONE,
	SIM_PIN_OUTPUT,
	SIM_PIN_PWM,
	SIM_PIN_INPUT_PULLUP,
	SIM_PIN_INPUT,
	SIM_PIN_ADC,
} simulatedPinMode_t;

int g_simulatedPinStates[PLATFORM_GPIO_MAX];
int g_simulatedPWMs[PLATFORM_GPIO_MAX];
simulatedPinMode_t g_pinModes[PLATFORM_GPIO_MAX];
int g_simulatedADCValues[PLATFORM_GPIO_MAX];

void SIM_Hack_ClearSimulatedPinRoles() {
	memset(g_simulatedPinStates, 0, sizeof(g_simulatedPinStates));
	memset(g_simulatedPWMs, 0, sizeof(g_simulatedPWMs));
	memset(g_pinModes, 0, sizeof(g_pinModes));
	memset(g_simulatedADCValues, 0, sizeof(g_simulatedADCValues));
}

static int adcToGpio[] = {
	-1,		// ADC0 - VBAT
	4, //GPIO4,	// ADC1
	5, //GPIO5,	// ADC2
	23,//GPIO23, // ADC3
	2,//GPIO2,	// ADC4
	3,//GPIO3,	// ADC5
	12,//GPIO12, // ADC6
	13,//GPIO13, // ADC7
};
static int c_adcToGpio = sizeof(adcToGpio)/sizeof(adcToGpio[0]);


static int gpioToAdc(int gpio) {
	int i;
	for (i = 0; i < c_adcToGpio; i++) {
		if (adcToGpio[i] == gpio)
			return i;
	}
	return -1;
}

void HAL_ADC_Init(int pinNumber) {
	g_pinModes[pinNumber] = SIM_PIN_ADC;
}
int HAL_ADC_Read(int pinNumber) {
	int channel = gpioToAdc(pinNumber);
	if (channel == -1)
		return 0;
	return g_simulatedADCValues[pinNumber];
}
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
bool SIM_IsPinADC(int index) {
	if (g_pinModes[index] == SIM_PIN_ADC)
		return true;
	return false;
}
void SIM_SetIntegerValueADCPin(int index, int v) {
	g_simulatedADCValues[index] = v;
}
void SIM_SetVoltageOnADCPin(int index, float v) {
	if (g_pinModes[index] != SIM_PIN_ADC)
		return;
	//printf("SIM_SetVoltageOnADCPin: %i has %f\n", index, v);
	if (v > 3.3f)
		v = 3.3f;
	if (v < 0)
		v = 0;
	float f = v / 3.3f;
	int iVal = f * 1024;
	SIM_SetIntegerValueADCPin(index, iVal);
}
int SIM_GetPWMValue(int index) {
	return g_simulatedPWMs[index];
}


void SIM_GeneratePinStatesDesc(char *o, int outLen) {
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		int mode = g_pinModes[i];
		if (mode == SIM_PIN_NONE)
			continue;
		const char *alias = HAL_PIN_GetPinNameAlias(i);
		char tmp[64];
		if (alias && *alias) {
		}
		else {
			alias = "N/A";
		}
		sprintf(tmp, "P%i (%s) ", i, alias);
		strcat_safe(o, tmp, outLen);
		if (mode == SIM_PIN_PWM) {
			sprintf(tmp, "PWM - %i", g_simulatedPWMs[i]);
			strcat_safe(o, tmp, outLen);
		}
		else if (mode == SIM_PIN_ADC) {
			sprintf(tmp, "ADC - %i", g_simulatedADCValues[i]);
			strcat_safe(o, tmp, outLen);
		}
		else if (mode == SIM_PIN_OUTPUT) {
			sprintf(tmp, "Output - %i", g_simulatedPinStates[i]);
			strcat_safe(o, tmp, outLen);
		} else {
			sprintf(tmp, "Input - %i", g_simulatedPinStates[i]);
			strcat_safe(o, tmp, outLen);
		}
		strcat_safe(o, "\n", outLen);
	}
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
	if (index == 7)
		return "PWM1";
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
void HAL_PIN_Setup_Input_Pulldown(int index) {
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
void HAL_PIN_PWM_Update(int index, float value) {
	if (value < 0)
		value = 0;
	if (value > 100)
		value = 100;
	g_simulatedPWMs[index] = value;
}

unsigned int HAL_GetGPIOPin(int index) {
	return index;
}

#endif

