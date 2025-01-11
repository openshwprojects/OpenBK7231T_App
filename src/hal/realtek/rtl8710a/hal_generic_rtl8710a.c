#ifdef PLATFORM_RTL8710A

#include "../../hal_generic.h"
#include "sys_api.h"

extern int g_sleepfactor;

void HAL_RebootModule()
{
	//sys_cpu_reset();
	ota_platform_reset();
}

void HAL_Delay_us(int delay)
{
	HalDelayUs(delay);
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

#endif // PLATFORM_RTL8710A
