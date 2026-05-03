/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sdkconfig.h"

/**
 * @brief Macros for check handle method
 */
#define ESP_UTILS_CHECK_HANDLE_WITH_NONE        (0) /*!< Do nothing when check failed */
#define ESP_UTILS_CHECK_HANDLE_WITH_ERROR_LOG   (1) /*!< Print error message when check failed */
#define ESP_UTILS_CHECK_HANDLE_WITH_ASSERT      (2) /*!< Assert when check failed */

/**
 * @brief Macros for log level
 */
#define ESP_UTILS_LOG_LEVEL_DEBUG   (0)     /*!< Extra information which is not necessary for normal use (values,
                                             *   pointers, sizes, etc). */
#define ESP_UTILS_LOG_LEVEL_INFO    (1)     /*!< Information messages which describe the normal flow of events */
#define ESP_UTILS_LOG_LEVEL_WARNING (2)     /*!< Error conditions from which recovery measures have been taken */
#define ESP_UTILS_LOG_LEVEL_ERROR   (3)     /*!< Critical errors, software module cannot recover on its own */
#define ESP_UTILS_LOG_LEVEL_NONE    (4)     /*!< No log output */

/**
 * @brief Macros for memory type
 */
#define ESP_UTILS_MEM_ALLOC_TYPE_STDLIB        (0)
#define ESP_UTILS_MEM_ALLOC_TYPE_ESP           (1)
#define ESP_UTILS_MEM_ALLOC_TYPE_MICROPYTHON   (2)
#define ESP_UTILS_MEM_ALLOC_TYPE_CUSTOM        (3)
