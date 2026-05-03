/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "unity.h"
#include "unity_test_runner.h"
#include "esp_display_panel.hpp"
#include "lcd_general_test.hpp"
#include "touch_general_test.hpp"
#include "board_configs.hpp"

using namespace std;
using namespace esp_panel::board;
using namespace esp_panel::drivers;

static const char *TAG = "test_common_board";

#if CONFIG_ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM
// *INDENT-OFF*
static esp_panel_lcd_vendor_init_cmd_t lcd_init_cmd[] = {
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC8, {0xFF, 0x93, 0x42}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC0, {0x0E, 0x0E}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC5, {0xD0}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xC1, {0x02}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB4, {0x02}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE0, {0x00, 0x03, 0x08, 0x06, 0x13, 0x09, 0x39, 0x39, 0x48, 0x02, 0x0a,
                                                0x08, 0x17, 0x17, 0x0F}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xE1, {0x00, 0x28, 0x29, 0x01, 0x0d, 0x03, 0x3f, 0x33, 0x52, 0x04, 0x0f,
                                                0x0e, 0x37, 0x38, 0x0F}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB1, {00, 0x1B}),
    ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(0, 0xB7, {0x06}),
    ESP_PANEL_LCD_CMD_WITH_NONE_PARAM(100, 0x11),
};
// *INDENT-ON*

bool lcd_pre_begin(void *args)
{
    Board *board = static_cast<Board *>(args);
    TEST_ASSERT_NOT_NULL_MESSAGE(board, "Board object is null");

    auto board_name = board->getConfig().name;

    if (strcmp(board_name, "Custom_ESP32_S3_BOX_3") == 0) {
        auto tp_config = board->getConfig().touch.value().device_config.device;
        int tp_int_io = -1;
        if (std::holds_alternative<Touch::DevicePartialConfig>(tp_config)) {
            tp_int_io = std::get<Touch::DevicePartialConfig>(tp_config).int_gpio_num;
        } else if (std::holds_alternative<Touch::DeviceFullConfig>(tp_config)) {
            tp_int_io = std::get<Touch::DeviceFullConfig>(tp_config).int_gpio_num;
        }
        /* Maintain the touch INT signal in a low state during the reset process to set its I2C address to `0x5D` */
        gpio_set_direction((gpio_num_t)tp_int_io, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)tp_int_io, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
    } else if (strcmp(board_name, "Custom_ESP32_S3_LCD_EV_BOARD_V1_5") == 0) {
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB
        auto bus_config = std::get<BusRGB::Config>(board->getConfig().lcd.value().bus_config).refresh_panel;
        int rgb_vsync_io = -1;
        if (std::holds_alternative<BusRGB::RefreshPanelPartialConfig>(bus_config)) {
            rgb_vsync_io = std::get<BusRGB::RefreshPanelPartialConfig>(bus_config).vsync_gpio_num;
        } else if (std::holds_alternative<BusRGB::RefreshPanelFullConfig>(bus_config)) {
            rgb_vsync_io = std::get<BusRGB::RefreshPanelFullConfig>(bus_config).vsync_gpio_num;
        }
        /* For the v1.5 version sub board, need to set `ESP_PANEL_BOARD_LCD_RGB_IO_VSYNC` to high before initialize LCD */
        gpio_set_direction((gpio_num_t)rgb_vsync_io, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)rgb_vsync_io, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level((gpio_num_t)rgb_vsync_io, 1);
        vTaskDelay(pdMS_TO_TICKS(120));
#endif
    }

    return true;
}
#endif

static void board_common_init(Board *board)
{
#if CONFIG_ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM
    auto board_name = board->getConfig().name;
    if ((strcmp(board_name, "Custom_ESP32_S3_BOX_3") == 0) ||
            (strcmp(board_name, "Custom_ESP32_S3_LCD_EV_BOARD_V1_5") == 0)) {
        TEST_ASSERT_TRUE_MESSAGE(
            board->configCallback(BoardConfig::STAGE_CALLBACK_PRE_LCD_BEGIN, lcd_pre_begin),
            "Config LCD pre-begin callback failed"
        );
    }
#endif

    ESP_LOGI(TAG, "Initialize board");
    TEST_ASSERT_TRUE_MESSAGE(board->init(), "Board init failed");

#if CONFIG_ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM
    if ((strcmp(board_name, "Custom_ESP32_S3_BOX_3") == 0)) {
        TEST_ASSERT_TRUE_MESSAGE(
            board->getLCD()->configVendorCommands(lcd_init_cmd, sizeof(lcd_init_cmd) / sizeof(lcd_init_cmd[0])),
            "Config LCD vendor init command failed"
        );
    }
#endif

    TEST_ASSERT_TRUE_MESSAGE(board->begin(), "Board begin failed");
}

TEST_CASE("Test common board with default config", "[board][common][default]")
{
    shared_ptr<Board> board = make_shared<Board>();
    TEST_ASSERT_NOT_NULL_MESSAGE(board, "Create board object failed");

    board_common_init(board.get());

    auto lcd = board->getLCD();
    if (lcd) {
        lcd_general_test(lcd);
    }

    auto touch = board->getTouch();
    if (touch) {
        touch_general_test(touch);
        gpio_uninstall_isr_service();
    }
}

#define CREATE_TEST_CASE(board_name) \
    TEST_CASE("Test common board with " #board_name " external config", "[board][common][external]") \
    { \
        shared_ptr<Board> board = make_shared<Board>(board_name ## _CONFIG); \
        TEST_ASSERT_NOT_NULL_MESSAGE(board, "Create board object failed"); \
        board_common_init(board.get()); \
        \
        auto lcd = board->getLCD(); \
        if (lcd) { \
            lcd_general_test(lcd); \
        } \
        \
        auto touch = board->getTouch(); \
        if (touch) { \
            touch_general_test(touch); \
            gpio_uninstall_isr_service(); \
        } \
    }

#if CONFIG_BOARD_ESPRESSIF_ESP32_C3_LCDKIT
CREATE_TEST_CASE(BOARD_ESPRESSIF_ESP32_C3_LCDKIT)
#endif
#if CONFIG_BOARD_ESPRESSIF_ESP32_P4_FUNCTION_EV_BOARD
CREATE_TEST_CASE(BOARD_ESPRESSIF_ESP32_P4_FUNCTION_EV_BOARD)
#endif
#if CONFIG_BOARD_ESPRESSIF_ESP32_S3_BOX_3
CREATE_TEST_CASE(BOARD_ESPRESSIF_ESP32_S3_BOX_3)
#endif
#if CONFIG_BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2_V1_5
CREATE_TEST_CASE(BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_2_V1_5)
#endif
#if CONFIG_BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_V1_5
CREATE_TEST_CASE(BOARD_ESPRESSIF_ESP32_S3_LCD_EV_BOARD_V1_5)
#endif
