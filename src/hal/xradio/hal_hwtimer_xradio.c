#include "../hal_hwtimer.h"

#if PLATFORM_XRADIO

#include "driver/chip/hal_timer.h"

#define FIRST_TIMER TIMER0_ID 
#define MAX_TIMER (TIMER_NUM - 1)

static uint16_t g_used_timers = 0b0;

int8_t HAL_RequestHWTimer(uint32_t period_us, HWTimerCB callback, void* arg)
{
	if(callback == NULL) return -1;
	uint8_t freetimer;
	for(freetimer = FIRST_TIMER; freetimer <= MAX_TIMER; freetimer++)
		if(((g_used_timers >> freetimer) & 1U) == 0) break;
	if(freetimer > MAX_TIMER)
		return -1;
	g_used_timers |= 1 << freetimer;

	TIMER_InitParam param;
	param.arg = arg;
	param.callback = callback;
	param.cfg = HAL_TIMER_MakeInitCfg(TIMER_MODE_REPEAT,
		TIMER_CLK_SRC_HFCLK, TIMER_CLK_PRESCALER_4);
	param.isEnableIRQ = 1;
	param.period = 6 * period_us;

	HAL_Status status = HAL_TIMER_Init(freetimer, &param);
	if(status != HAL_OK)
	{
		g_used_timers &= ~(1 << freetimer);
		return -1;
	}

	return freetimer;
}

void HAL_HWTimerStart(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER) return;
	HAL_TIMER_Start(timer);
}

void HAL_HWTimerStop(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER) return;
	HAL_TIMER_Stop(timer);
}

void HAL_HWTimerDeinit(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER) return;
	HAL_TIMER_DeInit(timer);
	g_used_timers &= ~(1 << timer);
}

#endif
