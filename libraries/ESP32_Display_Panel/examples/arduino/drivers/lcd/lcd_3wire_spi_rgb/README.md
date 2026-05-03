| Supported ESP SoCs | ESP32-S3 |
| ------------------ | -------- |

| Supported LCD Controllers | GC9503 | ST7701 | ST77903 | ST77922 |
| ------------------------- | ------ | ------ | ------- | ------- |

# "3-wire SPI + RGB" LCD Example

## Overview

This example demonstrates how to drive different model LCDs with `3-wire SPI + RGB` interface bus and test them by displaying color bars.

## How to Use

### Step 1. Install SDK and dependencies

- Install the following SDK and library dependencies:

  - See [SDK & Dependencies](../../../../../docs/envs/use_with_arduino.md#sdk--dependencies) and [Installing Libraries](../../../../../docs/envs/use_with_arduino.md#installing-libraries) sections for more information

### Step 2. Configure the libraries

- [Optional] `ESP32_Display_Panel`:

  - This example already has the [esp_panel_drivers_conf.h](./esp_panel_drivers_conf.h) configuration file in the project directory. Edit this file as needed
  - See [Configuring Guide](../../../../../docs/envs/use_with_arduino.md#configuration-guide) section for more information

- [Optional] `esp-lib-utils` :

  - This example already has the [esp_utils_conf.h](./esp_utils_conf.h) configuration file in the project directory. Edit this file as needed
  - See [Configuring esp-lib-utils](../../../../../docs/envs/use_with_arduino.md#configuring-esp-lib-utils) section for more information

### Step 3. Configure the example

- [Mandatory] Edit the macro definitions in the [lcd_3wire_spi_rgb.ino](./lcd_3wire_spi_rgb.ino) file

### Step 4. Configure Arduino IDE

- Navigate to the `Tools` menu
- Select the target board in the `Board` menu
- Change the `PSRAM` option to `OPI PSRAM` if using `ESP32S3R8 + RGB LCD`, or `Enabled` if using `ESP32P4 + MIPI-DSI LCD`
- Change the `USB CDC On Boot` option to `Enabled` if using `USB` port, or `Disabled` if using `UART` port. If this configuration differs from previous flashing, first enable `Erase All Flash Before Sketch Upload`, then it can be disabled after flashing.
- Change other configurations as needed
- see [Configuring Arduino IDE](../../../../../docs/envs/use_with_arduino.md#configuring-arduino-ide) for more information

### Step 5. Compile and upload the project

- Connect the board to your computer
- Select the correct serial port
- Click the `upload` button

### Step 6. Check the serial output

- Open the serial monitor and select the correct baud rate (like `115200`)
- Check the output logs

## Serial Output

The following are the logs output when using the `ST7701` LCD. The logs content may vary with different LCDs or different configurations, and it is provided for reference only.

```bash
...
Initializing backlight and turn it off
Initializing "3-wire SPI + RGB" LCD without config
[I][Panel][esp_lcd_st7701.c:0026](esp_lcd_new_panel_st7701): version: 1.1.1
Draw color bar from top left to bottom right, the order is B - G - R
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
Turn on the backlight
IDLE loop
LCD FPS: 98
LCD FPS: 98
IDLE loop
LCD FPS: 98
LCD FPS: 98
...
```

## Troubleshooting

Please check the [FAQ](../../../../../docs/envs/use_with_arduino.md#faq) first to see if the same question exists. If not, please create a [Github Issue](https://github.com/esp-arduino-libs/ESP32_Display_Panel/issues). We will get back to you as soon as possible.
