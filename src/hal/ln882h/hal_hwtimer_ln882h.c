#include "../hal_hwtimer.h"

#if PLATFORM_LN882H

#include "hal/hal_timer.h"
#include "hal/hal_clock.h"

#define MAX_TIMERS 3

static uint8_t g_used_timers = 0b0;
static HWTimerCB timerHandlers[MAX_TIMERS];
static void* timerArguments[MAX_TIMERS];

void TIMER0_IRQHandler()
{
	if(hal_tim_get_it_flag(TIMER0_BASE, TIM_IT_FLAG_ACTIVE))
	{
		hal_tim_clr_it_flag(TIMER0_BASE, TIM_IT_FLAG_ACTIVE);
		if(timerHandlers[0]) timerHandlers[0](timerArguments[0]);
	}
}

void TIMER1_IRQHandler()
{
	if(hal_tim_get_it_flag(TIMER1_BASE, TIM_IT_FLAG_ACTIVE))
	{
		hal_tim_clr_it_flag(TIMER1_BASE, TIM_IT_FLAG_ACTIVE);
		if(timerHandlers[1]) timerHandlers[1](timerArguments[1]);
	}
}

void TIMER2_IRQHandler()
{
	if(hal_tim_get_it_flag(TIMER2_BASE, TIM_IT_FLAG_ACTIVE))
	{
		hal_tim_clr_it_flag(TIMER2_BASE, TIM_IT_FLAG_ACTIVE);
		if(timerHandlers[2]) timerHandlers[2](timerArguments[2]);
	}
}

int8_t HAL_RequestHWTimer(uint32_t period_us, HWTimerCB callback, void* arg)
{
	if(callback == NULL) return -1;
	uint8_t freetimer;
	for(freetimer = 0; freetimer < MAX_TIMERS; freetimer++) if(((g_used_timers >> freetimer) & 1U) == 0) break;
	if(freetimer >= MAX_TIMERS)
		return -1;
	g_used_timers |= 1 << freetimer;
	timerHandlers[freetimer] = callback;
	timerArguments[freetimer] = arg;

	tim_init_t_def tim_init;
	memset(&tim_init, 0, sizeof(tim_init));
	tim_init.tim_mode = TIM_USER_DEF_CNT_MODE;
	if(period_us < 1000)
	{
		tim_init.tim_div = 0;
		tim_init.tim_load_value = period_us * (uint32_t)(hal_clock_get_apb0_clk() / 1000000) - 1;
	}
	else
	{
		tim_init.tim_div = (uint32_t)(hal_clock_get_apb0_clk() / 1000000) - 1;
		tim_init.tim_load_value = period_us - 1;
	}
	hal_tim_init(TIMER_BASE + 0x18U * freetimer, &tim_init);
	hal_tim_clr_it_flag(TIMER_BASE + 0x18U * freetimer, TIM_IT_FLAG_ACTIVE);
	switch(freetimer)
	{
		case 0:
			NVIC_SetPriority(TIMER0_IRQn, 4);
			NVIC_EnableIRQ(TIMER0_IRQn);
			break;
		case 1:
			NVIC_SetPriority(TIMER1_IRQn, 4);
			NVIC_EnableIRQ(TIMER1_IRQn);
			break;
		case 2:
			NVIC_SetPriority(TIMER2_IRQn, 4);
			NVIC_EnableIRQ(TIMER2_IRQn);
			break;
	}

	return freetimer;
}

void HAL_HWTimerStart(int8_t timer)
{
	if(timer == -1 || timer >= MAX_TIMERS) return;
	hal_tim_it_cfg(TIMER_BASE + 0x18U * timer, TIM_IT_FLAG_ACTIVE, HAL_ENABLE);
	hal_tim_en(TIMER_BASE + 0x18U * timer, HAL_ENABLE);
}

void HAL_HWTimerStop(int8_t timer)
{
	if(timer == -1 || timer >= MAX_TIMERS) return;
	hal_tim_en(TIMER_BASE + 0x18U * timer, HAL_DISABLE);
	hal_tim_it_cfg(TIMER_BASE + 0x18U * timer, TIM_IT_FLAG_ACTIVE, HAL_DISABLE);
}

void HAL_HWTimerDeinit(int8_t timer)
{
	if(timer == -1 || timer >= MAX_TIMERS) return;
	switch(timer)
	{
		case 0: NVIC_DisableIRQ(TIMER0_IRQn); break;
		case 1: NVIC_DisableIRQ(TIMER1_IRQn); break;
		case 2: NVIC_DisableIRQ(TIMER2_IRQn); break;
	}
	timerHandlers[timer] = NULL;
	timerArguments[timer] = NULL;
	g_used_timers &= ~(1 << timer);
}

#elif PLATFORM_LN8825

#include "ln88xx.h"
#include "hal/hal_timer.h"
#include "hal/hal_sleep.h"
#define FIRST_TIMER 2
#define MAX_TIMER 4

static uint8_t g_used_timers = 0b0;

int8_t HAL_RequestHWTimer(uint32_t period_us, HWTimerCB callback, void* arg)
{
	if(callback == NULL) return -1;
	uint8_t freetimer;
	for(freetimer = FIRST_TIMER; freetimer <= MAX_TIMER; freetimer++) if(((g_used_timers >> freetimer) & 1U) == 0) break;
	if(freetimer > MAX_TIMER)
		return -1;
	g_used_timers |= 1 << freetimer;

	TIMER_InitTypeDef hwtimer_cfg;

	hwtimer_cfg.index = freetimer;
	hwtimer_cfg.mask = TIMER_MASKED_NO;
	hwtimer_cfg.mode = TIMER_MODE_USERDEFINED;
	hwtimer_cfg.user_freq = 1000 * 1000;
	hwtimer_cfg.timer_cb.cb_func = callback;
	hwtimer_cfg.timer_cb.arg = arg;

	HAL_TIMER_Init(&hwtimer_cfg);
	HAL_TIMER_LoadCount_Set(freetimer, (hwtimer_cfg.user_freq / 1000000) * period_us);
	hal_sleep_register((hal_peripheral_module_t)(14 + freetimer), NULL, NULL, NULL);
	HAL_TIMER_Enable(freetimer, TIMER_DISABLE);
	NVIC_EnableIRQ(TIMER_IRQn);

	return freetimer;
}

void HAL_HWTimerStart(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER) return;
	HAL_TIMER_Enable(timer, TIMER_ENABLE);
}

void HAL_HWTimerStop(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER) return;
	HAL_TIMER_Enable(timer, TIMER_DISABLE);
}

void HAL_HWTimerDeinit(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER) return;
	g_used_timers &= ~(1 << timer);
}

#endif
