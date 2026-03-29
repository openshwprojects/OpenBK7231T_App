#if PLATFORM_BL_NEW

#include "../hal_flashConfig.h"
#include "../../logging/logging.h"
#include <easyflash.h>

extern void InitEasyFlashIfNeeded();

int HAL_Configuration_ReadConfigMemory(void* target, int dataLen)
{
	InitEasyFlashIfNeeded();
	return ef_get_env_blob("ObkCfg", target, dataLen, NULL);
}

int HAL_Configuration_SaveConfigMemory(void* src, int dataLen)
{
	InitEasyFlashIfNeeded();
	ef_set_env_blob("ObkCfg", src, dataLen);
	return 1;
}

#endif // PLATFORM_BL_NEW
