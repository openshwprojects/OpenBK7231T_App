/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <memory>
#include "esp_lib_utils.h"

namespace esp_panel::utils {

template <typename T, typename... Args>
std::shared_ptr<T> make_shared(Args &&... args)
{
    return std::allocate_shared<T, esp_utils::GeneralMemoryAllocator<T>>(
               esp_utils::GeneralMemoryAllocator<T>(), std::forward<Args>(args)...
           );
}

} // namespace esp_panel::utils
