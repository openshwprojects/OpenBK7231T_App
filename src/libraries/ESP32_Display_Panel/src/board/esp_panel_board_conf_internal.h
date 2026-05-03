/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

// *INDENT-OFF*

#include "esp_panel_conf_internal.h"

#ifndef ESP_PANEL_BOARD_FILE_SKIP
    /* If "esp_panel_board_*.h" are available from here, try to use them later */
    #if __has_include("esp_panel_board_supported_conf.h")
        #ifndef ESP_PANEL_BOARD_INCLUDE_SUPPORTED_SIMPLE
            #define ESP_PANEL_BOARD_INCLUDE_SUPPORTED_SIMPLE
        #endif
    #elif __has_include("../../../esp_panel_board_supported_conf.h")
        #ifndef ESP_PANEL_BOARD_INCLUDE_SUPPORTED_OUTSIDE
            #define ESP_PANEL_BOARD_INCLUDE_SUPPORTED_OUTSIDE
        #endif
    #endif
    #ifndef ESP_PANEL_BOARD_USE_SUPPORTED_FILE
        #if defined(ESP_PANEL_BOARD_INCLUDE_SUPPORTED_SIMPLE) || defined(ESP_PANEL_BOARD_INCLUDE_SUPPORTED_OUTSIDE)
            #define ESP_PANEL_BOARD_USE_SUPPORTED_FILE
        #endif
    #endif

    #if __has_include("esp_panel_board_custom_conf.h")
        #ifndef ESP_PANEL_BOARD_INCLUDE_CUSTOM_SIMPLE
            #define ESP_PANEL_BOARD_INCLUDE_CUSTOM_SIMPLE
        #endif
    #elif __has_include("../../../esp_panel_board_custom_conf.h")
        #ifndef ESP_PANEL_BOARD_INCLUDE_OUTSIDE_CUSTOM
            #define ESP_PANEL_BOARD_INCLUDE_OUTSIDE_CUSTOM
        #endif
    #endif
    #ifndef ESP_PANEL_BOARD_USE_CUSTOM_FILE
        #if defined(ESP_PANEL_BOARD_INCLUDE_CUSTOM_SIMPLE) || defined(ESP_PANEL_BOARD_INCLUDE_OUTSIDE_CUSTOM)
            #define ESP_PANEL_BOARD_USE_CUSTOM_FILE
        #endif
    #endif
#endif

// *INDENT-ON*
