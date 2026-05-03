/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_panel_lcd_conf_internal.h"
#include "esp_panel_lcd.hpp"

namespace esp_panel::drivers {

/**
 * @brief LCD driver class for GC9503 display controller
 *
 */
class LCD_GC9503: public LCD {
public:
    /**
     * @brief Default basic attributes for GC9503 LCD controller
     */
    static constexpr BasicAttributes BASIC_ATTRIBUTES_DEFAULT = {
        .name = "GC9503",
    };

    /**
     * @brief Construct the LCD device with individual parameters
     *
     * @param[in] bus Bus interface for communicating with the LCD device
     * @param[in] width Width of the panel (horizontal, in pixels)
     * @param[in] height Height of the panel (vertical, in pixels)
     * @param[in] color_bits Color depth in bits per pixel (16 for RGB565, 18 for RGB666, 24 for RGB888)
     * @param[in] rst_io Reset GPIO pin number (-1 if not used)
     * @note This constructor uses default values for most configuration parameters. Use config*() functions to
     *       customize
     * @note Vendor initialization commands may vary between manufacturers. Consult LCD supplier for them and use
     *       `configVendorCommands()` to configure
     */
    LCD_GC9503(Bus *bus, int width, int height, int color_bits, int rst_io):
        LCD(BASIC_ATTRIBUTES_DEFAULT, bus, width, height, color_bits, rst_io)
    {
    }

    /**
     * @brief Construct the LCD device with full configuration
     *
     * @param[in] bus Bus interface for communicating with the LCD device
     * @param[in] config Complete LCD configuration structure
     * @note Vendor initialization commands may vary between manufacturers. Consult LCD supplier for them
     */
    LCD_GC9503(Bus *bus, const Config &config):
        LCD(BASIC_ATTRIBUTES_DEFAULT, bus, config)
    {
    }

    /**
     * @brief Construct the LCD device with bus configuration and device configuration
     *
     * @param[in] bus_config Bus configuration
     * @param[in] lcd_config LCD configuration
     * @note This constructor creates a new bus instance using the provided bus configuration
     * @note Vendor initialization commands may vary between manufacturers. Consult LCD supplier for them
     */
    LCD_GC9503(const BusFactory::Config &bus_config, const Config &lcd_config):
        LCD(BASIC_ATTRIBUTES_DEFAULT, bus_config, lcd_config)
    {
    }

    /**
     * @brief Destroy the LCD device and free resources
     */
    ~LCD_GC9503() override;

    /**
     * @brief Initialize the LCD device
     *
     * @return `true` if initialization successful, `false` otherwise
     * @note This function must be called after bus interface is initialized
     * @note Creates panel handle by calling `esp_lcd_new_panel_*()` internally
     */
    bool init() override;

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use other constructor instead
     */
    [[deprecated("Use other constructor instead")]]
    LCD_GC9503(Bus *bus, uint8_t color_bits = 0, int rst_io = -1): LCD_GC9503(bus, 0, 0, color_bits, rst_io)
    {
    }

private:
    static const BasicBusSpecificationMap _bus_specifications;
};

} // namespace esp_panel::drivers

/**
 * @brief Alias for backward compatibility
 * @deprecated Use `esp_panel::drivers::LCD_GC9503` instead
 */
using ESP_PanelLcd_GC9503 [[deprecated("Use `esp_panel::drivers::LCD_GC9503` instead")]] =
    esp_panel::drivers::LCD_GC9503;
