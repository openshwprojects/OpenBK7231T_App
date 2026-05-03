/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "soc/soc_caps.h"
#if SOC_MIPI_DSI_SUPPORTED
#include "hal/mipi_dsi_ll.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_panel_host.hpp"

namespace esp_panel::drivers {

/**
 * @brief MIPI-DSI bus host class
 */
class HostDSI : public Host<HostDSI, esp_lcd_dsi_bus_config_t, MIPI_DSI_LL_NUM_BUS> {
public:
    /* Add friend class to allow them to access the private member */
    template <typename U>
    friend struct esp_utils::GeneralMemoryAllocator;    // To access `HostDSI()`
    template <class Instance, typename Config, int N>
    friend class Host;                                  // To access `del()`, `calibrateConfig()`

    /**
     * @brief Destroy the host
     */
    ~HostDSI() override;

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
    HostDSI(int id, const esp_lcd_dsi_bus_config_t &config):
        Host<HostDSI, esp_lcd_dsi_bus_config_t, MIPI_DSI_LL_NUM_BUS>(id, config) {}

    /**
     * @brief Calibrate configuration when host already exists
     *
     * @param[in] config New configuration
     * @return `true` if successful, `false` otherwise
     */
    bool calibrateConfig(const esp_lcd_dsi_bus_config_t &config) override;
};

} // namespace esp_panel::drivers

#endif // SOC_MIPI_DSI_SUPPORTED
