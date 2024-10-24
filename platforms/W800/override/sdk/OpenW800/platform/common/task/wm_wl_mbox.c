/**
 * @file    wm_wl_mbox.c
 *
 * @brief   mailbox (mbox) APIs
 *
 * @author  dave
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */
#include "wm_mem.h"
#include "wm_wl_mbox.h"
#include "wm_wl_task.h"
const void * const tls_null_pointer = (void *)0;

/**
 * @brief          create a malibox
 *
 * @param[out]     *mbox              pointer to the mailbox
 * @param[in]      size               size of mailbox
 *
 * @retval         TLS_OS_SUCCESS     success
 * @retval         TLS_OS_ERROR       failed
 *
 * @note           None
 */
s8 tls_mbox_new(tls_mbox_t *mbox, int size)
{
    s8 err;
    tls_os_status_t status;

    if (size == 0)
        size = 10;

    status = tls_os_queue_create(mbox, size);
	if (status == TLS_OS_SUCCESS) {
        err = TLS_OS_SUCCESS;
    }
    else
        err = TLS_OS_ERROR;
    return err;
}

#ifndef tls_mbox_valid
/**
 * @brief          check if an mbox is valid/allocated
 *
 * @param[in]      mbox    pointer to the mailbox
 *
 * @retval         0       invalid
 * @retval         1       valid
 *
 * @note           None
 */
int tls_mbox_valid(tls_mbox_t mbox)
{
    if (mbox == NULL)
        return 0;
    else
        return 1;
}
#endif

/**
 * @brief          sends a message to a mailbox
 *
 * @param[in]      mbox    pointer to the mailbox
 * @param[in]      *msg    pointer to the message to be post
 *
 * @return         None
 *
 * @note           None
 */
void tls_mbox_post(tls_mbox_t mbox, void *msg)
{
    u8 err;
    u8 i=0;

    if(msg == NULL )
        msg = (void*)tls_null_pointer;

    /* try 10 times */
    while (i < 10)
    {
	    err = tls_os_queue_send(mbox, msg, 0);
        if(err == TLS_OS_SUCCESS)
            break;
        i++;
        tls_os_time_delay(5);
    }
}

/**
 * @brief          posts the "msg" to the mailbox.
 *
 * @param[in]      mbox               pointer to the mailbox
 * @param[in]      *msg               pointer to the message to be post
 *
 * @retval         TLS_OS_SUCCESS     success
 * @retval         TLS_OS_ERROR       failed
 *
 * @note           this function have to block until the "msg" is really posted.
 */
s8 tls_mbox_trypost(tls_mbox_t mbox, void *msg)
{
    u8 err;

    if( msg == NULL )
    {
        msg = (void*)tls_null_pointer;
    }

	err = tls_os_queue_send(mbox, msg, 0);
    if(err == TLS_OS_SUCCESS)
        return TLS_OS_SUCCESS;

    return TLS_OS_ERROR;      
}

/**
 * @brief          waits for a message within the specified time
 *
 * @param[in]      mbox         pointer to the mailbox
 * @param[out]      **msg       pointer to the message to be received
 * @param[in]      timeout      the specified time
 *
 * @retval         SYS_ARCH_TIMEOUT     time out
 * @retval         other                time of elapsed
 *
 * @note           None
 */
u32 tls_arch_mbox_fetch(tls_mbox_t mbox, void **msg, u32 timeout)
{
    u8        err;
    u32         ucos_timeout = 0;
    u32         in_timeout = timeout * HZ / 1000;
    u32       tick_start, tick_stop, tick_elapsed;

    if(timeout && in_timeout == 0)
        in_timeout = 1;

    tick_start = tls_os_get_time();
//    *msg = OSQPend(mbox, in_timeout, &err);
	err = tls_os_queue_receive(mbox,msg,0,in_timeout);
//    if (err == OS_ERR_TIMEOUT ) {
	if (err != TLS_OS_SUCCESS ) {
        ucos_timeout = SYS_ARCH_TIMEOUT;
        return ucos_timeout;
    }
    tick_stop = tls_os_get_time();

    // Take care of wrap-around.
    if( tick_stop >= tick_start )
        tick_elapsed = tick_stop - tick_start;
    else
        tick_elapsed = 0xFFFFFFFF - tick_start + tick_stop ;


    return tick_elapsed * 1000/HZ;
}

