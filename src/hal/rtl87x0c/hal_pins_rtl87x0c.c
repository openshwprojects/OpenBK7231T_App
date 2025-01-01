#ifdef PLATFORM_RTL87X0C

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "hal_generic_rtl87x0c.h"

extern int g_pwmFrequency;

rtlPinMapping_t g_pins[] = {
	{ "PA0 (RX1)",	PA_0,	NULL, NULL },
	{ "PA1 (TX1)",	PA_1,	NULL, NULL },
	{ "PA2 (RX1)",	PA_2,	NULL, NULL },
	{ "PA3 (TX1)",	PA_3,	NULL, NULL },
	{ "PA4",		PA_4,	NULL, NULL },
	{ "PA5",		PA_5,	NULL, NULL },
	{ "PA6",		PA_6,	NULL, NULL },
	{ "PA7",		PA_7,	NULL, NULL },
	{ "PA8",		PA_8,	NULL, NULL },
	{ "PA9",		PA_9,	NULL, NULL },
	{ "PA10",		PA_10,	NULL, NULL },
	{ "PA11",		PA_11,	NULL, NULL },
	{ "PA12",		PA_12,	NULL, NULL },
	{ "PA13 (RX0)",	PA_13,	NULL, NULL },
	{ "PA14 (TX0)",	PA_14,	NULL, NULL },
	{ "PA15 (RX2)",	PA_15,	NULL, NULL },
	{ "PA16 (TX2)",	PA_16,	NULL, NULL },
	{ "PA17",		PA_17,	NULL, NULL },
	{ "PA18",		PA_18,	NULL, NULL },
	{ "PA19",		PA_19,	NULL, NULL },
	{ "PA20",		PA_20,	NULL, NULL },
	{ "PA21",		PA_21,	NULL, NULL },
	{ "PA22",		PA_22,	NULL, NULL },
	{ "PA23",		PA_23,	NULL, NULL },
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

void RTL_GPIO_Init(rtlPinMapping_t* pin)
{
	if(pin->gpio != NULL || pin->irq != NULL)
	{
		return;
	}
	pin->gpio = os_malloc(sizeof(gpio_t));
	memset(pin->gpio, 0, sizeof(gpio_t));
	gpio_init(pin->gpio, pin->pin);
}

void HAL_PIN_SetOutputValue(int index, int iVal)
{
	rtlPinMapping_t* pin = g_pins + index;
	if(index >= g_numPins || pin->gpio == NULL)
		return;
	gpio_write(pin->gpio, iVal ? 1 : 0);
}

int HAL_PIN_ReadDigitalInput(int index)
{
	rtlPinMapping_t* pin = g_pins + index;
	if(index >= g_numPins || pin->gpio == NULL)
		return 0;
	return gpio_read(pin->gpio);
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
	if(index >= g_numPins)
		return;
	rtlPinMapping_t* pin = g_pins + index;
	RTL_GPIO_Init(pin);
	if(pin->gpio == NULL)
		return;
	gpio_dir(pin->gpio, PIN_INPUT);
	gpio_mode(pin->gpio, PullUp);
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	if(index >= g_numPins)
		return;
	rtlPinMapping_t* pin = g_pins + index;
	RTL_GPIO_Init(pin);
	if(pin->gpio == NULL)
		return;
	gpio_dir(pin->gpio, PIN_INPUT);
	gpio_mode(pin->gpio, PullDown);
}

void HAL_PIN_Setup_Input(int index)
{
	if(index >= g_numPins)
		return;
	rtlPinMapping_t* pin = g_pins + index;
	RTL_GPIO_Init(pin);
	if(pin->gpio == NULL)
		return;
	gpio_dir(pin->gpio, PIN_INPUT);
	gpio_mode(pin->gpio, PullNone);
}

void HAL_PIN_Setup_Output(int index)
{
	if(index >= g_numPins)
		return;
	rtlPinMapping_t* pin = g_pins + index;
	RTL_GPIO_Init(pin);
	if(pin->gpio == NULL)
		return;
	gpio_dir(pin->gpio, PIN_OUTPUT);
	gpio_mode(pin->gpio, PullUp);
}

void HAL_PIN_PWM_Stop(int index)
{
	if(index >= g_numPins || !HAL_PIN_CanThisPinBePWM(index))
		return;
	rtlPinMapping_t* pin = g_pins + index;
	if(pin->pwm == NULL) return;
	//pwmout_stop(pin->pwm);
	pwmout_free(pin->pwm);
	os_free(pin->pwm);
	pin->pwm = NULL;
}

void HAL_PIN_PWM_Start(int index)
{
	if(index >= g_numPins || !HAL_PIN_CanThisPinBePWM(index))
		return;
	rtlPinMapping_t* pin = g_pins + index;
	if(pin->pwm != NULL) return;
	if(pin->gpio != NULL)
	{
		hal_pinmux_unregister(pin->pin, PID_GPIO);
		os_free(pin->gpio);
		pin->gpio = NULL;
	}
	pin->pwm = os_malloc(sizeof(pwmout_t));
	memset(pin->pwm, 0, sizeof(pwmout_t));
	pwmout_init(pin->pwm, pin->pin);
	pwmout_period_us(pin->pwm, g_pwmFrequency);
	pwmout_start(pin->pwm);
}

void HAL_PIN_PWM_Update(int index, float value)
{
	if(index >= g_numPins || !HAL_PIN_CanThisPinBePWM(index))
		return;
	rtlPinMapping_t* pin = g_pins + index;
	if(pin->pwm == NULL || !pin->pwm->is_init) return;
	pwmout_write(pin->pwm, value / 100);
}

unsigned int HAL_GetGPIOPin(int index)
{
	return index;
}

#endif // PLATFORM_RTL87X0C
