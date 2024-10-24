#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string.h>


#include "bt_target.h"
#if defined(BTA_JV_INCLUDED) && (BTA_JV_INCLUDED == TRUE)

#include "bluetooth.h"
#include "osi/include/list.h"
#include "osi/include/log.h"
#include "osi/include/osi.h"


#include "bt_utils.h"
#include "bta_jv_api.h"
#include "bta_api.h"
#include "btif_util.h"
#include "btif_common.h"
#include "btu.h"
#include "bt_common.h"
#include "gki.h"
#include "wm_bt.h"

#define CHECK_BTSPP_INIT() if (tls_spp_cb == NULL)\
    {\
        LOG_WARN(LOG_TAG, "%s: BTSPP not initialized", __FUNCTION__);\
        return TLS_BT_STATUS_NOT_READY;\
    } else {\
        LOG_VERBOSE(LOG_TAG, "%s", __FUNCTION__);\
    }

#define TLS_SPP_SERVER_NAME_MAX 32

typedef struct {
    uint8_t *data;
    uint16_t length;
    tls_bt_addr_t bd_addr;
    wm_spp_sec_t sec_mask;
    tls_spp_role_t role;
    uint8_t remote_scn;
    uint8_t local_scn;
    uint32_t handle;
    uint8_t num_uuid;
    tls_bt_uuid_t uuid;
    char service_name[TLS_SPP_SERVER_NAME_MAX + 1];
} __attribute__((packed)) btif_spp_cb_t;

typedef enum {
    BTIF_SPP_ENABLE = 4000,
    BTIF_SPP_DISABLE,
    BTIF_SPP_DISCOVERY,
    BTIF_SPP_CONNECT,
    BTIF_SPP_DISCONNECT,
    BTIF_SPP_START_SERVER,
    BTIF_SPP_WRITE
} btif_spp_event_t;


typedef struct {
    uint8_t serial;
    bool connected;
    uint8_t scn;
    uint8_t max_session;
    uint32_t id;
    uint32_t mtu;//unused
    uint32_t sdp_handle;
    uint32_t rfc_handle;
    uint32_t rfc_port_handle;
    int fd;
    uint8_t *write_data;
    tls_spp_role_t role;
    wm_spp_sec_t security;
    tls_bt_addr_t addr;
    list_t *list;
    list_t *incoming_list;
    uint8_t service_uuid[16];
    char service_name[TLS_SPP_SERVER_NAME_MAX + 1];
} spp_slot_t;

static struct spp_local_param_t {
    spp_slot_t *spp_slots[BTA_JV_MAX_RFC_SR_SESSION + 1];
    uint32_t spp_slot_id;
} spp_local_param;

static tls_bt_spp_callback_t tls_spp_cb = NULL;

static spp_slot_t *spp_malloc_slot(void)
{
    if(++spp_local_param.spp_slot_id == 0) {
        spp_local_param.spp_slot_id = 1;
    }

    for(size_t i = 1; i <= BTA_JV_MAX_RFC_SR_SESSION; i++) {
        if(spp_local_param.spp_slots[i] == NULL) {
            spp_local_param.spp_slots[i] = (spp_slot_t *)GKI_getbuf(sizeof(spp_slot_t));

            if(!spp_local_param.spp_slots[i]) {
                return NULL;
            }

            spp_local_param.spp_slots[i]->id = spp_local_param.spp_slot_id;
            spp_local_param.spp_slots[i]->serial = i;
            spp_local_param.spp_slots[i]->connected = FALSE;
            spp_local_param.spp_slots[i]->write_data = NULL;
            spp_local_param.spp_slots[i]->list = list_new(GKI_freebuf);
            return spp_local_param.spp_slots[i];
        }
    }

    return NULL;
}

static spp_slot_t *spp_find_slot_by_id(uint32_t id)
{
    for(size_t i = 1; i <= BTA_JV_MAX_RFC_SR_SESSION; i++) {
        if(spp_local_param.spp_slots[i] != NULL && spp_local_param.spp_slots[i]->id == id) {
            return spp_local_param.spp_slots[i];
        }
    }

    return NULL;
}

static spp_slot_t *spp_find_slot_by_handle(uint32_t handle)
{
    for(size_t i = 1; i <= BTA_JV_MAX_RFC_SR_SESSION; i++) {
        if(spp_local_param.spp_slots[i] != NULL && spp_local_param.spp_slots[i]->rfc_handle == handle) {
            return spp_local_param.spp_slots[i];
        }
    }

    return NULL;
}
#if 0
static spp_slot_t *spp_find_slot_by_fd(int fd)
{
    for(size_t i = 1; i <= BTA_JV_MAX_RFC_SR_SESSION; i++) {
        if(spp_local_param.spp_slots[i] != NULL && spp_local_param.spp_slots[i]->fd == fd) {
            return spp_local_param.spp_slots[i];
        }
    }

    return NULL;
}
#endif
static void spp_free_slot(spp_slot_t *slot)
{
    if(!slot) {
        return;
    }

    spp_local_param.spp_slots[slot->serial] = NULL;
    list_free(slot->list);
    GKI_freebuf(slot);
}
static void btapp_spp_handle_cback(uint16_t event, char *p_param)
{
    BTIF_TRACE_EVENT("%s: Event %d", __FUNCTION__, event);
    tBTA_JV *p_data = (tBTA_JV *)p_param;
    tls_spp_msg_t msg;
    spp_slot_t *slot = NULL;

    switch(event) {
        case BTA_JV_ENABLE_EVT:
            msg.init_msg.status = p_data->status;

            if(tls_spp_cb) { tls_spp_cb(WM_SPP_INIT_EVT, &msg); }

            break;

        case BTA_JV_GET_SCN_EVT:
            if(p_data->scn == 0) {
                msg.start_msg.status = TLS_BT_STATUS_INTERNAL_ERROR;

                if(tls_spp_cb) { tls_spp_cb(WM_SPP_START_EVT, &msg); }
            }

            break;

        case BTA_JV_CREATE_RECORD_EVT:
            if(p_data->create_rec.status != BTA_JV_SUCCESS) {
                msg.start_msg.status = TLS_BT_STATUS_INTERNAL_ERROR;

                if(tls_spp_cb) { tls_spp_cb(WM_SPP_START_EVT, &msg); }
            }

            break;

        case BTA_JV_DISCOVERY_COMP_EVT:
            msg.disc_comp_msg.status = p_data->disc_comp.status;
            msg.disc_comp_msg.scn_num = p_data->disc_comp.scn;

            if(tls_spp_cb) { tls_spp_cb(WM_SPP_DISCOVERY_COMP_EVT, &msg); }

            break;

        case BTA_JV_RFCOMM_OPEN_EVT:
            msg.open_msg.status = p_data->rfc_open.status;
            msg.open_msg.handle = p_data->rfc_open.handle;
            bdcpy(msg.open_msg.addr, p_data->rfc_open.rem_bda);

            if(tls_spp_cb) { tls_spp_cb(WM_SPP_OPEN_EVT, &msg); }

            break;

        case BTA_JV_RFCOMM_CLOSE_EVT:
            msg.close_msg.status = p_data->rfc_close.status;
            msg.close_msg.port_status = p_data->rfc_close.port_status;
            msg.close_msg.handle = p_data->rfc_close.handle;
            msg.close_msg.local = p_data->rfc_close.async;

            if(tls_spp_cb) { tls_spp_cb(WM_SPP_CLOSE_EVT, &msg); }

            break;

        case BTA_JV_RFCOMM_START_EVT:
            msg.start_msg.status = p_data->rfc_start.status;
            msg.start_msg.handle = p_data->rfc_start.handle;
            msg.start_msg.sec_id = p_data->rfc_start.sec_id;
            msg.start_msg.use_co_rfc = p_data->rfc_start.use_co;

            if(tls_spp_cb) { tls_spp_cb(WM_SPP_START_EVT, &msg); }

            break;

        case BTA_JV_RFCOMM_CL_INIT_EVT:
            msg.cli_init_msg.status = p_data->rfc_cl_init.status;
            msg.cli_init_msg.handle = p_data->rfc_cl_init.handle;
            msg.cli_init_msg.sec_id = p_data->rfc_cl_init.sec_id;
            msg.cli_init_msg.use_co_rfc = p_data->rfc_cl_init.use_co;

            if(tls_spp_cb) { tls_spp_cb(WM_SPP_CL_INIT_EVT, &msg); }

            break;

        case BTA_JV_RFCOMM_DATA_IND_EVT:
            msg.data_ind_msg.status = TLS_BT_STATUS_SUCCESS;
            msg.data_ind_msg.handle = p_data->data_ind.handle;

            if(p_data->data_ind.p_buf) {
                msg.data_ind_msg.length = p_data->data_ind.p_buf->len;
                msg.data_ind_msg.data = p_data->data_ind.p_buf->data + p_data->data_ind.p_buf->offset;
            } else {
                msg.data_ind_msg.length = 0;
                msg.data_ind_msg.data = NULL;
            }

            if(tls_spp_cb) { tls_spp_cb(WM_SPP_DATA_IND_EVT, &msg); }

            if(p_data->data_ind.p_buf) {
                GKI_freebuf(p_data->data_ind.p_buf);
                p_data->data_ind.p_buf = NULL;
            }

            break;

        case BTA_JV_RFCOMM_CONG_EVT:
            msg.congest_msg.status = p_data->rfc_cong.status;
            msg.congest_msg.handle = p_data->rfc_cong.handle;
            msg.congest_msg.congest = p_data->rfc_cong.cong;

            if(tls_spp_cb) { tls_spp_cb(WM_SPP_CONG_EVT, &msg); }

            break;

        case BTA_JV_RFCOMM_WRITE_EVT:
            slot = spp_find_slot_by_handle(p_data->rfc_write.handle);

            if(!slot) {
                BTIF_TRACE_ERROR("%s unable to find RFCOMM slot!", __func__);
                break;
            }

            msg.write_msg.status = p_data->rfc_write.status;
            msg.write_msg.handle = p_data->rfc_write.handle;
            msg.write_msg.length = p_data->rfc_write.len;
            msg.write_msg.congest = p_data->rfc_write.cong;

            if(tls_spp_cb) { tls_spp_cb(WM_SPP_WRITE_EVT, &msg); }

            list_remove(slot->list, list_front(slot->list));
            break;

        case BTA_JV_RFCOMM_SRV_OPEN_EVT:
            msg.srv_open_msg.status = p_data->rfc_srv_open.status;
            msg.srv_open_msg.handle = p_data->rfc_srv_open.handle;
            msg.srv_open_msg.new_listen_handle = p_data->rfc_srv_open.new_listen_handle;
            bdcpy(msg.srv_open_msg.addr, p_data->rfc_srv_open.rem_bda);

            if(tls_spp_cb) { tls_spp_cb(WM_SPP_SRV_OPEN_EVT, &msg); }

            break;
    }
}

#define PTR_TO_UINT(p) ((unsigned int) ((uintptr_t) (p)))

static void *btif_spp_rfcomm_inter_cb(tBTA_JV_EVT event, tBTA_JV *p_data, void *user_data)
{
    tls_bt_status_t status;
    void *new_user_data = NULL;
    uint32_t id = PTR_TO_UINT(user_data);
    spp_slot_t *slot, *slot_new;
    BTIF_TRACE_EVENT("btif_spp_rfcomm_inter_cb, event=%d\r\n", event);

    switch(event) {
        case BTA_JV_RFCOMM_SRV_OPEN_EVT:
            slot = spp_find_slot_by_id(id);

            if(!slot) {
                BTIF_TRACE_ERROR("%s unable to find RFCOMM slot!", __func__);
                break;
            }

            slot_new = spp_malloc_slot();

            if(!slot_new) {
                BTIF_TRACE_ERROR("%s unable to malloc RFCOMM slot!", __func__);
                break;
            }

            new_user_data = (void *)PTR_TO_UINT(slot_new->id);
            slot_new->security = slot->security;
            slot_new->role = slot->role;
            slot_new->scn = slot->scn;;
            slot_new->max_session = slot->max_session;
            strcpy(slot_new->service_name, slot->service_name);
            slot_new->sdp_handle = slot->sdp_handle;
            slot_new->rfc_handle = p_data->rfc_srv_open.new_listen_handle;
            slot_new->rfc_port_handle = BTA_JvRfcommGetPortHdl(p_data->rfc_srv_open.new_listen_handle);
            memcpy(slot->addr.address, p_data->rfc_srv_open.rem_bda, 6);
            slot->connected = TRUE;
            slot->rfc_handle = p_data->rfc_srv_open.handle;
            slot->rfc_port_handle = BTA_JvRfcommGetPortHdl(p_data->rfc_srv_open.handle);
            BTA_JvSetPmProfile(p_data->rfc_srv_open.handle, BTA_JV_PM_ALL, BTA_JV_CONN_OPEN);
            break;

        case BTA_JV_RFCOMM_OPEN_EVT:
            slot = spp_find_slot_by_id(id);

            if(!slot) {
                BTIF_TRACE_ERROR("%s unable to find RFCOMM slot!", __func__);
                break;
            }

            slot->connected = TRUE;
            slot->rfc_handle = p_data->rfc_open.handle;
            slot->rfc_port_handle = BTA_JvRfcommGetPortHdl(p_data->rfc_open.handle);
            BTA_JvSetPmProfile(p_data->rfc_open.handle, BTA_JV_PM_ID_1, BTA_JV_CONN_OPEN);
            break;

        case BTA_JV_RFCOMM_CLOSE_EVT:
            slot = spp_find_slot_by_id(id);

            if(!slot) {
                BTIF_TRACE_ERROR("%s unable to find RFCOMM slot!", __func__);
                break;
            }

            if(slot->connected) {
                BTA_JvRfcommClose(slot->rfc_handle, (void *)slot->id);
            }

            spp_free_slot(slot);
            p_data->rfc_close.status = BTA_JV_SUCCESS;
            break;

        case BTA_JV_RFCOMM_DATA_IND_EVT:
            break;

        default:
            break;
    }

    status = btif_transfer_context(btapp_spp_handle_cback, (uint16_t) event,
                                   (void *)p_data, sizeof(tBTA_JV), NULL);

    if(status != TLS_BT_STATUS_SUCCESS) {
        BTIF_TRACE_ERROR("%s btc_transfer_context failed", __func__);
    }

    return new_user_data;
}

static void btif_spp_dm_inter_cb(tBTA_JV_EVT event, tBTA_JV *p_data, void *user_data)
{
    tls_bt_status_t status;
    uint32_t id = PTR_TO_UINT(user_data);
    spp_slot_t *slot;
    BTIF_TRACE_EVENT("btif_spp_dm_inter_cb, evt=%d, id=%d\r\n", event, id);

    switch(event) {
        case BTA_JV_GET_SCN_EVT:
            slot = spp_find_slot_by_id(id);

            if(!slot) {
                BTIF_TRACE_ERROR("%s unable to find RFCOMM slot!", __func__);
                break;
            }

            if(p_data->scn == 0) {
                BTIF_TRACE_ERROR("%s unable to get scn, start server fail!", __func__);
                spp_free_slot(slot);
                break;
            }

            slot->scn = p_data->scn;
            BTA_JvCreateRecordByUser(slot->service_name, slot->scn, (void *)slot->id);
            break;

        case BTA_JV_CREATE_RECORD_EVT:
            slot = spp_find_slot_by_id(id);

            if(!slot) {
                BTIF_TRACE_ERROR("%s unable to find RFCOMM slot!", __func__);
                break;
            }

            if(p_data->create_rec.status == BTA_JV_SUCCESS) {
                slot->sdp_handle = p_data->create_rec.handle;
                BTA_JvRfcommStartServer(slot->security, slot->role, slot->scn,
                                        slot->max_session, (tBTA_JV_RFCOMM_CBACK *)btif_spp_rfcomm_inter_cb, (void *)slot->id);
            } else {
                BTIF_TRACE_ERROR("%s unable to create record, start server fail!", __func__);
                BTA_JvFreeChannel(slot->scn, BTA_JV_CONN_TYPE_RFCOMM);
                spp_free_slot(slot);
            }

            break;

        default:
            break;
    }

    status = btif_transfer_context(btapp_spp_handle_cback, (uint16_t) event,
                                   (void *)p_data, sizeof(tBTA_JV), NULL);
    ASSERTC(status == TLS_BT_STATUS_SUCCESS, "Context transfer failed!", status);
}

int bta_co_rfc_data_incoming(void *user_data, BT_HDR *p_buf)
{
    tls_bt_status_t status;
    tBTA_JV p_data;
    uint32_t id = PTR_TO_UINT(user_data);
    spp_slot_t *slot = spp_find_slot_by_id(id);

    if(!slot) {
        BTIF_TRACE_ERROR("%s unable to find RFCOMM slot!", __func__);
        return 0;
    }

    p_data.data_ind.handle = slot->rfc_handle;
    p_data.data_ind.p_buf = p_buf;
    status = btif_transfer_context(btapp_spp_handle_cback, (uint16_t) BTA_JV_RFCOMM_DATA_IND_EVT,
                                   (void *)&p_data, sizeof(tBTA_JV), NULL);

    if(status != TLS_BT_STATUS_SUCCESS) {
        BTIF_TRACE_ERROR("%s btc_transfer_context failed", __func__);
    }

    return 1;
}
int bta_co_rfc_data_outgoing_size(void *user_data, int *size)
{
    printf("%s\r\n", __func__);
    return 1;
}
int bta_co_rfc_data_outgoing(void *user_data, uint8_t *buf, uint16_t size)
{
    printf("%s\r\n", __func__);
    return 1;
}

static void btspp_handle_event(uint16_t event, char *p_param)
{
    btif_spp_cb_t *p_cb = (btif_spp_cb_t *)p_param;

    if(!p_cb) {
        return;
    }

    BTIF_TRACE_EVENT("%s: Event %d\r\n", __FUNCTION__, event);

    switch(event) {
        case BTIF_SPP_ENABLE: {
            spp_local_param.spp_slot_id = 0;
            BTA_JvEnable((tBTA_JV_DM_CBACK *)btif_spp_dm_inter_cb);
            break;
        }

        case BTIF_SPP_DISABLE: {
            for(size_t i = 1; i <= BTA_JV_MAX_RFC_SR_SESSION; i++) {
                if(spp_local_param.spp_slots[i] != NULL && spp_local_param.spp_slots[i]->connected) {
                    BTA_JvRfcommClose(spp_local_param.spp_slots[i]->rfc_handle,
                                      (void *)spp_local_param.spp_slots[i]->id);
                    spp_free_slot(spp_local_param.spp_slots[i]);
                    spp_local_param.spp_slots[i] = NULL;
                }
            }

            for(size_t i = 1; i <= BTA_JV_MAX_RFC_SR_SESSION; i++) {
                if(spp_local_param.spp_slots[i] != NULL && !(spp_local_param.spp_slots[i]->connected)) {
                    BTA_JvRfcommStopServer(spp_local_param.spp_slots[i]->sdp_handle,
                                           (void *)spp_local_param.spp_slots[i]->id);
                    BTA_JvDeleteRecord(spp_local_param.spp_slots[i]->sdp_handle);
                    BTA_JvFreeChannel(spp_local_param.spp_slots[i]->scn, BTA_JV_CONN_TYPE_RFCOMM);
                    spp_free_slot(spp_local_param.spp_slots[i]);
                    spp_local_param.spp_slots[i] = NULL;
                }
            }

            BTA_JvDisable();
            break;
        }

        case BTIF_SPP_DISCOVERY: {
            tSDP_UUID uuid;
            uuid.len = 16;
            memcpy(uuid.uu.uuid128, p_cb->uuid.uu, 16);
            BTA_JvStartDiscovery(p_cb->bd_addr.address, p_cb->num_uuid, &uuid, NULL);
            break;
        }

        case BTIF_SPP_CONNECT: {
            spp_slot_t *slot = spp_malloc_slot();

            if(!slot) {
                BTIF_TRACE_ERROR("%s unable to malloc RFCOMM slot!", __func__);
                return;
            }

            slot->security = p_cb->sec_mask;
            slot->role = p_cb->role;
            slot->scn = p_cb->remote_scn;;
            memcpy(slot->addr.address, p_cb->bd_addr.address, 6);
            BTA_JvRfcommConnect(p_cb->sec_mask, p_cb->role, p_cb->remote_scn,
                                p_cb->bd_addr.address, (tBTA_JV_RFCOMM_CBACK *)btif_spp_rfcomm_inter_cb, (void *)slot->id);
            break;
        }

        case BTIF_SPP_DISCONNECT: {
            spp_slot_t *slot = spp_find_slot_by_handle(p_cb->handle);

            if(!slot) {
                BTIF_TRACE_ERROR("%s unable to find RFCOMM slot! disconnect fail!", __func__);
                return;
            }

            BTA_JvRfcommClose(p_cb->handle, (void *)slot->id);
            //btif_disconnect_cb(slot->rfc_handle);
            spp_free_slot(slot);
            break;
        }

        case BTIF_SPP_START_SERVER: {
            spp_slot_t *slot = spp_malloc_slot();

            if(!slot) {
                BTIF_TRACE_ERROR("%s unable to malloc RFCOMM slot!", __func__);
                return;
            }

            slot->security = p_cb->sec_mask;
            slot->role = p_cb->role;
            slot->scn = p_cb->local_scn;;
            slot->max_session = BTA_JV_MAX_RFC_SR_SESSION;
            strcpy(slot->service_name, p_cb->service_name);
            BTA_JvGetChannelId(BTA_JV_CONN_TYPE_RFCOMM, (void *)slot->id, p_cb->local_scn);
            break;
        }

        case BTIF_SPP_WRITE: {
            spp_slot_t *slot = spp_find_slot_by_handle(p_cb->handle);

            if(!slot) {
                BTIF_TRACE_ERROR("%s unable to find RFCOMM slot!", __func__);

                if(p_cb->data) { GKI_freebuf(p_cb->data); }

                return;
            }

            list_append(slot->list, (void *)p_cb->data);
            BTA_JvRfcommWrite(p_cb->handle, slot->id, p_cb->length, p_cb->data);
            break;
        }

        default:
            LOG_ERROR(LOG_TAG, "%s: Unknown event (%d)!", __FUNCTION__, event);
            break;
    }
}



/************************************************************************************
**  SPP API Functions
************************************************************************************/

tls_bt_status_t tls_bt_spp_init(tls_bt_spp_callback_t callback)
{
    if(tls_spp_cb == NULL) {
        tls_spp_cb = callback;
    } else {
        return TLS_BT_STATUS_DONE;
    }

    return TLS_BT_STATUS_SUCCESS;
}
tls_bt_status_t tls_bt_spp_deinit()
{
    if(tls_spp_cb == NULL) {
        return TLS_BT_STATUS_DONE;
    } else {
        tls_spp_cb = NULL;
    }

    return TLS_BT_STATUS_SUCCESS;
}

tls_bt_status_t tls_bt_spp_enable(void)
{
    CHECK_BTSPP_INIT();
    return btif_transfer_context(btspp_handle_event, BTIF_SPP_ENABLE,
                                 (char *) NULL, 0, NULL);
}

tls_bt_status_t tls_bt_spp_disable(void)
{
    CHECK_BTSPP_INIT();
    return btif_transfer_context(btspp_handle_event, BTIF_SPP_DISABLE,
                                 (char *) NULL, 0, NULL);
}
tls_bt_status_t tls_bt_spp_start_discovery(tls_bt_addr_t *bd_addr, tls_bt_uuid_t *uuid)
{
    CHECK_BTSPP_INIT();
    btif_spp_cb_t btif_cb;
    memcpy(&btif_cb.uuid, uuid, sizeof(tls_bt_uuid_t));
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    btif_cb.num_uuid = 1;
    return btif_transfer_context(btspp_handle_event, BTIF_SPP_DISCOVERY,
                                 (char *) &btif_cb, sizeof(btif_spp_cb_t), NULL);
}
tls_bt_status_t tls_bt_spp_connect(wm_spp_sec_t sec_mask, tls_spp_role_t role, uint8_t remote_scn,
                                   tls_bt_addr_t *bd_addr)
{
    CHECK_BTSPP_INIT();
    btif_spp_cb_t btif_cb;
    btif_cb.sec_mask = sec_mask;
    btif_cb.role = role;
    btif_cb.remote_scn = remote_scn;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    return btif_transfer_context(btspp_handle_event, BTIF_SPP_CONNECT,
                                 (char *) &btif_cb, sizeof(btif_spp_cb_t), NULL);
}
tls_bt_status_t tls_bt_spp_disconnect(uint32_t handle)
{
    CHECK_BTSPP_INIT();
    btif_spp_cb_t btif_cb;
    btif_cb.handle = handle;
    return btif_transfer_context(btspp_handle_event, BTIF_SPP_DISCONNECT,
                                 (char *) &btif_cb, sizeof(btif_spp_cb_t), NULL);
}
tls_bt_status_t tls_bt_spp_start_server(wm_spp_sec_t sec_mask, tls_spp_role_t role,
                                        uint8_t local_scn, const char *name)
{
    CHECK_BTSPP_INIT();
    btif_spp_cb_t btif_cb;
    btif_cb.sec_mask = sec_mask;
    btif_cb.role = role;
    btif_cb.local_scn = local_scn;
    strcpy(btif_cb.service_name, name);
    return btif_transfer_context(btspp_handle_event, BTIF_SPP_START_SERVER,
                                 (char *) &btif_cb, sizeof(btif_spp_cb_t), NULL);
}

tls_bt_status_t tls_bt_spp_write(uint32_t handle, uint8_t *p_data, uint16_t length)
{
    CHECK_BTSPP_INIT();
    btif_spp_cb_t btif_cb;
    tls_bt_status_t status;
    btif_cb.length = length;
    btif_cb.data = (uint8_t *)GKI_getbuf(length);

    if(btif_cb.data == NULL) {
        return TLS_BT_STATUS_NOMEM;
    }

    memcpy(btif_cb.data, p_data, length);
    btif_cb.handle = handle;
    status = btif_transfer_context(btspp_handle_event, BTIF_SPP_WRITE,
                                   (char *) &btif_cb, sizeof(btif_spp_cb_t), NULL);

    if(status != TLS_BT_STATUS_SUCCESS) {
        GKI_freebuf(btif_cb.data);
    }

    return status;
}

#endif
