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
#include "unity.h"
#include "unity_test_runner.h"
#include "esp_display_panel.hpp"
#include "lcd_general_test.hpp"

using namespace std;
using namespace esp_panel::drivers;

/* The following default configurations are for the board 'Espressif: Custom, ST77922' */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define TEST_LCD_WIDTH               (532)
#define TEST_LCD_HEIGHT              (300)
#define TEST_LCD_COLOR_BITS          (16)
#define TEST_LCD_SPI_FREQ_HZ         (40 * 1000 * 1000)
#define TEST_LCD_USE_EXTERNAL_CMD    (0)
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
    ESP_PANEL_LCD_CMD_WITH_NONE_PARAM(120, 0x29),
};
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your board spec ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define TEST_LCD_PIN_NUM_SPI_CS      (9)
#define TEST_LCD_PIN_NUM_SPI_SCK     (10)
#define TEST_LCD_PIN_NUM_SPI_DATA0   (11)
#define TEST_LCD_PIN_NUM_SPI_DATA1   (12)
#define TEST_LCD_PIN_NUM_SPI_DATA2   (13)
#define TEST_LCD_PIN_NUM_SPI_DATA3   (14)
#define TEST_LCD_PIN_NUM_RST         (3)     // Set to -1 if not used
#define TEST_LCD_PIN_NUM_BK_LIGHT    (-1)    // Set to -1 if not used
#define TEST_LCD_BK_LIGHT_ON_LEVEL   (1)
#define TEST_LCD_BK_LIGHT_OFF_LEVEL !TEST_LCD_BK_LIGHT_ON_LEVEL

static const char *TAG = "test_qspi_lcd";

static BacklightPWM_LEDC::Config backlight_config = {
    .ledc_channel = BacklightPWM_LEDC::LEDC_ChannelPartialConfig{
        .io_num = TEST_LCD_PIN_NUM_BK_LIGHT,
        .on_level = TEST_LCD_BK_LIGHT_ON_LEVEL,
    },
};

static BusQSPI::Config bus_config = {
    .host = BusQSPI::HostPartialConfig{
        .sclk_io_num = TEST_LCD_PIN_NUM_SPI_SCK,
        .data0_io_num = TEST_LCD_PIN_NUM_SPI_DATA0,
        .data1_io_num = TEST_LCD_PIN_NUM_SPI_DATA1,
        .data2_io_num = TEST_LCD_PIN_NUM_SPI_DATA2,
        .data3_io_num = TEST_LCD_PIN_NUM_SPI_DATA3,
    },
    .control_panel = BusQSPI::ControlPanelPartialConfig{
        .cs_gpio_num = TEST_LCD_PIN_NUM_SPI_CS,
        .pclk_hz = TEST_LCD_SPI_FREQ_HZ,
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

static shared_ptr<Bus> init_bus(BusQSPI::Config *config)
{
    ESP_LOGI(TAG, "Create LCD bus");
    // *INDENT-OFF*
    std::shared_ptr<BusQSPI> bus = nullptr;
    if (config != nullptr) {
        ESP_LOGI(TAG, "Initialize bus with config");
        bus = make_shared<BusQSPI>(*config);
    } else {
        ESP_LOGI(TAG, "Initialize bus with individual parameters");
        bus = make_shared<BusQSPI>(
            TEST_LCD_PIN_NUM_SPI_CS, TEST_LCD_PIN_NUM_SPI_SCK,
            TEST_LCD_PIN_NUM_SPI_DATA0, TEST_LCD_PIN_NUM_SPI_DATA1, TEST_LCD_PIN_NUM_SPI_DATA2, TEST_LCD_PIN_NUM_SPI_DATA3
        );
    }
    // *INDENT-ON*
    TEST_ASSERT_NOT_NULL_MESSAGE(bus, "Create bus object failed");

    bus->configQSPI_FreqHz(TEST_LCD_SPI_FREQ_HZ);
    TEST_ASSERT_TRUE_MESSAGE(bus->begin(), "Bus begin failed");

    return bus;
}

#if TEST_ENABLE_ATTACH_CALLBACK
IRAM_ATTR static bool onDrawBitmapFinishCallback(void *user_data)
{
    esp_rom_printf("Draw bitmap finish callback\n");

    return false;
}
#endif

static void run_test(shared_ptr<LCD> lcd, bool use_config)
{
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
decltype(auto) create_lcd_impl(Bus *bus, const LCD::Config &config)
{
    ESP_LOGI(TAG, "Create LCD with config");
    return make_shared<T>(bus, config);
}

template<typename T>
decltype(auto) create_lcd_impl(Bus *bus, std::nullptr_t)
{
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
    TEST_CASE("Test LCD (" #name ") to draw color bar", "[lcd][qspi][" #name "]") \
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
CREATE_TEST_CASE(AXS15231B)
CREATE_TEST_CASE(GC9B71)
CREATE_TEST_CASE(SH8601)
CREATE_TEST_CASE(SPD2010)
CREATE_TEST_CASE(ST77916)
CREATE_TEST_CASE(ST77922)
