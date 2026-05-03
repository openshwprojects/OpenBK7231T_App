/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "soc/soc_caps.h"
#include "sdkconfig.h"

/**
 * @brief Macros for bus type
 */
#define ESP_PANEL_BUS_TYPE_SPI              (0)
#define ESP_PANEL_BUS_TYPE_QSPI             (1)
#define ESP_PANEL_BUS_TYPE_RGB              (2)
#define ESP_PANEL_BUS_TYPE_I2C              (3)
#define ESP_PANEL_BUS_TYPE_I80              (4)
#define ESP_PANEL_BUS_TYPE_MIPI_DSI         (5)

/**
 * @brief  Macros for LCD color format bits
 */
#define ESP_PANEL_LCD_COLOR_BITS_RGB565     (16)
#define ESP_PANEL_LCD_COLOR_BITS_RGB666     (18)
#define ESP_PANEL_LCD_COLOR_BITS_RGB888     (24)

/**
 * @brief Macros for backlight
 */
#define ESP_PANEL_BACKLIGHT_TYPE_SWITCH_GPIO        (0)
#define ESP_PANEL_BACKLIGHT_TYPE_SWITCH_EXPANDER    (1)
#define ESP_PANEL_BACKLIGHT_TYPE_PWM_LEDC           (2)
#define ESP_PANEL_BACKLIGHT_TYPE_CUSTOM             (3)
