/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*****************************************************************************
 *
 *  Filename:      btif_rc.c
 *
 *  Description:   Bluetooth AVRC implementation
 *
 *****************************************************************************/

#define LOG_TAG "bt_btif_avrc"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bt_target.h"
#if defined(BTA_AV_INCLUDED) && (BTA_AV_INCLUDED == TRUE) || defined(BTA_AV_SINK_INCLUDED) && (BTA_AV_SINK_INCLUDED == TRUE)

#include "bluetooth.h"
#include "bt_rc.h"
#include "gki.h"
#include "avrc_defs.h"
#include "bta_api.h"
#include "bta_av_api.h"
#include "btif_av.h"
#include "btif_common.h"
#include "btif_util.h"
#include "bt_common.h"
#include "uinput.h"
#include "bdaddr.h"
#include "osi/include/list.h"
#include "btu.h"
#include "wm_bt.h"
#include "bt_utils.h"
#define RC_INVALID_TRACK_ID (0xFFFFFFFFFFFFFFFFULL)


/*****************************************************************************
**  Constants & Macros
******************************************************************************/

/* cod value for Headsets */
#define COD_AV_HEADSETS        0x0404
/* for AVRC 1.4 need to change this */
#define MAX_RC_NOTIFICATIONS AVRC_EVT_VOLUME_CHANGE

#define IDX_GET_PLAY_STATUS_RSP   0
#define IDX_LIST_APP_ATTR_RSP     1
#define IDX_LIST_APP_VALUE_RSP    2
#define IDX_GET_CURR_APP_VAL_RSP  3
#define IDX_SET_APP_VAL_RSP       4
#define IDX_GET_APP_ATTR_TXT_RSP  5
#define IDX_GET_APP_VAL_TXT_RSP   6
#define IDX_GET_ELEMENT_ATTR_RSP  7
#define MAX_VOLUME 128
#define MAX_LABEL 16
#define MAX_TRANSACTIONS_PER_SESSION 8
#define MAX_CMD_QUEUE_LEN 8
#define PLAY_STATUS_PLAYING 1

#define CHECK_RC_CONNECTED                                                                  \
    BTIF_TRACE_DEBUG("## %s ##", __FUNCTION__);                                            \
    if (btif_rc_cb.rc_connected == FALSE)                                                    \
    {                                                                                       \
        BTIF_TRACE_WARNING("Function %s() called when RC is not connected", __FUNCTION__); \
        return TLS_BT_STATUS_NOT_READY;                                                         \
    }

#define FILL_PDU_QUEUE(index, ctype, label, pending)        \
    {                                                           \
        btif_rc_cb.rc_pdu_info[index].ctype = ctype;            \
        btif_rc_cb.rc_pdu_info[index].label = label;            \
        btif_rc_cb.rc_pdu_info[index].is_rsp_pending = pending; \
    }

#define SEND_METAMSG_RSP(index, avrc_rsp)                                                      \
    {                                                                                              \
        if (btif_rc_cb.rc_pdu_info[index].is_rsp_pending == FALSE)                                  \
        {                                                                                          \
            BTIF_TRACE_WARNING("%s Not sending response as no PDU was registered", __FUNCTION__); \
            return TLS_BT_STATUS_UNHANDLED;                                                            \
        }                                                                                          \
        send_metamsg_rsp(btif_rc_cb.rc_handle, btif_rc_cb.rc_pdu_info[index].label,                \
                         btif_rc_cb.rc_pdu_info[index].ctype, avrc_rsp);                                        \
        btif_rc_cb.rc_pdu_info[index].ctype = 0;                                                   \
        btif_rc_cb.rc_pdu_info[index].label = 0;                                                   \
        btif_rc_cb.rc_pdu_info[index].is_rsp_pending = FALSE;                                      \
    }

/*****************************************************************************
**  Local type definitions
******************************************************************************/
typedef struct {
    uint8_t bNotify;
    uint8_t label;
} btif_rc_reg_notifications_t;

typedef struct {
    uint8_t   label;
    uint8_t   ctype;
    uint8_t is_rsp_pending;
} btif_rc_cmd_ctxt_t;

/* 2 second timeout to get interim response */
#define BTIF_TIMEOUT_RC_INTERIM_RSP_MS     (2 * 1000)
#define BTIF_TIMEOUT_RC_STATUS_CMD_MS      (2 * 1000)
#define BTIF_TIMEOUT_RC_CONTROL_CMD_MS     (2 * 1000)


typedef enum {
    eNOT_REGISTERED,
    eREGISTERED,
    eINTERIM
} btif_rc_nfn_reg_status_t;

typedef struct {
    uint8_t                       event_id;
    uint8_t                       label;
    btif_rc_nfn_reg_status_t    status;
} btif_rc_supported_event_t;

#define BTIF_RC_STS_TIMEOUT     0xFE
typedef struct {
    uint8_t   label;
    uint8_t   pdu_id;
} btif_rc_status_cmd_timer_t;

typedef struct {
    uint8_t   label;
    uint8_t   pdu_id;
} btif_rc_control_cmd_timer_t;

typedef struct {
    union {
        btif_rc_status_cmd_timer_t rc_status_cmd;
        btif_rc_control_cmd_timer_t rc_control_cmd;
    };
} btif_rc_timer_context_t;

typedef struct {
    uint8_t  query_started;
    uint8_t num_attrs;
    uint8_t num_ext_attrs;

    uint8_t attr_index;
    uint8_t ext_attr_index;
    uint8_t ext_val_index;
    btrc_player_app_attr_t attrs[AVRC_MAX_APP_ATTR_SIZE];
    btrc_player_app_ext_attr_t ext_attrs[AVRC_MAX_APP_ATTR_SIZE];
} btif_rc_player_app_settings_t;

/* TODO : Merge btif_rc_reg_notifications_t and btif_rc_cmd_ctxt_t to a single struct */
typedef struct {
    uint8_t                     rc_connected;
    uint8_t                       rc_handle;
    tBTA_AV_FEAT                rc_features;
    BD_ADDR                     rc_addr;
    uint16_t                      rc_pending_play;
    btif_rc_cmd_ctxt_t          rc_pdu_info[MAX_CMD_QUEUE_LEN];
    btif_rc_reg_notifications_t rc_notif[MAX_RC_NOTIFICATIONS];
    unsigned int                rc_volume;
    uint8_t                     rc_vol_label;
    list_t                      *rc_supported_event_list;
    btif_rc_player_app_settings_t   rc_app_settings;
    TIMER_LIST_ENT  rc_play_status_timer;
    uint8_t                     rc_features_processed;
    uint64_t                      rc_playing_uid;
    uint8_t                     rc_procedure_complete;
} btif_rc_cb_t;

typedef struct {
    uint8_t in_use;
    uint8_t lbl;
    uint8_t handle;
    btif_rc_timer_context_t txn_timer_context;
    TIMER_LIST_ENT  txn_timer;
} rc_transaction_t;

typedef struct {
    //pthread_mutex_t lbllock;
    rc_transaction_t transaction[MAX_TRANSACTIONS_PER_SESSION];
} rc_device_t;

rc_device_t device;

#define MAX_UINPUT_PATHS 3
//static const char *uinput_dev_path[] =
//{"/dev/uinput", "/dev/input/uinput", "/dev/misc/uinput" };
static int uinput_fd = -1;

static int  send_event(int fd, uint16_t type, uint16_t code, int32_t value);
static void send_key(int fd, uint16_t key, int pressed);
static int  uinput_driver_check();
static int  uinput_create(char *name);
static int  init_uinput(void);
static void close_uinput(void);

static const struct {
    const char *name;
    uint8_t avrcp;
    uint16_t mapped_id;
    uint8_t release_quirk;
} key_map[] = {
    { "PLAY",         AVRC_ID_PLAY,     KEY_PLAYCD,       1 },
    { "STOP",         AVRC_ID_STOP,     KEY_STOPCD,       0 },
    { "PAUSE",        AVRC_ID_PAUSE,    KEY_PAUSECD,      1 },
    { "FORWARD",      AVRC_ID_FORWARD,  KEY_NEXTSONG,     0 },
    { "BACKWARD",     AVRC_ID_BACKWARD, KEY_PREVIOUSSONG, 0 },
    { "REWIND",       AVRC_ID_REWIND,   KEY_REWIND,       0 },
    { "FAST FORWARD", AVRC_ID_FAST_FOR, KEY_FAST_FORWARD, 0 },
    { NULL,           0,                0,                0 }
};
//static const uint8_t rc_black_addr_prefix[][3] = {
//    {0x0, 0x18, 0x6B}, // LG HBS-730
//    {0x0, 0x26, 0x7E}  // VW Passat
//};

static const uint8_t rc_white_addr_prefix[][3] = {
    {0x94, 0xCE, 0x2C}, // Sony SBH50
    {0x30, 0x17, 0xC8} // Sony wm600
};

static const char *rc_white_name[] = {
    "SBH50",
    "MW600"
};
static void send_reject_response(uint8_t rc_handle, uint8_t label,
                                 uint8_t pdu, uint8_t status);
static uint8_t opcode_from_pdu(uint8_t pdu);
static void send_metamsg_rsp(uint8_t rc_handle, uint8_t label,
                             tBTA_AV_CODE code, tAVRC_RESPONSE *pmetamsg_resp);
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
static void register_volumechange(uint8_t label);
#endif
static void lbl_init();
static void lbl_destroy();
static void init_all_transactions();
static tls_bt_status_t  get_transaction(rc_transaction_t **ptransaction);
static void release_transaction(uint8_t label);
static rc_transaction_t *get_transaction_by_lbl(uint8_t label);
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
static void handle_rc_metamsg_rsp(tBTA_AV_META_MSG *pmeta_msg);
#endif
#if (AVRC_CTLR_INCLUDED == TRUE)
static void handle_avk_rc_metamsg_cmd(tBTA_AV_META_MSG *pmeta_msg);
static void handle_avk_rc_metamsg_rsp(tBTA_AV_META_MSG *pmeta_msg);
static void btif_rc_ctrl_upstreams_rsp_cmd(
                uint8_t event, tAVRC_COMMAND *pavrc_cmd, uint8_t label);
static void rc_ctrl_procedure_complete();
static void rc_stop_play_status_timer();
static void register_for_event_notification(btif_rc_supported_event_t *p_event);
static void handle_get_capability_response(tBTA_AV_META_MSG *pmeta_msg, tAVRC_GET_CAPS_RSP *p_rsp);
static void handle_app_attr_response(tBTA_AV_META_MSG *pmeta_msg, tAVRC_LIST_APP_ATTR_RSP *p_rsp);
static void handle_app_val_response(tBTA_AV_META_MSG *pmeta_msg, tAVRC_LIST_APP_VALUES_RSP *p_rsp);
static void handle_app_cur_val_response(tBTA_AV_META_MSG *pmeta_msg,
                                        tAVRC_GET_CUR_APP_VALUE_RSP *p_rsp);
static void handle_app_attr_txt_response(tBTA_AV_META_MSG *pmeta_msg,
        tAVRC_GET_APP_ATTR_TXT_RSP *p_rsp);
static void handle_app_attr_val_txt_response(tBTA_AV_META_MSG *pmeta_msg,
        tAVRC_GET_APP_ATTR_TXT_RSP *p_rsp);
static void handle_get_playstatus_response(tBTA_AV_META_MSG *pmeta_msg,
        tAVRC_GET_PLAY_STATUS_RSP *p_rsp);
static void handle_get_elem_attr_response(tBTA_AV_META_MSG *pmeta_msg,
        tAVRC_GET_ELEM_ATTRS_RSP *p_rsp);
static void handle_set_app_attr_val_response(tBTA_AV_META_MSG *pmeta_msg, tAVRC_RSP *p_rsp);
static tls_bt_status_t get_play_status_cmd(void);
static tls_bt_status_t get_player_app_setting_attr_text_cmd(uint8_t *attrs, uint8_t num_attrs);
static tls_bt_status_t get_player_app_setting_value_text_cmd(uint8_t *vals, uint8_t num_vals);
static tls_bt_status_t register_notification_cmd(uint8_t label, uint8_t event_id,
        uint32_t event_value);
static tls_bt_status_t get_element_attribute_cmd(uint8_t num_attribute, uint32_t *p_attr_ids);
static tls_bt_status_t getcapabilities_cmd(uint8_t cap_id);
static tls_bt_status_t list_player_app_setting_attrib_cmd(void);
static tls_bt_status_t list_player_app_setting_value_cmd(uint8_t attrib_id);
static tls_bt_status_t get_player_app_setting_cmd(uint8_t num_attrib, uint8_t *attrib_ids);
#endif
static void btif_rc_upstreams_evt(uint16_t event, tAVRC_COMMAND *p_param, uint8_t ctype,
                                  uint8_t label);
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
static void btif_rc_upstreams_rsp_evt(uint16_t event, tAVRC_RESPONSE *pavrc_resp, uint8_t ctype,
                                      uint8_t label);
#endif
static void rc_start_play_status_timer(void);
//static uint8_t absolute_volume_disabled(void);
static uint8_t dev_blacklisted_for_absolute_volume(BD_ADDR peer_dev);
/*****************************************************************************
**  Static variables
******************************************************************************/
static btif_rc_cb_t btif_rc_cb;
static tls_btrc_callback_t bt_rc_callbacks = NULL;
static tls_btrc_ctrl_callback_t bt_rc_ctrl_callbacks = NULL;

/*****************************************************************************
**  Static functions
******************************************************************************/

/*****************************************************************************
**  Externs
******************************************************************************/
extern uint8_t btif_hf_call_terminated_recently();
extern uint8_t check_cod(const tls_bt_addr_t *remote_bdaddr, uint32_t cod);
#ifdef USE_ALARM
extern fixed_queue_t *btu_general_alarm_queue;
#endif

/*****************************************************************************
**  Functions
******************************************************************************/

/*****************************************************************************
**   Local uinput helper functions
******************************************************************************/
int send_event(int fd, uint16_t type, uint16_t code, int32_t value)
{
#if 0
    struct uinput_event event;
    BTIF_TRACE_DEBUG("%s type:%u code:%u value:%d", __FUNCTION__,
                     type, code, value);
    wm_memset(&event, 0, sizeof(event));
    event.type  = type;
    event.code  = code;
    event.value = value;
    return write(fd, &event, sizeof(event));
#endif
	return 0;
}

void send_key(int fd, uint16_t key, int pressed)
{
    BTIF_TRACE_DEBUG("%s fd:%d key:%u pressed:%d", __FUNCTION__,
                     fd, key, pressed);

    if(fd < 0) {
        return;
    }

    BTIF_TRACE_DEBUG("AVRCP: Send key %d (%d) fd=%d", key, pressed, fd);
    send_event(fd, 1, key, pressed);
    send_event(fd, 0, 0, 0);
}

/************** uinput related functions **************/
int uinput_driver_check()
{
#if 0
    uint32_t i;

    for(i = 0; i < MAX_UINPUT_PATHS; i++) {
        if(access(uinput_dev_path[i], O_RDWR) == 0) {
            return 0;
        }
    }

    BTIF_TRACE_ERROR("%s ERROR: uinput device is not in the system", __FUNCTION__);
    return -1;
#endif
    return 0;
}

int uinput_create(char *name)
{
#if 0
    struct uinput_dev dev;
    int fd, x = 0;

    for(x = 0; x < MAX_UINPUT_PATHS; x++) {
        fd = open(uinput_dev_path[x], O_RDWR);

        if(fd < 0) {
            continue;
        }

        break;
    }

    if(x == MAX_UINPUT_PATHS) {
        BTIF_TRACE_ERROR("%s ERROR: uinput device open failed", __FUNCTION__);
        return -1;
    }

    wm_memset(&dev, 0, sizeof(dev));

    if(name) {
        strncpy(dev.name, name, UINPUT_MAX_NAME_SIZE - 1);
    }

    dev.id.bustype = BUS_BLUETOOTH;
    dev.id.vendor  = 0x0000;
    dev.id.product = 0x0000;
    dev.id.version = 0x0000;
    ssize_t ret;
    OSI_NO_INTR(ret = write(fd, &dev, sizeof(dev)));

    if(ret < 0) {
        BTIF_TRACE_ERROR("%s Unable to write device information", __FUNCTION__);
        close(fd);
        return -1;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);

    for(x = 0; key_map[x].name != NULL; x++) {
        ioctl(fd, UI_SET_KEYBIT, key_map[x].mapped_id);
    }

    if(ioctl(fd, UI_DEV_CREATE, NULL) < 0) {
        BTIF_TRACE_ERROR("%s Unable to create uinput device", __FUNCTION__);
        close(fd);
        return -1;
    }

    return fd;
#endif
	return 0;
}

int init_uinput(void)
{
    char *name = "AVRCP";
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    uinput_fd = uinput_create(name);

    if(uinput_fd < 0) {
        BTIF_TRACE_ERROR("%s AVRCP: Failed to initialize uinput for %s (%d)",
                         __FUNCTION__, name, uinput_fd);
    } else {
        BTIF_TRACE_DEBUG("%s AVRCP: Initialized uinput for %s (fd=%d)",
                         __FUNCTION__, name, uinput_fd);
    }

    return uinput_fd;
}

void close_uinput(void)
{
#if 0
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    if(uinput_fd > 0) {
        ioctl(uinput_fd, UI_DEV_DESTROY);
        close(uinput_fd);
        uinput_fd = -1;
    }

#endif
}
#if (AVRC_CTLR_INCLUDED == TRUE)
void rc_cleanup_sent_cmd(void *p_data)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
}

void handle_rc_ctrl_features(BD_ADDR bd_addr)
{
    tls_btrc_ctrl_msg_t msg;

    if((btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG) ||
            ((btif_rc_cb.rc_features & BTA_AV_FEAT_RCCT) &&
             (btif_rc_cb.rc_features & BTA_AV_FEAT_ADV_CTRL))) {
        tls_bt_addr_t rc_addr;
        int rc_features = 0;
        bdcpy(rc_addr.address, bd_addr);

        if((btif_rc_cb.rc_features & BTA_AV_FEAT_ADV_CTRL) &&
                (btif_rc_cb.rc_features & BTA_AV_FEAT_RCCT)) {
            rc_features |= BTRC_FEAT_ABSOLUTE_VOLUME;
        }

        if((btif_rc_cb.rc_features & BTA_AV_FEAT_METADATA) &&
                (btif_rc_cb.rc_features & BTA_AV_FEAT_VENDOR) &&
                (btif_rc_cb.rc_features_processed != TRUE)) {
            rc_features |= BTRC_FEAT_METADATA;
            /* Mark rc features processed to avoid repeating
             * the AVRCP procedure every time on receiving this
             * update.
             */
            btif_rc_cb.rc_features_processed = TRUE;

            if(btif_av_is_sink_enabled()) {
                getcapabilities_cmd(AVRC_CAP_COMPANY_ID);
            }
        }

        BTIF_TRACE_DEBUG("%s Update rc features to CTRL %d", __FUNCTION__, rc_features);
        msg.getrcfeatures.bd_addr = &rc_addr;
        msg.getrcfeatures.features = rc_features;
        bt_rc_ctrl_callbacks(WM_BTRC_CTRL_GETRCFEATURES_EVT, &msg);
        //HAL_CBACK(bt_rc_ctrl_callbacks, getrcfeatures_cb, &rc_addr, rc_features);
    }
}
#endif


void handle_rc_features(BD_ADDR bd_addr)
{
    if(bt_rc_callbacks != NULL) {
        btrc_remote_features_t rc_features = BTRC_FEAT_NONE;
        tls_bt_addr_t rc_addr;
        bdcpy(rc_addr.address, btif_rc_cb.rc_addr);
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)		
        tls_bt_addr_t avdtp_addr  = btif_av_get_addr();
        bdstr_t addr1, addr2;
#endif		
        BTIF_TRACE_DEBUG("%s: AVDTP Address: %s AVCTP address: %s", __func__,
                         bdaddr_to_string(&avdtp_addr, addr1, sizeof(addr1)),
                         bdaddr_to_string(&rc_addr, addr2, sizeof(addr2)));

        if(dev_blacklisted_for_absolute_volume(btif_rc_cb.rc_addr)) {
            btif_rc_cb.rc_features &= ~BTA_AV_FEAT_ADV_CTRL;
        }

        if(btif_rc_cb.rc_features & BTA_AV_FEAT_BROWSE) {
            rc_features |= BTRC_FEAT_BROWSE;
        }

#if (AVRC_ADV_CTRL_INCLUDED == TRUE)

        if((btif_rc_cb.rc_features & BTA_AV_FEAT_ADV_CTRL) &&
                (btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG)) {
            rc_features |= BTRC_FEAT_ABSOLUTE_VOLUME;
        }

#endif

        if(btif_rc_cb.rc_features & BTA_AV_FEAT_METADATA) {
            rc_features |= BTRC_FEAT_METADATA;
        }

        BTIF_TRACE_DEBUG("%s: rc_features=0x%x", __FUNCTION__, rc_features);
        tls_btrc_msg_t msg;
        msg.remote_features.bd_addr = &rc_addr;
        msg.remote_features.features = rc_features;
        bt_rc_callbacks(WM_BTRC_REMOTE_FEATURE_EVT, &msg);
        //HAL_CBACK(bt_rc_callbacks, remote_features_cb, &rc_addr, rc_features)
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
        BTIF_TRACE_DEBUG("%s Checking for feature flags in btif_rc_handler with label %d",
                         __FUNCTION__, btif_rc_cb.rc_vol_label);

        // Register for volume change on connect
        if(btif_rc_cb.rc_features & BTA_AV_FEAT_ADV_CTRL &&
                btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG) {
            rc_transaction_t *p_transaction = NULL;
            tls_bt_status_t status = TLS_BT_STATUS_NOT_READY;

            if(MAX_LABEL == btif_rc_cb.rc_vol_label) {
                status = get_transaction(&p_transaction);
            } else {
                p_transaction = get_transaction_by_lbl(btif_rc_cb.rc_vol_label);

                if(NULL != p_transaction) {
                    BTIF_TRACE_DEBUG("%s register_volumechange already in progress for label %d",
                                     __FUNCTION__, btif_rc_cb.rc_vol_label);
                    return;
                } else {
                    status = get_transaction(&p_transaction);
                }
            }

            if(TLS_BT_STATUS_SUCCESS == status && NULL != p_transaction) {
                btif_rc_cb.rc_vol_label = p_transaction->lbl;
                register_volumechange(btif_rc_cb.rc_vol_label);
            }
        }

#endif
    }
}

/***************************************************************************
 *  Function       handle_rc_connect
 *
 *  - Argument:    tBTA_AV_RC_OPEN  RC open data structure
 *
 *  - Description: RC connection event handler
 *
 ***************************************************************************/
void handle_rc_connect(tBTA_AV_RC_OPEN *p_rc_open)
{
    BTIF_TRACE_DEBUG("%s: rc_handle: %d", __FUNCTION__, p_rc_open->rc_handle);
    tls_bt_status_t result = TLS_BT_STATUS_SUCCESS;
#if (AVRC_CTLR_INCLUDED == TRUE)
    tls_bt_addr_t rc_addr;
    tls_btrc_ctrl_msg_t msg;
#endif

    if(p_rc_open->status == BTA_AV_SUCCESS) {
        //check if already some RC is connected
        if(btif_rc_cb.rc_connected) {
            BTIF_TRACE_ERROR("%s Got RC OPEN in connected state, Connected RC: %d \
                and Current RC: %d", __FUNCTION__, btif_rc_cb.rc_handle, p_rc_open->rc_handle);

            if((btif_rc_cb.rc_handle != p_rc_open->rc_handle)
                    && (bdcmp(btif_rc_cb.rc_addr, p_rc_open->peer_addr))) {
                BTIF_TRACE_DEBUG("%s Got RC connected for some other handle", __FUNCTION__);
                BTA_AvCloseRc(p_rc_open->rc_handle);
                return;
            }
        }

        wm_memcpy(btif_rc_cb.rc_addr, p_rc_open->peer_addr, sizeof(BD_ADDR));
        btif_rc_cb.rc_features = p_rc_open->peer_features;
        btif_rc_cb.rc_vol_label = MAX_LABEL;
        btif_rc_cb.rc_volume = MAX_VOLUME;
        btif_rc_cb.rc_connected = TRUE;
        btif_rc_cb.rc_handle = p_rc_open->rc_handle;

        /* on locally initiated connection we will get remote features as part of connect */
        if(btif_rc_cb.rc_features != 0) {
            handle_rc_features(btif_rc_cb.rc_addr);
        }

        if(bt_rc_callbacks) {
            result = uinput_driver_check();

            if(result == TLS_BT_STATUS_SUCCESS) {
                init_uinput();
            }
        } else {
            BTIF_TRACE_WARNING("%s Avrcp TG role not enabled, not initializing UInput",
                               __FUNCTION__);
        }

        BTIF_TRACE_DEBUG("%s handle_rc_connect features %d ", __FUNCTION__, btif_rc_cb.rc_features);
#if (AVRC_CTLR_INCLUDED == TRUE)
        btif_rc_cb.rc_playing_uid = RC_INVALID_TRACK_ID;
        bdcpy(rc_addr.address, btif_rc_cb.rc_addr);

        if(bt_rc_ctrl_callbacks != NULL) {
            msg.connection_state.bd_addr = &rc_addr;
            msg.connection_state.state = TRUE;
            bt_rc_ctrl_callbacks(WM_BTRC_CONNECTION_STATE_EVT, &msg);
            //HAL_CBACK(bt_rc_ctrl_callbacks, connection_state_cb, TRUE, &rc_addr);
        }

        /* report connection state if remote device is AVRCP target */
        if((btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG) ||
                ((btif_rc_cb.rc_features & BTA_AV_FEAT_RCCT) &&
                 (btif_rc_cb.rc_features & BTA_AV_FEAT_ADV_CTRL))) {
            handle_rc_ctrl_features(btif_rc_cb.rc_addr);
        }

#endif
    } else {
        BTIF_TRACE_ERROR("%s Connect failed with error code: %d",
                         __FUNCTION__, p_rc_open->status);
        btif_rc_cb.rc_connected = FALSE;
    }
}

/***************************************************************************
 *  Function       handle_rc_disconnect
 *
 *  - Argument:    tBTA_AV_RC_CLOSE     RC close data structure
 *
 *  - Description: RC disconnection event handler
 *
 ***************************************************************************/
void handle_rc_disconnect(tBTA_AV_RC_CLOSE *p_rc_close)
{
#if (AVRC_CTLR_INCLUDED == TRUE)
    tls_bt_addr_t rc_addr;
//    tBTA_AV_FEAT features;
    tls_btrc_ctrl_msg_t msg;
#endif
    BTIF_TRACE_DEBUG("%s: rc_handle: %d", __FUNCTION__, p_rc_close->rc_handle);

    if((p_rc_close->rc_handle != btif_rc_cb.rc_handle)
            && (bdcmp(btif_rc_cb.rc_addr, p_rc_close->peer_addr))) {
        BTIF_TRACE_ERROR("Got disconnect of unknown device");
        return;
    }

#if (AVRC_CTLR_INCLUDED == TRUE)
    bdcpy(rc_addr.address, btif_rc_cb.rc_addr);
//    features = btif_rc_cb.rc_features;
    /* Clean up AVRCP procedure flags */
    wm_memset(&btif_rc_cb.rc_app_settings, 0,
              sizeof(btif_rc_player_app_settings_t));
    btif_rc_cb.rc_features_processed = FALSE;
    btif_rc_cb.rc_procedure_complete = FALSE;
    rc_stop_play_status_timer();

    /* Check and clear the notification event list */
    if(btif_rc_cb.rc_supported_event_list != NULL) {
        list_clear(btif_rc_cb.rc_supported_event_list);
        btif_rc_cb.rc_supported_event_list = NULL;
    }

#endif
    btif_rc_cb.rc_handle = 0;
    btif_rc_cb.rc_connected = FALSE;
    wm_memset(btif_rc_cb.rc_addr, 0, sizeof(BD_ADDR));
    wm_memset(btif_rc_cb.rc_notif, 0, sizeof(btif_rc_cb.rc_notif));
    btif_rc_cb.rc_features = 0;
    btif_rc_cb.rc_vol_label = MAX_LABEL;
    btif_rc_cb.rc_volume = MAX_VOLUME;
    init_all_transactions();

    if(bt_rc_callbacks != NULL) {
        close_uinput();
    } else {
        BTIF_TRACE_WARNING("%s Avrcp TG role not enabled, not closing UInput", __FUNCTION__);
    }

    wm_memset(btif_rc_cb.rc_addr, 0, sizeof(BD_ADDR));
#if (AVRC_CTLR_INCLUDED == TRUE)

    /* report connection state if device is AVRCP target */
    if(bt_rc_ctrl_callbacks != NULL) {
        msg.connection_state.bd_addr = &rc_addr;
        msg.connection_state.state = FALSE;
        bt_rc_ctrl_callbacks(WM_BTRC_CONNECTION_STATE_EVT, &msg);
        //HAL_CBACK(bt_rc_ctrl_callbacks, connection_state_cb, FALSE, &rc_addr);
    }

#endif
}

/***************************************************************************
 *  Function       handle_rc_passthrough_cmd
 *
 *  - Argument:    tBTA_AV_RC rc_id   remote control command ID
 *                 tBTA_AV_STATE key_state status of key press
 *
 *  - Description: Remote control command handler
 *
 ***************************************************************************/
void handle_rc_passthrough_cmd(tBTA_AV_REMOTE_CMD *p_remote_cmd)
{
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)
    const char *status;
#endif
    int pressed, i;
    tls_btrc_msg_t msg;
    BTIF_TRACE_DEBUG("%s: p_remote_cmd->rc_id=%d", __FUNCTION__, p_remote_cmd->rc_id);

    /* If AVRC is open and peer sends PLAY but there is no AVDT, then we queue-up this PLAY */
    if(p_remote_cmd) {
        /* queue AVRC PLAY if GAVDTP Open notification to app is pending (2 second timer) */
        if((p_remote_cmd->rc_id == BTA_AV_RC_PLAY) && (!btif_av_is_connected())) {
            if(p_remote_cmd->key_state == AVRC_STATE_PRESS) {
                APPL_TRACE_WARNING("%s: AVDT not open, queuing the PLAY command", __FUNCTION__);
                btif_rc_cb.rc_pending_play = TRUE;
            }

            return;
        }

        if((p_remote_cmd->rc_id == BTA_AV_RC_PAUSE) && (btif_rc_cb.rc_pending_play)) {
            APPL_TRACE_WARNING("%s: Clear the pending PLAY on PAUSE received", __FUNCTION__);
            btif_rc_cb.rc_pending_play = FALSE;
            return;
        }

        if((p_remote_cmd->rc_id == BTA_AV_RC_VOL_UP) || (p_remote_cmd->rc_id == BTA_AV_RC_VOL_DOWN)) {
            return;    // this command is not to be sent to UINPUT, only needed for PTS
        }
    }

    if((p_remote_cmd->rc_id == BTA_AV_RC_STOP) && (!btif_av_stream_started_ready())) {
        APPL_TRACE_WARNING("%s: Stream suspended, ignore STOP cmd", __FUNCTION__);
        return;
    }

    if(p_remote_cmd->key_state == AVRC_STATE_RELEASE) {
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)		
        status = "released";
#endif
        pressed = 0;
    } else {
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)    
        status = "pressed";
#endif
        pressed = 1;
    }

    if(p_remote_cmd->rc_id == BTA_AV_RC_FAST_FOR || p_remote_cmd->rc_id == BTA_AV_RC_REWIND) {
        msg.passthrough_cmd.id = p_remote_cmd->rc_id;
        msg.passthrough_cmd.key_state = pressed;
        bt_rc_callbacks(WM_BTRC_PASSTHROUGH_CMD_EVT, &msg);
        //HAL_CBACK(bt_rc_callbacks, passthrough_cmd_cb, p_remote_cmd->rc_id, pressed);
        return;
    }

    for(i = 0; key_map[i].name != NULL; i++) {
        if(p_remote_cmd->rc_id == key_map[i].avrcp) {
            BTIF_TRACE_DEBUG("%s: %s %s", __FUNCTION__, key_map[i].name, status);

            /* MusicPlayer uses a long_press_timeout of 1 second for PLAYPAUSE button
             * and maps that to autoshuffle. So if for some reason release for PLAY/PAUSE
             * comes 1 second after the press, the MediaPlayer UI goes into a bad state.
             * The reason for the delay could be sniff mode exit or some AVDTP procedure etc.
             * The fix is to generate a release right after the press and drown the 'actual'
             * release.
             */
            if((key_map[i].release_quirk == 1) && (pressed == 0)) {
                BTIF_TRACE_DEBUG("%s: AVRC %s Release Faked earlier, drowned now",
                                 __FUNCTION__, key_map[i].name);
                return;
            }

            send_key(uinput_fd, key_map[i].mapped_id, pressed);

            if((key_map[i].release_quirk == 1) && (pressed == 1)) {
                GKI_delay(30);   // 30ms
                BTIF_TRACE_DEBUG("%s: AVRC %s Release quirk enabled, send release now",
                                 __FUNCTION__, key_map[i].name);
                send_key(uinput_fd, key_map[i].mapped_id, 0);
            }

            break;
        }
    }

    if(key_map[i].name == NULL)
        BTIF_TRACE_ERROR("%s AVRCP: unknown button 0x%02X %s", __FUNCTION__,
                         p_remote_cmd->rc_id, status);
}

/***************************************************************************
 *  Function       handle_rc_passthrough_rsp
 *
 *  - Argument:    tBTA_AV_REMOTE_RSP passthrough command response
 *
 *  - Description: Remote control passthrough response handler
 *
 ***************************************************************************/
void handle_rc_passthrough_rsp(tBTA_AV_REMOTE_RSP *p_remote_rsp)
{
#if (AVRC_CTLR_INCLUDED == TRUE)
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)
    const char *status;
#endif
    tls_btrc_ctrl_msg_t msg;

    if(btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG) {
        int key_state;

        if(p_remote_rsp->key_state == AVRC_STATE_RELEASE) {
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)			
            status = "released";
#endif
            key_state = 1;
        } else {
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)        
            status = "pressed";
#endif
            key_state = 0;
        }

        BTIF_TRACE_DEBUG("%s: rc_id=%d status=%s", __FUNCTION__, p_remote_rsp->rc_id, status);
        release_transaction(p_remote_rsp->label);

        if(bt_rc_ctrl_callbacks != NULL) {
            msg.passthrough_rsp.id = p_remote_rsp->rc_id;
            msg.passthrough_rsp.key_state = key_state;
            bt_rc_ctrl_callbacks(WM_BTRC_PASSTHROUGH_RSP_EVT, &msg);
            //HAL_CBACK(bt_rc_ctrl_callbacks, passthrough_rsp_cb, p_remote_rsp->rc_id, key_state);
        }
    } else {
        BTIF_TRACE_ERROR("%s DUT does not support AVRCP controller role", __FUNCTION__);
    }

#else
    BTIF_TRACE_ERROR("%s AVRCP controller role is not enabled", __FUNCTION__);
#endif
}

/***************************************************************************
 *  Function       handle_rc_vendorunique_rsp
 *
 *  - Argument:    tBTA_AV_REMOTE_RSP  command response
 *
 *  - Description: Remote control vendor unique response handler
 *
 ***************************************************************************/
void handle_rc_vendorunique_rsp(tBTA_AV_REMOTE_RSP *p_remote_rsp)
{
#if (AVRC_CTLR_INCLUDED == TRUE)
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)
    const char *status;
#endif
    uint8_t vendor_id = 0;
    tls_btrc_ctrl_msg_t msg;

    if(btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG) {
        int key_state;

        if(p_remote_rsp->key_state == AVRC_STATE_RELEASE) {
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)			
            status = "released";
#endif
            key_state = 1;
        } else {
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)        
            status = "pressed";
#endif
            key_state = 0;
        }

        if(p_remote_rsp->len > 0) {
            if(p_remote_rsp->len >= AVRC_PASS_THRU_GROUP_LEN) {
                vendor_id = p_remote_rsp->p_data[AVRC_PASS_THRU_GROUP_LEN - 1];
            }

            GKI_free_and_reset_buf((void **)&p_remote_rsp->p_data);
        }

        BTIF_TRACE_DEBUG("%s: vendor_id=%d status=%s", __FUNCTION__, vendor_id, status);
        release_transaction(p_remote_rsp->label);
        msg.groupnavigation_rsp.id = vendor_id;
        msg.groupnavigation_rsp.key_state = key_state;
        bt_rc_ctrl_callbacks(WM_BTRC_GROUPNAVIGATION_RSP_EVT, &msg);
        //HAL_CBACK(bt_rc_ctrl_callbacks, groupnavigation_rsp_cb, vendor_id, key_state);
    } else {
        BTIF_TRACE_ERROR("%s Remote does not support AVRCP TG role", __FUNCTION__);
    }

#else
    BTIF_TRACE_ERROR("%s AVRCP controller role is not enabled", __FUNCTION__);
#endif
}

void handle_uid_changed_notification(tBTA_AV_META_MSG *pmeta_msg, tAVRC_COMMAND *pavrc_command)
{
    tAVRC_RESPONSE avrc_rsp = {0};
    avrc_rsp.rsp.pdu = pavrc_command->pdu;
    avrc_rsp.rsp.status = AVRC_STS_NO_ERROR;
    avrc_rsp.rsp.opcode = pavrc_command->cmd.opcode;
    avrc_rsp.reg_notif.event_id = pavrc_command->reg_notif.event_id;
    avrc_rsp.reg_notif.param.uid_counter = 0;
    send_metamsg_rsp(pmeta_msg->rc_handle, pmeta_msg->label, AVRC_RSP_INTERIM, &avrc_rsp);
    send_metamsg_rsp(pmeta_msg->rc_handle, pmeta_msg->label, AVRC_RSP_CHANGED, &avrc_rsp);
}

/***************************************************************************
 *  Function       handle_rc_metamsg_cmd
 *
 *  - Argument:    tBTA_AV_VENDOR Structure containing the received
 *                          metamsg command
 *
 *  - Description: Remote control metamsg command handler (AVRCP 1.3)
 *
 ***************************************************************************/
void handle_rc_metamsg_cmd(tBTA_AV_META_MSG *pmeta_msg)
{
    /* Parse the metamsg command and pass it on to BTL-IFS */
    uint8_t             scratch_buf[512] = {0};
    tAVRC_COMMAND    avrc_command = {0};
    tAVRC_STS status;
    BTIF_TRACE_EVENT("+ %s", __FUNCTION__);

    if(pmeta_msg->p_msg->hdr.opcode != AVRC_OP_VENDOR) {
        BTIF_TRACE_WARNING("Invalid opcode: %x", pmeta_msg->p_msg->hdr.opcode);
        return;
    }

    if(pmeta_msg->len < 3) {
        BTIF_TRACE_WARNING("Invalid length.Opcode: 0x%x, len: 0x%x", pmeta_msg->p_msg->hdr.opcode,
                           pmeta_msg->len);
        return;
    }

    if(pmeta_msg->code >= AVRC_RSP_NOT_IMPL) {
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
        {
            rc_transaction_t *transaction = NULL;
            transaction = get_transaction_by_lbl(pmeta_msg->label);

            if(NULL != transaction)
            {
                handle_rc_metamsg_rsp(pmeta_msg);
            } else
            {
                BTIF_TRACE_DEBUG("%s:Discard vendor dependent rsp. code: %d label:%d.",
                                 __FUNCTION__, pmeta_msg->code, pmeta_msg->label);
            }

            return;
        }
#else
        {
            BTIF_TRACE_DEBUG("%s:Received vendor dependent rsp. code: %d len: %d. Not processing it.",
                             __FUNCTION__, pmeta_msg->code, pmeta_msg->len);
            return;
        }
#endif
    }

    status = AVRC_ParsCommand(pmeta_msg->p_msg, &avrc_command, scratch_buf, sizeof(scratch_buf));
    BTIF_TRACE_DEBUG("%s Received vendor command.code,PDU and label: %d, %d,%d",
                     __FUNCTION__, pmeta_msg->code, avrc_command.cmd.pdu, pmeta_msg->label);

    if(status != AVRC_STS_NO_ERROR) {
        /* return error */
        BTIF_TRACE_WARNING("%s: Error in parsing received metamsg command. status: 0x%02x",
                           __FUNCTION__, status);
        send_reject_response(pmeta_msg->rc_handle, pmeta_msg->label, avrc_command.pdu, status);
    } else {
        /* if RegisterNotification, add it to our registered queue */
        if(avrc_command.cmd.pdu == AVRC_PDU_REGISTER_NOTIFICATION) {
            uint8_t event_id = avrc_command.reg_notif.event_id;
            BTIF_TRACE_EVENT("%s:New register notification received.event_id:%s,label:0x%x,code:%x",
                             __FUNCTION__, dump_rc_notification_event_id(event_id), pmeta_msg->label, pmeta_msg->code);
            btif_rc_cb.rc_notif[event_id - 1].bNotify = TRUE;
            btif_rc_cb.rc_notif[event_id - 1].label = pmeta_msg->label;

            if(event_id == AVRC_EVT_UIDS_CHANGE) {
                handle_uid_changed_notification(pmeta_msg, &avrc_command);
                return;
            }
        }

        BTIF_TRACE_EVENT("%s: Passing received metamsg command to app. pdu: %s",
                         __FUNCTION__, dump_rc_pdu(avrc_command.cmd.pdu));
        /* Since handle_rc_metamsg_cmd() itself is called from
            *btif context, no context switching is required. Invoke
            * btif_rc_upstreams_evt directly from here. */
        btif_rc_upstreams_evt((uint16_t)avrc_command.cmd.pdu, &avrc_command, pmeta_msg->code,
                              pmeta_msg->label);
    }
}

/***************************************************************************
 **
 ** Function       btif_rc_handler
 **
 ** Description    RC event handler
 **
 ***************************************************************************/
void btif_rc_handler(tBTA_AV_EVT event, tBTA_AV *p_data)
{
    BTIF_TRACE_DEBUG("%s event:%s\r\n", __FUNCTION__, dump_rc_event(event));

    switch(event) {
        case BTA_AV_RC_OPEN_EVT: {
            BTIF_TRACE_DEBUG("%s Peer_features:%x", __FUNCTION__, p_data->rc_open.peer_features);
            handle_rc_connect(&(p_data->rc_open));
        }
        break;

        case BTA_AV_RC_CLOSE_EVT: {
            handle_rc_disconnect(&(p_data->rc_close));
        }
        break;

        case BTA_AV_REMOTE_CMD_EVT: {
            if(bt_rc_callbacks != NULL) {
                BTIF_TRACE_DEBUG("%s rc_id:0x%x key_state:%d",
                                 __FUNCTION__, p_data->remote_cmd.rc_id,
                                 p_data->remote_cmd.key_state);

                /** In race conditions just after 2nd AVRCP is connected
                 *  remote might send pass through commands, so check for
                 *  Rc handle before processing pass through commands
                 **/
                if(btif_rc_cb.rc_handle == p_data->remote_cmd.rc_handle) {
                    handle_rc_passthrough_cmd((&p_data->remote_cmd));
                } else {
                    BTIF_TRACE_DEBUG("%s Pass-through command for Invalid rc handle", __FUNCTION__);
                }
            } else {
                BTIF_TRACE_ERROR("AVRCP TG role not up, drop passthrough commands");
            }
        }
        break;
#if (AVRC_CTLR_INCLUDED == TRUE)

        case BTA_AV_REMOTE_RSP_EVT: {
            BTIF_TRACE_DEBUG("%s RSP: rc_id:0x%x key_state:%d",
                             __FUNCTION__, p_data->remote_rsp.rc_id, p_data->remote_rsp.key_state);

            if(p_data->remote_rsp.rc_id == AVRC_ID_VENDOR) {
                handle_rc_vendorunique_rsp(&p_data->remote_rsp);
            } else {
                handle_rc_passthrough_rsp(&p_data->remote_rsp);
            }
        }
        break;
#endif

        case BTA_AV_RC_FEAT_EVT: {
            BTIF_TRACE_DEBUG("%s Peer_features:%x", __FUNCTION__, p_data->rc_feat.peer_features);
            btif_rc_cb.rc_features = p_data->rc_feat.peer_features;
            handle_rc_features(p_data->rc_feat.peer_addr);
#if (AVRC_CTLR_INCLUDED == TRUE)

            if((btif_rc_cb.rc_connected) && (bt_rc_ctrl_callbacks != NULL)) {
                handle_rc_ctrl_features(btif_rc_cb.rc_addr);
            }

#endif
        }
        break;

        case BTA_AV_META_MSG_EVT: {
            if(bt_rc_callbacks != NULL) {
                BTIF_TRACE_DEBUG("%s BTA_AV_META_MSG_EVT  code:%d label:%d",
                                 __FUNCTION__,
                                 p_data->meta_msg.code,
                                 p_data->meta_msg.label);
                BTIF_TRACE_DEBUG("%s company_id:0x%x len:%d handle:%d",
                                 __FUNCTION__,
                                 p_data->meta_msg.company_id,
                                 p_data->meta_msg.len,
                                 p_data->meta_msg.rc_handle);
                /* handle the metamsg command */
                handle_rc_metamsg_cmd(&(p_data->meta_msg));
                /* Free the Memory allocated for tAVRC_MSG */
            }

#if (AVRC_CTLR_INCLUDED == TRUE)
            else if((bt_rc_callbacks == NULL) && (bt_rc_ctrl_callbacks != NULL)) {
                /* This is case of Sink + CT + TG(for abs vol)) */
                BTIF_TRACE_DEBUG("%s BTA_AV_META_MSG_EVT  code:%d label:%d",
                                 __FUNCTION__,
                                 p_data->meta_msg.code,
                                 p_data->meta_msg.label);
                BTIF_TRACE_DEBUG("%s company_id:0x%x len:%d handle:%d",
                                 __FUNCTION__,
                                 p_data->meta_msg.company_id,
                                 p_data->meta_msg.len,
                                 p_data->meta_msg.rc_handle);

                if((p_data->meta_msg.code >= AVRC_RSP_NOT_IMPL) &&
                        (p_data->meta_msg.code <= AVRC_RSP_INTERIM)) {
                    /* Its a response */
                    handle_avk_rc_metamsg_rsp(&(p_data->meta_msg));
                } else if(p_data->meta_msg.code <= AVRC_CMD_GEN_INQ) {
                    /* Its a command  */
                    handle_avk_rc_metamsg_cmd(&(p_data->meta_msg));
                }
            }

#endif
            else {
                BTIF_TRACE_ERROR("Neither CTRL, nor TG is up, drop meta commands");
            }
        }
        break;

        default:
            BTIF_TRACE_DEBUG("%s Unhandled RC event : 0x%x", __FUNCTION__, event);
    }
}

/***************************************************************************
 **
 ** Function       btif_rc_get_connected_peer
 **
 ** Description    Fetches the connected headset's BD_ADDR if any
 **
 ***************************************************************************/
uint8_t btif_rc_get_connected_peer(BD_ADDR peer_addr)
{
    if(btif_rc_cb.rc_connected == TRUE) {
        bdcpy(peer_addr, btif_rc_cb.rc_addr);
        return TRUE;
    }

    return FALSE;
}

/***************************************************************************
 **
 ** Function       btif_rc_get_connected_peer_handle
 **
 ** Description    Fetches the connected headset's handle if any
 **
 ***************************************************************************/
uint8_t btif_rc_get_connected_peer_handle(void)
{
    return btif_rc_cb.rc_handle;
}

/***************************************************************************
 **
 ** Function       btif_rc_check_handle_pending_play
 **
 ** Description    Clears the queued PLAY command. if bSend is TRUE, forwards to app
 **
 ***************************************************************************/

/* clear the queued PLAY command. if bSend is TRUE, forward to app */
void btif_rc_check_handle_pending_play(BD_ADDR peer_addr, uint8_t bSendToApp)
{
    UNUSED(peer_addr);
    BTIF_TRACE_DEBUG("%s: bSendToApp=%d", __FUNCTION__, bSendToApp);

    if(btif_rc_cb.rc_pending_play) {
        if(bSendToApp) {
            tBTA_AV_REMOTE_CMD remote_cmd;
            APPL_TRACE_DEBUG("%s: Sending queued PLAYED event to app", __FUNCTION__);
            wm_memset(&remote_cmd, 0, sizeof(tBTA_AV_REMOTE_CMD));
            remote_cmd.rc_handle  = btif_rc_cb.rc_handle;
            remote_cmd.rc_id      = AVRC_ID_PLAY;
            remote_cmd.hdr.ctype  = AVRC_CMD_CTRL;
            remote_cmd.hdr.opcode = AVRC_OP_PASS_THRU;
            /* delay sending to app, else there is a timing issue in the framework,
             ** which causes the audio to be on th device's speaker. Delay between
             ** OPEN & RC_PLAYs
            */
            GKI_delay(200);
            /* send to app - both PRESSED & RELEASED */
            remote_cmd.key_state  = AVRC_STATE_PRESS;
            handle_rc_passthrough_cmd(&remote_cmd);
            GKI_delay(100);
            remote_cmd.key_state  = AVRC_STATE_RELEASE;
            handle_rc_passthrough_cmd(&remote_cmd);
        }

        btif_rc_cb.rc_pending_play = FALSE;
    }
}

/* Generic reject response */
static void send_reject_response(uint8_t rc_handle, uint8_t label, uint8_t pdu, uint8_t status)
{
    uint8_t ctype = AVRC_RSP_REJ;
    tAVRC_RESPONSE avrc_rsp;
    BT_HDR *p_msg = NULL;
    wm_memset(&avrc_rsp, 0, sizeof(tAVRC_RESPONSE));
    avrc_rsp.rsp.opcode = opcode_from_pdu(pdu);
    avrc_rsp.rsp.pdu    = pdu;
    avrc_rsp.rsp.status = status;

    if(AVRC_STS_NO_ERROR == (status = AVRC_BldResponse(rc_handle, &avrc_rsp, &p_msg))) {
        BTIF_TRACE_DEBUG("%s:Sending error notification to handle:%d. pdu:%s,status:0x%02x",
                         __FUNCTION__, rc_handle, dump_rc_pdu(pdu), status);
        BTA_AvMetaRsp(rc_handle, label, ctype, p_msg);
    }
}

/***************************************************************************
 *  Function       send_metamsg_rsp
 *
 *  - Argument:
 *                  rc_handle     RC handle corresponding to the connected RC
 *                  label            Label of the RC response
 *                  code            Response type
 *                  pmetamsg_resp    Vendor response
 *
 *  - Description: Remote control metamsg response handler (AVRCP 1.3)
 *
 ***************************************************************************/
static void send_metamsg_rsp(uint8_t rc_handle, uint8_t label, tBTA_AV_CODE code,
                             tAVRC_RESPONSE *pmetamsg_resp)
{
    uint8_t ctype;

    if(!pmetamsg_resp) {
        BTIF_TRACE_WARNING("%s: Invalid response received from application", __FUNCTION__);
        return;
    }

    BTIF_TRACE_EVENT("+%s: rc_handle: %d, label: %d, code: 0x%02x, pdu: %s", __FUNCTION__,
                     rc_handle, label, code, dump_rc_pdu(pmetamsg_resp->rsp.pdu));

    if(pmetamsg_resp->rsp.status != AVRC_STS_NO_ERROR) {
        ctype = AVRC_RSP_REJ;
    } else {
        if(code < AVRC_RSP_NOT_IMPL) {
            if(code == AVRC_CMD_NOTIF) {
                ctype = AVRC_RSP_INTERIM;
            } else if(code == AVRC_CMD_STATUS) {
                ctype = AVRC_RSP_IMPL_STBL;
            } else {
                ctype = AVRC_RSP_ACCEPT;
            }
        } else {
            ctype = code;
        }
    }

    /* if response is for register_notification, make sure the rc has
    actually registered for this */
    if((pmetamsg_resp->rsp.pdu == AVRC_PDU_REGISTER_NOTIFICATION) && (code == AVRC_RSP_CHANGED)) {
        uint8_t bSent = FALSE;
        uint8_t   event_id = pmetamsg_resp->reg_notif.event_id;
        uint8_t bNotify = (btif_rc_cb.rc_connected) && (btif_rc_cb.rc_notif[event_id - 1].bNotify);
        /* de-register this notification for a CHANGED response */
        btif_rc_cb.rc_notif[event_id - 1].bNotify = FALSE;
        BTIF_TRACE_DEBUG("%s rc_handle: %d. event_id: 0x%02d bNotify:%u", __FUNCTION__,
                         btif_rc_cb.rc_handle, event_id, bNotify);

        if(bNotify) {
            BT_HDR *p_msg = NULL;
            tAVRC_STS status;

            if(AVRC_STS_NO_ERROR == (status = AVRC_BldResponse(btif_rc_cb.rc_handle,
                                              pmetamsg_resp, &p_msg))) {
                BTIF_TRACE_DEBUG("%s Sending notification to rc_handle: %d. event_id: 0x%02d",
                                 __FUNCTION__, btif_rc_cb.rc_handle, event_id);
                bSent = TRUE;
                BTA_AvMetaRsp(btif_rc_cb.rc_handle, btif_rc_cb.rc_notif[event_id - 1].label,
                              ctype, p_msg);
            } else {
                BTIF_TRACE_WARNING("%s failed to build metamsg response. status: 0x%02x",
                                   __FUNCTION__, status);
            }
        }

        if(!bSent) {
            BTIF_TRACE_DEBUG("%s: Notification not sent, as there are no RC connections or the \
                CT has not subscribed for event_id: %s", __FUNCTION__,
                             dump_rc_notification_event_id(event_id));
        }
    } else {
        /* All other commands go here */
        BT_HDR *p_msg = NULL;
        tAVRC_STS status;
        status = AVRC_BldResponse(rc_handle, pmetamsg_resp, &p_msg);

        if(status == AVRC_STS_NO_ERROR) {
            BTA_AvMetaRsp(rc_handle, label, ctype, p_msg);
        } else {
            BTIF_TRACE_ERROR("%s: failed to build metamsg response. status: 0x%02x",
                             __FUNCTION__, status);
        }
    }
}

static uint8_t opcode_from_pdu(uint8_t pdu)
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
** Function         btif_rc_upstreams_evt
**
** Description      Executes AVRC UPSTREAMS events in btif context.
**
** Returns          void
**
*******************************************************************************/
static void btif_rc_upstreams_evt(uint16_t event, tAVRC_COMMAND *pavrc_cmd, uint8_t ctype,
                                  uint8_t label)
{
    BTIF_TRACE_EVENT("%s pdu: %s handle: 0x%x ctype:%x label:%x", __FUNCTION__,
                     dump_rc_pdu(pavrc_cmd->pdu), btif_rc_cb.rc_handle, ctype, label);
    tls_btrc_msg_t msg;

    switch(event) {
        case AVRC_PDU_GET_PLAY_STATUS: {
            FILL_PDU_QUEUE(IDX_GET_PLAY_STATUS_RSP, ctype, label, TRUE)
            msg.get_play_status.reserved = NULL;
            bt_rc_callbacks(WM_BTRC_GET_PLAY_STATUS_EVT, &msg);
            //HAL_CBACK(bt_rc_callbacks, get_play_status_cb);
        }
        break;

        case AVRC_PDU_LIST_PLAYER_APP_ATTR:
        case AVRC_PDU_LIST_PLAYER_APP_VALUES:
        case AVRC_PDU_GET_CUR_PLAYER_APP_VALUE:
        case AVRC_PDU_SET_PLAYER_APP_VALUE:
        case AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT:
        case AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT: {
            /* TODO: Add support for Application Settings */
            send_reject_response(btif_rc_cb.rc_handle, label, pavrc_cmd->pdu, AVRC_STS_BAD_CMD);
        }
        break;

        case AVRC_PDU_GET_ELEMENT_ATTR: {
            btrc_media_attr_t element_attrs[BTRC_MAX_ELEM_ATTR_SIZE];
            uint8_t num_attr;
            wm_memset(&element_attrs, 0, sizeof(element_attrs));

            if(pavrc_cmd->get_elem_attrs.num_attr == 0) {
                /* CT requests for all attributes */
                int attr_cnt;
                num_attr = BTRC_MAX_ELEM_ATTR_SIZE;

                for(attr_cnt = 0; attr_cnt < BTRC_MAX_ELEM_ATTR_SIZE; attr_cnt++) {
                    element_attrs[attr_cnt] = attr_cnt + 1;
                }
            } else if(pavrc_cmd->get_elem_attrs.num_attr == 0xFF) {
                /* 0xff indicates, no attributes requested - reject */
                send_reject_response(btif_rc_cb.rc_handle, label, pavrc_cmd->pdu,
                                     AVRC_STS_BAD_PARAM);
                return;
            } else {
                int attr_cnt, filled_attr_count;
                num_attr = 0;

                /* Attribute IDs from 1 to AVRC_MAX_NUM_MEDIA_ATTR_ID are only valid,
                 * hence HAL definition limits the attributes to AVRC_MAX_NUM_MEDIA_ATTR_ID.
                 * Fill only valid entries.
                 */
                for(attr_cnt = 0; (attr_cnt < pavrc_cmd->get_elem_attrs.num_attr) &&
                        (num_attr < AVRC_MAX_NUM_MEDIA_ATTR_ID); attr_cnt++) {
                    if((pavrc_cmd->get_elem_attrs.attrs[attr_cnt] > 0) &&
                            (pavrc_cmd->get_elem_attrs.attrs[attr_cnt] <= AVRC_MAX_NUM_MEDIA_ATTR_ID)) {
                        /* Skip the duplicate entries : PTS sends duplicate entries for Fragment cases
                         */
                        for(filled_attr_count = 0; filled_attr_count < num_attr; filled_attr_count++) {
                            if(element_attrs[filled_attr_count] == pavrc_cmd->get_elem_attrs.attrs[attr_cnt]) {
                                break;
                            }
                        }

                        if(filled_attr_count == num_attr) {
                            element_attrs[num_attr] = pavrc_cmd->get_elem_attrs.attrs[attr_cnt];
                            num_attr++;
                        }
                    }
                }
            }

            FILL_PDU_QUEUE(IDX_GET_ELEMENT_ATTR_RSP, ctype, label, TRUE);
            msg.get_element_attr.num_attr = num_attr;
            msg.get_element_attr.p_attrs = (tls_btrc_media_attr_t *)element_attrs;
            bt_rc_callbacks(WM_BTRC_GET_ELEMENT_ATTR_EVT, &msg);
            //HAL_CBACK(bt_rc_callbacks, get_element_attr_cb, num_attr, element_attrs);
        }
        break;

        case AVRC_PDU_REGISTER_NOTIFICATION: {
            if(pavrc_cmd->reg_notif.event_id == BTRC_EVT_PLAY_POS_CHANGED &&
                    pavrc_cmd->reg_notif.param == 0) {
                BTIF_TRACE_WARNING("%s Device registering position changed with illegal param 0.",
                                   __FUNCTION__);
                send_reject_response(btif_rc_cb.rc_handle, label, pavrc_cmd->pdu, AVRC_STS_BAD_PARAM);
                /* de-register this notification for a rejected response */
                btif_rc_cb.rc_notif[BTRC_EVT_PLAY_POS_CHANGED - 1].bNotify = FALSE;
                return;
            }

            msg.register_notification.event_id = pavrc_cmd->reg_notif.event_id;
            msg.register_notification.param = pavrc_cmd->reg_notif.param;
            bt_rc_callbacks(WM_BTRC_REGISTER_NOTIFICATION_EVT, &msg);
            //HAL_CBACK(bt_rc_callbacks, register_notification_cb, pavrc_cmd->reg_notif.event_id,pavrc_cmd->reg_notif.param);
        }
        break;

        case AVRC_PDU_INFORM_DISPLAY_CHARSET: {
            tAVRC_RESPONSE avrc_rsp;
            BTIF_TRACE_EVENT("%s() AVRC_PDU_INFORM_DISPLAY_CHARSET", __FUNCTION__);

            if(btif_rc_cb.rc_connected == TRUE) {
                wm_memset(&(avrc_rsp.inform_charset), 0, sizeof(tAVRC_RSP));
                avrc_rsp.inform_charset.opcode = opcode_from_pdu(AVRC_PDU_INFORM_DISPLAY_CHARSET);
                avrc_rsp.inform_charset.pdu = AVRC_PDU_INFORM_DISPLAY_CHARSET;
                avrc_rsp.inform_charset.status = AVRC_STS_NO_ERROR;
                send_metamsg_rsp(btif_rc_cb.rc_handle, label, ctype, &avrc_rsp);
            }
        }
        break;

        default: {
            send_reject_response(btif_rc_cb.rc_handle, label, pavrc_cmd->pdu,
                                 (pavrc_cmd->pdu == AVRC_PDU_SEARCH) ? AVRC_STS_SEARCH_NOT_SUP : AVRC_STS_BAD_CMD);
            return;
        }
        break;
    }
}

#if (AVRC_CTLR_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btif_rc_ctrl_upstreams_rsp_cmd
**
** Description      Executes AVRC UPSTREAMS response events in btif context.
**
** Returns          void
**
*******************************************************************************/
static void btif_rc_ctrl_upstreams_rsp_cmd(uint8_t event, tAVRC_COMMAND *pavrc_cmd,
        uint8_t label)
{
    BTIF_TRACE_DEBUG("%s pdu: %s handle: 0x%x", __FUNCTION__,
                     dump_rc_pdu(pavrc_cmd->pdu), btif_rc_cb.rc_handle);
    tls_bt_addr_t rc_addr;
    tls_btrc_ctrl_msg_t msg;
    bdcpy(rc_addr.address, btif_rc_cb.rc_addr);
#if (AVRC_CTLR_INCLUDED == TRUE)

    switch(event) {
        case AVRC_PDU_SET_ABSOLUTE_VOLUME:
            msg.setabsvol.bd_addr = &rc_addr;
            msg.setabsvol.abs_vol = pavrc_cmd->volume.volume;
            msg.setabsvol.label = label;
            bt_rc_ctrl_callbacks(WM_BTRC_CTRL_SETABSVOL_CMD_EVT, &msg);
            //HAL_CBACK(bt_rc_ctrl_callbacks, setabsvol_cmd_cb, &rc_addr,pavrc_cmd->volume.volume, label);
            break;

        case AVRC_PDU_REGISTER_NOTIFICATION:
            if(pavrc_cmd->reg_notif.event_id == AVRC_EVT_VOLUME_CHANGE) {
                msg.registernotification_abs_vol.bd_addr = &rc_addr;
                msg.registernotification_abs_vol.label = label;
                bt_rc_ctrl_callbacks(WM_BTRC_CTRL_REGISTERNOTIFICATION_ABS_VOL_EVT, &msg);
                //HAL_CBACK(bt_rc_ctrl_callbacks, registernotification_absvol_cb,&rc_addr, label);
            }

            break;
    }

#endif
}
#endif

#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btif_rc_upstreams_rsp_evt
**
** Description      Executes AVRC UPSTREAMS response events in btif context.
**
** Returns          void
**
*******************************************************************************/
static void btif_rc_upstreams_rsp_evt(uint16_t event, tAVRC_RESPONSE *pavrc_resp, uint8_t ctype,
                                      uint8_t label)
{
    BTIF_TRACE_EVENT("%s pdu: %s handle: 0x%x ctype:%x label:%x", __FUNCTION__,
                     dump_rc_pdu(pavrc_resp->pdu), btif_rc_cb.rc_handle, ctype, label);
    tls_btrc_msg_t msg;

    switch(event) {
        case AVRC_PDU_REGISTER_NOTIFICATION: {
            if(AVRC_RSP_CHANGED == ctype) {
                btif_rc_cb.rc_volume = pavrc_resp->reg_notif.param.volume;
            }

            msg.volume_change.ctype = ctype;
            msg.volume_change.volume = pavrc_resp->reg_notif.param.volume;
            bt_rc_callbacks(WM_BTRC_VOLUME_CHANGED_EVT, &msg);
            //HAL_CBACK(bt_rc_callbacks, volume_change_cb, pavrc_resp->reg_notif.param.volume, ctype)
        }
        break;

        case AVRC_PDU_SET_ABSOLUTE_VOLUME: {
            BTIF_TRACE_DEBUG("%s Set absolute volume change event received: volume %d,ctype %d",
                             __FUNCTION__, pavrc_resp->volume.volume, ctype);

            if(AVRC_RSP_ACCEPT == ctype) {
                btif_rc_cb.rc_volume = pavrc_resp->volume.volume;
            }

            msg.volume_change.ctype = ctype;
            msg.volume_change.volume = pavrc_resp->volume.volume;
            bt_rc_callbacks(WM_BTRC_VOLUME_CHANGED_EVT, &msg);
            //HAL_CBACK(bt_rc_callbacks, volume_change_cb, pavrc_resp->volume.volume, ctype)
        }
        break;

        default:
            return;
    }
}
#endif

/************************************************************************************
**  AVRCP API Functions
************************************************************************************/



static void rc_ctrl_procedure_complete()
{
    if(btif_rc_cb.rc_procedure_complete == TRUE) {
        return;
    }

    btif_rc_cb.rc_procedure_complete = TRUE;
    uint32_t attr_list[] = {
        AVRC_MEDIA_ATTR_ID_TITLE,
        AVRC_MEDIA_ATTR_ID_ARTIST,
        AVRC_MEDIA_ATTR_ID_ALBUM,
        AVRC_MEDIA_ATTR_ID_TRACK_NUM,
        AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
        AVRC_MEDIA_ATTR_ID_GENRE,
        AVRC_MEDIA_ATTR_ID_PLAYING_TIME
    };
    get_element_attribute_cmd(AVRC_MAX_NUM_MEDIA_ATTR_ID, attr_list);
}


#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
/***************************************************************************
**
** Function         register_volumechange
**
** Description     Register for volume change notification from remote side.
**
** Returns          void
**
***************************************************************************/

static void register_volumechange(uint8_t lbl)
{
    tAVRC_COMMAND avrc_cmd = {0};
    BT_HDR *p_msg = NULL;
    tAVRC_STS BldResp = AVRC_STS_BAD_CMD;
    rc_transaction_t *p_transaction = NULL;
    BTIF_TRACE_DEBUG("%s called with label:%d", __FUNCTION__, lbl);
    avrc_cmd.cmd.opcode = 0x00;
    avrc_cmd.pdu = AVRC_PDU_REGISTER_NOTIFICATION;
    avrc_cmd.reg_notif.event_id = AVRC_EVT_VOLUME_CHANGE;
    avrc_cmd.reg_notif.status = AVRC_STS_NO_ERROR;
    avrc_cmd.reg_notif.param = 0;
    BldResp = AVRC_BldCommand(&avrc_cmd, &p_msg);

    if(AVRC_STS_NO_ERROR == BldResp && p_msg) {
        p_transaction = get_transaction_by_lbl(lbl);

        if(p_transaction != NULL) {
            BTA_AvMetaCmd(btif_rc_cb.rc_handle, p_transaction->lbl,
                          AVRC_CMD_NOTIF, p_msg);
            BTIF_TRACE_DEBUG("%s:BTA_AvMetaCmd called", __func__);
        } else {
            GKI_freebuf(p_msg);
            BTIF_TRACE_ERROR("%s transaction not obtained with label: %d",
                             __func__, lbl);
        }
    } else {
        BTIF_TRACE_ERROR("%s failed to build command:%d", __func__, BldResp);
    }
}

/***************************************************************************
**
** Function         handle_rc_metamsg_rsp
**
** Description      Handle RC metamessage response
**
** Returns          void
**
***************************************************************************/
static void handle_rc_metamsg_rsp(tBTA_AV_META_MSG *pmeta_msg)
{
    tAVRC_RESPONSE    avrc_response = {0};
    uint8_t             scratch_buf[512] = {0};
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;

    if(AVRC_OP_VENDOR == pmeta_msg->p_msg->hdr.opcode && (AVRC_RSP_CHANGED == pmeta_msg->code
            || AVRC_RSP_INTERIM == pmeta_msg->code || AVRC_RSP_ACCEPT == pmeta_msg->code
            || AVRC_RSP_REJ == pmeta_msg->code || AVRC_RSP_NOT_IMPL == pmeta_msg->code)) {
        status = AVRC_ParsResponse(pmeta_msg->p_msg, &avrc_response, scratch_buf, sizeof(scratch_buf));
        BTIF_TRACE_DEBUG("%s: code %d,event ID %d,PDU %x,parsing status %d, label:%d",
                         __FUNCTION__, pmeta_msg->code, avrc_response.reg_notif.event_id, avrc_response.reg_notif.pdu,
                         status, pmeta_msg->label);

        if(status != AVRC_STS_NO_ERROR) {
            if(AVRC_PDU_REGISTER_NOTIFICATION == avrc_response.rsp.pdu
                    && AVRC_EVT_VOLUME_CHANGE == avrc_response.reg_notif.event_id
                    && btif_rc_cb.rc_vol_label == pmeta_msg->label) {
                btif_rc_cb.rc_vol_label = MAX_LABEL;
                release_transaction(btif_rc_cb.rc_vol_label);
            } else if(AVRC_PDU_SET_ABSOLUTE_VOLUME == avrc_response.rsp.pdu) {
                release_transaction(pmeta_msg->label);
            }

            return;
        } else if(AVRC_PDU_REGISTER_NOTIFICATION == avrc_response.rsp.pdu
                  && AVRC_EVT_VOLUME_CHANGE == avrc_response.reg_notif.event_id
                  && btif_rc_cb.rc_vol_label != pmeta_msg->label) {
            // Just discard the message, if the device sends back with an incorrect label
            BTIF_TRACE_DEBUG("%s:Discarding register notfn in rsp.code: %d and label %d",
                             __FUNCTION__, pmeta_msg->code, pmeta_msg->label);
            return;
        }
    } else {
        BTIF_TRACE_DEBUG("%s:Received vendor dependent in adv ctrl rsp. code: %d len: %d. Not processing it.",
                         __FUNCTION__, pmeta_msg->code, pmeta_msg->len);
        return;
    }

    if(AVRC_PDU_REGISTER_NOTIFICATION == avrc_response.rsp.pdu
            && AVRC_EVT_VOLUME_CHANGE == avrc_response.reg_notif.event_id
            && AVRC_RSP_CHANGED == pmeta_msg->code) {
        /* re-register for volume change notification */
        // Do not re-register for rejected case, as it might get into endless loop
        register_volumechange(btif_rc_cb.rc_vol_label);
    } else if(AVRC_PDU_SET_ABSOLUTE_VOLUME == avrc_response.rsp.pdu) {
        /* free up the label here */
        release_transaction(pmeta_msg->label);
    }

    BTIF_TRACE_EVENT("%s: Passing received metamsg response to app. pdu: %s",
                     __FUNCTION__, dump_rc_pdu(avrc_response.pdu));
    btif_rc_upstreams_rsp_evt((uint16_t)avrc_response.rsp.pdu, &avrc_response, pmeta_msg->code,
                              pmeta_msg->label);
}
#endif

#if (AVRC_CTLR_INCLUDED == TRUE)
/***************************************************************************
**
** Function         iterate_supported_event_list_for_interim_rsp
**
** Description      iterator callback function to match the event and handle
**                  timer cleanup
** Returns          true to continue iterating, false to stop
**
***************************************************************************/
uint8_t iterate_supported_event_list_for_interim_rsp(void *data, void *cb_data)
{
    uint8_t *p_event_id;
    btif_rc_supported_event_t *p_event = (btif_rc_supported_event_t *)data;
    p_event_id = (uint8_t *)cb_data;

    if(p_event->event_id == *p_event_id) {
        p_event->status = eINTERIM;
        return false;
    }

    return true;
}

/***************************************************************************
**
** Function         iterate_supported_event_list_for_timeout
**
** Description      Iterator callback function for timeout handling.
**                  As part of the failure handling, it releases the
**                  transaction label and removes the event from list,
**                  this event will not be requested again during
**                  the lifetime of the connection.
** Returns          false to stop iterating, true to continue
**
***************************************************************************/
uint8_t iterate_supported_event_list_for_timeout(void *data, void *cb_data)
{
    uint8_t label;
    btif_rc_supported_event_t *p_event = (btif_rc_supported_event_t *)data;
    label = (*(uint8_t *)cb_data) & 0xFF;

    if(p_event->label == label) {
        list_remove(btif_rc_cb.rc_supported_event_list, p_event);
        return false;
    }

    return true;
}

/***************************************************************************
**
** Function         rc_notification_interim_timout
**
** Description      Interim response timeout handler.
**                  Runs the iterator to check and clear the timed out event.
**                  Proceeds to register for the unregistered events.
** Returns          None
**
***************************************************************************/
static void rc_notification_interim_timout(uint8_t label)
{
    list_node_t *node;
    list_foreach(btif_rc_cb.rc_supported_event_list,
                 iterate_supported_event_list_for_timeout, &label);
    /* Timeout happened for interim response for the registered event,
     * check if there are any pending for registration
     */
    node = list_begin(btif_rc_cb.rc_supported_event_list);

    while(node != NULL) {
        btif_rc_supported_event_t *p_event;
        p_event = (btif_rc_supported_event_t *)list_node(node);

        if((p_event != NULL) && (p_event->status == eNOT_REGISTERED)) {
            register_for_event_notification(p_event);
            break;
        }

        node = list_next(node);
    }

    /* Todo. Need to initiate application settings query if this
     * is the last event registration.
     */
}

/***************************************************************************
**
** Function         btif_rc_status_cmd_timeout_handler
**
** Description      RC status command timeout handler (Runs in BTIF context).
** Returns          None
**
***************************************************************************/
static void btif_rc_status_cmd_timeout_handler(uint16_t event,
        char *data)
{
    btif_rc_timer_context_t *p_context;
    tAVRC_RESPONSE      avrc_response = {0};
    tBTA_AV_META_MSG    meta_msg;
    p_context = (btif_rc_timer_context_t *)data;
    wm_memset(&meta_msg, 0, sizeof(tBTA_AV_META_MSG));
    meta_msg.rc_handle = btif_rc_cb.rc_handle;

    switch(p_context->rc_status_cmd.pdu_id) {
        case AVRC_PDU_REGISTER_NOTIFICATION:
            rc_notification_interim_timout(p_context->rc_status_cmd.label);
            break;

        case AVRC_PDU_GET_CAPABILITIES:
            avrc_response.get_caps.status = BTIF_RC_STS_TIMEOUT;
            handle_get_capability_response(&meta_msg, &avrc_response.get_caps);
            break;

        case AVRC_PDU_LIST_PLAYER_APP_ATTR:
            avrc_response.list_app_attr.status = BTIF_RC_STS_TIMEOUT;
            handle_app_attr_response(&meta_msg, &avrc_response.list_app_attr);
            break;

        case AVRC_PDU_LIST_PLAYER_APP_VALUES:
            avrc_response.list_app_values.status = BTIF_RC_STS_TIMEOUT;
            handle_app_val_response(&meta_msg, &avrc_response.list_app_values);
            break;

        case AVRC_PDU_GET_CUR_PLAYER_APP_VALUE:
            avrc_response.get_cur_app_val.status = BTIF_RC_STS_TIMEOUT;
            handle_app_cur_val_response(&meta_msg, &avrc_response.get_cur_app_val);
            break;

        case AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT:
            avrc_response.get_app_attr_txt.status = BTIF_RC_STS_TIMEOUT;
            handle_app_attr_txt_response(&meta_msg, &avrc_response.get_app_attr_txt);
            break;

        case AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT:
            avrc_response.get_app_val_txt.status = BTIF_RC_STS_TIMEOUT;
            handle_app_attr_txt_response(&meta_msg, &avrc_response.get_app_val_txt);
            break;

        case AVRC_PDU_GET_ELEMENT_ATTR:
            avrc_response.get_elem_attrs.status = BTIF_RC_STS_TIMEOUT;
            handle_get_elem_attr_response(&meta_msg, &avrc_response.get_elem_attrs);
            break;

        case AVRC_PDU_GET_PLAY_STATUS:
            avrc_response.get_play_status.status = BTIF_RC_STS_TIMEOUT;
            handle_get_playstatus_response(&meta_msg, &avrc_response.get_play_status);
            break;
    }

    release_transaction(p_context->rc_status_cmd.label);
}

/***************************************************************************
**
** Function         btif_rc_status_cmd_timer_timeout
**
** Description      RC status command timeout callback.
**                  This is called from BTU context and switches to BTIF
**                  context to handle the timeout events
** Returns          None
**
***************************************************************************/
static void btif_rc_status_cmd_timer_timeout(void *data)
{
    btif_rc_timer_context_t *p_data = (btif_rc_timer_context_t *)data;
    btif_transfer_context(btif_rc_status_cmd_timeout_handler, 0,
                          (char *)p_data, sizeof(btif_rc_timer_context_t),
                          NULL);
}

/***************************************************************************
**
** Function         btif_rc_control_cmd_timeout_handler
**
** Description      RC control command timeout handler (Runs in BTIF context).
** Returns          None
**
***************************************************************************/
static void btif_rc_control_cmd_timeout_handler(uint16_t event,
        char *data)
{
    btif_rc_timer_context_t *p_context = (btif_rc_timer_context_t *)data;
    tAVRC_RESPONSE      avrc_response = {0};
    tBTA_AV_META_MSG    meta_msg;
    wm_memset(&meta_msg, 0, sizeof(tBTA_AV_META_MSG));
    meta_msg.rc_handle = btif_rc_cb.rc_handle;

    switch(p_context->rc_control_cmd.pdu_id) {
        case AVRC_PDU_SET_PLAYER_APP_VALUE:
            avrc_response.set_app_val.status = BTIF_RC_STS_TIMEOUT;
            handle_set_app_attr_val_response(&meta_msg,
                                             &avrc_response.set_app_val);
            break;
    }

    release_transaction(p_context->rc_control_cmd.label);
}

/***************************************************************************
**
** Function         btif_rc_control_cmd_timer_timeout
**
** Description      RC control command timeout callback.
**                  This is called from BTU context and switches to BTIF
**                  context to handle the timeout events
** Returns          None
**
***************************************************************************/
static void btif_rc_control_cmd_timer_timeout(void *data)
{
    btif_rc_timer_context_t *p_data = (btif_rc_timer_context_t *)data;
    btif_transfer_context(btif_rc_control_cmd_timeout_handler, 0,
                          (char *)p_data, sizeof(btif_rc_timer_context_t),
                          NULL);
}

/***************************************************************************
**
** Function         btif_rc_play_status_timeout_handler
**
** Description      RC play status timeout handler (Runs in BTIF context).
** Returns          None
**
***************************************************************************/
static void btif_rc_play_status_timeout_handler(uint16_t event,
        char *p_data)
{
    get_play_status_cmd();
    rc_start_play_status_timer();
}

/***************************************************************************
**
** Function         btif_rc_play_status_timer_timeout
**
** Description      RC play status timeout callback.
**                  This is called from BTU context and switches to BTIF
**                  context to handle the timeout events
** Returns          None
**
***************************************************************************/
static void btif_rc_play_status_timer_timeout(void *data)
{
    btif_transfer_context(btif_rc_play_status_timeout_handler, 0, 0, 0, NULL);
}

/***************************************************************************
**
** Function         rc_start_play_status_timer
**
** Description      Helper function to start the timer to fetch play status.
** Returns          None
**
***************************************************************************/
static void rc_start_play_status_timer(void)
{
    /* Start the Play status timer only if it is not started */
#ifdef USE_ALARM
    if(!alarm_is_scheduled(btif_rc_cb.rc_play_status_timer)) {
        if(btif_rc_cb.rc_play_status_timer == NULL) {
            btif_rc_cb.rc_play_status_timer =
                            alarm_new("btif_rc.rc_play_status_timer");
        }

        alarm_set_on_queue(btif_rc_cb.rc_play_status_timer,
                           BTIF_TIMEOUT_RC_INTERIM_RSP_MS,
                           btif_rc_play_status_timer_timeout, NULL,
                           btu_general_alarm_queue);
    }

#else

    if(!btif_rc_cb.rc_play_status_timer.in_use) {
        btif_rc_cb.rc_play_status_timer.param = (TIMER_PARAM_TYPE)NULL;
        btif_rc_cb.rc_play_status_timer.p_cback = (TIMER_CBACK *)&btif_rc_play_status_timer_timeout;
        btu_start_timer_oneshot(&btif_rc_cb.rc_play_status_timer, BTU_TTYPE_USER_FUNC,
                                BTIF_TIMEOUT_RC_INTERIM_RSP_MS / 1000);
    }

#endif
}

/***************************************************************************
**
** Function         rc_stop_play_status_timer
**
** Description      Helper function to stop the play status timer.
** Returns          None
**
***************************************************************************/
void rc_stop_play_status_timer()
{
#ifdef USE_ALARM

    if(btif_rc_cb.rc_play_status_timer != NULL) {
        alarm_cancel(btif_rc_cb.rc_play_status_timer);
    }

#else
    btu_stop_timer_oneshot(&btif_rc_cb.rc_play_status_timer);
#endif
}

/***************************************************************************
**
** Function         register_for_event_notification
**
** Description      Helper function registering notification events
**                  sets an interim response timeout to handle if the remote
**                  does not respond.
** Returns          None
**
***************************************************************************/
static void register_for_event_notification(btif_rc_supported_event_t *p_event)
{
    tls_bt_status_t status;
    rc_transaction_t *p_transaction;
    status = get_transaction(&p_transaction);

    if(status == TLS_BT_STATUS_SUCCESS) {
        btif_rc_timer_context_t *p_context = &p_transaction->txn_timer_context;
        status = register_notification_cmd(p_transaction->lbl, p_event->event_id, 0);

        if(status != TLS_BT_STATUS_SUCCESS) {
            BTIF_TRACE_ERROR("%s Error in Notification registration %d",
                             __FUNCTION__, status);
            release_transaction(p_transaction->lbl);
            return;
        }

        p_event->label = p_transaction->lbl;
        p_event->status = eREGISTERED;
        p_context->rc_status_cmd.label = p_transaction->lbl;
        p_context->rc_status_cmd.pdu_id = AVRC_PDU_REGISTER_NOTIFICATION;
#ifdef USE_ALARM
        alarm_free(p_transaction->txn_timer);
        p_transaction->txn_timer =
                        alarm_new("btif_rc.status_command_txn_timer");
        alarm_set_on_queue(p_transaction->txn_timer,
                           BTIF_TIMEOUT_RC_INTERIM_RSP_MS,
                           btif_rc_status_cmd_timer_timeout, p_context,
                           btu_general_alarm_queue);
#else
        p_transaction->txn_timer.param = (TIMER_PARAM_TYPE)p_context;
        p_transaction->txn_timer.p_cback = (TIMER_CBACK *)&btif_rc_status_cmd_timer_timeout;
        btu_start_timer_oneshot(&p_transaction->txn_timer, BTU_TTYPE_USER_FUNC,
                                BTIF_TIMEOUT_RC_INTERIM_RSP_MS / 1000);
#endif
    } else {
        BTIF_TRACE_ERROR("%s Error No more Transaction label %d",
                         __FUNCTION__, status);
    }
}

static void start_status_command_timer(uint8_t pdu_id, rc_transaction_t *p_txn)
{
    btif_rc_timer_context_t *p_context = &p_txn->txn_timer_context;
    p_context->rc_status_cmd.label = p_txn->lbl;
    p_context->rc_status_cmd.pdu_id = pdu_id;
#ifdef USE_ALARM
    alarm_free(p_txn->txn_timer);
    p_txn->txn_timer = alarm_new("btif_rc.status_command_txn_timer");
    alarm_set_on_queue(p_txn->txn_timer, BTIF_TIMEOUT_RC_STATUS_CMD_MS,
                       btif_rc_status_cmd_timer_timeout, p_context,
                       btu_general_alarm_queue);
#else
    p_txn->txn_timer.param = (TIMER_PARAM_TYPE)p_context;
    p_txn->txn_timer.p_cback = (TIMER_CBACK *)&btif_rc_status_cmd_timer_timeout;
    btu_start_timer_oneshot(&p_txn->txn_timer, BTU_TTYPE_USER_FUNC,
                            BTIF_TIMEOUT_RC_STATUS_CMD_MS / 1000);
#endif
}

static void start_control_command_timer(uint8_t pdu_id, rc_transaction_t *p_txn)
{
    btif_rc_timer_context_t *p_context = &p_txn->txn_timer_context;
    p_context->rc_control_cmd.label = p_txn->lbl;
    p_context->rc_control_cmd.pdu_id = pdu_id;
#ifdef USE_ALARM
    alarm_free(p_txn->txn_timer);
    p_txn->txn_timer = alarm_new("btif_rc.control_command_txn_timer");
    alarm_set_on_queue(p_txn->txn_timer,
                       BTIF_TIMEOUT_RC_CONTROL_CMD_MS,
                       btif_rc_control_cmd_timer_timeout, p_context,
                       btu_general_alarm_queue);
#else
    p_txn->txn_timer.param = (TIMER_PARAM_TYPE)p_context;
    p_txn->txn_timer.p_cback = (TIMER_CBACK *)&btif_rc_control_cmd_timer_timeout;
    btu_start_timer_oneshot(&p_txn->txn_timer, BTU_TTYPE_USER_FUNC,
                            BTIF_TIMEOUT_RC_CONTROL_CMD_MS / 1000);
#endif
}

/***************************************************************************
**
** Function         handle_get_capability_response
**
** Description      Handles the get_cap_response to populate company id info
**                  and query the supported events.
**                  Initiates Notification registration for events supported
** Returns          None
**
***************************************************************************/
static void handle_get_capability_response(tBTA_AV_META_MSG *pmeta_msg, tAVRC_GET_CAPS_RSP *p_rsp)
{
    int xx = 0;

    /* Todo: Do we need to retry on command timeout */
    if(p_rsp->status != AVRC_STS_NO_ERROR) {
        BTIF_TRACE_ERROR("%s Error capability response 0x%02X",
                         __FUNCTION__, p_rsp->status);
        return;
    }

    if(p_rsp->capability_id == AVRC_CAP_EVENTS_SUPPORTED) {
        btif_rc_supported_event_t *p_event;
        /* Todo: Check if list can be active when we hit here */
        btif_rc_cb.rc_supported_event_list = list_new(GKI_freebuf);

        for(xx = 0; xx < p_rsp->count; xx++) {
            /* Skip registering for Play position change notification */
            if((p_rsp->param.event_id[xx] == AVRC_EVT_PLAY_STATUS_CHANGE) ||
                    (p_rsp->param.event_id[xx] == AVRC_EVT_TRACK_CHANGE) ||
                    (p_rsp->param.event_id[xx] == AVRC_EVT_APP_SETTING_CHANGE)) {
                p_event = (btif_rc_supported_event_t *)GKI_getbuf(sizeof(btif_rc_supported_event_t));
                p_event->event_id = p_rsp->param.event_id[xx];
                p_event->status = eNOT_REGISTERED;
                list_append(btif_rc_cb.rc_supported_event_list, p_event);
            }
        }

        p_event = list_front(btif_rc_cb.rc_supported_event_list);

        if(p_event != NULL) {
            register_for_event_notification(p_event);
        }
    } else if(p_rsp->capability_id == AVRC_CAP_COMPANY_ID) {
        getcapabilities_cmd(AVRC_CAP_EVENTS_SUPPORTED);
        BTIF_TRACE_EVENT("%s AVRC_CAP_COMPANY_ID: ", __FUNCTION__);

        for(xx = 0; xx < p_rsp->count; xx++) {
            BTIF_TRACE_EVENT("%s    : %d", __FUNCTION__, p_rsp->param.company_id[xx]);
        }
    }
}

uint8_t rc_is_track_id_valid(tAVRC_UID uid)
{
    tAVRC_UID invalid_uid = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    if(memcmp(uid, invalid_uid, sizeof(tAVRC_UID)) == 0) {
        return false;
    } else {
        return true;
    }
}

/***************************************************************************
**
** Function         handle_notification_response
**
** Description      Main handler for notification responses to registered events
**                  1. Register for unregistered event(in interim response path)
**                  2. After registering for all supported events, start
**                     retrieving application settings and values
**                  3. Reregister for events on getting changed response
**                  4. Run play status timer for getting position when the
**                     status changes to playing
**                  5. Get the Media details when the track change happens
**                     or track change interim response is received with
**                     valid track id
**                  6. HAL callback for play status change and application
**                     setting change
** Returns          None
**
***************************************************************************/
static void handle_notification_response(tBTA_AV_META_MSG *pmeta_msg, tAVRC_REG_NOTIF_RSP *p_rsp)
{
    tls_bt_addr_t rc_addr;
    tls_btrc_ctrl_msg_t msg;
    uint32_t attr_list[] = {
        AVRC_MEDIA_ATTR_ID_TITLE,
        AVRC_MEDIA_ATTR_ID_ARTIST,
        AVRC_MEDIA_ATTR_ID_ALBUM,
        AVRC_MEDIA_ATTR_ID_TRACK_NUM,
        AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
        AVRC_MEDIA_ATTR_ID_GENRE,
        AVRC_MEDIA_ATTR_ID_PLAYING_TIME
    };
    bdcpy(rc_addr.address, btif_rc_cb.rc_addr);

    if(pmeta_msg->code == AVRC_RSP_INTERIM) {
        btif_rc_supported_event_t *p_event;
        list_node_t *node;
        BTIF_TRACE_DEBUG("%s Interim response : 0x%2X ", __FUNCTION__, p_rsp->event_id);

        switch(p_rsp->event_id) {
            case AVRC_EVT_PLAY_STATUS_CHANGE:

                /* Start timer to get play status periodically
                 * if the play state is playing.
                 */
                if(p_rsp->param.play_status == AVRC_PLAYSTATE_PLAYING) {
                    rc_start_play_status_timer();
                }

                msg.play_status_changed.bd_addr = &rc_addr;
                msg.play_status_changed.play_status = p_rsp->param.play_status;
                bt_rc_ctrl_callbacks(WM_BTRC_CTRL_PLAY_STATUS_CHANGED_EVT, &msg);
                //HAL_CBACK(bt_rc_ctrl_callbacks, play_status_changed_cb,&rc_addr, p_rsp->param.play_status);
                break;

            case AVRC_EVT_TRACK_CHANGE:
                if(rc_is_track_id_valid(p_rsp->param.track) != true) {
                    break;
                } else {
                    uint8_t *p_data = p_rsp->param.track;
                    /* Update the UID for current track
                     * Attributes will be fetched after the AVRCP procedure
                     */
                    BE_STREAM_TO_UINT64(btif_rc_cb.rc_playing_uid, p_data);
                }

                break;

            case AVRC_EVT_APP_SETTING_CHANGE:
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
                BTIF_TRACE_ERROR("%s  Unhandled interim response 0x%2X", __FUNCTION__,
                                 p_rsp->event_id);
                return;
        }

        list_foreach(btif_rc_cb.rc_supported_event_list,
                     iterate_supported_event_list_for_interim_rsp,
                     &p_rsp->event_id);
        node = list_begin(btif_rc_cb.rc_supported_event_list);

        while(node != NULL) {
            p_event = (btif_rc_supported_event_t *)list_node(node);

            if((p_event != NULL) && (p_event->status == eNOT_REGISTERED)) {
                register_for_event_notification(p_event);
                break;
            }

            node = list_next(node);
            p_event = NULL;
        }

        /* Registered for all events, we can request application settings */
        if((p_event == NULL) && (btif_rc_cb.rc_app_settings.query_started == false)) {
            /* we need to do this only if remote TG supports
             * player application settings
             */
            btif_rc_cb.rc_app_settings.query_started = TRUE;

            if(btif_rc_cb.rc_features & BTA_AV_FEAT_APP_SETTING) {
                list_player_app_setting_attrib_cmd();
            } else {
                BTIF_TRACE_DEBUG("%s App setting not supported, complete procedure", __FUNCTION__);
                rc_ctrl_procedure_complete();
            }
        }
    } else if(pmeta_msg->code == AVRC_RSP_CHANGED) {
        btif_rc_supported_event_t *p_event;
        list_node_t *node;
        BTIF_TRACE_DEBUG("%s Notification completed : 0x%2X ", __FUNCTION__,
                         p_rsp->event_id);
        node = list_begin(btif_rc_cb.rc_supported_event_list);

        while(node != NULL) {
            p_event = (btif_rc_supported_event_t *)list_node(node);

            if((p_event != NULL) && (p_event->event_id == p_rsp->event_id)) {
                p_event->status = eNOT_REGISTERED;
                register_for_event_notification(p_event);
                break;
            }

            node = list_next(node);
        }

        switch(p_rsp->event_id) {
            case AVRC_EVT_PLAY_STATUS_CHANGE:

                /* Start timer to get play status periodically
                 * if the play state is playing.
                 */
                if(p_rsp->param.play_status == AVRC_PLAYSTATE_PLAYING) {
                    rc_start_play_status_timer();
                } else {
                    rc_stop_play_status_timer();
                }

                msg.play_status_changed.bd_addr = &rc_addr;
                msg.play_status_changed.play_status = p_rsp->param.play_status;
                bt_rc_ctrl_callbacks(WM_BTRC_CTRL_PLAY_STATUS_CHANGED_EVT, &msg);
                // HAL_CBACK(bt_rc_ctrl_callbacks, play_status_changed_cb,&rc_addr, p_rsp->param.play_status);
                break;

            case AVRC_EVT_TRACK_CHANGE:
                if(rc_is_track_id_valid(p_rsp->param.track) != true) {
                    break;
                }

                get_element_attribute_cmd(AVRC_MAX_NUM_MEDIA_ATTR_ID, attr_list);
                break;

            case AVRC_EVT_APP_SETTING_CHANGE: {
                btrc_player_settings_t app_settings;
                uint16_t xx;
                app_settings.num_attr = p_rsp->param.player_setting.num_attr;

                for(xx = 0; xx < app_settings.num_attr; xx++) {
                    app_settings.attr_ids[xx] = p_rsp->param.player_setting.attr_id[xx];
                    app_settings.attr_values[xx] = p_rsp->param.player_setting.attr_value[xx];
                }

                msg.playerapplicationsetting_changed.bd_addr = &rc_addr;
                msg.playerapplicationsetting_changed.p_vals = (tls_btrc_player_settings_t *)&app_settings;
                bt_rc_ctrl_callbacks(WM_BTRC_CTRL_PLAYERAPPLICATIONSETTING_CHANGED_EVT, &msg);
                //HAL_CBACK(bt_rc_ctrl_callbacks, playerapplicationsetting_changed_cb, &rc_addr, &app_settings);
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
                BTIF_TRACE_ERROR("%s  Unhandled completion response 0x%2X",
                                 __FUNCTION__, p_rsp->event_id);
                return;
        }
    }
}

/***************************************************************************
**
** Function         handle_app_attr_response
**
** Description      handles the the application attributes response and
**                  initiates procedure to fetch the attribute values
** Returns          None
**
***************************************************************************/
static void handle_app_attr_response(tBTA_AV_META_MSG *pmeta_msg, tAVRC_LIST_APP_ATTR_RSP *p_rsp)
{
    uint8_t xx;

    if(p_rsp->status != AVRC_STS_NO_ERROR) {
        BTIF_TRACE_ERROR("%s Error getting Player application settings: 0x%2X",
                         __FUNCTION__, p_rsp->status);
        rc_ctrl_procedure_complete();
        return;
    }

    for(xx = 0; xx < p_rsp->num_attr; xx++) {
        uint8_t st_index;

        if(p_rsp->attrs[xx] > AVRC_PLAYER_SETTING_LOW_MENU_EXT) {
            st_index = btif_rc_cb.rc_app_settings.num_ext_attrs;
            btif_rc_cb.rc_app_settings.ext_attrs[st_index].attr_id = p_rsp->attrs[xx];
            btif_rc_cb.rc_app_settings.num_ext_attrs++;
        } else {
            st_index = btif_rc_cb.rc_app_settings.num_attrs;
            btif_rc_cb.rc_app_settings.attrs[st_index].attr_id = p_rsp->attrs[xx];
            btif_rc_cb.rc_app_settings.num_attrs++;
        }
    }

    btif_rc_cb.rc_app_settings.attr_index = 0;
    btif_rc_cb.rc_app_settings.ext_attr_index = 0;
    btif_rc_cb.rc_app_settings.ext_val_index = 0;

    if(p_rsp->num_attr) {
        list_player_app_setting_value_cmd(btif_rc_cb.rc_app_settings.attrs[0].attr_id);
    } else {
        BTIF_TRACE_ERROR("%s No Player application settings found",
                         __FUNCTION__);
    }
}

/***************************************************************************
**
** Function         handle_app_val_response
**
** Description      handles the the attributes value response and if extended
**                  menu is available, it initiates query for the attribute
**                  text. If not, it initiates procedure to get the current
**                  attribute values and calls the HAL callback for provding
**                  application settings information.
** Returns          None
**
***************************************************************************/
static void handle_app_val_response(tBTA_AV_META_MSG *pmeta_msg, tAVRC_LIST_APP_VALUES_RSP *p_rsp)
{
    uint8_t xx, attr_index;
    uint8_t attrs[AVRC_MAX_APP_ATTR_SIZE];
    btif_rc_player_app_settings_t *p_app_settings;
    tls_bt_addr_t rc_addr;
    tls_btrc_ctrl_msg_t msg;

    /* Todo: Do we need to retry on command timeout */
    if(p_rsp->status != AVRC_STS_NO_ERROR) {
        BTIF_TRACE_ERROR("%s Error fetching attribute values 0x%02X",
                         __FUNCTION__, p_rsp->status);
        return;
    }

    p_app_settings = &btif_rc_cb.rc_app_settings;
    bdcpy(rc_addr.address, btif_rc_cb.rc_addr);

    if(p_app_settings->attr_index < p_app_settings->num_attrs) {
        attr_index = p_app_settings->attr_index;
        p_app_settings->attrs[attr_index].num_val = p_rsp->num_val;

        for(xx = 0; xx < p_rsp->num_val; xx++) {
            p_app_settings->attrs[attr_index].attr_val[xx] = p_rsp->vals[xx];
        }

        attr_index++;
        p_app_settings->attr_index++;

        if(attr_index < p_app_settings->num_attrs) {
            list_player_app_setting_value_cmd(p_app_settings->attrs[p_app_settings->attr_index].attr_id);
        } else if(p_app_settings->ext_attr_index < p_app_settings->num_ext_attrs) {
            attr_index = 0;
            p_app_settings->ext_attr_index = 0;
            list_player_app_setting_value_cmd(p_app_settings->ext_attrs[attr_index].attr_id);
        } else {
            for(xx = 0; xx < p_app_settings->num_attrs; xx++) {
                attrs[xx] = p_app_settings->attrs[xx].attr_id;
            }

            get_player_app_setting_cmd(p_app_settings->num_attrs, attrs);
            msg.playerapplicationsetting.bd_addr = &rc_addr;
            msg.playerapplicationsetting.num_attr = p_app_settings->num_attrs;
            msg.playerapplicationsetting.app_attrs = (tls_btrc_player_app_attr_t *)p_app_settings->attrs;
            msg.playerapplicationsetting.num_ext_attr = 0;
            msg.playerapplicationsetting.ext_attrs = NULL;

            if(bt_rc_ctrl_callbacks) { bt_rc_ctrl_callbacks(WM_BTRC_CTRL_PLAYERAPPLICATIONSETTING_EVT, &msg); }

            // HAL_CBACK(bt_rc_ctrl_callbacks, playerapplicationsetting_cb, &rc_addr, p_app_settings->num_attrs, p_app_settings->attrs, 0, NULL);
        }
    } else if(p_app_settings->ext_attr_index < p_app_settings->num_ext_attrs) {
        attr_index = p_app_settings->ext_attr_index;
        p_app_settings->ext_attrs[attr_index].num_val = p_rsp->num_val;

        for(xx = 0; xx < p_rsp->num_val; xx++) {
            p_app_settings->ext_attrs[attr_index].ext_attr_val[xx].val = p_rsp->vals[xx];
        }

        attr_index++;
        p_app_settings->ext_attr_index++;

        if(attr_index < p_app_settings->num_ext_attrs) {
            list_player_app_setting_value_cmd(
                            p_app_settings->ext_attrs[p_app_settings->ext_attr_index].attr_id);
        } else {
            uint8_t attr[AVRC_MAX_APP_ATTR_SIZE];
            uint8_t xx;

            for(xx = 0; xx < p_app_settings->num_ext_attrs; xx++) {
                attr[xx] = p_app_settings->ext_attrs[xx].attr_id;
            }

            get_player_app_setting_attr_text_cmd(attr, xx);
        }
    }
}

/***************************************************************************
**
** Function         handle_app_cur_val_response
**
** Description      handles the the get attributes value response.
**
** Returns          None
**
***************************************************************************/
static void handle_app_cur_val_response(tBTA_AV_META_MSG *pmeta_msg,
                                        tAVRC_GET_CUR_APP_VALUE_RSP *p_rsp)
{
    btrc_player_settings_t app_settings;
    tls_bt_addr_t rc_addr;
    uint16_t xx;
    tls_btrc_ctrl_msg_t msg;

    /* Todo: Do we need to retry on command timeout */
    if(p_rsp->status != AVRC_STS_NO_ERROR) {
        BTIF_TRACE_ERROR("%s Error fetching current settings: 0x%02X",
                         __FUNCTION__, p_rsp->status);
        return;
    }

    bdcpy(rc_addr.address, btif_rc_cb.rc_addr);
    app_settings.num_attr = p_rsp->num_val;

    for(xx = 0; xx < app_settings.num_attr; xx++) {
        app_settings.attr_ids[xx] = p_rsp->p_vals[xx].attr_id;
        app_settings.attr_values[xx] = p_rsp->p_vals[xx].attr_val;
    }

    msg.playerapplicationsetting_changed.bd_addr = &rc_addr;
    msg.playerapplicationsetting_changed.p_vals = (tls_btrc_player_settings_t *)&app_settings;

    if(bt_rc_ctrl_callbacks) { bt_rc_ctrl_callbacks(WM_BTRC_CTRL_PLAYERAPPLICATIONSETTING_CHANGED_EVT, &msg); }

    //HAL_CBACK(bt_rc_ctrl_callbacks, playerapplicationsetting_changed_cb,&rc_addr, &app_settings);
    /* Application settings are fetched only once for initial values
     * initiate anything that follows after RC procedure.
     * Defer it if browsing is supported till players query
     */
    rc_ctrl_procedure_complete();
    GKI_free_and_reset_buf((void **)&p_rsp->p_vals);
}

/***************************************************************************
**
** Function         handle_app_attr_txt_response
**
** Description      handles the the get attributes text response, if fails
**                  calls HAL callback with just normal settings and initiates
**                  query for current settings else initiates query for value text
** Returns          None
**
***************************************************************************/
static void handle_app_attr_txt_response(tBTA_AV_META_MSG *pmeta_msg,
        tAVRC_GET_APP_ATTR_TXT_RSP *p_rsp)
{
    uint8_t xx;
    uint8_t vals[AVRC_MAX_APP_ATTR_SIZE];
    btif_rc_player_app_settings_t *p_app_settings;
    tls_bt_addr_t rc_addr;
    tls_btrc_ctrl_msg_t msg;
    p_app_settings = &btif_rc_cb.rc_app_settings;
    bdcpy(rc_addr.address, btif_rc_cb.rc_addr);

    /* Todo: Do we need to retry on command timeout */
    if(p_rsp->status != AVRC_STS_NO_ERROR) {
        uint8_t attrs[AVRC_MAX_APP_ATTR_SIZE];
        BTIF_TRACE_ERROR("%s Error fetching attribute text: 0x%02X",
                         __FUNCTION__, p_rsp->status);
        /* Not able to fetch Text for extended Menu, skip the process
         * and cleanup used memory. Proceed to get the current settings
         * for standard attributes.
         */
        p_app_settings->num_ext_attrs = 0;

        for(xx = 0; xx < p_app_settings->ext_attr_index; xx++) {
            GKI_free_and_reset_buf((void **)&p_app_settings->ext_attrs[xx].p_str);
        }

        p_app_settings->ext_attr_index = 0;

        for(xx = 0; xx < p_app_settings->num_attrs; xx++) {
            attrs[xx] = p_app_settings->attrs[xx].attr_id;
        }

        msg.playerapplicationsetting.bd_addr = &rc_addr;
        msg.playerapplicationsetting.app_attrs = (tls_btrc_player_app_attr_t *)&p_app_settings->attrs[0];
        msg.playerapplicationsetting.num_attr = p_app_settings->num_attrs;
        msg.playerapplicationsetting.num_ext_attr = 0;
        msg.playerapplicationsetting.ext_attrs = NULL;

        if(bt_rc_ctrl_callbacks) { bt_rc_ctrl_callbacks(WM_BTRC_CTRL_PLAYERAPPLICATIONSETTING_EVT, &msg); }

        //HAL_CBACK(bt_rc_ctrl_callbacks, playerapplicationsetting_cb, &rc_addr, p_app_settings->num_attrs, p_app_settings->attrs, 0, NULL);
        get_player_app_setting_cmd(xx, attrs);
        return;
    }

    for(xx = 0; xx < p_rsp->num_attr; xx++) {
        uint8_t x;

        for(x = 0; x < p_app_settings->num_ext_attrs; x++) {
            if(p_app_settings->ext_attrs[x].attr_id == p_rsp->p_attrs[xx].attr_id) {
                p_app_settings->ext_attrs[x].charset_id = p_rsp->p_attrs[xx].charset_id;
                p_app_settings->ext_attrs[x].str_len = p_rsp->p_attrs[xx].str_len;
                p_app_settings->ext_attrs[x].p_str = p_rsp->p_attrs[xx].p_str;
                break;
            }
        }
    }

    for(xx = 0; xx < p_app_settings->ext_attrs[0].num_val; xx++) {
        vals[xx] = p_app_settings->ext_attrs[0].ext_attr_val[xx].val;
    }

    get_player_app_setting_value_text_cmd(vals, xx);
}


/***************************************************************************
**
** Function         handle_app_attr_val_txt_response
**
** Description      handles the the get attributes value text response, if fails
**                  calls HAL callback with just normal settings and initiates
**                  query for current settings
** Returns          None
**
***************************************************************************/
static void handle_app_attr_val_txt_response(tBTA_AV_META_MSG *pmeta_msg,
        tAVRC_GET_APP_ATTR_TXT_RSP *p_rsp)
{
    uint8_t xx, attr_index;
    uint8_t vals[AVRC_MAX_APP_ATTR_SIZE];
    uint8_t attrs[AVRC_MAX_APP_ATTR_SIZE];
    btif_rc_player_app_settings_t *p_app_settings;
    tls_bt_addr_t rc_addr;
    tls_btrc_ctrl_msg_t msg;
    bdcpy(rc_addr.address, btif_rc_cb.rc_addr);
    p_app_settings = &btif_rc_cb.rc_app_settings;

    /* Todo: Do we need to retry on command timeout */
    if(p_rsp->status != AVRC_STS_NO_ERROR) {
        uint8_t attrs[AVRC_MAX_APP_ATTR_SIZE];
        BTIF_TRACE_ERROR("%s Error fetching attribute value text: 0x%02X",
                         __FUNCTION__, p_rsp->status);
        /* Not able to fetch Text for extended Menu, skip the process
         * and cleanup used memory. Proceed to get the current settings
         * for standard attributes.
         */
        p_app_settings->num_ext_attrs = 0;

        for(xx = 0; xx < p_app_settings->ext_attr_index; xx++) {
            int x;
            btrc_player_app_ext_attr_t *p_ext_attr = &p_app_settings->ext_attrs[xx];

            for(x = 0; x < p_ext_attr->num_val; x++) {
                GKI_free_and_reset_buf((void **)&p_ext_attr->ext_attr_val[x].p_str);
            }

            p_ext_attr->num_val = 0;
            GKI_free_and_reset_buf((void **)&p_app_settings->ext_attrs[xx].p_str);
        }

        p_app_settings->ext_attr_index = 0;

        for(xx = 0; xx < p_app_settings->num_attrs; xx++) {
            attrs[xx] = p_app_settings->attrs[xx].attr_id;
        }

        msg.playerapplicationsetting.bd_addr = &rc_addr;
        msg.playerapplicationsetting.app_attrs = (tls_btrc_player_app_attr_t *)&p_app_settings->attrs[0];
        msg.playerapplicationsetting.num_attr = p_app_settings->num_attrs;
        msg.playerapplicationsetting.num_ext_attr = 0;
        msg.playerapplicationsetting.ext_attrs = NULL;

        if(bt_rc_ctrl_callbacks) { bt_rc_ctrl_callbacks(WM_BTRC_CTRL_PLAYERAPPLICATIONSETTING_EVT, &msg); }

        //HAL_CBACK(bt_rc_ctrl_callbacks, playerapplicationsetting_cb, &rc_addr,p_app_settings->num_attrs, p_app_settings->attrs, 0, NULL);
        get_player_app_setting_cmd(xx, attrs);
        return;
    }

    for(xx = 0; xx < p_rsp->num_attr; xx++) {
        uint8_t x;
        btrc_player_app_ext_attr_t *p_ext_attr;
        p_ext_attr = &p_app_settings->ext_attrs[p_app_settings->ext_val_index];

        for(x = 0; x < p_rsp->num_attr; x++) {
            if(p_ext_attr->ext_attr_val[x].val == p_rsp->p_attrs[xx].attr_id) {
                p_ext_attr->ext_attr_val[x].charset_id = p_rsp->p_attrs[xx].charset_id;
                p_ext_attr->ext_attr_val[x].str_len = p_rsp->p_attrs[xx].str_len;
                p_ext_attr->ext_attr_val[x].p_str = p_rsp->p_attrs[xx].p_str;
                break;
            }
        }
    }

    p_app_settings->ext_val_index++;

    if(p_app_settings->ext_val_index < p_app_settings->num_ext_attrs) {
        attr_index = p_app_settings->ext_val_index;

        for(xx = 0; xx < p_app_settings->ext_attrs[attr_index].num_val; xx++) {
            vals[xx] = p_app_settings->ext_attrs[attr_index].ext_attr_val[xx].val;
        }

        get_player_app_setting_value_text_cmd(vals, xx);
    } else {
        uint8_t x;

        for(xx = 0; xx < p_app_settings->num_attrs; xx++) {
            attrs[xx] = p_app_settings->attrs[xx].attr_id;
        }

        for(x = 0; x < p_app_settings->num_ext_attrs; x++) {
            attrs[xx + x] = p_app_settings->ext_attrs[x].attr_id;
        }

        msg.playerapplicationsetting.bd_addr = &rc_addr;
        msg.playerapplicationsetting.app_attrs = (tls_btrc_player_app_attr_t *)&p_app_settings->attrs[0];
        msg.playerapplicationsetting.num_attr = p_app_settings->num_attrs;
        msg.playerapplicationsetting.num_ext_attr =  p_app_settings->num_ext_attrs;
        msg.playerapplicationsetting.ext_attrs = (tls_btrc_player_app_ext_attr_t *)
                &p_app_settings->ext_attrs[0];

        if(bt_rc_ctrl_callbacks) { bt_rc_ctrl_callbacks(WM_BTRC_CTRL_PLAYERAPPLICATIONSETTING_EVT, &msg); }

        //HAL_CBACK(bt_rc_ctrl_callbacks, playerapplicationsetting_cb, &rc_addr, p_app_settings->num_attrs, p_app_settings->attrs,p_app_settings->num_ext_attrs, p_app_settings->ext_attrs);
        get_player_app_setting_cmd(xx + x, attrs);

        /* Free the application settings information after sending to
         * application.
         */
        for(xx = 0; xx < p_app_settings->ext_attr_index; xx++) {
            int x;
            btrc_player_app_ext_attr_t *p_ext_attr = &p_app_settings->ext_attrs[xx];

            for(x = 0; x < p_ext_attr->num_val; x++) {
                GKI_free_and_reset_buf((void **)&p_ext_attr->ext_attr_val[x].p_str);
            }

            p_ext_attr->num_val = 0;
            GKI_free_and_reset_buf((void **)&p_app_settings->ext_attrs[xx].p_str);
        }

        p_app_settings->num_attrs = 0;
    }
}

/***************************************************************************
**
** Function         handle_set_app_attr_val_response
**
** Description      handles the the set attributes value response, if fails
**                  calls HAL callback to indicate the failure
** Returns          None
**
***************************************************************************/
static void handle_set_app_attr_val_response(tBTA_AV_META_MSG *pmeta_msg, tAVRC_RSP *p_rsp)
{
    uint8_t accepted = 0;
    tls_bt_addr_t rc_addr;
    tls_btrc_ctrl_msg_t msg;
    bdcpy(rc_addr.address, btif_rc_cb.rc_addr);

    /* For timeout pmeta_msg will be NULL, else we need to
     * check if this is accepted by TG
     */
    if(pmeta_msg && (pmeta_msg->code == AVRC_RSP_ACCEPT)) {
        accepted = 1;
    }

    msg.setplayerapplicationsetting_rsp.bd_addr = &rc_addr;
    msg.setplayerapplicationsetting_rsp.accepted = accepted;

    if(bt_rc_ctrl_callbacks) { bt_rc_ctrl_callbacks(WM_BTRC_CTRL_SETPLAYERAPPLICATIONSETTING_RSP_EVT, &msg); }

    //HAL_CBACK(bt_rc_ctrl_callbacks, setplayerappsetting_rsp_cb, &rc_addr, accepted);
}

/***************************************************************************
**
** Function         handle_get_elem_attr_response
**
** Description      handles the the element attributes response, calls
**                  HAL callback to update track change information.
** Returns          None
**
***************************************************************************/
static void handle_get_elem_attr_response(tBTA_AV_META_MSG *pmeta_msg,
        tAVRC_GET_ELEM_ATTRS_RSP *p_rsp)
{
    tls_btrc_ctrl_msg_t msg;

    if(p_rsp->status == AVRC_STS_NO_ERROR) {
        tls_bt_addr_t rc_addr;
        size_t buf_size = p_rsp->num_attr * sizeof(btrc_element_attr_val_t);
        btrc_element_attr_val_t *p_attr =
                        (btrc_element_attr_val_t *)GKI_getbuf(buf_size);
        bdcpy(rc_addr.address, btif_rc_cb.rc_addr);

        for(int i = 0; i < p_rsp->num_attr; i++) {
            p_attr[i].attr_id = p_rsp->p_attrs[i].attr_id;

            /* Todo. Legth limit check to include null */
            if(p_rsp->p_attrs[i].name.str_len &&
                    p_rsp->p_attrs[i].name.p_str) {
                wm_memcpy(p_attr[i].text, p_rsp->p_attrs[i].name.p_str,
                          p_rsp->p_attrs[i].name.str_len);
                GKI_free_and_reset_buf((void **)&p_rsp->p_attrs[i].name.p_str);
            }
        }

        msg.track_changed.bd_addr = &rc_addr;
        msg.track_changed.num_attr = p_rsp->num_attr;
        msg.track_changed.p_attrs = (tls_btrc_element_attr_val_t *)p_attr;

        if(bt_rc_ctrl_callbacks) { bt_rc_ctrl_callbacks(WM_BTRC_CTRL_TRACK_CHANGED_EVT, &msg); }

        //HAL_CBACK(bt_rc_ctrl_callbacks, track_changed_cb,&rc_addr, p_rsp->num_attr, p_attr);
        GKI_freebuf(p_attr);
    } else if(p_rsp->status == BTIF_RC_STS_TIMEOUT) {
        /* Retry for timeout case, this covers error handling
         * for continuation failure also.
         */
        uint32_t attr_list[] = {
            AVRC_MEDIA_ATTR_ID_TITLE,
            AVRC_MEDIA_ATTR_ID_ARTIST,
            AVRC_MEDIA_ATTR_ID_ALBUM,
            AVRC_MEDIA_ATTR_ID_TRACK_NUM,
            AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
            AVRC_MEDIA_ATTR_ID_GENRE,
            AVRC_MEDIA_ATTR_ID_PLAYING_TIME
        };
        get_element_attribute_cmd(AVRC_MAX_NUM_MEDIA_ATTR_ID, attr_list);
    } else {
        BTIF_TRACE_ERROR("%s: Error in get element attr procedure %d",
                         __func__, p_rsp->status);
    }
}

/***************************************************************************
**
** Function         handle_get_playstatus_response
**
** Description      handles the the play status response, calls
**                  HAL callback to update play position.
** Returns          None
**
***************************************************************************/
static void handle_get_playstatus_response(tBTA_AV_META_MSG *pmeta_msg,
        tAVRC_GET_PLAY_STATUS_RSP *p_rsp)
{
    tls_bt_addr_t rc_addr;
    tls_btrc_ctrl_msg_t msg;
    bdcpy(rc_addr.address, btif_rc_cb.rc_addr);

    if(p_rsp->status == AVRC_STS_NO_ERROR) {
        msg.play_position_changed.bd_addr = &rc_addr;
        msg.play_position_changed.song_len = p_rsp->song_len;
        msg.play_position_changed.song_pos = p_rsp->song_pos;

        if(bt_rc_ctrl_callbacks) { bt_rc_ctrl_callbacks(WM_BTRC_CTRL_PLAY_POSITION_CHANGED_EVT, &msg); }

        //HAL_CBACK(bt_rc_ctrl_callbacks, play_position_changed_cb,&rc_addr, p_rsp->song_len, p_rsp->song_pos);
    } else {
        BTIF_TRACE_ERROR("%s: Error in get play status procedure %d",
                         __FUNCTION__, p_rsp->status);
    }
}

/***************************************************************************
**
** Function         clear_cmd_timeout
**
** Description      helper function to stop the command timeout timer
** Returns          None
**
***************************************************************************/
static void clear_cmd_timeout(uint8_t label)
{
    rc_transaction_t *p_txn;
    p_txn = get_transaction_by_lbl(label);

    if(p_txn == NULL) {
        BTIF_TRACE_ERROR("%s: Error in transaction label lookup", __FUNCTION__);
        return;
    }

#ifdef USE_ALARM

    if(p_txn->txn_timer != NULL) {
        alarm_cancel(p_txn->txn_timer);
    }

#else
    btu_stop_timer_oneshot(&p_txn->txn_timer);
#endif
}

/***************************************************************************
**
** Function         handle_avk_rc_metamsg_rsp
**
** Description      Handle RC metamessage response
**
** Returns          void
**
***************************************************************************/
static void handle_avk_rc_metamsg_rsp(tBTA_AV_META_MSG *pmeta_msg)
{
    tAVRC_RESPONSE    avrc_response = {0};
    uint8_t             scratch_buf[512] = {0};// this variable is unused
    uint16_t            buf_len;
    tAVRC_STS         status;
    BTIF_TRACE_DEBUG("%s opcode = %d rsp_code = %d  ", __FUNCTION__,
                     pmeta_msg->p_msg->hdr.opcode, pmeta_msg->code);

    if((AVRC_OP_VENDOR == pmeta_msg->p_msg->hdr.opcode) &&
            (pmeta_msg->code >= AVRC_RSP_NOT_IMPL) &&
            (pmeta_msg->code <= AVRC_RSP_INTERIM)) {
        status = AVRC_Ctrl_ParsResponse(pmeta_msg->p_msg, &avrc_response, scratch_buf, &buf_len);
		UNUSED(status);
        BTIF_TRACE_DEBUG("%s parse status %d pdu = %d rsp_status = %d",
                         __FUNCTION__, status, avrc_response.pdu,
                         pmeta_msg->p_msg->vendor.hdr.ctype);

        switch(avrc_response.pdu) {
            case AVRC_PDU_REGISTER_NOTIFICATION:
                handle_notification_response(pmeta_msg, &avrc_response.reg_notif);

                if(pmeta_msg->code == AVRC_RSP_INTERIM) {
                    /* Don't free the transaction Id */
                    clear_cmd_timeout(pmeta_msg->label);
                    return;
                }

                break;

            case AVRC_PDU_GET_CAPABILITIES:
                handle_get_capability_response(pmeta_msg, &avrc_response.get_caps);
                break;

            case AVRC_PDU_LIST_PLAYER_APP_ATTR:
                handle_app_attr_response(pmeta_msg, &avrc_response.list_app_attr);
                break;

            case AVRC_PDU_LIST_PLAYER_APP_VALUES:
                handle_app_val_response(pmeta_msg, &avrc_response.list_app_values);
                break;

            case AVRC_PDU_GET_CUR_PLAYER_APP_VALUE:
                handle_app_cur_val_response(pmeta_msg, &avrc_response.get_cur_app_val);
                break;

            case AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT:
                handle_app_attr_txt_response(pmeta_msg, &avrc_response.get_app_attr_txt);
                break;

            case AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT:
                handle_app_attr_val_txt_response(pmeta_msg, &avrc_response.get_app_val_txt);
                break;

            case AVRC_PDU_SET_PLAYER_APP_VALUE:
                handle_set_app_attr_val_response(pmeta_msg, &avrc_response.set_app_val);
                break;

            case AVRC_PDU_GET_ELEMENT_ATTR:
                handle_get_elem_attr_response(pmeta_msg, &avrc_response.get_elem_attrs);
                break;

            case AVRC_PDU_GET_PLAY_STATUS:
                handle_get_playstatus_response(pmeta_msg, &avrc_response.get_play_status);
                break;
        }

        release_transaction(pmeta_msg->label);
    } else {
        BTIF_TRACE_DEBUG("%s:Invalid Vendor Command  code: %d len: %d. Not processing it.",
                         __FUNCTION__, pmeta_msg->code, pmeta_msg->len);
        return;
    }
}

/***************************************************************************
**
** Function         handle_avk_rc_metamsg_cmd
**
** Description      Handle RC metamessage response
**
** Returns          void
**
***************************************************************************/
static void handle_avk_rc_metamsg_cmd(tBTA_AV_META_MSG *pmeta_msg)
{
    tAVRC_COMMAND    avrc_cmd = {0};
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
    BTIF_TRACE_DEBUG("%s opcode = %d rsp_code = %d  ", __FUNCTION__,
                     pmeta_msg->p_msg->hdr.opcode, pmeta_msg->code);

    if((AVRC_OP_VENDOR == pmeta_msg->p_msg->hdr.opcode) &&
            (pmeta_msg->code <= AVRC_CMD_GEN_INQ)) {
        status = AVRC_Ctrl_ParsCommand(pmeta_msg->p_msg, &avrc_cmd);
        BTIF_TRACE_DEBUG("%s Received vendor command.code %d, PDU %d label %d",
                         __FUNCTION__, pmeta_msg->code, avrc_cmd.pdu, pmeta_msg->label);

        if(status != AVRC_STS_NO_ERROR) {
            /* return error */
            BTIF_TRACE_WARNING("%s: Error in parsing received metamsg command. status: 0x%02x",
                               __FUNCTION__, status);
            send_reject_response(pmeta_msg->rc_handle, pmeta_msg->label, avrc_cmd.pdu, status);
        } else {
            if(avrc_cmd.pdu == AVRC_PDU_REGISTER_NOTIFICATION) {
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)				
                uint8_t event_id = avrc_cmd.reg_notif.event_id;
#endif
                BTIF_TRACE_EVENT("%s:Register notification event_id: %s",
                                 __FUNCTION__, dump_rc_notification_event_id(event_id));
            } else if(avrc_cmd.pdu == AVRC_PDU_SET_ABSOLUTE_VOLUME) {
                BTIF_TRACE_EVENT("%s: Abs Volume Cmd Recvd", __FUNCTION__);
            }

            btif_rc_ctrl_upstreams_rsp_cmd(avrc_cmd.pdu, &avrc_cmd, pmeta_msg->label);
        }
    } else {
        BTIF_TRACE_DEBUG("%s:Invalid Vendor Command  code: %d len: %d. Not processing it.",
                         __FUNCTION__, pmeta_msg->code, pmeta_msg->len);
        return;
    }
}
#endif



/***************************************************************************
**
** Function         getcapabilities_cmd
**
** Description      GetCapabilties from Remote(Company_ID, Events_Supported)
**
** Returns          void
**
***************************************************************************/
static tls_bt_status_t getcapabilities_cmd(uint8_t cap_id)
{
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
    rc_transaction_t *p_transaction = NULL;
#if (AVRC_CTLR_INCLUDED == TRUE)
    BTIF_TRACE_DEBUG("%s: cap_id %d", __FUNCTION__, cap_id);
    CHECK_RC_CONNECTED
    tls_bt_status_t tran_status = get_transaction(&p_transaction);

    if(TLS_BT_STATUS_SUCCESS != tran_status) {
        return TLS_BT_STATUS_FAIL;
    }

    tAVRC_COMMAND avrc_cmd = {0};
    BT_HDR *p_msg = NULL;
    avrc_cmd.get_caps.opcode = AVRC_OP_VENDOR;
    avrc_cmd.get_caps.capability_id = cap_id;
    avrc_cmd.get_caps.pdu = AVRC_PDU_GET_CAPABILITIES;
    avrc_cmd.get_caps.status = AVRC_STS_NO_ERROR;
    status = AVRC_BldCommand(&avrc_cmd, &p_msg);

    if((status == AVRC_STS_NO_ERROR) && (p_msg != NULL)) {
        uint8_t *data_start = (uint8_t *)(p_msg + 1) + p_msg->offset;
        BTIF_TRACE_DEBUG("%s msgreq being sent out with label %d",
                         __FUNCTION__, p_transaction->lbl);
        BTA_AvVendorCmd(btif_rc_cb.rc_handle, p_transaction->lbl, AVRC_CMD_STATUS,
                        data_start, p_msg->len);
        status =  TLS_BT_STATUS_SUCCESS;
        start_status_command_timer(AVRC_PDU_GET_CAPABILITIES, p_transaction);
    } else {
        BTIF_TRACE_ERROR("%s: failed to build command. status: 0x%02x",
                         __FUNCTION__, status);
    }

    GKI_freebuf(p_msg);
#else
    BTIF_TRACE_DEBUG("%s: feature not enabled", __FUNCTION__);
#endif
    return status;
}

/***************************************************************************
**
** Function         list_player_app_setting_attrib_cmd
**
** Description      Get supported List Player Attributes
**
** Returns          void
**
***************************************************************************/
static tls_bt_status_t list_player_app_setting_attrib_cmd(void)
{
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
    rc_transaction_t *p_transaction = NULL;
#if (AVRC_CTLR_INCLUDED == TRUE)
    BTIF_TRACE_DEBUG("%s: ", __FUNCTION__);
    CHECK_RC_CONNECTED
    tls_bt_status_t tran_status = get_transaction(&p_transaction);

    if(TLS_BT_STATUS_SUCCESS != tran_status) {
        return TLS_BT_STATUS_FAIL;
    }

    tAVRC_COMMAND avrc_cmd = {0};
    BT_HDR *p_msg = NULL;
    avrc_cmd.list_app_attr.opcode = AVRC_OP_VENDOR;
    avrc_cmd.list_app_attr.pdu = AVRC_PDU_LIST_PLAYER_APP_ATTR;
    avrc_cmd.list_app_attr.status = AVRC_STS_NO_ERROR;
    status = AVRC_BldCommand(&avrc_cmd, &p_msg);

    if((status == AVRC_STS_NO_ERROR) && (p_msg != NULL)) {
        uint8_t *data_start = (uint8_t *)(p_msg + 1) + p_msg->offset;
        BTIF_TRACE_DEBUG("%s msgreq being sent out with label %d",
                         __FUNCTION__, p_transaction->lbl);
        BTA_AvVendorCmd(btif_rc_cb.rc_handle, p_transaction->lbl, AVRC_CMD_STATUS,
                        data_start, p_msg->len);
        status =  TLS_BT_STATUS_SUCCESS;
        start_status_command_timer(AVRC_PDU_LIST_PLAYER_APP_ATTR, p_transaction);
    } else {
        BTIF_TRACE_ERROR("%s: failed to build command. status: 0x%02x",
                         __FUNCTION__, status);
    }

    GKI_freebuf(p_msg);
#else
    BTIF_TRACE_DEBUG("%s: feature not enabled", __FUNCTION__);
#endif
    return status;
}

/***************************************************************************
**
** Function         list_player_app_setting_value_cmd
**
** Description      Get values of supported Player Attributes
**
** Returns          void
**
***************************************************************************/
static tls_bt_status_t list_player_app_setting_value_cmd(uint8_t attrib_id)
{
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
    rc_transaction_t *p_transaction = NULL;
#if (AVRC_CTLR_INCLUDED == TRUE)
    BTIF_TRACE_DEBUG("%s: attrib_id %d", __FUNCTION__, attrib_id);
    CHECK_RC_CONNECTED
    tls_bt_status_t tran_status = get_transaction(&p_transaction);

    if(TLS_BT_STATUS_SUCCESS != tran_status) {
        return TLS_BT_STATUS_FAIL;
    }

    tAVRC_COMMAND avrc_cmd = {0};
    BT_HDR *p_msg = NULL;
    avrc_cmd.list_app_values.attr_id = attrib_id;
    avrc_cmd.list_app_values.opcode = AVRC_OP_VENDOR;
    avrc_cmd.list_app_values.pdu = AVRC_PDU_LIST_PLAYER_APP_VALUES;
    avrc_cmd.list_app_values.status = AVRC_STS_NO_ERROR;
    status = AVRC_BldCommand(&avrc_cmd, &p_msg);

    if((status == AVRC_STS_NO_ERROR) && (p_msg != NULL)) {
        uint8_t *data_start = (uint8_t *)(p_msg + 1) + p_msg->offset;
        BTIF_TRACE_DEBUG("%s msgreq being sent out with label %d",
                         __FUNCTION__, p_transaction->lbl);
        BTA_AvVendorCmd(btif_rc_cb.rc_handle, p_transaction->lbl, AVRC_CMD_STATUS,
                        data_start, p_msg->len);
        status =  TLS_BT_STATUS_SUCCESS;
        start_status_command_timer(AVRC_PDU_LIST_PLAYER_APP_VALUES, p_transaction);
    } else {
        BTIF_TRACE_ERROR("%s: failed to build command. status: 0x%02x", __FUNCTION__, status);
    }

    GKI_freebuf(p_msg);
#else
    BTIF_TRACE_DEBUG("%s: feature not enabled", __FUNCTION__);
#endif
    return status;
}

/***************************************************************************
**
** Function         get_player_app_setting_cmd
**
** Description      Get current values of Player Attributes
**
** Returns          void
**
***************************************************************************/
static tls_bt_status_t get_player_app_setting_cmd(uint8_t num_attrib, uint8_t *attrib_ids)
{
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
    rc_transaction_t *p_transaction = NULL;
    int count  = 0;
#if (AVRC_CTLR_INCLUDED == TRUE)
    BTIF_TRACE_DEBUG("%s: num attrib_id %d", __FUNCTION__, num_attrib);
    CHECK_RC_CONNECTED
    tls_bt_status_t tran_status = get_transaction(&p_transaction);

    if(TLS_BT_STATUS_SUCCESS != tran_status) {
        return TLS_BT_STATUS_FAIL;
    }

    tAVRC_COMMAND avrc_cmd = {0};
    BT_HDR *p_msg = NULL;
    avrc_cmd.get_cur_app_val.opcode = AVRC_OP_VENDOR;
    avrc_cmd.get_cur_app_val.status = AVRC_STS_NO_ERROR;
    avrc_cmd.get_cur_app_val.num_attr = num_attrib;
    avrc_cmd.get_cur_app_val.pdu = AVRC_PDU_GET_CUR_PLAYER_APP_VALUE;

    for(count = 0; count < num_attrib; count++) {
        avrc_cmd.get_cur_app_val.attrs[count] = attrib_ids[count];
    }

    status = AVRC_BldCommand(&avrc_cmd, &p_msg);

    if((status == AVRC_STS_NO_ERROR) && (p_msg != NULL)) {
        uint8_t *data_start = (uint8_t *)(p_msg + 1) + p_msg->offset;
        BTIF_TRACE_DEBUG("%s msgreq being sent out with label %d",
                         __FUNCTION__, p_transaction->lbl);
        BTA_AvVendorCmd(btif_rc_cb.rc_handle, p_transaction->lbl, AVRC_CMD_STATUS,
                        data_start, p_msg->len);
        status =  TLS_BT_STATUS_SUCCESS;
        start_status_command_timer(AVRC_PDU_GET_CUR_PLAYER_APP_VALUE, p_transaction);
    } else {
        BTIF_TRACE_ERROR("%s: failed to build command. status: 0x%02x",
                         __FUNCTION__, status);
    }

    GKI_freebuf(p_msg);
#else
    BTIF_TRACE_DEBUG("%s: feature not enabled", __FUNCTION__);
#endif
    return status;
}



/***************************************************************************
**
** Function         get_player_app_setting_attr_text_cmd
**
** Description      Get text description for app attribute
**
** Returns          void
**
***************************************************************************/
static tls_bt_status_t get_player_app_setting_attr_text_cmd(uint8_t *attrs, uint8_t num_attrs)
{
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
    rc_transaction_t *p_transaction = NULL;
    int count  = 0;
#if (AVRC_CTLR_INCLUDED == TRUE)
    tAVRC_COMMAND avrc_cmd = {0};
    BT_HDR *p_msg = NULL;
    tls_bt_status_t tran_status;
    CHECK_RC_CONNECTED
    BTIF_TRACE_DEBUG("%s: num attrs %d", __FUNCTION__, num_attrs);
    tran_status = get_transaction(&p_transaction);

    if(TLS_BT_STATUS_SUCCESS != tran_status) {
        return TLS_BT_STATUS_FAIL;
    }

    avrc_cmd.pdu = AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT;
    avrc_cmd.get_app_attr_txt.opcode = AVRC_OP_VENDOR;
    avrc_cmd.get_app_attr_txt.num_attr = num_attrs;

    for(count = 0; count < num_attrs; count++) {
        avrc_cmd.get_app_attr_txt.attrs[count] = attrs[count];
    }

    status = AVRC_BldCommand(&avrc_cmd, &p_msg);

    if(status == AVRC_STS_NO_ERROR) {
        uint8_t *data_start = (uint8_t *)(p_msg + 1) + p_msg->offset;
        BTIF_TRACE_DEBUG("%s msgreq being sent out with label %d",
                         __FUNCTION__, p_transaction->lbl);
        BTA_AvVendorCmd(btif_rc_cb.rc_handle, p_transaction->lbl,
                        AVRC_CMD_STATUS, data_start, p_msg->len);
        GKI_freebuf(p_msg);
        status =  TLS_BT_STATUS_SUCCESS;
        start_status_command_timer(AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT, p_transaction);
    } else {
        BTIF_TRACE_ERROR("%s: failed to build command. status: 0x%02x", __FUNCTION__, status);
    }

    GKI_freebuf(p_msg);
#else
    BTIF_TRACE_DEBUG("%s: feature not enabled", __FUNCTION__);
#endif
    return status;
}

/***************************************************************************
**
** Function         get_player_app_setting_val_text_cmd
**
** Description      Get text description for app attribute values
**
** Returns          void
**
***************************************************************************/
static tls_bt_status_t get_player_app_setting_value_text_cmd(uint8_t *vals, uint8_t num_vals)
{
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
    rc_transaction_t *p_transaction = NULL;
    int count  = 0;
#if (AVRC_CTLR_INCLUDED == TRUE)
    tAVRC_COMMAND avrc_cmd = {0};
    BT_HDR *p_msg = NULL;
    tls_bt_status_t tran_status;
    CHECK_RC_CONNECTED
    BTIF_TRACE_DEBUG("%s: num_vals %d", __FUNCTION__, num_vals);
    tran_status = get_transaction(&p_transaction);

    if(TLS_BT_STATUS_SUCCESS != tran_status) {
        return TLS_BT_STATUS_FAIL;
    }

    avrc_cmd.pdu = AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT;
    avrc_cmd.get_app_val_txt.opcode = AVRC_OP_VENDOR;
    avrc_cmd.get_app_val_txt.num_val = num_vals;

    for(count = 0; count < num_vals; count++) {
        avrc_cmd.get_app_val_txt.vals[count] = vals[count];
    }

    status = AVRC_BldCommand(&avrc_cmd, &p_msg);

    if(status == AVRC_STS_NO_ERROR) {
        uint8_t *data_start = (uint8_t *)(p_msg + 1) + p_msg->offset;
        BTIF_TRACE_DEBUG("%s msgreq being sent out with label %d",
                         __FUNCTION__, p_transaction->lbl);

        if(p_msg != NULL) {
            BTA_AvVendorCmd(btif_rc_cb.rc_handle, p_transaction->lbl,
                            AVRC_CMD_STATUS, data_start, p_msg->len);
            status =  TLS_BT_STATUS_SUCCESS;
            start_status_command_timer(AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT, p_transaction);
        }
    } else {
        BTIF_TRACE_ERROR("%s: failed to build command. status: 0x%02x",
                         __FUNCTION__, status);
    }

    GKI_freebuf(p_msg);
#else
    BTIF_TRACE_DEBUG("%s: feature not enabled", __FUNCTION__);
#endif
    return status;
}

/***************************************************************************
**
** Function         register_notification_cmd
**
** Description      Send Command to register for a Notification ID
**
** Returns          void
**
***************************************************************************/
static tls_bt_status_t register_notification_cmd(uint8_t label, uint8_t event_id,
        uint32_t event_value)
{
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
#if (AVRC_CTLR_INCLUDED == TRUE)
    tAVRC_COMMAND avrc_cmd = {0};
    BT_HDR *p_msg = NULL;
    CHECK_RC_CONNECTED
    BTIF_TRACE_DEBUG("%s: event_id %d  event_value", __FUNCTION__, event_id, event_value);
    avrc_cmd.reg_notif.opcode = AVRC_OP_VENDOR;
    avrc_cmd.reg_notif.status = AVRC_STS_NO_ERROR;
    avrc_cmd.reg_notif.event_id = event_id;
    avrc_cmd.reg_notif.pdu = AVRC_PDU_REGISTER_NOTIFICATION;
    avrc_cmd.reg_notif.param = event_value;
    status = AVRC_BldCommand(&avrc_cmd, &p_msg);

    if(status == AVRC_STS_NO_ERROR) {
        uint8_t *data_start = (uint8_t *)(p_msg + 1) + p_msg->offset;
        BTIF_TRACE_DEBUG("%s msgreq being sent out with label %d",
                         __FUNCTION__, label);

        if(p_msg != NULL) {
            BTA_AvVendorCmd(btif_rc_cb.rc_handle, label, AVRC_CMD_NOTIF,
                            data_start, p_msg->len);
            status =  TLS_BT_STATUS_SUCCESS;
        }
    } else {
        BTIF_TRACE_ERROR("%s: failed to build command. status: 0x%02x",
                         __FUNCTION__, status);
    }

    GKI_freebuf(p_msg);
#else
    BTIF_TRACE_DEBUG("%s: feature not enabled", __FUNCTION__);
#endif
    return status;
}

/***************************************************************************
**
** Function         get_element_attribute_cmd
**
** Description      Get Element Attribute for  attributeIds
**
** Returns          void
**
***************************************************************************/
static tls_bt_status_t get_element_attribute_cmd(uint8_t num_attribute, uint32_t *p_attr_ids)
{
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
    rc_transaction_t *p_transaction = NULL;
    int count  = 0;
#if (AVRC_CTLR_INCLUDED == TRUE)
    tAVRC_COMMAND avrc_cmd = {0};
    BT_HDR *p_msg = NULL;
    tls_bt_status_t tran_status;
    CHECK_RC_CONNECTED
    BTIF_TRACE_DEBUG("%s: num_attribute  %d attribute_id %d",
                     __FUNCTION__, num_attribute, p_attr_ids[0]);
    tran_status = get_transaction(&p_transaction);

    if(TLS_BT_STATUS_SUCCESS != tran_status) {
        return TLS_BT_STATUS_FAIL;
    }

    avrc_cmd.get_elem_attrs.opcode = AVRC_OP_VENDOR;
    avrc_cmd.get_elem_attrs.status = AVRC_STS_NO_ERROR;
    avrc_cmd.get_elem_attrs.num_attr = num_attribute;
    avrc_cmd.get_elem_attrs.pdu = AVRC_PDU_GET_ELEMENT_ATTR;

    for(count = 0; count < num_attribute; count++) {
        avrc_cmd.get_elem_attrs.attrs[count] = p_attr_ids[count];
    }

    status = AVRC_BldCommand(&avrc_cmd, &p_msg);

    if(status == AVRC_STS_NO_ERROR) {
        uint8_t *data_start = (uint8_t *)(p_msg + 1) + p_msg->offset;
        BTIF_TRACE_DEBUG("%s msgreq being sent out with label %d",
                         __FUNCTION__, p_transaction->lbl);

        if(p_msg != NULL) {
            BTA_AvVendorCmd(btif_rc_cb.rc_handle, p_transaction->lbl,
                            AVRC_CMD_STATUS, data_start, p_msg->len);
            status =  TLS_BT_STATUS_SUCCESS;
            start_status_command_timer(AVRC_PDU_GET_ELEMENT_ATTR,
                                       p_transaction);
        }
    } else {
        BTIF_TRACE_ERROR("%s: failed to build command. status: 0x%02x",
                         __FUNCTION__, status);
    }

    GKI_freebuf(p_msg);
#else
    BTIF_TRACE_DEBUG("%s: feature not enabled", __FUNCTION__);
#endif
    return status;
}

/***************************************************************************
**
** Function         get_play_status_cmd
**
** Description      Get Element Attribute for  attributeIds
**
** Returns          void
**
***************************************************************************/
static tls_bt_status_t get_play_status_cmd(void)
{
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
    rc_transaction_t *p_transaction = NULL;
#if (AVRC_CTLR_INCLUDED == TRUE)
    tAVRC_COMMAND avrc_cmd = {0};
    BT_HDR *p_msg = NULL;
    tls_bt_status_t tran_status;
    CHECK_RC_CONNECTED
    BTIF_TRACE_DEBUG("%s: ", __FUNCTION__);
    tran_status = get_transaction(&p_transaction);

    if(TLS_BT_STATUS_SUCCESS != tran_status) {
        return TLS_BT_STATUS_FAIL;
    }

    avrc_cmd.get_play_status.opcode = AVRC_OP_VENDOR;
    avrc_cmd.get_play_status.pdu = AVRC_PDU_GET_PLAY_STATUS;
    avrc_cmd.get_play_status.status = AVRC_STS_NO_ERROR;
    status = AVRC_BldCommand(&avrc_cmd, &p_msg);

    if(status == AVRC_STS_NO_ERROR) {
        uint8_t *data_start = (uint8_t *)(p_msg + 1) + p_msg->offset;
        BTIF_TRACE_DEBUG("%s msgreq being sent out with label %d",
                         __FUNCTION__, p_transaction->lbl);

        if(p_msg != NULL) {
            BTA_AvVendorCmd(btif_rc_cb.rc_handle, p_transaction->lbl,
                            AVRC_CMD_STATUS, data_start, p_msg->len);
            status =  TLS_BT_STATUS_SUCCESS;
            start_status_command_timer(AVRC_PDU_GET_PLAY_STATUS, p_transaction);
        }
    } else {
        BTIF_TRACE_ERROR("%s: failed to build command. status: 0x%02x",
                         __FUNCTION__, status);
    }

    GKI_freebuf(p_msg);
#else
    BTIF_TRACE_DEBUG("%s: feature not enabled", __FUNCTION__);
#endif
    return status;
}



/*******************************************************************************
**      Function         initialize_transaction
**
**      Description    Initializes fields of the transaction structure
**
**      Returns          void
*******************************************************************************/
static void initialize_transaction(int lbl)
{
    //pthread_mutex_lock(&device.lbllock);
    if(lbl < MAX_TRANSACTIONS_PER_SESSION) {
#ifdef USE_ALARM

        if(alarm_is_scheduled(device.transaction[lbl].txn_timer)) {
#else

        if(device.transaction[lbl].txn_timer.in_use) {
#endif
            clear_cmd_timeout(lbl);
        }

        device.transaction[lbl].lbl = lbl;
        device.transaction[lbl].in_use = FALSE;
        device.transaction[lbl].handle = 0;
    }

    //pthread_mutex_unlock(&device.lbllock);
}

/*******************************************************************************
**      Function         lbl_init
**
**      Description    Initializes label structures and mutexes.
**
**      Returns         void
*******************************************************************************/
void lbl_init()
{
    wm_memset(&device, 0, sizeof(rc_device_t));
#ifdef USE_PTHREAD
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&(device.lbllock), &attr);
    pthread_mutexattr_destroy(&attr);
    init_all_transactions();
#endif
}

/*******************************************************************************
**
** Function         init_all_transactions
**
** Description    Initializes all transactions
**
** Returns          void
*******************************************************************************/
void init_all_transactions()
{
    uint8_t txn_indx = 0;

    for(txn_indx = 0; txn_indx < MAX_TRANSACTIONS_PER_SESSION; txn_indx++) {
        initialize_transaction(txn_indx);
    }
}

/*******************************************************************************
**
** Function         get_transaction_by_lbl
**
** Description    Will return a transaction based on the label. If not in use
**                     will return an error.
**
** Returns          bt_status_t
*******************************************************************************/
rc_transaction_t *get_transaction_by_lbl(uint8_t lbl)
{
    rc_transaction_t *transaction = NULL;
    //pthread_mutex_lock(&device.lbllock);

    /* Determine if this is a valid label */
    if(lbl < MAX_TRANSACTIONS_PER_SESSION) {
        if(FALSE == device.transaction[lbl].in_use) {
            transaction = NULL;
        } else {
            transaction = &(device.transaction[lbl]);
            BTIF_TRACE_DEBUG("%s: Got transaction.label: %d", __FUNCTION__, lbl);
        }
    }

    //pthread_mutex_unlock(&device.lbllock);
    return transaction;
}

/*******************************************************************************
**
** Function         get_transaction
**
** Description    Obtains the transaction details.
**
** Returns          bt_status_t
*******************************************************************************/

tls_bt_status_t  get_transaction(rc_transaction_t **ptransaction)
{
    tls_bt_status_t result = TLS_BT_STATUS_NOMEM;
    uint8_t i = 0;
    //pthread_mutex_lock(&device.lbllock);

    // Check for unused transactions
    for(i = 0; i < MAX_TRANSACTIONS_PER_SESSION; i++) {
        if(FALSE == device.transaction[i].in_use) {
            BTIF_TRACE_DEBUG("%s:Got transaction.label: %d", __FUNCTION__, device.transaction[i].lbl);
            device.transaction[i].in_use = TRUE;
            *ptransaction = &(device.transaction[i]);
            result = TLS_BT_STATUS_SUCCESS;
            break;
        }
    }

    //pthread_mutex_unlock(&device.lbllock);
    return result;
}

/*******************************************************************************
**
** Function         release_transaction
**
** Description    Will release a transaction for reuse
**
** Returns          bt_status_t
*******************************************************************************/
void release_transaction(uint8_t lbl)
{
    rc_transaction_t *transaction = get_transaction_by_lbl(lbl);

    /* If the transaction is in use... */
    if(transaction != NULL) {
        BTIF_TRACE_DEBUG("%s: lbl: %d", __FUNCTION__, lbl);
        initialize_transaction(lbl);
    }
}

/*******************************************************************************
**
** Function         lbl_destroy
**
** Description    Cleanup of the mutex
**
** Returns          void
*******************************************************************************/
void lbl_destroy()
{
    //pthread_mutex_destroy(&(device.lbllock));
}

/*******************************************************************************
**      Function       dev_blacklisted_for_absolute_volume
**
**      Description    Blacklist Devices that donot handle absolute volume well
**                     We are blacklisting all the devices that are not in whitelist
**
**      Returns        True if the device is in the list
*******************************************************************************/
static uint8_t dev_blacklisted_for_absolute_volume(BD_ADDR peer_dev)
{
    int i;
    char *dev_name_str = NULL;
    int whitelist_size = sizeof(rc_white_addr_prefix) / sizeof(rc_white_addr_prefix[0]);

    for(i = 0; i < whitelist_size; i++) {
        if(rc_white_addr_prefix[i][0] == peer_dev[0] &&
                rc_white_addr_prefix[i][1] == peer_dev[1] &&
                rc_white_addr_prefix[i][2] == peer_dev[2]) {
            BTIF_TRACE_DEBUG("whitelist absolute volume for %02x:%02x:%02x",
                             peer_dev[0], peer_dev[1], peer_dev[2]);
            return FALSE;
        }
    }

    dev_name_str = BTM_SecReadDevName(peer_dev);
    whitelist_size = sizeof(rc_white_name) / sizeof(char *);

    if(dev_name_str != NULL) {
        for(i = 0; i < whitelist_size; i++) {
            if(strcmp(dev_name_str, rc_white_name[i]) == 0) {
                BTIF_TRACE_DEBUG("whitelist absolute volume for %s", dev_name_str);
                return FALSE;
            }
        }
    }

    BTIF_TRACE_WARNING("blacklist absolute volume for %02x:%02x:%02x, name = %s",
                       peer_dev[0], peer_dev[1], peer_dev[2], dev_name_str);
    return TRUE;
}


//static uint8_t absolute_volume_disabled()
//{
//    return false;
//}

/**exported api*/

/**btrc related api supported by now*/
tls_bt_status_t tls_btrc_init(tls_btrc_callback_t callback)
{
    BTIF_TRACE_EVENT("## %s ##", __FUNCTION__);
    tls_bt_status_t result = TLS_BT_STATUS_SUCCESS;

    if(bt_rc_callbacks) {
        return TLS_BT_STATUS_DONE;
    }

    bt_rc_callbacks = callback;
    wm_memset(&btif_rc_cb, 0, sizeof(btif_rc_cb));
    btif_rc_cb.rc_vol_label = MAX_LABEL;
    btif_rc_cb.rc_volume = MAX_VOLUME;
    lbl_init();
    return result;
}

tls_bt_status_t tls_btrc_deinit(void)
{
    BTIF_TRACE_EVENT("## %s ##", __FUNCTION__);
    close_uinput();

    if(bt_rc_callbacks) {
        bt_rc_callbacks = NULL;
    }

#ifdef USE_ALARM
    alarm_free(btif_rc_cb.rc_play_status_timer);
#endif
    wm_memset(&btif_rc_cb, 0, sizeof(btif_rc_cb_t));
    lbl_destroy();
    BTIF_TRACE_EVENT("## %s ## completed", __FUNCTION__);
    return TLS_BT_STATUS_SUCCESS;
}

tls_bt_status_t tls_btrc_get_play_status_rsp(tls_btrc_play_status_t play_status, uint32_t song_len,
        uint32_t song_pos)
{
    tAVRC_RESPONSE avrc_rsp;
    CHECK_RC_CONNECTED
    wm_memset(&(avrc_rsp.get_play_status), 0, sizeof(tAVRC_GET_PLAY_STATUS_RSP));
    avrc_rsp.get_play_status.song_len = song_len;
    avrc_rsp.get_play_status.song_pos = song_pos;
    avrc_rsp.get_play_status.play_status = play_status;
    avrc_rsp.get_play_status.pdu = AVRC_PDU_GET_PLAY_STATUS;
    avrc_rsp.get_play_status.opcode = opcode_from_pdu(AVRC_PDU_GET_PLAY_STATUS);
    avrc_rsp.get_play_status.status = AVRC_STS_NO_ERROR;
    /* Send the response */
    SEND_METAMSG_RSP(IDX_GET_PLAY_STATUS_RSP, &avrc_rsp);
    return TLS_BT_STATUS_SUCCESS;
}
tls_bt_status_t tls_btrc_get_element_attr_rsp(uint8_t num_attr,
        tls_btrc_element_attr_val_t *p_attrs)
{
    tAVRC_RESPONSE avrc_rsp;
    uint32_t i;
    tAVRC_ATTR_ENTRY element_attrs[BTRC_MAX_ELEM_ATTR_SIZE];
    CHECK_RC_CONNECTED
    wm_memset(element_attrs, 0, sizeof(tAVRC_ATTR_ENTRY) * num_attr);

    if(num_attr == 0) {
        avrc_rsp.get_play_status.status = AVRC_STS_BAD_PARAM;
    } else {
        for(i = 0; i < num_attr; i++) {
            element_attrs[i].attr_id = p_attrs[i].attr_id;
            element_attrs[i].name.charset_id = AVRC_CHARSET_ID_UTF8;
            element_attrs[i].name.str_len = (uint16_t)strlen((char *)p_attrs[i].text);
            element_attrs[i].name.p_str = p_attrs[i].text;
            BTIF_TRACE_DEBUG("%s attr_id:0x%x, charset_id:0x%x, str_len:%d, str:%s",
                             __FUNCTION__, (unsigned int)element_attrs[i].attr_id,
                             element_attrs[i].name.charset_id, element_attrs[i].name.str_len,
                             element_attrs[i].name.p_str);
        }

        avrc_rsp.get_play_status.status = AVRC_STS_NO_ERROR;
    }

    avrc_rsp.get_elem_attrs.num_attr = num_attr;
    avrc_rsp.get_elem_attrs.p_attrs = element_attrs;
    avrc_rsp.get_elem_attrs.pdu = AVRC_PDU_GET_ELEMENT_ATTR;
    avrc_rsp.get_elem_attrs.opcode = opcode_from_pdu(AVRC_PDU_GET_ELEMENT_ATTR);
    /* Send the response */
    SEND_METAMSG_RSP(IDX_GET_ELEMENT_ATTR_RSP, &avrc_rsp);
    return TLS_BT_STATUS_SUCCESS;
}

tls_bt_status_t tls_btrc_register_notification_rsp(tls_btrc_event_id_t event_id,
        tls_btrc_notification_type_t type, tls_btrc_register_notification_t *p_param)
{
    tAVRC_RESPONSE avrc_rsp;
    CHECK_RC_CONNECTED
    BTIF_TRACE_EVENT("## %s ## event_id:%s", __FUNCTION__, dump_rc_notification_event_id(event_id));

    if(btif_rc_cb.rc_notif[event_id - 1].bNotify == FALSE) {
        BTIF_TRACE_ERROR("Avrcp Event id not registered: event_id = %x", event_id);
        return TLS_BT_STATUS_NOT_READY;
    }

    wm_memset(&(avrc_rsp.reg_notif), 0, sizeof(tAVRC_REG_NOTIF_RSP));
    avrc_rsp.reg_notif.event_id = event_id;

    switch(event_id) {
        case BTRC_EVT_PLAY_STATUS_CHANGED:
            avrc_rsp.reg_notif.param.play_status = p_param->play_status;

            if(avrc_rsp.reg_notif.param.play_status == PLAY_STATUS_PLAYING) {
                btif_av_clear_remote_suspend_flag();
            }

            break;

        case BTRC_EVT_TRACK_CHANGE:
            wm_memcpy(&(avrc_rsp.reg_notif.param.track), &(p_param->track), sizeof(btrc_uid_t));
            break;

        case BTRC_EVT_PLAY_POS_CHANGED:
            avrc_rsp.reg_notif.param.play_pos = p_param->song_pos;
            break;

        default:
            BTIF_TRACE_WARNING("%s : Unhandled event ID : 0x%x", __FUNCTION__, event_id);
            return TLS_BT_STATUS_UNHANDLED;
    }

    avrc_rsp.reg_notif.pdu = AVRC_PDU_REGISTER_NOTIFICATION;
    avrc_rsp.reg_notif.opcode = opcode_from_pdu(AVRC_PDU_REGISTER_NOTIFICATION);
    avrc_rsp.get_play_status.status = AVRC_STS_NO_ERROR;
    /* Send the response. */
    send_metamsg_rsp(btif_rc_cb.rc_handle, btif_rc_cb.rc_notif[event_id - 1].label,
                     (((int)type == (int)BTRC_NOTIFICATION_TYPE_INTERIM) ? AVRC_CMD_NOTIF : AVRC_RSP_CHANGED), &avrc_rsp);
    return TLS_BT_STATUS_SUCCESS;
}

tls_bt_status_t tls_btrc_set_volume(uint8_t volume)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    CHECK_RC_CONNECTED
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
    rc_transaction_t *p_transaction = NULL;

    if(btif_rc_cb.rc_volume == volume) {
        status = TLS_BT_STATUS_DONE;
        BTIF_TRACE_ERROR("%s: volume value already set earlier: 0x%02x", __FUNCTION__, volume);
        return status;
    }

    if((btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG) &&
            (btif_rc_cb.rc_features & BTA_AV_FEAT_ADV_CTRL)) {
        tAVRC_COMMAND avrc_cmd = {0};
        BT_HDR *p_msg = NULL;
        BTIF_TRACE_DEBUG("%s: Peer supports absolute volume. newVolume=%d", __FUNCTION__, volume);
        avrc_cmd.volume.opcode = AVRC_OP_VENDOR;
        avrc_cmd.volume.pdu = AVRC_PDU_SET_ABSOLUTE_VOLUME;
        avrc_cmd.volume.status = AVRC_STS_NO_ERROR;
        avrc_cmd.volume.volume = volume;

        if(AVRC_BldCommand(&avrc_cmd, &p_msg) == AVRC_STS_NO_ERROR) {
            tls_bt_status_t tran_status = get_transaction(&p_transaction);

            if(TLS_BT_STATUS_SUCCESS == tran_status && NULL != p_transaction) {
                BTIF_TRACE_DEBUG("%s msgreq being sent out with label %d",
                                 __FUNCTION__, p_transaction->lbl);
                BTA_AvMetaCmd(btif_rc_cb.rc_handle, p_transaction->lbl, AVRC_CMD_CTRL, p_msg);
                status =  TLS_BT_STATUS_SUCCESS;
            } else {
                GKI_freebuf(p_msg);
                BTIF_TRACE_ERROR("%s: failed to obtain transaction details. status: 0x%02x",
                                 __FUNCTION__, tran_status);
                status = TLS_BT_STATUS_FAIL;
            }
        } else {
            BTIF_TRACE_ERROR("%s: failed to build absolute volume command. status: 0x%02x",
                             __FUNCTION__, status);
            status = TLS_BT_STATUS_FAIL;
        }
    } else {
        status = TLS_BT_STATUS_NOT_READY;
    }

    return status;
}


/**btrc ctrl related api supported by now*/

tls_bt_status_t tls_btrc_ctrl_init(tls_btrc_ctrl_callback_t callback)
{
    BTIF_TRACE_EVENT("## %s ##", __FUNCTION__);
    tls_bt_status_t result = TLS_BT_STATUS_SUCCESS;

    if(bt_rc_ctrl_callbacks) {
        return TLS_BT_STATUS_DONE;
    }

    bt_rc_ctrl_callbacks = callback;
    wm_memset(&btif_rc_cb, 0, sizeof(btif_rc_cb));
    btif_rc_cb.rc_vol_label = MAX_LABEL;
    btif_rc_cb.rc_volume = MAX_VOLUME;
    lbl_init();
    return result;
}

tls_bt_status_t tls_btrc_ctrl_deinit(void)
{
    BTIF_TRACE_EVENT("## %s ##", __FUNCTION__);

    if(bt_rc_ctrl_callbacks) {
        bt_rc_ctrl_callbacks = NULL;
    }

#ifdef USE_ALARM
    alarm_free(btif_rc_cb.rc_play_status_timer);
#endif
    wm_memset(&btif_rc_cb, 0, sizeof(btif_rc_cb_t));
    lbl_destroy();
    BTIF_TRACE_EVENT("## %s ## completed", __FUNCTION__);
	return TLS_BT_STATUS_SUCCESS;
}

tls_bt_status_t tls_btrc_ctrl_send_passthrough_cmd(tls_bt_addr_t *bd_addr, uint8_t key_code,
        uint8_t key_state)
{
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
#if (AVRC_CTLR_INCLUDED == TRUE)
    CHECK_RC_CONNECTED
    rc_transaction_t *p_transaction = NULL;
    BTIF_TRACE_DEBUG("%s: key-code: %d, key-state: %d", __FUNCTION__,
                     key_code, key_state);

    if(btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG) {
        tls_bt_status_t tran_status = get_transaction(&p_transaction);

        if(TLS_BT_STATUS_SUCCESS == tran_status && NULL != p_transaction) {
            BTA_AvRemoteCmd(btif_rc_cb.rc_handle, p_transaction->lbl,
                            (tBTA_AV_RC)key_code, (tBTA_AV_STATE)key_state);
            status =  TLS_BT_STATUS_SUCCESS;
            BTIF_TRACE_DEBUG("%s: succesfully sent passthrough command to BTA", __FUNCTION__);
        } else {
            status =  TLS_BT_STATUS_FAIL;
            BTIF_TRACE_DEBUG("%s: error in fetching transaction", __FUNCTION__);
        }
    } else {
        status =  TLS_BT_STATUS_FAIL;
        BTIF_TRACE_DEBUG("%s: feature not supported", __FUNCTION__);
    }

#else
    BTIF_TRACE_DEBUG("%s: feature not enabled", __FUNCTION__);
#endif
    return status;
}



tls_bt_status_t tls_btrc_ctrl_send_groupnavigation_cmd(tls_bt_addr_t *bd_addr, uint8_t key_code,
        uint8_t key_state)
{
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
#if (AVRC_CTLR_INCLUDED == TRUE)
    rc_transaction_t *p_transaction = NULL;
    BTIF_TRACE_DEBUG("%s: key-code: %d, key-state: %d", __FUNCTION__,
                     key_code, key_state);
    CHECK_RC_CONNECTED

    if(btif_rc_cb.rc_features & BTA_AV_FEAT_RCTG) {
        tls_bt_status_t tran_status = get_transaction(&p_transaction);

        if((TLS_BT_STATUS_SUCCESS == tran_status) && (NULL != p_transaction)) {
            uint8_t buffer[AVRC_PASS_THRU_GROUP_LEN] = {0};
            uint8_t *start = buffer;
            UINT24_TO_BE_STREAM(start, AVRC_CO_METADATA);
            *(start)++ = 0;
            UINT8_TO_BE_STREAM(start, key_code);
            BTA_AvRemoteVendorUniqueCmd(btif_rc_cb.rc_handle,
                                        p_transaction->lbl,
                                        (tBTA_AV_STATE)key_state, buffer,
                                        AVRC_PASS_THRU_GROUP_LEN);
            status =  TLS_BT_STATUS_SUCCESS;
            BTIF_TRACE_DEBUG("%s: succesfully sent group_navigation command to BTA",
                             __FUNCTION__);
        } else {
            status =  TLS_BT_STATUS_FAIL;
            BTIF_TRACE_DEBUG("%s: error in fetching transaction", __FUNCTION__);
        }
    } else {
        status =  TLS_BT_STATUS_FAIL;
        BTIF_TRACE_DEBUG("%s: feature not supported", __FUNCTION__);
    }

#else
    BTIF_TRACE_DEBUG("%s: feature not enabled", __FUNCTION__);
#endif
    return status;
}


tls_bt_status_t tls_btrc_ctrl_change_player_app_setting(tls_bt_addr_t *bd_addr, uint8_t num_attrib,
        uint8_t *attrib_ids, uint8_t *attrib_vals)
{
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
    rc_transaction_t *p_transaction = NULL;
    int count  = 0;
#if (AVRC_CTLR_INCLUDED == TRUE)
    BTIF_TRACE_DEBUG("%s: num attrib_id %d", __FUNCTION__, num_attrib);
    CHECK_RC_CONNECTED
    tls_bt_status_t tran_status = get_transaction(&p_transaction);

    if(TLS_BT_STATUS_SUCCESS != tran_status) {
        return TLS_BT_STATUS_FAIL;
    }

    tAVRC_COMMAND avrc_cmd = {0};
    BT_HDR *p_msg = NULL;
    avrc_cmd.set_app_val.opcode = AVRC_OP_VENDOR;
    avrc_cmd.set_app_val.status = AVRC_STS_NO_ERROR;
    avrc_cmd.set_app_val.num_val = num_attrib;
    avrc_cmd.set_app_val.pdu = AVRC_PDU_SET_PLAYER_APP_VALUE;
    avrc_cmd.set_app_val.p_vals =
                    (tAVRC_APP_SETTING *)GKI_getbuf(sizeof(tAVRC_APP_SETTING) * num_attrib);

    for(count = 0; count < num_attrib; count++) {
        avrc_cmd.set_app_val.p_vals[count].attr_id = attrib_ids[count];
        avrc_cmd.set_app_val.p_vals[count].attr_val = attrib_vals[count];
    }

    status = AVRC_BldCommand(&avrc_cmd, &p_msg);

    if((status == AVRC_STS_NO_ERROR) && (p_msg != NULL)) {
        uint8_t *data_start = (uint8_t *)(p_msg + 1) + p_msg->offset;
        BTIF_TRACE_DEBUG("%s msgreq being sent out with label %d",
                         __FUNCTION__, p_transaction->lbl);
        BTA_AvVendorCmd(btif_rc_cb.rc_handle, p_transaction->lbl, AVRC_CMD_CTRL,
                        data_start, p_msg->len);
        status =  TLS_BT_STATUS_SUCCESS;
        start_control_command_timer(AVRC_PDU_SET_PLAYER_APP_VALUE, p_transaction);
    } else {
        BTIF_TRACE_ERROR("%s: failed to build command. status: 0x%02x",
                         __FUNCTION__, status);
    }

    GKI_freebuf(p_msg);
    GKI_free_and_reset_buf((void **)&avrc_cmd.set_app_val.p_vals);
#else
    BTIF_TRACE_DEBUG("%s: feature not enabled", __FUNCTION__);
#endif
    return status;
}

tls_bt_status_t tls_btrc_ctrl_set_volume_rsp(tls_bt_addr_t *bd_addr, uint8_t abs_vol, uint8_t label)
{
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
#if (AVRC_CTLR_INCLUDED == TRUE)
    tAVRC_RESPONSE avrc_rsp;
    BT_HDR *p_msg = NULL;
    CHECK_RC_CONNECTED
    BTIF_TRACE_DEBUG("%s: abs_vol %d", __FUNCTION__, abs_vol);
    avrc_rsp.volume.opcode = AVRC_OP_VENDOR;
    avrc_rsp.volume.pdu = AVRC_PDU_SET_ABSOLUTE_VOLUME;
    avrc_rsp.volume.status = AVRC_STS_NO_ERROR;
    avrc_rsp.volume.volume = abs_vol;
    status = AVRC_BldResponse(btif_rc_cb.rc_handle, &avrc_rsp, &p_msg);

    if(status == AVRC_STS_NO_ERROR) {
        uint8_t *data_start = (uint8_t *)(p_msg + 1) + p_msg->offset;
        BTIF_TRACE_DEBUG("%s msgreq being sent out with label %d",
                         __FUNCTION__, btif_rc_cb.rc_vol_label);

        if(p_msg != NULL) {
            BTA_AvVendorRsp(btif_rc_cb.rc_handle, label,
                            BTA_AV_RSP_ACCEPT, data_start, p_msg->len, 0);
            status =  TLS_BT_STATUS_SUCCESS;
        }
    } else {
        BTIF_TRACE_ERROR("%s: failed to build command. status: 0x%02x",
                         __FUNCTION__, status);
    }

    GKI_freebuf(p_msg);
#else
    BTIF_TRACE_DEBUG("%s: feature not enabled", __FUNCTION__);
#endif
    return status;
}

tls_bt_status_t tls_btrc_ctrl_volume_change_notification_rsp(tls_bt_addr_t *bd_addr,
        btrc_notification_type_t rsp_type,
        uint8_t abs_vol, uint8_t label)
{
    tAVRC_STS status = TLS_BT_STATUS_UNSUPPORTED;
    tAVRC_RESPONSE avrc_rsp;
    BT_HDR *p_msg = NULL;
#if (AVRC_CTLR_INCLUDED == TRUE)
    BTIF_TRACE_DEBUG("%s: rsp_type	%d abs_vol %d", __func__, rsp_type, abs_vol);
    CHECK_RC_CONNECTED
    avrc_rsp.reg_notif.opcode = AVRC_OP_VENDOR;
    avrc_rsp.reg_notif.pdu = AVRC_PDU_REGISTER_NOTIFICATION;
    avrc_rsp.reg_notif.status = AVRC_STS_NO_ERROR;
    avrc_rsp.reg_notif.param.volume = abs_vol;
    avrc_rsp.reg_notif.event_id = AVRC_EVT_VOLUME_CHANGE;
    status = AVRC_BldResponse(btif_rc_cb.rc_handle, &avrc_rsp, &p_msg);

    if(status == AVRC_STS_NO_ERROR) {
        BTIF_TRACE_DEBUG("%s msgreq being sent out with label %d",
                         __func__, label);
        uint8_t *data_start = (uint8_t *)(p_msg + 1) + p_msg->offset;
        BTA_AvVendorRsp(btif_rc_cb.rc_handle, label,
                        (rsp_type == BTRC_NOTIFICATION_TYPE_INTERIM) ?
                        AVRC_RSP_INTERIM : AVRC_RSP_CHANGED,
                        data_start, p_msg->len, 0);
        status = TLS_BT_STATUS_SUCCESS;
    } else {
        BTIF_TRACE_ERROR("%s: failed to build command. status: 0x%02x",
                         __func__, status);
    }

    GKI_freebuf(p_msg);
#else
    BTIF_TRACE_DEBUG("%s: feature not enabled", __func__);
#endif
    return status;
}
#endif
