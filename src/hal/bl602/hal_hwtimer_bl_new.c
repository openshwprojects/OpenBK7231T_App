#include "../hal_hwtimer.h"

#if PLATFORM_BL_NEW

#include "bflb_timer.h"
#include "bflb_mtimer.h"
#include "board.h"

#define FIRST_TIMER 0
#define MAX_TIMER   1

static struct bflb_device_s* hw_timers[2];
static HWTimerCB g_callbacks[2];
static void* g_args[2];
static uint8_t g_used_timers = 0;

#define TIMER_CLK_DIV 39
#define TIMER_TICK_HZ (40000000 / (TIMER_CLK_DIV + 1))

static void timer_isr(int irq, void* arg)
{
	int timer_id = (int)(uintptr_t)arg;
	struct bflb_device_s* dev = hw_timers[timer_id];

	if(bflb_timer_get_compint_status(dev, TIMER_COMP_ID_0))
	{
		bflb_timer_compint_clear(dev, TIMER_COMP_ID_0);

		if(g_callbacks[timer_id])
		{
			g_callbacks[timer_id](g_args[timer_id]);
		}
	}
}

int8_t HAL_RequestHWTimer(float requestPeriodUs, float* realPeriodUs,
	HWTimerCB callback, void* arg)
{
	if(callback == NULL) return -1;

	uint8_t freetimer;
	for(freetimer = FIRST_TIMER; freetimer <= MAX_TIMER; freetimer++)
	{
		if(((g_used_timers >> freetimer) & 1U) == 0) break;
	}

	if(freetimer > MAX_TIMER)
		return -1;

	g_used_timers |= (1 << freetimer);

	g_callbacks[freetimer] = callback;
	g_args[freetimer] = arg;

	if(freetimer == 0)
		hw_timers[freetimer] = bflb_device_get_by_name("timer0");
	else
		hw_timers[freetimer] = bflb_device_get_by_name("timer1");

	uint32_t ticks = (uint32_t)((requestPeriodUs * TIMER_TICK_HZ) / 1e6);
	if(ticks == 0) ticks = 1;

	if(realPeriodUs)
		*realPeriodUs = (ticks * 1e6f) / TIMER_TICK_HZ;

	struct bflb_timer_config_s cfg = {
		.counter_mode = TIMER_COUNTER_MODE_PROLOAD,
		.clock_source = TIMER_CLKSRC_XTAL,
		.clock_div = TIMER_CLK_DIV,
		.trigger_comp_id = TIMER_COMP_ID_0,
		.comp0_val = ticks,
		.comp1_val = 0xFFFFFFFF,
		.comp2_val = 0xFFFFFFFF,
		.preload_val = 0,
	};

	bflb_timer_init(hw_timers[freetimer], &cfg);

	bflb_irq_attach(hw_timers[freetimer]->irq_num, timer_isr,
		(void*)(uint32_t)freetimer);
	bflb_irq_enable(hw_timers[freetimer]->irq_num);

	return freetimer;
}

void HAL_HWTimerStart(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER) return;
	bflb_timer_start(hw_timers[timer]);
}

void HAL_HWTimerStop(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER) return;
	bflb_timer_stop(hw_timers[timer]);
}

void HAL_HWTimerDeinit(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER) return;

	HAL_HWTimerStop(timer);
	bflb_irq_disable(hw_timers[timer]->irq_num);
	bflb_timer_deinit(hw_timers[timer]);

	g_callbacks[timer] = NULL;
	g_args[timer] = NULL;

	g_used_timers &= ~(1 << timer);
}

#endif