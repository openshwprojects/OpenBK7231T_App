#include "../../new_common.h"
#include "../hal_generic.h"
#include <BkDriverWdg.h>

// from wlan_ui.c
void bk_reboot(void);
extern bool g_powersave;
extern uint64_t rtos_get_time_us();

void HAL_RebootModule() 
{
	bk_reboot();
}

void HAL_Delay_us(int delay)
{
#if PLATFORM_BK7231T
	float adj = 1;
	if(g_powersave) adj = 1.5;
	usleep((17 * delay * adj) / 10); // "1" is to fast and "2" to slow, 1.7 seems better than 1.5 (only from observing readings, no scope involved)
#else
	uint64_t m = (uint64_t)rtos_get_time_us();
	if(delay)
	{
		uint64_t e = (m + delay);
		if(m > e)
		{ //overflow
			while((uint64_t)rtos_get_time_us() > e)
			{
				__asm ("NOP");
			}
		}
		while((uint64_t)rtos_get_time_us() < e)
		{
			__asm ("NOP");
		}
	}
#endif
}

void HAL_Configure_WDT()
{
	bk_wdg_initialize(10000);
}

void HAL_Run_WDT()
{
	bk_wdg_reload();
}
