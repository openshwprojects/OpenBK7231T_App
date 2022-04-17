#ifdef PLATFORM_XR809

#include "../../new_common.h"
#include "../../logging/logging.h"

// main timer tick every 1s
static OS_Thread_t g_main_timer_1s;
static char g_otaRequest[128] = { 0 };
static int g_xr_ota_requested = 0;

#define HELLOWORLD_THREAD_STACK_SIZE	(1 * 1024)

#define LOG_FEATURE LOG_FEATURE_MAIN

void XR809_RequestOTAHTTP(const char *httpPath) {
	strcpy(g_otaRequest,httpPath);
	g_xr_ota_requested = 1;
}
static void helloworld_task(void *arg)
{
	while (1) {
		OS_Sleep(1);
		Main_OnEverySecond();
		if(g_xr_ota_requested) {
			ADDLOGF_DEBUG("Going to call cmd_ota_http_exec with %s\n",g_otaRequest);
			cmd_ota_http_exec(g_otaRequest);
			g_xr_ota_requested = 0;
		}
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