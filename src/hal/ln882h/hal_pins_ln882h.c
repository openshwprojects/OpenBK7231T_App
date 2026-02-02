#if PLATFORM_LN882H || PLATFORM_LN8825

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../hal_pins.h"
#include "hal_pinmap_ln882h.h"
// LN882H header
#if PLATFORM_LN882H
#include "hal/hal_adv_timer.h"
#include "hal/hal_clock.h"
#endif
#include "hal/hal_common.h"
#include "hal/hal_gpio.h"

#define IS_QSPI_PIN(index) (index > 12 && index < 19)

static int g_active_pwm = 0b0;

OBKInterruptHandler g_handlers[PLATFORM_GPIO_MAX];
OBKInterruptType g_modes[PLATFORM_GPIO_MAX];

#if PLATFORM_LN882H

lnPinMapping_t g_pins[] = {
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

#else

lnPinMapping_t g_pins[] = {
	{ "A0",  GPIOA_0, -1 },
	{ "A1",  GPIOA_1, -1 },
	{ "A2",  GPIOA_2, -1 },
	{ "A3",  GPIOA_3, -1 },
	{ "A4",  GPIOA_4, -1 },
	{ "A5",  GPIOA_5, -1 },
	{ "A6",  GPIOA_6, -1 },
	{ "A7",  GPIOA_7, -1 },
	{ "A8",  GPIOA_8, -1 },
	{ "A9",  GPIOA_9, -1 },
	{ "A10", GPIOA_10, -1 },
	{ "A11", GPIOA_11, -1 },
	{ "A12", GPIOA_12, -1 },
	{ "A13", GPIOA_13, -1 },
	{ "A14", GPIOA_14, -1 },
	{ "A15", GPIOA_15, -1 },
	// port B
	{ "B0", GPIOB_0, -1 }, // 16
	{ "B1", GPIOB_1, -1 }, // 17
	{ "B2", GPIOB_2, -1 }, // 18
	{ "B3", GPIOB_3, -1 }, // 19
	{ "B4", GPIOB_4, -1 }, // 20
	{ "B5", GPIOB_5, -1 }, // 21
	{ "B6", GPIOB_6, -1 }, // 22
	{ "B7", GPIOB_7, -1 }, // 23
	{ "B8", GPIOB_8, -1 }, // 24
	{ "B9", GPIOB_9, -1 }, // 25
	// ETC TODO
};

#endif

int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

int PIN_GetPWMIndexForPinIndex(int pin) {
	return -1;
}

const char *HAL_PIN_GetPinNameAlias(int index) {
	if (index >= g_numPins)
		return "error";
	return g_pins[index].name;
}

int HAL_PIN_CanThisPinBePWM(int index) 
{
	// qspi pins
	if(IS_QSPI_PIN(index)) 
		return 0;
	else return 1;
}

unsigned int HAL_GetGPIOPin(int index)
{
	return index;
}

#if PLATFORM_LN882H

void HAL_PIN_SetOutputValue(int index, int iVal) {
	if (index >= g_numPins)
		return;
	lnPinMapping_t *pin = g_pins + index;
	if (iVal) {
		hal_gpio_pin_set(pin->base, pin->pin);
	} else {
		hal_gpio_pin_reset(pin->base, pin->pin);
	}
}

int HAL_PIN_ReadDigitalInput(int index) {
	if (index >= g_numPins)
		return 0;
	lnPinMapping_t *pin = g_pins + index;
	return hal_gpio_pin_input_read(pin->base,pin->pin);
}

void My_LN882_Basic_GPIO_Setup(lnPinMapping_t *pin, int direction) {
	gpio_init_t_def gpio_init;
	memset(&gpio_init, 0, sizeof(gpio_init));
	gpio_init.dir = direction;
	gpio_init.pin = pin->pin;
	gpio_init.speed = GPIO_HIGH_SPEED;
	hal_gpio_init(pin->base, &gpio_init);
}

void HAL_PIN_Setup_Input_Pullup(int index) {
	if (index >= g_numPins)
		return;
	lnPinMapping_t *pin = g_pins + index;
	My_LN882_Basic_GPIO_Setup(pin, GPIO_INPUT);
	hal_gpio_pin_pull_set(pin->base,pin->pin, GPIO_PULL_UP);
}

void HAL_PIN_Setup_Input_Pulldown(int index) {
	if (index >= g_numPins)
		return;
	lnPinMapping_t *pin = g_pins + index;
	My_LN882_Basic_GPIO_Setup(pin, GPIO_INPUT);
	hal_gpio_pin_pull_set(pin->base, pin->pin, GPIO_PULL_DOWN);
}

void HAL_PIN_Setup_Input(int index) {
	if (index >= g_numPins)
		return;
	lnPinMapping_t *pin = g_pins + index;
	My_LN882_Basic_GPIO_Setup(pin, GPIO_INPUT);
	hal_gpio_pin_pull_set(pin->base, pin->pin, GPIO_PULL_NONE);
}

void HAL_PIN_Setup_Output(int index) {
	if (index >= g_numPins)
		return;
	if(IS_QSPI_PIN(index)) 
		return; // this would crash for me
	lnPinMapping_t *pin = g_pins + index;
	My_LN882_Basic_GPIO_Setup(pin, GPIO_OUTPUT);
	///hal_gpio_pin_pull_set(pin->base, pin->pin, GPIO_PULL_NONE);
}

uint32_t get_adv_timer_base(uint8_t ch)
{
	switch(ch)
	{
	case 0: return ADV_TIMER_0_BASE;
	case 1: return ADV_TIMER_1_BASE;
	case 2: return ADV_TIMER_2_BASE;
	case 3: return ADV_TIMER_3_BASE;
	case 4: return ADV_TIMER_4_BASE;
	case 5: return ADV_TIMER_5_BASE;
	}
}

void pwm_init(uint32_t freq, uint8_t pwm_channel_num, uint32_t gpio_reg_base, gpio_pin_t gpio_pin)
{
	uint32_t reg_base = get_adv_timer_base(pwm_channel_num);

	adv_tim_init_t_def adv_tim_init;
	memset(&adv_tim_init, 0, sizeof(adv_tim_init));

	if(freq >= 10000)
	{
		adv_tim_init.adv_tim_clk_div = 0;
		adv_tim_init.adv_tim_load_value = hal_clock_get_apb0_clk() * 1.0 / freq - 2;
	}
	else
	{
		adv_tim_init.adv_tim_clk_div = (uint32_t)(hal_clock_get_apb0_clk() / 1000000) - 1;
		adv_tim_init.adv_tim_load_value = 1000000 / freq - 2;
	}

	adv_tim_init.adv_tim_cmp_a_value = 0;
	adv_tim_init.adv_tim_dead_gap_value = 0;
	adv_tim_init.adv_tim_dead_en = ADV_TIMER_DEAD_DIS;
	adv_tim_init.adv_tim_cnt_mode = ADV_TIMER_CNT_MODE_INC;
	adv_tim_init.adv_tim_cha_inv_en = ADV_TIMER_CHA_INV_EN;

	hal_adv_tim_init(reg_base, &adv_tim_init);

	hal_gpio_pin_afio_select(gpio_reg_base, gpio_pin, (afio_function_t)(ADV_TIMER_PWM0 + pwm_channel_num * 2));
	hal_gpio_pin_afio_en(gpio_reg_base, gpio_pin, HAL_ENABLE);

}

void pwm_start(uint8_t pwm_channel_num)
{
	uint32_t reg_base = get_adv_timer_base(pwm_channel_num);
	hal_adv_tim_a_en(reg_base, HAL_ENABLE);
}

void pwm_set_duty(float duty, uint8_t pwm_channel_num)
{
	uint32_t reg_base = get_adv_timer_base(pwm_channel_num);
	hal_adv_tim_set_comp_a(reg_base, (hal_adv_tim_get_load_value(reg_base) + 2) * duty / 100.0f);
}

void HAL_PIN_PWM_Stop(int index) {
	if(index >= g_numPins)
		return;
	lnPinMapping_t* pin = g_pins + index;
	if(pin->pwm_cha < 0) return;
	uint32_t reg_base = get_adv_timer_base(pin->pwm_cha);
	hal_adv_tim_a_en(reg_base, HAL_DISABLE);
	hal_gpio_pin_afio_en(pin->base, pin->pin, HAL_DISABLE);
	g_active_pwm &= ~(1 << pin->pwm_cha);
	uint8_t chan = pin->pwm_cha;
	pin->pwm_cha = -1;
	ADDLOG_DEBUG(LOG_FEATURE_CMD, "PWM_Stop: ch: %i, all: %i", chan, g_active_pwm);
}

void HAL_PIN_PWM_Start(int index, int freq) 
{
	if(index >= g_numPins)
		return;
	lnPinMapping_t* pin = g_pins + index;
	if(pin->pwm_cha >= 0)
	{
		pwm_init(freq, pin->pwm_cha, pin->base, pin->pin);
		return;
	}
	uint8_t freecha;
	for(freecha = 0; freecha < 5; freecha++) if((g_active_pwm >> freecha & 1) == 0) break;
	g_active_pwm |= 1 << freecha;
	pin->pwm_cha = freecha;
	ADDLOG_DEBUG(LOG_FEATURE_CMD, "PWM_Start: ch_pwm: %u", freecha);
	pwm_init(freq, freecha, pin->base, pin->pin);
	pwm_start(freecha);
}

void HAL_PIN_PWM_Update(int index, float value) 
{
	if(index >= g_numPins)
		return;
	lnPinMapping_t* pin = g_pins + index;
	if(pin->pwm_cha < 0) return;
	if(value < 0)
		value = 0;
	if(value > 100)
		value = 100;
	pwm_set_duty(value, pin->pwm_cha);
}

uint32_t GetBaseForPin(int pinIndex)
{
	return pinIndex < 16 ? GPIOA_BASE : GPIOB_BASE;
}

int GetIRQForPin(int pinIndex)
{
	return pinIndex < 16 ? GPIOA_IRQn : GPIOB_IRQn;
}

uint16_t GetGPIOForPin(int pinIndex)
{
	return (uint16_t)1 << (uint16_t)(pinIndex % 16);
}
void Shared_Handler() {
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_handlers[i]) {
			uint32_t base = GetBaseForPin(i);
			uint16_t gpio_pin = GetGPIOForPin(i);
			if (hal_gpio_pin_get_it_flag(base, gpio_pin) == HAL_SET) {
				hal_gpio_pin_clr_it_flag(base, gpio_pin);
				g_handlers[i](i);
			}
		}
	}
}
void GPIOA_IRQHandler()
{
	Shared_Handler();
}

void GPIOB_IRQHandler()
{
	Shared_Handler();
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {
	g_handlers[pinIndex] = function;

	int ln_mode;
	if (mode == INTERRUPT_RISING) {
		ln_mode = GPIO_INT_RISING;
	}
	else {
		ln_mode = GPIO_INT_FALLING;
	}
	hal_gpio_pin_it_cfg(GetBaseForPin(pinIndex), GetGPIOForPin(pinIndex), ln_mode);
	hal_gpio_pin_it_en(GetBaseForPin(pinIndex), GetGPIOForPin(pinIndex), HAL_ENABLE);
	NVIC_SetPriority(GetIRQForPin(pinIndex), 1);
	NVIC_EnableIRQ(GetIRQForPin(pinIndex));

}

void HAL_DetachInterrupt(int pinIndex) {
	if (g_handlers[pinIndex] == 0) {
		return; // already removed;
	}
	g_handlers[pinIndex] = 0;
}

#else

void HAL_PIN_SetOutputValue(int index, int iVal)
{
	if(index >= g_numPins)
		return;
	lnPinMapping_t* pin = g_pins + index;
	HAL_GPIO_WritePin(pin->pin, iVal > 0 ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW);
}

int HAL_PIN_ReadDigitalInput(int index)
{
	if(index >= g_numPins)
		return 0;
	lnPinMapping_t* pin = g_pins + index;
	return HAL_GPIO_ReadPin(pin->pin);
}

void Basic_GPIO_Setup(lnPinMapping_t* pin, int direction)
{
	GPIO_InitTypeDef gpio_init;
	memset(&gpio_init, 0, sizeof(gpio_init));
	gpio_init.dir = direction;
	HAL_GPIO_Init(pin->pin, gpio_init);
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
	if(index >= g_numPins)
		return;
	lnPinMapping_t* pin = g_pins + index;
	Basic_GPIO_Setup(pin, GPIO_INPUT);
	HAL_SYSCON_GPIO_PullUp(pin->pin);
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	if(index >= g_numPins)
		return;
	lnPinMapping_t* pin = g_pins + index;
	Basic_GPIO_Setup(pin, GPIO_INPUT);
	HAL_SYSCON_GPIO_PullDown(pin->pin);
}

void HAL_PIN_Setup_Input(int index)
{
	if(index >= g_numPins)
		return;
	lnPinMapping_t* pin = g_pins + index;
	Basic_GPIO_Setup(pin, GPIO_INPUT);
	HAL_SYSCON_GPIO_NoPull(pin->pin);
}

void HAL_PIN_Setup_Output(int index)
{
	if(index >= g_numPins)
		return;
	if(IS_QSPI_PIN(index))
		return;
	lnPinMapping_t* pin = g_pins + index;
	Basic_GPIO_Setup(pin, GPIO_OUTPUT);
	HAL_SYSCON_GPIO_PullUp(pin->pin);
}

#include "hal/hal_pwm.h"
#include "ll/ll_pwm.h"

static uint16_t g_pwm_load[PWM_CH_MAX];

void HAL_PIN_PWM_Stop(int index)
{
	if(index >= g_numPins) return;
	lnPinMapping_t* pin = g_pins + index;
	if(pin->pwm_cha < 0) return;
	uint8_t chan = pin->pwm_cha;
	pin->pwm_cha = -1;
	HAL_PWM_Stop(chan);
	g_active_pwm &= ~(1 << chan);
	HAL_SYSCON_FuncIOSet(chan + GPIO_AF_PWM0, pin->pin, 0);
}

void HAL_PIN_PWM_Start(int index, int freq)
{
	if(index >= g_numPins) return;
	lnPinMapping_t* pin = g_pins + index;

	if(freq < 1) freq = 1;

	uint32_t div;
	uint32_t load;

	for(div = 0; div < 64; div++)
	{
		load = APBUS0_CLOCK / ((div + 1) * freq);
		if(load > 0 && load <= 65535) break;
	}

	if(div == 64)
	{
		div = 63;
		load = 65535;
	}

	if(pin->pwm_cha >= 0)
	{
		LL_PWM_Stop(pin->pwm_cha);
		LL_PWM_Div_Set(pin->pwm_cha, div);
		LL_PWM_Load_Set(pin->pwm_cha, load);
		g_pwm_load[pin->pwm_cha] = load;
		LL_PWM_Start(pin->pwm_cha);
		return;
	}
	uint8_t freecha;
	for(freecha = 0; freecha < PWM_CH_MAX; freecha++) if((g_active_pwm >> freecha & 1) == 0) break;

	g_active_pwm |= 1 << freecha;
	pin->pwm_cha = freecha;
	g_pwm_load[freecha] = load;

	HAL_SYSCON_FuncIOSet(freecha + GPIO_AF_PWM0, pin->pin, 1);

	LL_PWM_Stop(freecha);
	LL_PWM_Div_Set(freecha, div);
	LL_PWM_Load_Set(freecha, load);
	uint16_t cmp = (load > 1) ? 1 : 0;
	LL_PWM_Compare_Set(freecha, cmp);
	LL_PWM_CntMode_Set(freecha, 0);
	LL_PWM_Start(freecha);
}

void HAL_PIN_PWM_Update(int index, float value)
{
	if(index >= g_numPins) return;
	lnPinMapping_t* pin = g_pins + index;
	if(pin->pwm_cha < 0) return;
	if(value < 0) value = 0;
	if(value > 100) value = 100;
	value = 100 - value;

	uint32_t cmp = (uint32_t)(g_pwm_load[pin->pwm_cha] * value / 100.0f);

	if(cmp == 0 && value > 0) cmp = 1;
	if(cmp >= g_pwm_load[pin->pwm_cha]) cmp = g_pwm_load[pin->pwm_cha] - 1;
	LL_PWM_Compare_Set(pin->pwm_cha, (uint16_t)cmp);
}

void GPIO_IRQHandler()
{
	uint32_t status = HAL_GPIO_IntStatus();
	for(int i = 0; i < PLATFORM_GPIO_MAX; i++)
	{
		if(status & (1U << (i)))
		{
			if(g_handlers[i]) g_handlers[i](i);
			HAL_GPIO_IrqClear(i);
		}
	}
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function)
{
	if(!IS_PORTA_GPIO(pinIndex))
	{
		return;
	}
	g_handlers[pinIndex] = function;

	switch(mode)
	{
		case INTERRUPT_CHANGE:
			HAL_GPIO_TrigBothEdge(pinIndex, 1);
			break;
		case INTERRUPT_RISING:
			HAL_GPIO_TrigType(pinIndex, GPIO_TRIG_RISING_EDGE);
			break;
		case INTERRUPT_FALLING:
		default:
			HAL_GPIO_TrigType(pinIndex, GPIO_TRIG_FALLING_EDGE);
			break;
	}
	HAL_GPIO_UnmaskIrq(pinIndex);
	HAL_GPIO_IntEnable(pinIndex);
	NVIC_SetPriority(GPIO_IRQn, 1);
	NVIC_EnableIRQ(GPIO_IRQn);
}

void HAL_DetachInterrupt(int pinIndex)
{
	if(g_handlers[pinIndex] == 0)
	{
		return; // already removed;
	}
	HAL_GPIO_MaskIrq(pinIndex);
	g_handlers[pinIndex] = 0;
}

#endif

#endif // PLATFORM_LN882H
