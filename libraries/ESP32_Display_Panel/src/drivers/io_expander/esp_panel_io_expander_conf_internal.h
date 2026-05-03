/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_panel_conf_internal.h"

// *INDENT-OFF*

#include "drivers/esp_panel_drivers_conf_internal.h"

#ifndef ESP_PANEL_DRIVERS_INCLUDE_INSIDE
    /*
    * Define the driver configuration
    */
    #ifndef ESP_PANEL_DRIVERS_EXPANDER_USE_ALL
        #ifdef CONFIG_ESP_PANEL_DRIVERS_EXPANDER_USE_ALL
            #define ESP_PANEL_DRIVERS_EXPANDER_USE_ALL CONFIG_ESP_PANEL_DRIVERS_EXPANDER_USE_ALL
        #else
            #define ESP_PANEL_DRIVERS_EXPANDER_USE_ALL (0)
        #endif
    #endif

    #if ESP_PANEL_DRIVERS_EXPANDER_USE_ALL
        #define ESP_PANEL_DRIVERS_EXPANDER_USE_CH422G (1)
        #define ESP_PANEL_DRIVERS_EXPANDER_USE_HT8574 (1)
        #define ESP_PANEL_DRIVERS_EXPANDER_USE_TCA95XX_8BIT (1)
        #define ESP_PANEL_DRIVERS_EXPANDER_USE_TCA95XX_16BIT (1)
    #else
        #ifndef ESP_PANEL_DRIVERS_EXPANDER_USE_CH422G
            #ifdef CONFIG_ESP_PANEL_DRIVERS_EXPANDER_USE_CH422G
                #define ESP_PANEL_DRIVERS_EXPANDER_USE_CH422G CONFIG_ESP_PANEL_DRIVERS_EXPANDER_USE_CH422G
            #else
                #define ESP_PANEL_DRIVERS_EXPANDER_USE_CH422G (0)
            #endif
        #endif

        #ifndef ESP_PANEL_DRIVERS_EXPANDER_USE_HT8574
            #ifdef CONFIG_ESP_PANEL_DRIVERS_EXPANDER_USE_HT8574
                #define ESP_PANEL_DRIVERS_EXPANDER_USE_HT8574 CONFIG_ESP_PANEL_DRIVERS_EXPANDER_USE_HT8574
            #else
                #define ESP_PANEL_DRIVERS_EXPANDER_USE_HT8574 (0)
            #endif
        #endif

        #ifndef ESP_PANEL_DRIVERS_EXPANDER_USE_TCA95XX_8BIT
            #ifdef CONFIG_ESP_PANEL_DRIVERS_EXPANDER_USE_TCA95XX_8BIT
                #define ESP_PANEL_DRIVERS_EXPANDER_USE_TCA95XX_8BIT CONFIG_ESP_PANEL_DRIVERS_EXPANDER_USE_TCA95XX_8BIT
            #else
                #define ESP_PANEL_DRIVERS_EXPANDER_USE_TCA95XX_8BIT (0)
            #endif
        #endif

        #ifndef ESP_PANEL_DRIVERS_EXPANDER_USE_TCA95XX_16BIT
            #ifdef CONFIG_ESP_PANEL_DRIVERS_EXPANDER_USE_TCA95XX_16BIT
                #define ESP_PANEL_DRIVERS_EXPANDER_USE_TCA95XX_16BIT CONFIG_ESP_PANEL_DRIVERS_EXPANDER_USE_TCA95XX_16BIT
            #else
                #define ESP_PANEL_DRIVERS_EXPANDER_USE_TCA95XX_16BIT (0)
            #endif
        #endif
    #endif // ESP_PANEL_DRIVERS_EXPANDER_USE_ALL
#endif

/*
 * Enable the driver if it is used or if the compile unused drivers is enabled
 */
#ifndef ESP_PANEL_DRIVERS_EXPANDER_ENABLE_CH422G
    #if ESP_PANEL_DRIVERS_EXPANDER_USE_CH422G
        #define ESP_PANEL_DRIVERS_EXPANDER_ENABLE_CH422G  (1)
    #else
        #define ESP_PANEL_DRIVERS_EXPANDER_ENABLE_CH422G  (0)
    #endif
#endif

#ifndef ESP_PANEL_DRIVERS_EXPANDER_ENABLE_HT8574
    #if ESP_PANEL_DRIVERS_EXPANDER_USE_HT8574
        #define ESP_PANEL_DRIVERS_EXPANDER_ENABLE_HT8574  (1)
    #else
        #define ESP_PANEL_DRIVERS_EXPANDER_ENABLE_HT8574  (0)
    #endif
#endif

#ifndef ESP_PANEL_DRIVERS_EXPANDER_ENABLE_TCA95XX_8BIT
    #if ESP_PANEL_DRIVERS_EXPANDER_USE_TCA95XX_8BIT
        #define ESP_PANEL_DRIVERS_EXPANDER_ENABLE_TCA95XX_8BIT  (1)
    #else
        #define ESP_PANEL_DRIVERS_EXPANDER_ENABLE_TCA95XX_8BIT  (0)
    #endif
#endif

// *INDENT-ON*
