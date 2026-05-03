/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @note This file shouldn't be included in the public header file.
 */

#pragma once

// *INDENT-OFF*

#include "esp_panel_board_conf_internal.h"
#include "supported/esp_panel_board_config_supported.h"
#include "custom/esp_panel_board_config_custom.h"

/* Check if select both custom and supported board */
#if ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED && ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM
    #error "Please select either a custom or a supported development board, cannot enable both simultaneously"
#endif

/* Check if using a default board */
#define ESP_PANEL_BOARD_USE_DEFAULT     (ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED || ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM)

#if ESP_PANEL_BOARD_USE_DEFAULT
    /**
     * There are three purposes to include the this file:
     *  1. Convert configuration items starting with `CONFIG_` to the required configuration items.
     *  2. Define default values for configuration items that are not defined to keep compatibility.
     *  3. Check if missing configuration items
     */
    #include "custom/esp_panel_board_kconfig_custom.h"
#endif

/**
 * Define the name of drivers
 */
#ifdef ESP_PANEL_BOARD_LCD_BUS_TYPE
    #if ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_SPI
        #define ESP_PANEL_BOARD_LCD_BUS_NAME    SPI
    #elif ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_QSPI
        #define ESP_PANEL_BOARD_LCD_BUS_NAME    QSPI
    #elif ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB
        #define ESP_PANEL_BOARD_LCD_BUS_NAME    RGB
    #elif ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_I2C
        #define ESP_PANEL_BOARD_LCD_BUS_NAME    I2C
    #elif ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_I80
        #define ESP_PANEL_BOARD_LCD_BUS_NAME    I80
    #elif ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_MIPI_DSI
        #define ESP_PANEL_BOARD_LCD_BUS_NAME    DSI
    #else
        #error "Unknown LCD bus type selected!"
    #endif
#endif
#ifdef ESP_PANEL_BOARD_TOUCH_BUS_TYPE
    #if ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_SPI
        #define ESP_PANEL_BOARD_TOUCH_BUS_NAME    SPI
    #elif ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_I2C
        #define ESP_PANEL_BOARD_TOUCH_BUS_NAME    I2C
    #else
        #error "Unknown touch bus type selected!"
    #endif
#endif
#ifdef ESP_PANEL_BOARD_BACKLIGHT_TYPE
    #if ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_SWITCH_GPIO
        #define ESP_PANEL_BOARD_BACKLIGHT_NAME    SwitchGPIO
    #elif ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_SWITCH_EXPANDER
        #define ESP_PANEL_BOARD_BACKLIGHT_NAME    SwitchExpander
    #elif ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_PWM_LEDC
        #define ESP_PANEL_BOARD_BACKLIGHT_NAME    PWM_LEDC
    #elif ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_CUSTOM
        #define ESP_PANEL_BOARD_BACKLIGHT_NAME    Custom
    #else
        #error "Unknown backlight type selected!"
    #endif
#endif

// *INDENT-ON*
