#include "../hal_hwtimer.h"

#if PLATFORM_ESPIDF

#include "esp_attr.h"
#include "driver/gptimer.h"

#define MAX_TIMERS 8 // actual limit may be different. If there are no more hw timers available, gptimer_new_timer returns an error

typedef void (*HWTimerCB)(void*);

typedef struct
{
	bool used;
	gptimer_handle_t timer;
	HWTimerCB callback;
	void* arg;
} hw_timer_t;

static hw_timer_t hw_timers[MAX_TIMERS];

static bool IRAM_ATTR timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx)
{
	hw_timer_t* t = (hw_timer_t*)user_ctx;
	if(t && t->callback)
	{
		t->callback(t->arg);
	}
	return false;
}

int8_t HAL_RequestHWTimer(uint32_t period_us, HWTimerCB callback, void* arg)
{
	if(callback == NULL) return -1;

	int8_t idx;
	for(idx = 0; idx < MAX_TIMERS; idx++)
	{
		if(!hw_timers[idx].used) break;
	}
	if(idx >= MAX_TIMERS) return -1;

	hw_timer_t tim = hw_timers[idx];

	gptimer_config_t timer_cfg =
	{
		.clk_src = GPTIMER_CLK_SRC_DEFAULT,
		.direction = GPTIMER_COUNT_UP,
		.resolution_hz = 1000000, // 1 tick = 1 us
	};

	esp_err_t err = gptimer_new_timer(&timer_cfg, &tim.timer);
	if(err != ESP_OK) return -1;

	gptimer_alarm_config_t alarm_cfg =
	{
		.alarm_count = period_us,
		.reload_count = 0,
		.flags.auto_reload_on_alarm = true,
	};
	gptimer_set_alarm_action(tim.timer, &alarm_cfg);

	gptimer_event_callbacks_t cbs =
	{
		.on_alarm = timer_cb
	};
	gptimer_register_event_callbacks(tim.timer, &cbs, &tim);
	gptimer_enable(tim.timer);

	tim.used = true;
	tim.callback = callback;
	tim.arg = arg;

	return idx;
}

void HAL_HWTimerStart(int8_t timer)
{
	if(timer < 0 || timer >= MAX_TIMERS) return;
	if(!hw_timers[timer].used) return;

	gptimer_start(hw_timers[timer].timer);
}

void HAL_HWTimerStop(int8_t timer)
{
	if(timer < 0 || timer >= MAX_TIMERS) return;
	if(!hw_timers[timer].used) return;

	gptimer_stop(hw_timers[timer].timer);
}

void HAL_HWTimerDeinit(int8_t timer)
{
	if(timer < 0 || timer >= MAX_TIMERS) return;
	if(!hw_timers[timer].used) return;

	gptimer_disable(hw_timers[timer].timer);
	gptimer_del_timer(hw_timers[timer].timer);
	hw_timers[timer].used = false;
	hw_timers[timer].callback = NULL;
	hw_timers[timer].arg = NULL;
}

#elif PLATFORM_ESP8266

#include "stdbool.h"
#include "driver/hw_timer.h"

static bool isTimerInit = false;

int8_t HAL_RequestHWTimer(uint32_t period_us, HWTimerCB callback, void* arg)
{
	if(isTimerInit) return -1;
	isTimerInit = true;
	if(period_us < 50) period_us = 50;
	hw_timer_init(callback, arg);
	hw_timer_alarm_us(50, false);
	hw_timer_enable(false);
	return 0;
}

void HAL_HWTimerStart(int8_t timer)
{
	if(timer == -1) return;
	hw_timer_enable(true);
}

void HAL_HWTimerStop(int8_t timer)
{
	if(timer == -1) return;
	hw_timer_enable(false);
	//hw_timer_disarm();
}

void HAL_HWTimerDeinit(int8_t timer)
{
	if(timer == -1) return;
	hw_timer_deinit();
	isTimerInit = false;
}

#endif
