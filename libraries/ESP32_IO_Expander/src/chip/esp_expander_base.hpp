/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <variant>
#include "driver/i2c.h"
#include "port/esp_io_expander.h"

// Refer to `esp32-hal-gpio.h` in Arduino
#ifndef INPUT
#define INPUT             0x01
#endif
#ifndef OUTPUT
#define OUTPUT            0x03
#endif
#ifndef LOW
#define LOW               0x0
#endif
#ifndef HIGH
#define HIGH              0x1
#endif

namespace esp_expander {

/**
 * @brief The IO expander device class
 *
 * @note  This class is a base class for all types of IO expander chips. Due to it is a virtual class, users cannot use
 *        it directly
 */
class Base {
public:
    /**
     * Here are some default values for I2C bus
     */
    constexpr static int I2C_HOST_ID_DEFAULT = static_cast<int>(I2C_NUM_0);
    constexpr static int I2C_CLK_SPEED_DEFAULT = 400 * 1000;

    using DeviceHandle = esp_io_expander_handle_t;

    struct HostPartialConfig {
        int sda_io_num = -1;
        int scl_io_num = -1;
        bool sda_pullup_en = GPIO_PULLUP_ENABLE;
        bool scl_pullup_en = GPIO_PULLUP_ENABLE;
        int clk_speed = I2C_CLK_SPEED_DEFAULT;
    };
    using HostFullConfig = i2c_config_t;
    using HostConfig = std::variant<HostPartialConfig, HostFullConfig>;

    struct DeviceConfig {
        uint8_t address = 0;
    };

    /**
     * @brief Configuration for Base object
     */
    struct Config {
        void convertPartialToFull(void);
        void printHostConfig(void) const;
        void printDeviceConfig(void) const;

        bool isHostConfigValid(void) const
        {
            return host.has_value();
        }

        int host_id = I2C_HOST_ID_DEFAULT;  /*!< I2C host ID */
        std::optional<HostConfig> host;     /*!< I2C host configuration */
        DeviceConfig device = {};           /*!< I2C device configuration */
    };

    /**
     * @brief The driver state enumeration
     */
    enum class State : uint8_t {
        DEINIT = 0,
        INIT,
        BEGIN,
    };

// *INDENT-OFF*
    /**
     * @brief Construct a base device. With this function, call `init()` will initialize I2C by using the host
     *        configuration.
     *
     * @param[in] scl_io  I2C SCL pin number
     * @param[in] sda_io  I2C SDA pin number
     * @param[in] address I2C device 7-bit address. Should be like `ESP_IO_EXPANDER_I2C_<chip_name>_ADDRESS`.
     */
    Base(int scl_io, int sda_io, uint8_t address):
        _config{
            .host_id = I2C_HOST_ID_DEFAULT,
            .host = HostPartialConfig{
                .sda_io_num = sda_io,
                .scl_io_num = scl_io,
            },
            .device = DeviceConfig{
                .address = address
            }
        }
    {
    }

    /**
     * @brief Construct a base device. With this function, call `init()` will not initialize I2C, and users should
     *        initialize it manually.
     *
     * @param[in] host_id I2C host ID.
     * @param[in] address I2C device 7-bit address. Should be like `ESP_IO_EXPANDER_I2C_<chip_name>_ADDRESS`.
     */
    Base(int host_id, uint8_t address):
        _config{
            .host_id = host_id,
            .device = DeviceConfig{
                .address = address
            }
        }
    {
    }
// *INDENT-ON*

    /**
     * @brief Construct a base device.
     *
     * @param[in] config Configuration for the object
     */
    Base(const Config &config): _config(config) {}

    /**
     * @brief Virtual desutruct object.
     *
     * @note  Here make it virtual so that we can delete the derived object by using the base pointer.
     */
    virtual ~Base() = default;

    /**
     * @brief Configure whether to skip I2C initialization
     *
     * @note  This function should be called before `init()`.
     *
     * @param[in] skip_init Whether to skip I2C initialization
     *
     * @return true if success, otherwise false
     */
    bool configHostSkipInit(bool skip_init);

    /**
     * @brief Initialize object
     *
     * @note  This function will initialize I2C if needed.
     *
     * @return true if success, otherwise false
     */
    bool init(void);

    /**
     * @brief Begin object
     *
     * @note  This function typically calls `esp_io_expander_new_i2c_*()` to create the IO expander handle.
     */
    virtual bool begin(void) = 0;

    /**
     * @brief Reset object
     *
     * @return true if success, otherwise false
     */
    bool reset(void);

    /**
     * @brief Delete object
     */
    bool del(void);

    /**
     * @brief Set pin mode
     *
     * @note  This function is same as Arduino's `pinMode()`.
     *
     * @param[in] pin  Pin number (0-31)
     * @param[in] mode Pin mode (INPUT / OUTPUT)
     *
     * @return true if success, otherwise false
     */
    bool pinMode(uint8_t pin, uint8_t mode);

    /**
     * @brief Set pin level
     *
     * @note  This function is same as Arduino's `digitalWrite()`.
     *
     * @param[in] pin   Pin number (0-31)
     * @param[in] value Pin level (HIGH / LOW)
     *
     * @return true if success, otherwise false
     */
    bool digitalWrite(uint8_t pin, uint8_t value);

    /**
     * @brief Read pin level
     *
     * @note  This function is same as Arduino's `digitalRead()`.
     *
     * @param[in] pin Pin number (0-31)
     *
     * @return Pin level. HIGH or LOW if success, otherwise -1
     */
    int digitalRead(uint8_t pin);

    /**
     * @brief Set multiple pin modes
     *
     * @param pin_mask Pin mask (Bitwise OR of `IO_EXPANDER_PIN_NUM_*`)
     * @param mode     Mode to set (INPUT / OUTPUT)
     *
     * @return true if success, otherwise false
     */
    bool multiPinMode(uint32_t pin_mask, uint8_t mode);

    /**
     * @brief Set multiple pins level
     *
     * @param pin_mask Pin mask (Bitwise OR of `IO_EXPANDER_PIN_NUM_*`)
     * @param value Value to write (HIGH / LOW)
     *
     * @return true if success, otherwise false
     */
    bool multiDigitalWrite(uint32_t pin_mask, uint8_t value);

    /**
     * @brief Read multiple pin levels
     *
     * @param pin_mask Pin mask (Bitwise OR of `IO_EXPANDER_PIN_NUM_*`)
     *
     * @return Pin levels, every bit represents a pin (HIGH / LOW)
     */
    int64_t multiDigitalRead(uint32_t pin_mask);

    /**
     * @brief Print IO expander status, include pin index, direction, input level and output level
     *
     * @return Pin levels, every bit represents a pin (HIGH / LOW)
     */
    bool printStatus(void) const;

    /**
     * @brief Check if the driver has reached or passed the specified state
     *
     * @param[in] state The state to check against current state
     *
     * @return true if current state >= given state, otherwise false
     */
    bool isOverState(State state) const
    {
        return (_state >= state);
    }

    /**
     * @brief Get the IO expander configuration
     *
     * @return IO expander Configuration
     */
    const Config &getConfig(void) const
    {
        return _config;
    }

    /**
     * @brief Get low-level handle. Users can use this handle to call low-level functions (esp_io_expander_*()).
     *
     * @return Device handle if success, otherwise nullptr
     */
    DeviceHandle getDeviceHandle(void) const
    {
        return device_handle;
    }

    // TODO: Remove in the next major version
    Base(i2c_port_t id, uint8_t address, int scl_io, int sda_io):
        Base(scl_io, sda_io, address)
    {
        _config.host_id = id;
    }
    [[deprecated("Deprecated and will be removed in the next major version. Please use `getDeviceHandle()` instead.")]]
    esp_io_expander_handle_t getHandle(void) const
    {
        return getDeviceHandle();
    }

protected:
    bool isHostSkipInit(void) const
    {
        return !_config.isHostConfigValid() || _is_host_skip_init;
    }

    void setState(State state)
    {
        _state = state;
    }

    DeviceHandle device_handle = nullptr;

private:
    HostFullConfig *getHostFullConfig();

    State _state = State::DEINIT;
    bool _is_host_skip_init = false;
    Config _config = {};
};

} // namespace esp_expander

/**
 * @brief Alias for backward compatibility
 *
 * @deprecated Use `esp_expander::Base` instead
 */
using ESP_IOExpander [[deprecated("Use `esp_expander::Base` instead.")]] = esp_expander::Base;
