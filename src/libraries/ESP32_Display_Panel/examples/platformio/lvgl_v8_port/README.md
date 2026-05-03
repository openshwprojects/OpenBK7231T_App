| Supported ESP SoCs | ESP32 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 | ESP32-P4 |
| ------------------ | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

# LVGL (v8) Porting Example

## Overview

This example demonstrates how to port `LVGL v8`. And for `RGB/MIPI-DSI` interface LCDs, it can enable the avoid tearing and rotation function.

## How to Use

### Step 1. Configure the libraries

- [Optional] `ESP32_Display_Panel`:

  - This example already has the [esp_panel_board_custom_conf.h](./src/esp_panel_board_custom_conf.h) and [esp_panel_drivers_conf.h](./src/esp_panel_drivers_conf.h) configuration files in the project directory. Edit these files as needed
  - see [Board Configuration Guide](../../../docs/envs/use_with_arduino.md#configuration-guide) for more information

- [Optional] `esp-lib-utils` :

  - This example already has the [esp_utils_conf.h](./src/esp_utils_conf.h) configuration file in the project directory. Edit this file as needed
  - See [Configuring esp-lib-utils](../../../docs/envs/use_with_arduino.md#configuring-esp-lib-utils) section for more information

- [Optional] `lvgl` :

  - This example already has the [lv_conf.h](./src/lv_conf.h) configuration file which been modified with the recommended configurations in the project directory. Edit this file as needed
  - See [Configuring LVGL](../../../docs/envs/use_with_arduino.md#configuring-lvgl) section for more information

### Step 2. Configure PlatformIO

- This example uses the `BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2_V1_5` board as default. You can choose another board in the `[platformio]:default_envs` of the [platformio.ini](./platformio.ini) file.
- If there is no the board you want to use, please check the following:

  - **If using a [supported board](../../../README.md#supported-boards)**, take the `BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_1_85` board as an example, follow the steps:

    - Copy a supported board file in the [boards](./boards/) directory, which has the same chip as your board, for example, the `BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2_V1_5.json` file. Then paste it to the [boards](./boards/) directory and rename it to `BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_1_85.json`. Modify the content as needed.
    - Add a new board env in the *platformio.ini* file as follows:

      ```ini
      ...
      [env:BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_1_85]
      build_flags =
          ${common.build_flags}
          ${spi_qspi_lcd.build_flags}   ; or ${rgb_mipi_lcd.build_flags}
          -DESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED=1
          -DBOARD_M5STACK_M5CORE2
      board = BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_1_85
      ...
      ```

    - Add the board env to the `[platformio]:default_envs` as follows:

      ```ini
      ...
      default_envs =
        ...
        ; BOARD_ESPRESSIF_ESP32_S3_USB_OTG
        ; BOARD_ESPRESSIF_ESP32_P4_FUNCTION_EV_BOARD
        ...
        BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_1_85
      ...
      ```

  - **If using a custom board**, follow the steps:

    - Modify the [BOARD_CUSTOM.json](./boards/BOARD_CUSTOM.json) board file by referring to a supported board file which has the same chip as your board.
    - Modify the `[env:BOARD_CUSTOM]` board env in the *platformio.ini* file as needed
    - Modify the *esp_panel_board_custom_conf.h* file and set `ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM` to `1`. Then change other configurations as needed in the file

  - See [PlatformIO Docs](https://docs.platformio.org/en/latest/projectconf/index.html) for more information

### Step 3. Configure the example

- [Optional] Edit the macro definitions in the [lvgl_v8_port.h](./src/lvgl_v8_port.h) file

  - **If using `RGB/MIPI-DSI` interface**, change the `LVGL_PORT_AVOID_TEARING_MODE` macro definition to `1`/`2`/`3` to enable the avoid tearing function. After that, change the `LVGL_PORT_ROTATION_DEGREE` macro definition to the target rotation degree
  - **If using other interfaces**, please don't modify the `LVGL_PORT_AVOID_TEARING_MODE` and `LVGL_PORT_ROTATION_DEGREE` macro definitions

### Step 4. Compile and upload the project

- Connect the board to your computer
- Click the `upload` button

### Step 5. Check the serial output

- Open the serial monitor
- Check the output logs

## Serial Output

The following are the logs output when using the `Espressif:ESP32_S3_LCD_EV_BOARD_2_V1_5` development board. The logs content may vary with different development boards or different configurations, and it is provided for reference only.

```bash
...
Initializing board
[I][Panel][esp_panel_board.cpp:0066](init): Initializing board (Espressif:ESP32_S3_LCD_EV_BOARD_2_V1_5)
[I][Panel][esp_panel_board.cpp:0235](init): Board initialize success
[I][Panel][esp_panel_board.cpp:0253](begin): Beginning board (Espressif:ESP32_S3_LCD_EV_BOARD_2_V1_5)
[I][Panel][esp_lcd_touch_gt1151.c:0050](esp_lcd_touch_new_i2c_gt1151): version: 1.0.5
[I][Panel][esp_lcd_touch_gt1151.c:0234](read_product_id): IC version: GT1158_000101(Patch)_0102(Mask)_00(SensorID)
[I][Panel][esp_panel_board.cpp:0459](begin): Board begin success
Initializing LVGL
[I][LvPort][lvgl_v8_port.cpp:0769](lvgl_port_init): Initializing LVGL display driver
Creating UI
IDLE loop
IDLE loop
...
```

## Troubleshooting

Please check the [FAQ](../../../README.md#faq) first to see if the same question exists. If not, please create a [Github Issue](https://github.com/esp-arduino-libs/ESP32_Display_Panel/issues). We will get back to you as soon as possible.
