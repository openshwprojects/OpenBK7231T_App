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

typedef struct hci_t hci_t;

typedef struct hci_inject_t {
    // Starts the HCI injection module. Returns true on success, false on failure.
    // Once started, this module must be shut down with |close|.
    uint8_t (*open)(const hci_t *hci_interface);

    // Shuts down the HCI injection module.
    void (*close)(void);
} hci_inject_t;

const hci_inject_t *hci_inject_get_interface();
