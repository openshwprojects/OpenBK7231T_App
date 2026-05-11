/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>

#include "driver/i2c.h"
#include "esp_err.h"

#include "esp_io_expander.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_IO_EXPANDER_HT8574_VER_MAJOR    (0)
#define ESP_IO_EXPANDER_HT8574_VER_MINOR    (1)
#define ESP_IO_EXPANDER_HT8574_VER_PATCH    (0)

/**
 * @brief Create a new ht8574 IO expander driver
 *
 * @note The I2C communication should be initialized before use this function
 *
 * @param i2c_num: I2C port num
 * @param i2c_address: I2C address of chip
 * @param handle: IO expander handle
 *
 * @return
 *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
 */
esp_err_t esp_io_expander_new_i2c_ht8574(i2c_port_t i2c_num, uint32_t i2c_address, esp_io_expander_handle_t *handle);

/**
 * @brief I2C address of the ht8574
 *
 * The 8-bit address format is as follows:
 *
 *                (Slave Address)
 *     ┌─────────────────┷─────────────────┐
 *  ┌─────┐─────┐─────┐─────┐─────┐─────┐─────┐─────┐
 *  |  0  |  1  |  1  |  1  | A2  | A1  | A0  | R/W |
 *  └─────┘─────┘─────┘─────┘─────┘─────┘─────┘─────┘
 *     └────────┯────────┘     └─────┯──────┘
 *           (Fixed)        (Hardware Selectable)
 *
 * And the 7-bit slave address is the most important data for users.
 * For example, if a chip's A0,A1,A2 are connected to GND, it's 7-bit slave address is 0111000b(0x38).
 * Then users can use `ESP_IO_EXPANDER_I2C_HT8574_ADDRESS_000` to init it.
 */
#define ESP_IO_EXPANDER_I2C_HT8574_ADDRESS_000    (0x38)
#define ESP_IO_EXPANDER_I2C_HT8574_ADDRESS_001    (0x29)
#define ESP_IO_EXPANDER_I2C_HT8574_ADDRESS_010    (0x2A)
#define ESP_IO_EXPANDER_I2C_HT8574_ADDRESS_011    (0x2B)
#define ESP_IO_EXPANDER_I2C_HT8574_ADDRESS_100    (0x2C)


#ifdef __cplusplus
}
#endif
