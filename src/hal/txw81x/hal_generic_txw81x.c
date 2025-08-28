#ifdef PLATFORM_TXW81X

#include "../hal_generic.h"
#include "chip/txw81x/sysctrl.h"

void HAL_RebootModule()
{
	mcu_reset();
}

void HAL_Delay_us(int delay)
{
	delay_us(delay);
}

void HAL_Configure_WDT()
{
	mcu_watchdog_timeout(10);
}

void HAL_Run_WDT()
{
	mcu_watchdog_feed();
}

#endif // PLATFORM_REALTEK
