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

/*******************************************************************************
 *
 *  Filename:      btif_sock.h
 *
 *  Description:   Bluetooth socket Interface
 *
 *******************************************************************************/

#ifndef BTIF_SOCK_RFC_H
#define BTIF_SOCK_RFC_H

#include "btif_uid.h"

tls_bt_status_t btsock_rfc_init(int handle, uid_set_t *set);
tls_bt_status_t btsock_rfc_cleanup();
tls_bt_status_t btsock_rfc_listen(const char *name, const uint8_t *uuid, int channel,
                                  int *sock_fd, int flags, int app_uid);
tls_bt_status_t btsock_rfc_connect(const tls_bt_addr_t *bd_addr, const uint8_t *uuid,
                                   int channel, int *sock_fd, int flags, int app_uid);
void btsock_rfc_signaled(int fd, int flags, uint32_t user_id);

#endif
