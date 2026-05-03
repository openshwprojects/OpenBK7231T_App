/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_panel_types.h"
#include "utils/esp_panel_utils_log.h"
#include "esp_panel_backlight_conf_internal.h"
#include "esp_panel_backlight_factory.hpp"

namespace esp_panel::drivers {

struct ConfigVisitor {
    auto operator()(const BacklightSwitchGPIO::Config &config) const
    {
#if ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_SWITCH_GPIO
        return BacklightSwitchGPIO::BASIC_ATTRIBUTES_DEFAULT.type;
#else
        return -1;
#endif
    }

    auto operator()(const BacklightSwitchExpander::Config &config) const
    {
#if ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_SWITCH_EXPANDER
        return BacklightSwitchExpander::BASIC_ATTRIBUTES_DEFAULT.type;
#else
        return -1;
#endif
    }

    auto operator()(const BacklightPWM_LEDC::Config &config) const
    {
#if ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_PWM_LEDC
        return BacklightPWM_LEDC::BASIC_ATTRIBUTES_DEFAULT.type;
#else
        return -1;
#endif
    }

    auto operator()(const BacklightCustom::Config &config) const
    {
#if ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_CUSTOM
        return BacklightCustom::BASIC_ATTRIBUTES_DEFAULT.type;
#else
        return -1;
#endif
    }
};

#define TYPE_NAME_MAP_ITEM(type_name)                                      \
    {                                                                      \
        Backlight ##type_name::BASIC_ATTRIBUTES_DEFAULT.type, #type_name   \
    }

const utils::unordered_map<int, utils::string> BacklightFactory::_type_name_map = {
#if ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_SWITCH_GPIO
    TYPE_NAME_MAP_ITEM(SwitchGPIO),
#endif
#if ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_SWITCH_EXPANDER
    TYPE_NAME_MAP_ITEM(SwitchExpander),
#endif
#if ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_PWM_LEDC
    TYPE_NAME_MAP_ITEM(PWM_LEDC),
#endif
#if ESP_PANEL_DRIVERS_BACKLIGHT_ENABLE_CUSTOM
    TYPE_NAME_MAP_ITEM(Custom),
#endif
};

#define DEVICE_CREATOR(type_name)                                                                           \
    [](const Config &config) -> std::shared_ptr<Backlight> {                                                \
        ESP_UTILS_LOG_TRACE_ENTER();                                                                        \
        std::shared_ptr<Backlight> device = nullptr;                                                        \
        ESP_UTILS_CHECK_FALSE_RETURN(std::holds_alternative<Backlight ##type_name::Config>(config), device, \
            "Not a " #type_name " config"                                                         \
        );                                                                                                  \
        ESP_UTILS_CHECK_EXCEPTION_RETURN(                                                                   \
            (device = utils::make_shared<Backlight ##type_name>(                                        \
                std::get<Backlight ##type_name::Config>(config))                                            \
            ), nullptr, "Create " #type_name " failed"                                                      \
        );                                                                                                  \
        ESP_UTILS_LOG_TRACE_EXIT();                                                                         \
        return device;                                                                                      \
    }
#define TYPE_CREATOR_MAP_ITEM(type_name)                                                 \
    {                                                                               \
        Backlight ##type_name::BASIC_ATTRIBUTES_DEFAULT.type, DEVICE_CREATOR(type_name)   \
    }

const utils::unordered_map<int, BacklightFactory::FunctionDeviceConstructor> BacklightFactory::_type_constructor_map = {
#if ESP_PANEL_DRIVERS_BACKLIGHT_USE_SWITCH_GPIO
    TYPE_CREATOR_MAP_ITEM(SwitchGPIO),
#endif
#if ESP_PANEL_DRIVERS_BACKLIGHT_USE_SWITCH_EXPANDER
    TYPE_CREATOR_MAP_ITEM(SwitchExpander),
#endif
#if ESP_PANEL_DRIVERS_BACKLIGHT_USE_PWM_LEDC
    TYPE_CREATOR_MAP_ITEM(PWM_LEDC),
#endif
#if ESP_PANEL_DRIVERS_BACKLIGHT_USE_CUSTOM
    TYPE_CREATOR_MAP_ITEM(Custom),
#endif
};

std::shared_ptr<Backlight> BacklightFactory::create(const Config &config)
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

    std::shared_ptr<Backlight> device = it->second(config);
    ESP_UTILS_CHECK_NULL_RETURN(device, nullptr, "Create device(%s) failed", name);

    ESP_UTILS_LOG_TRACE_EXIT();

    return device;
}

int BacklightFactory::getConfigType(const Config &config)
{
    return std::visit(ConfigVisitor{}, config);
}

utils::string BacklightFactory::getTypeNameString(int type)
{
    auto it = _type_name_map.find(type);
    if (it != _type_name_map.end()) {
        return it->second;
    }

    return "Unknown";
}

} // namespace esp_panel::drivers
