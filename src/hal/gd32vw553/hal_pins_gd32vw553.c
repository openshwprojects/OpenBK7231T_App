#ifdef PLATFORM_GD32VW553

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../hal_pins.h"
#include "hal_pinmap_gd32vw553.h"

gd32PinMapping_t g_pins[] = {
	{ "A00",		GPIOA,	GPIO_PIN_0,  },
	{ "A01",		GPIOA,	GPIO_PIN_1,  },
	{ "A02",		GPIOA,	GPIO_PIN_2,  },
	{ "A03",		GPIOA,	GPIO_PIN_3,  },
	{ "A04",		GPIOA,	GPIO_PIN_4,  },
	{ "A05",		GPIOA,	GPIO_PIN_5,  },
	{ "A06",		GPIOA,	GPIO_PIN_6,  },
	{ "A07",		GPIOA,	GPIO_PIN_7,  },
	{ "A08",		GPIOA,	GPIO_PIN_8,  },
	{ "A09",		GPIOA,	GPIO_PIN_9,  },
	{ "A10",		GPIOA,	GPIO_PIN_10, },
	{ "A11",		GPIOA,	GPIO_PIN_11, },
	{ "A12",		GPIOA,	GPIO_PIN_12, },
	{ "A13",		GPIOA,	GPIO_PIN_13, },
	{ "A14",		GPIOA,	GPIO_PIN_14, },
	{ "A15",		GPIOA,	GPIO_PIN_15, },
	{ "B00",		GPIOB,	GPIO_PIN_0,  },
	{ "B01",		GPIOB,	GPIO_PIN_1,  },
	{ "B02",		GPIOB,	GPIO_PIN_2,  },
	{ "B03",		GPIOB,	GPIO_PIN_3,  },
	{ "B04",		GPIOB,	GPIO_PIN_4,  },
	{ "B05 (NC)",	GPIOB,	GPIO_PIN_5,  },
	{ "B06 (NC)",	GPIOB,	GPIO_PIN_6,  },
	{ "B07 (NC)",	GPIOB,	GPIO_PIN_7,  },
	{ "B08 (NC)",	GPIOB,	GPIO_PIN_8,  },
	{ "B09 (NC)",	GPIOB,	GPIO_PIN_9,  },
	{ "B10 (NC)",	GPIOB,	GPIO_PIN_10, },
	{ "B11",		GPIOB,	GPIO_PIN_11, },
	{ "B12",		GPIOB,	GPIO_PIN_12, },
	{ "B13",		GPIOB,	GPIO_PIN_13, },
	{ "B14",		GPIOB,	GPIO_PIN_14, },
	{ "B15",		GPIOB,	GPIO_PIN_15, },
	{ "C00 (NC)",	GPIOC,	GPIO_PIN_0,  },
	{ "C01 (NC)",	GPIOC,	GPIO_PIN_1,  },
	{ "C02 (NC)",	GPIOC,	GPIO_PIN_2,  },
	{ "C03 (NC)",	GPIOC,	GPIO_PIN_3,  },
	{ "C04 (NC)",	GPIOC,	GPIO_PIN_4,  },
	{ "C05 (NC)",	GPIOC,	GPIO_PIN_5,  },
	{ "C06 (NC)",	GPIOC,	GPIO_PIN_6,  },
	{ "C07 (NC)",	GPIOC,	GPIO_PIN_7,  },
	{ "C08",		GPIOC,	GPIO_PIN_8,  },
	{ "C09 (NC)",	GPIOC,	GPIO_PIN_9,  },
	{ "C10 (NC)",	GPIOC,	GPIO_PIN_10, },
	{ "C11 (NC)",	GPIOC,	GPIO_PIN_11, },
	{ "C12 (NC)",	GPIOC,	GPIO_PIN_12, },
	{ "C13",		GPIOC,	GPIO_PIN_13, },
	{ "C14",		GPIOC,	GPIO_PIN_14, },
	{ "C15",		GPIOC,	GPIO_PIN_15, },
};

int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

const char* HAL_PIN_GetPinNameAlias(int index)
{
	if(index >= g_numPins)
		return "error";
	return g_pins[index].name;
}

void HAL_PIN_SetOutputValue(int index, int iVal)
{
	if(index >= g_numPins)
		return;
	gd32PinMapping_t* pin = g_pins + index;
	gpio_bit_write(pin->gpio, pin->pin, iVal ? 1 : 0);
}

int HAL_PIN_ReadDigitalInput(int index)
{
	if(index >= g_numPins)
		return 0;
	gd32PinMapping_t* pin = g_pins + index;
	return gpio_input_bit_get(pin->gpio, pin->pin);
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
	if(index >= g_numPins)
		return;
	gd32PinMapping_t* pin = g_pins + index;
	gpio_mode_set(pin->gpio, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, pin->pin);
	gpio_output_options_set(pin->gpio, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, pin->pin);
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	if(index >= g_numPins)
		return;
	gd32PinMapping_t* pin = g_pins + index;
	gpio_mode_set(pin->gpio, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, pin->pin);
	gpio_output_options_set(pin->gpio, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, pin->pin);
}

void HAL_PIN_Setup_Input(int index)
{
	if(index >= g_numPins)
		return;
	gd32PinMapping_t* pin = g_pins + index;
	gpio_mode_set(pin->gpio, GPIO_MODE_INPUT, GPIO_PUPD_NONE, pin->pin);
	gpio_output_options_set(pin->gpio, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, pin->pin);
}

void HAL_PIN_Setup_Output(int index)
{
	if(index >= g_numPins)
		return;
	gd32PinMapping_t* pin = g_pins + index;
	gpio_mode_set(pin->gpio, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, pin->pin);
	gpio_output_options_set(pin->gpio, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, pin->pin);
}

#endif // PLATFORM_GD32VW553
