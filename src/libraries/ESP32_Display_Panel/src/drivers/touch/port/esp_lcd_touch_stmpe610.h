/*
 * SPDX-FileCopyrightText: 2022-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP LCD touch: STMPE610
 */

#pragma once

#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_LCD_TOUCH_STMPE610_VER_MAJOR    (1)
#define ESP_LCD_TOUCH_STMPE610_VER_MINOR    (0)
#define ESP_LCD_TOUCH_STMPE610_VER_PATCH    (6)

/**
 * @brief Create a new STMPE610 touch driver
 *
 * @note The SPI communication should be initialized before use this function.
 *
 * @param io LCD/Touch panel IO handle
 * @param config: Touch configuration
 * @param out_touch: Touch instance handle
 * @return
 *      - ESP_OK                    on success
 *      - ESP_ERR_NO_MEM            if there is no memory for allocating main structure
 */
esp_err_t esp_lcd_touch_new_spi_stmpe610(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config, esp_lcd_touch_handle_t *out_touch);

/**
 * @brief Recommended clock for SPI read of the STMPE610
 *
 */
#define ESP_LCD_TOUCH_SPI_CLOCK_HZ   (1 * 1000 * 1000)

/**
 * @brief Communication SPI device IO structure
 *
 */
#define ESP_LCD_TOUCH_IO_SPI_STMPE610_CONFIG(touch_cs)      \
    {                                               \
        .cs_gpio_num = touch_cs,                    \
        .pclk_hz = ESP_LCD_TOUCH_SPI_CLOCK_HZ,      \
        .lcd_cmd_bits = 8,                          \
        .lcd_param_bits = 8,                        \
        .spi_mode = 1,                              \
        .trans_queue_depth = 1,                    \
    }

#ifdef __cplusplus
}
#endif
