#include "../hal_hwtimer.h"

#if PLATFORM_BL602

#include "bl602_timer.h"
#include "hosal_timer.h"

#define FIRST_TIMER 0
#define MAX_TIMER 1

static hosal_timer_dev_t hw_timers[2];
static uint8_t g_used_timers = 0b0;

int8_t HAL_RequestHWTimer(uint32_t period_us, HWTimerCB callback, void* arg)
{
	if(callback == NULL) return -1;
	uint8_t freetimer;
	for(freetimer = FIRST_TIMER; freetimer <= MAX_TIMER; freetimer++) if(((g_used_timers >> freetimer) & 1U) == 0) break;
	if(freetimer > MAX_TIMER)
		return -1;
	g_used_timers |= 1 << freetimer;

	hw_timers[freetimer].port = freetimer;
	hw_timers[freetimer].config.period = period_us;
	hw_timers[freetimer].config.reload_mode = TIMER_RELOAD_PERIODIC;
	hw_timers[freetimer].config.cb = callback;
	hw_timers[freetimer].config.arg = arg;
	hosal_timer_init(&hw_timers[freetimer]);

	return freetimer;
}

void HAL_HWTimerStart(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER) return;
	hosal_timer_start(&hw_timers[timer]);
}

void HAL_HWTimerStop(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER) return;
	hosal_timer_stop(&hw_timers[timer]);
}

void HAL_HWTimerDeinit(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER) return;
	hosal_timer_finalize(&hw_timers[timer]);
	g_used_timers &= ~(1 << timer);
}

#endif
