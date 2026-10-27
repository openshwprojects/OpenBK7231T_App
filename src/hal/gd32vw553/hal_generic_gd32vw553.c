#if PLATFORM_GD32VW553

#include "stdint.h"
#include "bsp.h"
#include "systime.h"
#include "wdt.h"
#include "raw_flash_api.h"
#include "../hal_generic.h"

void HAL_RebootModule()
{
	platform_reset();
}

void HAL_Delay_us(int delay)
{
	systick_udelay(delay);
}

void HAL_Configure_WDT()
{
	fwdgt_init(15000);
	fwdgt_start();
}

void HAL_Run_WDT()
{
	fwdgt_refresh();
}

int HAL_FlashRead(char* buffer, int readlen, int startaddr)
{
	raw_flash_read(startaddr, (uint8_t*)buffer, readlen);
	return readlen;
}

int HAL_FlashWrite(char* buf, unsigned int len, unsigned int addr)
{
	raw_flash_write(addr, (uint8_t*)buf, len);
	return 0;
}

int HAL_FlashEraseSector(int startaddr)
{
	raw_flash_erase(startaddr, 0x1000);
	return 0;
}

#endif // PLATFORM_GD32VW553
