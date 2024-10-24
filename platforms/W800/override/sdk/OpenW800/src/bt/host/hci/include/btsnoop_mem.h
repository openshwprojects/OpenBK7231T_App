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

#include "bt_types.h"

// Callback invoked for each HCI packet.
// Highlander mode - there can be only one...
typedef void (*btsnoop_data_cb)(const uint16_t type, const uint8_t *p_data, const size_t len);

// This call sets the (one and only) callback that will
// be invoked once for each HCI packet/event.
void btsnoop_mem_set_callback(btsnoop_data_cb cb);

// This function is invoked every time an HCI packet
// is sent/received. Packets will be filtered  and then
// forwarded to the |btsnoop_data_cb|.
void btsnoop_mem_capture(const BT_HDR *p_buf);
