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
#include <stdbool.h>

typedef struct uuid_string_t uuid_string_t;

// Creates uuid string structure to hold a well formed UUID
// string.  Must release resources with |uuid_string_free|.
// Returns NULL if no memory.
uuid_string_t *uuid_string_new(void);

// Frees a uuid string structure created from |uuid_string_new|.
// |uuid_string| may be NULL.
void uuid_string_free(uuid_string_t *uuid_string);

// Returns a string pointer to the well formed UUID string
// entry.  |uuid_string| must not be NULL.
const char *uuid_string_data(const uuid_string_t *uuid_string);

// Creates uuid structure from a well formed UUID string
// |uuid_string|.  The caller takes ownership of the uuid
// structure and must release resources with |uuid_free|.
// |uuid_string| must not be NULL.
//
// Returns NULL if |uuid_string| is malformed or no memory.
//
// A well formed UUID string is structured like this:
//   "00112233-4455-6677-8899-aabbccddeeff"
tls_bt_uuid_t *uuid_new(const char *uuid_string);

// Frees a uuid structure created from |uuid_new| and friends.
// |uuid| may be NULL.
void uuid_free(tls_bt_uuid_t *uuid);

// Returns true if the UUID is all zeros, false otherwise.
// |uuid| may not be NULL.
uint8_t uuid_is_empty(const tls_bt_uuid_t *uuid);

// Returns true if the two UUIDs are equal, false otherwise.
// |first| and |second| may not be NULL.
uint8_t uuid_is_equal(const tls_bt_uuid_t *first, const tls_bt_uuid_t *second);

// Copies uuid |src| into |dest| and returns a pointer to |dest|.
// |src| and |dest| must not be NULL.
tls_bt_uuid_t *uuid_copy(tls_bt_uuid_t *dest, const tls_bt_uuid_t *src);

// Converts contents of |uuid| to a well formed UUID string
// |uuid_string| using numbers and lower case letter.  |uuid|
// and |uuid_string| must not be NULL.
void uuid_to_string(const tls_bt_uuid_t *uuid, uuid_string_t *uuid_string);

// Converts contents of |uuid| to a short uuid if possible.  Returns
// true if conversion is possible, false otherwise.
// |uuid|, |uuid16| and |uuid32| must not be NULL.
uint8_t uuid_128_to_16(const tls_bt_uuid_t *uuid, uint16_t *uuid16);
uint8_t uuid_128_to_32(const tls_bt_uuid_t *uuid, uint32_t *uuid32);
