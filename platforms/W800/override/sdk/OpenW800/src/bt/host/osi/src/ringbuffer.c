/******************************************************************************
 *
 *  Copyright (C) 2015 Google Inc.
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
#include <stdlib.h>

#include "osi/include/allocator.h"
#include "osi/include/ringbuffer.h"

struct ringbuffer_t {
    size_t total;
    size_t available;
    uint8_t *base;
    uint8_t *head;
    uint8_t *tail;
};

ringbuffer_t *ringbuffer_init(const size_t size)
{
    ringbuffer_t *p = GKI_getbuf(sizeof(ringbuffer_t));
    p->base = GKI_getbuf(size);
    p->head = p->tail = p->base;
    p->total = p->available = size;
    return p;
}

void ringbuffer_free(ringbuffer_t *rb)
{
    if(rb != NULL) {
        GKI_freebuf(rb->base);
    }

    GKI_freebuf(rb);
}

size_t ringbuffer_available(const ringbuffer_t *rb)
{
    assert(rb);
    return rb->available;
}

size_t ringbuffer_size(const ringbuffer_t *rb)
{
    assert(rb);
    return rb->total - rb->available;
}

size_t ringbuffer_insert(ringbuffer_t *rb, const uint8_t *p, size_t length)
{
    assert(rb);
    assert(p);

    if(length > ringbuffer_available(rb)) {
        length = ringbuffer_available(rb);
    }

    for(size_t i = 0; i != length; ++i) {
        *rb->tail++ = *p++;

        if(rb->tail >= (rb->base + rb->total)) {
            rb->tail = rb->base;
        }
    }

    rb->available -= length;
    return length;
}

size_t ringbuffer_delete(ringbuffer_t *rb, size_t length)
{
    assert(rb);

    if(length > ringbuffer_size(rb)) {
        length = ringbuffer_size(rb);
    }

    rb->head += length;

    if(rb->head >= (rb->base + rb->total)) {
        rb->head -= rb->total;
    }

    rb->available += length;
    return length;
}

size_t ringbuffer_peek(const ringbuffer_t *rb, int offset, uint8_t *p, size_t length)
{
    assert(rb);
    assert(p);
    assert(offset >= 0);
    assert((size_t)offset <= ringbuffer_size(rb));
    uint8_t *b = ((rb->head - rb->base + offset) % rb->total) + rb->base;
    const size_t bytes_to_copy = (offset + length > ringbuffer_size(rb)) ? ringbuffer_size(
            rb) - offset : length;

    for(size_t copied = 0; copied < bytes_to_copy; ++copied) {
        *p++ = *b++;

        if(b >= (rb->base + rb->total)) {
            b = rb->base;
        }
    }

    return bytes_to_copy;
}

size_t ringbuffer_pop(ringbuffer_t *rb, uint8_t *p, size_t length)
{
    assert(rb);
    assert(p);
    const size_t copied = ringbuffer_peek(rb, 0, p, length);
    rb->head += copied;

    if(rb->head >= (rb->base + rb->total)) {
        rb->head -= rb->total;
    }

    rb->available += copied;
    return copied;
}
