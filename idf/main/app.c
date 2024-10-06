#include <arch/sys_arch.h>
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "hal/wdt_hal.h"

void app_main(void);

void Main_Init();
void Main_OnEverySecond();
float g_wifi_temperature = 0;

#ifndef CONFIG_IDF_TARGET_ESP32

#include "driver/temperature_sensor.h"

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S2)
#define TEMP_STACK_SIZE 1024
#else
#define TEMP_STACK_SIZE 384
#endif

temperature_sensor_handle_t temp_handle = NULL;

void temp_func(void* pvParameters)
{
    for(;;)
    {
        temperature_sensor_enable(temp_handle);
        temperature_sensor_get_celsius(temp_handle, &g_wifi_temperature);
        temperature_sensor_disable(temp_handle);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

#endif

void app_main(void)
{
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_netif_init();
    esp_event_loop_create_default();
#ifndef CONFIG_IDF_TARGET_ESP32
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
    temperature_sensor_install(&temp_sensor_config, &temp_handle);
    xTaskCreate(temp_func, "IntTemp", TEMP_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
#endif

    Main_Init();

    while(1)
    {
        wdt_hal_context_t rtc_wdt_ctx = RWDT_HAL_CONTEXT_DEFAULT();
        wdt_hal_write_protect_disable(&rtc_wdt_ctx);
        wdt_hal_feed(&rtc_wdt_ctx);
        wdt_hal_write_protect_enable(&rtc_wdt_ctx);
        Main_OnEverySecond();
        sys_delay_ms(1000);
    }
}
