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

typedef struct {
    void (*init_stack)(void);
    void (*start_up_stack_async)(void);
    void (*shut_down_stack_async)(void);
    void (*clean_up_stack)(void);

    uint8_t (*get_stack_is_running)(void);
} stack_manager_t;

const stack_manager_t *stack_manager_get_interface();

// TODO(zachoverflow): remove this terrible hack once the startup sequence is more sane
future_t *stack_manager_get_hack_future();
