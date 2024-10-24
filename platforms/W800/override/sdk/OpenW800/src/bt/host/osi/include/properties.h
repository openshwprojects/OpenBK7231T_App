/******************************************************************************
 *
 *  Copyright (C) 2016 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#pragma once

#if defined(OS_GENERIC)
#define PROPERTY_VALUE_MAX 92
#else
#include <cutils/properties.h>
#endif  // defined(OS_GENERIC)

// Get value associated with key |key| into |value|.
// Returns the length of the value which will never be greater than
// PROPERTY_VALUE_MAX - 1 and will always be zero terminated.
// (the length does not include the terminating zero).
// If the property read fails or returns an empty value, the |default_value|
// is used (if nonnull).
//int osi_property_get(const char *key, char *value, const char *default_value)

// Write value of property associated with key |key| to |value|.
// Returns 0 on success, < 0 on failure
//int osi_property_set(const char *key, const char *value);
