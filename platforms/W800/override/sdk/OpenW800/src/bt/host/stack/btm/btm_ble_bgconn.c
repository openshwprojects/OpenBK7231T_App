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
 *  This file contains functions for BLE whitelist operation.
 *
 ******************************************************************************/

#include <assert.h>
#include <string.h>

//#include "device/include/controller.h"
//#include "osi/include/allocator.h"
#include "btcore/include/bdaddr.h"
#include "osi/include/hash_map.h"
#include "bt_types.h"
#include "btu.h"
#include "btm_int.h"
#include "l2c_int.h"
#include "hcimsgs.h"
#include "bt_utils.h"

#ifndef BTM_BLE_SCAN_PARAM_TOUT
#define BTM_BLE_SCAN_PARAM_TOUT      50    /* 50 seconds */
#endif

#if (BLE_INCLUDED == TRUE)

static void btm_suspend_wl_activity(tBTM_BLE_WL_STATE wl_state);
static void btm_resume_wl_activity(tBTM_BLE_WL_STATE wl_state);

// Unfortunately (for now?) we have to maintain a copy of the device whitelist
// on the host to determine if a device is pending to be connected or not. This
// controls whether the host should keep trying to scan for whitelisted
// peripherals or not.
// TODO: Move all of this to controller/le/background_list or similar?
static const size_t background_connection_buckets = 42;
static hash_map_t *background_connections = NULL;

typedef struct background_connection_t {
    tls_bt_addr_t address;
} background_connection_t;

static uint8_t bdaddr_equality_fn(const void *x, const void *y)
{
    return bdaddr_equals((tls_bt_addr_t *)x, (tls_bt_addr_t *)y);
}

static void background_connections_lazy_init()
{
    if(!background_connections) {
        background_connections = hash_map_new(background_connection_buckets,
                                              hash_function_bdaddr, NULL, GKI_freebuf, bdaddr_equality_fn);
        assert(background_connections);
    }
}

static void background_connection_add(tls_bt_addr_t *address)
{
    assert(address);
    background_connections_lazy_init();
    background_connection_t *connection = hash_map_get(background_connections, address);

    if(!connection) {
        connection = GKI_getbuf(sizeof(background_connection_t));
        connection->address = *address;
        hash_map_set(background_connections, &(connection->address), connection);
    }
}

static void background_connection_remove(tls_bt_addr_t *address)
{
    if(address && background_connections) {
        hash_map_erase(background_connections, address);
    }
}

static void background_connections_clear()
{
    if(background_connections) {
        hash_map_clear(background_connections);
    }
}

static uint8_t background_connections_pending_cb(hash_map_entry_t *hash_entry, void *context)
{
    uint8_t *pending_connections = context;
    background_connection_t *connection = hash_entry->data;
    const uint8_t connected = BTM_IsAclConnectionUp(connection->address.address, BT_TRANSPORT_LE);

    if(!connected) {
        *pending_connections = true;
        return false;
    }

    return true;
}

static uint8_t background_connections_pending()
{
    uint8_t pending_connections = false;

    if(background_connections) {
        hash_map_foreach(background_connections, background_connections_pending_cb, &pending_connections);
    }

    return pending_connections;
}

/*******************************************************************************
**
** Function         btm_update_scanner_filter_policy
**
** Description      This function updates the filter policy of scanner
*******************************************************************************/
void btm_update_scanner_filter_policy(tBTM_BLE_SFP scan_policy)
{
    tBTM_BLE_INQ_CB *p_inq = &btm_cb.ble_ctr_cb.inq_var;
    uint32_t scan_interval = !p_inq->scan_interval ? BTM_BLE_GAP_DISC_SCAN_INT : p_inq->scan_interval;
    uint32_t scan_window = !p_inq->scan_window ? BTM_BLE_GAP_DISC_SCAN_WIN : p_inq->scan_window;
    BTM_TRACE_EVENT("%s", __func__);
    p_inq->sfp = scan_policy;
    p_inq->scan_type = p_inq->scan_type == BTM_BLE_SCAN_MODE_NONE ? BTM_BLE_SCAN_MODE_ACTI :
                       p_inq->scan_type;

    if(btm_cb.cmn_ble_vsc_cb.extended_scan_support == 0) {
        btsnd_hcic_ble_set_scan_params(p_inq->scan_type, (uint16_t)scan_interval,
                                       (uint16_t)scan_window,
                                       btm_cb.ble_ctr_cb.addr_mgnt_cb.own_addr_type,
                                       scan_policy);
    } else {
        btm_ble_send_extended_scan_params(p_inq->scan_type, scan_interval, scan_window,
                                          btm_cb.ble_ctr_cb.addr_mgnt_cb.own_addr_type,
                                          scan_policy);
    }
}
/*******************************************************************************
**
** Function         btm_add_dev_to_controller
**
** Description      This function load the device into controller white list
*******************************************************************************/
uint8_t btm_add_dev_to_controller(uint8_t to_add, BD_ADDR bd_addr)
{
    tBTM_SEC_DEV_REC    *p_dev_rec = btm_find_dev(bd_addr);
    uint8_t             started = FALSE;
    BD_ADDR             dummy_bda = {0};

    if(p_dev_rec != NULL && p_dev_rec->device_type & BT_DEVICE_TYPE_BLE) {
        if(to_add) {
            if(p_dev_rec->ble.ble_addr_type == BLE_ADDR_PUBLIC || !BTM_BLE_IS_RESOLVE_BDA(bd_addr)) {
                started = btsnd_hcic_ble_add_white_list(p_dev_rec->ble.ble_addr_type, bd_addr);
                p_dev_rec->ble.in_controller_list |= BTM_WHITE_LIST_BIT;
            } else if(memcmp(p_dev_rec->ble.static_addr, bd_addr, BD_ADDR_LEN) != 0 &&
                      memcmp(p_dev_rec->ble.static_addr, dummy_bda, BD_ADDR_LEN) != 0) {
                started = btsnd_hcic_ble_add_white_list(p_dev_rec->ble.static_addr_type,
                                                        p_dev_rec->ble.static_addr);
                p_dev_rec->ble.in_controller_list |= BTM_WHITE_LIST_BIT;
            }
        } else {
            if(p_dev_rec->ble.ble_addr_type == BLE_ADDR_PUBLIC || !BTM_BLE_IS_RESOLVE_BDA(bd_addr)) {
                started = btsnd_hcic_ble_remove_from_white_list(p_dev_rec->ble.ble_addr_type, bd_addr);
            }

            if(memcmp(p_dev_rec->ble.static_addr, dummy_bda, BD_ADDR_LEN) != 0 &&
                    memcmp(p_dev_rec->ble.static_addr, bd_addr, BD_ADDR_LEN) != 0) {
                started = btsnd_hcic_ble_remove_from_white_list(p_dev_rec->ble.static_addr_type,
                          p_dev_rec->ble.static_addr);
            }

            p_dev_rec->ble.in_controller_list &= ~BTM_WHITE_LIST_BIT;
        }
    } else {
        /* not a known device, i.e. attempt to connect to device never seen before */
        uint8_t addr_type = BTM_IS_PUBLIC_BDA(bd_addr) ? BLE_ADDR_PUBLIC : BLE_ADDR_RANDOM;
        started = btsnd_hcic_ble_remove_from_white_list(addr_type, bd_addr);

        if(to_add) {
            started = btsnd_hcic_ble_add_white_list(addr_type, bd_addr);
        }
    }

    return started;
}
/*******************************************************************************
**
** Function         btm_execute_wl_dev_operation
**
** Description      execute the pending whitelist device operation(loading or removing)
*******************************************************************************/
uint8_t btm_execute_wl_dev_operation(void)
{
    tBTM_BLE_WL_OP *p_dev_op = btm_cb.ble_ctr_cb.wl_op_q;
    uint8_t   i = 0;
    uint8_t rt = TRUE;

    for(i = 0; i < BTM_BLE_MAX_BG_CONN_DEV_NUM && rt; i ++, p_dev_op ++) {
        if(p_dev_op->in_use) {
            rt = btm_add_dev_to_controller(p_dev_op->to_add, p_dev_op->bd_addr);
            wm_memset(p_dev_op, 0, sizeof(tBTM_BLE_WL_OP));
        } else {
            break;
        }
    }

    return rt;
}
/*******************************************************************************
**
** Function         btm_enq_wl_dev_operation
**
** Description      enqueue the pending whitelist device operation(loading or removing).
*******************************************************************************/
void btm_enq_wl_dev_operation(uint8_t to_add, BD_ADDR bd_addr)
{
    tBTM_BLE_WL_OP *p_dev_op = btm_cb.ble_ctr_cb.wl_op_q;
    uint8_t   i = 0;

    for(i = 0; i < BTM_BLE_MAX_BG_CONN_DEV_NUM; i ++, p_dev_op ++) {
        if(p_dev_op->in_use && !memcmp(p_dev_op->bd_addr, bd_addr, BD_ADDR_LEN)) {
            p_dev_op->to_add = to_add;
            return;
        } else if(!p_dev_op->in_use) {
            break;
        }
    }

    if(i != BTM_BLE_MAX_BG_CONN_DEV_NUM) {
        p_dev_op->in_use = TRUE;
        p_dev_op->to_add = to_add;
        wm_memcpy(p_dev_op->bd_addr, bd_addr, BD_ADDR_LEN);
    } else {
        BTM_TRACE_ERROR("max pending WL operation reached, discard");
    }

    return;
}

/*******************************************************************************
**
** Function         btm_update_dev_to_white_list
**
** Description      This function adds or removes a device into/from
**                  the white list.
**
*******************************************************************************/
uint8_t btm_update_dev_to_white_list(uint8_t to_add, BD_ADDR bd_addr)
{
    tBTM_BLE_CB *p_cb = &btm_cb.ble_ctr_cb;

    if(to_add && p_cb->white_list_avail_size == 0) {
        BTM_TRACE_ERROR("%s Whitelist full, unable to add device", __func__);
        return FALSE;
    }

    if(to_add) {
        background_connection_add((tls_bt_addr_t *)bd_addr);
    } else {
        background_connection_remove((tls_bt_addr_t *)bd_addr);
    }

    btm_suspend_wl_activity(p_cb->wl_state);
    btm_enq_wl_dev_operation(to_add, bd_addr);
    btm_resume_wl_activity(p_cb->wl_state);
    return TRUE;
}

/*******************************************************************************
**
** Function         btm_ble_clear_white_list
**
** Description      This function clears the white list.
**
*******************************************************************************/
void btm_ble_clear_white_list(void)
{
    BTM_TRACE_EVENT("btm_ble_clear_white_list");
    btsnd_hcic_ble_clear_white_list();
    background_connections_clear();
}

/*******************************************************************************
**
** Function         btm_ble_clear_white_list_complete
**
** Description      Indicates white list cleared.
**
*******************************************************************************/
void btm_ble_clear_white_list_complete(uint8_t *p_data, uint16_t evt_len)
{
    tBTM_BLE_CB *p_cb = &btm_cb.ble_ctr_cb;
    uint8_t       status;
    UNUSED(evt_len);
    BTM_TRACE_EVENT("btm_ble_clear_white_list_complete");
    STREAM_TO_UINT8(status, p_data);

    if(status == HCI_SUCCESS) {
        p_cb->white_list_avail_size =
                        btm_cb.devcb.ble_white_list_size;    //controller_get_interface()->get_ble_white_list_size();
    }
}

/*******************************************************************************
**
** Function         btm_ble_white_list_init
**
** Description      Initialize white list size
**
*******************************************************************************/
void btm_ble_white_list_init(uint8_t white_list_size)
{
    BTM_TRACE_DEBUG("%s white_list_size = %d", __func__, white_list_size);
    btm_cb.ble_ctr_cb.white_list_avail_size = white_list_size;
}

/*******************************************************************************
**
** Function         btm_ble_add_2_white_list_complete
**
** Description      White list element added
**
*******************************************************************************/
void btm_ble_add_2_white_list_complete(uint8_t status)
{
    BTM_TRACE_EVENT("%s status=%d", __func__, status);

    if(status == HCI_SUCCESS) {
        --btm_cb.ble_ctr_cb.white_list_avail_size;
    }
}

/*******************************************************************************
**
** Function         btm_ble_remove_from_white_list_complete
**
** Description      White list element removal complete
**
*******************************************************************************/
void btm_ble_remove_from_white_list_complete(uint8_t *p, uint16_t evt_len)
{
    UNUSED(evt_len);
    BTM_TRACE_EVENT("%s status=%d", __func__, *p);

    if(*p == HCI_SUCCESS) {
        ++btm_cb.ble_ctr_cb.white_list_avail_size;
    }
}

/*******************************************************************************
**
** Function         btm_ble_start_auto_conn
**
** Description      This function is to start/stop auto connection procedure.
**
** Parameters       start: TRUE to start; FALSE to stop.
**
** Returns          void
**
*******************************************************************************/
uint8_t btm_ble_start_auto_conn(uint8_t start)
{
    tBTM_BLE_CB *p_cb = &btm_cb.ble_ctr_cb;
    BD_ADDR dummy_bda = {0};
    uint8_t exec = TRUE;
    uint16_t scan_int;
    uint16_t scan_win;
    uint8_t own_addr_type = p_cb->addr_mgnt_cb.own_addr_type;
    uint8_t peer_addr_type = BLE_ADDR_PUBLIC;

    if(start) {
        if(p_cb->conn_state == BLE_CONN_IDLE && background_connections_pending()
                && btm_ble_topology_check(BTM_BLE_STATE_INIT)) {
            p_cb->wl_state  |= BTM_BLE_WL_INIT;
            btm_execute_wl_dev_operation();
#if BLE_PRIVACY_SPT == TRUE
            btm_ble_enable_resolving_list_for_platform(BTM_BLE_RL_INIT);
#endif
            scan_int = (p_cb->scan_int == BTM_BLE_SCAN_PARAM_UNDEF) ?
                       BTM_BLE_SCAN_SLOW_INT_1 : p_cb->scan_int;
            scan_win = (p_cb->scan_win == BTM_BLE_SCAN_PARAM_UNDEF) ?
                       BTM_BLE_SCAN_SLOW_WIN_1 : p_cb->scan_win;
#if BLE_PRIVACY_SPT == TRUE

            if(btm_cb.ble_ctr_cb.rl_state != BTM_BLE_RL_IDLE
                    && HCI_LE_ENHANCED_PRIVACY_SUPPORTED(btm_cb.devcb.local_le_features)) {
                own_addr_type |= BLE_ADDR_TYPE_ID_BIT;
                peer_addr_type |= BLE_ADDR_TYPE_ID_BIT;
            }

#endif

            if(!btsnd_hcic_ble_create_ll_conn(scan_int,    /* uint16_t scan_int      */
                                              scan_win,    /* uint16_t scan_win      */
                                              0x01,                   /* uint8_t white_list     */
                                              peer_addr_type,        /* uint8_t addr_type_peer */
                                              dummy_bda,              /* BD_ADDR bda_peer     */
                                              own_addr_type,          /* uint8_t addr_type_own */
                                              BTM_BLE_CONN_INT_MIN_DEF,   /* uint16_t conn_int_min  */
                                              BTM_BLE_CONN_INT_MAX_DEF,   /* uint16_t conn_int_max  */
                                              BTM_BLE_CONN_SLAVE_LATENCY_DEF,  /* uint16_t conn_latency  */
                                              BTM_BLE_CONN_TIMEOUT_DEF,        /* uint16_t conn_timeout  */
                                              0,                       /* uint16_t min_len       */
                                              0)) {                    /* uint16_t max_len       */
                /* start auto connection failed */
                exec =  FALSE;
                p_cb->wl_state &= ~BTM_BLE_WL_INIT;
            } else {
                btm_ble_set_conn_st(BLE_BG_CONN);
            }
        } else {
            exec = FALSE;
        }
    } else {
        if(p_cb->conn_state == BLE_BG_CONN) {
            btsnd_hcic_ble_create_conn_cancel();
            btm_ble_set_conn_st(BLE_CONN_CANCEL);
            p_cb->wl_state &= ~BTM_BLE_WL_INIT;
        } else {
            BTM_TRACE_DEBUG("conn_st = %d, not in auto conn state, cannot stop", p_cb->conn_state);
            exec = FALSE;
        }
    }

    return exec;
}

/*******************************************************************************
**
** Function         btm_ble_start_select_conn
**
** Description      This function is to start/stop selective connection procedure.
**
** Parameters       start: TRUE to start; FALSE to stop.
**                  p_select_cback: callback function to return application
**                                  selection.
**
** Returns          uint8_t: selective connectino procedure is started.
**
*******************************************************************************/
uint8_t btm_ble_start_select_conn(uint8_t start, tBTM_BLE_SEL_CBACK *p_select_cback)
{
    tBTM_BLE_CB *p_cb = &btm_cb.ble_ctr_cb;
    uint32_t scan_int = p_cb->scan_int == BTM_BLE_SCAN_PARAM_UNDEF ? BTM_BLE_SCAN_FAST_INT :
                        p_cb->scan_int;
    uint32_t scan_win = p_cb->scan_win == BTM_BLE_SCAN_PARAM_UNDEF ? BTM_BLE_SCAN_FAST_WIN :
                        p_cb->scan_win;
    BTM_TRACE_EVENT("%s", __func__);

    if(start) {
        if(!BTM_BLE_IS_SCAN_ACTIVE(p_cb->scan_activity)) {
            if(p_select_cback != NULL) {
                btm_cb.ble_ctr_cb.p_select_cback = p_select_cback;
            }

            btm_execute_wl_dev_operation();
            btm_update_scanner_filter_policy(SP_ADV_WL);
            btm_cb.ble_ctr_cb.inq_var.scan_type = BTM_BLE_SCAN_MODE_PASS;

            /* Process advertising packets only from devices in the white list */
            if(btm_cb.cmn_ble_vsc_cb.extended_scan_support == 0) {
                /* use passive scan by default */
                if(!btsnd_hcic_ble_set_scan_params(BTM_BLE_SCAN_MODE_PASS,
                                                   scan_int,
                                                   scan_win,
                                                   p_cb->addr_mgnt_cb.own_addr_type,
                                                   SP_ADV_WL)) {
                    return FALSE;
                }
            } else {
                if(!btm_ble_send_extended_scan_params(BTM_BLE_SCAN_MODE_PASS,
                                                      scan_int,
                                                      scan_win,
                                                      p_cb->addr_mgnt_cb.own_addr_type,
                                                      SP_ADV_WL)) {
                    return FALSE;
                }
            }

            if(!btm_ble_topology_check(BTM_BLE_STATE_PASSIVE_SCAN)) {
                BTM_TRACE_ERROR("peripheral device cannot initiate passive scan for a selective connection");
                return FALSE;
            } else if(background_connections_pending()) {
#if BLE_PRIVACY_SPT == TRUE
                btm_ble_enable_resolving_list_for_platform(BTM_BLE_RL_SCAN);
#endif

                if(!btsnd_hcic_ble_set_scan_enable(TRUE, TRUE)) {   /* duplicate filtering enabled */
                    return FALSE;
                }

                /* mark up inquiry status flag */
                p_cb->scan_activity |= BTM_LE_SELECT_CONN_ACTIVE;
                p_cb->wl_state |= BTM_BLE_WL_SCAN;
            }
        } else {
            BTM_TRACE_ERROR("scan active, can not start selective connection procedure");
            return FALSE;
        }
    } else { /* disable selective connection mode */
        p_cb->scan_activity &= ~BTM_LE_SELECT_CONN_ACTIVE;
        p_cb->p_select_cback = NULL;
        p_cb->wl_state &= ~BTM_BLE_WL_SCAN;

        /* stop scanning */
        if(!BTM_BLE_IS_SCAN_ACTIVE(p_cb->scan_activity)) {
            btm_ble_stop_scan();    /* duplicate filtering enabled */
        }
    }

    return TRUE;
}
/*******************************************************************************
**
** Function         btm_ble_initiate_select_conn
**
** Description      This function is to start/stop selective connection procedure.
**
** Parameters       start: TRUE to start; FALSE to stop.
**                  p_select_cback: callback function to return application
**                                  selection.
**
** Returns          uint8_t: selective connectino procedure is started.
**
*******************************************************************************/
void btm_ble_initiate_select_conn(BD_ADDR bda)
{
    BTM_TRACE_EVENT("btm_ble_initiate_select_conn");

    /* use direct connection procedure to initiate connection */
    if(!L2CA_ConnectFixedChnl(L2CAP_ATT_CID, bda)) {
        BTM_TRACE_ERROR("btm_ble_initiate_select_conn failed");
    }
}
/*******************************************************************************
**
** Function         btm_ble_suspend_bg_conn
**
** Description      This function is to suspend an active background connection
**                  procedure.
**
** Parameters       none.
**
** Returns          none.
**
*******************************************************************************/
uint8_t btm_ble_suspend_bg_conn(void)
{
    BTM_TRACE_EVENT("%s", __func__);

    if(btm_cb.ble_ctr_cb.bg_conn_type == BTM_BLE_CONN_AUTO) {
        return btm_ble_start_auto_conn(FALSE);
    } else if(btm_cb.ble_ctr_cb.bg_conn_type == BTM_BLE_CONN_SELECTIVE) {
        return btm_ble_start_select_conn(FALSE, NULL);
    }

    return FALSE;
}
/*******************************************************************************
**
** Function         btm_suspend_wl_activity
**
** Description      This function is to suspend white list related activity
**
** Returns          none.
**
*******************************************************************************/
static void btm_suspend_wl_activity(tBTM_BLE_WL_STATE wl_state)
{
    if(wl_state & BTM_BLE_WL_INIT) {
        btm_ble_start_auto_conn(FALSE);
    }

    if(wl_state & BTM_BLE_WL_SCAN) {
        btm_ble_start_select_conn(FALSE, NULL);
    }

    if(wl_state & BTM_BLE_WL_ADV) {
        btm_ble_stop_adv();
    }
}
/*******************************************************************************
**
** Function         btm_resume_wl_activity
**
** Description      This function is to resume white list related activity
**
** Returns          none.
**
*******************************************************************************/
static void btm_resume_wl_activity(tBTM_BLE_WL_STATE wl_state)
{
    btm_ble_resume_bg_conn();

    if(wl_state & BTM_BLE_WL_ADV) {
        btm_ble_start_adv();
    }
}
/*******************************************************************************
**
** Function         btm_ble_resume_bg_conn
**
** Description      This function is to resume a background auto connection
**                  procedure.
**
** Parameters       none.
**
** Returns          none.
**
*******************************************************************************/
uint8_t btm_ble_resume_bg_conn(void)
{
    tBTM_BLE_CB *p_cb = &btm_cb.ble_ctr_cb;
    uint8_t ret = FALSE;

    if(p_cb->bg_conn_type != BTM_BLE_CONN_NONE) {
        if(p_cb->bg_conn_type == BTM_BLE_CONN_AUTO) {
            ret = btm_ble_start_auto_conn(TRUE);
        }

        if(p_cb->bg_conn_type == BTM_BLE_CONN_SELECTIVE) {
            ret = btm_ble_start_select_conn(TRUE, btm_cb.ble_ctr_cb.p_select_cback);
        }
    }

    return ret;
}
/*******************************************************************************
**
** Function         btm_ble_get_conn_st
**
** Description      This function get BLE connection state
**
** Returns          connection state
**
*******************************************************************************/
tBTM_BLE_CONN_ST btm_ble_get_conn_st(void)
{
    return btm_cb.ble_ctr_cb.conn_state;
}
/*******************************************************************************
**
** Function         btm_ble_set_conn_st
**
** Description      This function set BLE connection state
**
** Returns          None.
**
*******************************************************************************/
void btm_ble_set_conn_st(tBTM_BLE_CONN_ST new_st)
{
    btm_cb.ble_ctr_cb.conn_state = new_st;

    if(new_st == BLE_BG_CONN || new_st == BLE_DIR_CONN) {
        btm_ble_set_topology_mask(BTM_BLE_STATE_INIT_BIT);
    } else {
        btm_ble_clear_topology_mask(BTM_BLE_STATE_INIT_BIT);
    }
}

/*******************************************************************************
**
** Function         btm_ble_enqueue_direct_conn_req
**
** Description      This function enqueue the direct connection request
**
** Returns          None.
**
*******************************************************************************/
void btm_ble_enqueue_direct_conn_req(void *p_param)
{
    tBTM_BLE_CONN_REQ *p = (tBTM_BLE_CONN_REQ *)GKI_getbuf(sizeof(tBTM_BLE_CONN_REQ));
    p->p_param = p_param;
    fixed_queue_enqueue(btm_cb.ble_ctr_cb.conn_pending_q, p);
}
/*******************************************************************************
**
** Function         btm_send_pending_direct_conn
**
** Description      This function send the pending direct connection request in queue
**
** Returns          TRUE if started, FALSE otherwise
**
*******************************************************************************/
uint8_t btm_send_pending_direct_conn(void)
{
    tBTM_BLE_CONN_REQ *p_req;
    uint8_t     rt = FALSE;
    p_req = (tBTM_BLE_CONN_REQ *)fixed_queue_try_dequeue(btm_cb.ble_ctr_cb.conn_pending_q);

    if(p_req != NULL) {
        tL2C_LCB *p_lcb = (tL2C_LCB *)(p_req->p_param);

        /* Ignore entries that might have been released while queued. */
        if(p_lcb->in_use) {
            rt = l2cble_init_direct_conn(p_lcb);
        }

        GKI_freebuf(p_req);
    }

    return rt;
}

#endif


