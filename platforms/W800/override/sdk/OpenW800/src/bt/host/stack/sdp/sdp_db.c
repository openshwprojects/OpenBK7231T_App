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
 *  this file contains functions that handle the database
 *
 ******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bt_target.h"

#include "bt_common.h"

#include "l2cdefs.h"
#include "hcidefs.h"
#include "hcimsgs.h"

#include "sdp_api.h"
#include "sdpint.h"

#if SDP_SERVER_ENABLED == TRUE
/********************************************************************************/
/*              L O C A L    F U N C T I O N     P R O T O T Y P E S            */
/********************************************************************************/
static uint8_t find_uuid_in_seq(uint8_t *p, uint32_t seq_len, uint8_t *p_his_uuid,
                                uint16_t his_len, int nest_level);


/*******************************************************************************
**
** Function         sdp_db_service_search
**
** Description      This function searches for a record that contains the
**                  specified UIDs. It is passed either NULL to start at the
**                  beginning, or the previous record found.
**
** Returns          Pointer to the record, or NULL if not found.
**
*******************************************************************************/
tSDP_RECORD *sdp_db_service_search(tSDP_RECORD *p_rec, tSDP_UUID_SEQ *p_seq)
{
    uint16_t          xx, yy;
    tSDP_ATTRIBUTE *p_attr;
    tSDP_RECORD     *p_end = &sdp_cb.server_db.record[sdp_cb.server_db.num_records];

    /* If NULL, start at the beginning, else start at the first specified record */
    if(!p_rec) {
        p_rec = &sdp_cb.server_db.record[0];
    } else {
        p_rec++;
    }

    /* Look through the records. The spec says that a match occurs if */
    /* the record contains all the passed UUIDs in it.                */
    for(; p_rec < p_end; p_rec++) {
        for(yy = 0; yy < p_seq->num_uids; yy++) {
            p_attr = &p_rec->attribute[0];

            for(xx = 0; xx < p_rec->num_attributes; xx++, p_attr++) {
                if(p_attr->type == UUID_DESC_TYPE) {
                    if(sdpu_compare_uuid_arrays(p_attr->value_ptr, p_attr->len,
                                                &p_seq->uuid_entry[yy].value[0],
                                                p_seq->uuid_entry[yy].len)) {
                        break;
                    }
                } else if(p_attr->type == DATA_ELE_SEQ_DESC_TYPE) {
                    if(find_uuid_in_seq(p_attr->value_ptr, p_attr->len,
                                        &p_seq->uuid_entry[yy].value[0],
                                        p_seq->uuid_entry[yy].len, 0)) {
                        break;
                    }
                }
            }

            /* If any UUID was not found,  on to the next record */
            if(xx == p_rec->num_attributes) {
                break;
            }
        }

        /* If every UUID was found in the record, return the record */
        if(yy == p_seq->num_uids) {
            return (p_rec);
        }
    }

    /* If here, no more records found */
    return (NULL);
}

/*******************************************************************************
**
** Function         find_uuid_in_seq
**
** Description      This function searches a data element sequenct for a UUID.
**
** Returns          TRUE if found, else FALSE
**
*******************************************************************************/
static uint8_t find_uuid_in_seq(uint8_t *p, uint32_t seq_len, uint8_t *p_uuid,
                                uint16_t uuid_len, int nest_level)
{
    uint8_t   *p_end = p + seq_len;
    uint8_t   type;
    uint32_t  len;

    /* A little safety check to avoid excessive recursion */
    if(nest_level > 3) {
        return (FALSE);
    }

    while(p < p_end) {
        type = *p++;
        p = sdpu_get_len_from_type(p, type, &len);
        type = type >> 3;

        if(type == UUID_DESC_TYPE) {
            if(sdpu_compare_uuid_arrays(p, len, p_uuid, uuid_len)) {
                return (TRUE);
            }
        } else if(type == DATA_ELE_SEQ_DESC_TYPE) {
            if(find_uuid_in_seq(p, len, p_uuid, uuid_len, nest_level + 1)) {
                return (TRUE);
            }
        }

        p = p + len;
    }

    /* If here, failed to match */
    return (FALSE);
}

/*******************************************************************************
**
** Function         sdp_db_find_record
**
** Description      This function searches for a record with a specific handle
**                  It is passed the handle of the record.
**
** Returns          Pointer to the record, or NULL if not found.
**
*******************************************************************************/
tSDP_RECORD *sdp_db_find_record(uint32_t handle)
{
    tSDP_RECORD     *p_rec;
    tSDP_RECORD     *p_end = &sdp_cb.server_db.record[sdp_cb.server_db.num_records];

    /* Look through the records for the caller's handle */
    for(p_rec = &sdp_cb.server_db.record[0]; p_rec < p_end; p_rec++) {
        if(p_rec->record_handle == handle) {
            return (p_rec);
        }
    }

    /* Record with that handle not found. */
    return (NULL);
}

/*******************************************************************************
**
** Function         sdp_db_find_attr_in_rec
**
** Description      This function searches a record for specific attributes.
**                  It is passed a pointer to the record. If the record contains
**                  the specified attribute, (the caller may specify be a range
**                  of attributes), the attribute is returned.
**
** Returns          Pointer to the attribute, or NULL if not found.
**
*******************************************************************************/
tSDP_ATTRIBUTE *sdp_db_find_attr_in_rec(tSDP_RECORD *p_rec, uint16_t start_attr,
                                        uint16_t end_attr)
{
    tSDP_ATTRIBUTE  *p_at;
    uint16_t          xx;

    /* Note that the attributes in a record are assumed to be in sorted order */
    for(xx = 0, p_at = &p_rec->attribute[0]; xx < p_rec->num_attributes;
            xx++, p_at++) {
        if((p_at->id >= start_attr) && (p_at->id <= end_attr)) {
            return (p_at);
        }
    }

    /* No matching attribute found */
    return (NULL);
}


/*******************************************************************************
**
** Function         sdp_compose_proto_list
**
** Description      This function is called to compose a data sequence from
**                  protocol element list struct pointer
**
** Returns          the length of the data sequence
**
*******************************************************************************/
static int sdp_compose_proto_list(uint8_t *p, uint16_t num_elem,
                                  tSDP_PROTOCOL_ELEM *p_elem_list)
{
    uint16_t          xx, yy, len;
    uint8_t            is_rfcomm_scn;
    uint8_t           *p_head = p;
    uint8_t            *p_len;

    /* First, build the protocol list. This consists of a set of data element
    ** sequences, one for each layer. Each layer sequence consists of layer's
    ** UUID and optional parameters
    */
    for(xx = 0; xx < num_elem; xx++, p_elem_list++) {
        len = 3 + (p_elem_list->num_params * 3);
        UINT8_TO_BE_STREAM(p, (DATA_ELE_SEQ_DESC_TYPE << 3) | SIZE_IN_NEXT_BYTE);
        p_len = p;
        *p++ = (uint8_t) len;
        UINT8_TO_BE_STREAM(p, (UUID_DESC_TYPE << 3) | SIZE_TWO_BYTES);
        UINT16_TO_BE_STREAM(p, p_elem_list->protocol_uuid);

        if(p_elem_list->protocol_uuid == UUID_PROTOCOL_RFCOMM) {
            is_rfcomm_scn = TRUE;
        } else {
            is_rfcomm_scn = FALSE;
        }

        for(yy = 0; yy < p_elem_list->num_params; yy++) {
            if(is_rfcomm_scn) {
                UINT8_TO_BE_STREAM(p, (UINT_DESC_TYPE << 3) | SIZE_ONE_BYTE);
                UINT8_TO_BE_STREAM(p, p_elem_list->params[yy]);
                *p_len -= 1;
            } else {
                UINT8_TO_BE_STREAM(p, (UINT_DESC_TYPE << 3) | SIZE_TWO_BYTES);
                UINT16_TO_BE_STREAM(p, p_elem_list->params[yy]);
            }
        }
    }

    return (p - p_head);
}

#endif  /* SDP_SERVER_ENABLED == TRUE */

/*******************************************************************************
**
** Function         SDP_CreateRecord
**
** Description      This function is called to create a record in the database.
**                  This would be through the SDP database maintenance API. The
**                  record is created empty, teh application should then call
**                  "add_attribute" to add the record's attributes.
**
** Returns          Record handle if OK, else 0.
**
*******************************************************************************/
uint32_t SDP_CreateRecord(void)
{
#if SDP_SERVER_ENABLED == TRUE
    uint32_t    handle;
    uint8_t     buf[4];
    tSDP_DB  *p_db = &sdp_cb.server_db;

    /* First, check if there is a free record */
    if(p_db->num_records < SDP_MAX_RECORDS) {
        wm_memset(&p_db->record[p_db->num_records], 0,
                  sizeof(tSDP_RECORD));

        /* We will use a handle of the first unreserved handle plus last record
        ** number + 1 */
        if(p_db->num_records) {
            handle = p_db->record[p_db->num_records - 1].record_handle + 1;
        } else {
            handle = 0x10000;
        }

        p_db->record[p_db->num_records].record_handle = handle;
        p_db->num_records++;
        SDP_TRACE_DEBUG("SDP_CreateRecord ok, num_records:%d", p_db->num_records);
        /* Add the first attribute (the handle) automatically */
        UINT32_TO_BE_FIELD(buf, handle);
        SDP_AddAttribute(handle, ATTR_ID_SERVICE_RECORD_HDL, UINT_DESC_TYPE,
                         4, buf);
        return (p_db->record[p_db->num_records - 1].record_handle);
    } else {
        SDP_TRACE_ERROR("SDP_CreateRecord fail, exceed maximum records:%d", SDP_MAX_RECORDS);
    }

#endif
    return (0);
}


/*******************************************************************************
**
** Function         SDP_DeleteRecord
**
** Description      This function is called to add a record (or all records)
**                  from the database. This would be through the SDP database
**                  maintenance API.
**
**                  If a record handle of 0 is passed, all records are deleted.
**
** Returns          TRUE if succeeded, else FALSE
**
*******************************************************************************/
uint8_t SDP_DeleteRecord(uint32_t handle)
{
#if SDP_SERVER_ENABLED == TRUE
    uint16_t          xx, yy, zz;
    tSDP_RECORD     *p_rec = &sdp_cb.server_db.record[0];

    if(handle == 0 || sdp_cb.server_db.num_records == 0) {
        /* Delete all records in the database */
        sdp_cb.server_db.num_records = 0;
        /* require new DI record to be created in SDP_SetLocalDiRecord */
        sdp_cb.server_db.di_primary_handle = 0;
        return (TRUE);
    } else {
        /* Find the record in the database */
        for(xx = 0; xx < sdp_cb.server_db.num_records; xx++, p_rec++) {
            if(p_rec->record_handle == handle) {
                /* Found it. Shift everything up one */
                for(yy = xx; yy < sdp_cb.server_db.num_records; yy++, p_rec++) {
                    *p_rec = *(p_rec + 1);

                    /* Adjust the attribute value pointer for each attribute */
                    for(zz = 0; zz < p_rec->num_attributes; zz++) {
                        p_rec->attribute[zz].value_ptr -= sizeof(tSDP_RECORD);
                    }
                }

                sdp_cb.server_db.num_records--;
                SDP_TRACE_DEBUG("SDP_DeleteRecord ok, num_records:%d", sdp_cb.server_db.num_records);

                /* if we're deleting the primary DI record, clear the */
                /* value in the control block */
                if(sdp_cb.server_db.di_primary_handle == handle) {
                    sdp_cb.server_db.di_primary_handle = 0;
                }

                return (TRUE);
            }
        }
    }

#endif
    return (FALSE);
}


/*******************************************************************************
**
** Function         SDP_AddAttribute
**
** Description      This function is called to add an attribute to a record.
**                  This would be through the SDP database maintenance API.
**                  If the attribute already exists in the record, it is replaced
**                  with the new value.
**
** NOTE             Attribute values must be passed as a Big Endian stream.
**
** Returns          TRUE if added OK, else FALSE
**
*******************************************************************************/
uint8_t SDP_AddAttribute(uint32_t handle, uint16_t attr_id, uint8_t attr_type,
                         uint32_t attr_len, uint8_t *p_val)
{
#if SDP_SERVER_ENABLED == TRUE
    uint16_t          xx, yy, zz;
    tSDP_RECORD     *p_rec = &sdp_cb.server_db.record[0];
#if (BT_TRACE_VERBOSE == TRUE)

    if(sdp_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) {
        if((attr_type == UINT_DESC_TYPE) ||
                (attr_type == TWO_COMP_INT_DESC_TYPE) ||
                (attr_type == UUID_DESC_TYPE) ||
                (attr_type == DATA_ELE_SEQ_DESC_TYPE) ||
                (attr_type == DATA_ELE_ALT_DESC_TYPE)) {
            uint8_t num_array[400];
            uint32_t i;
            uint32_t len = (attr_len > 200) ? 200 : attr_len;
            num_array[0] = '\0';

            for(i = 0; i < len; i++) {
                sprintf((char *)&num_array[i * 2], "%02X", (uint8_t)(p_val[i]));
            }

            SDP_TRACE_DEBUG("SDP_AddAttribute: handle:%X, id:%04X, type:%d, len:%d, p_val:%p, *p_val:%s",
                            handle, attr_id, attr_type, attr_len, p_val, num_array);
        } else if(attr_type == BOOLEAN_DESC_TYPE) {
            SDP_TRACE_DEBUG("SDP_AddAttribute: handle:%X, id:%04X, type:%d, len:%d, p_val:%p, *p_val:%d",
                            handle, attr_id, attr_type, attr_len, p_val, *p_val);
        } else {
            SDP_TRACE_DEBUG("SDP_AddAttribute: handle:%X, id:%04X, type:%d, len:%d, p_val:%p, *p_val:%s",
                            handle, attr_id, attr_type, attr_len, p_val, p_val);
        }
    }

#endif

    /* Find the record in the database */
    for(zz = 0; zz < sdp_cb.server_db.num_records; zz++, p_rec++) {
        if(p_rec->record_handle == handle) {
            tSDP_ATTRIBUTE  *p_attr = &p_rec->attribute[0];

            /* Found the record. Now, see if the attribute already exists */
            for(xx = 0; xx < p_rec->num_attributes; xx++, p_attr++) {
                /* The attribute exists. replace it */
                if(p_attr->id == attr_id) {
                    SDP_DeleteAttribute(handle, attr_id);
                    break;
                }

                if(p_attr->id > attr_id) {
                    break;
                }
            }

            if(p_rec->num_attributes == SDP_MAX_REC_ATTR) {
                return (FALSE);
            }

            /* If not found, see if we can allocate a new entry */
            if(xx == p_rec->num_attributes) {
                p_attr = &p_rec->attribute[p_rec->num_attributes];
            } else {
                /* Since the attributes are kept in sorted order, insert ours here */
                for(yy = p_rec->num_attributes; yy > xx; yy--) {
                    p_rec->attribute[yy] = p_rec->attribute[yy - 1];
                }
            }

            p_attr->id   = attr_id;
            p_attr->type = attr_type;
            p_attr->len  = attr_len;

            if(p_rec->free_pad_ptr + attr_len >= SDP_MAX_PAD_LEN) {
                /* do truncate only for text string type descriptor */
                if(attr_type == TEXT_STR_DESC_TYPE) {
                    SDP_TRACE_WARNING("SDP_AddAttribute: attr_len:%d too long. truncate to (%d)",
                                      attr_len, SDP_MAX_PAD_LEN - p_rec->free_pad_ptr);
                    attr_len = SDP_MAX_PAD_LEN - p_rec->free_pad_ptr;
                    p_val[SDP_MAX_PAD_LEN - p_rec->free_pad_ptr] = '\0';
                    p_val[SDP_MAX_PAD_LEN - p_rec->free_pad_ptr + 1] = '\0';
                } else {
                    attr_len = 0;
                }
            }

            if((attr_len > 0) && (p_val != 0)) {
                p_attr->len  = attr_len;
                wm_memcpy(&p_rec->attr_pad[p_rec->free_pad_ptr], p_val, (size_t)attr_len);
                p_attr->value_ptr = &p_rec->attr_pad[p_rec->free_pad_ptr];
                p_rec->free_pad_ptr += attr_len;
            } else if((attr_len == 0 && p_attr->len != 0) || /* if truncate to 0 length, simply don't add */
                      p_val == 0) {
                SDP_TRACE_ERROR("SDP_AddAttribute fail, length exceed maximum: ID %d: attr_len:%d ",
                                attr_id, attr_len);
                p_attr->id   = p_attr->type = p_attr->len  = 0;
                return (FALSE);
            }

            p_rec->num_attributes++;
            return (TRUE);
        }
    }

#endif
    return (FALSE);
}


/*******************************************************************************
**
** Function         SDP_AddSequence
**
** Description      This function is called to add a sequence to a record.
**                  This would be through the SDP database maintenance API.
**                  If the sequence already exists in the record, it is replaced
**                  with the new sequence.
**
** NOTE             Element values must be passed as a Big Endian stream.
**
** Returns          TRUE if added OK, else FALSE
**
*******************************************************************************/
uint8_t SDP_AddSequence(uint32_t handle,  uint16_t attr_id, uint16_t num_elem,
                        uint8_t type[], uint8_t len[], uint8_t *p_val[])
{
#if SDP_SERVER_ENABLED == TRUE
    uint16_t xx;
    uint8_t *p;
    uint8_t *p_head;
    uint8_t result;
    uint8_t *p_buff = (uint8_t *)GKI_getbuf(sizeof(uint8_t) * SDP_MAX_ATTR_LEN * 2);
    p = p_buff;

    /* First, build the sequence */
    for(xx = 0; xx < num_elem; xx++) {
        p_head = p;

        switch(len[xx]) {
            case 1:
                UINT8_TO_BE_STREAM(p, (type[xx] << 3) | SIZE_ONE_BYTE);
                break;

            case 2:
                UINT8_TO_BE_STREAM(p, (type[xx] << 3) | SIZE_TWO_BYTES);
                break;

            case 4:
                UINT8_TO_BE_STREAM(p, (type[xx] << 3) | SIZE_FOUR_BYTES);
                break;

            case 8:
                UINT8_TO_BE_STREAM(p, (type[xx] << 3) | SIZE_EIGHT_BYTES);
                break;

            case 16:
                UINT8_TO_BE_STREAM(p, (type[xx] << 3) | SIZE_SIXTEEN_BYTES);
                break;

            default:
                UINT8_TO_BE_STREAM(p, (type[xx] << 3) | SIZE_IN_NEXT_BYTE);
                UINT8_TO_BE_STREAM(p, len[xx]);
                break;
        }

        ARRAY_TO_BE_STREAM(p, p_val[xx], len[xx]);

        if(p - p_buff > SDP_MAX_ATTR_LEN) {
            /* go back to before we add this element */
            p = p_head;

            if(p_head == p_buff) {
                /* the first element exceed the max length */
                SDP_TRACE_ERROR("SDP_AddSequence - too long(attribute is not added)!!");
                GKI_freebuf(p_buff);
                return FALSE;
            } else {
                SDP_TRACE_ERROR("SDP_AddSequence - too long, add %d elements of %d", xx, num_elem);
            }

            break;
        }
    }

    result = SDP_AddAttribute(handle, attr_id, DATA_ELE_SEQ_DESC_TYPE, (uint32_t)(p - p_buff), p_buff);
    GKI_freebuf(p_buff);
    return result;
#else   /* SDP_SERVER_ENABLED == FALSE */
    return (FALSE);
#endif
}


/*******************************************************************************
**
** Function         SDP_AddUuidSequence
**
** Description      This function is called to add a UUID sequence to a record.
**                  This would be through the SDP database maintenance API.
**                  If the sequence already exists in the record, it is replaced
**                  with the new sequence.
**
** Returns          TRUE if added OK, else FALSE
**
*******************************************************************************/
uint8_t SDP_AddUuidSequence(uint32_t handle,  uint16_t attr_id, uint16_t num_uuids,
                            uint16_t *p_uuids)
{
#if SDP_SERVER_ENABLED == TRUE
    uint16_t xx;
    uint8_t *p;
    int32_t max_len = SDP_MAX_ATTR_LEN - 3;
    uint8_t result;
    uint8_t *p_buff = (uint8_t *)GKI_getbuf(sizeof(uint8_t) * SDP_MAX_ATTR_LEN * 2);
    p = p_buff;

    /* First, build the sequence */
    for(xx = 0; xx < num_uuids ; xx++, p_uuids++) {
        UINT8_TO_BE_STREAM(p, (UUID_DESC_TYPE << 3) | SIZE_TWO_BYTES);
        UINT16_TO_BE_STREAM(p, *p_uuids);

        if((p - p_buff) > max_len) {
            SDP_TRACE_WARNING("SDP_AddUuidSequence - too long, add %d uuids of %d", xx, num_uuids);
            break;
        }
    }

    result = SDP_AddAttribute(handle, attr_id, DATA_ELE_SEQ_DESC_TYPE, (uint32_t)(p - p_buff), p_buff);
    GKI_freebuf(p_buff);
    return result;
#else   /* SDP_SERVER_ENABLED == FALSE */
    return (FALSE);
#endif
}

/*******************************************************************************
**
** Function         SDP_AddProtocolList
**
** Description      This function is called to add a protocol descriptor list to
**                  a record. This would be through the SDP database maintenance API.
**                  If the protocol list already exists in the record, it is replaced
**                  with the new list.
**
** Returns          TRUE if added OK, else FALSE
**
*******************************************************************************/
uint8_t SDP_AddProtocolList(uint32_t handle, uint16_t num_elem,
                            tSDP_PROTOCOL_ELEM *p_elem_list)
{
#if SDP_SERVER_ENABLED == TRUE
    int offset;
    uint8_t result;
    uint8_t *p_buff = (uint8_t *)GKI_getbuf(sizeof(uint8_t) * SDP_MAX_ATTR_LEN * 2);
    offset = sdp_compose_proto_list(p_buff, num_elem, p_elem_list);
    result = SDP_AddAttribute(handle, ATTR_ID_PROTOCOL_DESC_LIST, DATA_ELE_SEQ_DESC_TYPE,
                              (uint32_t) offset, p_buff);
    GKI_freebuf(p_buff);
    return result;
#else   /* SDP_SERVER_ENABLED == FALSE */
    return (FALSE);
#endif
}


/*******************************************************************************
**
** Function         SDP_AddAdditionProtoLists
**
** Description      This function is called to add a protocol descriptor list to
**                  a record. This would be through the SDP database maintenance API.
**                  If the protocol list already exists in the record, it is replaced
**                  with the new list.
**
** Returns          TRUE if added OK, else FALSE
**
*******************************************************************************/
uint8_t SDP_AddAdditionProtoLists(uint32_t handle, uint16_t num_elem,
                                  tSDP_PROTO_LIST_ELEM *p_proto_list)
{
#if SDP_SERVER_ENABLED == TRUE
    uint16_t xx;
    uint8_t *p;
    uint8_t *p_len;
    int offset;
    uint8_t result;
    uint8_t *p_buff = (uint8_t *)GKI_getbuf(sizeof(uint8_t) * SDP_MAX_ATTR_LEN * 2);
    p = p_buff;

    /* for each ProtocolDescriptorList */
    for(xx = 0; xx < num_elem; xx++, p_proto_list++) {
        UINT8_TO_BE_STREAM(p, (DATA_ELE_SEQ_DESC_TYPE << 3) | SIZE_IN_NEXT_BYTE);
        p_len = p++;
        offset = sdp_compose_proto_list(p, p_proto_list->num_elems,
                                        p_proto_list->list_elem);
        p += offset;
        *p_len  = (uint8_t)(p - p_len - 1);
    }

    result = SDP_AddAttribute(handle, ATTR_ID_ADDITION_PROTO_DESC_LISTS, DATA_ELE_SEQ_DESC_TYPE,
                              (uint32_t)(p - p_buff), p_buff);
    GKI_freebuf(p_buff);
    return result;
#else   /* SDP_SERVER_ENABLED == FALSE */
    return (FALSE);
#endif
}

/*******************************************************************************
**
** Function         SDP_AddProfileDescriptorList
**
** Description      This function is called to add a profile descriptor list to
**                  a record. This would be through the SDP database maintenance API.
**                  If the version already exists in the record, it is replaced
**                  with the new one.
**
** Returns          TRUE if added OK, else FALSE
**
*******************************************************************************/
uint8_t SDP_AddProfileDescriptorList(uint32_t handle, uint16_t profile_uuid,
                                     uint16_t version)
{
#if SDP_SERVER_ENABLED == TRUE
    uint8_t *p;
    uint8_t result;
    uint8_t *p_buff = (uint8_t *)GKI_getbuf(sizeof(uint8_t) * SDP_MAX_ATTR_LEN);
    p = p_buff + 2;
    /* First, build the profile descriptor list. This consists of a data element sequence. */
    /* The sequence consists of profile's UUID and version number  */
    UINT8_TO_BE_STREAM(p, (UUID_DESC_TYPE << 3) | SIZE_TWO_BYTES);
    UINT16_TO_BE_STREAM(p, profile_uuid);
    UINT8_TO_BE_STREAM(p, (UINT_DESC_TYPE << 3) | SIZE_TWO_BYTES);
    UINT16_TO_BE_STREAM(p, version);
    /* Add in type and length fields */
    *p_buff = (uint8_t)((DATA_ELE_SEQ_DESC_TYPE << 3) | SIZE_IN_NEXT_BYTE);
    *(p_buff + 1) = (uint8_t)(p - (p_buff + 2));
    result = SDP_AddAttribute(handle, ATTR_ID_BT_PROFILE_DESC_LIST, DATA_ELE_SEQ_DESC_TYPE,
                              (uint32_t)(p - p_buff), p_buff);
    GKI_freebuf(p_buff);
    return result;
#else   /* SDP_SERVER_ENABLED == FALSE */
    return (FALSE);
#endif
}


/*******************************************************************************
**
** Function         SDP_AddLanguageBaseAttrIDList
**
** Description      This function is called to add a language base attr list to
**                  a record. This would be through the SDP database maintenance API.
**                  If the version already exists in the record, it is replaced
**                  with the new one.
**
** Returns          TRUE if added OK, else FALSE
**
*******************************************************************************/
uint8_t SDP_AddLanguageBaseAttrIDList(uint32_t handle, uint16_t lang,
                                      uint16_t char_enc, uint16_t base_id)
{
#if SDP_SERVER_ENABLED == TRUE
    uint8_t *p;
    uint8_t result;
    uint8_t *p_buff = (uint8_t *) GKI_getbuf(sizeof(uint8_t) * SDP_MAX_ATTR_LEN);
    p = p_buff;
    /* First, build the language base descriptor list. This consists of a data */
    /* element sequence. The sequence consists of 9 bytes (3 UINt16 fields)    */
    UINT8_TO_BE_STREAM(p, (UINT_DESC_TYPE << 3) | SIZE_TWO_BYTES);
    UINT16_TO_BE_STREAM(p, lang);
    UINT8_TO_BE_STREAM(p, (UINT_DESC_TYPE << 3) | SIZE_TWO_BYTES);
    UINT16_TO_BE_STREAM(p, char_enc);
    UINT8_TO_BE_STREAM(p, (UINT_DESC_TYPE << 3) | SIZE_TWO_BYTES);
    UINT16_TO_BE_STREAM(p, base_id);
    result = SDP_AddAttribute(handle, ATTR_ID_LANGUAGE_BASE_ATTR_ID_LIST, DATA_ELE_SEQ_DESC_TYPE,
                              (uint32_t)(p - p_buff), p_buff);
    GKI_freebuf(p_buff);
    return result;
#else   /* SDP_SERVER_ENABLED == FALSE */
    return (FALSE);
#endif
}


/*******************************************************************************
**
** Function         SDP_AddServiceClassIdList
**
** Description      This function is called to add a service list to a record.
**                  This would be through the SDP database maintenance API.
**                  If the service list already exists in the record, it is replaced
**                  with the new list.
**
** Returns          TRUE if added OK, else FALSE
**
*******************************************************************************/
uint8_t SDP_AddServiceClassIdList(uint32_t handle, uint16_t num_services,
                                  uint16_t *p_service_uuids)
{
#if SDP_SERVER_ENABLED == TRUE
    uint16_t xx;
    uint8_t *p;
    uint8_t result;
    uint8_t *p_buff = (uint8_t *) GKI_getbuf(sizeof(uint8_t) * SDP_MAX_ATTR_LEN * 2);
    p = p_buff;

    for(xx = 0; xx < num_services; xx++, p_service_uuids++) {
        UINT8_TO_BE_STREAM(p, (UUID_DESC_TYPE << 3) | SIZE_TWO_BYTES);
        UINT16_TO_BE_STREAM(p, *p_service_uuids);
    }

    result = SDP_AddAttribute(handle, ATTR_ID_SERVICE_CLASS_ID_LIST, DATA_ELE_SEQ_DESC_TYPE,
                              (uint32_t)(p - p_buff), p_buff);
    GKI_freebuf(p_buff);
    return result;
#else   /* SDP_SERVER_ENABLED == FALSE */
    return (FALSE);
#endif
}


/*******************************************************************************
**
** Function         SDP_DeleteAttribute
**
** Description      This function is called to delete an attribute from a record.
**                  This would be through the SDP database maintenance API.
**
** Returns          TRUE if deleted OK, else FALSE if not found
**
*******************************************************************************/
uint8_t SDP_DeleteAttribute(uint32_t handle, uint16_t attr_id)
{
#if SDP_SERVER_ENABLED == TRUE
    uint16_t          xx, yy;
    tSDP_RECORD     *p_rec = &sdp_cb.server_db.record[0];
    uint8_t           *pad_ptr;
    uint32_t  len;                        /* Number of bytes in the entry */

    /* Find the record in the database */
    for(xx = 0; xx < sdp_cb.server_db.num_records; xx++, p_rec++) {
        if(p_rec->record_handle == handle) {
            tSDP_ATTRIBUTE  *p_attr = &p_rec->attribute[0];
            SDP_TRACE_API("Deleting attr_id 0x%04x for handle 0x%x", attr_id, handle);

            /* Found it. Now, find the attribute */
            for(xx = 0; xx < p_rec->num_attributes; xx++, p_attr++) {
                if(p_attr->id == attr_id) {
                    pad_ptr = p_attr->value_ptr;
                    len = p_attr->len;

                    if(len) {
                        for(yy = 0; yy < p_rec->num_attributes; yy++) {
                            if(p_rec->attribute[yy].value_ptr > pad_ptr) {
                                p_rec->attribute[yy].value_ptr -= len;
                            }
                        }
                    }

                    /* Found it. Shift everything up one */
                    p_rec->num_attributes--;

                    for(yy = xx; yy < p_rec->num_attributes; yy++, p_attr++) {
                        *p_attr = *(p_attr + 1);
                    }

                    /* adjust attribute values if needed */
                    if(len) {
                        xx = (p_rec->free_pad_ptr - ((pad_ptr + len) -
                                                     &p_rec->attr_pad[0]));

                        for(yy = 0; yy < xx; yy++, pad_ptr++) {
                            *pad_ptr = *(pad_ptr + len);
                        }

                        p_rec->free_pad_ptr -= len;
                    }

                    return (TRUE);
                }
            }
        }
    }

#endif
    /* If here, not found */
    return (FALSE);
}

/*******************************************************************************
**
** Function         SDP_ReadRecord
**
** Description      This function is called to get the raw data of the record
**                  with the given handle from the database.
**
** Returns          -1, if the record is not found.
**                  Otherwise, the offset (0 or 1) to start of data in p_data.
**
**                  The size of data copied into p_data is in *p_data_len.
**
*******************************************************************************/
#if (SDP_RAW_DATA_INCLUDED == TRUE)
int32_t SDP_ReadRecord(uint32_t handle, uint8_t *p_data, int32_t *p_data_len)
{
#if SDP_SERVER_ENABLED == TRUE
    int32_t           len = 0;                        /* Number of bytes in the entry */
#endif
    int32_t           offset = -1; /* default to not found */
#if SDP_SERVER_ENABLED == TRUE
    tSDP_RECORD     *p_rec;
    uint16_t          start = 0;
    uint16_t          end = 0xffff;
    tSDP_ATTRIBUTE  *p_attr;
    uint16_t          rem_len;
    uint8_t           *p_rsp;
    /* Find the record in the database */
    p_rec = sdp_db_find_record(handle);

    if(p_rec && p_data && p_data_len) {
        p_rsp = &p_data[3];

        while((p_attr = sdp_db_find_attr_in_rec(p_rec, start, end)) != NULL) {
            /* Check if attribute fits. Assume 3-byte value type/length */
            rem_len = *p_data_len - (uint16_t)(p_rsp - p_data);

            if(p_attr->len > (uint32_t)(rem_len - 6)) {
                break;
            }

            p_rsp = sdpu_build_attrib_entry(p_rsp, p_attr);
            /* next attr id */
            start = p_attr->id + 1;
        }

        len = (int32_t)(p_rsp - p_data);

        /* Put in the sequence header (2 or 3 bytes) */
        if(len > 255) {
            offset = 0;
            p_data[0] = (uint8_t)((DATA_ELE_SEQ_DESC_TYPE << 3) | SIZE_IN_NEXT_WORD);
            p_data[1] = (uint8_t)((len - 3) >> 8);
            p_data[2] = (uint8_t)(len - 3);
        } else {
            offset = 1;
            p_data[1] = (uint8_t)((DATA_ELE_SEQ_DESC_TYPE << 3) | SIZE_IN_NEXT_BYTE);
            p_data[2] = (uint8_t)(len - 3);
            len--;
        }

        *p_data_len = len;
    }

#endif
    /* If here, not found */
    return (offset);
}
#endif
