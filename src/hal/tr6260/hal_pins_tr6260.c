#ifdef PLATFORM_TR6260

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "drv_gpio.h"

typedef struct trPinMapping_s
{
	const char* name;
	DRV_GPIO_PIN_NAME pin;
} trPinMapping_t;

trPinMapping_t g_pins[] = {
	{ "GPIO0",  DRV_GPIO_0 },
	{ "GPIO1",  DRV_GPIO_1 },
	{ "GPIO2",  DRV_GPIO_2 },
	{ "GPIO3",  DRV_GPIO_3 },
	{ "GPIO4",  DRV_GPIO_4 },
	{ "GPIO5",  DRV_GPIO_5 },
	{ "GPIO6",  DRV_GPIO_6 },
	{ "GPIO7",  DRV_GPIO_7 },
	{ "GPIO8",  DRV_GPIO_8 },
	{ "GPIO9",  DRV_GPIO_9 },
	{ "GPIO10", DRV_GPIO_10 },
	{ "GPIO11", DRV_GPIO_11 },
	{ "GPIO12", DRV_GPIO_12 },
	{ "GPIO13", DRV_GPIO_13 },
	{ "GPIO14", DRV_GPIO_14 },
	{ "GPIO15", DRV_GPIO_15 },
	{ "GPIO16", DRV_GPIO_16 },
	{ "GPIO17", DRV_GPIO_17 },
	{ "GPIO18", DRV_GPIO_18 },
	{ "GPIO19", DRV_GPIO_19 },
	{ "GPIO20", DRV_GPIO_20 },
	{ "GPIO21", DRV_GPIO_21 },
	{ "GPIO22", DRV_GPIO_22 },
	{ "GPIO23", DRV_GPIO_23 },
	{ "GPIO24", DRV_GPIO_24 },
};

static int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

int PIN_GetPWMIndexForPinIndex(int pin)
{
	return -1;
}

const char* HAL_PIN_GetPinNameAlias(int index)
{
	if(index >= g_numPins)
		return "error";
	return g_pins[index].name;
}

int HAL_PIN_CanThisPinBePWM(int index)
{
	return 0;
}

void HAL_PIN_SetOutputValue(int index, int iVal)
{
	if(index >= g_numPins)
		return;
	trPinMapping_t* pin = g_pins + index;
	gpio_write(pin->pin, iVal);
}

int HAL_PIN_ReadDigitalInput(int index)
{
	if(index >= g_numPins)
		return 0;
	trPinMapping_t* pin = g_pins + index;
	return gpio_read_level(pin->pin);
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
	if(index >= g_numPins)
		return;
	trPinMapping_t* pin = g_pins + index;
	DRV_GPIO_CONFIG gpioCfg;
	gpioCfg.GPIO_Pin = pin->pin;
	gpioCfg.GPIO_PullEn = DRV_GPIO_PULL_EN;
	gpioCfg.GPIO_PullType = DRV_GPIO_PULL_TYPE_UP;
	gpio_config(&gpioCfg);
	gpio_set_dir(pin->pin, DRV_GPIO_DIR_INPUT);
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	if(index >= g_numPins)
		return;
	trPinMapping_t* pin = g_pins + index;
	DRV_GPIO_CONFIG gpioCfg;
	gpioCfg.GPIO_Pin = pin->pin;
	gpioCfg.GPIO_PullEn = DRV_GPIO_PULL_EN;
	gpioCfg.GPIO_PullType = DRV_GPIO_PULL_TYPE_DOWN;
	gpio_config(&gpioCfg);
	gpio_set_dir(pin->pin, DRV_GPIO_DIR_INPUT);
}

void HAL_PIN_Setup_Input(int index)
{
	if(index >= g_numPins)
		return;
	trPinMapping_t* pin = g_pins + index;
	DRV_GPIO_CONFIG gpioCfg;
	gpioCfg.GPIO_Pin = pin->pin;
	gpioCfg.GPIO_PullEn = DRV_GPIO_PULL_DIS;
	gpio_config(&gpioCfg);
	gpio_set_dir(pin->pin, DRV_GPIO_DIR_INPUT);
}

void HAL_PIN_Setup_Output(int index)
{
	if(index >= g_numPins)
		return;
	trPinMapping_t* pin = g_pins + index;
	gpio_set_dir(pin->pin, DRV_GPIO_DIR_OUTPUT);
}

void HAL_PIN_PWM_Stop(int index)
{
	if(index >= g_numPins)
		return;
}

void HAL_PIN_PWM_Start(int index)
{
	if(index >= g_numPins)
		return;
}

void HAL_PIN_PWM_Update(int index, float value)
{
	if(index >= g_numPins)
		return;
}

unsigned int HAL_GetGPIOPin(int index)
{
	return index;
}

#endif // PLATFORM_TR6260
