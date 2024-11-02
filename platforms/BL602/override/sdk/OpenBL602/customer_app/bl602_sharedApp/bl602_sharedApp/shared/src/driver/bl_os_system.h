/****************************************************************************
 * components/platform/soc/bl602/bl602_os_adapter/bl602_os_adapter/include/bl_os_adapter/bl_os_system.h
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

#ifndef _BL_OS_SYSTEM_H_
#define _BL_OS_SYSTEM_H_

#include <bl_os_adapter/bl_os_adapter.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Definition
 ****************************************************************************/

/****************************************************************************
 * Name: assert
 *
 * Description:
 *   Validation program developer's expected result
 *
 * Input Parameters:
 *   file  - configASSERT file
 *   line  - configASSERT line
 *   func  - configASSERT function
 *   expr  - configASSERT condition
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#undef assert
#define assert(f)                                                         \
    do {                                                                  \
        if (!(f)) {                                                       \
            g_bl_ops_funcs._assert(__FILE__, __LINE__, __FUNCTION__, #f); \
        }                                                                 \
    } while (0)

/****************************************************************************
 * Name: bl_os_enter_critical
 *
 * Description:
 *   Enter critical state
 *   Must be used in pair with bl_os_exit_critical
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   CPU PS value
 *
 ****************************************************************************/

#define bl_os_enter_critical()                          \
  {                                                     \
    uint32_t _bl_os_flag;                               \
    _bl_os_flag = g_bl_ops_funcs._enter_critical();     \

/****************************************************************************
 * Name: bl_os_exit_critical
 *
 * Description:
 *   Exit from critical state
 *   Must be used in pair with bl_os_enter_critical
 *
 * Input Parameters:
 *   level - CPU PS value
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#define bl_os_exit_critical()                     \
    g_bl_ops_funcs._exit_critical(_bl_os_flag);   \
  }                                               \

/****************************************************************************
 * Name: bl_os_printf
 *
 * Description:
 *   Output format string and its arguments
 *
 * Input Parameters:
 *   format - format string
 *
 * Returned Value:
 *   0
 *
 ****************************************************************************/

#define bl_os_printf(M, ...)        g_bl_ops_funcs._printf(M, ##__VA_ARGS__)

/****************************************************************************
 * Name: bl_os_puts
 *
 * Description:
 *   Output format string
 *
 * Input Parameters:
 *   s - string
 *
 * Returned Value:
 *   0
 *
 ****************************************************************************/

#define bl_os_puts(S)               g_bl_ops_funcs._puts(S)

/****************************************************************************
 * Name: bl_os_msleep
 *
 * Description:
 *   Sleep in milliseconds
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_msleep                g_bl_ops_funcs._msleep

/****************************************************************************
 * Name: bl_os_sleep
 *
 * Description:
 *   Sleep in seconds
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_sleep                 g_bl_ops_funcs._sleep

/****************************************************************************
 * Name: bl_os_event_group_create(FreeRTOS only)
 *
 * Description:
 *   Create event group
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Event group data pointer
 *
 ****************************************************************************/

#define bl_os_event_group_create    g_bl_ops_funcs._event_group_create

/****************************************************************************
 * Name: bl_os_event_group_delete(FreeRTOS only)
 *
 * Description:
 *   Delete event group
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Event group data pointer
 *
 ****************************************************************************/

#define bl_os_event_group_delete    g_bl_ops_funcs._event_group_delete

/****************************************************************************
 * Name: bl_os_event_group_send(FreeRTOS only)
 *
 * Description:
 *  Set bits within an event group
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Event group data pointer
 *
 ****************************************************************************/

#define bl_os_event_group_send      g_bl_ops_funcs._event_group_send

/****************************************************************************
 * Name: bl_os_event_group_wait(FreeRTOS only)
 *
 * Description:
 *   block to wait for one or more bits to be set within a
 *   previously created event group.
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Event group data pointer
 *
 ****************************************************************************/

#define bl_os_event_group_wait      g_bl_ops_funcs._event_group_wait

/****************************************************************************
 * Name: bl_os_event_register
 *
 * Description:
 *   Temporarily only provide EV_WIFI to fw, leave it blank here
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Event group data pointer
 *
 ****************************************************************************/

#define bl_os_event_register        g_bl_ops_funcs._event_register

/****************************************************************************
 * Name: bl_os_event_notify
 *
 * Description:
 *   Post event to EV_WIFI
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Event group data pointer
 *
 ****************************************************************************/

#define bl_os_event_notify          g_bl_ops_funcs._event_notify

/****************************************************************************
 * Name: bl_os_task_create
 *
 * Description:
 *   Create task
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_task_create           g_bl_ops_funcs._task_create

/****************************************************************************
 * Name: bl_os_task_delete
 *
 * Description:
 *   Delete task
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_task_delete           g_bl_ops_funcs._task_delete

/****************************************************************************
 * Name: bl_os_task_get_current_task
 *
 * Description:
 *   Get current task handle
 *
 * Input Parameters:
 *
 * Returned Value:
 *   TaskHandle_t
 *
 ****************************************************************************/

#define bl_os_task_get_current_task g_bl_ops_funcs._task_get_current_task

/****************************************************************************
 * Name: bl_os_task_notify_create
 *
 * Description:
 *   Create task notify handle
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_task_notify_create    g_bl_ops_funcs._task_notify_create

/****************************************************************************
 * Name: bl_os_task_notify
 *
 * Description:
 *   Notify block task
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_task_notify           g_bl_ops_funcs._task_notify

/****************************************************************************
 * Name: bl_os_task_wait
 *
 * Description:
 *   Block task until notify
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_task_wait             g_bl_ops_funcs._task_wait

/****************************************************************************
 * Name: bl_os_irq_attach
 *
 * Description:
 *   Attach a irq callback
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_irq_attach            g_bl_ops_funcs._irq_attach

/****************************************************************************
 * Name: bl_os_irq_enable
 *
 * Description:
 *   Enable irq num
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_irq_enable            g_bl_ops_funcs._irq_enable

/****************************************************************************
 * Name: bl_os_irq_enable
 *
 * Description:
 *   Disable irq num
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_irq_disable           g_bl_ops_funcs._irq_disable

/****************************************************************************
 * Name: bl_os_workqueue_create
 *
 * Description:
 *   Create workqueue task
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_workqueue_create      g_bl_ops_funcs._workqueue_create

/****************************************************************************
 * Name: bl_os_workqueue_submit_hpwork
 *
 * Description:
 *   Start a task through high prio task
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_workqueue_submit_hp   g_bl_ops_funcs._workqueue_submit_hp

/****************************************************************************
 * Name: bl_os_workqueue_submit_hpwork
 *
 * Description:
 *   Start a task through lower prio task
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_workqueue_submit_lp   g_bl_ops_funcs._workqueue_submit_lp

/****************************************************************************
 * Name: bl_os_timer_create
 *
 * Description:
 *   Create a timer with callback and argv
 * Input Parameters:
 *   func - Callback function after the timer expires
 *   argv - The parameters passed into the callback function
 *
 * Returned Value:
 *   Timer handle
 *
 ****************************************************************************/

#define bl_os_timer_create          g_bl_ops_funcs._timer_create

/****************************************************************************
 * Name: bl_os_timer_delete
 *
 * Description:
 *   Delete timer
 *
 * Input Parameters:
 *   timerid - timer handle
 *   tick    - Specifies the time, in ticks, that the calling task should be
 *             held in the Blocked state to wait for the stop command to be
 *             successfully sent to the timer command queue
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_timer_delete          g_bl_ops_funcs._timer_delete

/****************************************************************************
 * Name: bl_os_timer_start_once
 *
 * Description:
 *   The timer starts once
 * Input Parameters:
 *   timerid - timer handle
 *   t_sec   - Timer trigger period, in seconds
 *   t_nsec  - Timer trigger period, in nanoseconds
 *
 * Returned Value:
 *   0 - success
 *   1 - fail
 *
 ****************************************************************************/

#define bl_os_timer_start_once      g_bl_ops_funcs._timer_start_once

/****************************************************************************
 * Name: bl_os_timer_start_once
 *
 * Description:
 *   The timer starts period
 * Input Parameters:
 *   timerid - timer handle
 *   t_sec   - Timer trigger period, in seconds
 *   t_nsec  - Timer trigger period, in nanoseconds
 *
 * Returned Value:
 *   0 - success
 *   1 - fail
 *
 ****************************************************************************/

#define bl_os_timer_start_periodic  g_bl_ops_funcs._timer_start_periodic

/****************************************************************************
 * Name: bl_os_sem_create
 *
 * Description:
 *   Create and initialize semaphore
 *
 * Input Parameters:
 *   max  - No mean
 *   init - semaphore initialization value
 *
 * Returned Value:
 *   Semaphore data pointer
 *
 ****************************************************************************/

#define bl_os_sem_create            g_bl_ops_funcs._sem_create

/****************************************************************************
 * Name: bl_os_sem_delete
 *
 * Description:
 *   Delete semaphore
 *
 * Input Parameters:
 *   semphr - Semaphore data pointer
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#define bl_os_sem_delete            g_bl_ops_funcs._sem_delete\

/****************************************************************************
 * Name: bl_os_sem_take
 *
 * Description:
 *   Wait semaphore within a certain period of time
 *
 * Input Parameters:
 *   semphr - Semaphore data pointer
 *   ticks  - Wait system ticks
 *
 * Returned Value:
 *   True if success or false if fail
 *
 ****************************************************************************/

#define bl_os_sem_take              g_bl_ops_funcs._sem_take

/****************************************************************************
 * Name: bl_os_sem_give
 *
 * Description:
 *   Post semaphore
 *
 * Input Parameters:
 *   semphr - Semaphore data pointer
 *
 * Returned Value:
 *   True if success or false if fail
 *
 ****************************************************************************/

#define bl_os_sem_give              g_bl_ops_funcs._sem_give

/****************************************************************************
 * Name: bl_os_mutex_create
 *
 * Description:
 *   Create mutex
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Mutex data pointer
 *
 ****************************************************************************/

#define bl_os_mutex_create          g_bl_ops_funcs._mutex_create

/****************************************************************************
 * Name: bl_os_mutex_delete
 *
 * Description:
 *   Delete mutex
 *
 * Input Parameters:
 *   mutex_data - mutex data pointer
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#define bl_os_mutex_delete          g_bl_ops_funcs._mutex_delete

/****************************************************************************
 * Name: bl_os_mutex_lock
 *
 * Description:
 *   Lock mutex
 *
 * Input Parameters:
 *   mutex_data - mutex data pointer
 *
 * Returned Value:
 *   True if success or false if fail
 *
 ****************************************************************************/

#define bl_os_mutex_lock            g_bl_ops_funcs._mutex_lock

/****************************************************************************
 * Name: bl_os_mutex_unlock
 *
 * Description:
 *   Lock mutex
 *
 * Input Parameters:
 *   mutex_data - mutex data pointer
 *
 * Returned Value:
 *   True if success or false if fail
 *
 ****************************************************************************/

#define bl_os_mutex_unlock          g_bl_ops_funcs._mutex_unlock

/****************************************************************************
 * Name: bl_os_mq_creat
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_queue_create          g_bl_ops_funcs._queue_create

/****************************************************************************
 * Name: bl_os_mq_delete
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_queue_delete          g_bl_ops_funcs._queue_delete

/****************************************************************************
 * Name: bl_os_mq_send_wait
 *
 * Description:
 *   Generic send message to queue within a certain period of time
 *
 * Input Parameters:
 *   queue - Message queue data pointer
 *   item  - Message data pointer
 *   ticks - Wait ticks
 *   prio  - Message priority
 *
 * Returned Value:
 *   True if success or false if fail
 *
 ****************************************************************************/

#define bl_os_queue_send_wait       g_bl_ops_funcs._queue_send_wait

/****************************************************************************
 * Name: bl_os_mq_send
 *
 * Description:
 *   Send message of low priority to queue within a certain period of time
 *
 * Input Parameters:
 *   queue - Message queue data pointer
 *   item  - Message data pointer
 *   ticks - Wait ticks
 *
 * Returned Value:
 *   True if success or false if fail
 *
 ****************************************************************************/

#define bl_os_queue_send            g_bl_ops_funcs._queue_send

/****************************************************************************
 * Name: bl_os_mq_recv
 *
 * Description:
 *   Receive message from queue within a certain period of time
 *
 * Input Parameters:
 *   queue - Message queue data pointer
 *   item  - Message data pointer
 *   ticks - Wait ticks
 *
 * Returned Value:
 *   True if success or false if fail
 *
 ****************************************************************************/

#define bl_os_queue_recv            g_bl_ops_funcs._queue_recv

/****************************************************************************
 * Name: bl_os_malloc
 *
 * Description:
 *   Allocate a block of memory
 *
 * Input Parameters:
 *   size - memory size
 *
 * Returned Value:
 *   Memory pointer
 *
 ****************************************************************************/

#define bl_os_malloc                g_bl_ops_funcs._malloc

/****************************************************************************
 * Name: bl_os_free
 *
 * Description:
 *   Free a block of memory
 *
 * Input Parameters:
 *   ptr - memory block
 *
 * Returned Value:
 *   No
 *
 ****************************************************************************/

#define bl_os_free                  g_bl_ops_funcs._free

/****************************************************************************
 * Name: bl_os_zalloc
 *
 * Description:
 *   Allocate a block of memory
 *
 * Input Parameters:
 *   size - memory size
 *
 * Returned Value:
 *   Memory pointer
 *
 ****************************************************************************/

#define bl_os_zalloc                g_bl_ops_funcs._zalloc

/****************************************************************************
 * Name: bl_os_clock_gettime_ms
 *
 * Description:
 *   Get the current system clock, in milliseconds
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

#define bl_os_get_time_ms           g_bl_ops_funcs._get_time_ms

/****************************************************************************
 * Name: bl_os_get_tick
 *
 * Description:
 *   Get current system tick
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/
#define bl_os_get_tick              g_bl_ops_funcs._get_tick

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _BL_OS_SYSTEM_H_ */
