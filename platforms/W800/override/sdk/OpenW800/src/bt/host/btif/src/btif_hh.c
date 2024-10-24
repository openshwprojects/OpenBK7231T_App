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
 *  Filename:      btif_hh.c
 *
 *  Description:   HID Host Profile Bluetooth Interface
 *
 *
 ***********************************************************************************/

#define LOG_TAG "bt_btif_hh"
#include "bt_target.h"

#if defined(BTA_HH_INCLUDED) && (BTA_HH_INCLUDED == TRUE)
#include "btif_hh.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "bta_api.h"
#include "btif_common.h"
#include "btif_storage.h"
#include "btif_util.h"
#include "bt_common.h"
#include "l2c_api.h"
#include "osi/include/log.h"

#define BTIF_HH_APP_ID_MI       0x01
#define BTIF_HH_APP_ID_KB       0x02

#define COD_HID_KEYBOARD        0x0540
#define COD_HID_POINTING        0x0580
#define COD_HID_COMBO           0x05C0

#define KEYSTATE_FILEPATH "/data/misc/bluedroid/bt_hh_ks" //keep this in sync with HID host jni

#define HID_REPORT_CAPSLOCK   0x39
#define HID_REPORT_NUMLOCK    0x53
#define HID_REPORT_SCROLLLOCK 0x47

//For Apple Magic Mouse
#define MAGICMOUSE_VENDOR_ID 0x05ac
#define MAGICMOUSE_PRODUCT_ID 0x030d

#define LOGITECH_KB_MX5500_VENDOR_ID  0x046D
#define LOGITECH_KB_MX5500_PRODUCT_ID 0xB30B

extern fixed_queue_t *btu_general_alarm_queue;
extern const int BT_UID;
extern const int BT_GID;
static int btif_hh_keylockstates = 0; //The current key state of each key

#define BTIF_HH_ID_1        0
#define BTIF_HH_DEV_DISCONNECTED 3

#define BTIF_TIMEOUT_VUP_MS   (3 * 1000)

#ifndef BTUI_HH_SECURITY
#define BTUI_HH_SECURITY (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTUI_HH_MOUSE_SECURITY
#define BTUI_HH_MOUSE_SECURITY (BTA_SEC_NONE)
#endif

/* HH request events */
typedef enum {
    BTIF_HH_CONNECT_REQ_EVT = 0,
    BTIF_HH_DISCONNECT_REQ_EVT,
    BTIF_HH_VUP_REQ_EVT
} btif_hh_req_evt_t;

/************************************************************************************
**  Constants & Macros
************************************************************************************/
#define BTIF_HH_SERVICES    (BTA_HID_SERVICE_MASK)

/************************************************************************************
**  Local type definitions
************************************************************************************/

typedef struct hid_kb_list {
    uint16_t product_id;
    uint16_t version_id;
    char  *kb_name;
} tHID_KB_LIST;

/************************************************************************************
**  Static variables
************************************************************************************/
btif_hh_cb_t btif_hh_cb;

static bthh_callbacks_t *bt_hh_callbacks = NULL;

/* List of HID keyboards for which the NUMLOCK state needs to be
 * turned ON by default. Add devices to this list to apply the
 * NUMLOCK state toggle on fpr first connect.*/
static tHID_KB_LIST hid_kb_numlock_on_list[] = {
    {
        LOGITECH_KB_MX5500_PRODUCT_ID,
        LOGITECH_KB_MX5500_VENDOR_ID,
        "Logitech MX5500 Keyboard"
    }
};

#define CHECK_BTHH_INIT() if (bt_hh_callbacks == NULL)\
    {\
        BTIF_TRACE_WARNING("BTHH: %s: BTHH not initialized", __FUNCTION__);\
        return TLS_BT_STATUS_NOT_READY;\
    }\
    else\
    {\
        BTIF_TRACE_EVENT("BTHH: %s", __FUNCTION__);\
    }

/************************************************************************************
**  Static functions
************************************************************************************/

/************************************************************************************
**  Externs
************************************************************************************/
extern void bta_hh_co_destroy(int fd);
extern void bta_hh_co_write(int fd, uint8_t *rpt, uint16_t len);
extern tls_bt_status_t btif_dm_remove_bond(const tls_bt_addr_t *bd_addr);
extern void bta_hh_co_send_hid_info(btif_hh_device_t *p_dev, char *dev_name, uint16_t vendor_id,
                                    uint16_t product_id, uint16_t version, uint8_t ctry_code,
                                    int dscp_len, uint8_t *p_dscp);
extern uint8_t check_cod(const tls_bt_addr_t *remote_bdaddr, uint32_t cod);
extern void btif_dm_cb_remove_bond(tls_bt_addr_t *bd_addr);
extern uint8_t check_cod_hid(const tls_bt_addr_t *remote_bdaddr);
extern int  scru_ascii_2_hex(char *p_ascii, int len, uint8_t *p_hex);
extern void btif_dm_hh_open_failed(tls_bt_addr_t *bdaddr);

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
static void set_keylockstate(int keymask, uint8_t isSet);
static void toggle_os_keylockstates(int fd, int changedkeystates);
static void sync_lockstate_on_connect(btif_hh_device_t *p_dev);
//static void hh_update_keyboard_lockstates(btif_hh_device_t *p_dev);
void btif_hh_timer_timeout(void *data);

/************************************************************************************
**  Functions
************************************************************************************/

static int get_keylockstates()
{
    return btif_hh_keylockstates;
}

static void set_keylockstate(int keymask, uint8_t isSet)
{
    if(isSet) {
        btif_hh_keylockstates |= keymask;
    }
}

/*******************************************************************************
**
** Function         toggle_os_keylockstates
**
** Description      Function to toggle the keyboard lock states managed by the linux.
**                  This function is used in by two call paths
**                  (1) if the lock state change occurred from an onscreen keyboard,
**                  this function is called to update the lock state maintained
                    for the HID keyboard(s)
**                  (2) if a HID keyboard is disconnected and reconnected,
**                  this function is called to update the lock state maintained
                    for the HID keyboard(s)
** Returns          void
*******************************************************************************/

static void toggle_os_keylockstates(int fd, int changedlockstates)
{
    BTIF_TRACE_EVENT("%s: fd = %d, changedlockstates = 0x%x",
                     __FUNCTION__, fd, changedlockstates);
    uint8_t hidreport[9];
    int reportIndex;
    wm_memset(hidreport, 0, 9);
    hidreport[0] = 1;
    reportIndex = 4;

    if(changedlockstates & BTIF_HH_KEYSTATE_MASK_CAPSLOCK) {
        BTIF_TRACE_DEBUG("%s Setting CAPSLOCK", __FUNCTION__);
        hidreport[reportIndex++] = (uint8_t)HID_REPORT_CAPSLOCK;
    }

    if(changedlockstates & BTIF_HH_KEYSTATE_MASK_NUMLOCK) {
        BTIF_TRACE_DEBUG("%s Setting NUMLOCK", __FUNCTION__);
        hidreport[reportIndex++] = (uint8_t)HID_REPORT_NUMLOCK;
    }

    if(changedlockstates & BTIF_HH_KEYSTATE_MASK_SCROLLLOCK) {
        BTIF_TRACE_DEBUG("%s Setting SCROLLLOCK", __FUNCTION__);
        hidreport[reportIndex++] = (uint8_t) HID_REPORT_SCROLLLOCK;
    }

    BTIF_TRACE_DEBUG("Writing hidreport #1 to os: "\
                     "%s:  %x %x %x", __FUNCTION__,
                     hidreport[0], hidreport[1], hidreport[2]);
    BTIF_TRACE_DEBUG("%s:  %x %x %x", __FUNCTION__,
                     hidreport[3], hidreport[4], hidreport[5]);
    BTIF_TRACE_DEBUG("%s:  %x %x %x", __FUNCTION__,
                     hidreport[6], hidreport[7], hidreport[8]);
    bta_hh_co_write(fd, hidreport, sizeof(hidreport));
    usleep(200000);
    wm_memset(hidreport, 0, 9);
    hidreport[0] = 1;
    BTIF_TRACE_DEBUG("Writing hidreport #2 to os: "\
                     "%s:  %x %x %x", __FUNCTION__,
                     hidreport[0], hidreport[1], hidreport[2]);
    BTIF_TRACE_DEBUG("%s:  %x %x %x", __FUNCTION__,
                     hidreport[3], hidreport[4], hidreport[5]);
    BTIF_TRACE_DEBUG("%s:  %x %x %x ", __FUNCTION__,
                     hidreport[6], hidreport[7], hidreport[8]);
    bta_hh_co_write(fd, hidreport, sizeof(hidreport));
}

/*******************************************************************************
**
** Function         create_pbuf
**
** Description      Helper function to create p_buf for send_data or set_report
**
*******************************************************************************/
static BT_HDR *create_pbuf(uint16_t len, uint8_t *data)
{
    BT_HDR *p_buf = GKI_getbuf(len + BTA_HH_MIN_OFFSET + sizeof(BT_HDR));
    uint8_t *pbuf_data;
    p_buf->len = len;
    p_buf->offset = BTA_HH_MIN_OFFSET;
    pbuf_data = (uint8_t *)(p_buf + 1) + p_buf->offset;
    wm_memcpy(pbuf_data, data, len);
    return p_buf;
}

/*******************************************************************************
**
** Function         update_keyboard_lockstates
**
** Description      Sends a report to the keyboard to set the lock states of keys
**
*******************************************************************************/
static void update_keyboard_lockstates(btif_hh_device_t *p_dev)
{
    uint8_t len = 2;  /* reportid + 1 byte report*/
    BD_ADDR *bda;
    BT_HDR *p_buf;
    uint8_t data[] = {0x01, /* report id */
                      btif_hh_keylockstates
                     }; /* keystate */
    /* Set report for other keyboards */
    BTIF_TRACE_EVENT("%s: setting report on dev_handle %d to 0x%x",
                     __FUNCTION__, p_dev->dev_handle, btif_hh_keylockstates);
    /* Get SetReport buffer */
    p_buf = create_pbuf(len, data);

    if(p_buf != NULL) {
        p_buf->layer_specific = BTA_HH_RPTT_OUTPUT;
        bda = (BD_ADDR *)(&p_dev->bd_addr);
        BTA_HhSendData(p_dev->dev_handle, *bda, p_buf);
    }
}

/*******************************************************************************
**
** Function         sync_lockstate_on_connect
**
** Description      Function to update the keyboard lock states managed by the OS
**                  when a HID keyboard is connected or disconnected and reconnected
** Returns          void
*******************************************************************************/
static void sync_lockstate_on_connect(btif_hh_device_t *p_dev)
{
    int keylockstates;
    BTIF_TRACE_EVENT("%s: Syncing keyboard lock states after "\
                     "reconnect...", __FUNCTION__);
    /*If the device is connected, update keyboard state */
    update_keyboard_lockstates(p_dev);
    /*Check if the lockstate of caps,scroll,num is set.
     If so, send a report to the kernel
    so the lockstate is in sync */
    keylockstates = get_keylockstates();

    if(keylockstates) {
        BTIF_TRACE_DEBUG("%s: Sending hid report to kernel "\
                         "indicating lock key state 0x%x", __FUNCTION__,
                         keylockstates);
        usleep(200000);
        toggle_os_keylockstates(p_dev->fd, keylockstates);
    } else {
        BTIF_TRACE_DEBUG("%s: NOT sending hid report to kernel "\
                         "indicating lock key state 0x%x", __FUNCTION__,
                         keylockstates);
    }
}

/*******************************************************************************
**
** Function         btif_hh_find_connected_dev_by_handle
**
** Description      Return the connected device pointer of the specified device handle
**
** Returns          Device entry pointer in the device table
*******************************************************************************/
btif_hh_device_t *btif_hh_find_connected_dev_by_handle(uint8_t handle)
{
    uint32_t i;

    for(i = 0; i < BTIF_HH_MAX_HID; i++) {
        if(btif_hh_cb.devices[i].dev_status == BTHH_CONN_STATE_CONNECTED &&
                btif_hh_cb.devices[i].dev_handle == handle) {
            return &btif_hh_cb.devices[i];
        }
    }

    return NULL;
}

/*******************************************************************************
**
** Function         btif_hh_find_dev_by_bda
**
** Description      Return the device pointer of the specified bt_bdaddr_t.
**
** Returns          Device entry pointer in the device table
*******************************************************************************/
static btif_hh_device_t *btif_hh_find_dev_by_bda(tls_bt_addr_t *bd_addr)
{
    uint32_t i;

    for(i = 0; i < BTIF_HH_MAX_HID; i++) {
        if(btif_hh_cb.devices[i].dev_status != BTHH_CONN_STATE_UNKNOWN &&
                memcmp(&(btif_hh_cb.devices[i].bd_addr), bd_addr, BD_ADDR_LEN) == 0) {
            return &btif_hh_cb.devices[i];
        }
    }

    return NULL;
}

/*******************************************************************************
**
** Function         btif_hh_find_connected_dev_by_bda
**
** Description      Return the connected device pointer of the specified bt_bdaddr_t.
**
** Returns          Device entry pointer in the device table
*******************************************************************************/
static btif_hh_device_t *btif_hh_find_connected_dev_by_bda(tls_bt_addr_t *bd_addr)
{
    uint32_t i;

    for(i = 0; i < BTIF_HH_MAX_HID; i++) {
        if(btif_hh_cb.devices[i].dev_status == BTHH_CONN_STATE_CONNECTED &&
                memcmp(&(btif_hh_cb.devices[i].bd_addr), bd_addr, BD_ADDR_LEN) == 0) {
            return &btif_hh_cb.devices[i];
        }
    }

    return NULL;
}

/*******************************************************************************
**
** Function      btif_hh_stop_vup_timer
**
** Description  stop vitual unplug timer
**
** Returns      void
*******************************************************************************/
void btif_hh_stop_vup_timer(tls_bt_addr_t *bd_addr)
{
    btif_hh_device_t *p_dev = btif_hh_find_connected_dev_by_bda(bd_addr);

    if(p_dev != NULL) {
        BTIF_TRACE_DEBUG("stop VUP timer");
        alarm_free(p_dev->vup_timer);
        p_dev->vup_timer = NULL;
    }
}
/*******************************************************************************
**
** Function      btif_hh_start_vup_timer
**
** Description  start virtual unplug timer
**
** Returns      void
*******************************************************************************/
void btif_hh_start_vup_timer(tls_bt_addr_t *bd_addr)
{
    BTIF_TRACE_DEBUG("%s", __func__);
    btif_hh_device_t *p_dev  = btif_hh_find_connected_dev_by_bda(bd_addr);
    assert(p_dev != NULL);
    alarm_free(p_dev->vup_timer);
    p_dev->vup_timer = alarm_new("btif_hh.vup_timer");
    alarm_set_on_queue(p_dev->vup_timer, BTIF_TIMEOUT_VUP_MS,
                       btif_hh_timer_timeout, p_dev,
                       btu_general_alarm_queue);
}

/*******************************************************************************
**
** Function         btif_hh_add_added_dev
**
** Description      Add a new device to the added device list.
**
** Returns          TRUE if add successfully, otherwise FALSE.
*******************************************************************************/
uint8_t btif_hh_add_added_dev(tls_bt_addr_t bda, tBTA_HH_ATTR_MASK attr_mask)
{
    int i;

    for(i = 0; i < BTIF_HH_MAX_ADDED_DEV; i++) {
        if(memcmp(&(btif_hh_cb.added_devices[i].bd_addr), &bda, BD_ADDR_LEN) == 0) {
            BTIF_TRACE_WARNING(" Device %02X:%02X:%02X:%02X:%02X:%02X already added",
                               bda.address[0], bda.address[1], bda.address[2], bda.address[3], bda.address[4], bda.address[5]);
            return FALSE;
        }
    }

    for(i = 0; i < BTIF_HH_MAX_ADDED_DEV; i++) {
        if(btif_hh_cb.added_devices[i].bd_addr.address[0] == 0 &&
                btif_hh_cb.added_devices[i].bd_addr.address[1] == 0 &&
                btif_hh_cb.added_devices[i].bd_addr.address[2] == 0 &&
                btif_hh_cb.added_devices[i].bd_addr.address[3] == 0 &&
                btif_hh_cb.added_devices[i].bd_addr.address[4] == 0 &&
                btif_hh_cb.added_devices[i].bd_addr.address[5] == 0) {
            BTIF_TRACE_WARNING(" Added device %02X:%02X:%02X:%02X:%02X:%02X",
                               bda.address[0], bda.address[1], bda.address[2], bda.address[3], bda.address[4], bda.address[5]);
            wm_memcpy(&(btif_hh_cb.added_devices[i].bd_addr), &bda, BD_ADDR_LEN);
            btif_hh_cb.added_devices[i].dev_handle = BTA_HH_INVALID_HANDLE;
            btif_hh_cb.added_devices[i].attr_mask  = attr_mask;
            return TRUE;
        }
    }

    BTIF_TRACE_WARNING("%s: Error, out of space to add device", __FUNCTION__);
    return FALSE;
}

/*******************************************************************************
 **
 ** Function         btif_hh_remove_device
 **
 ** Description      Remove an added device from the stack.
 **
 ** Returns          void
 *******************************************************************************/
void btif_hh_remove_device(tls_bt_addr_t bd_addr)
{
    int                    i;
    btif_hh_device_t       *p_dev;
    btif_hh_added_device_t *p_added_dev;
    LOG_INFO(LOG_TAG, "%s: bda = %02x:%02x:%02x:%02x:%02x:%02x", __FUNCTION__,
             bd_addr.address[0], bd_addr.address[1], bd_addr.address[2], bd_addr.address[3], bd_addr.address[4],
             bd_addr.address[5]);

    for(i = 0; i < BTIF_HH_MAX_ADDED_DEV; i++) {
        p_added_dev = &btif_hh_cb.added_devices[i];

        if(memcmp(&(p_added_dev->bd_addr), &bd_addr, 6) == 0) {
            BTA_HhRemoveDev(p_added_dev->dev_handle);
            btif_storage_remove_hid_info(&(p_added_dev->bd_addr));
            wm_memset(&(p_added_dev->bd_addr), 0, 6);
            p_added_dev->dev_handle = BTA_HH_INVALID_HANDLE;
            break;
        }
    }

    p_dev = btif_hh_find_dev_by_bda(&bd_addr);

    if(p_dev == NULL) {
        BTIF_TRACE_WARNING(" Oops, can't find device [%02x:%02x:%02x:%02x:%02x:%02x]",
                           bd_addr.address[0], bd_addr.address[1], bd_addr.address[2], bd_addr.address[3], bd_addr.address[4],
                           bd_addr.address[5]);
        return;
    }

    /* need to notify up-layer device is disconnected to avoid state out of sync with up-layer */
    HAL_CBACK(bt_hh_callbacks, connection_state_cb, &(p_dev->bd_addr), BTHH_CONN_STATE_DISCONNECTED);
    p_dev->dev_status = BTHH_CONN_STATE_UNKNOWN;
    p_dev->dev_handle = BTA_HH_INVALID_HANDLE;
    p_dev->ready_for_data = FALSE;

    if(btif_hh_cb.device_num > 0) {
        btif_hh_cb.device_num--;
    } else {
        BTIF_TRACE_WARNING("%s: device_num = 0", __FUNCTION__);
    }

    p_dev->hh_keep_polling = 0;
    p_dev->hh_poll_thread_id = -1;
    BTIF_TRACE_DEBUG("%s: uhid fd = %d", __FUNCTION__, p_dev->fd);

    if(p_dev->fd >= 0) {
        bta_hh_co_destroy(p_dev->fd);
        p_dev->fd = -1;
    }
}

uint8_t btif_hh_copy_hid_info(tBTA_HH_DEV_DSCP_INFO *dest, tBTA_HH_DEV_DSCP_INFO *src)
{
    dest->descriptor.dl_len = 0;

    if(src->descriptor.dl_len > 0) {
        dest->descriptor.dsc_list = (uint8_t *)GKI_getbuf(src->descriptor.dl_len);
    }

    wm_memcpy(dest->descriptor.dsc_list, src->descriptor.dsc_list, src->descriptor.dl_len);
    dest->descriptor.dl_len = src->descriptor.dl_len;
    dest->vendor_id  = src->vendor_id;
    dest->product_id = src->product_id;
    dest->version    = src->version;
    dest->ctry_code  = src->ctry_code;
    dest->ssr_max_latency = src->ssr_max_latency;
    dest->ssr_min_tout = src->ssr_min_tout;
    return TRUE;
}

/*******************************************************************************
**
** Function         btif_hh_virtual_unplug
**
** Description      Virtual unplug initiated from the BTIF thread context
**                  Special handling for HID mouse-
**
** Returns          void
**
*******************************************************************************/

tls_bt_status_t btif_hh_virtual_unplug(tls_bt_addr_t *bd_addr)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    btif_hh_device_t *p_dev;
    char bd_str[18];
    sprintf(bd_str, "%02X:%02X:%02X:%02X:%02X:%02X",
            bd_addr->address[0],  bd_addr->address[1],  bd_addr->address[2],  bd_addr->address[3],
            bd_addr->address[4], bd_addr->address[5]);
    p_dev = btif_hh_find_dev_by_bda(bd_addr);

    if((p_dev != NULL) && (p_dev->dev_status == BTHH_CONN_STATE_CONNECTED)
            && (p_dev->attr_mask & HID_VIRTUAL_CABLE)) {
        BTIF_TRACE_DEBUG("%s Sending BTA_HH_CTRL_VIRTUAL_CABLE_UNPLUG", __FUNCTION__);
        /* start the timer */
        btif_hh_start_vup_timer(bd_addr);
        p_dev->local_vup = TRUE;
        BTA_HhSendCtrl(p_dev->dev_handle, BTA_HH_CTRL_VIRTUAL_CABLE_UNPLUG);
        return TLS_BT_STATUS_SUCCESS;
    } else {
        BTIF_TRACE_ERROR("%s: Error, device %s not opened.", __FUNCTION__, bd_str);
        return TLS_BT_STATUS_FAIL;
    }
}

/*******************************************************************************
**
** Function         btif_hh_connect
**
** Description      connection initiated from the BTIF thread context
**
** Returns          int status
**
*******************************************************************************/

tls_bt_status_t btif_hh_connect(tls_bt_addr_t *bd_addr)
{
    btif_hh_device_t *dev;
    btif_hh_added_device_t *added_dev = NULL;
    char bda_str[20];
    int i;
    BD_ADDR *bda = (BD_ADDR *)bd_addr;
    CHECK_BTHH_INIT();
    dev = btif_hh_find_dev_by_bda(bd_addr);
    BTIF_TRACE_DEBUG("Connect _hh");
    sprintf(bda_str, "%02X:%02X:%02X:%02X:%02X:%02X",
            (*bda)[0], (*bda)[1], (*bda)[2], (*bda)[3], (*bda)[4], (*bda)[5]);

    if(dev == NULL && btif_hh_cb.device_num >= BTIF_HH_MAX_HID) {
        // No space for more HID device now.
        BTIF_TRACE_WARNING("%s: Error, exceeded the maximum supported HID device number %d",
                           __FUNCTION__, BTIF_HH_MAX_HID);
        return TLS_BT_STATUS_FAIL;
    }

    for(i = 0; i < BTIF_HH_MAX_ADDED_DEV; i++) {
        if(memcmp(&(btif_hh_cb.added_devices[i].bd_addr), bd_addr, BD_ADDR_LEN) == 0) {
            added_dev = &btif_hh_cb.added_devices[i];
            BTIF_TRACE_WARNING("%s: Device %s already added, attr_mask = 0x%x",
                               __FUNCTION__, bda_str, added_dev->attr_mask);
        }
    }

    if(added_dev != NULL) {
        if(added_dev->dev_handle == BTA_HH_INVALID_HANDLE) {
            // No space for more HID device now.
            BTIF_TRACE_ERROR("%s: Error, device %s added but addition failed", __FUNCTION__, bda_str);
            wm_memset(&(added_dev->bd_addr), 0, 6);
            added_dev->dev_handle = BTA_HH_INVALID_HANDLE;
            return TLS_BT_STATUS_FAIL;
        }
    }

    /* Not checking the NORMALLY_Connectible flags from sdp record, and anyways sending this
     request from host, for subsequent user initiated connection. If the remote is not in
     pagescan mode, we will do 2 retries to connect before giving up */
    tBTA_SEC sec_mask = BTUI_HH_SECURITY;
    btif_hh_cb.status = BTIF_HH_DEV_CONNECTING;
    BTA_HhOpen(*bda, BTA_HH_PROTO_RPT_MODE, sec_mask);
    HAL_CBACK(bt_hh_callbacks, connection_state_cb, bd_addr, BTHH_CONN_STATE_CONNECTING);
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_hh_disconnect
**
** Description      disconnection initiated from the BTIF thread context
**
** Returns          void
**
*******************************************************************************/

void btif_hh_disconnect(tls_bt_addr_t *bd_addr)
{
    btif_hh_device_t *p_dev;
    p_dev = btif_hh_find_connected_dev_by_bda(bd_addr);

    if(p_dev != NULL) {
        BTA_HhClose(p_dev->dev_handle);
    } else {
        BTIF_TRACE_DEBUG("%s-- Error: device not connected:", __FUNCTION__);
    }
}

/*******************************************************************************
**
** Function         btif_btif_hh_setreport
**
** Description      setreport initiated from the BTIF thread context
**
** Returns          void
**
*******************************************************************************/
void btif_hh_setreport(btif_hh_device_t *p_dev, bthh_report_type_t r_type, uint16_t size,
                       uint8_t *report)
{
    BT_HDR *p_buf = create_pbuf(size, report);

    if(p_buf == NULL) {
        APPL_TRACE_ERROR("%s: Error, failed to allocate RPT buffer, size = %d", __FUNCTION__, size);
        return;
    }

    BTA_HhSetReport(p_dev->dev_handle, r_type, p_buf);
}

/*****************************************************************************
**   Section name (Group of functions)
*****************************************************************************/

/*****************************************************************************
**
**   btif hh api functions (no context switch)
**
*****************************************************************************/

/*******************************************************************************
**
** Function         btif_hh_upstreams_evt
**
** Description      Executes HH UPSTREAMS events in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_hh_upstreams_evt(uint16_t event, char *p_param)
{
    tBTA_HH *p_data = (tBTA_HH *)p_param;
    btif_hh_device_t *p_dev = NULL;
    int i;
    int len, tmplen;
    BTIF_TRACE_DEBUG("%s: event=%s", __FUNCTION__, dump_hh_event(event));

    switch(event) {
        case BTA_HH_ENABLE_EVT:
            BTIF_TRACE_DEBUG("%s: BTA_HH_ENABLE_EVT: status =%d", __FUNCTION__, p_data->status);

            if(p_data->status == BTA_HH_OK) {
                btif_hh_cb.status = BTIF_HH_ENABLED;
                BTIF_TRACE_DEBUG("%s--Loading added devices", __FUNCTION__);
                /* Add hid descriptors for already bonded hid devices*/
                btif_storage_load_bonded_hid_info();
            } else {
                btif_hh_cb.status = BTIF_HH_DISABLED;
                BTIF_TRACE_WARNING("BTA_HH_ENABLE_EVT: Error, HH enabling failed, status = %d", p_data->status);
            }

            break;

        case BTA_HH_DISABLE_EVT:
            btif_hh_cb.status = BTIF_HH_DISABLED;

            if(p_data->status == BTA_HH_OK) {
                int i;

                // Clear the control block
                for(i = 0; i < BTIF_HH_MAX_HID; i++) {
                    alarm_free(btif_hh_cb.devices[i].vup_timer);
                }

                wm_memset(&btif_hh_cb, 0, sizeof(btif_hh_cb));

                for(i = 0; i < BTIF_HH_MAX_HID; i++) {
                    btif_hh_cb.devices[i].dev_status = BTHH_CONN_STATE_UNKNOWN;
                }
            } else {
                BTIF_TRACE_WARNING("BTA_HH_DISABLE_EVT: Error, HH disabling failed, status = %d", p_data->status);
            }

            break;

        case BTA_HH_OPEN_EVT:
            BTIF_TRACE_WARNING("%s: BTA_HH_OPN_EVT: handle=%d, status =%d", __FUNCTION__, p_data->conn.handle,
                               p_data->conn.status);

            if(p_data->conn.status == BTA_HH_OK) {
                p_dev = btif_hh_find_connected_dev_by_handle(p_data->conn.handle);

                if(p_dev == NULL) {
                    BTIF_TRACE_WARNING("BTA_HH_OPEN_EVT: Error, cannot find device with handle %d",
                                       p_data->conn.handle);
                    btif_hh_cb.status = BTIF_HH_DEV_DISCONNECTED;
                    // The connect request must come from device side and exceeded the connected
                    // HID device number.
                    BTA_HhClose(p_data->conn.handle);
                    HAL_CBACK(bt_hh_callbacks, connection_state_cb, (tls_bt_addr_t *) &p_data->conn.bda,
                              BTHH_CONN_STATE_DISCONNECTED);
                } else if(p_dev->fd < 0) {
                    BTIF_TRACE_WARNING("BTA_HH_OPEN_EVT: Error, failed to find the uhid driver...");
                    wm_memcpy(&(p_dev->bd_addr), p_data->conn.bda, BD_ADDR_LEN);
                    //remove the connection  and then try again to reconnect from the mouse side to recover
                    btif_hh_cb.status = BTIF_HH_DEV_DISCONNECTED;
                    BTA_HhClose(p_data->conn.handle);
                } else {
                    BTIF_TRACE_WARNING("BTA_HH_OPEN_EVT: Found device...Getting dscp info for handle ... %d",
                                       p_data->conn.handle);
                    wm_memcpy(&(p_dev->bd_addr), p_data->conn.bda, BD_ADDR_LEN);
                    btif_hh_cb.status = BTIF_HH_DEV_CONNECTED;

                    // Send set_idle if the peer_device is a keyboard
                    if(check_cod((tls_bt_addr_t *)p_data->conn.bda, COD_HID_KEYBOARD) ||
                            check_cod((tls_bt_addr_t *)p_data->conn.bda, COD_HID_COMBO)) {
                        BTA_HhSetIdle(p_data->conn.handle, 0);
                    }

                    btif_hh_cb.p_curr_dev = btif_hh_find_connected_dev_by_handle(p_data->conn.handle);
                    BTA_HhGetDscpInfo(p_data->conn.handle);
                    p_dev->dev_status = BTHH_CONN_STATE_CONNECTED;
                    HAL_CBACK(bt_hh_callbacks, connection_state_cb, &(p_dev->bd_addr), p_dev->dev_status);
                }
            } else {
                tls_bt_addr_t *bdaddr = (tls_bt_addr_t *)p_data->conn.bda;
                btif_dm_hh_open_failed(bdaddr);
                p_dev = btif_hh_find_dev_by_bda(bdaddr);

                if(p_dev != NULL) {
                    btif_hh_stop_vup_timer(&(p_dev->bd_addr));

                    if(p_dev->fd >= 0) {
                        bta_hh_co_destroy(p_dev->fd);
                        p_dev->fd = -1;
                    }

                    p_dev->dev_status = BTHH_CONN_STATE_DISCONNECTED;
                }

                HAL_CBACK(bt_hh_callbacks, connection_state_cb, (tls_bt_addr_t *) &p_data->conn.bda,
                          BTHH_CONN_STATE_DISCONNECTED);
                btif_hh_cb.status = BTIF_HH_DEV_DISCONNECTED;
            }

            break;

        case BTA_HH_CLOSE_EVT:
            BTIF_TRACE_DEBUG("BTA_HH_CLOSE_EVT: status = %d, handle = %d",
                             p_data->dev_status.status, p_data->dev_status.handle);
            p_dev = btif_hh_find_connected_dev_by_handle(p_data->dev_status.handle);

            if(p_dev != NULL) {
                BTIF_TRACE_DEBUG("%s: uhid fd=%d local_vup=%d", __func__, p_dev->fd, p_dev->local_vup);
                btif_hh_stop_vup_timer(&(p_dev->bd_addr));

                /* If this is a locally initiated VUP, remove the bond as ACL got
                 *  disconnected while VUP being processed.
                 */
                if(p_dev->local_vup) {
                    p_dev->local_vup = FALSE;
                    BTA_DmRemoveDevice((uint8_t *)p_dev->bd_addr.address);
                }

                btif_hh_cb.status = BTIF_HH_DEV_DISCONNECTED;
                p_dev->dev_status = BTHH_CONN_STATE_DISCONNECTED;

                if(p_dev->fd >= 0) {
                    bta_hh_co_destroy(p_dev->fd);
                    p_dev->fd = -1;
                }

                HAL_CBACK(bt_hh_callbacks, connection_state_cb, &(p_dev->bd_addr), p_dev->dev_status);
            } else {
                BTIF_TRACE_WARNING("Error: cannot find device with handle %d", p_data->dev_status.handle);
            }

            break;

        case BTA_HH_GET_RPT_EVT: {
            BT_HDR *hdr = p_data->hs_data.rsp_data.p_rpt_data;
            uint8_t *data = NULL;
            uint16_t len = 0;
            BTIF_TRACE_DEBUG("BTA_HH_GET_RPT_EVT: status = %d, handle = %d",
                             p_data->hs_data.status, p_data->hs_data.handle);
            p_dev = btif_hh_find_connected_dev_by_handle(p_data->hs_data.handle);

            if(p_dev) {
                /* p_rpt_data is NULL in HANDSHAKE response case */
                if(hdr) {
                    data = (uint8_t *)(hdr + 1) + hdr->offset;
                    len = hdr->len;
                    HAL_CBACK(bt_hh_callbacks, get_report_cb,
                              (tls_bt_addr_t *) & (p_dev->bd_addr),
                              (bthh_status_t) p_data->hs_data.status, data, len);
                } else {
                    HAL_CBACK(bt_hh_callbacks, handshake_cb,
                              (tls_bt_addr_t *) & (p_dev->bd_addr),
                              (bthh_status_t) p_data->hs_data.status);
                }
            } else {
                BTIF_TRACE_WARNING("Error: cannot find device with handle %d", p_data->hs_data.handle);
            }

            break;
        }

        case BTA_HH_SET_RPT_EVT:
            BTIF_TRACE_DEBUG("BTA_HH_SET_RPT_EVT: status = %d, handle = %d",
                             p_data->dev_status.status, p_data->dev_status.handle);
            p_dev = btif_hh_find_connected_dev_by_handle(p_data->dev_status.handle);

            if(p_dev != NULL) {
                HAL_CBACK(bt_hh_callbacks, handshake_cb,
                          (tls_bt_addr_t *) & (p_dev->bd_addr),
                          (bthh_status_t) p_data->hs_data.status);
            }

            break;

        case BTA_HH_GET_PROTO_EVT:
            p_dev = btif_hh_find_connected_dev_by_handle(p_data->dev_status.handle);
            BTIF_TRACE_WARNING("BTA_HH_GET_PROTO_EVT: status = %d, handle = %d, proto = [%d], %s",
                               p_data->hs_data.status, p_data->hs_data.handle,
                               p_data->hs_data.rsp_data.proto_mode,
                               (p_data->hs_data.rsp_data.proto_mode == BTA_HH_PROTO_RPT_MODE) ? "Report Mode" :
                               (p_data->hs_data.rsp_data.proto_mode == BTA_HH_PROTO_BOOT_MODE) ? "Boot Mode" : "Unsupported");

            if(p_data->hs_data.rsp_data.proto_mode != BTA_HH_PROTO_UNKNOWN) {
                HAL_CBACK(bt_hh_callbacks, protocol_mode_cb,
                          (tls_bt_addr_t *) & (p_dev->bd_addr),
                          (bthh_status_t)p_data->hs_data.status,
                          (bthh_protocol_mode_t) p_data->hs_data.rsp_data.proto_mode);
            } else {
                HAL_CBACK(bt_hh_callbacks, handshake_cb,
                          (tls_bt_addr_t *) & (p_dev->bd_addr),
                          (bthh_status_t)p_data->hs_data.status);
            }

            break;

        case BTA_HH_SET_PROTO_EVT:
            BTIF_TRACE_DEBUG("BTA_HH_SET_PROTO_EVT: status = %d, handle = %d",
                             p_data->dev_status.status, p_data->dev_status.handle);
            p_dev = btif_hh_find_connected_dev_by_handle(p_data->dev_status.handle);

            if(p_dev) {
                HAL_CBACK(bt_hh_callbacks, handshake_cb,
                          (tls_bt_addr_t *) & (p_dev->bd_addr),
                          (bthh_status_t)p_data->hs_data.status);
            }

            break;

        case BTA_HH_GET_IDLE_EVT:
            BTIF_TRACE_DEBUG("BTA_HH_GET_IDLE_EVT: handle = %d, status = %d, rate = %d",
                             p_data->hs_data.handle, p_data->hs_data.status,
                             p_data->hs_data.rsp_data.idle_rate);
            break;

        case BTA_HH_SET_IDLE_EVT:
            BTIF_TRACE_DEBUG("BTA_HH_SET_IDLE_EVT: status = %d, handle = %d",
                             p_data->dev_status.status, p_data->dev_status.handle);
            break;

        case BTA_HH_GET_DSCP_EVT:
            len = p_data->dscp_info.descriptor.dl_len;
            BTIF_TRACE_DEBUG("BTA_HH_GET_DSCP_EVT: len = %d", len);
            p_dev = btif_hh_cb.p_curr_dev;

            if(p_dev == NULL) {
                BTIF_TRACE_ERROR("BTA_HH_GET_DSCP_EVT: No HID device is currently connected");
                return;
            }

            if(p_dev->fd < 0) {
                LOG_ERROR(LOG_TAG, "BTA_HH_GET_DSCP_EVT: Error, failed to find the uhid driver...");
                return;
            }

            {
                char *cached_name = NULL;
                tls_bt_bdname_t bdname;
                tls_bt_property_t prop_name;
                BTIF_STORAGE_FILL_PROPERTY(&prop_name, WM_BT_PROPERTY_BDNAME,
                                           sizeof(tls_bt_bdname_t), &bdname);

                if(btif_storage_get_remote_device_property(
                                        &p_dev->bd_addr, &prop_name) == TLS_BT_STATUS_SUCCESS) {
                    cached_name = (char *)bdname.name;
                } else {
                    cached_name = "Bluetooth HID";
                }

                BTIF_TRACE_WARNING("%s: name = %s", __FUNCTION__, cached_name);
                bta_hh_co_send_hid_info(p_dev, cached_name,
                                        p_data->dscp_info.vendor_id, p_data->dscp_info.product_id,
                                        p_data->dscp_info.version,   p_data->dscp_info.ctry_code,
                                        len, p_data->dscp_info.descriptor.dsc_list);

                if(btif_hh_add_added_dev(p_dev->bd_addr, p_dev->attr_mask)) {
                    BD_ADDR bda;
                    bdcpy(bda, p_dev->bd_addr.address);
                    tBTA_HH_DEV_DSCP_INFO dscp_info;
                    tls_bt_status_t ret;
                    bdcpy(bda, p_dev->bd_addr.address);
                    btif_hh_copy_hid_info(&dscp_info, &p_data->dscp_info);
                    BTIF_TRACE_DEBUG("BTA_HH_GET_DSCP_EVT:bda = %02x:%02x:%02x:%02x:%02x:%02x",
                                     p_dev->bd_addr.address[0], p_dev->bd_addr.address[1],
                                     p_dev->bd_addr.address[2], p_dev->bd_addr.address[3],
                                     p_dev->bd_addr.address[4], p_dev->bd_addr.address[5]);
                    BTA_HhAddDev(bda, p_dev->attr_mask, p_dev->sub_class, p_dev->app_id, dscp_info);
                    // write hid info to nvram
                    ret = btif_storage_add_hid_device_info(&(p_dev->bd_addr), p_dev->attr_mask, p_dev->sub_class,
                                                           p_dev->app_id,
                                                           p_data->dscp_info.vendor_id, p_data->dscp_info.product_id,
                                                           p_data->dscp_info.version,   p_data->dscp_info.ctry_code,
                                                           p_data->dscp_info.ssr_max_latency, p_data->dscp_info.ssr_min_tout,
                                                           len, p_data->dscp_info.descriptor.dsc_list);
                    ASSERTC(ret == TLS_BT_STATUS_SUCCESS, "storing hid info failed", ret);
                    BTIF_TRACE_WARNING("BTA_HH_GET_DSCP_EVT: Called add device");

                    //Free buffer created for dscp_info;
                    if(dscp_info.descriptor.dl_len > 0 && dscp_info.descriptor.dsc_list != NULL) {
                        GKI_free_and_reset_buf((void **)&dscp_info.descriptor.dsc_list);
                        dscp_info.descriptor.dl_len = 0;
                    }
                } else {
                    //Device already added.
                    BTIF_TRACE_WARNING("%s: Device already added ", __FUNCTION__);
                }

                /*Sync HID Keyboard lockstates */
                tmplen = sizeof(hid_kb_numlock_on_list)
                         / sizeof(tHID_KB_LIST);

                for(i = 0; i < tmplen; i++) {
                    if(p_data->dscp_info.vendor_id
                            == hid_kb_numlock_on_list[i].version_id &&
                            p_data->dscp_info.product_id
                            == hid_kb_numlock_on_list[i].product_id) {
                        BTIF_TRACE_DEBUG("%s() idx[%d] Enabling "\
                                         "NUMLOCK for device :: %s", __FUNCTION__,
                                         i, hid_kb_numlock_on_list[i].kb_name);
                        /* Enable NUMLOCK by default so that numeric
                            keys work from first keyboard connect */
                        set_keylockstate(BTIF_HH_KEYSTATE_MASK_NUMLOCK,
                                         TRUE);
                        sync_lockstate_on_connect(p_dev);
                        /* End Sync HID Keyboard lockstates */
                        break;
                    }
                }
            }

            break;

        case BTA_HH_ADD_DEV_EVT:
            BTIF_TRACE_WARNING("BTA_HH_ADD_DEV_EVT: status = %d, handle = %d", p_data->dev_info.status,
                               p_data->dev_info.handle);
            int i;

            for(i = 0; i < BTIF_HH_MAX_ADDED_DEV; i++) {
                if(memcmp(btif_hh_cb.added_devices[i].bd_addr.address, p_data->dev_info.bda, 6) == 0) {
                    if(p_data->dev_info.status == BTA_HH_OK) {
                        btif_hh_cb.added_devices[i].dev_handle = p_data->dev_info.handle;
                    } else {
                        wm_memset(btif_hh_cb.added_devices[i].bd_addr.address, 0, 6);
                        btif_hh_cb.added_devices[i].dev_handle = BTA_HH_INVALID_HANDLE;
                    }

                    break;
                }
            }

            break;

        case BTA_HH_RMV_DEV_EVT:
            BTIF_TRACE_DEBUG("BTA_HH_RMV_DEV_EVT: status = %d, handle = %d",
                             p_data->dev_info.status, p_data->dev_info.handle);
            BTIF_TRACE_DEBUG("BTA_HH_RMV_DEV_EVT:bda = %02x:%02x:%02x:%02x:%02x:%02x",
                             p_data->dev_info.bda[0], p_data->dev_info.bda[1], p_data->dev_info.bda[2],
                             p_data->dev_info.bda[3], p_data->dev_info.bda[4], p_data->dev_info.bda[5]);
            break;

        case BTA_HH_VC_UNPLUG_EVT:
            BTIF_TRACE_DEBUG("BTA_HH_VC_UNPLUG_EVT: status = %d, handle = %d",
                             p_data->dev_status.status, p_data->dev_status.handle);
            p_dev = btif_hh_find_connected_dev_by_handle(p_data->dev_status.handle);
            btif_hh_cb.status = BTIF_HH_DEV_DISCONNECTED;

            if(p_dev != NULL) {
                BTIF_TRACE_DEBUG("BTA_HH_VC_UNPLUG_EVT:bda = %02x:%02x:%02x:%02x:%02x:%02x",
                                 p_dev->bd_addr.address[0], p_dev->bd_addr.address[1],
                                 p_dev->bd_addr.address[2], p_dev->bd_addr.address[3],
                                 p_dev->bd_addr.address[4], p_dev->bd_addr.address[5]);
                /* Stop the VUP timer */
                btif_hh_stop_vup_timer(&(p_dev->bd_addr));
                p_dev->dev_status = BTHH_CONN_STATE_DISCONNECTED;
                BTIF_TRACE_DEBUG("%s---Sending connection state change", __FUNCTION__);
                HAL_CBACK(bt_hh_callbacks, connection_state_cb, &(p_dev->bd_addr), p_dev->dev_status);
                BTIF_TRACE_DEBUG("%s---Removing HID bond", __FUNCTION__);

                /* If it is locally initiated VUP or remote device has its major COD as
                Peripheral removed the bond.*/
                if(p_dev->local_vup  || check_cod_hid(&(p_dev->bd_addr))) {
                    p_dev->local_vup = FALSE;
                    BTA_DmRemoveDevice((uint8_t *)p_dev->bd_addr.address);
                } else {
                    btif_hh_remove_device(p_dev->bd_addr);
                }

                HAL_CBACK(bt_hh_callbacks, virtual_unplug_cb, &(p_dev->bd_addr),
                          p_data->dev_status.status);
            }

            break;

        case BTA_HH_API_ERR_EVT  :
            LOG_INFO(LOG_TAG, "BTA_HH API_ERR");
            break;

        default:
            BTIF_TRACE_WARNING("%s: Unhandled event: %d", __FUNCTION__, event);
            break;
    }
}

/*******************************************************************************
**
** Function         bte_hh_evt
**
** Description      Switches context from BTE to BTIF for all HH events
**
** Returns          void
**
*******************************************************************************/

static void bte_hh_evt(tBTA_HH_EVT event, tBTA_HH *p_data)
{
    tls_bt_status_t status;
    int param_len = 0;

    if(BTA_HH_ENABLE_EVT == event) {
        param_len = sizeof(tBTA_HH_STATUS);
    } else if(BTA_HH_OPEN_EVT == event) {
        param_len = sizeof(tBTA_HH_CONN);
    } else if(BTA_HH_DISABLE_EVT == event) {
        param_len = sizeof(tBTA_HH_STATUS);
    } else if(BTA_HH_CLOSE_EVT == event) {
        param_len = sizeof(tBTA_HH_CBDATA);
    } else if(BTA_HH_GET_DSCP_EVT == event) {
        param_len = sizeof(tBTA_HH_DEV_DSCP_INFO);
    } else if((BTA_HH_GET_PROTO_EVT == event) || (BTA_HH_GET_RPT_EVT == event)
              || (BTA_HH_GET_IDLE_EVT == event)) {
        param_len = sizeof(tBTA_HH_HSDATA);
    } else if((BTA_HH_SET_PROTO_EVT == event) || (BTA_HH_SET_RPT_EVT == event)
              || (BTA_HH_VC_UNPLUG_EVT == event) || (BTA_HH_SET_IDLE_EVT == event)) {
        param_len = sizeof(tBTA_HH_CBDATA);
    } else if((BTA_HH_ADD_DEV_EVT == event) || (BTA_HH_RMV_DEV_EVT == event)) {
        param_len = sizeof(tBTA_HH_DEV_INFO);
    } else if(BTA_HH_API_ERR_EVT == event) {
        param_len = 0;
    }

    /* switch context to btif task context (copy full union size for convenience) */
    status = btif_transfer_context(btif_hh_upstreams_evt, (uint16_t)event, (void *)p_data, param_len,
                                   NULL);
    /* catch any failed context transfers */
    ASSERTC(status == TLS_BT_STATUS_SUCCESS, "context transfer failed", status);
}

/*******************************************************************************
**
** Function         btif_hh_handle_evt
**
** Description      Switches context for immediate callback
**
** Returns          void
**
*******************************************************************************/

static void btif_hh_handle_evt(uint16_t event, char *p_param)
{
    tls_bt_addr_t *bd_addr = (tls_bt_addr_t *)p_param;
    BTIF_TRACE_EVENT("%s: event=%d", __FUNCTION__, event);
    int ret;

    switch(event) {
        case BTIF_HH_CONNECT_REQ_EVT: {
            ret = btif_hh_connect(bd_addr);

            if(ret == TLS_BT_STATUS_SUCCESS) {
                HAL_CBACK(bt_hh_callbacks, connection_state_cb, bd_addr, BTHH_CONN_STATE_CONNECTING);
            } else {
                HAL_CBACK(bt_hh_callbacks, connection_state_cb, bd_addr, BTHH_CONN_STATE_DISCONNECTED);
            }
        }
        break;

        case BTIF_HH_DISCONNECT_REQ_EVT: {
            BTIF_TRACE_EVENT("%s: event=%d", __FUNCTION__, event);
            btif_hh_disconnect(bd_addr);
            HAL_CBACK(bt_hh_callbacks, connection_state_cb, bd_addr, BTHH_CONN_STATE_DISCONNECTING);
        }
        break;

        case BTIF_HH_VUP_REQ_EVT: {
            BTIF_TRACE_EVENT("%s: event=%d", __FUNCTION__, event);
            ret = btif_hh_virtual_unplug(bd_addr);
        }
        break;

        default: {
            BTIF_TRACE_WARNING("%s : Unknown event 0x%x", __FUNCTION__, event);
        }
        break;
    }
}

/*******************************************************************************
**
** Function      btif_hh_timer_timeout
**
** Description   Process timer timeout
**
** Returns      void
*******************************************************************************/
void btif_hh_timer_timeout(void *data)
{
    btif_hh_device_t *p_dev = (btif_hh_device_t *)data;
    tBTA_HH_EVT event = BTA_HH_VC_UNPLUG_EVT;
    tBTA_HH p_data;
    int param_len = sizeof(tBTA_HH_CBDATA);
    BTIF_TRACE_DEBUG("%s",  __func__);

    if(p_dev->dev_status != BTHH_CONN_STATE_CONNECTED) {
        return;
    }

    wm_memset(&p_data, 0, sizeof(tBTA_HH));
    p_data.dev_status.status = BTHH_ERR;
    p_data.dev_status.handle = p_dev->dev_handle;
    /* switch context to btif task context */
    btif_transfer_context(btif_hh_upstreams_evt, (uint16_t)event,
                          (char *)&p_data, param_len, NULL);
}

/*******************************************************************************
**
** Function         btif_hh_init
**
** Description     initializes the hh interface
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t init(bthh_callbacks_t *callbacks)
{
    uint32_t i;
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    bt_hh_callbacks = callbacks;
    wm_memset(&btif_hh_cb, 0, sizeof(btif_hh_cb));

    for(i = 0; i < BTIF_HH_MAX_HID; i++) {
        btif_hh_cb.devices[i].dev_status = BTHH_CONN_STATE_UNKNOWN;
    }

    /* Invoke the enable service API to the core to set the appropriate service_id */
    btif_enable_service(BTA_HID_SERVICE_ID);
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function        connect
**
** Description     connect to hid device
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t connect(tls_bt_addr_t *bd_addr)
{
    if(btif_hh_cb.status != BTIF_HH_DEV_CONNECTING) {
        btif_transfer_context(btif_hh_handle_evt, BTIF_HH_CONNECT_REQ_EVT,
                              (char *)bd_addr, sizeof(tls_bt_addr_t), NULL);
        return TLS_BT_STATUS_SUCCESS;
    } else {
        return TLS_BT_STATUS_BUSY;
    }
}

/*******************************************************************************
**
** Function         disconnect
**
** Description      disconnect from hid device
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t disconnect(tls_bt_addr_t *bd_addr)
{
    CHECK_BTHH_INIT();
    btif_hh_device_t *p_dev;

    if(btif_hh_cb.status == BTIF_HH_DISABLED) {
        BTIF_TRACE_WARNING("%s: Error, HH status = %d", __FUNCTION__, btif_hh_cb.status);
        return TLS_BT_STATUS_FAIL;
    }

    p_dev = btif_hh_find_connected_dev_by_bda(bd_addr);

    if(p_dev != NULL) {
        return btif_transfer_context(btif_hh_handle_evt, BTIF_HH_DISCONNECT_REQ_EVT,
                                     (char *)bd_addr, sizeof(tls_bt_addr_t), NULL);
    } else {
        BTIF_TRACE_WARNING("%s: Error, device  not opened.", __FUNCTION__);
        return TLS_BT_STATUS_FAIL;
    }
}

/*******************************************************************************
**
** Function         virtual_unplug
**
** Description      Virtual UnPlug (VUP) the specified HID device.
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t virtual_unplug(tls_bt_addr_t *bd_addr)
{
    CHECK_BTHH_INIT();
    btif_hh_device_t *p_dev;
    char bd_str[18];
    sprintf(bd_str, "%02X:%02X:%02X:%02X:%02X:%02X",
            bd_addr->address[0],  bd_addr->address[1],  bd_addr->address[2],  bd_addr->address[3],
            bd_addr->address[4], bd_addr->address[5]);

    if(btif_hh_cb.status == BTIF_HH_DISABLED) {
        BTIF_TRACE_ERROR("%s: Error, HH status = %d", __FUNCTION__, btif_hh_cb.status);
        return TLS_BT_STATUS_FAIL;
    }

    p_dev = btif_hh_find_dev_by_bda(bd_addr);

    if(!p_dev) {
        BTIF_TRACE_ERROR("%s: Error, device %s not opened.", __FUNCTION__, bd_str);
        return TLS_BT_STATUS_FAIL;
    }

    btif_transfer_context(btif_hh_handle_evt, BTIF_HH_VUP_REQ_EVT,
                          (char *)bd_addr, sizeof(tls_bt_addr_t), NULL);
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         set_info
**
** Description      Set the HID device descriptor for the specified HID device.
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t set_info(tls_bt_addr_t *bd_addr, bthh_hid_info_t hid_info)
{
    CHECK_BTHH_INIT();
    tBTA_HH_DEV_DSCP_INFO dscp_info;
    BD_ADDR *bda = (BD_ADDR *) bd_addr;
    BTIF_TRACE_DEBUG("addr = %02X:%02X:%02X:%02X:%02X:%02X",
                     (*bda)[0], (*bda)[1], (*bda)[2], (*bda)[3], (*bda)[4], (*bda)[5]);
    BTIF_TRACE_DEBUG("%s: sub_class = 0x%02x, app_id = %d, vendor_id = 0x%04x, "
                     "product_id = 0x%04x, version= 0x%04x",
                     __FUNCTION__, hid_info.sub_class,
                     hid_info.app_id, hid_info.vendor_id, hid_info.product_id,
                     hid_info.version);

    if(btif_hh_cb.status == BTIF_HH_DISABLED) {
        BTIF_TRACE_ERROR("%s: Error, HH status = %d", __FUNCTION__, btif_hh_cb.status);
        return TLS_BT_STATUS_FAIL;
    }

    dscp_info.vendor_id  = hid_info.vendor_id;
    dscp_info.product_id = hid_info.product_id;
    dscp_info.version    = hid_info.version;
    dscp_info.ctry_code  = hid_info.ctry_code;
    dscp_info.descriptor.dl_len = hid_info.dl_len;
    dscp_info.descriptor.dsc_list = (uint8_t *)GKI_getbuf(dscp_info.descriptor.dl_len);
    wm_memcpy(dscp_info.descriptor.dsc_list, &(hid_info.dsc_list), hid_info.dl_len);

    if(btif_hh_add_added_dev(*bd_addr, hid_info.attr_mask)) {
        BTA_HhAddDev(*bda, hid_info.attr_mask, hid_info.sub_class,
                     hid_info.app_id, dscp_info);
    }

    GKI_free_and_reset_buf((void **)&dscp_info.descriptor.dsc_list);
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         get_protocol
**
** Description      Get the HID proto mode.
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t get_protocol(tls_bt_addr_t *bd_addr, bthh_protocol_mode_t protocolMode)
{
    CHECK_BTHH_INIT();
    btif_hh_device_t *p_dev;
    BD_ADDR *bda = (BD_ADDR *) bd_addr;
    UNUSED(protocolMode);
    BTIF_TRACE_DEBUG(" addr = %02X:%02X:%02X:%02X:%02X:%02X",
                     (*bda)[0], (*bda)[1], (*bda)[2], (*bda)[3], (*bda)[4], (*bda)[5]);

    if(btif_hh_cb.status == BTIF_HH_DISABLED) {
        BTIF_TRACE_ERROR("%s: Error, HH status = %d", __FUNCTION__, btif_hh_cb.status);
        return TLS_BT_STATUS_FAIL;
    }

    p_dev = btif_hh_find_connected_dev_by_bda(bd_addr);

    if(p_dev != NULL) {
        BTA_HhGetProtoMode(p_dev->dev_handle);
    } else {
        return TLS_BT_STATUS_FAIL;
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         set_protocol
**
** Description      Set the HID proto mode.
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t set_protocol(tls_bt_addr_t *bd_addr, bthh_protocol_mode_t protocolMode)
{
    CHECK_BTHH_INIT();
    btif_hh_device_t *p_dev;
    uint8_t proto_mode = protocolMode;
    BD_ADDR *bda = (BD_ADDR *) bd_addr;
    BTIF_TRACE_DEBUG("%s:proto_mode = %d", __FUNCTION__, protocolMode);
    BTIF_TRACE_DEBUG("addr = %02X:%02X:%02X:%02X:%02X:%02X",
                     (*bda)[0], (*bda)[1], (*bda)[2], (*bda)[3], (*bda)[4], (*bda)[5]);

    if(btif_hh_cb.status == BTIF_HH_DISABLED) {
        BTIF_TRACE_ERROR("%s: Error, HH status = %d", __FUNCTION__, btif_hh_cb.status);
        return TLS_BT_STATUS_FAIL;
    }

    p_dev = btif_hh_find_connected_dev_by_bda(bd_addr);

    if(p_dev == NULL) {
        BTIF_TRACE_WARNING(" Error, device %02X:%02X:%02X:%02X:%02X:%02X not opened.",
                           (*bda)[0], (*bda)[1], (*bda)[2], (*bda)[3], (*bda)[4], (*bda)[5]);
        return TLS_BT_STATUS_FAIL;
    } else if(protocolMode != BTA_HH_PROTO_RPT_MODE && protocolMode != BTA_HH_PROTO_BOOT_MODE) {
        BTIF_TRACE_WARNING("%s: Error, device proto_mode = %d.", __func__, proto_mode);
        return TLS_BT_STATUS_FAIL;
    } else {
        BTA_HhSetProtoMode(p_dev->dev_handle, protocolMode);
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         get_report
**
** Description      Send a GET_REPORT to HID device.
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t get_report(tls_bt_addr_t *bd_addr, bthh_report_type_t reportType,
                                  uint8_t reportId, int bufferSize)
{
    CHECK_BTHH_INIT();
    btif_hh_device_t *p_dev;
    BD_ADDR *bda = (BD_ADDR *) bd_addr;
    BTIF_TRACE_DEBUG("%s: r_type = %d, rpt_id = %d, buf_size = %d", __func__,
                     reportType, reportId, bufferSize);
    BTIF_TRACE_DEBUG("addr = %02X:%02X:%02X:%02X:%02X:%02X",
                     (*bda)[0], (*bda)[1], (*bda)[2], (*bda)[3], (*bda)[4], (*bda)[5]);

    if(btif_hh_cb.status == BTIF_HH_DISABLED) {
        BTIF_TRACE_ERROR("%s: Error, HH status = %d", __FUNCTION__, btif_hh_cb.status);
        return TLS_BT_STATUS_FAIL;
    }

    p_dev = btif_hh_find_connected_dev_by_bda(bd_addr);

    if(p_dev == NULL) {
        BTIF_TRACE_ERROR("%s: Error, device %02X:%02X:%02X:%02X:%02X:%02X not opened.", __func__,
                         (*bda)[0], (*bda)[1], (*bda)[2], (*bda)[3], (*bda)[4], (*bda)[5]);
        return TLS_BT_STATUS_FAIL;
    } else if(((int) reportType) <= BTA_HH_RPTT_RESRV || ((int) reportType) > BTA_HH_RPTT_FEATURE) {
        BTIF_TRACE_ERROR(" Error, device %02X:%02X:%02X:%02X:%02X:%02X not opened.",
                         (*bda)[0], (*bda)[1], (*bda)[2], (*bda)[3], (*bda)[4], (*bda)[5]);
        return TLS_BT_STATUS_FAIL;
    } else {
        BTA_HhGetReport(p_dev->dev_handle, reportType,
                        reportId, bufferSize);
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         set_report
**
** Description      Send a SET_REPORT to HID device.
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t set_report(tls_bt_addr_t *bd_addr, bthh_report_type_t reportType,
                                  char *report)
{
    CHECK_BTHH_INIT();
    btif_hh_device_t *p_dev;
    BD_ADDR *bda = (BD_ADDR *) bd_addr;
    BTIF_TRACE_DEBUG("%s:reportType = %d", __FUNCTION__, reportType);
    BTIF_TRACE_DEBUG("addr = %02X:%02X:%02X:%02X:%02X:%02X",
                     (*bda)[0], (*bda)[1], (*bda)[2], (*bda)[3], (*bda)[4], (*bda)[5]);

    if(btif_hh_cb.status == BTIF_HH_DISABLED) {
        BTIF_TRACE_ERROR("%s: Error, HH status = %d", __FUNCTION__, btif_hh_cb.status);
        return TLS_BT_STATUS_FAIL;
    }

    p_dev = btif_hh_find_connected_dev_by_bda(bd_addr);

    if(p_dev == NULL) {
        BTIF_TRACE_ERROR("%s: Error, device %02X:%02X:%02X:%02X:%02X:%02X not opened.", __func__,
                         (*bda)[0], (*bda)[1], (*bda)[2], (*bda)[3], (*bda)[4], (*bda)[5]);
        return TLS_BT_STATUS_FAIL;
    } else if(((int) reportType) <= BTA_HH_RPTT_RESRV || ((int) reportType) > BTA_HH_RPTT_FEATURE) {
        BTIF_TRACE_ERROR(" Error, device %02X:%02X:%02X:%02X:%02X:%02X not opened.",
                         (*bda)[0], (*bda)[1], (*bda)[2], (*bda)[3], (*bda)[4], (*bda)[5]);
        return TLS_BT_STATUS_FAIL;
    } else {
        int    hex_bytes_filled;
        size_t len = (strlen(report) + 1) / 2;
        uint8_t  *hexbuf = GKI_getbuf(len);
        /* Build a SetReport data buffer */
        //TODO
        hex_bytes_filled = ascii_2_hex(report, len, hexbuf);
        LOG_INFO(LOG_TAG, "Hex bytes filled, hex value: %d", hex_bytes_filled);

        if(hex_bytes_filled) {
            BT_HDR *p_buf = create_pbuf(hex_bytes_filled, hexbuf);

            if(p_buf == NULL) {
                BTIF_TRACE_ERROR("%s: Error, failed to allocate RPT buffer, len = %d",
                                 __FUNCTION__, hex_bytes_filled);
                GKI_freebuf(hexbuf);
                return TLS_BT_STATUS_FAIL;
            }

            BTA_HhSetReport(p_dev->dev_handle, reportType, p_buf);
            GKI_freebuf(hexbuf);
            return TLS_BT_STATUS_SUCCESS;
        }

        GKI_freebuf(hexbuf);
        return TLS_BT_STATUS_FAIL;
    }
}

/*******************************************************************************
**
** Function         send_data
**
** Description      Send a SEND_DATA to HID device.
**
** Returns         bt_status_t
**
*******************************************************************************/
static tls_bt_status_t send_data(tls_bt_addr_t *bd_addr, char *data)
{
    CHECK_BTHH_INIT();
    btif_hh_device_t *p_dev;
    BD_ADDR *bda = (BD_ADDR *) bd_addr;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    BTIF_TRACE_DEBUG("addr = %02X:%02X:%02X:%02X:%02X:%02X",
                     (*bda)[0], (*bda)[1], (*bda)[2], (*bda)[3], (*bda)[4], (*bda)[5]);

    if(btif_hh_cb.status == BTIF_HH_DISABLED) {
        BTIF_TRACE_ERROR("%s: Error, HH status = %d", __FUNCTION__, btif_hh_cb.status);
        return TLS_BT_STATUS_FAIL;
    }

    p_dev = btif_hh_find_connected_dev_by_bda(bd_addr);

    if(p_dev == NULL) {
        BTIF_TRACE_ERROR("%s: Error, device %02X:%02X:%02X:%02X:%02X:%02X not opened.", __func__,
                         (*bda)[0], (*bda)[1], (*bda)[2], (*bda)[3], (*bda)[4], (*bda)[5]);
        return TLS_BT_STATUS_FAIL;
    } else {
        int    hex_bytes_filled;
        size_t len = (strlen(data) + 1) / 2;
        uint8_t *hexbuf = GKI_getbuf(len);
        /* Build a SendData data buffer */
        hex_bytes_filled = ascii_2_hex(data, len, hexbuf);
        BTIF_TRACE_ERROR("Hex bytes filled, hex value: %d, %d", hex_bytes_filled, len);

        if(hex_bytes_filled) {
            BT_HDR *p_buf = create_pbuf(hex_bytes_filled, hexbuf);

            if(p_buf == NULL) {
                BTIF_TRACE_ERROR("%s: Error, failed to allocate RPT buffer, len = %d",
                                 __FUNCTION__, hex_bytes_filled);
                GKI_freebuf(hexbuf);
                return TLS_BT_STATUS_FAIL;
            }

            p_buf->layer_specific = BTA_HH_RPTT_OUTPUT;
            BTA_HhSendData(p_dev->dev_handle, *bda, p_buf);
            GKI_freebuf(hexbuf);
            return TLS_BT_STATUS_SUCCESS;
        }

        GKI_freebuf(hexbuf);
        return TLS_BT_STATUS_FAIL;
    }
}

/*******************************************************************************
**
** Function         cleanup
**
** Description      Closes the HH interface
**
** Returns          bt_status_t
**
*******************************************************************************/
static void  cleanup(void)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    btif_hh_device_t *p_dev;
    int i;

    if(btif_hh_cb.status == BTIF_HH_DISABLED) {
        BTIF_TRACE_WARNING("%s: HH disabling or disabled already, status = %d", __FUNCTION__,
                           btif_hh_cb.status);
        return;
    }

    btif_hh_cb.status = BTIF_HH_DISABLING;

    for(i = 0; i < BTIF_HH_MAX_HID; i++) {
        p_dev = &btif_hh_cb.devices[i];

        if(p_dev->dev_status != BTHH_CONN_STATE_UNKNOWN && p_dev->fd >= 0) {
            BTIF_TRACE_DEBUG("%s: Closing uhid fd = %d", __FUNCTION__, p_dev->fd);

            if(p_dev->fd >= 0) {
                bta_hh_co_destroy(p_dev->fd);
                p_dev->fd = -1;
            }

            p_dev->hh_keep_polling = 0;
            p_dev->hh_poll_thread_id = -1;
        }
    }

    if(bt_hh_callbacks) {
        btif_disable_service(BTA_HID_SERVICE_ID);
        bt_hh_callbacks = NULL;
    }
}

static const bthh_interface_t bthhInterface = {
    sizeof(bthhInterface),
    init,
    connect,
    disconnect,
    virtual_unplug,
    set_info,
    get_protocol,
    set_protocol,
    //    get_idle_time,
    //    set_idle_time,
    get_report,
    set_report,
    send_data,
    cleanup,
};

/*******************************************************************************
**
** Function         btif_hh_execute_service
**
** Description      Initializes/Shuts down the service
**
** Returns          BT_STATUS_SUCCESS on success, BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_hh_execute_service(uint8_t b_enable)
{
    if(b_enable) {
        /* Enable and register with BTA-HH */
        BTA_HhEnable(BTUI_HH_SECURITY, bte_hh_evt);
    } else {
        /* Disable HH */
        BTA_HhDisable();
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_hh_get_interface
**
** Description      Get the hh callback interface
**
** Returns          bthh_interface_t
**
*******************************************************************************/
const bthh_interface_t *btif_hh_get_interface()
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    return &bthhInterface;
}
#endif
