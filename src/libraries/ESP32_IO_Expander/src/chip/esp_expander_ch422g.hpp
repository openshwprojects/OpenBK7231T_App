/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_expander_base.hpp"

namespace esp_expander {

/**
 * @brief The CH422G IO expander device class
 *
 * @note  This class is a derived class of `esp_expander::Base`, user can use it directly
 * @note  Pin map:
 *        | Pin Number   | 0   | 1   | 2   | 3   | 4   | 5   | 6   | 7   | 8   | 9   | 10  | 11  |
 *        | ------------ | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
 *        | Function     | IO0 | IO1 | IO2 | IO3 | IO4 | IO5 | IO6 | IO7 | OC0 | OC1 | OC2 | OC3 |
 */
class CH422G: public Base {
public:
    /**
     * @brief Construct a CH422G device. With this function, call `init()` will initialize I2C by using the host
     *        configuration.
     *
     * @param[in] scl_io  I2C SCL pin number
     * @param[in] sda_io  I2C SDA pin number
     * @param[in] address I2C device 7-bit address. Should be like `ESP_IO_EXPANDER_I2C_<chip name>_ADDRESS`.
     */
    CH422G(int scl_io, int sda_io, uint8_t address): Base(scl_io, sda_io, address) {}

    /**
     * @brief Construct a CH422G device. With this function, call `init()` will not initialize I2C, and users should
     *        initialize it manually.
     *
     * @param[in] host_id I2C host ID.
     * @param[in] address I2C device 7-bit address. Should be like `ESP_IO_EXPANDER_I2C_<chip name>_ADDRESS`.
     */
    CH422G(int host_id, uint8_t address): Base(host_id, address) {}

    /**
     * @brief Construct a CH422G device.
     *
     * @param[in] config Configuration for the object
     */
    CH422G(const Config &config): Base(config) {}

    /**
     * @deprecated Deprecated and will be removed in the next major version. Please use other constructors instead.
     */
    [[deprecated("Deprecated and will be removed in the next major version. Please use other constructors instead.")]]
    CH422G(i2c_port_t id, uint8_t address, int scl_io, int sda_io): Base(id, address, scl_io, sda_io) {}

    /**
     * @brief Desutruct object. This function will call `del()` to delete the object.
     */
    ~CH422G() override;

    /**
     * @brief Begin object
     *
     * @note  This function typically calls `esp_io_expander_new_i2c_*()` to create the IO expander handle.
     * @note  This function sets all IO0-7 pins to output high-level mode by default.
     *
     * @return true if success, otherwise false
     */
    bool begin(void) override;

    /**
     * @brief Enable OC0-OC3 output open-drain
     *
     * @return true if success, otherwise false
     */
    bool enableOC_OpenDrain(void);

    /**
     * @brief Enable OC0-OC3 output push-pull (default mode when power-on)
     *
     * @return true if success, otherwise false
     */
    bool enableOC_PushPull(void);

    /**
     * @brief Enable IO0-7 input mode
     *
     * @note  The driver initialization by default sets CH422G's IO0-7 to output high-level mode.
     * @note  Since the input/output mode of CH422G's IO0-7 must remain consistent, the driver will only set IO0-7 to
     *        input mode when it determines that all pins are configured as input.
     *
     * @return true if success, otherwise false
     */
    bool enableAllIO_Input(void);

    /**
     * @brief Enable IO0-7 output mode
     *
     * @return true if success, otherwise false
     */
    bool enableAllIO_Output(void);
};

} // namespace esp_expander

/**
 * @deprecated Deprecated and will be removed in the next major version. Please use `esp_expander::CH422G` instead.
 */
typedef esp_expander::CH422G ESP_IOExpander_CH422G __attribute__((deprecated("Deprecated and will be removed in the next major version. Please use `esp_expander::CH422G` instead.")));
