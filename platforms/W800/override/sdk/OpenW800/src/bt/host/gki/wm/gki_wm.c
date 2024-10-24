/****************************************************************************
**
**  Name        gki_wm.c
**
**  Function    this file contains a template to be used when porting
**              GKI to a new operating system
**
**
**  Copyright (c) 2000-2006, Broadcom Corp., All Rights Reserved.
**  Proprietary and confidential.
*****************************************************************************/

#include "gki_int.h"
#include "gki.h"
#include "wm_timer.h"
#include "core_804.h"
#include "wm_osal.h"
#include "wm_mem.h"



/* Define the structure that holds the GKI variables
*/
#if GKI_DYNAMIC_MEMORY == FALSE
tGKI_CB   gki_cb;
#endif

static uint8_t bt_current_task = -1;
static uint32_t irq_saver;
static volatile uint32_t bdroid_sys_ticks = 0;
static volatile uint32_t bdroid_cmp_ticks = 0;
static volatile uint32_t bdroid_run_ticks = 0;


uint32_t GKI_time_get_os_boottime_ms(void)
{
    return tls_os_get_time();
}


/*******************************************************************************
**
** Function         GKI_init
**
** Description      This function is called once at startup to initialize
**                  all the timer structures.
**
** Returns          void
**
*******************************************************************************/
void GKI_init(void)
{
    wm_memset(&gki_cb, 0, sizeof(gki_cb));
    gki_buffer_init();
    gki_timers_init();
    gki_cb.com.OSTicks = tls_os_get_time();
    /* TODO - add any extra initialization here
    **
    ** this may include event or semaphore creation,
    ** memory allocation, etc etc*/
}

void GKI_shutdown(void)
{
}
void GKI_destroy_task(uint8_t task_id)
{
    return;
}
/*******************************************************************************
**
** Function         GKI_get_os_tick_count
**
** Description      This function is called to retrieve the native OS system tick.
**
** Returns          Tick count of native OS.
**
*******************************************************************************/
uint32_t GKI_get_os_tick_count(void)
{
    /* TODO - add any OS specific code here
    **/
    return (gki_cb.com.OSTicks);
}



/*******************************************************************************
**
** Function         GKI_create_task
**
** Description      This function is called to create a new OSS task.
**
** Parameters:      task_entry  - (input) pointer to the entry function of the task
**                  task_id     - (input) Task id is mapped to priority
**                  taskname    - (input) name given to the task
**                  stack       - (input) pointer to the top of the stack (highest memory location)
**                  stacksize   - (input) size of the stack allocated for the task
**
** Returns          GKI_SUCCESS if all OK, GKI_FAILURE if any problem
**
** NOTE             This function take some parameters that may not be needed
**                  by your particular OS. They are here for compatability
**                  of the function prototype.
**
*******************************************************************************/
uint8_t GKI_create_task(TASKPTR task_entry, TASKPTR task_ptr, uint8_t task_id, int8_t *taskname)
{
    if(task_id >= GKI_MAX_TASKS) {
        return (GKI_FAILURE);
    }

    gki_cb.com.OSRdyTbl[task_id]    = TASK_READY;
    gki_cb.com.OSTName[task_id]     = taskname;
    gki_cb.com.OSWaitTmr[task_id]   = 0;
    gki_cb.com.OSWaitEvt[task_id]   = 0;
    /* start task thread */
    gki_cb.os.task_ptr[task_id]   = task_ptr;
    bt_current_task = task_id;
    task_entry(0);
    bt_current_task = -1;
    return (GKI_SUCCESS);
}


/*******************************************************************************
**
** Function         GKI_run
**
** Description      This function runs a task
**
** Parameters:      p_task_id  - (input) pointer to task id
**
** Returns          void
**
** NOTE             This function is only needed for operating systems where
**                  starting a task is a 2-step process. Most OS's do it in
**                  one step, If your OS does it in one step, this function
**                  should be empty.
**
*******************************************************************************/
void GKI_run(void *p_task_id)
{
    uint16_t evt;
    int i;
    uint16_t flag;
    bt_current_task = -1;

    for(i = 0; i < GKI_MAX_TASKS; i++) {
        if(gki_cb.com.OSRdyTbl[i] == TASK_READY) {
            /* Return only those bits which user wants... */
            flag = gki_cb.com.OSWaitForEvt[i];
            /* Clear the wait for event mask */
            gki_cb.com.OSWaitForEvt[i] = 0;
            /* Return only those bits which user wants... */
            evt = gki_cb.com.OSWaitEvt[i] & flag;
            /* Clear only those bits which user wants... */
            gki_cb.com.OSWaitEvt[i] &= ~flag;
            bt_current_task = i;
            gki_cb.os.task_ptr[i](evt);
            break;
        }
    }

    bt_current_task = -1;
}

void GKI_service()
{
    if(bdroid_cmp_ticks != tls_os_get_time()) {
        bdroid_cmp_ticks = tls_os_get_time();
        GKI_run(NULL);
        bdroid_run_ticks++;

        if(bdroid_run_ticks >= 10) {
            bdroid_run_ticks = 0;
            GKI_timer_update(1);
        }
    }
}
/*******************************************************************************
**
** Function         GKI_suspend_task()
**
** Description      This function suspends the task specified in the argument.
**
** Parameters:      task_id  - (input) the id of the task that has to suspended
**
** Returns          GKI_SUCCESS if all OK, else GKI_FAILURE
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. If you want to implement task suspension capability,
**                  put specific code here.
**
*******************************************************************************/
uint8_t GKI_suspend_task(uint8_t task_id)
{
    /* TODO - add code here if needed*/
    return (GKI_SUCCESS);
}
void GKI_task_self_cleanup(uint8_t task_id)
{
}


/*******************************************************************************
**
** Function         GKI_resume_task()
**
** Description      This function resumes the task specified in the argument.
**
** Parameters:      task_id  - (input) the id of the task that has to resumed
**
** Returns          GKI_SUCCESS if all OK
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. If you want to implement task suspension capability,
**                  put specific code here.
**
*******************************************************************************/
uint8_t GKI_resume_task(uint8_t task_id)
{
    /* TODO - add code here if needed*/
    return (GKI_SUCCESS);
}


/*******************************************************************************
**
** Function         GKI_exit_task
**
** Description      This function is called to stop a GKI task.
**
** Parameters:      task_id  - (input) the id of the task that has to be stopped
**
** Returns          void
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. If you want to use it in your own implementation,
**                  put specific code here to kill a task.
**
*******************************************************************************/
void GKI_exit_task(uint8_t task_id)
{
    GKI_disable();
    gki_cb.com.OSRdyTbl[task_id] = TASK_DEAD;
    GKI_enable();
    /* TODO - add code here if needed*/
    return;
}


/*******************************************************************************
**
** Function         GKI_sched_lock
**
** Description      This function is called by tasks to disable scheduler
**                  task context switching.
**
** Returns          void
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. If you want to use it in your own implementation,
**                  put code here to tell the OS to disable context switching.
**
*******************************************************************************/
void GKI_sched_lock(void)
{
    /* TODO - add code here if needed*/
    return;
}


/*******************************************************************************
**
** Function         GKI_sched_unlock
**
** Description      This function is called by tasks to enable scheduler switching.
**
** Returns          void
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. If you want to use it in your own implementation,
**                  put code here to tell the OS to re-enable context switching.
**
*******************************************************************************/
void GKI_sched_unlock(void)
{
    /* TODO - add code here if needed*/
}


/*******************************************************************************
**
** Function         GKI_wait
**
** Description      This function is called by tasks to wait for a specific
**                  event or set of events. The task may specify the duration
**                  that it wants to wait for, or 0 if infinite.
**
** Parameters:      flag -    (input) the event or set of events to wait for
**                  timeout - (input) the duration that the task wants to wait
**                                    for the specific events (in system ticks)
**
**
** Returns          the event mask of received events or zero if timeout
**
*******************************************************************************/
uint16_t GKI_wait(uint16_t flag, uint32_t timeout)
{
    uint8_t   rtask;
    rtask = GKI_get_taskid();
    gki_cb.com.OSWaitForEvt[rtask] = flag;

    /* Check if anything in any of the mailboxes. Possible race condition. */
    if(gki_cb.com.OSTaskQFirst[rtask][0]) {
        gki_cb.com.OSWaitEvt[rtask] |= TASK_MBOX_0_EVT_MASK;
    }

    if(gki_cb.com.OSTaskQFirst[rtask][1]) {
        gki_cb.com.OSWaitEvt[rtask] |= TASK_MBOX_1_EVT_MASK;
    }

    if(gki_cb.com.OSTaskQFirst[rtask][2]) {
        gki_cb.com.OSWaitEvt[rtask] |= TASK_MBOX_2_EVT_MASK;
    }

    if(gki_cb.com.OSTaskQFirst[rtask][3]) {
        gki_cb.com.OSWaitEvt[rtask] |= TASK_MBOX_3_EVT_MASK;
    }

    if(!(gki_cb.com.OSWaitEvt[rtask] & flag)) {
        //      printf("task %d now waiting\n", rtask);
        gki_cb.com.OSRdyTbl[rtask] = TASK_WAIT;
        gki_cb.com.OSWaitTmr[rtask] = timeout;
        gki_adjust_timer_count((int32_t)timeout);
    }

    return(0);
}


/*******************************************************************************
**
** Function         GKI_delay
**
** Description      This function is called by tasks to sleep unconditionally
**                  for a specified amount of time. The duration is in milliseconds
**
** Parameters:      timeout -    (input) the duration in milliseconds
**
** Returns          void
**
*******************************************************************************/
void GKI_delay(uint32_t timeout)
{
    uint8_t rtask = GKI_get_taskid();
    uint32_t timeout_ticks = 0;
    timeout_ticks = GKI_MS_TO_TICKS(timeout);     /* convert from milliseconds to ticks */

    if(timeout_ticks == 0) {
        timeout_ticks = 1;
    }

    gki_cb.com.OSRdyTbl[rtask] = TASK_WAIT;
    gki_cb.com.OSWaitTmr[rtask] = timeout_ticks;
    gki_adjust_timer_count((int32_t)timeout_ticks);

    /* Check if task was killed while sleeping */
    /* NOTE
    **      if you do not implement task killing, you do not
    **      need this check.
    */
    if(rtask && gki_cb.com.OSRdyTbl[rtask] == TASK_DEAD) {
        /* TODO - add code here to exit the task*/
    }

    return;
}


/*******************************************************************************
**
** Function         GKI_send_event
**
** Description      This function is called by tasks to send events to other
**                  tasks. Tasks can also send events to themselves.
**
** Parameters:      task_id -  (input) The id of the task to which the event has to
**                  be sent
**                  event   -  (input) The event that has to be sent
**
**
** Returns          GKI_SUCCESS if all OK, else GKI_FAILURE
**
*******************************************************************************/
uint8_t GKI_send_event(uint8_t task_id, uint16_t event)
{
    if(task_id >= GKI_MAX_TASKS) {
        return (GKI_FAILURE);
    }

    uint32_t task_event = event | task_id << 16;
#ifndef CONFIG_KERNEL_NONE
    extern void active_host_task(uint32_t event);
    active_host_task(task_event);
#endif
    return (GKI_SUCCESS);
}
void GKI_event_process(uint32_t task_event)
{
    uint8_t task_id = task_event >> 16 & 0xFF;
    uint16_t event = task_event & 0xFFFF;
    GKI_disable();
    /* Set the event bit */
    gki_cb.com.OSWaitEvt[task_id] |= event;
    gki_cb.com.OSRdyTbl[task_id] = TASK_READY;
    //printf("task %d now ready with event=0x%08x\n", task_id, event);
    GKI_enable();
    /* TODO - add code here to send an event to the task*/
    GKI_run(NULL);
}

/*******************************************************************************
**
** Function         GKI_isend_event
**
** Description      This function is called from ISRs to send events to other
**                  tasks. The only difference between this function and GKI_send_event
**                  is that this function assumes interrupts are already disabled.
**
** Parameters:      task_id -  (input) The destination task Id for the event.
**                  event   -  (input) The event flag
**
** Returns          GKI_SUCCESS if all OK, else GKI_FAILURE
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. If you want to use it in your own implementation,
**                  put your code here, otherwise you can delete the entire
**                  body of the function.
**
*******************************************************************************/
uint8_t GKI_isend_event(uint8_t task_id, uint16_t event)
{
    if(task_id >= GKI_MAX_TASKS) {
        return (GKI_FAILURE);
    }

    gki_cb.com.OSWaitEvt[task_id] |= event;
    gki_cb.com.OSRdyTbl[task_id] = TASK_READY;
    return (GKI_SUCCESS);
}


/*******************************************************************************
**
** Function         GKI_get_taskid
**
** Description      This function gets the currently running task ID.
**
** Returns          task ID
**
** NOTE             The Widcomm upper stack and profiles may run as a single task.
**                  If you only have one GKI task, then you can hard-code this
**                  function to return a '1'. Otherwise, you should have some
**                  OS-specific method to determine the current task.
**
*******************************************************************************/
uint8_t GKI_get_taskid(void)
{
    return(bt_current_task);
}


/*******************************************************************************
**
** Function         GKI_map_taskname
**
** Description      This function gets the task name of the taskid passed as arg.
**                  If GKI_MAX_TASKS is passed as arg the currently running task
**                  name is returned
**
** Parameters:      task_id -  (input) The id of the task whose name is being
**                  sought. GKI_MAX_TASKS is passed to get the name of the
**                  currently running task.
**
** Returns          pointer to task name
**
** NOTE             this function needs no customization
**
*******************************************************************************/
int8_t *GKI_map_taskname(uint8_t task_id)
{
    if(task_id < GKI_MAX_TASKS) {
        return (gki_cb.com.OSTName[task_id]);
    } else if(task_id == GKI_MAX_TASKS) {
        return (gki_cb.com.OSTName[GKI_get_taskid()]);
    } else {
        return (int8_t *)"BAD";
    }
}


/*******************************************************************************
**
** Function         GKI_enable
**
** Description      This function enables interrupts.
**
** Returns          void
**
*******************************************************************************/
void GKI_enable(void)
{
    /* TODO - add code here to enable interrupts*/
    //csi_irq_restore(irq_saver);
    tls_os_release_critical(irq_saver);
    return;
}


/*******************************************************************************
**
** Function         GKI_disable
**
** Description      This function disables interrupts.
**
** Returns          void
**
*******************************************************************************/
void GKI_disable(void)
{
    /* TODO - add code here to disable interrupts*/
    //irq_saver = csi_irq_save();
    irq_saver = tls_os_set_critical();
    return;
}


/*******************************************************************************
**
** Function         GKI_exception
**
** Description      This function throws an exception.
**                  This is normally only called for a nonrecoverable error.
**
** Parameters:      code    -  (input) The code for the error
**                  msg     -  (input) The message that has to be logged
**
** Returns          void
**
*******************************************************************************/

void GKI_exception(uint16_t code, char *msg)
{
#if (GKI_DEBUG == TRUE)
    GKI_disable();

    if(gki_cb.com.ExceptionCnt < GKI_MAX_EXCEPTION) {
        EXCEPTION_T *pExp;
        pExp =  &gki_cb.com.Exception[gki_cb.com.ExceptionCnt++];
        pExp->type = code;
        pExp->taskid = GKI_get_taskid();
        strncpy((char *)pExp->msg, msg, GKI_MAX_EXCEPTION_MSGLEN - 1);
    }

    GKI_enable();
#endif
    printf("xxxGKI_exception...code[%d], msg=%s\r\n", code, msg);
    /* TODO - add code here to display the message and reset the device*/
    return;
}


/*******************************************************************************
**
** Function         GKI_get_time_stamp
**
** Description      This function formats the time into a user area
**
** Parameters:      tbuf -  (output) the address to the memory containing the
**                  formatted time
**
** Returns          the address of the user area containing the formatted time
**                  The format of the time is ????
**
** NOTE             This function is only called by OBEX.
**
*******************************************************************************/
int8_t *GKI_get_time_stamp(int8_t *tbuf)
{
    /* TODO - replace the following with OS-specific code that gets a timestamp.*/
    uint32_t ms_time;
    uint32_t s_time;
    uint32_t m_time;
    uint32_t h_time;
    int8_t   *p_out = tbuf;
    ms_time = GKI_TICKS_TO_MS(gki_cb.com.OSTicks);
    s_time  = ms_time / 100; /* 100 Ticks per second */
    m_time  = s_time / 60;
    h_time  = m_time / 60;
    ms_time -= s_time * 100;
    s_time  -= m_time * 60;
    m_time  -= h_time * 60;
    *p_out++ = (int8_t)((h_time / 10) + '0');
    *p_out++ = (int8_t)((h_time % 10) + '0');
    *p_out++ = ':';
    *p_out++ = (int8_t)((m_time / 10) + '0');
    *p_out++ = (int8_t)((m_time % 10) + '0');
    *p_out++ = ':';
    *p_out++ = (int8_t)((s_time / 10) + '0');
    *p_out++ = (int8_t)((s_time % 10) + '0');
    *p_out++ = ':';
    *p_out++ = (int8_t)((ms_time / 10) + '0');
    *p_out++ = (int8_t)((ms_time % 10) + '0');
    *p_out++ = ':';
    *p_out   = 0;
    return (tbuf);
}


/*******************************************************************************
**
** Function         GKI_register_mempool
**
** Description      This function registers a specific memory pool.
**
** Parameters:      p_mem -  (input) pointer to the memory pool
**
** Returns          void
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. If your OS has different memory pools, you
**                  can tell GKI the pool to use by calling this function.
**
*******************************************************************************/
void GKI_register_mempool(void *p_mem)
{
    gki_cb.com.p_user_mempool = p_mem;
    return;
}

/*******************************************************************************
**
** Function         GKI_os_malloc
**
** Description      This function allocates memory
**
** Parameters:      size -  (input) The size of the memory that has to be
**                  allocated
**
** Returns          the address of the memory allocated, or NULL if failed
**
** NOTE             This function is called by the Widcomm stack when
**                  dynamic memory allocation is used. (see dyn_mem.h)
**
*******************************************************************************/
void *GKI_os_malloc_debug(uint32_t size, const char *file, int line)
{
    printf("%d bytes, %s, %d\r\n", size, file, line);
    void *pByte = tls_mem_alloc(size);
    return pByte;
}
void *GKI_os_malloc(uint32_t size)
{
    void *pByte = tls_mem_alloc(size);
    return pByte;
}

/*******************************************************************************
**
** Function         GKI_os_free
**
** Description      This function frees memory
**
** Parameters:      size -  (input) The address of the memory that has to be
**                  freed
**
** Returns          void
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. It is only called from within GKI if dynamic
**
*******************************************************************************/
void GKI_os_free(void *p_mem)
{
    if(p_mem) {
        tls_mem_free(p_mem);
    }

    return;
}

