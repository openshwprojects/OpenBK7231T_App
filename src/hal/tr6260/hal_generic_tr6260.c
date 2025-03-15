#ifdef PLATFORM_TR6260

#include "../hal_generic.h"

void HAL_RebootModule()
{
	wdt_reset_chip(0);
}

void HAL_Delay_us(int delay)
{
	usdelay(delay);
}

#endif // PLATFORM_TR6260
