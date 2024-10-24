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

#ifndef BTIF_HH_H
#define BTIF_HH_H

#include "bluetooth.h"
#include "bt_hh.h"
#include <stdint.h>
#include "bta_hh_api.h"
#include "btu.h"


/*******************************************************************************
**  Constants & Macros
********************************************************************************/

#define BTIF_HH_MAX_HID         8
#define BTIF_HH_MAX_ADDED_DEV   32

#define BTIF_HH_MAX_KEYSTATES            3
#define BTIF_HH_KEYSTATE_MASK_NUMLOCK    0x01
#define BTIF_HH_KEYSTATE_MASK_CAPSLOCK   0x02
#define BTIF_HH_KEYSTATE_MASK_SCROLLLOCK 0x04


/*******************************************************************************
**  Type definitions and return values
********************************************************************************/

typedef enum {
    BTIF_HH_DISABLED =   0,
    BTIF_HH_ENABLED,
    BTIF_HH_DISABLING,
    BTIF_HH_DEV_UNKNOWN,
    BTIF_HH_DEV_CONNECTING,
    BTIF_HH_DEV_CONNECTED,
    BTIF_HH_DEV_DISCONNECTED
} BTIF_HH_STATUS;

typedef struct {
    bthh_connection_state_t       dev_status;
    uint8_t                         dev_handle;
    tls_bt_addr_t                   bd_addr;
    tBTA_HH_ATTR_MASK             attr_mask;
    uint8_t                         sub_class;
    uint8_t                         app_id;
    int                           fd;
    uint8_t                       ready_for_data;
    uint32_t                     hh_poll_thread_id;
    uint8_t                         hh_keep_polling;
    TIMER_LIST_ENT  vup_timer;
    uint8_t                       local_vup; // Indicated locally initiated VUP
} btif_hh_device_t;

/* Control block to maintain properties of devices */
typedef struct {
    uint8_t             dev_handle;
    tls_bt_addr_t       bd_addr;
    tBTA_HH_ATTR_MASK attr_mask;
} btif_hh_added_device_t;

/**
 * BTIF-HH control block to maintain added devices and currently
 * connected hid devices
 */
typedef struct {
    BTIF_HH_STATUS          status;
    btif_hh_device_t        devices[BTIF_HH_MAX_HID];
    uint32_t                  device_num;
    btif_hh_added_device_t  added_devices[BTIF_HH_MAX_ADDED_DEV];
    btif_hh_device_t        *p_curr_dev;
} btif_hh_cb_t;


/*******************************************************************************
**  Functions
********************************************************************************/

extern btif_hh_cb_t btif_hh_cb;

extern btif_hh_device_t *btif_hh_find_connected_dev_by_handle(uint8_t handle);
extern void btif_hh_remove_device(tls_bt_addr_t bd_addr);
uint8_t btif_hh_add_added_dev(tls_bt_addr_t bda, tBTA_HH_ATTR_MASK attr_mask);
extern tls_bt_status_t btif_hh_virtual_unplug(tls_bt_addr_t *bd_addr);
extern void btif_hh_disconnect(tls_bt_addr_t *bd_addr);
extern void btif_hh_setreport(btif_hh_device_t *p_dev, bthh_report_type_t r_type,
                              uint16_t size, uint8_t *report);

uint8_t btif_hh_add_added_dev(tls_bt_addr_t bd_addr, tBTA_HH_ATTR_MASK attr_mask);

#endif
