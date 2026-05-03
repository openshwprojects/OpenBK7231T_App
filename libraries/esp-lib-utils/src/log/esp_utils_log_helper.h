/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_utils_log.h"

/**
 * Helper macros to replace ESP-IDF logging functions
 */
#undef ESP_LOGV
#undef ESP_LOGD
#undef ESP_LOGI
#undef ESP_LOGW
#undef ESP_LOGE
#define ESP_LOGV(TAG, ...) { (void)TAG; ESP_UTILS_LOGD(__VA_ARGS__); }
#define ESP_LOGD(TAG, ...) { (void)TAG; ESP_UTILS_LOGD(__VA_ARGS__); }
#define ESP_LOGI(TAG, ...) { (void)TAG; ESP_UTILS_LOGI(__VA_ARGS__); }
#define ESP_LOGW(TAG, ...) { (void)TAG; ESP_UTILS_LOGW(__VA_ARGS__); }
#define ESP_LOGE(TAG, ...) { (void)TAG; ESP_UTILS_LOGE(__VA_ARGS__); }
