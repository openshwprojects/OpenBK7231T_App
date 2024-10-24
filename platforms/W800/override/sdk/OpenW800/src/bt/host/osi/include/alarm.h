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

typedef struct alarm_t alarm_t;
typedef struct fixed_queue_t fixed_queue_t;
typedef struct thread_t thread_t;
typedef uint64_t period_ms_t;

// Prototype for the alarm callback function.
typedef void (*alarm_callback_t)(void *data);

// Creates a new one-time off alarm object with user-assigned
// |name|. |name| may not be NULL, and a copy of the string will
// be stored internally. The value of |name| has no semantic
// meaning. It is recommended that the name is unique (for
// better debuggability), but that is not enforced. The returned
// object must be freed by calling |alarm_free|. Returns NULL on
// failure.
alarm_t *alarm_new(const char *name);

// Creates a new periodic alarm object with user-assigned |name|.
// |name| may not be NULL, and a copy of the string will be
// stored internally. The value of |name| has no semantic
// meaning. It is recommended that the name is unique (for better
// debuggability), but that is not enforced. The returned object
// must be freed by calling |alarm_free|. Returns NULL on
// failure.
alarm_t *alarm_new_periodic(const char *name);

// Frees an |alarm| object created by |alarm_new| or
// |alarm_new_periodic|. |alarm| may be NULL. If the alarm is
// pending, it will be cancelled first. It is not safe to call
// |alarm_free| from inside the callback of |alarm|.
void alarm_free(alarm_t *alarm);

// Sets an |alarm| to execute a callback in the future. The |cb|
// callback is called after the given |interval_ms|, where
// |interval_ms| is the number of milliseconds relative to the
// current time. If |alarm| was created with
// |alarm_new_periodic|, the alarm is scheduled to fire
// periodically every |interval_ms|, otherwise it is a one time
// only alarm. A periodic alarm repeats every |interval_ms| until
// it is cancelled or freed. When the alarm fires, the |cb|
// callback is called with the context argument |data|:
//
//      void cb(void *data) {...}
//
// The |data| argument may be NULL, but the |cb| callback may not
// be NULL. All |cb| callbacks scheduled through this call are
// called within a single (internally created) thread. That
// thread is not same as the caller’s thread. If two (or more)
// alarms are set back-to-back with the same |interval_ms|, the
// callbacks will be called in the order the alarms are set.
void alarm_set(alarm_t *alarm, uint64_t interval_ms,
               alarm_callback_t cb, void *data);

// Sets an |alarm| to execute a callback in the future on a
// specific |queue|. This function is same as |alarm_set| except
// that the |cb| callback is scheduled for execution in the
// context of the thread responsible for processing |queue|.
// Also, the callback execution ordering guarantee exists only
// among alarms that are scheduled on the same queue. |queue|
// may not be NULL.
void alarm_set_on_queue(alarm_t *alarm, uint64_t interval_ms,
                        alarm_callback_t cb, void *data,
                        fixed_queue_t *queue);

// This function cancels the |alarm| if it was previously set.
// When this call returns, the caller has a guarantee that the
// callback is not in progress and will not be called if it
// hasn't already been called. This function is idempotent.
// |alarm| may not be NULL.
void alarm_cancel(alarm_t *alarm);

// Tests whether the |alarm| is scheduled.
// Return true if the |alarm| is scheduled or NULL, otherwise false.
uint8_t alarm_is_scheduled(const alarm_t *alarm);

// Registers |queue| for processing alarm callbacks on |thread|.
// |queue| may not be NULL. |thread| may not be NULL.
void alarm_register_processing_queue(fixed_queue_t *queue, thread_t *thread);

// Unregisters |queue| for processing alarm callbacks on whichever thread
// it is registered with. All alarms currently set for execution on |queue|
// will be canceled. |queue| may not be NULL. This function is idempotent.
void alarm_unregister_processing_queue(fixed_queue_t *queue);

// Figure out how much time until next expiration.
// Returns 0 if not armed. |alarm| may not be NULL.
// TODO: Remove this function once PM timers can be re-factored
uint64_t alarm_get_remaining_ms(const alarm_t *alarm);

// Cleanup the alarm internal state.
// This function should be called by the OSI module cleanup during
// graceful shutdown.
void alarm_cleanup(void);

// Dump alarm-related statistics and debug info to the |fd| file descriptor.
// The information is in user-readable text format. The |fd| must be valid.
void alarm_debug_dump(int fd);
