/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "port/esp_lcd_touch_st1633.h"
#include "esp_panel_touch_conf_internal.h"
#include "esp_panel_touch.hpp"

namespace esp_panel::drivers {

/**
 * @brief ST1633 touch controller
 *
 * This class provides implementation for ST1633 touch controller, inheriting from
 * the base Touch class to provide common touch functionality
 */
class TouchST1633 : public Touch {
public:
    /**
     * @brief Default basic attributes for ST1633
     */
    static constexpr BasicAttributes BASIC_ATTRIBUTES_DEFAULT = {
        .name = "ST1633",
        .max_points_num = 10,
    };

    /**
     * @brief Construct a touch device instance with individual configuration parameters
     *
     * @param bus Bus interface for communicating with the touch device
     * @param width Panel width in pixels
     * @param height Panel height in pixels
     * @param rst_io Reset GPIO pin number (-1 if unused)
     * @param int_io Interrupt GPIO pin number (-1 if unused)
     */
    TouchST1633(Bus *bus, uint16_t width, uint16_t height, int rst_io = -1, int int_io = -1):
        Touch(BASIC_ATTRIBUTES_DEFAULT, bus, width, height, rst_io, int_io)
    {
    }

    /**
     * @brief Construct a touch device instance with configuration
     *
     * @param[in] bus Pointer to the bus interface for communicating with the touch device
     * @param[in] config Configuration structure containing device settings and parameters
     */
    TouchST1633(Bus *bus, const Config &config): Touch(BASIC_ATTRIBUTES_DEFAULT, bus, config) {}

    /**
     * @brief Construct a touch device instance with bus configuration and device configuration
     *
     * @param[in] bus_config Bus configuration
     * @param[in] touch_config Touch configuration
     * @note This constructor creates a new bus instance using the provided bus configuration
     */
    TouchST1633(const BusFactory::Config &bus_config, const Config &touch_config):
        Touch(BASIC_ATTRIBUTES_DEFAULT, bus_config, touch_config)
    {
    }

    /**
     * @brief Destruct touch device
     */
    ~TouchST1633() override;

    /**
     * @brief Startup the touch device
     *
     * @return `true` if success, otherwise false
     *
     * @note This function should be called after `init()`
     */
    bool begin() override;
};

} // namespace esp_panel::drivers

/**
 * @brief Deprecated type alias for backward compatibility
 * @deprecated Use `esp_panel::drivers::TouchST1633` instead
 */
using ESP_PanelTouch_ST1633 [[deprecated("Use `esp_panel::drivers::TouchST1633` instead")]] =
    esp_panel::drivers::TouchST1633;
