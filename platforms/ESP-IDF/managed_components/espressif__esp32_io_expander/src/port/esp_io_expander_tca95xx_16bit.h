/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
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

#define ESP_IO_EXPANDER_TCA95XX_16BIT_VER_MAJOR    (1)
#define ESP_IO_EXPANDER_TCA95XX_16BIT_VER_MINOR    (0)
#define ESP_IO_EXPANDER_TCA95XX_16BIT_VER_PATCH    (0)

/**
 * @brief Create a new TCA95XX_16BIT IO expander driver
 *
 * @note The I2C communication should be initialized before use this function
 *
 * @param i2c_num: I2C port num
 * @param i2c_address: I2C address of chip (\see esp_io_expander_tca_95xx_16bit_address)
 * @param handle: IO expander handle
 *
 * @return
 *      - ESP_OK: Success, otherwise returns ESP_ERR_xxx
 */
esp_err_t esp_io_expander_new_i2c_tca95xx_16bit(i2c_port_t i2c_num, uint32_t i2c_address, esp_io_expander_handle_t *handle);

/**
 * @brief I2C address of the TCA9539 or TCA9555
 *
 * The 8-bit address format for the TCA9539 is as follows:
 *
 *                (Slave Address)
 *     ┌─────────────────┷─────────────────┐
 *  ┌─────┐─────┐─────┐─────┐─────┐─────┐─────┐─────┐
 *  |  1  |  1  |  1  |  0  |  1  | A1  | A0  | R/W |
 *  └─────┘─────┘─────┘─────┘─────┘─────┘─────┘─────┘
 *     └────────┯──────────────┘     └──┯──┘
 *           (Fixed)        (Hardware Selectable)
 *
 * The 8-bit address format for the TCA9555 is as follows:
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
 * For example, if a TCA9555 chip's A0,A1,A2 are connected to GND, it's 7-bit slave address is 0b0100000.
 * Then users can use `ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_000` to init it.
 */
enum esp_io_expander_tca_95xx_16bit_address {
    ESP_IO_EXPANDER_I2C_TCA9539_ADDRESS_00 = 0b1110100,
    ESP_IO_EXPANDER_I2C_TCA9539_ADDRESS_01 = 0b1110101,
    ESP_IO_EXPANDER_I2C_TCA9539_ADDRESS_10 = 0b1110110,
    ESP_IO_EXPANDER_I2C_TCA9539_ADDRESS_11 = 0b1110111,
    ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_000 = 0b0100000,
    ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_001 = 0b0100001,
    ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_010 = 0b0100010,
    ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_011 = 0b0100011,
    ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_100 = 0b0100000,
    ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_101 = 0b0100101,
    ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_110 = 0b0100110,
    ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_111 = 0b0100111,
};

#ifdef __cplusplus
}
#endif
