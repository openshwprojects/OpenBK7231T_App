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

#ifndef BT_UTILS_QUEUE_H
#define BT_UTILS_QUEUE_H


/******************************************************************************
**  Type definitions
******************************************************************************/

typedef struct {
    void        *p_first;
    void        *p_last;
    uint16_t    count;
} BUFFER_Q;

/******************************************************************************
**  Extern variables and functions
******************************************************************************/

/******************************************************************************
**  Functions
******************************************************************************/

/*******************************************************************************
**
** Function        utils_init
**
** Description     Utils initialization
**
** Returns         None
**
*******************************************************************************/
void utils_init();

/*******************************************************************************
**
** Function        utils_cleanup
**
** Description     Utils cleanup
**
** Returns         None
**
*******************************************************************************/
void utils_cleanup();

/*******************************************************************************
**
** Function        utils_queue_init
**
** Description     Initialize the given buffer queue
**
** Returns         None
**
*******************************************************************************/
void utils_queue_init(BUFFER_Q *p_q);

/*******************************************************************************
**
** Function        utils_enqueue
**
** Description     Enqueue a buffer at the tail of the given queue
**
** Returns         None
**
*******************************************************************************/
void utils_enqueue(BUFFER_Q *p_q, void *p_buf);

/*******************************************************************************
**
** Function        utils_dequeue
**
** Description     Dequeues a buffer from the head of the given queue
**
** Returns         NULL if queue is empty, else buffer
**
*******************************************************************************/
void *utils_dequeue(BUFFER_Q *p_q);

/*******************************************************************************
**
** Function        utils_dequeue_unlocked
**
** Description     Dequeues a buffer from the head of the given queue without lock
**
** Returns         NULL if queue is empty, else buffer
**
*******************************************************************************/
void *utils_dequeue_unlocked(BUFFER_Q *p_q);

/*******************************************************************************
**
** Function        utils_getnext
**
** Description     Return a pointer to the next buffer linked to the given buffer
**
** Returns         NULL if the given buffer does not point to any next buffer,
**                 else next buffer address
**
*******************************************************************************/
void *utils_getnext(void *p_buf);

/*******************************************************************************
**
** Function        utils_remove_from_queue
**
** Description     Dequeue the given buffer from the middle of the given queue
**
** Returns         NULL if the given queue is empty, else the given buffer
**
*******************************************************************************/
void *utils_remove_from_queue(BUFFER_Q *p_q, void *p_buf);

/*******************************************************************************
**
** Function        utils_remove_from_queue_unlocked
**
** Description     Dequeue the given buffer from the middle of the given queue without lock
**
** Returns         NULL if the given queue is empty, else the given buffer
**
*******************************************************************************/
void *utils_remove_from_queue_unlocked(BUFFER_Q *p_q, void *p_buf);


/*******************************************************************************
**
** Function        utils_delay
**
** Description     sleep unconditionally for timeout milliseconds
**
** Returns         None
**
*******************************************************************************/
void utils_delay(uint32_t timeout);

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
void utils_lock(void);

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
void utils_unlock(void);


#endif /* BT_UTILS_QUEUE_H */

