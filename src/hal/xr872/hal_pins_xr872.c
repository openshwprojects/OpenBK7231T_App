#ifdef PLATFORM_XR872

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "driver/chip/hal_gpio.h"

typedef struct xr809pin_s {
	const char *name;
	int port;
	int pin;
} xr809pin_t;

static xr809pin_t g_xrPins[] = {
	// Original known pins
	{ "PA19", GPIO_PORT_A, GPIO_PIN_19 },
	{ "PB03", GPIO_PORT_B, GPIO_PIN_3 },
	{ "PA12", GPIO_PORT_A, GPIO_PIN_12 },
	{ "PA14", GPIO_PORT_A, GPIO_PIN_14 },
	{ "PA15", GPIO_PORT_A, GPIO_PIN_15 },
	{ "PA06", GPIO_PORT_A, GPIO_PIN_6 },
	{ "PA07", GPIO_PORT_A, GPIO_PIN_7 },
	{ "PA16", GPIO_PORT_A, GPIO_PIN_16 },
	{ "PA20", GPIO_PORT_A, GPIO_PIN_20 },
	{ "PA21", GPIO_PORT_A, GPIO_PIN_21 },
	{ "PA22", GPIO_PORT_A, GPIO_PIN_22 },
	{ "PB02", GPIO_PORT_B, GPIO_PIN_2 },
	{ "PA08", GPIO_PORT_A, GPIO_PIN_8 },

	// XR872-specific additions
	{ "PA00", GPIO_PORT_A, GPIO_PIN_0 },
	{ "PA01", GPIO_PORT_A, GPIO_PIN_1 },
	{ "PA02", GPIO_PORT_A, GPIO_PIN_2 },
	{ "PA03", GPIO_PORT_A, GPIO_PIN_3 },
	{ "PA04", GPIO_PORT_A, GPIO_PIN_4 },
	{ "PA05", GPIO_PORT_A, GPIO_PIN_5 },
	{ "PA09", GPIO_PORT_A, GPIO_PIN_9 },
	{ "PA10", GPIO_PORT_A, GPIO_PIN_10 },
	{ "PA11", GPIO_PORT_A, GPIO_PIN_11 },
	{ "PA13", GPIO_PORT_A, GPIO_PIN_13 },
	{ "PA17", GPIO_PORT_A, GPIO_PIN_17 },
	{ "PA18", GPIO_PORT_A, GPIO_PIN_18 },

	{ "PB00", GPIO_PORT_B, GPIO_PIN_0 },
	{ "PB01", GPIO_PORT_B, GPIO_PIN_1 },
	{ "PB04", GPIO_PORT_B, GPIO_PIN_4 },
	{ "PB05", GPIO_PORT_B, GPIO_PIN_5 },
	{ "PB06", GPIO_PORT_B, GPIO_PIN_6 },
	{ "PB07", GPIO_PORT_B, GPIO_PIN_7 },
	{ "PB08", GPIO_PORT_B, GPIO_PIN_8 },
	{ "PB09", GPIO_PORT_B, GPIO_PIN_9 },
	{ "PB10", GPIO_PORT_B, GPIO_PIN_10 },
	{ "PB11", GPIO_PORT_B, GPIO_PIN_11 },
	{ "PB12", GPIO_PORT_B, GPIO_PIN_12 },
	{ "PB13", GPIO_PORT_B, GPIO_PIN_13 },
	{ "PB14", GPIO_PORT_B, GPIO_PIN_14 },
	{ "PB15", GPIO_PORT_B, GPIO_PIN_15 },

	{ "PC00", GPIO_PORT_C, GPIO_PIN_0 },
	{ "PC01", GPIO_PORT_C, GPIO_PIN_1 },
	{ "PC02", GPIO_PORT_C, GPIO_PIN_2 },
	{ "PC03", GPIO_PORT_C, GPIO_PIN_3 },
	{ "PC04", GPIO_PORT_C, GPIO_PIN_4 },
	{ "PC05", GPIO_PORT_C, GPIO_PIN_5 },
	{ "PC06", GPIO_PORT_C, GPIO_PIN_6 },
	{ "PC07", GPIO_PORT_C, GPIO_PIN_7 },
};

int g_numXRPins = sizeof(g_xrPins) / sizeof(g_xrPins[0]);

static void PIN_XR809_GetPortPinForIndex(int index, int *xr_port, int *xr_pin) {
	if(index < 0 || index >= g_numXRPins) {
		*xr_port = 0;
		*xr_pin = 0;
		return;
	}
	*xr_port = g_xrPins[index].port;
	*xr_pin = g_xrPins[index].pin;
}

const char *HAL_PIN_GetPinNameAlias(int index) {
	if(index < 0 || index >= g_numXRPins) {
		return "bad_index";
	}
	return g_xrPins[index].name;
}

int HAL_PIN_CanThisPinBePWM(int index) {
	// You can improve this later to return 1 for PWM-capable pins
	return 0;
}

void HAL_PIN_SetOutputValue(int index, int iVal) {
	int xr_port;
	int xr_pin;
	PIN_XR809_GetPortPinForIndex(index, &xr_port, &xr_pin);
	HAL_GPIO_WritePin(xr_port, xr_pin, iVal);
}

int HAL_PIN_ReadDigitalInput(int index) {
	int xr_port;
	int xr_pin;
	PIN_XR809_GetPortPinForIndex(index, &xr_port, &xr_pin);
	return (HAL_GPIO_ReadPin(xr_port, xr_pin) == GPIO_PIN_LOW) ? 0 : 1;
}

void HAL_PIN_Setup_Input_Pulldown(int index) {
	// Not implemented — can be added later
}

void HAL_PIN_Setup_Input_Pullup(int index) {
	int xr_port;
	int xr_pin;
	GPIO_InitParam param;

	PIN_XR809_GetPortPinForIndex(index, &xr_port, &xr_pin);

	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F0_INPUT;
	param.pull = GPIO_PULL_UP;
	HAL_GPIO_Init(xr_port, xr_pin, &param);
}

void HAL_PIN_Setup_Input(int index) {
	int xr_port;
	int xr_pin;
	GPIO_InitParam param;

	PIN_XR809_GetPortPinForIndex(index, &xr_port, &xr_pin);

	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F0_INPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(xr_port, xr_pin, &param);
}

void HAL_PIN_Setup_Output(int index) {
	int xr_port;
	int xr_pin;
	GPIO_InitParam param;

	PIN_XR809_GetPortPinForIndex(index, &xr_port, &xr_pin);

	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F1_OUTPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(xr_port, xr_pin, &param);
}

void HAL_PIN_PWM_Stop(int pinIndex) {
	// Not yet implemented
}

void HAL_PIN_PWM_Start(int index, int freq) {
	// Not yet implemented
}

void HAL_PIN_PWM_Update(int index, float value) {
	if (value < 0) value = 0;
	if (value > 100) value = 100;
}

unsigned int HAL_GetGPIOPin(int index) {
	int xr_port;
	int xr_pin;
	PIN_XR809_GetPortPinForIndex(index, &xr_port, &xr_pin);
	return xr_pin;
}

#endif
