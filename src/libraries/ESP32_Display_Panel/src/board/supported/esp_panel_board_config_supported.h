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

/* Check if using a supported board */
#ifdef ESP_PANEL_BOARD_USE_SUPPORTED_FILE
    #ifdef ESP_PANEL_BOARD_SUPPORTED_FILE_PATH
        #define __TO_STR_AUX(x) #x
        #define __TO_STR(x) __TO_STR_AUX(x)
        #include __TO_STR(ESP_PANEL_BOARD_SUPPORTED_FILE_PATH)
        #undef __TO_STR_AUX
        #undef __TO_STR
    #elif defined(ESP_PANEL_BOARD_INCLUDE_SUPPORTED_SIMPLE)
        #include "esp_panel_board_supported_conf.h"
    #elif defined(ESP_PANEL_BOARD_INCLUDE_SUPPORTED_OUTSIDE)
        #include "../../../../esp_panel_board_supported_conf.h"
    #endif
#endif

#ifndef ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED
    #ifdef CONFIG_ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED
        #define ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED CONFIG_ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED
    #else
        #define ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED 0
    #endif
#endif

#if defined(ESP_PANEL_BOARD_USE_SUPPORTED_FILE) && ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED
/**
 * Check if the current configuration file version is compatible with the library version
 */
    /* File `esp_panel_board_supported_conf.h` */
    // If the version is not defined, set it to `0.1.0`
    #if !defined(ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_MAJOR) && \
        !defined(ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_MINOR) && \
        !defined(ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_PATCH)
        #define ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_MAJOR 0
        #define ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_MINOR 1
        #define ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_PATCH 0
    #endif

    // Check if the current configuration file version is compatible with the library version
    #if ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_MAJOR != ESP_PANEL_BOARD_SUPPORTED_VERSION_MAJOR
        #error "The `esp_panel_board_supported_conf.h` file version is not compatible. Please update it with the file from the library"
    #elif ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_MINOR < ESP_PANEL_BOARD_SUPPORTED_VERSION_MINOR
        #warning "The `esp_panel_board_supported_conf.h` file version is outdated. Some new configurations are missing"
    #elif ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_MINOR > ESP_PANEL_BOARD_SUPPORTED_VERSION_MINOR
        #warning "The `esp_panel_board_supported_conf.h` file version is newer than the library. Some new configurations are not supported"
    #endif
#endif

#if ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED
    /* For using a supported board, include the supported board header file */
    #include "esp_panel_board_kconfig_supported.h"

    // Check if multiple boards are enabled
    #if \
        /* Espressif */ \
        defined(BOARD_ESPRESSIF_ESP32_C3_LCDKIT) \
        + defined(BOARD_ESPRESSIF_ESP32_S3_BOX) \
        + defined(BOARD_ESPRESSIF_ESP32_S3_BOX_3) \
        + defined(BOARD_ESPRESSIF_ESP32_S3_BOX_3_BETA) \
        + defined(BOARD_ESPRESSIF_ESP32_S3_BOX_LITE) \
        + defined(BOARD_ESPRESSIF_ESP32_S3_EYE) \
        + defined(BOARD_ESPRESSIF_ESP32_S3_KORVO_2) \
        + defined(BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD) \
        + defined(BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_V1_5) \
        + defined(BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2) \
        + defined(BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2_V1_5) \
        + defined(BOARD_ESPRESSIF_ESP32_S3_USB_OTG) \
        + defined(BOARD_ESPRESSIF_ESP32_P4_FUNCTION_EV_BOARD) \
        /* Elecrow */ \
        + defined(BOARD_ELECROW_CROWPANEL_7_0) \
        /* M5Stack */ \
        + defined(BOARD_M5STACK_M5CORE2) \
        + defined(BOARD_M5STACK_M5DIAL) \
        + defined(BOARD_M5STACK_M5CORES3) \
        /* JingCai */ \
        + defined(BOARD_JINGCAI_ESP32_4848S040C_I_Y_3) \
        + defined(BOARD_JINGCAI_JC8048W550C) \
        /* Waveshare */ \
        + defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_1_85) \
        + defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_2_1) \
        + defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_4_3) \
        + defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_4_3_B) \
        + defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_5) \
        + defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_5_B) \
        + defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_7) \
        + defined(BOARD_WAVESHARE_ESP32_P4_NANO) \
        /* Viewe */ \
        + defined(BOARD_VIEWE_UEDX24320024E_WB_A) \
        + defined(BOARD_VIEWE_UEDX24320028E_WB_A) \
        + defined(BOARD_VIEWE_UEDX24320035E_WB_A) \
        + defined(BOARD_VIEWE_UEDX32480035E_WB_A) \
        + defined(BOARD_VIEWE_UEDX48270043E_WB_A) \
        + defined(BOARD_VIEWE_UEDX48480040E_WB_A) \
        + defined(BOARD_VIEWE_UEDX80480043E_WB_A) \
        + defined(BOARD_VIEWE_UEDX80480050E_WB_A) \
        + defined(BOARD_VIEWE_UEDX80480050E_WB_A_2) \
        + defined(BOARD_VIEWE_UEDX80480070E_WB_A) \
        > 1
        #error "Multiple boards enabled! Please check file `esp_panel_board_supported_conf.h` and make sure only one board is enabled."
    #endif

    // Include board specific header file
    /* Espressif */
    #if defined(BOARD_ESPRESSIF_ESP32_C3_LCDKIT)
        #include "espressif/BOARD_ESPRESSIF_ESP32_C3_LCDKIT.h"
    #elif defined(BOARD_ESPRESSIF_ESP32_S3_BOX)
        #include "espressif/BOARD_ESPRESSIF_ESP32_S3_BOX.h"
    #elif defined(BOARD_ESPRESSIF_ESP32_S3_BOX_3)
        #include "espressif/BOARD_ESPRESSIF_ESP32_S3_BOX_3.h"
    #elif defined(BOARD_ESPRESSIF_ESP32_S3_BOX_3_BETA)
        #include "espressif/BOARD_ESPRESSIF_ESP32_S3_BOX_3_BETA.h"
    #elif defined(BOARD_ESPRESSIF_ESP32_S3_BOX_LITE)
        #include "espressif/BOARD_ESPRESSIF_ESP32_S3_BOX_LITE.h"
    #elif defined(BOARD_ESPRESSIF_ESP32_S3_EYE)
        #include "espressif/BOARD_ESPRESSIF_ESP32_S3_EYE.h"
    #elif defined(BOARD_ESPRESSIF_ESP32_S3_KORVO_2)
        #include "espressif/BOARD_ESPRESSIF_ESP32_S3_KORVO_2.h"
    #elif defined(BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD)
        #include "espressif/BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD.h"
    #elif defined(BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_V1_5)
        #include "espressif/BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_V1_5.h"
    #elif defined(BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2)
        #include "espressif/BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2.h"
    #elif defined(BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2_V1_5)
        #include "espressif/BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2_V1_5.h"
    #elif defined(BOARD_ESPRESSIF_ESP32_S3_USB_OTG)
        #include "espressif/BOARD_ESPRESSIF_ESP32_S3_USB_OTG.h"
    #elif defined(BOARD_ESPRESSIF_ESP32_P4_FUNCTION_EV_BOARD)
        #include "espressif/BOARD_ESPRESSIF_ESP32_P4_FUNCTION_EV_BOARD.h"
    /* Elecrow */
    #elif defined(BOARD_ELECROW_CROWPANEL_7_0)
        #include "elecrow/BOARD_ELECROW_CROWPANEL_7_0.h"
    /* M5Stack */
    #elif defined(BOARD_M5STACK_M5CORE2)
        #include "m5stack/BOARD_M5STACK_M5CORE2.h"
    #elif defined(BOARD_M5STACK_M5DIAL)
        #include "m5stack/BOARD_M5STACK_M5DIAL.h"
    #elif defined(BOARD_M5STACK_M5CORES3)
        #include "m5stack/BOARD_M5STACK_M5CORES3.h"
    /* Jingcai */
    #elif defined(BOARD_JINGCAI_ESP32_4848S040C_I_Y_3)
        #include "jingcai/BOARD_JINGCAI_ESP32_4848S040C_I_Y_3.h"
    #elif defined(BOARD_JINGCAI_JC8048W550C)
        #include "jingcai/BOARD_JINGCAI_JC8048W550C.h"
    /* Waveshare */
    #elif defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_1_85)
        #include "waveshare/BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_1_85.h"
    #elif defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_2_1)
        #include "waveshare/BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_2_1.h"
    #elif defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_4_3)
        #include "waveshare/BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_4_3.h"
    #elif defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_4_3_B)
        #include "waveshare/BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_4_3_B.h"
    #elif defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_5)
        #include "waveshare/BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_5.h"
    #elif defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_5_B)
        #include "waveshare/BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_5_B.h"
    #elif defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_7)
        #include "waveshare/BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_7.h"
    #elif defined(BOARD_WAVESHARE_ESP32_P4_NANO)
        #include "waveshare/BOARD_WAVESHARE_ESP32_P4_NANO.h"
    /* Viewe */
    #elif defined(BOARD_VIEWE_UEDX24320024E_WB_A)
        #include "viewe/BOARD_VIEWE_UEDX24320024E_WB_A.h"
    #elif defined(BOARD_VIEWE_UEDX24320028E_WB_A)
        #include "viewe/BOARD_VIEWE_UEDX24320028E_WB_A.h"
    #elif defined(BOARD_VIEWE_UEDX24320035E_WB_A)
        #include "viewe/BOARD_VIEWE_UEDX24320035E_WB_A.h"
    #elif defined(BOARD_VIEWE_UEDX32480035E_WB_A)
        #include "viewe/BOARD_VIEWE_UEDX32480035E_WB_A.h"
    #elif defined(BOARD_VIEWE_UEDX48270043E_WB_A)
        #include "viewe/BOARD_VIEWE_UEDX48270043E_WB_A.h"
    #elif defined(BOARD_VIEWE_UEDX48480040E_WB_A)
        #include "viewe/BOARD_VIEWE_UEDX48480040E_WB_A.h"
    #elif defined(BOARD_VIEWE_UEDX80480043E_WB_A)
        #include "viewe/BOARD_VIEWE_UEDX80480043E_WB_A.h"
    #elif defined(BOARD_VIEWE_UEDX80480050E_WB_A)
        #include "viewe/BOARD_VIEWE_UEDX80480050E_WB_A.h"
    #elif defined(BOARD_VIEWE_UEDX80480050E_WB_A_2)
        #include "viewe/BOARD_VIEWE_UEDX80480050E_WB_A_2.h"
    #elif defined(BOARD_VIEWE_UEDX80480070E_WB_A)
        #include "viewe/BOARD_VIEWE_UEDX80480070E_WB_A.h"
    #else
        #error "Unknown board selected!"
    #endif

    /**
     * Check if the internal board configuration file version is compatible with the library version
     */
    // If the version is not defined, set it to `0.1.0`
    #if !defined(ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MAJOR)
        #define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MAJOR 0
    #endif
    // Only check the major version
    #if ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MAJOR != ESP_PANEL_BOARD_CUSTOM_VERSION_MAJOR
        #error "The internal board configuration file version is not compatible. Please update it with the file from the library"
    #endif
#endif // ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED

// *INDENT-ON*
