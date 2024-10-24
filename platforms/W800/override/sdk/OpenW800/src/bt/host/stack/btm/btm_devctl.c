/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
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
 *  This file contains functions that handle BTM interface functions for the
 *  Bluetooth device including Rest, HCI buffer size and others
 *
 ******************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "bt_types.h"
#include "bt_utils.h"
#include "btm_int.h"
#include "btu.h"
#include "gki.h"
//#include "hci_layer.h"
#include "hcimsgs.h"
#include "l2c_int.h"
//#include "btcore/include/module.h"
#include "osi/include/compat.h"

#if BLE_INCLUDED == TRUE
#include "gatt_int.h"
#endif /* BLE_INCLUDED */

#ifdef USE_ALARM
extern fixed_queue_t *btu_general_alarm_queue;
extern thread_t *bt_workqueue_thread;
#endif

/********************************************************************************/
/*                 L O C A L    D A T A    D E F I N I T I O N S                */
/********************************************************************************/

/* The default class of device. */
#ifndef BTM_INIT_CLASS_OF_DEVICE
#define BTM_INIT_CLASS_OF_DEVICE    "\x00\x1F\x00"
#endif

#ifndef BTM_DEV_RESET_TIMEOUT
#define BTM_DEV_RESET_TIMEOUT   4
#endif

#define BTM_DEV_REPLY_TIMEOUT   2    /* 1 second expiration time is not good. Timer may start between 0 and 1 second. */
/* if it starts at the very end of the 0 second, timer will expire really easily. */

#define BTM_INFO_TIMEOUT        5   /* 5 seconds for info response */

/* After Reset a timeout can be specified in the target.h for specific targets
 * that may require additional time to reset
 * otherwise no timeout is required
*/
#ifndef BTM_AFTER_RESET_TIMEOUT
#define BTM_AFTER_RESET_TIMEOUT 0
#endif

/* Internal baseband so the parameters such as local features, version etc. are known
so there is no need to issue HCI commands and wait for responses at BTM initialization */
#ifndef BTM_INTERNAL_BB
#define BTM_INTERNAL_BB FALSE
#endif

/* The local version information in the format specified in the HCI read local version
response message */
#ifndef BTM_INTERNAL_LOCAL_VER
#define BTM_INTERNAL_LOCAL_VER {0x00, 0x01, 0x05, 0x81, 0x01, 0x30, 0x00, 0x40, 0x8D}
#endif

/* The local features information in the format specified in the HCI read local features
response message */
#ifndef BTM_INTERNAL_LOCAL_FEA
#define BTM_INTERNAL_LOCAL_FEA {0x00, 0xFF, 0xF9, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}
#endif

#ifndef BTM_SET_DEV_NAME_UPON_RESET
#define BTM_SET_DEV_NAME_UPON_RESET TRUE
#endif

/* host SCO buffer size */
#ifndef BTM_SCO_HOST_BUF_SIZE
#define BTM_SCO_HOST_BUF_SIZE       0xff
#endif

/********************************************************************************/
/*              L O C A L    F U N C T I O N     P R O T O T Y P E S            */
/********************************************************************************/
static void btm_dev_reset(void *p_data);
static void btm_after_reset_hold_complete(void);
static void btm_continue_reset(void);

static void btm_get_local_ext_features(uint8_t page_number);
static void btm_decode_ext_features_page(uint8_t page_number, const BD_FEATURES p_features);
static void btm_read_all_lmp_features_complete(uint8_t max_page_number);
static void btm_set_lmp_features_host_may_support(uint8_t max_page_number);
static void btm_get_local_features(void);
static void btm_issue_host_support_for_lmp_features(void);
static void btm_read_local_supported_cmds(uint8_t local_controller_id);

#if BLE_INCLUDED == TRUE
static void btm_read_ble_local_supported_features(void);
#endif

#if BTM_SCO_HCI_INCLUDED == TRUE
void btm_sco_process_num_bufs(uint16_t num_lm_sco_bufs);
#endif


/*******************************************************************************
**
** Function         btm_dev_init
**
** Description      This function is on the BTM startup
**
** Returns          void
**
*******************************************************************************/
void btm_dev_init(void)
{
#if 0  /* cleared in btm_init; put back in if called from anywhere else! */
    wm_memset(&btm_cb.devcb, 0, sizeof(tBTM_DEVCB));
#endif
    /* Initialize nonzero defaults */
#if (BTM_MAX_LOC_BD_NAME_LEN > 0)
    wm_memset(btm_cb.cfg.bd_name, 0, sizeof(tBTM_LOC_BD_NAME));
#endif
#ifdef USE_ALARM
    btm_cb.devcb.read_local_name_timer =
                    alarm_new("btm.read_local_name_timer");
    btm_cb.devcb.read_rssi_timer = alarm_new("btm.read_rssi_timer");
    btm_cb.devcb.read_link_quality_timer =
                    alarm_new("btm.read_link_quality_timer");
    btm_cb.devcb.read_inq_tx_power_timer =
                    alarm_new("btm.read_inq_tx_power_timer");
    btm_cb.devcb.qos_setup_timer = alarm_new("btm.qos_setup_timer");
    btm_cb.devcb.read_tx_power_timer = alarm_new("btm.read_tx_power_timer");
#endif
    btm_cb.btm_acl_pkt_types_supported = BTM_ACL_PKT_TYPES_MASK_DH1 + BTM_ACL_PKT_TYPES_MASK_DM1 +
                                         BTM_ACL_PKT_TYPES_MASK_DH3 + BTM_ACL_PKT_TYPES_MASK_DM3 +
                                         BTM_ACL_PKT_TYPES_MASK_DH5 + BTM_ACL_PKT_TYPES_MASK_DM5;
    btm_cb.btm_sco_pkt_types_supported = BTM_SCO_PKT_TYPES_MASK_HV1 +
                                         BTM_SCO_PKT_TYPES_MASK_HV2 +
                                         BTM_SCO_PKT_TYPES_MASK_HV3 +
                                         BTM_SCO_PKT_TYPES_MASK_EV3 +
                                         BTM_SCO_PKT_TYPES_MASK_EV4 +
                                         BTM_SCO_PKT_TYPES_MASK_EV5;
}


/*******************************************************************************
**
** Function         btm_db_reset
**
** Description      This function is called by BTM_DeviceReset and clears out any
**                  pending callbacks for inquiries, discoveries, other pending
**                  functions that may be in progress.
**
** Returns          void
**
*******************************************************************************/
static void btm_db_reset(void)
{
    tBTM_CMPL_CB    *p_cb;
    tBTM_STATUS      status = BTM_DEV_RESET;
    btm_inq_db_reset();

    if(btm_cb.devcb.p_rln_cmpl_cb) {
        p_cb = btm_cb.devcb.p_rln_cmpl_cb;
        btm_cb.devcb.p_rln_cmpl_cb = NULL;

        if(p_cb) {
            (*p_cb)((void *) NULL);
        }
    }

    if(btm_cb.devcb.p_rssi_cmpl_cb) {
        p_cb = btm_cb.devcb.p_rssi_cmpl_cb;
        btm_cb.devcb.p_rssi_cmpl_cb = NULL;

        if(p_cb) {
            (*p_cb)((tBTM_RSSI_RESULTS *) &status);
        }
    }
}



/*******************************************************************************
**
** Function         btm_dev_absent
**
** Description      This function is called by when it is detected that the
**                  device is not connected any more.
**
** Returns          void
**
*******************************************************************************/
void btm_dev_absent(void)
{
    btm_cb.devcb.state = BTM_DEV_STATE_WAIT_RESET_CMPLT;
    btm_db_reset();
    btm_inq_db_reset();
    /* If anyone wants device status notifications, give him one */
    btm_report_device_status(BTM_DEV_STATUS_DOWN);
    btu_stop_timer(&btm_cb.devcb.reset_timer);
}


/*******************************************************************************
**
** Function         BTM_DeviceReset
**
** Description      This function is called to reset the HCI.  Callback function
**                  if provided is called when startup of the device is
**                  completed.
**
** Returns          void
**
*******************************************************************************/
void BTM_DeviceReset(tBTM_CMPL_CB *p_cb)
{
    tBTM_STATUS status;
    BTM_TRACE_API("%s [%s] line:%d\n", __FILE__, __FUNCTION__, __LINE__);

    //printf("%s [%s] line:%d\r\n", __FILE__, __FUNCTION__, __LINE__);
    /* If device is already resetting, do not allow another */
    if((!btm_cb.devcb.p_reset_cmpl_cb) || (btm_cb.devcb.p_reset_cmpl_cb == p_cb)) {
        /* Flush all ACL connections */
        btm_acl_device_down();
        /* Clear the callback, so application would not hang on reset */
        btm_db_reset();
        /* Save address of the completion routine, if provided */
        btm_cb.devcb.p_reset_cmpl_cb = p_cb;
        btm_dev_reset(NULL);
    } else {
        /* pass an error to the bad callback, another one was already provided */
        if(p_cb) {
            status = BTM_ILLEGAL_VALUE;
            p_cb(&status);
        }
    }
}


/*******************************************************************************
**
** Function         BTM_IsDeviceUp
**
** Description      This function is called to check if the device is up.
**
** Returns          TRUE if device is up, else FALSE
**
*******************************************************************************/
uint8_t BTM_IsDeviceUp(void)
{
    return ((uint8_t)(btm_cb.devcb.state == BTM_DEV_STATE_READY));
}

/*******************************************************************************
**
** Function         BTM_SetAfhChannels
**
** Description      This function is called disable channels
**
** Returns          tBTM_STATUS
**
*******************************************************************************/
tBTM_STATUS BTM_SetAfhChannels(uint8_t first, uint8_t last)
{
    BTM_TRACE_API("BTM_SetAfhChannels first: %d (%d) last: %d (%d)",
                  first, btm_cb.first_disabled_channel, last,
                  btm_cb.last_disabled_channel);

    /* Make sure the local device supports the feature before sending */
    if((!HCI_LMP_AFH_CAP_MASTR_SUPPORTED(btm_cb.devcb.local_lmp_features[HCI_EXT_FEATURES_PAGE_0]))   &&
            (!HCI_LMP_AFH_CLASS_SLAVE_SUPPORTED(btm_cb.devcb.local_lmp_features[HCI_EXT_FEATURES_PAGE_0])) &&
            (!HCI_LMP_AFH_CLASS_MASTR_SUPPORTED(btm_cb.devcb.local_lmp_features[HCI_EXT_FEATURES_PAGE_0]))) {
        return (BTM_MODE_UNSUPPORTED);
    }

    if(!BTM_IsDeviceUp()) {
        return (BTM_WRONG_MODE);
    }

    if((btm_cb.first_disabled_channel != first)
            || (btm_cb.last_disabled_channel  != last)) {
        if(btsnd_hcic_set_afh_channels(first, last)) {
            btm_cb.first_disabled_channel = first;
            btm_cb.last_disabled_channel  = last;
        } else {
            return (BTM_NO_RESOURCES);
        }
    }

    return (BTM_SUCCESS);
}

/*******************************************************************************
**
** Function         BTM_SetAfhChannelAssessment
**
** Description      This function is called to set the channel assessment mode on or off
**
** Returns          none
**
*******************************************************************************/
tBTM_STATUS BTM_SetAfhChannelAssessment(uint8_t enable_or_disable)
{
    /* whatever app wants if device is not 1.2 scan type should be STANDARD */
    if(!HCI_LMP_AFH_CAP_SLAVE_SUPPORTED(btm_cb.devcb.local_lmp_features[HCI_EXT_FEATURES_PAGE_0])) {
        return (BTM_MODE_UNSUPPORTED);
    }

    if(!btsnd_hcic_write_afh_channel_assessment_mode(enable_or_disable)) {
        return (BTM_NO_RESOURCES);
    }

    return (BTM_SUCCESS);
}

/*******************************************************************************
**
** Function         BTM_ContinueReset
**
** Description      This function is called by the application to continue
**                  initialization after the application has completed its
**                  vendor specific sequence.  It is only used when
**                  BTM_APP_DEV_INIT is defined in target.h.
**
** Returns          void
**
*******************************************************************************/
void BTM_ContinueReset(void)
{
    BTM_TRACE_DEBUG("%s [%s] line:%d\n", __FILE__, __FUNCTION__, __LINE__);
#ifdef BTM_APP_DEV_INIT
    btm_continue_reset();
#endif
}

/*******************************************************************************
**
** Function         btm_dev_reset
**
** Description      Local function called to send a reset command
**
** Returns          void
**
*******************************************************************************/
static void btm_dev_reset(void *p_data)
{
    UNUSED(p_data);
    BTM_TRACE_DEBUG("%s [%s] line:%d\n", __FILE__, __FUNCTION__, __LINE__);
    btm_cb.devcb.state = BTM_DEV_STATE_WAIT_RESET_CMPLT;
    /* flush out the command complete queue and command transmit queue */
    btu_hcif_flush_cmd_queue();
    /* Start reset timer.  When timer expires we will send first command */
    /* from the setup sequence */
    btm_cb.devcb.reset_timer.p_cback = btm_dev_reset;
    btm_cb.devcb.reset_timer.param = (TIMER_PARAM_TYPE)NULL;
    btu_start_timer(&btm_cb.devcb.reset_timer, BTU_TTYPE_BTM_DEV_CTL,
                    BTM_DEV_RESET_TIMEOUT);
    btsnd_hcic_reset(LOCAL_BR_EDR_CONTROLLER_ID);
}


/*******************************************************************************
**
** Function         btm_get_hci_buf_size
**
** Description      Local function called to send a read buffer size command
**
** Returns          void
**
*******************************************************************************/
void btm_get_hci_buf_size(void)
{
    btm_cb.devcb.reset_timer.p_cback = btm_dev_reset;
    btm_cb.devcb.reset_timer.param = (TIMER_PARAM_TYPE)NULL;
    btu_start_timer(&btm_cb.devcb.reset_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_REPLY_TIMEOUT);
    /* Send a Read Buffer Size message to the Host Controller. */
    btsnd_hcic_read_buffer_size();
}
#if BLE_INCLUDED == TRUE
/*******************************************************************************
**
** Function         btm_read_ble_wl_size
**
** Description      Local function called to send a read BLE buffer size command
**
** Returns          void
**
*******************************************************************************/
void btm_read_ble_wl_size(void)
{
    BTM_TRACE_DEBUG("btm_read_ble_wl_size ");
    btm_cb.devcb.reset_timer.p_cback = btm_dev_reset;
    btm_cb.devcb.reset_timer.param = (TIMER_PARAM_TYPE)NULL;
    btu_start_timer(&btm_cb.devcb.reset_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_REPLY_TIMEOUT);
    /* Send a Read Buffer Size message to the Host Controller. */
    btsnd_hcic_ble_read_white_list_size();
}
/*******************************************************************************
**
** Function         btm_get_ble_buffer_size
**
** Description      Local function called to send a read BLE buffer size command
**
** Returns          void
**
*******************************************************************************/
void btm_get_ble_buffer_size(void)
{
    BTM_TRACE_DEBUG("btm_get_ble_buffer_size ");
    btm_cb.devcb.reset_timer.p_cback = btm_dev_reset;
    btm_cb.devcb.reset_timer.param = (TIMER_PARAM_TYPE)NULL;
    btu_start_timer(&btm_cb.devcb.reset_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_REPLY_TIMEOUT);
    /* Send a Read Buffer Size message to the Host Controller. */
    btsnd_hcic_ble_read_buffer_size();
}

/*******************************************************************************
**
** Function         btm_read_ble_local_supported_features
**
** Description      Local function called to send a read BLE local supported
**                  features command
**
** Returns          void
**
*******************************************************************************/
static void btm_read_ble_local_supported_features(void)
{
    BTM_TRACE_DEBUG("btm_read_ble_local_supported_features ");
    btm_cb.devcb.reset_timer.p_cback = btm_dev_reset;
    btm_cb.devcb.reset_timer.param = (TIMER_PARAM_TYPE)NULL;
    btu_start_timer(&btm_cb.devcb.reset_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_REPLY_TIMEOUT);
    /* Send a Read Local Supported Features message to the Host Controller. */
    btsnd_hcic_ble_read_local_spt_feat();
}

/*******************************************************************************
**
** Function         btm_read_ble_local_supported_states
**
** Description      Local function called to send a read BLE local supported
**                  features command
**
** Returns          void
**
*******************************************************************************/
static void btm_read_ble_local_supported_states(void)
{
    BTM_TRACE_DEBUG("btm_read_ble_local_supported_states ");
    btm_cb.devcb.reset_timer.p_cback = btm_dev_reset;
    btm_cb.devcb.reset_timer.param = (TIMER_PARAM_TYPE)NULL;
    btu_start_timer(&btm_cb.devcb.reset_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_REPLY_TIMEOUT);
    /* Send a Read Local Supported states message to the Host Controller. */
    btsnd_hcic_ble_read_supported_states();
}

#if (defined BLE_PRIVACY_SPT && BLE_PRIVACY_SPT == TRUE)
static void btm_read_ble_resolving_list_size(void)
{
    BTM_TRACE_DEBUG("btm_read_ble_local_supported_states ");
    btm_cb.devcb.reset_timer.p_cback = btm_dev_reset;
    btm_cb.devcb.reset_timer.param = (TIMER_PARAM_TYPE)NULL;
    btu_start_timer(&btm_cb.devcb.reset_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_REPLY_TIMEOUT);
    /* Send a Read Local Supported states message to the Host Controller. */
    btsnd_hcic_ble_read_resolving_list_size();
}
#endif

static void btm_read_ble_suggested_default_data_length(void)
{
    BTM_TRACE_DEBUG("btm_read_ble_suggested_default_data_length ");
    btm_cb.devcb.reset_timer.p_cback = btm_dev_reset;
    btm_cb.devcb.reset_timer.param = (TIMER_PARAM_TYPE)NULL;
    btu_start_timer(&btm_cb.devcb.reset_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_REPLY_TIMEOUT);
    /* Send a Read Local Supported states message to the Host Controller. */
    btsnd_hcic_ble_read_suggested_default_data_length();
}


#endif
/*******************************************************************************
**
** Function         btm_get_local_version
**
** Description      Local function called to send a read local version to controller
**
** Returns          void
**
*******************************************************************************/
void btm_get_local_version(void)
{
    btm_cb.devcb.reset_timer.p_cback = btm_dev_reset;
    btm_cb.devcb.reset_timer.param = (TIMER_PARAM_TYPE)NULL;
    btu_start_timer(&btm_cb.devcb.reset_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_REPLY_TIMEOUT);
    /* Send a Read Local Version message to the Host Controller. */
    btsnd_hcic_read_local_ver(LOCAL_BR_EDR_CONTROLLER_ID);
    btsnd_hcic_read_bd_addr();
#if BTM_PWR_MGR_INCLUDED == TRUE
    btm_pm_reset();
#endif
}

/*******************************************************************************
**
** Function         btm_read_local_supported_cmds
**
** Description      Local function called to send a read local supported commands
**
** Returns          void
**
*******************************************************************************/
static void btm_read_local_supported_cmds(uint8_t local_controller_id)
{
    BTM_TRACE_DEBUG("Start reading local supported commands");
    btm_cb.devcb.reset_timer.p_cback = btm_dev_reset;
    btm_cb.devcb.reset_timer.param = (TIMER_PARAM_TYPE)NULL;
    btu_start_timer(&btm_cb.devcb.reset_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_REPLY_TIMEOUT);
    btsnd_hcic_read_local_supported_cmds(local_controller_id);
}

/*******************************************************************************
**
** Function         btm_get_local_features
**
** Description      Local function called to send a read local features
**
** Returns          void
**
*******************************************************************************/
static void btm_get_local_features(void)
{
    /* If this BT controller supports Read Extended Feature */
    if(btm_cb.devcb.local_version.hci_version >= HCI_PROTO_VERSION_2_0) {
        btm_get_local_ext_features(HCI_EXT_FEATURES_PAGE_0);
    }
    /* else, if this is a very old BT controller */
    else {
        btm_cb.devcb.reset_timer.p_cback = btm_dev_reset;
        btm_cb.devcb.reset_timer.param = (TIMER_PARAM_TYPE)NULL;
        btu_start_timer(&btm_cb.devcb.reset_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_REPLY_TIMEOUT);
        /* Just read the basic features (legacy HCI command) */
        btsnd_hcic_read_local_features();
    }
}

/*******************************************************************************
**
** Function         btm_get_local_ext_features
**
** Description      Local function called to send a read local extended features
**
** Returns          void
**
*******************************************************************************/
static void btm_get_local_ext_features(uint8_t page_number)
{
    btm_cb.devcb.reset_timer.p_cback = btm_dev_reset;
    btm_cb.devcb.reset_timer.param = (TIMER_PARAM_TYPE)NULL;
    btu_start_timer(&btm_cb.devcb.reset_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_REPLY_TIMEOUT);
    btsnd_hcic_read_local_ext_features(page_number);
}

/*******************************************************************************
**
** Function         btm_dev_timeout
**
** Description      This function is called when a timer list entry expires.
**
** Returns          void
**
*******************************************************************************/
void btm_dev_timeout(TIMER_LIST_ENT  *p_tle)
{
#if 0
    TIMER_PARAM_TYPE timer_type = (TIMER_PARAM_TYPE)p_tle->param;

    if((timer_type & TT_DEV_RESET_MASK) == TT_DEV_RESET) {
        /* Call device reset as long as there is timeout*/
        btm_dev_reset(NULL);
    } else if(timer_type == (TIMER_PARAM_TYPE)TT_DEV_RLN) {
        tBTM_CMPL_CB  *p_cb = btm_cb.devcb.p_rln_cmpl_cb;
        btm_cb.devcb.p_rln_cmpl_cb = NULL;

        if(p_cb) {
            (*p_cb)((void *) NULL);
        }
    }

#endif
    printf("Impossible to run here...\r\n");
}

uint8_t set_sec_state_idle(void *data, void *context)
{
    tBTM_SEC_DEV_REC *p_dev_rec = data;
    p_dev_rec->sec_state = BTM_SEC_STATE_IDLE;
    return true;
}

/*******************************************************************************
**
** Function         btm_reset_complete
**
** Description      This function is called when command complete for HCI_Reset
**                  is received.  It does not make sense to send next command
**                  because device is resetting after command complete is
**                  received.  Just start timer and set required state.
**
** Returns          void
**
*******************************************************************************/
void btm_reset_complete(void)
{
    /* Handle if btm initiated the reset */
    if(btm_cb.devcb.state == BTM_DEV_STATE_WAIT_RESET_CMPLT) {
#if (BTM_AFTER_RESET_TIMEOUT > 0)
        btu_start_timer(&btm_cb.devcb.reset_timer, BTU_TTYPE_BTM_DEV_CTL,
                        BTM_AFTER_RESET_TIMEOUT);
#else
        btm_cb.devcb.state = BTM_DEV_STATE_WAIT_AFTER_RESET;
        btm_after_reset_hold_complete();
#endif
    }
}


/*******************************************************************************
**
** Function         btm_reset_complete-->btm_reset_ctrlr_complete
**
** Description      This function is called when command complete for HCI_Reset
**                  is received.  It does not make sense to send next command
**                  because device is resetting after command complete is
**                  received.  Just start timer and set required state.
**
** Returns          void
**
*******************************************************************************/
void btm_reset_ctrlr_complete()
{
    uint8_t         found = FALSE;
    uint8_t           i, j, max_page_number;
#if (BLE_INCLUDED == TRUE)	
    uint8_t support_le = 0, support_le_privacy = 0;
#endif
    tBTM_DEVCB     *p_devcb = &btm_cb.devcb;
    btu_stop_timer(&btm_cb.devcb.reset_timer);
    /* Tell L2CAP that all connections are gone */
    l2cu_device_reset();
    /* Clear current security state */
    list_foreach(btm_cb.sec_dev_rec, set_sec_state_idle, NULL);
    /* After the reset controller should restore all parameters to defaults. */
    btm_cb.btm_inq_vars.inq_counter       = 1;
    btm_cb.btm_inq_vars.inq_scan_window   = HCI_DEF_INQUIRYSCAN_WINDOW;
    btm_cb.btm_inq_vars.inq_scan_period   = HCI_DEF_INQUIRYSCAN_INTERVAL;
    btm_cb.btm_inq_vars.inq_scan_type     = HCI_DEF_SCAN_TYPE;
    btm_cb.btm_inq_vars.page_scan_window  = HCI_DEF_PAGESCAN_WINDOW;
    btm_cb.btm_inq_vars.page_scan_period  = HCI_DEF_PAGESCAN_INTERVAL;
    btm_cb.btm_inq_vars.page_scan_type    = HCI_DEF_SCAN_TYPE;
#if (BLE_INCLUDED == TRUE)
    btm_cb.ble_ctr_cb.conn_state = BLE_CONN_IDLE;
    btm_cb.ble_ctr_cb.bg_conn_type = BTM_BLE_CONN_NONE;
    btm_cb.ble_ctr_cb.p_select_cback = NULL;
    gatt_reset_bgdev_list();
#if (BLE_VND_INCLUDED == TRUE)
    btm_ble_multi_adv_init();
#endif
#endif
    btm_pm_reset();
    btm_cb.devcb.state = BTM_DEV_STATE_READY;
#if (BLE_INCLUDED == TRUE)
    support_le = HCI_LE_SPT_SUPPORTED(btm_cb.devcb.local_lmp_features[HCI_EXT_FEATURES_PAGE_0]);
    support_le_privacy = HCI_SC_CTRLR_SUPPORTED(
                                         btm_cb.devcb.local_lmp_features[HCI_EXT_FEATURES_PAGE_2]);
#if (defined BLE_PRIVACY_SPT && BLE_PRIVACY_SPT == TRUE)
    BTM_TRACE_DEBUG("support le=%d, support_le_privacy=%d, rlsize=%d\r\n", support_le,
                    support_le_privacy, btm_cb.devcb.ble_resolving_list_max_size);

    /* Set up the BLE privacy settings */
    if((support_le != 0) && (support_le_privacy != 0) &&
            btm_cb.devcb.ble_resolving_list_max_size > 0) {
        btm_ble_resolving_list_init(btm_cb.devcb.ble_resolving_list_max_size);
        /* set the default random private address timeout */
        //btsnd_hcic_ble_set_rand_priv_addr_timeout(BTM_BLE_PRIVATE_ADDR_INT_MS / 1000);
    }

#endif

    if(support_le != 0) {
        btm_ble_white_list_init(btm_cb.devcb.ble_white_list_size);
        //l2c_link_processs_ble_num_bufs(controller->get_acl_buffer_count_ble());
    }

#endif
    BTM_SetPinType(btm_cb.cfg.pin_type, btm_cb.cfg.pin_code, btm_cb.cfg.pin_code_len);

    /* find the highest feature page number which contains non-zero bits */
    for(i = HCI_EXT_FEATURES_PAGE_MAX; ; i--) {
        for(j = 0; j < HCI_FEATURE_BYTES_PER_PAGE; j++) {
            if(p_devcb->local_lmp_features[i][j] != 0) {
                found = TRUE;
                break;
            }
        }

        if(found || !i) {
            break;
        }
    }

    if(!found) {
        BTM_TRACE_WARNING("btm_reset_ctrlr_complete: NONE of local controller features is set");
    }

    max_page_number = i;
    BTM_TRACE_DEBUG("btm_reset_ctrlr_complete: max_page_number: %d", max_page_number);
    /*
    * Set State to Ready (needs to be done before btm_decode_ext_features_page
    * to allow it to send some HCI configuration commands)
    */
    p_devcb->state = BTM_DEV_STATE_READY;

    /* For every received/saved feature page */
    for(i = 0; i <= max_page_number; i++) {
        /* Decode the saved Feature Page */
        btm_decode_ext_features_page(i, p_devcb->local_lmp_features[i]);
    }

    btm_report_device_status(BTM_DEV_STATUS_UP);
}

/*******************************************************************************
**
** Function         btm_continue_reset
**
** Description      This function is called when wait period expired after
**                  device reset or called by the application to continue
**                  initialization after the application has completed its
**                  vendor specific sequence.
**
** Returns          void
**
*******************************************************************************/
void btm_continue_reset(void)
{
    BTM_TRACE_DEBUG("%s [%s] line:%d\n", __FILE__, __FUNCTION__, __LINE__);
    /* Reinitialize the default class of device */
#if BTM_INTERNAL_BB == TRUE
    btsnd_hcic_read_bd_addr();
#if BTM_PWR_MGR_INCLUDED == TRUE
    btm_pm_reset();
#endif
#endif
    btm_get_hci_buf_size();
    /* default device class */
    BTM_SetDeviceClass((uint8_t *) BTM_INIT_CLASS_OF_DEVICE);
#if (BTM_MAX_LOC_BD_NAME_LEN > 0) && (BTM_SET_DEV_NAME_UPON_RESET == TRUE)
    BTM_SetLocalDeviceName(btm_cb.cfg.bd_name);
#endif
    BTM_SetPinType(btm_cb.cfg.pin_type, btm_cb.cfg.pin_code, btm_cb.cfg.pin_code_len);
}

/*******************************************************************************
**
** Function         btm_after_reset_hold_complete
**
** Description      This function is called when wait period expired after
**                  device reset.  Continue intitialization
**
** Returns          void
**
*******************************************************************************/
void btm_after_reset_hold_complete(void)
{
    BTM_TRACE_DEBUG("enter %s [%s] line:%d\n", __FILE__, __FUNCTION__, __LINE__);
#ifdef BTM_APP_DEV_INIT
    btu_stop_timer(&btm_cb.devcb.reset_timer);
    BTM_APP_DEV_INIT();
#else
    btm_continue_reset();
#endif
    BTM_TRACE_DEBUG("leave %s [%s] line:%d\n", __FILE__, __FUNCTION__, __LINE__);
}


/*******************************************************************************
**
** Function         btm_read_hci_buf_size_complete
**
** Description      This function is called when command complete for
**                  get HCI buffer size is received.  Start timer and send
**                  read local features request
**
** Returns          void
**
*******************************************************************************/
void btm_read_hci_buf_size_complete(uint8_t *p, uint16_t evt_len)
{
    uint8_t       status;
    uint8_t       lm_sco_buf_size;
    uint16_t      lm_num_acl_bufs;
    uint16_t      lm_num_sco_bufs;
    uint16_t      acl_buf_size;
    UNUSED(evt_len);
    STREAM_TO_UINT8(status, p);

    if(status == HCI_SUCCESS) {
        STREAM_TO_UINT16(btu_cb.hcit_acl_data_size, p);
        STREAM_TO_UINT8(lm_sco_buf_size,   p);
        STREAM_TO_UINT16(lm_num_acl_bufs,   p);
        STREAM_TO_UINT16(lm_num_sco_bufs,   p);
        btu_cb.hcit_acl_pkt_size = btu_cb.hcit_acl_data_size + HCI_DATA_PREAMBLE_SIZE;
        l2c_link_processs_num_bufs(lm_num_acl_bufs);
#if BTM_SCO_HCI_INCLUDED == TRUE
        btm_sco_process_num_bufs(lm_num_sco_bufs);
#endif
#if BTM_ACL_BUF_SIZE > 0
        acl_buf_size = (BTM_ACL_BUF_SIZE < L2CAP_MTU_SIZE) ? BTM_ACL_BUF_SIZE : L2CAP_MTU_SIZE;
#else
        acl_buf_size = L2CAP_MTU_SIZE;
#endif
        /* Tell the controller what our buffer sizes are. ?? Need SCO info */
        btsnd_hcic_set_host_buf_size(acl_buf_size, BTM_SCO_HOST_BUF_SIZE, L2CAP_HOST_FC_ACL_BUFS, 10);
#if L2CAP_HOST_FLOW_CTRL == TRUE
        btsnd_hcic_set_host_flow_ctrl(HCI_HOST_FLOW_CTRL_ACL_ON);
#endif
    }

    /* Set the device into connectable and/or discoverable mode (if configured to do so) */
#if BTM_IS_CONNECTABLE == TRUE
    (void) BTM_SetConnectability(BTM_CONNECTABLE, BTM_DEFAULT_CONN_WINDOW, BTM_DEFAULT_CONN_INTERVAL);
#endif
#if BTM_IS_DISCOVERABLE == TRUE
    (void) BTM_SetDiscoverability(BTM_DEFAULT_DISC_MODE, BTM_DEFAULT_DISC_WINDOW,
                                  BTM_DEFAULT_DISC_INTERVAL);
#endif
	UNUSED(lm_num_sco_bufs);
	UNUSED(lm_sco_buf_size);
#if BTM_INTERNAL_BB == TRUE
    {
        uint8_t buf[9] = BTM_INTERNAL_LOCAL_VER;
        btm_read_local_version_complete(buf, 9);
    }
#else
    btm_get_local_version();
#endif
}

#if (BLE_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btm_read_ble_buf_size_complete
**
** Description      This function is called when command complete for
**                  get HCI buffer size is received.  Start timer and send
**                  read local supported features request
**
** Returns          void
**
*******************************************************************************/
void btm_read_ble_buf_size_complete(uint8_t *p, uint16_t evt_len)
{
    uint8_t       status;
    uint16_t      lm_num_le_bufs;
    UNUSED(evt_len);
    BTM_TRACE_DEBUG("btm_read_ble_buf_size_complete ");
    STREAM_TO_UINT8(status, p);

    if(status == HCI_SUCCESS) {
        STREAM_TO_UINT16(btu_cb.hcit_ble_acl_data_size, p);
        STREAM_TO_UINT8(lm_num_le_bufs,   p);

        if(btu_cb.hcit_ble_acl_data_size == 0) {
            btu_cb.hcit_ble_acl_data_size = btu_cb.hcit_acl_data_size;
        }

        btu_cb.hcit_ble_acl_pkt_size = btu_cb.hcit_ble_acl_data_size + HCI_DATA_PREAMBLE_SIZE;
        l2c_link_processs_ble_num_bufs(lm_num_le_bufs);
    }

    btm_read_ble_local_supported_states();
}
/*******************************************************************************
**
** Function         btm_read_ble_local_supported_states_complete
**
** Description      This function is called when command complete for
**                  Read LE Local Supported states complete is received.
**
** Returns          void
**
*******************************************************************************/
void btm_read_ble_local_supported_states_complete(uint8_t *p, uint16_t evt_len)
{
    uint8_t       status;
    UNUSED(evt_len);
    BTM_TRACE_DEBUG("btm_read_ble_local_supported_states_complete ");
    btu_stop_timer(&btm_cb.devcb.reset_timer);
    STREAM_TO_UINT8(status, p);

    if(status == HCI_SUCCESS) {
        STREAM_TO_ARRAY(&btm_cb.devcb.le_supported_states, p, BTM_LE_SUPPORT_STATE_SIZE);
    } else {
        BTM_TRACE_WARNING("btm_read_ble_local_supported_features_complete status = %d", status);
    }

    btm_read_ble_local_supported_features();
}

/*******************************************************************************
**
** Function         btm_read_ble_local_supported_features_complete
**
** Description      This function is called when command complete for
**                  Read LE Local Supported Features is received.  Start timer and send
**                  read LMP local features request
**
** Returns          void
**
*******************************************************************************/
void btm_read_ble_local_supported_features_complete(uint8_t *p, uint16_t evt_len)
{
    uint8_t       status;
    UNUSED(evt_len);
    BTM_TRACE_DEBUG("btm_read_ble_local_supported_features_complete ");
    btu_stop_timer(&btm_cb.devcb.reset_timer);
    STREAM_TO_UINT8(status, p);

    if(status == HCI_SUCCESS) {
        STREAM_TO_ARRAY(&btm_cb.devcb.local_le_features, p, HCI_FEATURE_BYTES_PER_PAGE);
    } else {
        BTM_TRACE_WARNING("btm_read_ble_local_supported_features_complete status = %d", status);
    }

#if (defined BLE_PRIVACY_SPT && BLE_PRIVACY_SPT == TRUE)

    if(HCI_LE_ENHANCED_PRIVACY_SUPPORTED(btm_cb.devcb.local_le_features)) {
        btm_read_ble_resolving_list_size();
    } else
#endif
    {
        if(HCI_LE_DATA_LEN_EXT_SUPPORTED(btm_cb.devcb.local_le_features)) {
            btm_read_ble_suggested_default_data_length();
        } else {
            btsnd_hcic_ble_set_evt_mask((uint8_t *)HCI_BLE_EVENT_MASK_DEF);
#if BTM_INTERNAL_BB == TRUE
            {
                uint8_t buf[9] = BTM_INTERNAL_LOCAL_FEA;
                btm_read_local_features_complete(buf, 9);
            }
#else
            /* get local feature if BRCM specific feature is not included  */
            btm_reset_ctrlr_complete();
#endif
        }
    }
}

/*******************************************************************************
**
** Function         btm_read_white_list_size_complete
**
** Description      This function read the current white list size.
*******************************************************************************/
void btm_read_white_list_size_complete(uint8_t *p, uint16_t evt_len)
{
    uint8_t       status;
    UNUSED(evt_len);
    BTM_TRACE_DEBUG("btm_read_white_list_size_complete ");
    STREAM_TO_UINT8(status, p);

    if(status == HCI_SUCCESS) {
        STREAM_TO_UINT8(btm_cb.ble_ctr_cb.white_list_avail_size, p);
        btm_cb.devcb.ble_white_list_size = btm_cb.ble_ctr_cb.white_list_avail_size;
    }

    /* write LE host support and simultatunous LE supported */
    btsnd_hcic_ble_write_host_supported(BTM_BLE_HOST_SUPPORT, BTM_BLE_SIMULTANEOUS_HOST);
    btm_get_ble_buffer_size();
}

#if BLE_PRIVACY_SPT == TRUE
void btm_read_ble_resolving_list_size_complete(uint8_t *p, uint16_t evt_len)
{
    uint8_t       status;
    UNUSED(evt_len);
    BTM_TRACE_DEBUG("btm_read_ble_resolving_list_size_complete ");
    STREAM_TO_UINT8(status, p);

    if(status == HCI_SUCCESS) {
        STREAM_TO_UINT8(btm_cb.ble_ctr_cb.resolving_list_avail_size, p);
        btm_cb.devcb.ble_resolving_list_max_size = btm_cb.ble_ctr_cb.resolving_list_avail_size;
    }

    btm_read_ble_suggested_default_data_length();
}
#endif

void btm_read_ble_suggested_default_data_length_complete(uint8_t *p, uint16_t evt_len)
{
    uint8_t       status;
    UNUSED(evt_len);
    BTM_TRACE_DEBUG("btm_read_ble_suggested_default_data_length_complete ");
    STREAM_TO_UINT8(status, p);

    if(status == HCI_SUCCESS) {
        STREAM_TO_UINT16(btm_cb.ble_ctr_cb.ble_suggested_default_data_length, p);
        btm_cb.devcb.ble_packet_len = btm_cb.ble_ctr_cb.ble_suggested_default_data_length;
    }

    //////////////////////////////////////////////////////////////////////////
    btsnd_hcic_ble_set_evt_mask((uint8_t *)HCI_BLE_EVENT_MASK_DEF);
#if BTM_INTERNAL_BB == TRUE
    {
        uint8_t buf[9] = BTM_INTERNAL_LOCAL_FEA;
        btm_read_local_features_complete(buf, 9);
    }
#else
    /* get local feature if BRCM specific feature is not included  */
    btm_reset_ctrlr_complete();
#endif
    //////////////////////////////////////////////////////////////////////////
}

#endif
/*******************************************************************************
**
** Function         btm_read_local_version_complete
**
** Description      This function is called when local BD Addr read complete
**                  message is received from the HCI.
**
** Returns          void
**
*******************************************************************************/
void btm_read_local_version_complete(uint8_t *p, uint16_t evt_len)
{
    tBTM_VERSION_INFO   *p_vi = &btm_cb.devcb.local_version;
    uint8_t                status;
    UNUSED(evt_len);
#ifdef BTA_PRM_CHECK_FW_VER

    if(BTA_PRM_CHECK_FW_VER(p)) {
        return;
    }

#endif
    STREAM_TO_UINT8(status, p);

    if(status == HCI_SUCCESS) {
        STREAM_TO_UINT8(p_vi->hci_version, p);
        STREAM_TO_UINT16(p_vi->hci_revision, p);
        STREAM_TO_UINT8(p_vi->lmp_version, p);
        STREAM_TO_UINT16(p_vi->manufacturer, p);
        STREAM_TO_UINT16(p_vi->lmp_subversion, p);
    }

    if(p_vi->hci_version >= HCI_PROTO_VERSION_1_2) {
        btm_read_local_supported_cmds(LOCAL_BR_EDR_CONTROLLER_ID);
    } else {
        btm_get_local_features();
    }
}
/*******************************************************************************
**
** Function         btm_decode_ext_features_page
**
** Description      This function is decodes a features page.
**
** Returns          void
**
*******************************************************************************/
static void btm_decode_ext_features_page(uint8_t page_number, const uint8_t *p_features)
{
    BTM_TRACE_DEBUG("btm_decode_ext_features_page page: %d", page_number);

    switch(page_number) {
        /* Extended (Legacy) Page 0 */
        case HCI_EXT_FEATURES_PAGE_0:
            /* Create ACL supported packet types mask */
            btm_cb.btm_acl_pkt_types_supported = (BTM_ACL_PKT_TYPES_MASK_DH1 +
                                                  BTM_ACL_PKT_TYPES_MASK_DM1);

            if(HCI_3_SLOT_PACKETS_SUPPORTED(p_features))
                btm_cb.btm_acl_pkt_types_supported |= (BTM_ACL_PKT_TYPES_MASK_DH3 +
                                                       BTM_ACL_PKT_TYPES_MASK_DM3);

            if(HCI_5_SLOT_PACKETS_SUPPORTED(p_features))
                btm_cb.btm_acl_pkt_types_supported |= (BTM_ACL_PKT_TYPES_MASK_DH5 +
                                                       BTM_ACL_PKT_TYPES_MASK_DM5);

            /* Add in EDR related ACL types */
            if(!HCI_EDR_ACL_2MPS_SUPPORTED(p_features)) {
                btm_cb.btm_acl_pkt_types_supported |= (BTM_ACL_PKT_TYPES_MASK_NO_2_DH1 +
                                                       BTM_ACL_PKT_TYPES_MASK_NO_2_DH3 +
                                                       BTM_ACL_PKT_TYPES_MASK_NO_2_DH5);
            }

            if(!HCI_EDR_ACL_3MPS_SUPPORTED(p_features)) {
                btm_cb.btm_acl_pkt_types_supported |= (BTM_ACL_PKT_TYPES_MASK_NO_3_DH1 +
                                                       BTM_ACL_PKT_TYPES_MASK_NO_3_DH3 +
                                                       BTM_ACL_PKT_TYPES_MASK_NO_3_DH5);
            }

            /* Check to see if 3 and 5 slot packets are available */
            if(HCI_EDR_ACL_2MPS_SUPPORTED(p_features) ||
                    HCI_EDR_ACL_3MPS_SUPPORTED(p_features)) {
                if(!HCI_3_SLOT_EDR_ACL_SUPPORTED(p_features))
                    btm_cb.btm_acl_pkt_types_supported |= (BTM_ACL_PKT_TYPES_MASK_NO_2_DH3 +
                                                           BTM_ACL_PKT_TYPES_MASK_NO_3_DH3);

                if(!HCI_5_SLOT_EDR_ACL_SUPPORTED(p_features))
                    btm_cb.btm_acl_pkt_types_supported |= (BTM_ACL_PKT_TYPES_MASK_NO_2_DH5 +
                                                           BTM_ACL_PKT_TYPES_MASK_NO_3_DH5);
            }

            BTM_TRACE_DEBUG("Local supported ACL packet types: 0x%04x",
                            btm_cb.btm_acl_pkt_types_supported);
            /* Create (e)SCO supported packet types mask */
            btm_cb.btm_sco_pkt_types_supported = 0;
#if BTM_SCO_INCLUDED == TRUE
            btm_cb.sco_cb.esco_supported = FALSE;
#endif

            if(HCI_SCO_LINK_SUPPORTED(p_features)) {
                btm_cb.btm_sco_pkt_types_supported = BTM_SCO_PKT_TYPES_MASK_HV1;

                if(HCI_HV2_PACKETS_SUPPORTED(p_features)) {
                    btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_HV2;
                }

                if(HCI_HV3_PACKETS_SUPPORTED(p_features)) {
                    btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_HV3;
                }
            }

            if(HCI_ESCO_EV3_SUPPORTED(p_features)) {
                btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_EV3;
            }

            if(HCI_ESCO_EV4_SUPPORTED(p_features)) {
                btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_EV4;
            }

            if(HCI_ESCO_EV5_SUPPORTED(p_features)) {
                btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_EV5;
            }

#if BTM_SCO_INCLUDED == TRUE

            if(btm_cb.btm_sco_pkt_types_supported & BTM_ESCO_LINK_ONLY_MASK) {
                btm_cb.sco_cb.esco_supported = TRUE;

                /* Add in EDR related eSCO types */
                if(HCI_EDR_ESCO_2MPS_SUPPORTED(p_features)) {
                    if(!HCI_3_SLOT_EDR_ESCO_SUPPORTED(p_features)) {
                        btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_NO_2_EV5;
                    }
                } else {
                    btm_cb.btm_sco_pkt_types_supported |= (BTM_SCO_PKT_TYPES_MASK_NO_2_EV3 +
                                                           BTM_SCO_PKT_TYPES_MASK_NO_2_EV5);
                }

                if(HCI_EDR_ESCO_3MPS_SUPPORTED(p_features)) {
                    if(!HCI_3_SLOT_EDR_ESCO_SUPPORTED(p_features)) {
                        btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_NO_3_EV5;
                    }
                } else {
                    btm_cb.btm_sco_pkt_types_supported |= (BTM_SCO_PKT_TYPES_MASK_NO_3_EV3 +
                                                           BTM_SCO_PKT_TYPES_MASK_NO_3_EV5);
                }
            }

#endif
            BTM_TRACE_DEBUG("Local supported SCO packet types: 0x%04x",
                            btm_cb.btm_sco_pkt_types_supported);

            /* Create Default Policy Settings */
            if(HCI_SWITCH_SUPPORTED(p_features)) {
                btm_cb.btm_def_link_policy |= HCI_ENABLE_MASTER_SLAVE_SWITCH;
            } else {
                btm_cb.btm_def_link_policy &= ~HCI_ENABLE_MASTER_SLAVE_SWITCH;
            }

            if(HCI_HOLD_MODE_SUPPORTED(p_features)) {
                btm_cb.btm_def_link_policy |= HCI_ENABLE_HOLD_MODE;
            } else {
                btm_cb.btm_def_link_policy &= ~HCI_ENABLE_HOLD_MODE;
            }

            if(HCI_SNIFF_MODE_SUPPORTED(p_features)) {
                btm_cb.btm_def_link_policy |= HCI_ENABLE_SNIFF_MODE;
            } else {
                btm_cb.btm_def_link_policy &= ~HCI_ENABLE_SNIFF_MODE;
            }

            if(HCI_PARK_MODE_SUPPORTED(p_features)) {
                btm_cb.btm_def_link_policy |= HCI_ENABLE_PARK_MODE;
            } else {
                btm_cb.btm_def_link_policy &= ~HCI_ENABLE_PARK_MODE;
            }

            btm_sec_dev_reset();

            if(HCI_LMP_INQ_RSSI_SUPPORTED(p_features)) {
                if(HCI_EXT_INQ_RSP_SUPPORTED(p_features)) {
                    BTM_SetInquiryMode(BTM_INQ_RESULT_EXTENDED);
                } else {
                    BTM_SetInquiryMode(BTM_INQ_RESULT_WITH_RSSI);
                }
            }

#if L2CAP_NON_FLUSHABLE_PB_INCLUDED == TRUE

            if(HCI_NON_FLUSHABLE_PB_SUPPORTED(p_features)) {
                l2cu_set_non_flushable_pbf(TRUE);
            } else {
                l2cu_set_non_flushable_pbf(FALSE);
            }

#endif
            BTM_SetPageScanType(BTM_DEFAULT_SCAN_TYPE);
            BTM_SetInquiryScanType(BTM_DEFAULT_SCAN_TYPE);
            break;

        /* Extended Page 1 */
        case HCI_EXT_FEATURES_PAGE_1:
            /* Nothing to do for page 1 */
            break;

        /* Extended Page 2 */
        case HCI_EXT_FEATURES_PAGE_2:
            /* Nothing to do for page 2 */
            break;

        default:
            BTM_TRACE_ERROR("btm_decode_ext_features_page page=%d unknown", page_number);
            break;
    }
}

/*******************************************************************************
**
** Function         btm_reset_ctrlr_complete
**
** Description      This is the last step of BR/EDR controller startup sequence.
**
** Returns          void
**
*******************************************************************************/
void btm_reset_ctrlr_complete_bluedroid5()
{
    tBTM_DEVCB     *p_devcb = &btm_cb.devcb;
    tBTM_CMPL_CB   *p_cb = p_devcb->p_reset_cmpl_cb;
    uint8_t         found = FALSE;
    uint8_t           i, j, max_page_number;
    btu_stop_timer(&btm_cb.devcb.reset_timer);

    /* find the highest feature page number which contains non-zero bits */
    for(i = HCI_EXT_FEATURES_PAGE_MAX; ; i--) {
        for(j = 0; j < HCI_FEATURE_BYTES_PER_PAGE; j++) {
            if(p_devcb->local_lmp_features[i][j] != 0) {
                found = TRUE;
                break;
            }
        }

        if(found || !i) {
            break;
        }
    }

    if(!found) {
        BTM_TRACE_WARNING("btm_reset_ctrlr_complete: NONE of local controller features is set");
    }

    max_page_number = i;
    BTM_TRACE_DEBUG("btm_reset_ctrlr_complete: max_page_number: %d", max_page_number);
    /*
    * Set State to Ready (needs to be done before btm_decode_ext_features_page
    * to allow it to send some HCI configuration commands)
    */
    p_devcb->state = BTM_DEV_STATE_READY;

    /* For every received/saved feature page */
    for(i = 0; i <= max_page_number; i++) {
        /* Decode the saved Feature Page */
        btm_decode_ext_features_page(i, p_devcb->local_lmp_features[i]);
    }

    /* If there was a callback address for reset complete, reset it */
    p_devcb->p_reset_cmpl_cb = NULL;
    /* If anyone wants device status notifications, give him one */
    btm_report_device_status(BTM_DEV_STATUS_UP);

    /* Reset sequence is complete. If this was an application originated */
    /* reset, tell him its done.                                         */
    if(p_cb) {
        (*p_cb)((void *) NULL);
    }
}

/*******************************************************************************
**
** Function         btm_issue_host_support_for_lmp_features
**
** Description      This function:
**                  - issues commands to set host supported LMP features (one at
**                    a time);
**                  - after this is done it issues command to re-read LMP features
**                    page 1;
**                  - after this is done it calls the last step of BR/EDR
**                    controller startup sequence.
**
** Returns          void
**
*******************************************************************************/
static void btm_issue_host_support_for_lmp_features(void)
{
    BTM_TRACE_DEBUG("btm_issue_host_support_for_lmp_features lmp_features_host_may_support: 0x%02x",
                    btm_cb.devcb.lmp_features_host_may_support);

    if(btm_cb.devcb.lmp_features_host_may_support & BTM_HOST_MAY_SUPP_SSP) {
        btsnd_hcic_write_simple_pairing_mode(HCI_SP_MODE_ENABLED);
        return;
    }

#if (BLE_INCLUDED == TRUE)

    if(btm_cb.devcb.lmp_features_host_may_support & BTM_HOST_MAY_SUPP_LE) {
        if(btm_cb.devcb.lmp_features_host_may_support & BTM_HOST_MAY_SUPP_SIMULT_BR_LE) {
            btsnd_hcic_ble_write_host_supported(BTM_BLE_HOST_SUPPORT, BTM_BLE_SIMULTANEOUS_HOST);
        } else {
            btsnd_hcic_ble_write_host_supported(BTM_BLE_HOST_SUPPORT, 0);
        }

        return;
    }

#endif

    if(btm_cb.devcb.lmp_features_host_may_support & BTM_RE_READ_1ST_PAGE) {
        btm_get_local_ext_features(HCI_EXT_FEATURES_PAGE_1);
        return;
    }

    if(!btm_cb.devcb.lmp_features_host_may_support) {
#if BLE_INCLUDED == TRUE

        if(HCI_LE_HOST_SUPPORTED(btm_cb.devcb.local_lmp_features[HCI_EXT_FEATURES_PAGE_1])) {
            btm_read_ble_wl_size();
        } else
#elif BTM_INTERNAL_BB == TRUE
        {
            uint8_t buf[9] = BTM_INTERNAL_LOCAL_FEA;
            btm_read_local_features_complete(buf, 9);
        }

#endif
        {
            btm_reset_ctrlr_complete();
        }
        return;
    }

    BTM_TRACE_ERROR("%s lmp_features_host_may_support: 0x%02x. This is unexpected.", __FUNCTION__,
                    btm_cb.devcb.lmp_features_host_may_support);
}

/*******************************************************************************
**
** Function         btm_set_lmp_features_host_may_support
**
** Description      This function is called after all LMP features provided by
**                  controller are read. It sets the mask that indicates LMP
**                  features the host may support based on LMP features supported
**                  by controller.
**                  Example:
**                  Host may set SSP (host support) bit only if SSP (controller
**                  support) bit is set by controller.
**
** Returns          void
**
*******************************************************************************/
static void btm_set_lmp_features_host_may_support(uint8_t max_page_number)
{
    btm_cb.devcb.lmp_features_host_may_support = 0;

    /* LMP page 0 is always read */
    if(HCI_SIMPLE_PAIRING_SUPPORTED(btm_cb.devcb.local_lmp_features[HCI_EXT_FEATURES_PAGE_0])) {
        /* host may support SSP */
        btm_cb.devcb.lmp_features_host_may_support |= BTM_HOST_MAY_SUPP_SSP;
    }

#if (BLE_INCLUDED == TRUE)

    if(HCI_LE_SPT_SUPPORTED(btm_cb.devcb.local_lmp_features[HCI_EXT_FEATURES_PAGE_0])) {
        /* host may support LE */
        btm_cb.devcb.lmp_features_host_may_support |= BTM_HOST_MAY_SUPP_LE;

        if(HCI_SIMUL_LE_BREDR_SUPPORTED(btm_cb.devcb.local_lmp_features[HCI_EXT_FEATURES_PAGE_0])) {
            /* host may support BR/EDR and LE simultaneously */
            btm_cb.devcb.lmp_features_host_may_support |= BTM_HOST_MAY_SUPP_SIMULT_BR_LE;
        }
    }

#endif

    if(max_page_number >= HCI_EXT_FEATURES_PAGE_1) {
        /* nothing yet for HCI_EXT_FEATURES_PAGE_1 */
    }

    if(max_page_number >= HCI_EXT_FEATURES_PAGE_1) {
        /* nothing yet for HCI_EXT_FEATURES_PAGE_2 */
    }

    if(btm_cb.devcb.lmp_features_host_may_support) {
        btm_cb.devcb.lmp_features_host_may_support |= BTM_RE_READ_1ST_PAGE;
    }
}

/*******************************************************************************
**
** Function         btm_read_all_lmp_features_complete
**
** Description      This function is called after all LMP features provided by
**                  controller are read.
**                  It works with controller supported LMP features which host
**                  may support too.
**
** Returns          void
**
*******************************************************************************/
static void btm_read_all_lmp_features_complete(uint8_t max_page_number)
{
    btm_set_lmp_features_host_may_support(max_page_number);
    btm_issue_host_support_for_lmp_features();
}

/*******************************************************************************
**
** Function         btm_read_local_features_complete
**
** Description      This function is called when local supported features read
**                  is complete.
**
** Returns          void
**
*******************************************************************************/
void btm_read_local_features_complete(uint8_t *p, uint16_t evt_len)
{
    tBTM_DEVCB     *p_devcb = &btm_cb.devcb;
    uint8_t           status;
    UNUSED(evt_len);
    btu_stop_timer(&p_devcb->reset_timer);
    STREAM_TO_UINT8(status, p);

    if(status == HCI_SUCCESS) {
        /* Save the Feature Page 0 */
        STREAM_TO_ARRAY(p_devcb->local_lmp_features[0],
                        p, HCI_FEATURE_BYTES_PER_PAGE);

        if((HCI_LMP_EXTENDED_SUPPORTED(p_devcb->local_lmp_features[HCI_EXT_FEATURES_PAGE_0])) &&
                (HCI_READ_LOCAL_EXT_FEATURES_SUPPORTED(p_devcb->supported_cmds))) {
            /* if local controller has extended features and supports
            **HCI_Read_Local_Extended_Features command,
            ** then start reading these feature starting with extended features page 1 */
            BTM_TRACE_DEBUG("Start reading local extended features");
            btm_get_local_ext_features(HCI_EXT_FEATURES_PAGE_1);
        } else {
            btm_read_all_lmp_features_complete(HCI_EXT_FEATURES_PAGE_0);
        }
    }
}

/*******************************************************************************
**
** Function         btm_read_local_ext_features_complete
**
** Description      This function is called when read local extended features
**                  command complete message is received from the HCI.
**
** Returns          void
**
*******************************************************************************/
void btm_read_local_ext_features_complete(uint8_t *p, uint16_t evt_len)
{
//    tBTM_DEVCB     *p_devcb = &btm_cb.devcb;
//    tBTM_CMPL_CB   *p_cb = p_devcb->p_reset_cmpl_cb;
    uint8_t           status;
    uint8_t           page_number;
    uint8_t           page_number_max;
    UNUSED(evt_len);
    btu_stop_timer(&btm_cb.devcb.reset_timer);
    STREAM_TO_UINT8(status, p);

    if(status != HCI_SUCCESS) {
        BTM_TRACE_WARNING("btm_read_local_ext_features_complete status = 0x%02X", status);
        btm_read_all_lmp_features_complete(HCI_EXT_FEATURES_PAGE_0);
        return;
    }

    /* Extract Page number */
    STREAM_TO_UINT8(page_number, p);
    /* Extract Page number Max */
    STREAM_TO_UINT8(page_number_max, p);

    if(page_number > HCI_EXT_FEATURES_PAGE_MAX) {
        BTM_TRACE_ERROR("btm_read_local_ext_features_complete page=%d unknown",
                        page_number);
        return;
    }

    /* Save the extended features Page received */
    STREAM_TO_ARRAY(btm_cb.devcb.local_lmp_features[page_number],
                    p, HCI_FEATURE_BYTES_PER_PAGE);

    /* If this is re-read of the 1-st extended page after host supported LMP features are set */
    if((page_number == HCI_EXT_FEATURES_PAGE_1) &&
            (btm_cb.devcb.lmp_features_host_may_support == BTM_RE_READ_1ST_PAGE)) {
        btm_cb.devcb.lmp_features_host_may_support &= ~BTM_RE_READ_1ST_PAGE;
        btm_issue_host_support_for_lmp_features();
        return;
    }

    /* If this is the last page supported by the local BT controller OR */
    /* if this is the last page supported by the Host */
    if((page_number == page_number_max) ||
            (page_number == HCI_EXT_FEATURES_PAGE_MAX)) {
        BTM_TRACE_DEBUG("BTM reached last extended features page (%d)", page_number);
        btm_read_all_lmp_features_complete(page_number);
    }
    /* Else (another page must be read) */
    else {
        /* Read the next features page */
        page_number++;
        BTM_TRACE_DEBUG("BTM reads next extended features page (%d)", page_number);
        btm_get_local_ext_features(page_number);
    }
}

/*******************************************************************************
**
** Function         btm_read_local_supported_cmds_complete
**
** Description      This function is called when local supported commands read
**                  is complete.
**
** Returns          void
**
*******************************************************************************/
void btm_read_local_supported_cmds_complete(uint8_t *p)
{
    tBTM_DEVCB     *p_devcb = &btm_cb.devcb;
    uint8_t           status;
    btu_stop_timer(&(p_devcb->reset_timer));
    STREAM_TO_UINT8(status, p);
    BTM_TRACE_DEBUG("btm_read_local_supported_cmds_complete status (0x%02x)", status);

    if(status == HCI_SUCCESS) {
        /* Save the supported commands bit mask */
        STREAM_TO_ARRAY(p_devcb->supported_cmds, p, HCI_NUM_SUPP_COMMANDS_BYTES);
    }

    btm_get_local_features();
}

/*******************************************************************************
**
** Function         btm_write_simple_paring_mode_complete
**
** Description      This function is called when the command complete message
**                  is received from the HCI for the write simple pairing mode
**                  command.
**
** Returns          void
**
*******************************************************************************/
void btm_write_simple_paring_mode_complete(uint8_t *p)
{
    uint8_t   status;
    STREAM_TO_UINT8(status, p);

    if(status != HCI_SUCCESS) {
        BTM_TRACE_WARNING("btm_write_simple_paring_mode_complete status: 0x%02x", status);
    }

    if(btm_cb.devcb.lmp_features_host_may_support & BTM_HOST_MAY_SUPP_SSP) {
        btm_cb.devcb.lmp_features_host_may_support &= ~BTM_HOST_MAY_SUPP_SSP;
        btm_issue_host_support_for_lmp_features();
    }
}

/*******************************************************************************
**
** Function         btm_write_le_host_supported_complete
**
** Description      This function is called when the command complete message
**                  is received from the HCI for the write LE host supported
**                  command.
**
** Returns          void
**
*******************************************************************************/
void btm_write_le_host_supported_complete(uint8_t *p)
{
    uint8_t   status;
    STREAM_TO_UINT8(status, p);

    if(status != HCI_SUCCESS) {
        BTM_TRACE_WARNING("btm_write_le_host_supported_complete status: 0x%02x", status);
    }

    if(btm_cb.devcb.lmp_features_host_may_support & BTM_HOST_MAY_SUPP_LE) {
        btm_cb.devcb.lmp_features_host_may_support &= ~BTM_HOST_MAY_SUPP_LE;

        if(btm_cb.devcb.lmp_features_host_may_support & BTM_HOST_MAY_SUPP_SIMULT_BR_LE) {
            btm_cb.devcb.lmp_features_host_may_support &= ~BTM_HOST_MAY_SUPP_SIMULT_BR_LE;
        }

        btm_issue_host_support_for_lmp_features();
    }
}

/*******************************************************************************
**
** Function         btm_get_voice_coding_support
**
** Description      This function is provides a way to get the voice coding schemes
**                  supported the device.
**
** Returns          A bit mask - Bit 0 if set indicates CVSD support
**                               Bit 1 if set indicates PCM A-law support
**                               Bit 2 if set indicates PCM Mu-law support
**
*******************************************************************************/

uint8_t btm_get_voice_coding_support(void)
{
    uint8_t code = 0;

    if(HCI_LMP_CVSD_SUPPORTED(btm_cb.devcb.local_lmp_features[HCI_EXT_FEATURES_PAGE_0])) {
        code |= 0x01 ;
    }

    if(HCI_LMP_A_LAW_SUPPORTED(btm_cb.devcb.local_lmp_features[HCI_EXT_FEATURES_PAGE_0])) {
        code |= 0x02 ;
    }

    if(HCI_LMP_U_LAW_SUPPORTED(btm_cb.devcb.local_lmp_features[HCI_EXT_FEATURES_PAGE_0])) {
        code |= 0x04 ;
    }

    return code ;
}

/*******************************************************************************
**
** Function         BTM_SetLocalDeviceName
**
** Description      This function is called to set the local device name.
**
** Returns          status of the operation
**
*******************************************************************************/
tBTM_STATUS BTM_SetLocalDeviceName(char *p_name)
{
    uint8_t    *p;

    if(!p_name || !p_name[0] || (strlen((char *)p_name) > BD_NAME_LEN)) {
        return (BTM_ILLEGAL_VALUE);
    }

    if(btm_cb.devcb.state == BTM_DEV_STATE_WAIT_RESET_CMPLT ||
            btm_cb.devcb.state == BTM_DEV_STATE_WAIT_AFTER_RESET) {
        return (BTM_DEV_RESET);
    }

#if BTM_MAX_LOC_BD_NAME_LEN > 0
    /* Save the device name if local storage is enabled */
    p = (uint8_t *)btm_cb.cfg.bd_name;

    if(p != (uint8_t *)p_name) {
        strlcpy(btm_cb.cfg.bd_name, p_name, BTM_MAX_LOC_BD_NAME_LEN);
    }

#else
    p = (uint8_t *)p_name;
#endif

    if(btsnd_hcic_change_name(p)) {
        return (BTM_CMD_STARTED);
    } else {
        return (BTM_NO_RESOURCES);
    }
}



/*******************************************************************************
**
** Function         BTM_ReadLocalDeviceName
**
** Description      This function is called to read the local device name.
**
** Returns          status of the operation
**                  If success, BTM_SUCCESS is returned and p_name points stored
**                              local device name
**                  If BTM doesn't store local device name, BTM_NO_RESOURCES is
**                              is returned and p_name is set to NULL
**
*******************************************************************************/
tBTM_STATUS BTM_ReadLocalDeviceName(char **p_name)
{
#if BTM_MAX_LOC_BD_NAME_LEN > 0
    *p_name = btm_cb.cfg.bd_name;
    return(BTM_SUCCESS);
#else
    *p_name = NULL;
    return(BTM_NO_RESOURCES);
#endif
}

/*******************************************************************************
**
** Function         btm_read_local_name_timeout
**
** Description      Callback when reading the local name times out.
**
** Returns          void
**
*******************************************************************************/
void btm_read_local_name_timeout(UNUSED_ATTR void *data)
{
    tBTM_CMPL_CB  *p_cb = btm_cb.devcb.p_rln_cmpl_cb;
    btm_cb.devcb.p_rln_cmpl_cb = NULL;

    if(p_cb) {
        (*p_cb)((void *) NULL);
    }
}

/*******************************************************************************
**
** Function         BTM_ReadLocalDeviceNameFromController
**
** Description      Get local device name from controller. Do not use cached
**                  name (used to get chip-id prior to btm reset complete).
**
** Returns          BTM_CMD_STARTED if successful, otherwise an error
**
*******************************************************************************/
tBTM_STATUS BTM_ReadLocalDeviceNameFromController(tBTM_CMPL_CB *p_rln_cmpl_cback)
{
    /* Check if rln already in progress */
    if(btm_cb.devcb.p_rln_cmpl_cb) {
        return(BTM_NO_RESOURCES);
    }

    /* Save callback */
    btm_cb.devcb.p_rln_cmpl_cb = p_rln_cmpl_cback;
    btsnd_hcic_read_name();
#ifdef USE_ALARM
    alarm_set_on_queue(btm_cb.devcb.read_local_name_timer,
                       BTM_DEV_NAME_REPLY_TIMEOUT_MS,
                       btm_read_local_name_timeout, NULL,
                       btu_general_alarm_queue);
#else
    btm_cb.devcb.read_local_name_timer.param = 0;
    btm_cb.devcb.read_local_name_timer.p_cback = (TIMER_CBACK *)&btm_read_local_name_timeout;
    btu_start_timer(&btm_cb.devcb.read_local_name_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_REPLY_TIMEOUT);
#endif
    return BTM_CMD_STARTED;
}

/*******************************************************************************
**
** Function         btm_read_local_name_complete
**
** Description      This function is called when local name read complete.
**                  message is received from the HCI.
**
** Returns          void
**
*******************************************************************************/
void btm_read_local_name_complete(uint8_t *p, uint16_t evt_len)
{
    tBTM_CMPL_CB   *p_cb = btm_cb.devcb.p_rln_cmpl_cb;
    uint8_t           status;
    UNUSED(evt_len);
#ifdef USE_ALARM
    alarm_cancel(btm_cb.devcb.read_local_name_timer);
#else
    btu_stop_timer(&btm_cb.devcb.read_local_name_timer);
#endif
    /* If there was a callback address for read local name, call it */
    btm_cb.devcb.p_rln_cmpl_cb = NULL;

    if(p_cb) {
        STREAM_TO_UINT8(status, p);

        if(status == HCI_SUCCESS) {
            (*p_cb)(p);
        } else {
            (*p_cb)(NULL);
        }
    }
}


/*******************************************************************************
**
** Function         BTM_GetLocalDeviceAddr
**
** Description      This function is called to read the local device address
**
** Returns          void
**                  the local device address is copied into bd_addr
**
*******************************************************************************/
void BTM_GetLocalDeviceAddr(BD_ADDR bd_addr)
{
    wm_memcpy(bd_addr, btm_cb.devcb.local_addr, BD_ADDR_LEN);
}

/*******************************************************************************
**
** Function         BTM_ReadLocalDeviceAddr
**
** Description      This function is called to read the local device address
**
** Returns          status of the operation
**
*******************************************************************************/
tBTM_STATUS BTM_ReadLocalDeviceAddr(tBTM_CMPL_CB *p_cb)
{
    if(p_cb) {
        (*p_cb)(btm_cb.devcb.local_addr);
    }

    return (BTM_SUCCESS);
}


/*******************************************************************************
**
** Function         btm_read_local_addr_complete
**
** Description      This function is called when local BD Addr read complete
**                  message is received from the HCI.
**
** Returns          void
**
*******************************************************************************/
void btm_read_local_addr_complete(uint8_t *p, uint16_t evt_len)
{
    uint8_t           status;
    UNUSED(evt_len);
    STREAM_TO_UINT8(status, p);

    if(status == HCI_SUCCESS) {
        STREAM_TO_BDADDR(btm_cb.devcb.local_addr, p);
    }
}


/*******************************************************************************
**
** Function         BTM_ReadLocalVersion
**
** Description      This function is called to read the local device version
**
** Returns          status of the operation
**
*******************************************************************************/
tBTM_STATUS BTM_ReadLocalVersion(tBTM_VERSION_INFO *p_vers)
{
    /* Make sure the device has retrieved the info (not being reset) */
    if(btm_cb.devcb.state < BTM_DEV_STATE_READY) {
        return (BTM_DEV_RESET);
    }

    *p_vers = btm_cb.devcb.local_version;
    return (BTM_SUCCESS);
}




/*******************************************************************************
**
** Function         BTM_SetDeviceClass
**
** Description      This function is called to set the local device class
**
** Returns          status of the operation
**
*******************************************************************************/
tBTM_STATUS BTM_SetDeviceClass(DEV_CLASS dev_class)
{
    if(!memcmp(btm_cb.devcb.dev_class, dev_class, DEV_CLASS_LEN)) {
        return(BTM_SUCCESS);
    }

    wm_memcpy(btm_cb.devcb.dev_class, dev_class, DEV_CLASS_LEN);
    BTM_TRACE_DEBUG("%s [%s] line:%d, btm_cb.devcb.state:0x%08x\n", __FILE__, __FUNCTION__, __LINE__,
                    btm_cb.devcb.state);

    if(btm_cb.devcb.state == BTM_DEV_STATE_WAIT_RESET_CMPLT ||
            btm_cb.devcb.state == BTM_DEV_STATE_WAIT_AFTER_RESET) {
        return (BTM_DEV_RESET);
    }

    if(!btsnd_hcic_write_dev_class(dev_class)) {
        return (BTM_NO_RESOURCES);
    }

    return (BTM_SUCCESS);
}


/*******************************************************************************
**
** Function         BTM_ReadDeviceClass
**
** Description      This function is called to read the local device class
**
** Returns          pointer to the device class
**
*******************************************************************************/
uint8_t *BTM_ReadDeviceClass(void)
{
    return ((uint8_t *)btm_cb.devcb.dev_class);
}


/*******************************************************************************
**
** Function         BTM_ReadLocalFeatures
**
** Description      This function is called to read the local features
**
** Returns          pointer to the local features string
**
*******************************************************************************/
// TODO(zachoverflow): get rid of this function
uint8_t *BTM_ReadLocalFeatures(void)
{
    return (btm_cb.devcb.local_lmp_features[HCI_EXT_FEATURES_PAGE_0]);
}

/*******************************************************************************
**
** Function         BTM_ReadLocalExtendedFeatures
**
** Description      This function is called to read the local extended features
**
** Returns          pointer to the local extended features mask or NULL if bad
**                  page
**
*******************************************************************************/
uint8_t *BTM_ReadLocalExtendedFeatures(uint8_t page_number)
{
    if(page_number <= HCI_EXT_FEATURES_PAGE_MAX) {
        return (btm_cb.devcb.local_lmp_features[page_number]);
    }

    BTM_TRACE_ERROR("Warning: BTM_ReadLocalExtendedFeatures page %d unknown",
                    page_number);
    return NULL;
}

/*******************************************************************************
**
** Function         BTM_ReadBrcmFeatures
**
** Description      This function is called to read the Broadcom specific features
**
** Returns          pointer to the Broadcom features string
**
*******************************************************************************/
uint8_t *BTM_ReadBrcmFeatures(void)
{
#if 0
    return (btm_cb.devcb.brcm_features);
#endif
    return NULL;
}

/*******************************************************************************
**
** Function         BTM_RegisterForDeviceStatusNotif
**
** Description      This function is called to register for device status
**                  change notifications.
**
**                  If one registration is already there calling function should
**                  save the pointer to the function that is return and
**                  call it when processing of the event is complete
**
** Returns          status of the operation
**
*******************************************************************************/
tBTM_DEV_STATUS_CB *BTM_RegisterForDeviceStatusNotif(tBTM_DEV_STATUS_CB *p_cb)
{
    tBTM_DEV_STATUS_CB *p_prev = btm_cb.devcb.p_dev_status_cb;
    btm_cb.devcb.p_dev_status_cb = p_cb;
    return (p_prev);
}

/*******************************************************************************
**
** Function         BTM_VendorSpecificCommand
**
** Description      Send a vendor specific HCI command to the controller.
**
** Returns
**      BTM_SUCCESS         Command sent. Does not expect command complete
**                              event. (command cmpl callback param is NULL)
**      BTM_CMD_STARTED     Command sent. Waiting for command cmpl event.
**
** Notes
**      Opcode will be OR'd with HCI_GRP_VENDOR_SPECIFIC.
**
*******************************************************************************/
tBTM_STATUS BTM_VendorSpecificCommand(uint16_t opcode, uint8_t param_len,
                                      uint8_t *p_param_buf, tBTM_VSC_CMPL_CB *p_cb)
{
    /* Allocate a buffer to hold HCI command plus the callback function */
    void *p_buf = GKI_getbuf(sizeof(BT_HDR) + sizeof(tBTM_CMPL_CB *) +
                             param_len + HCIC_PREAMBLE_SIZE);
    BTM_TRACE_EVENT("BTM: %s: Opcode: 0x%04X, ParamLen: %i.", __func__,
                    opcode, param_len);
    /* Send the HCI command (opcode will be OR'd with HCI_GRP_VENDOR_SPECIFIC) */
    btsnd_hcic_vendor_spec_cmd(p_buf, opcode, param_len, p_param_buf, (void *)p_cb);

    /* Return value */
    if(p_cb != NULL) {
        return (BTM_CMD_STARTED);
    } else {
        return (BTM_SUCCESS);
    }
}


/*******************************************************************************
**
** Function         btm_vsc_complete
**
** Description      This function is called when local HCI Vendor Specific
**                  Command complete message is received from the HCI.
**
** Returns          void
**
*******************************************************************************/
void btm_vsc_complete(uint8_t *p, uint16_t opcode, uint16_t evt_len,
                      tBTM_CMPL_CB *p_vsc_cplt_cback)
{
    tBTM_VSC_CMPL   vcs_cplt_params;

    /* If there was a callback address for vcs complete, call it */
    if(p_vsc_cplt_cback) {
        /* Pass paramters to the callback function */
        vcs_cplt_params.opcode = opcode;        /* Number of bytes in return info */
        vcs_cplt_params.param_len = evt_len;    /* Number of bytes in return info */
        vcs_cplt_params.p_param_buf = p;
        (*p_vsc_cplt_cback)(&vcs_cplt_params);     /* Call the VSC complete callback function */
    }
}

/*******************************************************************************
**
** Function         BTM_RegisterForVSEvents
**
** Description      This function is called to register/deregister for vendor
**                  specific HCI events.
**
**                  If is_register=TRUE, then the function will be registered;
**                  if is_register=FALSE, then the function will be deregistered.
**
** Returns          BTM_SUCCESS if successful,
**                  BTM_BUSY if maximum number of callbacks have already been
**                           registered.
**
*******************************************************************************/
tBTM_STATUS BTM_RegisterForVSEvents(tBTM_VS_EVT_CB *p_cb, uint8_t is_register)
{
    tBTM_STATUS retval = BTM_SUCCESS;
    uint8_t i, free_idx = BTM_MAX_VSE_CALLBACKS;

    /* See if callback is already registered */
    for(i = 0; i < BTM_MAX_VSE_CALLBACKS; i++) {
        if(btm_cb.devcb.p_vend_spec_cb[i] == NULL) {
            /* Found a free slot. Store index */
            free_idx = i;
        } else if(btm_cb.devcb.p_vend_spec_cb[i] == p_cb) {
            /* Found callback in lookup table. If deregistering, clear the entry. */
            if(is_register == FALSE) {
                btm_cb.devcb.p_vend_spec_cb[i] = NULL;
                BTM_TRACE_EVENT("BTM Deregister For VSEvents is successfully");
            }

            return (BTM_SUCCESS);
        }
    }

    /* Didn't find callback. Add callback to free slot if registering */
    if(is_register) {
        if(free_idx < BTM_MAX_VSE_CALLBACKS) {
            btm_cb.devcb.p_vend_spec_cb[free_idx] = p_cb;
            BTM_TRACE_EVENT("BTM Register For VSEvents is successfully");
        } else {
            /* No free entries available */
            BTM_TRACE_ERROR("BTM_RegisterForVSEvents: too many callbacks registered");
            retval = BTM_NO_RESOURCES;
        }
    }

    return (retval);
}

/*******************************************************************************
**
** Function         btm_vendor_specific_evt
**
** Description      Process event HCI_VENDOR_SPECIFIC_EVT
**
**                  Note: Some controllers do not send command complete, so
**                  the callback and busy flag are cleared here also.
**
** Returns          void
**
*******************************************************************************/
void btm_vendor_specific_evt(uint8_t *p, uint8_t evt_len)
{
    uint8_t i;
    BTM_TRACE_DEBUG("BTM Event: Vendor Specific event from controller");

    for(i = 0; i < BTM_MAX_VSE_CALLBACKS; i++) {
        if(btm_cb.devcb.p_vend_spec_cb[i]) {
            (*btm_cb.devcb.p_vend_spec_cb[i])(evt_len, p);
        }
    }
}


/*******************************************************************************
**
** Function         BTM_WritePageTimeout
**
** Description      Send HCI Write Page Timeout.
**
** Returns
**      BTM_SUCCESS         Command sent.
**      BTM_NO_RESOURCES     If out of resources to send the command.
**
**
*******************************************************************************/
tBTM_STATUS BTM_WritePageTimeout(uint16_t timeout)
{
    BTM_TRACE_EVENT("BTM: BTM_WritePageTimeout: Timeout: %d.", timeout);

    /* Send the HCI command */
    if(btsnd_hcic_write_page_tout(timeout)) {
        return (BTM_SUCCESS);
    } else {
        return (BTM_NO_RESOURCES);
    }
}

/*******************************************************************************
**
** Function         BTM_WriteVoiceSettings
**
** Description      Send HCI Write Voice Settings command.
**                  See hcidefs.h for settings bitmask values.
**
** Returns
**      BTM_SUCCESS         Command sent.
**      BTM_NO_RESOURCES     If out of resources to send the command.
**
**
*******************************************************************************/
tBTM_STATUS BTM_WriteVoiceSettings(uint16_t settings)
{
    BTM_TRACE_EVENT("BTM: BTM_WriteVoiceSettings: Settings: 0x%04x.", settings);

    /* Send the HCI command */
    if(btsnd_hcic_write_voice_settings((uint16_t)(settings & 0x03ff))) {
        return (BTM_SUCCESS);
    }

    return (BTM_NO_RESOURCES);
}

/*******************************************************************************
**
** Function         BTM_EnableTestMode
**
** Description      Send HCI the enable device under test command.
**
**                  Note: Controller can only be taken out of this mode by
**                      resetting the controller.
**
** Returns
**      BTM_SUCCESS         Command sent.
**      BTM_NO_RESOURCES    If out of resources to send the command.
**
**
*******************************************************************************/
tBTM_STATUS BTM_EnableTestMode(void)
{
    uint8_t   cond;
    BTM_TRACE_EVENT("BTM: BTM_EnableTestMode");
    /* set auto accept connection as this is needed during test mode */
    /* Allocate a buffer to hold HCI command */
    cond = HCI_DO_AUTO_ACCEPT_CONNECT;

    if(!btsnd_hcic_set_event_filter(HCI_FILTER_CONNECTION_SETUP,
                                    HCI_FILTER_COND_NEW_DEVICE,
                                    &cond, sizeof(cond))) {
        return (BTM_NO_RESOURCES);
    }

    /* put device to connectable mode */
    if(BTM_SetConnectability(BTM_CONNECTABLE, BTM_DEFAULT_CONN_WINDOW,
                             BTM_DEFAULT_CONN_INTERVAL) != BTM_SUCCESS) {
        return BTM_NO_RESOURCES;
    }

    /* put device to discoverable mode */
    if(BTM_SetDiscoverability(BTM_GENERAL_DISCOVERABLE,
                              BTM_DEFAULT_DISC_WINDOW,
                              BTM_DEFAULT_DISC_INTERVAL) != BTM_SUCCESS) {
        return BTM_NO_RESOURCES;
    }

    /* mask off all of event from controller */
    if(!btsnd_hcic_set_event_mask(LOCAL_BR_EDR_CONTROLLER_ID,
                                  (uint8_t *)"\x00\x00\x00\x00\x00\x00\x00\x00")) {
        return BTM_NO_RESOURCES;
    }

    /* Send the HCI command */
    if(btsnd_hcic_enable_test_mode()) {
        return (BTM_SUCCESS);
    } else {
        return (BTM_NO_RESOURCES);
    }
}

/*******************************************************************************
**
** Function         btm_get_hci_version
**
** Description      Local function called to retrieve the current HCI version
**
** Returns          Bluetooth HCI Version returned by the controller
**
*******************************************************************************/
uint8_t btm_get_hci_version(void)
{
    return (btm_cb.devcb.local_version.hci_version);
}



/*******************************************************************************
**
** Function         BTM_ReadStoredLinkKey
**
** Description      This function is called to obtain link key for the specified
**                  device from the NVRAM storage attached to the Bluetooth
**                  controller.
**
** Parameters:      bd_addr      - Address of the device
**                  p_cb         - Call back function to be called to return
**                                 the results
**
*******************************************************************************/
tBTM_STATUS BTM_ReadStoredLinkKey(BD_ADDR bd_addr, tBTM_CMPL_CB *p_cb)
{
    BD_ADDR local_bd_addr;
    uint8_t read_all_flag = FALSE;

    /* Check if the previous command is completed */
    if(btm_cb.devcb.p_stored_link_key_cmpl_cb) {
        return (BTM_BUSY);
    }

    if(!bd_addr) {
        /* This is to read all the link keys */
        read_all_flag = TRUE;
        /* We don't care the BD address. Just pass a non zero pointer */
        bd_addr = local_bd_addr;
    }

    BTM_TRACE_EVENT("BTM: BTM_ReadStoredLinkKey: Read_All: %s",
                    read_all_flag ? "TRUE" : "FALSE");
    /* Send the HCI command */
    btm_cb.devcb.p_stored_link_key_cmpl_cb = p_cb;

    if(btsnd_hcic_read_stored_key(bd_addr, read_all_flag)) {
        return (BTM_SUCCESS);
    } else {
        return (BTM_NO_RESOURCES);
    }
}


/*******************************************************************************
**
** Function         BTM_WriteStoredLinkKey
**
** Description      This function is called to write link keys for the specified
**                  device addresses to the NVRAM storage attached to the Bluetooth
**                  controller.
**
** Parameters:      num_keys     - Number of link keys
**                  bd_addr      - Addresses of the devices
**                  link_key     - Link Keys to be stored
**                  p_cb         - Call back function to be called to return
**                                 the results
**
*******************************************************************************/
tBTM_STATUS BTM_WriteStoredLinkKey(uint8_t num_keys,
                                   BD_ADDR *bd_addr,
                                   LINK_KEY *link_key,
                                   tBTM_CMPL_CB *p_cb)
{
    /* Check if the previous command is completed */
    if(btm_cb.devcb.p_stored_link_key_cmpl_cb) {
        return (BTM_BUSY);
    }

    BTM_TRACE_EVENT("BTM: BTM_WriteStoredLinkKey: num_keys: %d", num_keys);

    /* Check the maximum number of link keys */
    if(num_keys > HCI_MAX_NUM_OF_LINK_KEYS_PER_CMMD) {
        num_keys = HCI_MAX_NUM_OF_LINK_KEYS_PER_CMMD;
    }

    /* Send the HCI command */
    btm_cb.devcb.p_stored_link_key_cmpl_cb = p_cb;

    if(btsnd_hcic_write_stored_key(num_keys, bd_addr, link_key)) {
        return (BTM_SUCCESS);
    } else {
        return (BTM_NO_RESOURCES);
    }
}


/*******************************************************************************
**
** Function         BTM_DeleteStoredLinkKey
**
** Description      This function is called to delete link key for the specified
**                  device addresses from the NVRAM storage attached to the Bluetooth
**                  controller.
**
** Parameters:      bd_addr      - Addresses of the devices
**                  p_cb         - Call back function to be called to return
**                                 the results
**
*******************************************************************************/
tBTM_STATUS BTM_DeleteStoredLinkKey(BD_ADDR bd_addr, tBTM_CMPL_CB *p_cb)
{
    BD_ADDR local_bd_addr;
    uint8_t delete_all_flag = FALSE;

    /* Check if the previous command is completed */
    if(btm_cb.devcb.p_stored_link_key_cmpl_cb) {
        return (BTM_BUSY);
    }

    if(!bd_addr) {
        /* This is to delete all link keys */
        delete_all_flag = TRUE;
        /* We don't care the BD address. Just pass a non zero pointer */
        bd_addr = local_bd_addr;
    }

    BTM_TRACE_EVENT("BTM: BTM_DeleteStoredLinkKey: delete_all_flag: %s",
                    delete_all_flag ? "TRUE" : "FALSE");
    /* Send the HCI command */
    btm_cb.devcb.p_stored_link_key_cmpl_cb = p_cb;

    if(!btsnd_hcic_delete_stored_key(bd_addr, delete_all_flag)) {
        return (BTM_NO_RESOURCES);
    } else {
        return (BTM_SUCCESS);
    }
}


/*******************************************************************************
**
** Function         btm_read_stored_link_key_complete
**
** Description      This function is called when the command complete message
**                  is received from the HCI for the read stored link key command.
**
** Returns          void
**
*******************************************************************************/
void btm_read_stored_link_key_complete(uint8_t *p)
{
    tBTM_CMPL_CB      *p_cb = btm_cb.devcb.p_stored_link_key_cmpl_cb;
    tBTM_READ_STORED_LINK_KEY_COMPLETE  result;
    /* If there was a callback registered for read stored link key, call it */
    btm_cb.devcb.p_stored_link_key_cmpl_cb = NULL;

    if(p_cb) {
        /* Set the call back event to indicate command complete */
        result.event = BTM_CB_EVT_READ_STORED_LINK_KEYS;
        /* Extract the result fields from the HCI event if status is success */
        STREAM_TO_UINT8(result.status, p);

        if(result.status == HCI_SUCCESS) {
            STREAM_TO_UINT16(result.max_keys, p);
            STREAM_TO_UINT16(result.read_keys, p);
        } else {
            BTM_TRACE_WARNING("Read stored link key status %d", result.status);
            result.max_keys = 0;
            result.read_keys = 0;
        }

        /* Call the call back and pass the result */
        (*p_cb)(&result);
    }
}


/*******************************************************************************
**
** Function         btm_write_stored_link_key_complete
**
** Description      This function is called when the command complete message
**                  is received from the HCI for the write stored link key command.
**
** Returns          void
**
*******************************************************************************/
void btm_write_stored_link_key_complete(uint8_t *p)
{
    tBTM_CMPL_CB       *p_cb = btm_cb.devcb.p_stored_link_key_cmpl_cb;
    tBTM_WRITE_STORED_LINK_KEY_COMPLETE  result;
    /* If there was a callback registered for read stored link key, call it */
    btm_cb.devcb.p_stored_link_key_cmpl_cb = NULL;

    if(p_cb) {
        /* Set the call back event to indicate command complete */
        result.event = BTM_CB_EVT_WRITE_STORED_LINK_KEYS;
        /* Extract the result fields from the HCI event */
        STREAM_TO_UINT8(result.status, p);
        STREAM_TO_UINT8(result.num_keys, p);
        /* Call the call back and pass the result */
        (*p_cb)(&result);
    }
}


/*******************************************************************************
**
** Function         btm_delete_stored_link_key_complete
**
** Description      This function is called when the command complete message
**                  is received from the HCI for the delete stored link key command.
**
** Returns          void
**
*******************************************************************************/
void btm_delete_stored_link_key_complete(uint8_t *p)
{
    tBTM_CMPL_CB         *p_cb = btm_cb.devcb.p_stored_link_key_cmpl_cb;
    tBTM_DELETE_STORED_LINK_KEY_COMPLETE  result;
    /* If there was a callback registered for read stored link key, call it */
    btm_cb.devcb.p_stored_link_key_cmpl_cb = NULL;

    if(p_cb) {
        /* Set the call back event to indicate command complete */
        result.event = BTM_CB_EVT_DELETE_STORED_LINK_KEYS;
        /* Extract the result fields from the HCI event */
        STREAM_TO_UINT8(result.status, p);
        STREAM_TO_UINT16(result.num_keys, p);
        /* Call the call back and pass the result */
        (*p_cb)(&result);
    }
}


/*******************************************************************************
**
** Function         btm_return_link_keys_evt
**
** Description      This function is called when the return link keys event
**                  is received from the HCI for the read stored link key command.
**
** Returns          void
**
*******************************************************************************/
void btm_return_link_keys_evt(tBTM_RETURN_LINK_KEYS_EVT *result)
{
    tBTM_CMPL_CB  *p_cb = btm_cb.devcb.p_stored_link_key_cmpl_cb;
    uint8_t          i, *p, *p1;
    uint8_t          bd_addr[BD_ADDR_LEN];
    uint8_t          link_key[LINK_KEY_LEN];

    /* Call the call back to pass the link keys to application */
    if(p_cb) {
        /* Change the BD addr and Link key in to big endian order */
        p = (uint8_t *)(result + 1);

        for(i = 0; i < result->num_keys; i++) {
            /* Initialize the backup pointer */
            p1 = p;
            /* Extract the BD Addr and Link Key */
            REVERSE_STREAM_TO_ARRAY(bd_addr, p1, BD_ADDR_LEN);
            REVERSE_STREAM_TO_ARRAY(link_key, p1, LINK_KEY_LEN);
            /* Write the BD Addr and Link Key back in big endian format */
            ARRAY_TO_STREAM(p, bd_addr, BD_ADDR_LEN);
            ARRAY_TO_STREAM(p, link_key, LINK_KEY_LEN);
        }

        (*p_cb)(result);
    }
}



/*******************************************************************************
**
** Function         btm_report_device_status
**
** Description      This function is called when there is a change in the device
**                  status. This function will report the new device status to
**                  the application
**
** Returns          void
**
*******************************************************************************/
void btm_report_device_status(tBTM_DEV_STATUS status)
{
    tBTM_DEV_STATUS_CB *p_cb = btm_cb.devcb.p_dev_status_cb;

    /* Call the call back to pass the device status to application */
    if(p_cb) {
        (*p_cb)(status);
    }
}


