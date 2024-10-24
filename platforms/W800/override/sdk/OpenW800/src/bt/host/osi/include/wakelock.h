/******************************************************************************
 *
 *  Copyright (C) 2015 Google, Inc.
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
#include <hardware/bluetooth.h>

// Set the Bluetooth OS callouts to |callouts|.
// This function should be called when native kernel wakelocks are not used
// directly. If this function is not called, or |callouts| is NULL, then native
// kernel wakelocks will be used.
void wakelock_set_os_callouts(bt_os_callouts_t *callouts);

// Acquire the Bluetooth wakelock.
// The function is thread safe.
// Return true on success, otherwise false.
uint8_t wakelock_acquire(void);

// Release the Bluetooth wakelock.
// The function is thread safe.
// Return true on success, otherwise false.
uint8_t wakelock_release(void);

// Cleanup the wakelock internal state.
// This function should be called by the OSI module cleanup during
// graceful shutdown.
void wakelock_cleanup(void);

// This function should not need to be called normally.
// /sys/power/wake_{|un}lock are used by default.
// This is not guaranteed to have any effect after an alarm has been
// set with alarm_set.
// If |lock_path| or |unlock_path| are NULL, that path is not changed.
void wakelock_set_paths(const char *lock_path, const char *unlock_path);

// Dump wakelock-related debug info to the |fd| file descriptor.
// The caller is responsible for closing the |fd|.
void wakelock_debug_dump(int fd);
