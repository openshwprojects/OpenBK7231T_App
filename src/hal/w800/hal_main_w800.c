#if defined(PLATFORM_W800) || defined(PLATFORM_W600)

#include "../../new_common.h"
#include "../../logging/logging.h"

#include <string.h>
#include "wm_include.h"

#define LOG_FEATURE LOG_FEATURE_MAIN

#if defined(PLATFORM_W800)
//portTICK_RATE_MS is not defined in portmacro.h for W800. Copying this from W600.

#define portTICK_RATE_MS			( ( portTickType ) 1000 / configTICK_RATE_HZ )
#endif

#if 0

void testprintf() {
	printf("Test 123\n\r");
}

static tls_os_timer_t* main_second_timer;

void user_main(void)
{
	Main_Init();

	tls_os_timer_create(&main_second_timer, testprintf,
		NULL, 1 * HZ, FALSE, NULL);

	tls_os_timer_start(main_second_timer);
}





#elif 0

void Thread_EverySecond(void* sdata)
{
	while (1) {
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
		(void*)DemoTaskStk,          /* task's stack start address */
		DEMO_TASK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
		DEMO_TASK_PRIO,
		0);

}

#else

// static void print_rtc()
// {
// 	struct tm tblock;
// 	tls_get_rtc(&tblock);
// 	printf("\nrtc clock, sec=%d,min=%d,hour=%d,mon=%d,year=%d\n", tblock.tm_sec, tblock.tm_min, tblock.tm_hour, tblock.tm_mon, tblock.tm_year);
// }
// static void init_rtc() {
// 	struct tm tblock;
// 	tblock.tm_year = 2022;
// 	tblock.tm_mon = 1;
// 	tblock.tm_mday = 1;
// 	tblock.tm_hour = 0;
// 	tblock.tm_min = 0;
// 	tblock.tm_sec = 0;
// 	tls_set_rtc(&tblock);
// }

void user_main(void)
{
	//vTaskDelay delays by number of ticks, so to delay 1000ms we need to divide it by
	//portTICK_RATE_MS. On W600, portTICK_RATE_MS is 2.

	//Ideally, we would want to use tls_os_timer_start or tls_os_task_create but both
	//of them seem to cause a hang so I am contunuing with delay.

	//Since we are spending some time processing Main_OnEverySecond so delaying by 1000ms
	//actually starts shifting g_secondsElapsed. To compensate, we are checking how much
	//time was spent in Main_OnEverySecond and adjusting the delay.
	//If needed, then more precise time details can be obtained using RTC for debugging.

	u32 delay_duration = 1000;
	portTickType delay_ticks = delay_duration / portTICK_RATE_MS;
	//printf("\nportTICK_RATE_MS=%d\n", portTICK_RATE_MS);

	Main_Init();
	//init_rtc();
	while (1)
	{
		//sys_msleep(1000);
		tls_os_time_delay(delay_ticks);

		//print_rtc();
		u32 before_time = tls_os_get_time();
		Main_OnEverySecond();
		u32 time_spent = tls_os_get_time() - before_time;

		delay_duration = 1000;

		if (time_spent > 0) {
			if (time_spent < delay_duration)	//Reduce delay but don't let it become -ve
			{
				//printf("time_spent=%d\n", time_spent);
				delay_duration = delay_duration - time_spent;
			}
		}

		delay_ticks = delay_duration / portTICK_RATE_MS;
	}
}

#endif


#endif
