#if PLATFORM_ARMINO

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../hal_pins.h"
#include "gpio_map.h"
#include "driver/gpio.h"
#include "driver/pwm.h"
#include "driver/pwm_types.h"
#include "driver/hal/hal_pwm_types.h"
#include "driver/hal/hal_gpio_types.h"

static uint16_t g_active_pwm = 0b0;
static uint8_t g_pwm_ch[SOC_GPIO_NUM] = { 0 };
static uint32_t g_pwm_freq[PWM_ID_MAX] = { 0 };
bk_err_t gpio_dev_map(gpio_id_t gpio_id, gpio_dev_t dev);
bk_err_t gpio_dev_unmap(gpio_id_t gpio_id);

const char* HAL_PIN_GetPinNameAlias(int index)
{
#if PLATFORM_BK7236
	switch(index)
	{
		case 0:  return "ADC12";
		case 1:  return "ADC13";
		case 8:  return "ADC10/PWM2";
		case 9:  return "PWM3";
		case 12: return "ADC14";
		case 13: return "ADC15";
		case 18: return "PWM0";
		case 19: return "PWM1";
		case 21: return "ADC6";
		case 22: return "ADC5";
		case 23: return "ADC3";
		case 24: return "ADC2/PWM4";
		case 25: return "ADC1/PWM5";
		case 28: return "ADC4";
		case 32: return "PWM6";
		case 33: return "PWM7";
		case 34: return "PWM8";
		case 35: return "PWM9";
		case 36: return "PWM10";
		case 37: return "PWM11";
	}
#elif PLATFORM_BK7239N || PLATFORM_BK7236N
	switch(index)
	{
		case 2:  return "ADC1";
		case 3:  return "ADC2";
		case 4:  return "ADC3";
		case 5:  return "ADC4";
		case 6:  return "ADC5";
		case 7:  return "ADC6";
		case 8:  return "ADC10";
		case 12: return "ADC14";
		case 13: return "ADC15";
	}
#endif
	return "IO";
}

int HAL_PIN_CanThisPinBePWM(int index)
{
#if PLATFORM_BK7236
	switch(index)
	{
		case 8:
		case 9:
		case 18:
		case 19:
		case 24:
		case 25:
		case 32:
		case 33:
		case 34:
		case 35:
		case 36:
		case 37:
			return 1;
		default:
			return 0;
	}
#else
	return 1;
#endif
}

void HAL_PIN_SetOutputValue(int index, int iVal)
{
	bk_err_t _ = iVal ? bk_gpio_set_output_high(index) : bk_gpio_set_output_low(index);
	(void)_;
	return;
}

int HAL_PIN_ReadDigitalInput(int index)
{
	return bk_gpio_get_input(index);
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
	bk_gpio_enable_input(index);
	bk_gpio_enable_pull(index);
	bk_gpio_pull_up(index);
	return;
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	bk_gpio_enable_input(index);
	bk_gpio_enable_pull(index);
	bk_gpio_pull_down(index);
	return;
}

void HAL_PIN_Setup_Input(int index)
{
	bk_gpio_enable_input(index);
	bk_gpio_disable_pull(index);
	return;
}

void HAL_PIN_Setup_Output(int index)
{
	bk_gpio_enable_output(index);
	bk_gpio_enable_pull(index);
	bk_gpio_pull_up(index);
	return;
}

void HAL_PIN_PWM_Stop(int index)
{
	uint8_t ch = g_pwm_ch[index];
	if(ch == 0) return;
	bk_pwm_stop(ch - 1);
	bk_pwm_deinit(ch - 1);
	BIT_CLEAR(g_active_pwm, (ch - 1));
	g_pwm_freq[ch - 1] = 0;
	g_pwm_ch[index] = 0;
	gpio_dev_unmap(index);
	return;
}

void HAL_PIN_PWM_Start(int index, int freq)
{
	if(g_pwm_ch[index] != 0)
	{
		g_pwm_freq[g_pwm_ch[index] - 1] = freq;
		return;
	}
	if((g_active_pwm & ((1 << PWM_ID_MAX) - 1)) == ((1 << PWM_ID_MAX) - 1)) return;
	uint8_t freech;
	for(freech = 0; freech < PWM_ID_MAX; freech++) if(!BIT_CHECK(g_active_pwm, freech)) break;
#if PLATFORM_BK7236
	switch(index)
	{
		case 8:  freech = 2; break;
		case 9:  freech = 3; break;
		case 18: freech = 0; break;
		case 19: freech = 1; break;
		case 24: freech = 4; break;
		case 25: freech = 5; break;
		case 32: freech = 6; break;
		case 33: freech = 7; break;
		case 34: freech = 8; break;
		case 35: freech = 9; break;
		case 36: freech = 10; break;
		case 37: freech = 11; break;
		default: return;
	}
#endif
	BIT_SET(g_active_pwm, freech);
	g_pwm_ch[index] = freech + 1;
	g_pwm_freq[freech] = freq;
	pwm_init_config_t cfg =
	{
		.period_cycle = CONFIG_XTAL_FREQ / freq,
		.duty_cycle = 0,
		.duty2_cycle = 0,
		.duty3_cycle = 0,
		.psc = 0,
	};
	bk_pwm_init(freech, &cfg);
	bk_pwm_start(freech);
	gpio_dev_unmap(index);
	gpio_dev_map(index, freech + 1);
	return;
}

void HAL_PIN_PWM_Update(int index, float value)
{
	uint8_t ch = g_pwm_ch[index];
	if(ch == 0) return;
	uint32_t period = CONFIG_XTAL_FREQ / g_pwm_freq[ch - 1];
	pwm_period_duty_config_t cfg =
	{
		.period_cycle = period,
		.duty_cycle = (period * value) / 100,
		.duty2_cycle = 0,
		.duty3_cycle = 0,
		.psc = 0,
	};
	bk_pwm_set_period_duty(ch - 1, &cfg);
	return;
}

unsigned int HAL_GetGPIOPin(int index)
{
	return index;
}

OBKInterruptHandler g_handlers[PLATFORM_GPIO_MAX];
OBKInterruptType g_modes[PLATFORM_GPIO_MAX];

void Beken_Interrupt(gpio_id_t pinNum)
{
	bk_gpio_clear_interrupt(pinNum);
	if(g_handlers[pinNum])
	{
		g_handlers[pinNum](pinNum);
	}
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function)
{
	g_handlers[pinIndex] = function;
	gpio_dev_unmap(pinIndex);
	gpio_config_t gmode = { 0 };
	gmode.io_mode = GPIO_INPUT_ENABLE;
	gmode.pull_mode = GPIO_PULL_DISABLE;
	gpio_int_type_t bk_mode;
	if(mode == INTERRUPT_RISING)
	{
		bk_mode = GPIO_INT_TYPE_RISING_EDGE;
	}
	else
	{
		bk_mode = GPIO_INT_TYPE_FALLING_EDGE;
	}
	bk_gpio_set_config(pinIndex, &gmode);
	bk_gpio_register_isr(pinIndex, Beken_Interrupt);
	bk_gpio_set_interrupt_type(pinIndex, bk_mode);
	bk_gpio_enable_interrupt(pinIndex);
}

void HAL_DetachInterrupt(int pinIndex)
{
	if(g_handlers[pinIndex] == 0)
	{
		return; // already removed;
	}
	bk_gpio_disable_interrupt(pinIndex);
	g_handlers[pinIndex] = 0;
}

#endif
