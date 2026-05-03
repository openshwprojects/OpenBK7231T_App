# ChangeLog

## v1.0.2 - 2025-04-23

### Enhancements:

* feat(backlight): add PWM parameters configuration (#188)

### Bugfixes:

* fix(io_expander): remove incompatible header (#175)
* fix(pre-commit): update the pre-commit script
* fix(docs): update README
* fix(conf): update comments of custom config file
* fix(examples): enable CPU 240M and task WDT default in esp_idf/lvgl_v8_port
* fix(lcd): fix ST7701 mirror issue

## v1.0.1 - 2025-03-13

### Enhancements:

* feat(docs): add new FAQs

### Bugfixes:

* fix(touch): avoid reading the button state when disabled (#162)
* fix(docs): correct and add descriptions
* fix(board): resolve compilation error for SPI touch screens @tilordleo (#169)
* fix(conf): correct comments in `esp_panel_board_custom_conf.h`

## v1.0.0 - 2025-02-17

### Breaking changes:

* Rename configuration files to follow consistent naming convention:

  * `ESP_Panel_Board_Custom.h` -> `esp_panel_board_custom_conf.h`
  * `ESP_Panel_Board_Support.h` -> `esp_panel_board_supported_conf.h`
  * `ESP_Panel_Conf.h` -> `esp_panel_drivers_conf.h`

* Modernize codebase organization:

  * Add namespaces for better code organization:

    * `esp_panel::utils` - Utility functions and classes
    * `esp_panel::drivers` - Device drivers
    * `esp_panel::board` - Board driver

  * Deprecate legacy `ESP_Panel*` class names in favor of namespaced versions

* Add external dependency on `esp-lib-utils` library
* Add support for dynamic board configuration loading at runtime

### Enhancements:

* feat(examples): update PlatformIO build_flags
* feat(touch): add touch controller CHSC6540 @VIEWESMART (#128)
* feat(board): add various viewe boards @VIEWESMART (#128)
* feat(docs): update FAQ @VIEWESMART (#128)
* feat(repo): refactor with using esp-lib-utils
* feat(repo): support build on the MicroPython
* feat(lcd): add LCD controller AXS15231B, HX8399, JD9165, ST7703, ST77903(RGB)
* feat(touch): add touch controller AXS15231B, STMPE610, SPD2010
* feat(backlight): add backlight device Custom, SwitchExpander
* feat(board): add board Jingcai:JC8048W550C @lsroka76 (#132)

### Bugfixes:

* fix(touch): fix GT911 build warning
* fix(log): fix kernel panic when checking the error @Kanzll (#144)
* fix(version): fix minor number check @arduinomnomnom (#148)

## v0.2.2 - 2025-01-09

### Bugfixes:

* fix(lcd): use 'delete[]' instead of 'delete' for C array shared pointer @FranciscoMoya (#142)
* fix(lcd): load vendor config from bus
* fix(board): fix GT911 init error for waveshare boards
* fix(Kconfig): fix build error on esp-idf and incorrect descriptions @Cathgao (#133)
* fix(examples): update PlatformIO lib & platform URLs

## v0.2.1 - 2024-11-14

### Enhancements:

* feat(lcd): add LCD controller JD9365 @Y1hsiaochunnn (#123)
* feat(board): add board Waveshare ESP32-P4-NANO @Y1hsiaochunnn (#123)
* feat(board): add board Waveshare ESP32-S3-Touch-LCD-4.3B/5/5B/7 @H-sw123 (#124)
* feat(board): add configuration for ignoring board in Kconfig
* feat(ci): use finer-grained file modification jobs

### Bugfixes:

* fix(bus & lcd): update RGB conf based on esp-idf v5.4

## v0.2.0 - 2024-11-08

### Enhancements:

* feat(repo): support build on the esp-idf
* feat(bus & lcd): support MIPI-DSI LCD
* feat(lcd): add LCD controller EK79007
* feat(lcd): add LCD controller ILI9881C
* feat(panel): add support for MIPI-DSI LCD
* feat(board): add support for Waveshare ESP32-S3-Touch-LCD-2.1 @martinroger (#117)
* feat(board): add support for Espressif ESP32-P4-Function-EV-Board
* feat(examples): add MIPI-DSI LCD
* feat(examples): optimize anti-tear rotation in lvgl_v8_port
* feat(ci): update for MIPI-DSI LCD
* feat(test_apps): add MIPI-DSI LCD

### Bugfixes:

* fix(touch): release ISR semaphore when delete

## v0.1.8 - 2024-10-25

### Enhancements:

* feat(board): add support for Waveshare ESP32-S3-Touch-LCD-1.85 @martinroger (#115)
* feat(docs): add additional information about screen drift issue

### Bugfixes:

* fix(examples): correct readme broken links

## v0.1.6 - 2024-07-30

### Enhancements:

* feat(board): add support for Fitipower EK9716B LCD controller for CrowPanel 7.0" board by @lboue (#78)
* feat(board): add support for Waveshare ESP32-S3-Touch-LCD-4.3 by @lboue (#99)

### Bugfixes:

* fix(examples): fix `LVGL_PORT_ROTATION_DEGREE` issue by @lboue (#76)
* fix(examples): fix issue with I2C.ino `EXAMPLE_TOUCH_ADDRESS` missing as variable by @lboue (#84)
* fix(examples): fix WiFiClock wrong name `ScreenPassord` by @lboue (#82)
* fix(examples): fix LCD using `configVendorCommands()` before `init()`
* fix(examples): fix `LV_USE_DEMO_WIDGETS` typo by @lboue (#98)
* fix(examples): fix `Tearing function` typo by @lboue (#96)
* fix(examples): fix WiFiClock log HTTP error code to serial console by @lboue (#97)
* fix(examples): fix WiFiClock description
* fix(gt911): allow to set the GT911 touch device address by @lboue (#86)
* fix(conf): fix the issue that the `ESP_PANEL_BOARD_EXPANDER_I2C_HOST_ID` flag is not working properly
* fix(conf): fix `LCD Venbdor` typo (#92)

## v0.1.5 - 2024-07-09

### Enhancements:

* feat(gt911): support set I2C address by using RST and INT pins
* feat(lvgl_port): set the lvgl task to run on the same core as the Arduino task by default
* feat(board): increase the RGB pclk frequency to 26MHz for `ESP32_4848S040C_I_Y_3`
* feat(board): add new board `elecrow: CROWPANEL_7_0` by @lboue (#71)
* feat(conf): add connection comments for the RGB pins in *esp_panel_board_custom.h* (#58, #68)

### Bugfixes:

* fix(panel): init expander host with correct macro (#65)
* fix(panel): don't reset the LCD if the bus is RGB bus and the `ESP_PANEL_BOARD_LCD_FLAGS_ENABLE_IO_MULTIPLEX` is enabled
* fix(examples): fix lvgl port rotation issue when enabling avoid tearing by @NecroMancer05
* fix(pre-commit): switch to Python 3 for pre-commit @lboue (#70)
* fix(docs): specify lvgl version >= v8.3.9 and < 9
* fix(docs): update board ESP32-S3-BOX-3 & ESP32-S3-BOX-3B

## v0.1.4 - 2024-06-17

### Enhancements:

* feat(docs): add resolution column in board instructions by @lboue (#51)

### Bugfixes:

* fix(conf): fix error when include inside file (#52)
* fix(docs): switch M5Stack doc links to english by @lboue (#50)
* fix(board): fix m5stack coreS3 power issue (#54)

## v0.1.3 - 2024-06-14

### Enhancements:

* feat(board): add add new board M5CORE2 by @MacChu0315-Espressif (#40)
* feat(board): add add new board M5DIAL by @MacChu0315-Espressif (#41)
* feat(board): add add new board M5CORES3 by @MacChu0315-Espressif (#45)
* feat(example): add support for PlatformIO by @isthaison (#37)

### Bugfixes:

* fix(version): fix version mismatch
* fix(docs): update files related to version and board M5CORE2

## v0.1.2 - 2024-06-01

### Enhancements:

* feat(config): add version control for all configuration files by @lzw655 (#32)
* feat(touch): add i2c st1633 by @lzw655 (#22)
* feat(pre-commit): support to check file versions

### Bugfixes:

* fix(docs): fix broken links in 'Panel Test Example' README by @lboue (#27)
* fix(config): fix wrong header order by @lzw655 (#35)

## v0.1.1 - 2024-05-16

### Enhancements:

* feat(touch): add spi xpt2046 by @Lzw655 (#21)
* feat(config): add new IO expander CH422G

### Bugfixes:

* fix some typo by @Franck78 (#16, #17)
* fix(docs): add more details on the version by @lboue (#23)

## v0.1.0 - 2024-03-07

### Breaking changes:

* Restructure the driver framework based on **arduino-esp32 v3** version and is not compatible with the v2 version
* Add and modify some APIs for `bus`, `LCD` and `touch` object classes
* Temporarily remove `Kconfig` and `test_apps`, which are used for ESP-IDF. Remove action `build_test`.
* Support using independent drivers instead of the entire `ESP_Panel`. In this case, users don't need to enter the *ESP32_Display_Panel* folder and copy `ESP_Panel_Conf_Template.h`.

### Enhancements:

* Add new bus type: `QSPI`
* Add new LCD controllers: `GC9B71`, `NV3022B`, `ST7701`, `ST7789V`, `ST77916`, `ST77922`
* Add new touch controllers: `ST7123`
* Add new supported boards: `ESP32-4848S040C_I_Y_3`
* Add `LCD` and `Touch` examples for using standalone drivers
* `LCD` supports to reconfigure the vendor specific initialization sequence
* `Touch` supports to use ISR pin for interruption
* `LVLG` examples support **RGB LCD avoid tearing** function
* Update all README.md files and add FAQ section

## v0.0.2 - 2023-11-09

### Enhancements:

* Move extra boards configuration into panel
* Update all README.md files
* Add Squareline porting examples

## v0.0.1 - 2023-09-20

### Enhancements:

* Supports various Espressif official development boards
* Supports custom boards
* Supports multiple types of drivers, including **Bus**, **LCD**, **Touch**, **Backlight**
