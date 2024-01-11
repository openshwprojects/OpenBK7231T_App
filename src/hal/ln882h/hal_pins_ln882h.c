#ifdef PLATFORM_LN882H


#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
// LN882H header
#include "hal/hal_gpio.h"

typedef struct lnPinMapping_s {
	const char *name;
	uint32_t base;
	gpio_pin_t pin;
} lnPinMapping_t;

lnPinMapping_t g_pins[] = {
	{ "A0", GPIOA_BASE, GPIO_PIN_0 },
	{ "A1", GPIOA_BASE, GPIO_PIN_1 },
	{ "A2", GPIOA_BASE, GPIO_PIN_2 },
	{ "A3", GPIOA_BASE, GPIO_PIN_3 },
	{ "A4", GPIOA_BASE, GPIO_PIN_4 },
	{ "A5", GPIOA_BASE, GPIO_PIN_5 },
	{ "A6", GPIOA_BASE, GPIO_PIN_6 },
	{ "A7", GPIOA_BASE, GPIO_PIN_7 },
	{ "A8", GPIOA_BASE, GPIO_PIN_8 },
	{ "A9", GPIOA_BASE, GPIO_PIN_9 },
	{ "A10", GPIOA_BASE, GPIO_PIN_10 },
	{ "A11", GPIOA_BASE, GPIO_PIN_11 },
	{ "A12", GPIOA_BASE, GPIO_PIN_12 },
	{ "A13", GPIOA_BASE, GPIO_PIN_13 },
	{ "A14", GPIOA_BASE, GPIO_PIN_14 },
	{ "A15", GPIOA_BASE, GPIO_PIN_15 },
	// port B
	{ "B0", GPIOB_BASE, GPIO_PIN_0 },
	{ "B1", GPIOB_BASE, GPIO_PIN_1 },
	{ "B2", GPIOB_BASE, GPIO_PIN_2 },
	{ "B3", GPIOB_BASE, GPIO_PIN_3 },
	{ "B4", GPIOB_BASE, GPIO_PIN_4 },
	{ "B5", GPIOB_BASE, GPIO_PIN_5 },
	{ "B6", GPIOB_BASE, GPIO_PIN_6 },
	{ "B7", GPIOB_BASE, GPIO_PIN_7 },
	{ "B8", GPIOB_BASE, GPIO_PIN_8 },
	{ "B9", GPIOB_BASE, GPIO_PIN_9 },
	// ETC TODO
};
static int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

int PIN_GetPWMIndexForPinIndex(int pin) {
	return -1;
}

const char *HAL_PIN_GetPinNameAlias(int index) {
	if (index >= g_numPins)
		return "error";
	return g_pins[index].name;
}

int HAL_PIN_CanThisPinBePWM(int index) {
	
	return 1;
}

void HAL_PIN_SetOutputValue(int index, int iVal) {
	if (index >= g_numPins)
		return 0;
	lnPinMapping_t *pin = g_pins + index;
	if (iVal) {
		hal_gpio_pin_set(pin->base, pin->pin);
	} else {
		hal_gpio_pin_reset(pin->base, pin->pin);
	}
}

int HAL_PIN_ReadDigitalInput(int index) {
	if (index >= g_numPins)
		return 0;
	lnPinMapping_t *pin = g_pins + index;
	return hal_gpio_pin_input_read(pin->base,pin->pin);
}

void My_LN882_Basic_GPIO_Setup(lnPinMapping_t *pin, int direction) {
	gpio_init_t_def gpio_init;
	memset(&gpio_init, 0, sizeof(gpio_init));
	gpio_init.dir = direction;
	gpio_init.pin = pin->pin;
	gpio_init.speed = GPIO_HIGH_SPEED;
	hal_gpio_init(pin->base, &gpio_init);
}

void HAL_PIN_Setup_Input_Pullup(int index) {
	if (index >= g_numPins)
		return;
	lnPinMapping_t *pin = g_pins + index;
	My_LN882_Basic_GPIO_Setup(pin, GPIO_INPUT);
	hal_gpio_pin_pull_set(pin->base,pin->pin, GPIO_PULL_UP);
}

void HAL_PIN_Setup_Input_Pulldown(int index) {
	if (index >= g_numPins)
		return;
	lnPinMapping_t *pin = g_pins + index;
	My_LN882_Basic_GPIO_Setup(pin, GPIO_INPUT);
	hal_gpio_pin_pull_set(pin->base, pin->pin, GPIO_PULL_DOWN);
}

void HAL_PIN_Setup_Input(int index) {
	if (index >= g_numPins)
		return;
	lnPinMapping_t *pin = g_pins + index;
	My_LN882_Basic_GPIO_Setup(pin, GPIO_INPUT);
	hal_gpio_pin_pull_set(pin->base, pin->pin, GPIO_PULL_NONE);
}

void HAL_PIN_Setup_Output(int index) {
	if (index >= g_numPins)
		return;
	lnPinMapping_t *pin = g_pins + index;
	My_LN882_Basic_GPIO_Setup(pin, GPIO_OUTPUT);
	///hal_gpio_pin_pull_set(pin->base, pin->pin, GPIO_PULL_NONE);
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

#endif // PLATFORM_LN882H
