/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "nimble/nimble_npl.h"
#include "wm_osal.h"

struct ble_npl_event *
npl_freertos_eventq_get(struct ble_npl_eventq *evq, ble_npl_time_t tmo)
{
    struct ble_npl_event *ev = NULL;
    tls_os_status_t status;
    status = tls_os_queue_receive(evq->q, (void **)&ev, sizeof(ev), tmo);

    if(status == TLS_OS_SUCCESS) {
    if(ev) {
        ev->queued = false;
    }
    }

    return ev;
}

void
npl_freertos_eventq_put(struct ble_npl_eventq *evq, struct ble_npl_event *ev)
{
    tls_os_status_t status;

    if(ev->queued) {
        return;
    }

    ev->queued = true;
    status = tls_os_queue_send(evq->q, ev, sizeof(ev));
    assert(status == TLS_OS_SUCCESS);
}

void
npl_freertos_eventq_put_to_front(struct ble_npl_eventq *evq, struct ble_npl_event *ev)
{
    tls_os_status_t status;

    if(ev->queued) {
        return;
    }

    ev->queued = true;
    status = tls_os_queue_send_to_front(evq->q, ev, sizeof(ev));
    assert(status == TLS_OS_SUCCESS);
}

void
npl_freertos_eventq_put_to_back(struct ble_npl_eventq *evq, struct ble_npl_event *ev)
{
    tls_os_status_t status;

    if(ev->queued) {
        return;
    }

    ev->queued = true;
    status = tls_os_queue_send_to_back(evq->q, ev, sizeof(ev));
    assert(status == TLS_OS_SUCCESS);
}


void
npl_freertos_eventq_remove(struct ble_npl_eventq *evq,
                           struct ble_npl_event *ev)
{
    tls_os_status_t status;

    if(!ev->queued) {
        return;
    }

    status = tls_os_queue_remove(evq->q, ev, sizeof(ev));
    assert(status == TLS_OS_SUCCESS);
    ev->queued = 0;
}

ble_npl_error_t
npl_freertos_mutex_init(struct ble_npl_mutex *mu)
{
    tls_os_status_t status;

    if(!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    status = tls_os_mutex_create(0, &mu->handle);
    assert(status == TLS_OS_SUCCESS);
    assert(mu->handle);
    return BLE_NPL_OK;
}
ble_npl_error_t
npl_freertos_mutex_deinit(struct ble_npl_mutex *mu)
{
    tls_os_status_t status;

    if(!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    if(mu->handle) {
        status = tls_os_mutex_delete(mu->handle);
        assert(status == TLS_OS_SUCCESS);
    }

    return BLE_NPL_OK;
}

ble_npl_error_t
npl_freertos_mutex_pend(struct ble_npl_mutex *mu, ble_npl_time_t timeout)
{
    tls_os_status_t status;

    if(!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    assert(mu->handle);

    if(tls_get_isr_count() > 0) {
        status = TLS_OS_ERROR;
        assert(0);
    } else {
        status = tls_os_mutex_acquire(mu->handle, timeout);
    }

    return status == TLS_OS_SUCCESS ? BLE_NPL_OK : BLE_NPL_TIMEOUT;
}

ble_npl_error_t
npl_freertos_mutex_release(struct ble_npl_mutex *mu)
{
    tls_os_status_t status;

    if(!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    assert(mu->handle);

    if(tls_get_isr_count() > 0) {
        assert(0);
    } else {
        status = tls_os_mutex_release(mu->handle);

        if(status != TLS_OS_SUCCESS) {
            return BLE_NPL_BAD_MUTEX;
        }
    }

    return BLE_NPL_OK;
}


ble_npl_error_t
npl_freertos_sem_init(struct ble_npl_sem *sem, uint16_t tokens)
{
    tls_os_status_t status;

    if(!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    status = tls_os_sem_create(&sem->handle, tokens);
    assert(status == TLS_OS_SUCCESS);
    assert(sem->handle);
    return BLE_NPL_OK;
}
ble_npl_error_t
npl_freertos_sem_deinit(struct ble_npl_sem *sem)
{
    tls_os_status_t status;

    if(!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    if(sem->handle) {
        status = tls_os_sem_delete(sem->handle);
        assert(status == TLS_OS_SUCCESS);
    }

    return BLE_NPL_OK;
}

ble_npl_error_t
npl_freertos_sem_pend(struct ble_npl_sem *sem, ble_npl_time_t timeout)
{
    tls_os_status_t status;

    if(!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    assert(sem->handle);
    status = tls_os_sem_acquire(sem->handle, timeout);
    return status == TLS_OS_SUCCESS ? BLE_NPL_OK : BLE_NPL_TIMEOUT;
}
uint16_t npl_freertos_get_sem_count(struct ble_npl_sem *sem)
{
    if(!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    assert(sem->handle);
    return (uint16_t)tls_os_sem_get_count(sem->handle);
}

ble_npl_error_t
npl_freertos_sem_release(struct ble_npl_sem *sem)
{
    tls_os_status_t status;

    if(!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    assert(sem->handle);
    status = tls_os_sem_release(sem->handle);
    assert(status == TLS_OS_SUCCESS);
    return BLE_NPL_OK;
}
static void
os_callout_timer_cb(void *ptmr, void *parg)
{
    struct ble_npl_callout *co;
    co = (struct ble_npl_callout *)parg;

    if(co->evq) {
        ble_npl_eventq_put(co->evq, &co->ev);
    } else {
        co->ev.fn(&co->ev);
    }
}

void
npl_freertos_callout_init(struct ble_npl_callout *co, struct ble_npl_eventq *evq,
                          ble_npl_event_fn *ev_cb, void *ev_arg)
{
    tls_os_status_t status;
    memset(co, 0, sizeof(*co));
    status = tls_os_timer_create(&co->handle, os_callout_timer_cb, (void *)co, 1, 0, (u8 *)"co");
    assert(status == TLS_OS_SUCCESS);
    co->evq = evq;
    ble_npl_event_init(&co->ev, ev_cb, ev_arg);
}

ble_npl_error_t
npl_freertos_callout_reset(struct ble_npl_callout *co, ble_npl_time_t ticks)
{
    tls_os_timer_change(co->handle, ticks);
    return BLE_NPL_OK;
}

ble_npl_time_t
npl_freertos_callout_remaining_ticks(struct ble_npl_callout *co,
                                     ble_npl_time_t now)
{
    ble_npl_time_t rt;
    uint32_t exp;
    exp = tls_os_timer_expirytime(co->handle);

    if(exp > now) {
        rt = exp - now;
    } else {
        rt = 0;
    }

    return rt;
}

ble_npl_error_t
npl_freertos_time_ms_to_ticks(uint32_t ms, ble_npl_time_t *out_ticks)
{
    uint64_t ticks;
    ticks = ((uint64_t)ms * /*configTICK_RATE_HZ*/HZ) / 1000;

    if(ticks > UINT32_MAX) {
        return BLE_NPL_EINVAL;
    }

    *out_ticks = ticks;
    return 0;
}

ble_npl_error_t
npl_freertos_time_ticks_to_ms(ble_npl_time_t ticks, uint32_t *out_ms)
{
    uint64_t ms;
    ms = ((uint64_t)ticks * 1000) / /*configTICK_RATE_HZ*/HZ;

    if(ms > UINT32_MAX) {
        return BLE_NPL_EINVAL;
    }

    *out_ms = ms;
    return 0;
}
