/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <variant>
#include "utils/esp_panel_utils_cxx.hpp"
#include "esp_panel_backlight.hpp"
#include "esp_panel_backlight_custom.hpp"
#include "esp_panel_backlight_pwm_ledc.hpp"
#include "esp_panel_backlight_switch_gpio.hpp"
#include "esp_panel_backlight_switch_expander.hpp"

namespace esp_panel::drivers {

/**
 * @brief The backlight factory class for creating and managing backlight devices
 *
 * This class provides static methods to create backlight devices and manage their configurations
 */
class BacklightFactory {
public:
    /**
     * @brief The backlight configuration variant type
     *
     * Contains configurations for different types of backlight devices (Custom, PWM_LEDC, SwitchGPIO)
     */
    using Config = std::variant <
                   drivers::BacklightCustom::Config,
                   drivers::BacklightPWM_LEDC::Config,
                   drivers::BacklightSwitchGPIO::Config,
                   drivers::BacklightSwitchExpander::Config
                   >;

    /**
     * @brief Function pointer type for backlight device constructors
     *
     * Points to functions that create and return a shared pointer to a backlight device
     */
    using FunctionDeviceConstructor = std::shared_ptr<Backlight> (*)(const Config &config);

    /**
     * @brief Create a new backlight device with configuration
     *
     * @param[in] config The backlight configuration
     *
     * @return Shared pointer to the device if successful, `nullptr` otherwise
     */
    static std::shared_ptr<Backlight> create(const Config &config);

    /**
     * @brief Get the backlight type from configuration
     *
     * @param[in] config The backlight configuration
     *
     * @return Backlight type (`ESP_PANEL_BACKLIGHT_TYPE_*`) if successful, `-1` otherwise
     */
    static int getConfigType(const Config &config);

    /**
     * @brief Get the string representation of a backlight type
     *
     * @param[in] type The backlight type (`ESP_PANEL_BACKLIGHT_TYPE_*`)
     *
     * @return Backlight type name string if successful, `"Unknown"` otherwise
     */
    static utils::string getTypeNameString(int type);

private:
    /**
     * @brief Map of bus types to their type names
     *
     * Maps each bus type to its type name string
     */
    static const utils::unordered_map<int, utils::string> _type_name_map;

    /**
     * @brief Map of bus types to their constructors and type names
     *
     * Maps each bus type to its constructor function
     */
    static const utils::unordered_map<int, FunctionDeviceConstructor> _type_constructor_map;
};

} // namespace esp_panel::drivers
