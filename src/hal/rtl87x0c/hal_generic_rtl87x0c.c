#ifdef PLATFORM_RTL87X0C

#include "../hal_generic.h"
#include "sys_api.h"
#include "hal_timer.h"
#include "hal_wdt.h"

void HAL_RebootModule()
{
	sys_cpu_reset();
}

void HAL_Delay_us(int delay)
{
	hal_delay_us(delay);
}

void HAL_Configure_WDT()
{
	hal_misc_wdt_init(10 * 1000 * 1000);
	hal_misc_wdt_enable();
}

void HAL_Run_WDT()
{
	hal_misc_wdt_refresh();
}

#endif // PLATFORM_RTL87X0C
