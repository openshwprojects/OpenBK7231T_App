#ifdef PLATFORM_BL602

#include "../hal_generic.h"
#include "../../new_common.h"
#include <hal_sys.h>
#include <bl_wdt.h>
#include <bl_timer.h>

void HAL_RebootModule()
{
	hal_reboot();
}

void HAL_Delay_us(int delay)
{
	bl_timer_delay_us(delay);
}

void HAL_Configure_WDT()
{
	bl_wdt_init(3000);
}

void HAL_Run_WDT()
{
	bl_wdt_feed();
}

#endif // PLATFORM_BL602
