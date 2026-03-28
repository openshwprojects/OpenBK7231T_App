#ifdef PLATFORM_BL602

#include "../hal_flashConfig.h"
#include "../../logging/logging.h"
#include <easyflash.h>

#define EASYFLASH_MY_OBK_CONF "mY0bcFg"

extern void InitEasyFlashIfNeeded();

int HAL_Configuration_ReadConfigMemory(void *target, int dataLen)
{
	InitEasyFlashIfNeeded();
	return ef_get_env_blob(EASYFLASH_MY_OBK_CONF, target, dataLen, NULL);
}

int HAL_Configuration_SaveConfigMemory(void *src, int dataLen)
{
	InitEasyFlashIfNeeded();
	ef_set_env_blob(EASYFLASH_MY_OBK_CONF, src, dataLen);
	return dataLen;
}

#endif // PLATFORM_BL602
