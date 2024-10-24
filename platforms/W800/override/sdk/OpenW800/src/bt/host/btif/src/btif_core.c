/******************************************************************************
 *
 *  Copyright (C) 2014 The Android Open Source Project
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
 *  Filename:      btif_core.c
 *
 *  Description:   Contains core functionality related to interfacing between
 *                 Bluetooth HAL and BTE core stack.
 *
 ***********************************************************************************/

#define LOG_TAG "bt_btif_core"

#include <ctype.h>
#include <fcntl.h>
#include "bluetooth.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>

#include "bdaddr.h"
#include "bt_utils.h"
#include "bta_api.h"
#include "bte.h"
#include "btif_api.h"
#include "btif_av.h"
#include "btif_config.h"
#include "btif_profile_queue.h"

#include "btif_storage.h"
#include "btif_uid.h"
#include "btif_util.h"
#include "btu.h"
#include "bt_common.h"
#include "gki.h"
#include "osi/include/fixed_queue.h"

#if defined(BTA_PAN_INCLUDED) && (BTA_PAN_INCLUDED == TRUE)
#include "btif_pan.h"
#endif

#if defined(BTA_SOCK_INCLUDED) && BTA_SOCK_INCLUDED == TRUE
#include "btif_sock.h"
#endif

/************************************************************************************
**  Constants & Macros
************************************************************************************/


#define BTIF_TASK_STR        ((int8_t *) "BTIF")
/************************************************************************************
**  Local type definitions
************************************************************************************/

/* These type definitions are used when passing data from the HAL to BTIF context
*  in the downstream path for the adapter and remote_device property APIs */

typedef struct {
    tls_bt_addr_t bd_addr;
    tls_bt_property_type_t type;
} btif_storage_read_t;

typedef struct {
    tls_bt_addr_t bd_addr;
    tls_bt_property_t prop;
} btif_storage_write_t;

typedef union {
    btif_storage_read_t read_req;
    btif_storage_write_t write_req;
} btif_storage_req_t;

typedef enum {
    BTIF_CORE_STATE_DISABLED = 0,
    BTIF_CORE_STATE_ENABLING,
    BTIF_CORE_STATE_ENABLED,
    BTIF_CORE_STATE_DISABLING
} btif_core_state_t;

/************************************************************************************
**  Static variables
************************************************************************************/

tls_bt_addr_t btif_local_bd_addr;

/* holds main adapter state */
static btif_core_state_t btif_core_state = BTIF_CORE_STATE_DISABLED;

static int btif_shutdown_pending = 0;
static tBTA_SERVICE_MASK btif_enabled_services = 0;

/*
* This variable should be set to 1, if the Bluedroid+BTIF libraries are to
* function in DUT mode.
*
* To set this, the btif_init_bluetooth needs to be called with argument as 1
*/
static uint8_t btif_dut_mode = 0;

/************************************************************************************
**  Static functions
************************************************************************************/
//static uint8_t btif_fetch_property(const char *key, tls_bt_addr_t *addr);

/* sends message to btif task */
static void btif_sendmsg(void *p_msg);

/************************************************************************************
**  Externs
************************************************************************************/

/** TODO: Move these to _common.h */
void bte_main_boot_entry(void);
void bte_main_enable();
void bte_main_disable(void);
void bte_main_cleanup(void);
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
void bte_main_enable_lpm(uint8_t enable);
#endif
void bte_main_postload_cfg(void);
void btif_dm_execute_service_request(uint16_t event, char *p_param);
#if defined(BTIF_DM_OOB_TEST) && (BTIF_DM_OOB_TEST == TRUE)
void btif_dm_load_local_oob(void);
#endif
void bte_main_config_hci_logging(uint8_t enable, uint8_t bt_disabled);

/*******************************************************************************
**
** Function         btif_context_switched
**
** Description      Callback used to execute transferred context callback
**
**                  p_msg : message to be executed in btif context
**
** Returns          void
**
*******************************************************************************/

static void btif_context_switched(void *p_msg)
{
    BTIF_TRACE_VERBOSE("btif_context_switched");
    tBTIF_CONTEXT_SWITCH_CBACK *p = (tBTIF_CONTEXT_SWITCH_CBACK *) p_msg;

    /* each callback knows how to parse the data */
    if(p->p_cb) {
        p->p_cb(p->event, p->p_param);
    }
}


/*******************************************************************************
**
** Function         btif_transfer_context
**
** Description      This function switches context to btif task
**
**                  p_cback   : callback used to process message in btif context
**                  event     : event id of message
**                  p_params  : parameter area passed to callback (copied)
**                  param_len : length of parameter area
**                  p_copy_cback : If set this function will be invoked for deep copy
**
** Returns          void
**
*******************************************************************************/

tls_bt_status_t btif_transfer_context(tBTIF_CBACK *p_cback, uint16_t event, char *p_params,
                                      int param_len, tBTIF_COPY_CBACK *p_copy_cback)
{
    tBTIF_CONTEXT_SWITCH_CBACK *p_msg =
                    (tBTIF_CONTEXT_SWITCH_CBACK *)GKI_getbuf(sizeof(tBTIF_CONTEXT_SWITCH_CBACK) + param_len);

    if(p_msg == NULL) {
        printf("Warning GKI_getbuf null...event=%d(0x%04x)\r\n", event, event);
        return TLS_BT_STATUS_NOMEM;
    }

    /* allocate and send message that will be executed in btif context */
    p_msg->hdr.event = BT_EVT_CONTEXT_SWITCH_EVT; /* internal event */
    p_msg->p_cb = p_cback;
    p_msg->event = event;                         /* callback event */

    /* check if caller has provided a copy callback to do the deep copy */
    if(p_copy_cback) {
        p_copy_cback(event, p_msg->p_param, p_params);
    } else if(p_params) {
        wm_memcpy(p_msg->p_param, p_params, param_len);   /* callback parameter data */
    }

    btif_sendmsg(p_msg);
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_is_dut_mode
**
** Description      checks if BTIF is currently in DUT mode
**
** Returns          1 if test mode, otherwize 0
**
*******************************************************************************/

uint8_t btif_is_dut_mode(void)
{
    return (btif_dut_mode == 1);
}

/*******************************************************************************
**
** Function         btif_is_enabled
**
** Description      checks if main adapter is fully enabled
**
** Returns          1 if fully enabled, otherwize 0
**
*******************************************************************************/

int btif_is_enabled(void)
{
    return ((!btif_is_dut_mode()) && (btif_core_state == BTIF_CORE_STATE_ENABLED));
}

void btif_init_ok(uint16_t event, char *p_param)
{
    BTIF_TRACE_DEBUG("btif_task: received trigger stack init event");
#if (BLE_INCLUDED == TRUE)
    btif_dm_load_ble_local_keys();
#endif
    BTA_EnableBluetooth(bte_dm_evt);
}


/*******************************************************************************
**
** Function         btif_task_prev
**
** Description      BTIF task handler managing all messages being passed
**                  Bluetooth HAL and BTA.
**
** Returns          void
**
*******************************************************************************/

static void btif_task_prev(uint32_t params)
{
    UNUSED(params);
    BTIF_TRACE_DEBUG("btif task starting");
    GKI_wait(0xFFFF, 0);
}

/*******************************************************************************
**
** Function         btif_task
**
** Description      BTIF task handler managing all messages being passed
**                  Bluetooth HAL and BTA.
**
** Returns          void
**
*******************************************************************************/

static void btif_task(uint32_t event)
{
    BT_HDR   *p_msg;
    tls_bt_host_msg_t msg;

    do {
        /*
         * Wait for the trigger to init chip and stack. This trigger will
         * be received by btu_task once the UART is opened and ready
         */
        if(event == BT_EVT_TRIGGER_STACK_INIT) {
#if (BLE_INCLUDED == TRUE)
            btif_dm_load_ble_local_keys();
#endif
            BTA_EnableBluetooth(bte_dm_evt);
        }

        /*
         * Failed to initialize controller hardware, reset state and bring
         * down all threads
         */
        if(event == BT_EVT_HARDWARE_INIT_FAIL) {
            BTIF_TRACE_EVENT("btif_task: hardware init failed\n");
            bte_main_disable();
            btif_queue_release();
            GKI_task_self_cleanup(BTIF_TASK);
            bte_main_cleanup();
            btif_dut_mode = 0;
            btif_core_state = BTIF_CORE_STATE_DISABLED;
            msg.adapter_state_change.status = WM_BT_STATE_OFF;

            if(bt_host_cb) { bt_host_cb(WM_BT_ADAPTER_STATE_CHG_EVT, &msg); }

            break;
        }

        if(event & EVENT_MASK(GKI_SHUTDOWN_EVT)) {
            break;
        }

        if(event & TASK_MBOX_1_EVT_MASK) {
            while((p_msg = GKI_read_mbox(BTU_BTIF_MBOX)) != NULL) {
                BTIF_TRACE_EVENT("btif task fetched event 0x%x\n", p_msg->event);

                switch(p_msg->event) {
                    case BT_EVT_CONTEXT_SWITCH_EVT:
                        btif_context_switched(p_msg);
                        break;

                    default:
                        BTIF_TRACE_EVENT("unhandled btif event (%d)\n", p_msg->event & BT_EVT_MASK);
                        break;
                }

                GKI_freebuf(p_msg);
            }
        }
    } while(0);

    GKI_wait(0xFFFF, 0);
}


/*******************************************************************************
**
** Function         btif_sendmsg
**
** Description      Sends msg to BTIF task
**
** Returns          void
**
*******************************************************************************/

void btif_sendmsg(void *p_msg)
{
    GKI_send_msg(BTIF_TASK, BTU_BTIF_MBOX, p_msg);
}

static void btif_fetch_local_bdaddr(tls_bt_addr_t *local_addr)
{
    BTIF_TRACE_EVENT("Call BT device driver to get bdaddr...\n");
    extern int tls_get_bt_mac_addr(uint8_t *mac);
    tls_get_bt_mac_addr(local_addr->address);
}

/*******************************************************************************
**
** Function         btif_init_bluetooth
**
** Description      Creates BTIF task and prepares BT scheduler for startup
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_init_bluetooth()
{
    uint8_t status;
    //btif_config_init();
    bte_main_boot_entry();
    /* As part of the init, fetch the local BD ADDR */
    wm_memset(&btif_local_bd_addr, 0, sizeof(tls_bt_addr_t));
    btif_fetch_local_bdaddr(&btif_local_bd_addr);
    /* start btif task */
    status = GKI_create_task(btif_task_prev, btif_task, BTIF_TASK, BTIF_TASK_STR);

    if(status != GKI_SUCCESS) {
        return TLS_BT_STATUS_FAIL;
    }

    return TLS_BT_STATUS_SUCCESS;
}


/*******************************************************************************
**
** Function         btif_enable_bluetooth
**
** Description      Performs chip power on and kickstarts OS scheduler
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t btif_enable_bluetooth(void)
{
    BTIF_TRACE_DEBUG("BTIF ENABLE BLUETOOTH");

    if(btif_core_state != BTIF_CORE_STATE_DISABLED) {
        BTIF_TRACE_WARNING("btif_core_state...not disabled\n");
        return TLS_BT_STATUS_DONE;
    }

    btif_core_state = BTIF_CORE_STATE_ENABLING;
    /* Create the GKI tasks and run them */
    bte_main_enable();
    return TLS_BT_STATUS_SUCCESS;
}


/*******************************************************************************
**
** Function         btif_enable_bluetooth_evt
**
** Description      Event indicating bluetooth enable is completed
**                  Notifies HAL user with updated adapter state
**
** Returns          void
**
*******************************************************************************/

void btif_enable_bluetooth_evt(tBTA_STATUS status, BD_ADDR local_bd)
{
    tls_bt_host_msg_t msg;

    if(bdcmp(btif_local_bd_addr.address, local_bd)) {
        bdstr_t buf;
        tls_bt_property_t prop;
        /**
         * The Controller's BDADDR does not match to the BTIF's initial BDADDR!
         * This could be because the factory BDADDR was stored separatley in
         * the Controller's non-volatile memory rather than in device's file
         * system.
         **/
        bdcpy(btif_local_bd_addr.address, local_bd);
        //save the bd address to config file
        bdaddr_to_string(&btif_local_bd_addr, buf, sizeof(bdstr_t));
        btif_config_set_str("Local", "Adapter", "Address", buf);
        btif_config_save();
        //fire HAL callback for property change
        wm_memcpy(buf, &btif_local_bd_addr, sizeof(tls_bt_addr_t));
        prop.type = WM_BT_PROPERTY_BDADDR;
        prop.val = (void *)buf;
        prop.len = sizeof(tls_bt_addr_t);
        msg.adapter_prop.status = TLS_BT_STATUS_SUCCESS;
        msg.adapter_prop.num_properties = 1;
        msg.adapter_prop.properties = &prop;

        if(bt_host_cb) { bt_host_cb(WM_BT_ADAPTER_PROP_CHG_EVT, &msg); }
    }

    bte_main_postload_cfg();
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    bte_main_enable_lpm(TRUE);
#endif
    /* add passing up bd address as well ? */
    BTIF_TRACE_DEBUG("btif_enable_bluetooth_evt,status=%d\r\n", status);

    /* callback to HAL */
    if(status == BTA_SUCCESS) {
        /* now fully enabled, update state */
        btif_core_state = BTIF_CORE_STATE_ENABLED;
#if defined(BTA_AV_SINK_INCLUDED) && BTA_AV_SINK_INCLUDED ==TRUE
        btif_av_init(BTA_A2DP_SINK_SERVICE_ID);
#endif
#if defined(BTA_HFP_HSP_INCLUDED) && BTA_HFP_HSP_INCLUDED ==TRUE
        btif_enable_service(BTA_HFP_HS_SERVICE_ID);
#endif
#if defined(BTA_SOCK_INCLUDED) && BTA_SOCK_INCLUDED == TRUE
        /* init rfcomm & l2cap api */
        btif_sock_init(uid_set);
#endif
#if defined(BTA_PAN_INCLUDED) && (BTA_PAN_INCLUDED == TRUE)
        /* init pan */
        btif_pan_init();
#endif
        /* load did configuration */
#if defined(BTIF_DM_OOB_TEST) && (BTIF_DM_OOB_TEST == TRUE)
        btif_dm_load_local_oob();
#endif
        msg.adapter_state_change.status = WM_BT_STATE_ON;

        if(bt_host_cb) { bt_host_cb(WM_BT_ADAPTER_STATE_CHG_EVT, &msg); }
    } else {
#if defined(BTA_AV_SINK_INCLUDED) && BTA_AV_SINK_INCLUDED ==TRUE
        btif_av_cleanup(BTA_A2DP_SINK_SERVICE_ID);
#endif
#if defined(BTA_HFP_HSP_INCLUDED) && BTA_HFP_HSP_INCLUDED ==TRUE
        btif_disable_service(BTA_HFP_HS_SERVICE_ID);
#endif
        /* cleanup rfcomm & l2cap api */
#if defined(BTA_SOCK_INCLUDED) && (BTA_SOCK_INCLUDED == TRUE)
        btif_sock_cleanup();
#endif
#if defined(BTA_PAN_INCLUDED) && (BTA_PAN_INCLUDED == TRUE)
        btif_pan_cleanup();
#endif
        btif_core_state = BTIF_CORE_STATE_DISABLED;
        msg.adapter_state_change.status = WM_BT_STATE_OFF;

        if(bt_host_cb) { bt_host_cb(WM_BT_ADAPTER_STATE_CHG_EVT, &msg); }
    }
}

/*******************************************************************************
**
** Function         btif_disable_bluetooth
**
** Description      Inititates shutdown of Bluetooth system.
**                  Any active links will be dropped and device entering
**                  non connectable/discoverable mode
**
** Returns          void
**
*******************************************************************************/
tls_bt_status_t btif_disable_bluetooth(void)
{
    tBTA_STATUS status;

    if(!btif_is_enabled()) {
        BTIF_TRACE_ERROR("btif_disable_bluetooth : not yet enabled");
        return TLS_BT_STATUS_NOT_READY;
    }

    BTIF_TRACE_DEBUG("BTIF DISABLE BLUETOOTH");
    btif_dm_on_disable();
    btif_core_state = BTIF_CORE_STATE_DISABLING;
#if defined(BTA_AV_SINK_INCLUDED) && BTA_AV_SINK_INCLUDED ==TRUE
    btif_av_cleanup(BTA_A2DP_SINK_SERVICE_ID);
#endif
#if defined(BTA_HFP_HSP_INCLUDED) && BTA_HFP_HSP_INCLUDED ==TRUE
    btif_disable_service(BTA_HFP_HS_SERVICE_ID);
#endif
#if defined(BTA_SOCK_INCLUDED) && BTA_SOCK_INCLUDED == TRUE
    /* cleanup rfcomm & l2cap api */
    btif_sock_cleanup();
#endif
#if defined(BTA_PAN_INCLUDED) && BTA_PAN_INCLUDED == TRUE
    btif_pan_cleanup();
#endif
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
    BTA_GATTC_Disable();
    BTA_GATTS_Disable();
#endif
    status = BTA_DisableBluetooth();
    //btif_config_flush();  //do we need to flush to flash???

    if(status != BTA_SUCCESS) {
        BTIF_TRACE_ERROR("disable bt failed (%d)", status);
        /* reset the original state to allow attempting disable again */
        btif_core_state = BTIF_CORE_STATE_ENABLED;
        return TLS_BT_STATUS_FAIL;
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_disable_bluetooth_evt
**
** Description      Event notifying BT disable is now complete.
**                  Terminates main stack tasks and notifies HAL
**                  user with updated BT state.
**
** Returns          void
**
*******************************************************************************/

void btif_disable_bluetooth_evt(void)
{
    tls_bt_host_msg_t msg;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    bte_main_enable_lpm(FALSE);
#endif
#if (BLE_INCLUDED == TRUE)
    BTA_VendorCleanup();
#endif
    bte_main_disable();
    /* update local state */
    btif_core_state = BTIF_CORE_STATE_DISABLED;
    /* callback to HAL */
    msg.adapter_state_change.status = WM_BT_STATE_OFF;

    if(bt_host_cb) { bt_host_cb(WM_BT_ADAPTER_STATE_CHG_EVT, &msg); }

    if(btif_shutdown_pending) {
        BTIF_TRACE_DEBUG("%s: calling btif_shutdown_bluetooth", __FUNCTION__);
        btif_cleanup_bluetooth();
    }
}

/*******************************************************************************
**
** Function         btif_cleanup_bluetooth
**
** Description      Cleanup BTIF state.
**
** Returns          void
**
*******************************************************************************/

tls_bt_status_t btif_cleanup_bluetooth(void)
{
    tls_bt_host_msg_t msg;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    if(btif_core_state == BTIF_CORE_STATE_DISABLING) {
        BTIF_TRACE_WARNING("shutdown during disabling");
        /* shutdown called before disabling is done */
        btif_shutdown_pending = 1;
        return TLS_BT_STATUS_NOT_READY;
    }

    if(btif_is_enabled()) {
        BTIF_TRACE_WARNING("shutdown while still enabled, initiate disable");
        /* shutdown called prior to disabling, initiate disable */
        btif_disable_bluetooth();
        btif_shutdown_pending = 1;
        return TLS_BT_STATUS_NOT_READY;
    }

    btif_shutdown_pending = 0;

    if(btif_core_state == BTIF_CORE_STATE_ENABLING) {
        // Java layer abort BT ENABLING, could be due to ENABLE TIMEOUT
        // Direct call from cleanup()@bluetooth.c
        // bring down HCI/Vendor lib
        bte_main_disable();
        btif_core_state = BTIF_CORE_STATE_DISABLED;
        msg.adapter_state_change.status = WM_BT_STATE_OFF;

        if(bt_host_cb) { bt_host_cb(WM_BT_ADAPTER_STATE_CHG_EVT, &msg); }
    }

    GKI_destroy_task(BTIF_TASK);
    btif_queue_release();
    bte_main_cleanup();
    btif_dut_mode = 0;
    //bt_utils_cleanup();
    GKI_dealloc_free_queue();
    BTIF_TRACE_DEBUG("%s done", __FUNCTION__);
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dut_mode_cback
**
** Description     Callback invoked on completion of vendor specific test mode command
**
** Returns          None
**
*******************************************************************************/
static void btif_dut_mode_cback(tBTM_VSC_CMPL *p)
{
    UNUSED(p);
    /* For now nothing to be done. */
}

/*******************************************************************************
**
** Function         btif_dut_mode_configure
**
** Description      Configure Test Mode - 'enable' to 1 puts the device in test mode and 0 exits
**                       test mode
**
** Returns          BT_STATUS_SUCCESS on success
**
*******************************************************************************/
tls_bt_status_t btif_dut_mode_configure(uint8_t enable)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    if(btif_core_state != BTIF_CORE_STATE_ENABLED) {
        BTIF_TRACE_ERROR("btif_dut_mode_configure : Bluetooth not enabled");
        return TLS_BT_STATUS_NOT_READY;
    }

    btif_dut_mode = enable;

    if(enable == 1) {
        BTA_EnableTestMode();
    } else {
        BTA_DisableTestMode();
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dut_mode_send
**
** Description     Sends a HCI Vendor specific command to the controller
**
** Returns          BT_STATUS_SUCCESS on success
**
*******************************************************************************/
tls_bt_status_t btif_dut_mode_send(uint16_t opcode, uint8_t *buf, uint8_t len)
{
    /* TODO: Check that opcode is a vendor command group */
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    if(!btif_is_dut_mode()) {
        BTIF_TRACE_ERROR("Bluedroid HAL needs to be init with test_mode set to 1.");
        return TLS_BT_STATUS_FAIL;
    }

    BTM_VendorSpecificCommand(opcode, len, buf, btif_dut_mode_cback);
    return TLS_BT_STATUS_SUCCESS;
}

/*****************************************************************************
**
**   btif api adapter property functions
**
*****************************************************************************/

static tls_bt_status_t btif_in_get_adapter_properties(void)
{
    tls_bt_host_msg_t msg;
    tls_bt_property_t properties[6];
    uint32_t num_props;
    tls_bt_addr_t addr;
    tls_bt_bdname_t name;
    bt_scan_mode_t mode;
    uint32_t disc_timeout;
    tls_bt_addr_t bonded_devices[BTM_SEC_MAX_DEVICE_RECORDS];
    tls_bt_uuid_t local_uuids[BT_MAX_NUM_UUIDS];
    num_props = 0;
    /* BD_ADDR */
    BTIF_STORAGE_FILL_PROPERTY(&properties[num_props], WM_BT_PROPERTY_BDADDR,
                               sizeof(addr), &addr);
    btif_storage_get_adapter_property(&properties[num_props]);
    num_props++;
    /* BD_NAME */
    BTIF_STORAGE_FILL_PROPERTY(&properties[num_props], WM_BT_PROPERTY_BDNAME,
                               sizeof(name), &name);
    btif_storage_get_adapter_property(&properties[num_props]);
    num_props++;
    /* SCAN_MODE */
    BTIF_STORAGE_FILL_PROPERTY(&properties[num_props], WM_BT_PROPERTY_ADAPTER_SCAN_MODE,
                               sizeof(mode), &mode);
    btif_storage_get_adapter_property(&properties[num_props]);
    num_props++;
    /* DISC_TIMEOUT */
    BTIF_STORAGE_FILL_PROPERTY(&properties[num_props], WM_BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT,
                               sizeof(disc_timeout), &disc_timeout);
    btif_storage_get_adapter_property(&properties[num_props]);
    num_props++;
    /* BONDED_DEVICES */
    BTIF_STORAGE_FILL_PROPERTY(&properties[num_props], WM_BT_PROPERTY_ADAPTER_BONDED_DEVICES,
                               sizeof(bonded_devices), bonded_devices);
    btif_storage_get_adapter_property(&properties[num_props]);
    num_props++;
    /* LOCAL UUIDs */
    BTIF_STORAGE_FILL_PROPERTY(&properties[num_props], WM_BT_PROPERTY_UUIDS,
                               sizeof(local_uuids), local_uuids);
    btif_storage_get_adapter_property(&properties[num_props]);
    num_props++;
    msg.adapter_prop.status = TLS_BT_STATUS_SUCCESS;
    msg.adapter_prop.num_properties = num_props;
    msg.adapter_prop.properties = properties;

    if(bt_host_cb) { bt_host_cb(WM_BT_ADAPTER_PROP_CHG_EVT, &msg); }

    return TLS_BT_STATUS_SUCCESS;
}

static tls_bt_status_t btif_in_get_remote_device_properties(tls_bt_addr_t *bd_addr)
{
    tls_bt_host_msg_t msg;
    tls_bt_property_t remote_properties[8];
    uint32_t num_props = 0;
    tls_bt_bdname_t name, alias;
    uint32_t cod, devtype;
    tls_bt_uuid_t remote_uuids[BT_MAX_NUM_UUIDS];
    wm_memset(remote_properties, 0, sizeof(remote_properties));
    BTIF_STORAGE_FILL_PROPERTY(&remote_properties[num_props], WM_BT_PROPERTY_BDNAME,
                               sizeof(name), &name);
    btif_storage_get_remote_device_property(bd_addr,
                                            &remote_properties[num_props]);
    num_props++;
    BTIF_STORAGE_FILL_PROPERTY(&remote_properties[num_props], WM_BT_PROPERTY_REMOTE_FRIENDLY_NAME,
                               sizeof(alias), &alias);
    btif_storage_get_remote_device_property(bd_addr,
                                            &remote_properties[num_props]);
    num_props++;
    BTIF_STORAGE_FILL_PROPERTY(&remote_properties[num_props], WM_BT_PROPERTY_CLASS_OF_DEVICE,
                               sizeof(cod), &cod);
    btif_storage_get_remote_device_property(bd_addr,
                                            &remote_properties[num_props]);
    num_props++;
    BTIF_STORAGE_FILL_PROPERTY(&remote_properties[num_props], WM_BT_PROPERTY_TYPE_OF_DEVICE,
                               sizeof(devtype), &devtype);
    btif_storage_get_remote_device_property(bd_addr,
                                            &remote_properties[num_props]);
    num_props++;
    BTIF_STORAGE_FILL_PROPERTY(&remote_properties[num_props], WM_BT_PROPERTY_UUIDS,
                               sizeof(remote_uuids), remote_uuids);
    btif_storage_get_remote_device_property(bd_addr,
                                            &remote_properties[num_props]);
    num_props++;
    msg.remote_device_prop.status = TLS_BT_STATUS_SUCCESS;
    msg.remote_device_prop.num_properties = num_props;
    msg.remote_device_prop.properties = remote_properties;
    msg.remote_device_prop.address = bd_addr;

    if(bt_host_cb) { bt_host_cb(WM_BT_RMT_DEVICE_PROP_EVT, &msg); }

    return TLS_BT_STATUS_SUCCESS;
}


/*******************************************************************************
**
** Function         execute_storage_request
**
** Description      Executes adapter storage request in BTIF context
**
** Returns          bt_status_t
**
*******************************************************************************/

static void execute_storage_request(uint16_t event, char *p_param)
{
    tls_bt_host_msg_t msg;
    tls_bt_status_t status = TLS_BT_STATUS_SUCCESS;
    BTIF_TRACE_EVENT("execute storage request event : %d", event);

    switch(event) {
        case BTIF_CORE_STORAGE_ADAPTER_WRITE: {
            btif_storage_req_t *p_req = (btif_storage_req_t *)p_param;
            tls_bt_property_t *p_prop = &(p_req->write_req.prop);
            BTIF_TRACE_EVENT("type: %d, len %d, 0x%x", p_prop->type,
                             p_prop->len, p_prop->val);
            status = btif_storage_set_adapter_property(p_prop);
            msg.adapter_prop.status = status;
            msg.adapter_prop.properties = p_prop;
            msg.adapter_prop.num_properties = 1;

            if(bt_host_cb) { bt_host_cb(WM_BT_ADAPTER_PROP_CHG_EVT, &msg); }
        }
        break;

        case BTIF_CORE_STORAGE_ADAPTER_READ: {
            btif_storage_req_t *p_req = (btif_storage_req_t *)p_param;
            char buf[512];
            tls_bt_property_t prop;
            prop.type = p_req->read_req.type;
            prop.val = (void *)buf;
            prop.len = sizeof(buf);

            if(prop.type == WM_BT_PROPERTY_LOCAL_LE_FEATURES) {
#if (BLE_INCLUDED == TRUE)
                tBTM_BLE_VSC_CB cmn_vsc_cb;
                bt_local_le_features_t local_le_features;
                /* LE features are not stored in storage. Should be retrived from stack */
                BTM_BleGetVendorCapabilities(&cmn_vsc_cb);
                local_le_features.local_privacy_enabled = BTM_BleLocalPrivacyEnabled();
                prop.len = sizeof(bt_local_le_features_t);

                if(cmn_vsc_cb.filter_support == 1) {
                    local_le_features.max_adv_filter_supported = cmn_vsc_cb.max_filter;
                } else {
                    local_le_features.max_adv_filter_supported = 0;
                }

                local_le_features.max_adv_instance = cmn_vsc_cb.adv_inst_max;
                local_le_features.max_irk_list_size = cmn_vsc_cb.max_irk_list_sz;
                local_le_features.rpa_offload_supported = cmn_vsc_cb.rpa_offloading;
                local_le_features.scan_result_storage_size = cmn_vsc_cb.tot_scan_results_strg;
                local_le_features.activity_energy_info_supported = cmn_vsc_cb.energy_support;
                local_le_features.version_supported = cmn_vsc_cb.version_supported;
                local_le_features.total_trackable_advertisers =
                                cmn_vsc_cb.total_trackable_advertisers;
                local_le_features.extended_scan_support = cmn_vsc_cb.extended_scan_support > 0;
                local_le_features.debug_logging_supported = cmn_vsc_cb.debug_logging_supported > 0;
                wm_memcpy(prop.val, &local_le_features, prop.len);
#endif
            } else {
                status = btif_storage_get_adapter_property(&prop);
            }

            msg.adapter_prop.status = status;
            msg.adapter_prop.num_properties = 1;
            msg.adapter_prop.properties = &prop;

            if(bt_host_cb) { bt_host_cb(WM_BT_ADAPTER_PROP_CHG_EVT, &msg); }
        }
        break;

        case BTIF_CORE_STORAGE_ADAPTER_READ_ALL: {
            status = btif_in_get_adapter_properties();
        }
        break;

        case BTIF_CORE_STORAGE_NOTIFY_STATUS: {
            msg.adapter_prop.status = status;
            msg.adapter_prop.num_properties = 0;
            msg.adapter_prop.properties = NULL;

            if(bt_host_cb) { bt_host_cb(WM_BT_ADAPTER_PROP_CHG_EVT, &msg); }
        }
        break;

        default:
            BTIF_TRACE_ERROR("%s invalid event id (%d)", __FUNCTION__, event);
            break;
    }
}

static void execute_storage_remote_request(uint16_t event, char *p_param)
{
    tls_bt_host_msg_t msg;
    tls_bt_status_t status = TLS_BT_STATUS_FAIL;
    tls_bt_property_t prop;
    BTIF_TRACE_EVENT("execute storage remote request event : %d", event);

    switch(event) {
        case BTIF_CORE_STORAGE_REMOTE_READ: {
            char buf[1024];
            btif_storage_req_t *p_req = (btif_storage_req_t *)p_param;
            prop.type = p_req->read_req.type;
            prop.val = (void *) buf;
            prop.len = sizeof(buf);
            status = btif_storage_get_remote_device_property(&(p_req->read_req.bd_addr),
                     &prop);
            msg.remote_device_prop.num_properties = 1;
            msg.remote_device_prop.properties = &prop;
            msg.remote_device_prop.status = status;
            msg.remote_device_prop.address = &(p_req->read_req.bd_addr);

            if(bt_host_cb) { bt_host_cb(WM_BT_RMT_DEVICE_PROP_EVT, &msg); }
        }
        break;

        case BTIF_CORE_STORAGE_REMOTE_WRITE: {
            btif_storage_req_t *p_req = (btif_storage_req_t *)p_param;
            status = btif_storage_set_remote_device_property(&(p_req->write_req.bd_addr),
                     &(p_req->write_req.prop));
        }
        break;

        case BTIF_CORE_STORAGE_REMOTE_READ_ALL: {
            btif_storage_req_t *p_req = (btif_storage_req_t *)p_param;
            btif_in_get_remote_device_properties(&p_req->read_req.bd_addr);
        }
        break;
    }
}

void btif_adapter_properties_evt(tls_bt_status_t status, uint32_t num_props,
                                 tls_bt_property_t *p_props)
{
    tls_bt_host_msg_t msg;
    msg.adapter_prop.status = status;
    msg.adapter_prop.num_properties = num_props;
    msg.adapter_prop.properties = p_props;

    if(bt_host_cb) { bt_host_cb(WM_BT_ADAPTER_PROP_CHG_EVT, &msg); }
}
void btif_remote_properties_evt(tls_bt_status_t status, tls_bt_addr_t *remote_addr,
                                uint32_t num_props, tls_bt_property_t *p_props)
{
    tls_bt_host_msg_t msg;
    msg.remote_device_prop.address = remote_addr;
    msg.remote_device_prop.num_properties = num_props;
    msg.remote_device_prop.properties = p_props;
    msg.remote_device_prop.status = status;

    if(bt_host_cb) { bt_host_cb(WM_BT_RMT_DEVICE_PROP_EVT, &msg); }
}

/*******************************************************************************
**
** Function         btif_in_storage_request_copy_cb
**
** Description     Switch context callback function to perform the deep copy for
**                 both the adapter and remote_device property API
**
** Returns          None
**
*******************************************************************************/
static void btif_in_storage_request_copy_cb(uint16_t event,
        char *p_new_buf, char *p_old_buf)
{
    btif_storage_req_t *new_req = (btif_storage_req_t *)p_new_buf;
    btif_storage_req_t *old_req = (btif_storage_req_t *)p_old_buf;
    BTIF_TRACE_EVENT("%s", __FUNCTION__);

    switch(event) {
        case BTIF_CORE_STORAGE_REMOTE_WRITE:
        case BTIF_CORE_STORAGE_ADAPTER_WRITE: {
            bdcpy(new_req->write_req.bd_addr.address, old_req->write_req.bd_addr.address);
            /* Copy the member variables one at a time */
            new_req->write_req.prop.type = old_req->write_req.prop.type;
            new_req->write_req.prop.len = old_req->write_req.prop.len;
            new_req->write_req.prop.val = (uint8_t *)(p_new_buf + sizeof(btif_storage_req_t));
            wm_memcpy(new_req->write_req.prop.val, old_req->write_req.prop.val,
                      old_req->write_req.prop.len);
        }
        break;
    }
}

/*******************************************************************************
**
** Function         btif_get_adapter_properties
**
** Description      Fetch all available properties (local & remote)
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t btif_get_adapter_properties(void)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);

    if(!btif_is_enabled()) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_transfer_context(execute_storage_request,
                                 BTIF_CORE_STORAGE_ADAPTER_READ_ALL,
                                 NULL, 0, NULL);
}

/*******************************************************************************
**
** Function         btif_get_adapter_property
**
** Description      Fetches property value from local cache
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t btif_get_adapter_property(tls_bt_property_type_t type)
{
    btif_storage_req_t req;
    BTIF_TRACE_EVENT("%s %d", __FUNCTION__, type);

    /* Allow get_adapter_property only for BDADDR and BDNAME if BT is disabled */
    if(!btif_is_enabled() && (type != WM_BT_PROPERTY_BDADDR) && (type != WM_BT_PROPERTY_BDNAME)) {
        return TLS_BT_STATUS_NOT_READY;
    }

    wm_memset(&(req.read_req.bd_addr), 0, sizeof(tls_bt_addr_t));
    req.read_req.type = type;
    return btif_transfer_context(execute_storage_request,
                                 BTIF_CORE_STORAGE_ADAPTER_READ,
                                 (char *)&req, sizeof(btif_storage_req_t), NULL);
}

/*******************************************************************************
**
** Function         btif_set_adapter_property
**
** Description      Updates core stack with property value and stores it in
**                  local cache
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t btif_set_adapter_property(const tls_bt_property_t *property,
        uint8_t update_to_flash)
{
    btif_storage_req_t req;
    tls_bt_status_t status = TLS_BT_STATUS_SUCCESS;
    int storage_req_id = BTIF_CORE_STORAGE_NOTIFY_STATUS; /* default */
    char bd_name[BTM_MAX_LOC_BD_NAME_LEN + 1];
    uint16_t  name_len = 0;
    BTIF_TRACE_EVENT("btif_set_adapter_property type: %d, len %d, 0x%x",
                     property->type, property->len, property->val);

    if(!btif_is_enabled()) {
        return TLS_BT_STATUS_NOT_READY;
    }

    switch(property->type) {
        case WM_BT_PROPERTY_BDNAME: {
            name_len = property->len > BTM_MAX_LOC_BD_NAME_LEN ? BTM_MAX_LOC_BD_NAME_LEN :
                       property->len;
            wm_memcpy(bd_name, property->val, name_len);
            bd_name[name_len] = '\0';
            BTIF_TRACE_EVENT("set property name : %s", (char *)bd_name);
            BTA_DmSetDeviceName((char *)bd_name);

            if(update_to_flash) {
                storage_req_id = BTIF_CORE_STORAGE_ADAPTER_WRITE;
            }
        }
        break;

        case WM_BT_PROPERTY_ADAPTER_SCAN_MODE: {
            bt_scan_mode_t mode = BT_SCAN_MODE_NONE;
            tBTA_DM_DISC disc_mode;
            tBTA_DM_CONN conn_mode;
            uint8_t *ptr_mode = (uint8_t *)property->val;

            if(property->len == 1) {
                if(ptr_mode[0] == '2') {
                    mode = BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE;
                } else if(ptr_mode[0] == '1') {
                    mode = BT_SCAN_MODE_CONNECTABLE;
                } else {
                    mode = BT_SCAN_MODE_NONE;
                }
            }

            switch(mode) {
                case BT_SCAN_MODE_NONE:
                    disc_mode = BTA_DM_NON_DISC;
                    conn_mode = BTA_DM_NON_CONN;
                    break;

                case BT_SCAN_MODE_CONNECTABLE:
                    disc_mode = BTA_DM_NON_DISC;
                    conn_mode = BTA_DM_CONN;
                    break;

                case BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE:
                    disc_mode = BTA_DM_GENERAL_DISC;
                    conn_mode = BTA_DM_CONN;
                    break;

                default:
                    BTIF_TRACE_ERROR("invalid scan mode (0x%x)", mode);
                    return TLS_BT_STATUS_PARM_INVALID;
            }

#if (BLE_INCLUDED == TRUE)
            BTA_DmSetVisibility(disc_mode | BTA_DM_LE_IGNORE, conn_mode | BTA_DM_LE_IGNORE, BTA_DM_IGNORE,
                                BTA_DM_IGNORE);
#else
            BTA_DmSetVisibility(disc_mode, conn_mode, BTA_DM_IGNORE, BTA_DM_IGNORE);
#endif
            storage_req_id = BTIF_CORE_STORAGE_ADAPTER_WRITE;
        }
        break;

        case WM_BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT: {
            /* Nothing to do beside store the value in NV.  Java
                will change the SCAN_MODE property after setting timeout,
               if required */
            storage_req_id = BTIF_CORE_STORAGE_ADAPTER_WRITE;
        }
        break;

        case WM_BT_PROPERTY_BDADDR:
        case WM_BT_PROPERTY_UUIDS:
        case WM_BT_PROPERTY_ADAPTER_BONDED_DEVICES:
        case WM_BT_PROPERTY_REMOTE_FRIENDLY_NAME:
            /* no write support through HAL, these properties are only populated from BTA events */
            status = TLS_BT_STATUS_FAIL;
            break;

        default:
            BTIF_TRACE_ERROR("btif_get_adapter_property : invalid type %d",
                             property->type);
            status = TLS_BT_STATUS_FAIL;
            break;
    }

    if(storage_req_id != BTIF_CORE_STORAGE_NO_ACTION) {
        /* pass on to storage for updating local database */
        wm_memset(&(req.write_req.bd_addr), 0, sizeof(tls_bt_addr_t));
        wm_memcpy(&(req.write_req.prop), property, sizeof(tls_bt_property_t));
        return btif_transfer_context(execute_storage_request,
                                     storage_req_id,
                                     (char *)&req,
                                     sizeof(btif_storage_req_t) + property->len,
                                     btif_in_storage_request_copy_cb);
    }

    return status;
}

/*******************************************************************************
**
** Function         btif_get_remote_device_property
**
** Description      Fetches the remote device property from the NVRAM
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_get_remote_device_property(tls_bt_addr_t *remote_addr,
        tls_bt_property_type_t type)
{
    btif_storage_req_t req;

    if(!btif_is_enabled()) {
        return TLS_BT_STATUS_NOT_READY;
    }

    wm_memcpy(&(req.read_req.bd_addr), remote_addr, sizeof(tls_bt_addr_t));
    req.read_req.type = type;
    return btif_transfer_context(execute_storage_remote_request,
                                 BTIF_CORE_STORAGE_REMOTE_READ,
                                 (char *)&req, sizeof(btif_storage_req_t),
                                 NULL);
}

/*******************************************************************************
**
** Function         btif_get_remote_device_properties
**
** Description      Fetches all the remote device properties from NVRAM
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_get_remote_device_properties(tls_bt_addr_t *remote_addr)
{
    btif_storage_req_t req;

    if(!btif_is_enabled()) {
        return TLS_BT_STATUS_NOT_READY;
    }

    wm_memcpy(&(req.read_req.bd_addr), remote_addr, sizeof(tls_bt_addr_t));
    return btif_transfer_context(execute_storage_remote_request,
                                 BTIF_CORE_STORAGE_REMOTE_READ_ALL,
                                 (char *)&req, sizeof(btif_storage_req_t),
                                 NULL);
}

/*******************************************************************************
**
** Function         btif_set_remote_device_property
**
** Description      Writes the remote device property to NVRAM.
**                  Currently, BT_PROPERTY_REMOTE_FRIENDLY_NAME is the only
**                  remote device property that can be set
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_set_remote_device_property(tls_bt_addr_t *remote_addr,
        const tls_bt_property_t *property)
{
    btif_storage_req_t req;

    if(!btif_is_enabled()) {
        return TLS_BT_STATUS_NOT_READY;
    }

    wm_memcpy(&(req.write_req.bd_addr), remote_addr, sizeof(tls_bt_addr_t));
    wm_memcpy(&(req.write_req.prop), property, sizeof(tls_bt_property_t));
    return btif_transfer_context(execute_storage_remote_request,
                                 BTIF_CORE_STORAGE_REMOTE_WRITE,
                                 (char *)&req,
                                 sizeof(btif_storage_req_t) + property->len,
                                 btif_in_storage_request_copy_cb);
}


/*******************************************************************************
**
** Function         btif_get_remote_service_record
**
** Description      Looks up the service matching uuid on the remote device
**                  and fetches the SCN and service_name if the UUID is found
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_get_remote_service_record(tls_bt_addr_t *remote_addr,
        tls_bt_uuid_t *uuid)
{
    if(!btif_is_enabled()) {
        return TLS_BT_STATUS_NOT_READY;
    }

    return btif_dm_get_remote_service_record(remote_addr, uuid);
}


/*******************************************************************************
**
** Function         btif_get_enabled_services_mask
**
** Description      Fetches currently enabled services
**
** Returns          tBTA_SERVICE_MASK
**
*******************************************************************************/

tBTA_SERVICE_MASK btif_get_enabled_services_mask(void)
{
    return btif_enabled_services;
}

/*******************************************************************************
**
** Function         btif_enable_service
**
** Description      Enables the service 'service_ID' to the service_mask.
**                  Upon BT enable, BTIF core shall invoke the BTA APIs to
**                  enable the profiles
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_enable_service(tBTA_SERVICE_ID service_id)
{
    tBTA_SERVICE_ID *p_id = &service_id;
    /* If BT is enabled, we need to switch to BTIF context and trigger the
     * enable for that profile
     *
     * Otherwise, we just set the flag. On BT_Enable, the DM will trigger
     * enable for the profiles that have been enabled */

    if(btif_enabled_services & (1 << service_id))
    { return TLS_BT_STATUS_SUCCESS; }

    btif_enabled_services |= (1 << service_id);
    BTIF_TRACE_DEBUG("%s: current services:0x%x", __FUNCTION__, btif_enabled_services);

    if(btif_is_enabled()) {
        btif_transfer_context(btif_dm_execute_service_request,
                              BTIF_DM_ENABLE_SERVICE,
                              (char *)p_id, sizeof(tBTA_SERVICE_ID), NULL);
    }

    return TLS_BT_STATUS_SUCCESS;
}
/*******************************************************************************
**
** Function         btif_disable_service
**
** Description      Disables the service 'service_ID' to the service_mask.
**                  Upon BT disable, BTIF core shall invoke the BTA APIs to
**                  disable the profiles
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_disable_service(tBTA_SERVICE_ID service_id)
{
    tBTA_SERVICE_ID *p_id = &service_id;
    /* If BT is enabled, we need to switch to BTIF context and trigger the
     * disable for that profile so that the appropriate uuid_property_changed will
     * be triggerred. Otherwise, we just need to clear the service_id in the mask
     */
    btif_enabled_services &= (tBTA_SERVICE_MASK)(~(1 << service_id));
    BTIF_TRACE_DEBUG("%s: Current Services:0x%x", __FUNCTION__, btif_enabled_services);

    if(btif_is_enabled()) {
        btif_transfer_context(btif_dm_execute_service_request,
                              BTIF_DM_DISABLE_SERVICE,
                              (char *)p_id, sizeof(tBTA_SERVICE_ID), NULL);
    }

    return TLS_BT_STATUS_SUCCESS;
}

