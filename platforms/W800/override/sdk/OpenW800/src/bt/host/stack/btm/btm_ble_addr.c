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
 *  This file contains functions for BLE address management.
 *
 ******************************************************************************/

#include <string.h>

#include "bt_types.h"
#include "hcimsgs.h"
#include "btu.h"
#include "btm_int.h"
#include "gap_api.h"
//#include "device/include/controller.h"

#if (defined BLE_INCLUDED && BLE_INCLUDED == TRUE)
#include "btm_ble_int.h"
#include "smp_api.h"

#ifdef USE_ALARM
extern fixed_queue_t *btu_general_alarm_queue;
#endif

/*******************************************************************************
**
** Function         btm_gen_resolve_paddr_cmpl
**
** Description      This is callback functioin when resolvable private address
**                  generation is complete.
**
** Returns          void
**
*******************************************************************************/
static void btm_gen_resolve_paddr_cmpl(tSMP_ENC *p)
{
    tBTM_LE_RANDOM_CB *p_cb = &btm_cb.ble_ctr_cb.addr_mgnt_cb;
    BTM_TRACE_EVENT("btm_gen_resolve_paddr_cmpl");

    if(p) {
        /* set hash to be LSB of rpAddress */
        p_cb->private_addr[5] = p->param_buf[0];
        p_cb->private_addr[4] = p->param_buf[1];
        p_cb->private_addr[3] = p->param_buf[2];
        /* set it to controller */
        BTM_TRACE_DEBUG("update random address:%02x:%02x:%02x:%02x:%02x:%02x\r\n", p_cb->private_addr[0],
                        p_cb->private_addr[1], p_cb->private_addr[2]
                        , p_cb->private_addr[3], p_cb->private_addr[4], p_cb->private_addr[5]);
        btsnd_hcic_ble_set_random_addr(p_cb->private_addr);
        p_cb->own_addr_type = BLE_ADDR_RANDOM;
        /* start a periodical timer to refresh random addr */
        uint64_t interval_ms = BTM_BLE_PRIVATE_ADDR_INT_MS;
#if (BTM_BLE_CONFORMANCE_TESTING == TRUE)
        interval_ms = btm_cb.ble_ctr_cb.rpa_tout * 1000;
#endif
#ifdef USE_ALARM
        alarm_set_on_queue(p_cb->refresh_raddr_timer, interval_ms,
                           btm_ble_refresh_raddr_timer_timeout, NULL,
                           btu_general_alarm_queue);
#else
        p_cb->refresh_raddr_timer.p_cback = (TIMER_CBACK *)&btm_ble_refresh_raddr_timer_timeout;
        p_cb->refresh_raddr_timer.param = (TIMER_PARAM_TYPE)NULL;
        btu_start_timer_oneshot(&p_cb->refresh_raddr_timer, BTU_TTYPE_BLE_RANDOM_ADDR,
                                interval_ms / 1000);
#endif
    } else {
        /* random address set failure */
        BTM_TRACE_DEBUG("set random address failed");
    }
}
/*******************************************************************************
**
** Function         btm_gen_resolve_paddr_low
**
** Description      This function is called when random address has generate the
**                  random number base for low 3 byte bd address.
**
** Returns          void
**
*******************************************************************************/
void btm_gen_resolve_paddr_low(tBTM_RAND_ENC *p)
{
#if (BLE_INCLUDED == TRUE && SMP_INCLUDED == TRUE)
    tBTM_LE_RANDOM_CB *p_cb = &btm_cb.ble_ctr_cb.addr_mgnt_cb;
    tSMP_ENC    output;
    //uint8_t wifi_mac[6];
    BTM_TRACE_DEBUG("btm_gen_resolve_paddr_low...\r\n");
    //extern int tls_get_mac_addr(uint8_t *mac);
    //tls_get_mac_addr(wifi_mac);

    if(p) {
#if 0
        /////////////////////////////////////
        p->param_buf[0] = wifi_mac[3];
        p->param_buf[1] = wifi_mac[4];
        p->param_buf[2] = wifi_mac[5];
        /////////////////////////////////////
#endif
        p->param_buf[2] &= (~BLE_RESOLVE_ADDR_MASK);
        p->param_buf[2] |= BLE_RESOLVE_ADDR_MSB;
        p_cb->private_addr[2] = p->param_buf[0];
        p_cb->private_addr[1] = p->param_buf[1];
        p_cb->private_addr[0] = p->param_buf[2];

        /* encrypt with ur IRK */
        if(!SMP_Encrypt(btm_cb.devcb.id_keys.irk, BT_OCTET16_LEN, p->param_buf, 3, &output)) {
            btm_gen_resolve_paddr_cmpl(NULL);
        } else {
            btm_gen_resolve_paddr_cmpl(&output);
        }
    }

#endif
}
/*******************************************************************************
**
** Function         btm_gen_resolvable_private_addr
**
** Description      This function generate a resolvable private address.
**
** Returns          void
**
*******************************************************************************/
void btm_gen_resolvable_private_addr(void *p_cmd_cplt_cback)
{
    BTM_TRACE_EVENT("btm_gen_resolvable_private_addr");

    /* generate 3B rand as BD LSB, SRK with it, get BD MSB */
    if(!btsnd_hcic_ble_rand((void *)p_cmd_cplt_cback)) {
        btm_gen_resolve_paddr_cmpl(NULL);
    }
}
/*******************************************************************************
**
** Function         btm_gen_non_resolve_paddr_cmpl
**
** Description      This is the callback function when non-resolvable private
**                  function is generated and write to controller.
**
** Returns          void
**
*******************************************************************************/
static void btm_gen_non_resolve_paddr_cmpl(tBTM_RAND_ENC *p)
{
    tBTM_LE_RANDOM_CB *p_cb = &btm_cb.ble_ctr_cb.addr_mgnt_cb;
    tBTM_BLE_ADDR_CBACK *p_cback = p_cb->p_generate_cback;
    void    *p_data = p_cb->p;
    uint8_t   *pp;
    BD_ADDR     static_random;
    BTM_TRACE_EVENT("btm_gen_non_resolve_paddr_cmpl");
    p_cb->p_generate_cback = NULL;

    if(p) {
        pp = p->param_buf;
        STREAM_TO_BDADDR(static_random, pp);
        /* mask off the 2 MSB */
        static_random[0] &= BLE_STATIC_PRIVATE_MSB_MASK;

        /* report complete */
        if(p_cback) {
            (* p_cback)(static_random, p_data);
        }
    } else {
        BTM_TRACE_DEBUG("btm_gen_non_resolvable_private_addr failed");

        if(p_cback) {
            (* p_cback)(NULL, p_data);
        }
    }
}
/*******************************************************************************
**
** Function         btm_gen_non_resolvable_private_addr
**
** Description      This function generate a non-resolvable private address.
**
**
** Returns          void
**
*******************************************************************************/
void btm_gen_non_resolvable_private_addr(tBTM_BLE_ADDR_CBACK *p_cback, void *p)
{
    tBTM_LE_RANDOM_CB   *p_mgnt_cb = &btm_cb.ble_ctr_cb.addr_mgnt_cb;
    BTM_TRACE_EVENT("btm_gen_non_resolvable_private_addr");

    if(p_mgnt_cb->p_generate_cback != NULL) {
        return;
    }

    p_mgnt_cb->p_generate_cback = p_cback;
    p_mgnt_cb->p                = p;

    if(!btsnd_hcic_ble_rand((void *)btm_gen_non_resolve_paddr_cmpl)) {
        btm_gen_non_resolve_paddr_cmpl(NULL);
    }
}

#if SMP_INCLUDED == TRUE
/*******************************************************************************
**  Utility functions for Random address resolving
*******************************************************************************/
/*******************************************************************************
**
** Function         btm_ble_proc_resolve_x
**
** Description      This function compares the X with random address 3 MSO bytes
**                  to find a match.
**
** Returns          TRUE on match, FALSE otherwise
**
*******************************************************************************/
static uint8_t btm_ble_proc_resolve_x(tSMP_ENC *p)
{
    tBTM_LE_RANDOM_CB   *p_mgnt_cb = &btm_cb.ble_ctr_cb.addr_mgnt_cb;
    uint8_t    comp[3];
    BTM_TRACE_EVENT("btm_ble_proc_resolve_x");
    /* compare the hash with 3 LSB of bd address */
    comp[0] = p_mgnt_cb->random_bda[5];
    comp[1] = p_mgnt_cb->random_bda[4];
    comp[2] = p_mgnt_cb->random_bda[3];

    if(p) {
        if(!memcmp(p->param_buf, &comp[0], 3)) {
            /* match is found */
            BTM_TRACE_EVENT("match is found");
            return TRUE;
        }
    }

    return FALSE;
}

/*******************************************************************************
**
** Function         btm_ble_init_pseudo_addr
**
** Description      This function is used to initialize pseudo address.
**                  If pseudo address is not available, use dummy address
**
** Returns          TRUE is updated; FALSE otherwise.
**
*******************************************************************************/
uint8_t btm_ble_init_pseudo_addr(tBTM_SEC_DEV_REC *p_dev_rec, BD_ADDR new_pseudo_addr)
{
    BD_ADDR dummy_bda = {0};

    if(memcmp(p_dev_rec->ble.pseudo_addr, dummy_bda, BD_ADDR_LEN) == 0) {
        wm_memcpy(p_dev_rec->ble.pseudo_addr, new_pseudo_addr, BD_ADDR_LEN);
        return TRUE;
    }

    return FALSE;
}

/*******************************************************************************
**
** Function         btm_ble_addr_resolvable
**
** Description      This function checks if a RPA is resolvable by the device key.
**
** Returns          TRUE is resolvable; FALSE otherwise.
**
*******************************************************************************/
uint8_t btm_ble_addr_resolvable(BD_ADDR rpa, tBTM_SEC_DEV_REC *p_dev_rec)
{
    uint8_t rt = FALSE;

    if(!BTM_BLE_IS_RESOLVE_BDA(rpa)) {
        return rt;
    }

    uint8_t rand[3];
    tSMP_ENC output;

    if((p_dev_rec->device_type & BT_DEVICE_TYPE_BLE) &&
            (p_dev_rec->ble.key_type & BTM_LE_KEY_PID)) {
        BTM_TRACE_DEBUG("%s try to resolve", __func__);
        /* use the 3 MSB of bd address as prand */
        rand[0] = rpa[2];
        rand[1] = rpa[1];
        rand[2] = rpa[0];
        /* generate X = E irk(R0, R1, R2) and R is random address 3 LSO */
        SMP_Encrypt(p_dev_rec->ble.keys.irk, BT_OCTET16_LEN,
                    &rand[0], 3, &output);
        rand[0] = rpa[5];
        rand[1] = rpa[4];
        rand[2] = rpa[3];

        if(!memcmp(output.param_buf, &rand[0], 3)) {
            btm_ble_init_pseudo_addr(p_dev_rec, rpa);
            rt = TRUE;
        }
    }

    return rt;
}

/*******************************************************************************
**
** Function         btm_ble_match_random_bda
**
** Description      This function match the random address to the appointed device
**                  record, starting from calculating IRK. If record index exceed
**                  the maximum record number, matching failed and send callback.
**
** Returns          None.
**
*******************************************************************************/
static uint8_t btm_ble_match_random_bda(void *data, void *context)
{
#if (BLE_INCLUDED == TRUE && SMP_INCLUDED == TRUE)
    /* use the 3 MSB of bd address as prand */
    tBTM_LE_RANDOM_CB *p_mgnt_cb = &btm_cb.ble_ctr_cb.addr_mgnt_cb;
    uint8_t rand[3];
    rand[0] = p_mgnt_cb->random_bda[2];
    rand[1] = p_mgnt_cb->random_bda[1];
    rand[2] = p_mgnt_cb->random_bda[0];
    BTM_TRACE_EVENT("%s next iteration", __func__);
    tSMP_ENC output;
    tBTM_SEC_DEV_REC *p_dev_rec = data;
    BTM_TRACE_DEBUG("sec_flags = %02x device_type = %d", p_dev_rec->sec_flags,
                    p_dev_rec->device_type);

    if(!(p_dev_rec->device_type & BT_DEVICE_TYPE_BLE) ||
            !(p_dev_rec->ble.key_type & BTM_LE_KEY_PID)) {
        return TRUE;
    }

    /* generate X = E irk(R0, R1, R2) and R is random address 3 LSO */
    SMP_Encrypt(p_dev_rec->ble.keys.irk, BT_OCTET16_LEN,
                &rand[0], 3, &output);
    // if it was match, finish iteration, otherwise continue
    return !btm_ble_proc_resolve_x(&output);
#endif
}

/*******************************************************************************
**
** Function         btm_ble_resolve_random_addr
**
** Description      This function is called to resolve a random address.
**
** Returns          pointer to the security record of the device whom a random
**                  address is matched to.
**
*******************************************************************************/
void btm_ble_resolve_random_addr(BD_ADDR random_bda, tBTM_BLE_RESOLVE_CBACK *p_cback, void *p)
{
    tBTM_LE_RANDOM_CB   *p_mgnt_cb = &btm_cb.ble_ctr_cb.addr_mgnt_cb;
    BTM_TRACE_EVENT("%s", __func__);

    if(!p_mgnt_cb->busy) {
        p_mgnt_cb->p = p;
        p_mgnt_cb->busy = TRUE;
        wm_memcpy(p_mgnt_cb->random_bda, random_bda, BD_ADDR_LEN);
        /* start to resolve random address */
        /* check for next security record */
        list_node_t *n = list_foreach(btm_cb.sec_dev_rec, btm_ble_match_random_bda, NULL);
        tBTM_SEC_DEV_REC *p_dev_rec = n ? list_node(n) : NULL;
        BTM_TRACE_EVENT("%s:  %sresolved", __func__, (p_dev_rec == NULL ? "not " : ""));
        p_mgnt_cb->busy = FALSE;
        (*p_cback)(p_dev_rec, p);
    } else {
        (*p_cback)(NULL, p);
    }
}
#endif

/*******************************************************************************
**  address mapping between pseudo address and real connection address
*******************************************************************************/
/*******************************************************************************
**
** Function         btm_find_dev_by_identity_addr
**
** Description      find the security record whose LE static address is matching
**
*******************************************************************************/
tBTM_SEC_DEV_REC *btm_find_dev_by_identity_addr(BD_ADDR bd_addr, uint8_t addr_type)
{
#if BLE_PRIVACY_SPT == TRUE
    list_node_t *end = list_end(btm_cb.sec_dev_rec);

    for(list_node_t *node = list_begin(btm_cb.sec_dev_rec); node != end; node = list_next(node)) {
        tBTM_SEC_DEV_REC *p_dev_rec = list_node(node);

        if(memcmp(p_dev_rec->ble.static_addr, bd_addr, BD_ADDR_LEN) == 0) {
            if((p_dev_rec->ble.static_addr_type & (~BLE_ADDR_TYPE_ID_BIT)) !=
                    (addr_type & (~BLE_ADDR_TYPE_ID_BIT)))
                BTM_TRACE_WARNING("%s find pseudo->random match with diff addr type: %d vs %d",
                                  __func__, p_dev_rec->ble.static_addr_type, addr_type);

            /* found the match */
            return p_dev_rec;
        }
    }

#endif
    return NULL;
}

/*******************************************************************************
**
** Function         btm_identity_addr_to_random_pseudo
**
** Description      This function map a static BD address to a pseudo random address
**                  in security database.
**
*******************************************************************************/
uint8_t btm_identity_addr_to_random_pseudo(BD_ADDR bd_addr, uint8_t *p_addr_type, uint8_t refresh)
{
#if BLE_PRIVACY_SPT == TRUE
    tBTM_SEC_DEV_REC    *p_dev_rec = btm_find_dev_by_identity_addr(bd_addr, *p_addr_type);
    BTM_TRACE_EVENT("%s", __func__);

    /* evt reported on static address, map static address to random pseudo */
    if(p_dev_rec != NULL) {
        /* if RPA offloading is supported, or 4.2 controller, do RPA refresh */
        if(refresh && btm_cb.devcb.ble_resolving_list_max_size != 0) {
            btm_ble_read_resolving_list_entry(p_dev_rec);
        }

        /* assign the original address to be the current report address */
        if(!btm_ble_init_pseudo_addr(p_dev_rec, bd_addr)) {
            wm_memcpy(bd_addr, p_dev_rec->ble.pseudo_addr, BD_ADDR_LEN);
        }

        *p_addr_type = p_dev_rec->ble.ble_addr_type;
        return TRUE;
    }

#endif
    return FALSE;
}

/*******************************************************************************
**
** Function         btm_random_pseudo_to_identity_addr
**
** Description      This function map a random pseudo address to a public address
**                  random_pseudo is input and output parameter
**
*******************************************************************************/
uint8_t btm_random_pseudo_to_identity_addr(BD_ADDR random_pseudo, uint8_t *p_static_addr_type)
{
#if BLE_PRIVACY_SPT == TRUE
    tBTM_SEC_DEV_REC    *p_dev_rec = btm_find_dev(random_pseudo);

    if(p_dev_rec != NULL) {
        if(p_dev_rec->ble.in_controller_list & BTM_RESOLVING_LIST_BIT) {
            * p_static_addr_type = p_dev_rec->ble.static_addr_type;
            wm_memcpy(random_pseudo, p_dev_rec->ble.static_addr, BD_ADDR_LEN);

            //if (controller_get_interface()->supports_ble_privacy())
            if(HCI_LE_ENHANCED_PRIVACY_SUPPORTED(btm_cb.devcb.local_le_features)) {
                *p_static_addr_type |= BLE_ADDR_TYPE_ID_BIT;
            }

            return TRUE;
        }
    }

#endif
    return FALSE;
}

/*******************************************************************************
**
** Function         btm_ble_refresh_peer_resolvable_private_addr
**
** Description      This function refresh the currently used resolvable remote private address into security
**                  database and set active connection address.
**
*******************************************************************************/
void btm_ble_refresh_peer_resolvable_private_addr(BD_ADDR pseudo_bda, BD_ADDR rpa,
        uint8_t rra_type)
{
#if BLE_PRIVACY_SPT == TRUE
    uint8_t rra_dummy = FALSE;
    BD_ADDR dummy_bda = {0};

    if(memcmp(dummy_bda, rpa, BD_ADDR_LEN) == 0) {
        rra_dummy = TRUE;
    }

    /* update security record here, in adv event or connection complete process */
    tBTM_SEC_DEV_REC *p_sec_rec = btm_find_dev(pseudo_bda);

    if(p_sec_rec != NULL) {
        wm_memcpy(p_sec_rec->ble.cur_rand_addr, rpa, BD_ADDR_LEN);

        /* unknown, if dummy address, set to static */
        if(rra_type == BTM_BLE_ADDR_PSEUDO) {
            p_sec_rec->ble.active_addr_type = rra_dummy ? BTM_BLE_ADDR_STATIC : BTM_BLE_ADDR_RRA;
        } else {
            p_sec_rec->ble.active_addr_type = rra_type;
        }
    } else {
        BTM_TRACE_ERROR("No matching known device in record");
        return;
    }

    BTM_TRACE_DEBUG("%s: active_addr_type: %d ",
                    __func__, p_sec_rec->ble.active_addr_type);
    /* connection refresh remote address */
    tACL_CONN *p_acl = btm_bda_to_acl(p_sec_rec->bd_addr, BT_TRANSPORT_LE);

    if(p_acl == NULL) {
        p_acl = btm_bda_to_acl(p_sec_rec->ble.pseudo_addr, BT_TRANSPORT_LE);
    }

    if(p_acl != NULL) {
        if(rra_type == BTM_BLE_ADDR_PSEUDO) {
            /* use static address, resolvable_private_addr is empty */
            if(rra_dummy) {
                p_acl->active_remote_addr_type = p_sec_rec->ble.static_addr_type;
                wm_memcpy(p_acl->active_remote_addr, p_sec_rec->ble.static_addr, BD_ADDR_LEN);
            } else {
                p_acl->active_remote_addr_type = BLE_ADDR_RANDOM;
                wm_memcpy(p_acl->active_remote_addr, rpa, BD_ADDR_LEN);
            }
        } else {
            p_acl->active_remote_addr_type = rra_type;
            wm_memcpy(p_acl->active_remote_addr, rpa, BD_ADDR_LEN);
        }

        BTM_TRACE_DEBUG("p_acl->active_remote_addr_type: %d ", p_acl->active_remote_addr_type);
        BTM_TRACE_DEBUG("%s conn_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                        __func__, p_acl->active_remote_addr[0], p_acl->active_remote_addr[1],
                        p_acl->active_remote_addr[2], p_acl->active_remote_addr[3],
                        p_acl->active_remote_addr[4], p_acl->active_remote_addr[5]);
    }

#endif
}

/*******************************************************************************
**
** Function         btm_ble_refresh_local_resolvable_private_addr
**
** Description      This function refresh the currently used resolvable private address for the
**                  active link to the remote device
**
*******************************************************************************/
void btm_ble_refresh_local_resolvable_private_addr(BD_ADDR pseudo_addr,
        BD_ADDR local_rpa)
{
#if BLE_PRIVACY_SPT == TRUE
    tACL_CONN *p = btm_bda_to_acl(pseudo_addr, BT_TRANSPORT_LE);
    BD_ADDR     dummy_bda = {0};

    if(p != NULL) {
        if(btm_cb.ble_ctr_cb.privacy_mode != BTM_PRIVACY_NONE) {
            p->conn_addr_type = BLE_ADDR_RANDOM;

            if(memcmp(local_rpa, dummy_bda, BD_ADDR_LEN)) {
                wm_memcpy(p->conn_addr, local_rpa, BD_ADDR_LEN);
            } else {
                wm_memcpy(p->conn_addr, btm_cb.ble_ctr_cb.addr_mgnt_cb.private_addr, BD_ADDR_LEN);
            }
        } else {
            p->conn_addr_type = BLE_ADDR_PUBLIC;
            BTM_GetLocalDeviceAddr(p->conn_addr);
            //wm_memcpy(p->conn_addr,&controller_get_interface()->get_address()->address, BD_ADDR_LEN);
        }
    }

#endif
}
#endif


