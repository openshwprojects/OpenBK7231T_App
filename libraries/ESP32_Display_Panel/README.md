[![Arduino Lint](https://github.com/esp-arduino-libs/ESP32_Display_Panel/actions/workflows/arduino_lint.yml/badge.svg)](https://github.com/esp-arduino-libs/ESP32_Display_Panel/actions/workflows/arduino_lint.yml) [![Version Consistency](https://github.com/esp-arduino-libs/ESP32_Display_Panel/actions/workflows/check_lib_versions.yml/badge.svg)](https://github.com/esp-arduino-libs/ESP32_Display_Panel/actions/workflows/check_lib_versions.yml)

**Latest Arduino Library Version**: [![GitHub Release](https://img.shields.io/github/v/release/esp-arduino-libs/ESP32_Display_Panel)](https://github.com/esp-arduino-libs/ESP32_Display_Panel/releases)

**Latest Espressif Component Version**: [![Espressif Release](https://components.espressif.com/components/espressif/esp32_display_panel/badge.svg)](https://components.espressif.com/components/espressif/esp32_display_panel)

# ESP Display Panel

* [中文版](./README_CN.md)

## Overview

ESP32_Display_Panel is a **display driver** and **GUI porting** library designed by Espressif specifically for ESP series SoCs (ESP32, ESP32-S3, ESP32-P4, etc.). It supports multiple development frameworks including [ESP-IDF](https://github.com/espressif/esp-idf), [Arduino](https://github.com/espressif/arduino-esp32), and [MicroPython](https://github.com/micropython/micropython).

This library integrates most of Espressif's officially adapted [display-related components](https://components.espressif.com/components?q=esp_lcd), which can be used to drive displays (touch screens) with different interfaces and models. Additionally, the library provides common display features such as `backlight control` and `IO expander`, and integrates these features with `display` and `touch` functionality to form a complete display board driver solution. Developers can perform one-stop GUI application development based on [supported boards](#supported-boards) or `custom boards`.

ESP32_Display_Panel's main features include:

- Support for various display-related drivers, including `interface bus`, `LCD`, `touch`, `backlight`, and `IO expander`
- Support for multiple Espressif official and third-party display boards, including `M5Stack`, `Elecrow`, `Waveshare`, `VIEWE`, etc.
- Support for custom board configuration
- Support for flexible driver configuration and parameters
- Support for `ESP-IDF`, `Arduino`, and `MicroPython` development frameworks

The functional block diagram is shown below:

<div align="center"><img src="docs/_static/block_diagram.png" alt ="Block Diagram" width="600"></div>

## Table of Contents

- [ESP Display Panel](#esp-display-panel)
  - [Overview](#overview)
  - [Table of Contents](#table-of-contents)
  - [How to Use](#how-to-use)
  - [Supported Boards](#supported-boards)
  - [Supported Controllers](#supported-controllers)
    - [LCD Controllers](#lcd-controllers)
    - [Touch Controllers](#touch-controllers)
  - [FAQ](#faq)

## How to Use

📖 Here are the usage guides for ESP32_Display_Panel in different development environments:

* [ESP-IDF](./docs/envs/use_with_idf.md)
* [Arduino IDE](./docs/envs/use_with_arduino.md)
* [PlatformIO](./examples/platformio/lvgl_v8_port/README.md)

## Supported Boards

📋 Here is the list of boards supported by ESP32_Display_Panel:

| **Manufacturer** | **Model** |
| --------------- | --------- |
| [Espressif](./docs/board/board_espressif.md) | ESP32-C3-LCDkit, ESP32-S3-BOX, ESP32-S3-BOX-3, ESP32-S3-BOX-3B, ESP32-S3-BOX-3(beta), ESP32-S3-BOX-Lite, ESP32-S3-EYE, ESP32-S3-Korvo-2, ESP32-S3-LCD-EV-Board, ESP32-S3-LCD-EV-Board-2, ESP32-S3-USB-OTG, ESP32-P4-Function-EV-Board |
| [M5Stack](./docs/board/board_m5stack.md) | M5STACK-M5CORE2, M5STACK-M5DIAL, M5STACK-M5CORES3 |
| [Elecrow](./docs/board/board_elecrow.md) | CrowPanel 7.0" |
| [Jingcai](./docs/board/board_jingcai.md) | ESP32-4848S040C_I_Y_3 |
| [Waveshare](./docs/board/board_waveshare.md) | ESP32-S3-Touch-LCD-1.85, ESP32-S3-Touch-LCD-2.1, ESP32-S3-Touch-LCD-4.3, ESP32-S3-Touch-LCD-4.3B, ESP32-S3-Touch-LCD-5, ESP32-S3-Touch-LCD-5B, ESP32-S3-Touch-LCD-7, ESP32-P4-NANO |
| [VIEWE](./docs/board/board_viewe.md) | UEDX24320024E-WB-A, UEDX24320028E-WB-A, UEDX24320035E-WB-A, UEDX32480035E-WB-A, UEDX48270043E-WB-A, UEDX48480040E-WB-A, UEDX80480043E-WB-A, UEDX80480050E-WB-A, UEDX80480070E-WB-A |

📌 Click on the manufacturer name for detailed information.

💡 Developers and manufacturers are welcome to submit PRs to contribute support for more boards.

## Supported Controllers

### LCD Controllers

📋 Here is the list of LCD controllers supported by ESP32_Display_Panel:

| **Manufacturer** | **Model** |
| --------------- | --------- |
| AXS | AXS15231B |
| Fitipower | EK9716B、EK79007 |
| GalaxyCore | GC9A01、GC9B71、GC9503 |
| Himax | HX8399 |
| Ilitek | ILI9341、ILI9881C |
| JADARD | JD9165、JD9365 |
| NewVision | NV3022B |
| SHENGHE | SH8601 |
| Sitronix | ST7262、ST7701、ST7703、ST7789、ST7796、ST77903、ST77916、ST77922 |
| Solomon Systech | SPD2010 |

📌 For detailed information, please refer to [Supported LCD Controllers](./docs/drivers/lcd.md).

### Touch Controllers

📋 Here is the list of touch controllers supported by ESP32_Display_Panel:

| **Manufacturer** | **Model** |
| --------------- | --------- |
| AXS | AXS15231B |
| Chipsemicorp | CHSC6540 |
| FocalTech | FT5x06 |
| GOODiX | GT911、GT1151 |
| Hynitron | CST816S |
| Parade | TT21100 |
| Sitronix | ST7123、ST1633 |
| Solomon Systech | SPD2010 |
| ST | STMPE610 |
| Xptek | XPT2046 |

📌 For detailed information, please refer to [Supported Touch Controllers](./docs/drivers/touch.md).

## FAQ

🔍 Here are common issues in different development environments:

* [Arduino IDE](./docs/envs/use_with_arduino.md#faq)

  * [Where is the Arduino library directory?](./docs/envs/use_with_arduino.md#where-is-the-arduino-library-directory)
  * [Where are the arduino-esp32 installation directory and SDK directory?](./docs/envs/use_with_arduino.md#where-are-the-arduino-esp32-installation-directory-and-sdk-directory)
  * [How to install ESP32_Display_Panel in Arduino IDE?](./docs/envs/use_with_arduino.md#how-to-install-esp32_display_panel-in-arduino-ide)
  * [How to select and configure supported boards in Arduino IDE?](./docs/envs/use_with_arduino.md#how-to-select-and-configure-supported-boards-in-arduino-ide)
  * [How to use SquareLine exported UI source files in Arduino IDE?](./docs/envs/use_with_arduino.md#how-to-use-squareline-exported-ui-source-files-in-arduino-ide)
  * [How to debug when the screen doesn't light up using the library in Arduino IDE?](./docs/envs/use_with_arduino.md#how-to-debug-when-the-screen-doesnt-light-up-using-the-library-in-arduino-ide)
  * [How to fix the issue that log messages are missing or incomplete in Arduino IDE's Serial Monitor?](./docs/envs/use_with_arduino.md#how-to-fix-the-issue-that-log-messages-are-missing-or-incomplete-in-arduino-ides-serial-monitor)
  * [Solution for screen drift issue when using ESP32-S3 to drive RGB LCD in Arduino IDE](./docs/envs/use_with_arduino.md#solution-for-screen-drift-issue-when-using-esp32-s3-to-drive-rgb-lcd-in-arduino-ide)
  * [How to reduce Flash usage and speed up compilation when using ESP32_Display_Panel in Arduino IDE?](./docs/envs/use_with_arduino.md#how-to-reduce-flash-usage-and-speed-up-compilation-when-using-esp32_display_panel-in-arduino-ide)
  * [How to avoid I2C re-initialization when using ESP32_Display_Panel in Arduino IDE (e.g., when using Wire library)?](./docs/envs/use_with_arduino.md#how-to-avoid-i2c-re-initialization-when-using-esp32_display_panel-in-arduino-ide-eg-when-using-wire-library)

* [ESP-IDF](./docs/envs/use_with_idf.md#faq)

  * [Solution for screen drift issue when using ESP32-S3 to drive RGB LCD in ESP-IDF](./docs/envs/use_with_idf.md#solution-for-screen-drift-issue-when-using-esp32-s3-to-drive-rgb-lcd-in-esp-idf)
  * [How to reduce Flash usage and speed up compilation when using ESP32_Display_Panel in ESP-IDF?](./docs/envs/use_with_idf.md#how-to-reduce-flash-usage-and-speed-up-compilation-when-using-esp32_display_panel-in-esp-idf)
  * [Other LCD issues in ESP-IDF](./docs/envs/use_with_idf.md#other-lcd-issues-in-esp-idf)

* [Other FAQs](./docs/faq_others.md)

  * [Can't find drivers for the same model of LCD/Touch device?](./docs/faq_others.md#can-t-find-drivers-for-the-same-model-of-lcd/touch-device)
