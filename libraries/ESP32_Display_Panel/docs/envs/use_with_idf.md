# Using with ESP-IDF

* [中文版](./use_with_idf_cn.md)

## Table of Contents

- [Using with ESP-IDF](#using-with-esp-idf)
  - [Table of Contents](#table-of-contents)
  - [SDK \& Dependencies](#sdk--dependencies)
  - [Adding to Project](#adding-to-project)
  - [Configuration Guide](#configuration-guide)
  - [Example Description](#example-description)
  - [FAQ](#faq)
    - [Solution for screen drift issue when using ESP32-S3 to drive RGB LCD in ESP-IDF](#solution-for-screen-drift-issue-when-using-esp32-s3-to-drive-rgb-lcd-in-esp-idf)
    - [How to reduce Flash usage and speed up compilation when using ESP32\_Display\_Panel in ESP-IDF?](#how-to-reduce-flash-usage-and-speed-up-compilation-when-using-esp32_display_panel-in-esp-idf)
    - [Other LCD issues in ESP-IDF](#other-lcd-issues-in-esp-idf)

## SDK & Dependencies

Before using this library, please ensure you have installed the SDK that meets the following version requirements:

|                     **SDK**                     | **Version Required** |
| ----------------------------------------------- | ------------------- |
| [esp-idf](https://github.com/espressif/esp-idf) | >= 5.1              |

|                                       **Dependencies**                                       | **Version Required** |
| -------------------------------------------------------------------------------------------- | ------------------- |
| [esp32_io_expander](https://components.espressif.com/components/espressif/esp32_io_expander) | 1.*                 |
| [esp-lib-utils](https://components.espressif.com/components/espressif/esp-lib-utils)         | 0.2.*               |

> [!NOTE]
> * For SDK installation, please refer to [ESP-IDF Programming Guide - Installation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#get-started-how-to-get-esp-idf)
> * Dependencies will be automatically downloaded during compilation after adding this library to your project

## Adding to Project

ESP32_Display_Panel has been uploaded to the [Espressif Component Registry](https://components.espressif.com/). You can add it to your project in the following ways:

1. **Using Command Line**

    Run the following command in your project directory:

    ```bash
    idf.py add-dependency "espressif/esp32_display_panel==1.*"
    ```

2. **Modifying Configuration File**

    Create or modify the *idf_component.yml* file in your project directory:

    ```yaml
    dependencies:
      espressif/esp32_display_panel: "1.*"
    ```

For detailed information, please refer to [Espressif Documentation - IDF Component Manager](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-component-manager.html).

## Configuration Guide

When developing with ESP-IDF, you can configure ESP32_Display_Panel by modifying `menuconfig`. Here are the steps:

1. Run the command `idf.py menuconfig`.
2. Navigate to `Component config` > `ESP Display Panel Configurations`.

Additionally, since ESP32_Display_Panel depends on the `esp-lib-utils` component, you can also influence ESP32_Display_Panel's behavior by configuring the `esp-lib-utils` component, such as enabling debug information. Here are the steps:

1. Run the command `idf.py menuconfig`.
2. Navigate to `Component config` > `ESP Library Utils Configurations`.

> [!TIP]
> * Run `idf.py save-defconfig` to save the current project's `menuconfig` configuration and generate a *sdkconfig.defaults* default configuration file in the project directory.
> * To discard the current project's menuconfig configuration and load default settings, first delete the *sdkconfig* file in the project directory, then run `idf.py reconfigure`.

## Example Description

* [lvgl_v8_port](../../examples/esp_idf/lvgl_v8_port/): This example demonstrates how to port `LVGL v8`. And it runs LVGL's internal demos include `Music Player`, `Widgets`, `Stress` and `Benchmark`.

## FAQ

### Solution for screen drift issue when using ESP32-S3 to drive RGB LCD in ESP-IDF

Please follow these steps to resolve:

1. **Understand the Issue**

    * Please refer to [ESP32-S3 RGB LCD Screen Drift Issue Description](https://docs.espressif.com/projects/esp-faq/en/latest/software-framework/peripherals/lcd.html#why-do-i-get-drift-overall-drift-of-the-display-when-esp32-s3-is-driving-an-rgb-lcd-screen)

2. **Enable `RGB LCD Bounce Buffer + XIP on PSRAM` Feature**

    a. **Enable `XIP on PSRAM` Feature in `ESP-IDF`**

    - For `ESP-IDF` version `>= 5.1 && < 5.3`, enable `SPIRAM_FETCH_INSTRUCTIONS` and `SPIRAM_RODATA` options in `menuconfig`
    - For `ESP-IDF` version `>= 5.3`, enable `SPIRAM_XIP_FROM_PSRAM` option in `menuconfig`

    b. **Configure `RGB LCD Bounce Buffer`**

    - For supported boards:

        * Usually `ESP_PANEL_BOARD_LCD_RGB_BOUNCE_BUF_SIZE` is already configured to `(ESP_PANEL_BOARD_WIDTH * 10)` by default
        * If the issue persists, increase the buffer size by referring to the example code

    - For custom boards:

        * Set `ESP_PANEL_BOARD_LCD_RGB_BOUNCE_BUF_SIZE` in *esp_panel_board_custom_conf.h*
        * If the issue persists, increase the buffer size by referring to the example

    c. **Configure LVGL Task**

    - If using LVGL, setting the task that executes `lv_timer_handler()` to run on the same core as the task that executes `board->begin()` can help mitigate the screen drift issue

3. **Example Code**

    a. Modifying `Bounce Buffer` size when using a board.

    ```c
    ...
    esp_panel::board::Board *board = new esp_panel::board::Board();
    board->init();
    ...
    /**
     * 1. Should be called after `board->init()` and before `board->begin()`
     * 2. `ESP_PANEL_BOARD_WIDTH` should be replaced with the actual width of the LCD
     */
    auto bus = static_cast<esp_panel::drivers::BusRGB *>(board->getLCD()->getBus());
    bus->configRGB_BounceBufferSize(ESP_PANEL_BOARD_WIDTH * 20);
    ...
    board->begin();
    ...
    ```

    b. Modifying `Bounce Buffer` size when using standalone drivers.

    ```c
    ...
    esp_panel::drivers::BusRGB *bus = new esp_panel::drivers::BusRGB(...);
    ...
    /**
     * 1. Should be called before `bus->init()`
     * 2. `EXAMPLE_LCD_WIDTH` should be replaced with the actual width of the LCD
     */
    bus->configRGB_BounceBufferSize(EXAMPLE_LCD_WIDTH * 20);
    ...
    bus->init();
    ...
    ```

### How to reduce Flash usage and speed up compilation when using ESP32_Display_Panel in ESP-IDF?

Please follow these steps:

1. **Disable unused drivers**: Use `menuconfig` to disable unused drivers in `ESP Display Panel Configurations - Drivers`
2. **Turn off debug logs**: Use `menuconfig` to set `Log level` to `None` in `ESP Library Utils Configurations - Log functions`
3. See [Configuration Guide](#configuration-guide) for detailed configuration

### Other LCD issues in ESP-IDF

Please refer to [ESP-FAQ - LCD](https://docs.espressif.com/projects/esp-faq/en/latest/software-framework/peripherals/lcd.html)
