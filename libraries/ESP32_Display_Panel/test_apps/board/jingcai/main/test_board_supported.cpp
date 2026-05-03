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
#include "touch_general_test.hpp"

using namespace std;
using namespace esp_panel::board;
using namespace esp_panel::drivers;

static const char *TAG = "test_supported_board";

TEST_CASE("Test supported board", "[board][supported]")
{
    shared_ptr<Board> board = make_shared<Board>();
    TEST_ASSERT_NOT_NULL_MESSAGE(board, "Create board object failed");

    ESP_LOGI(TAG, "Initialize board");
    TEST_ASSERT_TRUE_MESSAGE(board->init(), "Board init failed");
    TEST_ASSERT_TRUE_MESSAGE(board->begin(), "Board begin failed");

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
