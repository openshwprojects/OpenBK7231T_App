| Supported ESP SoCs | ESP32 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 | ESP32-P4 |
| ------------------ | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

# LVGL (v8) Porting Example

## Overview

This example demonstrates how to port `LVGL v8`. And it runs LVGL's internal demos include `Music Player`, `Widgets`, `Stress` and `Benchmark`.

This example also shows three methods to avoid tearing effect when using `RGB/MIPI-DSI` interface LCD. It uses two or more frame buffers based on LVGL **buffering modes**. For more information about this, please refer to [LVGL documents](https://docs.lvgl.io/8.4/porting/display.html?highlight=buffering%20mode#buffering-modes).

## How to use

### ESP-IDF Required

* The ESP-IDF TAG `v5.1` or later is required to use this example. For using the branch of ESP-IDF, the latest branch `release/v5.3` is recommended. For using the tag of ESP-IDF, the tag `v5.3.2` or later is recommended.
* Please follow the [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html) to set up the development environment.

### Hardware Required

* An development board with [supported LCD](../../../docs/drivers/lcd.md) (or [supported touch](../../../docs/drivers/touch.md) screen)

### Configurations

- Run `idf.py menuconfig`
- Go to `Example Configurations`:

  - `Avoid Tearing Mode`: Select the avoid tearing mode you want to use. Only valid for `RGB/MIPI-DSI` interface LCDs.
  - `Rotation Degree`: Select the rotation degree you want to use. Only valid when `Avoid Tearing Mode` is not `None`.

- Go to `ESP Display Panel Configurations`:

  - See [Configuration Guide](../../../docs/envs/use_with_idf.md#configuration-guide) for more details.

### Build and Flash

Run `idf.py -p <PORT> build flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type `Ctrl-]`.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

## Example Output

The following animations show the example running on the ESP32-S3-LCD-EV-Board & ESP32-S3-LCD-EV-Board-2 development boards.

![lvgl_demos_480_480](https://dl.espressif.com/AE/esp-dev-kits/s3-lcd-ev-board_examples_lvgl_demos_480_480_2.gif)

![lvgl_demos_800_480](https://dl.espressif.com/AE/esp-dev-kits/s3-lcd-ev-board_examples_lvgl_demos_800_480.gif)

## Troubleshooting

Please check the [FAQ](../../../docs/envs/use_with_idf.md#faq) first to see if the same question exists. If not, please create a [Github Issue](https://github.com/esp-arduino-libs/ESP32_Display_Panel/issues). We will get back to you as soon as possible.
