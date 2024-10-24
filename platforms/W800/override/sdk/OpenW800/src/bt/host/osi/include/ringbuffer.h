/******************************************************************************
 *
 *  Copyright (C) 2015 Google Inc.
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

#include <stdint.h>

typedef struct ringbuffer_t ringbuffer_t;

// NOTE:
// None of the functions below are thread safe when it comes to accessing the
// *rb pointer. It is *NOT* possible to insert and pop/delete at the same time.
// Callers must protect the *rb pointer separately.

// Create a ringbuffer with the specified size
// Returns NULL if memory allocation failed. Resulting pointer must be freed
// using |ringbuffer_free|.
ringbuffer_t *ringbuffer_init(const size_t size);

// Frees the ringbuffer structure and buffer
// Save to call with NULL.
void ringbuffer_free(ringbuffer_t *rb);

// Returns remaining buffer size
size_t ringbuffer_available(const ringbuffer_t *rb);

// Returns size of data in buffer
size_t ringbuffer_size(const ringbuffer_t *rb);

// Attempts to insert up to |length| bytes of data at |p| into the buffer
// Return actual number of bytes added. Can be less than |length| if buffer
// is full.
size_t ringbuffer_insert(ringbuffer_t *rb, const uint8_t *p, size_t length);

// Peek |length| number of bytes from the ringbuffer, starting at |offset|,
// into the buffer |p|. Return the actual number of bytes peeked. Can be less
// than |length| if there is less than |length| data available. |offset| must
// be non-negative.
size_t ringbuffer_peek(const ringbuffer_t *rb, int offset, uint8_t *p, size_t length);

// Does the same as |ringbuffer_peek|, but also advances the ring buffer head
size_t ringbuffer_pop(ringbuffer_t *rb, uint8_t *p, size_t length);

// Deletes |length| bytes from the ringbuffer starting from the head
// Return actual number of bytes deleted.
size_t ringbuffer_delete(ringbuffer_t *rb, size_t length);
