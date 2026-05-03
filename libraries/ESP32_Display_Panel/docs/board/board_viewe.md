# [VIEWE](https://viewedisplay.com/)

## Supported Development Boards

|                                                **Picture**                                                 |                                                                        **Name**                                                                         |   **LCD Bus**    |        **LCD Controller**         | **LCD resolution** | **Touch Bus** | **Touch Controller** |
| :--------------------------------------------------------------------------------------------------------: | :-----------------------------------------------------------------------------------------------------------------------------------------------------: | :--------------: | :-------------------------------: | ------------------ | :-----------: | :------------------: |
|     <img src="https://viewedisplay.com/wp-content/uploads/2024/11/UEDX24320024E-WB-A.jpg" width="150">     | [UEDX24320024E-WB-A](https://viewedisplay.com/product/esp32-2-4-inch-240x320-rgb-ips-tft-display-touch-screen-arduino-lvgl-wifi-ble-uart-smart-module/) |       SPI        |        GC9307(like GC9A01)        | 240x320            |      I2C      |       CHSC6540       |
|     <img src="https://viewedisplay.com/wp-content/uploads/2024/11/UEDX24320028E-WB-A.jpg" width="150">     | [UEDX24320028E-WB-A](https://viewedisplay.com/product/esp32-2-8-inch-240x320-mcu-ips-tft-display-touch-screen-arduino-lvgl-wifi-ble-uart-smart-module/) |       SPI        |        GC9307(like GC9A01)        | 240x320            |      I2C      |       CHSC6540       |
|    <img src="https://viewedisplay.com/wp-content/uploads/2024/11/UEDX24320035E-WB-A-1.jpg" width="150">    | [UEDX24320035E-WB-A](https://viewedisplay.com/product/esp32-3-5-inch-240x320-mcu-ips-tft-display-touch-screen-arduino-lvgl-wifi-ble-uart-smart-module/) |       SPI        |        GC9307(like GC9A01)        | 240x320            |      I2C      |       CHSC6540       |
|     <img src="https://viewedisplay.com/wp-content/uploads/2024/11/UEDX32480035E-WB-A.jpg" width="150">     |  [UEDX32480035E-WB-A](https://viewedisplay.com/product/esp32-3-5-inch-320x4-mcu-ips-tft-display-touch-screen-arduino-lvgl-wifi-ble-uart-smart-module/)  |       SPI        |       ST7365P(like ST7789)        | 320x480            |      I2C      |       CHSC6540       |
|      <img src="https://viewedisplay.com/wp-content/uploads/2024/07/UEDX80480043E-13.jpg" width="150">      | [UEDX48270043E-WB-A](https://viewedisplay.com/product/esp32-4-3-inch-480x272-rgb-ips-tft-display-touch-screen-arduino-lvgl-wifi-ble-uart-smart-module/) |       RGB        |              ST7262               | 480x272            |      I2C      |        GT911         |
| <img src="https://viewedisplay.com/wp-content/uploads/2024/07/DX48480040E-WB-A-%E6%AD%A3.jpg" width="150"> |                       [UEDX48480040E-WB-A](https://viewedisplay.com/product/esp32-4-inch-tft-display-touch-screen-arduino-lvgl/)                        | 3-wire SPI + RGB |              GC9503               | 480x480            |      I2C      | FT6336U(like FT5x06) |
|      <img src="https://viewedisplay.com/wp-content/uploads/2024/07/UEDX80480043E-13.jpg" width="150">      |              [UEDX80480043E-WB-A](https://viewedisplay.com/product/esp32-4-3-inch-800x480-rgb-ips-tft-display-touch-screen-arduino-lvgl/)               |       RGB        |              ST7262               | 800x480            |      I2C      |        GT911         |
|       <img src="https://viewedisplay.com/wp-content/uploads/2024/06/DX80480050E-aa.jpg" width="150">       |               [UEDX80480050E-WB-A](https://viewedisplay.com/product/esp32-5-inch-800x480-rgb-ips-tft-display-touch-screen-arduino-lvgl/)                |       RGB        |     ST7262E43-G4(like ST7262)     | 800x480            |      I2C      |        GT911         |
|       <img src="https://viewedisplay.com/wp-content/uploads/2024/08/DX80480070E-a2.jpg" width="150">       |             [UEDX80480070E-WB-A](https://viewedisplay.com/product/esp32-7-inch-800x480-rgb-ips-tft-display-touch-screen-arduino-lvgl-uart/)             |       RGB        | EK9716BD3+EK73002AB2(like ST7262) | 800x480            |      I2C      |        GT911         |

## Recommended Configurations

Below are recommended configurations for developing GUI applications on different development boards.

### Arduino IDE

These settings can be adjusted according to specific requirements, and users can navigate to the `Tools` menu in the Arduino IDE to configure the following settings.

|  Supported Boards  |   Selected Board   | PSRAM | Flash Mode | Flash Size | USB CDC On Boot | Partition Scheme |
| :----------------: | :----------------: | :---: | :--------: | :--------: | :-------------: | :--------------: |
| UEDX24320024E-WB-A | ESP32S3 Dev Module |  OPI  | QIO 80MHz  |    16MB    |     Enabled     | 16M Flash (3MB)  |
| UEDX24320028E-WB-A | ESP32S3 Dev Module |  OPI  | QIO 80MHz  |    16MB    |     Enabled     | 16M Flash (3MB)  |
| UEDX24320035E-WB-A | ESP32S3 Dev Module |  OPI  | QIO 80MHz  |    16MB    |     Enabled     | 16M Flash (3MB)  |
| UEDX32480035E-WB-A | ESP32S3 Dev Module |  OPI  | QIO 80MHz  |    16MB    |     Enabled     | 16M Flash (3MB)  |
| UEDX48270043E-WB-A | ESP32S3 Dev Module |  OPI  | QIO 80MHz  |    16MB    |     Enabled     | 16M Flash (3MB)  |
| UEDX48480040E-WB-A | ESP32S3 Dev Module |  OPI  | QIO 80MHz  |    16MB    |     Enabled     | 16M Flash (3MB)  |
| UEDX80480043E-WB-A | ESP32S3 Dev Module |  OPI  | QIO 80MHz  |    16MB    |     Enabled     | 16M Flash (3MB)  |
| UEDX80480050E-WB-A | ESP32S3 Dev Module |  OPI  | QIO 80MHz  |    16MB    |     Enabled     | 16M Flash (3MB)  |
| UEDX80480070E-WB-A | ESP32S3 Dev Module |  OPI  | QIO 80MHz  |    16MB    |     Enabled     | 16M Flash (3MB)  |

> [!NOTE]
> 1. Enable or disable `USB CDC On Boot` based on the type of port used:
>
>    * Disable this configuration if using **UART** port; enable it if using **USB** port.
>    * If this configuration differs from previous flashing, first enable `Erase All Flash Before Sketch Upload`, then it can be disabled after flashing.
>    * If this configuration does not match the actual port type, it will prevent the development board from printing serial logs correctly.
>
> 2. To view more output logs, set `Core Debug Level` to `Info` or a lower level.
> 3. If the predefined partition schemes provided by ESP32 do not meet the requirements, users can also select `Custom` in the "Partition Scheme" and create a custom partition table file `Custom.csv` in the `hardware/esp32/3.x.x/tools/partitions` directory under the [arduino-esp32 installation directory](#where-are-the-installation-directory-for-arduino-esp32-and-the-sdk-located). For detailed information on partition tables, please refer to the [ESP-IDF documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html).
