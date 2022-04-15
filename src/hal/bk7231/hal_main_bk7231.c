
#include "../../new_common.h"
#include "../../logging/logging.h"


#include "../../beken378/app/config/param_config.h"


// main timer tick every 1s
beken_timer_t g_main_timer_1s;


#define LOG_FEATURE LOG_FEATURE_MAIN


void user_main(void)
{
    OSStatus err;
	Main_Init();


  err = rtos_init_timer(&g_main_timer_1s,
                        1 * 1000,
                        Main_OnEverySecond,
                        (void *)0);
  ASSERT(kNoErr == err);

  err = rtos_start_timer(&g_main_timer_1s);
  ASSERT(kNoErr == err);
	ADDLOGF_DEBUG("started timer\r\n");
}

#if PLATFORM_BK7231N

// right now Free is somewhere else

#else

#undef Free
// This is needed by tuya_hal_wifi_release_ap.
// How come that the Malloc was not undefined, but Free is?
// That's because Free is defined to os_free. It would be better to fix it elsewhere
void Free(void* ptr)
{
    os_free(ptr);
}


#endif


