/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils/esp_panel_utils_log.h"
#include "esp_io_expander.hpp"
#include "esp_panel_io_expander_conf_internal.h"
#include "esp_panel_io_expander_adapter.hpp"
#include "esp_panel_io_expander_factory.hpp"

namespace esp_panel::drivers {

#define DEVICE_CREATOR(chip) \
    [](const IO_Expander::Config &config) -> std::shared_ptr<IO_Expander> { \
        std::shared_ptr<IO_Expander> device = nullptr; \
        ESP_UTILS_CHECK_EXCEPTION_RETURN( \
            (device = utils::make_shared<IO_ExpanderAdapter<esp_expander::chip>>( \
                IO_Expander::BasicAttributes{#chip}, config \
            )), nullptr, "Create " #chip " failed" \
        ); \
        return device; \
    }
#define MAP_ITEM(chip) \
    {#chip, DEVICE_CREATOR(chip)}

const utils::unordered_map<utils::string, IO_ExpanderFactory::FunctionDeviceConstructor>
IO_ExpanderFactory::_name_function_map = {
#if ESP_PANEL_DRIVERS_EXPANDER_USE_CH422G
    MAP_ITEM(CH422G),
#endif
#if ESP_PANEL_DRIVERS_EXPANDER_USE_HT8574
    MAP_ITEM(HT8574),
#endif
#if ESP_PANEL_DRIVERS_EXPANDER_USE_TCA95XX_8BIT
    MAP_ITEM(TCA95XX_8BIT),
#endif
#if ESP_PANEL_DRIVERS_EXPANDER_USE_TCA95XX_16BIT
    MAP_ITEM(TCA95XX_16BIT),
#endif
};

std::shared_ptr<IO_Expander> IO_ExpanderFactory::create(utils::string name, const IO_Expander::Config &config)
{
    ESP_UTILS_LOGD("Param: name(%s), config(@%p)", name.c_str(), &config);

    auto it = _name_function_map.find(name);
    ESP_UTILS_CHECK_FALSE_RETURN(it != _name_function_map.end(), nullptr, "Unknown controller: %s", name.c_str());

    std::shared_ptr<IO_Expander> device = it->second(config);
    ESP_UTILS_CHECK_NULL_RETURN(device, nullptr, "Create device failed");

    return device;
}

} // namespace esp_panel::drivers
