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

/* The following default configurations are for the board 'Espressif: ESP32_S3_BOX_3, ILI9341' */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define TEST_LCD_WIDTH                  (320)
#define TEST_LCD_HEIGHT                 (240)
#define TEST_LCD_COLOR_BITS             (16)
#define TEST_LCD_SPI_FREQ_HZ            (40 * 1000 * 1000)
#define TEST_LCD_USE_EXTERNAL_CMD       (1)
#define TEST_LCD_RGB_ELE_REVERSE_ORDER  (1)
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
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC8, {0xFF, 0x93, 0x42}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC0, {0x0E, 0x0E}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC5, {0xD0}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC1, {0x02}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB4, {0x02}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE0, {
        0x00, 0x03, 0x08, 0x06, 0x13, 0x09, 0x39, 0x39, 0x48, 0x02, 0x0a, 0x08, 0x17, 0x17, 0x0F
    }),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE1, {
        0x00, 0x28, 0x29, 0x01, 0x0d, 0x03, 0x3f, 0x33, 0x52, 0x04, 0x0f, 0x0e, 0x37, 0x38, 0x0F
    }),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB1, {00, 0x1B}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB7, {0x06}),
    ESP_PANEL_LCD_CMD_WITH_NONE_PARAM(100, 0x11),

};
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your board spec ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define TEST_LCD_PIN_NUM_SPI_CS      (5)
#define TEST_LCD_PIN_NUM_SPI_DC      (4)
#define TEST_LCD_PIN_NUM_SPI_SCK     (7)
#define TEST_LCD_PIN_NUM_SPI_MOSI    (6)
#define TEST_LCD_PIN_NUM_RST         (48)    // Set to -1 if not used
#define TEST_LCD_RST_ACTIVE_LEVEL    (1)
#define TEST_LCD_PIN_NUM_BK_LIGHT    (47)    // Set to -1 if not used
#define TEST_LCD_BK_LIGHT_ON_LEVEL   (1)
#define TEST_LCD_BK_LIGHT_OFF_LEVEL !TEST_LCD_BK_LIGHT_ON_LEVEL

static const char *TAG = "test_spi_lcd";

static BacklightPWM_LEDC::Config backlight_config = {
    .ledc_channel = BacklightPWM_LEDC::LEDC_ChannelPartialConfig{
        .io_num = TEST_LCD_PIN_NUM_BK_LIGHT,
        .on_level = TEST_LCD_BK_LIGHT_ON_LEVEL,
    },
};

static BusSPI::Config bus_config = {
    .host = BusSPI::HostPartialConfig{
        .mosi_io_num = TEST_LCD_PIN_NUM_SPI_MOSI,
        .sclk_io_num = TEST_LCD_PIN_NUM_SPI_SCK,
    },
    .control_panel = BusSPI::ControlPanelPartialConfig{
        .cs_gpio_num = TEST_LCD_PIN_NUM_SPI_CS,
        .dc_gpio_num = TEST_LCD_PIN_NUM_SPI_DC,
        .pclk_hz = TEST_LCD_SPI_FREQ_HZ,
    },
};

static LCD::Config lcd_config = {
    .device = LCD::DevicePartialConfig{
        .reset_gpio_num = TEST_LCD_PIN_NUM_RST,
        .rgb_ele_order = TEST_LCD_RGB_ELE_REVERSE_ORDER ? LCD_RGB_ELEMENT_ORDER_BGR : LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = TEST_LCD_COLOR_BITS,
        .flags_reset_active_high = TEST_LCD_RST_ACTIVE_LEVEL,
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

static shared_ptr<Bus> init_bus(BusSPI::Config *config)
{
    ESP_LOGI(TAG, "Create LCD bus");
    // *INDENT-OFF*
    std::shared_ptr<BusSPI> bus = nullptr;
    if (config != nullptr) {
        ESP_LOGI(TAG, "Initialize bus with config");
        bus = make_shared<BusSPI>(*config);
    } else {
        ESP_LOGI(TAG, "Initialize bus with individual parameters");
        bus = make_shared<BusSPI>(
            TEST_LCD_PIN_NUM_SPI_CS, TEST_LCD_PIN_NUM_SPI_DC, TEST_LCD_PIN_NUM_SPI_SCK, TEST_LCD_PIN_NUM_SPI_MOSI
        );
    }
    // *INDENT-ON*
    TEST_ASSERT_NOT_NULL_MESSAGE(bus, "Create bus object failed");

    bus->configSPI_FreqHz(TEST_LCD_SPI_FREQ_HZ);
    TEST_ASSERT_TRUE_MESSAGE(bus->begin(), "Bus begin failed");

    return bus;
}

static void run_test(shared_ptr<LCD> lcd, bool use_config)
{
    if (!use_config) {
#if TEST_LCD_USE_EXTERNAL_CMD
        lcd->configVendorCommands(lcd_init_cmd, sizeof(lcd_init_cmd) / sizeof(lcd_init_cmd[0]));
#endif
        lcd->configResetActiveLevel(TEST_LCD_RST_ACTIVE_LEVEL);
        lcd->configColorRGB_Order(TEST_LCD_RGB_ELE_REVERSE_ORDER);
    }
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

#define CREATE_LCD(name, bus, config) \
    ({ \
        auto lcd = create_lcd_impl<LCD_##name>(bus, config); \
        TEST_ASSERT_NOT_NULL_MESSAGE(lcd, "Create LCD object failed"); \
        lcd; \
    })

#define CREATE_TEST_CASE(name) \
    TEST_CASE("Test LCD (" #name ") to draw color bar", "[lcd][spi][" #name "]") \
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
CREATE_TEST_CASE(GC9A01)
CREATE_TEST_CASE(GC9B71)
CREATE_TEST_CASE(ILI9341)
CREATE_TEST_CASE(NV3022B)
CREATE_TEST_CASE(SH8601)
CREATE_TEST_CASE(SPD2010)
CREATE_TEST_CASE(ST7789)
CREATE_TEST_CASE(ST7796)
CREATE_TEST_CASE(ST77916)
CREATE_TEST_CASE(ST77922)
