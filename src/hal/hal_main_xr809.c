#ifdef PLATFORM_XR809

#include "../new_common.h"
#include "../logging/logging.h"

// main timer tick every 1s
OS_Thread_t g_main_timer_1s;

#define HELLOWORLD_THREAD_STACK_SIZE	(1 * 1024)

#define LOG_FEATURE LOG_FEATURE_MAIN

static void helloworld_task(void *arg)
{
	while (1) {
		OS_Sleep(1);
		Main_OnEverySecond();
	}

	OS_ThreadDelete(&g_main_timer_1s);
}

void user_main(void)
{
	Main_Init();


	/* start helloworld task */
	if (OS_ThreadCreate(&g_main_timer_1s,
		                "helloworld",
		                helloworld_task,
		                NULL,
		                OS_THREAD_PRIO_CONSOLE,
		                HELLOWORLD_THREAD_STACK_SIZE) != OS_OK) {
		ADDLOGF_DEBUG("create helloworld failed\n");
		return ;
	}

}
#endif // PLATFORM_XR809