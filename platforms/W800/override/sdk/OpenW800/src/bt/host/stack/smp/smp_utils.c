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
 *  This file contains functions for the SMP L2CAP utility functions
 *
 ******************************************************************************/
#include "bt_target.h"

#if SMP_INCLUDED == TRUE

#include "bt_types.h"
#include "bt_utils.h"
#include <string.h>
#include <ctype.h>
#include "hcidefs.h"
#include "btm_ble_api.h"
#include "l2c_api.h"
#include "l2c_int.h"
#include "smp_int.h"
//#include "device/include/controller.h"
#include "btm_int.h"


extern fixed_queue_t *btu_general_alarm_queue;

#define SMP_PAIRING_REQ_SIZE    7
#define SMP_CONFIRM_CMD_SIZE    (BT_OCTET16_LEN + 1)
#define SMP_RAND_CMD_SIZE       (BT_OCTET16_LEN + 1)
#define SMP_INIT_CMD_SIZE       (BT_OCTET16_LEN + 1)
#define SMP_ENC_INFO_SIZE       (BT_OCTET16_LEN + 1)
#define SMP_MASTER_ID_SIZE      (BT_OCTET8_LEN + 2 + 1)
#define SMP_ID_INFO_SIZE        (BT_OCTET16_LEN + 1)
#define SMP_ID_ADDR_SIZE        (BD_ADDR_LEN + 1 + 1)
#define SMP_SIGN_INFO_SIZE      (BT_OCTET16_LEN + 1)
#define SMP_PAIR_FAIL_SIZE      2
#define SMP_SECURITY_REQUEST_SIZE  2
#define SMP_PAIR_PUBL_KEY_SIZE  (1 /* opcode */ + (2*BT_OCTET32_LEN))
#define SMP_PAIR_COMMITM_SIZE           (1 /* opcode */ + BT_OCTET16_LEN /*Commitment*/)
#define SMP_PAIR_DHKEY_CHECK_SIZE       (1 /* opcode */ + BT_OCTET16_LEN /*DHKey Check*/)
#define SMP_PAIR_KEYPR_NOTIF_SIZE       (1 /* opcode */ + 1 /*Notif Type*/)

/* SMP command sizes per spec */
static const uint8_t smp_cmd_size_per_spec[] = {
    0,
    SMP_PAIRING_REQ_SIZE,       /* 0x01: pairing request */
    SMP_PAIRING_REQ_SIZE,       /* 0x02: pairing response */
    SMP_CONFIRM_CMD_SIZE,       /* 0x03: pairing confirm */
    SMP_RAND_CMD_SIZE,          /* 0x04: pairing random */
    SMP_PAIR_FAIL_SIZE,         /* 0x05: pairing failed */
    SMP_ENC_INFO_SIZE,          /* 0x06: encryption information */
    SMP_MASTER_ID_SIZE,         /* 0x07: master identification */
    SMP_ID_INFO_SIZE,           /* 0x08: identity information */
    SMP_ID_ADDR_SIZE,           /* 0x09: identity address information */
    SMP_SIGN_INFO_SIZE,         /* 0x0A: signing information */
    SMP_SECURITY_REQUEST_SIZE,  /* 0x0B: security request */
    SMP_PAIR_PUBL_KEY_SIZE,     /* 0x0C: pairing public key */
    SMP_PAIR_DHKEY_CHECK_SIZE,  /* 0x0D: pairing dhkey check */
    SMP_PAIR_KEYPR_NOTIF_SIZE,  /* 0x0E: pairing keypress notification */
    SMP_PAIR_COMMITM_SIZE       /* 0x0F: pairing commitment */
};

static uint8_t smp_parameter_unconditionally_valid(tSMP_CB *p_cb);
static uint8_t smp_parameter_unconditionally_invalid(tSMP_CB *p_cb);

/* type for SMP command length validation functions */
typedef uint8_t (*tSMP_CMD_LEN_VALID)(tSMP_CB *p_cb);

static uint8_t smp_command_has_valid_fixed_length(tSMP_CB *p_cb);

static const tSMP_CMD_LEN_VALID smp_cmd_len_is_valid[] = {
    smp_parameter_unconditionally_invalid,
    smp_command_has_valid_fixed_length, /* 0x01: pairing request */
    smp_command_has_valid_fixed_length, /* 0x02: pairing response */
    smp_command_has_valid_fixed_length, /* 0x03: pairing confirm */
    smp_command_has_valid_fixed_length, /* 0x04: pairing random */
    smp_command_has_valid_fixed_length, /* 0x05: pairing failed */
    smp_command_has_valid_fixed_length, /* 0x06: encryption information */
    smp_command_has_valid_fixed_length, /* 0x07: master identification */
    smp_command_has_valid_fixed_length, /* 0x08: identity information */
    smp_command_has_valid_fixed_length, /* 0x09: identity address information */
    smp_command_has_valid_fixed_length, /* 0x0A: signing information */
    smp_command_has_valid_fixed_length, /* 0x0B: security request */
    smp_command_has_valid_fixed_length, /* 0x0C: pairing public key */
    smp_command_has_valid_fixed_length, /* 0x0D: pairing dhkey check */
    smp_command_has_valid_fixed_length, /* 0x0E: pairing keypress notification */
    smp_command_has_valid_fixed_length  /* 0x0F: pairing commitment */
};

/* type for SMP command parameter ranges validation functions */
typedef uint8_t (*tSMP_CMD_PARAM_RANGES_VALID)(tSMP_CB *p_cb);

static uint8_t smp_pairing_request_response_parameters_are_valid(tSMP_CB *p_cb);
static uint8_t smp_pairing_keypress_notification_is_valid(tSMP_CB *p_cb);

static const tSMP_CMD_PARAM_RANGES_VALID smp_cmd_param_ranges_are_valid[] = {
    smp_parameter_unconditionally_invalid,
    smp_pairing_request_response_parameters_are_valid, /* 0x01: pairing request */
    smp_pairing_request_response_parameters_are_valid, /* 0x02: pairing response */
    smp_parameter_unconditionally_valid, /* 0x03: pairing confirm */
    smp_parameter_unconditionally_valid, /* 0x04: pairing random */
    smp_parameter_unconditionally_valid, /* 0x05: pairing failed */
    smp_parameter_unconditionally_valid, /* 0x06: encryption information */
    smp_parameter_unconditionally_valid, /* 0x07: master identification */
    smp_parameter_unconditionally_valid, /* 0x08: identity information */
    smp_parameter_unconditionally_valid, /* 0x09: identity address information */
    smp_parameter_unconditionally_valid, /* 0x0A: signing information */
    smp_parameter_unconditionally_valid, /* 0x0B: security request */
    smp_parameter_unconditionally_valid, /* 0x0C: pairing public key */
    smp_parameter_unconditionally_valid, /* 0x0D: pairing dhkey check */
    smp_pairing_keypress_notification_is_valid, /* 0x0E: pairing keypress notification */
    smp_parameter_unconditionally_valid /* 0x0F: pairing commitment */
};

/* type for action functions */
typedef BT_HDR *(*tSMP_CMD_ACT)(uint8_t cmd_code, tSMP_CB *p_cb);

static BT_HDR *smp_build_pairing_cmd(uint8_t cmd_code, tSMP_CB *p_cb);
static BT_HDR *smp_build_confirm_cmd(uint8_t cmd_code, tSMP_CB *p_cb);
static BT_HDR *smp_build_rand_cmd(uint8_t cmd_code, tSMP_CB *p_cb);
static BT_HDR *smp_build_pairing_fail(uint8_t cmd_code, tSMP_CB *p_cb);
static BT_HDR *smp_build_identity_info_cmd(uint8_t cmd_code, tSMP_CB *p_cb);
static BT_HDR *smp_build_encrypt_info_cmd(uint8_t cmd_code, tSMP_CB *p_cb);
static BT_HDR *smp_build_security_request(uint8_t cmd_code, tSMP_CB *p_cb);
static BT_HDR *smp_build_signing_info_cmd(uint8_t cmd_code, tSMP_CB *p_cb);
static BT_HDR *smp_build_master_id_cmd(uint8_t cmd_code, tSMP_CB *p_cb);
static BT_HDR *smp_build_id_addr_cmd(uint8_t cmd_code, tSMP_CB *p_cb);
static BT_HDR *smp_build_pair_public_key_cmd(uint8_t cmd_code, tSMP_CB *p_cb);
static BT_HDR *smp_build_pairing_commitment_cmd(uint8_t cmd_code, tSMP_CB *p_cb);
static BT_HDR *smp_build_pair_dhkey_check_cmd(uint8_t cmd_code, tSMP_CB *p_cb);
static BT_HDR *smp_build_pairing_keypress_notification_cmd(uint8_t cmd_code, tSMP_CB *p_cb);

static const tSMP_CMD_ACT smp_cmd_build_act[] = {
    NULL,
    smp_build_pairing_cmd,          /* 0x01: pairing request */
    smp_build_pairing_cmd,          /* 0x02: pairing response */
    smp_build_confirm_cmd,          /* 0x03: pairing confirm */
    smp_build_rand_cmd,             /* 0x04: pairing random */
    smp_build_pairing_fail,         /* 0x05: pairing failure */
    smp_build_encrypt_info_cmd,     /* 0x06: encryption information */
    smp_build_master_id_cmd,        /* 0x07: master identification */
    smp_build_identity_info_cmd,    /* 0x08: identity information */
    smp_build_id_addr_cmd,          /* 0x09: identity address information */
    smp_build_signing_info_cmd,     /* 0x0A: signing information */
    smp_build_security_request,     /* 0x0B: security request */
    smp_build_pair_public_key_cmd,  /* 0x0C: pairing public key */
    smp_build_pair_dhkey_check_cmd, /* 0x0D: pairing DHKey check */
    smp_build_pairing_keypress_notification_cmd, /* 0x0E: pairing keypress notification */
    smp_build_pairing_commitment_cmd /* 0x0F: pairing commitment */
};

static const uint8_t smp_association_table[2][SMP_IO_CAP_MAX][SMP_IO_CAP_MAX] = {
    /* display only */    /* Display Yes/No */   /* keyboard only */
    /* No Input/Output */ /* keyboard display */

    /* initiator */
    /* model = tbl[peer_io_caps][loc_io_caps] */
    /* Display Only */
    {   {
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_PASSKEY,
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_PASSKEY
        },

        /* Display Yes/No */
        {
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_PASSKEY,
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_PASSKEY
        },

        /* Keyboard only */
        {
            SMP_MODEL_KEY_NOTIF, SMP_MODEL_KEY_NOTIF, SMP_MODEL_PASSKEY,
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_KEY_NOTIF
        },

        /* No Input No Output */
        {
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_ENCRYPTION_ONLY,
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_ENCRYPTION_ONLY
        },

        /* keyboard display */
        {
            SMP_MODEL_KEY_NOTIF, SMP_MODEL_KEY_NOTIF, SMP_MODEL_PASSKEY,
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_KEY_NOTIF
        }
    },

    /* responder */
    /* model = tbl[loc_io_caps][peer_io_caps] */
    /* Display Only */
    {   {
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_KEY_NOTIF,
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_KEY_NOTIF
        },

        /* Display Yes/No */
        {
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_KEY_NOTIF,
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_KEY_NOTIF
        },

        /* keyboard only */
        {
            SMP_MODEL_PASSKEY, SMP_MODEL_PASSKEY, SMP_MODEL_PASSKEY,
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_PASSKEY
        },

        /* No Input No Output */
        {
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_ENCRYPTION_ONLY,
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_ENCRYPTION_ONLY
        },

        /* keyboard display */
        {
            SMP_MODEL_PASSKEY, SMP_MODEL_PASSKEY, SMP_MODEL_KEY_NOTIF,
            SMP_MODEL_ENCRYPTION_ONLY, SMP_MODEL_PASSKEY
        }
    }
};

static const uint8_t smp_association_table_sc[2][SMP_IO_CAP_MAX][SMP_IO_CAP_MAX] = {
    /* display only */    /* Display Yes/No */   /* keyboard only */
    /* No InputOutput */  /* keyboard display */

    /* initiator */
    /* model = tbl[peer_io_caps][loc_io_caps] */

    /* Display Only */
    {   {
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_PASSKEY_ENT,
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_PASSKEY_ENT
        },

        /* Display Yes/No */
        {
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_NUM_COMP, SMP_MODEL_SEC_CONN_PASSKEY_ENT,
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_NUM_COMP
        },

        /* keyboard only */
        {
            SMP_MODEL_SEC_CONN_PASSKEY_DISP, SMP_MODEL_SEC_CONN_PASSKEY_DISP, SMP_MODEL_SEC_CONN_PASSKEY_ENT,
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_PASSKEY_DISP
        },

        /* No Input No Output */
        {
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_JUSTWORKS,
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_JUSTWORKS
        },

        /* keyboard display */
        {
            SMP_MODEL_SEC_CONN_PASSKEY_DISP, SMP_MODEL_SEC_CONN_NUM_COMP, SMP_MODEL_SEC_CONN_PASSKEY_ENT,
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_NUM_COMP
        }
    },

    /* responder */
    /* model = tbl[loc_io_caps][peer_io_caps] */

    /* Display Only */
    {   {
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_PASSKEY_DISP,
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_PASSKEY_DISP
        },

        /* Display Yes/No */
        {
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_NUM_COMP, SMP_MODEL_SEC_CONN_PASSKEY_DISP,
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_NUM_COMP
        },

        /* keyboard only */
        {
            SMP_MODEL_SEC_CONN_PASSKEY_ENT, SMP_MODEL_SEC_CONN_PASSKEY_ENT, SMP_MODEL_SEC_CONN_PASSKEY_ENT,
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_PASSKEY_ENT
        },

        /* No Input No Output */
        {
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_JUSTWORKS,
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_JUSTWORKS
        },

        /* keyboard display */
        {
            SMP_MODEL_SEC_CONN_PASSKEY_ENT, SMP_MODEL_SEC_CONN_NUM_COMP, SMP_MODEL_SEC_CONN_PASSKEY_DISP,
            SMP_MODEL_SEC_CONN_JUSTWORKS, SMP_MODEL_SEC_CONN_NUM_COMP
        }
    }
};

static tSMP_ASSO_MODEL smp_select_legacy_association_model(tSMP_CB *p_cb);
static tSMP_ASSO_MODEL smp_select_association_model_secure_connections(tSMP_CB *p_cb);

/*******************************************************************************
**
** Function         smp_send_msg_to_L2CAP
**
** Description      Send message to L2CAP.
**
*******************************************************************************/
uint8_t  smp_send_msg_to_L2CAP(BD_ADDR rem_bda, BT_HDR *p_toL2CAP)
{
    uint16_t l2cap_ret;
    uint16_t fixed_cid = L2CAP_SMP_CID;

    if(smp_cb.smp_over_br) {
        fixed_cid = L2CAP_SMP_BR_CID;
    }

    SMP_TRACE_EVENT("%s", __FUNCTION__);
    smp_cb.total_tx_unacked += 1;

    if((l2cap_ret = L2CA_SendFixedChnlData(fixed_cid, rem_bda, p_toL2CAP)) == L2CAP_DW_FAILED) {
        smp_cb.total_tx_unacked -= 1;
        SMP_TRACE_ERROR("SMP   failed to pass msg:0x%0x to L2CAP",
                        *((uint8_t *)(p_toL2CAP + 1) + p_toL2CAP->offset));
        return FALSE;
    } else {
        return TRUE;
    }
}


#ifndef CASE_RETURN_STR
#define CASE_RETURN_STR(const) case const: return #const;
#endif


#if SMP_OPCODE_DEBUG == 1

static char *smp_code_2_str(uint8_t event)
{
    switch(event) {
            CASE_RETURN_STR(SMP_OPCODE_PAIRING_REQ)
            CASE_RETURN_STR(SMP_OPCODE_PAIRING_RSP)
            CASE_RETURN_STR(SMP_OPCODE_CONFIRM)
            CASE_RETURN_STR(SMP_OPCODE_RAND)
            CASE_RETURN_STR(SMP_OPCODE_PAIRING_FAILED)
            CASE_RETURN_STR(SMP_OPCODE_ENCRYPT_INFO)
            CASE_RETURN_STR(SMP_OPCODE_MASTER_ID)
            CASE_RETURN_STR(SMP_OPCODE_IDENTITY_INFO)
            CASE_RETURN_STR(SMP_OPCODE_ID_ADDR)
            CASE_RETURN_STR(SMP_OPCODE_SIGN_INFO)
            CASE_RETURN_STR(SMP_OPCODE_SEC_REQ)
            CASE_RETURN_STR(SMP_OPCODE_PAIR_PUBLIC_KEY)
            CASE_RETURN_STR(SMP_OPCODE_PAIR_DHKEY_CHECK)
            CASE_RETURN_STR(SMP_OPCODE_PAIR_KEYPR_NOTIF)
            CASE_RETURN_STR(SMP_OPCODE_PAIR_COMMITM)

        default:
            return "Unknown smp opcde";
    }
}

#endif


/*******************************************************************************
**
** Function         smp_send_cmd
**
** Description      send a SMP command on L2CAP channel.
**
*******************************************************************************/
uint8_t smp_send_cmd(uint8_t cmd_code, tSMP_CB *p_cb)
{
    BT_HDR *p_buf;
    uint8_t sent = FALSE;
    uint8_t failure = SMP_PAIR_INTERNAL_ERR;
#if SMP_OPCODE_DEBUG == 1
    SMP_TRACE_EVENT("<===smp_send_cmd on l2cap cmd_code=[%s][%d]\r\n", smp_code_2_str(cmd_code),
                    cmd_code);
#endif

    if(cmd_code <= (SMP_OPCODE_MAX + 1 /* for SMP_OPCODE_PAIR_COMMITM */) &&
            smp_cmd_build_act[cmd_code] != NULL) {
        p_buf = (*smp_cmd_build_act[cmd_code])(cmd_code, p_cb);

        if(p_buf != NULL &&
                smp_send_msg_to_L2CAP(p_cb->pairing_bda, p_buf)) {
            sent = TRUE;
#ifdef USE_ALARM
            alarm_set_on_queue(p_cb->smp_rsp_timer_ent,
                               SMP_WAIT_FOR_RSP_TIMEOUT_MS, smp_rsp_timeout,
                               NULL, btu_general_alarm_queue);
#else
            btu_stop_timer(&p_cb->smp_rsp_timer_ent);
            p_cb->smp_rsp_timer_ent.p_cback = (TIMER_CBACK *)&smp_rsp_timeout;
            p_cb->smp_rsp_timer_ent.param = (TIMER_PARAM_TYPE)NULL;
            btu_start_timer(&p_cb->smp_rsp_timer_ent, BTU_TTYPE_SMP_PAIRING_CMD,
                            SMP_WAIT_FOR_RSP_TIMEOUT_MS / 1000);
#endif
        }
    }

    if(!sent) {
        if(p_cb->smp_over_br) {
            smp_br_state_machine_event(p_cb, SMP_BR_AUTH_CMPL_EVT, &failure);
        } else {
            smp_sm_event(p_cb, SMP_AUTH_CMPL_EVT, &failure);
        }
    }

    return sent;
}

/*******************************************************************************
**
** Function         smp_rsp_timeout
**
** Description      Called when SMP wait for SMP command response timer expires
**
** Returns          void
**
*******************************************************************************/
void smp_rsp_timeout(UNUSED_ATTR void *data)
{
    tSMP_CB   *p_cb = &smp_cb;
    uint8_t failure = SMP_RSP_TIMEOUT;
    SMP_TRACE_EVENT("%s state:%d br_state:%d", __FUNCTION__, p_cb->state, p_cb->br_state);

    if(p_cb->smp_over_br) {
        smp_br_state_machine_event(p_cb, SMP_BR_AUTH_CMPL_EVT, &failure);
    } else {
        smp_sm_event(p_cb, SMP_AUTH_CMPL_EVT, &failure);
    }
}

/*******************************************************************************
**
** Function         smp_build_pairing_req_cmd
**
** Description      Build pairing request command.
**
*******************************************************************************/
BT_HDR *smp_build_pairing_cmd(uint8_t cmd_code, tSMP_CB *p_cb)
{
    uint8_t *p;
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) +
                                         SMP_PAIRING_REQ_SIZE + L2CAP_MIN_OFFSET);
    SMP_TRACE_EVENT("%s", __func__);
    p = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
    UINT8_TO_STREAM(p, cmd_code);
    UINT8_TO_STREAM(p, p_cb->local_io_capability);
    UINT8_TO_STREAM(p, p_cb->loc_oob_flag);
    UINT8_TO_STREAM(p, p_cb->loc_auth_req);
    UINT8_TO_STREAM(p, p_cb->loc_enc_size);
    UINT8_TO_STREAM(p, p_cb->local_i_key);
    UINT8_TO_STREAM(p, p_cb->local_r_key);
    p_buf->offset = L2CAP_MIN_OFFSET;
    /* 1B ERR_RSP op code + 1B cmd_op_code + 2B handle + 1B status */
    p_buf->len = SMP_PAIRING_REQ_SIZE;
    return p_buf;
}

/*******************************************************************************
**
** Function         smp_build_confirm_cmd
**
** Description      Build confirm request command.
**
*******************************************************************************/
static BT_HDR *smp_build_confirm_cmd(uint8_t cmd_code, tSMP_CB *p_cb)
{
    uint8_t *p;
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) +
                                         SMP_CONFIRM_CMD_SIZE + L2CAP_MIN_OFFSET);
    UNUSED(cmd_code);
    SMP_TRACE_EVENT("%s", __func__);
    p = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
    UINT8_TO_STREAM(p, SMP_OPCODE_CONFIRM);
    ARRAY_TO_STREAM(p, p_cb->confirm, BT_OCTET16_LEN);
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_buf->len = SMP_CONFIRM_CMD_SIZE;
    return p_buf;
}

/*******************************************************************************
**
** Function         smp_build_rand_cmd
**
** Description      Build Random command.
**
*******************************************************************************/
static BT_HDR *smp_build_rand_cmd(uint8_t cmd_code, tSMP_CB *p_cb)
{
    uint8_t *p;
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) +
                                         SMP_RAND_CMD_SIZE + L2CAP_MIN_OFFSET);
    UNUSED(cmd_code);
    SMP_TRACE_EVENT("%s", __func__);
    p = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
    UINT8_TO_STREAM(p, SMP_OPCODE_RAND);
    ARRAY_TO_STREAM(p, p_cb->rand, BT_OCTET16_LEN);
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_buf->len = SMP_RAND_CMD_SIZE;
    return p_buf;
}

/*******************************************************************************
**
** Function         smp_build_encrypt_info_cmd
**
** Description      Build security information command.
**
*******************************************************************************/
static BT_HDR *smp_build_encrypt_info_cmd(uint8_t cmd_code, tSMP_CB *p_cb)
{
    uint8_t *p;
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) +
                                         SMP_ENC_INFO_SIZE + L2CAP_MIN_OFFSET);
    UNUSED(cmd_code);
    SMP_TRACE_EVENT("%s", __func__);
    p = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
    UINT8_TO_STREAM(p, SMP_OPCODE_ENCRYPT_INFO);
    ARRAY_TO_STREAM(p, p_cb->ltk, BT_OCTET16_LEN);
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_buf->len = SMP_ENC_INFO_SIZE;
    return p_buf;
}

/*******************************************************************************
**
** Function         smp_build_master_id_cmd
**
** Description      Build security information command.
**
*******************************************************************************/
static BT_HDR *smp_build_master_id_cmd(uint8_t cmd_code, tSMP_CB *p_cb)
{
    uint8_t *p;
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) +
                                         SMP_MASTER_ID_SIZE + L2CAP_MIN_OFFSET);
    UNUSED(cmd_code);
    SMP_TRACE_EVENT("%s", __func__);
    p = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
    UINT8_TO_STREAM(p, SMP_OPCODE_MASTER_ID);
    UINT16_TO_STREAM(p, p_cb->ediv);
    ARRAY_TO_STREAM(p, p_cb->enc_rand, BT_OCTET8_LEN);
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_buf->len = SMP_MASTER_ID_SIZE;
    return p_buf;
}

/*******************************************************************************
**
** Function         smp_build_identity_info_cmd
**
** Description      Build identity information command.
**
*******************************************************************************/
static BT_HDR *smp_build_identity_info_cmd(uint8_t cmd_code, tSMP_CB *p_cb)
{
    uint8_t *p;
    BT_OCTET16 irk;
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) +
                                         SMP_ID_INFO_SIZE + L2CAP_MIN_OFFSET);
    UNUSED(cmd_code);
    UNUSED(p_cb);
    SMP_TRACE_EVENT("%s", __func__);
    p = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
    BTM_GetDeviceIDRoot(irk);
    UINT8_TO_STREAM(p, SMP_OPCODE_IDENTITY_INFO);
    ARRAY_TO_STREAM(p,  irk, BT_OCTET16_LEN);
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_buf->len = SMP_ID_INFO_SIZE;
    return p_buf;
}

/*******************************************************************************
**
** Function         smp_build_id_addr_cmd
**
** Description      Build identity address information command.
**
*******************************************************************************/
static BT_HDR *smp_build_id_addr_cmd(uint8_t cmd_code, tSMP_CB *p_cb)
{
    uint8_t *p;
    BD_ADDR     static_addr;
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) +
                                         SMP_ID_ADDR_SIZE + L2CAP_MIN_OFFSET);
    UNUSED(cmd_code);
    UNUSED(p_cb);
    SMP_TRACE_EVENT("%s", __func__);
    p = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
    UINT8_TO_STREAM(p, SMP_OPCODE_ID_ADDR);
    UINT8_TO_STREAM(p, 0);
    BTM_GetLocalDeviceAddr(static_addr);
    BDADDR_TO_STREAM(p, static_addr);
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_buf->len = SMP_ID_ADDR_SIZE;
    return p_buf;
}

/*******************************************************************************
**
** Function         smp_build_signing_info_cmd
**
** Description      Build signing information command.
**
*******************************************************************************/
static BT_HDR *smp_build_signing_info_cmd(uint8_t cmd_code, tSMP_CB *p_cb)
{
    uint8_t *p;
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) +
                                         SMP_SIGN_INFO_SIZE + L2CAP_MIN_OFFSET);
    UNUSED(cmd_code);
    SMP_TRACE_EVENT("%s", __func__);
    p = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
    UINT8_TO_STREAM(p, SMP_OPCODE_SIGN_INFO);
    ARRAY_TO_STREAM(p, p_cb->csrk, BT_OCTET16_LEN);
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_buf->len = SMP_SIGN_INFO_SIZE;
    return p_buf;
}

/*******************************************************************************
**
** Function         smp_build_pairing_fail
**
** Description      Build Pairing Fail command.
**
*******************************************************************************/
static BT_HDR *smp_build_pairing_fail(uint8_t cmd_code, tSMP_CB *p_cb)
{
    uint8_t *p;
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) +
                                         SMP_PAIR_FAIL_SIZE + L2CAP_MIN_OFFSET);
    UNUSED(cmd_code);
    SMP_TRACE_EVENT("%s", __func__);
    p = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
    UINT8_TO_STREAM(p, SMP_OPCODE_PAIRING_FAILED);
    UINT8_TO_STREAM(p, p_cb->failure);
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_buf->len = SMP_PAIR_FAIL_SIZE;
    return p_buf;
}

/*******************************************************************************
**
** Function         smp_build_security_request
**
** Description      Build security request command.
**
*******************************************************************************/
static BT_HDR *smp_build_security_request(uint8_t cmd_code, tSMP_CB *p_cb)
{
    uint8_t *p;
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) +
                                         2 + L2CAP_MIN_OFFSET);
    UNUSED(cmd_code);
    SMP_TRACE_EVENT("%s", __func__);
    p = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
    UINT8_TO_STREAM(p, SMP_OPCODE_SEC_REQ);
    UINT8_TO_STREAM(p, p_cb->loc_auth_req);
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_buf->len = SMP_SECURITY_REQUEST_SIZE;
    SMP_TRACE_EVENT("opcode=%d auth_req=0x%x", SMP_OPCODE_SEC_REQ,  p_cb->loc_auth_req);
    return p_buf;
}

/*******************************************************************************
**
** Function         smp_build_pair_public_key_cmd
**
** Description      Build pairing public key command.
**
*******************************************************************************/
static BT_HDR *smp_build_pair_public_key_cmd(uint8_t cmd_code, tSMP_CB *p_cb)
{
    uint8_t   *p;
    uint8_t   publ_key[2 * BT_OCTET32_LEN];
    uint8_t   *p_publ_key = publ_key;
    BT_HDR  *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) +
                                          SMP_PAIR_PUBL_KEY_SIZE + L2CAP_MIN_OFFSET);
    UNUSED(cmd_code);
    SMP_TRACE_EVENT("%s", __func__);
    wm_memcpy(p_publ_key, p_cb->loc_publ_key.x, BT_OCTET32_LEN);
    wm_memcpy(p_publ_key + BT_OCTET32_LEN, p_cb->loc_publ_key.y, BT_OCTET32_LEN);
    p = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
    UINT8_TO_STREAM(p, SMP_OPCODE_PAIR_PUBLIC_KEY);
    ARRAY_TO_STREAM(p, p_publ_key, 2 * BT_OCTET32_LEN);
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_buf->len = SMP_PAIR_PUBL_KEY_SIZE;
    return p_buf;
}

/*******************************************************************************
**
** Function         smp_build_pairing_commitment_cmd
**
** Description      Build pairing commitment command.
**
*******************************************************************************/
static BT_HDR *smp_build_pairing_commitment_cmd(uint8_t cmd_code, tSMP_CB *p_cb)
{
    uint8_t *p;
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) +
                                         SMP_PAIR_COMMITM_SIZE + L2CAP_MIN_OFFSET);
    UNUSED(cmd_code);
    SMP_TRACE_EVENT("%s", __func__);
    p = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
    UINT8_TO_STREAM(p, SMP_OPCODE_CONFIRM);
    ARRAY_TO_STREAM(p, p_cb->commitment, BT_OCTET16_LEN);
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_buf->len = SMP_PAIR_COMMITM_SIZE;
    return p_buf;
}

/*******************************************************************************
**
** Function         smp_build_pair_dhkey_check_cmd
**
** Description      Build pairing DHKey check command.
**
*******************************************************************************/
static BT_HDR *smp_build_pair_dhkey_check_cmd(uint8_t cmd_code, tSMP_CB *p_cb)
{
    uint8_t *p;
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) +
                                         SMP_PAIR_DHKEY_CHECK_SIZE + L2CAP_MIN_OFFSET);
    UNUSED(cmd_code);
    SMP_TRACE_EVENT("%s", __func__);
    p = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
    UINT8_TO_STREAM(p, SMP_OPCODE_PAIR_DHKEY_CHECK);
    ARRAY_TO_STREAM(p, p_cb->dhkey_check, BT_OCTET16_LEN);
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_buf->len = SMP_PAIR_DHKEY_CHECK_SIZE;
    return p_buf;
}

/*******************************************************************************
**
** Function         smp_build_pairing_keypress_notification_cmd
**
** Description      Build keypress notification command.
**
*******************************************************************************/
static BT_HDR *smp_build_pairing_keypress_notification_cmd(uint8_t cmd_code, tSMP_CB *p_cb)
{
    uint8_t       *p;
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) +
                                         SMP_PAIR_KEYPR_NOTIF_SIZE + L2CAP_MIN_OFFSET);
    UNUSED(cmd_code);
    SMP_TRACE_EVENT("%s", __func__);
    p = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
    UINT8_TO_STREAM(p, SMP_OPCODE_PAIR_KEYPR_NOTIF);
    UINT8_TO_STREAM(p, p_cb->local_keypress_notification);
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_buf->len = SMP_PAIR_KEYPR_NOTIF_SIZE;
    return p_buf;
}

/*******************************************************************************
**
** Function         smp_convert_string_to_tk
**
** Description      This function is called to convert a 6 to 16 digits numeric
**                  character string into SMP TK.
**
**
** Returns          void
**
*******************************************************************************/
void smp_convert_string_to_tk(BT_OCTET16 tk, uint32_t passkey)
{
    uint8_t   *p = tk;
    tSMP_KEY    key;
    SMP_TRACE_EVENT("smp_convert_string_to_tk");
    UINT32_TO_STREAM(p, passkey);
    key.key_type    = SMP_KEY_TYPE_TK;
    key.p_data      = tk;
    smp_sm_event(&smp_cb, SMP_KEY_READY_EVT, &key);
}

/*******************************************************************************
**
** Function         smp_mask_enc_key
**
** Description      This function is called to mask off the encryption key based
**                  on the maximum encryption key size.
**
**
** Returns          void
**
*******************************************************************************/
void smp_mask_enc_key(uint8_t loc_enc_size, uint8_t *p_data)
{
    SMP_TRACE_EVENT("smp_mask_enc_key");

    if(loc_enc_size < BT_OCTET16_LEN) {
        for(; loc_enc_size < BT_OCTET16_LEN; loc_enc_size ++) {
            * (p_data + loc_enc_size) = 0;
        }
    }

    return;
}

/*******************************************************************************
**
** Function         smp_xor_128
**
** Description      utility function to do an biteise exclusive-OR of two bit
**                  strings of the length of BT_OCTET16_LEN.
**
** Returns          void
**
*******************************************************************************/
void smp_xor_128(BT_OCTET16 a, BT_OCTET16 b)
{
    uint8_t i, *aa = a, *bb = b;
    SMP_TRACE_EVENT("smp_xor_128");

    for(i = 0; i < BT_OCTET16_LEN; i++) {
        aa[i] = aa[i] ^ bb[i];
    }
}

/*******************************************************************************
**
** Function         smp_cb_cleanup
**
** Description      Clean up SMP control block
**
** Returns          void
**
*******************************************************************************/
void smp_cb_cleanup(tSMP_CB   *p_cb)
{
    tSMP_CALLBACK   *p_callback = p_cb->p_callback;
    uint8_t           trace_level = p_cb->trace_level;
    SMP_TRACE_EVENT("smp_cb_cleanup");
#ifdef USE_ALARM
    alarm_free(p_cb->smp_rsp_timer_ent);
#endif
    wm_memset(p_cb, 0, sizeof(tSMP_CB));
    p_cb->p_callback = p_callback;
    p_cb->trace_level = trace_level;
#ifdef USE_ALARM
    p_cb->smp_rsp_timer_ent = alarm_new("smp.smp_rsp_timer_ent");
#endif
}

/*******************************************************************************
**
** Function         smp_remove_fixed_channel
**
** Description      This function is called to remove the fixed channel
**
** Returns          void
**
*******************************************************************************/
void smp_remove_fixed_channel(tSMP_CB *p_cb)
{
    SMP_TRACE_DEBUG("%s", __func__);

    if(p_cb->smp_over_br) {
        L2CA_RemoveFixedChnl(L2CAP_SMP_BR_CID, p_cb->pairing_bda);
    } else {
        L2CA_RemoveFixedChnl(L2CAP_SMP_CID, p_cb->pairing_bda);
    }
}

/*******************************************************************************
**
** Function         smp_reset_control_value
**
** Description      This function is called to reset the control block value when
**                  pairing procedure finished.
**
**
** Returns          void
**
*******************************************************************************/
void smp_reset_control_value(tSMP_CB *p_cb)
{
    SMP_TRACE_EVENT("%s", __func__);
#ifdef USE_ALARM
    alarm_cancel(p_cb->smp_rsp_timer_ent);
#else
    btu_stop_timer(&p_cb->smp_rsp_timer_ent);
#endif
    p_cb->flags = 0;
    /* set the link idle timer to drop the link when pairing is done
       usually service discovery will follow authentication complete, to avoid
       racing condition for a link down/up, set link idle timer to be
       SMP_LINK_TOUT_MIN to guarantee SMP key exchange */
    L2CA_SetIdleTimeoutByBdAddr(p_cb->pairing_bda, SMP_LINK_TOUT_MIN, BT_TRANSPORT_LE);
    /* We can tell L2CAP to remove the fixed channel (if it has one) */
    smp_remove_fixed_channel(p_cb);
    smp_cb_cleanup(p_cb);
}

/*******************************************************************************
**
** Function         smp_proc_pairing_cmpl
**
** Description      This function is called to process pairing complete
**
**
** Returns          void
**
*******************************************************************************/
void smp_proc_pairing_cmpl(tSMP_CB *p_cb)
{
    tSMP_EVT_DATA   evt_data = {0};
    tSMP_CALLBACK   *p_callback = p_cb->p_callback;
    BD_ADDR         pairing_bda;
    SMP_TRACE_DEBUG("smp_proc_pairing_cmpl ");
    evt_data.cmplt.reason = p_cb->status;
    evt_data.cmplt.smp_over_br = p_cb->smp_over_br;

    if(p_cb->status == SMP_SUCCESS) {
        evt_data.cmplt.sec_level = p_cb->sec_level;
    }

    evt_data.cmplt.is_pair_cancel  = FALSE;

    if(p_cb->is_pair_cancel) {
        evt_data.cmplt.is_pair_cancel = TRUE;
    }

    SMP_TRACE_DEBUG("send SMP_COMPLT_EVT reason=0x%0x sec_level=0x%0x",
                    evt_data.cmplt.reason,
                    evt_data.cmplt.sec_level);
    wm_memcpy(pairing_bda, p_cb->pairing_bda, BD_ADDR_LEN);
    smp_reset_control_value(p_cb);

    if(p_callback) {
        (*p_callback)(SMP_COMPLT_EVT, pairing_bda, &evt_data);
    }
}

/*******************************************************************************
**
** Function         smp_command_has_invalid_parameters
**
** Description      Checks if the received SMP command has invalid parameters i.e.
**                  if the command length is valid and the command parameters are
**                  inside specified range.
**                  It returns TRUE if the command has invalid parameters.
**
** Returns          TRUE if the command has invalid parameters, FALSE otherwise.
**
*******************************************************************************/
uint8_t smp_command_has_invalid_parameters(tSMP_CB *p_cb)
{
    uint8_t cmd_code = p_cb->rcvd_cmd_code;
    SMP_TRACE_DEBUG("%s for cmd code 0x%02x", __func__, cmd_code);

    if((cmd_code > (SMP_OPCODE_MAX + 1 /* for SMP_OPCODE_PAIR_COMMITM */)) ||
            (cmd_code < SMP_OPCODE_MIN)) {
        SMP_TRACE_WARNING("Somehow received command with the RESERVED code 0x%02x", cmd_code);
        return TRUE;
    }

    if(!(*smp_cmd_len_is_valid[cmd_code])(p_cb)) {
        return TRUE;
    }

    if(!(*smp_cmd_param_ranges_are_valid[cmd_code])(p_cb)) {
        return TRUE;
    }

    return FALSE;
}

/*******************************************************************************
**
** Function         smp_command_has_valid_fixed_length
**
** Description      Checks if the received command size is equal to the size
**                  according to specs.
**
** Returns          TRUE if the command size is as expected, FALSE otherwise.
**
** Note             The command is expected to have fixed length.
*******************************************************************************/
uint8_t smp_command_has_valid_fixed_length(tSMP_CB *p_cb)
{
    uint8_t   cmd_code = p_cb->rcvd_cmd_code;
    SMP_TRACE_DEBUG("%s for cmd code 0x%02x", __func__, cmd_code);

    if(p_cb->rcvd_cmd_len != smp_cmd_size_per_spec[cmd_code]) {
        SMP_TRACE_WARNING("Rcvd from the peer cmd 0x%02x with invalid length\
            0x%02x (per spec the length is 0x%02x).",
                          cmd_code, p_cb->rcvd_cmd_len, smp_cmd_size_per_spec[cmd_code]);
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************************
**
** Function         smp_pairing_request_response_parameters_are_valid
**
** Description      Validates parameter ranges in the received SMP command
**                  pairing request or pairing response.
**                  The parameters to validate:
**                  IO capability,
**                  OOB data flag,
**                  Bonding_flags in AuthReq
**                  Maximum encryption key size.
**                  Returns FALSE if at least one of these parameters is out of range.
**
*******************************************************************************/
uint8_t smp_pairing_request_response_parameters_are_valid(tSMP_CB *p_cb)
{
    uint8_t   io_caps = p_cb->peer_io_caps;
    uint8_t   oob_flag = p_cb->peer_oob_flag;
    uint8_t   bond_flag = p_cb->peer_auth_req & 0x03; //0x03 is gen bond with appropriate mask
    uint8_t   enc_size = p_cb->peer_enc_size;
    SMP_TRACE_DEBUG("%s for cmd code 0x%02x", __func__, p_cb->rcvd_cmd_code);

    if(io_caps >= BTM_IO_CAP_MAX) {
        SMP_TRACE_WARNING("Rcvd from the peer cmd 0x%02x with IO Capabilty \
            value (0x%02x) out of range).",
                          p_cb->rcvd_cmd_code, io_caps);
        return FALSE;
    }

    if(!((oob_flag == SMP_OOB_NONE) || (oob_flag == SMP_OOB_PRESENT))) {
        SMP_TRACE_WARNING("Rcvd from the peer cmd 0x%02x with OOB data flag value \
            (0x%02x) out of range).",
                          p_cb->rcvd_cmd_code, oob_flag);
        return FALSE;
    }

    if(!((bond_flag == SMP_AUTH_NO_BOND) || (bond_flag == SMP_AUTH_BOND))) {
        SMP_TRACE_WARNING("Rcvd from the peer cmd 0x%02x with Bonding_Flags value (0x%02x)\
                           out of range).",
                          p_cb->rcvd_cmd_code, bond_flag);
        return FALSE;
    }

    if((enc_size < SMP_ENCR_KEY_SIZE_MIN) || (enc_size > SMP_ENCR_KEY_SIZE_MAX)) {
        SMP_TRACE_WARNING("Rcvd from the peer cmd 0x%02x with Maximum Encryption \
            Key value (0x%02x) out of range).",
                          p_cb->rcvd_cmd_code, enc_size);
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************************
**
** Function         smp_pairing_keypress_notification_is_valid
**
** Description      Validates Notification Type parameter range in the received SMP command
**                  pairing keypress notification.
**                  Returns FALSE if this parameter is out of range.
**
*******************************************************************************/
uint8_t smp_pairing_keypress_notification_is_valid(tSMP_CB *p_cb)
{
    tBTM_SP_KEY_TYPE keypress_notification = p_cb->peer_keypress_notification;
    SMP_TRACE_DEBUG("%s for cmd code 0x%02x", __func__, p_cb->rcvd_cmd_code);

    if(keypress_notification >= BTM_SP_KEY_OUT_OF_RANGE) {
        SMP_TRACE_WARNING("Rcvd from the peer cmd 0x%02x with Pairing Keypress \
            Notification value (0x%02x) out of range).",
                          p_cb->rcvd_cmd_code, keypress_notification);
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************************
**
** Function         smp_parameter_unconditionally_valid
**
** Description      Always returns TRUE.
**
*******************************************************************************/
uint8_t smp_parameter_unconditionally_valid(UNUSED_ATTR tSMP_CB *p_cb)
{
    return TRUE;
}

/*******************************************************************************
**
** Function         smp_parameter_unconditionally_invalid
**
** Description      Always returns FALSE.
**
*******************************************************************************/
uint8_t smp_parameter_unconditionally_invalid(UNUSED_ATTR tSMP_CB *p_cb)
{
    return FALSE;
}

/*******************************************************************************
**
** Function         smp_reject_unexpected_pairing_command
**
** Description      send pairing failure to an unexpected pairing command during
**                  an active pairing process.
**
** Returns          void
**
*******************************************************************************/
void smp_reject_unexpected_pairing_command(BD_ADDR bd_addr)
{
    uint8_t *p;
    BT_HDR *p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) +
                                         SMP_PAIR_FAIL_SIZE + L2CAP_MIN_OFFSET);
    SMP_TRACE_DEBUG("%s", __func__);
    p = (uint8_t *)(p_buf + 1) + L2CAP_MIN_OFFSET;
    UINT8_TO_STREAM(p, SMP_OPCODE_PAIRING_FAILED);
    UINT8_TO_STREAM(p, SMP_PAIR_NOT_SUPPORT);
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_buf->len = SMP_PAIR_FAIL_SIZE;
    smp_send_msg_to_L2CAP(bd_addr, p_buf);
}

/*******************************************************************************
** Function         smp_select_association_model
**
** Description      This function selects association model to use for STK
**                  generation. Selection is based on both sides' io capability,
**                  oob data flag and authentication request.
**
** Note             If Secure Connections Only mode is required locally then we
**                  come to this point only if both sides support Secure Connections
**                  mode, i.e. if p_cb->secure_connections_only_mode_required = TRUE then we come
**                  to this point only if
**                      (p_cb->peer_auth_req & SMP_SC_SUPPORT_BIT) ==
**                      (p_cb->loc_auth_req & SMP_SC_SUPPORT_BIT) ==
**                      SMP_SC_SUPPORT_BIT
**
*******************************************************************************/
tSMP_ASSO_MODEL smp_select_association_model(tSMP_CB *p_cb)
{
    tSMP_ASSO_MODEL model = SMP_MODEL_OUT_OF_RANGE;
    p_cb->le_secure_connections_mode_is_used = FALSE;
    SMP_TRACE_EVENT("%s", __FUNCTION__);
    SMP_TRACE_DEBUG("%s p_cb->peer_io_caps = %d p_cb->local_io_capability = %d",
                    __FUNCTION__, p_cb->peer_io_caps, p_cb->local_io_capability);
    SMP_TRACE_DEBUG("%s p_cb->peer_oob_flag = %d p_cb->loc_oob_flag = %d",
                    __FUNCTION__, p_cb->peer_oob_flag, p_cb->loc_oob_flag);
    SMP_TRACE_DEBUG("%s p_cb->peer_auth_req = 0x%02x p_cb->loc_auth_req = 0x%02x",
                    __FUNCTION__, p_cb->peer_auth_req, p_cb->loc_auth_req);
    SMP_TRACE_DEBUG("%s p_cb->secure_connections_only_mode_required = %s",
                    __FUNCTION__, p_cb->secure_connections_only_mode_required ?
                    "TRUE" : "FALSE");

    if((p_cb->peer_auth_req & SMP_SC_SUPPORT_BIT) && (p_cb->loc_auth_req & SMP_SC_SUPPORT_BIT)) {
        p_cb->le_secure_connections_mode_is_used = TRUE;
    }

    SMP_TRACE_DEBUG("use_sc_process = %d", p_cb->le_secure_connections_mode_is_used);

    if(p_cb->le_secure_connections_mode_is_used) {
        model = smp_select_association_model_secure_connections(p_cb);
    } else {
        model = smp_select_legacy_association_model(p_cb);
    }

    return model;
}

/*******************************************************************************
** Function         smp_select_legacy_association_model
**
** Description      This function is called to select association mode if at least
**                  one side doesn't support secure connections.
**
*******************************************************************************/
tSMP_ASSO_MODEL smp_select_legacy_association_model(tSMP_CB *p_cb)
{
    tSMP_ASSO_MODEL model = SMP_MODEL_OUT_OF_RANGE;
    SMP_TRACE_DEBUG("%s", __func__);

    /* if OOB data is present on both devices, then use OOB association model */
    if(p_cb->peer_oob_flag == SMP_OOB_PRESENT && p_cb->loc_oob_flag == SMP_OOB_PRESENT) {
        return SMP_MODEL_OOB;
    }

    /* else if neither device requires MITM, then use Just Works association model */
    if(SMP_NO_MITM_REQUIRED(p_cb->peer_auth_req) && SMP_NO_MITM_REQUIRED(p_cb->loc_auth_req)) {
        return SMP_MODEL_ENCRYPTION_ONLY;
    }

    /* otherwise use IO capability to select association model */
    if(p_cb->peer_io_caps < SMP_IO_CAP_MAX && p_cb->local_io_capability < SMP_IO_CAP_MAX) {
        if(p_cb->role == HCI_ROLE_MASTER) {
            model = smp_association_table[p_cb->role][p_cb->peer_io_caps]
                    [p_cb->local_io_capability];
        } else {
            model = smp_association_table[p_cb->role][p_cb->local_io_capability]
                    [p_cb->peer_io_caps];
        }
    }

    return model;
}

/*******************************************************************************
** Function         smp_select_association_model_secure_connections
**
** Description      This function is called to select association mode if both
**                  sides support secure connections.
**
*******************************************************************************/
tSMP_ASSO_MODEL smp_select_association_model_secure_connections(tSMP_CB *p_cb)
{
    tSMP_ASSO_MODEL model = SMP_MODEL_OUT_OF_RANGE;
    SMP_TRACE_DEBUG("%s", __func__);

    /* if OOB data is present on at least one device, then use OOB association model */
    if(p_cb->peer_oob_flag == SMP_OOB_PRESENT || p_cb->loc_oob_flag == SMP_OOB_PRESENT) {
        return SMP_MODEL_SEC_CONN_OOB;
    }

    /* else if neither device requires MITM, then use Just Works association model */
    if(SMP_NO_MITM_REQUIRED(p_cb->peer_auth_req) && SMP_NO_MITM_REQUIRED(p_cb->loc_auth_req)) {
        return SMP_MODEL_SEC_CONN_JUSTWORKS;
    }

    /* otherwise use IO capability to select association model */
    if(p_cb->peer_io_caps < SMP_IO_CAP_MAX && p_cb->local_io_capability < SMP_IO_CAP_MAX) {
        if(p_cb->role == HCI_ROLE_MASTER) {
            model = smp_association_table_sc[p_cb->role][p_cb->peer_io_caps]
                    [p_cb->local_io_capability];
        } else {
            model = smp_association_table_sc[p_cb->role][p_cb->local_io_capability]
                    [p_cb->peer_io_caps];
        }
    }

    return model;
}

/*******************************************************************************
** Function         smp_reverse_array
**
** Description      This function reverses array bytes
**
*******************************************************************************/
void smp_reverse_array(uint8_t *arr, uint8_t len)
{
    uint8_t i = 0, tmp;
    SMP_TRACE_DEBUG("smp_reverse_array");

    for(i = 0; i < len / 2; i ++) {
        tmp = arr[i];
        arr[i] = arr[len - 1 - i];
        arr[len - 1 - i] = tmp;
    }
}

/*******************************************************************************
** Function         smp_calculate_random_input
**
** Description      This function returns random input value to be used in commitment
**                  calculation for SC passkey entry association mode
**                  (if bit["round"] in "random" array == 1 then returns 0x81
**                   else returns 0x80).
**
** Returns          ri value
**
*******************************************************************************/
uint8_t smp_calculate_random_input(uint8_t *random, uint8_t round)
{
    uint8_t i = round / 8;
    uint8_t j = round % 8;
    uint8_t ri;
    SMP_TRACE_DEBUG("random: 0x%02x, round: %d, i: %d, j: %d", random[i], round, i, j);
    ri = ((random[i] >> j) & 1) | 0x80;
    SMP_TRACE_DEBUG("%s ri=0x%02x", __func__, ri);
    return ri;
}

/*******************************************************************************
** Function         smp_collect_local_io_capabilities
**
** Description      This function puts into IOcap array local device
**                  IOCapability, OOB data, AuthReq.
**
** Returns          void
**
*******************************************************************************/
void smp_collect_local_io_capabilities(uint8_t *iocap, tSMP_CB *p_cb)
{
    SMP_TRACE_DEBUG("%s", __func__);
    iocap[0] = p_cb->local_io_capability;
    iocap[1] = p_cb->loc_oob_flag;
    iocap[2] = p_cb->loc_auth_req;
}

/*******************************************************************************
** Function         smp_collect_peer_io_capabilities
**
** Description      This function puts into IOcap array peer device
**                  IOCapability, OOB data, AuthReq.
**
** Returns          void
**
*******************************************************************************/
void smp_collect_peer_io_capabilities(uint8_t *iocap, tSMP_CB *p_cb)
{
    SMP_TRACE_DEBUG("%s", __func__);
    iocap[0] = p_cb->peer_io_caps;
    iocap[1] = p_cb->peer_oob_flag;
    iocap[2] = p_cb->peer_auth_req;
}

/*******************************************************************************
** Function         smp_collect_local_ble_address
**
** Description      This function puts into le_addr array local device le address:
**                  le_addr[0-5] = local BD ADDR,
**                  le_addr[6] = local le address type (PUBLIC/RANDOM).
**
** Returns          void
**
*******************************************************************************/
void smp_collect_local_ble_address(uint8_t *le_addr, tSMP_CB *p_cb)
{
    tBLE_ADDR_TYPE  addr_type = 0;
    BD_ADDR         bda;
    uint8_t           *p = le_addr;
    SMP_TRACE_DEBUG("%s", __func__);
    BTM_ReadConnectionAddr(p_cb->pairing_bda, bda, &addr_type);
    BDADDR_TO_STREAM(p, bda);
    UINT8_TO_STREAM(p, addr_type);
}

/*******************************************************************************
** Function         smp_collect_peer_ble_address
**
** Description      This function puts into le_addr array peer device le address:
**                  le_addr[0-5] = peer BD ADDR,
**                  le_addr[6] = peer le address type (PUBLIC/RANDOM).
**
** Returns          void
**
*******************************************************************************/
void smp_collect_peer_ble_address(uint8_t *le_addr, tSMP_CB *p_cb)
{
    tBLE_ADDR_TYPE  addr_type = 0;
    BD_ADDR         bda;
    uint8_t           *p = le_addr;
    SMP_TRACE_DEBUG("%s", __func__);

    if(!BTM_ReadRemoteConnectionAddr(p_cb->pairing_bda, bda, &addr_type)) {
        SMP_TRACE_ERROR("can not collect peer le addr information for unknown device");
        return;
    }

    BDADDR_TO_STREAM(p, bda);
    UINT8_TO_STREAM(p, addr_type);
}

/*******************************************************************************
** Function         smp_check_commitment
**
** Description      This function compares peer commitment values:
**                  - expected (i.e. calculated locally),
**                  - received from the peer.
**
** Returns          TRUE  if the values are the same
**                  FALSE otherwise
**
*******************************************************************************/
uint8_t smp_check_commitment(tSMP_CB *p_cb)
{
    BT_OCTET16 expected;
    SMP_TRACE_DEBUG("%s", __func__);
    smp_calculate_peer_commitment(p_cb, expected);
    print128(expected, (const uint8_t *)"calculated peer commitment");
    print128(p_cb->remote_commitment, (const uint8_t *)"received peer commitment");

    if(memcmp(p_cb->remote_commitment, expected, BT_OCTET16_LEN)) {
        SMP_TRACE_WARNING("Commitment check fails");
        return FALSE;
    }

    SMP_TRACE_DEBUG("Commitment check succeeds");
    return TRUE;
}

/*******************************************************************************
**
** Function         smp_save_secure_connections_long_term_key
**
** Description      The function saves SC LTK as BLE key for future use as local
**                  and/or peer key.
**
** Returns          void
**
*******************************************************************************/
void smp_save_secure_connections_long_term_key(tSMP_CB *p_cb)
{
    tBTM_LE_LENC_KEYS   lle_key;
    tBTM_LE_PENC_KEYS   ple_key;
    SMP_TRACE_DEBUG("%s-Save LTK as local LTK key", __func__);
    wm_memcpy(lle_key.ltk, p_cb->ltk, BT_OCTET16_LEN);
    lle_key.div = 0;
    lle_key.key_size = p_cb->loc_enc_size;
    lle_key.sec_level = p_cb->sec_level;
    btm_sec_save_le_key(p_cb->pairing_bda, BTM_LE_KEY_LENC, (tBTM_LE_KEY_VALUE *)&lle_key, TRUE);
    SMP_TRACE_DEBUG("%s-Save LTK as peer LTK key", __func__);
    ple_key.ediv = 0;
    wm_memset(ple_key.rand, 0, BT_OCTET8_LEN);
    wm_memcpy(ple_key.ltk, p_cb->ltk, BT_OCTET16_LEN);
    ple_key.sec_level = p_cb->sec_level;
    ple_key.key_size  = p_cb->loc_enc_size;
    btm_sec_save_le_key(p_cb->pairing_bda, BTM_LE_KEY_PENC, (tBTM_LE_KEY_VALUE *)&ple_key, TRUE);
}

/*******************************************************************************
**
** Function         smp_calculate_f5_mackey_and_long_term_key
**
** Description      The function calculates MacKey and LTK and saves them in CB.
**                  To calculate MacKey and LTK it calls smp_calc_f5(...).
**                  MacKey is used in dhkey calculation, LTK is used to encrypt
**                  the link.
**
** Returns          FALSE if out of resources, TRUE otherwise.
**
*******************************************************************************/
uint8_t smp_calculate_f5_mackey_and_long_term_key(tSMP_CB *p_cb)
{
    uint8_t a[7];
    uint8_t b[7];
    uint8_t *p_na;
    uint8_t *p_nb;
    SMP_TRACE_DEBUG("%s", __func__);

    if(p_cb->role == HCI_ROLE_MASTER) {
        smp_collect_local_ble_address(a, p_cb);
        smp_collect_peer_ble_address(b, p_cb);
        p_na = p_cb->rand;
        p_nb = p_cb->rrand;
    } else {
        smp_collect_local_ble_address(b, p_cb);
        smp_collect_peer_ble_address(a, p_cb);
        p_na = p_cb->rrand;
        p_nb = p_cb->rand;
    }

    if(!smp_calculate_f5(p_cb->dhkey, p_na, p_nb, a, b, p_cb->mac_key, p_cb->ltk)) {
        SMP_TRACE_ERROR("%s failed", __func__);
        return FALSE;
    }

    SMP_TRACE_EVENT("%s is completed", __func__);
    return TRUE;
}

/*******************************************************************************
**
** Function         smp_request_oob_data
**
** Description      Requests application to provide OOB data.
**
** Returns          TRUE - OOB data has to be provided by application
**                  FALSE - otherwise (unexpected)
**
*******************************************************************************/
uint8_t smp_request_oob_data(tSMP_CB *p_cb)
{
    tSMP_OOB_DATA_TYPE req_oob_type = SMP_OOB_INVALID_TYPE;
    SMP_TRACE_DEBUG("%s", __func__);

    if(p_cb->peer_oob_flag == SMP_OOB_PRESENT && p_cb->loc_oob_flag == SMP_OOB_PRESENT) {
        /* both local and peer rcvd data OOB */
        req_oob_type = SMP_OOB_BOTH;
    } else if(p_cb->peer_oob_flag == SMP_OOB_PRESENT) {
        /* peer rcvd OOB local data, local didn't receive OOB peer data */
        req_oob_type = SMP_OOB_LOCAL;
    } else if(p_cb->loc_oob_flag == SMP_OOB_PRESENT) {
        req_oob_type = SMP_OOB_PEER;
    }

    SMP_TRACE_DEBUG("req_oob_type = %d", req_oob_type);

    if(req_oob_type == SMP_OOB_INVALID_TYPE) {
        return FALSE;
    }

    p_cb->req_oob_type = req_oob_type;
    p_cb->cb_evt = SMP_SC_OOB_REQ_EVT;
    smp_sm_event(p_cb, SMP_TK_REQ_EVT, &req_oob_type);
    return TRUE;
}


#endif

