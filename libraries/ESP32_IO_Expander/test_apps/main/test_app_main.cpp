/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "unity.h"
#include "unity_test_utils.h"

#define TEST_MEMORY_LEAK_THRESHOLD (400)

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
void setUp(void)
{
    unity_utils_record_free_mem();
}

void tearDown(void)
{
    esp_reent_cleanup();    //clean up some of the newlib's lazy allocations
    unity_utils_evaluate_leaks_direct(TEST_MEMORY_LEAK_THRESHOLD);
}
#else
static size_t before_free_8bit;
static size_t before_free_32bit;

static void check_leak(size_t before_free, size_t after_free, const char *type)
{
    ssize_t delta = before_free - after_free;
    printf("MALLOC_CAP_%s: Before %u bytes free, After %u bytes free (delta %d)\n", type, before_free, after_free, delta);
    TEST_ASSERT_MESSAGE(delta < TEST_MEMORY_LEAK_THRESHOLD, "memory leak");
}

void setUp(void)
{
    before_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    before_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
}

void tearDown(void)
{
    size_t after_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t after_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
    check_leak(before_free_8bit, after_free_8bit, "8BIT");
    check_leak(before_free_32bit, after_free_32bit, "32BIT");
}
#endif

extern "C" void app_main(void)
{
    //   _____  ___     __                            _
    //   \_   \/___\   /__\_  ___ __   __ _ _ __   __| | ___ _ __
    //    / /\//  //  /_\ \ \/ / '_ \ / _` | '_ \ / _` |/ _ \ '__|
    // /\/ /_/ \_//  //__  >  <| |_) | (_| | | | | (_| |  __/ |
    // \____/\___/   \__/ /_/\_\ .__/ \__,_|_| |_|\__,_|\___|_|
    //                         |_|
    printf("  _____  ___     __                            _\r\n");
    printf("  \\_   \\/___\\   /__\\_  ___ __   __ _ _ __   __| | ___ _ __\r\n");
    printf("   / /\\//  //  /_\\ \\ \\/ / '_ \\ / _` | '_ \\ / _` |/ _ \\ '__|\r\n");
    printf("/\\/ /_/ \\_//  //__  >  <| |_) | (_| | | | | (_| |  __/ |\r\n");
    printf("\\____/\\___/   \\__/ /_/\\_\\ .__/ \\__,_|_| |_|\\__,_|\\___|_|\r\n");
    printf("                        |_|\r\n");
    unity_run_menu();
}
