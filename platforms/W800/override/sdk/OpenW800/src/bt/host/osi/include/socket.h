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
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct reactor_t reactor_t;
typedef struct socket_t socket_t;
typedef uint16_t port_t;

typedef void (*socket_cb)(socket_t *socket, void *context);

// Returns a new socket object. The socket is in an idle, disconnected state when
// it is returned by this function. The returned object must be freed by calling
// |socket_free|. Returns NULL on failure.
socket_t *socket_new(void);

// Returns a new socket object backed by |fd|. The socket object is in whichever
// state |fd| is in when it was passed to this function. The returned object must
// be freed by calling |socket_free|. Returns NULL on failure. If this function
// is successful, ownership of |fd| is transferred and the caller must not close it.
socket_t *socket_new_from_fd(int fd);

// Frees a socket object created by |socket_new| or |socket_accept|. |socket| may
// be NULL. If the socket was connected, it will be disconnected.
void socket_free(socket_t *socket);

// Puts |socket| in listening mode for incoming TCP connections on the specified
// |port| and the loopback IPv4 address. Returns true on success, false on
// failure (e.g. |port| is bound by another socket). |socket| may not be NULL.
uint8_t socket_listen(const socket_t *socket, port_t port);

// Blocks on a listening socket, |socket|, until a client connects to it. Returns
// a connected socket on success, NULL on failure. The returned object must be
// freed by calling |socket_free|. |socket| may not be NULL.
socket_t *socket_accept(const socket_t *socket);

// Reads up to |count| bytes from |socket| into |buf|. This function will not
// block. This function returns a positive integer representing the number
// of bytes copied into |buf| on success, 0 if the socket has disconnected,
// and -1 on error. This function may return a value less than |count| if not
// enough data is currently available. If this function returns -1, errno will
// also be set (see recv(2) for possible errno values). However, if the reading
// system call was interrupted with errno of EINTR, the read operation is
// restarted internally without propagating EINTR back to the caller. If there
// were no bytes available to be read, this function returns -1 and sets errno
// to EWOULDBLOCK. Neither |socket| nor |buf| may be NULL.
ssize_t socket_read(const socket_t *socket, void *buf, size_t count);

// Writes up to |count| bytes from |buf| into |socket|. This function will not
// block. Returns a positive integer representing the number of bytes written
// to |socket| on success, 0 if the socket has disconnected, and -1 on error.
// This function may return a value less than |count| if writing more bytes
// would result in blocking. If this function returns -1, errno will also be
// set (see send(2) for possible errno values). However, if the writing system
// call was interrupted with errno of EINTR, the write operation is restarted
// internally without propagating EINTR back to the caller. If no bytes could
// be written without blocking, this function will return -1 and set errno to
// EWOULDBLOCK. Neither |socket| nor |buf| may be NULL.
ssize_t socket_write(const socket_t *socket, const void *buf, size_t count);

// This function performs the same write operation as |socket_write| and also
// sends the file descriptor |fd| over the socket to a remote process. Ownership
// of |fd| transfers to this function and the descriptor must not be used any longer.
// If |fd| is INVALID_FD, this function behaves the same as |socket_write|.
ssize_t socket_write_and_transfer_fd(const socket_t *socket, const void *buf, size_t count, int fd);

// Returns the number of bytes that can be read from |socket| without blocking. On error,
// this function returns -1. |socket| may not be NULL.
//
// Note: this function should not be part of the socket interface. It is only provided as
//       a stop-gap until we can refactor away code that depends on a priori knowledge of
//       the byte count. Do not use this function unless you need it while refactoring
//       legacy bluedroid code.
ssize_t socket_bytes_available(const socket_t *socket);

// Registers |socket| with the |reactor|. When the socket becomes readable, |read_cb|
// will be called. When the socket becomes writeable, |write_cb| will be called. The
// |context| parameter is passed, untouched, to each of the callback routines. Neither
// |socket| nor |reactor| may be NULL. |read_cb|, |write_cb|, and |context| may be NULL.
void socket_register(socket_t *socket, reactor_t *reactor, void *context, socket_cb read_cb,
                     socket_cb write_cb);

// Unregisters |socket| from whichever reactor it is registered with, if any. This
// function is idempotent.
void socket_unregister(socket_t *socket);
