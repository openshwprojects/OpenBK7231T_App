#if PLATFORM_ESPIDF || PLATFORM_ESP8266

#include <unistd.h>
#include "../hal_generic.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#if PLATFORM_ESPIDF
#include "hal/wdt_hal.h"
#endif

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

void HAL_Delay_us(int delay)
{
	usleep(delay);
}

void HAL_Run_WDT()
{
#if PLATFORM_ESPIDF
	wdt_hal_context_t rtc_wdt_ctx = RWDT_HAL_CONTEXT_DEFAULT();
	wdt_hal_write_protect_disable(&rtc_wdt_ctx);
	wdt_hal_feed(&rtc_wdt_ctx);
	wdt_hal_write_protect_enable(&rtc_wdt_ctx);
#endif
}

#endif // PLATFORM_ESPIDF
