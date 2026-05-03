/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <bitset>
#include <string>
#include "esp_lcd_types.h"
#include "esp_lcd_panel_io.h"
#include "esp_panel_bus_conf_internal.h"

namespace esp_panel::drivers {

/**
 * @brief Base class for all bus types
 *
 * This class serves as an abstract base class for different types of buses. Cannot be instantiated directly.
 */
class Bus {
public:
    using ControlPanelHandle = esp_lcd_panel_io_handle_t;

    /**
     * @brief Basic attributes structure for bus configuration
     */
    struct BasicAttributes {
        int type = -1;          /*!< Bus type identifier, defaults to `-1` */
        const char *name = "";  /*!< Bus name string, defaults to `""` */
    };

    /**
     * @brief Driver state enumeration
     */
    enum class State : uint8_t {
        DEINIT = 0,    /*!< Driver is deinitialized */
        INIT,          /*!< Driver is initialized */
        BEGIN,         /*!< Driver has started */
    };

    /**
     * @brief Construct a new Bus instance
     *
     * @param[in] attr Basic attributes for bus configuration
     */
    Bus(const BasicAttributes &attr): _basic_attributes(attr) {}

    /**
     * @brief Virtual destructor
     */
    virtual ~Bus() = default;

    /**
     * @brief Initialize the bus hardware and resources
     *
     * @return `true` if initialization succeeds, `false` otherwise
     */
    virtual bool init() = 0;

    /**
     * @brief Start the bus operation
     *
     * @return `true` if startup succeeds, `false` otherwise
     */
    virtual bool begin() = 0;

    /**
     * @brief Delete the bus and release resources
     *
     * @return `true` if deletion succeeds, `false` otherwise
     * @note Requires calling `init()` to reinitialize the bus after deletion
     */
    virtual bool del() = 0;

    /**
     * @brief Delete the LCD control panel
     *
     * @return `true` if deletion succeeds, `false` otherwise
     * @note Internally calls `esp_lcd_panel_io_del()` to delete the control panel
     */
    bool delControlPanel();

    /**
     * @brief Read data from a register
     *
     * @param[in] address Register address to read from
     * @param[out] data Buffer to store the read data
     * @param[in] data_size Size of data to read in bytes
     *
     * @return `true` if read succeeds, `false` otherwise
     */
    bool readRegisterData(uint32_t address, void *data, uint32_t data_size) const;

    /**
     * @brief Write data to a register
     *
     * @param[in] address Register address to write to
     * @param[in] data Data buffer to write
     * @param[in] data_size Size of data to write in bytes
     *
     * @return `true` if write succeeds, `false` otherwise
     */
    bool writeRegisterData(uint32_t address, const void *data, uint32_t data_size) const;

    /**
     * @brief Write color data to display
     *
     * @param[in] address Register address for color data
     * @param[in] color Color data buffer to write
     * @param[in] color_size Size of color data in bytes
     *
     * @return `true` if write succeeds, `false` otherwise
     */
    bool writeColorData(uint32_t address, const void *color, uint32_t color_size) const;

    /**
     * @brief Disable the LCD control panel handle
     *
     * @note Useful for "3-wire SPI + RGB" LCD panels when `auto_del_panel_io/enable_io_multiplex` is enabled
     */
    void disableControlPanelHandle()
    {
        control_panel = nullptr;
    }

    /**
     * @brief Check if driver state is at or beyond specified state
     *
     * @param[in] state State to check against
     *
     * @return `true` if current state >= given state, `false` otherwise
     */
    bool isOverState(State state) const
    {
        return (_state >= state);
    }

    /**
     * @brief Get the basic attributes of the bus
     *
     * @return Reference to bus basic attributes
     */
    const BasicAttributes &getBasicAttributes() const
    {
        return _basic_attributes;
    }

    /**
     * @brief Get the LCD control panel handle
     *
     * @return Control panel handle if valid, nullptr otherwise
     * @note Handle can be used with low-level `esp_lcd_panel_io_*()` functions
     */
    ControlPanelHandle getControlPanelHandle() const
    {
        return control_panel;
    }

    /**
     * @brief Alias for backward compatibility
     * @deprecated Use `getBasicAttributes().type` instead
     */
    [[deprecated("Use `getBasicAttributes().type` instead")]]
    int getType()
    {
        return getBasicAttributes().type;
    }

protected:
    /**
     * @brief Set the current driver state
     *
     * @param[in] state New state to set
     */
    void setState(State state)
    {
        _state = state;
    }

    /**
     * @brief Check if control panel handle is valid
     *
     * @return `true` if control panel handle is not nullptr, `false` otherwise
     */
    bool isControlPanelValid() const
    {
        return (control_panel != nullptr);
    }

    ControlPanelHandle control_panel = nullptr;  /*!< Control panel handle, created by derived classes */

private:
    State _state = State::DEINIT;              /*!< Current driver state */
    BasicAttributes _basic_attributes = {};     /*!< Bus basic attributes */
};

} // namespace esp_panel::drivers

/**
 * @brief Alias for backward compatibility
 * @deprecated Use `esp_panel::drivers::Bus` instead
 */
using ESP_PanelBus [[deprecated("Use `esp_panel::drivers::Bus` instead")]] = esp_panel::drivers::Bus;
