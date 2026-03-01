#include "../hal_hwtimer.h"

int8_t __attribute__((weak)) HAL_RequestHWTimer(uint32_t period_us, HWTimerCB callback, void* arg)
{
	return -1;
}

void __attribute__((weak)) HAL_HWTimerStart(int8_t timer)
{
	return;
}

void __attribute__((weak)) HAL_HWTimerStop(int8_t timer)
{
	return;
}

void __attribute__((weak)) HAL_HWTimerDeinit(int8_t timer)
{
	return;
}
