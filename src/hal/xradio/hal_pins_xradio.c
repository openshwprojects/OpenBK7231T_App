#if PLATFORM_XRADIO

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../hal_pins.h"

#include "hal_pinmap_xradio.h"

const char *HAL_PIN_GetPinNameAlias(int index)
{
	if(index < 0 || index >= g_numPins + 1) return "error";
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
//	for (int i = 0; i < g_numPins; i++) {
// no "g_numPins" for Windows - use PLATFORM_GPIO_MAX instead
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (str_match_in_alias(HAL_PIN_GetPinNameAlias(i), name)) {
			return i;
		}
	}
	return -1;
}

void HAL_PIN_SetOutputValue(int index, int iVal)
{
	if(index >= g_numPins)
		return;

	HAL_GPIO_WritePin(g_pins[index].port, g_pins[index].pin, iVal);
}

int HAL_PIN_ReadDigitalInput(int index)
{
	if(index >= g_numPins)
		return 0;

	if (HAL_GPIO_ReadPin(g_pins[index].port, g_pins[index].pin) == GPIO_PIN_LOW)
		return 0;
	return 1;
}

void HAL_XR_ConfigurePin(GPIO_Port port, GPIO_Pin pin, GPIO_WorkMode mode, GPIO_PullType pull)
{
	GPIO_InitParam param;
	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = mode;
	param.pull = pull;
	HAL_GPIO_Init(port, pin, &param);
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	if(index >= g_numPins)
		return;
	HAL_XR_ConfigurePin(g_pins[index].port, g_pins[index].pin, GPIOx_Pn_F0_INPUT, GPIO_PULL_DOWN);
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
	if(index >= g_numPins)
		return;
	HAL_XR_ConfigurePin(g_pins[index].port, g_pins[index].pin, GPIOx_Pn_F0_INPUT, GPIO_PULL_UP);
}

void HAL_PIN_Setup_Input(int index)
{
	if(index >= g_numPins)
		return;
	HAL_XR_ConfigurePin(g_pins[index].port, g_pins[index].pin, GPIOx_Pn_F0_INPUT, GPIO_PULL_NONE);
}

void HAL_PIN_Setup_Output(int index)
{
	if(index >= g_numPins)
		return;
	HAL_XR_ConfigurePin(g_pins[index].port, g_pins[index].pin, GPIOx_Pn_F1_OUTPUT, GPIO_PULL_NONE);
}

int HAL_PIN_CanThisPinBePWM(int index)
{
	if(index >= g_numPins)
		return 0;
	if(g_pins[index].pwm == -1) return 0;
	else return 1;
}

void HAL_PIN_PWM_Stop(int index)
{
	if(index >= g_numPins)
		return;
	HAL_PWM_EnableCh(g_pins[index].pwm, PWM_CYCLE_MODE, 0);
	HAL_PWM_ChDeinit(g_pins[index].pwm);

	HAL_XR_ConfigurePin(g_pins[index].port, g_pins[index].pin, GPIOx_Pn_F7_DISABLE, GPIO_PULL_NONE);
}

void HAL_PIN_PWM_Start(int index, int freq)
{
	if(index >= g_numPins)
		return;
	HAL_Status status = HAL_ERROR;
	PWM_ClkParam clk_param;
	PWM_ChInitParam ch_param;

	clk_param.clk = PWM_CLK_HOSC;
	clk_param.div = PWM_SRC_CLK_DIV_1;
	status = HAL_PWM_GroupClkCfg(g_pins[index].pwm / 2, &clk_param);
	if(status != HAL_OK) printf("HAL_PWM_GroupClkCfg error\r\n");

	ch_param.hz = freq;
	ch_param.mode = PWM_CYCLE_MODE;
	ch_param.polarity = PWM_HIGHLEVE;
	g_pins[index].max_duty = HAL_PWM_ChInit(g_pins[index].pwm, &ch_param);
	if(g_pins[index].max_duty == -1) printf("HAL_PWM_ChInit error\r\n");

	status = HAL_PWM_EnableCh(g_pins[index].pwm, PWM_CYCLE_MODE, 1);
	if(status != HAL_OK) printf("HAL_PWM_EnableCh error\r\n");

	HAL_XR_ConfigurePin(g_pins[index].port, g_pins[index].pin, g_pins[index].pinmux_pwm, GPIO_PULL_NONE);
}

void HAL_PIN_PWM_Update(int index, float value)
{
	if(index >= g_numPins)
		return;
	if(g_pins[index].max_duty < 0) return;

	if(value < 0)
		value = 0;
	if(value > 100)
		value = 100;

	HAL_PWM_ChSetDutyRatio(g_pins[index].pwm, (g_pins[index].max_duty / 100) * value);
}

OBKInterruptHandler g_handlers[PLATFORM_GPIO_MAX];
OBKInterruptType g_modes[PLATFORM_GPIO_MAX];

void XRadio_Interrupt(void* context) {
	int obkPinNum = (int)context;
	if (g_handlers[obkPinNum]) {
		g_handlers[obkPinNum](obkPinNum);
	}
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {
	g_handlers[pinIndex] = function;

	xrpin_t *xr_cf = g_pins + pinIndex;
	HAL_XR_ConfigurePin(xr_cf->port, xr_cf->pin, GPIOx_Pn_F6_EINT, GPIO_PULL_UP);
	GPIO_IrqParam cfparam;
	int xr_mode;
	if (mode == INTERRUPT_RISING) {
		xr_mode = GPIO_IRQ_EVT_RISING_EDGE;
	}
	else {
		xr_mode = GPIO_IRQ_EVT_FALLING_EDGE;
	}
	cfparam.event = xr_mode;
	cfparam.callback = XRadio_Interrupt;
	cfparam.arg = (void*)pinIndex;
	HAL_GPIO_EnableIRQ(xr_cf->port, xr_cf->pin, &cfparam);

}
void HAL_DetachInterrupt(int pinIndex) {
	if (g_handlers[pinIndex] == 0) {
		return; // already removed;
	}
	xrpin_t* xr_cf;
	xr_cf = g_pins + pinIndex;
	HAL_GPIO_DeInit(xr_cf->port, xr_cf->pin);
	HAL_GPIO_DisableIRQ(xr_cf->port, xr_cf->pin);
	g_handlers[pinIndex] = 0;
}


#endif
