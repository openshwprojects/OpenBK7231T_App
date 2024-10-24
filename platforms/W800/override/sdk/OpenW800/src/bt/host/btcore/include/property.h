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

#include "bluetooth.h"
#include <stdint.h>
#include <stdlib.h>

#include "btcore/include/device_class.h"

// Copies an array of consecutive properties of |count| to a newly
// allocated array. |properties| must not be NULL.
tls_bt_property_t *property_copy_array(const tls_bt_property_t *properties, size_t count);

// Copies |src| to |dest|. Returns the value of |dest|.
// |src| and |dest| must not be NULL.
tls_bt_property_t *property_copy(tls_bt_property_t *dest, const tls_bt_property_t *src);

// Returns true if the value of the two properties |p1| and |p2| are equal.
// |p1| and |p2| must not be NULL.
uint8_t property_equals(const tls_bt_property_t *p1, const tls_bt_property_t *p2);

// Property resource allocations. Caller is expected to free |property|
// using |property_free| or |property_free_array|.
// Parameter must not be NULL. A copy of the parameter is made and
// stored in the property.
tls_bt_property_t *property_new_addr(const tls_bt_addr_t *addr);
tls_bt_property_t *property_new_device_class(const bt_device_class_t *dc);
tls_bt_property_t *property_new_device_type(bt_device_type_t device_type);
tls_bt_property_t *property_new_discovery_timeout(const uint32_t timeout);
tls_bt_property_t *property_new_name(const char *name);
tls_bt_property_t *property_new_rssi(const int8_t rssi);
tls_bt_property_t *property_new_scan_mode(bt_scan_mode_t scan_mode);
tls_bt_property_t *property_new_uuids(const tls_bt_uuid_t *uuid, size_t count);

// Property resource frees both property and value.
void property_free(tls_bt_property_t *property);
void property_free_array(tls_bt_property_t *properties, size_t count);

// Value check convenience methods. The contents of the property are
// checked for the respective validity and returns true, false otherwise.
// |property| must not be NULL.
uint8_t property_is_addr(const tls_bt_property_t *property);
uint8_t property_is_device_class(const tls_bt_property_t *property);
uint8_t property_is_device_type(const tls_bt_property_t *property);
uint8_t property_is_discovery_timeout(const tls_bt_property_t *property);
uint8_t property_is_name(const tls_bt_property_t *property);
uint8_t property_is_rssi(const tls_bt_property_t *property);
uint8_t property_is_scan_mode(const tls_bt_property_t *property);
uint8_t property_is_uuids(const tls_bt_property_t *property);

// Value conversion convenience methods. The contents of the property are
// properly typed and returned to the caller. |property| must not be NULL.
const tls_bt_addr_t *property_as_addr(const tls_bt_property_t *property);
const bt_device_class_t *property_as_device_class(const tls_bt_property_t *property);
bt_device_type_t property_as_device_type(const tls_bt_property_t *property);
uint32_t property_as_discovery_timeout(const tls_bt_property_t *property);
const tls_bt_bdname_t *property_as_name(const tls_bt_property_t *property);
int8_t property_as_rssi(const tls_bt_property_t *property);
bt_scan_mode_t property_as_scan_mode(const tls_bt_property_t *property);
const tls_bt_uuid_t *property_as_uuids(const tls_bt_property_t *property, size_t *count);
