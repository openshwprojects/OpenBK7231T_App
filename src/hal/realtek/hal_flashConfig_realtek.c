#ifdef PLATFORM_REALTEK

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

#if PLATFORM_RTL8720D || PLATFORM_RTL87X0C

uint32_t ftl_save_to_storage(void* pdata_tmp, uint16_t offset, uint16_t size)
{
	InitEasyFlashIfNeeded();
	char buf[20];
	int n = snprintf(buf, sizeof(buf), "ftl%u", offset);
	return ef_set_env_blob(buf, pdata_tmp, size) != EF_NO_ERR;
}

uint32_t ftl_load_from_storage(void* pdata, uint16_t offset, uint16_t size)
{
	InitEasyFlashIfNeeded();
	char buf[20];
	int n = snprintf(buf, sizeof(buf), "ftl%u", offset);
	return ef_get_env_blob(buf, pdata, size, NULL) == 0;
}

#endif

#endif // PLATFORM_REALTEK
