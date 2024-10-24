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

#define BTSNOOZ_CURRENT_VERSION 0x02

// The preamble is stored un-encrypted as the first part
// of the file.
typedef struct btsnooz_preamble_t {
    uint8_t version;
    uint64_t last_timestamp_ms;
} __attribute__((__packed__)) btsnooz_preamble_t;

// One header for each HCI packet
typedef struct btsnooz_header_t {
    uint16_t length;
    uint16_t packet_length;
    uint32_t delta_time_ms;
    uint8_t type;
} __attribute__((__packed__)) btsnooz_header_t;

// Initializes btsnoop memory logging and registers
void btif_debug_btsnoop_init(void);

// Writes btsnoop data base64 encoded to fd
void btif_debug_btsnoop_dump(int fd);
