/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_panel_lcd_conf_internal.h"
#if ESP_PANEL_DRIVERS_LCD_ENABLE_EK9716B

#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_rgb.h"
#endif // ESP_PANEL_DRIVERS_BUS_ENABLE_RGB
#include "esp_lcd_panel_vendor.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "utils/esp_panel_utils_log.h"
#include "esp_panel_lcd_ek9716b.hpp"

namespace esp_panel::drivers {

// *INDENT-OFF*
const LCD::BasicBusSpecificationMap LCD_EK9716B::_bus_specifications = {
    {
        ESP_PANEL_BUS_TYPE_RGB, BasicBusSpecification{
            .functions = (1U << BasicBusSpecification::FUNC_INVERT_COLOR) |
                         (1U << BasicBusSpecification::FUNC_MIRROR_X) |
                         (1U << BasicBusSpecification::FUNC_MIRROR_Y) |
                         (1U << BasicBusSpecification::FUNC_SWAP_XY) |
                         (1U << BasicBusSpecification::FUNC_GAP) |
                         (1U << BasicBusSpecification::FUNC_DISPLAY_ON_OFF),
        },
    },
};
// *INDENT-ON*

LCD_EK9716B::~LCD_EK9716B()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_EXIT(del(), "Delete failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

bool LCD_EK9716B::init()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::INIT), false, "Already initialized");

    // Process the device on initialization
    ESP_UTILS_CHECK_FALSE_RETURN(processDeviceOnInit(_bus_specifications), false, "Process device on init failed");

#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB
    auto device_config = getConfig().getDeviceFullConfig();
    ESP_UTILS_CHECK_NULL_RETURN(device_config, false, "Invalid device config");

    /* Initialize RST pin */
    if (device_config->reset_gpio_num >= 0) {
        gpio_config_t gpio_conf = {
            .pin_bit_mask = BIT64(device_config->reset_gpio_num),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_UTILS_CHECK_ERROR_RETURN(gpio_config(&gpio_conf), false, "`Config Reset GPIO failed");
    }

    // Create refresh panel
    auto vendor_config = getConfig().getVendorFullConfig();
    ESP_UTILS_CHECK_NULL_RETURN(vendor_config, false, "Invalid vendor config");

    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_lcd_new_rgb_panel(vendor_config->rgb_config, &refresh_panel), false, "Create refresh panel failed"
    );
    ESP_UTILS_LOGD("Create refresh panel(@%p)", refresh_panel);
#else
    ESP_UTILS_CHECK_FALSE_RETURN(false, false, "RGB is not supported");
#endif // ESP_PANEL_DRIVERS_BUS_ENABLE_RGB

    setState(State::INIT);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool LCD_EK9716B::reset()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::INIT), false, "Not initialized");

    ESP_UTILS_CHECK_FALSE_RETURN(LCD::reset(), false, "Reset base LCD failed");

    auto device_config = getConfig().getDeviceFullConfig();
    if (device_config->reset_gpio_num >= 0) {
        gpio_set_level((gpio_num_t)device_config->reset_gpio_num, device_config->flags.reset_active_high);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level((gpio_num_t)device_config->reset_gpio_num, !device_config->flags.reset_active_high);
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

} // namespace esp_panel::drivers

#endif // ESP_PANEL_DRIVERS_LCD_ENABLE_EK9716B
