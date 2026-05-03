/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "esp_err.h"
#include "esp_utils_conf_internal.h"
#include "log/esp_utils_log.h"

#if ESP_UTILS_CONF_CHECK_HANDLE_METHOD == ESP_UTILS_CHECK_HANDLE_WITH_NONE

/**
 * @brief Check if the pointer is NULL; if NULL, return the specified value.
 *
 * @param x Pointer to check
 * @param ret Value to return if the pointer is NULL
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_NULL_RETURN(x, ret, fmt, ...) do { \
            if (unlikely((x) == NULL)) {                          \
                return ret;                             \
            }                                           \
        } while(0)

/**
 * @brief Check if the pointer is NULL; if NULL, goto the specified label.
 *
 * @param x Pointer to check
 * @param goto_tag Label to jump to if the pointer is NULL
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_NULL_GOTO(x, goto_tag, fmt, ...) do { \
            if (unlikely((x) == NULL)) {                             \
                goto goto_tag;                             \
            }                                              \
        } while(0)

/**
 * @brief Check if the pointer is NULL; if NULL, return without a value.
 *
 * @param x Pointer to check
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_NULL_EXIT(x, fmt, ...) do { \
            if (unlikely((x) == NULL)) {                   \
                return;                          \
            }                                    \
        } while(0)

/**
 * @brief Check if the value is false; if false, return the specified value.
 *
 * @param x Value to check
 * @param ret Value to return if the value is false
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_FALSE_RETURN(x, ret, fmt, ...) do { \
            if (unlikely((x) == false)) {                          \
                return ret;                              \
            }                                            \
        } while(0)

/**
 * @brief Check if the value is false; if false, goto the specified label.
 *
 * @param x Value to check
 * @param goto_tag Label to jump to if the value is false
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_FALSE_GOTO(x, goto_tag, fmt, ...) do { \
            if (unlikely((x) == false)) {                   \
                goto goto_tag;                              \
            }                                               \
        } while(0)

/**
 * @brief Check if the value is false; if false, return without a value.
 *
 * @param x Value to check
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_FALSE_EXIT(x, fmt, ...) do { \
            if (unlikely((x) == false)) {                   \
                return;                           \
            }                                     \
        } while(0)

/**
 * @brief Check if the value is not `ESP_OK`; if not, return the specified value.
 *
 * @param x Value to check
 * @param ret Value to return if the value is false
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_ERROR_RETURN(x, ret, fmt, ...) do { \
            esp_err_t err = (x);                        \
            if (unlikely(err != ESP_OK)) {                          \
                return ret;                              \
            }                                            \
        } while(0)

/**
 * @brief Check if the value is not `ESP_OK`; if not, goto the specified label.
 *
 * @param x Value to check
 * @param goto_tag Label to jump to if the value is false
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_ERROR_GOTO(x, goto_tag, fmt, ...) do { \
            if (unlikely((x) != ESP_OK)) {                   \
                goto goto_tag;                              \
            }                                               \
        } while(0)

/**
 * @brief Check if the value is not `ESP_OK`; if not, return without a value.
 *
 * @param x Value to check
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_ERROR_EXIT(x, fmt, ...) do { \
            if (unlikely((x) != ESP_OK)) {                   \
                return;                           \
            }                                     \
        } while(0)

/**
 * The `try {} catch {}` block is only available in C++ and `CONFIG_COMPILER_CXX_EXCEPTIONS = 1`
 */
#if defined(__cplusplus) && defined(CONFIG_COMPILER_CXX_EXCEPTIONS)
/**
 * @brief Check if the expression throws an exception; if it does, return the specified value.
 *
 * @param x Value to check
 * @param ret Value to return if the value is false
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_EXCEPTION_RETURN(x, ret, fmt, ...) do {\
        try { \
            x; \
        } catch (const std::exception &e) { \
            return ret; \
        } \
    } while (0)

/**
 * @brief Check if the expression throws an exception; if it does, goto the specified label.
 *
 * @param x Value to check
 * @param goto_tag Label to jump to if the value is false
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_EXCEPTION_GOTO(x, goto_tag, fmt, ...) do {\
        try { \
            x; \
        } catch (const std::exception &e) { \
            goto goto_tag; \
        } \
    } while (0)

/**
 * @brief Check if the expression throws an exception; if it does, return without a value.
 *
 * @param x Value to check
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_EXCEPTION_EXIT(x, fmt, ...) do {\
        try { \
            x; \
        } catch (const std::exception &e) { \
            return; \
        } \
    } while (0)
#endif // __cplusplus && CONFIG_COMPILER_CXX_EXCEPTIONS

#else

#ifndef unlikely
#define unlikely(x)  (x)
#endif

#if ESP_UTILS_CONF_CHECK_HANDLE_METHOD == ESP_UTILS_CHECK_HANDLE_WITH_ERROR_LOG

/**
 * @brief Check if the pointer is NULL; if NULL, log an error and return the specified value.
 *
 * @param x Pointer to check
 * @param ret Value to return if the pointer is NULL
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_NULL_RETURN(x, ret, fmt, ...) do { \
            if (unlikely((x) == NULL)) {                          \
                ESP_UTILS_LOGE(fmt, ##__VA_ARGS__);        \
                return ret;                             \
            }                                           \
        } while(0)

/**
 * @brief Check if the pointer is NULL; if NULL, log an error and goto the specified label.
 *
 * @param x Pointer to check
 * @param goto_tag Label to jump to if the pointer is NULL
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_NULL_GOTO(x, goto_tag, fmt, ...) do { \
            if (unlikely((x) == NULL)) {                             \
                ESP_UTILS_LOGE(fmt, ##__VA_ARGS__);           \
                goto goto_tag;                             \
            }                                              \
        } while(0)

/**
 * @brief Check if the pointer is NULL; if NULL, log an error and return without a value.
 *
 * @param x Pointer to check
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_NULL_EXIT(x, fmt, ...) do { \
            if (unlikely((x) == NULL)) {                   \
                ESP_UTILS_LOGE(fmt, ##__VA_ARGS__); \
                return;                          \
            }                                    \
        } while(0)

/**
 * @brief Check if the value is false; if false, log an error and return the specified value.
 *
 * @param x Value to check
 * @param ret Value to return if the value is false
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_FALSE_RETURN(x, ret, fmt, ...) do { \
            if (unlikely((x) == false)) {                          \
                ESP_UTILS_LOGE(fmt, ##__VA_ARGS__);         \
                return ret;                              \
            }                                            \
        } while(0)

/**
 * @brief Check if the value is false; if false, log an error and goto the specified label.
 *
 * @param x Value to check
 * @param goto_tag Label to jump to if the value is false
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_FALSE_GOTO(x, goto_tag, fmt, ...) do { \
            if (unlikely((x) == false)) {                   \
                ESP_UTILS_LOGE(fmt, ##__VA_ARGS__);            \
                goto goto_tag;                              \
            }                                               \
        } while(0)

/**
 * @brief Check if the value is false; if false, log an error and return without a value.
 *
 * @param x Value to check
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_FALSE_EXIT(x, fmt, ...) do { \
            if (unlikely((x) == false)) {                   \
                ESP_UTILS_LOGE(fmt, ##__VA_ARGS__);  \
                return;                           \
            }                                     \
        } while(0)

/**
 * @brief Check if the value is not `ESP_OK`; if not, log an error and return the specified value.
 *
 * @param x Value to check
 * @param ret Value to return if the value is false
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_ERROR_RETURN(x, ret, fmt, ...) do { \
            esp_err_t err = (x);                        \
            if (unlikely(err != ESP_OK)) {                          \
                ESP_UTILS_LOGE(fmt " [%s]", ##__VA_ARGS__, esp_err_to_name(err)); \
                return ret;                              \
            }                                            \
        } while(0)

/**
 * @brief Check if the value is not `ESP_OK`; if not, log an error and goto the specified label.
 *
 * @param x Value to check
 * @param goto_tag Label to jump to if the value is false
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_ERROR_GOTO(x, goto_tag, fmt, ...) do { \
            esp_err_t err = (x);                        \
            if (unlikely((err) != ESP_OK)) {                   \
                ESP_UTILS_LOGE(fmt " [%s]", ##__VA_ARGS__, esp_err_to_name(err)); \
                goto goto_tag;                              \
            }                                               \
        } while(0)

/**
 * @brief Check if the value is not `ESP_OK`; if not, log an error and return without a value.
 *
 * @param x Value to check
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_ERROR_EXIT(x, fmt, ...) do { \
            esp_err_t err = (x);                        \
            if (unlikely((err) != ESP_OK)) {                   \
                ESP_UTILS_LOGE(fmt " [%s]", ##__VA_ARGS__, esp_err_to_name(err)); \
                return;                           \
            }                                     \
        } while(0)

/**
 * The `try {} catch {}` block is only available in C++ and `CONFIG_COMPILER_CXX_EXCEPTIONS = 1`
 */
#if defined(__cplusplus) && defined(CONFIG_COMPILER_CXX_EXCEPTIONS)
/**
 * @brief Check if the expression throws an exception; if it does, log an error and return the specified value.
 *
 * @param x Value to check
 * @param ret Value to return if the value is false
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_EXCEPTION_RETURN(x, ret, fmt, ...) do {\
        try { \
            x; \
        } catch (const std::exception &e) { \
            ESP_UTILS_LOGE("Exception caught: %s", e.what()); \
            ESP_UTILS_LOGE(fmt, ##__VA_ARGS__); \
            return ret; \
        } \
    } while (0)

/**
 * @brief Check if the expression throws an exception; if it does, log an error and goto the specified label.
 *
 * @param x Value to check
 * @param goto_tag Label to jump to if the value is false
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_EXCEPTION_GOTO(x, goto_tag, fmt, ...) do {\
        try { \
            x; \
        } catch (const std::exception &e) { \
            ESP_UTILS_LOGE("Exception caught: %s", e.what()); \
            ESP_UTILS_LOGE(fmt, ##__VA_ARGS__); \
            goto goto_tag; \
        } \
    } while (0)

/**
 * @brief Check if the expression throws an exception; if it does, log an error and return without a value.
 *
 * @param x Value to check
 * @param fmt Format string for the error message
 * @param ... Additional arguments for the format string
 */
#define ESP_UTILS_CHECK_EXCEPTION_EXIT(x, fmt, ...) do {\
        try { \
            x; \
        } catch (const std::exception &e) { \
            ESP_UTILS_LOGE("Exception caught: %s", e.what()); \
            ESP_UTILS_LOGE(fmt, ##__VA_ARGS__); \
            return; \
        } \
    } while (0)
#endif // __cplusplus && CONFIG_COMPILER_CXX_EXCEPTIONS

#elif ESP_UTILS_CONF_CHECK_HANDLE_METHOD == ESP_UTILS_CHECK_HANDLE_WITH_ASSERT

#define ESP_UTILS_CHECK_NULL_RETURN(x, ...)             assert((x) != NULL)
#define ESP_UTILS_CHECK_NULL_GOTO(x, goto_tag, ...)     do { \
            assert((x) != NULL); \
            /* Avoid unused tag warning */ \
            if (0) { \
                goto goto_tag; \
            } \
    } while (0)
#define ESP_UTILS_CHECK_NULL_EXIT(x, ...)               assert((x) != NULL)

#define ESP_UTILS_CHECK_FALSE_RETURN(x, ...)            assert(x)
#define ESP_UTILS_CHECK_FALSE_GOTO(x, goto_tag, ...)    do { \
            assert(x); \
            /* Avoid unused tag warning */ \
            if (0) { \
                goto goto_tag; \
            } \
    } while (0)
#define ESP_UTILS_CHECK_FALSE_EXIT(x, ...)              assert(x)

#define ESP_UTILS_CHECK_ERROR_RETURN(x, ...)            assert((x) == ESP_OK)
#define ESP_UTILS_CHECK_ERROR_GOTO(x, goto_tag, ...)    do { \
            assert((x) == ESP_OK); \
            /* Avoid unused tag warning */ \
            if (0) { \
                goto goto_tag; \
            } \
    } while (0)
#define ESP_UTILS_CHECK_ERROR_EXIT(x, ...)              assert((x) == ESP_OK)

/**
 * The `try {} catch {}` block is only available in C++ and `CONFIG_COMPILER_CXX_EXCEPTIONS = 1`
 */
#if defined(__cplusplus) && defined(CONFIG_COMPILER_CXX_EXCEPTIONS)
#define ESP_UTILS_CHECK_EXCEPTION_RETURN(x, ...) do {\
        try { \
            x; \
        } catch (const std::exception &e) { \
            assert(false); \
        } \
    } while (0)

#define ESP_UTILS_CHECK_EXCEPTION_GOTO(x, goto_tag, ...) do {\
        try { \
            x; \
        } catch (const std::exception &e) { \
            assert(false); \
            goto goto_tag; \
        } \
    } while (0)

#define ESP_UTILS_CHECK_EXCEPTION_EXIT(x, ...) do {\
        try { \
            x; \
        } catch (const std::exception &e) { \
            assert(false); \
        } \
    } while (0)
#endif // __cplusplus && CONFIG_COMPILER_CXX_EXCEPTIONS

#endif // ESP_UTILS_CONF_CHECK_HANDLE_METHOD
#endif // ESP_UTILS_CONF_CHECK_HANDLE_METHOD

#ifdef __cplusplus
#ifndef ESP_UTILS_CHECK_EXCEPTION_RETURN
#define ESP_UTILS_CHECK_EXCEPTION_RETURN(x, ret, fmt, ...)      ((void)(x))
#endif // ESP_UTILS_CHECK_EXCEPTION_RETURN

#ifndef ESP_UTILS_CHECK_EXCEPTION_GOTO
#define ESP_UTILS_CHECK_EXCEPTION_GOTO(x, goto_tag, fmt, ...)   do { \
            (void)x; \
            /* Avoid unused tag warning */ \
            if (0) { \
                goto goto_tag; \
            } \
    } while (0)
#endif // ESP_UTILS_CHECK_EXCEPTION_GOTO

#ifndef ESP_UTILS_CHECK_EXCEPTION_EXIT
#define ESP_UTILS_CHECK_EXCEPTION_EXIT(x, fmt, ...)             ((void)(x))
#endif // ESP_UTILS_CHECK_EXCEPTION_EXIT
#endif // __cplusplus
