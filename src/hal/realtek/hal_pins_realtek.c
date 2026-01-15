#ifdef PLATFORM_REALTEK

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../hal_pins.h"
#include "hal_pinmap_realtek.h"
#if !PLATFORM_REALTEK_NEW
#include "gpio_ex_api.h"
#endif
#if !PLATFORM_RTL8710A && !PLATFORM_RTL8710B
#include "pwmout_ex_api.h"
#endif

#if PLATFORM_REALTEK_NEW

#include "pwmout_ex_api.h"
static int g_active_pwm = 0b0;

uint8_t HAL_RTK_GetFreeChannel()
{
	uint8_t freech;
	for(freech = 0; freech <= 9; freech++) if((g_active_pwm >> freech & 1) == 0) break;
	if(freech == 9) return -1;
	g_active_pwm |= 1 << freech;
	return freech;
}

void HAL_RTK_FreeChannel(uint8_t channel)
{
	g_active_pwm &= ~(1 << channel);
}

#endif

int PIN_GetPWMIndexForPinIndex(int index)
{
	rtlPinMapping_t* pin = g_pins + index;
	if(index >= g_numPins)
		return -1;
	if(pin->pwm != NULL) return pin->pwm->pwm_idx;
	else return HAL_PIN_CanThisPinBePWM(index);
}

const char* HAL_PIN_GetPinNameAlias(int index)
{
	if(index >= g_numPins)
		return "error";
	return g_pins[index].name;
}


// code to find a pin index by name
// we migth have "complex" or alternate names like
// "IO0/A0" or even "IO2 (B2/TX1)" and would like all to match
// so we need to make sure the substring is found an propperly "terminated"
// by '\0', '/' , '(', ')' or ' '


// Define valid "terminators" and "start characters" for a string
#define TERMINATORS " ()/\0"
#define STARTCHARS " /()"

int is_valid_termination(char ch) {
    // Check if character is in defined terminators
    return strchr(TERMINATORS, ch) != NULL;
}

int is_start_character(char ch) {
    // Check if character is a valid start character
    return strchr(STARTCHARS, ch) != NULL;
}


int str_match_in_alias(const char* alias, const char* str) {
    size_t alias_length = strlen(alias);
    size_t str_length = strlen(str);

    // Early return if `str` (substring) is longer than `alias`
    if (str_length > alias_length) {
        return 0; // No match possible
    }

    // Check if the substring is at the start of the string
    // --> no need (and no possibility) to check previous char
    if (strncmp(alias, str, str_length) == 0) {
        if (str_length < alias_length) {
            char following = alias[str_length];
            if (is_valid_termination(following)) {
                return 1;  // Match found at the start with valid termination
            }
        } else {
            return 1; // whole string matches
        }
    }

    // try finding str inside alias (not at beginning)
    const char* found = strstr(alias, str);
    while (found != NULL) {
        char previous = *(found - 1);
        if (previous) {  // Ensure previous is valid
            char following = *(found + str_length);
            if (is_start_character(previous) && (following == '\0' || is_valid_termination(following))) {
                return 1; // Match found at valid boundaries
            }
        }
        found = strstr(found + 1, str);  // Continue searching
    }

    return 0;  // No valid match found
}

int HAL_PIN_Find(const char *name) {
	if (isdigit(name[0])) {
		return atoi(name);
	}
	for (int i = 0; i < g_numPins; i++) {
		if (str_match_in_alias(HAL_PIN_GetPinNameAlias(i), name)) {
			return i;
		}
	}
	return -1;
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
#if PLATFORM_REALTEK_NEW
	HAL_RTK_FreeChannel(pin->pwm->pwm_idx);
#endif
	pwmout_free(pin->pwm);
	os_free(pin->pwm);
	pin->pwm = NULL;
}

void HAL_PIN_PWM_Start(int index, int freq)
{
	if(index >= g_numPins || !HAL_PIN_CanThisPinBePWM(index))
		return;
	rtlPinMapping_t* pin = g_pins + index;
	if(pin->pwm != NULL)
	{
		pwmout_period_us(pin->pwm, 1000000 / freq);
		return;
	}
	if(pin->gpio != NULL)
	{
		gpio_deinit(pin->gpio);
		os_free(pin->gpio);
		pin->gpio = NULL;
	}
	pin->pwm = os_malloc(sizeof(pwmout_t));
	memset(pin->pwm, 0, sizeof(pwmout_t));
#if PLATFORM_REALTEK_NEW
	int ch = HAL_RTK_GetFreeChannel();
	if(ch == -1)
	{
		os_free(pin->pwm);
		pin->pwm = NULL;
		return;
	}
	pin->pwm->pwm_idx = ch;
#endif
	pwmout_init(pin->pwm, pin->pin);
	pwmout_period_us(pin->pwm, 1000000 / freq);
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

OBKInterruptHandler g_handlers[PLATFORM_GPIO_MAX];
OBKInterruptType g_modes[PLATFORM_GPIO_MAX];

#include "gpio_irq_api.h"

void Realtek_Interrupt(uint32_t obkPinNum, gpio_irq_event event)
{
	if (g_handlers[obkPinNum]) {
		g_handlers[obkPinNum](obkPinNum);
	}
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {
	g_handlers[pinIndex] = function;

	rtlPinMapping_t *rtl_cf = g_pins + pinIndex;
#if PLATFORM_RTL87X0C
	if (rtl_cf->gpio != NULL)
	{
		hal_pinmux_unregister(rtl_cf->pin, PID_GPIO);
		os_free(rtl_cf->gpio);
		rtl_cf->gpio = NULL;
	}
#endif
	rtl_cf->irq = os_malloc(sizeof(gpio_irq_t));
	memset(rtl_cf->irq, 0, sizeof(gpio_irq_t));

	int rtl_mode;
	if (mode == INTERRUPT_RISING) {
		rtl_mode = IRQ_RISE;
	}
	else {
		rtl_mode = IRQ_FALL;
	}
	gpio_irq_init(rtl_cf->irq, rtl_cf->pin, Realtek_Interrupt, pinIndex);
	gpio_irq_set(rtl_cf->irq, rtl_mode, 1);
	gpio_irq_enable(rtl_cf->irq);

}
void HAL_DetachInterrupt(int pinIndex) {
	if (g_handlers[pinIndex] == 0) {
		return; // already removed;
	}
	rtlPinMapping_t *rtl_cf = g_pins + pinIndex;
	gpio_irq_free(rtl_cf->irq);
	os_free(rtl_cf->irq);
	rtl_cf->irq = NULL;
	g_handlers[pinIndex] = 0;
}

#endif // PLATFORM_REALTEK
