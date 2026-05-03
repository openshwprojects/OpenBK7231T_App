/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

// *INDENT-OFF*

#include "esp_panel_conf_internal.h"

#ifndef ESP_PANEL_DRIVERS_FILE_SKIP
    /* Try to locate the configuration file in different paths */
    #if __has_include("esp_panel_drivers_conf.h")
        #define ESP_PANEL_DRIVERS_INCLUDE_SIMPLE
    #elif __has_include("../../../esp_panel_drivers_conf.h")
        #define ESP_PANEL_DRIVERS_INCLUDE_OUTSIDE
    #else
        #define ESP_PANEL_DRIVERS_INCLUDE_INSIDE
    #endif

    /* Include the configuration file based on the path found */
    #ifdef ESP_PANEL_DRIVERS_PATH
        /* Use the custom path if defined */
        #define __TO_STR_AUX(x) #x
        #define __TO_STR(x) __TO_STR_AUX(x)
        #include __TO_STR(ESP_PANEL_DRIVERS_PATH)
        #undef __TO_STR_AUX
        #undef __TO_STR
    #elif defined(ESP_PANEL_DRIVERS_INCLUDE_SIMPLE)
        #include "esp_panel_drivers_conf.h"
    #elif defined(ESP_PANEL_DRIVERS_INCLUDE_OUTSIDE)
        #include "../../../esp_panel_drivers_conf.h"
    #elif defined(ESP_PANEL_DRIVERS_INCLUDE_INSIDE)
        #include "../esp_panel_drivers_conf.h"
    #endif

    #if defined(ESP_PANEL_DRIVERS_CONF_FILE_VERSION_MAJOR) && \
        !defined(ESP_PANEL_DRIVERS_CONF_FILE_VERSION_MINOR) && \
        !defined(ESP_PANEL_DRIVERS_CONF_FILE_VERSION_PATCH)
        /* Set default version if not defined */
        #define ESP_PANEL_DRIVERS_CONF_FILE_VERSION_MAJOR 0
        #define ESP_PANEL_DRIVERS_CONF_FILE_VERSION_MINOR 1
        #define ESP_PANEL_DRIVERS_CONF_FILE_VERSION_PATCH 0
    #endif

    /* Check version compatibility */
    #if ESP_PANEL_DRIVERS_CONF_FILE_VERSION_MAJOR != ESP_PANEL_DRIVERS_CONF_VERSION_MAJOR
        #error "The file `esp_panel_drivers_conf.h` version is not compatible. Please update it with the file from the library"
    #elif ESP_PANEL_DRIVERS_CONF_FILE_VERSION_MINOR < ESP_PANEL_DRIVERS_CONF_VERSION_MINOR
        #warning "The file `esp_panel_drivers_conf.h` version is outdated. Some new configurations are missing"
    #elif ESP_PANEL_DRIVERS_CONF_FILE_VERSION_MINOR > ESP_PANEL_DRIVERS_CONF_VERSION_MINOR
        #warning "The file `esp_panel_drivers_conf.h` version is newer than the library. Some new configurations are not supported"
    #endif
#endif // ESP_PANEL_DRIVERS_FILE_SKIP

// *INDENT-ON*
