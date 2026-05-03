/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
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

#define ESP_IO_EXPANDER_CH422G_VER_MAJOR    (0)
#define ESP_IO_EXPANDER_CH422G_VER_MINOR    (1)
#define ESP_IO_EXPANDER_CH422G_VER_PATCH    (0)

/**
 * @brief Create a new ch422g IO expander driver
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
esp_err_t esp_io_expander_new_i2c_ch422g(i2c_port_t i2c_num, uint32_t i2c_address, esp_io_expander_handle_t *handle);

/**
 * @brief I2C address of the ch422g. Just to keep the same with other IO expanders, but it is ignored.
 */
#define ESP_IO_EXPANDER_I2C_CH422G_ADDRESS    (0x24)

esp_err_t esp_io_expander_ch422g_set_oc_open_drain(esp_io_expander_handle_t handle);

esp_err_t esp_io_expander_ch422g_set_oc_push_pull(esp_io_expander_handle_t handle);

esp_err_t esp_io_expander_ch422g_set_all_input(esp_io_expander_handle_t handle);

esp_err_t esp_io_expander_ch422g_set_all_output(esp_io_expander_handle_t handle);

#ifdef __cplusplus
}
#endif
