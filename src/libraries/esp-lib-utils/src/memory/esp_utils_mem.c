/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "esp_heap_caps.h"
#include "esp_utils_conf_internal.h"
#include "check/esp_utils_check.h"
#include "log/esp_utils_log.h"
#if ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_ESP
#include "esp_heap_caps.h"
#elif ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_CUSTOM
#include ESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_INCLUDE
#elif ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_MICROPYTHON
#include <py/mpconfig.h>
#include <py/misc.h>
#include <py/gc.h>
#endif // ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE

#define PRINT_INFO_BUFFER_SIZE  256

static bool is_alloc_enabled = ESP_UTILS_CONF_MEM_GEN_ALLOC_DEFAULT_ENABLE;

void esp_utils_mem_gen_enable_alloc(bool enable)
{
    is_alloc_enabled = enable;
}

bool esp_utils_mem_check_alloc_enabled(void)
{
    return is_alloc_enabled;
}

void *esp_utils_mem_gen_malloc(size_t size)
{
    ESP_UTILS_LOG_TRACE_ENTER();

    void *p = NULL;

    if (!is_alloc_enabled) {
        p = malloc(size);
        goto end;
    }

#if ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_STDLIB
    p = malloc(size);
#elif ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_ESP
    p = heap_caps_aligned_alloc(ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_ALIGN, size, ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_CAPS);
#elif ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_CUSTOM
    p = ESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_MALLOC(size);
#elif ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_MICROPYTHON
#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
    p = gc_alloc(size, true);
#else
    p = m_malloc(size);
#endif // MICROPY_MALLOC_USES_ALLOCATED_SIZE
#endif // ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE

end:
    ESP_UTILS_LOGD("Malloc @%p: %d", p, (int)size);

    ESP_UTILS_LOG_TRACE_EXIT();

    return p;
}

void esp_utils_mem_gen_free(void *p)
{
    ESP_UTILS_LOG_TRACE_ENTER();

    ESP_UTILS_LOGD("Free @%p", p);

    if (!is_alloc_enabled) {
        free(p);
        goto end;
    }

#if ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_STDLIB
    free(p);
#elif ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_ESP
    heap_caps_free(p);
#elif ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_CUSTOM
    ESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_FREE(p);
#elif ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_MICROPYTHON
#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
    gc_free(p);
#else
    m_free(p);
#endif // MICROPY_MALLOC_USES_ALLOCATED_SIZE
#endif // ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE

end:
    ESP_UTILS_LOG_TRACE_EXIT();
}

void *esp_utils_mem_gen_calloc(size_t n, size_t size)
{
    ESP_UTILS_LOG_TRACE_ENTER();

    size_t total_size = (size_t)n * size;
    void *p = esp_utils_mem_gen_malloc(total_size);
    if (p != NULL) {
        memset(p, 0, total_size);
    }

    ESP_UTILS_LOG_TRACE_EXIT();

    return p;
}

bool esp_utils_mem_print_info(void)
{
    ESP_UTILS_LOG_TRACE_ENTER();

    char *buffer = esp_utils_mem_gen_calloc(1, PRINT_INFO_BUFFER_SIZE);
    ESP_UTILS_CHECK_NULL_RETURN(buffer, false, "Allocate buffer failed");

    snprintf(
        buffer, PRINT_INFO_BUFFER_SIZE,
        "ESP Memory Info:\n"
        "          Biggest /     Free /    Total\n"
        " SRAM : [%8d / %8d / %8d]\n"
        "PSRAM : [%8d / %8d / %8d]",
        (int)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), (int)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        (int)heap_caps_get_total_size(MALLOC_CAP_INTERNAL),
        (int)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM), (int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        (int)heap_caps_get_total_size(MALLOC_CAP_SPIRAM)
    );
    printf("%s\n", buffer);

    esp_utils_mem_gen_free(buffer);

    ESP_UTILS_LOG_TRACE_EXIT();

    return true;
}
