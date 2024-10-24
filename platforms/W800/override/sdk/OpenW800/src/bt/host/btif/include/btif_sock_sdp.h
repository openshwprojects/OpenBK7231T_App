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

#ifndef BTIF_SOCK_SDP_H
#define BTIF_SOCK_SDP_H

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static const uint8_t  UUID_OBEX_OBJECT_PUSH[] = {0x00, 0x00, 0x11, 0x05, 0x00, 0x00, 0x10, 0x00,
                                                 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB
                                                };
static const uint8_t  UUID_PBAP_PSE[]         = {0x00, 0x00, 0x11, 0x2F, 0x00, 0x00, 0x10, 0x00,
                                                 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB
                                                };
static const uint8_t  UUID_MAP_MAS[]          = {0x00, 0x00, 0x11, 0x32, 0x00, 0x00, 0x10, 0x00,
                                                 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB
                                                };
static const uint8_t  UUID_SAP[]              = {0x00, 0x00, 0x11, 0x2D, 0x00, 0x00, 0x10, 0x00,
                                                 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB
                                                };
static const uint8_t  UUID_SPP[]              = {0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00,
                                                 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB
                                                };

static inline uint8_t is_uuid_empty(const uint8_t *uuid)
{
    static uint8_t empty_uuid[16];
    return uuid == NULL || memcmp(uuid, empty_uuid, sizeof(empty_uuid)) == 0;
}

int add_rfc_sdp_rec(const char *name, const uint8_t *uuid, int scn);
void del_rfc_sdp_rec(int handle);
uint8_t is_reserved_rfc_channel(int channel);
int get_reserved_rfc_channel(const uint8_t *uuid);

#endif
