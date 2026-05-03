| Supported ESP SoCs | ESP32 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 | ESP32-P4 |
| ------------------ | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

# Board Dynamic Config Example

## Overview

This example demonstrates how to dynamically load the display screen settings of development boards through code, and verify the configuration by displaying color bars and monitoring touch coordinates.

This example in *esp_panel_drivers_conf.h* enables all drivers, while the default configuration file in the ESP32_Display_Panel directory only enable some drivers. **If you want to dynamically load board configurations through code, please enable all the drivers you need first.**

## How to Use

### Step 1. Install SDK and dependencies

- Install the following SDK and library dependencies:

  - See [SDK & Dependencies](../../../../docs/envs/use_with_arduino.md#sdk--dependencies) and [Installing Libraries](../../../../docs/envs/use_with_arduino.md#installing-libraries) sections for more information

### Step 2. Configure the libraries

- [Optional] `ESP32_Display_Panel`:

  - Unlike the [board_static_config](../board_static_config) example, this example does not provide *esp_panel_board_supported_conf.h* and *esp_panel_board_custom_conf.h* files in the project directory.
  - This example already has the [esp_panel_drivers_conf.h](./esp_panel_drivers_conf.h) configuration file in the project directory. **And to make sure the example works on all target boards, all drivers are enabled by default.**
  - See [Configuring Guide](../../../../docs/envs/use_with_arduino.md#configuration-guide) section for more information

- [Optional] `esp-lib-utils` :

  - This example already has the [esp_utils_conf.h](./esp_utils_conf.h) configuration file in the project directory. Edit this file as needed
  - See [Configuring esp-lib-utils](../../../../docs/envs/use_with_arduino.md#configuring-esp-lib-utils) section for more information

### Step 3. Configure the example

- [Mandatory] Edit the [board_external_config.cpp](./board_external_config.cpp) file

  - If using a supported development board, modify the line `#include "board/supported/espressif/BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2_V1_5.h"` to `#include "board/supported/<manufacturer>/<board_name>.h"`
  - If using a custom development board, comment out the line `#include "board/supported/espressif/BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2_V1_5.h"`, then directly modify the `BOARD_EXTERNAL_CONFIG` structure

- [Optional] Edit the macro definitions in the [board_dynamic_config.ino](./board_dynamic_config.ino) file

### Step 4. Configure Arduino IDE

- Navigate to the `Tools` menu
- Select the target board in the `Board` menu
- Change the `PSRAM` option to `OPI PSRAM` if using `ESP32S3R8 + RGB LCD`, or `Enabled` if using `ESP32P4 + MIPI-DSI LCD`
- Change the `USB CDC On Boot` option to `Enabled` if using `USB` port, or `Disabled` if using `UART` port. If this configuration differs from previous flashing, first enable `Erase All Flash Before Sketch Upload`, then it can be disabled after flashing.
- Change other configurations as needed
- see [Configuring Arduino IDE](../../../../docs/envs/use_with_arduino.md#configuring-arduino-ide) for more information

### Step 5. Compile and upload the project

- Connect the board to your computer
- Select the correct serial port
- Click the `upload` button

### Step 6. Check the serial output

- Open the serial monitor and select the correct baud rate (like `115200`)
- Check the output logs

## Serial Output

The following are the logs output when using the `Espressif:ESP32_S3_LCD_EV_BOARD_2_V1_5` development board. The logs content may vary with different development boards or different configurations, and it is provided for reference only.

```bash
...
Initializing board with external config
[I][Panel][esp_panel_board.cpp:0066](init): Initializing board (Espressif:ESP32_S3_LCD_EV_BOARD_2_V1_5)
[I][Panel][esp_panel_board.cpp:0235](init): Board initialize success
[I][Panel][esp_panel_board.cpp:0253](begin): Beginning board (Espressif:ESP32_S3_LCD_EV_BOARD_2_V1_5)
[I][Panel][esp_lcd_touch_gt1151.c:0050](esp_lcd_touch_new_i2c_gt1151): version: 1.0.5
[I][Panel][esp_lcd_touch_gt1151.c:0234](read_product_id): IC version: GT1158_000101(Patch)_0102(Mask)_00(SensorID)
[I][Panel][esp_panel_board.cpp:0459](begin): Board begin success
Backlight is not available
Testing LCD
Draw color bar from top to bottom, the order is B - G - R
LCD draw finish callback
LCD draw finish callback
LCD draw finish callback
LCD draw finish callback
LCD draw finish callback
LCD draw finish callback
LCD draw finish callback
LCD draw finish callback
LCD draw finish callback
LCD draw finish callback
LCD draw finish callback
LCD draw finish callback
LCD draw finish callback
LCD draw finish callback
LCD draw finish callback
LCD draw finish callback
Testing Touch
Reading touch points...
LCD FPS: 31
Touch point(0): x 240, y 372, strength 50
Touch point(1): x 273, y 215, strength 57
Touch point(2): x 506, y 84, strength 53
Touch point(3): x 375, y 122, strength 37
...
```

## Troubleshooting

Please check the [FAQ](../../../../docs/envs/use_with_arduino.md#faq) first to see if the same question exists. If not, please create a [Github Issue](https://github.com/esp-arduino-libs/ESP32_Display_Panel/issues). We will get back to you as soon as possible.
