#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"


int __attribute__((weak)) PIN_GetPWMIndexForPinIndex(int pin)
{
	return -1;
}

const char* __attribute__((weak)) HAL_PIN_GetPinNameAlias(int index)
{
	return "error";
}

int __attribute__((weak)) HAL_PIN_CanThisPinBePWM(int index)
{
	return 0;
}

void __attribute__((weak)) HAL_PIN_SetOutputValue(int index, int iVal)
{
	return;
}

int __attribute__((weak)) HAL_PIN_ReadDigitalInput(int index)
{
	return 0;
}

void __attribute__((weak)) HAL_PIN_Setup_Input_Pullup(int index)
{
	return;
}

void __attribute__((weak)) HAL_PIN_Setup_Input_Pulldown(int index)
{
	return;
}

void __attribute__((weak)) HAL_PIN_Setup_Input(int index)
{
	return;
}

void __attribute__((weak)) HAL_PIN_Setup_Output(int index)
{
	return;
}

void __attribute__((weak)) HAL_PIN_PWM_Stop(int index)
{
	return;
}

void __attribute__((weak)) HAL_PIN_PWM_Start(int index)
{
	return;
}

void __attribute__((weak)) HAL_PIN_PWM_Update(int index, float value)
{
	return;
}

unsigned int __attribute__((weak)) HAL_GetGPIOPin(int index)
{
	return index;
}
