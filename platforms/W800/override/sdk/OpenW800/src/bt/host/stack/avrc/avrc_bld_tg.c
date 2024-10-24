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
** Function         avrc_bld_get_capability_rsp
**
** Description      This function builds the Get Capability response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_capability_rsp(tAVRC_GET_CAPS_RSP *p_rsp, BT_HDR *p_pkt)
{
    uint8_t   *p_data, *p_start, *p_len, *p_count;
    uint16_t  len = 0;
    uint8_t   xx;
    uint32_t  *p_company_id;
    uint8_t   *p_event_id;
    tAVRC_STS status = AVRC_STS_NO_ERROR;

    if(!(AVRC_IS_VALID_CAP_ID(p_rsp->capability_id))) {
        AVRC_TRACE_ERROR("%s bad parameter. p_rsp: %x", __func__, p_rsp);
        status = AVRC_STS_BAD_PARAM;
        return status;
    }

    AVRC_TRACE_API("%s", __func__);
    /* get the existing length, if any, and also the num attributes */
    p_start = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_len = p_start + 2; /* pdu + rsvd */
    BE_STREAM_TO_UINT16(len, p_data);
    UINT8_TO_BE_STREAM(p_data, p_rsp->capability_id);
    p_count = p_data;

    if(len == 0) {
        *p_count = p_rsp->count;
        p_data++;
        len = 2; /* move past the capability_id and count */
    } else {
        p_data = p_start + p_pkt->len;
        *p_count += p_rsp->count;
    }

    if(p_rsp->capability_id == AVRC_CAP_COMPANY_ID) {
        p_company_id = p_rsp->param.company_id;

        for(xx = 0; xx < p_rsp->count; xx++) {
            UINT24_TO_BE_STREAM(p_data, p_company_id[xx]);
        }

        len += p_rsp->count * 3;
    } else {
        p_event_id = p_rsp->param.event_id;
        *p_count = 0;

        for(xx = 0; xx < p_rsp->count; xx++) {
            if(AVRC_IS_VALID_EVENT_ID(p_event_id[xx])) {
                (*p_count)++;
                UINT8_TO_BE_STREAM(p_data, p_event_id[xx]);
            }
        }

        len += (*p_count);
    }

    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);
    status = AVRC_STS_NO_ERROR;
    return status;
}

/*******************************************************************************
**
** Function         avrc_bld_list_app_settings_attr_rsp
**
** Description      This function builds the List Application Settings Attribute
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_list_app_settings_attr_rsp(tAVRC_LIST_APP_ATTR_RSP *p_rsp, BT_HDR *p_pkt)
{
    uint8_t   *p_data, *p_start, *p_len, *p_num;
    uint16_t  len = 0;
    uint8_t   xx;
    AVRC_TRACE_API("%s", __func__);
    /* get the existing length, if any, and also the num attributes */
    p_start = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_len = p_start + 2; /* pdu + rsvd */
    BE_STREAM_TO_UINT16(len, p_data);
    p_num = p_data;

    if(len == 0) {
        /* first time initialize the attribute count */
        *p_num = 0;
        p_data++;
    } else {
        p_data = p_start + p_pkt->len;
    }

    for(xx = 0; xx < p_rsp->num_attr; xx++) {
        if(AVRC_IsValidPlayerAttr(p_rsp->attrs[xx])) {
            (*p_num)++;
            UINT8_TO_BE_STREAM(p_data, p_rsp->attrs[xx]);
        }
    }

    len = *p_num + 1;
    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_list_app_settings_values_rsp
**
** Description      This function builds the List Application Setting Values
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_list_app_settings_values_rsp(tAVRC_LIST_APP_VALUES_RSP *p_rsp,
        BT_HDR *p_pkt)
{
    uint8_t   *p_data, *p_start, *p_len, *p_num;
    uint8_t   xx;
    uint16_t  len;
    AVRC_TRACE_API("%s", __func__);
    p_start = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_len = p_start + 2; /* pdu + rsvd */
    /* get the existing length, if any, and also the num attributes */
    BE_STREAM_TO_UINT16(len, p_data);
    p_num = p_data;

    /* first time initialize the attribute count */
    if(len == 0) {
        *p_num = p_rsp->num_val;
        p_data++;
    } else {
        p_data = p_start + p_pkt->len;
        *p_num += p_rsp->num_val;
    }

    for(xx = 0; xx < p_rsp->num_val; xx++) {
        UINT8_TO_BE_STREAM(p_data, p_rsp->vals[xx]);
    }

    len = *p_num + 1;
    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_get_cur_app_setting_value_rsp
**
** Description      This function builds the Get Current Application Setting Value
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_cur_app_setting_value_rsp(tAVRC_GET_CUR_APP_VALUE_RSP *p_rsp,
        BT_HDR *p_pkt)
{
    uint8_t   *p_data, *p_start, *p_len, *p_count;
    uint16_t  len;
    uint8_t   xx;

    if(!p_rsp->p_vals) {
        AVRC_TRACE_ERROR("%s NULL parameter", __func__);
        return AVRC_STS_BAD_PARAM;
    }

    AVRC_TRACE_API("%s", __func__);
    /* get the existing length, if any, and also the num attributes */
    p_start = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_len = p_start + 2; /* pdu + rsvd */
    BE_STREAM_TO_UINT16(len, p_data);
    p_count = p_data;

    if(len == 0) {
        /* first time initialize the attribute count */
        *p_count = 0;
        p_data++;
    } else {
        p_data = p_start + p_pkt->len;
    }

    for(xx = 0; xx < p_rsp->num_val; xx++) {
        if(avrc_is_valid_player_attrib_value(p_rsp->p_vals[xx].attr_id, p_rsp->p_vals[xx].attr_val)) {
            (*p_count)++;
            UINT8_TO_BE_STREAM(p_data, p_rsp->p_vals[xx].attr_id);
            UINT8_TO_BE_STREAM(p_data, p_rsp->p_vals[xx].attr_val);
        }
    }

    len = ((*p_count) << 1) + 1;
    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_set_app_setting_value_rsp
**
** Description      This function builds the Set Application Setting Value
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_set_app_setting_value_rsp(tAVRC_RSP *p_rsp, BT_HDR *p_pkt)
{
    UNUSED(p_rsp);
    UNUSED(p_pkt);
    /* nothing to be added. */
    AVRC_TRACE_API("%s", __func__);
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_app_setting_text_rsp
**
** Description      This function builds the Get Application Settings Attribute Text
**                  or Get Application Settings Value Text response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_app_setting_text_rsp(tAVRC_GET_APP_ATTR_TXT_RSP *p_rsp, BT_HDR *p_pkt)
{
    uint8_t   *p_data, *p_start, *p_len, *p_count;
    uint16_t  len, len_left;
    uint8_t   xx;
    tAVRC_STS   sts = AVRC_STS_NO_ERROR;
    uint8_t       num_added = 0;

    if(!p_rsp->p_attrs) {
        AVRC_TRACE_ERROR("%s NULL parameter", __func__);
        return AVRC_STS_BAD_PARAM;
    }

    /* get the existing length, if any, and also the num attributes */
    p_start = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_len = p_start + 2; /* pdu + rsvd */
    /*
     * NOTE: The buffer is allocated within avrc_bld_init_rsp_buffer(), and is
     * always of size BT_DEFAULT_BUFFER_SIZE.
     */
    len_left = BT_DEFAULT_BUFFER_SIZE - BT_HDR_SIZE - p_pkt->offset - p_pkt->len;
    BE_STREAM_TO_UINT16(len, p_data);
    p_count = p_data;

    if(len == 0) {
        *p_count = 0;
        p_data++;
    } else {
        p_data = p_start + p_pkt->len;
    }

    for(xx = 0; xx < p_rsp->num_attr; xx++) {
        if(len_left < (p_rsp->p_attrs[xx].str_len + 4)) {
            AVRC_TRACE_ERROR("%s out of room (str_len:%d, left:%d)",
                             __func__, xx, p_rsp->p_attrs[xx].str_len, len_left);
            p_rsp->num_attr = num_added;
            sts = AVRC_STS_INTERNAL_ERR;
            break;
        }

        if(!p_rsp->p_attrs[xx].str_len || !p_rsp->p_attrs[xx].p_str) {
            AVRC_TRACE_ERROR("%s NULL attr text[%d]", __func__, xx);
            continue;
        }

        UINT8_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].attr_id);
        UINT16_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].charset_id);
        UINT8_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].str_len);
        ARRAY_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].p_str, p_rsp->p_attrs[xx].str_len);
        (*p_count)++;
        num_added++;
    }

    len = p_data - p_count;
    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);
    return sts;
}

/*******************************************************************************
**
** Function         avrc_bld_get_app_setting_attr_text_rsp
**
** Description      This function builds the Get Application Setting Attribute Text
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_app_setting_attr_text_rsp(tAVRC_GET_APP_ATTR_TXT_RSP *p_rsp,
        BT_HDR *p_pkt)
{
    AVRC_TRACE_API("%s", __func__);
    return avrc_bld_app_setting_text_rsp(p_rsp, p_pkt);
}

/*******************************************************************************
**
** Function         avrc_bld_get_app_setting_value_text_rsp
**
** Description      This function builds the Get Application Setting Value Text
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_app_setting_value_text_rsp(tAVRC_GET_APP_ATTR_TXT_RSP *p_rsp,
        BT_HDR *p_pkt)
{
    AVRC_TRACE_API("%s", __func__);
    return avrc_bld_app_setting_text_rsp(p_rsp, p_pkt);
}

/*******************************************************************************
**
** Function         avrc_bld_inform_charset_rsp
**
** Description      This function builds the Inform Displayable Character Set
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_inform_charset_rsp(tAVRC_RSP *p_rsp, BT_HDR *p_pkt)
{
    UNUSED(p_rsp);
    UNUSED(p_pkt);
    /* nothing to be added. */
    AVRC_TRACE_API("%s", __func__);
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_inform_battery_status_rsp
**
** Description      This function builds the Inform Battery Status
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_inform_battery_status_rsp(tAVRC_RSP *p_rsp, BT_HDR *p_pkt)
{
    UNUSED(p_rsp);
    UNUSED(p_pkt);
    /* nothing to be added. */
    AVRC_TRACE_API("%s", __func__);
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_get_elem_attrs_rsp
**
** Description      This function builds the Get Element Attributes
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_elem_attrs_rsp(tAVRC_GET_ELEM_ATTRS_RSP *p_rsp, BT_HDR *p_pkt)
{
    uint8_t   *p_data, *p_start, *p_len, *p_count;
    uint16_t  len;
    uint8_t   xx;
    AVRC_TRACE_API("%s", __func__);

    if(!p_rsp->p_attrs) {
        AVRC_TRACE_ERROR("%s NULL parameter", __func__);
        return AVRC_STS_BAD_PARAM;
    }

    /* get the existing length, if any, and also the num attributes */
    p_start = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_len = p_start + 2; /* pdu + rsvd */
    BE_STREAM_TO_UINT16(len, p_data);
    p_count = p_data;

    if(len == 0) {
        *p_count = 0;
        p_data++;
    } else {
        p_data = p_start + p_pkt->len;
    }

    for(xx = 0; xx < p_rsp->num_attr; xx++) {
        if(!AVRC_IS_VALID_MEDIA_ATTRIBUTE(p_rsp->p_attrs[xx].attr_id)) {
            AVRC_TRACE_ERROR("%s invalid attr id[%d]: %d",
                             __func__, xx, p_rsp->p_attrs[xx].attr_id);
            continue;
        }

        if(!p_rsp->p_attrs[xx].name.p_str) {
            p_rsp->p_attrs[xx].name.str_len = 0;
        }

        UINT32_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].attr_id);
        UINT16_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].name.charset_id);
        UINT16_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].name.str_len);
        ARRAY_TO_BE_STREAM(p_data, p_rsp->p_attrs[xx].name.p_str, p_rsp->p_attrs[xx].name.str_len);
        (*p_count)++;
    }

    len = p_data - p_count;
    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_get_play_status_rsp
**
** Description      This function builds the Get Play Status
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_get_play_status_rsp(tAVRC_GET_PLAY_STATUS_RSP *p_rsp, BT_HDR *p_pkt)
{
    uint8_t   *p_data, *p_start;
    AVRC_TRACE_API("avrc_bld_get_play_status_rsp");
    p_start = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_start + 2;
    /* add fixed lenth - song len(4) + song position(4) + status(1) */
    UINT16_TO_BE_STREAM(p_data, 9);
    UINT32_TO_BE_STREAM(p_data, p_rsp->song_len);
    UINT32_TO_BE_STREAM(p_data, p_rsp->song_pos);
    UINT8_TO_BE_STREAM(p_data, p_rsp->play_status);
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_notify_rsp
**
** Description      This function builds the Notification response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_notify_rsp(tAVRC_REG_NOTIF_RSP *p_rsp, BT_HDR *p_pkt)
{
    uint8_t   *p_data, *p_start;
    uint8_t   *p_len;
    uint16_t  len = 0;
    uint8_t   xx;
    tAVRC_STS status = AVRC_STS_NO_ERROR;
    AVRC_TRACE_API("%s event_id %d", __func__, p_rsp->event_id);
    p_start = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
    p_data = p_len = p_start + 2; /* pdu + rsvd */
    p_data += 2;
    UINT8_TO_BE_STREAM(p_data, p_rsp->event_id);

    switch(p_rsp->event_id) {
        case AVRC_EVT_PLAY_STATUS_CHANGE:       /* 0x01 */

            /* p_rsp->param.play_status >= AVRC_PLAYSTATE_STOPPED is always TRUE */
            if((p_rsp->param.play_status <= AVRC_PLAYSTATE_REV_SEEK) ||
                    (p_rsp->param.play_status == AVRC_PLAYSTATE_ERROR)) {
                UINT8_TO_BE_STREAM(p_data, p_rsp->param.play_status);
                len = 2;
            } else {
                AVRC_TRACE_ERROR("%s bad play state", __func__);
                status = AVRC_STS_BAD_PARAM;
            }

            break;

        case AVRC_EVT_TRACK_CHANGE:             /* 0x02 */
            ARRAY_TO_BE_STREAM(p_data, p_rsp->param.track, AVRC_UID_SIZE);
            len = (uint8_t)(AVRC_UID_SIZE + 1);
            break;

        case AVRC_EVT_TRACK_REACHED_END:        /* 0x03 */
        case AVRC_EVT_TRACK_REACHED_START:      /* 0x04 */
            len = 1;
            break;

        case AVRC_EVT_PLAY_POS_CHANGED:         /* 0x05 */
            UINT32_TO_BE_STREAM(p_data, p_rsp->param.play_pos);
            len = 5;
            break;

        case AVRC_EVT_BATTERY_STATUS_CHANGE:    /* 0x06 */
            if(AVRC_IS_VALID_BATTERY_STATUS(p_rsp->param.battery_status)) {
                UINT8_TO_BE_STREAM(p_data, p_rsp->param.battery_status);
                len = 2;
            } else {
                AVRC_TRACE_ERROR("%s bad battery status", __func__);
                status = AVRC_STS_BAD_PARAM;
            }

            break;

        case AVRC_EVT_SYSTEM_STATUS_CHANGE:     /* 0x07 */
            if(AVRC_IS_VALID_SYSTEM_STATUS(p_rsp->param.system_status)) {
                UINT8_TO_BE_STREAM(p_data, p_rsp->param.system_status);
                len = 2;
            } else {
                AVRC_TRACE_ERROR("%s bad system status", __func__);
                status = AVRC_STS_BAD_PARAM;
            }

            break;

        case AVRC_EVT_APP_SETTING_CHANGE:       /* 0x08 */
            if(p_rsp->param.player_setting.num_attr > AVRC_MAX_APP_SETTINGS) {
                p_rsp->param.player_setting.num_attr = AVRC_MAX_APP_SETTINGS;
            }

            if(p_rsp->param.player_setting.num_attr > 0) {
                UINT8_TO_BE_STREAM(p_data, p_rsp->param.player_setting.num_attr);
                len = 2;

                for(xx = 0; xx < p_rsp->param.player_setting.num_attr; xx++) {
                    if(avrc_is_valid_player_attrib_value(p_rsp->param.player_setting.attr_id[xx],
                                                         p_rsp->param.player_setting.attr_value[xx])) {
                        UINT8_TO_BE_STREAM(p_data, p_rsp->param.player_setting.attr_id[xx]);
                        UINT8_TO_BE_STREAM(p_data, p_rsp->param.player_setting.attr_value[xx]);
                    } else {
                        AVRC_TRACE_ERROR("%s bad player app seeting attribute or value", __func__);
                        status = AVRC_STS_BAD_PARAM;
                        break;
                    }

                    len += 2;
                }
            } else {
                status = AVRC_STS_BAD_PARAM;
            }

            break;

        case AVRC_EVT_VOLUME_CHANGE:
            len = 2;
            UINT8_TO_BE_STREAM(p_data, (AVRC_MAX_VOLUME & p_rsp->param.volume));
            break;

        default:
            status = AVRC_STS_BAD_PARAM;
            AVRC_TRACE_ERROR("%s unknown event_id", __func__);
    }

    UINT16_TO_BE_STREAM(p_len, len);
    p_pkt->len = (p_data - p_start);
    return status;
}

/*******************************************************************************
**
** Function         avrc_bld_next_rsp
**
** Description      This function builds the Request Continue or Abort
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
static tAVRC_STS avrc_bld_next_rsp(tAVRC_RSP *p_rsp, BT_HDR *p_pkt)
{
    UNUSED(p_rsp);
    UNUSED(p_pkt);
    /* nothing to be added. */
    AVRC_TRACE_API("%s", __func__);
    return AVRC_STS_NO_ERROR;
}

/*****************************************************************************
**
** Function      avrc_bld_set_address_player_rsp
**
** Description   This function builds the set address player response
**
** Returns       AVRC_STS_NO_ERROR
**
******************************************************************************/
static tAVRC_STS avrc_bld_set_address_player_rsp(tAVRC_RSP *p_rsp, BT_HDR *p_pkt)
{
    AVRC_TRACE_API("%s", __func__);
    uint8_t *p_start = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
    /* To calculate length */
    uint8_t *p_data = p_start + 2;
    /* add fixed lenth status(1) */
    UINT16_TO_BE_STREAM(p_data, 1);
    UINT8_TO_BE_STREAM(p_data, p_rsp->status);
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}

/*****************************************************************************
**
** Function      avrc_bld_play_item_rsp
**
** Description   This function builds the play item response
**
** Returns       AVRC_STS_NO_ERROR, if the response is build successfully
**
******************************************************************************/
static tAVRC_STS avrc_bld_play_item_rsp(tAVRC_RSP *p_rsp, BT_HDR *p_pkt)
{
    AVRC_TRACE_API("%s", __func__);
    uint8_t *p_start = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
    /* To calculate length */
    uint8_t *p_data = p_start + 2;
    /* add fixed lenth status(1) */
    UINT16_TO_BE_STREAM(p_data, 1);
    UINT8_TO_BE_STREAM(p_data, p_rsp->status);
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}

/*****************************************************************************
**
** Function      avrc_bld_set_absolute_volume_rsp
**
** Description   This function builds the set absolute volume response
**
** Returns       AVRC_STS_NO_ERROR, if the response is build successfully
**
******************************************************************************/
static tAVRC_STS avrc_bld_set_absolute_volume_rsp(uint8_t abs_vol, BT_HDR *p_pkt)
{
    AVRC_TRACE_API("%s", __func__);
    uint8_t *p_start = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
    /* To calculate length */
    uint8_t *p_data = p_start + 2;
    /* add fixed lenth status(1) */
    UINT16_TO_BE_STREAM(p_data, 1);
    UINT8_TO_BE_STREAM(p_data, abs_vol);
    p_pkt->len = (p_data - p_start);
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_group_navigation_rsp
**
** Description      This function builds the Group Navigation
**                  response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
tAVRC_STS avrc_bld_group_navigation_rsp(uint16_t navi_id, BT_HDR *p_pkt)
{
    if(!AVRC_IS_VALID_GROUP(navi_id)) {
        AVRC_TRACE_ERROR("%s bad navigation op id: %d", __func__, navi_id);
        return AVRC_STS_BAD_PARAM;
    }

    AVRC_TRACE_API("%s", __func__);
    uint8_t *p_data = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
    UINT16_TO_BE_STREAM(p_data, navi_id);
    p_pkt->len = 2;
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_rejected_rsp
**
** Description      This function builds the General Response response.
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**
*******************************************************************************/
static tAVRC_STS avrc_bld_rejected_rsp(tAVRC_RSP *p_rsp, BT_HDR *p_pkt)
{
    AVRC_TRACE_API("%s: status=%d, pdu:x%x", __func__, p_rsp->status, p_rsp->pdu);
    uint8_t *p_start = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
    uint8_t *p_data = p_start + 2;
    AVRC_TRACE_DEBUG("%s pdu:x%x", __func__, *p_start);
    UINT16_TO_BE_STREAM(p_data, 1);
    UINT8_TO_BE_STREAM(p_data, p_rsp->status);
    p_pkt->len = p_data - p_start;
    return AVRC_STS_NO_ERROR;
}

/*******************************************************************************
**
** Function         avrc_bld_init_rsp_buffer
**
** Description      This function initializes the response buffer based on PDU
**
** Returns          NULL, if no GKI buffer or failure to build the message.
**                  Otherwise, the GKI buffer that contains the initialized message.
**
*******************************************************************************/
static BT_HDR *avrc_bld_init_rsp_buffer(tAVRC_RESPONSE *p_rsp)
{
    uint16_t offset = AVRC_MSG_PASS_THRU_OFFSET;
    uint16_t chnl = AVCT_DATA_CTRL;
    uint8_t  opcode = avrc_opcode_from_pdu(p_rsp->pdu);
    AVRC_TRACE_API("%s: pdu=%x, opcode=%x/%x", __func__, p_rsp->pdu, opcode, p_rsp->rsp.opcode);

    if(opcode != p_rsp->rsp.opcode && p_rsp->rsp.status != AVRC_STS_NO_ERROR &&
            avrc_is_valid_opcode(p_rsp->rsp.opcode)) {
        opcode = p_rsp->rsp.opcode;
        AVRC_TRACE_API("%s opcode=%x", __func__, opcode);
    }

    switch(opcode) {
        case AVRC_OP_PASS_THRU:
            offset = AVRC_MSG_PASS_THRU_OFFSET;
            break;

        case AVRC_OP_VENDOR:
            offset = AVRC_MSG_VENDOR_OFFSET;
            break;
    }

    /* allocate and initialize the buffer */
    BT_HDR *p_pkt = (BT_HDR *)GKI_getbuf(BT_DEFAULT_BUFFER_SIZE);
    uint8_t *p_data, *p_start;
    p_pkt->layer_specific = chnl;
    p_pkt->event    = opcode;
    p_pkt->offset   = offset;
    p_data = (uint8_t *)(p_pkt + 1) + p_pkt->offset;
    p_start = p_data;

    /* pass thru - group navigation - has a two byte op_id, so dont do it here */
    if(opcode != AVRC_OP_PASS_THRU) {
        *p_data++ = p_rsp->pdu;
    }

    switch(opcode) {
        case AVRC_OP_VENDOR:
            /* reserved 0, packet_type 0 */
            UINT8_TO_BE_STREAM(p_data, 0);
            /* continue to the next "case to add length */
            /* add fixed lenth - 0 */
            UINT16_TO_BE_STREAM(p_data, 0);
            break;
    }

    p_pkt->len = (p_data - p_start);
    p_rsp->rsp.opcode = opcode;
    return p_pkt;
}

/*******************************************************************************
**
** Function         AVRC_BldResponse
**
** Description      This function builds the given AVRCP response to the given
**                  GKI buffer
**
** Returns          AVRC_STS_NO_ERROR, if the response is built successfully
**                  Otherwise, the error code.
**
*******************************************************************************/
tAVRC_STS AVRC_BldResponse(uint8_t handle, tAVRC_RESPONSE *p_rsp, BT_HDR **pp_pkt)
{
    tAVRC_STS status = AVRC_STS_BAD_PARAM;
    BT_HDR *p_pkt;
    uint8_t alloc = FALSE;
    UNUSED(handle);

    if(!p_rsp || !pp_pkt) {
        AVRC_TRACE_API("%s Invalid parameters passed. p_rsp=%p, pp_pkt=%p",
                       __func__, p_rsp, pp_pkt);
        return AVRC_STS_BAD_PARAM;
    }

    if(*pp_pkt == NULL) {
        if((*pp_pkt = avrc_bld_init_rsp_buffer(p_rsp)) == NULL) {
            AVRC_TRACE_API("%s Failed to initialize response buffer", __func__);
            return AVRC_STS_INTERNAL_ERR;
        }

        alloc = TRUE;
    }

    status = AVRC_STS_NO_ERROR;
    p_pkt = *pp_pkt;
    AVRC_TRACE_API("%s pdu=%x status=%x", __func__, p_rsp->rsp.pdu, p_rsp->rsp.status);

    if(p_rsp->rsp.status != AVRC_STS_NO_ERROR) {
        return(avrc_bld_rejected_rsp(&p_rsp->rsp, p_pkt));
    }

    switch(p_rsp->pdu) {
        case AVRC_PDU_NEXT_GROUP:
        case AVRC_PDU_PREV_GROUP:
            status = avrc_bld_group_navigation_rsp(p_rsp->pdu, p_pkt);
            break;

        case AVRC_PDU_GET_CAPABILITIES:
            status = avrc_bld_get_capability_rsp(&p_rsp->get_caps, p_pkt);
            break;

        case AVRC_PDU_LIST_PLAYER_APP_ATTR:
            status = avrc_bld_list_app_settings_attr_rsp(&p_rsp->list_app_attr, p_pkt);
            break;

        case AVRC_PDU_LIST_PLAYER_APP_VALUES:
            status = avrc_bld_list_app_settings_values_rsp(&p_rsp->list_app_values, p_pkt);
            break;

        case AVRC_PDU_GET_CUR_PLAYER_APP_VALUE:
            status = avrc_bld_get_cur_app_setting_value_rsp(&p_rsp->get_cur_app_val, p_pkt);
            break;

        case AVRC_PDU_SET_PLAYER_APP_VALUE:
            status = avrc_bld_set_app_setting_value_rsp(&p_rsp->set_app_val, p_pkt);
            break;

        case AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT:
            status = avrc_bld_get_app_setting_attr_text_rsp(&p_rsp->get_app_attr_txt, p_pkt);
            break;

        case AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT:
            status = avrc_bld_get_app_setting_value_text_rsp(&p_rsp->get_app_val_txt, p_pkt);
            break;

        case AVRC_PDU_INFORM_DISPLAY_CHARSET:
            status = avrc_bld_inform_charset_rsp(&p_rsp->inform_charset, p_pkt);
            break;

        case AVRC_PDU_INFORM_BATTERY_STAT_OF_CT:
            status = avrc_bld_inform_battery_status_rsp(&p_rsp->inform_battery_status, p_pkt);
            break;

        case AVRC_PDU_GET_ELEMENT_ATTR:
            status = avrc_bld_get_elem_attrs_rsp(&p_rsp->get_elem_attrs, p_pkt);
            break;

        case AVRC_PDU_GET_PLAY_STATUS:
            status = avrc_bld_get_play_status_rsp(&p_rsp->get_play_status, p_pkt);
            break;

        case AVRC_PDU_REGISTER_NOTIFICATION:
            status = avrc_bld_notify_rsp(&p_rsp->reg_notif, p_pkt);
            break;

        case AVRC_PDU_REQUEST_CONTINUATION_RSP:     /*        0x40 */
            status = avrc_bld_next_rsp(&p_rsp->continu, p_pkt);
            break;

        case AVRC_PDU_ABORT_CONTINUATION_RSP:       /*          0x41 */
            status = avrc_bld_next_rsp(&p_rsp->abort, p_pkt);
            break;

        case AVRC_PDU_SET_ADDRESSED_PLAYER: /*PDU 0x60*/
            status = avrc_bld_set_address_player_rsp(&p_rsp->addr_player, p_pkt);
            break;

        case AVRC_PDU_PLAY_ITEM:
            status = avrc_bld_play_item_rsp(&p_rsp->play_item, p_pkt);
            break;

        case AVRC_PDU_SET_ABSOLUTE_VOLUME:
            status = avrc_bld_set_absolute_volume_rsp(p_rsp->volume.volume, p_pkt);
            break;
    }

    if(alloc && (status != AVRC_STS_NO_ERROR)) {
        GKI_freebuf(p_pkt);
        *pp_pkt = NULL;
    }

    AVRC_TRACE_API("%s returning %d", __func__, status);
    return status;
}

#endif /* (AVRC_METADATA_INCLUDED == TRUE)*/

