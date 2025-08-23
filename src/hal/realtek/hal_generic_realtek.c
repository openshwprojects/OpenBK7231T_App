#ifdef PLATFORM_REALTEK

#include "../hal_generic.h"
#include "sys_api.h"
#include "wdt_api.h"

extern void ota_platform_reset(void);

#if PLATFORM_RTL8710B || PLATFORM_RTL8720D
extern int g_sleepfactor;
#define haldelay delay * g_sleepfactor
#else
#define haldelay delay
#endif

void HAL_RebootModule()
{
#if !PLATFORM_REALTEK_NEW
	ota_platform_reset();
#else
	sys_reset();
#endif
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
