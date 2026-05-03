/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_panel_backlight_conf_internal.h"
#if ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_SWITCH_EXPANDER

#include "utils/esp_panel_utils_log.h"
#include "esp_panel_backlight_switch_expander.hpp"

namespace esp_panel::drivers {

void BacklightSwitchExpander::Config::print() const
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

BacklightSwitchExpander::~BacklightSwitchExpander()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_EXIT(del(), "Delete failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

void BacklightSwitchExpander::configIO_Expander(esp_expander::Base *expander)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_LOGD("Param: expander(%p)", expander);
    _expander = expander;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

bool BacklightSwitchExpander::begin()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::BEGIN), false, "Already begun");
    ESP_UTILS_CHECK_NULL_RETURN(_expander, false, "Invalid expander");

#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
    _config.print();
#endif // ESP_UTILS_LOG_LEVEL_DEBUG

    ESP_UTILS_CHECK_FALSE_RETURN(_expander->pinMode(_config.io_num, OUTPUT), false, "Expander set pin mode failed");
    ESP_UTILS_CHECK_FALSE_RETURN(
        _expander->digitalWrite(_config.io_num, _config.on_level), false, "Expander set pin level failed"
    );

    setState(State::BEGIN);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BacklightSwitchExpander::del()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (_expander != nullptr) {
        ESP_UTILS_CHECK_FALSE_RETURN(_expander->pinMode(_config.io_num, INPUT), false, "Expander set pin mode failed");
    }

    setState(State::DEINIT);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BacklightSwitchExpander::setBrightness(int percent)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: percent(%d)", percent);

    int level = (percent > 0) ? _config.on_level : !_config.on_level;
    ESP_UTILS_CHECK_FALSE_RETURN(
        _expander->digitalWrite(_config.io_num, level), false, "Expander set pin level failed"
    );

    setBrightnessValue(percent);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

} // namespace esp_panel::drivers

#endif // ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_SWITCH_EXPANDER
