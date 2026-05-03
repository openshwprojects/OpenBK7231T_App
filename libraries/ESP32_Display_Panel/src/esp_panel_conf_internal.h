/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/* Include type definitions before including configuration header file */
#include "esp_panel_types.h"
#include "esp_panel_versions.h"

#include "soc/soc_caps.h"

// *INDENT-OFF*

/* Handle special Kconfig options */
#ifndef ESP_PANEL_KCONFIG_IGNORE
    #include "sdkconfig.h"

    #ifndef ESP_PANEL_DRIVERS_FILE_SKIP
        #ifdef CONFIG_ESP_PANEL_DRIVERS_FILE_SKIP
            #define ESP_PANEL_DRIVERS_FILE_SKIP
        #endif
    #endif

    #ifndef ESP_PANEL_BOARD_FILE_SKIP
        #ifdef CONFIG_ESP_PANEL_BOARD_FILE_SKIP
            #define ESP_PANEL_BOARD_FILE_SKIP
        #endif
    #endif
#endif

// *INDENT-ON*
