/******************************************************************************
 *
 *  Copyright (C) 2006-2013 Broadcom Corporation
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
#include "avrc_defs.h"
#include "avrc_int.h"
#include "bt_utils.h"
#include "gki.h"
/*****************************************************************************
**  Global data
*****************************************************************************/

#if (AVRC_METADATA_INCLUDED == TRUE)

/*******************************************************************************
**
** Function         avrc_pars_vendor_rsp
**
** Description      This function parses the vendor specific commands defined by
**                  Bluetooth SIG
**
** Returns          AVRC_STS_NO_ERROR, if the message in p_data is parsed successfully.
**                  Otherwise, the error code defined by AVRCP 1.4
**
*******************************************************************************/
static tAVRC_STS avrc_pars_vendor_rsp(tAVRC_MSG_VENDOR *p_msg, tAVRC_RESPONSE *p_result)
{
    tAVRC_STS  status = AVRC_STS_NO_ERROR;
    uint8_t   *p;
    uint16_t  len;
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
    uint8_t eventid = 0;
#endif

    /* Check the vendor data */
    if(p_msg->vendor_len == 0) {
        return AVRC_STS_NO_ERROR;
    }

    if(p_msg->p_vendor_data == NULL) {
        return AVRC_STS_INTERNAL_ERR;
    }

    p = p_msg->p_vendor_data;
    BE_STREAM_TO_UINT8(p_result->pdu, p);
    p++; /* skip the reserved/packe_type byte */
    BE_STREAM_TO_UINT16(len, p);
    AVRC_TRACE_DEBUG("%s ctype:0x%x pdu:0x%x, len:%d/0x%x",
                     __func__, p_msg->hdr.ctype, p_result->pdu, len, len);

    if(p_msg->hdr.ctype == AVRC_RSP_REJ) {
        p_result->rsp.status = *p;
        return p_result->rsp.status;
    }

    switch(p_result->pdu) {
            /* case AVRC_PDU_REQUEST_CONTINUATION_RSP: 0x40 */
            /* case AVRC_PDU_ABORT_CONTINUATION_RSP:   0x41 */
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
        case AVRC_PDU_SET_ABSOLUTE_VOLUME:      /* 0x50 */
            if(len != 1) {
                status = AVRC_STS_INTERNAL_ERR;
            } else {
                BE_STREAM_TO_UINT8(p_result->volume.volume, p);
            }

            break;
#endif /* (AVRC_ADV_CTRL_INCLUDED == TRUE) */

        case AVRC_PDU_REGISTER_NOTIFICATION:    /* 0x31 */
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
            BE_STREAM_TO_UINT8(eventid, p);

            if(AVRC_EVT_VOLUME_CHANGE == eventid
                    && (AVRC_RSP_CHANGED == p_msg->hdr.ctype || AVRC_RSP_INTERIM == p_msg->hdr.ctype
                        || AVRC_RSP_REJ == p_msg->hdr.ctype || AVRC_RSP_NOT_IMPL == p_msg->hdr.ctype)) {
                p_result->reg_notif.status = p_msg->hdr.ctype;
                p_result->reg_notif.event_id = eventid;
                BE_STREAM_TO_UINT8(p_result->reg_notif.param.volume, p);
            }

            AVRC_TRACE_DEBUG("%s PDU reg notif response:event %x, volume %x",
                             __func__, eventid, p_result->reg_notif.param.volume);
#endif /* (AVRC_ADV_CTRL_INCLUDED == TRUE) */
            break;

        default:
            status = AVRC_STS_BAD_CMD;
            break;
    }

    return status;
}

void avrc_parse_notification_rsp(uint8_t *p_stream, tAVRC_REG_NOTIF_RSP *p_rsp)
{
    BE_STREAM_TO_UINT8(p_rsp->event_id, p_stream);

    switch(p_rsp->event_id) {
        case AVRC_EVT_PLAY_STATUS_CHANGE:
            BE_STREAM_TO_UINT8(p_rsp->param.play_status, p_stream);
            break;

        case AVRC_EVT_TRACK_CHANGE:
            BE_STREAM_TO_ARRAY(p_stream, p_rsp->param.track, 8);
            break;

        case AVRC_EVT_APP_SETTING_CHANGE:
            BE_STREAM_TO_UINT8(p_rsp->param.player_setting.num_attr, p_stream);

            for(int index = 0; index < p_rsp->param.player_setting.num_attr; index++) {
                BE_STREAM_TO_UINT8(p_rsp->param.player_setting.attr_id[index], p_stream);
                BE_STREAM_TO_UINT8(p_rsp->param.player_setting.attr_value[index], p_stream);
            }

            break;

        case AVRC_EVT_NOW_PLAYING_CHANGE:
            break;

        case AVRC_EVT_AVAL_PLAYERS_CHANGE:
            break;

        case AVRC_EVT_ADDR_PLAYER_CHANGE:
            break;

        case AVRC_EVT_UIDS_CHANGE:
            break;

        case AVRC_EVT_TRACK_REACHED_END:
        case AVRC_EVT_TRACK_REACHED_START:
        case AVRC_EVT_PLAY_POS_CHANGED:
        case AVRC_EVT_BATTERY_STATUS_CHANGE:
        case AVRC_EVT_SYSTEM_STATUS_CHANGE:
        default:
            break;
    }
}

#if (AVRC_CTLR_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         avrc_ctrl_pars_vendor_rsp
**
** Description      This function parses the vendor specific commands defined by
**                  Bluetooth SIG
**
** Returns          AVRC_STS_NO_ERROR, if the message in p_data is parsed successfully.
**                  Otherwise, the error code defined by AVRCP 1.4
**
*******************************************************************************/
static tAVRC_STS avrc_ctrl_pars_vendor_rsp(
                tAVRC_MSG_VENDOR *p_msg, tAVRC_RESPONSE *p_result, uint8_t *p_buf, uint16_t *buf_len)
{
    uint8_t   *p = p_msg->p_vendor_data;
    BE_STREAM_TO_UINT8(p_result->pdu, p);
    p++; /* skip the reserved/packe_type byte */
    uint16_t  len;
    BE_STREAM_TO_UINT16(len, p);
    AVRC_TRACE_DEBUG("%s ctype:0x%x pdu:0x%x, len:%d",
                     __func__, p_msg->hdr.ctype, p_result->pdu, len);

    /* Todo: Issue in handling reject, check */
    if(p_msg->hdr.ctype == AVRC_RSP_REJ) {
        p_result->rsp.status = *p;
        return p_result->rsp.status;
    }

    /* TODO: Break the big switch into functions. */
    switch(p_result->pdu) {
        /* case AVRC_PDU_REQUEST_CONTINUATION_RSP: 0x40 */
        /* case AVRC_PDU_ABORT_CONTINUATION_RSP:   0x41 */
        case AVRC_PDU_REGISTER_NOTIFICATION:
            avrc_parse_notification_rsp(p, &p_result->reg_notif);
            break;

        case AVRC_PDU_GET_CAPABILITIES:
            if(len == 0) {
                p_result->get_caps.count = 0;
                p_result->get_caps.capability_id = 0;
                break;
            }

            BE_STREAM_TO_UINT8(p_result->get_caps.capability_id, p);
            BE_STREAM_TO_UINT8(p_result->get_caps.count, p);
            AVRC_TRACE_DEBUG("%s cap id = %d, cap_count = %d ",
                             __func__, p_result->get_caps.capability_id, p_result->get_caps.count);

            if(p_result->get_caps.capability_id == AVRC_CAP_COMPANY_ID) {
                for(int xx = 0; ((xx < p_result->get_caps.count) && (xx < AVRC_CAP_MAX_NUM_COMP_ID));
                        xx++) {
                    BE_STREAM_TO_UINT24(p_result->get_caps.param.company_id[xx], p);
                }
            } else if(p_result->get_caps.capability_id == AVRC_CAP_EVENTS_SUPPORTED) {
                for(int xx = 0; ((xx < p_result->get_caps.count) && (xx < AVRC_CAP_MAX_NUM_EVT_ID));
                        xx++) {
                    BE_STREAM_TO_UINT8(p_result->get_caps.param.event_id[xx], p);
                }
            }

            break;

        case AVRC_PDU_LIST_PLAYER_APP_ATTR:
            if(len == 0) {
                p_result->list_app_attr.num_attr = 0;
                break;
            }

            BE_STREAM_TO_UINT8(p_result->list_app_attr.num_attr, p);
            AVRC_TRACE_DEBUG("%s attr count = %d ", __func__, p_result->list_app_attr.num_attr);

            for(int xx = 0; xx < p_result->list_app_attr.num_attr; xx++) {
                BE_STREAM_TO_UINT8(p_result->list_app_attr.attrs[xx], p);
            }

            break;

        case AVRC_PDU_LIST_PLAYER_APP_VALUES:
            if(len == 0) {
                p_result->list_app_values.num_val = 0;
                break;
            }

            BE_STREAM_TO_UINT8(p_result->list_app_values.num_val, p);
            AVRC_TRACE_DEBUG("%s value count = %d ", __func__, p_result->list_app_values.num_val);

            for(int xx = 0; xx < p_result->list_app_values.num_val; xx++) {
                BE_STREAM_TO_UINT8(p_result->list_app_values.vals[xx], p);
            }

            break;

        case AVRC_PDU_GET_CUR_PLAYER_APP_VALUE: {
            if(len == 0) {
                p_result->get_cur_app_val.num_val = 0;
                break;
            }

            BE_STREAM_TO_UINT8(p_result->get_cur_app_val.num_val, p);
            tAVRC_APP_SETTING *app_sett =
                            (tAVRC_APP_SETTING *)GKI_getbuf(p_result->get_cur_app_val.num_val * sizeof(tAVRC_APP_SETTING));
            AVRC_TRACE_DEBUG("%s attr count = %d ", __func__, p_result->get_cur_app_val.num_val);

            for(int xx = 0; xx < p_result->get_cur_app_val.num_val; xx++) {
                BE_STREAM_TO_UINT8(app_sett[xx].attr_id, p);
                BE_STREAM_TO_UINT8(app_sett[xx].attr_val, p);
            }

            p_result->get_cur_app_val.p_vals = app_sett;
        }
        break;

        case AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT: {
           // tAVRC_APP_SETTING_TEXT   *p_setting_text;
            uint8_t                    num_attrs;

            if(len == 0) {
                p_result->get_app_attr_txt.num_attr = 0;
                break;
            }

            BE_STREAM_TO_UINT8(num_attrs, p);
            AVRC_TRACE_DEBUG("%s attr count = %d ", __func__, p_result->get_app_attr_txt.num_attr);
            p_result->get_app_attr_txt.num_attr = num_attrs;
           // p_setting_text = (tAVRC_APP_SETTING_TEXT *)GKI_getbuf(num_attrs * sizeof(tAVRC_APP_SETTING_TEXT));

            for(int xx = 0; xx < num_attrs; xx++) {
                BE_STREAM_TO_UINT8(p_result->get_app_attr_txt.p_attrs[xx].attr_id, p);
                BE_STREAM_TO_UINT16(p_result->get_app_attr_txt.p_attrs[xx].charset_id, p);
                BE_STREAM_TO_UINT8(p_result->get_app_attr_txt.p_attrs[xx].str_len, p);

                if(p_result->get_app_attr_txt.p_attrs[xx].str_len != 0) {
                    uint8_t *p_str = (uint8_t *)GKI_getbuf(p_result->get_app_attr_txt.p_attrs[xx].str_len);
                    BE_STREAM_TO_ARRAY(p, p_str, p_result->get_app_attr_txt.p_attrs[xx].str_len);
                    p_result->get_app_attr_txt.p_attrs[xx].p_str = p_str;
                } else {
                    p_result->get_app_attr_txt.p_attrs[xx].p_str = NULL;
                }
            }
        }
        break;

        case AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT: {
           // tAVRC_APP_SETTING_TEXT   *p_setting_text;
            uint8_t                    num_vals;

            if(len == 0) {
                p_result->get_app_val_txt.num_attr = 0;
                break;
            }

            BE_STREAM_TO_UINT8(num_vals, p);
            p_result->get_app_val_txt.num_attr = num_vals;
            AVRC_TRACE_DEBUG("%s value count = %d ", __func__, p_result->get_app_val_txt.num_attr);
         //   p_setting_text = (tAVRC_APP_SETTING_TEXT *)GKI_getbuf(num_vals * sizeof(tAVRC_APP_SETTING_TEXT));

            for(int i = 0; i < num_vals; i++) {
                BE_STREAM_TO_UINT8(p_result->get_app_val_txt.p_attrs[i].attr_id, p);
                BE_STREAM_TO_UINT16(p_result->get_app_val_txt.p_attrs[i].charset_id, p);
                BE_STREAM_TO_UINT8(p_result->get_app_val_txt.p_attrs[i].str_len, p);

                if(p_result->get_app_val_txt.p_attrs[i].str_len != 0) {
                    uint8_t *p_str = (uint8_t *)GKI_getbuf(p_result->get_app_val_txt.p_attrs[i].str_len);
                    BE_STREAM_TO_ARRAY(p, p_str, p_result->get_app_val_txt.p_attrs[i].str_len);
                    p_result->get_app_val_txt.p_attrs[i].p_str = p_str;
                } else {
                    p_result->get_app_val_txt.p_attrs[i].p_str = NULL;
                }
            }
        }
        break;

        case AVRC_PDU_SET_PLAYER_APP_VALUE:
            /* nothing comes as part of this rsp */
            break;

        case AVRC_PDU_GET_ELEMENT_ATTR: {
            uint8_t               num_attrs;

            if(len <= 0) {
                p_result->get_elem_attrs.num_attr = 0;
                break;
            }

            BE_STREAM_TO_UINT8(num_attrs, p);
            p_result->get_elem_attrs.num_attr = num_attrs;

            if(num_attrs) {
                tAVRC_ATTR_ENTRY *p_attrs =
                                (tAVRC_ATTR_ENTRY *)GKI_getbuf(num_attrs * sizeof(tAVRC_ATTR_ENTRY));

                for(int i = 0; i < num_attrs; i++) {
                    BE_STREAM_TO_UINT32(p_attrs[i].attr_id, p);
                    BE_STREAM_TO_UINT16(p_attrs[i].name.charset_id, p);
                    BE_STREAM_TO_UINT16(p_attrs[i].name.str_len, p);

                    if(p_attrs[i].name.str_len > 0) {
                        p_attrs[i].name.p_str = (uint8_t *)GKI_getbuf(p_attrs[i].name.str_len);
                        BE_STREAM_TO_ARRAY(p, p_attrs[i].name.p_str, p_attrs[i].name.str_len);
                    }
                }

                p_result->get_elem_attrs.p_attrs = p_attrs;
            }
        }
        break;

        case AVRC_PDU_GET_PLAY_STATUS:
            if(len == 0) {
                break;
            }

            BE_STREAM_TO_UINT32(p_result->get_play_status.song_len, p);
            BE_STREAM_TO_UINT32(p_result->get_play_status.song_pos, p);
            BE_STREAM_TO_UINT8(p_result->get_play_status.status, p);
            break;

        default:
            return AVRC_STS_BAD_CMD;
    }

    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         AVRC_Ctrl_ParsResponse
**
** Description      This function is a parse response for AVRCP Controller.
**
** Returns          AVRC_STS_NO_ERROR, if the message in p_data is parsed successfully.
**                  Otherwise, the error code defined by AVRCP 1.4
**
*******************************************************************************/
tAVRC_STS AVRC_Ctrl_ParsResponse(tAVRC_MSG *p_msg, tAVRC_RESPONSE *p_result, uint8_t *p_buf,
                                 uint16_t *buf_len)
{
    tAVRC_STS  status = AVRC_STS_INTERNAL_ERR;

    if(p_msg && p_result) {
        switch(p_msg->hdr.opcode) {
            case AVRC_OP_VENDOR:     /*  0x00    Vendor-dependent commands */
                status = avrc_ctrl_pars_vendor_rsp(&p_msg->vendor, p_result, p_buf, buf_len);
                break;

            default:
                AVRC_TRACE_ERROR("%s unknown opcode:0x%x", __func__, p_msg->hdr.opcode);
                break;
        }

        p_result->rsp.opcode = p_msg->hdr.opcode;
        p_result->rsp.status = status;
    }

    return status;
}
#endif /* (AVRC_CTRL_INCLUDED) == TRUE) */
/*******************************************************************************
**
** Function         AVRC_ParsResponse
**
** Description      This function is a superset of AVRC_ParsMetadata to parse the response.
**
** Returns          AVRC_STS_NO_ERROR, if the message in p_data is parsed successfully.
**                  Otherwise, the error code defined by AVRCP 1.4
**
*******************************************************************************/
tAVRC_STS AVRC_ParsResponse(tAVRC_MSG *p_msg, tAVRC_RESPONSE *p_result, uint8_t *p_buf,
                            uint16_t buf_len)
{
    tAVRC_STS  status = AVRC_STS_INTERNAL_ERR;
    uint16_t  id;
    UNUSED(p_buf);
    UNUSED(buf_len);

    if(p_msg && p_result) {
        switch(p_msg->hdr.opcode) {
            case AVRC_OP_VENDOR:     /*  0x00    Vendor-dependent commands */
                status = avrc_pars_vendor_rsp(&p_msg->vendor, p_result);
                break;

            case AVRC_OP_PASS_THRU:  /*  0x7C    panel subunit opcode */
                status = avrc_pars_pass_thru(&p_msg->pass, &id);

                if(status == AVRC_STS_NO_ERROR) {
                    p_result->pdu = (uint8_t)id;
                }

                break;

            default:
                AVRC_TRACE_ERROR("%s unknown opcode:0x%x", __func__, p_msg->hdr.opcode);
                break;
        }

        p_result->rsp.opcode = p_msg->hdr.opcode;
        p_result->rsp.status = status;
    }

    return status;
}
#endif /* (AVRC_METADATA_INCLUDED == TRUE) */
