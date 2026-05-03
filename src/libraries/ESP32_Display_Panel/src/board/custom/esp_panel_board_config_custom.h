/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @note This file shouldn't be included in the public header file.
 */

#pragma once

// *INDENT-OFF*

#include "board/esp_panel_board_conf_internal.h"

/* Check if using a custom board */
#ifdef ESP_PANEL_BOARD_USE_CUSTOM_FILE
    #ifdef ESP_PANEL_BOARD_CUSTOM_FILE_PATH
        #define __TO_STR_AUX(x) #x
        #define __TO_STR(x) __TO_STR_AUX(x)
        #include __TO_STR(ESP_PANEL_BOARD_CUSTOM_FILE_PATH)
        #undef __TO_STR_AUX
        #undef __TO_STR
    #elif defined(ESP_PANEL_BOARD_INCLUDE_CUSTOM_SIMPLE)
        #include "esp_panel_board_custom_conf.h"
    #elif defined(ESP_PANEL_BOARD_INCLUDE_OUTSIDE_CUSTOM)
        #include "../../../../esp_panel_board_custom_conf.h"
    #endif
#endif

#ifndef ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM
    #ifdef CONFIG_ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM
        #define ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM CONFIG_ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM
    #else
        #define ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM 0
    #endif
#endif

#if defined(ESP_PANEL_BOARD_USE_CUSTOM_FILE) && ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM
/**
 * Check if the current configuration file version is compatible with the library version
 */
    /* File `esp_panel_board_custom_conf.h` */
    // If the version is not defined, set it to `0.1.0`
    #if !defined(ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MAJOR) && \
        !defined(ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MINOR) && \
        !defined(ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_PATCH)
        #define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MAJOR 0
        #define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MINOR 1
        #define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_PATCH 0
    #endif

    #if ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MAJOR != ESP_PANEL_BOARD_CUSTOM_VERSION_MAJOR
        #error "The `esp_panel_board_custom_conf.h` file version is not compatible. Please update it with the file from the library"
    #elif ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MINOR < ESP_PANEL_BOARD_CUSTOM_VERSION_MINOR
        #warning "The `esp_panel_board_custom_conf.h` file version is outdated. Some new configurations are missing"
    #elif ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MINOR > ESP_PANEL_BOARD_CUSTOM_VERSION_MINOR
        #warning "The `esp_panel_board_custom_conf.h` file version is newer than the library. Some new configurations are not supported"
    #endif
#endif
// *INDENT-ON*
