/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#include <memory>
#include <thread>
#include "esp_log.h"
#include "esp_timer.h"
#include "unity.h"
#include "unity_test_runner.h"
#include "esp_display_panel.hpp"

#define TEST_LCD_ENABLE_PRINT_FPS               (1)
#define TEST_LCD_ENABLE_DRAW_FINISH_CALLBACK    (1)
#define TEST_LCD_ENABLE_DSI_PATTERN_TEST        (1)
#define TEST_LCD_COLOR_BAR_SHOW_TIME_MS     (5000)

#define delay(x)     vTaskDelay(pdMS_TO_TICKS(x))

using namespace std;
using namespace esp_panel::drivers;

static const char *TAG = "lcd_general_test";

#if TEST_LCD_ENABLE_PRINT_FPS
#define TEST_LCD_PRINT_FPS_PERIOD_MS    (1000)
#define TEST_LCD_PRINT_FPS_COUNT_MAX    (50)
#ifndef millis
#define millis()                (esp_timer_get_time() / 1000)
#endif

DRAM_ATTR int frame_count = 0;
DRAM_ATTR int fps = 0;
DRAM_ATTR long start_time = 0;

IRAM_ATTR bool onLCD_RefreshFinishCallback(void *user_data)
{
    long frame_start_time = *(long *)user_data;
    if (frame_start_time == 0) {
        (*(long *)user_data) = millis();

        return false;
    }

    frame_count++;
    if (frame_count >= TEST_LCD_PRINT_FPS_COUNT_MAX) {
        fps = TEST_LCD_PRINT_FPS_COUNT_MAX * 1000 / (millis() - frame_start_time);
        frame_count = 0;
        (*(long *)user_data) = millis();
    }

    return false;
}
#endif

#if TEST_LCD_ENABLE_DRAW_FINISH_CALLBACK
IRAM_ATTR bool onLCD_DrawFinishCallback(void *user_data)
{
    esp_rom_printf("LCD draw finish callback\n");

    return false;
}
#endif

void lcd_general_test(LCD *lcd)
{
    ESP_LOGI(TAG, "Run LCD general test");

    TEST_ASSERT_NOT_NULL_MESSAGE(lcd, "Invalid LCD");

    frame_count = 0;
    fps = 0;
    start_time = 0;

#if TEST_LCD_ENABLE_PRINT_FPS
    auto bus_type = lcd->getBus()->getBasicAttributes().type;
    if ((bus_type == ESP_PANEL_BUS_TYPE_RGB) || (bus_type == ESP_PANEL_BUS_TYPE_MIPI_DSI)) {
        TEST_ASSERT_TRUE_MESSAGE(
            lcd->attachRefreshFinishCallback(onLCD_RefreshFinishCallback, (void *)&start_time), "Attach refresh callback failed"
        );
    }
#endif
#if TEST_LCD_ENABLE_DRAW_FINISH_CALLBACK
    TEST_ASSERT_TRUE_MESSAGE(
        lcd->attachDrawBitmapFinishCallback(onLCD_DrawFinishCallback, (void *)&start_time),
        "Attach draw finish callback failed"
    );
#endif

    thread lcd_thread = std::thread([&]() {
#if TEST_LCD_ENABLE_DSI_PATTERN_TEST && ESP_PANEL_DRIVERS_BUS_ENABLE_MIPI_DSI
        ESP_LOGI(TAG, "Show MIPI-DSI patterns");
        TEST_ASSERT_TRUE_MESSAGE(
            lcd->DSI_ColorBarPatternTest(LCD::DSI_ColorBarPattern::BAR_HORIZONTAL), "MIPI DPI bar horizontal pattern test failed"
        );
        delay(1000);
        TEST_ASSERT_TRUE_MESSAGE(
            lcd->DSI_ColorBarPatternTest(LCD::DSI_ColorBarPattern::BAR_VERTICAL), "MIPI DPI bar vertical pattern test failed"
        );
        delay(1000);
        TEST_ASSERT_TRUE_MESSAGE(
            lcd->DSI_ColorBarPatternTest(LCD::DSI_ColorBarPattern::BER_VERTICAL), "MIPI DPI ber vertical pattern test failed"
        );
        delay(1000);
        TEST_ASSERT_TRUE_MESSAGE(
            lcd->DSI_ColorBarPatternTest(LCD::DSI_ColorBarPattern::NONE), "MIPI DPI none pattern test failed"
        );
#endif

        ESP_LOGI(TAG, "Draw color bar from top left to bottom right, the order is B - G - R");
        TEST_ASSERT_TRUE_MESSAGE(lcd->colorBarTest(), "LCD color bar test failed");

#if TEST_LCD_ENABLE_PRINT_FPS
        ESP_LOGI(TAG, "Wait for %d ms to show the color bar", TEST_LCD_COLOR_BAR_SHOW_TIME_MS);
        int i = 0;
        while (i++ < TEST_LCD_COLOR_BAR_SHOW_TIME_MS / TEST_LCD_PRINT_FPS_PERIOD_MS) {
            if ((bus_type == ESP_PANEL_BUS_TYPE_RGB) || (bus_type == ESP_PANEL_BUS_TYPE_MIPI_DSI)) {
                ESP_LOGI(TAG, "FPS: %d", fps);
            }
            vTaskDelay(pdMS_TO_TICKS(TEST_LCD_PRINT_FPS_PERIOD_MS));
        }
#else
        vTaskDelay(pdMS_TO_TICKS(TEST_LCD_COLOR_BAR_SHOW_TIME_MS));
#endif
    });

    if (lcd_thread.joinable()) {
        lcd_thread.join();
    }
}
