#ifdef PLATFORM_RTL87X0C

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "hal_generic_rtl87x0c.h"

extern int g_pwmFrequency;

rtlPinMapping_t g_pins[] = {
	{ "PA0 (RX1)",	PA_0,	false },
	{ "PA1 (TX1)",	PA_1,	false },
	{ "PA2 (RX1)",	PA_2,	false },
	{ "PA3 (TX1)",	PA_3,	false },
	{ "PA4",		PA_4,	false },
	{ "PA5",		PA_5,	false },
	{ "PA6",		PA_6,	false },
	{ "PA7",		PA_7,	false },
	{ "PA8",		PA_8,	false },
	{ "PA9",		PA_9,	false },
	{ "PA10",		PA_10,	false },
	{ "PA11",		PA_11,	false },
	{ "PA12",		PA_12,	false },
	{ "PA13 (RX0)",	PA_13,	false },
	{ "PA14 (TX0)",	PA_14,	false },
	{ "PA15 (RX2)",	PA_15,	false },
	{ "PA16 (TX2)",	PA_16,	false },
	{ "PA17",		PA_17,	false },
	{ "PA18",		PA_18,	false },
	{ "PA19",		PA_19,	false },
	{ "PA20",		PA_20,	false },
	{ "PA21",		PA_21,	false },
	{ "PA22",		PA_22,	false },
	{ "PA23",		PA_23,	false },
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
	if(index > 6 && index < 11) return 0;
	return 1;
}

void RTL_Gpio_Init(rtlPinMapping_t* pin)
{
	if(pin->isInit)
	{
		return;
	}
	gpio_init(&pin->gpio, pin->pin);
	pin->isInit = true;
}

void HAL_PIN_SetOutputValue(int index, int iVal)
{
	if(index >= g_numPins)
		return;
	rtlPinMapping_t* pin = g_pins + index;
	gpio_write(&pin->gpio, iVal ? 1 : 0);
}

int HAL_PIN_ReadDigitalInput(int index)
{
	if(index >= g_numPins)
		return 0;
	rtlPinMapping_t* pin = g_pins + index;
	return gpio_read(&pin->gpio);
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
	if(index >= g_numPins)
		return;
	rtlPinMapping_t* pin = g_pins + index;
	RTL_Gpio_Init(pin);
	gpio_dir(&pin->gpio, PIN_INPUT);
	gpio_mode(&pin->gpio, PullUp);
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	if(index >= g_numPins)
		return;
	rtlPinMapping_t* pin = g_pins + index;
	RTL_Gpio_Init(pin);
	gpio_dir(&pin->gpio, PIN_INPUT);
	gpio_mode(&pin->gpio, PullDown);
}

void HAL_PIN_Setup_Input(int index)
{
	if(index >= g_numPins)
		return;
	rtlPinMapping_t* pin = g_pins + index;
	RTL_Gpio_Init(pin);
	gpio_dir(&pin->gpio, PIN_INPUT);
	gpio_mode(&pin->gpio, PullNone);
}

void HAL_PIN_Setup_Output(int index)
{
	if(index >= g_numPins)
		return;
	rtlPinMapping_t* pin = g_pins + index;
	RTL_Gpio_Init(pin);
	gpio_dir(&pin->gpio, PIN_OUTPUT);
	gpio_mode(&pin->gpio, PullUp);
}

void HAL_PIN_PWM_Stop(int index)
{
	if(index >= g_numPins || !HAL_PIN_CanThisPinBePWM(index))
		return;
	rtlPinMapping_t* pin = g_pins + index;
	//pwmout_stop(&pin->pwm);
	pwmout_free(&pin->pwm);
	pin->isInit = false;
}

void HAL_PIN_PWM_Start(int index)
{
	if(index >= g_numPins || !HAL_PIN_CanThisPinBePWM(index))
		return;
	rtlPinMapping_t* pin = g_pins + index;
	pwmout_init(&pin->pwm, pin->pin);
	pwmout_period_us(&pin->pwm, g_pwmFrequency);
	pwmout_start(&pin->pwm);
	pin->isInit = true;
}

void HAL_PIN_PWM_Update(int index, float value)
{
	if(index >= g_numPins || !HAL_PIN_CanThisPinBePWM(index))
		return;
	rtlPinMapping_t* pin = g_pins + index;
	pwmout_write(&pin->pwm, value / 100);
}

unsigned int HAL_GetGPIOPin(int index)
{
	return index;
}

#endif // PLATFORM_RTL87X0C
