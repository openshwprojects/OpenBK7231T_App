/******************************************************************************
 *
 *  Copyright (C) 2002-2012 Broadcom Corporation
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
 *  This AVDTP adaption layer module interfaces to L2CAP
 *
 ******************************************************************************/

#include <string.h>
#include "bt_types.h"
#include "bt_target.h"
#include "bt_utils.h"
#include "avdt_api.h"
#include "avdtc_api.h"
#include "avdt_int.h"
#include "l2c_api.h"
#include "l2cdefs.h"
#include "btm_api.h"
#include "btm_int.h"
//#include "device/include/interop.h"
// Some headsets have audio jitter issues because of increased re-transmissions as the
// 3 Mbps packets have a lower link margin, and are more prone to interference. We can
// disable 3DH packets (use only 2DH packets) for the ACL link to improve sensitivity
// when streaming A2DP audio to the headset. Air sniffer logs show reduced
// re-transmissions after switching to 2DH packets.
//
// Disable 3Mbps packets and use only 2Mbps packets for ACL links when streaming audio.
//INTEROP_2MBPS_LINK_ONLY

/* callback function declarations */
void avdt_l2c_connect_ind_cback(BD_ADDR bd_addr, uint16_t lcid, uint16_t psm, uint8_t id);
void avdt_l2c_connect_cfm_cback(uint16_t lcid, uint16_t result);
void avdt_l2c_config_cfm_cback(uint16_t lcid, tL2CAP_CFG_INFO *p_cfg);
void avdt_l2c_config_ind_cback(uint16_t lcid, tL2CAP_CFG_INFO *p_cfg);
void avdt_l2c_disconnect_ind_cback(uint16_t lcid, uint8_t ack_needed);
void avdt_l2c_disconnect_cfm_cback(uint16_t lcid, uint16_t result);
void avdt_l2c_congestion_ind_cback(uint16_t lcid, uint8_t is_congested);
void avdt_l2c_data_ind_cback(uint16_t lcid, BT_HDR *p_buf);

/* L2CAP callback function structure */
const tL2CAP_APPL_INFO avdt_l2c_appl = {
    avdt_l2c_connect_ind_cback,
    avdt_l2c_connect_cfm_cback,
    NULL,
    avdt_l2c_config_ind_cback,
    avdt_l2c_config_cfm_cback,
    avdt_l2c_disconnect_ind_cback,
    avdt_l2c_disconnect_cfm_cback,
    NULL,
    avdt_l2c_data_ind_cback,
    avdt_l2c_congestion_ind_cback,
    NULL                /* tL2CA_TX_COMPLETE_CB */
};

/*******************************************************************************
**
** Function         avdt_sec_check_complete_term
**
** Description      The function called when Security Manager finishes
**                  verification of the service side connection
**
** Returns          void
**
*******************************************************************************/
static void avdt_sec_check_complete_term(BD_ADDR bd_addr, tBT_TRANSPORT transport,
        void *p_ref_data, uint8_t res)
{
    tAVDT_CCB       *p_ccb = NULL;
    tL2CAP_CFG_INFO cfg;
    tAVDT_TC_TBL    *p_tbl;
    UNUSED(p_ref_data);
    AVDT_TRACE_DEBUG("avdt_sec_check_complete_term res: %d", res);

    if(!bd_addr) {
        AVDT_TRACE_WARNING("avdt_sec_check_complete_term: NULL BD_ADDR");
        return;
    }

    p_ccb = avdt_ccb_by_bd(bd_addr);
    p_tbl = avdt_ad_tc_tbl_by_st(AVDT_CHAN_SIG, p_ccb, AVDT_AD_ST_SEC_ACP);

    if(p_tbl == NULL) {
        return;
    }

    if(res == BTM_SUCCESS) {
        /* Send response to the L2CAP layer. */
        L2CA_ConnectRsp(bd_addr, p_tbl->id, p_tbl->lcid, L2CAP_CONN_OK, L2CAP_CONN_OK);
        /* store idx in LCID table, store LCID in routing table */
        avdt_cb.ad.lcid_tbl[p_tbl->lcid - L2CAP_BASE_APPL_CID] = avdt_ad_tc_tbl_to_idx(p_tbl);
        avdt_cb.ad.rt_tbl[avdt_ccb_to_idx(p_ccb)][p_tbl->tcid].lcid = p_tbl->lcid;
        /* transition to configuration state */
        p_tbl->state = AVDT_AD_ST_CFG;
        /* Send L2CAP config req */
        wm_memset(&cfg, 0, sizeof(tL2CAP_CFG_INFO));
        cfg.mtu_present = TRUE;
        cfg.mtu = p_tbl->my_mtu;
        cfg.flush_to_present = TRUE;
        cfg.flush_to = p_tbl->my_flush_to;
        L2CA_ConfigReq(p_tbl->lcid, &cfg);
    } else {
        L2CA_ConnectRsp(bd_addr, p_tbl->id, p_tbl->lcid, L2CAP_CONN_SECURITY_BLOCK, L2CAP_CONN_OK);
        avdt_ad_tc_close_ind(p_tbl, L2CAP_CONN_SECURITY_BLOCK);
    }
}

/*******************************************************************************
**
** Function         avdt_sec_check_complete_orig
**
** Description      The function called when Security Manager finishes
**                  verification of the service side connection
**
** Returns          void
**
*******************************************************************************/
static void avdt_sec_check_complete_orig(BD_ADDR bd_addr, tBT_TRANSPORT trasnport,
        void *p_ref_data, uint8_t res)
{
    tAVDT_CCB       *p_ccb = NULL;
    tL2CAP_CFG_INFO cfg;
    tAVDT_TC_TBL    *p_tbl;
    UNUSED(p_ref_data);
    AVDT_TRACE_DEBUG("avdt_sec_check_complete_orig res: %d", res);

    if(bd_addr) {
        p_ccb = avdt_ccb_by_bd(bd_addr);
    }

    p_tbl = avdt_ad_tc_tbl_by_st(AVDT_CHAN_SIG, p_ccb, AVDT_AD_ST_SEC_INT);

    if(p_tbl == NULL) {
        return;
    }

    if(res == BTM_SUCCESS) {
        /* set channel state */
        p_tbl->state = AVDT_AD_ST_CFG;
        /* Send L2CAP config req */
        wm_memset(&cfg, 0, sizeof(tL2CAP_CFG_INFO));
        cfg.mtu_present = TRUE;
        cfg.mtu = p_tbl->my_mtu;
        cfg.flush_to_present = TRUE;
        cfg.flush_to = p_tbl->my_flush_to;
        L2CA_ConfigReq(p_tbl->lcid, &cfg);
    } else {
        L2CA_DisconnectReq(p_tbl->lcid);
        avdt_ad_tc_close_ind(p_tbl, L2CAP_CONN_SECURITY_BLOCK);
    }
}
/*******************************************************************************
**
** Function         avdt_l2c_connect_ind_cback
**
** Description      This is the L2CAP connect indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
void avdt_l2c_connect_ind_cback(BD_ADDR bd_addr, uint16_t lcid, uint16_t psm, uint8_t id)
{
    tAVDT_CCB       *p_ccb;
    tAVDT_TC_TBL    *p_tbl = NULL;
    uint16_t          result;
    tL2CAP_CFG_INFO cfg;
    tBTM_STATUS rc;
    UNUSED(psm);

    /* do we already have a control channel for this peer? */
    if((p_ccb = avdt_ccb_by_bd(bd_addr)) == NULL) {
        /* no, allocate ccb */
        if((p_ccb = avdt_ccb_alloc(bd_addr)) == NULL) {
            /* no ccb available, reject L2CAP connection */
            result = L2CAP_CONN_NO_RESOURCES;
        } else {
            /* allocate and set up entry; first channel is always signaling */
            p_tbl = avdt_ad_tc_tbl_alloc(p_ccb);
            p_tbl->my_mtu = avdt_cb.rcb.ctrl_mtu;
            p_tbl->my_flush_to = L2CAP_DEFAULT_FLUSH_TO;
            p_tbl->tcid = AVDT_CHAN_SIG;
            p_tbl->lcid = lcid;
            p_tbl->id   = id;
            p_tbl->state = AVDT_AD_ST_SEC_ACP;
            p_tbl->cfg_flags = AVDT_L2C_CFG_CONN_ACP;
#if 0

            if(interop_match_addr(INTEROP_2MBPS_LINK_ONLY, (const bt_bdaddr_t *)&bd_addr)) {
                // Disable 3DH packets for AVDT ACL to improve sensitivity on HS
                tACL_CONN *p_acl_cb = btm_bda_to_acl(bd_addr, BT_TRANSPORT_BR_EDR);
                btm_set_packet_types(p_acl_cb, (btm_cb.btm_acl_pkt_types_supported |
                                                HCI_PKT_TYPES_MASK_NO_3_DH1 |
                                                HCI_PKT_TYPES_MASK_NO_3_DH3 |
                                                HCI_PKT_TYPES_MASK_NO_3_DH5));
            }

#endif
            /* Check the security */
            rc = btm_sec_mx_access_request(bd_addr, AVDT_PSM,
                                           FALSE, BTM_SEC_PROTO_AVDT,
                                           AVDT_CHAN_SIG,
                                           &avdt_sec_check_complete_term, NULL);

            if(rc == BTM_CMD_STARTED) {
                L2CA_ConnectRsp(p_ccb->peer_addr, p_tbl->id, lcid, L2CAP_CONN_PENDING, L2CAP_CONN_OK);
            }

            return;
        }
    }
    /* deal with simultaneous control channel connect case */
    else if((p_tbl = avdt_ad_tc_tbl_by_st(AVDT_CHAN_SIG, p_ccb, AVDT_AD_ST_CONN)) != NULL) {
        /* reject their connection */
        result = L2CAP_CONN_NO_RESOURCES;
    }
    /* this must be a traffic channel; are we accepting a traffic channel
    ** for this ccb?
    */
    else if((p_tbl = avdt_ad_tc_tbl_by_st(AVDT_CHAN_MEDIA, p_ccb, AVDT_AD_ST_ACP)) != NULL) {
        /* yes; proceed with connection */
        result = L2CAP_CONN_OK;
    }

#if AVDT_REPORTING == TRUE
    /* this must be a reporting channel; are we accepting a reporting channel
    ** for this ccb?
    */
    else if((p_tbl = avdt_ad_tc_tbl_by_st(AVDT_CHAN_REPORT, p_ccb, AVDT_AD_ST_ACP)) != NULL) {
        /* yes; proceed with connection */
        result = L2CAP_CONN_OK;
    }

#endif
    /* else we're not listening for traffic channel; reject */
    else {
        result = L2CAP_CONN_NO_PSM;
    }

    /* Send L2CAP connect rsp */
    L2CA_ConnectRsp(bd_addr, id, lcid, result, 0);

    /* if result ok, proceed with connection */
    if(result == L2CAP_CONN_OK) {
        /* store idx in LCID table, store LCID in routing table */
        avdt_cb.ad.lcid_tbl[lcid - L2CAP_BASE_APPL_CID] = avdt_ad_tc_tbl_to_idx(p_tbl);
        avdt_cb.ad.rt_tbl[avdt_ccb_to_idx(p_ccb)][p_tbl->tcid].lcid = lcid;
        /* transition to configuration state */
        p_tbl->state = AVDT_AD_ST_CFG;
        /* Send L2CAP config req */
        wm_memset(&cfg, 0, sizeof(tL2CAP_CFG_INFO));
        cfg.mtu_present = TRUE;
        cfg.mtu = p_tbl->my_mtu;
        cfg.flush_to_present = TRUE;
        cfg.flush_to = p_tbl->my_flush_to;
        L2CA_ConfigReq(lcid, &cfg);
    }
}

/*******************************************************************************
**
** Function         avdt_l2c_connect_cfm_cback
**
** Description      This is the L2CAP connect confirm callback function.
**
**
** Returns          void
**
*******************************************************************************/
void avdt_l2c_connect_cfm_cback(uint16_t lcid, uint16_t result)
{
    tAVDT_TC_TBL    *p_tbl;
    tL2CAP_CFG_INFO cfg;
    tAVDT_CCB *p_ccb;
    AVDT_TRACE_DEBUG("avdt_l2c_connect_cfm_cback lcid: %d, result: %d",
                     lcid, result);

    /* look up info for this channel */
    if((p_tbl = avdt_ad_tc_tbl_by_lcid(lcid)) != NULL) {
        /* if in correct state */
        if(p_tbl->state == AVDT_AD_ST_CONN) {
            /* if result successful */
            if(result == L2CAP_CONN_OK) {
                if(p_tbl->tcid != AVDT_CHAN_SIG) {
                    /* set channel state */
                    p_tbl->state = AVDT_AD_ST_CFG;
                    /* Send L2CAP config req */
                    wm_memset(&cfg, 0, sizeof(tL2CAP_CFG_INFO));
                    cfg.mtu_present = TRUE;
                    cfg.mtu = p_tbl->my_mtu;
                    cfg.flush_to_present = TRUE;
                    cfg.flush_to = p_tbl->my_flush_to;
                    L2CA_ConfigReq(lcid, &cfg);
                } else {
                    p_ccb = avdt_ccb_by_idx(p_tbl->ccb_idx);

                    if(p_ccb == NULL) {
                        result = L2CAP_CONN_NO_RESOURCES;
                    } else {
                        /* set channel state */
                        p_tbl->state = AVDT_AD_ST_SEC_INT;
                        p_tbl->lcid = lcid;
                        p_tbl->cfg_flags = AVDT_L2C_CFG_CONN_INT;
#if 0

                        if(interop_match_addr(INTEROP_2MBPS_LINK_ONLY, (const bt_bdaddr_t *) &p_ccb->peer_addr)) {
                            // Disable 3DH packets for AVDT ACL to improve sensitivity on HS
                            tACL_CONN *p_acl_cb = btm_bda_to_acl(p_ccb->peer_addr, BT_TRANSPORT_BR_EDR);
                            btm_set_packet_types(p_acl_cb, (btm_cb.btm_acl_pkt_types_supported |
                                                            HCI_PKT_TYPES_MASK_NO_3_DH1 |
                                                            HCI_PKT_TYPES_MASK_NO_3_DH3 |
                                                            HCI_PKT_TYPES_MASK_NO_3_DH5));
                        }

#endif
                        /* Check the security */
                        btm_sec_mx_access_request(p_ccb->peer_addr, AVDT_PSM,
                                                  TRUE, BTM_SEC_PROTO_AVDT,
                                                  AVDT_CHAN_SIG,
                                                  &avdt_sec_check_complete_orig, NULL);
                    }
                }
            }

            /* failure; notify adaption that channel closed */
            if(result != L2CAP_CONN_OK) {
                avdt_ad_tc_close_ind(p_tbl, result);
            }
        }
    }
}

/*******************************************************************************
**
** Function         avdt_l2c_config_cfm_cback
**
** Description      This is the L2CAP config confirm callback function.
**
**
** Returns          void
**
*******************************************************************************/
void avdt_l2c_config_cfm_cback(uint16_t lcid, tL2CAP_CFG_INFO *p_cfg)
{
    tAVDT_TC_TBL    *p_tbl;

    /* look up info for this channel */
    if((p_tbl = avdt_ad_tc_tbl_by_lcid(lcid)) != NULL) {
        p_tbl->lcid = lcid;

        /* if in correct state */
        if(p_tbl->state == AVDT_AD_ST_CFG) {
            /* if result successful */
            if(p_cfg->result == L2CAP_CONN_OK) {
                /* update cfg_flags */
                p_tbl->cfg_flags |= AVDT_L2C_CFG_CFM_DONE;

                /* if configuration complete */
                if(p_tbl->cfg_flags & AVDT_L2C_CFG_IND_DONE) {
                    avdt_ad_tc_open_ind(p_tbl);
                }
            }
            /* else failure */
            else {
                /* Send L2CAP disconnect req */
                L2CA_DisconnectReq(lcid);
            }
        }
    }
}

/*******************************************************************************
**
** Function         avdt_l2c_config_ind_cback
**
** Description      This is the L2CAP config indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
void avdt_l2c_config_ind_cback(uint16_t lcid, tL2CAP_CFG_INFO *p_cfg)
{
    tAVDT_TC_TBL    *p_tbl;

    /* look up info for this channel */
    if((p_tbl = avdt_ad_tc_tbl_by_lcid(lcid)) != NULL) {
        /* store the mtu in tbl */
        if(p_cfg->mtu_present) {
            p_tbl->peer_mtu = p_cfg->mtu;
        } else {
            p_tbl->peer_mtu = L2CAP_DEFAULT_MTU;
        }

        AVDT_TRACE_DEBUG("peer_mtu: %d, lcid: x%x", p_tbl->peer_mtu, lcid);
        /* send L2CAP configure response */
        wm_memset(p_cfg, 0, sizeof(tL2CAP_CFG_INFO));
        p_cfg->result = L2CAP_CFG_OK;
        L2CA_ConfigRsp(lcid, p_cfg);

        /* if first config ind */
        if((p_tbl->cfg_flags & AVDT_L2C_CFG_IND_DONE) == 0) {
            /* update cfg_flags */
            p_tbl->cfg_flags |= AVDT_L2C_CFG_IND_DONE;

            /* if configuration complete */
            if(p_tbl->cfg_flags & AVDT_L2C_CFG_CFM_DONE) {
                avdt_ad_tc_open_ind(p_tbl);
            }
        }
    }
}

/*******************************************************************************
**
** Function         avdt_l2c_disconnect_ind_cback
**
** Description      This is the L2CAP disconnect indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
void avdt_l2c_disconnect_ind_cback(uint16_t lcid, uint8_t ack_needed)
{
    tAVDT_TC_TBL    *p_tbl;
    AVDT_TRACE_DEBUG("avdt_l2c_disconnect_ind_cback lcid: %d, ack_needed: %d",
                     lcid, ack_needed);

    /* look up info for this channel */
    if((p_tbl = avdt_ad_tc_tbl_by_lcid(lcid)) != NULL) {
        if(ack_needed) {
            /* send L2CAP disconnect response */
            L2CA_DisconnectRsp(lcid);
        }

        avdt_ad_tc_close_ind(p_tbl, 0);
    }
}

/*******************************************************************************
**
** Function         avdt_l2c_disconnect_cfm_cback
**
** Description      This is the L2CAP disconnect confirm callback function.
**
**
** Returns          void
**
*******************************************************************************/
void avdt_l2c_disconnect_cfm_cback(uint16_t lcid, uint16_t result)
{
    tAVDT_TC_TBL    *p_tbl;
    AVDT_TRACE_DEBUG("avdt_l2c_disconnect_cfm_cback lcid: %d, result: %d",
                     lcid, result);

    /* look up info for this channel */
    if((p_tbl = avdt_ad_tc_tbl_by_lcid(lcid)) != NULL) {
        avdt_ad_tc_close_ind(p_tbl, result);
    }
}

/*******************************************************************************
**
** Function         avdt_l2c_congestion_ind_cback
**
** Description      This is the L2CAP congestion indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
void avdt_l2c_congestion_ind_cback(uint16_t lcid, uint8_t is_congested)
{
    tAVDT_TC_TBL    *p_tbl;

    /* look up info for this channel */
    if((p_tbl = avdt_ad_tc_tbl_by_lcid(lcid)) != NULL) {
        avdt_ad_tc_cong_ind(p_tbl, is_congested);
    }
}

/*******************************************************************************
**
** Function         avdt_l2c_data_ind_cback
**
** Description      This is the L2CAP data indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
void avdt_l2c_data_ind_cback(uint16_t lcid, BT_HDR *p_buf)
{
    tAVDT_TC_TBL    *p_tbl;

    /* look up info for this channel */
    if((p_tbl = avdt_ad_tc_tbl_by_lcid(lcid)) != NULL) {
        avdt_ad_tc_data_ind(p_tbl, p_buf);
    } else { /* prevent buffer leak */
        GKI_freebuf(p_buf);
    }
}

