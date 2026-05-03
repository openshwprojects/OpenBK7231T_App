# [Elecrow](https://www.elecrow.com/)

## Supported Development Boards

|                                                                 **Picture**                                                                  |                                                       **Name**                                                        | **LCD Bus** |   **LCD Controller**    | **LCD resolution** | **Touch Bus** | **Touch Controller** |
| :------------------------------------------------------------------------------------------------------------------------------------------: | :-------------------------------------------------------------------------------------------------------------------: | :---------: | :---------------------: | :----------------: | :-----------: | :------------------: |
| <img src="https://www.elecrow.com/media/catalog/product/cache/acf3559c3a3e20af42aec3d2d8cc99f6/e/s/esp32_7inch_display_1_1.png" width="150"> | [CrowPanel 7.0"](https://www.elecrow.com/esp32-display-7-inch-hmi-display-rgb-tft-lcd-touch-screen-support-lvgl.html) |     RGB     | EK9716BD3 & EK73002ACGB |      800x480       |      I2C      |        GT911         |

## Recommended Configurations

Below are recommended configurations for developing GUI applications on different development boards.

### Arduino IDE

These settings can be adjusted according to specific requirements, and users can navigate to the `Tools` menu in the Arduino IDE to configure the following settings.

|         Supported Boards          |   Selected Board   |  PSRAM   | Flash Mode | Flash Size | USB CDC On Boot |    Partition Scheme     |
|:---------------------------------:|:------------------:|:--------:|:----------:|:----------:|:---------------:|:-----------------------:|
|       ElecrowCrowPanel 7.0"       | ESP32S3 Dev Module |   OPI    | QIO 80MHz  |    4MB     |    Disabled     |     Huge App (3MB)      |

> [!NOTE]
> 1. Enable or disable `USB CDC On Boot` based on the type of port used:
>
>    * Disable this configuration if using **UART** port; enable it if using **USB** port.
>    * If this configuration differs from previous flashing, first enable `Erase All Flash Before Sketch Upload`, then it can be disabled after flashing.
>    * If this configuration does not match the actual port type, it will prevent the development board from printing serial logs correctly.
>
> 2. To view more output logs, set `Core Debug Level` to `Info` or a lower level.
> 3. If the predefined partition schemes provided by ESP32 do not meet the requirements, users can also select `Custom` in the "Partition Scheme" and create a custom partition table file `Custom.csv` in the `hardware/esp32/3.x.x/tools/partitions` directory under the [arduino-esp32 installation directory](#where-are-the-installation-directory-for-arduino-esp32-and-the-sdk-located). For detailed information on partition tables, please refer to the [ESP-IDF documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html).
