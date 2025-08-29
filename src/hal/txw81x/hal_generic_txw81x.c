#if PLATFORM_TXW81X

#include "../hal_generic.h"
#include "chip/txw81x/sysctrl.h"
extern struct spi_nor_flash* obk_flash;

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
	mcu_watchdog_irq_request(HAL_RebootModule);
}

void HAL_Run_WDT()
{
	mcu_watchdog_feed();
}

int HAL_FlashRead(char* buffer, int readlen, int startaddr)
{
	int res = 1;
	spi_nor_read(obk_flash, startaddr, buffer, readlen);
	return res;
}

int HAL_FlashWrite(char* buffer, int writelen, int startaddr)
{
	int res = 1;
	spi_nor_write(obk_flash, startaddr, buffer, writelen);
	return res;
}

int HAL_FlashEraseSector(int startaddr)
{
	int res = 1;
	spi_nor_sector_erase(obk_flash, startaddr);
	return res;
}

#endif // PLATFORM_TXW81X
