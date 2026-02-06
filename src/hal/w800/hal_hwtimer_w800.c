#include "../hal_hwtimer.h"

#if PLATFORM_W800 || PLATFORM_W600

#include "wm_include.h"
#include "wm_timer.h"

int8_t HAL_RequestHWTimer(uint32_t period_us, HWTimerCB callback, void* arg)
{
	struct tls_timer_cfg timer_cfg;

	timer_cfg.unit = TLS_TIMER_UNIT_US;
	timer_cfg.timeout = period_us;
	timer_cfg.is_repeat = 1;
	timer_cfg.callback = callback;
	timer_cfg.arg = arg;
	return tls_timer_create(&timer_cfg);
}

void HAL_HWTimerStart(int8_t timer)
{
	if(timer == -1) return;
	tls_timer_start(timer);
}

void HAL_HWTimerStop(int8_t timer)
{
	if(timer == -1) return;
	tls_timer_stop(timer);
}

void HAL_HWTimerDeinit(int8_t timer)
{
	if(timer == -1) return;
	tls_timer_destroy(timer);
}

#endif
