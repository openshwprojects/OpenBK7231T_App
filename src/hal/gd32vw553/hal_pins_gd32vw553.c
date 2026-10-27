#ifdef PLATFORM_GD32VW553

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../hal_pins.h"
#include "hal_pinmap_gd32vw553.h"
#include "gd32vw55x_timer.h"
#include "wrapper_os.h"

//typedef struct gd32PinMapping_s
//{
//	const char* name;
//	uint32_t gpio;
//	uint32_t pin;
//	uint32_t pwmtimer;
//	uint16_t pwmchannel;
//	uint32_t pwmaf;
//} gd32PinMapping_t;
gd32PinMapping_t g_pins[] =
{
	{ "A00 (PWM1_0/A0)",	GPIOA,	GPIO_PIN_0,		TIMER1,		TIMER_CH_0,	GPIO_AF_1 },
	{ "A01 (PWM1_1/A1)",	GPIOA,	GPIO_PIN_1,		TIMER1,		TIMER_CH_1,	GPIO_AF_1 },
	{ "A02 (PWM0_0/A2)",	GPIOA,	GPIO_PIN_2,		TIMER0,		TIMER_CH_0,	GPIO_AF_6 },
	{ "A03 (PWM1_3/A3)",	GPIOA,	GPIO_PIN_3,		TIMER1,		TIMER_CH_3,	GPIO_AF_1 },
	{ "A04 (PWM0_1/A4)",	GPIOA,	GPIO_PIN_4,		TIMER0,		TIMER_CH_1,	GPIO_AF_8 },
	{ "A05 (A5)",			GPIOA,	GPIO_PIN_5,		-1,			-1,			-1 },
	{ "A06 (PWM0_1/A6)",	GPIOA,	GPIO_PIN_6,		TIMER0,		TIMER_CH_1,	GPIO_AF_8 },
	{ "A07 (PWM1_2/A7)",	GPIOA,	GPIO_PIN_7,		TIMER1,		TIMER_CH_2,	GPIO_AF_9 },
	{ "A08 (PWM0_0)",		GPIOA,	GPIO_PIN_8,		TIMER0,		TIMER_CH_0,	GPIO_AF_1 },
	{ "A09 (PWM0_1)",		GPIOA,	GPIO_PIN_9,		TIMER0,		TIMER_CH_1,	GPIO_AF_1 },
	{ "A10 (PWM4_0)",		GPIOA,	GPIO_PIN_10,	TIMER16,	TIMER_CH_0,	GPIO_AF_7 },
	{ "A11 (PWM0_3)",		GPIOA,	GPIO_PIN_11,	TIMER0,		TIMER_CH_3,	GPIO_AF_1 },
	{ "A12 (PWM1_2)",		GPIOA,	GPIO_PIN_12,	TIMER1,		TIMER_CH_2,	GPIO_AF_9 },
	{ "A13",				GPIOA,	GPIO_PIN_13,	-1,			-1,			-1 },
	{ "A14",				GPIOA,	GPIO_PIN_14,	-1,			-1,			-1 },
	{ "A15 (PWM1_0)",		GPIOA,	GPIO_PIN_15,	TIMER1,		TIMER_CH_0,	GPIO_AF_1 },
	{ "B00 (PWM4_0/A8)",	GPIOB,	GPIO_PIN_0,		TIMER16,	TIMER_CH_0,	GPIO_AF_9 },
	{ "B01 (PWM2_2)",		GPIOB,	GPIO_PIN_1,		TIMER2,		TIMER_CH_2,	GPIO_AF_3 },
	{ "B02 (PWM1_3)",		GPIOB,	GPIO_PIN_2,		TIMER1,		TIMER_CH_3,	GPIO_AF_1 },
	{ "B03 (PWM1_1)",		GPIOB,	GPIO_PIN_3,		TIMER1,		TIMER_CH_1,	GPIO_AF_1 },
	{ "B04 (PWM1_0)",		GPIOB,	GPIO_PIN_4,		TIMER1,		TIMER_CH_0,	GPIO_AF_1 },
	{ "B05 (NC)",			GPIOB,	GPIO_PIN_5,		-1,			-1,			-1 },
	{ "B06 (NC)",			GPIOB,	GPIO_PIN_6,		-1,			-1,			-1 },
	{ "B07 (NC)",			GPIOB,	GPIO_PIN_7,		-1,			-1,			-1 },
	{ "B08 (NC)",			GPIOB,	GPIO_PIN_8,		-1,			-1,			-1 },
	{ "B09 (NC)",			GPIOB,	GPIO_PIN_9,		-1,			-1,			-1 },
	{ "B10 (NC)",			GPIOB,	GPIO_PIN_10,	-1,			-1,			-1 },
	{ "B11 (PWM1_2)",		GPIOB,	GPIO_PIN_11,	TIMER1,		TIMER_CH_2,	GPIO_AF_1 },
	{ "B12 (PWM0_3)",		GPIOB,	GPIO_PIN_12,	TIMER0,		TIMER_CH_3,	GPIO_AF_2 },
	{ "B13 (PWM3_0)",		GPIOB,	GPIO_PIN_13,	TIMER15,	TIMER_CH_0,	GPIO_AF_8 },
	{ "B14 (NC)",			GPIOB,	GPIO_PIN_14,	-1,			-1,			-1 },
	{ "B15 (PWM2_0)",		GPIOB,	GPIO_PIN_15,	TIMER2,		TIMER_CH_0,	GPIO_AF_2 },
	{ "C00 (NC)",			GPIOC,	GPIO_PIN_0,		-1,			-1,			-1 },
	{ "C01 (NC)",			GPIOC,	GPIO_PIN_1,		-1,			-1,			-1 },
	{ "C02 (NC)",			GPIOC,	GPIO_PIN_2,		-1,			-1,			-1 },
	{ "C03 (NC)",			GPIOC,	GPIO_PIN_3,		-1,			-1,			-1 },
	{ "C04 (NC)",			GPIOC,	GPIO_PIN_4,		-1,			-1,			-1 },
	{ "C05 (NC)",			GPIOC,	GPIO_PIN_5,		-1,			-1,			-1 },
	{ "C06 (NC)",			GPIOC,	GPIO_PIN_6,		-1,			-1,			-1 },
	{ "C07 (NC)",			GPIOC,	GPIO_PIN_7,		-1,			-1,			-1 },
	{ "C08 (PWM2_2)",		GPIOC,	GPIO_PIN_8,		TIMER2,		TIMER_CH_2,	GPIO_AF_2 },
	{ "C09 (NC)",			GPIOC,	GPIO_PIN_9,		-1,			-1,			-1 },
	{ "C10 (NC)",			GPIOC,	GPIO_PIN_10,	-1,			-1,			-1 },
	{ "C11 (NC)",			GPIOC,	GPIO_PIN_11,	-1,			-1,			-1 },
	{ "C12 (NC)",			GPIOC,	GPIO_PIN_12,	-1,			-1,			-1 },
	{ "C13",				GPIOC,	GPIO_PIN_13,	-1,			-1,			-1 },
	{ "C14",				GPIOC,	GPIO_PIN_14,	-1,			-1,			-1 },
	{ "C15",				GPIOC,	GPIO_PIN_15,	-1,			-1,			-1 },
};

int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

const char* HAL_PIN_GetPinNameAlias(int index)
{
	if(index >= g_numPins)
		return "error";
	return g_pins[index].name;
}

int HAL_PIN_CanThisPinBePWM(int index)
{
	if(index >= g_numPins)
		return 0;
	return g_pins[index].pwmtimer != -1;
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

typedef struct
{
	uint32_t timer;
	int users;
	uint32_t period;
} pwmTimerState_t;

typedef struct
{
	bool initialized;
	int freq;
	float duty;
} pwmState_t;

static pwmTimerState_t g_pwmTimers[] =
{
	{ TIMER0,	0,	0	},
	{ TIMER1,	0,	0	},
	{ TIMER2,	0,	0	},
	{ TIMER15,	0,	0	},
	{ TIMER16,	0,	0	},
};

static pwmState_t g_pwmStates[sizeof(g_pins) / sizeof(g_pins[0])];

static uint32_t PWM_GetPeriod(int freq)
{
	if(freq <= 0 || freq > 1000000)
		return 0;

	return (1000000 / freq) - 1;
}

static pwmTimerState_t* PWM_GetTimerState(uint32_t timer)
{
	if(timer == (uint32_t)-1) 
		return NULL;

	for(int i = 0; i < sizeof(g_pwmTimers) / sizeof(g_pwmTimers[0]); i++)
	{
		if(g_pwmTimers[i].timer == timer)
			return &g_pwmTimers[i];
	}

	return NULL;
}

void HAL_PIN_PWM_Stop(int index)
{
	if(index < 0 || index >= g_numPins)
		return;

	gd32PinMapping_t* pin = g_pins + index;
	pwmState_t* st = g_pwmStates + index;
	pwmTimerState_t* ts = PWM_GetTimerState(pin->pwmtimer);

	if(pin->pwmtimer == (uint32_t)-1 || !st->initialized)
		return;

	timer_channel_output_state_config(pin->pwmtimer, pin->pwmchannel, TIMER_CCX_DISABLE);

	if(ts)
	{
		ts->users--;

		if(ts->users <= 0)
		{
			timer_disable(pin->pwmtimer);
			ts->users = 0;
		}
	}

	st->initialized = false;
}

void HAL_PIN_PWM_Start(int index, int freq)
{
	if(index < 0 || index >= g_numPins || freq <= 0)
		return;

	gd32PinMapping_t* pin = g_pins + index;
	pwmState_t* st = g_pwmStates + index;
	pwmTimerState_t* ts = PWM_GetTimerState(pin->pwmtimer);

	if(ts == NULL)
		return;

	uint32_t period = PWM_GetPeriod(freq);

	bool reconfigure = st->initialized && /*ts->users == 1*/ts->users > 0 && ts->period != period;

	if(ts->users > 0 && ts->period != period && !reconfigure)
	{
		return;
	}

	st->freq = freq;

	switch(pin->pwmtimer)
	{
		case TIMER0:
			rcu_periph_clock_enable(RCU_TIMER0);
			break;
		case TIMER1:
			rcu_periph_clock_enable(RCU_TIMER1);
			break;
		case TIMER2:
			rcu_periph_clock_enable(RCU_TIMER2);
			break;
		case TIMER15:
			rcu_periph_clock_enable(RCU_TIMER15);
			break;
		case TIMER16:
			rcu_periph_clock_enable(RCU_TIMER16);
			break;
		default: return;
	}

	gpio_mode_set(pin->gpio, GPIO_MODE_AF, GPIO_PUPD_NONE, pin->pin);
	gpio_output_options_set(pin->gpio, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, pin->pin);
	gpio_af_set(pin->gpio, pin->pwmaf, pin->pin);

	if(ts->users == 0 || reconfigure)
	{
		timer_parameter_struct timer_init_struct;

		timer_struct_para_init(&timer_init_struct);

		extern uint32_t SystemCoreClock;

		timer_init_struct.prescaler = (SystemCoreClock / 1000000) - 1;
		timer_init_struct.alignedmode = TIMER_COUNTER_EDGE;
		timer_init_struct.counterdirection = TIMER_COUNTER_UP;
		timer_init_struct.period = period;
		timer_init_struct.clockdivision = TIMER_CKDIV_DIV1;
		timer_init_struct.repetitioncounter = 0;

		timer_init(pin->pwmtimer, &timer_init_struct);

		timer_auto_reload_shadow_enable(pin->pwmtimer);

		if(pin->pwmtimer == TIMER0)
		{
			timer_primary_output_config(pin->pwmtimer, ENABLE);
		}

		timer_enable(pin->pwmtimer);

		ts->period = period;
	}

	timer_oc_parameter_struct oc;

	timer_channel_output_struct_para_init(&oc);

	oc.outputstate = TIMER_CCX_ENABLE;
	oc.ocpolarity = TIMER_OC_POLARITY_HIGH;

	timer_channel_output_config(pin->pwmtimer, pin->pwmchannel, &oc);
	timer_channel_output_mode_config(pin->pwmtimer, pin->pwmchannel, TIMER_OC_MODE_PWM0);
	timer_channel_output_shadow_config(pin->pwmtimer, pin->pwmchannel, TIMER_OC_SHADOW_DISABLE);
	timer_channel_output_state_config(pin->pwmtimer, pin->pwmchannel, TIMER_CCX_ENABLE);

	if(!st->initialized)
	{
		st->initialized = true;
		ts->users++;
	}
}

void HAL_PIN_PWM_Update(int index, float value)
{
	if(index < 0 || index >= g_numPins)
		return;

	gd32PinMapping_t* pin = g_pins + index;
	pwmState_t* st = g_pwmStates + index;

	if(pin->pwmtimer == (uint32_t)-1 || !st->initialized)
		return;

	if(value < 0)
		value = 0;

	if(value > 100)
		value = 100;

	st->duty = value;

	uint32_t period = TIMER_CAR(pin->pwmtimer);

	uint32_t pulse = (uint32_t)((value * (period + 1U)) / 100.0f);

	timer_channel_output_pulse_value_config(pin->pwmtimer, pin->pwmchannel, pulse);
}

static OBKInterruptHandler handlers[16] = { 0 };
static int8_t irq_gpio_index[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

void HAL_AttachInterrupt(int index, OBKInterruptType mode, OBKInterruptHandler function)
{
	if(index < 0 || index >= g_numPins)
		return;

	gd32PinMapping_t* pin = g_pins + index;
	uint32_t exti_pin = index % 16;
	uint32_t exti_port = index / 16;

	//switch(pin->gpio)
	//{
	//	case GPIOA: exti_port = EXTI_SOURCE_GPIOA; break;
	//	case GPIOB: exti_port = EXTI_SOURCE_GPIOB; break;
	//	case GPIOC: exti_port = EXTI_SOURCE_GPIOC; break;
	//	default: return;
	//}
	handlers[exti_pin] = function;
	irq_gpio_index[exti_pin] = index;

	syscfg_exti_line_config(exti_port, exti_pin);

	exti_init(
		BIT(exti_pin),
		EXTI_INTERRUPT,
		mode == INTERRUPT_RISING ? EXTI_TRIG_RISING :
		mode == INTERRUPT_FALLING ? EXTI_TRIG_FALLING :
		EXTI_TRIG_BOTH
	);

	switch(exti_pin)
	{
		case 0 ... 4: eclic_irq_enable(EXTI0_IRQn + exti_pin, 9, 0); break;
		case 5 ... 9: eclic_irq_enable(EXTI5_9_IRQn, 9, 0); break;
		case 10 ... 15: eclic_irq_enable(EXTI10_15_IRQn, 9, 0); break;
		default: break;
	}
	exti_interrupt_flag_clear(BIT(exti_pin));
}

void HAL_DetachInterrupt(int index)
{
	handlers[index % 16] = NULL;
	irq_gpio_index[index % 16] = -1;
}

static void irq(uint8_t exti_line)
{
	if(RESET != exti_interrupt_flag_get(BIT(exti_line)))
	{
		if(handlers[exti_line] && irq_gpio_index[exti_line] != -1)
		{
			handlers[exti_line](irq_gpio_index[exti_line]);
		}

		exti_interrupt_flag_clear(BIT(exti_line));
	}
}

void EXTI0_IRQHandler(void) { sys_int_enter(); irq(0); sys_int_exit(); }
void EXTI1_IRQHandler(void) { sys_int_enter(); irq(1); sys_int_exit(); }
void EXTI2_IRQHandler(void) { sys_int_enter(); irq(2); sys_int_exit(); }
void EXTI3_IRQHandler(void) { sys_int_enter(); irq(3); sys_int_exit(); }
void EXTI4_IRQHandler(void) { sys_int_enter(); irq(4); sys_int_exit(); }

void EXTI5_9_IRQHandler(void)
{
	sys_int_enter();
	for(int i = 5; i <= 9; i++) irq(i);
	sys_int_exit();
}

void EXTI10_15_IRQHandler(void)
{
	sys_int_enter();
	for(int i = 10; i <= 15; i++) irq(i);
	sys_int_exit();
}

#endif // PLATFORM_GD32VW553
