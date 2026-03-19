#include "../hal_hwtimer.h"

#if PLATFORM_ECR6600

#include "hal_timer.h"

int8_t HAL_RequestHWTimer(float requestPeriodUs, float* realPeriodUs, HWTimerCB callback, void* arg)
{
	if(realPeriodUs) *realPeriodUs = requestPeriodUs;
	return hal_timer_create((uint32_t)requestPeriodUs, callback, arg, 0);
}

void HAL_HWTimerStart(int8_t timer)
{
	if(timer == -1) return;
	hal_timer_start(timer);
}

void HAL_HWTimerStop(int8_t timer)
{
	if(timer == -1) return;
	hal_timer_stop(timer);
}

void HAL_HWTimerDeinit(int8_t timer)
{
	if(timer == -1) return;
	hal_timer_delete(timer);
}

#endif
