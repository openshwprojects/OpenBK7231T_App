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
 *  This file contains the L2CAP channel state machine
 *
 ******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bt_target.h"
#include "gki.h"
#include "bt_common.h"
#include "hcidefs.h"
#include "hcimsgs.h"
#include "l2cdefs.h"
#include "l2c_int.h"
#include "btm_int.h"
#include "btu.h"
#include "hcimsgs.h"

#ifdef USE_ALARM
extern fixed_queue_t *btu_general_alarm_queue;
#endif

/********************************************************************************/
/*              L O C A L    F U N C T I O N     P R O T O T Y P E S            */
/********************************************************************************/
static void l2c_csm_closed(tL2C_CCB *p_ccb, uint16_t event, void *p_data);
static void l2c_csm_orig_w4_sec_comp(tL2C_CCB *p_ccb, uint16_t event, void *p_data);
static void l2c_csm_term_w4_sec_comp(tL2C_CCB *p_ccb, uint16_t event, void *p_data);
static void l2c_csm_w4_l2cap_connect_rsp(tL2C_CCB *p_ccb, uint16_t event, void *p_data);
static void l2c_csm_w4_l2ca_connect_rsp(tL2C_CCB *p_ccb, uint16_t event, void *p_data);
static void l2c_csm_config(tL2C_CCB *p_ccb, uint16_t event, void *p_data);
static void l2c_csm_open(tL2C_CCB *p_ccb, uint16_t event, void *p_data);
static void l2c_csm_w4_l2cap_disconnect_rsp(tL2C_CCB *p_ccb, uint16_t event, void *p_data);
static void l2c_csm_w4_l2ca_disconnect_rsp(tL2C_CCB *p_ccb, uint16_t event, void *p_data);
#define WM_BT_TRACE_VERBOSE  FALSE

#if (WM_BT_TRACE_VERBOSE == TRUE)
static char *l2c_csm_get_event_name(uint16_t event);
#endif

/*******************************************************************************
**
** Function         l2c_csm_execute
**
** Description      This function executes the state machine.
**
** Returns          void
**
*******************************************************************************/
void l2c_csm_execute(tL2C_CCB *p_ccb, uint16_t event, void *p_data)
{
    L2CAP_TRACE_EVENT("l2c_csm_execute state:%d, event=%s\r\n", p_ccb->chnl_state,
                      l2c_csm_get_event_name(event));

    switch(p_ccb->chnl_state) {
        case CST_CLOSED:
            l2c_csm_closed(p_ccb, event, p_data);
            break;

        case CST_ORIG_W4_SEC_COMP:
            l2c_csm_orig_w4_sec_comp(p_ccb, event, p_data);
            break;

        case CST_TERM_W4_SEC_COMP:
            l2c_csm_term_w4_sec_comp(p_ccb, event, p_data);
            break;

        case CST_W4_L2CAP_CONNECT_RSP:
            l2c_csm_w4_l2cap_connect_rsp(p_ccb, event, p_data);
            break;

        case CST_W4_L2CA_CONNECT_RSP:
            l2c_csm_w4_l2ca_connect_rsp(p_ccb, event, p_data);
            break;

        case CST_CONFIG:
            l2c_csm_config(p_ccb, event, p_data);
            break;

        case CST_OPEN:
            l2c_csm_open(p_ccb, event, p_data);
            break;

        case CST_W4_L2CAP_DISCONNECT_RSP:
            l2c_csm_w4_l2cap_disconnect_rsp(p_ccb, event, p_data);
            break;

        case CST_W4_L2CA_DISCONNECT_RSP:
            l2c_csm_w4_l2ca_disconnect_rsp(p_ccb, event, p_data);
            break;

        default:
            L2CAP_TRACE_DEBUG("Unhandled event! event = %d", event);
            break;
    }
}

/*******************************************************************************
**
** Function         l2c_csm_closed
**
** Description      This function handles events when the channel is in
**                  CLOSED state. This state exists only when the link is
**                  being initially established.
**
** Returns          void
**
*******************************************************************************/
static void l2c_csm_closed(tL2C_CCB *p_ccb, uint16_t event, void *p_data)
{
    tL2C_CONN_INFO          *p_ci = (tL2C_CONN_INFO *)p_data;
    uint16_t                  local_cid = p_ccb->local_cid;
    tL2CA_DISCONNECT_IND_CB *disconnect_ind;
    tL2CA_CONNECT_CFM_CB    *connect_cfm;

    if(p_ccb->p_rcb == NULL) {
#if (BT_TRACE_VERBOSE == TRUE)
        L2CAP_TRACE_ERROR("L2CAP - LCID: 0x%04x  st: CLOSED  evt: %s p_rcb == NULL", p_ccb->local_cid,
                          l2c_csm_get_event_name(event));
#else
        L2CAP_TRACE_ERROR("L2CAP - LCID: 0x%04x  st: CLOSED  evt: 0x%04x p_rcb == NULL", p_ccb->local_cid,
                          event);
#endif
        return;
    }

#if (L2CAP_UCD_INCLUDED == TRUE)

    if(local_cid == L2CAP_CONNECTIONLESS_CID) {
        /* check if this event can be processed by UCD */
        if(l2c_ucd_process_event(p_ccb, event, p_data)) {
            /* The event is processed by UCD state machine */
            return;
        }
    }

#endif
    disconnect_ind = p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb;
    connect_cfm    = p_ccb->p_rcb->api.pL2CA_ConnectCfm_Cb;
#if (BT_TRACE_VERBOSE == TRUE)
    L2CAP_TRACE_EVENT("L2CAP - LCID: 0x%04x  st: CLOSED  evt: %s", p_ccb->local_cid,
                      l2c_csm_get_event_name(event));
#else
    L2CAP_TRACE_EVENT("L2CAP - st: CLOSED evt: %d", event);
#endif

    switch(event) {
        case L2CEVT_LP_DISCONNECT_IND:                  /* Link was disconnected */
            L2CAP_TRACE_API("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed",
                            p_ccb->local_cid);
            l2cu_release_ccb(p_ccb);
            (*disconnect_ind)(local_cid, FALSE);
            break;

        case L2CEVT_LP_CONNECT_CFM:                         /* Link came up         */
#if (BLE_INCLUDED == TRUE)
            if(p_ccb->p_lcb->transport == BT_TRANSPORT_LE) {
                p_ccb->chnl_state = CST_ORIG_W4_SEC_COMP;
                l2ble_sec_access_req(p_ccb->p_lcb->remote_bd_addr, p_ccb->p_rcb->psm, TRUE,
                                     &l2c_link_sec_comp, p_ccb);
            } else
#endif
            {
                p_ccb->chnl_state = CST_ORIG_W4_SEC_COMP;
                btm_sec_l2cap_access_req(p_ccb->p_lcb->remote_bd_addr, p_ccb->p_rcb->psm,
                                         p_ccb->p_lcb->handle, TRUE, &l2c_link_sec_comp, p_ccb);
            }

            break;

        case L2CEVT_LP_CONNECT_CFM_NEG:                     /* Link failed          */

            /* Disconnect unless ACL collision and upper layer wants to handle it */
            if(p_ci->status != HCI_ERR_CONNECTION_EXISTS
                    || !btm_acl_notif_conn_collision(p_ccb->p_lcb->remote_bd_addr)) {
                L2CAP_TRACE_API("L2CAP - Calling ConnectCfm_Cb(), CID: 0x%04x  Status: %d", p_ccb->local_cid,
                                p_ci->status);
                l2cu_release_ccb(p_ccb);
                (*connect_cfm)(local_cid, p_ci->status);
            }

            break;

        case L2CEVT_L2CA_CONNECT_REQ:                       /* API connect request  */
#if (BLE_INCLUDED == TRUE)
            if(p_ccb->p_lcb->transport == BT_TRANSPORT_LE) {
                p_ccb->chnl_state = CST_ORIG_W4_SEC_COMP;
                l2ble_sec_access_req(p_ccb->p_lcb->remote_bd_addr, p_ccb->p_rcb->psm, TRUE,
                                     &l2c_link_sec_comp, p_ccb);
            } else
#endif
            {
                /* Cancel sniff mode if needed */
                {
                    tBTM_PM_PWR_MD settings;
                    wm_memset((void *)&settings, 0, sizeof(settings));
                    settings.mode = BTM_PM_MD_ACTIVE;
                    /* COVERITY
                    Event uninit_use_in_call: Using uninitialized value "settings" (field "settings".timeout uninitialized) in call to function "BTM_SetPowerMode" [details]
                    Event uninit_use_in_call: Using uninitialized value "settings.max" in call to function "BTM_SetPowerMode" [details]
                    Event uninit_use_in_call: Using uninitialized value "settings.min" in call to function "BTM_SetPowerMode"
                    // FALSE-POSITIVE error from Coverity test-tool. Please do NOT remove following comment.
                    // coverity[uninit_use_in_call] False-positive: setting the mode to BTM_PM_MD_ACTIVE only uses settings.mode the other data members of tBTM_PM_PWR_MD are ignored
                    */
                    BTM_SetPowerMode(BTM_PM_SET_ONLY_ID, p_ccb->p_lcb->remote_bd_addr, &settings);
                }

                /* If sec access does not result in started SEC_COM or COMP_NEG are already processed */
                if(btm_sec_l2cap_access_req(p_ccb->p_lcb->remote_bd_addr, p_ccb->p_rcb->psm,
                                            p_ccb->p_lcb->handle, TRUE, &l2c_link_sec_comp, p_ccb) == BTM_CMD_STARTED) {
                    p_ccb->chnl_state = CST_ORIG_W4_SEC_COMP;
                }
            }

            break;

        case L2CEVT_SEC_COMP:
            p_ccb->chnl_state = CST_W4_L2CAP_CONNECT_RSP;

            /* Wait for the info resp in this state before sending connect req (if needed) */
            if(!p_ccb->p_lcb->w4_info_rsp) {
                /* Need to have at least one compatible channel to continue */
                if(!l2c_fcr_chk_chan_modes(p_ccb)) {
                    l2cu_release_ccb(p_ccb);
                    (*p_ccb->p_rcb->api.pL2CA_ConnectCfm_Cb)(local_cid, L2CAP_CONN_NO_LINK);
                } else {
                    l2cu_send_peer_connect_req(p_ccb);
#ifdef USE_ALARM
                    alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                                       L2CAP_CHNL_CONNECT_TIMEOUT_MS,
                                       l2c_ccb_timer_timeout, p_ccb,
                                       btu_general_alarm_queue);
#else
                    p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
                    p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
                    btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CONNECT_TIMEOUT_MS / 1000);
#endif
                }
            }

            break;

        case L2CEVT_SEC_COMP_NEG:                           /* something is really bad with security */
            L2CAP_TRACE_API("L2CAP - Calling ConnectCfm_Cb(), CID: 0x%04x  Status: %d", p_ccb->local_cid,
                            L2CAP_CONN_TIMEOUT);
            l2cu_release_ccb(p_ccb);
            (*connect_cfm)(local_cid, L2CAP_CONN_SECURITY_BLOCK);
            break;

        case L2CEVT_L2CAP_CONNECT_REQ:                      /* Peer connect request */
            /* stop link timer to avoid race condition between A2MP, Security, and L2CAP */
#ifdef USE_ALARM
            alarm_cancel(p_ccb->p_lcb->l2c_lcb_timer);
#else
            btu_stop_timer(&p_ccb->p_lcb->l2c_lcb_timer);
#endif
#if (BLE_INCLUDED == TRUE)

            if(p_ccb->p_lcb->transport == BT_TRANSPORT_LE) {
                p_ccb->chnl_state = CST_TERM_W4_SEC_COMP;
                l2ble_sec_access_req(p_ccb->p_lcb->remote_bd_addr, p_ccb->p_rcb->psm, FALSE,
                                     &l2c_link_sec_comp, p_ccb);
            } else
#endif
            {
                /* Cancel sniff mode if needed */
                {
                    tBTM_PM_PWR_MD settings;
                    wm_memset((void *)&settings, 0, sizeof(settings));
                    settings.mode = BTM_PM_MD_ACTIVE;
                    /* COVERITY
                    Event uninit_use_in_call: Using uninitialized value "settings" (field "settings".timeout uninitialized) in call to function "BTM_SetPowerMode" [details]
                    Event uninit_use_in_call: Using uninitialized value "settings.max" in call to function "BTM_SetPowerMode" [details]
                    Event uninit_use_in_call: Using uninitialized value "settings.min" in call to function "BTM_SetPowerMode"
                    // FALSE-POSITIVE error from Coverity test-tool. Please do NOT remove following comment.
                    // coverity[uninit_use_in_call] False-positive: setting the mode to BTM_PM_MD_ACTIVE only uses settings.mode the other data members of tBTM_PM_PWR_MD are ignored
                    */
                    BTM_SetPowerMode(BTM_PM_SET_ONLY_ID, p_ccb->p_lcb->remote_bd_addr, &settings);
                }
                p_ccb->chnl_state = CST_TERM_W4_SEC_COMP;

                if(btm_sec_l2cap_access_req(p_ccb->p_lcb->remote_bd_addr, p_ccb->p_rcb->psm,
                                            p_ccb->p_lcb->handle, FALSE, &l2c_link_sec_comp, p_ccb) == BTM_CMD_STARTED) {
                    /* started the security process, tell the peer to set a longer timer */
                    l2cu_send_peer_connect_rsp(p_ccb, L2CAP_CONN_PENDING, 0);
                }
            }

            break;

        case L2CEVT_TIMEOUT:
            L2CAP_TRACE_API("L2CAP - Calling ConnectCfm_Cb(), CID: 0x%04x  Status: %d", p_ccb->local_cid,
                            L2CAP_CONN_TIMEOUT);
            l2cu_release_ccb(p_ccb);
            (*connect_cfm)(local_cid, L2CAP_CONN_TIMEOUT);
            break;

        case L2CEVT_L2CAP_DATA:                         /* Peer data packet rcvd    */
        case L2CEVT_L2CA_DATA_WRITE:                    /* Upper layer data to send */
            GKI_freebuf(p_data);
            break;

        case L2CEVT_L2CA_DISCONNECT_REQ:                 /* Upper wants to disconnect */
            l2cu_release_ccb(p_ccb);
            break;

        case L2CEVT_L2CA_SEND_FLOW_CONTROL_CREDIT:
        case L2CEVT_L2CAP_RECV_FLOW_CONTROL_CREDIT:
            GKI_freebuf(p_data);
            break;
    }
}


/*******************************************************************************
**
** Function         l2c_csm_orig_w4_sec_comp
**
** Description      This function handles events when the channel is in
**                  CST_ORIG_W4_SEC_COMP state.
**
** Returns          void
**
*******************************************************************************/
static void l2c_csm_orig_w4_sec_comp(tL2C_CCB *p_ccb, uint16_t event, void *p_data)
{
    tL2CA_DISCONNECT_IND_CB *disconnect_ind = p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb;
    tL2CA_CONNECT_CFM_CB    *connect_cfm = p_ccb->p_rcb->api.pL2CA_ConnectCfm_Cb;
    uint16_t                  local_cid = p_ccb->local_cid;
#if (BT_TRACE_VERBOSE == TRUE)
    L2CAP_TRACE_EVENT("L2CAP - LCID: 0x%04x  st: ORIG_W4_SEC_COMP  evt: %s", p_ccb->local_cid,
                      l2c_csm_get_event_name(event));
#else
    L2CAP_TRACE_EVENT("L2CAP - st: ORIG_W4_SEC_COMP evt: %d", event);
#endif
#if (L2CAP_UCD_INCLUDED == TRUE)

    if(local_cid == L2CAP_CONNECTIONLESS_CID) {
        /* check if this event can be processed by UCD */
        if(l2c_ucd_process_event(p_ccb, event, p_data)) {
            /* The event is processed by UCD state machine */
            return;
        }
    }

#endif

    switch(event) {
        case L2CEVT_LP_DISCONNECT_IND:                   /* Link was disconnected */
            L2CAP_TRACE_API("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed",
                            p_ccb->local_cid);
            l2cu_release_ccb(p_ccb);
            (*disconnect_ind)(local_cid, FALSE);
            break;

        case L2CEVT_SEC_RE_SEND_CMD:                    /* BTM has enough info to proceed */
        case L2CEVT_LP_CONNECT_CFM:                     /* Link came up         */
#if (BLE_INCLUDED == TRUE)
            if(p_ccb->p_lcb->transport == BT_TRANSPORT_LE) {
                l2ble_sec_access_req(p_ccb->p_lcb->remote_bd_addr, p_ccb->p_rcb->psm, FALSE,
                                     &l2c_link_sec_comp, p_ccb);
            } else
#endif
            {
                btm_sec_l2cap_access_req(p_ccb->p_lcb->remote_bd_addr, p_ccb->p_rcb->psm,
                                         p_ccb->p_lcb->handle, TRUE, &l2c_link_sec_comp, p_ccb);
            }

            break;

        case L2CEVT_SEC_COMP:                            /* Security completed success */
            /* Wait for the info resp in this state before sending connect req (if needed) */
            p_ccb->chnl_state = CST_W4_L2CAP_CONNECT_RSP;
#if (BLE_INCLUDED == TRUE)

            if(p_ccb->p_lcb->transport == BT_TRANSPORT_LE) {
#ifdef USE_ALARM
                alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                                   L2CAP_CHNL_CONNECT_TIMEOUT_MS,
                                   l2c_ccb_timer_timeout, p_ccb,
                                   btu_general_alarm_queue);
#else
                p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
                p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
                btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CONNECT_TIMEOUT_MS / 1000);
#endif
                l2cble_credit_based_conn_req(p_ccb);           /* Start Connection     */
            } else
#endif
            {
                if(!p_ccb->p_lcb->w4_info_rsp) {
                    /* Need to have at least one compatible channel to continue */
                    if(!l2c_fcr_chk_chan_modes(p_ccb)) {
                        l2cu_release_ccb(p_ccb);
                        (*connect_cfm)(local_cid, L2CAP_CONN_NO_LINK);
                    } else {
#ifdef USE_ALARM
                        alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                                           L2CAP_CHNL_CONNECT_TIMEOUT_MS,
                                           l2c_ccb_timer_timeout, p_ccb,
                                           btu_general_alarm_queue);
#else
                        p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
                        p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
                        btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CONNECT_TIMEOUT_MS / 1000);
#endif
                        l2cu_send_peer_connect_req(p_ccb);           /* Start Connection     */
                    }
                }
            }

            break;

        case L2CEVT_SEC_COMP_NEG:
            L2CAP_TRACE_API("L2CAP - Calling ConnectCfm_Cb(), CID: 0x%04x  Status: %d", p_ccb->local_cid,
                            HCI_ERR_AUTH_FAILURE);

            /* If last channel immediately disconnect the ACL for better security.
               Also prevents a race condition between BTM and L2CAP */
            if((p_ccb == p_ccb->p_lcb->ccb_queue.p_first_ccb)
                    && (p_ccb == p_ccb->p_lcb->ccb_queue.p_last_ccb)) {
                p_ccb->p_lcb->idle_timeout = 0;
            }

            l2cu_release_ccb(p_ccb);
            (*connect_cfm)(local_cid, HCI_ERR_AUTH_FAILURE);
            break;

        case L2CEVT_L2CA_DATA_WRITE:                    /* Upper layer data to send */
        case L2CEVT_L2CAP_DATA:                         /* Peer data packet rcvd    */
            GKI_freebuf(p_data);
            break;

        case L2CEVT_L2CA_DISCONNECT_REQ:                 /* Upper wants to disconnect */
            /* Tell security manager to abort */
            btm_sec_abort_access_req(p_ccb->p_lcb->remote_bd_addr);
            l2cu_release_ccb(p_ccb);
            break;
    }
}


/*******************************************************************************
**
** Function         l2c_csm_term_w4_sec_comp
**
** Description      This function handles events when the channel is in
**                  CST_TERM_W4_SEC_COMP state.
**
** Returns          void
**
*******************************************************************************/
static void l2c_csm_term_w4_sec_comp(tL2C_CCB *p_ccb, uint16_t event, void *p_data)
{
#if (BT_TRACE_VERBOSE == TRUE)
    L2CAP_TRACE_EVENT("L2CAP - LCID: 0x%04x  st: TERM_W4_SEC_COMP  evt: %s", p_ccb->local_cid,
                      l2c_csm_get_event_name(event));
#else
    L2CAP_TRACE_EVENT("L2CAP - st: TERM_W4_SEC_COMP evt: %d", event);
#endif
#if (L2CAP_UCD_INCLUDED == TRUE)

    if(p_ccb->local_cid == L2CAP_CONNECTIONLESS_CID) {
        /* check if this event can be processed by UCD */
        if(l2c_ucd_process_event(p_ccb, event, p_data)) {
            /* The event is processed by UCD state machine */
            return;
        }
    }

#endif

    switch(event) {
        case L2CEVT_LP_DISCONNECT_IND:                  /* Link was disconnected */
            /* Tell security manager to abort */
            btm_sec_abort_access_req(p_ccb->p_lcb->remote_bd_addr);
            l2cu_release_ccb(p_ccb);
            break;

        case L2CEVT_SEC_COMP:
            p_ccb->chnl_state = CST_W4_L2CA_CONNECT_RSP;

            /* Wait for the info resp in next state before sending connect ind (if needed) */
            if(!p_ccb->p_lcb->w4_info_rsp) {
                /* Don't need to get info from peer or already retrieved so continue */
#ifdef USE_ALARM
                alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                                   L2CAP_CHNL_CONNECT_TIMEOUT_MS,
                                   l2c_ccb_timer_timeout, p_ccb,
                                   btu_general_alarm_queue);
#else
                p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
                p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
                btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CONNECT_TIMEOUT_MS / 1000);
#endif
                L2CAP_TRACE_API("L2CAP - Calling Connect_Ind_Cb(), CID: 0x%04x", p_ccb->local_cid);
                (*p_ccb->p_rcb->api.pL2CA_ConnectInd_Cb)(p_ccb->p_lcb->remote_bd_addr, p_ccb->local_cid,
                        p_ccb->p_rcb->psm, p_ccb->remote_id);
            } else {
                /*
                ** L2CAP Connect Response will be sent out by 3 sec timer expiration
                ** because Bluesoleil doesn't respond to L2CAP Information Request.
                ** Bluesoleil seems to disconnect ACL link as failure case, because
                ** it takes too long (4~7secs) to get response.
                ** product version : Bluesoleil 2.1.1.0 EDR Release 060123
                ** stack version   : 05.04.11.20060119
                */
                /* Waiting for the info resp, tell the peer to set a longer timer */
                l2cu_send_peer_connect_rsp(p_ccb, L2CAP_CONN_PENDING, 0);
            }

            break;

        case L2CEVT_SEC_COMP_NEG:
            if(((tL2C_CONN_INFO *)p_data)->status == BTM_DELAY_CHECK) {
                /* start a timer - encryption change not received before L2CAP connect req */
#ifdef USE_ALARM
                alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                                   L2CAP_DELAY_CHECK_SM4_TIMEOUT_MS,
                                   l2c_ccb_timer_timeout, p_ccb,
                                   btu_general_alarm_queue);
#else
                p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
                p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
                btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL,
                                L2CAP_DELAY_CHECK_SM4_TIMEOUT_MS / 1000);
#endif
            } else {
#if (BLE_INCLUDED == TRUE)

                if(p_ccb->p_lcb->transport == BT_TRANSPORT_LE) {
                    l2cu_reject_ble_connection(p_ccb->p_lcb, p_ccb->remote_id, L2CAP_LE_INSUFFICIENT_AUTHENTICATION);
                } else
#endif
                    l2cu_send_peer_connect_rsp(p_ccb, L2CAP_CONN_SECURITY_BLOCK, 0);

                l2cu_release_ccb(p_ccb);
            }

            break;

        case L2CEVT_L2CA_DATA_WRITE:                    /* Upper layer data to send */
        case L2CEVT_L2CAP_DATA:                         /* Peer data packet rcvd    */
            GKI_freebuf(p_data);
            break;

        case L2CEVT_L2CA_DISCONNECT_REQ:                 /* Upper wants to disconnect */
            l2cu_release_ccb(p_ccb);
            break;

        case L2CEVT_L2CAP_DISCONNECT_REQ:                  /* Peer disconnected request */
            l2cu_send_peer_disc_rsp(p_ccb->p_lcb, p_ccb->remote_id, p_ccb->local_cid, p_ccb->remote_cid);
            /* Tell security manager to abort */
            btm_sec_abort_access_req(p_ccb->p_lcb->remote_bd_addr);
            l2cu_release_ccb(p_ccb);
            break;

        case L2CEVT_TIMEOUT:

            /* SM4 related. */
            if(!btsnd_hcic_disconnect(p_ccb->p_lcb->handle, HCI_ERR_AUTH_FAILURE)) {
                L2CAP_TRACE_API("L2CAP - Calling btsnd_hcic_disconnect for handle %i failed", p_ccb->p_lcb->handle);
#ifdef USE_ALARM
                alarm_set_on_queue(p_ccb->l2c_ccb_timer, BT_1SEC_TIMEOUT_MS,
                                   l2c_ccb_timer_timeout, p_ccb,
                                   btu_general_alarm_queue);
#else
                p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
                p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
                btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL, 1);
#endif
            }

            break;

        case L2CEVT_SEC_RE_SEND_CMD:                    /* BTM has enough info to proceed */
            btm_sec_l2cap_access_req(p_ccb->p_lcb->remote_bd_addr, p_ccb->p_rcb->psm,
                                     p_ccb->p_lcb->handle, FALSE, &l2c_link_sec_comp, p_ccb);
            break;
    }
}


/*******************************************************************************
**
** Function         l2c_csm_w4_l2cap_connect_rsp
**
** Description      This function handles events when the channel is in
**                  CST_W4_L2CAP_CONNECT_RSP state.
**
** Returns          void
**
*******************************************************************************/
static void l2c_csm_w4_l2cap_connect_rsp(tL2C_CCB *p_ccb, uint16_t event, void *p_data)
{
    tL2C_CONN_INFO          *p_ci = (tL2C_CONN_INFO *)p_data;
    tL2CA_DISCONNECT_IND_CB *disconnect_ind = p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb;
    tL2CA_CONNECT_CFM_CB    *connect_cfm = p_ccb->p_rcb->api.pL2CA_ConnectCfm_Cb;
    uint16_t                  local_cid = p_ccb->local_cid;
#if (BT_TRACE_VERBOSE == TRUE)
    L2CAP_TRACE_EVENT("L2CAP - LCID: 0x%04x  st: W4_L2CAP_CON_RSP  evt: %s", p_ccb->local_cid,
                      l2c_csm_get_event_name(event));
#else
    L2CAP_TRACE_EVENT("L2CAP - st: W4_L2CAP_CON_RSP evt: %d", event);
#endif

    switch(event) {
        case L2CEVT_LP_DISCONNECT_IND:                  /* Link was disconnected */
            /* Send disc indication unless peer to peer race condition AND normal disconnect */
            /* *((uint8_t *)p_data) != HCI_ERR_PEER_USER happens when peer device try to disconnect for normal reason */
            p_ccb->chnl_state = CST_CLOSED;

            if((p_ccb->flags & CCB_FLAG_NO_RETRY) || !p_data || (*((uint8_t *)p_data) != HCI_ERR_PEER_USER)) {
                L2CAP_TRACE_API("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed",
                                p_ccb->local_cid);
                l2cu_release_ccb(p_ccb);
                (*disconnect_ind)(local_cid, FALSE);
            }

            p_ccb->flags |= CCB_FLAG_NO_RETRY;
            break;

        case L2CEVT_L2CAP_CONNECT_RSP:                  /* Got peer connect confirm */
            p_ccb->remote_cid = p_ci->remote_cid;

            if(p_ccb->p_lcb->transport == BT_TRANSPORT_LE) {
                /* Connection is completed */
#ifdef USE_ALARM
                alarm_cancel(p_ccb->l2c_ccb_timer);
#else
                btu_stop_timer(&p_ccb->l2c_ccb_timer);
#endif
                p_ccb->chnl_state = CST_OPEN;
            } else {
                p_ccb->chnl_state = CST_CONFIG;
#ifdef USE_ALARM
                alarm_set_on_queue(p_ccb->l2c_ccb_timer, L2CAP_CHNL_CFG_TIMEOUT_MS,
                                   l2c_ccb_timer_timeout, p_ccb,
                                   btu_general_alarm_queue);
#else
                p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
                p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
                btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CFG_TIMEOUT_MS / 1000);
#endif
            }

            L2CAP_TRACE_API("L2CAP - Calling Connect_Cfm_Cb(), CID: 0x%04x, Success", p_ccb->local_cid);
            (*p_ccb->p_rcb->api.pL2CA_ConnectCfm_Cb)(local_cid, L2CAP_CONN_OK);
            break;

        case L2CEVT_L2CAP_CONNECT_RSP_PND:              /* Got peer connect pending */
            p_ccb->remote_cid = p_ci->remote_cid;
#ifdef USE_ALARM
            alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                               L2CAP_CHNL_CONNECT_EXT_TIMEOUT_MS,
                               l2c_ccb_timer_timeout, p_ccb,
                               btu_general_alarm_queue);
#else
            p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
            p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
            btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL,
                            L2CAP_CHNL_CONNECT_EXT_TIMEOUT_MS / 1000);
#endif

            if(p_ccb->p_rcb->api.pL2CA_ConnectPnd_Cb) {
                L2CAP_TRACE_API("L2CAP - Calling Connect_Pnd_Cb(), CID: 0x%04x", p_ccb->local_cid);
                (*p_ccb->p_rcb->api.pL2CA_ConnectPnd_Cb)(p_ccb->local_cid);
            }

            break;

        case L2CEVT_L2CAP_CONNECT_RSP_NEG:              /* Peer rejected connection */
            L2CAP_TRACE_API("L2CAP - Calling Connect_Cfm_Cb(), CID: 0x%04x, Failure Code: %d", p_ccb->local_cid,
                            p_ci->l2cap_result);
            l2cu_release_ccb(p_ccb);
            (*connect_cfm)(local_cid, p_ci->l2cap_result);
            break;

        case L2CEVT_TIMEOUT:
            L2CAP_TRACE_API("L2CAP - Calling Connect_Cfm_Cb(), CID: 0x%04x, Timeout", p_ccb->local_cid);
            l2cu_release_ccb(p_ccb);
            (*connect_cfm)(local_cid, L2CAP_CONN_TIMEOUT);
            break;

        case L2CEVT_L2CA_DISCONNECT_REQ:                 /* Upper wants to disconnect */

            /* If we know peer CID from connect pending, we can send disconnect */
            if(p_ccb->remote_cid != 0) {
                l2cu_send_peer_disc_req(p_ccb);
                p_ccb->chnl_state = CST_W4_L2CAP_DISCONNECT_RSP;
#ifdef USE_ALARM
                alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                                   L2CAP_CHNL_DISCONNECT_TIMEOUT_MS,
                                   l2c_ccb_timer_timeout, p_ccb,
                                   btu_general_alarm_queue);
#else
                p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
                p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
                btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL,
                                L2CAP_CHNL_DISCONNECT_TIMEOUT_MS / 1000);
#endif
            } else {
                l2cu_release_ccb(p_ccb);
            }

            break;

        case L2CEVT_L2CA_DATA_WRITE:                    /* Upper layer data to send */
        case L2CEVT_L2CAP_DATA:                         /* Peer data packet rcvd    */
            GKI_freebuf(p_data);
            break;

        case L2CEVT_L2CAP_INFO_RSP:

            /* Need to have at least one compatible channel to continue */
            if(!l2c_fcr_chk_chan_modes(p_ccb)) {
                l2cu_release_ccb(p_ccb);
                (*connect_cfm)(local_cid, L2CAP_CONN_NO_LINK);
            } else {
                /* We have feature info, so now send peer connect request */
#ifdef USE_ALARM
                alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                                   L2CAP_CHNL_CONNECT_TIMEOUT_MS,
                                   l2c_ccb_timer_timeout, p_ccb,
                                   btu_general_alarm_queue);
#else
                p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
                p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
                btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CONNECT_TIMEOUT_MS / 1000);
#endif
                l2cu_send_peer_connect_req(p_ccb);           /* Start Connection     */
            }

            break;

        case L2CEVT_L2CA_SEND_FLOW_CONTROL_CREDIT:
        case L2CEVT_L2CAP_RECV_FLOW_CONTROL_CREDIT:
            GKI_freebuf(p_data);
            break;
    }
}


/*******************************************************************************
**
** Function         l2c_csm_w4_l2ca_connect_rsp
**
** Description      This function handles events when the channel is in
**                  CST_W4_L2CA_CONNECT_RSP state.
**
** Returns          void
**
*******************************************************************************/
static void l2c_csm_w4_l2ca_connect_rsp(tL2C_CCB *p_ccb, uint16_t event, void *p_data)
{
    tL2C_CONN_INFO          *p_ci;
    tL2CA_DISCONNECT_IND_CB *disconnect_ind = p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb;
    uint16_t                  local_cid = p_ccb->local_cid;
#if (BT_TRACE_VERBOSE == TRUE)
    L2CAP_TRACE_EVENT("L2CAP - LCID: 0x%04x  st: W4_L2CA_CON_RSP  evt: %s", p_ccb->local_cid,
                      l2c_csm_get_event_name(event));
#else
    L2CAP_TRACE_EVENT("L2CAP - st: W4_L2CA_CON_RSP evt: %d", event);
#endif

    switch(event) {
        case L2CEVT_LP_DISCONNECT_IND:                  /* Link was disconnected */
            L2CAP_TRACE_API("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed",
                            p_ccb->local_cid);
            l2cu_release_ccb(p_ccb);
            (*disconnect_ind)(local_cid, FALSE);
            break;

        case L2CEVT_L2CA_CONNECT_RSP:
            p_ci = (tL2C_CONN_INFO *)p_data;
#if (BLE_INCLUDED == TRUE)

            if(p_ccb->p_lcb->transport == BT_TRANSPORT_LE) {
                /* Result should be OK or Reject */
                if((!p_ci) || (p_ci->l2cap_result == L2CAP_CONN_OK)) {
                    l2cble_credit_based_conn_res(p_ccb, L2CAP_CONN_OK);
                    p_ccb->chnl_state = CST_OPEN;
#ifdef USE_ALARM
                    alarm_cancel(p_ccb->l2c_ccb_timer);
#else
                    btu_stop_timer(&p_ccb->l2c_ccb_timer);
#endif
                } else {
                    l2cble_credit_based_conn_res(p_ccb, p_ci->l2cap_result);
                    l2cu_release_ccb(p_ccb);
                }
            } else
#endif
            {
                /* Result should be OK or PENDING */
                if((!p_ci) || (p_ci->l2cap_result == L2CAP_CONN_OK)) {
                    l2cu_send_peer_connect_rsp(p_ccb, L2CAP_CONN_OK, 0);
                    p_ccb->chnl_state = CST_CONFIG;
#ifdef USE_ALARM
                    alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                                       L2CAP_CHNL_CFG_TIMEOUT_MS,
                                       l2c_ccb_timer_timeout, p_ccb,
                                       btu_general_alarm_queue);
#else
                    p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
                    p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
                    btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CFG_TIMEOUT_MS / 1000);
#endif
                } else {
                    /* If pending, stay in same state and start extended timer */
                    l2cu_send_peer_connect_rsp(p_ccb, p_ci->l2cap_result, p_ci->l2cap_status);
#ifdef USE_ALARM
                    alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                                       L2CAP_CHNL_CONNECT_EXT_TIMEOUT_MS,
                                       l2c_ccb_timer_timeout, p_ccb,
                                       btu_general_alarm_queue);
#else
                    p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
                    p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
                    btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL,
                                    L2CAP_CHNL_CONNECT_EXT_TIMEOUT_MS / 1000);
#endif
                }
            }

            break;

        case L2CEVT_L2CA_CONNECT_RSP_NEG:
            p_ci = (tL2C_CONN_INFO *)p_data;
#if (BLE_INCLUDED == TRUE)

            if(p_ccb->p_lcb->transport == BT_TRANSPORT_LE) {
                l2cble_credit_based_conn_res(p_ccb, p_ci->l2cap_result);
            } else
#endif
                l2cu_send_peer_connect_rsp(p_ccb, p_ci->l2cap_result, p_ci->l2cap_status);

            l2cu_release_ccb(p_ccb);
            break;

        case L2CEVT_TIMEOUT:
            l2cu_send_peer_connect_rsp(p_ccb, L2CAP_CONN_NO_PSM, 0);
            L2CAP_TRACE_API("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed",
                            p_ccb->local_cid);
            l2cu_release_ccb(p_ccb);
            (*disconnect_ind)(local_cid, FALSE);
            break;

        case L2CEVT_L2CA_DATA_WRITE:                    /* Upper layer data to send */
        case L2CEVT_L2CAP_DATA:                         /* Peer data packet rcvd    */
            GKI_freebuf(p_data);
            break;

        case L2CEVT_L2CA_DISCONNECT_REQ:                 /* Upper wants to disconnect */
            l2cu_send_peer_disc_req(p_ccb);
            p_ccb->chnl_state = CST_W4_L2CAP_DISCONNECT_RSP;
#ifdef USE_ALARM
            alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                               L2CAP_CHNL_DISCONNECT_TIMEOUT_MS,
                               l2c_ccb_timer_timeout, p_ccb,
                               btu_general_alarm_queue);
#else
            p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
            p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
            btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL,
                            L2CAP_CHNL_DISCONNECT_TIMEOUT_MS / 1000);
#endif
            break;

        case L2CEVT_L2CAP_INFO_RSP:
            /* We have feature info, so now give the upper layer connect IND */
#ifdef USE_ALARM
            alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                               L2CAP_CHNL_CONNECT_TIMEOUT_MS,
                               l2c_ccb_timer_timeout, p_ccb,
                               btu_general_alarm_queue);
#else
            p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
            p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
            btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CONNECT_TIMEOUT_MS / 1000);
#endif
            L2CAP_TRACE_API("L2CAP - Calling Connect_Ind_Cb(), CID: 0x%04x", p_ccb->local_cid);
            (*p_ccb->p_rcb->api.pL2CA_ConnectInd_Cb)(p_ccb->p_lcb->remote_bd_addr,
                    p_ccb->local_cid,
                    p_ccb->p_rcb->psm,
                    p_ccb->remote_id);
            break;
    }
}


/*******************************************************************************
**
** Function         l2c_csm_config
**
** Description      This function handles events when the channel is in
**                  CONFIG state.
**
** Returns          void
**
*******************************************************************************/
static void l2c_csm_config(tL2C_CCB *p_ccb, uint16_t event, void *p_data)
{
    tL2CAP_CFG_INFO         *p_cfg = (tL2CAP_CFG_INFO *)p_data;
    tL2CA_DISCONNECT_IND_CB *disconnect_ind = p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb;
    uint16_t                  local_cid = p_ccb->local_cid;
    uint8_t                   cfg_result;
#if (BT_TRACE_VERBOSE == TRUE)
    L2CAP_TRACE_EVENT("L2CAP - LCID: 0x%04x  st: CONFIG  evt: %s", p_ccb->local_cid,
                      l2c_csm_get_event_name(event));
#else
    L2CAP_TRACE_EVENT("L2CAP - st: CONFIG evt: %d", event);
#endif

    switch(event) {
        case L2CEVT_LP_DISCONNECT_IND:                  /* Link was disconnected */
            L2CAP_TRACE_API("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed",
                            p_ccb->local_cid);
            l2cu_release_ccb(p_ccb);
            (*disconnect_ind)(local_cid, FALSE);
            break;

        case L2CEVT_L2CAP_CONFIG_REQ:                  /* Peer config request   */
            if((cfg_result = l2cu_process_peer_cfg_req(p_ccb, p_cfg)) == L2CAP_PEER_CFG_OK) {
                L2CAP_TRACE_EVENT("L2CAP - Calling Config_Req_Cb(), CID: 0x%04x, C-bit %d",
                                  p_ccb->local_cid, (p_cfg->flags & L2CAP_CFG_FLAGS_MASK_CONT));
                (*p_ccb->p_rcb->api.pL2CA_ConfigInd_Cb)(p_ccb->local_cid, p_cfg);
            } else if(cfg_result == L2CAP_PEER_CFG_DISCONNECT) {
                /* Disconnect if channels are incompatible */
                L2CAP_TRACE_EVENT("L2CAP - incompatible configurations disconnect");
                l2cu_disconnect_chnl(p_ccb);
            } else { /* Return error to peer so he can renegotiate if possible */
                L2CAP_TRACE_EVENT("L2CAP - incompatible configurations trying reconfig");
                l2cu_send_peer_config_rsp(p_ccb, p_cfg);
            }

            break;

        case L2CEVT_L2CAP_CONFIG_RSP:                  /* Peer config response  */
            l2cu_process_peer_cfg_rsp(p_ccb, p_cfg);

            if(p_cfg->result != L2CAP_CFG_PENDING) {
                /* TBD: When config options grow beyong minimum MTU (48 bytes)
                 *      logic needs to be added to handle responses with
                 *      continuation bit set in flags field.
                 *       1. Send additional config request out until C-bit is cleared in response
                 */
                p_ccb->config_done |= OB_CFG_DONE;

                if(p_ccb->config_done & IB_CFG_DONE) {
                    /* Verify two sides are in compatible modes before continuing */
                    if(p_ccb->our_cfg.fcr.mode != p_ccb->peer_cfg.fcr.mode) {
                        l2cu_send_peer_disc_req(p_ccb);
                        L2CAP_TRACE_WARNING("L2CAP - Calling Disconnect_Ind_Cb(Incompatible CFG), CID: 0x%04x  No Conf Needed",
                                            p_ccb->local_cid);
                        l2cu_release_ccb(p_ccb);
                        (*disconnect_ind)(local_cid, FALSE);
                        break;
                    }

                    p_ccb->config_done |= RECONFIG_FLAG;
                    p_ccb->chnl_state = CST_OPEN;
                    l2c_link_adjust_chnl_allocation();
#ifdef USE_ALARM
                    alarm_cancel(p_ccb->l2c_ccb_timer);
#else
                    btu_stop_timer(&p_ccb->l2c_ccb_timer);
#endif

                    /* If using eRTM and waiting for an ACK, restart the ACK timer */
                    if(p_ccb->fcrb.wait_ack) {
                        l2c_fcr_start_timer(p_ccb);
                    }

                    /*
                    ** check p_ccb->our_cfg.fcr.mon_tout and p_ccb->our_cfg.fcr.rtrans_tout
                    ** we may set them to zero when sending config request during renegotiation
                    */
                    if((p_ccb->our_cfg.fcr.mode == L2CAP_FCR_ERTM_MODE)
                            && ((p_ccb->our_cfg.fcr.mon_tout == 0) || (p_ccb->our_cfg.fcr.rtrans_tout))) {
                        l2c_fcr_adj_monitor_retran_timeout(p_ccb);
                    }

#if (L2CAP_ERTM_STATS == TRUE)
                    p_ccb->fcrb.connect_tick_count = GKI_time_get_os_boottime_ms();
#endif

                    /* See if we can forward anything on the hold queue */
                    if(!fixed_queue_is_empty(p_ccb->xmit_hold_q)) {
                        l2c_link_check_send_pkts(p_ccb->p_lcb, NULL, NULL);
                    }
                }
            }

            L2CAP_TRACE_API("L2CAP - Calling Config_Rsp_Cb(), CID: 0x%04x", p_ccb->local_cid);
            (*p_ccb->p_rcb->api.pL2CA_ConfigCfm_Cb)(p_ccb->local_cid, p_cfg);
            break;

        case L2CEVT_L2CAP_CONFIG_RSP_NEG:              /* Peer config error rsp */
            /* Disable the Timer */
#ifdef USE_ALARM
            alarm_cancel(p_ccb->l2c_ccb_timer);
#else
            btu_stop_timer(&p_ccb->l2c_ccb_timer);
#endif

            /* If failure was channel mode try to renegotiate */
            if(l2c_fcr_renegotiate_chan(p_ccb, p_cfg) == FALSE) {
                L2CAP_TRACE_API("L2CAP - Calling Config_Rsp_Cb(), CID: 0x%04x, Failure: %d", p_ccb->local_cid,
                                p_cfg->result);
                (*p_ccb->p_rcb->api.pL2CA_ConfigCfm_Cb)(p_ccb->local_cid, p_cfg);
            }

            break;

        case L2CEVT_L2CAP_DISCONNECT_REQ:                  /* Peer disconnected request */
#ifdef USE_ALARM
            alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                               L2CAP_CHNL_DISCONNECT_TIMEOUT_MS,
                               l2c_ccb_timer_timeout, p_ccb,
                               btu_general_alarm_queue);
#else
            p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
            p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
            btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL,
                            L2CAP_CHNL_DISCONNECT_TIMEOUT_MS / 1000);
#endif
            p_ccb->chnl_state = CST_W4_L2CA_DISCONNECT_RSP;
            L2CAP_TRACE_API("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  Conf Needed", p_ccb->local_cid);
            (*p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb)(p_ccb->local_cid, TRUE);
            break;

        case L2CEVT_L2CA_CONFIG_REQ:                   /* Upper layer config req   */
            l2cu_process_our_cfg_req(p_ccb, p_cfg);
            l2cu_send_peer_config_req(p_ccb, p_cfg);
#ifdef USE_ALARM
            alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                               L2CAP_CHNL_CFG_TIMEOUT_MS,
                               l2c_ccb_timer_timeout, p_ccb,
                               btu_general_alarm_queue);
#else
            p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
            p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
            btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CFG_TIMEOUT_MS / 1000);
#endif
            break;

        case L2CEVT_L2CA_CONFIG_RSP:                   /* Upper layer config rsp   */
            l2cu_process_our_cfg_rsp(p_ccb, p_cfg);

            /* Not finished if continuation flag is set */
            if((p_cfg->flags & L2CAP_CFG_FLAGS_MASK_CONT) || (p_cfg->result == L2CAP_CFG_PENDING)) {
                /* Send intermediate response; remain in cfg state */
                l2cu_send_peer_config_rsp(p_ccb, p_cfg);
                break;
            }

            /* Local config done; clear cached configuration in case reconfig takes place later */
            p_ccb->peer_cfg.mtu_present = FALSE;
            p_ccb->peer_cfg.flush_to_present = FALSE;
            p_ccb->peer_cfg.qos_present = FALSE;
            p_ccb->config_done |= IB_CFG_DONE;

            if(p_ccb->config_done & OB_CFG_DONE) {
                /* Verify two sides are in compatible modes before continuing */
                if(p_ccb->our_cfg.fcr.mode != p_ccb->peer_cfg.fcr.mode) {
                    l2cu_send_peer_disc_req(p_ccb);
                    L2CAP_TRACE_WARNING("L2CAP - Calling Disconnect_Ind_Cb(Incompatible CFG), CID: 0x%04x  No Conf Needed",
                                        p_ccb->local_cid);
                    l2cu_release_ccb(p_ccb);
                    (*disconnect_ind)(local_cid, FALSE);
                    break;
                }

                p_ccb->config_done |= RECONFIG_FLAG;
                p_ccb->chnl_state = CST_OPEN;
                l2c_link_adjust_chnl_allocation();
#ifdef USE_ALARM
                alarm_cancel(p_ccb->l2c_ccb_timer);
#else
                btu_stop_timer(&p_ccb->l2c_ccb_timer);
#endif
            }

            l2cu_send_peer_config_rsp(p_ccb, p_cfg);

            /* If using eRTM and waiting for an ACK, restart the ACK timer */
            if(p_ccb->fcrb.wait_ack) {
                l2c_fcr_start_timer(p_ccb);
            }

#if (L2CAP_ERTM_STATS == TRUE)
            p_ccb->fcrb.connect_tick_count = GKI_time_get_os_boottime_ms();
#endif

            /* See if we can forward anything on the hold queue */
            if((p_ccb->chnl_state == CST_OPEN) &&
                    (!fixed_queue_is_empty(p_ccb->xmit_hold_q))) {
                l2c_link_check_send_pkts(p_ccb->p_lcb, NULL, NULL);
            }

            break;

        case L2CEVT_L2CA_CONFIG_RSP_NEG:               /* Upper layer config reject */
            l2cu_send_peer_config_rsp(p_ccb, p_cfg);
#ifdef USE_ALARM
            alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                               L2CAP_CHNL_CFG_TIMEOUT_MS,
                               l2c_ccb_timer_timeout, p_ccb,
                               btu_general_alarm_queue);
#else
            p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
            p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
            btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CFG_TIMEOUT_MS / 1000);
#endif
            break;

        case L2CEVT_L2CA_DISCONNECT_REQ:                 /* Upper wants to disconnect */
            l2cu_send_peer_disc_req(p_ccb);
            p_ccb->chnl_state = CST_W4_L2CAP_DISCONNECT_RSP;
#ifdef USE_ALARM
            alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                               L2CAP_CHNL_DISCONNECT_TIMEOUT_MS,
                               l2c_ccb_timer_timeout, p_ccb,
                               btu_general_alarm_queue);
#else
            p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
            p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
            btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL,
                            L2CAP_CHNL_DISCONNECT_TIMEOUT_MS / 1000);
#endif
            break;

        case L2CEVT_L2CAP_DATA:                        /* Peer data packet rcvd    */
            L2CAP_TRACE_API("L2CAP - Calling DataInd_Cb(), CID: 0x%04x", p_ccb->local_cid);
#if (L2CAP_NUM_FIXED_CHNLS > 0)

            if(p_ccb->local_cid >= L2CAP_FIRST_FIXED_CHNL &&
                    p_ccb->local_cid <= L2CAP_LAST_FIXED_CHNL) {
                if(p_ccb->local_cid < L2CAP_BASE_APPL_CID) {
                    if(l2cb.fixed_reg[p_ccb->local_cid - L2CAP_FIRST_FIXED_CHNL].pL2CA_FixedData_Cb)
                        (*l2cb.fixed_reg[p_ccb->local_cid - L2CAP_FIRST_FIXED_CHNL].pL2CA_FixedData_Cb)
                        (p_ccb->local_cid, p_ccb->p_lcb->remote_bd_addr, (BT_HDR *)p_data);
                    else {
                        GKI_freebuf(p_data);
                    }

                    break;
                }
            }

#endif
            (*p_ccb->p_rcb->api.pL2CA_DataInd_Cb)(p_ccb->local_cid, (BT_HDR *)p_data);
            break;

        case L2CEVT_L2CA_DATA_WRITE:                   /* Upper layer data to send */
            if(p_ccb->config_done & OB_CFG_DONE) {
                l2c_enqueue_peer_data(p_ccb, (BT_HDR *)p_data);
            } else {
                GKI_freebuf(p_data);
            }

            break;

        case L2CEVT_TIMEOUT:
            l2cu_send_peer_disc_req(p_ccb);
            L2CAP_TRACE_API("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed",
                            p_ccb->local_cid);
            l2cu_release_ccb(p_ccb);
            (*disconnect_ind)(local_cid, FALSE);
            break;
    }
}


/*******************************************************************************
**
** Function         l2c_csm_open
**
** Description      This function handles events when the channel is in
**                  OPEN state.
**
** Returns          void
**
*******************************************************************************/
static void l2c_csm_open(tL2C_CCB *p_ccb, uint16_t event, void *p_data)
{
    uint16_t                  local_cid = p_ccb->local_cid;
    tL2CAP_CFG_INFO         *p_cfg;
    tL2C_CHNL_STATE         tempstate;
    uint8_t                   tempcfgdone;
    uint8_t                   cfg_result;
#if (BLE_INCLUDED == TRUE)	
    uint16_t                  *credit;
#endif
#if (BT_TRACE_VERBOSE == TRUE)
    L2CAP_TRACE_EVENT("L2CAP - LCID: 0x%04x  st: OPEN  evt: %s",
                      p_ccb->local_cid, l2c_csm_get_event_name(event));
#else
    L2CAP_TRACE_EVENT("L2CAP - st: OPEN evt: %d", event);
#endif
#if (L2CAP_UCD_INCLUDED == TRUE)

    if(local_cid == L2CAP_CONNECTIONLESS_CID) {
        /* check if this event can be processed by UCD */
        if(l2c_ucd_process_event(p_ccb, event, p_data)) {
            /* The event is processed by UCD state machine */
            return;
        }
    }

#endif

    switch(event) {
        case L2CEVT_LP_DISCONNECT_IND:                  /* Link was disconnected */
            L2CAP_TRACE_API("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed",
                            p_ccb->local_cid);
            l2cu_release_ccb(p_ccb);

            if(p_ccb->p_rcb) {
                (*p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb)(local_cid, FALSE);
            }

            break;

        case L2CEVT_LP_QOS_VIOLATION_IND:               /* QOS violation         */

            /* Tell upper layer. If service guaranteed, then clear the channel   */
            if(p_ccb->p_rcb->api.pL2CA_QoSViolationInd_Cb) {
                (*p_ccb->p_rcb->api.pL2CA_QoSViolationInd_Cb)(p_ccb->p_lcb->remote_bd_addr);
            }

            break;

        case L2CEVT_L2CAP_CONFIG_REQ:                  /* Peer config request   */
            p_cfg = (tL2CAP_CFG_INFO *)p_data;
            tempstate = p_ccb->chnl_state;
            tempcfgdone = p_ccb->config_done;
            p_ccb->chnl_state = CST_CONFIG;
            p_ccb->config_done &= ~CFG_DONE_MASK;
#ifdef USE_ALARM
            alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                               L2CAP_CHNL_CFG_TIMEOUT_MS,
                               l2c_ccb_timer_timeout, p_ccb,
                               btu_general_alarm_queue);
#else
            p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
            p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
            btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CFG_TIMEOUT_MS / 1000);
#endif

            if((cfg_result = l2cu_process_peer_cfg_req(p_ccb, p_cfg)) == L2CAP_PEER_CFG_OK) {
                (*p_ccb->p_rcb->api.pL2CA_ConfigInd_Cb)(p_ccb->local_cid, p_cfg);
            }
            /* Error in config parameters: reset state and config flag */
            else if(cfg_result == L2CAP_PEER_CFG_UNACCEPTABLE) {
#ifdef USE_ALARM
                alarm_cancel(p_ccb->l2c_ccb_timer);
#else
                btu_stop_timer(&p_ccb->l2c_ccb_timer);
#endif
                p_ccb->chnl_state = tempstate;
                p_ccb->config_done = tempcfgdone;
                l2cu_send_peer_config_rsp(p_ccb, p_cfg);
            } else { /* L2CAP_PEER_CFG_DISCONNECT */
                /* Disconnect if channels are incompatible
                 * Note this should not occur if reconfigure
                 * since this should have never passed original config.
                 */
                l2cu_disconnect_chnl(p_ccb);
            }

            break;

        case L2CEVT_L2CAP_DISCONNECT_REQ:                  /* Peer disconnected request */
            if(p_ccb->p_lcb->transport != BT_TRANSPORT_LE) {
                /* Make sure we are not in sniff mode */
                {
                    tBTM_PM_PWR_MD settings;
                    wm_memset((void *)&settings, 0, sizeof(settings));
                    settings.mode = BTM_PM_MD_ACTIVE;
                    BTM_SetPowerMode(BTM_PM_SET_ONLY_ID, p_ccb->p_lcb->remote_bd_addr, &settings);
                }
            }

            p_ccb->chnl_state = CST_W4_L2CA_DISCONNECT_RSP;
#ifdef USE_ALARM
            alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                               L2CAP_CHNL_DISCONNECT_TIMEOUT_MS,
                               l2c_ccb_timer_timeout, p_ccb,
                               btu_general_alarm_queue);
#else
            p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
            p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
            btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL,
                            L2CAP_CHNL_DISCONNECT_TIMEOUT_MS / 1000);
#endif
            L2CAP_TRACE_API("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  Conf Needed\r\n",
                            p_ccb->local_cid);
            (*p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb)(p_ccb->local_cid, TRUE);
            break;

        case L2CEVT_L2CAP_DATA:                         /* Peer data packet rcvd    */
            if((p_ccb->p_rcb) && (p_ccb->p_rcb->api.pL2CA_DataInd_Cb)) {
                (*p_ccb->p_rcb->api.pL2CA_DataInd_Cb)(p_ccb->local_cid, (BT_HDR *)p_data);
            }

            break;

        case L2CEVT_L2CA_DISCONNECT_REQ:                 /* Upper wants to disconnect */
            if(p_ccb->p_lcb->transport != BT_TRANSPORT_LE) {
                /* Make sure we are not in sniff mode */
                {
                    tBTM_PM_PWR_MD settings;
                    wm_memset((void *)&settings, 0, sizeof(settings));
                    settings.mode = BTM_PM_MD_ACTIVE;
                    BTM_SetPowerMode(BTM_PM_SET_ONLY_ID, p_ccb->p_lcb->remote_bd_addr, &settings);
                }
            }

#if (BLE_INCLUDED == TRUE)

            if(p_ccb->p_lcb->transport == BT_TRANSPORT_LE) {
                l2cble_send_peer_disc_req(p_ccb);
            } else
#endif
                l2cu_send_peer_disc_req(p_ccb);

            p_ccb->chnl_state = CST_W4_L2CAP_DISCONNECT_RSP;
#ifdef USE_ALARM
            alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                               L2CAP_CHNL_DISCONNECT_TIMEOUT_MS,
                               l2c_ccb_timer_timeout, p_ccb,
                               btu_general_alarm_queue);
#else
            p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
            p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
            btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL,
                            L2CAP_CHNL_DISCONNECT_TIMEOUT_MS / 1000);
#endif
            break;

        case L2CEVT_L2CA_DATA_WRITE:                    /* Upper layer data to send */
            l2c_enqueue_peer_data(p_ccb, (BT_HDR *)p_data);
            l2c_link_check_send_pkts(p_ccb->p_lcb, NULL, NULL);
            break;

        case L2CEVT_L2CA_CONFIG_REQ:                   /* Upper layer config req   */
            p_ccb->chnl_state = CST_CONFIG;
            p_ccb->config_done &= ~CFG_DONE_MASK;
            l2cu_process_our_cfg_req(p_ccb, (tL2CAP_CFG_INFO *)p_data);
            l2cu_send_peer_config_req(p_ccb, (tL2CAP_CFG_INFO *)p_data);
#ifdef USE_ALARM
            alarm_set_on_queue(p_ccb->l2c_ccb_timer,
                               L2CAP_CHNL_CFG_TIMEOUT_MS,
                               l2c_ccb_timer_timeout, p_ccb,
                               btu_general_alarm_queue);
#else
            p_ccb->l2c_ccb_timer.p_cback = (TIMER_CBACK *)&l2c_ccb_timer_timeout;
            p_ccb->l2c_ccb_timer.param = (TIMER_PARAM_TYPE)p_ccb;
            btu_start_timer(&p_ccb->l2c_ccb_timer, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CFG_TIMEOUT_MS / 1000);
#endif
            break;

        case L2CEVT_TIMEOUT:

            /* Process the monitor/retransmission time-outs in flow control/retrans mode */
            if(p_ccb->peer_cfg.fcr.mode == L2CAP_FCR_ERTM_MODE) {
                l2c_fcr_proc_tout(p_ccb);
            }

            break;

        case L2CEVT_ACK_TIMEOUT:
            l2c_fcr_proc_ack_tout(p_ccb);
            break;
#if (BLE_INCLUDED == TRUE)

        case L2CEVT_L2CA_SEND_FLOW_CONTROL_CREDIT:
            L2CAP_TRACE_DEBUG("%s Sending credit", __func__);
            credit = (uint16_t *)p_data;
            l2cble_send_flow_control_credit(p_ccb, *credit);
            break;

        case L2CEVT_L2CAP_RECV_FLOW_CONTROL_CREDIT:
            credit = (uint16_t *)p_data;
            L2CAP_TRACE_DEBUG("%s Credits received %d", __func__, *credit);

            if((p_ccb->peer_conn_cfg.credits + *credit) > L2CAP_LE_MAX_CREDIT) {
                /* we have received credits more than max coc credits,
                 * so disconnecting the Le Coc Channel
                 */
                l2cble_send_peer_disc_req(p_ccb);
            } else {
                p_ccb->peer_conn_cfg.credits += *credit;
                l2c_link_check_send_pkts(p_ccb->p_lcb, NULL, NULL);
            }

            break;
#endif
    }
}


/*******************************************************************************
**
** Function         l2c_csm_w4_l2cap_disconnect_rsp
**
** Description      This function handles events when the channel is in
**                  CST_W4_L2CAP_DISCONNECT_RSP state.
**
** Returns          void
**
*******************************************************************************/
static void l2c_csm_w4_l2cap_disconnect_rsp(tL2C_CCB *p_ccb, uint16_t event, void *p_data)
{
    tL2CA_DISCONNECT_CFM_CB *disconnect_cfm = p_ccb->p_rcb->api.pL2CA_DisconnectCfm_Cb;
    uint16_t                  local_cid = p_ccb->local_cid;
#if (BT_TRACE_VERBOSE == TRUE)
    L2CAP_TRACE_EVENT("L2CAP - LCID: 0x%04x  st: W4_L2CAP_DISC_RSP  evt: %s", p_ccb->local_cid,
                      l2c_csm_get_event_name(event));
#else
    L2CAP_TRACE_EVENT("L2CAP - st: W4_L2CAP_DISC_RSP evt: %d", event);
#endif

    switch(event) {
        case L2CEVT_L2CAP_DISCONNECT_RSP:                /* Peer disconnect response */
            l2cu_release_ccb(p_ccb);

            if(disconnect_cfm) {
                L2CAP_TRACE_API("L2CAP - Calling DisconnectCfm_Cb(), CID: 0x%04x", local_cid);
                (*disconnect_cfm)(local_cid, L2CAP_DISC_OK);
            }

            break;

        case L2CEVT_L2CAP_DISCONNECT_REQ:                /* Peer disconnect request  */
            l2cu_send_peer_disc_rsp(p_ccb->p_lcb, p_ccb->remote_id, p_ccb->local_cid, p_ccb->remote_cid);
            l2cu_release_ccb(p_ccb);

            if(disconnect_cfm) {
                L2CAP_TRACE_API("L2CAP - Calling DisconnectCfm_Cb(), CID: 0x%04x", local_cid);
                (*disconnect_cfm)(local_cid, L2CAP_DISC_OK);
            }

            break;

        case L2CEVT_LP_DISCONNECT_IND:                  /* Link was disconnected */
        case L2CEVT_TIMEOUT:                            /* Timeout */
            l2cu_release_ccb(p_ccb);

            if(disconnect_cfm) {
                L2CAP_TRACE_API("L2CAP - Calling DisconnectCfm_Cb(), CID: 0x%04x", local_cid);
                (*disconnect_cfm)(local_cid, L2CAP_DISC_TIMEOUT);
            }

            break;

        case L2CEVT_L2CAP_DATA:                         /* Peer data packet rcvd    */
        case L2CEVT_L2CA_DATA_WRITE:                    /* Upper layer data to send */
            GKI_freebuf(p_data);
            break;

        case L2CEVT_L2CA_SEND_FLOW_CONTROL_CREDIT:
        case L2CEVT_L2CAP_RECV_FLOW_CONTROL_CREDIT:
            GKI_freebuf(p_data);
            break;
    }
}


/*******************************************************************************
**
** Function         l2c_csm_w4_l2ca_disconnect_rsp
**
** Description      This function handles events when the channel is in
**                  CST_W4_L2CA_DISCONNECT_RSP state.
**
** Returns          void
**
*******************************************************************************/
static void l2c_csm_w4_l2ca_disconnect_rsp(tL2C_CCB *p_ccb, uint16_t event, void *p_data)
{
    tL2CA_DISCONNECT_IND_CB *disconnect_ind = p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb;
    uint16_t                  local_cid = p_ccb->local_cid;
#if (BT_TRACE_VERBOSE == TRUE)
    L2CAP_TRACE_EVENT("L2CAP - LCID: 0x%04x  st: W4_L2CA_DISC_RSP  evt: %s", p_ccb->local_cid,
                      l2c_csm_get_event_name(event));
#else
    L2CAP_TRACE_EVENT("L2CAP - st: W4_L2CA_DISC_RSP evt: %d", event);
#endif

    switch(event) {
        case L2CEVT_LP_DISCONNECT_IND:                  /* Link was disconnected */
            L2CAP_TRACE_API("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed",
                            p_ccb->local_cid);
            l2cu_release_ccb(p_ccb);
            (*disconnect_ind)(local_cid, FALSE);
            break;

        case L2CEVT_TIMEOUT:
            l2cu_send_peer_disc_rsp(p_ccb->p_lcb, p_ccb->remote_id, p_ccb->local_cid, p_ccb->remote_cid);
            L2CAP_TRACE_API("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed",
                            p_ccb->local_cid);
            l2cu_release_ccb(p_ccb);
            (*disconnect_ind)(local_cid, FALSE);
            break;

        case L2CEVT_L2CA_DISCONNECT_REQ:                /* Upper disconnect request */
        case L2CEVT_L2CA_DISCONNECT_RSP:                /* Upper disconnect response */
            l2cu_send_peer_disc_rsp(p_ccb->p_lcb, p_ccb->remote_id, p_ccb->local_cid, p_ccb->remote_cid);
            l2cu_release_ccb(p_ccb);
            break;

        case L2CEVT_L2CAP_DATA:                         /* Peer data packet rcvd    */
        case L2CEVT_L2CA_DATA_WRITE:                    /* Upper layer data to send */
            GKI_freebuf(p_data);
            break;
    }
}


#if (WM_BT_TRACE_VERBOSE == TRUE)
/*******************************************************************************
**
** Function         l2c_csm_get_event_name
**
** Description      This function returns the event name.
**
** NOTE             conditionally compiled to save memory.
**
** Returns          pointer to the name
**
*******************************************************************************/
static char *l2c_csm_get_event_name(uint16_t event)
{
    switch(event) {
        case L2CEVT_LP_CONNECT_CFM:                  /* Lower layer connect confirm          */
            return ("LOWER_LAYER_CONNECT_CFM");

        case L2CEVT_LP_CONNECT_CFM_NEG:              /* Lower layer connect confirm (failed) */
            return ("LOWER_LAYER_CONNECT_CFM_NEG");

        case L2CEVT_LP_CONNECT_IND:                  /* Lower layer connect indication       */
            return ("LOWER_LAYER_CONNECT_IND");

        case L2CEVT_LP_DISCONNECT_IND:               /* Lower layer disconnect indication    */
            return ("LOWER_LAYER_DISCONNECT_IND");

        case L2CEVT_LP_QOS_CFM:                      /* Lower layer QOS confirmation         */
            return ("LOWER_LAYER_QOS_CFM");

        case L2CEVT_LP_QOS_CFM_NEG:                  /* Lower layer QOS confirmation (failed)*/
            return ("LOWER_LAYER_QOS_CFM_NEG");

        case L2CEVT_LP_QOS_VIOLATION_IND:            /* Lower layer QOS violation indication */
            return ("LOWER_LAYER_QOS_VIOLATION_IND");

        case L2CEVT_SEC_COMP:                        /* Security cleared successfully        */
            return ("SECURITY_COMPLETE");

        case L2CEVT_SEC_COMP_NEG:                    /* Security procedure failed            */
            return ("SECURITY_COMPLETE_NEG");

        case L2CEVT_L2CAP_CONNECT_REQ:               /* Peer connection request              */
            return ("PEER_CONNECT_REQ");

        case L2CEVT_L2CAP_CONNECT_RSP:               /* Peer connection response             */
            return ("PEER_CONNECT_RSP");

        case L2CEVT_L2CAP_CONNECT_RSP_PND:           /* Peer connection response pending     */
            return ("PEER_CONNECT_RSP_PND");

        case L2CEVT_L2CAP_CONNECT_RSP_NEG:           /* Peer connection response (failed)    */
            return ("PEER_CONNECT_RSP_NEG");

        case L2CEVT_L2CAP_CONFIG_REQ:                /* Peer configuration request           */
            return ("PEER_CONFIG_REQ");

        case L2CEVT_L2CAP_CONFIG_RSP:                /* Peer configuration response          */
            return ("PEER_CONFIG_RSP");

        case L2CEVT_L2CAP_CONFIG_RSP_NEG:            /* Peer configuration response (failed) */
            return ("PEER_CONFIG_RSP_NEG");

        case L2CEVT_L2CAP_DISCONNECT_REQ:            /* Peer disconnect request              */
            return ("PEER_DISCONNECT_REQ");

        case L2CEVT_L2CAP_DISCONNECT_RSP:            /* Peer disconnect response             */
            return ("PEER_DISCONNECT_RSP");

        case L2CEVT_L2CAP_DATA:                      /* Peer data                            */
            return ("PEER_DATA");

        case L2CEVT_L2CA_CONNECT_REQ:                /* Upper layer connect request          */
            return ("UPPER_LAYER_CONNECT_REQ");

        case L2CEVT_L2CA_CONNECT_RSP:                /* Upper layer connect response         */
            return ("UPPER_LAYER_CONNECT_RSP");

        case L2CEVT_L2CA_CONNECT_RSP_NEG:            /* Upper layer connect response (failed)*/
            return ("UPPER_LAYER_CONNECT_RSP_NEG");

        case L2CEVT_L2CA_CONFIG_REQ:                 /* Upper layer config request           */
            return ("UPPER_LAYER_CONFIG_REQ");

        case L2CEVT_L2CA_CONFIG_RSP:                 /* Upper layer config response          */
            return ("UPPER_LAYER_CONFIG_RSP");

        case L2CEVT_L2CA_CONFIG_RSP_NEG:             /* Upper layer config response (failed) */
            return ("UPPER_LAYER_CONFIG_RSP_NEG");

        case L2CEVT_L2CA_DISCONNECT_REQ:             /* Upper layer disconnect request       */
            return ("UPPER_LAYER_DISCONNECT_REQ");

        case L2CEVT_L2CA_DISCONNECT_RSP:             /* Upper layer disconnect response      */
            return ("UPPER_LAYER_DISCONNECT_RSP");

        case L2CEVT_L2CA_DATA_READ:                  /* Upper layer data read                */
            return ("UPPER_LAYER_DATA_READ");

        case L2CEVT_L2CA_DATA_WRITE:                 /* Upper layer data write               */
            return ("UPPER_LAYER_DATA_WRITE");

        case L2CEVT_TIMEOUT:                         /* Timeout                              */
            return ("TIMEOUT");

        case L2CEVT_SEC_RE_SEND_CMD:
            return ("SEC_RE_SEND_CMD");

        case L2CEVT_L2CAP_INFO_RSP:                  /* Peer information response            */
            return ("L2CEVT_L2CAP_INFO_RSP");

        case L2CEVT_ACK_TIMEOUT:
            return ("L2CEVT_ACK_TIMEOUT");

        case L2CEVT_L2CA_SEND_FLOW_CONTROL_CREDIT:   /* Upper layer send credit packet       */
            return ("SEND_FLOW_CONTROL_CREDIT");

        case L2CEVT_L2CAP_RECV_FLOW_CONTROL_CREDIT:  /* Peer send credit packet              */
            return ("RECV_FLOW_CONTROL_CREDIT");

        default:
            return ("???? UNKNOWN EVENT");
    }
}
#endif /* (BT_TRACE_VERBOSE == TRUE) */


/*******************************************************************************
**
** Function         l2c_enqueue_peer_data
**
** Description      Enqueues data destined for the peer in the ccb. Handles
**                  FCR segmentation and checks for congestion.
**
** Returns          void
**
*******************************************************************************/
void l2c_enqueue_peer_data(tL2C_CCB *p_ccb, BT_HDR *p_buf)
{
    uint8_t       *p;

    if(p_ccb->peer_cfg.fcr.mode != L2CAP_FCR_BASIC_MODE) {
        p_buf->event = 0;
    } else {
        /* Save the channel ID for faster counting */
        p_buf->event = p_ccb->local_cid;
        /* Step back to add the L2CAP header */
        p_buf->offset -= L2CAP_PKT_OVERHEAD;
        p_buf->len    += L2CAP_PKT_OVERHEAD;
        /* Set the pointer to the beginning of the data */
        p = (uint8_t *)(p_buf + 1) + p_buf->offset;
        /* Now the L2CAP header */
        UINT16_TO_STREAM(p, p_buf->len - L2CAP_PKT_OVERHEAD);
        UINT16_TO_STREAM(p, p_ccb->remote_cid);
    }

    if(p_ccb->xmit_hold_q == NULL) {
        L2CAP_TRACE_ERROR("%s: empty queue: p_ccb = %p p_ccb->in_use = %d p_ccb->chnl_state = %d p_ccb->local_cid = %u p_ccb->remote_cid = %u",
                          __func__, p_ccb, p_ccb->in_use, p_ccb->chnl_state,
                          p_ccb->local_cid, p_ccb->remote_cid);
    }

    fixed_queue_enqueue(p_ccb->xmit_hold_q, p_buf);
    l2cu_check_channel_congestion(p_ccb);
#if (L2CAP_ROUND_ROBIN_CHANNEL_SERVICE == TRUE)

    /* if new packet is higher priority than serving ccb and it is not overrun */
    if((p_ccb->p_lcb->rr_pri > p_ccb->ccb_priority)
            && (p_ccb->p_lcb->rr_serv[p_ccb->ccb_priority].quota > 0)) {
        /* send out higher priority packet */
        p_ccb->p_lcb->rr_pri = p_ccb->ccb_priority;
    }

#endif

    /* if we are doing a round robin scheduling, set the flag */
    if(p_ccb->p_lcb->link_xmit_quota == 0) {
        l2cb.check_round_robin = TRUE;
    }
}


