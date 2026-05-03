# 在 ESP-IDF 下使用

* [English Version](./use_with_idf.md)

## 目录

- [在 ESP-IDF 下使用](#在-esp-idf-下使用)
  - [目录](#目录)
  - [SDK 及依赖组件](#sdk-及依赖组件)
  - [添加到工程](#添加到工程)
  - [配置说明](#配置说明)
  - [示例说明](#示例说明)
  - [常见问题及解答](#常见问题及解答)
    - [在 ESP-IDF 中使用 ESP32-S3 驱动 RGB LCD 时出现画面漂移问题的解决方案](#在-esp-idf-中使用-esp32-s3-驱动-rgb-lcd-时出现画面漂移问题的解决方案)
    - [在 ESP-IDF 中使用 ESP32\_Display\_Panel 时，如何降低其 Flash 占用及加快编译速度？](#在-esp-idf-中使用-esp32_display_panel-时如何降低其-flash-占用及加快编译速度)
    - [在 ESP-IDF 中驱动 LCD 遇到其他问题](#在-esp-idf-中驱动-lcd-遇到其他问题)

## SDK 及依赖组件

在使用本库之前，请确保已安装符合以下版本要求的 SDK：

|                     **SDK**                     | **版本要求** |
| ----------------------------------------------- | ------------ |
| [esp-idf](https://github.com/espressif/esp-idf) | >= 5.1       |

|                                         **依赖组件**                                         | **版本要求** |
| -------------------------------------------------------------------------------------------- | ------------ |
| [esp32_io_expander](https://components.espressif.com/components/espressif/esp32_io_expander) | 1.*          |
| [esp-lib-utils](https://components.espressif.com/components/espressif/esp-lib-utils)         | 0.2.*        |

> [!NOTE]
> * SDK 的安装方法请参阅 [ESP-IDF 编程指南 - 安装](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html#get-started-how-to-get-esp-idf)
> * 依赖组件无需手动下载，工程添加本库后在编译时会自动下载

## 添加到工程

ESP32_Display_Panel 已上传到 [Espressif 组件库](https://components.espressif.com/)，您可以通过以下方式将其添加到工程中：

1. **使用命令行**

    在工程目录下运行以下命令：

   ```bash
   idf.py add-dependency "espressif/esp32_display_panel==1.*"
   ```

2. **修改配置文件**

   在工程目录下创建或修改 *idf_component.yml* 文件：

   ```yaml
   dependencies:
     espressif/esp32_display_panel: "1.*"
   ```

详细说明请参阅 [Espressif 文档 - IDF 组件管理器](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-guides/tools/idf-component-manager.html)。

## 配置说明

在使用 ESP-IDF 开发时，您可以通过修改 `menuconfig` 来配置 ESP32_Display_Panel，配置步骤如下：

1. 运行命令 `idf.py menuconfig`。
2. 导航到 `Component config` > `ESP Display Panel Configurations`。

除此之外，由于 ESP32_Display_Panel 依赖于 `esp-lib-utils` 组件，您也可以通过配置 `esp-lib-utils` 组件来影响 ESP32_Display_Panel 代码的行为，如开启调试信息等。配置步骤如下：

1. 运行命令 `idf.py menuconfig`。
2. 导航到 `Component config` > `ESP Library Utils Configurations`。

> [!TIP]
> * 运行 `idf.py save-defconfig` 即可保存当前工程的 `menuconfig` 配置，并在工程目录下生成 *sdkconfig.defaults* 默认配置文件。
> * 如果想要舍弃当前工程 menuconfig 配置并加载默认配置，请先删除工程目录下的 *sdkconfig* 文件，然后再运行 `idf.py reconfigure` 即可。

## 示例说明

* [lvgl_v8_port](../../examples/esp_idf/lvgl_v8_port/): 该示例演示了如何移植 `LVGL v8`，并且运行了 LVGL 的内部 Demos，包括 `Music Player`、`Widgets`、`Stress` 和 `Benchmark`。

## 常见问题及解答

### 在 ESP-IDF 中使用 ESP32-S3 驱动 RGB LCD 时出现画面漂移问题的解决方案

请按照以下步骤解决：

1. **了解问题**

    * 请参阅 [ESP32-S3 RGB LCD 画面漂移问题说明](https://docs.espressif.com/projects/esp-faq/zh_CN/latest/software-framework/peripherals/lcd.html#esp32-s3-rgb-lcd)

2. **启用 `RGB LCD Bounce Buffer + XIP on PSRAM` 特性**

    a. **使能 `ESP-IDF` 的 `XIP on PSRAM` 功能**

    - 对于 `ESP-IDF` 版本 `>= 5.1 && < 5.3` 的情况，请在 `menuconfig` 中使能 `SPIRAM_FETCH_INSTRUCTIONS` 和 `SPIRAM_RODATA` 选项
    - 对于 `ESP-IDF` 版本 `>= 5.3` 的情况，请在 `menuconfig` 中使能 `SPIRAM_XIP_FROM_PSRAM` 选项

    b. **配置 `RGB LCD Bounce Buffer`**

    - 对于支持的开发板：

        * 通常已默认配置 `ESP_PANEL_BOARD_LCD_RGB_BOUNCE_BUF_SIZE` 为 `(ESP_PANEL_BOARD_WIDTH * 10)`
        * 如果问题仍存在,可参考示例代码增大缓冲区

    - 对于自定义开发板：

        * 在 *esp_panel_board_custom_conf.h* 中设置 `ESP_PANEL_BOARD_LCD_RGB_BOUNCE_BUF_SIZE`
        * 如果问题仍存在,可参考示例代码增大缓冲区

    c. **配置 LVGL 任务**

    - 如果使用 LVGL，设置执行 `lv_timer_handler()` 的任务与执行 `board->begin()` 的任务在同一个核心上运行可以缓解画面漂移问题

3. **示例代码**

    a. 使用开发板时修改 `Bounce Buffer` 大小。

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

    b. 使用独立驱动时修改 `Bounce Buffer` 大小。

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

### 在 ESP-IDF 中使用 ESP32_Display_Panel 时，如何降低其 Flash 占用及加快编译速度？

请参考以下步骤实现：

1. **禁用不需要的驱动**： 通过 `menuconfig` 禁用 `ESP Display Panel Configurations - Drivers` 中不需要的驱动
2. **关闭调试日志**： 通过 `menuconfig` 设置 `ESP Library Utils Configurations - Log functions` 中 `Log level` 为 `None`
3. 详细配置方法请参阅 [配置说明](#配置说明)

### 在 ESP-IDF 中驱动 LCD 遇到其他问题

请参阅 [ESP-FAQ - LCD](https://docs.espressif.com/projects/esp-faq/zh_CN/latest/software-framework/peripherals/lcd.html)
