/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_utils_conf_internal.h"

#ifndef ESP_UTILS_LOG_TAG
#define ESP_UTILS_LOG_TAG "Utils"
#endif

// #include <cstdio>
// #include <cstdarg>
// #include <cstring>
// #include <functional>
// #include <mutex>

// extern "C" const char *esp_utils_log_extract_file_name(const char *file_path);

// namespace esp_utils {

// /**
//  * Class to handle logging
//  */
// class Log {
// public:
//     // Singleton pattern: Get the unique instance of the class
//     static Log &getInstance()
//     {
//         static Log instance;
//         return instance;
//     }

//     // Templates and conditional compilation: Filter logs by different levels
//     template <int level>
//     void print(const char *file, int line, const char *func, const char *format, ...)
//     {
//         // Logs below the global level will not be compiled
//         if constexpr (level >= ESP_UTILS_CONF_LOG_LEVEL) {
//             // Mutex to avoid interleaved log messages
//             std::lock_guard<std::mutex> lock(_mutex);
//             // Use variadic arguments for formatted output
//             va_list args;
//             va_start(args, format);
//             vsnprintf(_buffer, sizeof(_buffer), format, args);
//             va_end(args);
//             // Output log message
//             printf(
//                 "[%c][%s][%s:%04d](%s): %s\n", logLevelToChar(level), ESP_UTILS_LOG_TAG,
//                 esp_utils_log_extract_file_name(file), line, func, _buffer
//             );
//         }
//     }

// private:
//     Log() = default;

//     // Convert log level to string
//     static constexpr char logLevelToChar(int level)
//     {
//         switch (level) {
//         case ESP_UTILS_LOG_LEVEL_DEBUG:   return 'D';
//         case ESP_UTILS_LOG_LEVEL_INFO:    return 'I';
//         case ESP_UTILS_LOG_LEVEL_WARNING: return 'W';
//         case ESP_UTILS_LOG_LEVEL_ERROR:   return 'E';
//         default: break;
//         }
//         return ' ';
//     }

//     char _buffer[ESP_UTILS_CONF_LOG_BUFFER_SIZE];
//     std::mutex _mutex;
// };

// } // namespace esp_utils

/**
 * Macros to simplify logging calls
 */
/*
#define ESP_UTILS_LOGD(format, ...) \
    esp_utils::Log::getInstance().print<ESP_UTILS_LOG_LEVEL_DEBUG>(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define ESP_UTILS_LOGI(format, ...) \
    esp_utils::Log::getInstance().print<ESP_UTILS_LOG_LEVEL_INFO>(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define ESP_UTILS_LOGW(format, ...) \
    esp_utils::Log::getInstance().print<ESP_UTILS_LOG_LEVEL_WARNING>(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define ESP_UTILS_LOGE(format, ...) \
    esp_utils::Log::getInstance().print<ESP_UTILS_LOG_LEVEL_ERROR>(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
*/

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *esp_utils_log_extract_file_name(const char *file_path);

#ifdef __cplusplus
}
#endif

#define ESP_UTILS_IMPL_LOGD(format, ...) printf("[D][" ESP_UTILS_LOG_TAG "][%s:%04d](%s): " format "\n", \
                                         esp_utils_log_extract_file_name(__FILE__), __LINE__, __func__,  ##__VA_ARGS__)
#define ESP_UTILS_IMPL_LOGI(format, ...) printf("[I][" ESP_UTILS_LOG_TAG "][%s:%04d](%s): " format "\n", \
                                         esp_utils_log_extract_file_name(__FILE__), __LINE__, __func__,  ##__VA_ARGS__)
#define ESP_UTILS_IMPL_LOGW(format, ...) printf("[W][" ESP_UTILS_LOG_TAG "][%s:%04d](%s): " format "\n", \
                                         esp_utils_log_extract_file_name(__FILE__), __LINE__, __func__,  ##__VA_ARGS__)
#define ESP_UTILS_IMPL_LOGE(format, ...) printf("[E][" ESP_UTILS_LOG_TAG "][%s:%04d](%s): " format "\n", \
                                         esp_utils_log_extract_file_name(__FILE__), __LINE__, __func__,  ##__VA_ARGS__)

#define ESP_UTILS_LOG_LEVEL(level, format, ...) do {                                                    \
        if      (level == ESP_UTILS_LOG_LEVEL_DEBUG)   { ESP_UTILS_IMPL_LOGD(format, ##__VA_ARGS__); }  \
        else if (level == ESP_UTILS_LOG_LEVEL_INFO)    { ESP_UTILS_IMPL_LOGI(format, ##__VA_ARGS__); }  \
        else if (level == ESP_UTILS_LOG_LEVEL_WARNING) { ESP_UTILS_IMPL_LOGW(format, ##__VA_ARGS__); }  \
        else if (level == ESP_UTILS_LOG_LEVEL_ERROR)   { ESP_UTILS_IMPL_LOGE(format, ##__VA_ARGS__); }  \
        else { }                                                                                        \
    } while(0)

#define ESP_UTILS_LOG_LEVEL_LOCAL(level, format, ...) do {                                          \
        if (level >= ESP_UTILS_CONF_LOG_LEVEL) ESP_UTILS_LOG_LEVEL(level, format, ##__VA_ARGS__); \
    } while(0)

/**
 * Macros to simplify logging calls
 */
#define ESP_UTILS_LOGD(format, ...) ESP_UTILS_LOG_LEVEL_LOCAL(ESP_UTILS_LOG_LEVEL_DEBUG,   format, ##__VA_ARGS__)
#define ESP_UTILS_LOGI(format, ...) ESP_UTILS_LOG_LEVEL_LOCAL(ESP_UTILS_LOG_LEVEL_INFO,    format, ##__VA_ARGS__)
#define ESP_UTILS_LOGW(format, ...) ESP_UTILS_LOG_LEVEL_LOCAL(ESP_UTILS_LOG_LEVEL_WARNING, format, ##__VA_ARGS__)
#define ESP_UTILS_LOGE(format, ...) ESP_UTILS_LOG_LEVEL_LOCAL(ESP_UTILS_LOG_LEVEL_ERROR,   format, ##__VA_ARGS__)

/**
 * Micros to log trace of function calls
 */
#if ESP_UTILS_CONF_ENABLE_LOG_TRACE
#define ESP_UTILS_LOG_TRACE_ENTER() ESP_UTILS_LOGD("Enter")
#define ESP_UTILS_LOG_TRACE_EXIT()  ESP_UTILS_LOGD("Exit")
#else
#define ESP_UTILS_LOG_TRACE_ENTER()
#define ESP_UTILS_LOG_TRACE_EXIT()
#endif
#ifdef __cplusplus
#if ESP_UTILS_CONF_ENABLE_LOG_TRACE
#define ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS() ESP_UTILS_LOGD("(@%p) Enter", this)
#define ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS()  ESP_UTILS_LOGD("(@%p) Exit", this)
#else
#define ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS()
#define ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS()
#endif
#endif
