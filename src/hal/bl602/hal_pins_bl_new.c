#if PLATFORM_BL_NEW

#include "../../new_pins.h"
#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../hal_pins.h"

#include "bflb_gpio.h"
#include "bflb_clock.h"
#if PLATFORM_BL616
#include "bl616_glb_gpio.h"
#include "bflb_pwm_v2.h"
#define PWM_NAME "pwm_v2_0"
static uint32_t pwmfreq = 0;
#else
#include "bflb_pwm_v1.h"
#define PWM_NAME "pwm_v1"
static uint32_t pwm_periods[5];
#endif

static struct bflb_device_s* gpio;
static struct bflb_device_s* dpwm;
static inline void ensure_gpio()
{
	if(!gpio) gpio = bflb_device_get_by_name("gpio");
}

int BL_FindPWMForPin(int index)
{
#if PLATFORM_BL602
	return index % 5;
#else
	return index % 4;
#endif
}

void HAL_PIN_SetOutputValue(int index, int iVal)
{
	ensure_gpio();
	if(iVal) bflb_gpio_set(gpio, index);
	else bflb_gpio_reset(gpio, (uint8_t)index);
}

const char* HAL_PIN_GetPinNameAlias(int index)
{
	return 0;
}

int HAL_PIN_CanThisPinBePWM(int index)
{
	return 1;
}

int HAL_PIN_ReadDigitalInput(int index)
{
	ensure_gpio();
	return bflb_gpio_read(gpio, (uint8_t)index);
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	ensure_gpio();
	bflb_gpio_init(gpio, (uint8_t)index, GPIO_INPUT | GPIO_PULLDOWN | GPIO_SMT_EN | GPIO_DRV_0);
}
void HAL_PIN_Setup_Input_Pullup(int index)
{
	ensure_gpio();
	bflb_gpio_init(gpio, (uint8_t)index, GPIO_INPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);
}
void HAL_PIN_Setup_Input(int index)
{
	ensure_gpio();
	bflb_gpio_init(gpio, (uint8_t)index, GPIO_INPUT | GPIO_SMT_EN | GPIO_DRV_0);
}
void HAL_PIN_Setup_Output(int index)
{
	ensure_gpio();
	bflb_gpio_init(gpio, (uint8_t)index, GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
	bflb_gpio_reset(gpio, (uint8_t)index);
}

void HAL_PIN_PWM_Stop(int index)
{
	if(!dpwm) dpwm = bflb_device_get_by_name(PWM_NAME);
	int pwm;

	pwm = BL_FindPWMForPin(index);

#if PLATFORM_BL616
	bflb_pwm_v2_channel_positive_stop(dpwm, pwm);
#elif PLATFORM_BL602
	bflb_pwm_v1_stop(dpwm, pwm);
#endif
}

void HAL_PIN_PWM_Start(int index, int freq)
{
	ensure_gpio();
	if(!dpwm) dpwm = bflb_device_get_by_name(PWM_NAME);
	int pwm;

	pwm = BL_FindPWMForPin(index);

#if PLATFORM_BL616 || defined(BL702L)
	if(freq != pwmfreq)
	{
		pwmfreq = freq;
		struct bflb_pwm_v2_config_s cfg = {
			.clk_source = BFLB_SYSTEM_PBCLK,
#if defined(BL702L)
			.clk_div = 64,
#else
			.clk_div = 80,
#endif
			.period = freq,
		};
		bflb_pwm_v2_stop(dpwm);
		bflb_pwm_v2_deinit(dpwm);
		bflb_pwm_v2_init(dpwm, &cfg);
		bflb_pwm_v2_start(dpwm);
	}
	struct bflb_pwm_v2_channel_config_s ch_cfg = {
		.positive_polarity = PWM_POLARITY_ACTIVE_HIGH,
	};

	bflb_pwm_v2_channel_init(dpwm, pwm, &ch_cfg);
#elif PLATFORM_BL602
	struct bflb_pwm_v1_channel_config_s cfg = {
		.clk_source = BFLB_SYSTEM_XCLK,
		.clk_div = 32,
		.period = freq,
	};
	pwm_periods[pwm] = freq;
	bflb_pwm_v1_channel_init(dpwm, pwm, &cfg);
#endif
	bflb_gpio_init(gpio, (uint8_t)index, GPIO_FUNC_PWM0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
}

void HAL_PIN_PWM_Update(int index, float value)
{
	if(!dpwm) dpwm = bflb_device_get_by_name(PWM_NAME);
	int pwm;

	pwm = BL_FindPWMForPin(index);

	if(value < 0)
		value = 0;
	if(value > 100)
		value = 100;

#if PLATFORM_BL616
	uint32_t threshold = (uint32_t)((value / 100.0f) * pwmfreq);
	bflb_pwm_v2_channel_set_threshold(dpwm, pwm, 0, threshold);
	bflb_pwm_v2_channel_positive_start(dpwm, pwm);
#elif PLATFORM_BL602
	uint32_t threshold = (uint32_t)((value / 100.0f) * pwm_periods[pwm]);
	bflb_pwm_v1_channel_set_threshold(dpwm, pwm, 0, threshold);
	bflb_pwm_v1_start(dpwm, pwm);
#endif
}

unsigned int HAL_GetGPIOPin(int index)
{
	return index;
}

OBKInterruptHandler g_handlers[PLATFORM_GPIO_MAX];
OBKInterruptType g_modes[PLATFORM_GPIO_MAX];

void BL_NEW_Interrupt(uint8_t pin)
{
	if(g_handlers[pin])
	{
		g_handlers[pin](pin);
	}
	bflb_gpio_int_clear(gpio, pin);
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function)
{
	if(pinIndex >= PLATFORM_GPIO_MAX) return;
	ensure_gpio();
	g_handlers[pinIndex] = function;
	uint8_t bl_mode;
	switch(mode)
	{
		case INTERRUPT_RISING: bl_mode = GPIO_INT_TRIG_MODE_SYNC_RISING_EDGE; break;
		default:
		case INTERRUPT_FALLING: bl_mode = GPIO_INT_TRIG_MODE_SYNC_FALLING_EDGE; break;
	}
	bflb_irq_disable(gpio->irq_num);
	bflb_gpio_int_init(gpio, (uint8_t)pinIndex, bl_mode);
	bflb_gpio_irq_attach((uint8_t)pinIndex, BL_NEW_Interrupt);
	bflb_irq_enable(gpio->irq_num);
}
void HAL_DetachInterrupt(int pinIndex)
{
	if(g_handlers[pinIndex] == 0)
	{
		return; // already removed;
	}
	ensure_gpio();
	bflb_irq_disable(gpio->irq_num);
	g_handlers[pinIndex] = 0;
	bflb_gpio_int_mask(gpio, (uint8_t)pinIndex, true);
	bflb_irq_enable(gpio->irq_num);
}

#endif
