#ifdef PLATFORM_LN882H

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
// LN882H header
#include "hal/hal_gpio.h"
#include "hal/hal_adv_timer.h"
#include "hal/hal_clock.h"

static int g_active_pwm = 0b0;

typedef struct lnPinMapping_s {
	const char *name;
	uint32_t base;
	gpio_pin_t pin;
	int8_t pwm_cha;
} lnPinMapping_t;

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

static int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

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
	if(index > 12 && index < 19) return 0;
	else return 1;
}

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

void HAL_PIN_PWM_Start(int index) 
{
	if(index >= g_numPins)
		return;
	uint8_t freecha;
	for(freecha = 0; freecha < 5; freecha++) if((g_active_pwm >> freecha & 1) == 0) break;
	lnPinMapping_t* pin = g_pins + index;
	g_active_pwm |= 1 << freecha;
	pin->pwm_cha = freecha;
	ADDLOG_DEBUG(LOG_FEATURE_CMD, "PWM_Start: ch_pwm: %u", freecha);
	pwm_init(10000, freecha, pin->base, pin->pin);
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

unsigned int HAL_GetGPIOPin(int index) {
	return index;
}

#endif // PLATFORM_LN882H
