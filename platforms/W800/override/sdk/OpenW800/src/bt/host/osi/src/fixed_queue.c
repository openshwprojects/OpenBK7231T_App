/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
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

#include <assert.h>
#include <string.h>

#include "gki.h"
#include "osi/include/fixed_queue.h"
#include "osi/include/list.h"
#include "osi/include/osi.h"

typedef struct fixed_queue_t {
    list_t *list;
    size_t capacity;

} fixed_queue_t;


fixed_queue_t *fixed_queue_new(size_t capacity)
{
    fixed_queue_t *ret = GKI_getbuf(sizeof(fixed_queue_t));
    memset(ret, 0, sizeof(fixed_queue_t));
    ret->capacity = capacity;
    ret->list = list_new(NULL);

    if(!ret->list) {
        goto error;
    }

    return ret;
error:
    fixed_queue_free(ret, NULL);
    return NULL;
}

void fixed_queue_free(fixed_queue_t *queue, fixed_queue_free_cb free_cb)
{
    if(!queue) {
        return;
    }

    if(free_cb)
        for(const list_node_t *node = list_begin(queue->list); node != list_end(queue->list);
                node = list_next(node)) {
            free_cb(list_node(node));
        }

    list_free(queue->list);
    GKI_freebuf(queue);
}

uint8_t fixed_queue_is_empty(fixed_queue_t *queue)
{
    if(queue == NULL) {
        return true;
    }

    uint8_t is_empty = list_is_empty(queue->list);
    return is_empty;
}

size_t fixed_queue_length(fixed_queue_t *queue)
{
    if(queue == NULL) {
        return 0;
    }

    size_t length = list_length(queue->list);
    return length;
}

size_t fixed_queue_capacity(fixed_queue_t *queue)
{
    assert(queue != NULL);
    return queue->capacity;
}

void fixed_queue_enqueue(fixed_queue_t *queue, void *data)
{
    assert(queue != NULL);
    assert(data != NULL);
    list_append(queue->list, data);
}

void *fixed_queue_dequeue(fixed_queue_t *queue)
{
    assert(queue != NULL);
    void *ret = list_front(queue->list);
    list_remove(queue->list, ret);
    return ret;
}

uint8_t fixed_queue_try_enqueue(fixed_queue_t *queue, void *data)
{
    assert(queue != NULL);
    assert(data != NULL);
    list_append(queue->list, data);
    return true;
}

void *fixed_queue_try_dequeue(fixed_queue_t *queue)
{
    if(queue == NULL) {
        return NULL;
    }

    if(list_is_empty(queue->list)) {
        return NULL;
    }

    void *ret = list_front(queue->list);
    list_remove(queue->list, ret);
    return ret;
}

void *fixed_queue_try_peek_first(fixed_queue_t *queue)
{
    if(queue == NULL) {
        return NULL;
    }

    void *ret = list_is_empty(queue->list) ? NULL : list_front(queue->list);
    return ret;
}

void *fixed_queue_try_peek_last(fixed_queue_t *queue)
{
    if(queue == NULL) {
        return NULL;
    }

    void *ret = list_is_empty(queue->list) ? NULL : list_back(queue->list);
    return ret;
}

void *fixed_queue_try_remove_from_queue(fixed_queue_t *queue, void *data)
{
    if(queue == NULL) {
        return NULL;
    }

    uint8_t removed = false;

    if(list_contains(queue->list, data)) {
        removed = list_remove(queue->list, data);
        assert(removed);
    }

    if(removed) {
        return data;
    }

    return NULL;
}

list_t *fixed_queue_get_list(fixed_queue_t *queue)
{
    assert(queue != NULL);
    // NOTE: This function is not thread safe, and there is no point for
    // calling pthread_mutex_lock() / pthread_mutex_unlock()
    return queue->list;
}


