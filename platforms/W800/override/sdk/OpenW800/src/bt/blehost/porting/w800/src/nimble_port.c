/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stddef.h>
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#if NIMBLE_CFG_CONTROLLER
#include "controller/ble_ll.h"
#endif
#if MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST)
#include "store/config/ble_store_config.h"
#else
#include "store/ram/ble_store_ram.h"
#endif


static struct ble_npl_eventq g_eventq_dflt ;
static struct ble_hs_stop_listener stop_listener;
static struct ble_npl_sem ble_hs_stop_sem;
static struct ble_npl_event ble_hs_ev_stop;

#define NIMBLE_PORT_DEINIT_EV_ARG (-1)


void
nimble_port_init(void)
{
#if NIMBLE_CFG_CONTROLLER
    void ble_hci_ram_init(void);
#endif
    /* Initialize default event queue */
    ble_npl_eventq_init(&g_eventq_dflt);
    os_mempool_reset();
    os_msys_init();
    ble_hs_init();
    /* XXX Need to have template for store */
#if MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST)
    ble_store_config_init();
#else
    ble_store_ram_init();
#endif
#if NIMBLE_CFG_CONTROLLER
    hal_timer_init(5, NULL);
    os_cputime_init(32768);
    ble_ll_init();
    ble_hci_ram_init();
#endif
}

void
nimble_port_deinit(void)
{
    /* Deinitialize default event queue */
    ble_npl_eventq_deinit(&g_eventq_dflt);
    ble_hs_deinit();
    os_msys_deinit();
#if MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST)
    ble_store_config_deinit();
#endif
}


void
nimble_port_run(void)
{
    struct ble_npl_event *ev;
    int arg;

    while(1) {
        ev = ble_npl_eventq_get(&g_eventq_dflt, BLE_NPL_TIME_FOREVER);
        ble_npl_event_run(ev);
        arg = (int)ble_npl_event_get_arg(ev);

        if(arg == NIMBLE_PORT_DEINIT_EV_ARG) {
            ;//break;
        }
    }
}

/**
 * Called when the host stop procedure has completed.
 */
static void
ble_hs_stop_cb(int status, void *arg)
{
    ble_npl_sem_release(&ble_hs_stop_sem);
}

static void
nimble_port_stop_cb(struct ble_npl_event *ev)
{
    ble_npl_sem_release(&ble_hs_stop_sem);
}

int
nimble_port_stop(void)
{
    int rc;
    ble_npl_sem_init(&ble_hs_stop_sem, 0);
    /* Initiate a host stop procedure. */
    rc = ble_hs_stop(&stop_listener, ble_hs_stop_cb,
                     NULL);

    if(rc != 0) {
        ble_npl_sem_deinit(&ble_hs_stop_sem);
        return rc;
    }

    /* Wait till the host stop procedure is complete */
    ble_npl_sem_pend(&ble_hs_stop_sem, BLE_NPL_TIME_FOREVER);
    ble_npl_event_init(&ble_hs_ev_stop, nimble_port_stop_cb,
                       (void *)NIMBLE_PORT_DEINIT_EV_ARG);
    ble_npl_eventq_put(&g_eventq_dflt, &ble_hs_ev_stop);
    /* Wait till the event is serviced */
    ble_npl_sem_pend(&ble_hs_stop_sem, BLE_NPL_TIME_FOREVER);
    ble_npl_sem_deinit(&ble_hs_stop_sem);

    /*Adding shutdown cb function, inform application free resources*/
    if(ble_hs_cfg.shutdown_cb != NULL) {
        ble_hs_cfg.shutdown_cb(0);
    }

    return rc;
}



struct ble_npl_eventq *
nimble_port_get_dflt_eventq(void)
{
    return &g_eventq_dflt;
}

#if NIMBLE_CFG_CONTROLLER
void
nimble_port_ll_task_func(void *arg)
{
    extern void ble_ll_task(void *);
    ble_ll_task(arg);
}
#endif
