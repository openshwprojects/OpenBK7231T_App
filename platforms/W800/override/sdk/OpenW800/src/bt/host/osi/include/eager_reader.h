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

#include "osi/include/allocator.h"
#include "osi/include/thread.h"

typedef struct eager_reader_t eager_reader_t;
typedef struct reactor_t reactor_t;

typedef void (*eager_reader_cb)(eager_reader_t *reader, void *context);

// Creates a new eager reader object, which pulls data from |fd_to_read| into
// buffers of size |buffer_size| allocated using |allocator|, and has an
// internal read thread named |thread_name|. The returned object must be freed using
// |eager_reader_free|. |fd_to_read| must be valid, |buffer_size| and |max_buffer_count|
// must be greater than zero. |allocator| and |thread_name| may not be NULL.
eager_reader_t *eager_reader_new(
                int fd_to_read,
                const allocator_t *allocator,
                size_t buffer_size,
                size_t max_buffer_count,
                const char *thread_name
);

// Frees an eager reader object, and associated internal resources.
// |reader| may be NULL.
void eager_reader_free(eager_reader_t *reader);

// Registers |reader| with the |reactor|. When the reader has data
// |read_cb| will be called. The |context| parameter is passed, untouched, to |read_cb|.
// Neither |reader|, nor |reactor|, nor |read_cb| may be NULL. |context| may be NULL.
void eager_reader_register(eager_reader_t *reader, reactor_t *reactor, eager_reader_cb read_cb,
                           void *context);

// Unregisters |reader| from whichever reactor it is registered with, if any. This
// function is idempotent.
void eager_reader_unregister(eager_reader_t *reader);

// Reads up to |max_size| bytes into |buffer| from currently available bytes.
// NOT SAFE FOR READING FROM MULTIPLE THREADS
// but you should probably only be reading from one thread anyway,
// otherwise the byte stream probably doesn't make sense.
size_t eager_reader_read(eager_reader_t *reader, uint8_t *buffer, size_t max_size);

// Returns the inbound read thread for a given |reader| or NULL if the thread
// is not running.
thread_t *eager_reader_get_read_thread(const eager_reader_t *reader);
