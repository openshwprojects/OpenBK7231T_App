/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct array_t array_t;

// Returns a new array object that stores elements of size |element_size|. The returned
// object must be freed with |array_free|. |element_size| must be greater than 0. Returns
// NULL on failure.
array_t *array_new(size_t element_size);

// Frees an array that was allocated with |array_new|. |array| may be NULL.
void array_free(array_t *array);

// Returns a pointer to the first stored element in |array|. |array| must not be NULL.
void *array_ptr(const array_t *array);

// Returns a pointer to the |index|th element of |array|. |index| must be less than
// the array's length. |array| must not be NULL.
void *array_at(const array_t *array, size_t index);

// Returns the number of elements stored in |array|. |array| must not be NULL.
size_t array_length(const array_t *array);

// Inserts an element to the end of |array| by value. For example, a caller
// may simply call array_append_value(array, 5) instead of storing 5 into a
// variable and then inserting by pointer. Although |value| is a uint32_t,
// only the lowest |element_size| bytes will be stored. |array| must not be
// NULL. Returns true if the element could be inserted into the array, false
// on error.
uint8_t array_append_value(array_t *array, uint32_t value);

// Inserts an element to the end of |array|. The value pointed to by |data| must
// be at least |element_size| bytes long and will be copied into the array. Neither
// |array| nor |data| may be NULL. Returns true if the element could be inserted into
// the array, false on error.
uint8_t array_append_ptr(array_t *array, void *data);
