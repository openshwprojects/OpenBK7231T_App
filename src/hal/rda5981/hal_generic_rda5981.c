#if PLATFORM_RDA5981

#include "../hal_generic.h"
#include "wland_flash.h"
#include "rda_wdt_api.h"
#include "wait_api.h"

static wdt_t obj;

void HAL_RebootModule()
{
	//rda_wdt_softreset();
	rda_sys_softreset();
}

void HAL_Delay_us(int delay)
{
	wait_us(delay);
}

void HAL_Configure_WDT()
{
	rda_wdt_init(&obj, 10);
	rda_wdt_start(&obj);
}

void HAL_Run_WDT()
{
	rda_wdt_feed(&obj);
}

int HAL_FlashRead(char* buffer, int readlen, int startaddr)
{
	return rda5981_read_flash(0x18000000 + startaddr, (uint8_t*)buffer, readlen);
}

int HAL_FlashWrite(char* buffer, int writelen, int startaddr)
{
	return rda5981_write_flash(0x18000000 + startaddr, (uint8_t*)buffer, writelen);
}

int HAL_FlashEraseSector(int startaddr)
{
	return rda5981_erase_flash(0x18000000 + startaddr, 0x1000);
}

#endif // PLATFORM_RDA5981
