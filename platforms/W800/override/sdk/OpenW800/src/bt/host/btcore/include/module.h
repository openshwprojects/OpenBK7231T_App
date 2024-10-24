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

#include "osi/include/future.h"
#include "osi/include/thread.h"

typedef future_t *(*module_lifecycle_fn)(void);

typedef struct {
    const char *name;
    module_lifecycle_fn init;
    module_lifecycle_fn start_up;
    module_lifecycle_fn shut_down;
    module_lifecycle_fn clean_up;
    const char *dependencies[];
} module_t;

// Prepares module management. Must be called before doing anything with modules.
void module_management_start(void);
// Cleans up all module management resources.
void module_management_stop(void);

const module_t *get_module(const char *name);

// Initialize the provided module. |module| may not be NULL
// and must not be initialized.
uint8_t module_init(const module_t *module);
// Start up the provided module. |module| may not be NULL
// and must be initialized or have no init function.
uint8_t module_start_up(const module_t *module);
// Shut down the provided module. |module| may not be NULL.
// If not started, does nothing.
void module_shut_down(const module_t *module);
// Clean up the provided module. |module| may not be NULL.
// If not initialized, does nothing.
void module_clean_up(const module_t *module);

// Temporary callbacked wrapper for module start up, so real modules can be
// spliced into the current janky startup sequence. Runs on a separate thread,
// which terminates when the module start up has finished. When module startup
// has finished, |callback| is called within the context of |callback_thread|
// with |FUTURE_SUCCESS| or |FUTURE_FAIL| depending on whether startup succeeded
// or not.
void module_start_up_callbacked_wrapper(
                const module_t *module,
                thread_t *callback_thread,
                thread_fn callback
);
