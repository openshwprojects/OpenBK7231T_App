#if PLATFORM_ARMINO

#include "../hal_flashConfig.h"

extern int HAL_FlashRead(char* buffer, int readlen, int startaddr);
extern int HAL_FlashWrite(char* buf, unsigned int len, unsigned int addr);
extern int HAL_FlashEraseSector(int startaddr);

extern uint32_t g_ota_end_addr;

int HAL_Configuration_ReadConfigMemory(void* target, int dataLen)
{
	return HAL_FlashRead(target, dataLen, g_ota_end_addr - 0x1000);
}

int HAL_Configuration_SaveConfigMemory(void* src, int dataLen)
{
	HAL_FlashEraseSector(g_ota_end_addr - 0x1000);
	HAL_FlashWrite(src, dataLen, g_ota_end_addr - 0x1000);
	return dataLen;
}

#endif // PLATFORM_BK7239N
