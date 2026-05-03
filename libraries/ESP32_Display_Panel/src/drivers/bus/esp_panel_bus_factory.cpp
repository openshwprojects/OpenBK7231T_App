/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc/soc_caps.h"
#include "utils/esp_panel_utils_log.h"
#include "esp_utils_helpers.h"
#include "esp_panel_bus_conf_internal.h"
#include "esp_panel_bus_factory.hpp"

namespace esp_panel::drivers {

#define TYPE_NAME_MAP_ITEM(type_name)                                                 \
    {                                                                               \
        Bus ##type_name::BASIC_ATTRIBUTES_DEFAULT.type, #type_name   \
    }
#define DEVICE_CREATOR(type_name)                                                                     \
    [](const BusFactory::Config &config) -> std::shared_ptr<Bus> {                                                            \
        ESP_UTILS_LOG_TRACE_ENTER();                                                                  \
        std::shared_ptr<Bus> device = nullptr;                                                        \
        ESP_UTILS_CHECK_FALSE_RETURN(std::holds_alternative<Bus ##type_name::Config>(config), device, \
            "Bus config is not a " #type_name " config"                                               \
        );                                                                                            \
        ESP_UTILS_CHECK_EXCEPTION_RETURN(                                                             \
            (device = utils::make_shared<Bus ##type_name>(                                        \
                std::get<Bus ##type_name::Config>(config))                                            \
            ), nullptr, "Create " #type_name " failed"                                                \
        );                                                                                            \
        ESP_UTILS_LOG_TRACE_EXIT();                                                                   \
        return device;                                                                                \
    }
#define TYPE_CREATOR_MAP_ITEM(type_name)                                                 \
    {                                                                               \
        Bus ##type_name::BASIC_ATTRIBUTES_DEFAULT.type, DEVICE_CREATOR(type_name)   \
    }

const utils::unordered_map<int, utils::string> BusFactory::_type_name_map = {
#if ESP_PANEL_DRIVERS_BUS_ENABLE_I2C
    TYPE_NAME_MAP_ITEM(I2C),
#endif
#if ESP_PANEL_DRIVERS_BUS_ENABLE_SPI
    TYPE_NAME_MAP_ITEM(SPI),
#endif
#if ESP_PANEL_DRIVERS_BUS_ENABLE_QSPI
    TYPE_NAME_MAP_ITEM(QSPI),
#endif
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB
    TYPE_NAME_MAP_ITEM(RGB),
#endif
#if ESP_PANEL_DRIVERS_BUS_ENABLE_MIPI_DSI
    TYPE_NAME_MAP_ITEM(DSI),
#endif
};

const utils::unordered_map<int, BusFactory::FunctionDeviceConstructor> BusFactory::_type_constructor_map = {
#if ESP_PANEL_DRIVERS_BUS_USE_I2C
    TYPE_CREATOR_MAP_ITEM(I2C),
#endif
#if ESP_PANEL_DRIVERS_BUS_USE_SPI
    TYPE_CREATOR_MAP_ITEM(SPI),
#endif
#if ESP_PANEL_DRIVERS_BUS_USE_QSPI
    TYPE_CREATOR_MAP_ITEM(QSPI),
#endif
#if ESP_PANEL_DRIVERS_BUS_USE_RGB
    TYPE_CREATOR_MAP_ITEM(RGB),
#endif
#if ESP_PANEL_DRIVERS_BUS_USE_MIPI_DSI
    TYPE_CREATOR_MAP_ITEM(DSI),
#endif
};

std::shared_ptr<Bus> BusFactory::create(const Config &config)
{
    ESP_UTILS_LOG_TRACE_ENTER();

    ESP_UTILS_LOGD("Param: config(@%p)", &config);

    auto name = getTypeNameString(getConfigType(config)).c_str();

    auto type = getConfigType(config);
    ESP_UTILS_LOGD("Get config type: %d(%s)", type, name);

    auto it = _type_constructor_map.find(type);
    ESP_UTILS_CHECK_FALSE_RETURN(
        it != _type_constructor_map.end(), nullptr, "Disabled or unsupported type: %d(%s)", type, name
    );

    std::shared_ptr<Bus> device = it->second(config);
    ESP_UTILS_CHECK_NULL_RETURN(device, nullptr, "Create device(%s) failed", name);

    ESP_UTILS_LOG_TRACE_EXIT();

    return device;
}

int BusFactory::getConfigType(const Config &config)
{
#if ESP_PANEL_DRIVERS_BUS_ENABLE_I2C
    if (std::holds_alternative<BusI2C::Config>(config)) {
        return BusI2C::BASIC_ATTRIBUTES_DEFAULT.type;
    }
#endif // ESP_PANEL_DRIVERS_BUS_ENABLE_I2C
#if ESP_PANEL_DRIVERS_BUS_ENABLE_SPI
    if (std::holds_alternative<BusSPI::Config>(config)) {
        return BusSPI::BASIC_ATTRIBUTES_DEFAULT.type;
    }
#endif // ESP_PANEL_DRIVERS_BUS_ENABLE_SPI
#if ESP_PANEL_DRIVERS_BUS_ENABLE_QSPI
    if (std::holds_alternative<BusQSPI::Config>(config)) {
        return BusQSPI::BASIC_ATTRIBUTES_DEFAULT.type;
    }
#endif // ESP_PANEL_DRIVERS_BUS_ENABLE_QSPI
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB
    if (std::holds_alternative<BusRGB::Config>(config)) {
        return BusRGB::BASIC_ATTRIBUTES_DEFAULT.type;
    }
#endif // ESP_PANEL_DRIVERS_BUS_ENABLE_RGB
#if ESP_PANEL_DRIVERS_BUS_ENABLE_MIPI_DSI
    if (std::holds_alternative<BusDSI::Config>(config)) {
        return BusDSI::BASIC_ATTRIBUTES_DEFAULT.type;
    }
#endif // ESP_PANEL_DRIVERS_BUS_ENABLE_MIPI_DSI

    return -1;
}

utils::string BusFactory::getTypeNameString(int type)
{
    auto it = _type_name_map.find(type);
    if (it != _type_name_map.end()) {
        return it->second;
    }

    return "Unknown";
}

} // namespace esp_panel::drivers
