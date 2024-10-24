/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
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

// This module manages the serial port over which HCI commands
// and data are sent/received.

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    USERIAL_PORT_1,
    USERIAL_PORT_2,
    USERIAL_PORT_3,
    USERIAL_PORT_4,
    USERIAL_PORT_5,
    USERIAL_PORT_6,
    USERIAL_PORT_7,
    USERIAL_PORT_8,
    USERIAL_PORT_9,
    USERIAL_PORT_10,
    USERIAL_PORT_11,
    USERIAL_PORT_12,
    USERIAL_PORT_13,
    USERIAL_PORT_14,
    USERIAL_PORT_15,
    USERIAL_PORT_16,
    USERIAL_PORT_17,
    USERIAL_PORT_18,
} userial_port_t;

// Initializes the userial module. This function should only ever be called once.
// It returns true if the module could be initialized, false if there was an error.
uint8_t userial_init(void);

// Opens the given serial port. Returns true if successful, false otherwise.
// Once this function is called, the userial module will begin producing
// buffers from data read off the serial port. If you wish to pause the
// production of buffers, call |userial_pause_reading|. You can then resume
// by calling |userial_resume_reading|. This function returns true if the
// serial port was successfully opened and buffer production has started. It
// returns false if there was an error.
uint8_t userial_open(userial_port_t port);
void userial_close(void);
void userial_close_reader(void);

// Reads a maximum of |len| bytes from the serial port into |p_buffer|.
// This function returns the number of bytes actually read, which may be
// less than |len|. This function will not block.
uint16_t userial_read(uint16_t msg_id, uint8_t *p_buffer, uint16_t len);

// Writes a maximum of |len| bytes from |p_data| to the serial port.
// This function returns the number of bytes actually written, which may be
// less than |len|. This function may block.
uint16_t userial_write(uint16_t msg_id, const uint8_t *p_data, uint16_t len);
