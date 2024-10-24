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
#include <string.h>
#include "generic.h"
#include "btcore/include/device_class.h"
#include "osi/include/osi.h"

typedef struct _bt_device_class_t {
    uint32_t unused : 2;          // LSBs
    uint32_t minor_device : 6;
    uint32_t major_device : 5;
    uint32_t major_service : 11;  // MSBs
} __attribute__((__packed__)) _bt_device_class_t;

// Convenience to interpret raw device class bytes.
#define DC(x) ((_bt_device_class_t *)x)

// Ensure the internal device class implementation and public one
// have equal size.
COMPILE_ASSERT(sizeof(_bt_device_class_t) == sizeof(bt_device_class_t));

// [Major Service Classes](https://www.bluetooth.org/en-us/specification/assigned-numbers/baseband)
enum {
    DC_LIMITED_DISCOVERABLE_MODE = 0x0001,
    DC_RESERVED14 = 0x0002,
    DC_RESERVED15 = 0x0004,
    DC_POSITIONING = 0x0008,
    DC_NETWORKING = 0x0010,
    DC_RENDERING = 0x0020,
    DC_CAPTURING = 0x0040,
    DC_OBJECT_TRANSFER = 0x0080,
    DC_AUDIO = 0x0100,
    DC_TELEPHONY = 0x0200,
    DC_INFORMATION = 0x0400,
};

static uint8_t device_class_get_major_service_(const bt_device_class_t *dc, int bitmask);
static void device_class_clr_major_service_(bt_device_class_t *dc, int bitmask);
static void device_class_set_major_service_(bt_device_class_t *dc, int bitmask);

void device_class_from_stream(bt_device_class_t *dc, const uint8_t *data)
{
    assert(dc != NULL);
    assert(data != NULL);
    *dc = *(bt_device_class_t *)data;
}

int device_class_to_stream(const bt_device_class_t *dc, uint8_t *data, size_t len)
{
    assert(dc != NULL);
    assert(data != NULL);
    assert(len >= sizeof(bt_device_class_t));

    for(size_t i = 0; i < sizeof(bt_device_class_t); ++i) {
        data[i] = dc->_[i];
    }

    return sizeof(bt_device_class_t);
}

void device_class_from_int(bt_device_class_t *dc, int data)
{
    assert(dc != NULL);
    assert(data != 0);
    // Careful with endianess.
    dc->_[0] = data & 0xff;
    dc->_[1] = (data >> 8) & 0xff;
    dc->_[2] = (data >> 16) & 0xff;
}

int device_class_to_int(const bt_device_class_t *dc)
{
    assert(dc != NULL);
    // Careful with endianess.
    return (int)(le32_to_cpu(*(int *)dc) & 0xffffff);
}

uint8_t device_class_equals(const bt_device_class_t *p1, const bt_device_class_t *p2)
{
    assert(p1 != NULL);
    assert(p2 != NULL);
    return (memcmp(p1, p2, sizeof(bt_device_class_t)) == 0);
}

uint8_t device_class_copy(bt_device_class_t *dest, const bt_device_class_t *src)
{
    assert(dest != NULL);
    assert(src != NULL);
    return (memcpy(dest, src, sizeof(bt_device_class_t)) == dest);
}

int device_class_get_major_device(const bt_device_class_t *dc)
{
    assert(dc != NULL);
    return DC(dc)->major_device;
}

void device_class_set_major_device(bt_device_class_t *dc, int val)
{
    assert(dc != NULL);
    DC(dc)->major_device = val;
}

int device_class_get_minor_device(const bt_device_class_t *dc)
{
    assert(dc != NULL);
    return DC(dc)->minor_device;
}

void device_class_set_minor_device(bt_device_class_t *dc, int val)
{
    assert(dc != NULL);
    DC(dc)->minor_device = val;
}

uint8_t device_class_get_information(const bt_device_class_t *dc)
{
    assert(dc != NULL);
    return device_class_get_major_service_(dc, DC_INFORMATION);
}

void device_class_set_information(bt_device_class_t *dc, uint8_t set)
{
    assert(dc != NULL);

    if(set) {
        device_class_set_major_service_(dc, DC_INFORMATION);
    } else {
        device_class_clr_major_service_(dc, DC_INFORMATION);
    }
}

uint8_t device_class_get_limited(const bt_device_class_t *dc)
{
    assert(dc != NULL);
    return device_class_get_major_service_(dc, DC_LIMITED_DISCOVERABLE_MODE);
}

void device_class_set_limited(bt_device_class_t *dc, uint8_t set)
{
    assert(dc != NULL);

    if(set) {
        device_class_set_major_service_(dc, DC_LIMITED_DISCOVERABLE_MODE);
    } else {
        device_class_clr_major_service_(dc, DC_LIMITED_DISCOVERABLE_MODE);
    }
}

static uint8_t device_class_get_major_service_(const bt_device_class_t *dc, int bitmask)
{
    return (DC(dc)->major_service & bitmask);
}

static void device_class_clr_major_service_(bt_device_class_t *dc, int bitmask)
{
    DC(dc)->major_service &= ~bitmask;
}

static void device_class_set_major_service_(bt_device_class_t *dc, int bitmask)
{
    DC(dc)->major_service |= bitmask;
}
