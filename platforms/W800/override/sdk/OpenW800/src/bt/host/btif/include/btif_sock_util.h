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
 *  Filename:      btif_sock_util.h
 *
 *  Description:   Bluetooth socket Interface Helper functions
 *
 *******************************************************************************/

#ifndef BTIF_SOCK_UTIL_H
#define BTIF_SOCK_UTIL_H

#include <stdint.h>

void dump_bin(const char *title, const char *data, int size);

int sock_send_fd(int sock_fd, const uint8_t *buffer, int len, int send_fd);
int sock_send_all(int sock_fd, const uint8_t *buf, int len);
int sock_recv_all(int sock_fd, uint8_t *buf, int len);

#endif
