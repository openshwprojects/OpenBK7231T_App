/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <unordered_map>
#include "esp_lib_utils.h"

namespace esp_panel::utils {

template <typename K, typename V>
using unordered_map = std::unordered_map <
                      K, V, std::hash<K>, std::equal_to<K>, esp_utils::GeneralMemoryAllocator<std::pair<const K, V>>
                      >;

template <typename K, typename V>
using map = std::map<K, V, std::less<K>, esp_utils::GeneralMemoryAllocator<std::pair<const K, V>>>;

} // namespace esp_panel::utils
