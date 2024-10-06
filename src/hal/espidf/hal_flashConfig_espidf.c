#ifdef PLATFORM_ESPIDF

#include "../hal_flashConfig.h"
#include "nvs_flash.h"
#include "nvs.h"

void InitFlashIfNeeded();

int HAL_Configuration_ReadConfigMemory(void* target, int dataLen)
{
	InitFlashIfNeeded();
	nvs_handle_t handle = 0;
	nvs_open("config", NVS_READONLY, &handle);
	nvs_get_blob(handle, "cfg", target, (size_t*) &dataLen);
	nvs_close(handle);

	return dataLen;
}

int HAL_Configuration_SaveConfigMemory(void* src, int dataLen)
{
	InitFlashIfNeeded();
	nvs_handle_t handle = 0;
	nvs_open("config", NVS_READWRITE, &handle);
	nvs_set_blob(handle, "cfg", src, dataLen);
	nvs_commit(handle);
	nvs_close(handle);

	return dataLen;
}

#endif // PLATFORM_ESPIDF
