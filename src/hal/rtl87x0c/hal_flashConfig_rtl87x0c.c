#ifdef PLATFORM_RTL87X0C

#include "../hal_flashConfig.h"
#include <easyflash.h>

static int g_easyFlash_Ready = 0;
void InitEasyFlashIfNeeded()
{
	if(g_easyFlash_Ready == 0)
	{
		easyflash_init();
		g_easyFlash_Ready = 1;
	}
}

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

#endif // PLATFORM_RTL87X0C
