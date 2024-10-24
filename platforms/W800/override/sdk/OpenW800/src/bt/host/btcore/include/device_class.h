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

// Provides Class Of Device primitive as specified in the bluetooth spec.
// [Class Of Device](https://www.bluetooth.org/en-us/specification/assigned-numbers/baseband)

// Device class may be defined in other structures.
// Only use defined methods to manipulate internals.
typedef struct bt_device_class_t {
    uint8_t _[3];  // Do not access directly; use methods below.
} bt_device_class_t;

// Copies the |data| class of device stream into device class |dc|.  |dc|
// and |data| must not be NULL.
void device_class_from_stream(bt_device_class_t *dc, const uint8_t *data);

// Serializes the device class |dc| to pointer argument |data| in big endian
// format.  |len| must contain the buffer size of |data|.  Returns the actual
// number of bytes copied into |data|.  |dc| and |data| must not be NULL.
int device_class_to_stream(const bt_device_class_t *dc, uint8_t *data, size_t len);

// Copies the |data| class of device integer into device class |dc|.  |dc|
// must not be NULL.
void device_class_from_int(bt_device_class_t *dc, int data);

// Returns the device class |dc| in integer format.  |dc| must not be NULL.
int device_class_to_int(const bt_device_class_t *dc);

// Compares and returns |true| if two device classes |p1| and |p2| are equal.
// False otherwise.
uint8_t device_class_equals(const bt_device_class_t *p1, const bt_device_class_t *p2);

// Copies and returns |true| if the device class was successfully copied from
//  |p2| into |p1|.  False otherwise.
uint8_t device_class_copy(bt_device_class_t *dest, const bt_device_class_t *src);

// Query, getters and setters for the major device class.  |dc| must not be NULL.
int device_class_get_major_device(const bt_device_class_t *dc);
void device_class_set_major_device(bt_device_class_t *dc, int val);

// Query, getters and setters for the minor device class. |dc| must not be NULL.
int device_class_get_minor_device(const bt_device_class_t *dc);
void device_class_set_minor_device(bt_device_class_t *dc, int val);

// Query, getters and setters for the various major service class features.
// |dc| must not be NULL.
uint8_t device_class_get_limited(const bt_device_class_t *dc);
void device_class_set_limited(bt_device_class_t *dc, uint8_t set);

uint8_t device_class_get_positioning(const bt_device_class_t *dc);
void device_class_set_positioning(bt_device_class_t *dc, uint8_t set);

uint8_t device_class_get_networking(const bt_device_class_t *dc);
void device_class_set_networking(bt_device_class_t *dc, uint8_t set);

uint8_t device_class_get_rendering(const bt_device_class_t *dc);
void device_class_set_rendering(bt_device_class_t *dc, uint8_t set);

uint8_t device_class_get_capturing(const bt_device_class_t *dc);
void device_class_set_capturing(bt_device_class_t *dc, uint8_t set);

uint8_t device_class_get_object_transfer(const bt_device_class_t *dc);
void device_class_set_object_transfer(bt_device_class_t *dc, uint8_t set);

uint8_t device_class_get_audio(const bt_device_class_t *dc);
void device_class_set_audio(bt_device_class_t *dc, uint8_t set);

uint8_t device_class_get_telephony(const bt_device_class_t *dc);
void device_class_set_telephony(bt_device_class_t *dc, uint8_t set);

uint8_t device_class_get_information(const bt_device_class_t *dc);
void device_class_set_information(bt_device_class_t *dc, uint8_t set);
