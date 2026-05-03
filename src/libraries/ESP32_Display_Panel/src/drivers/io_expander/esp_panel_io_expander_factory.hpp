/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include "utils/esp_panel_utils_cxx.hpp"
#include "esp_panel_io_expander.hpp"
#include "esp_panel_io_expander_conf_internal.h"

namespace esp_panel::drivers {

/**
 * @brief Factory class for creating IO expander device instances
 */
class IO_ExpanderFactory {
public:
    /**
     * @brief Function pointer type for IO expander device constructors
     *
     * @param[in] config IO expander device configuration
     * @return Shared pointer to the created IO expander device
     */
    using FunctionDeviceConstructor = std::shared_ptr<IO_Expander> (*)(const IO_Expander::Config &config);

    /**
     * @brief Create a new IO expander device instance
     *
     * @param[in] name Name of the IO expander device to create
     * @param[in] config Configuration for the IO expander device
     * @return Shared pointer to the created IO expander device if successful, nullptr otherwise
     */
    static std::shared_ptr<IO_Expander> create(utils::string name, const IO_Expander::Config &config);

private:
    /**
     * @brief Map of IO expander device names to their constructor functions
     */
    static const utils::unordered_map<utils::string, FunctionDeviceConstructor> _name_function_map;
};

} // namespace esp_panel::drivers
