#ifdef PLATFORM_ESPIDF

#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

static int bFlashReady = 0;

void InitFlashIfNeeded()
{
	if(bFlashReady)
		return;

	esp_err_t ret = nvs_flash_init();
	if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		nvs_flash_erase();
		nvs_flash_init();
	}

	bFlashReady = 1;

}

void HAL_RebootModule()
{
	esp_restart();
}

#endif // PLATFORM_ESPIDF
