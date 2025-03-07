#if PLATFORM_ECR6600

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "hal_gpio.h"
#include "gpio.h"
#include "pwm.h"
#include "chip_pinmux.h"

extern int g_pwmFrequency;

typedef struct ecrPinMapping_s
{
	const char* name;
	int pin;
} ecrPinMapping_t;

ecrPinMapping_t g_pins[] = {
	{ "P0 (PWM0/TX2)",  GPIO_NUM_0 },
	{ "P1 (PWM1/RX1)",  GPIO_NUM_1 },
	{ "P2 (PWM2/TX1)",  GPIO_NUM_2 },
	{ "P3 (PWM3)",  GPIO_NUM_3 },
	{ "P4 (PWM4)",  GPIO_NUM_4 },
	{ "P5 (RX0)",  GPIO_NUM_5 },
	{ "P6 (TX0)",  GPIO_NUM_6 },
	{ "P7",  GPIO_NUM_7 },
	{ "P8",  GPIO_NUM_8 },
	{ "P9",  GPIO_NUM_9 },
	{ "P10", GPIO_NUM_10 },
	{ "P11", GPIO_NUM_11 },
	{ "P12", GPIO_NUM_12 },
	{ "P13 (TX2)", GPIO_NUM_13 },
	{ "P14 (PWM4/ADC0)", GPIO_NUM_14 },
	{ "P15 (PWM5/ADC1)", GPIO_NUM_15 },
	{ "P16 (PWM2)", GPIO_NUM_16 },
	{ "P17 (PWM5/RX2)", GPIO_NUM_17 },
	{ "P18 (ADC3)", GPIO_NUM_18 },
	{ "P19 (NC)", GPIO_NUM_19 },
	{ "P20 (PWM3/ADC2)", GPIO_NUM_20 },
	{ "P21 (RX0)", GPIO_NUM_21 },
	{ "P22 (PWM0/TX0)", GPIO_NUM_22 },
	{ "P23 (PWM1)", GPIO_NUM_23 },
	{ "P24 (PWM2)", GPIO_NUM_24 },
	{ "P25 (PWM3)", GPIO_NUM_25 },
	{ "VBAT", -1 },
};

static int g_numPins = (sizeof(g_pins) / sizeof(g_pins[0])) - 1;

static uint32_t g_periods[6] = { 0 };

int PIN_GetPWMIndexForPinIndex(int pin)
{
	switch(pin)
	{
		case 0:  return 0;
		case 22: return 0;
		case 1:  return 1;
		case 23: return 1;
		case 2:  return 2;
		case 16: return 2;
		case 24: return 2;
		case 3:  return 3;
		case 25: return 3;
		case 4:  return 4;
		case 14: return 4;
		case 15: return 5;
		case 17: return 5;
		default: return -1;
	}
}

bool HAL_PIN_Set_As_GPIO(int pin)
{
	switch(pin)
	{
		case GPIO_NUM_0:
			PIN_FUNC_SET(IO_MUX_GPIO0, FUNC_GPIO0_GPIO0);
			return true;
		case GPIO_NUM_1:
			PIN_FUNC_SET(IO_MUX_GPIO1, FUNC_GPIO1_GPIO1);
			return true;
		case GPIO_NUM_2:
			PIN_FUNC_SET(IO_MUX_GPIO2, FUNC_GPIO2_GPIO2);
			return true;
		case GPIO_NUM_3:
			PIN_FUNC_SET(IO_MUX_GPIO3, FUNC_GPIO3_GPIO3);
			return true;
		case GPIO_NUM_4:
			PIN_FUNC_SET(IO_MUX_GPIO4, FUNC_GPIO4_GPIO4);
			return true;
		case GPIO_NUM_5:
			PIN_FUNC_SET(IO_MUX_GPIO5, FUNC_GPIO5_GPIO5);
			return true;
		case GPIO_NUM_6:
			PIN_FUNC_SET(IO_MUX_GPIO6, FUNC_GPIO6_GPIO6);
			return true;
		// spi?
		//case GPIO_NUM_7:
		//	PIN_FUNC_SET(IO_MUX_GPIO7, FUNC_GPIO7_GPIO7);
		//	return true;
		//case GPIO_NUM_8:
		//	PIN_FUNC_SET(IO_MUX_GPIO8, FUNC_GPIO8_GPIO8);
		//	return true;
		//case GPIO_NUM_9:
		//	PIN_FUNC_SET(IO_MUX_GPIO9, FUNC_GPIO9_GPIO9);
		//	return true;
		//case GPIO_NUM_10:
		//	PIN_FUNC_SET(IO_MUX_GPIO10, FUNC_GPIO10_GPIO10);
		//	return true;
		//case GPIO_NUM_11:
		//	PIN_FUNC_SET(IO_MUX_GPIO11, FUNC_GPIO11_GPIO11);
		//	return true;
		//case GPIO_NUM_12:
		//	PIN_FUNC_SET(IO_MUX_GPIO12, FUNC_GPIO12_GPIO12);
		//	return true;
		case GPIO_NUM_13:
			PIN_FUNC_SET(IO_MUX_GPIO13, FUNC_GPIO13_GPIO13);
			return true;
		case GPIO_NUM_14:
			PIN_FUNC_SET(IO_MUX_GPIO14, FUNC_GPIO14_GPIO14);
			return true;
		case GPIO_NUM_15:
			PIN_FUNC_SET(IO_MUX_GPIO15, FUNC_GPIO15_GPIO15);
			return true;
		case GPIO_NUM_16:
			PIN_FUNC_SET(IO_MUX_GPIO16, FUNC_GPIO16_GPIO16);
			return true;
		case GPIO_NUM_17:
			PIN_FUNC_SET(IO_MUX_GPIO17, FUNC_GPIO17_GPIO17);
			return true;
		case GPIO_NUM_18:
			PIN_FUNC_SET(IO_MUX_GPIO18, FUNC_GPIO18_GPIO18);
			return true;
		//case GPIO_NUM_19:
		//	PIN_FUNC_SET(IO_MUX_GPIO19, FUNC_GPIO19_GPIO19);
		//	return true;
		case GPIO_NUM_20:
			PIN_FUNC_SET(IO_MUX_GPIO20, FUNC_GPIO20_GPIO20);
			return true;
		case GPIO_NUM_21:
			PIN_FUNC_SET(IO_MUX_GPIO21, FUNC_GPIO21_GPIO21);
			return true;
		case GPIO_NUM_22:
			PIN_FUNC_SET(IO_MUX_GPIO22, FUNC_GPIO22_GPIO22);
			return true;
		case GPIO_NUM_23:
			PIN_FUNC_SET(IO_MUX_GPIO23, FUNC_GPIO23_GPIO23);
			return true;
		case GPIO_NUM_24:
			PIN_FUNC_SET(IO_MUX_GPIO24, FUNC_GPIO24_GPIO24);
			return true;
		case GPIO_NUM_25:
			PIN_FUNC_SET(IO_MUX_GPIO25, FUNC_GPIO25_GPIO25);
			return true;
		default:
			return false;
	}
}

const char* HAL_PIN_GetPinNameAlias(int index)
{
	if(index >= g_numPins + 1)
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
	ecrPinMapping_t* pin = g_pins + index;
	hal_gpio_write(pin->pin, iVal);
}

int HAL_PIN_ReadDigitalInput(int index)
{
	if(index >= g_numPins)
		return 0;
	ecrPinMapping_t* pin = g_pins + index;
	return hal_gpio_read(pin->pin);
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
	if(index >= g_numPins)
		return;
	ecrPinMapping_t* pin = g_pins + index;
	if(HAL_PIN_Set_As_GPIO(pin->pin))
	{
		hal_gpio_dir_set(pin->pin, DRV_GPIO_ARG_DIR_IN);
		hal_gpio_set_pull_mode(pin->pin, DRV_GPIO_ARG_PULL_UP);
	}
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	if(index >= g_numPins)
		return;
	ecrPinMapping_t* pin = g_pins + index;
	if(HAL_PIN_Set_As_GPIO(pin->pin))
	{
		hal_gpio_dir_set(pin->pin, DRV_GPIO_ARG_DIR_IN);
		hal_gpio_set_pull_mode(pin->pin, DRV_GPIO_ARG_PULL_DOWN);
	}
}

void HAL_PIN_Setup_Input(int index)
{
	if(index >= g_numPins)
		return;
	ecrPinMapping_t* pin = g_pins + index;
	if(HAL_PIN_Set_As_GPIO(pin->pin))
	{
		hal_gpio_dir_set(pin->pin, DRV_GPIO_ARG_DIR_IN);
	}
}

void HAL_PIN_Setup_Output(int index)
{
	if(index >= g_numPins)
		return;
	ecrPinMapping_t* pin = g_pins + index;
	if(HAL_PIN_Set_As_GPIO(pin->pin))
	{
		hal_gpio_dir_set(pin->pin, DRV_GPIO_ARG_DIR_OUT);
		hal_gpio_set_pull_mode(pin->pin, DRV_GPIO_ARG_PULL_UP);
	}
}

void ECR_SetPinAsPWM(int pin)
{
	switch(pin)
	{
		case 0:  chip_pwm_pinmux_cfg(PWM_CH0_USED_GPIO0);	break;
		case 22: chip_pwm_pinmux_cfg(PWM_CH0_USED_GPIO22);	break;
		case 1:  chip_pwm_pinmux_cfg(PWM_CH1_USED_GPIO1);	break;
		case 23: chip_pwm_pinmux_cfg(PWM_CH1_USED_GPIO23);	break;
		case 2:  chip_pwm_pinmux_cfg(PWM_CH2_USED_GPIO2);	break;
		case 16: chip_pwm_pinmux_cfg(PWM_CH2_USED_GPIO16);	break;
		case 24: chip_pwm_pinmux_cfg(PWM_CH2_USED_GPIO24);	break;
		case 3:  chip_pwm_pinmux_cfg(PWM_CH3_USED_GPIO3);	break;
		case 25: chip_pwm_pinmux_cfg(PWM_CH3_USED_GPIO25);	break;
		case 4:  chip_pwm_pinmux_cfg(PWM_CH4_USED_GPIO4);	break;
		case 14: chip_pwm_pinmux_cfg(PWM_CH4_USED_GPIO14);	break;
		case 15: chip_pwm_pinmux_cfg(PWM_CH5_USED_GPIO15);	break;
		case 17: chip_pwm_pinmux_cfg(PWM_CH5_USED_GPIO17);	break;
		default: break;
	}
}

void HAL_PIN_PWM_Stop(int index)
{
	if(index >= g_numPins)
		return;
	int ch = PIN_GetPWMIndexForPinIndex(index);
	if(ch < 0) return;
	drv_pwm_stop(ch);
	drv_pwm_close(ch);
	g_periods[ch] = 0;
}

void HAL_PIN_PWM_Start(int index, int freq)
{
	if(index >= g_numPins)
		return;
	int ch = PIN_GetPWMIndexForPinIndex(index);
	if(ch < 0) return;
	if(g_periods[ch] != 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_PINS, "PWM Channel %i is already configured, stopping it!", ch);
		drv_pwm_stop(ch);
		drv_pwm_close(ch);
	}
	g_periods[ch] = (uint32_t)freq;
	ECR_SetPinAsPWM(index);
	drv_pwm_open(ch);
	drv_pwm_start(ch);
}

void HAL_PIN_PWM_Update(int index, float value)
{
	if(index >= g_numPins)
		return;
	int ch = PIN_GetPWMIndexForPinIndex(index);
	if(ch < 0) return;
	if(value < 0)
		value = 0;
	if(value > 100)
		value = 100;
	drv_pwm_config(ch, g_periods[ch], (uint32_t)(value * 10));
	drv_pwm_update(ch);
}

unsigned int HAL_GetGPIOPin(int index)
{
	return index;
}

#endif // PLATFORM_ECR6600
