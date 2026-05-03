/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#include <memory>
#include "unity.h"
#define ESP_UTILS_LOG_TAG "TestCpp"
#include "esp_lib_utils.h"
#include "esp_utils_helpers.h"

TEST_CASE("Test log functions on cpp", "[utils][log][CPP]")
{
    ESP_UTILS_LOG_TRACE_ENTER();

    ESP_UTILS_LOGD("This is a debug message");
    ESP_UTILS_LOGI("This is an info message");
    ESP_UTILS_LOGW("This is a warning message");
    ESP_UTILS_LOGE("This is an error message");

    ESP_UTILS_LOG_TRACE_EXIT();
}

#define MALLOC_GOOD_SIZE    (1 * 1024)
#define MALLOC_BAD_SIZE     (1 * 1024 * 1024)

template <typename T, typename... Args>
std::shared_ptr<T> make_shared(Args &&... args)
{
    return std::allocate_shared<T, esp_utils::GeneralMemoryAllocator<T>>(
               esp_utils::GeneralMemoryAllocator<T>(), std::forward<Args>(args)...
           );
}

template <int N>
class TestClass {
public:
    char buffer[N] = { 128 };
};

using TestGoodClass = TestClass<MALLOC_GOOD_SIZE>;
using TestBadClass = TestClass<MALLOC_BAD_SIZE>;

TEST_CASE("Test memory functions on cpp", "[utils][memory][CPP]")
{
    TEST_ASSERT_TRUE_MESSAGE(esp_utils_mem_print_info(), "Print memory info failed");

    std::shared_ptr<TestGoodClass> good_ptr = nullptr;
    ESP_UTILS_CHECK_EXCEPTION_GOTO(
        (good_ptr = make_shared<TestGoodClass>()), err, "Failed to allocate memory size: %d",
        MALLOC_GOOD_SIZE
    );
    ESP_UTILS_LOGI("Malloced value: %d", good_ptr->buffer[0]);

    {
        std::shared_ptr<TestBadClass> bad_ptr = nullptr;
        ESP_UTILS_CHECK_EXCEPTION_GOTO(
            (bad_ptr = make_shared<TestBadClass>()), end, "Failed to allocate memory size: %d", MALLOC_BAD_SIZE
        );
        ESP_UTILS_LOGI("Malloced value: %d", bad_ptr->buffer[0]);
    }

err:
    TEST_ASSERT(false && "Memory allocation failed");

end:
    return;
}

static bool test_check_false_return(void)
{
    ESP_UTILS_CHECK_FALSE_RETURN(true, false, "Check false return failed");
    ESP_UTILS_CHECK_FALSE_RETURN(false, true, "Check false return success");

    TEST_ASSERT(false && "Check false return failed");

    return false;
}

static bool test_check_false_goto(void)
{
    ESP_UTILS_CHECK_FALSE_GOTO(true, err, "Check false goto failed");
    ESP_UTILS_CHECK_FALSE_GOTO(false, end, "Check false goto success");

err:
    TEST_ASSERT(false && "Check false goto failed");

    return false;

end:
    return true;
}

static bool test_check_false_exit_result = false;
static void test_check_false_exit(void)
{
    ESP_UTILS_CHECK_FALSE_EXIT(true, "Check false exit failed");
    test_check_false_exit_result = true;
    ESP_UTILS_CHECK_FALSE_EXIT(false, "Check false exit success");

    TEST_ASSERT(false && "Check false exit failed");
}

static bool test_check_error_return(void)
{
    ESP_UTILS_CHECK_ERROR_RETURN(ESP_OK, false, "Check error return failed");
    ESP_UTILS_CHECK_ERROR_RETURN(ESP_FAIL, true, "Check error return success");

    TEST_ASSERT(false && "Check error return failed");

    return false;
}

static bool test_check_error_goto(void)
{
    ESP_UTILS_CHECK_ERROR_GOTO(ESP_OK, err, "Check error goto failed");
    ESP_UTILS_CHECK_ERROR_GOTO(ESP_FAIL, end, "Check error goto success");

err:
    TEST_ASSERT(false && "Check error goto failed");

    return false;

end:
    return true;
}

static bool test_check_error_exit_result = false;
static void test_check_error_exit(void)
{
    ESP_UTILS_CHECK_ERROR_EXIT(ESP_OK, "Check error exit failed");
    test_check_error_exit_result = true;
    ESP_UTILS_CHECK_ERROR_EXIT(ESP_FAIL, "Check error exit success");

    TEST_ASSERT(false && "Check error exit failed");
}

static bool test_check_null_return(void)
{
    ESP_UTILS_CHECK_NULL_RETURN((void *)1, false, "Check null return failed");
    ESP_UTILS_CHECK_NULL_RETURN(NULL, true, "Check null return success");

    TEST_ASSERT(false && "Check null return failed");

    return false;
}

static bool test_check_null_goto(void)
{
    ESP_UTILS_CHECK_NULL_GOTO((void *)1, err, "Check null goto failed");
    ESP_UTILS_CHECK_NULL_GOTO(NULL, end, "Check null goto success");

err:
    TEST_ASSERT(false && "Check null goto failed");

    return false;

end:
    return true;
}

static bool test_check_null_exit_result = false;
static void test_check_null_exit(void)
{
    ESP_UTILS_CHECK_NULL_EXIT((void *)1, "Check null exit failed");
    test_check_null_exit_result = true;
    ESP_UTILS_CHECK_NULL_EXIT(NULL, "Check null exit success");

    TEST_ASSERT(false && "Check null exit failed");
}

TEST_CASE("Test check functions on cpp", "[utils][check][CPP]")
{
    test_check_false_return();
    test_check_false_goto();
    test_check_false_exit();
    TEST_ASSERT(test_check_false_exit_result);
    test_check_error_return();
    test_check_error_goto();
    test_check_error_exit();
    TEST_ASSERT(test_check_error_exit_result);
    test_check_null_return();
    test_check_null_goto();
    test_check_null_exit();
    TEST_ASSERT(test_check_null_exit_result);
}
