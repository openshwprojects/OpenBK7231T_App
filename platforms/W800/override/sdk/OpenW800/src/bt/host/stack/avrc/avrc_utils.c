/******************************************************************************
 *
 *  Copyright (C) 2003-2013 Broadcom Corporation
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
#include <string.h>

#include "bt_common.h"
#include "avrc_api.h"
#include "avrc_int.h"


#if (AVRC_METADATA_INCLUDED == TRUE)

/**************************************************************************
**
** Function         AVRC_IsValidAvcType
**
** Description      Check if correct AVC type is specified
**
** Returns          returns TRUE if it is valid
**
**
*******************************************************************************/
uint8_t AVRC_IsValidAvcType(uint8_t pdu_id, uint8_t avc_type)
{
    uint8_t result = FALSE;

    if(avc_type < AVRC_RSP_NOT_IMPL) { /* command msg */
        switch(pdu_id) {
            case AVRC_PDU_GET_CAPABILITIES:            /* 0x10 */
            case AVRC_PDU_LIST_PLAYER_APP_ATTR:        /* 0x11 */
            case AVRC_PDU_LIST_PLAYER_APP_VALUES:      /* 0x12 */
            case AVRC_PDU_GET_CUR_PLAYER_APP_VALUE:    /* 0x13 */
            case AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT:    /* 0x15 */
            case AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT:   /* 0x16 */
            case AVRC_PDU_GET_ELEMENT_ATTR:            /* 0x20 */
            case AVRC_PDU_GET_PLAY_STATUS:             /* 0x30 */
                if(avc_type == AVRC_CMD_STATUS) {
                    result = TRUE;
                }

                break;

            case AVRC_PDU_SET_PLAYER_APP_VALUE:        /* 0x14 */
            case AVRC_PDU_INFORM_DISPLAY_CHARSET:      /* 0x17 */
            case AVRC_PDU_INFORM_BATTERY_STAT_OF_CT:   /* 0x18 */
            case AVRC_PDU_REQUEST_CONTINUATION_RSP:    /* 0x40 */
            case AVRC_PDU_ABORT_CONTINUATION_RSP:      /* 0x41 */
            case AVRC_PDU_SET_ADDRESSED_PLAYER:
            case AVRC_PDU_PLAY_ITEM:
            case AVRC_PDU_SET_ABSOLUTE_VOLUME:
                if(avc_type == AVRC_CMD_CTRL) {
                    result = TRUE;
                }

                break;

            case AVRC_PDU_REGISTER_NOTIFICATION:       /* 0x31 */
                if(avc_type == AVRC_CMD_NOTIF) {
                    result = TRUE;
                }

                break;
        }
    } else { /* response msg */
        if(avc_type >= AVRC_RSP_NOT_IMPL  &&
                avc_type <= AVRC_RSP_INTERIM) {
            result = TRUE;
        }
    }

    return result;
}

/*******************************************************************************
**
** Function         avrc_is_valid_player_attrib_value
**
** Description      Check if the given attrib value is valid for its attribute
**
** Returns          returns TRUE if it is valid
**
*******************************************************************************/
uint8_t avrc_is_valid_player_attrib_value(uint8_t attrib, uint8_t value)
{
    uint8_t result = FALSE;

    switch(attrib) {
        case AVRC_PLAYER_SETTING_EQUALIZER:
            if((value > 0)  &&
                    (value <= AVRC_PLAYER_VAL_ON)) {
                result = TRUE;
            }

            break;

        case AVRC_PLAYER_SETTING_REPEAT:
            if((value > 0)  &&
                    (value <= AVRC_PLAYER_VAL_GROUP_REPEAT)) {
                result = TRUE;
            }

            break;

        case AVRC_PLAYER_SETTING_SHUFFLE:
        case AVRC_PLAYER_SETTING_SCAN:
            if((value > 0)  &&
                    (value <= AVRC_PLAYER_VAL_GROUP_SHUFFLE)) {
                result = TRUE;
            }

            break;
    }

    if(attrib >= AVRC_PLAYER_SETTING_LOW_MENU_EXT) {
        result = TRUE;
    }

    if(!result)
        AVRC_TRACE_ERROR(
                        "avrc_is_valid_player_attrib_value() found not matching attrib(x%x)-value(x%x) pair!",
                        attrib, value);

    return result;
}

/*******************************************************************************
**
** Function         AVRC_IsValidPlayerAttr
**
** Description      Check if the given attrib value is a valid one
**
** Returns          returns TRUE if it is valid
**
*******************************************************************************/
uint8_t AVRC_IsValidPlayerAttr(uint8_t attr)
{
    uint8_t result = FALSE;

    if((attr >= AVRC_PLAYER_SETTING_EQUALIZER && attr <= AVRC_PLAYER_SETTING_SCAN) ||
            (attr >= AVRC_PLAYER_SETTING_LOW_MENU_EXT)) {
        result = TRUE;
    }

    return result;
}



/*******************************************************************************
**
** Function         avrc_pars_pass_thru
**
** Description      This function parses the pass thru commands defined by
**                  Bluetooth SIG
**
** Returns          AVRC_STS_NO_ERROR, if the message in p_data is parsed successfully.
**                  Otherwise, the error code defined by AVRCP 1.4
**
*******************************************************************************/
tAVRC_STS avrc_pars_pass_thru(tAVRC_MSG_PASS *p_msg, uint16_t *p_vendor_unique_id)
{
    uint8_t      *p_data;
    uint32_t      co_id;
    uint16_t      id;
    tAVRC_STS  status = AVRC_STS_BAD_CMD;

    if(p_msg->op_id == AVRC_ID_VENDOR && p_msg->pass_len == AVRC_PASS_THRU_GROUP_LEN) {
        p_data = p_msg->p_pass_data;
        AVRC_BE_STREAM_TO_CO_ID(co_id, p_data);

        if(co_id == AVRC_CO_METADATA) {
            BE_STREAM_TO_UINT16(id, p_data);

            if(AVRC_IS_VALID_GROUP(id)) {
                *p_vendor_unique_id = id;
                status = AVRC_STS_NO_ERROR;
            }
        }
    }

    return status;
}

/*******************************************************************************
**
** Function         avrc_opcode_from_pdu
**
** Description      This function returns the opcode of the given pdu
**
** Returns          AVRC_OP_VENDOR, AVRC_OP_PASS_THRU or AVRC_OP_BROWSE
**
*******************************************************************************/
uint8_t avrc_opcode_from_pdu(uint8_t pdu)
{
    uint8_t opcode = 0;

    switch(pdu) {
        case AVRC_PDU_NEXT_GROUP:
        case AVRC_PDU_PREV_GROUP: /* pass thru */
            opcode  = AVRC_OP_PASS_THRU;
            break;

        default: /* vendor */
            opcode  = AVRC_OP_VENDOR;
            break;
    }

    return opcode;
}

/*******************************************************************************
**
** Function         avrc_is_valid_opcode
**
** Description      This function returns the opcode of the given pdu
**
** Returns          AVRC_OP_VENDOR, AVRC_OP_PASS_THRU or AVRC_OP_BROWSE
**
*******************************************************************************/
uint8_t avrc_is_valid_opcode(uint8_t opcode)
{
    uint8_t is_valid = FALSE;

    switch(opcode) {
        case AVRC_OP_BROWSE:
        case AVRC_OP_PASS_THRU:
        case AVRC_OP_VENDOR:
            is_valid = TRUE;
            break;
    }

    return is_valid;
}

#endif /* (AVRC_METADATA_INCLUDED == TRUE) */

