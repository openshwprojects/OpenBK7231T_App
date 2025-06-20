#ifdef PLATFORM_TR6260

#include "../hal_flashConfig.h"
#include <easyflash.h>

int HAL_Configuration_ReadConfigMemory(void* target, int dataLen)
{
	return ef_get_env_blob("ObkCfg", target, dataLen, NULL);
}

int HAL_Configuration_SaveConfigMemory(void* src, int dataLen)
{
	//ef_del_env("ObkCfg");
	ef_set_env_blob("ObkCfg", src, dataLen);
	return 1;
}

#endif // PLATFORM_TR6260
