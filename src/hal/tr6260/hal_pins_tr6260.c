#ifdef PLATFORM_TR6260

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../hal_pins.h"
#include "drv_gpio.h"
#include "drv_pwm.h"
#include "soc_pin_mux.h"

typedef struct trPinMapping_s
{
	const char* name;
	DRV_GPIO_PIN_NAME pin;
	int freq;
} trPinMapping_t;

trPinMapping_t g_pins[] = {
	{ "GPIO0 (PWM0)",	DRV_GPIO_0  },
	{ "GPIO1 (PWM1)",	DRV_GPIO_1  },
	{ "GPIO2 (PWM2)",	DRV_GPIO_2  },
	{ "GPIO3 (PWM3)",	DRV_GPIO_3  },
	{ "GPIO4 (PWM4)",	DRV_GPIO_4  },
	{ "GPIO5 (RX0)",	DRV_GPIO_5  },
	{ "GPIO6 (TX0)",	DRV_GPIO_6  },
	{ "GPIO7",			DRV_GPIO_7  },
	{ "GPIO8",			DRV_GPIO_8  },
	{ "GPIO9",			DRV_GPIO_9  },
	{ "GPIO10",			DRV_GPIO_10 },
	{ "GPIO11",			DRV_GPIO_11 },
	{ "GPIO12",			DRV_GPIO_12 },
	{ "GPIO13 (PWM5)",	DRV_GPIO_13 },
	{ "GPIO14 (PWM3)",	DRV_GPIO_14 },
	{ "GPIO15 (PWM5)",	DRV_GPIO_15 },
	{ "GPIO16",			DRV_GPIO_16 },
	{ "GPIO17",			DRV_GPIO_17 },
	{ "GPIO18",			DRV_GPIO_18 },
	{ "GPIO19",			DRV_GPIO_19 },
	{ "GPIO20 (PWM0)",	DRV_GPIO_20 },
	{ "GPIO21 (PWM1)",	DRV_GPIO_21 },
	{ "GPIO22 (PWM2)",	DRV_GPIO_22 },
#if defined (_USR_TR6260)
	{ "GPIO23 (PWM3)",	DRV_GPIO_23 },
	{ "GPIO24 (PWM4)",	DRV_GPIO_24 },
#else
	{ "GPIO23",			DRV_GPIO_23 },
	{ "GPIO24",			DRV_GPIO_24 },
#endif
};

static int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

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


int PIN_GetPWMIndexForPinIndex(int pin)
{
	switch(pin)
	{
		case 0:
		case 20: return PMW_CHANNEL_0;
		case 1:
		case 21: return PMW_CHANNEL_1;
		case 2:
		case 22: return PMW_CHANNEL_2;
		case 3:
		case 14: return PMW_CHANNEL_3;
		case 4: return PMW_CHANNEL_4;
		case 13:
		case 15: return PMW_CHANNEL_5;
		default: return -1;
	}
}

void HAL_PIN_Set_As_GPIO(DRV_GPIO_PIN_NAME pin)
{
	switch(pin)
	{
		case DRV_GPIO_0:
			PIN_FUNC_SET(IO_MUX0_GPIO0, FUNC_GPIO0_GPIO0);
			break;
		case DRV_GPIO_1:
			PIN_FUNC_SET(IO_MUX0_GPIO1, FUNC_GPIO1_GPIO1);
			break;
		case DRV_GPIO_2:
			PIN_FUNC_SET(IO_MUX0_GPIO2, FUNC_GPIO2_GPIO2);
			break;
		case DRV_GPIO_3:
			PIN_FUNC_SET(IO_MUX0_GPIO3, FUNC_GPIO3_GPIO3);
			break;
		case DRV_GPIO_4:
			PIN_FUNC_SET(IO_MUX0_GPIO4, FUNC_GPIO4_GPIO4);
			break;
		case DRV_GPIO_5:
			PIN_FUNC_SET(IO_MUX0_GPIO5, FUNC_GPIO5_GPIO5);
			break;
		case DRV_GPIO_6:
			PIN_FUNC_SET(IO_MUX0_GPIO6, FUNC_GPIO6_GPIO6);
			break;
		case DRV_GPIO_7:
#ifndef _USR_TR6260S1
			PIN_FUNC_SET(IO_MUX0_GPIO7, FUNC_GPIO7_GPIO7);
			break;
		case DRV_GPIO_8:
			PIN_FUNC_SET(IO_MUX0_GPIO8, FUNC_GPIO8_GPIO8);
			break;
		case DRV_GPIO_9:
			PIN_FUNC_SET(IO_MUX0_GPIO9, FUNC_GPIO9_GPIO9);
			break;
		case DRV_GPIO_10:
			PIN_FUNC_SET(IO_MUX0_GPIO10, FUNC_GPIO10_GPIO10);
			break;
		case DRV_GPIO_11:
			PIN_FUNC_SET(IO_MUX0_GPIO11, FUNC_GPIO11_GPIO11);
			break;
		case DRV_GPIO_12:
			PIN_FUNC_SET(IO_MUX0_GPIO12, FUNC_GPIO12_GPIO12);
#endif
			break;
		case DRV_GPIO_13: /*don't use in gpio mode*/
			PIN_FUNC_SET(IO_MUX0_GPIO13, FUNC_GPIO13_GPIO13);
			break;
		case DRV_GPIO_14:
			PIN_FUNC_SET(IO_MUX0_GPIO14, FUNC_GPIO14_GPIO14);
			break;
		case DRV_GPIO_15:
			PIN_FUNC_SET(IO_MUX0_GPIO15, FUNC_GPIO15_GPIO15);
			break;
		case DRV_GPIO_16: /*don't use in gpio mode*/
			break;
		case DRV_GPIO_17: /*don't use in gpio mode*/
			break;
#ifndef _USR_TR6260S1
		case DRV_GPIO_18:
			PIN_FUNC_SET(IO_MUX0_GPIO18, FUNC_GPIO18_GPIO18);
			break;
		case DRV_GPIO_19:
			PIN_FUNC_SET(IO_MUX0_GPIO19, FUNC_GPIO19_GPIO19);
#endif
			break;
		case DRV_GPIO_20:
			PIN_FUNC_SET(IO_MUX0_GPIO20, FUNC_GPIO20_GPIO20);
			break;
		case DRV_GPIO_21:
			PIN_FUNC_SET(IO_MUX0_GPIO21, FUNC_GPIO21_GPIO21);
			break;
		case DRV_GPIO_22:
			PIN_FUNC_SET(IO_MUX0_GPIO22, FUNC_GPIO22_GPIO22);
			break;
		case DRV_GPIO_23:
#ifndef _USR_TR6260S1
			PIN_FUNC_SET(IO_MUX0_GPIO23, FUNC_GPIO23_GPIO23);
			break;
		case DRV_GPIO_24:
			PIN_FUNC_SET(IO_MUX0_GPIO24, FUNC_GPIO24_GPIO24);
#endif
			break;
		default:
			break;
	}
}

const char* HAL_PIN_GetPinNameAlias(int index)
{
	if(index >= g_numPins)
		return "error";
	return g_pins[index].name;
}

int HAL_PIN_CanThisPinBePWM(int index)
{
	int ch = PIN_GetPWMIndexForPinIndex(index);
	if(ch == -1) return 0;
	return 1;
}

void HAL_PIN_SetOutputValue(int index, int iVal)
{
	if(index >= g_numPins)
		return;
	trPinMapping_t* pin = g_pins + index;
	gpio_write(pin->pin, iVal);
}

int HAL_PIN_ReadDigitalInput(int index)
{
	if(index >= g_numPins)
		return 0;
	trPinMapping_t* pin = g_pins + index;
	//return gpio_read_level(pin->pin);
	int value = -1;
	gpio_read(pin->pin, &value);
	return value;
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
	if(index >= g_numPins)
		return;
	trPinMapping_t* pin = g_pins + index;
	HAL_PIN_Set_As_GPIO(pin->pin);
	DRV_GPIO_CONFIG gpioCfg;
	gpioCfg.GPIO_Pin = pin->pin;
	gpioCfg.GPIO_PullEn = DRV_GPIO_PULL_EN;
	gpioCfg.GPIO_Dir = DRV_GPIO_DIR_INPUT;
	gpioCfg.GPIO_PullType = DRV_GPIO_PULL_TYPE_UP;
	gpio_config(&gpioCfg);
	//gpio_set_dir(pin->pin, DRV_GPIO_DIR_INPUT);
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	if(index >= g_numPins)
		return;
	trPinMapping_t* pin = g_pins + index;
	HAL_PIN_Set_As_GPIO(pin->pin);
	DRV_GPIO_CONFIG gpioCfg;
	gpioCfg.GPIO_Pin = pin->pin;
	gpioCfg.GPIO_PullEn = DRV_GPIO_PULL_EN;
	gpioCfg.GPIO_Dir = DRV_GPIO_DIR_INPUT;
	gpioCfg.GPIO_PullType = DRV_GPIO_PULL_TYPE_DOWN;
	gpio_config(&gpioCfg);
	//gpio_set_dir(pin->pin, DRV_GPIO_DIR_INPUT);
}

void HAL_PIN_Setup_Input(int index)
{
	if(index >= g_numPins)
		return;
	trPinMapping_t* pin = g_pins + index;
	HAL_PIN_Set_As_GPIO(pin->pin);
	DRV_GPIO_CONFIG gpioCfg;
	gpioCfg.GPIO_Pin = pin->pin;
	gpioCfg.GPIO_PullEn = DRV_GPIO_PULL_DIS;
	gpioCfg.GPIO_Dir = DRV_GPIO_DIR_INPUT;
	gpio_config(&gpioCfg);
	//gpio_set_dir(pin->pin, DRV_GPIO_DIR_INPUT);
}

void HAL_PIN_Setup_Output(int index)
{
	if(index >= g_numPins)
		return;
	trPinMapping_t* pin = g_pins + index;
	HAL_PIN_Set_As_GPIO(pin->pin);
	DRV_GPIO_CONFIG gpioCfg;
	gpioCfg.GPIO_Pin = pin->pin;
	gpioCfg.GPIO_PullEn = DRV_GPIO_PULL_EN;
	gpioCfg.GPIO_Dir = DRV_GPIO_DIR_OUTPUT;
	gpioCfg.GPIO_PullType = DRV_GPIO_PULL_TYPE_UP;
	gpio_config(&gpioCfg);
	//gpio_set_dir(pin->pin, DRV_GPIO_DIR_OUTPUT);
}

void HAL_PIN_PWM_Stop(int index)
{
	if(index >= g_numPins)
		return;
	int ch = PIN_GetPWMIndexForPinIndex(index);
	if(ch < 0) return;
	pwm_stop(ch);
	switch(index)
	{
		case 0:
			PIN_FUNC_SET(IO_MUX0_GPIO0, FUNC_GPIO0_GPIO0);
			break;
		case 20:
			PIN_FUNC_SET(IO_MUX0_GPIO20, FUNC_GPIO20_GPIO20);
			break;
		case 1:
			PIN_FUNC_SET(IO_MUX0_GPIO1, FUNC_GPIO1_GPIO1);
			break;
		case 21:
			PIN_FUNC_SET(IO_MUX0_GPIO21, FUNC_GPIO21_GPIO21);
			break;
		case 2:
			PIN_FUNC_SET(IO_MUX0_GPIO2, FUNC_GPIO2_GPIO2);
			break;
		case 22:
			PIN_FUNC_SET(IO_MUX0_GPIO22, FUNC_GPIO22_GPIO22);
			break;
		case 3:
			PIN_FUNC_SET(IO_MUX0_GPIO3, FUNC_GPIO3_GPIO3);
			break;
		case 14:
			PIN_FUNC_SET(IO_MUX0_GPIO14, FUNC_GPIO14_GPIO14);
			break;
		case 4:
			PIN_FUNC_SET(IO_MUX0_GPIO4, FUNC_GPIO4_GPIO4);
			break;
		case 13:
			PIN_FUNC_SET(IO_MUX0_GPIO13, FUNC_GPIO13_GPIO13);
			break;
		case 15:
			PIN_FUNC_SET(IO_MUX0_GPIO15, FUNC_GPIO15_GPIO15);
			break;
	}
}

void HAL_PIN_PWM_Start(int index, int freq)
{
	if(index >= g_numPins)
		return;
	int ch = PIN_GetPWMIndexForPinIndex(index);
	if(ch < 0) return;
	g_pins[index].freq = freq;
	pwm_stop(ch);
	switch(index)
	{
		case 0:
			PIN_FUNC_SET(IO_MUX0_GPIO0, FUNC_GPIO0_PWM_CTRL0);
			break;
		case 20:
			PIN_FUNC_SET(IO_MUX0_GPIO20, FUNC_GPIO20_PWM_CTRL0);
			break;
		case 1:
			PIN_FUNC_SET(IO_MUX0_GPIO1, FUNC_GPIO1_PWM_CTRL1);
			break;
		case 21:
			PIN_FUNC_SET(IO_MUX0_GPIO21, FUNC_GPIO21_PWM_CTRL1);
			break;
		case 2:
			PIN_FUNC_SET(IO_MUX0_GPIO2, FUNC_GPIO2_PWM_CTRL2);
			break;
		case 22:
			PIN_FUNC_SET(IO_MUX0_GPIO22, FUNC_GPIO22_PWM_CTRL2);
			break;
		case 3:
			PIN_FUNC_SET(IO_MUX0_GPIO3, FUNC_GPIO3_PWM_CTRL3);
			break;
		case 14:
			PIN_FUNC_SET(IO_MUX0_GPIO14, FUNC_GPIO14_PWM_CTRL3);
			break;
		case 4:
			PIN_FUNC_SET(IO_MUX0_GPIO4, FUNC_GPIO4_PWM_CTRL4);
			break;
		case 13:
			PIN_FUNC_SET(IO_MUX0_GPIO13, FUNC_GPIO13_PWM_CTRL5);
			break;
		case 15:
			PIN_FUNC_SET(IO_MUX0_GPIO15, FUNC_GPIO15_PWM_CTRL5);
			break;
	}
	pwm_start(ch);
}

void HAL_PIN_PWM_Update(int index, float value)
{
	if(index >= g_numPins)
		return;
	int ch = PIN_GetPWMIndexForPinIndex(index);
	if(ch < 0) return;
	if(value < 0.1)
		value = 0.1;
	if(value >= 100)
		value = 99.9;
	pwm_config(ch, g_pins[index].freq, (uint32_t)(value * 10));
	pwm_start(ch);
}

unsigned int HAL_GetGPIOPin(int index)
{
	return index;
}

OBKInterruptHandler g_handlers[PLATFORM_GPIO_MAX];
OBKInterruptType g_modes[PLATFORM_GPIO_MAX];

void TR6260_Interrupt(int obkPinNum)
{
	if(g_handlers[obkPinNum])
	{
		g_handlers[obkPinNum](obkPinNum);
	}
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function)
{
	if(pinIndex >= g_numPins)
		return;
	g_handlers[pinIndex] = function;
	trPinMapping_t* pin = g_pins + pinIndex;
	gpio_isr_init();
	uint32_t irqmode = DRV_GPIO_INT_NEG_EDGE;
	switch(mode)
	{
		case INTERRUPT_RISING:
			irqmode = DRV_GPIO_INT_POS_EDGE;
			break;
		case INTERRUPT_FALLING:
			irqmode = DRV_GPIO_INT_NEG_EDGE;
			break;
		case INTERRUPT_CHANGE:
			irqmode = DRV_GPIO_INT_DUAL_EDGE;
			break;
	}
	gpio_irq_level(pin->pin, irqmode);
	gpio_irq_callback_register(pin->pin, TR6260_Interrupt, pinIndex);
	gpio_irq_unmusk(pin->pin);
}

void HAL_DetachInterrupt(int pinIndex)
{
	if(pinIndex >= g_numPins || g_handlers[pinIndex] == 0)
		return;
	trPinMapping_t* pin = g_pins + pinIndex;
	gpio_irq_musk(pin->pin);
	g_handlers[pinIndex] = 0;
}

#endif // PLATFORM_TR6260
