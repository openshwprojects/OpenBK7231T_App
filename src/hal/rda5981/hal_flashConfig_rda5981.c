#ifdef PLATFORM_RDA5981

#include "../hal_flashConfig.h"
#include "wland_flash.h"

int HAL_Configuration_ReadConfigMemory(void* target, int dataLen)
{
	return rda5981_read_flash(0x180fc000, target, dataLen) == 0;
}

int HAL_Configuration_SaveConfigMemory(void* src, int dataLen)
{
	rda5981_erase_flash(0x180fc000, 0x1000);
	return rda5981_write_flash(0x180fc000, src, dataLen) == 0;
}

#endif // PLATFORM_RDA5981
