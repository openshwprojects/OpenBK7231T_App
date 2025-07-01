#if PLATFORM_ESPIDF || PLATFORM_ESP8266

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../quicktick.h"
#include <arch/sys_arch.h>
#include "esp_wifi.h"
#include "esp_sleep.h"
#if PLATFORM_ESP8266
#include "esp_netif.h"
#endif
void app_main(void);

void Main_Init();
void Main_OnEverySecond();
float g_wifi_temperature = 0;

#if !CONFIG_IDF_TARGET_ESP32 && !PLATFORM_ESP8266

#include "driver/temperature_sensor.h"

#define TEMP_STACK_SIZE 1024

temperature_sensor_handle_t temp_handle = NULL;

void temp_func(void* pvParameters)
{
    for(;;)
    {
        temperature_sensor_enable(temp_handle);
        temperature_sensor_get_celsius(temp_handle, &g_wifi_temperature);
        temperature_sensor_disable(temp_handle);
        sys_delay_ms(10000);
    }
}

#endif

void app_main(void)
{
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_netif_init();
    esp_event_loop_create_default();
#if !CONFIG_IDF_TARGET_ESP32 && !PLATFORM_ESP8266
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
    temperature_sensor_install(&temp_sensor_config, &temp_handle);
    xTaskCreate(temp_func, "IntTemp", TEMP_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
#endif

    Main_Init();

    while(1)
    {
        sys_delay_ms(1000);
        Main_OnEverySecond();
    }
}

#endif // PLATFORM_ESPIDF
