/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_panel_backlight_conf_internal.h"
#if ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_SWITCH_GPIO

#include "driver/gpio.h"
#include "utils/esp_panel_utils_log.h"
#include "esp_panel_backlight_switch_gpio.hpp"

namespace esp_panel::drivers {

void BacklightSwitchGPIO::Config::print() const
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_LOGI(
        "\n\t{Config}"
        "\n\t\t-> [io_num]: %d"
        "\n\t\t-> [on_level]: %d"
        , static_cast<int>(io_num)
        , static_cast<int>(on_level)
    );

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

BacklightSwitchGPIO::~BacklightSwitchGPIO()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_EXIT(del(), "Delete failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

bool BacklightSwitchGPIO::begin()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::BEGIN), false, "Already begun");

#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
    _config.print();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG

    gpio_config_t io_config = {
        .pin_bit_mask = (1ULL << _config.io_num),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_UTILS_CHECK_ERROR_RETURN(gpio_config(&io_config), false, "GPIO config failed");

    setState(State::BEGIN);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BacklightSwitchGPIO::del()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (isOverState(State::BEGIN)) {
        ESP_UTILS_CHECK_ERROR_RETURN(gpio_reset_pin((gpio_num_t)_config.io_num), false, "GPIO reset pin failed");
        setState(State::DEINIT);
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BacklightSwitchGPIO::setBrightness(int percent)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: percent(%d)", percent);

    bool level = (percent > 0) ? _config.on_level : !_config.on_level;
    ESP_UTILS_CHECK_ERROR_RETURN(gpio_set_level((gpio_num_t)_config.io_num, level), false, "GPIO set level failed");

    setBrightnessValue(percent);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

} // namespace esp_panel::drivers

#endif // ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_SWITCH_GPIO
