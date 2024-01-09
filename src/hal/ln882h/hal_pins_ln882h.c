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
	// ETC TODO
};

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
	lnPinMapping_t *pin = g_pins + index;
	return hal_gpio_pin_input_read(pin->base,pin->pin);
}
/*
	gpio_init.speed = GPIO_HIGH_SPEED;
	hal_gpio_init(GPIOA_BASE,&gpio_init);
	hal_gpio_pin_set(GPIOA_BASE,GPIO_PIN_15);
	gpio_init.pin = GPIO_PIN_0;
	hal_gpio_init(GPIOB_BASE,&gpio_init);
	hal_gpio_pin_reset(GPIOB_BASE,GPIO_PIN_0);
*/
void HAL_PIN_Setup_Input_Pullup(int index) {
	gpio_init_t_def gpio_init;

	lnPinMapping_t *pin = g_pins + index;

	memset(&gpio_init, 0, sizeof(gpio_init));
	gpio_init.dir = GPIO_OUTPUT;
	gpio_init.pin = pin->pin;
	gpio_init.speed = GPIO_HIGH_SPEED;
	hal_gpio_init(pin->base, &gpio_init);
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

#endif // PLATFORM_LN882H
