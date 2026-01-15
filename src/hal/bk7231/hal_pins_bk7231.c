
#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../hal_pins.h"
//#include "../../new_pins.h"
#include <gpio_pub.h>

#include "../../beken378/func/include/net_param_pub.h"
#include "../../beken378/func/user_driver/BkDriverPwm.h"
#include "../../beken378/func/user_driver/BkDriverI2c.h"
#include "../../beken378/driver/i2c/i2c1.h"
#include "../../beken378/driver/gpio/gpio.h"

// must fit all pwm indexes
static uint32_t g_periods[6] = { 0 };
static bool g_pwm_on[6] = { false };

int PIN_GetPWMIndexForPinIndex(int pin) 
{
	switch(pin)
	{
		case 6:		return 0;
		case 7:		return 1;
		case 8:		return 2;
		case 9:		return 3;
		case 24:	return 4;
		case 26:	return 5;
		default:	return -1;
	}
}

const char *HAL_PIN_GetPinNameAlias(int index) 
{
	// some of pins have special roles
	switch(index)
	{
#if PLATFORM_BK7238
		case 1:		return "RXD2/ADC5";
		case 10:	return "RXD1/ADC6";
		case 20:	return "ADC3";
		case 28:	return "ADC4";
#else
		case 1:		return "RXD2";
		case 10:	return "RXD1";
		case 23:	return "ADC3";
		case 13:	return "ADC7";
#endif
#if PLATFORM_BK7238 || PLATFORM_BK7231N
		case 24:	return "PWM4/ADC2";
		case 26:	return "PWM5/ADC1";
#else
		case 24:	return "PWM4";
		case 26:	return "PWM5";
#endif
		case 0:		return "TXD2";
		case 11:	return "TXD1";
		case 6:		return "PWM0";
		case 7:		return "PWM1";
		case 8:		return "PWM2";
		case 9:		return "PWM3";
#if PLATFORM_BK7231N
		case 28:	return "ADC4";
		case 22:	return "ADC5";
		case 21:	return "ADC6";
#elif !PLATFORM_BK7238
		case 4:		return "ADC1";
		case 5:		return "ADC2";
		case 2:		return "ADC4";
		case 3:		return "ADC5";
		case 12:	return "ADC6";
#endif
		default:	return "N/A";
	}
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
//	for (int i = 0; i < g_numPins; i++) {
// no "g_numPins" for Beken - use PLATFORM_GPIO_MAX instead
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (str_match_in_alias(HAL_PIN_GetPinNameAlias(i), name)) {
			return i;
		}
	}
	return -1;
}


int HAL_PIN_CanThisPinBePWM(int index) {
	int pwmIndex = PIN_GetPWMIndexForPinIndex(index);
	if(pwmIndex == -1)
		return 0;
	return 1;
}
void HAL_PIN_SetOutputValue(int index, int iVal) {
	gpio_output(index, iVal);
}

int HAL_PIN_ReadDigitalInput(int index) {
	return gpio_input(index);
}
void HAL_PIN_Setup_Input_Pullup(int index) {
	gpio_config(index, GMODE_INPUT_PULLUP);
}
void HAL_PIN_Setup_Input_Pulldown(int index) {
	gpio_config(index, GMODE_INPUT_PULLDOWN);
}
void HAL_PIN_Setup_Input(int index) {
	gpio_config(index, GMODE_INPUT);
}
void HAL_PIN_Setup_Output(int index) {
	gpio_config(index, GMODE_OUTPUT);
	gpio_output(index, 0);
}
void HAL_PIN_PWM_Stop(int index) {
	int pwmIndex;

	pwmIndex = PIN_GetPWMIndexForPinIndex(index);

	// is this pin capable of PWM?
	if(pwmIndex == -1) {
		return;
	}

	g_periods[pwmIndex] = 0;
	bk_pwm_stop(pwmIndex);
	g_pwm_on[pwmIndex] = false;
}

void HAL_PIN_PWM_Start(int index, int freq) {
	int pwmIndex;

	pwmIndex = PIN_GetPWMIndexForPinIndex(index);

	// is this pin capable of PWM?
	if(pwmIndex == -1) {
		return;
	}

	uint32_t period = (26000000 / freq);

	g_periods[pwmIndex] = period;
	if(g_pwm_on[pwmIndex] == true) return;
#if defined(PLATFORM_BK7231N) && !defined(PLATFORM_BEKEN_NEW)
	// OSStatus bk_pwm_initialize(bk_pwm_t pwm, uint32_t frequency, uint32_t duty_cycle);
	bk_pwm_initialize(pwmIndex, period, 0, 0, 0);
#else
	bk_pwm_initialize(pwmIndex, period, 0);
#endif
	bk_pwm_start(pwmIndex);
	g_pwm_on[pwmIndex] = true;
}
void HAL_PIN_PWM_Update(int index, float value) {
	int pwmIndex;

	pwmIndex = PIN_GetPWMIndexForPinIndex(index);

	// is this pin capable of PWM?
	if(pwmIndex == -1) {
		return;
	}
	if(value<0)
		value = 0;
	if(value>100)
		value = 100;

	//uint32_t value_upscaled = value * 10.0f; //Duty cycle 0...100 -> 0...1000
	uint32_t period = g_periods[pwmIndex];
	uint32_t duty = (value / 100.0 * period); //No need to use upscaled variable
#if defined(PLATFORM_BK7231N) && !defined(PLATFORM_BEKEN_NEW)
	bk_pwm_update_param(pwmIndex, period, duty,0,0);
#else
	bk_pwm_update_param(pwmIndex, period, duty);
#endif
}

unsigned int HAL_GetGPIOPin(int index) {
	return index;
}

OBKInterruptHandler g_handlers[PLATFORM_GPIO_MAX];
OBKInterruptType g_modes[PLATFORM_GPIO_MAX];

#include "BkDriverTimer.h"
#include "BkDriverGpio.h"
#include "sys_timer.h"
#include "gw_intf.h"

void Beken_Interrupt(unsigned char pinNum) {
	if (g_handlers[pinNum]) {
		g_handlers[pinNum](pinNum);
	}
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {
	g_handlers[pinIndex] = function;
	int bk_mode;
	if (mode == INTERRUPT_RISING) {
		bk_mode = IRQ_TRIGGER_RISING_EDGE;
	}
	else {
		bk_mode = IRQ_TRIGGER_FALLING_EDGE;
	}
	gpio_int_enable(pinIndex, bk_mode, Beken_Interrupt);
}
void HAL_DetachInterrupt(int pinIndex) {
	if (g_handlers[pinIndex] == 0) {
		return; // already removed;
	}
	gpio_int_disable(pinIndex);
	g_handlers[pinIndex] = 0;
}

