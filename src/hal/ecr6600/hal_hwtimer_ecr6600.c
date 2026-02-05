#include "../hal_hwtimer.h"

#if PLATFORM_ECR6600

#include "hal_timer.h"

int8_t HAL_RequestHWTimer(uint32_t period_us, HWTimerCB callback, void* arg)
{
	return hal_timer_create(period_us, callback, arg, 0);
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
