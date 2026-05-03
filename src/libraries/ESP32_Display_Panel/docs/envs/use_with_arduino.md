# Using with Arduino IDE

* [中文版](./use_with_arduino_cn.md)

## Table of Contents

- [Using with Arduino IDE](#using-with-arduino-ide)
  - [Table of Contents](#table-of-contents)
  - [Quick Start](#quick-start)
    - [Environment Setup](#environment-setup)
    - [Run First Example](#run-first-example)
    - [Next Steps](#next-steps)
  - [SDK \& Dependencies](#sdk--dependencies)
  - [Installing Libraries](#installing-libraries)
  - [Configuration Guide](#configuration-guide)
    - [Adjusting Driver Configuration](#adjusting-driver-configuration)
    - [Loading of Supported Boards](#loading-of-supported-boards)
    - [Loading of Custom Boards](#loading-of-custom-boards)
  - [Example Description](#example-description)
    - [Drivers](#drivers)
    - [Board](#board)
    - [GUI](#gui)
  - [Additional Information](#additional-information)
    - [Configuring esp-lib-utils](#configuring-esp-lib-utils)
    - [Configuring Arduino IDE](#configuring-arduino-ide)
    - [Configuring LVGL](#configuring-lvgl)
    - [Porting SquareLine Projects](#porting-squareline-projects)
  - [FAQ](#faq)
    - [Where is the Arduino library directory?](#where-is-the-arduino-library-directory)
    - [Where are the arduino-esp32 installation directory and SDK directory?](#where-are-the-arduino-esp32-installation-directory-and-sdk-directory)
    - [How to install ESP32\_Display\_Panel in Arduino IDE?](#how-to-install-esp32_display_panel-in-arduino-ide)
    - [How to select and configure supported boards in Arduino IDE?](#how-to-select-and-configure-supported-boards-in-arduino-ide)
    - [How to use SquareLine exported UI source files in Arduino IDE?](#how-to-use-squareline-exported-ui-source-files-in-arduino-ide)
    - [How to debug when the screen doesn't light up using the library in Arduino IDE?](#how-to-debug-when-the-screen-doesnt-light-up-using-the-library-in-arduino-ide)
    - [How to fix the issue that log messages are missing or incomplete in Arduino IDE's Serial Monitor?](#how-to-fix-the-issue-that-log-messages-are-missing-or-incomplete-in-arduino-ides-serial-monitor)
    - [Solution for screen drift issue when using ESP32-S3 to drive RGB LCD in Arduino IDE](#solution-for-screen-drift-issue-when-using-esp32-s3-to-drive-rgb-lcd-in-arduino-ide)
    - [How to reduce Flash usage and speed up compilation when using ESP32\_Display\_Panel in Arduino IDE?](#how-to-reduce-flash-usage-and-speed-up-compilation-when-using-esp32_display_panel-in-arduino-ide)
    - [How to avoid I2C re-initialization when using ESP32\_Display\_Panel in Arduino IDE (e.g., when using Wire library)?](#how-to-avoid-i2c-re-initialization-when-using-esp32_display_panel-in-arduino-ide-eg-when-using-wire-library)

## Quick Start

### Environment Setup

1. **Install Arduino IDE**

- Download and install Arduino IDE from [Arduino's official website](https://www.arduino.cc/en/software), version 2.x is recommended

2. **Install ESP32 SDK**

- Open Arduino IDE
- Navigate to `File` > `Preferences`
- Add to `Additional boards manager URLs`:

  ```
  https://espressif.github.io/arduino-esp32/package_esp32_index.json
  ```

- Navigate to `Tools` > `Board` > `Boards Manager`
- Search for `esp32` by `Espressif Systems` and install the required version (see [SDK & Dependencies](#sdk--dependencies))

3. **Install Required Libraries**

- Navigate to `Sketch` > `Include Library` > `Manage Libraries`
- Search for `ESP32_Display_Panel` and its dependencies and install the required versions (see [SDK & Dependencies](#sdk--dependencies))
- For more information, see [Installing Libraries](#installing-libraries)

### Run First Example

1. **Select and Configure Board**

- Navigate to `Tools` > `Board` > `esp32`
- Select your board model. If you can't find a matching model, refer to:

  - If using a [supported board](../../README.md#supported-boards), see [Configuring Arduino IDE](#configuring-arduino-ide)
  - If using a custom board, select a generic board with the same chip series, like `ESP32S3 Dev Module`. Then set other configurations as needed.

2. **Open Example**

- Navigate to `File` > `Examples` > `ESP32_Display_Panel`
- Select `Arduino` > `board` > [`board_static_config`](../../examples/arduino/board/board_static_config/)

3. **Modify Code**

- If you're using a [supported board](../../README.md#supported-boards), modify the macro definitions in *esp_panel_board_supported_conf.h* to enable the target board. See [Loading of Supported Boards](#loading-of-supported-boards) for more information.
- If you're using a custom board, modify the macro definitions in *esp_panel_board_custom_conf.h* to configure board parameters. See [Loading of Custom Boards](#loading-of-custom-boards) for more information.

4. **Compile and Upload**

- Connect the board to your computer
- Select the correct serial port
- Click the upload button

### Next Steps

- Try other [example programs](#example-description)
- Learn detailed [configuration instructions](#configuration-guide)
- Learn how to [port UI](#porting-squareline-projects)
- Check [FAQ](#faq)

> [!NOTE]
> If you encounter any issues, please check the [FAQ](#faq) section first. If the problem persists, you can submit an issue on [GitHub](https://github.com/esp-arduino-libs/ESP32_Display_Panel/issues).

## SDK & Dependencies

Before using this library, ensure you have installed SDK and dependencies that meet the following version requirements:

|                           **SDK**                           | **Required Version** |
| ----------------------------------------------------------- | -------------------- |
| [arduino-esp32](https://github.com/espressif/arduino-esp32) | >= 3.1.0             |

|                              **Dependencies**                              | **Required Version** |
| -------------------------------------------------------------------------- | -------------------- |
| [ESP32_IO_Expander](https://github.com/esp-arduino-libs/ESP32_IO_Expander) | >= 1.0.0 && < 2.0.0  |
| [esp-lib-utils](https://github.com/esp-arduino-libs/esp-lib-utils)         | >= 0.2.0 && < 0.3.0  |

> [!NOTE]
> * For SDK installation, please refer to [Arduino ESP32 Documentation - Installing](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).
> * For dependencies installation, please refer to [Installing Libraries](#installing-libraries) section.

## Installing Libraries

ESP32_Display_Panel and its dependencies have been uploaded to the Arduino Library Manager. You can install them directly online following these steps:

1. In Arduino IDE, navigate to `Sketch` > `Include Library` > `Manage Libraries...`.
2. Search for `ESP32_Display_Panel` and its dependencies, click the `Install` button to install.

For manual installation, you can download the required version's `.zip` file from [Github](https://github.com/esp-arduino-libs/ESP32_Display_Panel) or [Arduino Library](https://www.arduinolibraries.info/libraries/esp32_display_panel), then in Arduino IDE navigate to `Sketch` > `Include Library` > `Add .ZIP Library...`, select the downloaded `.zip` file and click `Open` to install.

For more detailed library installation guides, please refer to [Arduino IDE v1.x.x](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries) or [Arduino IDE v2.x.x](https://docs.arduino.cc/software/ide-v2/tutorials/ide-v2-installing-a-library) documentation.

## Configuration Guide

Since Arduino IDE cannot adjust configurations through menuconfig like ESP-IDF or through compilation options like PlatformIO, this library provides configuration through modifying specific configuration files. This mainly includes three configuration files:

- [esp_panel_drivers_conf.h](../../esp_panel_drivers_conf.h)
- [esp_panel_board_supported_conf.h](../../esp_panel_board_supported_conf.h)
- [esp_panel_board_custom_conf.h](../../esp_panel_board_custom_conf.h)

Here are the characteristics of using configuration files:

1. ESP32_Display_Panel searches for configuration files in the following priority order: `Current project directory` > `Arduino library directory`. If a configuration file is found in the current project directory, it will be used first; otherwise, it will continue searching in the Arduino library directory. If no configuration file is found, the default configuration in the library will be used.
2. All example projects include their required configuration files by default, and you can directly modify the macro definitions to update configurations.
3. For custom projects without configuration files, you can copy the required configuration files from ESP32_Display_Panel's root directory or example projects.
4. If multiple projects need to use the same configuration, configuration files can be placed in the [Arduino library directory](#where-is-the-arduino-library-directory), so all projects without configuration files can share these configurations.

> [!WARNING]
> * The same directory can contain both *esp_panel_board_supported_conf.h* and *esp_panel_board_custom_conf.h* configuration files, but they cannot be enabled simultaneously. That is, `ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED` and `ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM` cannot both be set to `1`, otherwise it will cause compilation errors.
> * Since the content of configuration files may change with version updates (such as adding, deleting, or renaming configuration items), to ensure compatibility, this library manages configuration files with independent versioning and checks whether the your current configuration file version is compatible with the library during compilation. Detailed version information and checking rules can be found at the end of each configuration file.

Below are detailed instructions on how to configure ESP32_Display_Panel, mainly targeting three usage scenarios: [Adjusting Driver Configuration](#adjusting-driver-configuration), [Loading of Supported Boards](#loading-of-supported-boards), and [Loading of Custom Boards](#loading-of-custom-boards), all implemented by modifying their respective configuration files.

### Adjusting Driver Configuration

ESP32_Display_Panel adjusts the functionality and parameters of code in `esp_panel::drivers` based on the [esp_panel_drivers_conf.h](../../esp_panel_drivers_conf.h) configuration file. Please follow these steps to configure:

1. Refer to [Configuration Guide](#configuration-guide) to understand the configuration file search path.
2. Confirm that *esp_panel_drivers_conf.h* exists in either the `current project directory` or `Arduino library directory`. If not, copy the configuration file from ESP32_Display_Panel's root directory or example projects to either directory.
3. Modify the macro definitions in the configuration file to update driver behavior or default parameters. For example, to set the maximum number of touch points to `5`, here's part of the modified *esp_panel_drivers_conf.h* file:

    ```c
    ...
    /**
     * @brief Touch panel configuration parameters
    */
    #define ESP_PANEL_DRIVERS_TOUCH_MAX_POINTS              (5)    // Maximum number of touch points supported
    ...
    ```

> [!NOTE]
> By default, only some drivers are enabled in *esp_panel_drivers_conf.h*. If you want to dynamically load board configurations through code (like in the [board_dynamic_config](../../examples/arduino/board/board_dynamic_config/) example), please enable all the drivers you need first.

### Loading of Supported Boards

ESP32_Display_Panel sets the default board configuration in `esp_panel::board::Board` based on the [esp_panel_board_supported_conf.h](../../esp_panel_board_supported_conf.h) configuration file. Please follow these steps to configure:

1. Refer to [Configuration Guide](#configuration-guide) to understand the configuration file search path.
2. Confirm that *esp_panel_board_supported_conf.h* exists in either the `current project directory` or `Arduino library directory`. If not, copy the configuration file from ESP32_Display_Panel's root directory or example projects to either directory.
3. Set the `ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED` macro definition to `1` in the configuration file.
4. Uncomment the macro definition corresponding to the target board model.
5. Now, when calling the default constructor of `esp_panel::board::Board`, it will load the target board's configuration.
6. For example, to use the `ESP32-S3-BOX-3` board, here's part of the modified *esp_panel_board_supported_conf.h* file:

    ```c
    ...
    /**
    * @brief Flag to enable supported board configuration (0/1)
    *
    * Set to `1` to enable supported board configuration, `0` to disable
    */
    #define ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED       (1)
    ...
    // #define BOARD_ESPRESSIF_ESP32_C3_LCDKIT
    // #define BOARD_ESPRESSIF_ESP32_S3_BOX
    #define BOARD_ESPRESSIF_ESP32_S3_BOX_3
    // #define BOARD_ESPRESSIF_ESP32_S3_BOX_3_BETA
    ...
    ```

### Loading of Custom Boards

ESP32_Display_Panel sets the default board configuration in `esp_panel::board::Board` based on the [esp_panel_board_custom_conf.h](../../esp_panel_board_custom_conf.h) configuration file. Please follow these steps to configure:

1. Refer to [Configuration Guide](#configuration-guide) to understand the configuration file search path.
2. Confirm that *esp_panel_board_custom_conf.h* exists in either the `current project directory` or `Arduino library directory`. If not, copy the configuration file from ESP32_Display_Panel's root directory or example projects to either directory.
3. Set the `ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM` macro definition to `1` in the configuration file.
4. Set other macro definitions according to the actual hardware configuration of the target board.
5. Now, when calling the default constructor of `esp_panel::board::Board`, it will load the custom board's configuration.
6. For example, to use a custom board with `480x480 RGB ST7701 LCD + I2C GT911 Touch`, here's part of the modified *esp_panel_board_custom_conf.h* file:

    ```c
    ...
    #define ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM  (1)
    ...
    /**
     * @brief Board name (format: "Manufacturer:Model")
    */
    #define ESP_PANEL_BOARD_NAME                "Custom:Custom"

    /**
     * @brief Panel resolution configuration in pixels
    */
    #define ESP_PANEL_BOARD_WIDTH               (480)   // Panel width (horizontal, in pixels)
    #define ESP_PANEL_BOARD_HEIGHT              (480)   // Panel height (vertical, in pixels)
    ...
    /**
     * @brief LCD panel configuration flag (0/1)
    *
    * Set to `1` to enable LCD panel support, `0` to disable
    */
    #define ESP_PANEL_BOARD_USE_LCD             (1)
    ...
    #define ESP_PANEL_BOARD_LCD_CONTROLLER      ST7701
    ...
    #define ESP_PANEL_BOARD_LCD_BUS_TYPE        (ESP_PANEL_BUS_TYPE_RGB)
    ...
        /**
         * @brief RGB bus
        */
        /**
         * Set to 0 if using simple "RGB" interface which does not contain "3-wire SPI" interface.
        */
        #define ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL       (1) // 0/1. Typically set to 1
    ...
    /**
     * @brief LCD vendor initialization commands
    *
    * Vendor specific initialization can be different between manufacturers, should consult the LCD supplier for
    * initialization sequence code. Please uncomment and change the following macro definitions. Otherwise, the LCD driver
    * will use the default initialization sequence code.
    *
    * The initialization sequence can be specified in two formats:
    * 1. Raw format:
    *    {command, (uint8_t []){data0, data1, ...}, data_size, delay_ms}
    * 2. Helper macros:
    *    - ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(delay_ms, command, {data0, data1, ...})
    *    - ESP_PANEL_LCD_CMD_WITH_NONE_PARAM(delay_ms, command)
    */
    #define ESP_PANEL_BOARD_LCD_VENDOR_INIT_CMD()                       \
        {                                                               \
            ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xFF, {0x77, 0x01, 0x00, 0x00, 0x10}), \
            ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC0, {0x3B, 0x00}),                   \
            ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC1, {0x0D, 0x02}),                   \
            ESP_PANEL_LCD_CMD_WITH_NONE_PARAM(120, 0x29),                               \
        }
    ...
    /**
     * @brief Touch panel configuration flag (0/1)
    *
    * Set to `1` to enable touch panel support, `0` to disable
    */
    #define ESP_PANEL_BOARD_USE_TOUCH               (1)
    ...
    #define ESP_PANEL_BOARD_TOUCH_CONTROLLER        GT911
    ...
    #define ESP_PANEL_BOARD_TOUCH_BUS_TYPE          (ESP_PANEL_BUS_TYPE_I2C)
    ...
    ```

## Example Description

You can access all examples in Arduino IDE through `File` > `Examples` > `ESP32_Display_Panel`.

> [!NOTE]
> If you don't see the `ESP32_Display_Panel` option, please check:
> * Whether ESP32_Display_Panel library is properly installed
> * Whether SDK is properly installed
> * Whether an ESP board is selected

### Drivers

The following examples demonstrate how to use `esp_panel::drivers::LCD` to drive LCD controllers with different interfaces and models, and test them by displaying color bars:

* [SPI LCD](../../examples/arduino/drivers/lcd/lcd_spi/)
* [QSPI LCD](../../examples/arduino/drivers/lcd/lcd_qspi/)
* [Single RGB LCD](../../examples/arduino/drivers/lcd/lcd_single_rgb/)
* [3-wire SPI + RGB LCD](../../examples/arduino/drivers/lcd/lcd_3wire_spi_rgb/)
* [MIPI-DSI LCD](../../examples/arduino/drivers/lcd/lcd_mipi_dsi/)

The following examples demonstrate how to use `esp_panel::drivers::Touch` to drive touch controllers with different interfaces and models, and test them by printing touch point coordinates:

* [I2C Touch](../../examples/arduino/drivers/touch/touch_i2c/)
* [SPI Touch](../../examples/arduino/drivers/touch/touch_spi/)

### Board

The following examples demonstrate how to use `esp_panel::board::Board` to drive the screen of built-in development boards or custom boards in one stop:

* [Board Dynamic Config](../../examples/arduino/board/board_dynamic_config/): This example demonstrates how to dynamically load the display screen settings of development boards through code, and verify the configuration by displaying color bars and monitoring touch coordinates.
* [Board Static Config](../../examples/arduino/board/board_static_config/): This example demonstrates how to statically load the display screen settings of development boards through `esp_panel_board_supported_conf.h` and `esp_panel_board_custom_conf.h` configuration files, and verify the configuration by displaying color bars and monitoring touch coordinates.

### GUI

The following examples demonstrate how to develop GUI interfaces using `LVGL v8` version:

* [Simple Port](../../examples/arduino/gui/lvgl_v8/simple_port/): This example demonstrates how to port `LVGL v8`. And for `RGB/MIPI-DSI` interfaces, it can enable the avoid tearing and rotation function.
* [Simple Rotation](../../examples/arduino/gui/lvgl_v8/simple_rotation/): This example demonstrates how to rotate the display by using the `LVGL v8`.
* [SquareLine Port](../../examples/arduino/gui/lvgl_v8/squareline_port/): This example demonstrates how to port `SquareLine (v1.4.x)` project. And for `RGB/MIPI-DSI` interfaces, it can enable the avoid tearing and rotation function.
* [SquareLine Wi-Fi Clock](../../examples/arduino/gui/lvgl_v8/squareline_wifi_clock/): This example implements a simple Wi-Fi clock demo, which UI is created by Squareline Studio.

> [!NOTE]
> * When using the above examples, please ensure you have installed the `LVGL` library that meets the version requirements.
> * For how to configure `LVGL` (v8.4.x), please refer to [Configuring LVGL](#configuring-lvgl) for more details.
> * For how to port `SquareLine` (v1.4.x) projects, please refer to [Porting SquareLine Projects](#porting-squareline-projects) for more details.

> [!WARNING]
> Currently, the anti-tearing feature of `LVGL` only supports `RGB/MIPI-DSI` LCD and requires its version to be `>= v8.3.9`. If you are using other types of LCD or a `LVGL` version that doesn't meet the requirements, please do not enable this feature.

## Additional Information

### Configuring esp-lib-utils

ESP32_Display_Panel depends on the `esp-lib-utils` library, using its `logging`, `memory allocation`, and `checking` features. The `esp-lib-utils` library also adopts adjusting library behavior and parameters by modifying the specific configuration file [esp_utils_conf.h](../../template_files/esp_utils_conf.h). You can refer to [Adjusting Driver Configuration](#adjusting-driver-configuration) for modification.

For example, to set the `logging` level to `DEBUG` and enable the `function tracing` feature, here's part of the modified *esp_utils_conf.h* file:

```c
...
/**
 * Global log level, logs with a level lower than this will not be compiled. Choose one of the following:
 *  - ESP_UTILS_LOG_LEVEL_DEBUG:   Extra information which is not necessary for normal use (values, pointers, sizes, etc)
 *                                 (lowest level)
 *  - ESP_UTILS_LOG_LEVEL_INFO:    Information messages which describe the normal flow of events
 *  - ESP_UTILS_LOG_LEVEL_WARNING: Error conditions from which recovery measures have been taken
 *  - ESP_UTILS_LOG_LEVEL_ERROR:   Critical errors, software module cannot recover on its own
 *  - ESP_UTILS_LOG_LEVEL_NONE:    No log output (highest level) (Minimum code size)
 */
#define ESP_UTILS_CONF_LOG_LEVEL                            (ESP_UTILS_LOG_LEVEL_DEBUG)
...
    /**
     * @brief Set to 1 if print trace log messages when enter/exit functions, useful for debugging
     */
    #define ESP_UTILS_CONF_ENABLE_LOG_TRACE                 (1)
...
```

### Configuring Arduino IDE

If your are using a custom board, please select a generic board with the same chip series, like `ESP32S3 Dev Module`. Then set other configurations as needed.

If you are using a [supported board](../../README.md#supported-boards), here are the recommended configuration guides provided by board manufacturers:

- [Espressif](../../docs/board/board_espressif.md#arduino-ide)
- [M5Stack](../../docs/board/board_m5stack.md#arduino-ide)
- [Elecrow](../../docs/board/board_elecrow.md#arduino-ide)
- [Jingcai](../../docs/board/board_jingcai.md#arduino-ide)
- [Waveshare](../../docs/board/board_waveshare.md#arduino-ide)
- [VIEWE](../../docs/board/board_viewe.md#arduino-ide)

### Configuring LVGL

LVGL's features and parameters can be adjusted by modifying the [lv_conf.h](../../template_files/lv_conf.h) configuration file. Here's how to configure LVGL:

1. **Configuration File Search Rules**

- When using arduino-esp32 v3 version, LVGL searches for configuration files in priority order: `Current project directory` > `Arduino library directory`
- If no configuration file is found, a warning will be shown during compilation
- Please ensure at least one directory contains the *lv_conf.h* file

2. **Configuration File Usage**

- Single project configuration: Place the configuration file in the project directory
- Multiple projects sharing configuration: Place the configuration file in the [Arduino library directory](#where-is-the-arduino-library-directory)

3. **Configuration Steps**

    a. Get configuration file template

    - Navigate to [Arduino library directory](#where-is-the-arduino-library-directory)
    - Enter the *lvgl* folder
    - Copy *lv_conf_template.h* to target directory and rename it to *lv_conf.h*

    b. Enable configuration file

    - Open *lv_conf.h*
    - Change the first `#if 0` to `#if 1`

    c. Common configuration items

      ```c
      // Color configuration
      #define LV_COLOR_DEPTH          16    // Usually use 16-bit color depth (RGB565)
                                              // Set to 32 to support 24-bit color depth with alpha (ARGB8888)
      #define LV_COLOR_16_SWAP        0     // SPI/QSPI LCD (like ESP32-C3-LCDkit) needs to set to 1
      #define LV_COLOR_SCREEN_TRANSP  1     // LCD with alpha needs to set to 1
      // Memory configuration
      #define LV_MEM_CUSTOM           1     // Use malloc/free for better performance
      #define LV_MEMCPY_MEMSET_STD    1     // Use standard library functions
      // Resource configuration
      #define LV_FONT_MONTSERRAT_N    1     // Enable required built-in fonts (replace N with font size)
      // Debug configuration
      #define LV_USE_PERF_MONITOR     1     // Display CPU usage and FPS
      #define LV_USE_LOG              1     // Enable logging feature
      #define LV_LOG_PRINTF           1     // Use printf for log output
      // Other configuration
      #define LV_ATTRIBUTE_FAST_MEM   IRAM_ATTR  // Improve performance but will use more SRAM
      ```

4. For more information, please refer to [LVGL Official Documentation](https://docs.lvgl.io/8.4/get-started/platforms/arduino.html)

### Porting SquareLine Projects

`SquareLine Studio (v1.4.x)` provides a graphical interface editor tool for quickly designing beautiful UIs. To use SquareLine exported UI source files in Arduino IDE, please follow these steps:

1. Create New Project

- Open SquareLine Studio
- Go to `Create` > `Arduino`
- Select `Arduino with TFT-eSPI` template
- Configure LCD parameters (like resolution, color depth, etc.) in `PROJECT SETTINGS`
- Click `Create` to create project

2. Configure Existing Project

- Click `File` > `Project Settings`
- In `BOARD PROPERTIES` set:

  - `Board Group`: `Arduino`
  - `Board`: `Arduino with TFT-eSPI`

- Configure LCD parameters in `DISPLAY PROPERTIES`
- Click `Save` to save settings

3. Export Project

- Complete UI design and configure export path
- Click `Export` > `Create Template Project` and `Export UI Files` in sequence
- The exported project directory structure is as follows:

    ```
    Project
    ├── libraries
    │   ├── lv_conf.h
    │   ├── lvgl
    │   ├── readme.txt
    │   ├── TFT_eSPI
    │   └── ui
    ├── README.md
    └── ui
    ```

4. Configure Arduino Libraries

- Copy *lv_conf.h*, *lvgl*, and *ui* from the *libraries* folder to Arduino library directory
- If using locally installed LVGL, skip copying *lvgl* and *lv_conf.h* and refer to [Configuring LVGL](#configuring-lvgl) for setup
- Example Arduino library directory structure:

  ```
  Arduino
  └── libraries
      ├── ESP32_Display_Panel
      ├── esp_panel_drivers_conf.h (optional)
      ├── esp_panel_board_supported_conf.h (optional)
      ├── esp_panel_board_custom_conf.h (optional)
      ├── lv_conf.h (optional)
      ├── lvgl
      ├── ui
      ├── other_lib_1
      └── other_lib_2
  ```

## FAQ

### Where is the Arduino library directory?

You can find and modify the Arduino library directory path in Arduino IDE by selecting `File` > `Preferences` > `Settings` > `Sketchbook location`.

### Where are the arduino-esp32 installation directory and SDK directory?

The default installation path for arduino-esp32 depends on your operating system:

- Windows: `C:\You\<user name>\AppData\Local\Arduino15\packages\esp32`
- Linux: `~/.arduino15/packages/esp32`
- macOS: `~/Library/Arduino15/packages/esp32`

For arduino-esp32 v3.x version, the SDK is located in the `tools > esp32-arduino-libs > idf-release_x` directory under the default installation path.

### How to install ESP32_Display_Panel in Arduino IDE?

Please refer to [Installing Libraries](#installing-libraries).

### How to select and configure supported boards in Arduino IDE?

Please refer to [Configuring Arduino IDE](#configuring-arduino-ide).

### How to use SquareLine exported UI source files in Arduino IDE?

Please refer to [Porting SquareLine Projects](#porting-squareline-projects).

### How to debug when the screen doesn't light up using the library in Arduino IDE?

Please follow these steps to troubleshoot:

1. Refer to [Configuring esp-lib-utils](#configuring-esp-lib-utils) to set the logging level of the `esp-lib-utils` library to `DEBUG` and enable function tracing.
2. In Arduino IDE, set: `Tools` > `Core Debug Level` to `Debug` or lower level, then recompile and upload the code.
3. Check detailed log information through the serial monitor to analyze the problem.
4. If the problem still cannot be solved through the above steps, please submit an issue report on [GitHub Issues](https://github.com/esp-arduino-libs/ESP32_Display_Panel/issues) with complete log information.

### How to fix the issue that log messages are missing or incomplete in Arduino IDE's Serial Monitor?

Please follow these steps to resolve:

1. Check if `Tools` > `Port` is set correctly in Arduino IDE
2. Check if `Tools` > `Core Debug Level` is set to the desired level, such as `Info` or lower
3. Check if `Tools` > `USB CDC On Boot` is set correctly in Arduino IDE - set to `Disabled` when ESP32 is connected via `UART` port, or `Enabled` when connected via `USB` port. After changing this setting, enable the `Erase All Flash Before Sketch Upload` option and reflash.
4. Check if the baud rate in Arduino IDE's Serial Monitor is set correctly, such as `115200`

### Solution for screen drift issue when using ESP32-S3 to drive RGB LCD in Arduino IDE

Please follow these steps to resolve:

1. **Understand the Issue**

    * Please refer to [ESP32-S3 RGB LCD Screen Drift Issue Description](https://docs.espressif.com/projects/esp-faq/en/latest/software-framework/peripherals/lcd.html#why-do-i-get-drift-overall-drift-of-the-display-when-esp32-s3-is-driving-an-rgb-lcd-screen)

2. **Enable `RGB LCD Bounce Buffer + XIP on PSRAM` Feature**

    a. **Update SDK to Enable `XIP on PSRAM` Function**

    - Download `high_perf` version from [arduino-esp32-sdk](https://github.com/esp-arduino-libs/arduino-esp32-sdk)
    - Replace it in [arduino-esp32 installation directory](#where-are-the-arduino-esp32-installation-directory-and-sdk-directory) according to the [documentation](https://github.com/esp-arduino-libs/arduino-esp32-sdk#how-to-use)

    b. **Configure `RGB LCD Bounce Buffer`**

    - For supported boards:

        - Usually `ESP_PANEL_BOARD_LCD_RGB_BOUNCE_BUF_SIZE` is already configured to `(ESP_PANEL_BOARD_WIDTH * 10)` by default
        - If the issue persists, increase the buffer size by referring to the example code

    - For custom boards:

        - Set `ESP_PANEL_BOARD_LCD_RGB_BOUNCE_BUF_SIZE` in *esp_panel_board_custom_conf.h*
        - If the issue persists, increase the buffer size by referring to the example

    c. **Configure LVGL Task**

    - If using LVGL, setting the task that executes `lv_timer_handler()` to run on the same core as the task that executes `board->begin()` can help mitigate the screen drift issue

3. **Example Code**

    a. Modifying `Bounce Buffer` size when using a board.

    ```c
    ...
    esp_panel::board::Board *board = new esp_panel::board::Board();
    board->init();
    ...
    /**
     * 1. Should be called after `board->init()` and before `board->begin()`
     * 2. `ESP_PANEL_BOARD_WIDTH` should be replaced with the actual width of the LCD
     */
    auto bus = static_cast<esp_panel::drivers::BusRGB *>(board->getLCD()->getBus());
    bus->configRGB_BounceBufferSize(ESP_PANEL_BOARD_WIDTH * 20);
    ...
    board->begin();
    ...
    ```

    b. Modifying `Bounce Buffer` size when using standalone drivers.

    ```c
    ...
    esp_panel::drivers::BusRGB *bus = new esp_panel::drivers::BusRGB(...);
    ...
    /**
     * 1. Should be called before `bus->init()`
     * 2. `EXAMPLE_LCD_WIDTH` should be replaced with the actual width of the LCD
     */
    bus->configRGB_BounceBufferSize(EXAMPLE_LCD_WIDTH * 20);
    ...
    bus->init();
    ...
    ```

### How to reduce Flash usage and speed up compilation when using ESP32_Display_Panel in Arduino IDE?

Please follow these steps:

1. **Disable unused drivers**:

- Only enable required drivers in `esp_panel_drivers_conf.h`
- For example, when `RGB LCD` driver is not needed:

    ```c
    ...
    #define ESP_PANEL_DRIVERS_BUS_USE_ALL    (0)
    ...
    #define ESP_PANEL_DRIVERS_BUS_USE_RGB    (0)
    ```

- For detailed configuration, please refer to [Adjusting Driver Configuration](#adjusting-driver-configuration)

2. **Turn off debug logs**:

- Set in `esp_utils_conf.h`:

    ```c
    #define ESP_UTILS_CONF_LOG_LEVEL    ESP_UTILS_LOG_LEVEL_NONE
    ```

- For detailed configuration, please refer to [Configuring esp-lib-utils](#configuring-esp-lib-utils)

### How to avoid I2C re-initialization when using ESP32_Display_Panel in Arduino IDE (e.g., when using Wire library)?

If you need to use ESP32_Display_Panel's I2C bus functionality, such as `I2C IO_Expander` or `I2C Touch` drivers, while your project is already using the `Wire` library, errors may occur due to I2C bus re-initialization. To avoid this, initialize `Wire` before creating the `Board` object, then follow these steps to configure ESP32_Display_Panel to skip I2C initialization:

1. By modifying the *esp_panel_board_custom_conf.h* configuration file (only applicable for [custom boards](#loading-of-custom-boards)):

- For `I2C Touch` driver:

    ```c
    ...
    #define ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST    (1)     // 0/1
    ...
    ```

- For `I2C IO_Expander` driver:

    ```c
    ...
    #define ESP_PANEL_BOARD_EXPANDER_SKIP_INIT_HOST     (1)     // 0/1
    ...
    ```

2. By using code (applicable for both [supported boards](#loading-of-supported-boards) and [custom boards](#loading-of-custom-boards)):

    ```c
    ...
    esp_panel::board::Board *board = new esp_panel::board::Board();
    board->init();
    ...
    /**
     * Should be called after `board->init()` and before `board->begin()`
     */
    // For I2C Touch
    static_cast<esp_panel::drivers::BusI2C *>(board->getTouch()->getBus())->configI2C_HostSkipInit();
    // For I2C IO_Expander
    board->getIO_Expander()->skipInitHost();
    ...
    board->begin();
    ...
    ```
