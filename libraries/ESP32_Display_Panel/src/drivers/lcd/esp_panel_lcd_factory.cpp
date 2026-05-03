/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils/esp_panel_utils_log.h"
#include "esp_utils_helpers.h"
#include "esp_panel_lcd_conf_internal.h"
#include "esp_panel_lcd_factory.hpp"

namespace esp_panel::drivers {

#define DEVICE_CREATOR(controller) \
    [](const BusFactory::Config &bus_config, const LCD::Config &lcd_config) -> std::shared_ptr<LCD> { \
        std::shared_ptr<LCD> device = nullptr; \
        ESP_UTILS_CHECK_EXCEPTION_RETURN( \
            (device = utils::make_shared<LCD_ ## controller>(bus_config, lcd_config)), nullptr, \
            "Create " #controller " failed" \
        ); \
        return device; \
    }
#define MAP_ITEM(controller) \
    {LCD_ ##controller::BASIC_ATTRIBUTES_DEFAULT.name, DEVICE_CREATOR(controller)}

const utils::unordered_map<utils::string, LCD_Factory::FunctionDeviceConstructor> LCD_Factory::_name_function_map = {
#if ESP_PANEL_DRIVERS_LCD_USE_AXS15231B
    MAP_ITEM(AXS15231B),
#endif // CONFIG_ESP_PANEL_LCD_AXS15231B
#if ESP_PANEL_DRIVERS_LCD_USE_EK9716B
    MAP_ITEM(EK9716B),
#endif // CONFIG_ESP_PANEL_LCD_EK9716B
#if ESP_PANEL_DRIVERS_LCD_USE_EK79007
    MAP_ITEM(EK79007),
#endif // CONFIG_ESP_PANEL_LCD_EK79007
#if ESP_PANEL_DRIVERS_LCD_USE_GC9A01
    MAP_ITEM(GC9A01),
#endif // CONFIG_ESP_PANEL_LCD_GC9A01
#if ESP_PANEL_DRIVERS_LCD_USE_GC9B71
    MAP_ITEM(GC9B71),
#endif // CONFIG_ESP_PANEL_LCD_GC9B71
#if ESP_PANEL_DRIVERS_LCD_USE_GC9503
    MAP_ITEM(GC9503),
#endif // CONFIG_ESP_PANEL_LCD_GC9503
#if ESP_PANEL_DRIVERS_LCD_USE_HX8399
    MAP_ITEM(HX8399),
#endif // CONFIG_ESP_PANEL_LCD_HX8399
#if ESP_PANEL_DRIVERS_LCD_USE_ILI9341
    MAP_ITEM(ILI9341),
#endif // CONFIG_ESP_PANEL_LCD_ILI9341
#if ESP_PANEL_DRIVERS_LCD_USE_ILI9881C
    MAP_ITEM(ILI9881C),
#endif // CONFIG_ESP_PANEL_LCD_ILI9881C
#if ESP_PANEL_DRIVERS_LCD_USE_JD9165
    MAP_ITEM(JD9165),
#endif // CONFIG_ESP_PANEL_LCD_JD9165
#if ESP_PANEL_DRIVERS_LCD_USE_JD9365
    MAP_ITEM(JD9365),
#endif // CONFIG_ESP_PANEL_LCD_JD9365
#if ESP_PANEL_DRIVERS_LCD_USE_NV3022B
    MAP_ITEM(NV3022B),
#endif // CONFIG_ESP_PANEL_LCD_NV3022B
#if ESP_PANEL_DRIVERS_LCD_USE_SH8601
    MAP_ITEM(SH8601),
#endif // CONFIG_ESP_PANEL_LCD_SH8601
#if ESP_PANEL_DRIVERS_LCD_USE_SPD2010
    MAP_ITEM(SPD2010),
#endif // CONFIG_ESP_PANEL_LCD_SPD2010
#if ESP_PANEL_DRIVERS_LCD_USE_ST7262
    MAP_ITEM(ST7262),
#endif // CONFIG_ESP_PANEL_LCD_ST7262
#if ESP_PANEL_DRIVERS_LCD_USE_ST7701
    MAP_ITEM(ST7701),
#endif // CONFIG_ESP_PANEL_LCD_ST7701
#if ESP_PANEL_DRIVERS_LCD_USE_ST7703
    MAP_ITEM(ST7703),
#endif // CONFIG_ESP_PANEL_LCD_ST7703
#if ESP_PANEL_DRIVERS_LCD_USE_ST7789
    MAP_ITEM(ST7789),
#endif // CONFIG_ESP_PANEL_LCD_ST7789
#if ESP_PANEL_DRIVERS_LCD_USE_ST7796
    MAP_ITEM(ST7796),
#endif // CONFIG_ESP_PANEL_LCD_ST7796
#if ESP_PANEL_DRIVERS_LCD_USE_ST77903
    MAP_ITEM(ST77903),
#endif // CONFIG_ESP_PANEL_LCD_ST77903
#if ESP_PANEL_DRIVERS_LCD_USE_ST77916
    MAP_ITEM(ST77916),
#endif // CONFIG_ESP_PANEL_LCD_ST77916
#if ESP_PANEL_DRIVERS_LCD_USE_ST77922
    MAP_ITEM(ST77922),
#endif // CONFIG_ESP_PANEL_LCD_ST77922
};

std::shared_ptr<LCD> LCD_Factory::create(
    utils::string name, const BusFactory::Config &bus_config, const LCD::Config &lcd_config
)
{
    ESP_UTILS_LOGD("Param: name(%s), bus_config(@%p), lcd_config(@%p)", name.c_str(), &bus_config, &lcd_config);

    auto it = _name_function_map.find(name);
    ESP_UTILS_CHECK_FALSE_RETURN(it != _name_function_map.end(), nullptr, "Unknown controller: %s", name.c_str());

    std::shared_ptr<LCD> device = it->second(bus_config, lcd_config);
    ESP_UTILS_CHECK_NULL_RETURN(device, nullptr, "Create device failed");

    return device;
}

} // namespace esp_panel::drivers
