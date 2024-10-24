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

/************************************************************************************
 *
 *  Filename:      bluetooth.c
 *
 *  Description:   Bluetooth HAL implementation
 *
 ***********************************************************************************/

#define LOG_TAG "bt_btif"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bluetooth.h"
#include "btif_api.h"
#include "bt_utils.h"
#include "gki.h"

#include "wm_osal.h"
#include "wm_bt.h"

/************************************************************************************
**  Static variables
************************************************************************************/

tls_bt_host_callback_t bt_host_cb = NULL;
tls_bt_log_level_t tls_bt_log_level = TLS_BT_LOG_NONE;


/************************************************************************************
**  Externs
************************************************************************************/


/************************************************************************************
**  Functions
************************************************************************************/

static uint8_t interface_ready(void)
{
    return bt_host_cb != NULL;
}


/*****************************************************************************
**
**   BLUETOOTH HAL INTERFACE FUNCTIONS
**
*****************************************************************************/


tls_os_timer_t *bdroid_timer = NULL;
static tls_os_queue_t *host_msg_queue = NULL;
#define HOST_QUEUE_SIZE 120

void bdroid_timer_cb(void *ptmr, void *parg)
{
    GKI_timer_update(1);
}
void active_host_task(uint32_t event)
{
    if(host_msg_queue == NULL) {
        return;
    }

    tls_os_queue_send(host_msg_queue, (void *)event, 4);
}

void bt_host_task()
{
    tls_os_status_t err;
    void *msg;
    tls_os_timer_start(bdroid_timer);

    while(1) {
        err = tls_os_queue_receive(host_msg_queue, (void **)&msg, 0, 0);

        if(err != TLS_OS_SUCCESS) { continue; }

        GKI_event_process((u32)msg);
    }
}
#define   TSK_PRIOR_BT_HOST                   8
#define   APP_BT_HOST_TASK_STK_SIZE           1560

static tls_os_task_t tls_host_task_hdl = (void *)NULL;
static void *tls_host_task_stack_ptr = NULL;

tls_os_status_t wm_bt_host_init(void)
{
    tls_os_status_t err;
    err = tls_os_queue_create(&host_msg_queue, HOST_QUEUE_SIZE);

    if(err != TLS_OS_SUCCESS) {
        assert(0);
    }

    err = tls_os_timer_create(&bdroid_timer,
                              bdroid_timer_cb,
                              (void *)NULL,
                              HZ / 100,
                              TRUE,
                              NULL);

    if(err != TLS_OS_SUCCESS) {
        assert(0);
    }

    btif_init_bluetooth();
    btif_enable_bluetooth();
    tls_host_task_stack_ptr = (void *)GKI_os_malloc(APP_BT_HOST_TASK_STK_SIZE * sizeof(uint32_t));
    assert(tls_host_task_stack_ptr != NULL);
    return tls_os_task_create(&tls_host_task_hdl, "BTH",
                              bt_host_task,
                              (void *)0,
                              (void *)tls_host_task_stack_ptr,
                              APP_BT_HOST_TASK_STK_SIZE * sizeof(uint32_t),
                              TSK_PRIOR_BT_HOST,
                              0);
}
tls_bt_status_t tls_bt_host_enable(tls_bt_host_callback_t callback, tls_bt_log_level_t log_level)
{
    tls_bt_status_t status = TLS_BT_STATUS_DONE;
    tls_bt_log_level = log_level;

    if(bt_host_cb == NULL) {
        bt_host_cb = callback;
        /* init btif */
        status = wm_bt_host_init();
        return status;
    } else {
        printf("warn, bt host enabled already\r\n");
    }

    return status;
}
tls_bt_status_t tls_bt_host_disable()
{
    if(!interface_ready()) {
        /*we give a success return, also means the host disabled*/
        return TLS_BT_STATUS_SUCCESS;
    }

    return btif_disable_bluetooth();
}
void free_host_task_stack()
{
    if(tls_host_task_stack_ptr) {
        GKI_os_free(tls_host_task_stack_ptr);
        tls_host_task_stack_ptr = NULL;
    }
}
tls_bt_status_t tls_bt_host_cleanup()
{
    tls_bt_status_t status = btif_cleanup_bluetooth();

    while(status != TLS_BT_STATUS_SUCCESS) {
        printf("status=%d\r\n", status);
        tls_os_time_delay(100);
        status = btif_cleanup_bluetooth();
    }

    if(bdroid_timer) {
        tls_os_timer_stop(bdroid_timer);
        tls_os_timer_delete(bdroid_timer);
        bdroid_timer = NULL;
    }

    if(tls_host_task_hdl) {
        tls_os_task_del_by_task_handle(tls_host_task_hdl, free_host_task_stack);
    }

    if(host_msg_queue) {
        tls_os_queue_delete(host_msg_queue);
        host_msg_queue = NULL;
    }

    bt_host_cb = NULL;
    status = tls_bt_ctrl_disable();

    if((status != TLS_BT_STATUS_SUCCESS) && (status != TLS_BT_STATUS_DONE)) {
        return TLS_BT_STATUS_CTRL_DISABLE_FAILED;
    }

    return status;
}
tls_bt_status_t tls_bt_pin_reply(const tls_bt_addr_t *bd_addr, uint8_t accept,
                                 uint8_t pin_len, tls_bt_pin_code_t *pin_code)
{
    return btif_dm_pin_reply(bd_addr, accept, pin_len, pin_code);
}
tls_bt_status_t tls_bt_ssp_reply(const tls_bt_addr_t *bd_addr, tls_bt_ssp_variant_t variant,
                                 uint8_t accept, uint32_t passkey)
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_dm_ssp_reply(bd_addr, variant, accept, passkey);
}
tls_bt_status_t tls_bt_set_adapter_property(const tls_bt_property_t *property,
        uint8_t update_to_flash)
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_set_adapter_property(property, update_to_flash);
}
tls_bt_status_t tls_bt_get_adapter_property(tls_bt_property_type_t type)
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_get_adapter_property(type);
}

tls_bt_status_t tls_bt_start_discovery()
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_dm_start_discovery();
}
tls_bt_status_t tls_bt_cancel_discovery()
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_dm_cancel_discovery();
}
tls_bt_status_t tls_bt_create_bond(const tls_bt_addr_t *bd_addr, int transport)
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_dm_create_bond(bd_addr, transport);
}
tls_bt_status_t tls_bt_cancel_bond(const tls_bt_addr_t *bd_addr)
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_dm_cancel_bond(bd_addr);
}
tls_bt_status_t tls_bt_remove_bond(const tls_bt_addr_t *bd_addr)
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_dm_remove_bond(bd_addr);
}
tls_bt_status_t tls_bt_get_adapter_properties(void)
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_get_adapter_properties();
}


tls_bt_status_t tls_bt_get_remote_device_properties(tls_bt_addr_t *remote_addr)
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_get_remote_device_properties(remote_addr);
}

tls_bt_status_t tls_bt_get_remote_device_property(tls_bt_addr_t *remote_addr,
        tls_bt_property_type_t type)
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_get_remote_device_property(remote_addr, type);
}

tls_bt_status_t tls_bt_set_remote_device_property(tls_bt_addr_t *remote_addr,
        const tls_bt_property_t *property)
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_set_remote_device_property(remote_addr, property);
}

tls_bt_status_t tls_bt_get_remote_service_record(tls_bt_addr_t *remote_addr, tls_bt_uuid_t *uuid)
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_get_remote_service_record(remote_addr, uuid);
}

tls_bt_status_t tls_bt_get_remote_services(tls_bt_addr_t *remote_addr)
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_dm_get_remote_services(remote_addr);
}

tls_bt_status_t tls_bt_create_bond_out_of_band(const tls_bt_addr_t *bd_addr, int transport,
        const bt_out_of_band_data_t *oob_data)
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_dm_create_bond_out_of_band(bd_addr, transport, oob_data);
}



tls_bt_status_t tls_bt_get_connection_state(const tls_bt_addr_t *bd_addr)
{
    /* sanity check */
    if(interface_ready() == FALSE) {
        return 0;
    }

    return btif_dm_get_connection_state(bd_addr);
}

tls_bt_status_t tls_bt_enable(tls_bt_host_callback_t callback, tls_bt_hci_if_t *p_hci_if,
                              tls_bt_log_level_t log_level)
{
    tls_bt_status_t status;
    status = tls_bt_ctrl_enable(p_hci_if, log_level);

    if((status != TLS_BT_STATUS_SUCCESS) && (status != TLS_BT_STATUS_DONE)) {
        return TLS_BT_STATUS_CTRL_ENABLE_FAILED;
    }

    status = tls_bt_host_enable(callback, log_level);

    if((status != TLS_BT_STATUS_SUCCESS) && (status != TLS_BT_STATUS_DONE)) {
        /*disable already enabled controller*/
        tls_bt_ctrl_disable();
        return TLS_BT_STATUS_HOST_ENABLE_FAILED;
    }

    return status;
}
tls_bt_status_t tls_bt_disable()
{
    tls_bt_status_t status;
    status = tls_bt_host_disable();

    if(status != TLS_BT_STATUS_SUCCESS) {
        return TLS_BT_STATUS_HOST_DISABLE_FAILED;
    }

    return status;
}


