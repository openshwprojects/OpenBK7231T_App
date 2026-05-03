/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "driver/i2c.h"
#include "esp_panel_host.hpp"

namespace esp_panel::drivers {

/**
 * @brief I2C bus host class
 */
class HostI2C : public Host<HostI2C, i2c_config_t, static_cast<int>(I2C_NUM_MAX)> {
public:
    /* Add friend class to allow them to access the private member */
    template <typename U>
    friend struct esp_utils::GeneralMemoryAllocator;    // To access `HostI2C()`
    template <class Instance, typename Config, int N>
    friend class Host;                                  // To access `del()`, `calibrateConfig()`

    /**
     * @brief Destroy the host
     */
    ~HostI2C() override;

    /**
     * @brief Startup the host
     *
     * @return `true` if successful, `false` otherwise
     */
    bool begin() override;

private:
    /**
     * @brief Private constructor to prevent direct instantiation
     *
     * @param[in] id Host ID
     * @param[in] config Host configuration
     */
    HostI2C(int id, const i2c_config_t &config):
        Host<HostI2C, i2c_config_t, static_cast<int>(I2C_NUM_MAX)>(id, config) {}

    /**
     * @brief Calibrate configuration when host already exists
     *
     * @param[in] config New configuration
     * @return `true` if successful, `false` otherwise
     */
    bool calibrateConfig(const i2c_config_t &config) override;
};

} // namespace esp_panel::drivers
