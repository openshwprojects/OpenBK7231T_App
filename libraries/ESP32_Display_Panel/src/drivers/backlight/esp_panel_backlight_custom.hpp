/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_panel_backlight_conf_internal.h"
#include "esp_panel_backlight.hpp"

namespace esp_panel::drivers {

/**
 * @brief The custom backlight device class
 *
 * This class is a derived class of `Backlight`. Users can implement custom backlight control through callback functions
 */
class BacklightCustom: public Backlight {
public:
    /**
     * @brief Default values for custom backlight device
     */
    static constexpr BasicAttributes BASIC_ATTRIBUTES_DEFAULT = {
        .type = ESP_PANEL_BACKLIGHT_TYPE_CUSTOM,
        .name = "Custom",
    };

    /**
     * @brief Function pointer type for brightness control callback
     *
     * @param[in] percent   The brightness percent (0-100)
     * @param[in] user_data User data passed to the callback function
     *
     * @return `true` if successful, `false` otherwise
     */
    using FunctionSetBrightnessCallback = bool (*)(int percent, void *user_data);

    /**
     * @brief The custom backlight device configuration structure
     */
    struct Config {
        FunctionSetBrightnessCallback callback = nullptr;  ///< Callback function to set brightness, default is `nullptr`
        void *user_data = nullptr;                        ///< User data passed to callback function, default is `nullptr`
    };

    /**
     * @brief Construct the custom backlight device with separate parameters
     *
     * @param[in] callback   The callback function to set brightness
     * @param[in] user_data  The user data passed to the callback function
     */
    BacklightCustom(FunctionSetBrightnessCallback callback, void *user_data):
        Backlight(BASIC_ATTRIBUTES_DEFAULT),
        _config(Config{callback, user_data})
    {
    }
// *INDENT-ON*

    /**
     * @brief Construct the custom backlight device with configuration
     *
     * @param[in] config The custom backlight configuration
     */
    BacklightCustom(const Config &config): Backlight(BASIC_ATTRIBUTES_DEFAULT), _config(config) {}

    /**
     * @brief Destroy the device
     */
    ~BacklightCustom() override;

    /**
     * @brief Configure the callback function and user data
     *
     * @param[in] callback  The callback function
     * @param[in] user_data The user data passed to the callback function
     */
    void configCallback(FunctionSetBrightnessCallback callback, void *user_data);

    /**
     * @brief Startup the device
     *
     * @return `true` if successful, `false` otherwise
     */
    bool begin() override;

    /**
     * @brief Delete the device, release the resources
     *
     * @return `true` if successful, `false` otherwise
     *
     * @note After calling this function, users should call `begin()` to re-init the device
     */
    bool del() override;

    /**
     * @brief Set the brightness by percent
     *
     * @param[in] percent The brightness percent (0-100)
     *
     * @return `true` if successful, `false` otherwise
     *
     * @note This function should be called after `begin()`
     */
    bool setBrightness(int percent) override;

private:
    Config _config = {};     ///< Custom backlight configuration
};

} // namespace esp_panel::drivers

/**
 * @brief Alias for backward compatibility
 *
 * @deprecated Use `esp_panel::drivers::BacklightCustom` instead
 */
using ESP_PanelBacklightCustom [[deprecated("Use `esp_panel::drivers::BacklightCustom` instead")]] =
    esp_panel::drivers::BacklightCustom;
