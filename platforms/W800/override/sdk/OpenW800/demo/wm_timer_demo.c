
#include <string.h>
#include "wm_include.h"
#include "wm_demo.h"
#include "wm_timer.h"

#if DEMO_TIMER


static void demo_timer_irq(u8 *arg)
{
	printf("timer irq\n");
}


int timer_demo(void)
{

	u8 timer_id;
	struct tls_timer_cfg timer_cfg;
	
	timer_cfg.unit = TLS_TIMER_UNIT_MS;
	timer_cfg.timeout = 2000;
	timer_cfg.is_repeat = 1;
	timer_cfg.callback = (tls_timer_irq_callback)demo_timer_irq;
	timer_cfg.arg = NULL;
	timer_id = tls_timer_create(&timer_cfg);
	tls_timer_start(timer_id);
	printf("timer start\n");	

	return WM_SUCCESS;
}



#endif




