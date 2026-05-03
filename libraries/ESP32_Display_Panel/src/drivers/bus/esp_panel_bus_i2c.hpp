/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <memory>
#include <variant>
#include "driver/i2c.h"
#include "esp_panel_types.h"
#include "utils/esp_panel_utils_cxx.hpp"
#include "esp_panel_bus_conf_internal.h"
#include "esp_panel_bus.hpp"

namespace esp_panel::drivers {

// Forward declaration
class HostI2C;

/**
 * @brief The I2C bus class
 *
 * This class is derived from `Bus` class and provides I2C bus implementation for ESP Panel
 */
class BusI2C: public Bus {
public:
    /**
     * @brief Default values for I2C bus configuration
     */
    static constexpr BasicAttributes BASIC_ATTRIBUTES_DEFAULT = {
        .type = ESP_PANEL_BUS_TYPE_I2C,
        .name = "I2C",
    };
    static constexpr int I2C_HOST_ID_DEFAULT = static_cast<int>(I2C_NUM_0);
    static constexpr int I2C_CLK_SPEED_DEFAULT = 400 * 1000;


    struct HostPartialConfig {
        int sda_io_num = -1;         ///< GPIO number for SDA signal
        int scl_io_num = -1;         ///< GPIO number for SCL signal
        bool sda_pullup_en = GPIO_PULLUP_ENABLE;  ///< Enable internal pullup for SDA
        bool scl_pullup_en = GPIO_PULLUP_ENABLE;  ///< Enable internal pullup for SCL
        int clk_speed = I2C_CLK_SPEED_DEFAULT;    ///< I2C clock frequency in Hz
    };
    using HostFullConfig = i2c_config_t;

    using ControlPanelFullConfig = esp_lcd_panel_io_i2c_config_t;

    using HostConfig = std::variant<HostPartialConfig, HostFullConfig>;
    using ControlPanelConfig = ControlPanelFullConfig;

    /**
     * @brief The I2C bus configuration structure
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

        int host_id = I2C_HOST_ID_DEFAULT;      ///< I2C host ID
        std::optional<HostConfig> host;         ///< Host configuration. If not set, the host will not be initialized
        ControlPanelConfig control_panel = {};  ///< Control panel configuration
    };

// *INDENT-OFF*
    /**
     * @brief Construct a new I2C bus instance with individual parameters
     *
     * Uses default values for most configurations. Call `config*()` functions to modify the default settings
     *
     * @param[in] scl_io I2C SCL pin
     * @param[in] sda_io I2C SDA pin
     * @param[in] config I2C control panel configuration
     */
    BusI2C(int scl_io, int sda_io, const ControlPanelFullConfig &config):
        Bus(BASIC_ATTRIBUTES_DEFAULT),
        _config{
            // Host
            .host = HostPartialConfig{
                .sda_io_num = sda_io,
                .scl_io_num = scl_io,
            },
            // Control Panel
            .control_panel = config,
        }
    {
    }

    /**
     * @brief Construct a new I2C bus instance with pre-initialized host
     *
     * @param[in] host_id I2C host ID
     * @param[in] config  I2C control panel configuration
     */
    BusI2C(int host_id, const ControlPanelFullConfig &config):
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
     * @brief Construct a new I2C bus instance with complete configuration
     *
     * @param[in] config Complete I2C bus configuration
     */
    BusI2C(const Config &config):
        Bus(BASIC_ATTRIBUTES_DEFAULT),
        _config(config)
    {
    }
// *INDENT-ON*

    /**
     * @brief Destroy the I2C bus instance
     */
    ~BusI2C() override;

    /**
     * @brief Configure I2C host skip initialization
     *
     * @return `true` if configuration succeeds, `false` otherwise
     */
    bool configI2C_HostSkipInit();

    /**
     * @brief Configure I2C internal pullup
     *
     * @param[in] sda_pullup_en Enable internal pullup for SDA
     * @param[in] scl_pullup_en Enable internal pullup for SCL
     * @return `true` if configuration succeeds, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configI2C_PullupEnable(bool sda_pullup_en, bool scl_pullup_en);

    /**
     * @brief Configure I2C clock frequency
     *
     * @param[in] hz Clock frequency in Hz
     * @return `true` if configuration succeeds, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configI2C_FreqHz(uint32_t hz);

    /**
     * @brief Configure I2C device address
     *
     * @param[in] address 7-bit I2C device address
     * @return `true` if configuration succeeds, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configI2C_Address(uint8_t address);

    /**
     * @brief Configure number of bytes in control phase
     *
     * @param[in] num Number of bytes in control phase
     * @return `true` if configuration succeeds, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configI2C_CtrlPhaseBytes(uint8_t num);

    /**
     * @brief Configure DC bit offset in control phase
     *
     * @param[in] num DC bit offset
     * @return `true` if configuration succeeds, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configI2C_DC_BitOffset(uint8_t num);

    /**
     * @brief Configure number of bits for I2C commands
     *
     * @param[in] num Number of bits for commands
     * @return `true` if configuration succeeds, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configI2C_CommandBits(uint8_t num);

    /**
     * @brief Configure number of bits for I2C parameters
     *
     * @param[in] num Number of bits for parameters
     * @return `true` if configuration succeeds, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configI2C_ParamBits(uint8_t num);

    /**
     * @brief Configure I2C flags
     *
     * @param[in] dc_low_on_data Set DC low on data phase
     * @param[in] disable_control_phase Disable control phase
     * @return `true` if configuration succeeds, `false` otherwise
     * @note This function should be called before `init()`
     */
    bool configI2C_Flags(bool dc_low_on_data, bool disable_control_phase);

    /**
     * @brief Initialize the I2C bus
     *
     * @return `true` if initialization succeeds, `false` otherwise
     */
    bool init() override;

    /**
     * @brief Start the I2C bus operation
     *
     * @return `true` if startup succeeds, `false` otherwise
     */
    bool begin() override;

    /**
     * @brief Delete the I2C bus instance and release resources
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
     * @brief Get the I2C device address
     *
     * @return 7-bit I2C device address
     */
    uint8_t getI2cAddress() const
    {
        return static_cast<uint8_t>(getConfig().control_panel.dev_addr);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configI2C_PullupEnable()` instead
     */
    [[deprecated("Use `configI2C_PullupEnable()` instead")]]
    void configI2cPullupEnable(bool sda_pullup_en, bool scl_pullup_en)
    {
        configI2C_PullupEnable(sda_pullup_en, scl_pullup_en);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configI2C_FreqHz()` instead
     */
    [[deprecated("Use `configI2C_FreqHz()` instead")]]
    void configI2cFreqHz(uint32_t hz)
    {
        configI2C_FreqHz(hz);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configI2C_Address()` instead
     */
    [[deprecated("Use `configI2C_Address()` instead")]]
    void configI2cAddress(uint32_t address)
    {
        configI2C_Address(address);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configI2C_CtrlPhaseBytes()` instead
     */
    [[deprecated("Use `configI2C_CtrlPhaseBytes()` instead")]]
    void configI2cCtrlPhaseBytes(uint32_t num)
    {
        configI2C_CtrlPhaseBytes(num);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configI2C_DC_BitOffset()` instead
     */
    [[deprecated("Use `configI2C_DC_BitOffset()` instead")]]
    void configI2cDcBitOffset(uint32_t num)
    {
        configI2C_DC_BitOffset(num);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configI2C_CommandBits()` instead
     */
    [[deprecated("Use `configI2C_CommandBits()` instead")]]
    void configI2cCommandBits(uint32_t num)
    {
        configI2C_CommandBits(num);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configI2C_ParamBits()` instead
     */
    [[deprecated("Use `configI2C_ParamBits()` instead")]]
    void configI2cParamBits(uint32_t num)
    {
        configI2C_ParamBits(num);
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `configI2C_Flags()` instead
     */
    [[deprecated("Use `configI2C_Flags()` instead")]]
    void configI2cFlags(bool dc_low_on_data, bool disable_control_phase)
    {
        configI2C_Flags(dc_low_on_data, disable_control_phase);
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
     * @return Reference to control panel full configuration
     */
    ControlPanelFullConfig &getControlPanelFullConfig();

    Config _config = {};                      ///< I2C bus configuration
    std::shared_ptr<HostI2C> _host = nullptr; ///< I2C host instance
};

} // namespace esp_panel::drivers

/**
 * @brief Alias for backward compatibility
 * @deprecated Use `esp_panel::drivers::BusI2C` instead
 */
using ESP_PanelBusI2C [[deprecated("Use `esp_panel::drivers::BusI2C` instead")]] = esp_panel::drivers::BusI2C;
