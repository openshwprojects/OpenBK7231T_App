#ifdef PLATFORM_W800

#include "../../new_common.h"
#include "../../logging/logging.h"

#include <string.h>
#include "wm_include.h"

#define LOG_FEATURE LOG_FEATURE_MAIN



#if 0

void testprintf(){
	printf("Test 123\n\r");
}

static tls_os_timer_t *main_second_timer;

void user_main(void)
{
	Main_Init();

    tls_os_timer_create(&main_second_timer, testprintf,
                        NULL, 1 * HZ, FALSE, NULL);

    tls_os_timer_start(main_second_timer);
}




#elif 0

void Thread_EverySecond(void *sdata)
{
	while(1) {
    	Main_OnEverySecond();
    	//sys_msleep(1000);
    	tls_os_time_delay(1000);
	}
}
#define  DEMO_TASK_PRIO			                32

#define    DEMO_TASK_SIZE      2048
static OS_STK 			DemoTaskStk[DEMO_TASK_SIZE];
#define DEMO_CONSOLE_BUF_SIZE   512

void user_main(void)
{
	Main_Init();

    tls_os_task_create(NULL, NULL,
    		Thread_EverySecond,
                       NULL,
                       (void *)DemoTaskStk,          /* task's stack start address */
                       DEMO_TASK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
                       DEMO_TASK_PRIO,
                       0);

}

#else

void user_main(void)
{
	Main_Init();
    while(1)
    {
    	//sys_msleep(1000);
    	tls_os_time_delay(1000);
    	Main_OnEverySecond();
    }
}

#endif


#endif
