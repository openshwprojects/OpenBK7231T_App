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
#include "lvgl.h"
#include "lvgl_v8_port.h"
#include "lv_demos.h"

using namespace std;
using namespace esp_panel::drivers;
using namespace esp_panel::board;

#define TEST_DISPLAY_SHOW_TIME_MS   (10000)

#define delay(x)     vTaskDelay(pdMS_TO_TICKS(x))

static const char *TAG = "test_lvgl_port";

TEST_CASE("Test board lvgl port to show demo", "[board][lvgl]")
{
    shared_ptr<Board> board = make_shared<Board>();
    TEST_ASSERT_NOT_NULL_MESSAGE(board, "Create board object failed");

    ESP_LOGI(TAG, "Initialize display board");
    TEST_ASSERT_TRUE_MESSAGE(board->init(), "Board init failed");
#if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();
    // When avoid tearing function is enabled, the frame buffer number should be set in the board driver
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();
    /**
     * As the anti-tearing feature typically consumes more PSRAM bandwidth, for the ESP32-S3, we need to utilize the
     * "bounce buffer" functionality to enhance the RGB data bandwidth.
     * This feature will consume `bounce_buffer_size * bytes_per_pixel * 2` of SRAM memory.
     */
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
    }
#endif
#endif
    TEST_ASSERT_TRUE_MESSAGE(board->begin(), "Board begin failed");

    ESP_LOGI(TAG, "Initialize LVGL");
    lvgl_port_init(board->getLCD(), board->getTouch());

    ESP_LOGI(TAG, "Creating UI");
    /* Lock the mutex due to the LVGL APIs are not thread-safe */
    lvgl_port_lock(-1);

    // lv_demo_widgets();
    // lv_demo_benchmark();
    lv_demo_music();
    // lv_demo_stress();

    /* Release the mutex */
    lvgl_port_unlock();

    delay(TEST_DISPLAY_SHOW_TIME_MS);

    lvgl_port_deinit();
}
