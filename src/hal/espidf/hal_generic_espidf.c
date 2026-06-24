#if PLATFORM_ESPIDF || PLATFORM_ESP8266

#include <unistd.h>
#include "../hal_generic.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#if PLATFORM_ESPIDF
#include "hal/wdt_hal.h"
#include "soc/rtc.h"
#endif

#define OBK_ESPIDF_RTC_WDT_TIMEOUT_MS 10000

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

void HAL_Configure_WDT()
{
#if PLATFORM_ESPIDF
	wdt_hal_context_t rtc_wdt_ctx = RWDT_HAL_CONTEXT_DEFAULT();
	uint32_t stage_timeout_ticks = (uint32_t)((uint64_t)OBK_ESPIDF_RTC_WDT_TIMEOUT_MS * rtc_clk_slow_freq_get_hz() / 1000ULL);

	wdt_hal_init(&rtc_wdt_ctx, WDT_RWDT, 0, false);
	wdt_hal_write_protect_disable(&rtc_wdt_ctx);
	wdt_hal_config_stage(&rtc_wdt_ctx, WDT_STAGE0, stage_timeout_ticks, WDT_STAGE_ACTION_RESET_SYSTEM);
	wdt_hal_config_stage(&rtc_wdt_ctx, WDT_STAGE1, stage_timeout_ticks, WDT_STAGE_ACTION_RESET_RTC);
	wdt_hal_enable(&rtc_wdt_ctx);
	wdt_hal_write_protect_enable(&rtc_wdt_ctx);
#endif
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
