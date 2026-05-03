# [Shenzhen Jingcai Intelligent](https://www.displaysmodule.com/)

## Supported Development Boards

|                                                                                                                                       **Picture**                                                                                                                                       |                                                **Name**                                                 |   **LCD Bus**    | **LCD Controller** | **LCD resolution** | **Touch Bus** | **Touch Controller** |
| :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------: | :-----------------------------------------------------------------------------------------------------: | :--------------: | :----------------: | :----------------: | :-----------: | :------------------: |
| [<img src="https://www.displaysmodule.com/photo/ps162171631-experience_the_power_of_the_esp32_display_module_sku_esp32_4848s040c_i_y_3.jpg" width="150">](https://www.displaysmodule.com/sale-41828962-experience-the-power-of-the-esp32-display-module-sku-esp32-4848s040c-i-y-3.html) | [ESP32-4848S040C_I_Y_3](http://pan.jczn1688.com/directlink/1/ESP32%20module/4.0inch_ESP32-4848S040.zip) | 3-wire SPI + RGB |       ST7701       |      480x480       |      I2C      |        GT911         |

## Recommended Configurations

Below are recommended configurations for developing GUI applications on different development boards.

### Arduino IDE

These settings can be adjusted according to specific requirements, and users can navigate to the `Tools` menu in the Arduino IDE to configure the following settings.

|         Supported Boards          |   Selected Board   |  PSRAM   | Flash Mode | Flash Size | USB CDC On Boot |    Partition Scheme     |
|:---------------------------------:|:------------------:|:--------:|:----------:|:----------:|:---------------:|:-----------------------:|
|       ESP32-4848S040C_I_Y_3       | ESP32S3 Dev Module |   OPI    | QIO 80MHz  |    16MB    |    Disabled     |     16M Flash (3MB)     |

> [!NOTE]
> 1. Enable or disable `USB CDC On Boot` based on the type of port used:
>
>    * Disable this configuration if using **UART** port; enable it if using **USB** port.
>    * If this configuration differs from previous flashing, first enable `Erase All Flash Before Sketch Upload`, then it can be disabled after flashing.
>    * If this configuration does not match the actual port type, it will prevent the development board from printing serial logs correctly.
>
> 2. To view more output logs, set `Core Debug Level` to `Info` or a lower level.
> 3. If the predefined partition schemes provided by ESP32 do not meet the requirements, users can also select `Custom` in the "Partition Scheme" and create a custom partition table file `Custom.csv` in the `hardware/esp32/3.x.x/tools/partitions` directory under the [arduino-esp32 installation directory](#where-are-the-installation-directory-for-arduino-esp32-and-the-sdk-located). For detailed information on partition tables, please refer to the [ESP-IDF documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html).
