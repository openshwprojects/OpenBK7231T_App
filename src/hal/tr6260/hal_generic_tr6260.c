#ifdef PLATFORM_TR6260

#include "stdint.h"
#include "../hal_generic.h"
#include "drv_spiflash.h"

void HAL_RebootModule()
{
	wdt_reset_chip(0);
}

void HAL_Delay_us(int delay)
{
	usdelay(delay);
}

int HAL_FlashRead(char* buffer, int readlen, int startaddr)
{
	int res;
	res = hal_spiflash_read(startaddr, (uint8_t*)buffer, readlen);
	return res;
}

#endif // PLATFORM_TR6260
