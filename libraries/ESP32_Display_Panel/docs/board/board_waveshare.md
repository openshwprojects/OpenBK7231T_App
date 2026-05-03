# [Waveshare](https://www.waveshare.com/)

## Supported Development Boards

|                                                                                                                                       **Picture**                                                                                                                                       |                                                **Name**                                                 |   **LCD Bus**    | **LCD Controller** | **LCD resolution** | **Touch Bus** | **Touch Controller** |
| :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------: | :-----------------------------------------------------------------------------------------------------: | :--------------: | :----------------: | ------------------ | :-----------: | :------------------: |
| <img src="https://www.waveshare.com/w/upload/5/5f/ESP32-S3-Touch-LCD-1.85_Entity.jpg" width="150"> | [ESP32-S3-Touch-LCD-1.85](https://www.waveshare.com/esp32-s3-touch-lcd-1.85.htm) | QSPI |       ST77916       |      360x360       |      I2C      |        CST816         |
| <img src="https://www.waveshare.com/w/upload/thumb/1/10/ESP32-S3-Touch-LCD-2.1.jpg/300px-ESP32-S3-Touch-LCD-2.1.jpg" width="150"> | [ESP32-S3-Touch-LCD-2.1](https://www.waveshare.com/esp32-s3-touch-lcd-2.1.htm) | RGB |       ST7701       |      480x480       |      I2C      |        CST820 (CST816-like)         |
| <img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-s3-touch-lcd-4.3-1.jpg" width="150"> | [ESP32-S3-Touch-LCD-4.3](https://www.waveshare.com/esp32-s3-touch-lcd-4.3.htm) | RGB |       ST7262       |      800x480       |      I2C      |        GT911         |
| <img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-s3-touch-lcd-4.3b-1.jpg" width="150"> | [ESP32-S3-Touch-LCD-4.3B](https://www.waveshare.com/esp32-s3-touch-lcd-4.3B.htm) | RGB |       ST7262       |      800x480       |      I2C      |        GT911         |
| <img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-s3-touch-lcd-5-1.jpg" width="150"> | [ESP32-S3-Touch-LCD-5](https://www.waveshare.com/esp32-s3-touch-lcd-5.htm?sku=28117) | RGB |       ST7262       |      800x480       |      I2C      |        GT911         |
| <img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-s3-touch-lcd-5-1.jpg" width="150"> | [ESP32-S3-Touch-LCD-5B](https://www.waveshare.com/esp32-s3-touch-lcd-5.htm?sku=28151) | RGB |       ST7262       |      1024x600       |      I2C      |        GT911         |
| <img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-s3-touch-lcd-7-1.jpg" width="150"> | [ESP32-S3-Touch-LCD-7](https://www.waveshare.com/esp32-s3-touch-lcd-7.htm) | RGB |       ST7262       |      800x480       |      I2C      |        GT911         |
| <img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-nano-1.jpg" width="150"> | [ESP32-P4-NANO](https://www.waveshare.com/esp32-p4-nano.htm) |  MIPI-DSI   |       JD9365       | 800x1280           |      I2C      |        GT9271 (GT911-like)        |

## Recommended Configurations

Below are recommended configurations for developing GUI applications on different development boards.

## Arduino IDE

These settings can be adjusted according to specific requirements, and users can navigate to the `Tools` menu in the Arduino IDE to configure the following settings.

|         Supported Boards          |   Selected Board   |  PSRAM   | Flash Mode | Flash Size | USB CDC On Boot |    Partition Scheme     |
|:---------------------------------:|:------------------:|:--------:|:----------:|:----------:|:---------------:|:-----------------------:|
| Waveshare-ESP32-S3-Touch-LCD-1.85 | ESP32S3 Dev Module |   OPI    | QIO 80MHz  |    16MB    |     Enabled     |     16M Flash (3MB)     |
| Waveshare-ESP32-S3-Touch-LCD-2.1  | ESP32S3 Dev Module |   OPI    | QIO 80MHz  |    16MB    |     Enabled     |     16M Flash (3MB)     |
| Waveshare-ESP32-S3-Touch-LCD-4.3  | ESP32S3 Dev Module |   OPI    | QIO 80MHz  |    8MB     |    Disabled     |     8M with spiffs      |
| Waveshare-ESP32-S3-Touch-LCD-4.3B | ESP32S3 Dev Module |   OPI    | QIO 80MHz  |    16MB    |    Enabled      |     16M Flash (3MB)     |
| Waveshare-ESP32-S3-Touch-LCD-5    | ESP32S3 Dev Module |   OPI    | QIO 80MHz  |    16MB    |    Enabled      |     16M Flash (3MB)     |
| Waveshare-ESP32-S3-Touch-LCD-5B   | ESP32S3 Dev Module |   OPI    | QIO 80MHz  |    16MB    |    Enabled      |     16M Flash (3MB)     |
| Waveshare-ESP32-S3-Touch-LCD-7    | ESP32S3 Dev Module |   OPI    | QIO 80MHz  |    8MB     |    Disabled     |     8M with spiffs      |
|      Waveshare-ESP32-P4-NANO      | ESP32P4 Dev Module | Enabled  |    QIO     |    16MB    |    Disabled     |     16M Flash (3MB)     |

> [!NOTE]
> 1. Enable or disable `USB CDC On Boot` based on the type of port used:
>
>    * Disable this configuration if using **UART** port; enable it if using **USB** port.
>    * If this configuration differs from previous flashing, first enable `Erase All Flash Before Sketch Upload`, then it can be disabled after flashing.
>    * If this configuration does not match the actual port type, it will prevent the development board from printing serial logs correctly.
>
> 2. To view more output logs, set `Core Debug Level` to `Info` or a lower level.
> 3. If the predefined partition schemes provided by ESP32 do not meet the requirements, users can also select `Custom` in the "Partition Scheme" and create a custom partition table file `Custom.csv` in the `hardware/esp32/3.x.x/tools/partitions` directory under the [arduino-esp32 installation directory](#where-are-the-installation-directory-for-arduino-esp32-and-the-sdk-located). For detailed information on partition tables, please refer to the [ESP-IDF documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html).
