/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

/**
 * @note This file shouldn't be included in the public header file.
 */

// Define the log tag for the current library, should be declared before `esp_lib_utils.h`
#undef ESP_UTILS_LOG_TAG
#define ESP_UTILS_LOG_TAG "Panel"
#include "esp_lib_utils.h"
