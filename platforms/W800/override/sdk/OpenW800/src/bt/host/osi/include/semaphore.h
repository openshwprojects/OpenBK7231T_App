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

struct semaphore_t;
typedef struct semaphore_t semaphore_t;

// Creates a new semaphore with an initial value of |value|.
// Returns NULL on failure. The returned object must be released
// with |semaphore_free|.
semaphore_t *semaphore_new(unsigned int value);

// Frees a semaphore allocated with |semaphore_new|. |semaphore| may
// be NULL.
void semaphore_free(semaphore_t *semaphore);

// Decrements the value of |semaphore|. If it is 0, this call blocks until
// it becomes non-zero. |semaphore| may not be NULL.
void semaphore_wait(semaphore_t *semaphore);

// Tries to decrement the value of |semaphore|. Returns true if the value was
// decremented, false if the value was 0. This function never blocks. |semaphore|
// may not be NULL.
uint8_t semaphore_try_wait(semaphore_t *semaphore);

// Increments the value of |semaphore|. |semaphore| may not be NULL.
void semaphore_post(semaphore_t *semaphore);

// Returns a file descriptor representing this semaphore. The caller may
// only perform one operation on the file descriptor: select(2). If |select|
// indicates the fd is readable, the caller may call |semaphore_wait|
// without blocking. If select indicates the fd is writable, the caller may
// call |semaphore_post| without blocking. Note that there may be a race
// condition between calling |select| and |semaphore_wait| or |semaphore_post|
// which results in blocking behaviour.
//
// The caller must not close the returned file descriptor. |semaphore| may not
// be NULL.
int semaphore_get_fd(const semaphore_t *semaphore);
