#include "hal/hal_gpio.h"

typedef struct lnPinMapping_s {
	const char *name;
	uint32_t base;
	gpio_pin_t pin;
	int8_t pwm_cha;
} lnPinMapping_t;

static lnPinMapping_t g_pins[] = {
	{ "A0", GPIOA_BASE, GPIO_PIN_0, -1 },
	{ "A1", GPIOA_BASE, GPIO_PIN_1, -1 },
	{ "A2", GPIOA_BASE, GPIO_PIN_2, -1 },
	{ "A3", GPIOA_BASE, GPIO_PIN_3, -1 },
	{ "A4", GPIOA_BASE, GPIO_PIN_4, -1 },
	{ "A5", GPIOA_BASE, GPIO_PIN_5, -1 },
	{ "A6", GPIOA_BASE, GPIO_PIN_6, -1 },
	{ "A7", GPIOA_BASE, GPIO_PIN_7, -1 },
	{ "A8", GPIOA_BASE, GPIO_PIN_8, -1 },
	{ "A9", GPIOA_BASE, GPIO_PIN_9, -1 },
	{ "A10", GPIOA_BASE, GPIO_PIN_10, -1 },
	{ "A11", GPIOA_BASE, GPIO_PIN_11, -1 },
	{ "A12", GPIOA_BASE, GPIO_PIN_12, -1 },
	{ "A13", GPIOA_BASE, GPIO_PIN_13, -1 },
	{ "A14", GPIOA_BASE, GPIO_PIN_14, -1 },
	{ "A15", GPIOA_BASE, GPIO_PIN_15, -1 },
	// port B
	{ "B0", GPIOB_BASE, GPIO_PIN_0, -1 }, // 16
	{ "B1", GPIOB_BASE, GPIO_PIN_1, -1 }, // 17
	{ "B2", GPIOB_BASE, GPIO_PIN_2, -1 }, // 18
	{ "B3", GPIOB_BASE, GPIO_PIN_3, -1 }, // 19
	{ "B4", GPIOB_BASE, GPIO_PIN_4, -1 }, // 20
	{ "B5", GPIOB_BASE, GPIO_PIN_5, -1 }, // 21
	{ "B6", GPIOB_BASE, GPIO_PIN_6, -1 }, // 22
	{ "B7", GPIOB_BASE, GPIO_PIN_7, -1 }, // 23
	{ "B8", GPIOB_BASE, GPIO_PIN_8, -1 }, // 24
	{ "B9", GPIOB_BASE, GPIO_PIN_9, -1 }, // 25
	// ETC TODO
};

static int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);