/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#include <memory>
#include <thread>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "unity.h"
#include "unity_test_runner.h"
#include "esp_display_panel.hpp"
#include "lcd_general_test.hpp"

using namespace std;
using namespace esp_panel::drivers;

// *INDENT-OFF*

/* The following default configurations are for the board 'jingcai: ESP32_4848S040C_I_Y_3, ST7701' */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define TEST_LCD_WIDTH                      (480)
#define TEST_LCD_HEIGHT                     (480)
                                                    // | 8-bit RGB888 | 16-bit RGB565 |
#define TEST_LCD_COLOR_BITS                 (18)    // |      24      |   16/18/24    |
#define TEST_LCD_RGB_DATA_WIDTH             (16)    // |      8       |      16       |
#define TEST_LCD_RGB_COLOR_BITS             (16)    // |      24      |      16       |
#define TEST_LCD_RGB_TIMING_FREQ_HZ         (26 * 1000 * 1000)
#define TEST_LCD_RGB_TIMING_HPW             (10)
#define TEST_LCD_RGB_TIMING_HBP             (10)
#define TEST_LCD_RGB_TIMING_HFP             (20)
#define TEST_LCD_RGB_TIMING_VPW             (10)
#define TEST_LCD_RGB_TIMING_VBP             (10)
#define TEST_LCD_RGB_TIMING_VFP             (10)
#define TEST_LCD_RGB_BOUNCE_BUFFER_SIZE     (TEST_LCD_WIDTH * 10)
#define TEST_LCD_USE_EXTERNAL_CMD           (1)
#if TEST_LCD_USE_EXTERNAL_CMD
/**
 * LCD initialization commands.
 *
 * Vendor specific initialization can be different between manufacturers, should consult the LCD supplier for
 * initialization sequence code.
 *
 * Please uncomment and change the following macro definitions, then use `configVendorCommands()` to pass them in the
 * same format if needed. Otherwise, the LCD driver will use the default initialization sequence code.
 *
 * There are two formats for the sequence code:
 *   1. Raw data: {command, (uint8_t []){ data0, data1, ... }, data_size, delay_ms}
 *   2. Formatter: ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(delay_ms, command, { data0, data1, ... }) and
 *                ESP_PANEL_LCD_CMD_WITH_NONE_PARAM(delay_ms, command)
 */
const esp_panel_lcd_vendor_init_cmd_t lcd_init_cmd[] = {
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xFF, {0x77, 0x01, 0x00, 0x00, 0x10}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC0, {0x3B, 0x00}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC1, {0x0D, 0x02}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC2, {0x31, 0x05}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xCD, {0x00}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB0, {0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB1, {0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xFF, {0x77, 0x01, 0x00, 0x00, 0x11}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB0, {0x60}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB1, {0x32}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB2, {0x07}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB3, {0x80}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB5, {0x49}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB7, {0x85}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB8, {0x21}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC1, {0x78}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC2, {0x78}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE0, {0x00, 0x1B, 0x02}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE1, {0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE2, {0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE3, {0x00, 0x00, 0x11, 0x11}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE4, {0x44, 0x44}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE5, {0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE6, {0x00, 0x00, 0x11, 0x11}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE7, {0x44, 0x44}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE8, {0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xEB, {0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xEC, {0x3C, 0x00}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xED, {0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xFF, {0x77, 0x01, 0x00, 0x00, 0x13}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE5, {0xE4}),
        ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xFF, {0x77, 0x01, 0x00, 0x00, 0x00}),
        ESP_PANEL_LCD_CMD_WITH_NONE_PARAM(120, 0x11),
};
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your board spec ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define TEST_LCD_PIN_NUM_RGB_DISP           (-1)
#define TEST_LCD_PIN_NUM_RGB_VSYNC          (17)
#define TEST_LCD_PIN_NUM_RGB_HSYNC          (16)
#define TEST_LCD_PIN_NUM_RGB_DE             (18)
#define TEST_LCD_PIN_NUM_RGB_PCLK           (21)
                                                    // | RGB565 | RGB666 | RGB888 |
                                                    // |--------|--------|--------|
#define TEST_LCD_PIN_NUM_RGB_DATA0          (4)     // |   B0   |  B0-1  |   B0-3 |
#define TEST_LCD_PIN_NUM_RGB_DATA1          (5)     // |   B1   |  B2    |   B4   |
#define TEST_LCD_PIN_NUM_RGB_DATA2          (6)     // |   B2   |  B3    |   B5   |
#define TEST_LCD_PIN_NUM_RGB_DATA3          (7)     // |   B3   |  B4    |   B6   |
#define TEST_LCD_PIN_NUM_RGB_DATA4          (15)    // |   B4   |  B5    |   B7   |
#define TEST_LCD_PIN_NUM_RGB_DATA5          (8)     // |   G0   |  G0    |   G0-2 |
#define TEST_LCD_PIN_NUM_RGB_DATA6          (20)    // |   G1   |  G1    |   G3   |
#define TEST_LCD_PIN_NUM_RGB_DATA7          (3)     // |   G2   |  G2    |   G4   |
#if TEST_LCD_RGB_DATA_WIDTH > 8
#define TEST_LCD_PIN_NUM_RGB_DATA8          (46)    // |   G3   |  G3    |   G5   |
#define TEST_LCD_PIN_NUM_RGB_DATA9          (9)     // |   G4   |  G4    |   G6   |
#define TEST_LCD_PIN_NUM_RGB_DATA10         (10)    // |   G5   |  G5    |   G7   |
#define TEST_LCD_PIN_NUM_RGB_DATA11         (11)    // |   R0   |  R0-1  |   R0-3 |
#define TEST_LCD_PIN_NUM_RGB_DATA12         (12)    // |   R1   |  R2    |   R4   |
#define TEST_LCD_PIN_NUM_RGB_DATA13         (13)    // |   R2   |  R3    |   R5   |
#define TEST_LCD_PIN_NUM_RGB_DATA14         (14)    // |   R3   |  R4    |   R6   |
#define TEST_LCD_PIN_NUM_RGB_DATA15         (0)     // |   R4   |  R5    |   R7   |
#endif
#define TEST_LCD_PIN_NUM_SPI_CS             (39)
#define TEST_LCD_PIN_NUM_SPI_SCK            (48)
#define TEST_LCD_PIN_NUM_SPI_SDA            (47)
#define TEST_LCD_PIN_NUM_RST                (-1)    // Set to -1 if not used
#define TEST_LCD_PIN_NUM_BK_LIGHT           (38)    // Set to -1 if not used
#define TEST_LCD_BK_LIGHT_ON_LEVEL          (1)
#define TEST_LCD_BK_LIGHT_OFF_LEVEL !TEST_LCD_BK_LIGHT_ON_LEVEL

// *INDENT-ON*

static const char *TAG = "test_3wire_spi_rgb_lcd";

static BacklightPWM_LEDC::Config backlight_config = {
    .ledc_channel = BacklightPWM_LEDC::LEDC_ChannelPartialConfig{
        .io_num = TEST_LCD_PIN_NUM_BK_LIGHT,
        .on_level = TEST_LCD_BK_LIGHT_ON_LEVEL,
    },
};

static BusRGB::Config bus_config = {
    .control_panel = BusRGB::ControlPanelPartialConfig{
        .cs_gpio_num = TEST_LCD_PIN_NUM_SPI_CS,
        .scl_gpio_num = TEST_LCD_PIN_NUM_SPI_SCK,
        .sda_gpio_num = TEST_LCD_PIN_NUM_SPI_SDA,
    },
    .refresh_panel = BusRGB::RefreshPanelPartialConfig{
        .pclk_hz = TEST_LCD_RGB_TIMING_FREQ_HZ,
        .h_res = TEST_LCD_WIDTH,
        .v_res = TEST_LCD_HEIGHT,
        .hsync_pulse_width = TEST_LCD_RGB_TIMING_HPW,
        .hsync_back_porch = TEST_LCD_RGB_TIMING_HBP,
        .hsync_front_porch = TEST_LCD_RGB_TIMING_HFP,
        .vsync_pulse_width = TEST_LCD_RGB_TIMING_VPW,
        .vsync_back_porch = TEST_LCD_RGB_TIMING_VBP,
        .vsync_front_porch = TEST_LCD_RGB_TIMING_VFP,
        .data_width = TEST_LCD_RGB_DATA_WIDTH,
        .bits_per_pixel = TEST_LCD_RGB_COLOR_BITS,
        .bounce_buffer_size_px = TEST_LCD_RGB_BOUNCE_BUFFER_SIZE,
        .hsync_gpio_num = TEST_LCD_PIN_NUM_RGB_HSYNC,
        .vsync_gpio_num = TEST_LCD_PIN_NUM_RGB_VSYNC,
        .de_gpio_num = TEST_LCD_PIN_NUM_RGB_DE,
        .pclk_gpio_num = TEST_LCD_PIN_NUM_RGB_PCLK,
        .disp_gpio_num = TEST_LCD_PIN_NUM_RGB_DISP,
        .data_gpio_nums = {
            TEST_LCD_PIN_NUM_RGB_DATA0, TEST_LCD_PIN_NUM_RGB_DATA1, TEST_LCD_PIN_NUM_RGB_DATA2, TEST_LCD_PIN_NUM_RGB_DATA3,
            TEST_LCD_PIN_NUM_RGB_DATA4, TEST_LCD_PIN_NUM_RGB_DATA5, TEST_LCD_PIN_NUM_RGB_DATA6, TEST_LCD_PIN_NUM_RGB_DATA7,
#if TEST_LCD_RGB_DATA_WIDTH > 8
            TEST_LCD_PIN_NUM_RGB_DATA8, TEST_LCD_PIN_NUM_RGB_DATA9, TEST_LCD_PIN_NUM_RGB_DATA10, TEST_LCD_PIN_NUM_RGB_DATA11,
            TEST_LCD_PIN_NUM_RGB_DATA12, TEST_LCD_PIN_NUM_RGB_DATA13, TEST_LCD_PIN_NUM_RGB_DATA14, TEST_LCD_PIN_NUM_RGB_DATA15,
#endif
        },
    },
};

static LCD::Config lcd_config = {
    .device = LCD::DevicePartialConfig{
        .reset_gpio_num = TEST_LCD_PIN_NUM_RST,
        .bits_per_pixel = TEST_LCD_COLOR_BITS,
    },
    .vendor = LCD::VendorPartialConfig{
        .hor_res = TEST_LCD_WIDTH,
        .ver_res = TEST_LCD_HEIGHT,
#if TEST_LCD_USE_EXTERNAL_CMD
        .init_cmds = lcd_init_cmd,
        .init_cmds_size = sizeof(lcd_init_cmd) / sizeof(lcd_init_cmd[0]),
#endif
    },
};

static shared_ptr<Backlight> init_backlight(BacklightPWM_LEDC::Config *config)
{
#if TEST_LCD_PIN_NUM_BK_LIGHT >= 0
    std::shared_ptr<Backlight> backlight = nullptr;
    if (config != nullptr) {
        ESP_LOGI(TAG, "Initialize backlight with config");
        backlight = make_shared<BacklightPWM_LEDC>(*config);
    } else {
        ESP_LOGI(TAG, "Initialize backlight with individual parameters");
        backlight = make_shared<BacklightPWM_LEDC>(TEST_LCD_PIN_NUM_BK_LIGHT, TEST_LCD_BK_LIGHT_ON_LEVEL);
    }
    TEST_ASSERT_NOT_NULL_MESSAGE(backlight, "Create backlight object failed");

    TEST_ASSERT_TRUE_MESSAGE(backlight->begin(), "Backlight begin failed");
    TEST_ASSERT_TRUE_MESSAGE(backlight->on(), "Backlight on failed");

    return backlight;
#else
    return nullptr;
#endif
}

static shared_ptr<Bus> init_bus(BusRGB::Config *config)
{
    std::shared_ptr<BusRGB> bus = nullptr;
    if (config != nullptr) {
        ESP_LOGI(TAG, "Initialize bus with config");
        bus = std::make_shared<BusRGB>(*config);
    } else {
        ESP_LOGI(TAG, "Initialize bus with default parameters");
        bus = std::make_shared<BusRGB>(
// *INDENT-OFF*
#if TEST_LCD_RGB_DATA_WIDTH == 8
        /* 3-wire SPI IOs */
        TEST_LCD_PIN_NUM_SPI_CS, TEST_LCD_PIN_NUM_SPI_SCK, TEST_LCD_PIN_NUM_SPI_SDA,
        /* 8-bit RGB IOs */
        TEST_LCD_PIN_NUM_RGB_DATA0, TEST_LCD_PIN_NUM_RGB_DATA1, TEST_LCD_PIN_NUM_RGB_DATA2, TEST_LCD_PIN_NUM_RGB_DATA3,
        TEST_LCD_PIN_NUM_RGB_DATA4, TEST_LCD_PIN_NUM_RGB_DATA5, TEST_LCD_PIN_NUM_RGB_DATA6, TEST_LCD_PIN_NUM_RGB_DATA7,
        TEST_LCD_PIN_NUM_RGB_HSYNC, TEST_LCD_PIN_NUM_RGB_VSYNC, TEST_LCD_PIN_NUM_RGB_PCLK, TEST_LCD_PIN_NUM_RGB_DE,
        TEST_LCD_PIN_NUM_RGB_DISP,
        /* RGB timings */
        TEST_LCD_RGB_TIMING_FREQ_HZ, TEST_LCD_WIDTH, TEST_LCD_HEIGHT, TEST_LCD_RGB_TIMING_HPW, TEST_LCD_RGB_TIMING_HBP,
        TEST_LCD_RGB_TIMING_HFP, TEST_LCD_RGB_TIMING_VPW, TEST_LCD_RGB_TIMING_VBP, TEST_LCD_RGB_TIMING_VFP
#elif TEST_LCD_RGB_DATA_WIDTH == 16
        /* 3-wire SPI IOs */
        TEST_LCD_PIN_NUM_SPI_CS, TEST_LCD_PIN_NUM_SPI_SCK, TEST_LCD_PIN_NUM_SPI_SDA,
        /* 16-bit RGB IOs */
        TEST_LCD_PIN_NUM_RGB_DATA0, TEST_LCD_PIN_NUM_RGB_DATA1, TEST_LCD_PIN_NUM_RGB_DATA2, TEST_LCD_PIN_NUM_RGB_DATA3,
        TEST_LCD_PIN_NUM_RGB_DATA4, TEST_LCD_PIN_NUM_RGB_DATA5, TEST_LCD_PIN_NUM_RGB_DATA6, TEST_LCD_PIN_NUM_RGB_DATA7,
        TEST_LCD_PIN_NUM_RGB_DATA8, TEST_LCD_PIN_NUM_RGB_DATA9, TEST_LCD_PIN_NUM_RGB_DATA10, TEST_LCD_PIN_NUM_RGB_DATA11,
        TEST_LCD_PIN_NUM_RGB_DATA12, TEST_LCD_PIN_NUM_RGB_DATA13, TEST_LCD_PIN_NUM_RGB_DATA14, TEST_LCD_PIN_NUM_RGB_DATA15,
        TEST_LCD_PIN_NUM_RGB_HSYNC, TEST_LCD_PIN_NUM_RGB_VSYNC, TEST_LCD_PIN_NUM_RGB_PCLK, TEST_LCD_PIN_NUM_RGB_DE,
        TEST_LCD_PIN_NUM_RGB_DISP,
        /* RGB timings */
        TEST_LCD_RGB_TIMING_FREQ_HZ, TEST_LCD_WIDTH, TEST_LCD_HEIGHT, TEST_LCD_RGB_TIMING_HPW, TEST_LCD_RGB_TIMING_HBP,
        TEST_LCD_RGB_TIMING_HFP, TEST_LCD_RGB_TIMING_VPW, TEST_LCD_RGB_TIMING_VBP, TEST_LCD_RGB_TIMING_VFP
#endif
        );
    }
// *INDENT-ON*
    TEST_ASSERT_NOT_NULL_MESSAGE(bus, "Create bus object failed");

    if (config == nullptr) {
        bus->configRGB_BounceBufferSize(TEST_LCD_RGB_BOUNCE_BUFFER_SIZE); // Set bounce buffer to avoid screen drift
    }
    TEST_ASSERT_TRUE_MESSAGE(bus->begin(), "Bus begin failed");

    return bus;
}

static void run_test(shared_ptr<LCD> lcd, bool use_config) {
#if TEST_LCD_USE_EXTERNAL_CMD
    if (!use_config) {
        lcd->configVendorCommands(lcd_init_cmd, sizeof(lcd_init_cmd) / sizeof(lcd_init_cmd[0]));
    }
#endif
    TEST_ASSERT_TRUE_MESSAGE(lcd->init(), "LCD init failed");
    TEST_ASSERT_TRUE_MESSAGE(lcd->reset(), "LCD reset failed");
    TEST_ASSERT_TRUE_MESSAGE(lcd->begin(), "LCD begin failed");
    if (lcd->getBasicAttributes().basic_bus_spec.isFunctionValid(LCD::BasicBusSpecification::FUNC_DISPLAY_ON_OFF)) {
        TEST_ASSERT_TRUE_MESSAGE(lcd->setDisplayOnOff(true), "LCD display on failed");
    }

    lcd_general_test(lcd.get());
}

template<typename T>
decltype(auto) create_lcd_impl(Bus * bus, const LCD::Config & config) {
    ESP_LOGI(TAG, "Create LCD with config");
    return make_shared<T>(bus, config);
}

template<typename T>
decltype(auto) create_lcd_impl(Bus * bus, std::nullptr_t) {
    ESP_LOGI(TAG, "Create LCD with default parameters");
    return make_shared<T>(bus, TEST_LCD_WIDTH, TEST_LCD_HEIGHT, TEST_LCD_COLOR_BITS, TEST_LCD_PIN_NUM_RST);
}

#define _CREATE_LCD(name, bus, config) \
    ({ \
        auto lcd = create_lcd_impl<LCD_##name>(bus, config); \
        TEST_ASSERT_NOT_NULL_MESSAGE(lcd, "Create LCD object failed"); \
        lcd; \
    })
#define CREATE_LCD(name, bus, config) _CREATE_LCD(name, bus, config)

#define CREATE_TEST_CASE(name) \
    TEST_CASE("Test LCD (" #name ") to draw color bar", "[lcd][3wire_spi_rgb][" #name "]") \
    { \
        /* 1. Test with individual parameters */ \
        auto backlight = init_backlight(nullptr); \
        auto bus = init_bus(nullptr); \
        auto lcd = CREATE_LCD(name, bus.get(), nullptr); \
        run_test(lcd, false); \
        backlight = nullptr; \
        lcd = nullptr; \
        bus = nullptr; \
        /* 2. Test with config */ \
        backlight = init_backlight(&backlight_config); \
        bus = init_bus(&bus_config); \
        lcd = CREATE_LCD(name, bus.get(), lcd_config); \
        run_test(lcd, true); \
    }

/**
 * Here to create test cases for different LCDs
 */
CREATE_TEST_CASE(GC9503)
CREATE_TEST_CASE(ST7701)
CREATE_TEST_CASE(ST77903)
CREATE_TEST_CASE(ST77922)
