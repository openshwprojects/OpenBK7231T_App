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

#define TEST_TOUCH_ENABLE_INTERRUPT_CALLBACK   (1)
#define TEST_TOUCH_READ_PERIOD_MS           (30)
#define TEST_TOUCH_READ_TIME_MS             (5000)

#define delay(x)     vTaskDelay(pdMS_TO_TICKS(x))

using namespace std;
using namespace esp_panel;
using namespace esp_panel::drivers;

static const char *TAG = "touch_general_test";

#if TEST_TOUCH_ENABLE_INTERRUPT_CALLBACK
IRAM_ATTR static bool onTouchInterruptCallback(void *user_data)
{
    esp_rom_printf("Touch interrupt callback\n");

    return false;
}
#endif

void touch_general_test(Touch *touch)
{
    ESP_LOGI(TAG, "Run touch general test");

#if TEST_TOUCH_ENABLE_INTERRUPT_CALLBACK
    if (touch->isInterruptEnabled()) {
        TEST_ASSERT_TRUE_MESSAGE(
            touch->attachInterruptCallback(onTouchInterruptCallback, nullptr), "Attach touch interrupt callback failed"
        );
    }
#endif

    thread touch_thread = std::thread([&]() {
        ESP_LOGI(TAG, "Reading touch_device point...");

        uint32_t t = 0;
        std::vector<drivers::TouchPoint> points;
        std::vector<drivers::TouchButton> buttons;

        while (t++ < TEST_TOUCH_READ_TIME_MS / TEST_TOUCH_READ_PERIOD_MS) {
            TEST_ASSERT_TRUE_MESSAGE(touch->readRawData(-1, -1, TEST_TOUCH_READ_PERIOD_MS), "Read touch raw data failed");
            TEST_ASSERT_TRUE_MESSAGE(touch->getPoints(points), "Read touch points failed");
            TEST_ASSERT_TRUE_MESSAGE(touch->getButtons(buttons), "Read touch buttons failed");
            int i = 0;
            for (auto &point : points) {
                ESP_LOGI(TAG, "Point(%d): x(%d), y(%d), strength(%d)", i++, point.x, point.y, point.strength);
            }
            i = 0;
            for (auto &button : buttons) {
                ESP_LOGI(TAG, "Button(%d): %d", i++, button.second);
            }
            if (!touch->isInterruptEnabled()) {
                delay(TEST_TOUCH_READ_PERIOD_MS);
            }
        }
    });

    if (touch_thread.joinable()) {
        touch_thread.join();
    }
}
