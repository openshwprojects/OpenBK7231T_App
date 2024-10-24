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

#ifndef BTIF_SOCK_THREAD_H
#define BTIF_SOCK_THREAD_H

#include <stdbool.h>

#include <hardware/bluetooth.h>
#include <hardware/bt_sock.h>

/*******************************************************************************
**  Constants & Macros
********************************************************************************/

#define SOCK_THREAD_FD_RD           1        /* BT socket read signal */
#define SOCK_THREAD_FD_WR           (1 << 1) /* BT socket write signal */
#define SOCK_THREAD_FD_EXCEPTION    (1 << 2) /* BT socket exception singal */
#define SOCK_THREAD_ADD_FD_SYNC     (1 << 3) /* Add BT socket fd in current socket
                                                poll thread context immediately */

/*******************************************************************************
**  Functions
********************************************************************************/

typedef void (*btsock_signaled_cb)(int fd, int type, int flags, uint32_t user_id);
typedef void (*btsock_cmd_cb)(int cmd_fd, int type, int size, uint32_t user_id);

int btsock_thread_init();
int btsock_thread_add_fd(int handle, int fd, int type, int flags, uint32_t user_id);
uint8_t btsock_thread_remove_fd_and_close(int thread_handle, int fd);
int btsock_thread_wakeup(int handle);
int btsock_thread_post_cmd(int handle, int cmd_type, const unsigned char *cmd_data,
                           int data_size, uint32_t user_id);
int btsock_thread_create(btsock_signaled_cb callback, btsock_cmd_cb cmd_callback);
int btsock_thread_exit(int handle);

#endif
