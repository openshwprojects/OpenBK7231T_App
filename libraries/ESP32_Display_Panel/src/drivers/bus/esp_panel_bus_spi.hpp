/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <optional>
#include <variant>
#include <memory>
#include "driver/spi_master.h"
#include "utils/esp_panel_utils_cxx.hpp"
#include "esp_panel_bus_conf_internal.h"
#include "esp_panel_bus.hpp"

namespace esp_panel::drivers {

// Forward declaration
class HostSPI;

/**
 * @brief The SPI bus class for ESP Panel
 *
 * This class is derived from `Bus` class and provides SPI bus implementation for ESP Panel
 */
class BusSPI: public Bus {
public:
    /**
     * @brief Default values for SPI bus configuration
     */
    static constexpr BasicAttributes BASIC_ATTRIBUTES_DEFAULT = {
        .type = ESP_PANEL_BUS_TYPE_SPI,
        .name = "SPI",
    };
    static constexpr int SPI_HOST_ID_DEFAULT = static_cast<int>(SPI2_HOST);
    static constexpr int SPI_PCLK_HZ_DEFAULT = SPI_MASTER_FREQ_40M;

    /**
     * @brief Partial host configuration structure
     */
    struct HostPartialConfig {
        int mosi_io_num = -1;    ///< GPIO number for MOSI signal
        int miso_io_num = -1;    ///< GPIO number for MISO signal
        int sclk_io_num = -1;    ///< GPIO number for SCLK signal
    };
    using HostFullConfig = spi_bus_config_t;

    /**
     * @brief Partial control panel configuration structure
     */
    struct ControlPanelPartialConfig {
        int cs_gpio_num = -1;        ///< GPIO number for CS signal
        int dc_gpio_num = -1;        ///< GPIO number for DC signal
        int spi_mode = 0;            ///< SPI mode (0-3)
        int pclk_hz = SPI_PCLK_HZ_DEFAULT;  ///< SPI clock frequency in Hz
        int lcd_cmd_bits = 8;        ///< Bits for LCD commands
        int lcd_param_bits = 8;      ///< Bits for LCD parameters
    };
    using ControlPanelFullConfig = esp_lcd_panel_io_spi_config_t;

    using HostConfig = std::variant<HostPartialConfig, HostFullConfig>;
    using ControlPanelConfig = std::variant<ControlPanelPartialConfig, ControlPanelFullConfig>;

    /**
     * @brief The SPI bus configuration structure
     */
    struct Config {
        /**
         * @brief Convert partial configurations to full configurations
         */
        void convertPartialToFull();

        /**
         * @brief Print host configuration for debugging
         */
        void printHostConfig() const;

        /**
         * @brief Print control panel configuration for debugging
         */
        void printControlPanelConfig() const;

        /**
         * @brief Check if host configuration is valid
         *
         * @return `true` if host configuration is valid, `false` otherwise
         */
        bool isHostConfigValid() const
        {
            return host.has_value();
        }

        int host_id = SPI_HOST_ID_DEFAULT;  ///< SPI host ID
        std::optional<HostConfig> host;     ///< Host configuration. If not set, the host will not be initialized
        ControlPanelConfig control_panel = ControlPanelPartialConfig{}; ///< Control panel configuration
    };

// *INDENT-OFF*
    /**
     * @brief Construct a new SPI bus instance with individual parameters
     *
     * Uses default values for most configurations. Call `config*()` functions to modify the default settings
     *
     * @param[in] cs_io  GPIO number for SPI CS signal
     * @param[in] dc_io  GPIO number for SPI DC signal
     * @param[in] sck_io GPIO number for SPI SCK signal
     * @param[in] sda_io GPIO number for SPI SDA (MOSI) signal
     * @param[in] sdo_io GPIO number for SPI SDO (MISO) signal, set to -1 if not used
     */
    BusSPI(int cs_io, int dc_io, int sck_io, int sda_io, int sdo_io = -1):
        Bus(BASIC_ATTRIBUTES_DEFAULT),
        _config{
            // Host
            .host = HostPartialConfig{
                .mosi_io_num = sda_io,
                .miso_io_num = sdo_io,
                .sclk_io_num = sck_io,
            },
            // Control Panel
            .control_panel = ControlPanelPartialConfig{
                .cs_gpio_num = cs_io,
                .dc_gpio_num = dc_io,
            },
        }
    {
    }

    /**
     * @brief Construct a new SPI bus instance with pre-initialized host
     *
     * @param[in] host_id SPI host ID
     * @param[in] cs_io GPIO number for CS signal
     * @param[in] dc_io GPIO number for DC signal
     */
    BusSPI(int host_id, int cs_io, int dc_io):
        Bus(BASIC_ATTRIBUTES_DEFAULT),
        _config{
            // General
            .host_id = host_id,
            // Control Panel
            .control_panel = ControlPanelPartialConfig{
                .cs_gpio_num = cs_io,
                .dc_gpio_num = dc_io,
            },
        }
    {
    }

    /**
     * @brief Construct a new SPI bus instance with host initialization
     *
     * @param[in] sck_io GPIO number for SPI SCK signal
     * @param[in] sda_io GPIO number for SPI SDA (MOSI) signal
     * @param[in] sdo_io GPIO number for SPI SDO (MISO) signal
     * @param[in] config Full control panel configuration
     */
    BusSPI(int sck_io, int sda_io, int sdo_io, const ControlPanelFullConfig &config):
        Bus(BASIC_ATTRIBUTES_DEFAULT),
        _config{
            // Host
            .host = HostPartialConfig{
                .mosi_io_num = sda_io,
                .miso_io_num = sdo_io,
                .sclk_io_num = sck_io,
            },
            // Control Panel
            .control_panel = config,
        }
    {
    }

    /**
     * @brief Construct a new SPI bus instance with pre-initialized host
     *
     * @param[in] host_id SPI host ID
     * @param[in] config  Full control panel configuration
     */
    BusSPI(int host_id, const ControlPanelFullConfig &config):
        Bus(BASIC_ATTRIBUTES_DEFAULT),
        _config{
            // General
            .host_id = host_id,
            // Control Panel
            .control_panel = config,
        }
    {
    }

    /**
     * @brief Construct a new SPI bus instance with complete configuration
     *
     * @param[in] config Complete SPI bus configuration
     */
    BusSPI(const Config &config):
        Bus(BASIC_ATTRIBUTES_DEFAULT),
        _config(config)
    {
    }
// *INDENT-ON*

    /**
     * @brief Destroy the SPI bus instance
     */
    ~BusSPI() override;

    /**
     * @brief Configure SPI host to skip initialization
     *
     * @return `true` if configuration succeeds, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configSPI_HostSkipInit();

    /**
     * @brief Configure SPI mode
     *
     * @param[in] mode SPI mode (0-3)
     * @return `true` if configuration succeeds, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configSPI_Mode(uint8_t mode);

    /**
     * @brief Configure SPI clock frequency
     *
     * @param[in] hz Clock frequency in Hz
     * @return `true` if configuration succeeds, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configSPI_FreqHz(uint32_t hz);

    /**
     * @brief Configure number of bits for SPI commands
     *
     * @param[in] num Number of bits for commands
     * @return `true` if configuration succeeds, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configSPI_CommandBits(uint32_t num);

    /**
     * @brief Configure number of bits for SPI parameters
     *
     * @param[in] num Number of bits for parameters
     * @return `true` if configuration succeeds, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configSPI_ParamBits(uint32_t num);

    /**
     * @brief Configure SPI transaction queue depth
     *
     * @param[in] depth Queue depth for SPI transactions
     * @return `true` if configuration succeeds, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configSPI_TransQueueDepth(uint8_t depth);

    /**
     * @brief Initialize the SPI bus
     *
     * @return `true` if initialization succeeds, `false` otherwise
     */
    bool init() override;

    /**
     * @brief Start the SPI bus operation
     *
     * @return `true` if startup succeeds, `false` otherwise
     */
    bool begin() override;

    /**
     * @brief Delete the SPI bus instance and release resources
     *
     * @return `true` if deletion succeeds, `false` otherwise
     */
    bool del() override;

    /**
     * @brief Get the current bus configuration
     *
     * @return Reference to the current bus configuration
     */
    const Config &getConfig() const
    {
        return _config;
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configSPI_Mode()` instead
     */
    [[deprecated("Use `configSPI_Mode()` instead")]]
    void configSpiMode(uint8_t mode)
    {
        configSPI_Mode(mode);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configSPI_FreqHz()` instead
     */
    [[deprecated("Use `configSPI_FreqHz()` instead")]]
    void configSpiFreqHz(uint32_t hz)
    {
        configSPI_FreqHz(hz);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configSPI_CommandBits()` instead
     */
    [[deprecated("Use `configSPI_CommandBits()` instead")]]
    void configSpiCommandBits(uint32_t num)
    {
        configSPI_CommandBits(num);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configSPI_ParamBits()` instead
     */
    [[deprecated("Use `configSPI_ParamBits()` instead")]]
    void configSpiParamBits(uint32_t num)
    {
        configSPI_ParamBits(num);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configSPI_TransQueueDepth()` instead
     */
    [[deprecated("Use `configSPI_TransQueueDepth()` instead")]]
    void configSpiTransQueueDepth(uint8_t depth)
    {
        configSPI_TransQueueDepth(depth);
    }

private:
    /**
     * @brief Check if host is skipped initialization
     *
     * @return `true` if host is skipped initialization, `false` otherwise
     */
    bool isHostSkipInit() const
    {
        return !_config.isHostConfigValid();
    }

    /**
     * @brief Get mutable reference to host full configuration
     *
     * Converts partial configuration to full configuration if necessary
     *
     * @return Reference to host full configuration
     */
    HostFullConfig &getHostFullConfig();

    /**
     * @brief Get mutable reference to control panel full configuration
     *
     * Converts partial configuration to full configuration if necessary
     *
     * @return Reference to control panel full configuration
     */
    ControlPanelFullConfig &getControlPanelFullConfig();

    Config _config = {};                      ///< SPI bus configuration
    std::shared_ptr<HostSPI> _host = nullptr; ///< SPI host instance
};

} // namespace esp_panel::drivers

/**
 * @brief Alias for backward compatibility
 * @deprecated Use `esp_panel::drivers::BusSPI` instead
 */
using ESP_PanelBusSPI [[deprecated("Use `esp_panel::drivers::BusSPI` instead")]] = esp_panel::drivers::BusSPI;
