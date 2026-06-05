#include "../hal_hwtimer.h"

#if PLATFORM_GD32VW553

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "gd32vw55x_timer.h"
#include "wrapper_os.h"

typedef void (*HWTimerCB)(void* arg);

#define FIRST_TIMER  0

enum
{
	IDX_TIMER5 = 0,
	//IDX_TIMER15,
	//IDX_TIMER16,
	//IDX_TIMER0,
	//IDX_TIMER1,
	//IDX_TIMER2,
	MAX_TIMER,
};

static uint32_t g_timer_bases[MAX_TIMER] =
{
	TIMER5,
	//TIMER15,
	//TIMER16,
	//TIMER0,
	//TIMER1,
	//TIMER2,
};

static IRQn_Type g_timer_irqs[MAX_TIMER] =
{
	TIMER5_IRQn,
	//TIMER15_IRQn,
	//TIMER16_IRQn,
	//TIMER0_UP_IRQn,
	//TIMER1_IRQn,
	//TIMER2_IRQn,
};

static rcu_periph_enum g_timer_rcu[MAX_TIMER] =
{
	RCU_TIMER5,
	//RCU_TIMER15,
	//RCU_TIMER16,
	//RCU_TIMER0,
	//RCU_TIMER1,
	//RCU_TIMER2,
};

static HWTimerCB g_handlers[MAX_TIMER];
static void* g_args[MAX_TIMER];
static uint32_t g_used = 0;

static void timer_set_freq(uint32_t timer, uint32_t freq)
{
	uint32_t clk, tmp;
	uint16_t psc;

	if(timer == TIMER0 || timer == TIMER15 || timer == TIMER16)
	{
		clk = rcu_clock_freq_get(CK_APB2);
		tmp = ((RCU_CFG0 & RCU_CFG0_APB2PSC) >> 11) < 4 ? 0 : 1;
	}
	else
	{
		clk = rcu_clock_freq_get(CK_APB1);
		tmp = ((RCU_CFG0 & RCU_CFG0_APB1PSC) >> 8) < 4 ? 0 : 1;
	}

	psc = ((clk << tmp) / freq) - 1;
	timer_prescaler_config(timer, psc, TIMER_PSC_RELOAD_NOW);
}

static void timer_init_us(uint8_t t, uint32_t us)
{
	timer_parameter_struct p;

	rcu_periph_clock_enable(g_timer_rcu[t]);

	timer_deinit(g_timer_bases[t]);
	timer_struct_para_init(&p);

	p.prescaler = 0;
	p.period = us - 1;
	p.alignedmode = TIMER_COUNTER_EDGE;
	p.counterdirection = TIMER_COUNTER_UP;
	p.clockdivision = TIMER_CKDIV_DIV1;
	p.repetitioncounter = 0;

	timer_init(g_timer_bases[t], &p);

	timer_set_freq(g_timer_bases[t], 1000000);

	timer_interrupt_enable(g_timer_bases[t], TIMER_INT_UP);

	ECLIC_EnableIRQ(g_timer_irqs[t]);
}

int8_t HAL_RequestHWTimer(float requestPeriodUs, float* realPeriodUs, HWTimerCB cb, void* arg)
{
	uint8_t t;

	if(realPeriodUs) *realPeriodUs = requestPeriodUs;
	if(!cb) return -1;

	for(t = FIRST_TIMER; t < MAX_TIMER; t++) if(!(g_used & (1U << t))) break;

	if(t >= MAX_TIMER)
		return -1;

	g_handlers[t] = cb;
	g_args[t] = arg;

	timer_init_us(t, (uint32_t)requestPeriodUs);
	timer_enable(g_timer_bases[t]);

	g_used |= 1U << t;

	return t;
}

void HAL_HWTimerStart(int8_t timer)
{
	if(timer < 0 || timer >= MAX_TIMER) return;
	timer_enable(g_timer_bases[timer]);
}

void HAL_HWTimerStop(int8_t timer)
{
	if(timer < 0 || timer >= MAX_TIMER) return;
	timer_disable(g_timer_bases[timer]);
}

void HAL_HWTimerDeinit(int8_t timer)
{
	if(timer < 0 || timer >= MAX_TIMER) return;

	timer_disable(g_timer_bases[timer]);
	timer_interrupt_disable(g_timer_bases[timer], TIMER_INT_UP);
	ECLIC_DisableIRQ(g_timer_irqs[timer]);

	g_handlers[timer] = 0;
	g_args[timer] = 0;

	g_used &= ~(1U << timer);
}

static inline void timer_isr(uint8_t t)
{
	if(timer_flag_get(g_timer_bases[t], TIMER_FLAG_UP))
	{
		timer_flag_clear(g_timer_bases[t], TIMER_FLAG_UP);

		if(g_handlers[t])
			g_handlers[t](g_args[t]);
	}
}

//void TIMER0_UP_IRQHandler(void) { timer_isr(IDX_TIMER0); }
//void TIMER1_IRQHandler(void) { timer_isr(IDX_TIMER1); }
//void TIMER2_IRQHandler(void) { timer_isr(IDX_TIMER2); }
void TIMER5_IRQHandler(void) { sys_int_enter(); timer_isr(IDX_TIMER5); sys_int_exit(); }
//void TIMER15_IRQHandler(void) { timer_isr(IDX_TIMER15); }
//void TIMER16_IRQHandler(void) { timer_isr(IDX_TIMER16); }

#endif
