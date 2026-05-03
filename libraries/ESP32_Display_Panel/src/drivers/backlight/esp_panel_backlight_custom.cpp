/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_panel_backlight_conf_internal.h"
#if ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_CUSTOM

#include "utils/esp_panel_utils_log.h"
#include "esp_panel_backlight_custom.hpp"

namespace esp_panel::drivers {

BacklightCustom::~BacklightCustom()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_EXIT(del(), "Delete failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

void BacklightCustom::configCallback(FunctionSetBrightnessCallback callback, void *user_data)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_LOGD("Param: callback(%p), user_data(%p)", callback, user_data);
    _config.callback = callback;
    _config.user_data = user_data;

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

bool BacklightCustom::begin()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!isOverState(State::BEGIN), false, "Already begun");

    ESP_UTILS_CHECK_NULL_RETURN(_config.callback, false, "Invalid callback function");

    setState(State::BEGIN);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BacklightCustom::del()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    setState(State::DEINIT);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

bool BacklightCustom::setBrightness(int percent)
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(isOverState(State::BEGIN), false, "Not begun");

    ESP_UTILS_LOGD("Param: percent(%d)", percent);

    percent = std::clamp(percent, 0, 100);
    if (_config.callback != nullptr) {
        ESP_UTILS_CHECK_FALSE_RETURN(_config.callback(percent, _config.user_data), false, "Run callback failed");
    }

    setBrightnessValue(percent);

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

} // namespace esp_panel::drivers

#endif // ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_CUSTOM
