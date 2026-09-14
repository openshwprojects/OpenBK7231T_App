#if PLATFORM_ARMINO

#include "../hal_hwtimer.h"
#include "driver/timer.h"
#include "driver/hal/hal_timer_types.h"

#define FIRST_TIMER TIMER_ID0
#define MAX_TIMER (TIMER_ID_MAX - 1)
static const uint16_t g_excluded_timers = (1 << TIMER_ID0) | (1 << TIMER_ID1) | (1 << TIMER_ID2);

static HWTimerCB timerHandlers[MAX_TIMER + 1];
static void* timerArguments[MAX_TIMER + 1];
static uint16_t g_used_timers = 0b0;
static void BK_ISR_CB(timer_id_t t)
{
	if(t >= FIRST_TIMER && t <= MAX_TIMER && timerHandlers[t]) timerHandlers[t](timerArguments[t]);
}

int8_t HAL_RequestHWTimer(float requestPeriodUs, float* realPeriodUs, HWTimerCB callback, void* arg)
{
	if(realPeriodUs) *realPeriodUs = requestPeriodUs;
	if(callback == NULL) return -1;
	uint8_t freetimer;
	for(freetimer = FIRST_TIMER; freetimer <= MAX_TIMER; freetimer++)
		if(((g_used_timers >> freetimer) & 1U) == 0 && ((g_excluded_timers >> freetimer) & 1U) == 0) break;
	if(freetimer > MAX_TIMER)
		return -1;

	timerHandlers[freetimer] = callback;
	timerArguments[freetimer] = arg;
	if(bk_timer_start_us(freetimer, requestPeriodUs, BK_ISR_CB) != BK_OK)
	{
		return -1;
	}
	bk_timer_disable(freetimer);
	g_used_timers |= 1 << freetimer;
	return freetimer;
}

void HAL_HWTimerStart(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER || (g_excluded_timers >> timer) & 1U) return;
	bk_timer_enable(timer);
}

void HAL_HWTimerStop(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER || (g_excluded_timers >> timer) & 1U) return;
	bk_timer_disable(timer);
}

void HAL_HWTimerDeinit(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER || (g_excluded_timers >> timer) & 1U) return;
	bk_timer_stop(timer);
	g_used_timers &= ~(1 << timer);
}

#endif
