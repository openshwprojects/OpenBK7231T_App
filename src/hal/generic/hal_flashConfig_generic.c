#include "../hal_flashConfig.h"

int __attribute__((weak)) HAL_Configuration_ReadConfigMemory(void* target, int dataLen)
{
	return 0;
}

int __attribute__((weak)) HAL_Configuration_SaveConfigMemory(void* src, int dataLen)
{
	return 0;
}
