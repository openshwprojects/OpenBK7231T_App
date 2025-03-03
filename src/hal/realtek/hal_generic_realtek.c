#ifdef PLATFORM_REALTEK

#include "../hal_generic.h"
#include "sys_api.h"

#ifdef PLATFORM_RTL8710B
extern int g_sleepfactor;
#define haldelay delay * g_sleepfactor
#else
#define haldelay delay
#endif

void HAL_RebootModule()
{
	ota_platform_reset();
}

void HAL_Delay_us(int delay)
{
#if PLATFORM_RTL87X0C
	hal_delay_us(delay);
#elif PLATFORM_RTL8710A
	HalDelayUs(delay);
#else
	DelayUs(haldelay);
#endif
}

void HAL_Configure_WDT()
{
	watchdog_init(10 * 1000);
	watchdog_start();
}

void HAL_Run_WDT()
{
	watchdog_refresh();
}

#endif // PLATFORM_REALTEK
