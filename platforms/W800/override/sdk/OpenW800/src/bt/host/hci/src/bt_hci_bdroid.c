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

/******************************************************************************
 *
 *  Filename:      bt_hci_bdroid.c
 *
 *  Description:   Bluedroid Bluetooth Host/Controller interface library
 *                 implementation
 *
 ******************************************************************************/

#define LOG_TAG "bt_hci_bdroid"

#include "bt_hci_bdroid.h"
#include "bt_utils.h"
#include "hcih.h"

//#include "utils.h"
#include "vendor.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

extern int num_hci_cmd_pkts;

bt_hc_callbacks_t *bt_hc_cbacks = NULL;
tHCI_IF *p_hci_if;
volatile bool fwcfg_acked;
// Cleanup state indication.
volatile bool has_cleaned_up = false;

static bool tx_cmd_pkts_pending = false;


//#define SEND_USE_QUEUE

#ifdef SEND_USE_QUEUE
static BUFFER_Q tx_q;
#endif


#ifndef BTHC_DBG
#define BTHC_DBG FALSE
#endif


#if defined (BTHC_DBG) && (BTHC_DBG == TRUE)

#define BTHCDBG(fmt, ...)   \
    do{\
        if(1) \
            LogMsg(0xffff, "%s(L%d): " fmt, __FUNCTION__, __LINE__,  ## __VA_ARGS__); \
    }while(0)
#else
#define BTHCDBG(fmt, ...)
#endif




// Closes the interface.
// This routine is not thread safe.
static void cleanup(void)
{
}

static void event_lpm_enable(void *context)
{
#ifdef WM_LPM_ENABLED
    lpm_enable(true);
#endif
}

static void event_lpm_disable(void *context)
{
#ifdef WM_LPM_ENABLED
    lpm_enable(false);
#endif
}

static void event_lpm_wake_device(void *context)
{
#ifdef WM_LPM_ENABLED
    lpm_wake_assert();
#endif
}

static void event_lpm_allow_sleep(void *context)
{
#ifdef WM_LPM_ENABLED
    lpm_allow_bt_device_sleep();
#endif
}

static void event_lpm_idle_timeout(void *context)
{
#ifdef WM_LPM_ENABLED
    lpm_wake_deassert();
#endif
}
#if 0
static void event_epilog(void *context)
{
    //vendor_send_command(BT_VND_OP_EPILOG, NULL);
    (void)context;
}
#endif
static void event_tx_cmd(void *msg)
{
    HC_BT_HDR *p_msg = (HC_BT_HDR *)msg;
    BTHCDBG("%s: p_msg: %p, event: 0x%x\n", __func__, p_msg, p_msg->event);
    int event = p_msg->event & MSG_EVT_MASK;
    int sub_event = p_msg->event & MSG_SUB_EVT_MASK;

    if(event == MSG_CTRL_TO_HC_CMD && sub_event == BT_HC_AUDIO_STATE) {
        //vendor_send_command(BT_VND_OP_SET_AUDIO_STATE, p_msg->data);
        BTHCDBG("%s (event: 0x%x, sub_event: 0x%x) >>>>>\n", __func__, event, sub_event);
    } else {
        BTHCDBG("%s (event: 0x%x, sub_event: 0x%x) not supported\n", __func__, event, sub_event);
    }

    bt_hc_cbacks->dealloc(msg);
}


void bthc_idle_timeout(void)
{
    event_lpm_idle_timeout(NULL);
}


/*****************************************************************************
**
**   BLUETOOTH HOST/CONTROLLER INTERFACE LIBRARY FUNCTIONS
**
*****************************************************************************/

static int init(const bt_hc_callbacks_t *p_cb, unsigned char *local_bdaddr)
{
    //int result;
    BTHCDBG("%s %s line:%d\n", __FILE__, __FUNCTION__, __LINE__);

    if(p_cb == NULL) {
        BTHCDBG("init failed with no user callbacks!\n");
        return BT_HC_STATUS_FAIL;
    }

    // hc_cb.epilog_timer_created = false;
    fwcfg_acked = false;
    has_cleaned_up = false;
    /* store reference to user callbacks */
    bt_hc_cbacks = (bt_hc_callbacks_t *) p_cb;
    //vendor_open(local_bdaddr);
    //utils_init();
#ifdef HCI_USE_MCT
    extern tHCI_IF hci_mct_func_table;
    p_hci_if = &hci_mct_func_table;
#else
    extern tHCI_IF hci_h4_func_table;
    p_hci_if = &hci_h4_func_table;
    p_hci_if->init();
#endif
#ifdef WM_LPM_ENABLED
    lpm_init();
#endif
#ifdef SEND_USE_QUEUE
    utils_queue_init(&tx_q);
#endif
    return BT_HC_STATUS_SUCCESS;
}


/** Controls HCI logging on/off */
static int logging(bt_hc_logging_state_t state, char *p_path, bool save_existing)
{
    return BT_HC_STATUS_SUCCESS;
}

/** Called prior to stack initialization */
static void preload(TRANSAC transac)
{
    //vendor_send_command(BT_VND_OP_FW_CFG, NULL);
    BTHCDBG("%s (event: %s) >>>>>\n", __func__, "BT_VND_OP_FW_CFG");
    //send back fw cfg success directly, we are unity(host+controller)
    bt_hc_cbacks->preload_cb(transac, BT_HC_PRELOAD_SUCCESS);
    return ;
}

/** Called post stack initialization */
static void postload(TRANSAC transac)
{
    /* Start from SCO related H/W configuration, if SCO configuration
     * is required. Then, follow with reading requests of getting
     * ACL data length for both BR/EDR and LE.
     */
    BTHCDBG("%s (event: %s) >>>>>\n", __func__, "BT_VND_OP_SCO_CFG");
    //int result = vendor_send_command(BT_VND_OP_SCO_CFG, NULL);
    //if (result == -1)
    //  p_hci_if->get_acl_max_len();
    return;
}
/** Transmit frame */
static int transmit_buf(TRANSAC transac, char *p_buf, int len)
{
#ifdef SEND_USE_QUEUE
    utils_enqueue(&tx_q, (HC_BT_HDR *)transac);
#else
    p_hci_if->send((HC_BT_HDR *)transac);
#endif
    return BT_HC_STATUS_SUCCESS;
}


/** Configure low power mode wake state */
static int lpm(bt_hc_low_power_event_t event)
{
#if 1

    //printf("lpm--->event:0x%08x\n", event);
    switch(event) {
        case BT_HC_LPM_DISABLE:
            //thread_post(hc_cb.worker_thread, event_lpm_disable, NULL);
            event_lpm_disable(NULL);
            break;

        case BT_HC_LPM_ENABLE:
            //thread_post(hc_cb.worker_thread, event_lpm_enable, NULL);
            event_lpm_enable(NULL);
            break;

        case BT_HC_LPM_WAKE_ASSERT:
            //thread_post(hc_cb.worker_thread, event_lpm_wake_device, NULL);
            event_lpm_wake_device(NULL);
            break;

        case BT_HC_LPM_WAKE_DEASSERT:
            //thread_post(hc_cb.worker_thread, event_lpm_allow_sleep, NULL);
            event_lpm_allow_sleep(NULL);
            break;
    }

#endif
    return BT_HC_STATUS_SUCCESS;
}

/** Chip power control */
/** Chip power control */
static void set_power(bt_hc_chip_power_state_t state)
{
    int pwr_state;
    BTHCDBG("set_power %d\n", state);
    /* Calling vendor-specific part */
    pwr_state = (state == BT_HC_CHIP_PWR_ON) ? BT_VND_PWR_ON : BT_VND_PWR_OFF;
	UNUSED(pwr_state);
    //vendor_send_command(BT_VND_OP_POWER_CTRL, &pwr_state);
    BTHCDBG("%s (event: %s) >>>>>\n", __func__, "BT_VND_OP_POWER_CTRL");
}


static int tx_hc_cmd(TRANSAC transac, char *p_buf, int len)
{
    event_tx_cmd(transac);
    return 1;
}

static const bt_hc_interface_t bluetoothHCLibInterface = {
    sizeof(bt_hc_interface_t),
    init,
    set_power,
    lpm,
    preload,
    (void *)postload,
    transmit_buf,
    (void *)logging,
    cleanup,
    tx_hc_cmd,
};

/*******************************************************************************
**
** Function        bt_hc_get_interface
**
** Description     Caller calls this function to get API instance
**
** Returns         API table
**
*******************************************************************************/
const bt_hc_interface_t *bt_hc_get_interface(void)
{
    return &bluetoothHCLibInterface;
}

void bthc_tx(HC_BT_HDR *buf)
{
#ifdef SEND_USE_QUEUE
    utils_enqueue(&tx_q, buf);
#else
    p_hci_if->send(buf);
#endif
}



int credit_counter = 0;
void event_tx(void)
{
#ifdef SEND_USE_QUEUE
    /*
     *  We will go through every packets in the tx queue.
     *  Fine to clear tx_cmd_pkts_pending.
     */
    int i = 0;
    tx_cmd_pkts_pending = false;
    HC_BT_HDR *sending_msg_que[64];
    int sending_msg_count = 0;
    int sending_hci_cmd_pkts_count = 0;
    utils_lock();
    HC_BT_HDR *p_next_msg = tx_q.p_first;

    while(p_next_msg && sending_msg_count < ARRAY_SIZE(sending_msg_que)) {
        if((p_next_msg->event & MSG_EVT_MASK) == MSG_STACK_TO_HC_HCI_CMD) {
            /*
             *  if we have used up controller's outstanding HCI command
             *  credits (normally is 1), skip all HCI command packets in
             *  the queue.
             *  The pending command packets will be sent once controller
             *  gives back us credits through CommandCompleteEvent or
             *  CommandStatusEvent.
             */
            if(tx_cmd_pkts_pending ||
                    (sending_hci_cmd_pkts_count >= num_hci_cmd_pkts)) {
                BTHCDBG("tx_pending:%d, shcps:%d, hci_cmd_pkts:%d\n", tx_cmd_pkts_pending,
                        sending_hci_cmd_pkts_count, num_hci_cmd_pkts);
                tx_cmd_pkts_pending = true;
                p_next_msg = utils_getnext(p_next_msg);
                continue;
            }

            sending_hci_cmd_pkts_count++;
        }

        HC_BT_HDR *p_msg = p_next_msg;
        p_next_msg = utils_getnext(p_msg);
        utils_remove_from_queue_unlocked(&tx_q, p_msg);
        sending_msg_que[sending_msg_count++] = p_msg;
    }

    utils_unlock();

    for(i = 0; i < sending_msg_count; i++) {
        p_hci_if->send(sending_msg_que[i]);
    }

    if(tx_cmd_pkts_pending) {
        credit_counter++;
        //printf("Used up Tx Cmd credits\n");
    }

#endif
}

int eventrx = 0;
void event_rx()
{
    eventrx++;
   // process_intr_bulk_recv_packet();
    /*change from skb queue to userial queue*/
   // userial_read_thread(NULL);
#ifndef HCI_USE_MCT
    p_hci_if->rcv();

    if(tx_cmd_pkts_pending && num_hci_cmd_pkts > 0) {
        // Got HCI Cmd credits from controller. Send whatever data
        // we have in our tx queue. We can call |event_tx| directly
        // here since we're already on the worker thread.
        event_tx();
    }

#endif
}


