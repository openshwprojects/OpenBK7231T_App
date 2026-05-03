| Supported ESP SoCs | ESP32 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 | ESP32-P4 |
| ------------------ | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

| Supported LCD Controllers | AXS15231B | GC9A01 | GC9B71 | ILI9341 | NV3022B | SH8601 | SPD2010 | ST7789 | ST7796 | ST77916 | ST77922 |
| ------------------------- | --------- | ------ | ------ | ------- | ------- | ------ | ------- | ------ | ------ | ------- | ------- |

# SPI LCD Example

## Overview

This example demonstrates how to drive different model LCDs with `SPI` interface bus and test them by displaying color bars.

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

- [Mandatory] Edit the macro definitions in the [lcd_spi.ino](./lcd_spi.ino) file

### Step 4. Configure Arduino IDE

- Navigate to the `Tools` menu
- Select the target board in the `Board` menu
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

The following are the logs output when using the `ILI9341` LCD. The logs content may vary with different LCDs or different configurations, and it is provided for reference only.

```bash
...
Initializing backlight and turn it off
Initializing "SPI" LCD with config
[I][Panel][esp_lcd_ili9341.c:0061](esp_lcd_new_panel_ili9341): version: 2.0.0
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
IDLE loop
...
```

## Troubleshooting

Please check the [FAQ](../../../../../docs/envs/use_with_arduino.md#faq) first to see if the same question exists. If not, please create a [Github Issue](https://github.com/esp-arduino-libs/ESP32_Display_Panel/issues). We will get back to you as soon as possible.
