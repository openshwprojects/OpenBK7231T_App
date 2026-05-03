/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <algorithm>
#include <string>
#include "esp_panel_backlight_conf_internal.h"

namespace esp_panel::drivers {

/**
 * @brief The backlight base class
 *
 * This class is a base class for all backlight devices. Due to it being a virtual class, users cannot construct it
 * directly
 */
class Backlight {
public:
    /**
     * @brief The backlight basic attributes structure
     */
    struct BasicAttributes {
        int type = -1;          ///< The device type, default is `-1`
        const char *name = "";  ///< The device name, default is `""`
    };

    /**
     * @brief The driver state enumeration
     */
    enum class State : int {
        DEINIT = 0,    ///< Driver is not initialized
        BEGIN,         ///< Driver is initialized and ready
    };

    /**
     * @brief Construct a new backlight device
     *
     * @param[in] attr The backlight basic attributes
     */
    Backlight(const BasicAttributes &attr): _basic_attributes(attr) {}

    /**
     * @brief Destroy the backlight device
     */
    virtual ~Backlight() = default;

    /**
     * @brief Initialize and start the backlight device
     *
     * @return `true` if successful, `false` otherwise
     */
    virtual bool begin() = 0;

    /**
     * @brief Delete the backlight device and release resources
     *
     * @return `true` if successful, `false` otherwise
     *
     * @note After calling this function, users should call `begin()` to re-init the device
     */
    virtual bool del() = 0;

    /**
     * @brief Set the brightness by percent
     *
     * @param[in] percent The brightness percent (0-100)
     *
     * @return `true` if successful, `false` otherwise
     *
     * @note This function should be called after `begin()`
     */
    virtual bool setBrightness(int percent) = 0;

    /**
     * @brief Turn on the backlight
     *
     * @return `true` if successful, `false` otherwise
     *
     * @note This function should be called after `begin()`
     * @note This function is equivalent to `setBrightness(100)`
     */
    bool on();

    /**
     * @brief Turn off the backlight
     *
     * @return `true` if successful, `false` otherwise
     *
     * @note This function should be called after `begin()`
     * @note This function is equivalent to `setBrightness(0)`
     */
    bool off();

    /**
     * @brief Check if the driver has reached or passed the specified state
     *
     * @param[in] state The state to check against current state
     *
     * @return `true` if current state >= given state, `false` otherwise
     */
    bool isOverState(State state)
    {
        return (_state >= state);
    }

    /**
     * @brief Get the device basic attributes
     *
     * @return Reference to the backlight basic attributes
     */
    const BasicAttributes &getBasicAttributes()
    {
        return _basic_attributes;
    }

    /**
     * @brief Get the current brightness percent
     *
     * @return The current brightness percent (0-100)
     */
    int getBrightness() const
    {
        return _brightness;
    }

protected:
    /**
     * @brief Set the current driver state
     *
     * @param[in] state The state to set
     */
    void setState(State state)
    {
        _state = state;
    }

    /**
     * @brief Set the current brightness percent
     *
     * @param[in] percent The brightness percent (0-100)
     */
    void setBrightnessValue(int percent)
    {
        _brightness = std::clamp(percent, 0, 100);
    }

private:
    State _state = State::DEINIT;               ///< Current driver state
    BasicAttributes _basic_attributes = {};     ///< Device basic attributes
    int _brightness = 0;                        ///< Current brightness percent (0-100)
};

} // namespace esp_panel::drivers

/**
 * @brief Alias for backward compatibility
 * @deprecated Use `esp_panel::drivers::Backlight` instead
 */
using ESP_PanelBacklight [[deprecated("Use `esp_panel::drivers::Backlight` instead")]] = esp_panel::drivers::Backlight;
