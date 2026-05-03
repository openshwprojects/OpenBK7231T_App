/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils/esp_panel_utils_log.h"
#include "esp_utils_helpers.h"
#include "esp_panel_touch_conf_internal.h"
#include "esp_panel_touch_factory.hpp"

namespace esp_panel::drivers {

#define DEVICE_CREATOR(controller) \
    [](const BusFactory::Config &bus_config, const Touch::Config &touch_config) -> std::shared_ptr<Touch> { \
        std::shared_ptr<Touch> device = nullptr; \
        ESP_UTILS_CHECK_EXCEPTION_RETURN( \
            (device = utils::make_shared<Touch ## controller>(bus_config, touch_config)), nullptr, \
            "Create " #controller " failed" \
        ); \
        return device; \
    }
#define MAP_ITEM(controller) \
    {Touch ##controller::BASIC_ATTRIBUTES_DEFAULT.name, DEVICE_CREATOR(controller)}

const utils::unordered_map<utils::string, TouchFactory::FunctionCreateDevice> TouchFactory::_name_function_map = {
#if ESP_PANEL_DRIVERS_TOUCH_USE_AXS15231B
    MAP_ITEM(AXS15231B),
#endif // CONFIG_ESP_PANEL_TOUCH_AXS15231B
#if ESP_PANEL_DRIVERS_TOUCH_USE_CHSC6540
    MAP_ITEM(CHSC6540),
#endif // CONFIG_ESP_PANEL_TOUCH_CHSC6540
#if ESP_PANEL_DRIVERS_TOUCH_USE_CST816S
    MAP_ITEM(CST816S),
#endif // CONFIG_ESP_PANEL_TOUCH_CST816S
#if ESP_PANEL_DRIVERS_TOUCH_USE_FT5x06
    MAP_ITEM(FT5x06),
#endif // CONFIG_ESP_PANEL_TOUCH_FT5x06
#if ESP_PANEL_DRIVERS_TOUCH_USE_GT911
    MAP_ITEM(GT911),
#endif // CONFIG_ESP_PANEL_TOUCH_GT911
#if ESP_PANEL_DRIVERS_TOUCH_USE_GT1151
    MAP_ITEM(GT1151),
#endif // CONFIG_ESP_PANEL_TOUCH_GT1151
#if ESP_PANEL_DRIVERS_TOUCH_USE_SPD2010
    MAP_ITEM(SPD2010),
#endif // CONFIG_ESP_PANEL_TOUCH_SPD2010
#if ESP_PANEL_DRIVERS_TOUCH_USE_ST1633
    MAP_ITEM(ST1633),
#endif // CONFIG_ESP_PANEL_TOUCH_ST1633
#if ESP_PANEL_DRIVERS_TOUCH_USE_ST7123
    MAP_ITEM(ST7123),
#endif // CONFIG_ESP_PANEL_TOUCH_ST7123
#if ESP_PANEL_DRIVERS_TOUCH_USE_STMPE610
    MAP_ITEM(STMPE610),
#endif // CONFIG_ESP_PANEL_TOUCH_STMPE610
#if ESP_PANEL_DRIVERS_TOUCH_USE_TT21100
    MAP_ITEM(TT21100),
#endif // CONFIG_ESP_PANEL_TOUCH_TT21100
#if ESP_PANEL_DRIVERS_TOUCH_USE_XPT2046
    MAP_ITEM(XPT2046),
#endif // CONFIG_ESP_PANEL_TOUCH_XPT2046
};

std::shared_ptr<Touch> TouchFactory::create(
    utils::string name, const BusFactory::Config &bus_config, const Touch::Config &touch_config
)
{
    ESP_UTILS_LOGD("Param: name(%s), bus_config(@%p), touch_config(@%p)", name.c_str(), &bus_config, &touch_config);

    auto it = _name_function_map.find(name);
    ESP_UTILS_CHECK_FALSE_RETURN(it != _name_function_map.end(), nullptr, "Unknown controller: %s", name.c_str());

    std::shared_ptr<Touch> device = it->second(bus_config, touch_config);
    ESP_UTILS_CHECK_NULL_RETURN(device, nullptr, "Create device failed");

    return device;
}

} // namespace esp_panel::drivers
