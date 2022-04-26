#ifdef PLATFORM_XR809

#include "../../new_common.h"
#include "../../logging/logging.h"

#include "driver/chip/hal_gpio.h"

typedef struct xr809pin_s {
	const char *name;
	int port;
	int pin;
} xr809pin_t;

// https://developer.tuya.com/en/docs/iot/xr3-datasheet?id=K98s9168qi49g

static xr809pin_t g_xrPins[] = {
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



// XR809 - PWM todo
int HAL_PIN_CanThisPinBePWM(int index) {
	return 0;
}
void HAL_PIN_SetOutputValue(int index, int iVal) {
	int xr_port; // eg GPIO_PORT_A
	int xr_pin; // eg. GPIO_PIN_20

	PIN_XR809_GetPortPinForIndex(index, &xr_port, &xr_pin);

	HAL_GPIO_WritePin(xr_port, xr_pin, iVal);
}

int HAL_PIN_ReadDigitalInput(int index) {
	int xr_port; // eg GPIO_PORT_A
	int xr_pin; // eg. GPIO_PIN_20

	PIN_XR809_GetPortPinForIndex(index, &xr_port, &xr_pin);

	if (HAL_GPIO_ReadPin(xr_port, xr_pin) == GPIO_PIN_LOW)
		return 0;
	return 1;
}
void HAL_PIN_Setup_Input_Pullup(int index) {
	int xr_port; // eg GPIO_PORT_A
	int xr_pin; // eg. GPIO_PIN_20
	GPIO_InitParam param;

	PIN_XR809_GetPortPinForIndex(index, &xr_port, &xr_pin);

	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F0_INPUT;
	param.pull = GPIO_PULL_UP;
	HAL_GPIO_Init(xr_port, xr_pin, &param);
}
void HAL_PIN_Setup_Input(int index) {
	int xr_port; // eg GPIO_PORT_A
	int xr_pin; // eg. GPIO_PIN_20
	GPIO_InitParam param;

	PIN_XR809_GetPortPinForIndex(index, &xr_port, &xr_pin);

	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F0_INPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(xr_port, xr_pin, &param);
}

void HAL_PIN_Setup_Output(int index) {
	GPIO_InitParam param;
	int xr_port; // eg GPIO_PORT_A
	int xr_pin; // eg. GPIO_PIN_20

	PIN_XR809_GetPortPinForIndex(index, &xr_port, &xr_pin);

	/*set pin driver capability*/
	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F1_OUTPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(xr_port, xr_pin, &param);
}


void HAL_PIN_PWM_Stop(int pinIndex) {
}

void HAL_PIN_PWM_Start(int index) {

}
void HAL_PIN_PWM_Update(int index, int value) {

	if(value<0)
		value = 0;
	if(value>100)
		value = 100;
}



#endif 

