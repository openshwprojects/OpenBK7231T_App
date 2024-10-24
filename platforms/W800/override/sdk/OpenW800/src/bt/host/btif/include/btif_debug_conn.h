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

#include "bluetooth.h"

#include "gatt_api.h"

typedef enum {
    BTIF_DEBUG_CONNECTED = 1,
    BTIF_DEBUG_DISCONNECTED
} btif_debug_conn_state_t;

// Report a connection state change
void btif_debug_conn_state(const tls_bt_addr_t bda, const btif_debug_conn_state_t state,
                           const tGATT_DISCONN_REASON disconnect_reason);

void btif_debug_conn_dump(int fd);
