
#include <string.h>
#include "wm_include.h"
#include "wm_demo.h"
#include "wm_pmu.h"



#if DEMO_PMU //DEMO_PMU_TIMER


static void pmu_timer0_irq(u8 *arg)
{
    tls_pmu_timer0_stop();
    printf("pmu timer0 irq\n");
}

static void pmu_timer1_irq(u8 *arg)
{
    tls_pmu_timer1_stop();
    printf("pmu timer1 irq\n");
}

int pmu_timer0_demo(u8 enterStandby)
{
	tls_pmu_clk_select(0);/*0:select 32K RC osc, 1: select 40M divide clock*/
    tls_pmu_timer0_isr_register((tls_pmu_irq_callback)pmu_timer0_irq, NULL);
    tls_pmu_timer0_start(10);
    printf("pmu timer0 test start\n");

    if(enterStandby)
    {
        printf("pmu will standby\n");
        tls_pmu_standby_start();/*If you want to verify sleep function, using function tls_pmu_sleep_start()*/
        printf("pmu enter standby\n");
    }
    return 	WM_SUCCESS;
}

int pmu_timer1_demo(u8 enterStandby)
{
	tls_pmu_clk_select(0);/*0:select 32K RC osc, 1: select 40M divide clock*/
    tls_pmu_timer1_isr_register((tls_pmu_irq_callback)pmu_timer1_irq, NULL);
    tls_pmu_timer1_start(5000);
    printf("pmu timer1 test start\n");

    if(enterStandby)
    {
        printf("pmu will standby\n");
        tls_pmu_standby_start();/*If you want to verify sleep function, using function tls_pmu_sleep_start()*/
        printf("pmu enter standby\n");
    }
    return 	WM_SUCCESS;
}


#endif

