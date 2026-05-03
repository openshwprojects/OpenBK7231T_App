/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "drivers/esp_panel_drivers_conf_internal.h"

// *INDENT-OFF*

#ifndef ESP_PANEL_DRIVERS_INCLUDE_INSIDE
    /*
    * Define the driver configuration
    */
    #ifndef ESP_PANEL_DRIVERS_BUS_USE_ALL
        #ifdef CONFIG_ESP_PANEL_DRIVERS_BUS_USE_ALL
            #define ESP_PANEL_DRIVERS_BUS_USE_ALL CONFIG_ESP_PANEL_DRIVERS_BUS_USE_ALL
        #else
            #define ESP_PANEL_DRIVERS_BUS_USE_ALL (0)
        #endif
    #endif

    #if ESP_PANEL_DRIVERS_BUS_USE_ALL
        #define ESP_PANEL_DRIVERS_BUS_USE_SPI      (1)
        #define ESP_PANEL_DRIVERS_BUS_USE_QSPI     (1)
        #define ESP_PANEL_DRIVERS_BUS_USE_RGB      (1)
        #define ESP_PANEL_DRIVERS_BUS_USE_I2C      (1)
        #define ESP_PANEL_DRIVERS_BUS_USE_MIPI_DSI (1)
    #else
        #ifndef ESP_PANEL_DRIVERS_BUS_USE_SPI
            #ifdef CONFIG_ESP_PANEL_DRIVERS_BUS_USE_SPI
                #define ESP_PANEL_DRIVERS_BUS_USE_SPI CONFIG_ESP_PANEL_DRIVERS_BUS_USE_SPI
            #else
                #define ESP_PANEL_DRIVERS_BUS_USE_SPI (0)
            #endif
        #endif

        #ifndef ESP_PANEL_DRIVERS_BUS_USE_QSPI
            #ifdef CONFIG_ESP_PANEL_DRIVERS_BUS_USE_QSPI
                #define ESP_PANEL_DRIVERS_BUS_USE_QSPI CONFIG_ESP_PANEL_DRIVERS_BUS_USE_QSPI
            #else
                #define ESP_PANEL_DRIVERS_BUS_USE_QSPI (0)
            #endif
        #endif

        #if SOC_LCD_RGB_SUPPORTED
            #ifndef ESP_PANEL_DRIVERS_BUS_USE_RGB
                #ifdef CONFIG_ESP_PANEL_DRIVERS_BUS_USE_RGB
                    #define ESP_PANEL_DRIVERS_BUS_USE_RGB CONFIG_ESP_PANEL_DRIVERS_BUS_USE_RGB
                #else
                    #define ESP_PANEL_DRIVERS_BUS_USE_RGB (0)
                #endif
            #endif
        #else
            #undef ESP_PANEL_DRIVERS_BUS_USE_RGB
            #define ESP_PANEL_DRIVERS_BUS_USE_RGB (0)
        #endif

        #ifndef ESP_PANEL_DRIVERS_BUS_USE_I2C
            #ifdef CONFIG_ESP_PANEL_DRIVERS_BUS_USE_I2C
                #define ESP_PANEL_DRIVERS_BUS_USE_I2C CONFIG_ESP_PANEL_DRIVERS_BUS_USE_I2C
            #else
                #define ESP_PANEL_DRIVERS_BUS_USE_I2C (0)
            #endif
        #endif

        #if SOC_MIPI_DSI_SUPPORTED
            #ifndef ESP_PANEL_DRIVERS_BUS_USE_MIPI_DSI
                #ifdef CONFIG_ESP_PANEL_DRIVERS_BUS_USE_MIPI_DSI
                    #define ESP_PANEL_DRIVERS_BUS_USE_MIPI_DSI CONFIG_ESP_PANEL_DRIVERS_BUS_USE_MIPI_DSI
                #else
                    #define ESP_PANEL_DRIVERS_BUS_USE_MIPI_DSI (0)
                #endif
            #endif
        #else
            #undef ESP_PANEL_DRIVERS_BUS_USE_MIPI_DSI
            #define ESP_PANEL_DRIVERS_BUS_USE_MIPI_DSI (0)
        #endif
    #endif // ESP_PANEL_DRIVERS_BUS_USE_ALL

    #ifndef ESP_PANEL_DRIVERS_BUS_COMPILE_UNUSED_DRIVERS
        #ifdef CONFIG_ESP_PANEL_DRIVERS_BUS_COMPILE_UNUSED_DRIVERS
            #define ESP_PANEL_DRIVERS_BUS_COMPILE_UNUSED_DRIVERS CONFIG_ESP_PANEL_DRIVERS_BUS_COMPILE_UNUSED_DRIVERS
        #else
            #define ESP_PANEL_DRIVERS_BUS_COMPILE_UNUSED_DRIVERS (0)
        #endif
    #endif
#endif // ESP_PANEL_DRIVERS_INCLUDE_INSIDE

/*
 * Enable the driver if it is used or if the compile unused drivers is enabled
 */
#ifndef ESP_PANEL_DRIVERS_BUS_ENABLE_SPI
    #if ESP_PANEL_DRIVERS_BUS_COMPILE_UNUSED_DRIVERS || ESP_PANEL_DRIVERS_BUS_USE_SPI
        #define ESP_PANEL_DRIVERS_BUS_ENABLE_SPI  (1)
    #else
        #define ESP_PANEL_DRIVERS_BUS_ENABLE_SPI  (0)
    #endif
#endif

#ifndef ESP_PANEL_DRIVERS_BUS_ENABLE_QSPI
    #if ESP_PANEL_DRIVERS_BUS_COMPILE_UNUSED_DRIVERS || ESP_PANEL_DRIVERS_BUS_USE_QSPI
        #define ESP_PANEL_DRIVERS_BUS_ENABLE_QSPI  (1)
    #else
        #define ESP_PANEL_DRIVERS_BUS_ENABLE_QSPI  (0)
    #endif
#endif

#if SOC_LCD_RGB_SUPPORTED
    #ifndef ESP_PANEL_DRIVERS_BUS_ENABLE_RGB
        #if ESP_PANEL_DRIVERS_BUS_COMPILE_UNUSED_DRIVERS || ESP_PANEL_DRIVERS_BUS_USE_RGB
            #define ESP_PANEL_DRIVERS_BUS_ENABLE_RGB  (1)
        #else
            #define ESP_PANEL_DRIVERS_BUS_ENABLE_RGB  (0)
        #endif
    #endif
#else
    #undef ESP_PANEL_DRIVERS_BUS_ENABLE_RGB
    #define ESP_PANEL_DRIVERS_BUS_ENABLE_RGB  (0)
    #undef ESP_PANEL_DRIVERS_BUS_USE_RGB
    #define ESP_PANEL_DRIVERS_BUS_USE_RGB  (0)
#endif

#ifndef ESP_PANEL_DRIVERS_BUS_ENABLE_I2C
    #if ESP_PANEL_DRIVERS_BUS_COMPILE_UNUSED_DRIVERS || ESP_PANEL_DRIVERS_BUS_USE_I2C
        #define ESP_PANEL_DRIVERS_BUS_ENABLE_I2C  (1)
    #else
        #define ESP_PANEL_DRIVERS_BUS_ENABLE_I2C  (0)
    #endif
#endif

#if SOC_MIPI_DSI_SUPPORTED
    #ifndef ESP_PANEL_DRIVERS_BUS_ENABLE_MIPI_DSI
        #if ESP_PANEL_DRIVERS_BUS_COMPILE_UNUSED_DRIVERS || ESP_PANEL_DRIVERS_BUS_USE_MIPI_DSI
            #define ESP_PANEL_DRIVERS_BUS_ENABLE_MIPI_DSI  (1)
        #else
            #define ESP_PANEL_DRIVERS_BUS_ENABLE_MIPI_DSI  (0)
        #endif
    #endif
#else
    #undef ESP_PANEL_DRIVERS_BUS_ENABLE_MIPI_DSI
    #define ESP_PANEL_DRIVERS_BUS_ENABLE_MIPI_DSI  (0)
    #undef ESP_PANEL_DRIVERS_BUS_USE_MIPI_DSI
    #define ESP_PANEL_DRIVERS_BUS_USE_MIPI_DSI  (0)
#endif

// *INDENT-ON*
