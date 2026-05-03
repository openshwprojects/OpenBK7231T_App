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
 * @brief The switch(GPIO) backlight device class
 *
 * @note  The class is a derived class of `Backlight`, users can use it to construct a custom backlight device
 */
class BacklightSwitchGPIO: public Backlight {
public:
    /**
     * Here are some default values for switch(GPIO) backlight device
     */
    static constexpr BasicAttributes BASIC_ATTRIBUTES_DEFAULT = {
        .type = ESP_PANEL_BACKLIGHT_TYPE_SWITCH_GPIO,
        .name = "switch(GPIO)",
    };

    /**
     * @brief The switch(GPIO) backlight device configuration structure
     */
    struct Config {
        void print() const;

        int io_num = -1;    /*!< GPIO number. Default is `-1` */
        int on_level = 1;   /*!< Level when light up. Default is `1` */
    };

// *INDENT-OFF*
    /**
     * @brief Construct the switch(GPIO) backlight device with separate parameters
     *
     * @param[in] io_num   GPIO number
     * @param[in] on_level Level when light up
     */
    BacklightSwitchGPIO(int io_num, bool on_level):
        Backlight(BASIC_ATTRIBUTES_DEFAULT),
        _config{
            .io_num = io_num,
            .on_level = on_level,
        }
    {
    }

    /**
     * @brief Construct the switch(GPIO) backlight device with configuration
     *
     * @param[in] config The switch(GPIO) backlight configuration
     */
    BacklightSwitchGPIO(const Config &config):
        Backlight(BASIC_ATTRIBUTES_DEFAULT),
        _config(config)
    {
    }
// *INDENT-ON*

    /**
     * @brief Destroy the device
     */
    ~BacklightSwitchGPIO() override;

    /**
     * @brief Startup the device
     *
     * @return `true` if success, otherwise false
     */
    bool begin() override;

    /**
     * @brief Delete the device, release the resources
     *
     * @note  After calling this function, users should call `begin()` to re-init the device
     *
     * @return `true` if success, otherwise false
     */
    bool del() override;

    /**
     * @brief Set the brightness by percent
     *
     * @note  This function should be called after `begin()`
     *
     * @param[in] percent The brightness percent (0-100)
     *
     * @return `true` if success, otherwise false
     */
    bool setBrightness(int percent) override;

private:
    Config _config = {};
};

} // namespace esp_panel::drivers
