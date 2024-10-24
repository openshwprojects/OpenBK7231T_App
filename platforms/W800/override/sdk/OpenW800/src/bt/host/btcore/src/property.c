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
#include "btcore/include/bdaddr.h"
#include "btcore/include/device_class.h"
#include "btcore/include/property.h"
#include "btcore/include/uuid.h"
#include "osi/include/allocator.h"

static tls_bt_property_t *property_new_(void *val, size_t len, tls_bt_property_type_t type);

tls_bt_property_t *property_copy_array(const tls_bt_property_t *properties, size_t count)
{
    assert(properties != NULL);
    tls_bt_property_t *clone = GKI_getbuf(sizeof(tls_bt_property_t) * count);
    memcpy(&clone[0], &properties[0], sizeof(tls_bt_property_t) * count);

    for(size_t i = 0; i < count; ++i) {
        clone[i].val = GKI_getbuf(clone[i].len);
        memcpy(clone[i].val, properties[i].val, clone[i].len);
    }

    return clone;
}

tls_bt_property_t *property_copy(tls_bt_property_t *dest, const tls_bt_property_t *src)
{
    assert(dest != NULL);
    assert(src != NULL);
    return (tls_bt_property_t *)memcpy(dest, src, sizeof(tls_bt_property_t));
}

uint8_t property_equals(const tls_bt_property_t *p1, const tls_bt_property_t *p2)
{
    // Two null properties are not the same. May need to revisit that
    // decision when we have a test case that exercises that condition.
    if(!p1 || !p2 || p1->type != p2->type) {
        return false;
    }

    // Although the Bluetooth name is a 249-byte array, the implementation
    // treats it like a variable-length array with its size specified in the
    // property's `len` field. We special-case the equivalence of BDNAME
    // types here by truncating the larger, zero-padded name to its string
    // length and comparing against the shorter name.
    //
    // Note: it may be the case that both strings are zero-padded but that
    // hasn't come up yet so this implementation doesn't handle it.
    if(p1->type == WM_BT_PROPERTY_BDNAME && p1->len != p2->len) {
        const tls_bt_property_t *shorter = p1, *longer = p2;

        if(p1->len > p2->len) {
            shorter = p2;
            longer = p1;
        }

        return strlen((const char *)longer->val) == (size_t)shorter->len
               && !memcmp(longer->val, shorter->val, shorter->len);
    }

    return p1->len == p2->len && !memcmp(p1->val, p2->val, p1->len);
}

tls_bt_property_t *property_new_addr(const tls_bt_addr_t *addr)
{
    assert(addr != NULL);
    return property_new_((void *)addr, sizeof(tls_bt_addr_t), WM_BT_PROPERTY_BDADDR);
}

tls_bt_property_t *property_new_device_class(const bt_device_class_t *dc)
{
    assert(dc != NULL);
    return property_new_((void *)dc, sizeof(bt_device_class_t), WM_BT_PROPERTY_CLASS_OF_DEVICE);
}

tls_bt_property_t *property_new_device_type(bt_device_type_t type)
{
    return property_new_((void *)&type, sizeof(bt_device_type_t), WM_BT_PROPERTY_TYPE_OF_DEVICE);
}

tls_bt_property_t *property_new_discovery_timeout(const uint32_t timeout)
{
    return property_new_((void *)&timeout, sizeof(uint32_t), WM_BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT);
}

tls_bt_property_t *property_new_name(const char *name)
{
    assert(name != NULL);
    return property_new_((void *)name, sizeof(tls_bt_bdname_t), WM_BT_PROPERTY_BDNAME);
}

tls_bt_property_t *property_new_rssi(int8_t rssi)
{
    return property_new_((void *)&rssi, sizeof(int8_t), WM_BT_PROPERTY_REMOTE_RSSI);
}

tls_bt_property_t *property_new_scan_mode(bt_scan_mode_t scan_mode)
{
    return property_new_((void *)&scan_mode, sizeof(bt_scan_mode_t), WM_BT_PROPERTY_ADAPTER_SCAN_MODE);
}

tls_bt_property_t *property_new_uuids(const tls_bt_uuid_t *uuid, size_t count)
{
    assert(uuid != NULL);
    return property_new_((void *)uuid, sizeof(tls_bt_uuid_t) * count, WM_BT_PROPERTY_UUIDS);
}

void property_free(tls_bt_property_t *property)
{
    property_free_array(property, 1);
}

void property_free_array(tls_bt_property_t *properties, size_t count)
{
    if(properties == NULL) {
        return;
    }

    for(size_t i = 0; i < count; ++i) {
        GKI_freebuf(properties[i].val);
    }

    GKI_freebuf(properties);
}

uint8_t property_is_addr(const tls_bt_property_t *property)
{
    assert(property != NULL);
    return property->type == WM_BT_PROPERTY_BDADDR;
}

uint8_t property_is_device_class(const tls_bt_property_t *property)
{
    assert(property != NULL);
    return property->type == WM_BT_PROPERTY_CLASS_OF_DEVICE;
}

uint8_t property_is_device_type(const tls_bt_property_t *property)
{
    assert(property != NULL);
    return property->type == WM_BT_PROPERTY_TYPE_OF_DEVICE;
}

uint8_t property_is_discovery_timeout(const tls_bt_property_t *property)
{
    assert(property != NULL);
    return property->type == WM_BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT;
}

uint8_t property_is_name(const tls_bt_property_t *property)
{
    assert(property != NULL);
    return property->type == WM_BT_PROPERTY_BDNAME;
}

uint8_t property_is_rssi(const tls_bt_property_t *property)
{
    assert(property != NULL);
    return property->type == WM_BT_PROPERTY_REMOTE_RSSI;
}

uint8_t property_is_scan_mode(const tls_bt_property_t *property)
{
    assert(property != NULL);
    return property->type == WM_BT_PROPERTY_ADAPTER_SCAN_MODE;
}

uint8_t property_is_uuids(const tls_bt_property_t *property)
{
    assert(property != NULL);
    return property->type == WM_BT_PROPERTY_UUIDS;
}

// Convenience conversion methods to property values
const tls_bt_addr_t *property_as_addr(const tls_bt_property_t *property)
{
    assert(property_is_addr(property));
    return (const tls_bt_addr_t *)property->val;
}

const bt_device_class_t *property_as_device_class(const tls_bt_property_t *property)
{
    assert(property_is_device_class(property));
    return (const bt_device_class_t *)property->val;
}

bt_device_type_t property_as_device_type(const tls_bt_property_t *property)
{
    assert(property_is_device_type(property));
    return *(const bt_device_type_t *)property->val;
}

uint32_t property_as_discovery_timeout(const tls_bt_property_t *property)
{
    assert(property_is_discovery_timeout(property));
    return *(const uint32_t *)property->val;
}

const tls_bt_bdname_t *property_as_name(const tls_bt_property_t *property)
{
    assert(property_is_name(property));
    return (const tls_bt_bdname_t *)property->val;
}

int8_t property_as_rssi(const tls_bt_property_t *property)
{
    assert(property_is_rssi(property));
    return *(const int8_t *)property->val;
}

bt_scan_mode_t property_as_scan_mode(const tls_bt_property_t *property)
{
    assert(property_is_scan_mode(property));
    return *(const bt_scan_mode_t *)property->val;
}

const tls_bt_uuid_t *property_as_uuids(const tls_bt_property_t *property, size_t *count)
{
    assert(property_is_uuids(property));
    *count = sizeof(tls_bt_uuid_t) / property->len;
    return (const tls_bt_uuid_t *)property->val;
}

static tls_bt_property_t *property_new_(void *val, size_t len, tls_bt_property_type_t type)
{
    tls_bt_property_t *property = GKI_getbuf(sizeof(tls_bt_property_t));
    property->val = GKI_getbuf(len);
    memcpy(property->val, val, len);
    property->type = type;
    property->len = len;
    return property;
}
