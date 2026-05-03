/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file  esp_panel_board_supported_conf.h
 * @brief Configuration file for supported ESP development boards
 *
 * This file contains configuration options for various supported development boards using ESP Panel.
 * Users can select their specific board by uncommenting the corresponding macro definition.
 */

#pragma once

/**
 * @brief Flag to enable supported board configuration (0/1)
 *
 * Set to `1` to enable supported board configuration, `0` to disable
 */
#define ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED       (0)

#if ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED
/**
 * @brief Board selection macros
 *
 * Uncomment one of the following macros to select a supported development board. Multiple macros enabled
 * simultaneously will trigger a compilation error.
 */

/*
 * Espressif (https://www.espressif.com/en/products/devkits):
 *
 *  -BOARD_ESPRESSIF_ESP32_C3_LCDKIT (ESP32-C3-LCDkit): https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c3/esp32-c3-lcdkit/index.html
 *  -BOARD_ESPRESSIF_ESP32_S3_BOX (ESP32-S3-Box): https://github.com/espressif/esp-box/tree/master
 *  -BOARD_ESPRESSIF_ESP32_S3_BOX_3 (ESP32-S3-Box-3 & ESP32-S3-Box-3B): https://github.com/espressif/esp-box/tree/master
 *  -BOARD_ESPRESSIF_ESP32_S3_BOX_3_BETA (ESP32-S3-Box-3(beta)): https://github.com/espressif/esp-box/tree/c4c954888e11250423f083df0067d99e22d59fbe
 *  -BOARD_ESPRESSIF_ESP32_S3_BOX_LITE (ESP32-S3-Box-Lite): https://github.com/espressif/esp-box/tree/master
 *  -BOARD_ESPRESSIF_ESP32_S3_EYE (ESP32-S3-EYE): https://github.com/espressif/esp-who/blob/master/docs/en/get-started/ESP32-S3-EYE_Getting_Started_Guide.md
 *  -BOARD_ESPRESSIF_ESP32_S3_KORVO_2 (ESP32-S3-Korvo-2): https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/user-guide-esp32-s3-korvo-2.html
 *  -BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD (ESP32-S3-LCD-EV-Board(v1.1-v1.4)): https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-lcd-ev-board/user_guide_v1.4.html
 *  -BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_V1_5 (ESP32-S3-LCD-EV-Board(v1.5)): https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-lcd-ev-board/user_guide.html
 *  -BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2 (ESP32-S3-LCD-EV-Board-2(v1.1-v1.4))): https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-lcd-ev-board/user_guide_v1.4.html
 *  -BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2_V1_5 (ESP32-S3-LCD-EV-Board-2(v1.5)): https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-lcd-ev-board/user_guide.html
 *  -BOARD_ESPRESSIF_ESP32_S3_USB_OTG (ESP32-S3-USB-OTG): https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-usb-otg/index.html
 *  -BOARD_ESPRESSIF_ESP32_P4_FUNCTION_EV_BOARD (ESP32-P4-Function-EV-Board): https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/index.html
 */
// #define BOARD_ESPRESSIF_ESP32_C3_LCDKIT
// #define BOARD_ESPRESSIF_ESP32_S3_BOX
// #define BOARD_ESPRESSIF_ESP32_S3_BOX_3
// #define BOARD_ESPRESSIF_ESP32_S3_BOX_3_BETA
// #define BOARD_ESPRESSIF_ESP32_S3_BOX_LITE
// #define BOARD_ESPRESSIF_ESP32_S3_EYE
// #define BOARD_ESPRESSIF_ESP32_S3_KORVO_2
// #define BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD
// #define BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_V1_5
// #define BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2
// #define BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2_V1_5
// #define BOARD_ESPRESSIF_ESP32_S3_USB_OTG
// #define BOARD_ESPRESSIF_ESP32_P4_FUNCTION_EV_BOARD

/*
 * Elecrow (https://www.elecrow.com):
 *
 *  -BOARD_ELECROW_CROWPANEL_7_0 (ELECROW_CROWPANEL_7_0): https://www.elecrow.com/esp32-display-7-inch-hmi-display-rgb-tft-lcd-touch-screen-support-lvgl.html
 */
// #define BOARD_ELECROW_CROWPANEL_7_0

/*
 * M5Stack (https://m5stack.com/):
 *
 *  -BOARD_M5STACK_M5CORE2 (M5STACK_M5CORE2): https://docs.m5stack.com/en/core/core2
 *  -BOARD_M5STACK_M5DIAL (M5STACK_M5DIAL): https://docs.m5stack.com/en/core/M5Dial
 *  -BOARD_M5STACK_M5CORES3 (M5STACK_M5CORES3): https://docs.m5stack.com/en/core/CoreS3
 */
// #define BOARD_M5STACK_M5CORE2
// #define BOARD_M5STACK_M5DIAL
// #define BOARD_M5STACK_M5CORES3

/*
 * Shenzhen Jingcai Intelligent Supported Boards (https://www.displaysmodule.com/):
 *
 *  -BOARD_JINGCAI_ESP32_4848S040C_I_Y_3 (ESP32-4848S040C_I_Y_3):
 *      - https://www.displaysmodule.com/sale-41828962-experience-the-power-of-the-esp32-display-module-sku-esp32-4848s040c-i-y-3.html
 *      - http://pan.jczn1688.com/directlink/1/ESP32%20module/4.0inch_ESP32-4848S040.zip
 *  -BOARD_JINGCAI_JC8048W550C (JC8048W550C):
 *      - https://www.displaysmodule.com/sale-43987867-capacitive-touch-advanced-tft-hmi-display-module-jc8048w550-800-480-pixel-resolution-st7262-driver-c.html
 */
// #define BOARD_JINGCAI_ESP32_4848S040C_I_Y_3
// #define BOARD_JINGCAI_JC8048W550C

/*
 * Waveshare Supported Boards (https://www.waveshare.com/):
 *
 *  -BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_1_85 (ESP32_S3_Touch_LCD_1_85): https://www.waveshare.com/esp32-s3-touch-lcd-1.85.htm
 *  -BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_2_1 (ESP32_S3_Touch_LCD_2_1): https://www.waveshare.com/esp32-s3-touch-lcd-2.1.htm
 *  -BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_4_3 (ESP32_S3_Touch_LCD_4_3): https://www.waveshare.com/esp32-s3-touch-lcd-4.3.htm
 *  -BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_4_3_B (ESP32_S3_Touch_LCD_4_3_B): https://www.waveshare.com/esp32-s3-touch-lcd-4.3B.htm
 *  -BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_5 (ESP32_S3_Touch_LCD_5): https://www.waveshare.com/esp32-s3-touch-lcd-5.htm?sku=28117
 *  -BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_5_B (ESP32_S3_Touch_LCD_5_B): https://www.waveshare.com/esp32-s3-touch-lcd-5.htm?sku=28151
 *  -BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_7 (ESP32_S3_Touch_LCD_7): https://www.waveshare.com/esp32-s3-touch-lcd-7.htm
 *  -BOARD_WAVESHARE_ESP32_P4_NANO (ESP32_P4_NANO): https://www.waveshare.com/esp32-p4-nano.htm
 */
// #define BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_1_85
// #define BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_2_1
// #define BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_4_3
// #define BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_4_3_B
// #define BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_5
// #define BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_5_B
// #define BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_7
// #define BOARD_WAVESHARE_ESP32_P4_NANO

/*
 * VIEWE Supported Boards (https://viewedisplay.com/):
 *
 *  VIEWE Model Number Format (Take `UEDX24320024E` as an example):
 *  +--------+----+----+----+----+--------+
 *  |  UEDX  | 24 | 32 | 00 | 24 | E-WB-A |
 *  +--------+----+----+----+----+--------+
 *              |    |         |
 *              |    |         +---- Display size: 2.4 inch
 *              |    +-------------- Vertical resolution: 320
 *              +------------------- Horizontal resolution: 240
 *  So UEDX24320024E means: 240x320 resolution & 2.4 inch display
 *
 *  - BOARD_VIEWE_UEDX24320024E_WB_A (UEDX24320024E-WB-A): https://viewedisplay.com/product/esp32-2-4-inch-240x320-rgb-ips-tft-display-touch-screen-arduino-lvgl-wifi-ble-uart-smart-module/
 *  - BOARD_VIEWE_UEDX24320028E_WB_A (UEDX24320028E-WB-A): https://viewedisplay.com/product/esp32-2-8-inch-240x320-mcu-ips-tft-display-touch-screen-arduino-lvgl-wifi-ble-uart-smart-module/
 *  - BOARD_VIEWE_UEDX24320035E_WB_A (UEDX24320035E-WB-A): https://viewedisplay.com/product/esp32-3-5-inch-240x320-mcu-ips-tft-display-touch-screen-arduino-lvgl-wifi-ble-uart-smart-module/
 *  - BOARD_VIEWE_UEDX32480035E_WB_A (UEDX32480035E-WB-A): https://github.com/VIEWESMART/Product-Specification-and-Schematic/blob/main/ESP32/3.5inch/320480/UEDX32480035E-WB-A%20SPEC.pdf
 *  - BOARD_VIEWE_UEDX48270043E_WB_A (UEDX48270043E-WB-A): https://github.com/VIEWESMART/Product-Specification-and-Schematic/blob/main/ESP32/4.3inch/Low-Resolution_480272/UEDX48270043E-WB-A%20SPEC.pdf
 *  - BOARD_VIEWE_UEDX48480040E_WB_A (UEDX48480040E-WB-A): https://viewedisplay.com/product/esp32-4-inch-tft-display-touch-screen-arduino-lvgl/
 *  - BOARD_VIEWE_UEDX80480043E_WB_A (UEDX80480043E-WB-A): https://viewedisplay.com/product/esp32-4-3-inch-800x480-rgb-ips-tft-display-touch-screen-arduino-lvgl/
 *  - BOARD_VIEWE_UEDX80480050E_WB_A (UEDX80480050E-WB-A): https://viewedisplay.com/product/esp32-5-inch-800x480-rgb-ips-tft-display-touch-screen-arduino-lvgl/
 *  - BOARD_VIEWE_UEDX80480050E_WB_A_2 (UEDX80480050E-WB-A): https://viewedisplay.com/product/esp32-5-inch-800x480-rgb-ips-tft-display-touch-screen-arduino-lvgl/
 *  - BOARD_VIEWE_UEDX80480070E_WB_A (UEDX80480070E-WB-A): https://viewedisplay.com/product/esp32-7-inch-800x480-rgb-ips-tft-display-touch-screen-arduino-lvgl-uart/
 */
// #define BOARD_VIEWE_UEDX24320024E_WB_A
// #define BOARD_VIEWE_UEDX24320028E_WB_A
// #define BOARD_VIEWE_UEDX24320035E_WB_A
// #define BOARD_VIEWE_UEDX32480035E_WB_A
// #define BOARD_VIEWE_UEDX48270043E_WB_A
// #define BOARD_VIEWE_UEDX48480040E_WB_A
// #define BOARD_VIEWE_UEDX80480043E_WB_A
// #define BOARD_VIEWE_UEDX80480050E_WB_A
// #define BOARD_VIEWE_UEDX80480050E_WB_A_2
// #define BOARD_VIEWE_UEDX80480070E_WB_A

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// File Version ///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Do not change the following versions. These version numbers are used to check compatibility between this
 * configuration file and the library. Rules for version numbers:
 * 1. Major version mismatch: Configurations are incompatible, must use library version
 * 2. Minor version mismatch: May be missing new configurations, recommended to update
 * 3. Patch version mismatch: No impact on functionality
 */
#define ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_MAJOR 1
#define ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_MINOR 0
#define ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_PATCH 0

#endif
