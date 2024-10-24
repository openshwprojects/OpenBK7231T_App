/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
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

/******************************************************************************
 *
 *  Filename:      utils.c
 *
 *  Description:   Contains helper functions
 *
 ******************************************************************************/

#include "bt_hci_bdroid.h"
#include "bt_buffer_q.h"

/******************************************************************************
**  Static variables
******************************************************************************/



/*******************************************************************************
**
** Function        utils_queue_init
**
** Description     Initialize the given buffer queue
**
** Returns         None
**
*******************************************************************************/
void utils_queue_init(BUFFER_Q *p_q)
{
    p_q->p_first = p_q->p_last = NULL;
    p_q->count = 0;
}

/*******************************************************************************
**
** Function        utils_enqueue
**
** Description     Enqueue a buffer at the tail of the given queue
**
** Returns         None
**
*******************************************************************************/
void utils_enqueue(BUFFER_Q *p_q, void *p_buf)
{
    HC_BUFFER_HDR_T    *p_hdr;
    p_hdr = (HC_BUFFER_HDR_T *)((uint8_t *) p_buf - BT_HC_BUFFER_HDR_SIZE);

    //pthread_mutex_lock(&utils_mutex);

    if(p_q->p_last) {
        HC_BUFFER_HDR_T *p_last_hdr = \
                                      (HC_BUFFER_HDR_T *)((uint8_t *)p_q->p_last - BT_HC_BUFFER_HDR_SIZE);
        p_last_hdr->p_next = p_hdr;
    } else {
        p_q->p_first = p_buf;
    }

    p_q->p_last = p_buf;
    p_q->count++;
    p_hdr->p_next = NULL;
    //pthread_mutex_unlock(&utils_mutex);
}
/*******************************************************************************
**
** Function        utils_dequeue
**
** Description     Dequeues a buffer from the head of the given queue
**
** Returns         NULL if queue is empty, else buffer
**
*******************************************************************************/
void *utils_dequeue(BUFFER_Q *p_q)
{
    //pthread_mutex_lock(&utils_mutex);
    void *p_buf =  utils_dequeue_unlocked(p_q);
    //pthread_mutex_unlock(&utils_mutex);
    return p_buf;
}

/*******************************************************************************
**
** Function        utils_dequeue_unlocked
**
** Description     Dequeues a buffer from the head of the given queue without lock
**
** Returns         NULL if queue is empty, else buffer
**
*******************************************************************************/
void *utils_dequeue_unlocked(BUFFER_Q *p_q)
{
    HC_BUFFER_HDR_T    *p_hdr;

    if(!p_q || !p_q->count) {
        return (NULL);
    }

    p_hdr = (HC_BUFFER_HDR_T *)((uint8_t *)p_q->p_first - BT_HC_BUFFER_HDR_SIZE);

    if(p_hdr->p_next) {
        p_q->p_first = ((uint8_t *)p_hdr->p_next + BT_HC_BUFFER_HDR_SIZE);
    } else {
        p_q->p_first = NULL;
        p_q->p_last  = NULL;
    }

    p_q->count--;
    p_hdr->p_next = NULL;
    return ((uint8_t *)p_hdr + BT_HC_BUFFER_HDR_SIZE);
}

/*******************************************************************************
**
** Function        utils_getnext
**
** Description     Return a pointer to the next buffer linked to the given
**                 buffer
**
** Returns         NULL if the given buffer does not point to any next buffer,
**                 else next buffer address
**
*******************************************************************************/
void *utils_getnext(void *p_buf)
{
    HC_BUFFER_HDR_T    *p_hdr;
    p_hdr = (HC_BUFFER_HDR_T *)((uint8_t *) p_buf - BT_HC_BUFFER_HDR_SIZE);

    if(p_hdr->p_next) {
        return ((uint8_t *)p_hdr->p_next + BT_HC_BUFFER_HDR_SIZE);
    } else {
        return (NULL);
    }
}

/*******************************************************************************
**
** Function        utils_remove_from_queue
**
** Description     Dequeue the given buffer from the middle of the given queue
**
** Returns         NULL if the given queue is empty, else the given buffer
**
*******************************************************************************/
void *utils_remove_from_queue(BUFFER_Q *p_q, void *p_buf)
{
    //pthread_mutex_lock(&utils_mutex);
    p_buf = utils_remove_from_queue_unlocked(p_q, p_buf);
    //pthread_mutex_unlock(&utils_mutex);
    return p_buf;
}
/*******************************************************************************
**
** Function        utils_remove_from_queue_unlocked
**
** Description     Dequeue the given buffer from the middle of the given queue
**
** Returns         NULL if the given queue is empty, else the given buffer
**
*******************************************************************************/
void *utils_remove_from_queue_unlocked(BUFFER_Q *p_q, void *p_buf)
{
    HC_BUFFER_HDR_T    *p_prev;
    HC_BUFFER_HDR_T    *p_buf_hdr;

    if(p_buf == p_q->p_first) {
        return (utils_dequeue_unlocked(p_q));
    }

    p_buf_hdr = (HC_BUFFER_HDR_T *)((uint8_t *)p_buf - BT_HC_BUFFER_HDR_SIZE);
    p_prev = (HC_BUFFER_HDR_T *)((uint8_t *)p_q->p_first - BT_HC_BUFFER_HDR_SIZE);

    for(; p_prev; p_prev = p_prev->p_next) {
        /* If the previous points to this one, move the pointers around */
        if(p_prev->p_next == p_buf_hdr) {
            p_prev->p_next = p_buf_hdr->p_next;

            /* If we are removing the last guy in the queue, update p_last */
            if(p_buf == p_q->p_last) {
                p_q->p_last = p_prev + 1;
            }

            /* One less in the queue */
            p_q->count--;
            /* The buffer is now unlinked */
            p_buf_hdr->p_next = NULL;
            return (p_buf);
        }
    }

    return (NULL);
}

/*******************************************************************************
**
** Function        utils_delay
**
** Description     sleep unconditionally for timeout milliseconds
**
** Returns         None
**
*******************************************************************************/
void utils_delay(uint32_t timeout)
{
    //UTIL_delay_msec(timeout, 1);
#if 0
    struct timespec delay;
    int err;
    delay.tv_sec = timeout / 1000;
    delay.tv_nsec = 1000 * 1000 * (timeout % 1000);

    /* [u]sleep can't be used because it uses SIGALRM */
    do {
        err = nanosleep(&delay, &delay);
    } while(err < 0 && errno == EINTR);

#endif
}

/*******************************************************************************
**
** Function        utils_lock
**
** Description     application calls this function before entering critical
**                 section
**
** Returns         None
**
*******************************************************************************/
void utils_lock(void)
{
    //pthread_mutex_lock(&utils_mutex);
}

/*******************************************************************************
**
** Function        utils_unlock
**
** Description     application calls this function when leaving critical
**                 section
**
** Returns         None
**
*******************************************************************************/
void utils_unlock(void)
{
    // pthread_mutex_unlock(&utils_mutex);
}

