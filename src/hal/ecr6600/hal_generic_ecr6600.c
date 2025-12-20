#ifdef PLATFORM_ECR6600

#include "../hal_generic.h"
#include "os/oshal.h"
#include "hal_system.h"
#include "hal_wdt.h"

void HAL_RebootModule()
{
	hal_system_reset(RST_TYPE_SOFTWARE_REBOOT);
}

void HAL_Delay_us(int delay)
{
	os_usdelay(delay);
}

void HAL_Configure_WDT()
{
	unsigned int effect_time = 0;
	hal_wdt_init(10000, &effect_time);
	hal_wdt_start();
}

void HAL_Run_WDT()
{
	hal_wdt_feed();
}

#endif // PLATFORM_ECR6600
