/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
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
#ifndef GKI_H
#define GKI_H


#include "bt_target.h"
#include "bt_types.h"
#include "wm_mem.h"
/* Error codes */
#define GKI_SUCCESS         0x00
#define GKI_FAILURE         0x01
#define GKI_INVALID_TASK    0xF0
#define GKI_INVALID_POOL    0xFF


/************************************************************************
** Mailbox definitions. Each task has 4 mailboxes that are used to
** send buffers to the task.
*/
#define TASK_MBOX_0    0
#define TASK_MBOX_1    1
#define TASK_MBOX_2    2
#define TASK_MBOX_3    3

#define NUM_TASK_MBOX  4

/************************************************************************
** Event definitions.
**
** There are 4 reserved events used to signal messages rcvd in task mailboxes.
** There are 4 reserved events used to signal timeout events.
** There are 8 general purpose events available for applications.
*/
#define MAX_EVENTS              16

#define TASK_MBOX_0_EVT_MASK   0x0001
#define TASK_MBOX_1_EVT_MASK   0x0002
#define TASK_MBOX_2_EVT_MASK   0x0004
#define TASK_MBOX_3_EVT_MASK   0x0008


#define TIMER_0             0
#define TIMER_1             1
#define TIMER_2             2
#define TIMER_3             3

#define TIMER_0_EVT_MASK    0x0010
#define TIMER_1_EVT_MASK    0x0020
#define TIMER_2_EVT_MASK    0x0040
#define TIMER_3_EVT_MASK    0x0080

#define APPL_EVT_0          8
#define APPL_EVT_1          9
#define APPL_EVT_2          10
#define APPL_EVT_3          11
#define APPL_EVT_4          12
#define APPL_EVT_5          13
#define APPL_EVT_6          14
#define APPL_EVT_7          15

#define EVENT_MASK(evt)       ((uint16_t)(0x0001 << (evt)))

/* Timer list entry callback type
*/
typedef void (TIMER_CBACK)(void *p_tle);
#ifndef TIMER_PARAM_TYPE
#define TIMER_PARAM_TYPE    uint32_t
#endif
/* Define a timer list entry
*/
typedef struct _tle {
    struct _tle  *p_next;
    struct _tle  *p_prev;
    TIMER_CBACK  *p_cback;
    int32_t         ticks;
    int32_t         ticks_initial;
    TIMER_PARAM_TYPE   param;
    TIMER_PARAM_TYPE   data;
    uint16_t        event;
    uint8_t         in_use;
} TIMER_LIST_ENT;

/* Define a timer list queue
*/
typedef struct {
    TIMER_LIST_ENT   *p_first;
    TIMER_LIST_ENT   *p_last;
} TIMER_LIST_Q;


/***********************************************************************
** This queue is a general purpose buffer queue, for application use.
*/
typedef struct {
    void    *p_first;
    void    *p_last;
    uint16_t   count;
} BUFFER_Q;

#define GKI_IS_QUEUE_EMPTY(p_q) ((p_q)->count == 0)

/* Task constants
*/
#ifndef TASKPTR
typedef void (*TASKPTR)(uint32_t);
#endif


#define GKI_PUBLIC_POOL         0       /* General pool accessible to GKI_getbuf() */
#define GKI_RESTRICTED_POOL     1       /* Inaccessible pool to GKI_getbuf() */

/***********************************************************************
** Function prototypes
*/

#ifdef __cplusplus
extern "C" {
#endif

/* Task management
*/
extern uint8_t   GKI_create_task(TASKPTR, TASKPTR, uint8_t, int8_t *);
extern void    GKI_destroy_task(uint8_t task_id);
extern void    GKI_task_self_cleanup(uint8_t task_id);
extern void    GKI_exit_task(uint8_t);
extern uint8_t   GKI_get_taskid(void);
extern void    GKI_init(void);
extern void    GKI_shutdown(void);
extern int8_t   *GKI_map_taskname(uint8_t);
extern void    GKI_run(void *);
extern void    GKI_stop(void);
extern void    GKI_event_process(uint32_t task_event);

/* To send buffers and events between tasks
*/
extern void   *GKI_read_mbox(uint8_t);
extern void    GKI_send_msg(uint8_t, uint8_t, void *);
extern uint8_t   GKI_send_event(uint8_t, uint16_t);


/* To get and release buffers, change owner and get size
*/
extern void    GKI_freebuf(void *);
extern void   *GKI_getbuf(uint16_t);
extern uint16_t  GKI_get_buf_size(void *);
extern void   *GKI_getpoolbuf(uint8_t);
extern void GKI_free_and_reset_buf(void **p_ptr);
extern uint16_t  GKI_poolcount(uint8_t);
extern uint16_t  GKI_poolfreecount(uint8_t);
extern uint16_t  GKI_poolutilization(uint8_t);
extern void GKI_dealloc_free_queue(void);



/* User buffer queue management
*/
extern void   *GKI_dequeue(BUFFER_Q *);
extern void    GKI_enqueue(BUFFER_Q *, void *);
extern void    GKI_enqueue_head(BUFFER_Q *, void *);
extern void   *GKI_getfirst(BUFFER_Q *);
extern void   *GKI_getlast(BUFFER_Q *);
extern void   *GKI_getnext(void *);
extern void    GKI_init_q(BUFFER_Q *);
extern uint8_t GKI_queue_is_empty(BUFFER_Q *);
extern void   *GKI_remove_from_queue(BUFFER_Q *, void *);
extern uint16_t  GKI_get_pool_bufsize(uint8_t);

/* Timer management
*/
extern void    GKI_add_to_timer_list(TIMER_LIST_Q *, TIMER_LIST_ENT *);
extern void    GKI_delay(uint32_t);
extern uint32_t  GKI_get_tick_count(void);
extern void    GKI_init_timer_list(TIMER_LIST_Q *);
extern int32_t   GKI_ready_to_sleep(void);
extern uint8_t GKI_remove_from_timer_list(TIMER_LIST_Q *, TIMER_LIST_ENT *);
extern void    GKI_start_timer(uint8_t, int32_t, uint8_t);
extern void    GKI_stop_timer(uint8_t);
extern void    GKI_timer_update(int32_t);
extern uint16_t  GKI_update_timer_list(TIMER_LIST_Q *, int32_t);
extern uint32_t  GKI_get_remaining_ticks(TIMER_LIST_Q *, TIMER_LIST_ENT *);
extern uint16_t  GKI_wait(uint16_t, uint32_t);
extern bool GKI_timer_queue_is_empty(const TIMER_LIST_Q *timer_q);
extern TIMER_LIST_ENT *GKI_timer_getfirst(const TIMER_LIST_Q *timer_q);
extern int32_t GKI_timer_ticks_getinitial(const TIMER_LIST_ENT *tle);
extern uint32_t GKI_time_get_os_boottime_ms(void);

/* Disable Interrupts, Enable Interrupts
*/
extern void    GKI_enable(void);
extern void    GKI_disable(void);

/* Allocate (Free) memory from an OS
*/
#if 0
extern void *GKI_os_malloc_debug(uint32_t size, const char *file, int line);
#define GKI_os_malloc(size)   GKI_os_malloc_debug(size, __FILE__, __LINE__)
#else
extern void *GKI_os_malloc(uint32_t size);
#endif

extern void     GKI_os_free(void *);

/* os timer operation */
extern uint32_t GKI_get_os_tick_count(void);

/* Exception handling
*/
extern void    GKI_exception(uint16_t, char *);

#if GKI_DEBUG == TRUE
extern void    GKI_PrintBufferUsage(uint8_t *p_num_pools, uint16_t *p_cur_used);
extern void    GKI_PrintBuffer(void);
extern void    GKI_print_task(void);
#else
#undef GKI_PrintBufferUsage
#define GKI_PrintBuffer() NULL
#endif

#ifdef __cplusplus
}
#endif


#endif
