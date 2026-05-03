/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <variant>
#include "driver/ledc.h"
#include "esp_idf_version.h"
#include "esp_panel_backlight_conf_internal.h"
#include "esp_panel_backlight.hpp"

namespace esp_panel::drivers {

/**
 * @brief The PWM(LEDC) backlight device class
 *
 * This class is a derived class of `Backlight`, users can use it to construct a custom backlight device
 */
class BacklightPWM_LEDC: public Backlight {
public:
    /**
     * @brief Default values for PWM(LEDC) backlight device
     */
    static constexpr BasicAttributes BASIC_ATTRIBUTES_DEFAULT = {
        .type = ESP_PANEL_BACKLIGHT_TYPE_PWM_LEDC,
        .name = "PWM(LEDC)",
    };
    static constexpr int LEDC_TIMER_FREQ_DEFAULT = 5000;
    static constexpr int LEDC_TIMER_BIT_DEFAULT = 10;
    static constexpr ledc_timer_t LEDC_TIMER_NUM_DEFAULT = LEDC_TIMER_0;
    static constexpr ledc_mode_t LEDC_SPEED_MODE_DEFAULT = LEDC_LOW_SPEED_MODE;

    /**
     * @brief Partial LEDC timer configuration structure
     */
    struct LEDC_TimerPartialConfig {
        int freq_hz = LEDC_TIMER_FREQ_DEFAULT;
        int duty_resolution = LEDC_TIMER_BIT_DEFAULT;
    };
    using LEDC_TimerFullConfig = ledc_timer_config_t;
    using LEDC_TimerConfig = std::variant<LEDC_TimerPartialConfig, LEDC_TimerFullConfig>;

    /**
     * @brief Partial LEDC channel configuration structure
     */
    struct LEDC_ChannelPartialConfig {
        int io_num = -1;    ///< GPIO number, default is `-1`
        int on_level = 1;   ///< Level when light up, default is `1`
    };
    using LEDC_ChannelFullConfig = ledc_channel_config_t;
    using LEDC_ChannelConfig = std::variant<LEDC_ChannelPartialConfig, LEDC_ChannelFullConfig>;

    /**
     * @brief The configuration for PWM(LEDC) backlight device
     */
    struct Config {
        /**
         * @brief Convert partial configurations to full configurations
         */
        void convertPartialToFull();

        /**
         * @brief Print LEDC timer configuration for debugging
         */
        void printLEDC_TimerConfig() const;

        /**
         * @brief Print LEDC channel configuration for debugging
         */
        void printLEDC_ChannelConfig() const;

        LEDC_TimerConfig ledc_timer = LEDC_TimerPartialConfig{};        /*!< LEDC timer configuration */
        LEDC_ChannelConfig ledc_channel = LEDC_ChannelPartialConfig{};  /*!< LEDC channel configuration */
    };

// *INDENT-OFF*
    /**
     * @brief Construct the PWM(LEDC) backlight device with separate parameters
     *
     * @param[in] io_num   GPIO number
     * @param[in] on_level Level when light up
     */
    BacklightPWM_LEDC(int io_num, bool on_level):
        Backlight(BASIC_ATTRIBUTES_DEFAULT),
        _config{
            .ledc_channel = LEDC_ChannelPartialConfig{
                .io_num = io_num,
                .on_level = on_level,
            }
        }
    {
    }

    /**
     * @brief Construct the PWM(LEDC) backlight device with configuration
     *
     * @param[in] config The PWM(LEDC) backlight configuration
     */
    BacklightPWM_LEDC(const Config &config):
        Backlight(BASIC_ATTRIBUTES_DEFAULT),
        _config(config)
    {
    }
// *INDENT-ON*

    /**
     * @brief Destroy the device
     */
    ~BacklightPWM_LEDC() override;

    /**
     * @brief Set the LEDC frequency
     *
     * @param[in] hz The frequency in Hz
     *
     * @return `true` if successful, `false` otherwise
     */
    bool configLEDC_FreqHz(int hz);

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

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use other constructors instead
     */
    [[deprecated("Use other constructors instead")]]
    BacklightPWM_LEDC(int io_num, bool light_up_level, bool use_pwm): BacklightPWM_LEDC(io_num, light_up_level) {}

private:
    /**
     * @brief Get mutable reference to LEDC timer configuration
     *
     * @return Reference to LEDC timer configuration
     */
    LEDC_TimerFullConfig &getLEDC_TimerConfig();

    /**
     * @brief Get mutable reference to LEDC channel configuration
     *
     * @return Reference to LEDC channel configuration
     */
    LEDC_ChannelFullConfig &getLEDC_ChannelConfig();

    Config _config = {};     ///< PWM(LEDC) backlight configuration
};

} // namespace esp_panel::drivers

/**
 * @brief Alias for backward compatibility
 * @deprecated Use `esp_panel::drivers::BacklightPWM_LEDC` instead
 */
using ESP_PanelBacklightPWM_LEDC [[deprecated("Use `esp_panel::drivers::BacklightPWM_LEDC` instead")]] =
    esp_panel::drivers::BacklightPWM_LEDC;
