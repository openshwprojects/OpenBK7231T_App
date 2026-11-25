#if PLATFORM_MT7687

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../hal_pins.h"
#include "hal_pinmap_mt7687.h"

mtPinMapping_t g_pins[] =
{
	//name			pin				pwm_ch	pwm_total	irq
	{ "IO0",		HAL_GPIO_0,		0,		0,			HAL_EINT_NUMBER_0 },
	{ "IO1",		HAL_GPIO_1,		1,		0,			HAL_EINT_NUMBER_1 },
	{ "IO2 (RX1)",	HAL_GPIO_2,		23,		0,			HAL_WIC_NUMBER_0 },
	{ "IO3 (TX1)",	HAL_GPIO_3,		24,		0,			HAL_EINT_NUMBER_2 },
	{ "IO4",		HAL_GPIO_4,		2,		0,			HAL_EINT_NUMBER_3 },
	{ "IO5",		HAL_GPIO_5,		3,		0,			HAL_EINT_NUMBER_4 },
	{ "IO6",		HAL_GPIO_6,		4,		0,			HAL_EINT_NUMBER_5 },
	{ "IO7",		HAL_GPIO_7,		5,		0,			HAL_EINT_NUMBER_6 },
	{ "IO24",		HAL_GPIO_24,	25,		0,			-1 },
	{ "IO25",		HAL_GPIO_25,	26,		0,			HAL_WIC_NUMBER_1 },
	{ "IO26",		HAL_GPIO_26,	27,		0,			-1 },
	{ "IO27",		HAL_GPIO_27,	28,		0,			HAL_WIC_NUMBER_2 },
	{ "IO28",		HAL_GPIO_28,	29,		0,			-1 },
	{ "IO29",		HAL_GPIO_29,	30,		0,			HAL_WIC_NUMBER_3 },
	{ "IO30",		HAL_GPIO_30,	31,		0,			-1 },
	{ "IO31",		HAL_GPIO_31,	32,		0,			-1 },
	{ "IO32",		HAL_GPIO_32,	33,		0,			HAL_WIC_NUMBER_4 },
	{ "IO33",		HAL_GPIO_33,	34,		0,			HAL_WIC_NUMBER_5 },
	{ "IO34",		HAL_GPIO_34,	35,		0,			HAL_WIC_NUMBER_6 },
	{ "IO35",		HAL_GPIO_35,	18,		0,			HAL_EINT_NUMBER_19 },
	{ "IO36 (RX2)",	HAL_GPIO_36,	19,		0,			HAL_WIC_NUMBER_7 },
	{ "IO37 (TX2)",	HAL_GPIO_37,	20,		0,			HAL_EINT_NUMBER_20 },
	{ "IO38",		HAL_GPIO_38,	21,		0,			HAL_EINT_NUMBER_21 },
	{ "IO39",		HAL_GPIO_39,	22,		0,			HAL_EINT_NUMBER_22 },
	{ "IO57",		HAL_GPIO_57,	36,		0,			HAL_WIC_NUMBER_8 },
	{ "IO58",		HAL_GPIO_58,	37,		0,			HAL_WIC_NUMBER_9 },
	{ "IO59",		HAL_GPIO_59,	38,		0,			HAL_WIC_NUMBER_10 },
	{ "IO60",		HAL_GPIO_60,	39,		0,			HAL_WIC_NUMBER_11 },
};

int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]) - 1;

int HAL_PIN_CanThisPinBePWM(int index)
{
	if(index >= g_numPins)
		return 0;
	mtPinMapping_t* pin = g_pins + index;
	return pin->pwm_ch >= 0;
}

//int PIN_GetPWMIndexForPinIndex(int index)
//{
//	mtPinMapping_t* pin = g_pins + index;
//	if(index >= g_numPins)
//		return -1;
//	return pin->pwm_ch;
//}

const char* HAL_PIN_GetPinNameAlias(int index)
{
	if(index >= g_numPins + 1)
		return "error";
	return g_pins[index].name;
}

void HAL_PIN_SetOutputValue(int index, int iVal)
{
	mtPinMapping_t* pin = g_pins + index;
	if(index >= g_numPins)
		return;
	hal_gpio_set_output(pin->pin, iVal ? 1 : 0);
}

int HAL_PIN_ReadDigitalInput(int index)
{
	mtPinMapping_t* pin = g_pins + index;
	if(index >= g_numPins)
		return 0;
	hal_gpio_data_t data;
	hal_gpio_get_input(pin->pin, &data);
	return data;
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
	if(index >= g_numPins)
		return;
	mtPinMapping_t* pin = g_pins + index;
	hal_gpio_init(pin->pin);
	hal_gpio_set_direction(pin->pin, HAL_GPIO_DIRECTION_INPUT);
	hal_gpio_pull_up(pin->pin);
	hal_pinmux_set_function(pin->pin, 8);
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	if(index >= g_numPins)
		return;
	mtPinMapping_t* pin = g_pins + index;
	hal_gpio_init(pin->pin);
	hal_gpio_set_direction(pin->pin, HAL_GPIO_DIRECTION_OUTPUT);
	hal_gpio_pull_down(pin->pin);
	hal_pinmux_set_function(pin->pin, 8);
}

void HAL_PIN_Setup_Input(int index)
{
	if(index >= g_numPins)
		return;
	mtPinMapping_t* pin = g_pins + index;
	hal_gpio_init(pin->pin);
	hal_gpio_set_direction(pin->pin, HAL_GPIO_DIRECTION_OUTPUT);
	hal_gpio_disable_pull(pin->pin);
	hal_pinmux_set_function(pin->pin, 8);
}

void HAL_PIN_Setup_Output(int index)
{
	if(index >= g_numPins)
		return;
	mtPinMapping_t* pin = g_pins + index;
	hal_gpio_init(pin->pin);
	hal_gpio_set_direction(pin->pin, HAL_GPIO_DIRECTION_OUTPUT);
	hal_gpio_pull_up(pin->pin);
	hal_pinmux_set_function(pin->pin, 8);
}

void HAL_PIN_PWM_Stop(int index)
{
	if(index >= g_numPins || !HAL_PIN_CanThisPinBePWM(index))
		return;
	mtPinMapping_t* pin = g_pins + index;
	hal_pwm_stop(pin->pwm_ch);
	hal_gpio_deinit(pin->pin);
}

void HAL_PIN_PWM_Start(int index, int freq)
{
	if(index >= g_numPins || !HAL_PIN_CanThisPinBePWM(index))
		return;
	mtPinMapping_t* pin = g_pins + index;
	hal_gpio_init(pin->pin);
	hal_pinmux_set_function(pin->pin, 9);
#if PLATFORM_MT7682
	hal_pwm_init(HAL_PWM_CLOCK_32KHZ);
#else
	hal_pwm_init(HAL_PWM_CLOCK_2MHZ);
#endif
	hal_pwm_set_frequency(pin->pwm_ch, freq, &pin->pwm_total);
}

void HAL_PIN_PWM_Update(int index, float value)
{
	if(index >= g_numPins || !HAL_PIN_CanThisPinBePWM(index))
		return;
	mtPinMapping_t* pin = g_pins + index;
	hal_pwm_set_duty_cycle(pin->pwm_ch, (uint32_t)((pin->pwm_total * value) / 100));
	hal_pwm_start(pin->pwm_ch);
}

unsigned int HAL_GetGPIOPin(int index)
{
	return index;
}

OBKInterruptHandler g_handlers[PLATFORM_GPIO_MAX];
OBKInterruptType g_modes[PLATFORM_GPIO_MAX];

void MT_Interrupt(uint32_t obkPinNum)
{
	if (g_handlers[obkPinNum]) {
		g_handlers[obkPinNum](obkPinNum);
	}
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function)
{
	g_handlers[pinIndex] = function;

	mtPinMapping_t* mt_irq = g_pins + pinIndex;
	if(mt_irq->irq == -1)
		return;
	hal_eint_trigger_mode_t mt_mode;
	switch(mode)
	{
		case INTERRUPT_RISING:
			mt_mode = HAL_EINT_EDGE_RISING;
			break;
		default:
		case INTERRUPT_FALLING:
			mt_mode = HAL_EINT_EDGE_FALLING;
			break;
		case INTERRUPT_CHANGE:
			mt_mode = HAL_EINT_EDGE_FALLING_AND_RISING;
			break;
	}
	hal_gpio_init(mt_irq->pin);
	hal_pinmux_set_function(mt_irq->pin, 3);
	hal_gpio_set_direction(mt_irq->pin, HAL_GPIO_DIRECTION_INPUT);
	hal_gpio_disable_pull(mt_irq->pin);
	hal_eint_config_t eint_config =
	{
		.trigger_mode = mt_mode,
		.debounce_time = 0,
	};
	hal_eint_init(mt_irq->irq, &eint_config);
	hal_eint_register_callback(mt_irq->irq, (hal_eint_callback_t)MT_Interrupt, (void*)pinIndex);
}

void HAL_DetachInterrupt(int pinIndex)
{
	if(g_handlers[pinIndex] == 0)
	{
		return; // already removed;
	}
	mtPinMapping_t* mt_irq = g_pins + pinIndex;
	if(mt_irq->irq == -1)
		return;
	hal_eint_deinit(mt_irq->irq);
	g_handlers[pinIndex] = 0;
}

#endif // PLATFORM_MT7687
