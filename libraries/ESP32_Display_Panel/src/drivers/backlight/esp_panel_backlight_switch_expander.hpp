/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_io_expander.hpp"
#include "esp_panel_backlight_conf_internal.h"
#include "esp_panel_backlight.hpp"

namespace esp_panel::drivers {

/**
 * @brief The Switch(Expander) backlight device class
 *
 * @note  The class is a derived class of `Backlight`, users can use it to construct a custom backlight device
 */
class BacklightSwitchExpander: public Backlight {
public:
    /**
     * Here are some default values for Switch(Expander) backlight device
     */
    static constexpr BasicAttributes BASIC_ATTRIBUTES_DEFAULT = {
        .type = ESP_PANEL_BACKLIGHT_TYPE_SWITCH_EXPANDER,
        .name = "Switch(Expander)",
    };

    /**
     * @brief The Switch(Expander) backlight device configuration structure
     */
    struct Config {
        void print() const;

        int io_num = -1;    /*!< GPIO number. Default is `-1` */
        int on_level = 1;   /*!< Level when light up. Default is `1` */
    };

// *INDENT-OFF*
    /**
     * @brief Construct the Switch(Expander) backlight device with separate parameters
     *
     * @param[in] io_num   GPIO number
     * @param[in] on_level Level when light up
     * @param[in] expander The IO expander pointer (optional, default is `nullptr`)
     */
    BacklightSwitchExpander(int io_num, bool on_level, esp_expander::Base *expander = nullptr):
        Backlight(BASIC_ATTRIBUTES_DEFAULT),
        _config{
            .io_num = io_num,
            .on_level = on_level,
        },
        _expander(expander)
    {
    }

    /**
     * @brief Construct the Switch(Expander) backlight device with configuration
     *
     * @param[in] config The Switch(Expander) backlight configuration
     * @param[in] expander The IO expander pointer (optional, default is `nullptr`)
     */
    BacklightSwitchExpander(const Config &config, esp_expander::Base *expander = nullptr):
        Backlight(BASIC_ATTRIBUTES_DEFAULT),
        _config(config),
        _expander(expander)
    {
    }
// *INDENT-ON*

    /**
     * @brief Destroy the device
     */
    ~BacklightSwitchExpander() override;

    /**
     * @brief Configure the IO expander
     *
     * @param[in] expander The IO expander pointer
     */
    void configIO_Expander(esp_expander::Base *expander);

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

    /**
     * @brief Get the IO expander
     *
     * @return The IO expander pointer
     */
    esp_expander::Base *getIO_Expander() const
    {
        return _expander;
    }

private:
    Config _config = {};
    esp_expander::Base *_expander = nullptr;
};

} // namespace esp_panel::drivers
