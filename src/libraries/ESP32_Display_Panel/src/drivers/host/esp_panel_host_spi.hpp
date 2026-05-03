/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sdkconfig.h"
#include "driver/spi_master.h"
#include "esp_panel_host.hpp"

namespace esp_panel::drivers {

/**
 * @brief SPI bus host class
 */
class HostSPI : public Host<HostSPI, spi_bus_config_t, static_cast<int>(SPI_HOST_MAX)> {
public:
    /* Add friend class to allow them to access the private member */
    template <typename U>
    friend struct esp_utils::GeneralMemoryAllocator;    // To access `HostSPI()`
    template <class Instance, typename Config, int N>
    friend class Host;                                  // To access `del()`, `calibrateConfig()`

    /**
     * @brief Destroy the host
     */
    ~HostSPI() override;

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
    HostSPI(int id, const spi_bus_config_t &config):
        Host<HostSPI, spi_bus_config_t, static_cast<int>(SPI_HOST_MAX)>(id, config) {}

    /**
     * @brief Calibrate configuration when host already exists
     *
     * @param[in] config New configuration
     * @return `true` if successful, `false` otherwise
     */
    bool calibrateConfig(const spi_bus_config_t &config) override;
};

/**
 * SPI & QSPI Host Default Configuration
 *
 */
/* Refer to `hal/spi_ll.h` in SDK (ESP-IDF) */
#ifdef CONFIG_IDF_TARGET_ESP32
// ESP32
#define ESP_PANEL_HOST_SPI_MAX_TRANSFER_SIZE   ((1U << 24) >> 3)
#elif CONFIG_IDF_TARGET_ESP32S2
// ESP32-S2
#define ESP_PANEL_HOST_SPI_MAX_TRANSFER_SIZE   ((1U << 23) >> 3)
#else
// ESP32-C3, ESP32-C3, ESP32-C5, ESP32-C6, ESP32-C61
// ESP32-S3
// ESP32-P4
#define ESP_PANEL_HOST_SPI_MAX_TRANSFER_SIZE   ((1U << 18) >> 3)
#endif

} // namespace esp_panel::drivers
