/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#include <memory>
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

/* The following default configurations are for the board 'Espressif: ESP32_S3_LCD_EV_BOARD_2_V1_5, ST7262' */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define TEST_LCD_WIDTH                      (800)
#define TEST_LCD_HEIGHT                     (480)
                                                    // | 8-bit RGB888 | 16-bit RGB565 |
#define TEST_LCD_COLOR_BITS                 (16)    // |      24      |   16/18/24    |
#define TEST_LCD_RGB_DATA_WIDTH             (16)    // |      8       |      16       |
#define TEST_LCD_RGB_COLOR_BITS             (16)    // |      24      |      16       |
#define TEST_LCD_RGB_TIMING_FREQ_HZ         (16 * 1000 * 1000)
#define TEST_LCD_RGB_TIMING_HPW             (40)
#define TEST_LCD_RGB_TIMING_HBP             (40)
#define TEST_LCD_RGB_TIMING_HFP             (40)
#define TEST_LCD_RGB_TIMING_VPW             (23)
#define TEST_LCD_RGB_TIMING_VBP             (32)
#define TEST_LCD_RGB_TIMING_VFP             (13)
#define TEST_LCD_RGB_BOUNCE_BUFFER_SIZE     (TEST_LCD_WIDTH * 10)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your board spec ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define TEST_LCD_PIN_NUM_RGB_DISP           (-1)
#define TEST_LCD_PIN_NUM_RGB_VSYNC          (3)
#define TEST_LCD_PIN_NUM_RGB_HSYNC          (46)
#define TEST_LCD_PIN_NUM_RGB_DE             (17)
#define TEST_LCD_PIN_NUM_RGB_PCLK           (9)
                                                    // | RGB565 | RGB666 | RGB888 |
                                                    // |--------|--------|--------|
#define TEST_LCD_PIN_NUM_RGB_DATA0          (10)    // |   B0   |  B0-1  |   B0-3 |
#define TEST_LCD_PIN_NUM_RGB_DATA1          (11)    // |   B1   |  B2    |   B4   |
#define TEST_LCD_PIN_NUM_RGB_DATA2          (12)    // |   B2   |  B3    |   B5   |
#define TEST_LCD_PIN_NUM_RGB_DATA3          (13)    // |   B3   |  B4    |   B6   |
#define TEST_LCD_PIN_NUM_RGB_DATA4          (14)    // |   B4   |  B5    |   B7   |
#define TEST_LCD_PIN_NUM_RGB_DATA5          (21)    // |   G0   |  G0    |   G0-2 |
#define TEST_LCD_PIN_NUM_RGB_DATA6          (8)     // |   G1   |  G1    |   G3   |
#define TEST_LCD_PIN_NUM_RGB_DATA7          (18)    // |   G2   |  G2    |   G4   |
#if TEST_LCD_RGB_DATA_WIDTH > 8
#define TEST_LCD_PIN_NUM_RGB_DATA8          (45)    // |   G3   |  G3    |   G5   |
#define TEST_LCD_PIN_NUM_RGB_DATA9          (38)    // |   G4   |  G4    |   G6   |
#define TEST_LCD_PIN_NUM_RGB_DATA10         (39)    // |   G5   |  G5    |   G7   |
#define TEST_LCD_PIN_NUM_RGB_DATA11         (40)    // |   R0   |  R0-1  |   R0-3 |
#define TEST_LCD_PIN_NUM_RGB_DATA12         (41)    // |   R1   |  R2    |   R4   |
#define TEST_LCD_PIN_NUM_RGB_DATA13         (42)    // |   R2   |  R3    |   R5   |
#define TEST_LCD_PIN_NUM_RGB_DATA14         (2)     // |   R3   |  R4    |   R6   |
#define TEST_LCD_PIN_NUM_RGB_DATA15         (1)     // |   R4   |  R5    |   R7   |
#endif
#define TEST_LCD_PIN_NUM_RST                (-1)    // Set to -1 if not used
#define TEST_LCD_PIN_NUM_BK_LIGHT           (-1)    // Set to -1 if not used
#define TEST_LCD_BK_LIGHT_ON_LEVEL          (1)
#define TEST_LCD_BK_LIGHT_OFF_LEVEL !TEST_LCD_BK_LIGHT_ON_LEVEL

// *INDENT-ON*

static const char *TAG = "test_rgb_lcd";

static BacklightPWM_LEDC::Config backlight_config = {
    .ledc_channel = BacklightPWM_LEDC::LEDC_ChannelPartialConfig{
        .io_num = TEST_LCD_PIN_NUM_BK_LIGHT,
        .on_level = TEST_LCD_BK_LIGHT_ON_LEVEL,
    },
};

static BusRGB::Config bus_config = {
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
        /* 8-bit RGB IOs */
        TEST_LCD_PIN_NUM_RGB_DATA0, TEST_LCD_PIN_NUM_RGB_DATA1, TEST_LCD_PIN_NUM_RGB_DATA2, TEST_LCD_PIN_NUM_RGB_DATA3,
        TEST_LCD_PIN_NUM_RGB_DATA4, TEST_LCD_PIN_NUM_RGB_DATA5, TEST_LCD_PIN_NUM_RGB_DATA6, TEST_LCD_PIN_NUM_RGB_DATA7,
        TEST_LCD_PIN_NUM_RGB_HSYNC, TEST_LCD_PIN_NUM_RGB_VSYNC, TEST_LCD_PIN_NUM_RGB_PCLK, TEST_LCD_PIN_NUM_RGB_DE,
        TEST_LCD_PIN_NUM_RGB_DISP,
        /* RGB timings */
        TEST_LCD_RGB_TIMING_FREQ_HZ, TEST_LCD_WIDTH, TEST_LCD_HEIGHT, TEST_LCD_RGB_TIMING_HPW, TEST_LCD_RGB_TIMING_HBP,
        TEST_LCD_RGB_TIMING_HFP, TEST_LCD_RGB_TIMING_VPW, TEST_LCD_RGB_TIMING_VBP, TEST_LCD_RGB_TIMING_VFP
#elif TEST_LCD_RGB_DATA_WIDTH == 16
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
CREATE_TEST_CASE(ST7262)
CREATE_TEST_CASE(EK9716B)
