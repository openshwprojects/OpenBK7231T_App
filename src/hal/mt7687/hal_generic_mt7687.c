#if PLATFORM_MT7687

#include "../hal_generic.h"
#include "hal_sys.h"
#include "hal_cache.h"
#include "hal_wdt.h"
#include "hal_flash.h"
#include "timer.h"

void HAL_RebootModule()
{
	hal_cache_disable();
	hal_cache_deinit();

	hal_sys_reboot(HAL_SYS_REBOOT_MAGIC, WHOLE_SYSTEM_REBOOT_COMMAND);
}

void HAL_Delay_us(int delay)
{
	delay_us(delay);
}

void HAL_Configure_WDT()
{
	hal_wdt_config_t cfg =
	{
		.mode = HAL_WDT_MODE_RESET,
		.seconds = 10,
	};
	hal_wdt_init(&cfg);
	hal_wdt_enable(HAL_WDT_ENABLE_MAGIC);
}

void HAL_Run_WDT()
{
	hal_wdt_feed(HAL_WDT_FEED_MAGIC);
}

int HAL_FlashRead(char* buffer, int readlen, int startaddr)
{
	return hal_flash_read(startaddr, (uint8_t*)buffer, readlen);
}

int HAL_FlashWrite(char* buf, unsigned int len, unsigned int addr)
{
	hal_flash_write(addr, (const uint8_t*)buf, len);
	return 0;
}

int HAL_FlashEraseSector(int startaddr)
{
	return hal_flash_erase(startaddr, HAL_FLASH_BLOCK_4K);
}

#endif // PLATFORM_MT7687
