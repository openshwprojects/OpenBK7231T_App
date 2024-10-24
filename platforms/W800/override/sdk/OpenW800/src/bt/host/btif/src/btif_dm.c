/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
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

/************************************************************************************
 *
 *  Filename:      btif_dm.c
 *
 *  Description:   Contains Device Management (DM) related functionality
 *
 *
 ***********************************************************************************/

#define LOG_TAG "bt_btif_dm"

#include "btif_dm.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "bluetooth.h"

#include "bdaddr.h"
#include "bta_gatt_api.h"
#include "btif_api.h"
#include "btif_config.h"
#include "btif_hh.h"
#include "btif_sdp.h"
#include "btif_storage.h"
#include "btif_util.h"
#include "btu.h"
#include "bt_common.h"
#include "bta_gatt_api.h"
#include "btm_int.h"

extern void BTA_DmDiscoverByTransport(BD_ADDR bd_addr, tBTA_SERVICE_MASK_EXT *p_services,
                               tBTA_DM_SEARCH_CBACK *p_cback, uint8_t sdp_search,
                               tBTA_TRANSPORT transport);
extern void  hci_dbg_hexstring(const char *msg, const uint8_t *ptr, int a_length);

/**
 * The devices below have proven problematic during the pairing process, often
 * requiring multiple retries to complete pairing. To avoid degrading the user
 * experience for other devices, explicitely blacklist troubled devices here.
 */
static const uint8_t blacklist_pairing_retries[][3] = {
    {0x9C, 0xDF, 0x03} // BMW car kits (Harman/Becker)
};

uint8_t blacklistPairingRetries(BD_ADDR bd_addr)
{
    int i;
    const unsigned blacklist_size = sizeof(blacklist_pairing_retries)
                                    / sizeof(blacklist_pairing_retries[0]);

    for(i = 0; i != blacklist_size; ++i) {
        if(blacklist_pairing_retries[i][0] == bd_addr[0] &&
                blacklist_pairing_retries[i][1] == bd_addr[1] &&
                blacklist_pairing_retries[i][2] == bd_addr[2]) {
            return TRUE;
        }
    }

    return FALSE;
}
/******************************************************************************
**  Constants & Macros
******************************************************************************/

#define COD_MASK                            0x07FF

#define COD_UNCLASSIFIED ((0x1F) << 8)
#define COD_HID_KEYBOARD                    0x0540
#define COD_HID_POINTING                    0x0580
#define COD_HID_COMBO                       0x05C0
#define COD_HID_MAJOR                       0x0500
#define COD_HID_MASK                        0x0700
#define COD_AV_HEADSETS                     0x0404
#define COD_AV_HANDSFREE                    0x0408
#define COD_AV_HEADPHONES                   0x0418
#define COD_AV_PORTABLE_AUDIO               0x041C
#define COD_AV_HIFI_AUDIO                   0x0428

#define BTIF_DM_DEFAULT_INQ_MAX_RESULTS     0
#define BTIF_DM_DEFAULT_INQ_MAX_DURATION    5
#define BTIF_DM_MAX_SDP_ATTEMPTS_AFTER_PAIRING 2

#define NUM_TIMEOUT_RETRIES                 5

#define PROPERTY_PRODUCT_MODEL "ro.product.model"
#define DEFAULT_LOCAL_NAME_MAX  31
#if (DEFAULT_LOCAL_NAME_MAX > BTM_MAX_LOC_BD_NAME_LEN)
#error "default btif local name size exceeds stack supported length"
#endif

#if (defined(BTA_HOST_INTERLEAVE_SEARCH) && BTA_HOST_INTERLEAVE_SEARCH == TRUE)
#define BTIF_DM_INTERLEAVE_DURATION_BR_ONE    2
#define BTIF_DM_INTERLEAVE_DURATION_LE_ONE    2
#define BTIF_DM_INTERLEAVE_DURATION_BR_TWO    3
#define BTIF_DM_INTERLEAVE_DURATION_LE_TWO    4
#endif

#define ENCRYPTED_BREDR       2
#define ENCRYPTED_LE          4

#define PROPERTY_VALUE_MAX 256
typedef struct {
    tls_bt_bond_state_t state;
    tls_bt_addr_t static_bdaddr;
    BD_ADDR bd_addr;
    tBTM_BOND_TYPE bond_type;
    uint8_t pin_code_len;
    uint8_t is_ssp;
    uint8_t auth_req;
    uint8_t io_cap;
    uint8_t autopair_attempts;
    uint8_t timeout_retries;
    uint8_t is_local_initiated;
    uint8_t sdp_attempts;
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
    uint8_t is_le_only;
    uint8_t is_le_nc; /* LE Numeric comparison */
    btif_dm_ble_cb_t ble;
#endif
} btif_dm_pairing_cb_t;

typedef struct {
    uint8_t       ir[BT_OCTET16_LEN];
    uint8_t       irk[BT_OCTET16_LEN];
    uint8_t       dhk[BT_OCTET16_LEN];
} btif_dm_local_key_id_t;

typedef struct {
    uint8_t                 is_er_rcvd;
    uint8_t                   er[BT_OCTET16_LEN];
    uint8_t                 is_id_keys_rcvd;
    btif_dm_local_key_id_t  id_keys;  /* ID kyes */

} btif_dm_local_key_cb_t;

typedef struct {
    BD_ADDR bd_addr;
    BD_NAME bd_name;
} btif_dm_remote_name_t;

/* this structure holds optional OOB data for remote device */
typedef struct {
    BD_ADDR  bdaddr;    /* peer bdaddr */
    bt_out_of_band_data_t oob_data;
} btif_dm_oob_cb_t;

typedef struct {
    tls_bt_addr_t  bdaddr;
    uint8_t        transport; /* 0=Unknown, 1=BR/EDR, 2=LE */
} btif_dm_create_bond_cb_t;

typedef struct {
    uint8_t  status;
    uint8_t  ctrl_state;
    uint64_t tx_time;
    uint64_t rx_time;
    uint64_t idle_time;
    uint64_t energy_used;
} btif_activity_energy_info_cb_t;

typedef struct {
    unsigned int   manufact_id;
} skip_sdp_entry_t;

typedef enum {
    BTIF_DM_FUNC_CREATE_BOND,
    BTIF_DM_FUNC_CANCEL_BOND,
    BTIF_DM_FUNC_REMOVE_BOND,
    BTIF_DM_FUNC_BOND_STATE_CHANGED,
} bt_bond_function_t;

typedef struct {
    tls_bt_addr_t bd_addr;
    bt_bond_function_t function;
    tls_bt_bond_state_t state;
    //struct timespec timestamp;
} btif_bond_event_t;

#define BTA_SERVICE_ID_TO_SERVICE_MASK(id)       (1 << (id))

#define UUID_HUMAN_INTERFACE_DEVICE "00001124-0000-1000-8000-00805f9b34fb"

#define MAX_BTIF_BOND_EVENT_ENTRIES 15

static skip_sdp_entry_t sdp_blacklist[] = {{76}}; //Apple Mouse and Keyboard

/* This flag will be true if HCI_Inquiry is in progress */
static uint8_t btif_dm_inquiry_in_progress = FALSE;

/************************************************************************************
**  Static variables
************************************************************************************/
static char btif_default_local_name[DEFAULT_LOCAL_NAME_MAX + 1] = {'\0'};

#ifdef USE_UID_SET
static uid_set_t *uid_set = NULL;
static pthread_mutex_t bond_event_lock;
#endif


/******************************************************************************
**  Static functions
******************************************************************************/
static btif_dm_pairing_cb_t pairing_cb;
static btif_dm_oob_cb_t     oob_cb;
static void btif_dm_generic_evt(uint16_t event, char *p_param);
static void btif_dm_cb_create_bond(tls_bt_addr_t *bd_addr, tBTA_TRANSPORT transport);
static void btif_dm_cb_hid_remote_name(tBTM_REMOTE_DEV_NAME *p_remote_name);
static void btif_update_remote_properties(BD_ADDR bd_addr, BD_NAME bd_name,
        DEV_CLASS dev_class, tBT_DEVICE_TYPE dev_type);
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
static btif_dm_local_key_cb_t ble_local_key_cb;
static void btif_dm_ble_key_notif_evt(tBTA_DM_SP_KEY_NOTIF *p_ssp_key_notif);
static void btif_dm_ble_auth_cmpl_evt(tBTA_DM_AUTH_CMPL *p_auth_cmpl);
static void btif_dm_ble_passkey_req_evt(tBTA_DM_PIN_REQ *p_pin_req);
static void btif_dm_ble_key_nc_req_evt(tBTA_DM_SP_KEY_NOTIF *p_notif_req) ;
static void btif_dm_ble_oob_req_evt(tBTA_DM_SP_RMT_OOB *req_oob_type);
#endif
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
static void bte_scan_filt_param_cfg_evt(uint8_t action_type,
                                        tBTA_DM_BLE_PF_AVBL_SPACE avbl_space,
                                        tBTA_DM_BLE_REF_VALUE ref_value,
                                        tBTA_STATUS status);
#endif
static char *btif_get_default_local_name();

static void btif_stats_add_bond_event(const tls_bt_addr_t *bd_addr,
                                      bt_bond_function_t function,
                                      tls_bt_bond_state_t state);

/******************************************************************************
**  Externs
******************************************************************************/
extern uint16_t bta_service_id_to_uuid_lkup_tbl [BTA_MAX_SERVICE_ID];
extern tls_bt_status_t btif_hf_execute_service(uint8_t b_enable);
extern tls_bt_status_t btif_av_execute_service(uint8_t b_enable);
extern tls_bt_status_t btif_av_sink_execute_service(uint8_t b_enable);
extern tls_bt_status_t btif_hh_execute_service(uint8_t b_enable);
extern tls_bt_status_t btif_hf_client_execute_service(uint8_t b_enable);
extern tls_bt_status_t btif_sdp_execute_service(uint8_t b_enable);
extern int btif_hh_connect(tls_bt_addr_t *bd_addr);
extern void bta_gatt_convert_uuid16_to_uuid128(uint8_t uuid_128[LEN_UUID_128], uint16_t uuid_16);

/******************************************************************************
**  Functions
******************************************************************************/

static uint8_t is_empty_128bit(uint8_t *data)
{
    static const uint8_t zero[16] = { 0 };
    return !memcmp(zero, data, sizeof(zero));
}

static void btif_dm_data_copy(uint16_t event, char *dst, char *src)
{
    tBTA_DM_SEC *dst_dm_sec = (tBTA_DM_SEC *)dst;
    tBTA_DM_SEC *src_dm_sec = (tBTA_DM_SEC *)src;

    if(!src_dm_sec) {
        return;
    }

    assert(dst_dm_sec);
    maybe_non_aligned_memcpy(dst_dm_sec, src_dm_sec, sizeof(*src_dm_sec));

    if(event == BTA_DM_BLE_KEY_EVT) {
        dst_dm_sec->ble_key.p_key_value = GKI_getbuf(sizeof(tBTM_LE_KEY_VALUE));
        assert(src_dm_sec->ble_key.p_key_value);
        wm_memcpy(dst_dm_sec->ble_key.p_key_value, src_dm_sec->ble_key.p_key_value,
                  sizeof(tBTM_LE_KEY_VALUE));
    }
}

static void btif_dm_data_free(uint16_t event, tBTA_DM_SEC *dm_sec)
{
    if(event == BTA_DM_BLE_KEY_EVT) {
        GKI_free_and_reset_buf((void **)&dm_sec->ble_key.p_key_value);
    }
}

#ifdef USE_UID_SET
void btif_dm_init(uid_set_t *set)
{
    uid_set = set;
    pthread_mutex_init(&bond_event_lock, NULL);
}

void btif_dm_cleanup(void)
{
    if(uid_set) {
        uid_set_destroy(uid_set);
        uid_set = NULL;
    }

    pthread_mutex_destroy(&bond_event_lock);
}
#endif



tls_bt_status_t btif_in_execute_service_request(tBTA_SERVICE_ID service_id,
        uint8_t b_enable)
{
    BTIF_TRACE_DEBUG("%s service_id: %d", __FUNCTION__, service_id);

    /* Check the service_ID and invoke the profile's BT state changed API */
    switch(service_id) {
#if defined(BTA_HFP_HSP_AG_INCLUDED) && (BTA_HFP_HSP_AG_INCLUDED == TRUE)

        case BTA_HSP_SERVICE_ID:
        case BTA_HFP_SERVICE_ID: {
            btif_hf_execute_service(b_enable);
        }
        break;
#endif
#if defined(BTA_AV_INCLUDED) && (BTA_AV_INCLUDED == TRUE)

        case BTA_A2DP_SOURCE_SERVICE_ID: {
            btif_av_execute_service(b_enable);
        }
        break;
#endif
#if defined(BTA_AV_SINK_INCLUDED) && (BTA_AV_SINK_INCLUDED == TRUE)

        case BTA_A2DP_SINK_SERVICE_ID: {
            btif_av_sink_execute_service(b_enable);
        }
        break;
#endif
#if defined(BTA_HH_INCLUDED) && (BTA_HH_INCLUDED == TRUE)

        case BTA_HID_SERVICE_ID: {
            btif_hh_execute_service(b_enable);
        }
        break;
#endif
#if defined(BTA_HFP_HSP_INCLUDED) && (BTA_HFP_HSP_INCLUDED == TRUE)

        case BTA_HFP_HS_SERVICE_ID: {
            btif_hf_client_execute_service(b_enable);
        }
        break;
#endif
#if defined(BTA_SDP_SERVER_INCLUDED) && (BTA_SDP_SERVER_INCLUDED == TRUE)

        case BTA_SDP_SERVICE_ID: {
            btif_sdp_execute_service(b_enable);
        }
        break;
#endif

        default:
            BTIF_TRACE_ERROR("%s: Unknown service %d being %s",
                             __func__, service_id,
                             (b_enable) ? "enabled" : "disabled");
            return TLS_BT_STATUS_FAIL;
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         check_eir_remote_name
**
** Description      Check if remote name is in the EIR data
**
** Returns          TRUE if remote name found
**                  Populate p_remote_name, if provided and remote name found
**
*******************************************************************************/
static uint8_t check_eir_remote_name(tBTA_DM_SEARCH *p_search_data,
                                     uint8_t *p_remote_name, uint8_t *p_remote_name_len)
{
    uint8_t *p_eir_remote_name = NULL;
    uint8_t remote_name_len = 0;

    /* Check EIR for remote name and services */
    if(p_search_data->inq_res.p_eir) {
        p_eir_remote_name = BTM_CheckEirData(p_search_data->inq_res.p_eir,
                                             BTM_EIR_COMPLETE_LOCAL_NAME_TYPE, &remote_name_len);

        if(!p_eir_remote_name) {
            p_eir_remote_name = BTM_CheckEirData(p_search_data->inq_res.p_eir,
                                                 BTM_EIR_SHORTENED_LOCAL_NAME_TYPE, &remote_name_len);
        }

        if(p_eir_remote_name) {
            if(remote_name_len > BD_NAME_LEN) {
                remote_name_len = BD_NAME_LEN;
            }

            if(p_remote_name && p_remote_name_len) {
                wm_memcpy(p_remote_name, p_eir_remote_name, remote_name_len);
                *(p_remote_name + remote_name_len) = 0;
                *p_remote_name_len = remote_name_len;
            }

            return TRUE;
        }
    }

    return FALSE;
}

/*******************************************************************************
**
** Function         check_cached_remote_name
**
** Description      Check if remote name is in the NVRAM cache
**
** Returns          TRUE if remote name found
**                  Populate p_remote_name, if provided and remote name found
**
*******************************************************************************/
static uint8_t check_cached_remote_name(tBTA_DM_SEARCH *p_search_data,
                                        uint8_t *p_remote_name, uint8_t *p_remote_name_len)
{
    tls_bt_bdname_t bdname;
    tls_bt_addr_t remote_bdaddr;
    tls_bt_property_t prop_name;
    /* check if we already have it in our btif_storage cache */
    bdcpy(remote_bdaddr.address, p_search_data->inq_res.bd_addr);
    BTIF_STORAGE_FILL_PROPERTY(&prop_name, WM_BT_PROPERTY_BDNAME,
                               sizeof(tls_bt_bdname_t), &bdname);

    if(btif_storage_get_remote_device_property(
                            &remote_bdaddr, &prop_name) == TLS_BT_STATUS_SUCCESS) {
        if(p_remote_name && p_remote_name_len) {
            strcpy((char *)p_remote_name, (char *)bdname.name);
            *p_remote_name_len = strlen((char *)p_remote_name);
        }

        return TRUE;
    }

    return FALSE;
}

static uint32_t get_cod(const tls_bt_addr_t *remote_bdaddr)
{
    uint32_t    remote_cod;
    tls_bt_property_t prop_name;
    /* check if we already have it in our btif_storage cache */
    BTIF_STORAGE_FILL_PROPERTY(&prop_name, WM_BT_PROPERTY_CLASS_OF_DEVICE,
                               sizeof(uint32_t), &remote_cod);

    if(btif_storage_get_remote_device_property((tls_bt_addr_t *)remote_bdaddr,
            &prop_name) == TLS_BT_STATUS_SUCCESS) {
        LOG_INFO(LOG_TAG, "%s remote_cod = 0x%08x", __func__, remote_cod);
        return remote_cod & COD_MASK;
    }

    return 0;
}

uint8_t check_cod(const tls_bt_addr_t *remote_bdaddr, uint32_t cod)
{
    return get_cod(remote_bdaddr) == cod;
}

uint8_t check_cod_hid(const tls_bt_addr_t *remote_bdaddr)
{
    return (get_cod(remote_bdaddr) & COD_HID_MASK) == COD_HID_MAJOR;
}

uint8_t check_hid_le(const tls_bt_addr_t *remote_bdaddr)
{
    uint32_t    remote_dev_type;
    tls_bt_property_t prop_name;
    /* check if we already have it in our btif_storage cache */
    BTIF_STORAGE_FILL_PROPERTY(&prop_name, WM_BT_PROPERTY_TYPE_OF_DEVICE,
                               sizeof(uint32_t), &remote_dev_type);

    if(btif_storage_get_remote_device_property((tls_bt_addr_t *)remote_bdaddr,
            &prop_name) == TLS_BT_STATUS_SUCCESS) {
        if(remote_dev_type == BT_DEVICE_DEVTYPE_BLE) {
            bdstr_t bdstr;
            bdaddr_to_string(remote_bdaddr, bdstr, sizeof(bdstr));

            if(btif_config_exist("Remote", bdstr, "HidAppId")) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

/*****************************************************************************
**
** Function        check_sdp_bl
**
** Description     Checks if a given device is blacklisted to skip sdp
**
** Parameters     skip_sdp_entry
**
** Returns         TRUE if the device is present in blacklist, else FALSE
**
*******************************************************************************/
uint8_t check_sdp_bl(const tls_bt_addr_t *remote_bdaddr)
{
    uint16_t manufacturer = 0;
    uint8_t lmp_ver = 0;
    uint16_t lmp_subver = 0;
    tls_bt_property_t prop_name;
    bt_remote_version_t info;

    if(remote_bdaddr == NULL) {
        return FALSE;
    }

    /* fetch additional info about remote device used in iop query */
    BTM_ReadRemoteVersion(*(BD_ADDR *)remote_bdaddr, &lmp_ver,
                          &manufacturer, &lmp_subver);
    /* if not available yet, try fetching from config database */
    BTIF_STORAGE_FILL_PROPERTY(&prop_name, WM_BT_PROPERTY_REMOTE_VERSION_INFO,
                               sizeof(bt_remote_version_t), &info);

    if(btif_storage_get_remote_device_property((tls_bt_addr_t *)remote_bdaddr,
            &prop_name) != TLS_BT_STATUS_SUCCESS) {
        return FALSE;
    }

    manufacturer = info.manufacturer;

    for(unsigned int i = 0; i < ARRAY_SIZE(sdp_blacklist); i++) {
        if(manufacturer == sdp_blacklist[i].manufact_id) {
            return TRUE;
        }
    }

    return FALSE;
}

static void bond_state_changed(tls_bt_status_t status, tls_bt_addr_t *bd_addr,
                               tls_bt_bond_state_t state)
{
    tls_bt_host_msg_t msg;
    btif_stats_add_bond_event(bd_addr, BTIF_DM_FUNC_BOND_STATE_CHANGED, state);

    // Send bonding state only once - based on outgoing/incoming we may receive duplicates
    if((pairing_cb.state == state) && (state == WM_BT_BOND_STATE_BONDING)) {
        // Cross key pairing so send callback for static address
        if(!bdaddr_is_empty(&pairing_cb.static_bdaddr)) {
            msg.bond_state.remote_bd_addr = bd_addr;
            msg.bond_state.state = state;
            msg.bond_state.status = status;

            if(bt_host_cb) {
                bt_host_cb(WM_BT_BOND_STATE_CHG_EVT, &msg);
            }
        }

        return;
    }

    if(pairing_cb.bond_type == BOND_TYPE_TEMPORARY) {
        state = WM_BT_BOND_STATE_NONE;
    }

    BTIF_TRACE_DEBUG("%s: state=%d, prev_state=%d, sdp_attempts = %d", __func__,
                     state, pairing_cb.state, pairing_cb.sdp_attempts);
    msg.bond_state.remote_bd_addr = bd_addr;
    msg.bond_state.state = state;
    msg.bond_state.status = status;

    if(bt_host_cb) {
        bt_host_cb(WM_BT_BOND_STATE_CHG_EVT, &msg);
    }

    if(state == WM_BT_BOND_STATE_BONDING) {
        pairing_cb.state = state;
        bdcpy(pairing_cb.bd_addr, bd_addr->address);
    } else {
        if(!pairing_cb.sdp_attempts) {
            wm_memset(&pairing_cb, 0, sizeof(pairing_cb));
        } else {
            BTIF_TRACE_DEBUG("%s: BR-EDR service discovery active", __func__);
        }
    }
}

/* store remote version in bt config to always have access
   to it post pairing*/
static void btif_update_remote_version_property(tls_bt_addr_t *p_bd)
{
    tls_bt_property_t property;
    uint8_t lmp_ver = 0;
    uint16_t lmp_subver = 0;
    uint16_t mfct_set = 0;
    tBTM_STATUS btm_status;
    bt_remote_version_t info;
    tls_bt_status_t status;
   // bdstr_t bdstr;
    btm_status = BTM_ReadRemoteVersion(*(BD_ADDR *)p_bd, &lmp_ver,
                                       &mfct_set, &lmp_subver);
#if 0   
    LOG_DEBUG(LOG_TAG, "remote version info [%s]: %x, %x, %x", bdaddr_to_string(p_bd, bdstr,
              sizeof(bdstr)),
              lmp_ver, mfct_set, lmp_subver);
#endif
    if(btm_status == BTM_SUCCESS) {
        // Always update cache to ensure we have availability whenever BTM API is not populated
        info.manufacturer = mfct_set;
        info.sub_ver = lmp_subver;
        info.version = lmp_ver;
        BTIF_STORAGE_FILL_PROPERTY(&property,
                                   WM_BT_PROPERTY_REMOTE_VERSION_INFO, sizeof(bt_remote_version_t),
                                   &info);
        status = btif_storage_set_remote_device_property(p_bd, &property);
        ASSERTC(status == TLS_BT_STATUS_SUCCESS, "failed to save remote version", status);
    }
}

static void btif_update_remote_properties(BD_ADDR bd_addr, BD_NAME bd_name,
        DEV_CLASS dev_class, tBT_DEVICE_TYPE device_type)
{
    tls_bt_host_msg_t msg;
    int num_properties = 0;
    tls_bt_property_t properties[3];
    tls_bt_addr_t bdaddr;
    tls_bt_status_t status;
    uint32_t cod;
    bt_device_type_t dev_type;
    wm_memset(properties, 0, sizeof(properties));
    bdcpy(bdaddr.address, bd_addr);

    /* remote name */
    if(strlen((const char *) bd_name)) {
        BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties], WM_BT_PROPERTY_BDNAME,
                                   strlen((char *)bd_name), bd_name);
        status = btif_storage_set_remote_device_property(&bdaddr, &properties[num_properties]);
        ASSERTC(status == TLS_BT_STATUS_SUCCESS, "failed to save remote device name", status);
        num_properties++;
    }

    /* class of device */
    cod = devclass2uint(dev_class);
    BTIF_TRACE_DEBUG("%s cod is 0x%06x", __func__, cod);

    if(cod == 0) {
        /* Try to retrieve cod from storage */
        BTIF_TRACE_DEBUG("%s cod is 0, checking cod from storage", __func__);
        BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties], WM_BT_PROPERTY_CLASS_OF_DEVICE, sizeof(cod),
                                   &cod);
        status = btif_storage_get_remote_device_property(&bdaddr, &properties[num_properties]);
        BTIF_TRACE_DEBUG("%s cod retrieved from storage is 0x%06x", __func__, cod);

        if(cod == 0) {
            BTIF_TRACE_DEBUG("%s cod is again 0, set as unclassified", __func__);
            cod = COD_UNCLASSIFIED;
        }
    }

    BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties], WM_BT_PROPERTY_CLASS_OF_DEVICE, sizeof(cod),
                               &cod);
    status = btif_storage_set_remote_device_property(&bdaddr, &properties[num_properties]);
    ASSERTC(status == TLS_BT_STATUS_SUCCESS, "failed to save remote device class", status);
    num_properties++;
    /* device type */
    tls_bt_property_t prop_name;
    uint8_t remote_dev_type;
    BTIF_STORAGE_FILL_PROPERTY(&prop_name, WM_BT_PROPERTY_TYPE_OF_DEVICE, sizeof(uint8_t),
                               &remote_dev_type);

    if(btif_storage_get_remote_device_property(&bdaddr, &prop_name) == TLS_BT_STATUS_SUCCESS) {
        dev_type = remote_dev_type | device_type;
    } else {
        dev_type = device_type;
    }

    BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties], WM_BT_PROPERTY_TYPE_OF_DEVICE,
                               sizeof(dev_type), &dev_type);
    status = btif_storage_set_remote_device_property(&bdaddr, &properties[num_properties]);
    ASSERTC(status == TLS_BT_STATUS_SUCCESS, "failed to save remote device type", status);
    num_properties++;
    msg.remote_device_prop.address = &bdaddr;
    msg.remote_device_prop.num_properties = num_properties;
    msg.remote_device_prop.properties = properties;
    msg.remote_device_prop.status = status;

    if(bt_host_cb) {
        bt_host_cb(WM_BT_RMT_DEVICE_PROP_EVT, &msg);
    }
}

/*******************************************************************************
**
** Function         btif_dm_cb_hid_remote_name
**
** Description      Remote name callback for HID device. Called in btif context
**                  Special handling for HID devices
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_cb_hid_remote_name(tBTM_REMOTE_DEV_NAME *p_remote_name)
{
    BTIF_TRACE_DEBUG("%s: status=%d pairing_cb.state=%d", __FUNCTION__, p_remote_name->status,
                     pairing_cb.state);

    if(pairing_cb.state == WM_BT_BOND_STATE_BONDING) {
        tls_bt_addr_t remote_bd;
        bdcpy(remote_bd.address, pairing_cb.bd_addr);

        if(p_remote_name->status == BTM_SUCCESS) {
            bond_state_changed(TLS_BT_STATUS_SUCCESS, &remote_bd, WM_BT_BOND_STATE_BONDED);
        } else {
            bond_state_changed(TLS_BT_STATUS_FAIL, &remote_bd, WM_BT_BOND_STATE_NONE);
        }
    }
}

/*******************************************************************************
**
** Function         btif_dm_cb_create_bond
**
** Description      Create bond initiated from the BTIF thread context
**                  Special handling for HID devices
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_cb_create_bond(tls_bt_addr_t *bd_addr, tBTA_TRANSPORT transport)
{
#if defined(BTA_HH_INCLUDE)&&(BTA_HH_INCLUDE == TRUE)
    uint8_t is_hid = check_cod(bd_addr, COD_HID_POINTING);
#endif
    bond_state_changed(TLS_BT_STATUS_SUCCESS, bd_addr, WM_BT_BOND_STATE_BONDING);
#if BLE_INCLUDED == TRUE
    int device_type;
    int addr_type;
    bdstr_t bdstr;
    bdaddr_to_string(bd_addr, bdstr, sizeof(bdstr));

    if(transport == BT_TRANSPORT_LE) {
        if(!btif_config_get_int("Remote", (char const *)&bdstr, "DevType", &device_type)) {
            btif_config_set_int("Remote", bdstr, "DevType", BT_DEVICE_TYPE_BLE);
        }

        if(btif_storage_get_remote_addr_type(bd_addr, &addr_type) != TLS_BT_STATUS_SUCCESS) {
            btif_storage_set_remote_addr_type(bd_addr, BLE_ADDR_PUBLIC, 1);
        }
    }

    if((btif_config_get_int("Remote", (char const *)&bdstr, "DevType", &device_type) &&
            (btif_storage_get_remote_addr_type(bd_addr, &addr_type) == TLS_BT_STATUS_SUCCESS) &&
            (device_type & BT_DEVICE_TYPE_BLE) == BT_DEVICE_TYPE_BLE) || (transport == BT_TRANSPORT_LE)) {
        BTA_DmAddBleDevice(bd_addr->address, addr_type, device_type);
    }

#endif
#if defined(BTA_HH_INCLUDE)&&(BTA_HH_INCLUDE == TRUE)
#if BLE_INCLUDED == TRUE

    if(is_hid && (device_type & BT_DEVICE_TYPE_BLE) == 0)
#else
    if(is_hid)
#endif
    {
        int status;
        status = btif_hh_connect(bd_addr);

        if(status != TLS_BT_STATUS_SUCCESS) {
            bond_state_changed(status, bd_addr, WM_BT_BOND_STATE_NONE);
        }
    } else
#endif
    {
        BTA_DmBondByTransport((uint8_t *)bd_addr->address, transport);
    }

    /*  Track  originator of bond creation  */
    pairing_cb.is_local_initiated = TRUE;
}

/*******************************************************************************
**
** Function         btif_dm_cb_remove_bond
**
** Description      remove bond initiated from the BTIF thread context
**                  Special handling for HID devices
**
** Returns          void
**
*******************************************************************************/
void btif_dm_cb_remove_bond(tls_bt_addr_t *bd_addr)
{
    /*special handling for HID devices */
    /*  VUP needs to be sent if its a HID Device. The HID HOST module will check if there
    is a valid hid connection with this bd_addr. If yes VUP will be issued.*/
#if (defined(BTA_HH_INCLUDED) && (BTA_HH_INCLUDED == TRUE))
    if(btif_hh_virtual_unplug(bd_addr) != TLS_BT_STATUS_SUCCESS)
#endif
    {
        BTIF_TRACE_DEBUG("%s: Removing HH device", __func__);
        BTA_DmRemoveDevice((uint8_t *)bd_addr->address);
    }
}

/*******************************************************************************
**
** Function         btif_dm_get_connection_state
**
** Description      Returns whether the remote device is currently connected
**                  and whether encryption is active for the connection
**
** Returns          0 if not connected; 1 if connected and > 1 if connection is
**                  encrypted
**
*******************************************************************************/
uint16_t btif_dm_get_connection_state(const tls_bt_addr_t *bd_addr)
{
    uint8_t *bda = (uint8_t *)bd_addr->address;
    uint16_t rc = BTA_DmGetConnectionState(bda);

    if(rc != 0) {
        uint8_t flags = 0;
        BTM_GetSecurityFlagsByTransport(bda, &flags, BT_TRANSPORT_BR_EDR);
        BTIF_TRACE_DEBUG("%s: security flags (BR/EDR)=0x%02x", __FUNCTION__, flags);

        if(flags & BTM_SEC_FLAG_ENCRYPTED) {
            rc |= ENCRYPTED_BREDR;
        }

        BTM_GetSecurityFlagsByTransport(bda, &flags, BT_TRANSPORT_LE);
        BTIF_TRACE_DEBUG("%s: security flags (LE)=0x%02x", __FUNCTION__, flags);

        if(flags & BTM_SEC_FLAG_ENCRYPTED) {
            rc |= ENCRYPTED_LE;
        }
    }

    return rc;
}

/*******************************************************************************
**
** Function         search_devices_copy_cb
**
** Description      Deep copy callback for search devices event
**
** Returns          void
**
*******************************************************************************/
static void search_devices_copy_cb(uint16_t event, char *p_dest, char *p_src)
{
    tBTA_DM_SEARCH *p_dest_data = (tBTA_DM_SEARCH *) p_dest;
    tBTA_DM_SEARCH *p_src_data = (tBTA_DM_SEARCH *) p_src;

    if(!p_src) {
        return;
    }

    BTIF_TRACE_DEBUG("%s: event=%s", __FUNCTION__, dump_dm_search_event(event));
    maybe_non_aligned_memcpy(p_dest_data, p_src_data, sizeof(*p_src_data));

    switch(event) {
        case BTA_DM_INQ_RES_EVT: {
            if(p_src_data->inq_res.p_eir) {
                p_dest_data->inq_res.p_eir = (uint8_t *)(p_dest + sizeof(tBTA_DM_SEARCH));
                wm_memcpy(p_dest_data->inq_res.p_eir, p_src_data->inq_res.p_eir, HCI_EXT_INQ_RESPONSE_LEN);
            }
        }
        break;

        case BTA_DM_DISC_RES_EVT: {
            if(p_src_data->disc_res.raw_data_size && p_src_data->disc_res.p_raw_data) {
                p_dest_data->disc_res.p_raw_data = (uint8_t *)(p_dest + sizeof(tBTA_DM_SEARCH));
                wm_memcpy(p_dest_data->disc_res.p_raw_data,
                          p_src_data->disc_res.p_raw_data, p_src_data->disc_res.raw_data_size);
            }
        }
        break;
    }
}

static void search_services_copy_cb(uint16_t event, char *p_dest, char *p_src)
{
    tBTA_DM_SEARCH *p_dest_data = (tBTA_DM_SEARCH *) p_dest;
    tBTA_DM_SEARCH *p_src_data = (tBTA_DM_SEARCH *) p_src;

    if(!p_src) {
        return;
    }

    maybe_non_aligned_memcpy(p_dest_data, p_src_data, sizeof(*p_src_data));

    switch(event) {
        case BTA_DM_DISC_RES_EVT: {
            if(p_src_data->disc_res.result == BTA_SUCCESS) {
                if(p_src_data->disc_res.num_uuids > 0) {
                    p_dest_data->disc_res.p_uuid_list =
                                    (uint8_t *)(p_dest + sizeof(tBTA_DM_SEARCH));
                    wm_memcpy(p_dest_data->disc_res.p_uuid_list, p_src_data->disc_res.p_uuid_list,
                              p_src_data->disc_res.num_uuids * MAX_UUID_SIZE);
                    GKI_free_and_reset_buf((void **)&p_src_data->disc_res.p_uuid_list);
                }

                GKI_free_and_reset_buf((void **)&p_src_data->disc_res.p_raw_data);
            }
        }
        break;
    }
}
/******************************************************************************
**
**  BTIF DM callback events
**
*****************************************************************************/

/*******************************************************************************
**
** Function         btif_dm_pin_req_evt
**
** Description      Executes pin request event in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_pin_req_evt(tBTA_DM_PIN_REQ *p_pin_req)
{
    tls_bt_host_msg_t msg;
    tls_bt_addr_t bd_addr;
    tls_bt_bdname_t bd_name;
    uint32_t cod;
    bt_pin_code_t pin_code;
    int dev_type;
    int addr_type = 0;

    /* Remote properties update */
    if(!btif_get_device_type((tls_bt_addr_t *)p_pin_req->bd_addr, &addr_type, &dev_type)) {
        dev_type = BT_DEVICE_TYPE_BREDR;
    }

    btif_update_remote_properties(p_pin_req->bd_addr, p_pin_req->bd_name,
                                  p_pin_req->dev_class, (tBT_DEVICE_TYPE) dev_type);
    bdcpy(bd_addr.address, p_pin_req->bd_addr);
    wm_memcpy(bd_name.name, p_pin_req->bd_name, BD_NAME_LEN);
    bond_state_changed(TLS_BT_STATUS_SUCCESS, &bd_addr, WM_BT_BOND_STATE_BONDING);
    cod = devclass2uint(p_pin_req->dev_class);

    if(cod == 0) {
        BTIF_TRACE_DEBUG("%s cod is 0, set as unclassified", __func__);
        cod = COD_UNCLASSIFIED;
    }

    /* check for auto pair possiblity only if bond was initiated by local device */
    if(pairing_cb.is_local_initiated && (p_pin_req->min_16_digit == FALSE)) {
        if(check_cod(&bd_addr, COD_AV_HEADSETS) ||
                check_cod(&bd_addr, COD_AV_HANDSFREE) ||
                check_cod(&bd_addr, COD_AV_HEADPHONES) ||
                check_cod(&bd_addr, COD_AV_PORTABLE_AUDIO) ||
                check_cod(&bd_addr, COD_AV_HIFI_AUDIO) ||
                check_cod(&bd_addr, COD_HID_POINTING)) {
            BTIF_TRACE_DEBUG("%s()cod matches for auto pair", __FUNCTION__);
            printf("%s()cod matches for auto pair\n", __FUNCTION__);

            /*  Check if this device can be auto paired  */
            if((btif_storage_is_device_autopair_blacklisted(&bd_addr) == FALSE) &&
                    (pairing_cb.autopair_attempts == 0)) {
                BTIF_TRACE_DEBUG("%s() Attempting auto pair", __FUNCTION__);
                pin_code.pin[0] = 0x30;
                pin_code.pin[1] = 0x30;
                pin_code.pin[2] = 0x30;
                pin_code.pin[3] = 0x30;
                pairing_cb.autopair_attempts++;
                BTA_DmPinReply((uint8_t *)bd_addr.address, TRUE, 4, pin_code.pin);
                return;
            }
        } else if(check_cod(&bd_addr, COD_HID_KEYBOARD) ||
                  check_cod(&bd_addr, COD_HID_COMBO)) {
            if((btif_storage_is_fixed_pin_zeros_keyboard(&bd_addr) == TRUE) &&
                    (pairing_cb.autopair_attempts == 0)) {
                BTIF_TRACE_DEBUG("%s() Attempting auto pair", __FUNCTION__);
                pin_code.pin[0] = 0x30;
                pin_code.pin[1] = 0x30;
                pin_code.pin[2] = 0x30;
                pin_code.pin[3] = 0x30;
                pairing_cb.autopair_attempts++;
                BTA_DmPinReply((uint8_t *)bd_addr.address, TRUE, 4, pin_code.pin);
                return;
            }
        }
    }

    msg.pin_request.bd_name = &bd_name;
    msg.pin_request.cod = cod;
    msg.pin_request.min_16_digit = p_pin_req->min_16_digit;
    msg.pin_request.remote_bd_addr = &bd_addr;

    if(bt_host_cb) {
        bt_host_cb(WM_BT_PIN_REQUEST_EVT, &msg);
    }
}

/*******************************************************************************
**
** Function         btif_dm_ssp_cfm_req_evt
**
** Description      Executes SSP confirm request event in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_ssp_cfm_req_evt(tBTA_DM_SP_CFM_REQ *p_ssp_cfm_req)
{
    tls_bt_host_msg_t msg;
    tls_bt_addr_t bd_addr;
    tls_bt_bdname_t bd_name;
    uint32_t cod;
    uint8_t is_incoming = !(pairing_cb.state == WM_BT_BOND_STATE_BONDING);
    int dev_type;
    int addr_type = 0;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    /* Remote properties update */
    if(!btif_get_device_type((tls_bt_addr_t *)p_ssp_cfm_req->bd_addr, &addr_type, &dev_type)) {
        dev_type = BT_DEVICE_TYPE_BREDR;
    }

    btif_update_remote_properties(p_ssp_cfm_req->bd_addr, p_ssp_cfm_req->bd_name,
                                  p_ssp_cfm_req->dev_class, (tBT_DEVICE_TYPE) dev_type);
    bdcpy(bd_addr.address, p_ssp_cfm_req->bd_addr);
    wm_memcpy(bd_name.name, p_ssp_cfm_req->bd_name, BD_NAME_LEN);
    /* Set the pairing_cb based on the local & remote authentication requirements */
    bond_state_changed(TLS_BT_STATUS_SUCCESS, &bd_addr, WM_BT_BOND_STATE_BONDING);

    /* if just_works and bonding bit is not set treat this as temporary */
    if(p_ssp_cfm_req->just_works && !(p_ssp_cfm_req->loc_auth_req & BTM_AUTH_BONDS) &&
            !(p_ssp_cfm_req->rmt_auth_req & BTM_AUTH_BONDS) &&
            !(check_cod((tls_bt_addr_t *)&p_ssp_cfm_req->bd_addr, COD_HID_POINTING))) {
        pairing_cb.bond_type = BOND_TYPE_TEMPORARY;
    } else {
        pairing_cb.bond_type = BOND_TYPE_PERSISTENT;
    }

    btm_set_bond_type_dev(p_ssp_cfm_req->bd_addr, pairing_cb.bond_type);
    pairing_cb.is_ssp = TRUE;

    /* If JustWorks auto-accept */
    if(p_ssp_cfm_req->just_works) {
        /* Pairing consent for JustWorks needed if:
         * 1. Incoming (non-temporary) pairing is detected AND
         * 2. local IO capabilities are DisplayYesNo AND
         * 3. remote IO capabiltiies are DisplayOnly or NoInputNoOutput;
         */
        if(is_incoming && pairing_cb.bond_type != BOND_TYPE_TEMPORARY &&
                ((p_ssp_cfm_req->loc_io_caps == HCI_IO_CAP_DISPLAY_YESNO) &&
                 (p_ssp_cfm_req->rmt_io_caps == HCI_IO_CAP_DISPLAY_ONLY ||
                  p_ssp_cfm_req->rmt_io_caps == HCI_IO_CAP_NO_IO))) {
            BTIF_TRACE_EVENT("%s: User consent needed for incoming pairing request. loc_io_caps: %d, rmt_io_caps: %d",
                             __FUNCTION__, p_ssp_cfm_req->loc_io_caps, p_ssp_cfm_req->rmt_io_caps);
        } else {
            BTIF_TRACE_EVENT("%s: Auto-accept JustWorks pairing", __FUNCTION__);
            btif_dm_ssp_reply(&bd_addr, WM_BT_SSP_VARIANT_CONSENT, TRUE, 0);
            return;
        }
    }

    cod = devclass2uint(p_ssp_cfm_req->dev_class);

    if(cod == 0) {
        LOG_DEBUG(LOG_TAG, "%s cod is 0, set as unclassified", __func__);
        cod = COD_UNCLASSIFIED;
    }

    pairing_cb.sdp_attempts = 0;
    msg.ssp_request.bd_name = &bd_name;
    msg.ssp_request.cod = cod;
    msg.ssp_request.pairing_variant = p_ssp_cfm_req->just_works ? WM_BT_SSP_VARIANT_CONSENT :
                                      WM_BT_SSP_VARIANT_PASSKEY_CONFIRMATION;
    msg.ssp_request.pass_key = p_ssp_cfm_req->num_val;
    msg.ssp_request.remote_bd_addr = &bd_addr;

    if(bt_host_cb) {
        bt_host_cb(WM_BT_SSP_REQUEST_EVT, &msg);
    }
}

static void btif_dm_ssp_key_notif_evt(tBTA_DM_SP_KEY_NOTIF *p_ssp_key_notif)
{
    tls_bt_host_msg_t msg;
    tls_bt_addr_t bd_addr;
    tls_bt_bdname_t bd_name;
    uint32_t cod;
    int dev_type;
    int addr_type = 0;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    /* Remote properties update */
    if(!btif_get_device_type((tls_bt_addr_t *)p_ssp_key_notif->bd_addr, &addr_type, &dev_type)) {
        dev_type = BT_DEVICE_TYPE_BREDR;
    }

    btif_update_remote_properties(p_ssp_key_notif->bd_addr, p_ssp_key_notif->bd_name,
                                  p_ssp_key_notif->dev_class, (tBT_DEVICE_TYPE) dev_type);
    bdcpy(bd_addr.address, p_ssp_key_notif->bd_addr);
    wm_memcpy(bd_name.name, p_ssp_key_notif->bd_name, BD_NAME_LEN);
    bond_state_changed(TLS_BT_STATUS_SUCCESS, &bd_addr, WM_BT_BOND_STATE_BONDING);
    pairing_cb.is_ssp = TRUE;
    cod = devclass2uint(p_ssp_key_notif->dev_class);

    if(cod == 0) {
        LOG_DEBUG(LOG_TAG, "%s cod is 0, set as unclassified", __func__);
        cod = COD_UNCLASSIFIED;
    }

    msg.ssp_request.bd_name = &bd_name;
    msg.ssp_request.cod = cod;
    msg.ssp_request.pairing_variant = WM_BT_SSP_VARIANT_PASSKEY_NOTIFICATION;
    msg.ssp_request.pass_key = p_ssp_key_notif->passkey;
    msg.ssp_request.remote_bd_addr = &bd_addr;

    if(bt_host_cb) {
        bt_host_cb(WM_BT_SSP_REQUEST_EVT, &msg);
    }
}
/*******************************************************************************
**
** Function         btif_dm_auth_cmpl_evt
**
** Description      Executes authentication complete event in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_auth_cmpl_evt(tBTA_DM_AUTH_CMPL *p_auth_cmpl)
{
    /* Save link key, if not temporary */
    tls_bt_host_msg_t msg;
    tls_bt_addr_t bd_addr;
    tls_bt_status_t status = TLS_BT_STATUS_FAIL;
    tls_bt_bond_state_t state = WM_BT_BOND_STATE_NONE;
    uint8_t skip_sdp = FALSE;
    BTIF_TRACE_DEBUG("%s: bond state=%d", __func__, pairing_cb.state);
    bdcpy(bd_addr.address, p_auth_cmpl->bd_addr);
    hci_dbg_hexstring(">>>>>>btif_dm_auth_cmpl_evt, auth_cmpl_bd_addr:", bd_addr.address, 6);
    hci_dbg_hexstring(">>>>>>btif_dm_auth_cmpl_evt, pairing_cb_bd_addr:", pairing_cb.bd_addr, 6);

    if((p_auth_cmpl->success == TRUE) && (p_auth_cmpl->key_present)) {
        if((p_auth_cmpl->key_type < HCI_LKEY_TYPE_DEBUG_COMB) ||
                (p_auth_cmpl->key_type == HCI_LKEY_TYPE_AUTH_COMB) ||
                (p_auth_cmpl->key_type == HCI_LKEY_TYPE_CHANGED_COMB) ||
                (p_auth_cmpl->key_type == HCI_LKEY_TYPE_AUTH_COMB_P_256) ||
                pairing_cb.bond_type == BOND_TYPE_PERSISTENT) {
            tls_bt_status_t ret;
            BTIF_TRACE_DEBUG("%s: Storing link key. key_type=0x%x, bond_type=%d",
                             __FUNCTION__, p_auth_cmpl->key_type, pairing_cb.bond_type);
            ret = btif_storage_add_bonded_device(&bd_addr,
                                                 p_auth_cmpl->key, p_auth_cmpl->key_type,
                                                 pairing_cb.pin_code_len);
            ASSERTC(ret == TLS_BT_STATUS_SUCCESS, "storing link key failed", ret);
        } else {
            BTIF_TRACE_DEBUG("%s: Temporary key. Not storing. key_type=0x%x, bond_type=%d",
                             __FUNCTION__, p_auth_cmpl->key_type, pairing_cb.bond_type);

            if(pairing_cb.bond_type == BOND_TYPE_TEMPORARY) {
                BTIF_TRACE_DEBUG("%s: sending WM_BT_BOND_STATE_NONE for Temp pairing",
                                 __FUNCTION__);
                btif_storage_remove_bonded_device(&bd_addr);
                bond_state_changed(TLS_BT_STATUS_SUCCESS, &bd_addr, WM_BT_BOND_STATE_NONE);
                return;
            }
        }
    }

    // We could have received a new link key without going through the pairing flow.
    // If so, we don't want to perform SDP or any other operations on the authenticated
    // device.
    if(bdcmp(p_auth_cmpl->bd_addr, pairing_cb.bd_addr) != 0) {
        char address[32];
        tls_bt_addr_t bt_bdaddr;
        wm_memcpy(bt_bdaddr.address, p_auth_cmpl->bd_addr,
                  sizeof(bt_bdaddr.address));
        bdaddr_to_string(&bt_bdaddr, address, sizeof(address));
        LOG_INFO(LOG_TAG, "%s skipping SDP since we did not initiate pairing to %s.", __func__, address);
        return;
    }

    // Skip SDP for certain  HID Devices
    if(p_auth_cmpl->success) {
        LOG_INFO(LOG_TAG, "btif_dm_auth_cmpl_evt:...\r\n");
#if BLE_INCLUDED == TRUE
        btif_storage_set_remote_addr_type(&bd_addr, p_auth_cmpl->addr_type, 1);
#endif
        btif_update_remote_properties(p_auth_cmpl->bd_addr,
                                      p_auth_cmpl->bd_name, NULL, p_auth_cmpl->dev_type);
        pairing_cb.timeout_retries = 0;
        status = TLS_BT_STATUS_SUCCESS;
        state = WM_BT_BOND_STATE_BONDED;
        bdcpy(bd_addr.address, p_auth_cmpl->bd_addr);

        if(check_sdp_bl(&bd_addr) && check_cod_hid(&bd_addr)) {
            LOG_WARN(LOG_TAG, "%s:skip SDP", __FUNCTION__);
            skip_sdp = TRUE;
        }

        if(!pairing_cb.is_local_initiated && skip_sdp) {
            bond_state_changed(status, &bd_addr, state);
            LOG_WARN(LOG_TAG, "%s: Incoming HID Connection", __FUNCTION__);
            tls_bt_property_t prop;
            tls_bt_addr_t bd_addr;
            tls_bt_uuid_t  uuid;
            char uuid_str[128] = UUID_HUMAN_INTERFACE_DEVICE;
            string_to_uuid(uuid_str, &uuid);
            prop.type = WM_BT_PROPERTY_UUIDS;
            prop.val = uuid.uu;
            prop.len = MAX_UUID_SIZE;
            /* Send the event to the BTIF */
            msg.remote_device_prop.address = &bd_addr;
            msg.remote_device_prop.num_properties = 1;
            msg.remote_device_prop.properties = &prop;
            msg.remote_device_prop.status = TLS_BT_STATUS_SUCCESS;

            if(bt_host_cb) {
                bt_host_cb(WM_BT_RMT_DEVICE_PROP_EVT, &msg);
            }
        } else {
#if BLE_INCLUDED == TRUE
            uint8_t is_crosskey = FALSE;

            /* If bonded due to cross-key, save the static address too*/
            if(pairing_cb.state == WM_BT_BOND_STATE_BONDING &&
                    (bdcmp(p_auth_cmpl->bd_addr, pairing_cb.bd_addr) != 0)) {
                BTIF_TRACE_DEBUG("%s: bonding initiated due to cross key, adding static address",
                                 __func__);
                bdcpy(pairing_cb.static_bdaddr.address, p_auth_cmpl->bd_addr);
                is_crosskey = TRUE;
            }

            if(!is_crosskey /*|| !(stack_config_get_interface()->get_pts_crosskey_sdp_disable())*/) {
#endif
                // Ensure inquiry is stopped before attempting service discovery
                btif_dm_cancel_discovery();
                /* Trigger SDP on the device */
                pairing_cb.sdp_attempts = 1;
                btif_dm_get_remote_services(&bd_addr);
#if BLE_INCLUDED == TRUE
            }

#endif
        }

        // Do not call bond_state_changed_cb yet. Wait until remote service discovery is complete
    } else {
        // Map the HCI fail reason  to  bt status
        switch(p_auth_cmpl->fail_reason) {
            case HCI_ERR_PAGE_TIMEOUT:
                if(blacklistPairingRetries(bd_addr.address) && pairing_cb.timeout_retries) {
                    BTIF_TRACE_WARNING("%s() - Pairing timeout; retrying (%d) ...", __FUNCTION__,
                                       pairing_cb.timeout_retries);
                    --pairing_cb.timeout_retries;
                    btif_dm_cb_create_bond(&bd_addr, BTA_TRANSPORT_UNKNOWN);
                    return;
                }

            /* Fall-through */
            case HCI_ERR_CONNECTION_TOUT:
                status =  TLS_BT_STATUS_RMT_DEV_DOWN;
                break;

            case HCI_ERR_PAIRING_NOT_ALLOWED:
                status = TLS_BT_STATUS_AUTH_REJECTED;
                break;

            case HCI_ERR_LMP_RESPONSE_TIMEOUT:
                status =  TLS_BT_STATUS_AUTH_FAILURE;
                break;

            /* map the auth failure codes, so we can retry pairing if necessary */
            case HCI_ERR_AUTH_FAILURE:
            case HCI_ERR_KEY_MISSING:
                btif_storage_remove_bonded_device(&bd_addr);

            case HCI_ERR_HOST_REJECT_SECURITY:
            case HCI_ERR_ENCRY_MODE_NOT_ACCEPTABLE:
            case HCI_ERR_UNIT_KEY_USED:
            case HCI_ERR_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED:
            case HCI_ERR_INSUFFCIENT_SECURITY:
            case HCI_ERR_PEER_USER:
            case HCI_ERR_UNSPECIFIED:
                BTIF_TRACE_DEBUG(" %s() Authentication fail reason %d",
                                 __FUNCTION__, p_auth_cmpl->fail_reason);

                if(pairing_cb.autopair_attempts  == 1) {
                    /* Create the Bond once again */
                    BTIF_TRACE_WARNING("%s() auto pair failed. Reinitiate Bond", __FUNCTION__);
                    btif_dm_cb_create_bond(&bd_addr, BTA_TRANSPORT_UNKNOWN);
                    return;
                } else {
                    /* if autopair attempts are more than 1, or not attempted */
                    status =  TLS_BT_STATUS_AUTH_FAILURE;
                }

                break;

            default:
                status =  TLS_BT_STATUS_FAIL;
        }

        /* Special Handling for HID Devices */
        if(check_cod(&bd_addr, COD_HID_POINTING)) {
            /* Remove Device as bonded in nvram as authentication failed */
            BTIF_TRACE_DEBUG("%s(): removing hid pointing device from nvram", __FUNCTION__);
            btif_storage_remove_bonded_device(&bd_addr);
        }

        bond_state_changed(status, &bd_addr, state);
    }
}

/******************************************************************************
**
** Function         btif_dm_search_devices_evt
**
** Description      Executes search devices callback events in btif context
**
** Returns          void
**
******************************************************************************/
static void btif_dm_search_devices_evt(uint16_t event, char *p_param)
{
    tls_bt_host_msg_t msg;
    tBTA_DM_SEARCH *p_search_data;
    BTIF_TRACE_EVENT("%s event=%s", __FUNCTION__, dump_dm_search_event(event));

    switch(event) {
        case BTA_DM_DISC_RES_EVT: {
            p_search_data = (tBTA_DM_SEARCH *)p_param;

            /* Remote name update */
            if(strlen((const char *) p_search_data->disc_res.bd_name)) {
                tls_bt_property_t properties[1];
                tls_bt_addr_t bdaddr;
                tls_bt_status_t status = TLS_BT_STATUS_SUCCESS;
                properties[0].type = WM_BT_PROPERTY_BDNAME;
                properties[0].val = p_search_data->disc_res.bd_name;
                properties[0].len = strlen((char *)p_search_data->disc_res.bd_name);
                bdcpy(bdaddr.address, p_search_data->disc_res.bd_addr);
                //status = btif_storage_set_remote_device_property(&bdaddr, &properties[0]);
                //ASSERTC(status == TLS_BT_STATUS_SUCCESS, "failed to save remote device property", status);
                msg.remote_device_prop.address = &bdaddr;
                msg.remote_device_prop.num_properties = 1;
                msg.remote_device_prop.properties = properties;
                msg.remote_device_prop.status = status;

                if(bt_host_cb) {
                    bt_host_cb(WM_BT_RMT_DEVICE_PROP_EVT, &msg);
                }
            }

            /* TODO: Services? */
        }
        break;

        case BTA_DM_INQ_RES_EVT: {
            /* inquiry result */
            uint32_t cod;
            tls_bt_bdname_t bdname;
            tls_bt_addr_t bdaddr;
            uint8_t remote_name_len;
            tBTA_SERVICE_MASK services = 0;
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))			
            bdstr_t bdstr;
#endif
            p_search_data = (tBTA_DM_SEARCH *)p_param;
            bdcpy(bdaddr.address, p_search_data->inq_res.bd_addr);
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))			
            BTIF_TRACE_DEBUG("%s() %s device_type = 0x%x\n", __FUNCTION__, bdaddr_to_string(&bdaddr, bdstr,
                             sizeof(bdstr)),                           
#if (BLE_INCLUDED == TRUE)
                             p_search_data->inq_res.device_type);
#else
                             BT_DEVICE_TYPE_BREDR);
#endif
#endif

            bdname.name[0] = 0;
            cod = devclass2uint(p_search_data->inq_res.dev_class);

            if(cod == 0) {
                LOG_DEBUG(LOG_TAG, "%s cod is 0, set as unclassified", __func__);
                cod = COD_UNCLASSIFIED;
            }

            if(!check_eir_remote_name(p_search_data, bdname.name, &remote_name_len)) {
                check_cached_remote_name(p_search_data, bdname.name, &remote_name_len);
            }

            /* Check EIR for remote name and services */
            if(p_search_data->inq_res.p_eir) {
                BTA_GetEirService(p_search_data->inq_res.p_eir, &services);
                BTIF_TRACE_DEBUG("%s()EIR BTA services = %08X", __FUNCTION__, (uint32_t)services);
                /* TODO:  Get the service list and check to see which uuids we got and send it back to the client. */
            }

            {
                tls_bt_property_t properties[5];
                bt_device_type_t dev_type;
                uint32_t num_properties = 0;
                //tls_bt_status_t status;
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))                
                int addr_type = 0;
#endif
                wm_memset(properties, 0, sizeof(properties));
                /* BD_ADDR */
                BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties],
                                           WM_BT_PROPERTY_BDADDR, sizeof(bdaddr), &bdaddr);
                num_properties++;

                /* BD_NAME */
                /* Don't send BDNAME if it is empty */
                if(bdname.name[0]) {
                    BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties],
                                               WM_BT_PROPERTY_BDNAME,
                                               strlen((char *)bdname.name), &bdname);
                    num_properties++;
                }

                /* DEV_CLASS */
                BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties],
                                           WM_BT_PROPERTY_CLASS_OF_DEVICE, sizeof(cod), &cod);
                num_properties++;
                /* DEV_TYPE */
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
                /* FixMe: Assumption is that bluetooth.h and BTE enums match */
                /* Verify if the device is dual mode in NVRAM */
                int stored_device_type = 0;

                if(btif_get_device_type(bdaddr.address, &addr_type, &stored_device_type) &&
                        ((stored_device_type != BT_DEVICE_TYPE_BREDR &&
                          p_search_data->inq_res.device_type == BT_DEVICE_TYPE_BREDR) ||
                         (stored_device_type != BT_DEVICE_TYPE_BLE &&
                          p_search_data->inq_res.device_type == BT_DEVICE_TYPE_BLE))) {
                    dev_type = BT_DEVICE_TYPE_DUMO;
                } else {
                    dev_type = p_search_data->inq_res.device_type;
                }

                if(p_search_data->inq_res.device_type == BT_DEVICE_TYPE_BLE) {
                    addr_type = p_search_data->inq_res.ble_addr_type;
                }

#else
                dev_type = BT_DEVICE_TYPE_BREDR;
#endif
                BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties],
                                           WM_BT_PROPERTY_TYPE_OF_DEVICE, sizeof(dev_type), &dev_type);
                num_properties++;
                /* RSSI */
                BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties],
                                           WM_BT_PROPERTY_REMOTE_RSSI, sizeof(int8_t),
                                           &(p_search_data->inq_res.rssi));
                num_properties++;
#if 0
                status = btif_storage_add_remote_device(&bdaddr, num_properties, properties);
                ASSERTC(status == TLS_BT_STATUS_SUCCESS, "failed to save remote device (inquiry)", status);
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
                status = btif_storage_set_remote_addr_type(&bdaddr, addr_type, 1);
                ASSERTC(status == TLS_BT_STATUS_SUCCESS, "failed to save remote addr type (inquiry)", status);
#endif
#endif
                /* Callback to notify upper layer of device */
                msg.device_found.num_properties = num_properties;
                msg.device_found.properties = properties;

                if(bt_host_cb) {
                    bt_host_cb(WM_BT_DEVICE_FOUND_EVT, &msg);
                }
            }
        }
        break;

        case BTA_DM_INQ_CMPL_EVT: {
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
            tBTA_DM_BLE_PF_FILT_PARAMS adv_filt_param;
            wm_memset(&adv_filt_param, 0, sizeof(tBTA_DM_BLE_PF_FILT_PARAMS));
            BTA_DmBleScanFilterSetup(BTA_DM_BLE_SCAN_COND_DELETE, 0, &adv_filt_param, NULL,
                                     bte_scan_filt_param_cfg_evt, 0);
#endif
        }
        break;

        case BTA_DM_DISC_CMPL_EVT: {
            msg.discovery_state.state = WM_BT_DISCOVERY_STOPPED;

            if(bt_host_cb) {
                bt_host_cb(WM_BT_DISCOVERY_STATE_CHG_EVT, &msg);
            }
        }
        break;

        case BTA_DM_SEARCH_CANCEL_CMPL_EVT: {
            /* if inquiry is not in progress and we get a cancel event, then
             * it means we are done with inquiry, but remote_name fetches are in
             * progress
             *
             * if inquiry  is in progress, then we don't want to act on this cancel_cmpl_evt
             * but instead wait for the cancel_cmpl_evt via the Busy Level
             *
             */
            if(btif_dm_inquiry_in_progress == FALSE) {
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
                tBTA_DM_BLE_PF_FILT_PARAMS adv_filt_param;
                wm_memset(&adv_filt_param, 0, sizeof(tBTA_DM_BLE_PF_FILT_PARAMS));
                BTA_DmBleScanFilterSetup(BTA_DM_BLE_SCAN_COND_DELETE, 0, &adv_filt_param, NULL,
                                         bte_scan_filt_param_cfg_evt, 0);
#endif
                msg.discovery_state.state = WM_BT_DISCOVERY_STOPPED;

                if(bt_host_cb) {
                    bt_host_cb(WM_BT_DISCOVERY_STATE_CHG_EVT, &msg);
                }
            }
        }
        break;
    }
}

/*******************************************************************************
**
** Function         btif_dm_search_services_evt
**
** Description      Executes search services event in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_search_services_evt(uint16_t event, char *p_param)
{
    tls_bt_host_msg_t msg;
    tBTA_DM_SEARCH *p_data = (tBTA_DM_SEARCH *)p_param;
    BTIF_TRACE_EVENT("%s:  event = %d", __FUNCTION__, event);

    switch(event) {
        case BTA_DM_DISC_RES_EVT: {
            tls_bt_property_t prop;
            uint32_t i = 0;
            tls_bt_addr_t bd_addr;
            bdcpy(bd_addr.address, p_data->disc_res.bd_addr);
            BTIF_TRACE_DEBUG("%s:(result=0x%x, services 0x%x)", __FUNCTION__,
                             p_data->disc_res.result, p_data->disc_res.services);

            if((p_data->disc_res.result != BTA_SUCCESS) &&
                    (pairing_cb.state == WM_BT_BOND_STATE_BONDING) &&
                    (pairing_cb.sdp_attempts < BTIF_DM_MAX_SDP_ATTEMPTS_AFTER_PAIRING)) {
                BTIF_TRACE_WARNING("%s:SDP failed after bonding re-attempting", __FUNCTION__);
                pairing_cb.sdp_attempts++;
                btif_dm_get_remote_services(&bd_addr);
                return;
            }

            prop.type = WM_BT_PROPERTY_UUIDS;
            prop.len = 0;

            if((p_data->disc_res.result == BTA_SUCCESS) && (p_data->disc_res.num_uuids > 0)) {
                prop.val = p_data->disc_res.p_uuid_list;
                prop.len = p_data->disc_res.num_uuids * MAX_UUID_SIZE;

                for(i = 0; i < p_data->disc_res.num_uuids; i++) {
                    char temp[256];
                    uuid_to_string_legacy((tls_bt_uuid_t *)(p_data->disc_res.p_uuid_list + (i * MAX_UUID_SIZE)), temp);
                    LOG_INFO(LOG_TAG, "%s index:%d uuid:%s", __func__, i, temp);
                }
            }

            /* onUuidChanged requires getBondedDevices to be populated.
            ** bond_state_changed needs to be sent prior to remote_device_property
            */
            if((pairing_cb.state == WM_BT_BOND_STATE_BONDING) &&
                    ((bdcmp(p_data->disc_res.bd_addr, pairing_cb.bd_addr) == 0) ||
                     (bdcmp(p_data->disc_res.bd_addr, pairing_cb.static_bdaddr.address) == 0)) &&
                    pairing_cb.sdp_attempts > 0) {
                BTIF_TRACE_DEBUG("%s Remote Service SDP done. Call bond_state_changed_cb BONDED",
                                 __FUNCTION__);
                pairing_cb.sdp_attempts  = 0;

                // If bonding occured due to cross-key pairing, send bonding callback
                // for static address now
                if(bdcmp(p_data->disc_res.bd_addr, pairing_cb.static_bdaddr.address) == 0) {
                    bond_state_changed(TLS_BT_STATUS_SUCCESS, &bd_addr, WM_BT_BOND_STATE_BONDING);
                }

                bond_state_changed(TLS_BT_STATUS_SUCCESS, &bd_addr, WM_BT_BOND_STATE_BONDED);
            }

            if(p_data->disc_res.num_uuids != 0) {
                /* Also write this to the NVRAM */
                //ret = btif_storage_set_remote_device_property(&bd_addr, &prop);
                //ASSERTC(ret == TLS_BT_STATUS_SUCCESS, "storing remote services failed", ret);
                /* Send the event to the BTIF */
                msg.remote_device_prop.address = &bd_addr;
                msg.remote_device_prop.num_properties = 1;
                msg.remote_device_prop.properties = &prop;
                msg.remote_device_prop.status = TLS_BT_STATUS_SUCCESS;

                if(bt_host_cb) {
                    bt_host_cb(WM_BT_RMT_DEVICE_PROP_EVT, &msg);
                }
            }
        }
        break;

        case BTA_DM_DISC_CMPL_EVT:
            /* fixme */
            break;
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))

        case BTA_DM_DISC_BLE_RES_EVT:
            BTIF_TRACE_DEBUG("%s:, services 0x%x)", __FUNCTION__,
                             p_data->disc_ble_res.service.uu.uuid16);
            tls_bt_uuid_t  uuid;
            int i = 0;
            int j = 15;

            if(p_data->disc_ble_res.service.uu.uuid16 == UUID_SERVCLASS_LE_HID) {
                BTIF_TRACE_DEBUG("%s: Found HOGP UUID", __FUNCTION__);
                tls_bt_property_t prop;
                tls_bt_addr_t bd_addr;
                char temp[256];
                tls_bt_status_t ret;
                bta_gatt_convert_uuid16_to_uuid128(uuid.uu, p_data->disc_ble_res.service.uu.uuid16);

                while(i < j) {
                    unsigned char c = uuid.uu[j];
                    uuid.uu[j] = uuid.uu[i];
                    uuid.uu[i] = c;
                    i++;
                    j--;
                }

                uuid_to_string_legacy(&uuid, temp);
                LOG_INFO(LOG_TAG, "%s uuid:%s", __func__, temp);
                bdcpy(bd_addr.address, p_data->disc_ble_res.bd_addr);
                prop.type = WM_BT_PROPERTY_UUIDS;
                prop.val = uuid.uu;
                prop.len = MAX_UUID_SIZE;
                /* Also write this to the NVRAM */
                //ret = btif_storage_set_remote_device_property(&bd_addr, &prop);
                //ASSERTC(ret == TLS_BT_STATUS_SUCCESS, "storing remote services failed", ret);
                /* Send the event to the BTIF */
                msg.remote_device_prop.address = &bd_addr;
                msg.remote_device_prop.num_properties = 1;
                msg.remote_device_prop.properties = &prop;
                msg.remote_device_prop.status = TLS_BT_STATUS_SUCCESS;

                if(bt_host_cb) {
                    bt_host_cb(WM_BT_RMT_DEVICE_PROP_EVT, &msg);
                }
            }

            break;
#endif /* BLE_INCLUDED */

        default: {
            ASSERTC(0, "unhandled search services event", event);
        }
        break;
    }
}

/*******************************************************************************
**
** Function         btif_dm_remote_service_record_evt
**
** Description      Executes search service record event in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_remote_service_record_evt(uint16_t event, char *p_param)
{
    tls_bt_host_msg_t msg;
    tBTA_DM_SEARCH *p_data = (tBTA_DM_SEARCH *)p_param;
    BTIF_TRACE_EVENT("%s:  event = %d", __FUNCTION__, event);

    switch(event) {
        case BTA_DM_DISC_RES_EVT: {
            bt_service_record_t rec;
            tls_bt_property_t prop;
            tls_bt_addr_t bd_addr;
            wm_memset(&rec, 0, sizeof(bt_service_record_t));
            bdcpy(bd_addr.address, p_data->disc_res.bd_addr);
            BTIF_TRACE_DEBUG("%s:(result=0x%x, services 0x%x)", __FUNCTION__,
                             p_data->disc_res.result, p_data->disc_res.services);
            prop.type = WM_BT_PROPERTY_SERVICE_RECORD;
            prop.val = (void *)&rec;
            prop.len = sizeof(rec);
            /* disc_res.result is overloaded with SCN. Cannot check result */
            p_data->disc_res.services &= ~BTA_USER_SERVICE_MASK;
            /* TODO: Get the UUID as well */
            rec.channel = p_data->disc_res.result - 3;
            /* TODO: Need to get the service name using p_raw_data */
            rec.name[0] = 0;
            msg.remote_device_prop.address = &bd_addr;
            msg.remote_device_prop.num_properties = 1;
            msg.remote_device_prop.properties = &prop;
            msg.remote_device_prop.status = TLS_BT_STATUS_SUCCESS;

            if(bt_host_cb) {
                bt_host_cb(WM_BT_RMT_DEVICE_PROP_EVT, &msg);
            }
        }
        break;

        default: {
            ASSERTC(0, "unhandled remote service record event", event);
        }
        break;
    }
}

/*******************************************************************************
**
** Function         btif_dm_upstreams_cback
**
** Description      Executes UPSTREAMS events in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_upstreams_evt(uint16_t event, char *p_param)
{
    tBTA_DM_SEC *p_data = (tBTA_DM_SEC *)p_param;
    tls_bt_host_msg_t msg;
    tBTA_SERVICE_MASK service_mask;
    uint32_t i;
    tls_bt_addr_t bd_addr;
    BTIF_TRACE_EVENT("btif_dm_upstreams_cback  ev: %s", dump_dm_event(event));

    switch(event) {
        case BTA_DM_ENABLE_EVT: {
            BD_NAME bdname;
            tls_bt_status_t status;
            tls_bt_property_t prop;
            prop.type = WM_BT_PROPERTY_BDNAME;
            prop.len = BD_NAME_LEN;
            prop.val = (void *)bdname;
            status = btif_storage_get_adapter_property(&prop);

            if(status == TLS_BT_STATUS_SUCCESS) {
                /* A name exists in the storage. Make this the device name */
                BTA_DmSetDeviceName((char *)prop.val);
            } else {
                /* Storage does not have a name yet.
                        * Use the default name and write it to the chip
                         */
                BTA_DmSetDeviceName(btif_get_default_local_name());
            }

#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
            /* Enable local privacy */
            //BTA_DmBleConfigLocalPrivacy(BLE_LOCAL_PRIVACY_ENABLED);
#endif
            /* for each of the enabled services in the mask, trigger the profile
             * enable */
            service_mask = btif_get_enabled_services_mask();

            for(i = 0; i <= BTA_MAX_SERVICE_ID; i++) {
                if(service_mask &
                        (tBTA_SERVICE_MASK)(BTA_SERVICE_ID_TO_SERVICE_MASK(i))) {
                    btif_in_execute_service_request(i, TRUE);
                }
            }

            /* clear control blocks */
            wm_memset(&pairing_cb, 0, sizeof(btif_dm_pairing_cb_t));
            pairing_cb.bond_type = BOND_TYPE_PERSISTENT;
            /* This function will also trigger the adapter_properties_cb
            ** and bonded_devices_info_cb
            */
            btif_storage_load_bonded_devices();
            btif_enable_bluetooth_evt(p_data->enable.status, p_data->enable.bd_addr);
        }
        break;

        case BTA_DM_DISABLE_EVT:
            /* for each of the enabled services in the mask, trigger the profile
             * disable */
            service_mask = btif_get_enabled_services_mask();

            for(i = 0; i <= BTA_MAX_SERVICE_ID; i++) {
                if(service_mask &
                        (tBTA_SERVICE_MASK)(BTA_SERVICE_ID_TO_SERVICE_MASK(i))) {
                    btif_in_execute_service_request(i, FALSE);
                }
            }

            btif_disable_bluetooth_evt();
            break;

        case BTA_DM_PIN_REQ_EVT:
            btif_dm_pin_req_evt(&p_data->pin_req);
            break;

        case BTA_DM_AUTH_CMPL_EVT:
            btif_dm_auth_cmpl_evt(&p_data->auth_cmpl);
            break;

        case BTA_DM_BOND_CANCEL_CMPL_EVT:
            if(pairing_cb.state == WM_BT_BOND_STATE_BONDING) {
                bdcpy(bd_addr.address, pairing_cb.bd_addr);
                btm_set_bond_type_dev(pairing_cb.bd_addr, BOND_TYPE_UNKNOWN);
                bond_state_changed(p_data->bond_cancel_cmpl.result, &bd_addr, WM_BT_BOND_STATE_NONE);
            }

            break;

        case BTA_DM_SP_CFM_REQ_EVT:
            btif_dm_ssp_cfm_req_evt(&p_data->cfm_req);
            break;

        case BTA_DM_SP_KEY_NOTIF_EVT:
            btif_dm_ssp_key_notif_evt(&p_data->key_notif);
            break;

        case BTA_DM_DEV_UNPAIRED_EVT:
            bdcpy(bd_addr.address, p_data->link_down.bd_addr);
            btm_set_bond_type_dev(p_data->link_down.bd_addr, BOND_TYPE_UNKNOWN);
            /*special handling for HID devices */
#if (defined(BTA_HH_INCLUDED) && (BTA_HH_INCLUDED == TRUE))
            btif_hh_remove_device(bd_addr);
#endif
            btif_storage_remove_bonded_device(&bd_addr);
            bond_state_changed(TLS_BT_STATUS_SUCCESS, &bd_addr, WM_BT_BOND_STATE_NONE);
            break;

        case BTA_DM_BUSY_LEVEL_EVT: {
            if(p_data->busy_level.level_flags & BTM_BL_INQUIRY_PAGING_MASK) {
                if(p_data->busy_level.level_flags == BTM_BL_INQUIRY_STARTED) {
                    msg.discovery_state.state = WM_BT_DISCOVERY_STARTED;

                    if(bt_host_cb) {
                        bt_host_cb(WM_BT_DISCOVERY_STATE_CHG_EVT, &msg);
                    }

                    btif_dm_inquiry_in_progress = TRUE;
                } else if(p_data->busy_level.level_flags == BTM_BL_INQUIRY_CANCELLED) {
                    msg.discovery_state.state = WM_BT_DISCOVERY_STOPPED;

                    if(bt_host_cb) {
                        bt_host_cb(WM_BT_DISCOVERY_STATE_CHG_EVT, &msg);
                    }

                    btif_dm_inquiry_in_progress = FALSE;
                } else if(p_data->busy_level.level_flags == BTM_BL_INQUIRY_COMPLETE) {
                    btif_dm_inquiry_in_progress = FALSE;
                }
            }
        }
        break;

        case BTA_DM_LINK_UP_EVT:
            bdcpy(bd_addr.address, p_data->link_up.bd_addr);
            BTIF_TRACE_DEBUG("BTA_DM_LINK_UP_EVT. Sending WM_BT_ACL_STATE_CONNECTED");
            btif_update_remote_version_property(&bd_addr);
#if BLE_INCLUDED == TRUE
            msg.acl_state.link_type = p_data->link_up.link_type;
#else
            msg.acl_state.link_type = BT_TRANSPORT_BR_EDR;
#endif
            msg.acl_state.remote_address = &bd_addr;
            msg.acl_state.state = WM_BT_ACL_STATE_CONNECTED;
            msg.acl_state.status = TLS_BT_STATUS_SUCCESS;

            if(bt_host_cb) {
                bt_host_cb(WM_BT_ACL_STATE_CHG_EVT, &msg);
            }

            //resort the remote devices in NVRAM
            btif_storage_update_remote_devices(&bd_addr);
            break;

        case BTA_DM_LINK_DOWN_EVT:
            bdcpy(bd_addr.address, p_data->link_down.bd_addr);
            btm_set_bond_type_dev(p_data->link_down.bd_addr, BOND_TYPE_UNKNOWN);
            BTIF_TRACE_DEBUG("BTA_DM_LINK_DOWN_EVT. Sending WM_BT_ACL_STATE_DISCONNECTED");
#if BLE_INCLUDED == TRUE
            msg.acl_state.link_type = p_data->link_down.link_type;
#else
            msg.acl_state.link_type = BT_TRANSPORT_BR_EDR;
#endif
            msg.acl_state.remote_address = &bd_addr;
            msg.acl_state.state = WM_BT_ACL_STATE_DISCONNECTED;
            msg.acl_state.status = TLS_BT_STATUS_SUCCESS;

            if(bt_host_cb) {
                bt_host_cb(WM_BT_ACL_STATE_CHG_EVT, &msg);
            }

            break;

        case BTA_DM_HW_ERROR_EVT:
            printf("Received H/W Error. !!!!!!!");
#if 0
            /* Flush storage data */
            btif_config_flush();
            usleep(100000);   /* 100milliseconds */
            /* Killing the process to force a restart as part of fault tolerance */
            kill(getpid(), SIGKILL);
#endif
            break;
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))

        case BTA_DM_BLE_KEY_EVT:
            BTIF_TRACE_DEBUG("BTA_DM_BLE_KEY_EVT key_type=0x%02x ", p_data->ble_key.key_type);

            /* If this pairing is by-product of local initiated GATT client Read or Write,
            BTA would not have sent BTA_DM_BLE_SEC_REQ_EVT event and Bond state would not
            have setup properly. Setup pairing_cb and notify App about Bonding state now*/
            if(pairing_cb.state != WM_BT_BOND_STATE_BONDING) {
                BTIF_TRACE_DEBUG("Bond state not sent to App so far.Notify the app now");
                bond_state_changed(TLS_BT_STATUS_SUCCESS, (tls_bt_addr_t *)p_data->ble_key.bd_addr,
                                   WM_BT_BOND_STATE_BONDING);
            } else if(memcmp(pairing_cb.bd_addr, p_data->ble_key.bd_addr, BD_ADDR_LEN) != 0) {
                BTIF_TRACE_ERROR("BD mismatch discard BLE key_type=%d ", p_data->ble_key.key_type);
                break;
            }

            switch(p_data->ble_key.key_type) {
                case BTA_LE_KEY_PENC:
                    //hci_dbg_hexstring("Rcv BTA_LE_KEY_PENC", &p_data->ble_key.p_key_value->penc_key, sizeof(tBTM_LE_PENC_KEYS));
                    pairing_cb.ble.is_penc_key_rcvd = TRUE;
                    pairing_cb.ble.penc_key = p_data->ble_key.p_key_value->penc_key;
                    memcpy(&pairing_cb.ble.penc_key, &p_data->ble_key.p_key_value->penc_key, sizeof(tBTM_LE_PENC_KEYS));
                    break;

                case BTA_LE_KEY_PID:
                    //hci_dbg_hexstring("Rcv BTA_LE_KEY_PID", &p_data->ble_key.p_key_value->pid_key, sizeof(tBTM_LE_PID_KEYS));
                    pairing_cb.ble.is_pid_key_rcvd = TRUE;
                    pairing_cb.ble.pid_key = p_data->ble_key.p_key_value->pid_key;
                    memcpy(&pairing_cb.ble.pid_key, &p_data->ble_key.p_key_value->pid_key, sizeof(tBTM_LE_PID_KEYS));
                    break;

                case BTA_LE_KEY_PCSRK:
                    //hci_dbg_hexstring("Rcv BTA_LE_KEY_PCSRK", &p_data->ble_key.p_key_value->pcsrk_key, sizeof(tBTM_LE_PCSRK_KEYS));
                    pairing_cb.ble.is_pcsrk_key_rcvd = TRUE;
                    pairing_cb.ble.pcsrk_key = p_data->ble_key.p_key_value->pcsrk_key;
                    memcpy(&pairing_cb.ble.pcsrk_key, &p_data->ble_key.p_key_value->pcsrk_key,
                           sizeof(tBTM_LE_PCSRK_KEYS));
                    break;

                case BTA_LE_KEY_LENC:
                    //hci_dbg_hexstring("Rcv BTA_LE_KEY_LENC",&p_data->ble_key.p_key_value->lenc_key, sizeof(tBTM_LE_LENC_KEYS));
                    pairing_cb.ble.is_lenc_key_rcvd = TRUE;
                    pairing_cb.ble.lenc_key = p_data->ble_key.p_key_value->lenc_key;
                    memcpy(&pairing_cb.ble.lenc_key, &p_data->ble_key.p_key_value->lenc_key, sizeof(tBTM_LE_LENC_KEYS));
                    break;

                case BTA_LE_KEY_LCSRK:
                    //hci_dbg_hexstring("Rcv BTA_LE_KEY_LCSRK",&p_data->ble_key.p_key_value->lcsrk_key, sizeof(tBTM_LE_LCSRK_KEYS));
                    pairing_cb.ble.is_lcsrk_key_rcvd = TRUE;
                    pairing_cb.ble.lcsrk_key = p_data->ble_key.p_key_value->lcsrk_key;
                    memcpy(&pairing_cb.ble.lcsrk_key, &p_data->ble_key.p_key_value->lcsrk_key,
                           sizeof(tBTM_LE_LCSRK_KEYS));
                    break;

                case BTA_LE_KEY_LID:
                    //hci_dbg_msg("Rcv BTA_LE_KEY_LID\r\n");
                    pairing_cb.ble.is_lidk_key_rcvd =  TRUE;
                    break;

                default:
                    BTIF_TRACE_ERROR("unknown BLE key type (0x%02x)", p_data->ble_key.key_type);
                    break;
            }

            break;

        case BTA_DM_BLE_SEC_REQ_EVT:
            BTIF_TRACE_DEBUG("BTA_DM_BLE_SEC_REQ_EVT. ");
            btif_dm_ble_sec_req_evt(&p_data->ble_req);
            break;

        case BTA_DM_BLE_PASSKEY_NOTIF_EVT:
            BTIF_TRACE_DEBUG("BTA_DM_BLE_PASSKEY_NOTIF_EVT. ");
            btif_dm_ble_key_notif_evt(&p_data->key_notif);
            break;

        case BTA_DM_BLE_PASSKEY_REQ_EVT:
            BTIF_TRACE_DEBUG("BTA_DM_BLE_PASSKEY_REQ_EVT. ");
            btif_dm_ble_passkey_req_evt(&p_data->pin_req);
            break;

        case BTA_DM_BLE_NC_REQ_EVT:
            BTIF_TRACE_DEBUG("BTA_DM_BLE_PASSKEY_REQ_EVT. ");
            btif_dm_ble_key_nc_req_evt(&p_data->key_notif);
            break;

        case BTA_DM_BLE_OOB_REQ_EVT:
            BTIF_TRACE_DEBUG("BTA_DM_BLE_OOB_REQ_EVT. ");
            btif_dm_ble_oob_req_evt(&p_data->rmt_oob);
            break;

        case BTA_DM_BLE_LOCAL_IR_EVT:
            BTIF_TRACE_DEBUG("BTA_DM_BLE_LOCAL_IR_EVT. ");
            ble_local_key_cb.is_id_keys_rcvd = TRUE;
            wm_memcpy(&ble_local_key_cb.id_keys.irk[0],
                      &p_data->ble_id_keys.irk[0], sizeof(BT_OCTET16));
            wm_memcpy(&ble_local_key_cb.id_keys.ir[0],
                      &p_data->ble_id_keys.ir[0], sizeof(BT_OCTET16));
            wm_memcpy(&ble_local_key_cb.id_keys.dhk[0],
                      &p_data->ble_id_keys.dhk[0], sizeof(BT_OCTET16));
            btif_storage_add_ble_local_key((char *)&ble_local_key_cb.id_keys.irk[0],
                                           BTIF_DM_LE_LOCAL_KEY_IRK,
                                           BT_OCTET16_LEN);
            btif_storage_add_ble_local_key((char *)&ble_local_key_cb.id_keys.ir[0],
                                           BTIF_DM_LE_LOCAL_KEY_IR,
                                           BT_OCTET16_LEN);
            btif_storage_add_ble_local_key((char *)&ble_local_key_cb.id_keys.dhk[0],
                                           BTIF_DM_LE_LOCAL_KEY_DHK,
                                           BT_OCTET16_LEN);
            btif_storage_save();
            break;

        case BTA_DM_BLE_LOCAL_ER_EVT:
            BTIF_TRACE_DEBUG("BTA_DM_BLE_LOCAL_ER_EVT. ");
            ble_local_key_cb.is_er_rcvd = TRUE;
            wm_memcpy(&ble_local_key_cb.er[0], &p_data->ble_er[0], sizeof(BT_OCTET16));
            btif_storage_add_ble_local_key((char *)&ble_local_key_cb.er[0],
                                           BTIF_DM_LE_LOCAL_KEY_ER,
                                           BT_OCTET16_LEN);
            btif_storage_save();
            break;

        case BTA_DM_BLE_AUTH_CMPL_EVT:
            BTIF_TRACE_DEBUG("BTA_DM_BLE_AUTH_CMPL_EVT. ");
            btif_dm_ble_auth_cmpl_evt(&p_data->auth_cmpl);
            break;

        case BTA_DM_LE_FEATURES_READ: {
            tBTM_BLE_VSC_CB cmn_vsc_cb;
            bt_local_le_features_t local_le_features;
            char buf[512];
            tls_bt_property_t prop;
            prop.type = WM_BT_PROPERTY_LOCAL_LE_FEATURES;
            prop.val = (void *)buf;
            prop.len = sizeof(buf);
            /* LE features are not stored in storage. Should be retrived from stack */
            BTM_BleGetVendorCapabilities(&cmn_vsc_cb);
            local_le_features.local_privacy_enabled = BTM_BleLocalPrivacyEnabled();
            prop.len = sizeof(bt_local_le_features_t);

            if(cmn_vsc_cb.filter_support == 1) {
                local_le_features.max_adv_filter_supported = cmn_vsc_cb.max_filter;
            } else {
                local_le_features.max_adv_filter_supported = 0;
            }

            local_le_features.max_adv_instance = cmn_vsc_cb.adv_inst_max;
            local_le_features.max_irk_list_size = cmn_vsc_cb.max_irk_list_sz;
            local_le_features.rpa_offload_supported = cmn_vsc_cb.rpa_offloading;
            local_le_features.activity_energy_info_supported = cmn_vsc_cb.energy_support;
            local_le_features.scan_result_storage_size = cmn_vsc_cb.tot_scan_results_strg;
            local_le_features.version_supported = cmn_vsc_cb.version_supported;
            local_le_features.total_trackable_advertisers =
                            cmn_vsc_cb.total_trackable_advertisers;
            local_le_features.extended_scan_support = cmn_vsc_cb.extended_scan_support > 0;
            local_le_features.debug_logging_supported = cmn_vsc_cb.debug_logging_supported > 0;
            wm_memcpy(prop.val, &local_le_features, prop.len);
            msg.adapter_prop.num_properties = 1;
            msg.adapter_prop.properties = &prop;
            msg.adapter_prop.status = TLS_BT_STATUS_SUCCESS;

            if(bt_host_cb) {
                bt_host_cb(WM_BT_ADAPTER_PROP_CHG_EVT, &msg);
            }

            break;
        }

        case BTA_DM_ENER_INFO_READ: {
            btif_activity_energy_info_cb_t *p_ener_data = (btif_activity_energy_info_cb_t *) p_param;
            tls_bt_activity_energy_info energy_info;
            energy_info.status = p_ener_data->status;
            energy_info.ctrl_state = p_ener_data->ctrl_state;
            energy_info.rx_time = p_ener_data->rx_time;
            energy_info.tx_time = p_ener_data->tx_time;
            energy_info.idle_time = p_ener_data->idle_time;
            energy_info.energy_used = p_ener_data->energy_used;
            msg.energy_info.energy_info = &energy_info;

            if(bt_host_cb) {
                bt_host_cb(WM_BT_ENERGY_INFO_EVT, &msg);
            }

#ifdef USE_UID_SET
            bt_uid_traffic_t *data = uid_set_read_and_clear(uid_set);
            HAL_CBACK(bt_hal_cbacks, energy_info_cb, &energy_info, data);
            GKI_freebuf(data);
#else

            if(bt_host_cb) {
                bt_host_cb(WM_BT_ENERGY_INFO_EVT, &msg);
            }

#endif
            break;
        }

#endif

        case BTA_DM_AUTHORIZE_EVT:
        case BTA_DM_SIG_STRENGTH_EVT:
        case BTA_DM_SP_RMT_OOB_EVT:
        case BTA_DM_SP_KEYPRESS_EVT:
        case BTA_DM_ROLE_CHG_EVT:
        default:
            BTIF_TRACE_WARNING("btif_dm_cback : unhandled event (%d)", event);
            break;
    }

    btif_dm_data_free(event, p_data);
}

/*******************************************************************************
**
** Function         btif_dm_generic_evt
**
** Description      Executes non-BTA upstream events in BTIF context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_generic_evt(uint16_t event, char *p_param)
{
    BTIF_TRACE_EVENT("%s: event=%d", __FUNCTION__, event);
    tls_bt_host_msg_t msg;

    switch(event) {
        case BTIF_DM_CB_DISCOVERY_STARTED: {
            msg.discovery_state.state = WM_BT_DISCOVERY_STARTED;

            if(bt_host_cb) {
                bt_host_cb(WM_BT_DISCOVERY_STATE_CHG_EVT, &msg);
            }
        }
        break;

        case BTIF_DM_CB_CREATE_BOND: {
            pairing_cb.timeout_retries = NUM_TIMEOUT_RETRIES;
            btif_dm_create_bond_cb_t *create_bond_cb = (btif_dm_create_bond_cb_t *)p_param;
            btif_dm_cb_create_bond(&create_bond_cb->bdaddr, create_bond_cb->transport);
        }
        break;

        case BTIF_DM_CB_REMOVE_BOND: {
            btif_dm_cb_remove_bond((tls_bt_addr_t *)p_param);
        }
        break;

        case BTIF_DM_CB_HID_REMOTE_NAME: {
            btif_dm_cb_hid_remote_name((tBTM_REMOTE_DEV_NAME *)p_param);
        }
        break;

        case BTIF_DM_CB_BOND_STATE_BONDING: {
            bond_state_changed(TLS_BT_STATUS_SUCCESS, (tls_bt_addr_t *)p_param, WM_BT_BOND_STATE_BONDING);
        }
        break;

        case BTIF_DM_CB_LE_TX_TEST:
        case BTIF_DM_CB_LE_RX_TEST: {
            uint8_t status;
            STREAM_TO_UINT8(status, p_param);
            msg.ble_test.status = (status == 0) ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
            msg.ble_test.count = 0;

            if(bt_host_cb) {
                bt_host_cb(WM_BT_LE_TEST_EVT, &msg);
            }
        }
        break;

        case BTIF_DM_CB_LE_TEST_END: {
            uint8_t status;
            uint16_t count = 0;
            STREAM_TO_UINT8(status, p_param);

            if(status == 0) {
                STREAM_TO_UINT16(count, p_param);
            }

            msg.ble_test.status = (status == 0) ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
            msg.ble_test.count = count;

            if(bt_host_cb) {
                bt_host_cb(WM_BT_LE_TEST_EVT, &msg);
            }
        }
        break;

        default: {
            BTIF_TRACE_WARNING("%s : Unknown event 0x%x", __FUNCTION__, event);
        }
        break;
    }
}

/*******************************************************************************
**
** Function         bte_dm_evt
**
** Description      Switches context from BTE to BTIF for all DM events
**
** Returns          void
**
*******************************************************************************/

void bte_dm_evt(tBTA_DM_SEC_EVT event, tBTA_DM_SEC *p_data)
{
    /* switch context to btif task context (copy full union size for convenience) */
    tls_bt_status_t status = btif_transfer_context(btif_dm_upstreams_evt, (uint16_t)event,
                             (void *)p_data, sizeof(tBTA_DM_SEC), btif_dm_data_copy);
    /* catch any failed context transfers */
    ASSERTC(status == TLS_BT_STATUS_SUCCESS, "context transfer failed", status);
}

/*******************************************************************************
**
** Function         bte_search_devices_evt
**
** Description      Switches context from BTE to BTIF for DM search events
**
** Returns          void
**
*******************************************************************************/
static void bte_search_devices_evt(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data)
{
    uint16_t param_len = 0;

    if(p_data) {
        param_len += sizeof(tBTA_DM_SEARCH);
    }

    /* Allocate buffer to hold the pointers (deep copy). The pointers will point to the end of the tBTA_DM_SEARCH */
    switch(event) {
        case BTA_DM_INQ_RES_EVT: {
            if(p_data->inq_res.p_eir) {
                param_len += HCI_EXT_INQ_RESPONSE_LEN;
            }
        }
        break;

        case BTA_DM_DISC_RES_EVT: {
            if(p_data->disc_res.raw_data_size && p_data->disc_res.p_raw_data) {
                param_len += p_data->disc_res.raw_data_size;
            }
        }
        break;
    }

    BTIF_TRACE_DEBUG("%s event=%s param_len=%d", __FUNCTION__, dump_dm_search_event(event), param_len);

    /* if remote name is available in EIR, set teh flag so that stack doesnt trigger RNR */
    if(event == BTA_DM_INQ_RES_EVT) {
        p_data->inq_res.remt_name_not_required = check_eir_remote_name(p_data, NULL, NULL);
    }

    btif_transfer_context(btif_dm_search_devices_evt, (uint16_t) event, (void *)p_data, param_len,
                          (param_len > sizeof(tBTA_DM_SEARCH)) ? search_devices_copy_cb : NULL);
}

/*******************************************************************************
**
** Function         bte_dm_search_services_evt
**
** Description      Switches context from BTE to BTIF for DM search services
**                  event
**
** Returns          void
**
*******************************************************************************/
static void bte_dm_search_services_evt(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data)
{
    uint16_t param_len = 0;

    if(p_data) {
        param_len += sizeof(tBTA_DM_SEARCH);
    }

    switch(event) {
        case BTA_DM_DISC_RES_EVT: {
            if((p_data->disc_res.result == BTA_SUCCESS) && (p_data->disc_res.num_uuids > 0)) {
                param_len += (p_data->disc_res.num_uuids * MAX_UUID_SIZE);
            }
        }
        break;
    }

    /* TODO: The only other member that needs a deep copy is the p_raw_data. But not sure
     * if raw_data is needed. */
    btif_transfer_context(btif_dm_search_services_evt, event, (char *)p_data, param_len,
                          (param_len > sizeof(tBTA_DM_SEARCH)) ? search_services_copy_cb : NULL);
}

/*******************************************************************************
**
** Function         bte_dm_remote_service_record_evt
**
** Description      Switches context from BTE to BTIF for DM search service
**                  record event
**
** Returns          void
**
*******************************************************************************/
static void bte_dm_remote_service_record_evt(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data)
{
    /* TODO: The only member that needs a deep copy is the p_raw_data. But not sure yet if this is needed. */
    btif_transfer_context(btif_dm_remote_service_record_evt, event, (char *)p_data,
                          sizeof(tBTA_DM_SEARCH), NULL);
}

#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
/*******************************************************************************
**
** Function         bta_energy_info_cb
**
** Description      Switches context from BTE to BTIF for DM energy info event
**
** Returns          void
**
*******************************************************************************/
static void bta_energy_info_cb(tBTA_DM_BLE_TX_TIME_MS tx_time, tBTA_DM_BLE_RX_TIME_MS rx_time,
                               tBTA_DM_BLE_IDLE_TIME_MS idle_time,
                               tBTA_DM_BLE_ENERGY_USED energy_used,
                               tBTA_DM_CONTRL_STATE ctrl_state, tBTA_STATUS status)
{
    BTIF_TRACE_DEBUG("energy_info_cb-Status:%d,state=%d,tx_t=%ld, rx_t=%ld, idle_time=%ld,used=%ld",
                     status, ctrl_state, tx_time, rx_time, idle_time, energy_used);
    btif_activity_energy_info_cb_t btif_cb;
    btif_cb.status = status;
    btif_cb.ctrl_state = ctrl_state;
    btif_cb.tx_time = (uint64_t) tx_time;
    btif_cb.rx_time = (uint64_t) rx_time;
    btif_cb.idle_time = (uint64_t) idle_time;
    btif_cb.energy_used = (uint64_t) energy_used;
    btif_transfer_context(btif_dm_upstreams_evt, BTA_DM_ENER_INFO_READ,
                          (char *) &btif_cb, sizeof(btif_activity_energy_info_cb_t), NULL);
}
#endif

/*******************************************************************************
**
** Function         bte_scan_filt_param_cfg_evt
**
** Description      Scan filter param config event
**
** Returns          void
**
*******************************************************************************/
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
static void bte_scan_filt_param_cfg_evt(uint8_t action_type,
                                        tBTA_DM_BLE_PF_AVBL_SPACE avbl_space,
                                        tBTA_DM_BLE_REF_VALUE ref_value, tBTA_STATUS status)
{
    /* This event occurs on calling BTA_DmBleCfgFilterCondition internally,
    ** and that is why there is no HAL callback
    */
    if(BTA_SUCCESS != status) {
        BTIF_TRACE_ERROR("%s, %d", __FUNCTION__, status);
    } else {
        BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    }
}
#endif

/*****************************************************************************
**
**   btif api functions (no context switch)
**
*****************************************************************************/

/*******************************************************************************
**
** Function         btif_dm_start_discovery
**
** Description      Start device discovery/inquiry
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_start_discovery(void)
{
    tBTA_DM_INQ inq_params;
    tBTA_SERVICE_MASK services = 0;
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
    tBTA_DM_BLE_PF_FILT_PARAMS adv_filt_param;
#endif
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
    wm_memset(&adv_filt_param, 0, sizeof(tBTA_DM_BLE_PF_FILT_PARAMS));
    /* Cleanup anything remaining on index 0 */
    BTA_DmBleScanFilterSetup(BTA_DM_BLE_SCAN_COND_DELETE, 0, &adv_filt_param, NULL,
                             bte_scan_filt_param_cfg_evt, 0);
    /* Add an allow-all filter on index 0*/
    adv_filt_param.dely_mode = IMMEDIATE_DELY_MODE;
    adv_filt_param.feat_seln = ALLOW_ALL_FILTER;
    adv_filt_param.filt_logic_type = BTA_DM_BLE_PF_FILT_LOGIC_OR;
    adv_filt_param.list_logic_type = BTA_DM_BLE_PF_LIST_LOGIC_OR;
    adv_filt_param.rssi_low_thres = LOWEST_RSSI_VALUE;
    adv_filt_param.rssi_high_thres = LOWEST_RSSI_VALUE;
    BTA_DmBleScanFilterSetup(BTA_DM_BLE_SCAN_COND_ADD, 0, &adv_filt_param, NULL,
                             bte_scan_filt_param_cfg_evt, 0);
    /* TODO: Do we need to handle multiple inquiries at the same time? */
    /* Set inquiry params and call API */
    inq_params.mode = BTA_DM_GENERAL_INQUIRY | BTA_BLE_GENERAL_INQUIRY;
#if (defined(BTA_HOST_INTERLEAVE_SEARCH) && BTA_HOST_INTERLEAVE_SEARCH == TRUE)
    inq_params.intl_duration[0] = BTIF_DM_INTERLEAVE_DURATION_BR_ONE;
    inq_params.intl_duration[1] = BTIF_DM_INTERLEAVE_DURATION_LE_ONE;
    inq_params.intl_duration[2] = BTIF_DM_INTERLEAVE_DURATION_BR_TWO;
    inq_params.intl_duration[3] = BTIF_DM_INTERLEAVE_DURATION_LE_TWO;
#endif
#else
    inq_params.mode = BTA_DM_GENERAL_INQUIRY;
#endif
    inq_params.duration = BTIF_DM_DEFAULT_INQ_MAX_DURATION;
    inq_params.max_resps = BTIF_DM_DEFAULT_INQ_MAX_RESULTS;
    inq_params.report_dup = TRUE;
    inq_params.filter_type = BTA_DM_INQ_CLR;
    /* TODO: Filter device by BDA needs to be implemented here */
    /* Will be enabled to TRUE once inquiry busy level has been received */
    btif_dm_inquiry_in_progress = FALSE;
    /* find nearby devices */
    BTA_DmSearch(&inq_params, services, bte_search_devices_evt);
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_cancel_discovery
**
** Description      Cancels search
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_cancel_discovery(void)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    BTA_DmSearchCancel();
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_create_bond
**
** Description      Initiate bonding with the specified device
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_create_bond(const tls_bt_addr_t *bd_addr, int transport)
{
    btif_dm_create_bond_cb_t create_bond_cb;
    create_bond_cb.transport = transport;
    bdcpy(create_bond_cb.bdaddr.address, bd_addr->address);
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)	
    bdstr_t bdstr;
    BTIF_TRACE_EVENT("%s: bd_addr=%s, transport=%d", __FUNCTION__, bdaddr_to_string(bd_addr, bdstr,
                     sizeof(bdstr)), transport);
#endif	

    if(pairing_cb.state != WM_BT_BOND_STATE_NONE) {
        return TLS_BT_STATUS_BUSY;
    }

    btif_stats_add_bond_event(bd_addr, BTIF_DM_FUNC_CREATE_BOND, pairing_cb.state);
    btif_transfer_context(btif_dm_generic_evt, BTIF_DM_CB_CREATE_BOND,
                          (char *)&create_bond_cb, sizeof(btif_dm_create_bond_cb_t), NULL);
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_create_bond_out_of_band
**
** Description      Initiate bonding with the specified device using out of band data
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_create_bond_out_of_band(const tls_bt_addr_t *bd_addr, int transport,
        const bt_out_of_band_data_t *oob_data)
{
    bdcpy(oob_cb.bdaddr, bd_addr->address);
    wm_memcpy(&oob_cb.oob_data, oob_data, sizeof(bt_out_of_band_data_t));
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)	
    bdstr_t bdstr;
    BTIF_TRACE_EVENT("%s: bd_addr=%s, transport=%d", __FUNCTION__, bdaddr_to_string(bd_addr, bdstr,
                     sizeof(bdstr)), transport);
#endif	
    return btif_dm_create_bond(bd_addr, transport);
}

/*******************************************************************************
**
** Function         btif_dm_cancel_bond
**
** Description      Initiate bonding with the specified device
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t btif_dm_cancel_bond(const tls_bt_addr_t *bd_addr)
{
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)
    bdstr_t bdstr;
    BTIF_TRACE_EVENT("%s: bd_addr=%s", __FUNCTION__, bdaddr_to_string(bd_addr, bdstr, sizeof(bdstr)));
#endif	
    btif_stats_add_bond_event(bd_addr, BTIF_DM_FUNC_CANCEL_BOND, pairing_cb.state);

    /* TODO:
    **  1. Restore scan modes
    **  2. special handling for HID devices
    */
    if(pairing_cb.state == WM_BT_BOND_STATE_BONDING) {
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))

        if(pairing_cb.is_ssp) {
            if(pairing_cb.is_le_only) {
                BTA_DmBleSecurityGrant((uint8_t *)bd_addr->address, BTA_DM_SEC_PAIR_NOT_SPT);
            } else {
                BTA_DmConfirm((uint8_t *)bd_addr->address, FALSE);
                BTA_DmBondCancel((uint8_t *)bd_addr->address);
                btif_storage_remove_bonded_device((tls_bt_addr_t *)bd_addr);
            }
        } else {
            if(pairing_cb.is_le_only) {
                BTA_DmBondCancel((uint8_t *)bd_addr->address);
            } else {
                BTA_DmPinReply((uint8_t *)bd_addr->address, FALSE, 0, NULL);
            }

            /* Cancel bonding, in case it is in ACL connection setup state */
            BTA_DmBondCancel((uint8_t *)bd_addr->address);
        }

#else

        if(pairing_cb.is_ssp) {
            BTA_DmConfirm((uint8_t *)bd_addr->address, FALSE);
        } else {
            BTA_DmPinReply((uint8_t *)bd_addr->address, FALSE, 0, NULL);
        }

        /* Cancel bonding, in case it is in ACL connection setup state */
        BTA_DmBondCancel((uint8_t *)bd_addr->address);
        btif_storage_remove_bonded_device((tls_bt_addr_t *)bd_addr);
#endif
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_hh_open_failed
**
** Description      informs the upper layers if the HH have failed during bonding
**
** Returns          none
**
*******************************************************************************/

void btif_dm_hh_open_failed(tls_bt_addr_t *bdaddr)
{
    if(pairing_cb.state == WM_BT_BOND_STATE_BONDING &&
            bdcmp(bdaddr->address, pairing_cb.bd_addr) == 0) {
        bond_state_changed(TLS_BT_STATUS_FAIL, bdaddr, WM_BT_BOND_STATE_NONE);
    }
}

/*******************************************************************************
**
** Function         btif_dm_remove_bond
**
** Description      Removes bonding with the specified device
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t btif_dm_remove_bond(const tls_bt_addr_t *bd_addr)
{
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)
    bdstr_t bdstr;
    BTIF_TRACE_EVENT("%s: bd_addr=%s", __FUNCTION__, bdaddr_to_string(bd_addr, bdstr, sizeof(bdstr)));
#endif	
    btif_stats_add_bond_event(bd_addr, BTIF_DM_FUNC_REMOVE_BOND, pairing_cb.state);
    btif_transfer_context(btif_dm_generic_evt, BTIF_DM_CB_REMOVE_BOND,
                          (char *)bd_addr, sizeof(tls_bt_addr_t), NULL);
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_pin_reply
**
** Description      BT legacy pairing - PIN code reply
**
** Returns          bt_status_t
**
*******************************************************************************/

tls_bt_status_t btif_dm_pin_reply(const tls_bt_addr_t *bd_addr, uint8_t accept,
                                  uint8_t pin_len, tls_bt_pin_code_t *pin_code)
{
    BTIF_TRACE_EVENT("%s: accept=%d", __FUNCTION__, accept);

    if(pin_code == NULL || pin_len > PIN_CODE_LEN) {
        return TLS_BT_STATUS_FAIL;
    }

#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))

    if(pairing_cb.is_le_only) {
        int i;
        uint32_t passkey = 0;
        int multi[] = {100000, 10000, 1000, 100, 10, 1};
        BD_ADDR remote_bd_addr;
        bdcpy(remote_bd_addr, bd_addr->address);

        for(i = 0; i < 6; i++) {
            passkey += (multi[i] * (pin_code->pin[i] - '0'));
        }

        BTIF_TRACE_DEBUG("btif_dm_pin_reply: passkey: %d", passkey);
        BTA_DmBlePasskeyReply(remote_bd_addr, accept, passkey);
    } else {
        BTA_DmPinReply((uint8_t *)bd_addr->address, accept, pin_len, pin_code->pin);

        if(accept) {
            pairing_cb.pin_code_len = pin_len;
        }
    }

#else
    BTA_DmPinReply((uint8_t *)bd_addr->address, accept, pin_len, pin_code->pin);

    if(accept) {
        pairing_cb.pin_code_len = pin_len;
    }

#endif
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_ssp_reply
**
** Description      BT SSP Reply - Just Works, Numeric Comparison & Passkey Entry
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_ssp_reply(const tls_bt_addr_t *bd_addr,
                                  tls_bt_ssp_variant_t variant, uint8_t accept,
                                  uint32_t passkey)
{
    UNUSED(passkey);

    if(variant == WM_BT_SSP_VARIANT_PASSKEY_ENTRY) {
        /* This is not implemented in the stack.
         * For devices with display, this is not needed
        */
        BTIF_TRACE_WARNING("%s: Not implemented", __FUNCTION__);
        return TLS_BT_STATUS_FAIL;
    }

    /* BT_SSP_VARIANT_CONSENT & BT_SSP_VARIANT_PASSKEY_CONFIRMATION supported */
    BTIF_TRACE_EVENT("%s: accept=%d", __FUNCTION__, accept);
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))

    if(pairing_cb.is_le_only) {
        if(pairing_cb.is_le_nc) {
            BTA_DmBleConfirmReply((uint8_t *)bd_addr->address, accept);
        } else {
            if(accept) {
                BTA_DmBleSecurityGrant((uint8_t *)bd_addr->address, BTA_DM_SEC_GRANTED);
            } else {
                BTA_DmBleSecurityGrant((uint8_t *)bd_addr->address, BTA_DM_SEC_PAIR_NOT_SPT);
            }
        }
    } else {
        BTA_DmConfirm((uint8_t *)bd_addr->address, accept);
    }

#else
    BTA_DmConfirm((uint8_t *)bd_addr->address, accept);
#endif
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_get_adapter_property
**
** Description     Queries the BTA for the adapter property
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_get_adapter_property(tls_bt_property_t *prop)
{
    BTIF_TRACE_EVENT("%s: type=0x%x", __FUNCTION__, prop->type);

    switch(prop->type) {
        case WM_BT_PROPERTY_BDNAME: {
            tls_bt_bdname_t *bd_name = (tls_bt_bdname_t *)prop->val;
            strncpy((char *)bd_name->name, (char *)btif_get_default_local_name(),
                    sizeof(bd_name->name) - 1);
            bd_name->name[sizeof(bd_name->name) - 1] = 0;
            prop->len = strlen((char *)bd_name->name);
        }
        break;

        case WM_BT_PROPERTY_ADAPTER_SCAN_MODE: {
            /* if the storage does not have it. Most likely app never set it. Default is NONE */
            bt_scan_mode_t *mode = (bt_scan_mode_t *)prop->val;
            *mode = BT_SCAN_MODE_NONE;
            prop->len = sizeof(bt_scan_mode_t);
        }
        break;

        case WM_BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT: {
            uint32_t *tmt = (uint32_t *)prop->val;
            *tmt = 120; /* default to 120s, if not found in NV */
            prop->len = sizeof(uint32_t);
        }
        break;

        default:
            prop->len = 0;
            return TLS_BT_STATUS_FAIL;
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_get_remote_services
**
** Description      Start SDP to get remote services
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_get_remote_services(tls_bt_addr_t *remote_addr)
{
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)
    bdstr_t bdstr;
    BTIF_TRACE_EVENT("%s: remote_addr=%s", __FUNCTION__, bdaddr_to_string(remote_addr, bdstr,
                     sizeof(bdstr)));
#endif	
    BTA_DmDiscover(remote_addr->address, BTA_ALL_SERVICE_MASK,
                   bte_dm_search_services_evt, TRUE);
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_get_remote_services_transport
**
** Description      Start SDP to get remote services by transport
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_get_remote_services_by_transport(tls_bt_addr_t *remote_addr,
        const int transport)
{
    BTIF_TRACE_EVENT("%s", __func__);
    /* Set the mask extension */
    tBTA_SERVICE_MASK_EXT mask_ext;
    mask_ext.num_uuid = 0;
    mask_ext.p_uuid = NULL;
    mask_ext.srvc_mask = BTA_ALL_SERVICE_MASK;
    BTA_DmDiscoverByTransport(remote_addr->address, &mask_ext,
                              bte_dm_search_services_evt, TRUE, transport);
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_dm_get_remote_service_record
**
** Description      Start SDP to get remote service record
**
**
** Returns          bt_status_t
*******************************************************************************/
tls_bt_status_t btif_dm_get_remote_service_record(tls_bt_addr_t *remote_addr,
        tls_bt_uuid_t *uuid)
{
    tSDP_UUID sdp_uuid;
#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)	
    bdstr_t bdstr;
    BTIF_TRACE_EVENT("%s: remote_addr=%s", __FUNCTION__, bdaddr_to_string(remote_addr, bdstr,
                     sizeof(bdstr)));
#endif	
    sdp_uuid.len = MAX_UUID_SIZE;
    wm_memcpy(sdp_uuid.uu.uuid128, uuid->uu, MAX_UUID_SIZE);
    BTA_DmDiscoverUUID(remote_addr->address, &sdp_uuid,
                       bte_dm_remote_service_record_evt, TRUE);
    return TLS_BT_STATUS_SUCCESS;
}

void btif_dm_execute_service_request(uint16_t event, char *p_param)
{
    uint8_t b_enable = FALSE;
    tls_bt_status_t status;
    tls_bt_host_msg_t msg;

    if(event == BTIF_DM_ENABLE_SERVICE) {
        b_enable = TRUE;
    }

    status = btif_in_execute_service_request(*((tBTA_SERVICE_ID *)p_param), b_enable);

    if(status == TLS_BT_STATUS_SUCCESS) {
        tls_bt_property_t property;
        tls_bt_uuid_t local_uuids[BT_MAX_NUM_UUIDS];
        /* Now send the UUID_PROPERTY_CHANGED event to the upper layer */
        BTIF_STORAGE_FILL_PROPERTY(&property, WM_BT_PROPERTY_UUIDS,
                                   sizeof(local_uuids), local_uuids);
        btif_storage_get_adapter_property(&property);
        msg.adapter_prop.num_properties = 1;
        msg.adapter_prop.properties = &property;
        msg.adapter_prop.status = status;

        if(bt_host_cb) {
            bt_host_cb(WM_BT_ADAPTER_PROP_CHG_EVT, &msg);
        }
    }

    return;
}

void btif_dm_proc_io_req(BD_ADDR bd_addr, tBTA_IO_CAP *p_io_cap, tBTA_OOB_DATA *p_oob_data,
                         tBTA_AUTH_REQ *p_auth_req, uint8_t is_orig)
{
    uint8_t   yes_no_bit = BTA_AUTH_SP_YES & *p_auth_req;
    /* if local initiated:
    **      1. set DD + MITM
    ** if remote initiated:
    **      1. Copy over the auth_req from peer's io_rsp
    **      2. Set the MITM if peer has it set or if peer has DisplayYesNo (iPhone)
    ** as a fallback set MITM+GB if peer had MITM set
    */
    UNUSED(bd_addr);
    UNUSED(p_io_cap);
    UNUSED(p_oob_data);
    BTIF_TRACE_DEBUG("+%s: p_auth_req=%d", __FUNCTION__, *p_auth_req);

    if(pairing_cb.is_local_initiated) {
        /* if initing/responding to a dedicated bonding, use dedicate bonding bit */
        *p_auth_req = BTA_AUTH_DD_BOND | BTA_AUTH_SP_YES;
    } else if(!is_orig) {
        /* peer initiated paring. They probably know what they want.
        ** Copy the mitm from peer device.
        */
        BTIF_TRACE_DEBUG("%s: setting p_auth_req to peer's: %d",
                         __FUNCTION__, pairing_cb.auth_req);
        *p_auth_req = (pairing_cb.auth_req & BTA_AUTH_BONDS);

        /* copy over the MITM bit as well. In addition if the peer has DisplayYesNo, force MITM */
        if((yes_no_bit) || (pairing_cb.io_cap & BTM_IO_CAP_IO)) {
            *p_auth_req |= BTA_AUTH_SP_YES;
        }
    } else if(yes_no_bit) {
        /* set the general bonding bit for stored device */
        *p_auth_req = BTA_AUTH_GEN_BOND | yes_no_bit;
    }

    BTIF_TRACE_DEBUG("-%s: p_auth_req=%d", __FUNCTION__, *p_auth_req);
}

void btif_dm_proc_io_rsp(BD_ADDR bd_addr, tBTA_IO_CAP io_cap,
                         tBTA_OOB_DATA oob_data, tBTA_AUTH_REQ auth_req)
{
    UNUSED(bd_addr);
    UNUSED(oob_data);

    if(auth_req & BTA_AUTH_BONDS) {
        BTIF_TRACE_DEBUG("%s auth_req:%d", __FUNCTION__, auth_req);
        pairing_cb.auth_req = auth_req;
        pairing_cb.io_cap = io_cap;
    }
}

void btif_dm_set_oob_for_io_req(tBTA_OOB_DATA  *p_has_oob_data)
{
    if(is_empty_128bit(oob_cb.oob_data.c192)) {
        *p_has_oob_data = FALSE;
    } else {
        *p_has_oob_data = TRUE;
    }

    BTIF_TRACE_DEBUG("%s: *p_has_oob_data=%d", __func__, *p_has_oob_data);
}

void btif_dm_set_oob_for_le_io_req(BD_ADDR bd_addr, tBTA_OOB_DATA  *p_has_oob_data,
                                   tBTA_LE_AUTH_REQ *p_auth_req)
{
    /* We currently support only Security Manager TK as OOB data for LE transport.
       If it's not present mark no OOB data.
     */
    if(!is_empty_128bit(oob_cb.oob_data.sm_tk)) {
        /* make sure OOB data is for this particular device */
        if(memcmp(bd_addr, oob_cb.bdaddr, BD_ADDR_LEN) == 0) {
            // When using OOB with TK, SC Secure Connections bit must be disabled.
            tBTA_LE_AUTH_REQ mask = ~BTM_LE_AUTH_REQ_SC_ONLY;
            *p_auth_req = ((*p_auth_req) & mask);
            *p_has_oob_data = TRUE;
        } else {
            *p_has_oob_data = FALSE;
            BTIF_TRACE_WARNING("%s: remote address didn't match OOB data address",
                               __func__);
        }
    } else {
        *p_has_oob_data = FALSE;
    }

    BTIF_TRACE_DEBUG("%s *p_has_oob_data=%d", __func__, *p_has_oob_data);
}

#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))

static void btif_dm_ble_key_notif_evt(tBTA_DM_SP_KEY_NOTIF *p_ssp_key_notif)
{
    tls_bt_host_msg_t msg;
    tls_bt_addr_t bd_addr;
    tls_bt_bdname_t bd_name;
    uint32_t cod;
    int dev_type;
    int addr_type = 0;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    /* Remote name update */
    if(!btif_get_device_type(p_ssp_key_notif->bd_addr, &addr_type, &dev_type)) {
        dev_type = BT_DEVICE_TYPE_BLE;
    }

    btif_dm_update_ble_remote_properties(p_ssp_key_notif->bd_addr, p_ssp_key_notif->bd_name,
                                         (tBT_DEVICE_TYPE) dev_type);
    bdcpy(bd_addr.address, p_ssp_key_notif->bd_addr);
    wm_memcpy(bd_name.name, p_ssp_key_notif->bd_name, BD_NAME_LEN);
    bond_state_changed(TLS_BT_STATUS_SUCCESS, &bd_addr, WM_BT_BOND_STATE_BONDING);
    pairing_cb.is_ssp = FALSE;
    cod = COD_UNCLASSIFIED;
    msg.ssp_request.bd_name = &bd_name;
    msg.ssp_request.cod = cod;
    msg.ssp_request.pairing_variant = WM_BT_SSP_VARIANT_PASSKEY_NOTIFICATION;
    msg.ssp_request.pass_key = p_ssp_key_notif->passkey;
    msg.ssp_request.remote_bd_addr = &bd_addr;

    if(bt_host_cb) {
        bt_host_cb(WM_BT_SSP_REQUEST_EVT, &msg);
    }
}

/*******************************************************************************
**
** Function         btif_dm_ble_auth_cmpl_evt
**
** Description      Executes authentication complete event in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_ble_auth_cmpl_evt(tBTA_DM_AUTH_CMPL *p_auth_cmpl)
{
    /* Save link key, if not temporary */
    tls_bt_addr_t bd_addr;
    tls_bt_status_t status = TLS_BT_STATUS_FAIL;
    tls_bt_bond_state_t state = WM_BT_BOND_STATE_NONE;
    bdcpy(bd_addr.address, p_auth_cmpl->bd_addr);
    /* Clear OOB data */
    wm_memset(&oob_cb, 0, sizeof(oob_cb));

    if((p_auth_cmpl->success == TRUE) && (p_auth_cmpl->key_present)) {
        /* store keys */
    }

    if(p_auth_cmpl->success) {
        status = TLS_BT_STATUS_SUCCESS;
        state = WM_BT_BOND_STATE_BONDED;
        int addr_type, dev_type;
        tls_bt_addr_t bdaddr;
        bdcpy(bdaddr.address, p_auth_cmpl->bd_addr);
        BTIF_TRACE_DEBUG("btif_dm_ble_auth_cmpl_evt:...\r\n");

        if(btif_storage_get_remote_addr_type(&bdaddr, &addr_type) != TLS_BT_STATUS_SUCCESS) {
            btif_storage_set_remote_addr_type(&bdaddr, p_auth_cmpl->addr_type, 0);
        }

        if(btif_storage_get_remote_device_type(&bdaddr, &dev_type) != TLS_BT_STATUS_SUCCESS) {
            btif_storage_set_remote_device_type(&bdaddr, p_auth_cmpl->dev_type, 0);
        }

        hci_dbg_msg("btif_dm_ble_auth_cmpl_evt success:...addr_type=%d, dev_type=%d\r\n",
                    p_auth_cmpl->addr_type, p_auth_cmpl->dev_type);
        hci_dbg_hexstring("address:", bd_addr.address, 6);

        /* Test for temporary bonding */
        if(btm_get_bond_type_dev(p_auth_cmpl->bd_addr) == BOND_TYPE_TEMPORARY) {
            BTIF_TRACE_DEBUG("%s: sending WM_BT_BOND_STATE_NONE for Temp pairing",
                             __func__);
            btif_storage_remove_bonded_device(&bdaddr);
            state = WM_BT_BOND_STATE_NONE;
        } else {
            btif_dm_save_ble_bonding_keys();
            BTA_GATTC_Refresh(bd_addr.address);
            btif_dm_get_remote_services_by_transport(&bd_addr, BTA_GATT_TRANSPORT_LE);
        }
    } else {
        /*Map the HCI fail reason  to  bt status  */
        tls_bt_addr_t bdaddr;
        bdcpy(bdaddr.address, p_auth_cmpl->bd_addr);
        hci_dbg_msg("btif_dm_ble_auth_cmpl_evt failed, reason=0x%04x, BTA_DM_AUTH_SMP_OOB_FAIL=%04x\r\n",
                    p_auth_cmpl->fail_reason, BTA_DM_AUTH_SMP_OOB_FAIL);
        hci_dbg_hexstring("address:", bd_addr.address, 6);

        switch(p_auth_cmpl->fail_reason) {
            case BTA_DM_AUTH_SMP_PAIR_AUTH_FAIL:
            case BTA_DM_AUTH_SMP_CONFIRM_VALUE_FAIL:
                //btif_dm_remove_ble_bonding_keys();
                btif_storage_remove_ble_bonding_keys(&bdaddr);
                status = TLS_BT_STATUS_AUTH_FAILURE;
                break;

            case BTA_DM_AUTH_SMP_PAIR_NOT_SUPPORT:
                status = TLS_BT_STATUS_AUTH_REJECTED;
                break;

            default:
                //btif_dm_remove_ble_bonding_keys();
                btif_storage_remove_ble_bonding_keys(&bdaddr);
                status =  TLS_BT_STATUS_FAIL;
                break;
        }
    }

    bond_state_changed(status, &bd_addr, state);
}

void    btif_dm_load_ble_local_keys(void)
{
    wm_memset(&ble_local_key_cb, 0, sizeof(btif_dm_local_key_cb_t));

    if(btif_storage_get_ble_local_key(BTIF_DM_LE_LOCAL_KEY_ER, (char *)&ble_local_key_cb.er[0],
                                      BT_OCTET16_LEN) == TLS_BT_STATUS_SUCCESS) {
        ble_local_key_cb.is_er_rcvd = TRUE;
        BTIF_TRACE_DEBUG("%s BLE ER key loaded", __FUNCTION__);
    }

    if((btif_storage_get_ble_local_key(BTIF_DM_LE_LOCAL_KEY_IR, (char *)&ble_local_key_cb.id_keys.ir[0],
                                       BT_OCTET16_LEN) == TLS_BT_STATUS_SUCCESS) &&
            (btif_storage_get_ble_local_key(BTIF_DM_LE_LOCAL_KEY_IRK, (char *)&ble_local_key_cb.id_keys.irk[0],
                                            BT_OCTET16_LEN) == TLS_BT_STATUS_SUCCESS) &&
            (btif_storage_get_ble_local_key(BTIF_DM_LE_LOCAL_KEY_DHK, (char *)&ble_local_key_cb.id_keys.dhk[0],
                                            BT_OCTET16_LEN) == TLS_BT_STATUS_SUCCESS)) {
        ble_local_key_cb.is_id_keys_rcvd = TRUE;
        BTIF_TRACE_DEBUG("%s BLE ID keys loaded", __FUNCTION__);
    }
}
void    btif_dm_get_ble_local_keys(tBTA_DM_BLE_LOCAL_KEY_MASK *p_key_mask, BT_OCTET16 er,
                                   tBTA_BLE_LOCAL_ID_KEYS *p_id_keys)
{
    if(ble_local_key_cb.is_er_rcvd) {
        wm_memcpy(&er[0], &ble_local_key_cb.er[0], sizeof(BT_OCTET16));
        *p_key_mask |= BTA_BLE_LOCAL_KEY_TYPE_ER;
    }

    if(ble_local_key_cb.is_id_keys_rcvd) {
        wm_memcpy(&p_id_keys->ir[0], &ble_local_key_cb.id_keys.ir[0], sizeof(BT_OCTET16));
        wm_memcpy(&p_id_keys->irk[0],  &ble_local_key_cb.id_keys.irk[0], sizeof(BT_OCTET16));
        wm_memcpy(&p_id_keys->dhk[0],  &ble_local_key_cb.id_keys.dhk[0], sizeof(BT_OCTET16));
        *p_key_mask |= BTA_BLE_LOCAL_KEY_TYPE_ID;
    }

    BTIF_TRACE_DEBUG("%s  *p_key_mask=0x%02x", __FUNCTION__,   *p_key_mask);
}

void btif_dm_save_ble_bonding_keys(void)
{
    tls_bt_addr_t bd_addr;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    bool save = false;
    bdcpy(bd_addr.address, pairing_cb.bd_addr);
    hci_dbg_hexstring("btif_dm_save_ble_bonding_keys address:", bd_addr.address, 6);

    if(pairing_cb.ble.is_penc_key_rcvd) {
        btif_storage_add_ble_bonding_key(&bd_addr,
                                         (char *) &pairing_cb.ble.penc_key,
                                         BTIF_DM_LE_KEY_PENC,
                                         sizeof(tBTM_LE_PENC_KEYS));
        save = true;
    }

    if(pairing_cb.ble.is_pid_key_rcvd) {
        btif_storage_add_ble_bonding_key(&bd_addr,
                                         (char *) &pairing_cb.ble.pid_key,
                                         BTIF_DM_LE_KEY_PID,
                                         sizeof(tBTM_LE_PID_KEYS));
        save = true;
    }

    if(pairing_cb.ble.is_pcsrk_key_rcvd) {
        btif_storage_add_ble_bonding_key(&bd_addr,
                                         (char *) &pairing_cb.ble.pcsrk_key,
                                         BTIF_DM_LE_KEY_PCSRK,
                                         sizeof(tBTM_LE_PCSRK_KEYS));
        save = true;
    }

    if(pairing_cb.ble.is_lenc_key_rcvd) {
        btif_storage_add_ble_bonding_key(&bd_addr,
                                         (char *) &pairing_cb.ble.lenc_key,
                                         BTIF_DM_LE_KEY_LENC,
                                         sizeof(tBTM_LE_LENC_KEYS));
        save = true;
    }

    if(pairing_cb.ble.is_lcsrk_key_rcvd) {
        btif_storage_add_ble_bonding_key(&bd_addr,
                                         (char *) &pairing_cb.ble.lcsrk_key,
                                         BTIF_DM_LE_KEY_LCSRK,
                                         sizeof(tBTM_LE_LCSRK_KEYS));
        save = true;
    }

    if(pairing_cb.ble.is_lidk_key_rcvd) {
        btif_storage_add_ble_bonding_key(&bd_addr,
                                         NULL,
                                         BTIF_DM_LE_KEY_LID,
                                         0);
        save = true;
    }

    if(save) {
        btif_storage_flush(1);
    }
}

void btif_dm_remove_ble_bonding_keys(void)
{
    tls_bt_addr_t bd_addr;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    bdcpy(bd_addr.address, pairing_cb.bd_addr);
    btif_storage_remove_ble_bonding_keys(&bd_addr);
}

/*******************************************************************************
**
** Function         btif_dm_ble_sec_req_evt
**
** Description      Eprocess security request event in btif context
**
** Returns          void
**
*******************************************************************************/
void btif_dm_ble_sec_req_evt(tBTA_DM_BLE_SEC_REQ *p_ble_req)
{
    tls_bt_host_msg_t msg;
    tls_bt_addr_t bd_addr;
    tls_bt_bdname_t bd_name;
    uint32_t cod;
    int dev_type;
    int addr_type = 0;
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);

    if(pairing_cb.state == WM_BT_BOND_STATE_BONDING) {
        BTIF_TRACE_DEBUG("%s Discard security request", __FUNCTION__);
        return;
    }

    /* Remote name update */
    if(!btif_get_device_type(p_ble_req->bd_addr, &addr_type, &dev_type)) {
        dev_type = BT_DEVICE_TYPE_BLE;
    }

    btif_dm_update_ble_remote_properties(p_ble_req->bd_addr, p_ble_req->bd_name,
                                         (tBT_DEVICE_TYPE) dev_type);
    bdcpy(bd_addr.address, p_ble_req->bd_addr);
    wm_memcpy(bd_name.name, p_ble_req->bd_name, BD_NAME_LEN);
    bond_state_changed(TLS_BT_STATUS_SUCCESS, &bd_addr, WM_BT_BOND_STATE_BONDING);
    pairing_cb.bond_type = BOND_TYPE_PERSISTENT;
    pairing_cb.is_le_only = TRUE;
    pairing_cb.is_le_nc = FALSE;
    pairing_cb.is_ssp = TRUE;
    btm_set_bond_type_dev(p_ble_req->bd_addr, pairing_cb.bond_type);
    cod = COD_UNCLASSIFIED;
    msg.ssp_request.bd_name = &bd_name;
    msg.ssp_request.cod = cod;
    msg.ssp_request.pairing_variant = WM_BT_SSP_VARIANT_CONSENT;
    msg.ssp_request.pass_key = 0;
    msg.ssp_request.remote_bd_addr = &bd_addr;

    if(bt_host_cb) {
        bt_host_cb(WM_BT_SSP_REQUEST_EVT, &msg);
    }
}

/*******************************************************************************
**
** Function         btif_dm_ble_passkey_req_evt
**
** Description      Executes pin request event in btif context
**
** Returns          void
**
*******************************************************************************/
static void btif_dm_ble_passkey_req_evt(tBTA_DM_PIN_REQ *p_pin_req)
{
    tls_bt_host_msg_t msg;
    tls_bt_addr_t bd_addr;
    tls_bt_bdname_t bd_name;
    uint32_t cod;
    int dev_type;
    int addr_type = 0;

    /* Remote name update */
    if(!btif_get_device_type(p_pin_req->bd_addr, &addr_type, &dev_type)) {
        dev_type = BT_DEVICE_TYPE_BLE;
    }

    btif_dm_update_ble_remote_properties(p_pin_req->bd_addr, p_pin_req->bd_name,
                                         (tBT_DEVICE_TYPE) dev_type);
    bdcpy(bd_addr.address, p_pin_req->bd_addr);
    wm_memcpy(bd_name.name, p_pin_req->bd_name, BD_NAME_LEN);
    bond_state_changed(TLS_BT_STATUS_SUCCESS, &bd_addr, WM_BT_BOND_STATE_BONDING);
    pairing_cb.is_le_only = TRUE;
    cod = COD_UNCLASSIFIED;
    msg.pin_request.bd_name = &bd_name;
    msg.pin_request.cod = cod;
    msg.pin_request.min_16_digit = 0;

    if(bt_host_cb) {
        bt_host_cb(WM_BT_PIN_REQUEST_EVT, &msg);
    }
}
static void btif_dm_ble_key_nc_req_evt(tBTA_DM_SP_KEY_NOTIF *p_notif_req)
{
    tls_bt_host_msg_t msg;
    /* TODO implement key notification for numeric comparison */
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    /* Remote name update */
    btif_update_remote_properties(p_notif_req->bd_addr, p_notif_req->bd_name,
                                  NULL, BT_DEVICE_TYPE_BLE);
    tls_bt_addr_t bd_addr;
    bdcpy(bd_addr.address, p_notif_req->bd_addr);
    tls_bt_bdname_t bd_name;
    wm_memcpy(bd_name.name, p_notif_req->bd_name, BD_NAME_LEN);
    bond_state_changed(TLS_BT_STATUS_SUCCESS, &bd_addr, WM_BT_BOND_STATE_BONDING);
    pairing_cb.is_ssp = FALSE;
    pairing_cb.is_le_only = TRUE;
    pairing_cb.is_le_nc = TRUE;
    msg.ssp_request.bd_name = &bd_name;
    msg.ssp_request.cod = COD_UNCLASSIFIED;
    msg.ssp_request.pairing_variant = WM_BT_SSP_VARIANT_PASSKEY_CONFIRMATION;
    msg.ssp_request.pass_key = p_notif_req->passkey;
    msg.ssp_request.remote_bd_addr = &bd_addr;

    if(bt_host_cb) {
        bt_host_cb(WM_BT_SSP_REQUEST_EVT, &msg);
    }
}

static void btif_dm_ble_oob_req_evt(tBTA_DM_SP_RMT_OOB *req_oob_type)
{
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    tls_bt_addr_t bd_addr;
    bdcpy(bd_addr.address, req_oob_type->bd_addr);

    /* We currently support only Security Manager TK as OOB data. We already
     * checked if it's present in btif_dm_set_oob_for_le_io_req, but check here
     * again. If it's not present do nothing, pairing will timeout.
     */
    if(is_empty_128bit(oob_cb.oob_data.sm_tk)) {
        return;
    }

    /* make sure OOB data is for this particular device */
    if(memcmp(req_oob_type->bd_addr, oob_cb.bdaddr, BD_ADDR_LEN) != 0) {
        BTIF_TRACE_WARNING("%s: remote address didn't match OOB data address", __func__);
        return;
    }

    /* Remote name update */
    btif_update_remote_properties(req_oob_type->bd_addr, req_oob_type->bd_name,
                                  NULL, BT_DEVICE_TYPE_BLE);
    bond_state_changed(TLS_BT_STATUS_SUCCESS, &bd_addr, WM_BT_BOND_STATE_BONDING);
    pairing_cb.is_ssp = FALSE;
    pairing_cb.is_le_only = TRUE;
    pairing_cb.is_le_nc = FALSE;
    BTM_BleOobDataReply(req_oob_type->bd_addr, 0, 16, oob_cb.oob_data.sm_tk);
}

void btif_dm_update_ble_remote_properties(BD_ADDR bd_addr, BD_NAME bd_name,
        tBT_DEVICE_TYPE dev_type)
{
    btif_update_remote_properties(bd_addr, bd_name, NULL, dev_type);
}

static void btif_dm_ble_tx_test_cback(void *p)
{
    btif_transfer_context(btif_dm_generic_evt, BTIF_DM_CB_LE_TX_TEST,
                          (char *)p, 1, NULL);
}

static void btif_dm_ble_rx_test_cback(void *p)
{
    btif_transfer_context(btif_dm_generic_evt, BTIF_DM_CB_LE_RX_TEST,
                          (char *)p, 1, NULL);
}

static void btif_dm_ble_test_end_cback(void *p)
{
    btif_transfer_context(btif_dm_generic_evt, BTIF_DM_CB_LE_TEST_END,
                          (char *)p, 3, NULL);
}
/*******************************************************************************
**
** Function         btif_le_test_mode
**
** Description     Sends a HCI BLE Test command to the Controller
**
** Returns          BT_STATUS_SUCCESS on success
**
*******************************************************************************/
tls_bt_status_t btif_le_test_mode(uint16_t opcode, uint8_t *buf, uint8_t len)
{
    switch(opcode) {
        case HCI_BLE_TRANSMITTER_TEST:
            if(len != 3) {
                return TLS_BT_STATUS_PARM_INVALID;
            }

            BTM_BleTransmitterTest(buf[0], buf[1], buf[2], btif_dm_ble_tx_test_cback);
            break;

        case HCI_BLE_RECEIVER_TEST:
            if(len != 1) {
                return TLS_BT_STATUS_PARM_INVALID;
            }

            BTM_BleReceiverTest(buf[0], btif_dm_ble_rx_test_cback);
            break;

        case HCI_BLE_TEST_END:
            BTM_BleTestEnd((tBTM_CMPL_CB *) btif_dm_ble_test_end_cback);
            break;

        default:
            BTIF_TRACE_ERROR("%s: Unknown LE Test Mode Command 0x%x", __FUNCTION__, opcode);
            return TLS_BT_STATUS_UNSUPPORTED;
    }

    return TLS_BT_STATUS_SUCCESS;
}
#endif

void btif_dm_on_disable()
{
    /* cancel any pending pairing requests */
    if(pairing_cb.state == WM_BT_BOND_STATE_BONDING) {
        tls_bt_addr_t bd_addr;
        BTIF_TRACE_DEBUG("%s: Cancel pending pairing request", __FUNCTION__);
        bdcpy(bd_addr.address, pairing_cb.bd_addr);
        btif_dm_cancel_bond(&bd_addr);
    }
}

/*******************************************************************************
**
** Function         btif_dm_read_energy_info
**
** Description     Reads the energy info from controller
**
** Returns         void
**
*******************************************************************************/
void btif_dm_read_energy_info()
{
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
    BTA_DmBleGetEnergyInfo(bta_energy_info_cb);
#endif
}

static char *btif_get_default_local_name()
{
    if(btif_default_local_name[0] == '\0') {
        int ret_len = 0;
        int max_len = sizeof(btif_default_local_name) - 1;
        unsigned char bt_mac[6];

        if(btif_config_get_str("Local", "Adapter", "Name", btif_default_local_name, &ret_len)) {
            assert(ret_len < max_len);
            btif_default_local_name[ret_len] = '\0';
            BTIF_TRACE_API("%s: Readout Local Name", __FUNCTION__, btif_default_local_name);
        } else if(BTM_DEF_LOCAL_NAME[0] != '\0') {
            strncpy(btif_default_local_name, BTM_DEF_LOCAL_NAME, max_len);
            extern int tls_get_bt_mac_addr(uint8_t *mac);
            tls_get_bt_mac_addr(bt_mac);
            assert((strlen(BTM_DEF_LOCAL_NAME) + 9) < max_len);
            sprintf(btif_default_local_name + strlen(BTM_DEF_LOCAL_NAME), "-%02X:%02X:%02X", bt_mac[3],
                    bt_mac[4], bt_mac[5]);
        }

        btif_default_local_name[max_len] = '\0';
    }

    return btif_default_local_name;
}

static void btif_stats_add_bond_event(const tls_bt_addr_t *bd_addr,
                                      bt_bond_function_t function,
                                      tls_bt_bond_state_t state)
{
}

