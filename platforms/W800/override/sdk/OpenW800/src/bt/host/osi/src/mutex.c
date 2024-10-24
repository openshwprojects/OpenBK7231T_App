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

#define LOG_TAG "bt_osi_mutex"

//#include <pthread.h>

#include "osi/include/mutex.h"

//static pthread_mutex_t global_lock;

void mutex_init(void)
{
#if 0
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&global_lock, &attr);
#endif
}

void mutex_cleanup(void)
{
#if 0
    pthread_mutex_destroy(&global_lock);
#endif
}

void mutex_global_lock(void)
{
#if 0
    pthread_mutex_lock(&global_lock);
#endif
}

void mutex_global_unlock(void)
{
#if 0
    pthread_mutex_unlock(&global_lock);
#endif
}
