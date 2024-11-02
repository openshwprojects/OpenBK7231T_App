/****************************************************************************
 * components/platform/soc/bl602/bl602_os_adapter/bl602_os_adapter/include/bl_os_adapter/bl_os_adapter.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef _BL_OS_ADAPTER_H_
#define _BL_OS_ADAPTER_H_

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <bl_os_adapter/bl_os_type.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Definition
 ****************************************************************************/
#define BL_OS_TRUE            (1)
#define BL_OS_FALSE           (0)

#define BL_OS_WAITING_FOREVER (0xffffffffUL)
#define BL_OS_NO_WAITING      (0x0UL)

#define BL_OS_ADAPTER_VERSION ((int)0x00000001)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct bl_ops_funcs
{
    int _version;
    void (*_printf)(const char *fmt, ...);
    void (*_puts)(const char *c);
    void (*_assert)(const char *file, int line, const char *func, const char *expr);
    int (*_init)(void);
    uint32_t (*_enter_critical)(void);
    void (*_exit_critical)(uint32_t level);
    int (*_msleep)(long ms);
    int (*_sleep)(unsigned int seconds);
    BL_EventGroup_t (*_event_group_create)(void);
    void (*_event_group_delete)(BL_EventGroup_t event);
    uint32_t (*_event_group_send)(BL_EventGroup_t event, uint32_t bits);
    uint32_t (*_event_group_wait)(BL_EventGroup_t event,
                                  uint32_t bits_to_wait_for,
                                  int clear_on_exit,
                                  int wait_for_all_bits,
                                  uint32_t block_time_tick);
    int (*_event_register)(int type, void *cb, void *arg);
    int (*_event_notify)(int evt, int val);
    int (*_task_create)(const char *name,
                        void *entry,
                        uint32_t stack_depth,
                        void *param,
                        uint32_t prio,
                        BL_TaskHandle_t task_handle);
    void (*_task_delete)(BL_TaskHandle_t task_handle);
    BL_TaskHandle_t (*_task_get_current_task)(void);
    BL_TaskHandle_t (*_task_notify_create)(void);
    void (*_task_notify)(BL_TaskHandle_t task_handle);
    void (*_task_wait)(BL_TaskHandle_t task_handle, uint32_t tick);
    void (*_lock_gaint)(void);
    void (*_unlock_gaint)(void);
    void (*_irq_attach)(int32_t n, void *f, void *arg);
    void (*_irq_enable)(int32_t n);
    void (*_irq_disable)(int32_t n);
    void *(*_workqueue_create)(void);
    int (*_workqueue_submit_hp)(void *work, void *woker, void *argv, long tick);
    int (*_workqueue_submit_lp)(void *work, void *woker, void *argv, long tick);
    BL_Timer_t (*_timer_create)(void *func, void *argv);
    int (*_timer_delete)(BL_Timer_t timerid, uint32_t tick);
    int (*_timer_start_once)(BL_Timer_t timerid, long t_sec, long t_nsec);
    int (*_timer_start_periodic)(BL_Timer_t timerid, long t_sec, long t_nsec);
    BL_Sem_t (*_sem_create)(uint32_t init);
    void (*_sem_delete)(BL_Sem_t semphr);
    int32_t (*_sem_take)(BL_Sem_t semphr, uint32_t tick);
    int32_t (*_sem_give)(BL_Sem_t semphr);
    BL_Mutex_t (*_mutex_create)(void);
    void (*_mutex_delete)(BL_Mutex_t mutex);
    int32_t (*_mutex_lock)(BL_Mutex_t mutex);
    int32_t (*_mutex_unlock)(BL_Mutex_t mutex);
    BL_MessageQueue_t (*_queue_create)(uint32_t queue_len, uint32_t item_size);
    void (*_queue_delete)(BL_MessageQueue_t queue);
    int (*_queue_send_wait)(BL_MessageQueue_t queue, void *item, uint32_t len, uint32_t ticks, int prio);
    int (*_queue_send)(BL_MessageQueue_t queue, void *item, uint32_t len);
    int (*_queue_recv)(BL_MessageQueue_t queue, void *item, uint32_t len, uint32_t tick);
    void *(*_malloc)(unsigned int size);
    void (*_free)(void *p);
    void *(*_zalloc)(unsigned int size);
    uint64_t (*_get_time_ms)(void);
    uint32_t (*_get_tick)(void);
    void (*_log_write)(uint32_t level, const char *tag, const char *file, int line, const char *format, ...);
    int (*_task_notify_isr)(BL_TaskHandle_t task_handle);
    void (*_yield_from_isr)(int xYield);
    unsigned int (*_ms_to_tick)(unsigned int ms);
    BL_TimeOut_t (*_set_timeout)(void);
    int (*_check_timeout)(BL_TimeOut_t xTimeOut, BL_TickType_t *xTicksToWait);
};

typedef struct bl_ops_funcs bl_ops_funcs_t;

extern bl_ops_funcs_t g_bl_ops_funcs;

#ifdef __cplusplus
}
#endif

#endif /* _BL_OS_ADAPTER_H_ */
