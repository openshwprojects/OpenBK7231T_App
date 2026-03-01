#include "../hal_hwtimer.h"

#if PLATFORM_REALTEK

#include "timer_api.h"

#define FIRST_TIMER TIMER0

#if PLATFORM_RTL8710A
#define MAX_TIMER GTIMER_MAX
#elif PLATFORM_REALTEK_NEW
#undef FIRST_TIMER
#define FIRST_TIMER TIMER10
#define MAX_TIMER (GTIMER_MAX - 1)
#else
#define MAX_TIMER (GTIMER_MAX - 1)
#endif

#if PLATFORM_RTL8710A
static const uint16_t g_excluded_timers = 1 << TIMER4;
#elif PLATFORM_RTL87X0C
static const uint16_t g_excluded_timers = (1 << TIMER0) | (1 << TIMER4);
#elif PLATFORM_RTL8710B || PLATFORM_RTL8720D
static const uint16_t g_excluded_timers = (1 << TIMER0) | (1 << TIMER1);
//#elif PLATFORM_REALTEK_NEW
//static const uint16_t g_excluded_timers = (1 << TIMER8) | (1 << TIMER9); // timer8 pwm, timer9 ??
#else
static const uint16_t g_excluded_timers = 0b0;
#endif

static gtimer_t hw_timers[MAX_TIMER + 1];
static HWTimerCB timerHandlers[MAX_TIMER + 1];
static void* timerArguments[MAX_TIMER + 1];
static uint32_t timerPeriods[MAX_TIMER + 1];
static uint16_t g_used_timers = 0b0;

int8_t HAL_RequestHWTimer(uint32_t period_us, HWTimerCB callback, void* arg)
{
#if PLATFORM_RTL8720D //|| PLATFORM_RTL8710B //on B too, but this is not needed
	if(period_us < 62) period_us = 62; // timers are using 32khz clock
#endif
	if(callback == NULL) return -1;
	uint8_t freetimer;
	for(freetimer = FIRST_TIMER; freetimer <= MAX_TIMER; freetimer++)
		if(((g_used_timers >> freetimer) & 1U) == 0 && ((g_excluded_timers >> freetimer) & 1U) == 0) break;
	if(freetimer > MAX_TIMER)
		return -1;
	g_used_timers |= 1 << freetimer;

	timerHandlers[freetimer] = callback;
	timerArguments[freetimer] = arg;
	timerPeriods[freetimer] = period_us;
	gtimer_init(&hw_timers[freetimer], freetimer);

	return freetimer;
}

void HAL_HWTimerStart(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER || (g_excluded_timers >> timer) & 1U) return;
	gtimer_start_periodical(&hw_timers[timer], timerPeriods[timer], timerHandlers[timer], timerArguments[timer]);
}

void HAL_HWTimerStop(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER || (g_excluded_timers >> timer) & 1U) return;
	gtimer_stop(&hw_timers[timer]);
}

void HAL_HWTimerDeinit(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER || (g_excluded_timers >> timer) & 1U) return;
	gtimer_deinit(&hw_timers[timer]);
	g_used_timers &= ~(1 << timer);
}

#endif
