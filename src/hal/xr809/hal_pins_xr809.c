#ifdef PLATFORM_XR809 || PLATFORM_XR872

#include "../../new_common.h"
#include "../../logging/logging.h"

#include "driver/chip/hal_gpio.h"

typedef struct xr809pin_s {
	const char *name;
	int port;
	int pin;
} xr809pin_t;

// https://developer.tuya.com/en/docs/iot/xr3-datasheet?id=K98s9168qi49g

// Define missing macros for XR872 compatibility
#ifndef GPIO_PORT_A
#define GPIO_PORT_A 0
#define GPIO_PORT_B 1
#define GPIO_PORT_C 2
#endif

#ifndef GPIO_PIN_0
#define GPIO_PIN_0  0
#define GPIO_PIN_1  1
#define GPIO_PIN_2  2
#define GPIO_PIN_3  3
#define GPIO_PIN_4  4
#define GPIO_PIN_5  5
#define GPIO_PIN_6  6
#define GPIO_PIN_7  7
#define GPIO_PIN_8  8
#define GPIO_PIN_9  9
#define GPIO_PIN_10 10
#define GPIO_PIN_11 11
#define GPIO_PIN_12 12
#define GPIO_PIN_13 13
#define GPIO_PIN_14 14
#define GPIO_PIN_15 15
#endif

static xr809pin_t g_xrPins[] = {
// Port A (PA00–PA15)
    { "PA00", GPIO_PORT_A, GPIO_PIN_0 },  // PWM0_CH0
    { "PA01", GPIO_PORT_A, GPIO_PIN_1 },  // PWM0_CH1
    { "PA02", GPIO_PORT_A, GPIO_PIN_2 },  // PWM0_CH2
    { "PA03", GPIO_PORT_A, GPIO_PIN_3 },  // PWM0_CH3
    { "PA04", GPIO_PORT_A, GPIO_PIN_4 },  // PWM0_CH4
    { "PA05", GPIO_PORT_A, GPIO_PIN_5 },  // PWM0_CH5
    { "PA06", GPIO_PORT_A, GPIO_PIN_6 },  // UART0_TX
    { "PA07", GPIO_PORT_A, GPIO_PIN_7 },  // UART0_RX
    { "PA08", GPIO_PORT_A, GPIO_PIN_8 },  // SPI0_CLK
    { "PA09", GPIO_PORT_A, GPIO_PIN_9 },  // SPI0_CS
    { "PA10", GPIO_PORT_A, GPIO_PIN_10 }, // SPI0_MOSI
    { "PA11", GPIO_PORT_A, GPIO_PIN_11 }, // SPI0_MISO
    { "PA12", GPIO_PORT_A, GPIO_PIN_12 }, // I2C0_SCL
    { "PA13", GPIO_PORT_A, GPIO_PIN_13 }, // I2C0_SDA
    { "PA14", GPIO_PORT_A, GPIO_PIN_14 }, // Reserved / GPIO
    { "PA15", GPIO_PORT_A, GPIO_PIN_15 }, // Reserved / GPIO

    // Port B (PB00–PB15)
    { "PB00", GPIO_PORT_B, GPIO_PIN_0 },  // ADC_IN0
    { "PB01", GPIO_PORT_B, GPIO_PIN_1 },  // ADC_IN1
    { "PB02", GPIO_PORT_B, GPIO_PIN_2 },  // ADC_IN2
    { "PB03", GPIO_PORT_B, GPIO_PIN_3 },  // ADC_IN3
    { "PB04", GPIO_PORT_B, GPIO_PIN_4 },  // UART1_TX, PWM2_CH0
    { "PB05", GPIO_PORT_B, GPIO_PIN_5 },  // UART1_RX, PWM2_CH1
    { "PB06", GPIO_PORT_B, GPIO_PIN_6 },  // UART2_TX, PWM2_CH2
    { "PB07", GPIO_PORT_B, GPIO_PIN_7 },  // UART2_RX, PWM2_CH3
    { "PB08", GPIO_PORT_B, GPIO_PIN_8 },  // SPI1_MOSI, PWM2_CH4
    { "PB09", GPIO_PORT_B, GPIO_PIN_9 },  // SPI1_MISO, PWM2_CH5
    { "PB10", GPIO_PORT_B, GPIO_PIN_10 }, // SPI1_CLK
    { "PB11", GPIO_PORT_B, GPIO_PIN_11 }, // SPI1_CS
    { "PB12", GPIO_PORT_B, GPIO_PIN_12 }, // I2C1_SCL
    { "PB13", GPIO_PORT_B, GPIO_PIN_13 }, // I2C1_SDA
    { "PB14", GPIO_PORT_B, GPIO_PIN_14 }, // IR_TX
    { "PB15", GPIO_PORT_B, GPIO_PIN_15 }, // IR_RX

    // Port C (PC00–PC07)
    { "PC00", GPIO_PORT_C, GPIO_PIN_0 },  // SDIO_CLK
    { "PC01", GPIO_PORT_C, GPIO_PIN_1 },  // SDIO_CMD
    { "PC02", GPIO_PORT_C, GPIO_PIN_2 },  // SDIO_D0
    { "PC03", GPIO_PORT_C, GPIO_PIN_3 },  // SDIO_D1
    { "PC04", GPIO_PORT_C, GPIO_PIN_4 },  // SDIO_D2
    { "PC05", GPIO_PORT_C, GPIO_PIN_5 },  // SDIO_D3
    { "PC06", GPIO_PORT_C, GPIO_PIN_6 },  // SDIO_DET
    { "PC07", GPIO_PORT_C, GPIO_PIN_7 },  // Reserved / GPIO
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
void HAL_PIN_Setup_Input_Pulldown(int index) {

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

void HAL_PIN_PWM_Start(int index, int freq) {

}
void HAL_PIN_PWM_Update(int index, float value) {

	if(value<0)
		value = 0;
	if(value>100)
		value = 100;
}

unsigned int HAL_GetGPIOPin(int index) {
	int xr_port;
	int xr_pin;
	PIN_XR809_GetPortPinForIndex(index, &xr_port, &xr_pin);
	return xr_pin;
}

#endif

