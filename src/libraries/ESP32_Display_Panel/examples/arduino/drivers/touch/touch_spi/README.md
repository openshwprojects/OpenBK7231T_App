| Supported ESP SoCs | ESP32 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 | ESP32-P4 |
| ------------------ | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

| Supported Touch Controllers | STMPE610 | XPT2046 |
| --------------------------- | -------- | ------- |

# SPI Touch Example

## Overview

This example demonstrates how to develop different model touches with SPI interface using standalone drivers and test them by printing touch point coordinates.

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

- [Mandatory] Edit the macro definitions in the [touch_i2c.ino](./touch_i2c.ino) file

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

The following are the logs output when using the `XPT2046` touch. The logs content may vary with different touches or different configurations, and it is provided for reference only.

```bash
...
Initializing "SPI" touch without config
[I][Panel][esp_lcd_touch_xpt2046.c:0095](esp_lcd_touch_new_spi_xpt2046): version: 1.0.5
Reading touch points and buttons...
Touch point(0): x 156, y 182, strength 1337
Touch point(0): x 157, y 182, strength 1365
Touch point(0): x 144, y 197, strength 981
Touch point(0): x 136, y 201, strength 1182
Touch point(0): x 152, y 226, strength 1070
Touch point(0): x 160, y 233, strength 1166
Touch point(0): x 152, y 217, strength 1186
Touch point(0): x 106, y 251, strength 1024
Touch point(0): x 161, y 241, strength 1314
...
```

## Troubleshooting

Please check the [FAQ](../../../../../docs/envs/use_with_arduino.md#faq) first to see if the same question exists. If not, please create a [Github Issue](https://github.com/esp-arduino-libs/ESP32_Display_Panel/issues). We will get back to you as soon as possible.
