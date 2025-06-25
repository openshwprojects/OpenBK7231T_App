#if PLATFORM_XRADIO

#include "../../new_common.h"
#include "../../logging/logging.h"

#include "hal_pinmap_xradio.h"

const char *HAL_PIN_GetPinNameAlias(int index)
{
	if(index < 0 || index >= g_numPins + 1) return "error";
	return g_pins[index].name;
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

#endif
