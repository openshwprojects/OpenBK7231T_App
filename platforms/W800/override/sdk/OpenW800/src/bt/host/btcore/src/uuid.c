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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btcore/include/uuid.h"
#include "osi/include/allocator.h"

static const size_t UUID_WELL_FORMED_STRING_LEN = 36;
static const size_t UUID_WELL_FORMED_STRING_LEN_WITH_NULL = 36 + 1;

typedef struct uuid_string_t {
    char string[0];
} uuid_string_t;

static const tls_bt_uuid_t empty_uuid = {{ 0 }};

// The base UUID is used for calculating 128-bit UUIDs from 16 and
// 32 bit UUIDs as described in the SDP specification.
static const tls_bt_uuid_t base_uuid = {
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
        0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb,
    }
};

static uint8_t uuid_is_base(const tls_bt_uuid_t *uuid);

uuid_string_t *uuid_string_new(void)
{
    return GKI_getbuf(UUID_WELL_FORMED_STRING_LEN_WITH_NULL);
}

void uuid_string_free(uuid_string_t *uuid_string)
{
    GKI_freebuf(uuid_string);
}

const char *uuid_string_data(const uuid_string_t *uuid_string)
{
    assert(uuid_string != NULL);
    return (const char *)uuid_string->string;
}

tls_bt_uuid_t *uuid_new(const char *uuid_string)
{
    assert(uuid_string != NULL);

    if(strlen(uuid_string) < UUID_WELL_FORMED_STRING_LEN) {
        return NULL;
    }

    if(uuid_string[8] != '-' || uuid_string[13] != '-' || uuid_string[18] != '-'
            || uuid_string[23] != '-') {
        return NULL;
    }

    tls_bt_uuid_t *uuid = GKI_getbuf(sizeof(tls_bt_uuid_t));
    const char *s = uuid_string;

    for(size_t i = 0; i < sizeof(tls_bt_uuid_t); ++i, s += 2) {
        char buf[3] = {0};
        buf[0] = s[0];
        buf[1] = s[1];
        uuid->uu[i] = strtoul(buf, NULL, 16);

        // Adjust by skipping the dashes
        switch(i) {
            case 3:
            case 5:
            case 7:
            case 9:
                s++;
                break;
        }
    }

    return uuid;
}

void uuid_free(tls_bt_uuid_t *uuid)
{
    GKI_freebuf(uuid);
}

uint8_t uuid_is_empty(const tls_bt_uuid_t *uuid)
{
    return !uuid || !memcmp(uuid, &empty_uuid, sizeof(tls_bt_uuid_t));
}

uint8_t uuid_is_equal(const tls_bt_uuid_t *first, const tls_bt_uuid_t *second)
{
    assert(first != NULL);
    assert(second != NULL);
    return !memcmp(first, second, sizeof(tls_bt_uuid_t));
}

tls_bt_uuid_t *uuid_copy(tls_bt_uuid_t *dest, const tls_bt_uuid_t *src)
{
    assert(dest != NULL);
    assert(src != NULL);
    return (tls_bt_uuid_t *)memcpy(dest, src, sizeof(tls_bt_uuid_t));
}

uint8_t uuid_128_to_16(const tls_bt_uuid_t *uuid, uint16_t *uuid16)
{
    assert(uuid != NULL);
    assert(uuid16 != NULL);

    if(!uuid_is_base(uuid)) {
        return false;
    }

    *uuid16 = (uuid->uu[2] << 8) + uuid->uu[3];
    return true;
}

uint8_t uuid_128_to_32(const tls_bt_uuid_t *uuid, uint32_t *uuid32)
{
    assert(uuid != NULL);
    assert(uuid32 != NULL);

    if(!uuid_is_base(uuid)) {
        return false;
    }

    *uuid32 = (uuid->uu[0] << 24) + (uuid->uu[1] << 16) + (uuid->uu[2] << 8) + uuid->uu[3];
    return true;
}

void uuid_to_string(const tls_bt_uuid_t *uuid, uuid_string_t *uuid_string)
{
    assert(uuid != NULL);
    assert(uuid_string != NULL);
    char *string = uuid_string->string;

    for(int i = 0; i < 4; i++) {
        string += sprintf(string, "%02x", uuid->uu[i]);
    }

    string += sprintf(string, "-");

    for(int i = 4; i < 6; i++) {
        string += sprintf(string, "%02x", uuid->uu[i]);
    }

    string += sprintf(string, "-");

    for(int i = 6; i < 8; i++) {
        string += sprintf(string, "%02x", uuid->uu[i]);
    }

    string += sprintf(string, "-");

    for(int i = 8; i < 10; i++) {
        string += sprintf(string, "%02x", uuid->uu[i]);
    }

    string += sprintf(string, "-");

    for(int i = 10; i < 16; i++) {
        string += sprintf(string, "%02x", uuid->uu[i]);
    }
}

static uint8_t uuid_is_base(const tls_bt_uuid_t *uuid)
{
    if(!uuid) {
        return false;
    }

    for(int i = 4; i < 16; i++) {
        if(uuid->uu[i] != base_uuid.uu[i]) {
            return false;
        }
    }

    return true;
}
