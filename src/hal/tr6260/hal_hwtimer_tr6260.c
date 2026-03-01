#include "../hal_hwtimer.h"

#if PLATFORM_TR6260

#include "stdbool.h"
#include "drv_timer.h"

static bool isTimerInit = false;

int8_t HAL_RequestHWTimer(uint32_t period_us, HWTimerCB callback, void* arg)
{
	if(isTimerInit) return -1;
	isTimerInit = true;
	hal_timer_init();
	hal_timer_config(period_us, 1);
	hal_timer_callback_register(callback, arg);
}

void HAL_HWTimerStart(int8_t timer)
{
	if(timer == -1) return;
	hal_timer_start();
}

void HAL_HWTimerStop(int8_t timer)
{
	if(timer == -1) return;
	hal_timer_stop();
}

void HAL_HWTimerDeinit(int8_t timer)
{
	if(timer == -1) return;
	hal_timer_callback_unregister();
	isTimerInit = false;
}

#endif
