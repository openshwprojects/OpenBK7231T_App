/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_utils_mem.h"

/**
 * C helper functions to allocate memory using the memory allocator
 */

#undef malloc
#undef free
#undef calloc
#define malloc(size)    esp_utils_mem_gen_malloc(size)
#define free(p)         esp_utils_mem_gen_free(p)
#define calloc(n, size) esp_utils_mem_gen_calloc(n, size)
