/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP IO expander: TCA9554
 */

#pragma once

#include <stdint.h>

#include "driver/i2c.h"
#include "esp_err.h"

#include "esp_io_expander.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_IO_EXPANDER_TCA9554_VER_MAJOR    (1)
#define ESP_IO_EXPANDER_TCA9554_VER_MINOR    (0)
#define ESP_IO_EXPANDER_TCA9554_VER_PATCH    (1)

/**
 * @brief Create a new TCA9554 IO expander driver
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
esp_err_t esp_io_expander_new_i2c_tca9554(i2c_port_t i2c_num, uint32_t i2c_address, esp_io_expander_handle_t *handle);

/**
 * @brief I2C address of the TCA9554
 *
 * The 8-bit address format is as follows:
 *
 *                (Slave Address)
 *     ┌─────────────────┷─────────────────┐
 *  ┌─────┐─────┐─────┐─────┐─────┐─────┐─────┐─────┐
 *  |  0  |  1  |  0  |  0  | A2  | A1  | A0  | R/W |
 *  └─────┘─────┘─────┘─────┘─────┘─────┘─────┘─────┘
 *     └────────┯────────┘     └─────┯──────┘
 *           (Fixed)        (Hardware Selectable)
 *
 * And the 7-bit slave address is the most important data for users.
 * For example, if a chip's A0,A1,A2 are connected to GND, it's 7-bit slave address is 0100000b(0x20).
 * Then users can use `ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000` to init it.
 */
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000    (0x20)
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_001    (0x21)
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_010    (0x22)
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_011    (0x23)
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_100    (0x24)
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_101    (0x25)
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_110    (0x26)
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_111    (0x27)


/**
 * @brief I2C address of the TCA9554A
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
 * Then users can use `ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_000` to init it.
 */
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_000    (0x38)
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_001    (0x39)
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_010    (0x3A)
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_011    (0x3B)
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_100    (0x3C)
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_101    (0x3D)
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_110    (0x3E)
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_111    (0x3F)

#ifdef __cplusplus
}
#endif
