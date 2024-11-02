/****************************************************************************
 * components/platform/soc/bl602/bl602_os_adapter/bl602_os_adapter/bl602_os_hal.h
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

#ifndef __ARCH_RISCV_SRC_BL602_OS_HAL_H
#define __ARCH_RISCV_SRC_BL602_OS_HAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <bl_os_adapter/bl_os_type.h>
#include <bl_os_adapter/bl_os_adapter.h>

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef __ASSEMBLY__

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Definition
 ****************************************************************************/

#if defined(CFG_FREERTOS)
    #define OS_USING_FREERTOS
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void bl_os_printf(const char *__fmt, ...);

void bl_os_puts(const char *s);

void bl_os_assert_func(const char *file, int line, const char *func, const char *expr);

void *bl_os_malloc(unsigned int size);

void bl_os_free(void *ptr);

void *bl_os_zalloc(unsigned int size);

int bl_os_task_create(const char *name, void *entry, uint32_t stack_depth, void *param, uint32_t prio, BL_TaskHandle_t task_handle);

void bl_os_task_delete(BL_TaskHandle_t task_handle);

BL_TaskHandle_t bl_os_task_get_current_task(void);

BL_TaskHandle_t bl_os_task_notify_create(void);

void bl_os_task_notify(BL_TaskHandle_t task_handle);

void bl_os_task_wait(BL_TaskHandle_t task_handle, uint32_t tick);

BL_MessageQueue_t bl_os_mq_creat(uint32_t queue_len, uint32_t item_size);

void bl_os_mq_delete(BL_MessageQueue_t mq);

int bl_os_mq_send_wait(BL_MessageQueue_t queue, void *item, uint32_t len, uint32_t ticks, int prio);

int bl_os_mq_send(BL_MessageQueue_t queue, void *item, uint32_t len);

int bl_os_mq_recv(BL_MessageQueue_t queue, void *item, uint32_t len, uint32_t tick);

BL_Timer_t bl_os_timer_create(void *func, void *argv);

int bl_os_timer_delete(BL_Timer_t timerid, uint32_t tick);

int bl_os_timer_start_once(BL_Timer_t timerid, long t_sec, long t_nsec);

int bl_os_timer_start_periodic(BL_Timer_t timerid, long t_sec, long t_nsec);

void *bl_os_workqueue_create(void);

int bl_os_workqueue_submit_hpwork(void *work, void *worker, void *argv, long tick);

int bl_os_workqueue_submit_lpwork(void *work, void *worker, void *argv, long tick);

uint64_t bl_os_clock_gettime_ms(void);

void bl_os_irq_attach(int32_t n, void *f, void *arg);

void bl_os_irq_enable(int32_t n);

void bl_os_irq_disable(int32_t n);

BL_Mutex_t bl_os_mutex_create(void);

void bl_os_mutex_delete(BL_Mutex_t mutex_data);

int32_t bl_os_mutex_lock(BL_Mutex_t mutex_data);

int32_t bl_os_mutex_unlock(BL_Mutex_t mutex_data);

BL_Sem_t bl_os_sem_create(uint32_t init);

int32_t bl_os_sem_take(BL_Sem_t semphr, uint32_t ticks);

int32_t bl_os_sem_give(BL_Sem_t semphr);

void bl_os_sem_delete(BL_Sem_t semphr);

int bl_os_api_init(void);

BL_EventGroup_t bl_os_event_create(void);

void bl_os_event_delete(BL_EventGroup_t event);

uint32_t bl_os_event_send(BL_EventGroup_t event, uint32_t bits);

uint32_t bl_os_event_wait(BL_EventGroup_t event, uint32_t bits_to_wait_for, int clear_on_exit, int wait_for_all_bits, uint32_t block_time_tick);

int bl_os_event_register(int type, void *cb, void *arg);

int bl_os_event_notify(int evt, int val);

void bl_os_lock_gaint(void);

void bl_os_unlock_gaint(void);

int bl_os_msleep(long msec);

uint32_t bl_os_get_tick(void);

int bl_os_sleep(unsigned int seconds);

uint32_t bl_os_enter_critical(void);

void bl_os_exit_critical(uint32_t level);

void bl_os_log_write(uint32_t level, const char *tag, const char *file, int line, const char *format, ...);

int bl_os_task_notify_isr(BL_TaskHandle_t task_handle);

void bl_os_yield_from_isr(int xYield);

unsigned int bl_os_ms_to_tick(unsigned int ms);

BL_TimeOut_t bl_os_set_timeout(void);

int bl_os_check_timeout(BL_TimeOut_t xTimeOut, BL_TickType_t *xTicksToWait);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __ARCH_RISCV_SRC_BL602_OS_HAL_H*/
