/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include "esp_panel_lcd.hpp"
#include "esp_panel_lcd_axs15231b.hpp"
#include "esp_panel_lcd_ek9716b.hpp"
#include "esp_panel_lcd_ek79007.hpp"
#include "esp_panel_lcd_gc9a01.hpp"
#include "esp_panel_lcd_gc9b71.hpp"
#include "esp_panel_lcd_gc9503.hpp"
#include "esp_panel_lcd_hx8399.hpp"
#include "esp_panel_lcd_ili9881c.hpp"
#include "esp_panel_lcd_ili9341.hpp"
#include "esp_panel_lcd_jd9165.hpp"
#include "esp_panel_lcd_jd9365.hpp"
#include "esp_panel_lcd_nv3022b.hpp"
#include "esp_panel_lcd_sh8601.hpp"
#include "esp_panel_lcd_spd2010.hpp"
#include "esp_panel_lcd_st7262.hpp"
#include "esp_panel_lcd_st7701.hpp"
#include "esp_panel_lcd_st7703.hpp"
#include "esp_panel_lcd_st7789.hpp"
#include "esp_panel_lcd_st7796.hpp"
#include "esp_panel_lcd_st77903.hpp"
#include "esp_panel_lcd_st77916.hpp"
#include "esp_panel_lcd_st77922.hpp"

namespace esp_panel::drivers {

/**
 * @brief Factory class for creating LCD device instances
 */
class LCD_Factory {
public:
    /**
     * @brief Function pointer type for LCD device constructors
     *
     * @param[in] bus Pointer to the bus interface
     * @param[in] config LCD device configuration
     *
     * @return Shared pointer to the created LCD device
     */
    using FunctionDeviceConstructor = std::shared_ptr<LCD> (*)(
                                          const BusFactory::Config &bus_config, const LCD::Config &lcd_config
                                      );

    /**
     * @brief Create a new LCD device instance
     *
     * @param[in] name Name of the LCD device to create
     * @param[in] bus_config Bus configuration
     * @param[in] lcd_config LCD configuration
     *
     * @return Shared pointer to the created LCD device if successful, nullptr otherwise
     *
     * @note This is a static factory method that creates LCD device instances based on the provided name
     */
    static std::shared_ptr<LCD> create(
        utils::string name, const BusFactory::Config &bus_config, const LCD::Config &lcd_config
    );

private:
    /**
     * @brief Map of LCD device names to their constructor functions
     */
    static const utils::unordered_map<utils::string, FunctionDeviceConstructor> _name_function_map;
};

} // namespace esp_panel::drivers
