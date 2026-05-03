# [M5Stack](https://m5stack.com/)

## Supported Development Boards

|                                                               **Picture**                                                               |                          **Name**                           | **LCD Bus** | **LCD Controller** | **LCD resolution** | **Touch Bus** | **Touch Controller** |
| --------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------- | ----------- | ------------------ | ------------------ | ------------- | -------------------- |
| <img src="https://static-cdn.m5stack.com/resource/docs/products/core/core2/core2_01.webp" width="150">                                  | [M5STACK_M5CORE2](https://docs.m5stack.com/en/core/core2)   | SPI         | ILI9342C           | 320x240            | I2C           | FT6336U              |
| <img src="https://static-cdn.m5stack.com/resource/docs/products/core/M5Dial/img-2afd549e-8af8-47b4-823a-e90e063a0139.webp" width="150"> | [M5STACK_M5DIAL](https://docs.m5stack.com/en/core/M5Dial)   | SPI         | GC9A01             | 240x240            | I2C           | FT5x06               |
| <img src="https://static-cdn.m5stack.com/resource/docs/products/core/CoreS3/img-96063e2a-637a-4d11-ac47-1ce4f1cdfd3e.webp" width="150"> | [M5STACK_M5CORES3](https://docs.m5stack.com/en/core/CoreS3) | SPI         | ILI9342C           | 320x240            | I2C           | FT6336U              |

## Recommended Configurations

Below are recommended configurations for developing GUI applications on different development boards.

### Arduino IDE

These settings can be adjusted according to specific requirements, and users can navigate to the `Tools` menu in the Arduino IDE to configure the following settings.

|         Supported Boards          |   Selected Board   |  PSRAM   | Flash Mode | Flash Size | USB CDC On Boot |    Partition Scheme     |
|:---------------------------------:|:------------------:|:--------:|:----------:|:----------:|:---------------:|:-----------------------:|
|          M5STACK-M5CORE2          |   M5Stack-Core2    | Enabled  |     -      |     -      |        -        |         Default         |
|          M5STACK-M5DIAL           | ESP32S3 Dev Module |   OPI    | QIO 80MHz  |    8MB     |    Disabled     |         Default         |
|         M5STACK-M5CORES3          | ESP32S3 Dev Module |   QSPI   | QIO 80MHz  |    16MB    |     Enabled     | Default 4MB with spiffs |

> [!NOTE]
> 1. Enable or disable `USB CDC On Boot` based on the type of port used:
>
>    * Disable this configuration if using **UART** port; enable it if using **USB** port.
>    * If this configuration differs from previous flashing, first enable `Erase All Flash Before Sketch Upload`, then it can be disabled after flashing.
>    * If this configuration does not match the actual port type, it will prevent the development board from printing serial logs correctly.
>
> 2. To view more output logs, set `Core Debug Level` to `Info` or a lower level.
> 3. If the predefined partition schemes provided by ESP32 do not meet the requirements, users can also select `Custom` in the "Partition Scheme" and create a custom partition table file `Custom.csv` in the `hardware/esp32/3.x.x/tools/partitions` directory under the [arduino-esp32 installation directory](#where-are-the-installation-directory-for-arduino-esp32-and-the-sdk-located). For detailed information on partition tables, please refer to the [ESP-IDF documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html).
