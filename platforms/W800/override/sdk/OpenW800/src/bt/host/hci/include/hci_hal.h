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

#include "osi/include/thread.h"
#include "vendor.h"

typedef enum {
    DATA_TYPE_COMMAND = 1,
    DATA_TYPE_ACL     = 2,
    DATA_TYPE_SCO     = 3,
    DATA_TYPE_EVENT   = 4
} serial_data_type_t;

typedef void (*data_ready_cb)(serial_data_type_t type);

typedef struct {
    // Called when the HAL detects inbound data.
    // Data |type| may be ACL, SCO, or EVENT.
    // Executes in the context of the thread supplied to |init|.
    data_ready_cb data_ready;

    /*
    // Called when the HAL detects inbound astronauts named Dave.
    // HAL will deny all requests to open the pod bay doors after this.
    dave_ready_cb dave_ready;
    */
} hci_hal_callbacks_t;

typedef struct hci_hal_t {
    // Initialize the HAL, with |upper_callbacks| and |upper_thread| to run in the context of.
    uint8_t (*init)(const hci_hal_callbacks_t *upper_callbacks, thread_t *upper_thread);

    // Connect to the underlying hardware, and let data start flowing.
    uint8_t (*open)(void);
    // Disconnect from the underlying hardware, and close the HAL.
    // "Daisy, Daisy..."
    void (*close)(void);

    // Retrieve up to |max_size| bytes for ACL, SCO, or EVENT data packets into
    // |buffer|. Only guaranteed to be correct in the context of a data_ready
    // callback of the corresponding type.
    size_t (*read_data)(serial_data_type_t type, uint8_t *buffer, size_t max_size);
    // The upper layer must call this to notify the HAL that it has finished
    // reading a packet of the specified |type|. Underlying implementations that
    // use shared channels for multiple data types depend on this to know when
    // to reinterpret the data stream.
    void (*packet_finished)(serial_data_type_t type);
    // Transmit COMMAND, ACL, or SCO data packets.
    // |data| may not be NULL. |length| must be greater than zero.
    //
    // IMPORTANT NOTE:
    // Depending on the underlying implementation, the byte right
    // before the beginning of |data| may be borrowed during this call
    // and then restored to its original value.
    // This is safe in the bluetooth context, because there is always a buffer
    // header that prefixes data you're sending.
    uint16_t (*transmit_data)(serial_data_type_t type, uint8_t *data, uint16_t length);
} hci_hal_t;

// Gets the correct hal implementation, as compiled for.
const hci_hal_t *hci_hal_get_interface(void);

const hci_hal_t *hci_hal_h4_get_interface(void);
const hci_hal_t *hci_hal_h4_get_test_interface(vendor_t *vendor_interface);

const hci_hal_t *hci_hal_mct_get_interface(void);
const hci_hal_t *hci_hal_mct_get_test_interface(vendor_t *vendor_interface);
