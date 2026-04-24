#if PLATFORM_GD32VW553

#include "config_gdm32.h"
#include "../hal_flashConfig.h"
#include "../../logging/logging.h"

extern int HAL_FlashRead(char* buffer, int readlen, int startaddr);
extern int HAL_FlashWrite(char* buf, unsigned int len, unsigned int addr);
extern int HAL_FlashEraseSector(int startaddr);

int HAL_Configuration_ReadConfigMemory(void *target, int dataLen)
{
	return HAL_FlashRead(target, dataLen, RE_NVDS_DATA_OFFSET - 0x1000);
}

int HAL_Configuration_SaveConfigMemory(void *src, int dataLen)
{
	HAL_FlashEraseSector(RE_NVDS_DATA_OFFSET - 0x1000);
	HAL_FlashWrite(src, dataLen, RE_NVDS_DATA_OFFSET - 0x1000);
	return dataLen;
}

#endif // PLATFORM_GD32VW553
