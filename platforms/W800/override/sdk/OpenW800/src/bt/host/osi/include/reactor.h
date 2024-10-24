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
#include <stdint.h>

#include "osi/include/osi.h"

// This module implements the Reactor pattern.
// See http://en.wikipedia.org/wiki/Reactor_pattern for details.

typedef struct reactor_t reactor_t;
typedef struct reactor_object_t reactor_object_t;

// Enumerates the reasons a reactor has stopped.
typedef enum {
    REACTOR_STATUS_STOP,     // |reactor_stop| was called.
    REACTOR_STATUS_ERROR,    // there was an error during the operation.
    REACTOR_STATUS_DONE,     // the reactor completed its work (for the _run_once* variants).
} reactor_status_t;

// Creates a new reactor object. Returns NULL on failure. The returned object
// must be freed by calling |reactor_free|.
reactor_t *reactor_new(void);

// Frees a reactor object created with |reactor_new|. |reactor| may be NULL.
void reactor_free(reactor_t *reactor);

// Starts the reactor. This function blocks the caller until |reactor_stop| is called
// from another thread or in a callback. |reactor| may not be NULL.
reactor_status_t reactor_start(reactor_t *reactor);

// Runs one iteration of the reactor. This function blocks until at least one registered object
// becomes ready. |reactor| may not be NULL.
reactor_status_t reactor_run_once(reactor_t *reactor);

// Immediately unblocks the reactor. This function is safe to call from any thread.
// |reactor| may not be NULL.
void reactor_stop(reactor_t *reactor);

// Registers a file descriptor with the reactor. The file descriptor, |fd|, must be valid
// when this function is called and its ownership is not transferred to the reactor. The
// |context| variable is a user-defined opaque handle that is passed back to the |read_ready|
// and |write_ready| functions. It is not copied or even dereferenced by the reactor so it
// may contain any value including NULL. The |read_ready| and |write_ready| arguments are
// optional and may be NULL. This function returns an opaque object that represents the
// file descriptor's registration with the reactor. When the caller is no longer interested
// in events on the |fd|, it must free the returned object by calling |reactor_unregister|.
reactor_object_t *reactor_register(reactor_t *reactor,
                                   int fd, void *context,
                                   void (*read_ready)(void *context),
                                   void (*write_ready)(void *context));

// Changes the subscription mode for the file descriptor represented by |object|. If the
// caller has already registered a file descriptor with a reactor, has a valid |object|,
// and decides to change the |read_ready| and/or |write_ready| callback routines, they
// can call this routine. Returns true if the subscription was changed, false otherwise.
// |object| may not be NULL, |read_ready| and |write_ready| may be NULL.
uint8_t reactor_change_registration(reactor_object_t *object,
                                    void (*read_ready)(void *context),
                                    void (*write_ready)(void *context));

// Unregisters a previously registered file descriptor with its reactor. |obj| may not be NULL.
// |obj| is invalid after calling this function so the caller must drop all references to it.
void reactor_unregister(reactor_object_t *obj);
