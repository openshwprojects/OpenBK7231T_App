#ifdef PLATFORM_ECR6600

#include "../hal_flashConfig.h"
#include <easyflash.h>
#define ef_get_env_blob obk_get_env_blob
#define ef_set_env_blob obk_set_env_blob

extern int obk_get_env_blob(const char* key, void* value_buf, signed int buf_len, signed int* value_len);
extern EfErrCode obk_set_env_blob(const char* key, const void* value_buf, signed int buf_len);


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

#endif // PLATFORM_ECR6600
