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
#include <stdint.h>

#include "osi/include/fixed_queue.h"

#define DISPATCHER_NAME_MAX 16

typedef struct data_dispatcher_t data_dispatcher_t;
typedef uintptr_t data_dispatcher_type_t;

// Creates a new data dispatcher object, with the given name.
// The returned object must be freed by calling |data_dispatcher_free|.
// Returns NULL on failure. |name| may not be NULL.
data_dispatcher_t *data_dispatcher_new(const char *name);

// Frees a data dispatcher object created by |data_dispatcher_new|.
// |data_dispatcher| may be NULL.
void data_dispatcher_free(data_dispatcher_t *dispatcher);

// Registers |type| and |queue| with the data dispatcher so that data
// sent under |type| ends up in |queue|. If |type| is already registered,
// it is replaced. If |queue| is NULL, the existing registration is
// removed, if it exists. |dispatcher| may not be NULL.
void data_dispatcher_register(data_dispatcher_t *dispatcher, data_dispatcher_type_t type,
                              fixed_queue_t *queue);

// Registers a default queue to send data to when there is not a specific
// type/queue relationship registered. If a default queue is already registered,
// it is replaced. If |queue| is NULL, the existing registration is
// removed, if it exists. |dispatcher| may not be NULL.
void data_dispatcher_register_default(data_dispatcher_t *dispatcher, fixed_queue_t *queue);

// Dispatches |data| to the queue registered for |type|. If no such registration
// exists, it is dispatched to the default queue if it exists.
// Neither |dispatcher| nor |data| may be NULL.
// Returns true if data dispatch was successful.
uint8_t data_dispatcher_dispatch(data_dispatcher_t *dispatcher, data_dispatcher_type_t type,
                                 void *data);
