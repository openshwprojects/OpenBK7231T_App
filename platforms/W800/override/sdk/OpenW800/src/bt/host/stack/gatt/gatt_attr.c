/******************************************************************************
 *
 *  Copyright (C) 2008-2012 Broadcom Corporation
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
 *  this file contains the main GATT server attributes access request
 *  handling functions.
 *
 ******************************************************************************/

#include "bt_target.h"
#include "bt_utils.h"

#include "gatt_api.h"
#include "gatt_int.h"

#if BLE_INCLUDED == TRUE

#define GATTP_MAX_NUM_INC_SVR       0
#define GATTP_MAX_CHAR_NUM          2
#define GATTP_MAX_ATTR_NUM          (GATTP_MAX_CHAR_NUM * 2 + GATTP_MAX_NUM_INC_SVR + 1)
#define GATTP_MAX_CHAR_VALUE_SIZE   50

#ifndef GATTP_ATTR_DB_SIZE
#define GATTP_ATTR_DB_SIZE      GATT_DB_MEM_SIZE(GATTP_MAX_NUM_INC_SVR, GATTP_MAX_CHAR_NUM, GATTP_MAX_CHAR_VALUE_SIZE)
#endif

static void gatt_request_cback(uint16_t conn_id, uint32_t trans_id, uint8_t op_code,
                               tGATTS_DATA *p_data);
static void gatt_connect_cback(tGATT_IF gatt_if, BD_ADDR bda, uint16_t conn_id, uint8_t connected,
                               tGATT_DISCONN_REASON reason, tBT_TRANSPORT transport);
static void gatt_disc_res_cback(uint16_t conn_id, tGATT_DISC_TYPE disc_type,
                                tGATT_DISC_RES *p_data);
static void gatt_disc_cmpl_cback(uint16_t conn_id, tGATT_DISC_TYPE disc_type, tGATT_STATUS status);
static void gatt_cl_op_cmpl_cback(uint16_t conn_id, tGATTC_OPTYPE op, tGATT_STATUS status,
                                  tGATT_CL_COMPLETE *p_data);

static void gatt_cl_start_config_ccc(tGATT_PROFILE_CLCB *p_clcb);


static tGATT_CBACK gatt_profile_cback = {
    gatt_connect_cback,
    gatt_cl_op_cmpl_cback,
    gatt_disc_res_cback,
    gatt_disc_cmpl_cback,
    gatt_request_cback,
    NULL,
    NULL
} ;

/*******************************************************************************
**
** Function         gatt_profile_find_conn_id_by_bd_addr
**
** Description      Find the connection ID by remote address
**
** Returns          Connection ID
**
*******************************************************************************/
uint16_t gatt_profile_find_conn_id_by_bd_addr(BD_ADDR remote_bda)
{
    uint16_t conn_id = GATT_INVALID_CONN_ID;
    GATT_GetConnIdIfConnected(gatt_cb.gatt_if, remote_bda, &conn_id, BT_TRANSPORT_LE);
    return conn_id;
}

/*******************************************************************************
**
** Function         gatt_profile_find_clcb_by_conn_id
**
** Description      find clcb by Connection ID
**
** Returns          Pointer to the found link conenction control block.
**
*******************************************************************************/
static tGATT_PROFILE_CLCB *gatt_profile_find_clcb_by_conn_id(uint16_t conn_id)
{
    uint8_t i_clcb;
    tGATT_PROFILE_CLCB    *p_clcb = NULL;

    for(i_clcb = 0, p_clcb = gatt_cb.profile_clcb; i_clcb < GATT_MAX_APPS; i_clcb++, p_clcb++) {
        if(p_clcb->in_use && p_clcb->conn_id == conn_id) {
            return p_clcb;
        }
    }

    return NULL;
}

/*******************************************************************************
**
** Function         gatt_profile_find_clcb_by_bd_addr
**
** Description      The function searches all LCBs with macthing bd address.
**
** Returns          Pointer to the found link conenction control block.
**
*******************************************************************************/
static tGATT_PROFILE_CLCB *gatt_profile_find_clcb_by_bd_addr(BD_ADDR bda, tBT_TRANSPORT transport)
{
    uint8_t i_clcb;
    tGATT_PROFILE_CLCB    *p_clcb = NULL;

    for(i_clcb = 0, p_clcb = gatt_cb.profile_clcb; i_clcb < GATT_MAX_APPS; i_clcb++, p_clcb++) {
        if(p_clcb->in_use && p_clcb->transport == transport &&
                p_clcb->connected && !memcmp(p_clcb->bda, bda, BD_ADDR_LEN)) {
            return p_clcb;
        }
    }

    return NULL;
}

/*******************************************************************************
**
** Function         gatt_profile_clcb_alloc
**
** Description      The function allocates a GATT profile  connection link control block
**
** Returns           NULL if not found. Otherwise pointer to the connection link block.
**
*******************************************************************************/
tGATT_PROFILE_CLCB *gatt_profile_clcb_alloc(uint16_t conn_id, BD_ADDR bda, tBT_TRANSPORT tranport)
{
    uint8_t                   i_clcb = 0;
    tGATT_PROFILE_CLCB      *p_clcb = NULL;

    for(i_clcb = 0, p_clcb = gatt_cb.profile_clcb; i_clcb < GATT_MAX_APPS; i_clcb++, p_clcb++) {
        if(!p_clcb->in_use) {
            p_clcb->in_use      = TRUE;
            p_clcb->conn_id     = conn_id;
            p_clcb->connected   = TRUE;
            p_clcb->transport   = tranport;
            wm_memcpy(p_clcb->bda, bda, BD_ADDR_LEN);
            break;
        }
    }

    if(i_clcb < GATT_MAX_APPS) {
        return p_clcb;
    }

    return NULL;
}

/*******************************************************************************
**
** Function         gatt_profile_clcb_dealloc
**
** Description      The function deallocates a GATT profile  connection link control block
**
** Returns          void
**
*******************************************************************************/
void gatt_profile_clcb_dealloc(tGATT_PROFILE_CLCB *p_clcb)
{
    wm_memset(p_clcb, 0, sizeof(tGATT_PROFILE_CLCB));
}

/*******************************************************************************
**
** Function         gatt_request_cback
**
** Description      GATT profile attribute access request callback.
**
** Returns          void.
**
*******************************************************************************/
static void gatt_request_cback(uint16_t conn_id, uint32_t trans_id, tGATTS_REQ_TYPE type,
                               tGATTS_DATA *p_data)
{
    uint8_t       status = GATT_INVALID_PDU;
    tGATTS_RSP   rsp_msg ;
    uint8_t     ignore = FALSE;
    wm_memset(&rsp_msg, 0, sizeof(tGATTS_RSP));

    switch(type) {
        case GATTS_REQ_TYPE_READ:
            status = GATT_READ_NOT_PERMIT;
            break;

        case GATTS_REQ_TYPE_WRITE:
            status = GATT_WRITE_NOT_PERMIT;
            break;

        case GATTS_REQ_TYPE_WRITE_EXEC:
        case GATT_CMD_WRITE:
            ignore = TRUE;
            GATT_TRACE_EVENT("Ignore GATT_REQ_EXEC_WRITE/WRITE_CMD");
            break;

        case GATTS_REQ_TYPE_MTU:
            GATT_TRACE_EVENT("Get MTU exchange new mtu size: %d", p_data->mtu);
            ignore = TRUE;
            break;

        default:
            GATT_TRACE_EVENT("Unknown/unexpected LE GAP ATT request: 0x%02x", type);
            break;
    }

    if(!ignore) {
        GATTS_SendRsp(conn_id, trans_id, status, &rsp_msg);
    }
}

/*******************************************************************************
**
** Function         gatt_connect_cback
**
** Description      Gatt profile connection callback.
**
** Returns          void
**
*******************************************************************************/
static void gatt_connect_cback(tGATT_IF gatt_if, BD_ADDR bda, uint16_t conn_id,
                               uint8_t connected, tGATT_DISCONN_REASON reason,
                               tBT_TRANSPORT transport)
{
    UNUSED(gatt_if);
    GATT_TRACE_EVENT("%s: from %08x%04x connected:%d conn_id=%d reason = 0x%04x", __func__,
                     (bda[0] << 24) + (bda[1] << 16) + (bda[2] << 8) + bda[3],
                     (bda[4] << 8) + bda[5], connected, conn_id, reason);
    tGATT_PROFILE_CLCB *p_clcb = gatt_profile_find_clcb_by_bd_addr(bda, transport);

    if(connected) {
        if(p_clcb == NULL) {
            p_clcb = gatt_profile_clcb_alloc(conn_id, bda, transport);
        }

        if(p_clcb == NULL) {
            return;
        }

        p_clcb->connected = TRUE;
        p_clcb->ccc_stage = GATT_SVC_CHANGED_SERVICE;
        gatt_cl_start_config_ccc(p_clcb);
    } else {
        if(p_clcb != NULL) {
            gatt_profile_clcb_dealloc(p_clcb);
        }
    }
}

/*******************************************************************************
**
** Function         gatt_profile_db_init
**
** Description      Initializa the GATT profile attribute database.
**
*******************************************************************************/
void gatt_profile_db_init(void)
{
    tBT_UUID          app_uuid = {LEN_UUID_128, {0}};
    tBT_UUID          uuid = {LEN_UUID_16, {UUID_SERVCLASS_GATT_SERVER}};
    uint16_t            service_handle = 0;
    tGATT_STATUS      status;
    /* Fill our internal UUID with a fixed pattern 0x81 */
    wm_memset(&app_uuid.uu.uuid128, 0x81, LEN_UUID_128);
    /* Create a GATT profile service */
    gatt_cb.gatt_if = GATT_Register(&app_uuid, &gatt_profile_cback);
    GATT_StartIf(gatt_cb.gatt_if);
    service_handle = GATTS_CreateService(gatt_cb.gatt_if, &uuid, 0, GATTP_MAX_ATTR_NUM, TRUE);
    /* add Service Changed characteristic
    */
    uuid.uu.uuid16 = gatt_cb.gattp_attr.uuid = GATT_UUID_GATT_SRV_CHGD;
    gatt_cb.gattp_attr.service_change = 0;
    gatt_cb.gattp_attr.handle   =
                    gatt_cb.handle_of_h_r       = GATTS_AddCharacteristic(service_handle, &uuid, 0,
                            GATT_CHAR_PROP_BIT_INDICATE);
    GATT_TRACE_DEBUG("gatt_profile_db_init:  handle of service changed%d",
                     gatt_cb.handle_of_h_r);
    /* start service
    */
    status = GATTS_StartService(gatt_cb.gatt_if, service_handle, GATTP_TRANSPORT_SUPPORTED);
    GATT_TRACE_DEBUG("gatt_profile_db_init:  gatt_if=%d   start status%d",
                     gatt_cb.gatt_if,  status);
}

/*******************************************************************************
**
** Function         gatt_disc_res_cback
**
** Description      Gatt profile discovery result callback
**
** Returns          void
**
*******************************************************************************/
static void gatt_disc_res_cback(uint16_t conn_id, tGATT_DISC_TYPE disc_type, tGATT_DISC_RES *p_data)
{
    tGATT_PROFILE_CLCB *p_clcb = gatt_profile_find_clcb_by_conn_id(conn_id);

    if(p_clcb == NULL) {
        return;
    }

    switch(disc_type) {
        case GATT_DISC_SRVC_BY_UUID:/* stage 1 */
            p_clcb->e_handle = p_data->value.group_value.e_handle;
            p_clcb->ccc_result ++;
            break;

        case GATT_DISC_CHAR:/* stage 2 */
            p_clcb->s_handle = p_data->value.dclr_value.val_handle;
            p_clcb->ccc_result ++;
            break;

        case GATT_DISC_CHAR_DSCPT: /* stage 3 */
            if(p_data->type.uu.uuid16 == GATT_UUID_CHAR_CLIENT_CONFIG) {
                p_clcb->s_handle = p_data->handle;
                p_clcb->ccc_result ++;
            }

            break;
    }
}

/*******************************************************************************
**
** Function         gatt_disc_cmpl_cback
**
** Description      Gatt profile discovery complete callback
**
** Returns          void
**
*******************************************************************************/
static void gatt_disc_cmpl_cback(uint16_t conn_id, tGATT_DISC_TYPE disc_type, tGATT_STATUS status)
{
    tGATT_PROFILE_CLCB *p_clcb = gatt_profile_find_clcb_by_conn_id(conn_id);

    if(p_clcb == NULL) {
        return;
    }

    if(status == GATT_SUCCESS && p_clcb->ccc_result > 0) {
        p_clcb->ccc_result = 0;
        p_clcb->ccc_stage ++;
        gatt_cl_start_config_ccc(p_clcb);
    } else {
        GATT_TRACE_ERROR("%s() - Unable to register for service changed indication", __func__);
    }
}

/*******************************************************************************
**
** Function         gatt_cl_op_cmpl_cback
**
** Description      Gatt profile client operation complete callback
**
** Returns          void
**
*******************************************************************************/
static void gatt_cl_op_cmpl_cback(uint16_t conn_id, tGATTC_OPTYPE op,
                                  tGATT_STATUS status, tGATT_CL_COMPLETE *p_data)
{
    UNUSED(conn_id);
    UNUSED(op);
    UNUSED(status);
    UNUSED(p_data);
}

/*******************************************************************************
**
** Function         gatt_cl_start_config_ccc
**
** Description      Gatt profile start configure service change CCC
**
** Returns          void
**
*******************************************************************************/
static void gatt_cl_start_config_ccc(tGATT_PROFILE_CLCB *p_clcb)
{
    tGATT_DISC_PARAM    srvc_disc_param;
    tGATT_VALUE         ccc_value;
    GATT_TRACE_DEBUG("%s() - stage: %d", __FUNCTION__, p_clcb->ccc_stage);
    wm_memset(&srvc_disc_param, 0, sizeof(tGATT_DISC_PARAM));
    wm_memset(&ccc_value, 0, sizeof(tGATT_VALUE));

    switch(p_clcb->ccc_stage) {
        case GATT_SVC_CHANGED_SERVICE: /* discover GATT service */
            srvc_disc_param.s_handle = 1;
            srvc_disc_param.e_handle = 0xffff;
            srvc_disc_param.service.len = 2;
            srvc_disc_param.service.uu.uuid16 = UUID_SERVCLASS_GATT_SERVER;
            GATTC_Discover(p_clcb->conn_id, GATT_DISC_SRVC_BY_UUID, &srvc_disc_param);
            break;

        case GATT_SVC_CHANGED_CHARACTERISTIC: /* discover service change char */
            srvc_disc_param.s_handle = 1;
            srvc_disc_param.e_handle = p_clcb->e_handle;
            srvc_disc_param.service.len = 2;
            srvc_disc_param.service.uu.uuid16 = GATT_UUID_GATT_SRV_CHGD;
            GATTC_Discover(p_clcb->conn_id, GATT_DISC_CHAR, &srvc_disc_param);
            break;

        case GATT_SVC_CHANGED_DESCRIPTOR: /* discover service change ccc */
            srvc_disc_param.s_handle = p_clcb->s_handle;
            srvc_disc_param.e_handle = p_clcb->e_handle;
            GATTC_Discover(p_clcb->conn_id, GATT_DISC_CHAR_DSCPT, &srvc_disc_param);
            break;

        case GATT_SVC_CHANGED_CONFIGURE_CCCD: /* write ccc */
            ccc_value.handle = p_clcb->s_handle;
            ccc_value.len = 2;
            ccc_value.value[0] = GATT_CLT_CONFIG_INDICATION;
            GATTC_Write(p_clcb->conn_id, GATT_WRITE, &ccc_value);
            break;
    }
}

/*******************************************************************************
**
** Function         GATT_ConfigServiceChangeCCC
**
** Description      Configure service change indication on remote device
**
** Returns          none
**
*******************************************************************************/
void GATT_ConfigServiceChangeCCC(BD_ADDR remote_bda, uint8_t enable, tBT_TRANSPORT transport)
{
    tGATT_PROFILE_CLCB   *p_clcb = gatt_profile_find_clcb_by_bd_addr(remote_bda, transport);

    if(p_clcb == NULL) {
        p_clcb = gatt_profile_clcb_alloc(0, remote_bda, transport);
    }

    if(p_clcb == NULL) {
        return;
    }

    if(GATT_GetConnIdIfConnected(gatt_cb.gatt_if, remote_bda, &p_clcb->conn_id, transport)) {
        p_clcb->connected = TRUE;
    }

    /* hold the link here */
    GATT_Connect(gatt_cb.gatt_if, remote_bda, TRUE, transport);
    p_clcb->ccc_stage = GATT_SVC_CHANGED_CONNECTING;

    if(!p_clcb->connected) {
        /* wait for connection */
        return;
    }

    p_clcb->ccc_stage ++;
    gatt_cl_start_config_ccc(p_clcb);
}

#endif  /* BLE_INCLUDED */
