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

/*******************************************************************************
 *
 *  Filename:      btif_profile_queue.c
 *
 *  Description:   Bluetooth remote device connection queuing implementation.
 *
 ******************************************************************************/

#define LOG_TAG "bt_btif_queue"

#include "btif_profile_queue.h"

#include <assert.h>
#include <string.h>

#include "btif_common.h"
#include "bt_common.h"
#include "gki.h"
#include "osi/include/list.h"

/*******************************************************************************
**  Local type definitions
*******************************************************************************/

typedef enum {
    BTIF_QUEUE_CONNECT_EVT,
    BTIF_QUEUE_ADVANCE_EVT,
} btif_queue_event_t;

typedef struct {
    tls_bt_addr_t bda;
    uint16_t uuid;
    uint8_t busy;
    btif_connect_cb_t connect_cb;
} connect_node_t;

/*******************************************************************************
**  Static variables
*******************************************************************************/

static list_t *connect_queue;

static const size_t MAX_REASONABLE_REQUESTS = 10;

/*******************************************************************************
**  Queue helper functions
*******************************************************************************/

static void queue_int_add(connect_node_t *p_param)
{
    if(!connect_queue) {
        connect_queue = list_new(GKI_freebuf);
        assert(connect_queue != NULL);
    }

    // Sanity check to make sure we're not leaking connection requests
    assert(list_length(connect_queue) < MAX_REASONABLE_REQUESTS);

    for(const list_node_t *node = list_begin(connect_queue); node != list_end(connect_queue);
            node = list_next(node)) {
        if(((connect_node_t *)list_node(node))->uuid == p_param->uuid) {
            LOG_INFO(LOG_TAG, "%s dropping duplicate connect request for uuid: %04x", __func__, p_param->uuid);
            return;
        }
    }

    connect_node_t *p_node = GKI_getbuf(sizeof(connect_node_t));
    wm_memcpy(p_node, p_param, sizeof(connect_node_t));
    list_append(connect_queue, p_node);
}

static void queue_int_advance()
{
    if(connect_queue && !list_is_empty(connect_queue)) {
        list_remove(connect_queue, list_front(connect_queue));
    }
}

static void queue_int_handle_evt(uint16_t event, char *p_param)
{
    switch(event) {
        case BTIF_QUEUE_CONNECT_EVT:
            queue_int_add((connect_node_t *)p_param);
            break;

        case BTIF_QUEUE_ADVANCE_EVT:
            queue_int_advance();
            break;
    }

    btif_queue_connect_next();
}

/*******************************************************************************
**
** Function         btif_queue_connect
**
** Description      Add a new connection to the queue and trigger the next
**                  scheduled connection.
**
** Returns          BT_STATUS_SUCCESS if successful
**
*******************************************************************************/
tls_bt_status_t btif_queue_connect(uint16_t uuid, const tls_bt_addr_t *bda,
                                   btif_connect_cb_t connect_cb)
{
    connect_node_t node;
    wm_memset(&node, 0, sizeof(connect_node_t));
    wm_memcpy(&node.bda, bda, sizeof(tls_bt_addr_t));
    node.uuid = uuid;
    node.connect_cb = connect_cb;
    return btif_transfer_context(queue_int_handle_evt, BTIF_QUEUE_CONNECT_EVT,
                                 (char *)&node, sizeof(connect_node_t), NULL);
}

/*******************************************************************************
**
** Function         btif_queue_advance
**
** Description      Clear the queue's busy status and advance to the next
**                  scheduled connection.
**
** Returns          void
**
*******************************************************************************/
void btif_queue_advance()
{
    btif_transfer_context(queue_int_handle_evt, BTIF_QUEUE_ADVANCE_EVT,
                          NULL, 0, NULL);
}

// This function dispatches the next pending connect request. It is called from
// stack_manager when the stack comes up.
tls_bt_status_t btif_queue_connect_next(void)
{
    if(!connect_queue || list_is_empty(connect_queue)) {
        return TLS_BT_STATUS_FAIL;
    }

    connect_node_t *p_head = list_front(connect_queue);

    // If the queue is currently busy, we return success anyway,
    // since the connection has been queued...
    if(p_head->busy) {
        return TLS_BT_STATUS_SUCCESS;
    }

    p_head->busy = true;
    return p_head->connect_cb(&p_head->bda, p_head->uuid);
}

/*******************************************************************************
**
** Function         btif_queue_release
**
** Description      Free up all the queue nodes and set the queue head to NULL
**
** Returns          void
**
*******************************************************************************/
void btif_queue_release()
{
    list_free(connect_queue);
    connect_queue = NULL;
}
