#ifdef PLATFORM_REALTEK

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "hal_generic_realtek.h"

extern rtlPinMapping_t g_pins[];
extern int g_numPins;

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

void HAL_PIN_PWM_Start(int index, int freq)
{
	if(index >= g_numPins || !HAL_PIN_CanThisPinBePWM(index))
		return;
	rtlPinMapping_t* pin = g_pins + index;
	if(pin->pwm != NULL) return;
	if(pin->gpio != NULL)
	{
		gpio_deinit(pin->gpio);
		os_free(pin->gpio);
		pin->gpio = NULL;
	}
	pin->pwm = os_malloc(sizeof(pwmout_t));
	memset(pin->pwm, 0, sizeof(pwmout_t));
	pwmout_init(pin->pwm, pin->pin);
	pwmout_period_us(pin->pwm, freq);
#ifndef PLATFORM_RTL8710A
	pwmout_start(pin->pwm);
#endif
}

void HAL_PIN_PWM_Update(int index, float value)
{
	if(index >= g_numPins || !HAL_PIN_CanThisPinBePWM(index))
		return;
	rtlPinMapping_t* pin = g_pins + index;
#ifdef PLATFORM_RTL87X0C
	if(pin->pwm == NULL || !pin->pwm->is_init) return;
#else
	if(pin->pwm == NULL) return;
#endif
	pwmout_write(pin->pwm, value / 100);
}

unsigned int HAL_GetGPIOPin(int index)
{
	return index;
}

#endif // PLATFORM_REALTEK
