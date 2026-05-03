/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

// *INDENT-OFF*

#ifndef ESP_PANEL_BOARD_NAME
    #ifdef CONFIG_ESP_PANEL_BOARD_NAME
        #define ESP_PANEL_BOARD_NAME CONFIG_ESP_PANEL_BOARD_NAME
    #else
        #error "Missing configuration: ESP_PANEL_BOARD_NAME"
    #endif
#endif

#ifndef ESP_PANEL_BOARD_WIDTH
    #ifdef CONFIG_ESP_PANEL_BOARD_WIDTH
        #define ESP_PANEL_BOARD_WIDTH CONFIG_ESP_PANEL_BOARD_WIDTH
    #else
        #error "Missing configuration: ESP_PANEL_BOARD_WIDTH"
    #endif
#endif

#ifndef ESP_PANEL_BOARD_HEIGHT
    #ifdef CONFIG_ESP_PANEL_BOARD_HEIGHT
        #define ESP_PANEL_BOARD_HEIGHT CONFIG_ESP_PANEL_BOARD_HEIGHT
    #else
        #error "Missing configuration: ESP_PANEL_BOARD_HEIGHT"
    #endif
#endif

#include "esp_panel_board_kconfig_custom_backlight.h"
#include "esp_panel_board_kconfig_custom_expander.h"
#include "esp_panel_board_kconfig_custom_lcd.h"
#include "esp_panel_board_kconfig_custom_touch.h"

// *INDENT-ON*
