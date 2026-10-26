#if PLATFORM_ARMINO

#include "stdint.h"
#include "driver/wdt.h"
#include "driver/flash.h"
#include "bk_misc.h"
#include "../hal_generic.h"

void HAL_RebootModule()
{
	bk_reboot();
}

void HAL_Delay_us(int delay)
{
	bk_delay_us(delay);
}

void HAL_Configure_WDT()
{
	bk_wdt_driver_init();
	bk_wdt_start(15000);
}

void HAL_Run_WDT()
{
	bk_wdt_feed();
}

int HAL_FlashRead(char* buffer, int readlen, int startaddr)
{
	bk_flash_read_bytes(startaddr, (uint8_t*)buffer, (unsigned long)readlen);
	return readlen;
}

int HAL_FlashWrite(char* buf, unsigned int len, unsigned int addr)
{
	flash_protect_type_t protect_type = bk_flash_get_protect_type();
	if(FLASH_PROTECT_NONE != protect_type)
	{
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	}
	bk_flash_write_bytes(addr, (const uint8_t*)buf, len);
	if(FLASH_PROTECT_NONE != protect_type)
	{
		bk_flash_set_protect_type(protect_type);
	}
	return 0;
}

int HAL_FlashEraseSector(int startaddr)
{
	flash_protect_type_t protect_type = bk_flash_get_protect_type();
	if(FLASH_PROTECT_NONE != protect_type)
	{
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	}
	bk_flash_erase_sector(startaddr & 0x00FFF000);
	if(FLASH_PROTECT_NONE != protect_type)
	{
		bk_flash_set_protect_type(protect_type);
	}
	return 0;
}

#endif // PLATFORM_ARMINO
