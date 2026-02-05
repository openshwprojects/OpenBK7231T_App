#include "../hal_hwtimer.h"

#if PLATFORM_BEKEN

#include "../../new_common.h"
#include "bk_timer_pub.h"
#include "../../logging/logging.h"

#define FIRST_TIMER BKTIMER0
#define MAX_TIMER (BKTIMER_COUNT - 1)

#if PLATFORM_BEKEN_NEW
// timer 4 for task watchdog, timer 1 is in beken378/func/rf_use/arbitrate.h bluetooth?
static const uint16_t g_excluded_timers = /*(1 << BKTIMER1) | */ (1 << BKTIMER2) | (1 << BKTIMER3) | (1 << BKTIMER4);
#else
static const uint16_t g_excluded_timers = (1 << BKTIMER2) | (1 << BKTIMER3);
#endif

static HWTimerCB timerHandlers[MAX_TIMER + 1];
static void* timerArguments[MAX_TIMER + 1];
static uint16_t g_used_timers = 0b0;
static void BK_ISR_CB(UINT8 t)
{
	if(t >= FIRST_TIMER && t <= MAX_TIMER && timerHandlers[t]) timerHandlers[t](timerArguments[t]);
}

int8_t HAL_RequestHWTimer(uint32_t period_us, HWTimerCB callback, void* arg)
{
	if(callback == NULL) return -1;
	uint8_t freetimer;
	for(freetimer = FIRST_TIMER; freetimer <= MAX_TIMER; freetimer++)
		if(((g_used_timers >> freetimer) & 1U) == 0 && ((g_excluded_timers >> freetimer) & 1U) == 0) break;
	if(freetimer > MAX_TIMER)
		return -1;

	timerHandlers[freetimer] = callback;
	timerArguments[freetimer] = arg;

	timer_param_t params = {
		(unsigned char)freetimer,
		(unsigned char)1, // div
		period_us, // us
		BK_ISR_CB
	};
	if(arg == NULL)
	{
		params.t_Int_Handler = (TFUNC)callback;
	}
	UINT32 res = sddev_control((char*)TIMER_DEV_NAME, -1, NULL);
	if(res != 1)
	{
		return -1;
	}
	g_used_timers |= 1 << freetimer;
	sddev_control((char*)TIMER_DEV_NAME, CMD_TIMER_INIT_PARAM_US, &params);
	UINT32 ch = freetimer;
	sddev_control((char*)TIMER_DEV_NAME, CMD_TIMER_UNIT_ENABLE, &ch);

	return freetimer;
}

void HAL_HWTimerStart(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER || (g_excluded_timers >> timer) & 1U) return;
	UINT32 ch = timer;
	sddev_control((char*)TIMER_DEV_NAME, CMD_TIMER_UNIT_ENABLE, &ch);
}

void HAL_HWTimerStop(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER || (g_excluded_timers >> timer) & 1U) return;
	UINT32 ch = timer;
	sddev_control((char*)TIMER_DEV_NAME, CMD_TIMER_UNIT_DISABLE, &ch);
}

void HAL_HWTimerDeinit(int8_t timer)
{
	if(timer < FIRST_TIMER || timer > MAX_TIMER || (g_excluded_timers >> timer) & 1U) return;
	g_used_timers &= ~(1 << timer);
}

#endif
