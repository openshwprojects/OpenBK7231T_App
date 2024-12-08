#ifdef PLATFORM_TR6260

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "drv_gpio.h"
#include "drv_pwm.h"

extern int g_pwmFrequency;

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
	{ "GPIO5 (RX0)",  DRV_GPIO_5 },
	{ "GPIO6 (TX0)",  DRV_GPIO_6 },
	{ "GPIO7",  DRV_GPIO_7 },
	{ "GPIO8",  DRV_GPIO_8 },
	{ "GPIO9",  DRV_GPIO_9 },
	{ "GPIO10", DRV_GPIO_10 },
	{ "GPIO11", DRV_GPIO_11 },
	{ "GPIO12", DRV_GPIO_12 },
	{ "GPIO13 (PWM5)", DRV_GPIO_13 },
	{ "GPIO14", DRV_GPIO_14 },
	{ "GPIO15", DRV_GPIO_15 },
	{ "GPIO16", DRV_GPIO_16 },
	{ "GPIO17", DRV_GPIO_17 },
	{ "GPIO18", DRV_GPIO_18 },
	{ "GPIO19", DRV_GPIO_19 },
	{ "GPIO20 (PWM0)", DRV_GPIO_20 },
	{ "GPIO21 (PWM1)", DRV_GPIO_21 },
	{ "GPIO22 (PWM2)", DRV_GPIO_22 },
#if defined (_USR_TR6260)
	{ "GPIO23 (PWM3)", DRV_GPIO_23 },
	{ "GPIO24 (PWM4)", DRV_GPIO_24 },
#else
	{ "GPIO23", DRV_GPIO_23 },
	{ "GPIO24", DRV_GPIO_24 },
#endif
};

static int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

int PIN_GetPWMIndexForPinIndex(int pin)
{
	switch(pin)
	{
		case 13: return PMW_CHANNEL_5;
		case 20: return PMW_CHANNEL_0;
		case 21: return PMW_CHANNEL_1;
		case 22: return PMW_CHANNEL_2;
#if defined (_USR_TR6260)
		case 23: return PMW_CHANNEL_3;
		case 24: return PMW_CHANNEL_4;
#endif
		default: return -1;
	}
}

const char* HAL_PIN_GetPinNameAlias(int index)
{
	if(index >= g_numPins)
		return "error";
	return g_pins[index].name;
}

int HAL_PIN_CanThisPinBePWM(int index)
{
	int ch = PIN_GetPWMIndexForPinIndex(index);
	if(ch == -1) return 0;
	return 1;
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
	//return gpio_read_level(pin->pin);
	int value = -1;
	gpio_read(pin->pin, &value);
	return value;
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
	if(index >= g_numPins)
		return;
	trPinMapping_t* pin = g_pins + index;
	DRV_GPIO_CONFIG gpioCfg;
	gpioCfg.GPIO_Pin = pin->pin;
	gpioCfg.GPIO_PullEn = DRV_GPIO_PULL_EN;
	gpioCfg.GPIO_Dir = DRV_GPIO_DIR_INPUT;
	gpioCfg.GPIO_PullType = DRV_GPIO_PULL_TYPE_UP;
	gpio_config(&gpioCfg);
	//gpio_set_dir(pin->pin, DRV_GPIO_DIR_INPUT);
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	if(index >= g_numPins)
		return;
	trPinMapping_t* pin = g_pins + index;
	DRV_GPIO_CONFIG gpioCfg;
	gpioCfg.GPIO_Pin = pin->pin;
	gpioCfg.GPIO_PullEn = DRV_GPIO_PULL_EN;
	gpioCfg.GPIO_Dir = DRV_GPIO_DIR_INPUT;
	gpioCfg.GPIO_PullType = DRV_GPIO_PULL_TYPE_DOWN;
	gpio_config(&gpioCfg);
	//gpio_set_dir(pin->pin, DRV_GPIO_DIR_INPUT);
}

void HAL_PIN_Setup_Input(int index)
{
	if(index >= g_numPins)
		return;
	trPinMapping_t* pin = g_pins + index;
	DRV_GPIO_CONFIG gpioCfg;
	gpioCfg.GPIO_Pin = pin->pin;
	gpioCfg.GPIO_PullEn = DRV_GPIO_PULL_DIS;
	gpioCfg.GPIO_Dir = DRV_GPIO_DIR_INPUT;
	gpio_config(&gpioCfg);
	//gpio_set_dir(pin->pin, DRV_GPIO_DIR_INPUT);
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
	int ch = PIN_GetPWMIndexForPinIndex(index);
	if(ch < 0) return;
	pwm_stop(ch);
	pwm_deinit(ch);
}

void HAL_PIN_PWM_Start(int index)
{
	if(index >= g_numPins)
		return;
	int ch = PIN_GetPWMIndexForPinIndex(index);
	if(ch < 0) return;
	pwm_init(ch);
}

void HAL_PIN_PWM_Update(int index, float value)
{
	if(index >= g_numPins)
		return;
	int ch = PIN_GetPWMIndexForPinIndex(index);
	if(ch < 0) return;
	if(value < 0)
		value = 0;
	if(value > 100)
		value = 100;
	pwm_config(ch, g_pwmFrequency, (uint32_t)(value * 10));
	pwm_start(ch);
}

unsigned int HAL_GetGPIOPin(int index)
{
	return index;
}

#endif // PLATFORM_TR6260
