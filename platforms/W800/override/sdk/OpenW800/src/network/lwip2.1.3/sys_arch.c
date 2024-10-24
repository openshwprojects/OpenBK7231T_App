#include "lwip/debug.h"
#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/stats.h"
#include "wm_wl_task.h"
#include "arch/sys_arch.h"
#include "wm_osal.h"
#include "wm_mem.h"
#include "wm_debug.h"
#include "wm_config.h"
#include "wm_socket.h"


const void * const null_pointer = (void *)0;
OS_STK         lwip_task_stk[LWIP_TASK_MAX*LWIP_STK_SIZE];
u8_t            lwip_task_priopity_stack[LWIP_TASK_MAX];
//OS_TCB          lwip_task_tcb[LWIP_TASK_MAX];
#if LWIP_NETCONN_SEM_PER_THREAD
#if TLS_OS_FREERTOS
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wm_type_def.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rtosqueue.h"
#include "semphr.h"
#include "rtostimers.h"
#include "FreeRTOSConfig.h"
#endif

sys_sem_t g_lwip_netconn_thread_sems[64];
sys_sem_t* sys_lwip_netconn_thread_sem_get()
{
	long prio = uxTaskPriorityGet(NULL);
	if(g_lwip_netconn_thread_sems[prio] == NULL)
	{
		if(sys_sem_new(&g_lwip_netconn_thread_sems[prio], 0) != ERR_OK)
		{
			LWIP_ASSERT("invalid lwip_netconn_thread_sem!", (g_lwip_netconn_thread_sems[prio] != NULL));
		}
	}
	return &g_lwip_netconn_thread_sems[prio];
}

#endif
/**
 * \brief Initialize the sys_arch layer.
 */
void sys_init(void)
{
#if LWIP_NETCONN_SEM_PER_THREAD
	memset(g_lwip_netconn_thread_sems, 0, sizeof(sys_sem_t) * 64);
#endif
}

u32_t sys_now(void)
{
	return tls_os_get_time()*1000ULL/HZ;
}

/**
 * \brief Creates and returns a new semaphore.
 *
 * \param sem Pointer to the semaphore.
 * \param count Initial state of the semaphore.
 *
 * \return ERR_OK for OK, other value indicates error.
 */
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
	err_t err_sem = ERR_MEM;
    tls_os_status_t status;

	/* Sanity check */
	if (sem != NULL) {

        status = tls_os_sem_create(sem, count);
		if (status == TLS_OS_SUCCESS) {
  #if SYS_STATS
			lwip_stats.sys.sem.used++;
			if (lwip_stats.sys.sem.used > lwip_stats.sys.sem.max) {
				lwip_stats.sys.sem.max = lwip_stats.sys.sem.used;
			}

  #endif /* SYS_STATS */
			err_sem = ERR_OK;
		}

	}

	return err_sem;
}

/**
 * \brief Frees a semaphore created by sys_sem_new.
 *
 * \param sem Pointer to the semaphore.
 */
void sys_sem_free(sys_sem_t *sem)
{
	/* Sanity check */
	if (*sem != NULL)
        tls_os_sem_delete(*sem);

	*sem = SYS_SEM_NULL;
}

/**
 * \brief Signals (or releases) a semaphore.
 *
 * \param sem Pointer to the semaphore.
 */
void sys_sem_signal(sys_sem_t *sem)
{
	/* Sanity check */
	if (*sem != NULL) {
        tls_os_sem_release(*sem);
	}
}

/**
 * \brief Blocks the thread while waiting for the semaphore to be signaled.
 * Note that there is another function sys_sem_wait in sys.c, but it is a wrapper
 * for the sys_arch_sem_wait function. Please note that it is important for the
 * semaphores to return an accurate count of elapsed milliseconds, since they are
 * used to schedule timers in lwIP.
 *
 * \param sem Pointer to the semaphore.
 * \param timeout The timeout parameter specifies how many milliseconds the
 * function should block before returning; if the function times out, it should
 * return SYS_ARCH_TIMEOUT. If timeout=0, then the function should block
 * indefinitely. If the function acquires the semaphore, it should return how
 * many milliseconds expired while waiting for the semaphore. 
 *
 * \return SYS_ARCH_TIMEOUT if times out, ERR_MEM for semaphore erro otherwise
 * return the milliseconds expired while waiting for the semaphore.
 */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    tls_os_status_t status;
    u32_t        os_timeout = 0;
    u32_t        in_timeout = (timeout*HZ)/1000;

    if(timeout && in_timeout == 0)
        in_timeout = 1;  
    status = tls_os_sem_acquire(*sem, in_timeout); 
#if TLS_OS_FREERTOS
	if(status == TLS_OS_ERROR)
		os_timeout = SYS_ARCH_TIMEOUT;
#endif
    return os_timeout;    
}

#ifndef sys_sem_valid
/**
 * \brief Check if a sempahore is valid/allocated.
 *
 * \param sem Pointer to the semaphore.
 *
 * \return Semaphore number on valid, 0 for invalid.
 */
int sys_sem_valid(sys_sem_t *sem)
{
	if(*sem == SYS_SEM_NULL)
		return 0;
	return 1;
}

#endif

#ifndef sys_sem_set_invalid
/**
 * \brief Set a semaphore invalid.
 *
 * \param sem Pointer to the semaphore.
 */
void sys_sem_set_invalid(sys_sem_t *sem)
{
#if 1
	*sem = SYS_SEM_NULL;
#endif
}
#endif

/**
 * \brief Creates an empty mailbox for maximum "size" elements. Elements stored
 * in mailboxes are pointers. 
 *
 * \param mBoxNew Pointer to the new mailbox.
 * \param size Maximum "size" elements.
 *
 * \return ERR_OK if successfull or ERR_MEM on error.
 */
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
//    void *msg;
    err_t err;
    tls_os_status_t status;

    if (size == 0)
        size = 10;

    status = tls_os_queue_create(mbox,size);
	if (status == TLS_OS_SUCCESS) {
        err = ERR_OK;
    }
    else
        err = ERR_RTE;		

#if SYS_STATS
    if (SYS_MBOX_NULL != *mbox) {
        lwip_stats.sys.mbox.used++;
        if (lwip_stats.sys.mbox.used > lwip_stats.sys.mbox.max) {
            lwip_stats.sys.mbox.max	= lwip_stats.sys.mbox.used;
        }
    }
#endif /* SYS_STATS */
    return err;
}

/**
 * \brief Deallocates a mailbox.
 * If there are messages still present in the mailbox when the mailbox is
 * deallocated, it is an indication of a programming error in lwIP and the
 * developer should be notified.
 *
 * \param mbox Pointer to the new mailbox.
 */
void sys_mbox_free(sys_mbox_t *mbox)
{
    //u32_t cpu_sr;
    LWIP_ASSERT( "sys_mbox_free ", *mbox != SYS_MBOX_NULL );      

	tls_os_queue_flush(*mbox);
	//cpu_sr = tls_os_set_critical();
	//int err = tls_os_queue_delete(*mbox);
	tls_os_queue_delete(*mbox);
    //tls_os_release_critical(cpu_sr);
    //LWIP_ASSERT("OSQDel ", err == TLS_OS_SUCCESS);
    /* Sanity check */
}

/**
 * \brief Posts the "msg" to the mailbox. This function have to block until the
 * "msg" is really posted.
 *
 * \param mbox Pointer to the mailbox.
 * \param msg Pointer to the message to be post.
 */
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    u8_t     err;
    u8_t  i=0; 

    if( msg == NULL ) 
        msg = (void*)null_pointer;
    /* try 10 times */
    while (i < 10){
        //err = OSQPost(mbox, msg);
        err = tls_os_queue_send(*mbox, msg, 0);
        if(err == TLS_OS_SUCCESS)
            break;
        i++;
        //OSTimeDly(5);
        tls_os_time_delay(1);
    }
    LWIP_ASSERT( "sys_mbox_post error!\n", i !=10 );      
}

/**
 * \brief Try to posts the "msg" to the mailbox.
 *
 * \param mbox Pointer to the mailbox.
 * \param msg Pointer to the message to be post.
 *
 * \return ERR_MEM if the mailbox is full otherwise ERR_OK if the "msg" is posted.
 */
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    u8_t     err;
    u8_t  i=0; 

    if(msg == NULL ) 
        msg = (void*)null_pointer;  

    /* try 10 times */
    while (i < 10){
       // err = OSQPost(mbox, msg);
	err = tls_os_queue_send(*mbox, msg, 0);
        if(err == TLS_OS_SUCCESS)
            return ERR_OK;
        i++;
        //OSTimeDly(5);
        tls_os_time_delay(1);
    }
    return ERR_ABRT;    
}

/**
 * \brief Blocks the thread until a message arrives in the mailbox, but does not
 * block the thread longer than "timeout" milliseconds (similar to the
 * sys_arch_sem_wait() function).
 *
 * \param mbox Pointer to the mailbox.
 * \param msg A result parameter that is set by the function (i.e., by doing
 * "*msg = ptr"). The "msg" parameter maybe NULL to indicate that the message
 * should be dropped.
 * \timeout 0 indicates the thread should be blocked until a message arrives.
 *
 * \return Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was
 * a timeout. Or ERR_MEM if invalid pointer to message box.
 */
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
    u8_t        err;
    u32_t         in_timeout = timeout * HZ / 1000;
    u32_t       tick_start, tick_stop, tick_elapsed;

    if(timeout && in_timeout == 0)
        in_timeout = 1;

    tick_start = tls_os_get_time();
//    *msg = OSQPend(mbox, in_timeout, &err);
	err = tls_os_queue_receive(*mbox,msg,0,in_timeout);
//    if (err == OS_ERR_TIMEOUT ) {
	if (err != TLS_OS_SUCCESS ) {

        return SYS_ARCH_TIMEOUT;
    }
    tick_stop = tls_os_get_time();

    // Take care of wrap-around.
    if( tick_stop >= tick_start )
        tick_elapsed = tick_stop - tick_start;
    else
        tick_elapsed = 0xFFFFFFFF - tick_start + tick_stop ;
    

    return tick_elapsed * 1000/HZ;    
}

#ifndef sys_mbox_valid
/**
 * \brief Check if an mbox is valid/allocated.
 *
 * \param mbox Pointer to the mailbox.
 *
 * \return Mailbox for valid, 0 for invalid.
 */
int sys_mbox_valid(sys_mbox_t *mbox)
{
    if (*mbox == NULL)
        return 0;
    else 
        return 1;
}
#endif

#ifndef sys_mbox_set_invalid
/**
 * \brief Set an mbox invalid.
 *
 * \param mbox Pointer to the mailbox.
 */
void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
#if 1
	*mbox = SYS_MBOX_NULL;
#endif
}

#endif
/**
 * \brief Instantiate a thread for lwIP. Both the id and the priority are
 * system dependent.
 *
 * \param name Pointer to the thread name.
 * \param thread Thread function.
 * \param arg Argument will be passed into the thread().
 * \param stacksize Stack size of the thread.
 * \param prio Thread priority.
 *
 * \return The id of the new thread.
 */
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg,
		int stacksize, int prio)
{
    u8_t        ubPrio = LWIP_TASK_START_PRIO;
    int         i; 

    if (prio) {
        ubPrio += (prio-1);
        for (i=0; i<LWIP_TASK_MAX; ++i)
            if (lwip_task_priopity_stack[i] == ubPrio)
                break;
        if (i == LWIP_TASK_MAX) {
            for (i=0; i<LWIP_TASK_MAX; ++i)
                if (lwip_task_priopity_stack[i]==0){
                    lwip_task_priopity_stack[i] = ubPrio;
                    break;
                }
            if (i == LWIP_TASK_MAX) {
                LWIP_ASSERT( "sys_thread_new: there is no space for priority", 0 );
                return (1);
            }        
        } else
            prio = 0;
    }
    /* Search for a suitable priority */     
    if (!prio) {
        ubPrio = LWIP_TASK_START_PRIO;
        while (ubPrio < (LWIP_TASK_START_PRIO+LWIP_TASK_MAX)) { 
            for (i=0; i<LWIP_TASK_MAX; ++i)
                if (lwip_task_priopity_stack[i] == ubPrio) {
                    ++ubPrio;
                    break;
                }
            if (i == LWIP_TASK_MAX)
                break;
        }
        if (ubPrio < (LWIP_TASK_START_PRIO+LWIP_TASK_MAX))
            for (i=0; i<LWIP_TASK_MAX; ++i)
                if (lwip_task_priopity_stack[i]==0) {
                    lwip_task_priopity_stack[i] = ubPrio;
                    break;
                }
        if (ubPrio >= (LWIP_TASK_START_PRIO+LWIP_TASK_MAX) || i == LWIP_TASK_MAX) {
            LWIP_ASSERT( "sys_thread_new: there is no free priority", 0 );
            return (1);
        }
    }
    if (stacksize > LWIP_STK_SIZE || !stacksize)   
        stacksize = LWIP_STK_SIZE;

    int tsk_prio = ubPrio-LWIP_TASK_START_PRIO;
    OS_STK * task_stk = &lwip_task_stk[tsk_prio*LWIP_STK_SIZE];

    tls_os_task_create(NULL, NULL,
                       thread,
                       (void *)arg,
                       (void *)task_stk,
                       stacksize * sizeof(u32),
                       ubPrio,
                       0);
    
    return ubPrio;
}

/**
 * \brief Protect the system.
 *
 * \return 1 on success.
 */
sys_prot_t sys_arch_protect(void)
{
	return tls_os_set_critical(); /* Not used */
}

/**
 * \brief Unprotect the system.
 *
 * \param pval Protect value.
 */
void sys_arch_unprotect(sys_prot_t pval)
{
    tls_os_release_critical(pval);
}
