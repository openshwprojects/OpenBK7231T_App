/******************************************************************************
 *
 *  Copyright (c) 2014 The Android Open Source Project
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
 *  Filename:      btif_storage.c
 *
 *  Description:   Stores the local BT adapter and remote device properties in
 *                 NVRAM storage, typically as xml file in the
 *                 mobile's filesystem
 *
 *
 */
#include "bluetooth.h"
#include "wm_bt_storage.h"
#define LOG_TAG "BTIF_STORAGE"

#include "btif_api.h"
#include "btif_storage.h"
#include "btif_util.h"
#include "bdaddr.h"
#include "config.h"
#include "gki.h"
#include "osi.h"
#include "bta_hh_api.h"
#include "btif_hh.h"
#include "wm_params.h"
#include "uuid.h"

extern void hci_dbg_msg(const char *format, ...);
extern void bd2str(const tls_bt_addr_t * bd_addr, bdstr_t *bdstr);

/************************************************************************************
**  Constants & Macros
************************************************************************************/

#define BTIF_STORAGE_PATH_BLUEDROID "/data/misc/bluedroid"

//#define BTIF_STORAGE_PATH_ADAPTER_INFO "adapter_info"
//#define BTIF_STORAGE_PATH_REMOTE_DEVICES "remote_devices"
#define BTIF_STORAGE_PATH_REMOTE_DEVTIME "Timestamp"
#define BTIF_STORAGE_PATH_REMOTE_DEVCLASS "DevClass"
#define BTIF_STORAGE_PATH_REMOTE_DEVTYPE "DevType"
#define BTIF_STORAGE_PATH_REMOTE_NAME "Name"
#define BTIF_STORAGE_PATH_REMOTE_VER_MFCT "Manufacturer"
#define BTIF_STORAGE_PATH_REMOTE_VER_VER "LmpVer"
#define BTIF_STORAGE_PATH_REMOTE_VER_SUBVER "LmpSubVer"

//#define BTIF_STORAGE_PATH_REMOTE_LINKKEYS "remote_linkkeys"
#define BTIF_STORAGE_PATH_REMOTE_ALIASE "Name"
#define BTIF_STORAGE_PATH_REMOTE_SERVICE "Service"
#define BTIF_STORAGE_PATH_REMOTE_HIDINFO "HidInfo"
#define BTIF_STORAGE_KEY_ADAPTER_NAME "Name"
#define BTIF_STORAGE_KEY_ADAPTER_SCANMODE "ScanMode"
#define BTIF_STORAGE_KEY_ADAPTER_DISC_TIMEOUT "DiscoveryTimeout"


#define BTIF_AUTO_PAIR_CONF_FILE  "/etc/bluetooth/auto_pair_devlist.conf"
#define BTIF_STORAGE_PATH_AUTOPAIR_BLACKLIST "AutoPairBlacklist"
#define BTIF_STORAGE_KEY_AUTOPAIR_BLACKLIST_ADDR "AddressBlacklist"
#define BTIF_STORAGE_KEY_AUTOPAIR_BLACKLIST_EXACTNAME "ExactNameBlacklist"
#define BTIF_STORAGE_KEY_AUTOPAIR_BLACKLIST_PARTIALNAME "PartialNameBlacklist"
#define BTIF_STORAGE_KEY_AUTOPAIR_FIXPIN_KBLIST "FixedPinZerosKeyboardBlacklist"
#define BTIF_STORAGE_KEY_AUTOPAIR_DYNAMIC_BLACKLIST_ADDR "DynamicAddressBlacklist"

#define BTIF_AUTO_PAIR_CONF_VALUE_SEPARATOR ","


/* This is a local property to add a device found */
#define BT_PROPERTY_REMOTE_DEVICE_TIMESTAMP 0xFF

#define BTIF_STORAGE_GET_ADAPTER_PROP(t,v,l,p) \
    {p.type=t;p.val=v;p.len=l; btif_storage_get_adapter_property(&p);}

#define BTIF_STORAGE_GET_REMOTE_PROP(b,t,v,l,p) \
    {p.type=t;p.val=v;p.len=l;btif_storage_get_remote_device_property(b,&p);}

#define STORAGE_BDADDR_STRING_SZ           (18)      /* 00:11:22:33:44:55 */
#define STORAGE_UUID_STRING_SIZE           (36+1)    /* 00001200-0000-1000-8000-00805f9b34fb; */
#define STORAGE_PINLEN_STRING_MAX_SIZE     (2)       /* ascii pinlen max chars */
#define STORAGE_KEYTYPE_STRING_MAX_SIZE    (1)       /* ascii keytype max chars */

#define STORAGE_KEY_TYPE_MAX               (10)

#define STORAGE_HID_ATRR_MASK_SIZE           (4)
#define STORAGE_HID_SUB_CLASS_SIZE           (2)
#define STORAGE_HID_APP_ID_SIZE              (2)
#define STORAGE_HID_VENDOR_ID_SIZE           (4)
#define STORAGE_HID_PRODUCT_ID_SIZE          (4)
#define STORAGE_HID_VERSION_SIZE             (4)
#define STORAGE_HID_CTRY_CODE_SIZE           (2)
#define STORAGE_HID_DESC_LEN_SIZE            (4)
#define STORAGE_HID_DESC_MAX_SIZE            (2*512)

/* <18 char bd addr> <space> LIST< <36 char uuid> <;> > <keytype (dec)> <pinlen> */
#define BTIF_REMOTE_SERVICES_ENTRY_SIZE_MAX (STORAGE_BDADDR_STRING_SZ + 1 +\
        STORAGE_UUID_STRING_SIZE*BT_MAX_NUM_UUIDS + \
        STORAGE_PINLEN_STRING_MAX_SIZE +\
        STORAGE_KEYTYPE_STRING_MAX_SIZE)

#define STORAGE_REMOTE_LINKKEYS_ENTRY_SIZE (LINK_KEY_LEN*2 + 1 + 2 + 1 + 2)

/* <18 char bd addr> <space>LIST <attr_mask> <space> > <sub_class> <space> <app_id> <space>
                                <vendor_id> <space> > <product_id> <space> <version> <space>
                                <ctry_code> <space> > <desc_len> <space> <desc_list> <space> */
#define BTIF_HID_INFO_ENTRY_SIZE_MAX    (STORAGE_BDADDR_STRING_SZ + 1 +\
        STORAGE_HID_ATRR_MASK_SIZE + 1 +\
        STORAGE_HID_SUB_CLASS_SIZE + 1 +\
        STORAGE_HID_APP_ID_SIZE+ 1 +\
        STORAGE_HID_VENDOR_ID_SIZE+ 1 +\
        STORAGE_HID_PRODUCT_ID_SIZE+ 1 +\
        STORAGE_HID_VERSION_SIZE+ 1 +\
        STORAGE_HID_CTRY_CODE_SIZE+ 1 +\
        STORAGE_HID_DESC_LEN_SIZE+ 1 +\
        STORAGE_HID_DESC_MAX_SIZE+ 1 )


/* currently remote services is the potentially largest entry */
#define BTIF_STORAGE_MAX_LINE_SZ BTIF_REMOTE_SERVICES_ENTRY_SIZE_MAX


/* check against unv max entry size at compile time */
#if (BTIF_STORAGE_ENTRY_MAX_SIZE > UNV_MAXLINE_LENGTH)
#error "btif storage entry size exceeds unv max line size"
#endif


#define BTIF_STORAGE_HL_APP          "hl_app"
#define BTIF_STORAGE_HL_APP_CB       "hl_app_cb"
#define BTIF_STORAGE_HL_APP_DATA     "hl_app_data_"
#define BTIF_STORAGE_HL_APP_MDL_DATA "hl_app_mdl_data_"

/************************************************************************************
**  Local type definitions
************************************************************************************/
typedef struct {
    uint32_t num_devices;
    tls_bt_addr_t devices[BTM_SEC_MAX_DEVICE_RECORDS];
} btif_bonded_devices_t;

/************************************************************************************
**  External variables
************************************************************************************/
extern uint16_t bta_service_id_to_uuid_lkup_tbl [BTA_MAX_SERVICE_ID];
extern tls_bt_addr_t btif_local_bd_addr;
extern void uuid_to_string(const tls_bt_uuid_t *uuid, uuid_string_t *uuid_string);
extern void BTA_DmAddBleDevice(BD_ADDR bd_addr, tBLE_ADDR_TYPE addr_type, tBT_DEVICE_TYPE dev_type);
extern void BTA_DmAddBleKey(BD_ADDR bd_addr, tBTA_LE_KEY_VALUE *p_le_key, tBTA_LE_KEY_TYPE key_type);
extern int btif_config_remove_remote(const char *key);

/************************************************************************************
**  External functions
************************************************************************************/

extern void btif_gatts_add_bonded_dev_from_nv(BD_ADDR bda);

/************************************************************************************
**  Internal Functions
************************************************************************************/

tls_bt_status_t btif_in_fetch_bonded_ble_device(char *remote_bd_addr, int add,
        btif_bonded_devices_t *p_bonded_devices);
tls_bt_status_t btif_storage_get_remote_addr_type(tls_bt_addr_t *remote_bd_addr,
        int *addr_type);
static tls_bt_status_t btif_in_fetch_bonded_device(char *bdstr);
tls_bt_status_t btif_storage_get_ble_bonding_key(tls_bt_addr_t *remote_bd_addr,
        uint8_t key_type,
        char *key_value,
        int key_length);


/************************************************************************************
**  Static functions
************************************************************************************/
#if 0
/*******************************************************************************
**
** Function         btif_in_split_uuids_string_to_list
**
** Description      Internal helper function to split the string of UUIDs
**                  read from the NVRAM to an array
**
** Returns          None
**
*******************************************************************************/
static void btif_in_split_uuids_string_to_list(char *str, tls_bt_uuid_t *p_uuid,
        uint32_t *p_num_uuid)
{
    char buf[64];
    char *p_start = str;
    char *p_needle;
    uint32_t num = 0;

    do {
        //p_needle = strchr(p_start, ';');
        p_needle = strchr(p_start, ' ');

        if(p_needle < p_start) {
            break;
        }

        wm_memset(buf, 0, sizeof(buf));
        strncpy(buf, p_start, (p_needle - p_start));
        string_to_uuid(buf, p_uuid + num);
        num++;
        p_start = ++p_needle;
    } while(*p_start != 0);

    *p_num_uuid = num;
}
#endif
static int prop2cfg(tls_bt_addr_t *remote_bd_addr, tls_bt_property_t *prop)
{
    bdstr_t bdstr = {0};

    if(remote_bd_addr) {
        bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr_t));
    }

    BTIF_TRACE_DEBUG("in, bd addr:%s, prop type:%d, len:%d", bdstr, prop->type, prop->len);
    char value[256];

    if(prop->len <= 0 || prop->len > (int)sizeof(value) - 1) {
        BTIF_TRACE_ERROR("property type:%d, len:%d is invalid", prop->type, prop->len);
        return FALSE;
    }

    switch(prop->type) {
        case WM_BT_PROPERTY_REMOTE_DEVICE_TIMESTAMP:
            btif_config_set_int("Remote", bdstr,
                                BTIF_STORAGE_PATH_REMOTE_DEVTIME, (int)rand());
            static const char *exclude_filter[] =
            {"LinkKey", "LE_KEY_PENC", "LE_KEY_PID", "LE_KEY_PCSRK", "LE_KEY_LENC", "LE_KEY_LCSRK"};
            btif_config_filter_remove("Remote", exclude_filter, sizeof(exclude_filter) / sizeof(char *),
                                      BTIF_STORAGE_MAX_ALLOWED_REMOTE_DEVICE);
            break;

        case WM_BT_PROPERTY_BDNAME:
            strncpy(value, (char *)prop->val, prop->len);
            value[prop->len] = '\0';

            if(remote_bd_addr)
                btif_config_set_str("Remote", bdstr,
                                    BTIF_STORAGE_PATH_REMOTE_NAME, value);
            else btif_config_set_str("Local", "Adapter",
                                         BTIF_STORAGE_KEY_ADAPTER_NAME, value);

            btif_config_flush(1);
            break;

        case WM_BT_PROPERTY_REMOTE_FRIENDLY_NAME:
            strncpy(value, (char *)prop->val, prop->len);
            value[prop->len] = '\0';
            btif_config_set_str("Remote", bdstr, BTIF_STORAGE_PATH_REMOTE_ALIASE, value);
            break;

        case WM_BT_PROPERTY_ADAPTER_SCAN_MODE:
            btif_config_set_int("Local", "Adapter",
                                BTIF_STORAGE_KEY_ADAPTER_SCANMODE, *(int *)prop->val);
            break;

        case WM_BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT:
            btif_config_set_int("Local", "Adapter",
                                BTIF_STORAGE_KEY_ADAPTER_DISC_TIMEOUT, *(int *)prop->val);
            break;

        case WM_BT_PROPERTY_CLASS_OF_DEVICE:
            btif_config_set_int("Remote", bdstr,
                                BTIF_STORAGE_PATH_REMOTE_DEVCLASS, *(int *)prop->val);
            break;

        case WM_BT_PROPERTY_TYPE_OF_DEVICE:
            btif_config_set_int("Remote", bdstr,
                                BTIF_STORAGE_PATH_REMOTE_DEVTYPE, *(int *)prop->val);
            break;

        case WM_BT_PROPERTY_UUIDS: {
            uint32_t i;
            char buf[64];
            value[0] = 0;

            for(i = 0; i < (prop->len) / sizeof(tls_bt_uuid_t); i++) {
                tls_bt_uuid_t *p_uuid = (tls_bt_uuid_t *)prop->val + i;
                wm_memset(buf, 0, sizeof(buf));
                uuid_to_string(p_uuid, (uuid_string_t *)buf);
                strcat(value, buf);
                //strcat(value, ";");
                strcat(value, " ");
            }

            btif_config_set_str("Remote", bdstr, BTIF_STORAGE_PATH_REMOTE_SERVICE, value);
            break;
        }

        case WM_BT_PROPERTY_REMOTE_VERSION_INFO: {
            bt_remote_version_t *info = (bt_remote_version_t *)prop->val;

            if(!info) {
                return FALSE;
            }

            btif_config_set_int("Remote", bdstr,
                                BTIF_STORAGE_PATH_REMOTE_VER_MFCT, info->manufacturer);
            btif_config_set_int("Remote", bdstr,
                                BTIF_STORAGE_PATH_REMOTE_VER_VER, info->version);
            btif_config_set_int("Remote", bdstr,
                                BTIF_STORAGE_PATH_REMOTE_VER_SUBVER, info->sub_ver);
        }
        break;

        default:
            BTIF_TRACE_ERROR("Unknown prop type:%d", prop->type);
            return FALSE;
    }

    /* save changes if the device was bonded */
    if(btif_in_fetch_bonded_device(bdstr) == TLS_BT_STATUS_SUCCESS) {
        btif_config_save();
    }

    return TRUE;
}
static int cfg2prop(tls_bt_addr_t *remote_bd_addr, tls_bt_property_t *prop)
{
    bdstr_t bdstr = {0};

    if(remote_bd_addr) {
        bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr_t));
    }

    BTIF_TRACE_DEBUG("in, bd addr:%s, prop type:%d, len:%d", bdstr, prop->type, prop->len);

    if(prop->len <= 0) {
        BTIF_TRACE_ERROR("property type:%d, len:%d is invalid", prop->type, prop->len);
        return FALSE;
    }

    int ret = FALSE;

    switch(prop->type) {
        case WM_BT_PROPERTY_REMOTE_DEVICE_TIMESTAMP:
            if(prop->len >= (int)sizeof(int))
                ret = btif_config_get_int("Remote", bdstr,
                                          BTIF_STORAGE_PATH_REMOTE_DEVTIME, (int *)prop->val);

            break;

        case WM_BT_PROPERTY_BDNAME: {
            int len = prop->len;

            if(remote_bd_addr)
                ret = btif_config_get_str("Remote", bdstr,
                                          BTIF_STORAGE_PATH_REMOTE_NAME, (char *)prop->val, &len);
            else ret = btif_config_get_str("Local", "Adapter",
                                               BTIF_STORAGE_KEY_ADAPTER_NAME, (char *)prop->val, &len);

            if(ret && len && len <= prop->len) {
                prop->len = len - 1;
            } else {
                prop->len = 0;
                ret = FALSE;
            }

            break;
        }

        case WM_BT_PROPERTY_REMOTE_FRIENDLY_NAME: {
            int len = prop->len;
            ret = btif_config_get_str("Remote", bdstr,
                                      BTIF_STORAGE_PATH_REMOTE_ALIASE, (char *)prop->val, &len);

            if(ret && len && len <= prop->len) {
                prop->len = len - 1;
            } else {
                prop->len = 0;
                ret = FALSE;
            }

            break;
        }

        case WM_BT_PROPERTY_ADAPTER_SCAN_MODE:
            if(prop->len >= (int)sizeof(int))
                ret = btif_config_get_int("Local", "Adapter",
                                          BTIF_STORAGE_KEY_ADAPTER_SCANMODE, (int *)prop->val);

            break;

        case WM_BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT:
            if(prop->len >= (int)sizeof(int))
                ret = btif_config_get_int("Local", "Adapter",
                                          BTIF_STORAGE_KEY_ADAPTER_DISC_TIMEOUT, (int *)prop->val);

            break;

        case WM_BT_PROPERTY_CLASS_OF_DEVICE:
            if(prop->len >= (int)sizeof(int))
                ret = btif_config_get_int("Remote", bdstr,
                                          BTIF_STORAGE_PATH_REMOTE_DEVCLASS, (int *)prop->val);

            break;

        case WM_BT_PROPERTY_TYPE_OF_DEVICE:
            if(prop->len >= (int)sizeof(int))
                ret = btif_config_get_int("Remote",
                                          bdstr, BTIF_STORAGE_PATH_REMOTE_DEVTYPE, (int *)prop->val);

            break;

        case WM_BT_PROPERTY_UUIDS: {
            {
                prop->val = NULL;
                prop->len = 0;
            }
        }
        break;

        case WM_BT_PROPERTY_REMOTE_VERSION_INFO: {
            bt_remote_version_t *info = (bt_remote_version_t *)prop->val;

            if(prop->len >= (int)sizeof(bt_remote_version_t)) {
                ret = btif_config_get_int("Remote", bdstr,
                                          BTIF_STORAGE_PATH_REMOTE_VER_MFCT, &info->manufacturer);

                if(ret == TRUE)
                    ret = btif_config_get_int("Remote", bdstr,
                                              BTIF_STORAGE_PATH_REMOTE_VER_VER, &info->version);

                if(ret == TRUE)
                    ret = btif_config_get_int("Remote", bdstr,
                                              BTIF_STORAGE_PATH_REMOTE_VER_SUBVER, &info->sub_ver);
            }
        }
        break;

        default:
            BTIF_TRACE_ERROR("Unknow prop type:%d", prop->type);
            return FALSE;
    }

    return ret;
}

/*******************************************************************************
**
** Function         btif_in_fetch_bonded_devices
**
** Description      Internal helper function to fetch the bonded devices
**                  from NVRAM
**
** Returns          BT_STATUS_SUCCESS if successful, BT_STATUS_FAIL otherwise
**
*******************************************************************************/
static tls_bt_status_t btif_in_fetch_bonded_device(char *bdstr)
{
    uint8_t bt_linkkey_file_found = FALSE;
//    int device_type;
    int type = BTIF_CFG_TYPE_BIN;
    LINK_KEY link_key;
    int size = sizeof(link_key);

    if(btif_config_get("Remote", bdstr, "LinkKey", (char *)link_key, &size, &type)) {
        int linkkey_type;

        if(btif_config_get_int("Remote", bdstr, "LinkKeyType", &linkkey_type)) {
            bt_linkkey_file_found = TRUE;
        } else {
            bt_linkkey_file_found = FALSE;
        }
    }

#if (BLE_INCLUDED == TRUE)

    if((btif_in_fetch_bonded_ble_device(bdstr, FALSE, NULL) != TLS_BT_STATUS_SUCCESS)
            && (!bt_linkkey_file_found)) {
        BTIF_TRACE_DEBUG("Remote device:%s, no link key or ble key found", bdstr);
        return TLS_BT_STATUS_FAIL;
    }

#else

    if((!bt_linkkey_file_found)) {
        BTIF_TRACE_DEBUG("Remote device:%s, no link key found", bdstr);
        return TLS_BT_STATUS_FAIL;
    }

#endif
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_in_fetch_bonded_devices
**
** Description      Internal helper function to fetch the bonded devices
**                  from NVRAM
**
** Returns          BT_STATUS_SUCCESS if successful, BT_STATUS_FAIL otherwise
**
*******************************************************************************/
static tls_bt_status_t btif_in_fetch_bonded_devices(btif_bonded_devices_t *p_bonded_devices,
        int add)
{
    BTIF_TRACE_DEBUG("btif_in_fetch_bonded_devices in add:%d", add);
    int index = 0;
    bt_remote_device_t remote_device;
    BD_ADDR bd_addr;
    DEV_CLASS dev_class;
//    LINK_KEY link_key;
    wm_memset(p_bonded_devices, 0, sizeof(btif_bonded_devices_t));

    for(index = 0; index < BTM_SEC_MAX_DEVICE_RECORDS; index++) {
        /*Always return OK*/
        btif_wm_config_get_remote_device(index, (void *)&remote_device, 0);

        if((remote_device.valid_tag == 0xdeadbeaf) && (remote_device.in_use)
                && (remote_device.valid_bit & DEVICE_LINK_KEY_VALID_BIT)) {
            if(add) {
                memcpy(bd_addr, remote_device.bd_addr, 6);
                printf("btif_storage Link key Loaded  BDA: %08x%04x and BTA_DmAddDevice\r\n",
                       (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                       (bd_addr[4] << 8) + bd_addr[5]);
                uint2devclass(remote_device.class_of_device, dev_class);
                BTA_DmAddDevice(bd_addr, dev_class, remote_device.link_key, 0, 0,
                                remote_device.key_type, remote_device.io_cap, remote_device.pin_length);
#if BLE_INCLUDED == TRUE

                if((remote_device.valid_bit & DEVICE_TYPE_VALID_BIT)
                        && (remote_device.device_type == BT_DEVICE_TYPE_DUMO)) {
                    btif_gatts_add_bonded_dev_from_nv(bd_addr);
                }

#endif
            }

            memcpy(&p_bonded_devices->devices[p_bonded_devices->num_devices++].address[0],
                   remote_device.bd_addr, 6);
        }

#if BLE_INCLUDED == TRUE

        if((remote_device.valid_tag == 0xdeadbeaf) && (remote_device.in_use)
                && (remote_device.valid_bit & DEVICE_TYPE_VALID_BIT)
                && (remote_device.device_type == BT_DEVICE_TYPE_BLE)) {
            if(1) {
                if(add) {
                    memcpy(bd_addr, remote_device.bd_addr, 6);
                    printf("btif_storage ble device type Loaded  BDA: %08x%04x and BTA_DmAddDevice\r\n",
                           (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                           (bd_addr[4] << 8) + bd_addr[5]);
                    BTA_DmAddBleDevice(bd_addr, remote_device.ble_addr_type, BT_DEVICE_TYPE_BLE);

                    if(remote_device.valid_bit & DEVICE_BLE_KEY_PENC_VALID_BIT) {
                        BTA_DmAddBleKey(remote_device.bd_addr, (tBTA_LE_KEY_VALUE *)remote_device.key_penc,
                                        BTIF_DM_LE_KEY_PENC);
                    }

                    if(remote_device.valid_bit & DEVICE_BLE_KEY_LENC_VALID_BIT) {
                        BTA_DmAddBleKey(remote_device.bd_addr, (tBTA_LE_KEY_VALUE *)remote_device.key_lenc,
                                        BTIF_DM_LE_KEY_LENC);
                    }

                    if(remote_device.valid_bit & DEVICE_BLE_KEY_PID_VALID_BIT) {
                        BTA_DmAddBleKey(remote_device.bd_addr, (tBTA_LE_KEY_VALUE *)remote_device.key_pid,
                                        BTIF_DM_LE_KEY_PID);
                    }

                    if(remote_device.valid_bit & DEVICE_BLE_KEY_LID_VALID_BIT) {
                        BTA_DmAddBleKey(remote_device.bd_addr, (tBTA_LE_KEY_VALUE *)remote_device.key_lid,
                                        BTIF_DM_LE_KEY_LID);
                    }

                    if(remote_device.valid_bit & DEVICE_BLE_KEY_PCSRK_VALID_BIT) {
                        BTA_DmAddBleKey(remote_device.bd_addr, (tBTA_LE_KEY_VALUE *)remote_device.key_pcsrk,
                                        BTIF_DM_LE_KEY_PCSRK);
                    }

                    if(remote_device.valid_bit & DEVICE_BLE_KEY_LCSRK_VALID_BIT) {
                        BTA_DmAddBleKey(remote_device.bd_addr, (tBTA_LE_KEY_VALUE *)remote_device.key_lcsrk,
                                        BTIF_DM_LE_KEY_LCSRK);
                    }

                    //Twice???
                    memcpy(&p_bonded_devices->devices[p_bonded_devices->num_devices++].address[0],
                           remote_device.bd_addr, 6);
                    btif_gatts_add_bonded_dev_from_nv(bd_addr);
                }
            }
        }

#endif
    }

    return TLS_BT_STATUS_SUCCESS;
}

#if (BLE_INCLUDED == TRUE)
static void btif_read_le_key(const uint8_t key_type, const size_t key_len, tls_bt_addr_t bd_addr,
                             const uint8_t addr_type, const bool add_key, bool *device_added, bool *key_found)
{
    assert(device_added);
    assert(key_found);
    char buffer[100];
    memset(buffer, 0, sizeof(buffer));

    if(btif_storage_get_ble_bonding_key(&bd_addr, key_type, buffer, key_len) == TLS_BT_STATUS_SUCCESS) {
        if(add_key) {
            BD_ADDR bta_bd_addr;
            bdcpy(bta_bd_addr, bd_addr.address);

            if(!*device_added) {
                BTA_DmAddBleDevice(bta_bd_addr, addr_type, BT_DEVICE_TYPE_BLE);
                *device_added = true;
            }

#if (BT_USE_TRACES == TRUE && BT_TRACE_BTIF == TRUE)
            char bd_str[20] = {0};
            BTIF_TRACE_DEBUG("%s() Adding key type %d for %s", __func__,
                             key_type, bdaddr_to_string(&bd_addr, bd_str, sizeof(bd_str)));
#endif			
            BTA_DmAddBleKey(bta_bd_addr, (tBTA_LE_KEY_VALUE *)buffer, key_type);
        }

        *key_found = true;
    }
}
#endif
/************************************************************************************
**  Externs
************************************************************************************/

/************************************************************************************
**  Functions
************************************************************************************/

/** functions are synchronous.
 * functions can be called by both internal modules such as BTIF_DM and by external entiries from HAL via BTIF_context_switch
 * For OUT parameters,  caller is expected to provide the memory.
 * Caller is expected to provide a valid pointer to 'property->value' based on the property->type
 */
/*******************************************************************************
**
** Function         btif_storage_get_adapter_property
**
** Description      BTIF storage API - Fetches the adapter property->type
**                  from NVRAM and fills property->val.
**                  Caller should provide memory for property->val and
**                  set the property->val
**
** Returns          BT_STATUS_SUCCESS if the fetch was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_get_adapter_property(tls_bt_property_t *property)
{
    BTIF_TRACE_DEBUG("%s, type=%d\r\n", __FUNCTION__, property->type);

    /* Special handling for adapter BD_ADDR and BONDED_DEVICES */
    if(property->type == WM_BT_PROPERTY_BDADDR) {
//        BD_ADDR addr;
        tls_bt_addr_t *bd_addr = (tls_bt_addr_t *)property->val;
        /* This has been cached in btif. Just fetch it from there */
        wm_memcpy(bd_addr, &btif_local_bd_addr, sizeof(tls_bt_addr_t));
        property->len = sizeof(tls_bt_addr_t);
        return TLS_BT_STATUS_SUCCESS;
    } else if(property->type == WM_BT_PROPERTY_ADAPTER_BONDED_DEVICES) {
        btif_bonded_devices_t bonded_devices;
        btif_in_fetch_bonded_devices(&bonded_devices, 0);
        BTIF_TRACE_DEBUG("%s: Number of bonded devices: %d Property:WM_BT_PROPERTY_ADAPTER_BONDED_DEVICES",
                         __FUNCTION__, bonded_devices.num_devices);

        if(bonded_devices.num_devices > 0) {
            property->len = bonded_devices.num_devices * sizeof(tls_bt_addr_t);
            wm_memcpy(property->val, bonded_devices.devices, property->len);
        }

        /* if there are no bonded_devices, then length shall be 0 */
        return TLS_BT_STATUS_SUCCESS;
    } else if(property->type == WM_BT_PROPERTY_UUIDS) {
        /* publish list of local supported services */
        tls_bt_uuid_t *p_uuid = (tls_bt_uuid_t *)property->val;
        uint32_t num_uuids = 0;
        uint32_t i;
        tBTA_SERVICE_MASK service_mask = btif_get_enabled_services_mask();

        //BTIF_TRACE_ERROR("%s service_mask:0x%x", __FUNCTION__, service_mask);
        for(i = 0; i < BTA_MAX_SERVICE_ID; i++) {
            /* This should eventually become a function when more services are enabled */
            if(service_mask
                    & (tBTA_SERVICE_MASK)(1 << i)) {
                switch(i) {
                    case BTA_HFP_SERVICE_ID: {
                        uuid16_to_uuid128(UUID_SERVCLASS_AG_HANDSFREE,
                                          p_uuid + num_uuids);
                        num_uuids++;
                    }

                    /* intentional fall through: Send both BFP & HSP UUIDs if HFP is enabled */
                    case BTA_HSP_SERVICE_ID: {
                        uuid16_to_uuid128(UUID_SERVCLASS_HEADSET_AUDIO_GATEWAY,
                                          p_uuid + num_uuids);
                        num_uuids++;
                    }
                    break;

                    case BTA_A2DP_SOURCE_SERVICE_ID: {
                        uuid16_to_uuid128(UUID_SERVCLASS_AUDIO_SOURCE,
                                          p_uuid + num_uuids);
                        num_uuids++;
                    }
                    break;

                    case BTA_A2DP_SINK_SERVICE_ID: {
                        uuid16_to_uuid128(UUID_SERVCLASS_AUDIO_SINK,
                                          p_uuid + num_uuids);
                        num_uuids++;
                    }
                    break;

                    case BTA_HFP_HS_SERVICE_ID: {
                        uuid16_to_uuid128(UUID_SERVCLASS_HF_HANDSFREE,
                                          p_uuid + num_uuids);
                        num_uuids++;
                    }
                    break;
                }
            }
        }

        property->len = (num_uuids) * sizeof(tls_bt_uuid_t);
        return TLS_BT_STATUS_SUCCESS;
    }

    /* fall through for other properties */
    if(!cfg2prop(NULL, property)) {
        return btif_dm_get_adapter_property(property);
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_storage_set_adapter_property
**
** Description      BTIF storage API - Stores the adapter property
**                  to NVRAM
**
** Returns          BT_STATUS_SUCCESS if the store was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_set_adapter_property(tls_bt_property_t *property)
{
    return prop2cfg(NULL, property) ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}


/*******************************************************************************
**
** Function         btif_storage_get_remote_device_property
**
** Description      BTIF storage API - Fetches the remote device property->type
**                  from NVRAM and fills property->val.
**                  Caller should provide memory for property->val and
**                  set the property->val
**
** Returns          BT_STATUS_SUCCESS if the fetch was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_get_remote_device_property(tls_bt_addr_t *remote_bd_addr,
        tls_bt_property_t *property)
{
    return cfg2prop(remote_bd_addr, property) ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}
/*******************************************************************************
**
** Function         btif_storage_set_remote_device_property
**
** Description      BTIF storage API - Stores the remote device property
**                  to NVRAM
**
** Returns          BT_STATUS_SUCCESS if the store was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_set_remote_device_property(tls_bt_addr_t *remote_bd_addr,
        tls_bt_property_t *property)
{
    return prop2cfg(remote_bd_addr, property) ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         btif_storage_add_remote_device
**
** Description      BTIF storage API - Adds a newly discovered device to NVRAM
**                  along with the timestamp. Also, stores the various
**                  properties - RSSI, BDADDR, NAME (if found in EIR)
**
** Returns          BT_STATUS_SUCCESS if the store was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_add_remote_device(tls_bt_addr_t *remote_bd_addr,
        uint32_t num_properties,
        tls_bt_property_t *properties)
{
    uint32_t i = 0;

    /* TODO: If writing a property, fails do we go back undo the earlier
     * written properties? */
    for(i = 0; i < num_properties; i++) {
        /* Ignore the RSSI as this is not stored in DB */
        if(properties[i].type == WM_BT_PROPERTY_REMOTE_RSSI) {
            continue;
        }

        /* BD_ADDR for remote device needs special handling as we also store timestamp */
        if(properties[i].type == WM_BT_PROPERTY_BDADDR) {
            tls_bt_property_t addr_prop;
            wm_memcpy(&addr_prop, &properties[i], sizeof(tls_bt_property_t));
            addr_prop.type = WM_BT_PROPERTY_REMOTE_DEVICE_TIMESTAMP;
            btif_storage_set_remote_device_property(remote_bd_addr,
                                                    &addr_prop);
        } else {
            btif_storage_set_remote_device_property(remote_bd_addr,
                                                    &properties[i]);
        }
    }

    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_storage_add_bonded_device
**
** Description      BTIF storage API - Adds the newly bonded device to NVRAM
**                  along with the link-key, Key type and Pin key length
**
** Returns          BT_STATUS_SUCCESS if the store was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/

tls_bt_status_t btif_storage_add_bonded_device(tls_bt_addr_t *remote_bd_addr,
        LINK_KEY link_key,
        uint8_t key_type,
        uint8_t pin_length)
{
    bdstr_t bdstr;
    bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr_t));
    int ret = btif_config_set_int("Remote", bdstr, "LinkKeyType", (int)key_type);
    ret &= btif_config_set_int("Remote", bdstr, "PinLength", (int)pin_length);
    ret &= btif_config_set("Remote", bdstr, "LinkKey", (const char *)link_key, sizeof(LINK_KEY),
                           BTIF_CFG_TYPE_BIN);
    /* write bonded info immediately */
    btif_config_flush(0);
    return TLS_BT_STATUS_SUCCESS;
    return ret ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}
uint8_t btif_storage_update_remote_devices(const tls_bt_addr_t *remote_bd_addr)
{
    bdstr_t bdstr;
    bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr_t));
    return btif_wm_config_update_remote_device(bdstr);
}

/*******************************************************************************
**
** Function         btif_storage_remove_bonded_device
**
** Description      BTIF storage API - Deletes the bonded device from NVRAM
**
** Returns          BT_STATUS_SUCCESS if the deletion was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_remove_bonded_device(tls_bt_addr_t *remote_bd_addr)
{
    bdstr_t bdstr;
    bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr_t));
    BTIF_TRACE_DEBUG("in bd addr:%s", bdstr);
    int ret = 1;
#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
    btif_storage_remove_ble_bonding_keys(remote_bd_addr);
#endif

    if(btif_config_exist("Remote", bdstr, "LinkKeyType")) {
        ret &= btif_config_remove("Remote", bdstr, "LinkKeyType");
    }

    if(btif_config_exist("Remote", bdstr, "PinLength")) {
        ret &= btif_config_remove("Remote", bdstr, "PinLength");
    }

    if(btif_config_exist("Remote", bdstr, "LinkKey")) {
        ret &= btif_config_remove("Remote", bdstr, "LinkKey");
    }

    /*Remove this device*/
    btif_config_remove_remote(bdstr);
    /* write bonded info immediately */
    btif_config_flush(1);
    return ret ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         btif_storage_load_bonded_devices
**
** Description      BTIF storage API - Loads all the bonded devices from NVRAM
**                  and adds to the BTA.
**                  Additionally, this API also invokes the adaper_properties_cb
**                  and remote_device_properties_cb for each of the bonded devices.
**
** Returns          BT_STATUS_SUCCESS if successful, BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_load_bonded_devices(void)
{
//    char *fname;
    btif_bonded_devices_t bonded_devices;
    uint32_t i = 0;
    tls_bt_property_t adapter_props[6];
    uint32_t num_props = 0;
    tls_bt_property_t remote_properties[8];
    tls_bt_addr_t addr;
    tls_bt_bdname_t name, alias;
    bt_scan_mode_t mode;
    uint32_t disc_timeout;
    tls_bt_addr_t *devices_list = NULL;
    tls_bt_uuid_t local_uuids[BT_MAX_NUM_UUIDS];
    tls_bt_uuid_t remote_uuids[BT_MAX_NUM_UUIDS];
    uint32_t cod, devtype;
    btif_in_fetch_bonded_devices(&bonded_devices, 1);
    /* Now send the adapter_properties_cb with all adapter_properties */
    {
        wm_memset(adapter_props, 0, sizeof(adapter_props));
        /* BD_ADDR */
        BTIF_STORAGE_GET_ADAPTER_PROP(WM_BT_PROPERTY_BDADDR, &addr, sizeof(addr),
                                      adapter_props[num_props]);
        num_props++;
        /* BD_NAME */
        BTIF_STORAGE_GET_ADAPTER_PROP(WM_BT_PROPERTY_BDNAME, &name, sizeof(name),
                                      adapter_props[num_props]);
        num_props++;
        /* SCAN_MODE */
        /* TODO: At the time of BT on, always report the scan mode as 0 irrespective
         of the scan_mode during the previous enable cycle.
         This needs to be re-visited as part of the app/stack enable sequence
         synchronization */
        mode = BT_SCAN_MODE_NONE;
        adapter_props[num_props].type = WM_BT_PROPERTY_ADAPTER_SCAN_MODE;
        adapter_props[num_props].len = sizeof(mode);
        adapter_props[num_props].val = &mode;
        num_props++;
        /* DISC_TIMEOUT */
        BTIF_STORAGE_GET_ADAPTER_PROP(WM_BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT,
                                      &disc_timeout, sizeof(disc_timeout),
                                      adapter_props[num_props]);
        num_props++;

        if(bonded_devices.num_devices > 0) {
            /* BONDED_DEVICES */
            devices_list = (tls_bt_addr_t *)GKI_getbuf(sizeof(tls_bt_addr_t) * bonded_devices.num_devices);
            assert(devices_list != NULL);
            adapter_props[num_props].type = WM_BT_PROPERTY_ADAPTER_BONDED_DEVICES;
            adapter_props[num_props].len = bonded_devices.num_devices * sizeof(tls_bt_addr_t);
            adapter_props[num_props].val = devices_list;

            for(i = 0; i < bonded_devices.num_devices; i++) {
                wm_memcpy(devices_list + i, &bonded_devices.devices[i], sizeof(tls_bt_addr_t));
            }

            num_props++;
        }

        /* LOCAL UUIDs */
        BTIF_STORAGE_GET_ADAPTER_PROP(WM_BT_PROPERTY_UUIDS,
                                      local_uuids, sizeof(local_uuids),
                                      adapter_props[num_props]);
        num_props++;
        btif_adapter_properties_evt(TLS_BT_STATUS_SUCCESS, num_props, adapter_props);

        if(bonded_devices.num_devices > 0) {
            GKI_freebuf(devices_list);
        }
    }
    BTIF_TRACE_EVENT("%s: %d bonded devices found", __FUNCTION__, bonded_devices.num_devices);
    {
        for(i = 0; i < bonded_devices.num_devices; i++) {
            tls_bt_addr_t *p_remote_addr;
            num_props = 0;
            p_remote_addr = &bonded_devices.devices[i];
            wm_memset(remote_properties, 0, sizeof(remote_properties));
            BTIF_STORAGE_GET_REMOTE_PROP(p_remote_addr, WM_BT_PROPERTY_BDNAME,
                                         &name, sizeof(name),
                                         remote_properties[num_props]);
            num_props++;
            BTIF_STORAGE_GET_REMOTE_PROP(p_remote_addr, WM_BT_PROPERTY_REMOTE_FRIENDLY_NAME,
                                         &alias, sizeof(alias),
                                         remote_properties[num_props]);
            num_props++;
            BTIF_STORAGE_GET_REMOTE_PROP(p_remote_addr, WM_BT_PROPERTY_CLASS_OF_DEVICE,
                                         &cod, sizeof(cod),
                                         remote_properties[num_props]);
            num_props++;
            BTIF_STORAGE_GET_REMOTE_PROP(p_remote_addr, WM_BT_PROPERTY_TYPE_OF_DEVICE,
                                         &devtype, sizeof(devtype),
                                         remote_properties[num_props]);
            num_props++;
            BTIF_STORAGE_GET_REMOTE_PROP(p_remote_addr, WM_BT_PROPERTY_UUIDS,
                                         remote_uuids, sizeof(remote_uuids),
                                         remote_properties[num_props]);
            num_props++;
            btif_remote_properties_evt(TLS_BT_STATUS_SUCCESS, p_remote_addr,
                                       num_props, remote_properties);
        }
    }
    return TLS_BT_STATUS_SUCCESS;
}

#if (BLE_INCLUDED == TRUE)

/*******************************************************************************
**
** Function         btif_storage_add_ble_bonding_key
**
** Description      BTIF storage API - Adds the newly bonded device to NVRAM
**                  along with the ble-key, Key type and Pin key length
**
** Returns          BT_STATUS_SUCCESS if the store was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/

tls_bt_status_t btif_storage_add_ble_bonding_key(tls_bt_addr_t *remote_bd_addr,
        char *key,
        uint8_t key_type,
        uint8_t key_length)
{
    bdstr_t bdstr;
    bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr_t));
    const char *name;

    switch(key_type) {
        case BTIF_DM_LE_KEY_PENC:
            name = "LE_KEY_PENC";
            break;

        case BTIF_DM_LE_KEY_PID:
            name = "LE_KEY_PID";
            break;

        case BTIF_DM_LE_KEY_PCSRK:
            name = "LE_KEY_PCSRK";
            break;

        case BTIF_DM_LE_KEY_LENC:
            name = "LE_KEY_LENC";
            break;

        case BTIF_DM_LE_KEY_LCSRK:
            name = "LE_KEY_LCSRK";
            break;

        default:
            return TLS_BT_STATUS_FAIL;
    }

    int ret = btif_config_set("Remote", bdstr, name, (const char *)key, (int)key_length,
                              BTIF_CFG_TYPE_BIN);
    return ret ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         btif_storage_get_ble_bonding_key
**
** Description
**
** Returns          BT_STATUS_SUCCESS if the fetch was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_get_ble_bonding_key(tls_bt_addr_t *remote_bd_addr,
        uint8_t key_type,
        char *key_value,
        int key_length)
{
    bdstr_t bdstr;
    bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr_t));
    const char *name;
    int type = BTIF_CFG_TYPE_BIN;

    switch(key_type) {
        case BTIF_DM_LE_KEY_PENC:
            name = "LE_KEY_PENC";
            break;

        case BTIF_DM_LE_KEY_PID:
            name = "LE_KEY_PID";
            break;

        case BTIF_DM_LE_KEY_PCSRK:
            name = "LE_KEY_PCSRK";
            break;

        case BTIF_DM_LE_KEY_LENC:
            name = "LE_KEY_LENC";
            break;

        case BTIF_DM_LE_KEY_LCSRK:
            name = "LE_KEY_LCSRK";
            break;

        default:
            return TLS_BT_STATUS_FAIL;
    }

    int ret = btif_config_get("Remote", bdstr, name, key_value, &key_length, &type);
    return ret ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         btif_storage_remove_ble_keys
**
** Description      BTIF storage API - Deletes the bonded device from NVRAM
**
** Returns          BT_STATUS_SUCCESS if the deletion was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_remove_ble_bonding_keys(tls_bt_addr_t *remote_bd_addr)
{
    bdstr_t bdstr;
    bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr_t));
    BTIF_TRACE_DEBUG(" %s in bd addr:%s", __FUNCTION__, bdstr);
    int ret = 1;
    int ret_remove = FALSE;

    if(btif_config_exist("Remote", bdstr, "LE_KEY_PENC")) {
        ret &= btif_config_remove("Remote", bdstr, "LE_KEY_PENC");
    }

    if(btif_config_exist("Remote", bdstr, "LE_KEY_PID")) {
        ret &= btif_config_remove("Remote", bdstr, "LE_KEY_PID");
    }

    if(btif_config_exist("Remote", bdstr, "LE_KEY_PCSRK")) {
        ret &= btif_config_remove("Remote", bdstr, "LE_KEY_PCSRK");
    }

    if(btif_config_exist("Remote", bdstr, "LE_KEY_LENC")) {
        ret &= btif_config_remove("Remote", bdstr, "LE_KEY_LENC");
    }

    if(btif_config_exist("Remote", bdstr, "LE_KEY_LCSRK")) {
        ret &= btif_config_remove("Remote", bdstr, "LE_KEY_LCSRK");
    }

    /*Remove this device*/
    ret_remove = btif_config_remove_remote(bdstr);

    if(ret_remove) {
        btif_config_save();
    }

    return ret ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         btif_storage_add_ble_local_key
**
** Description      BTIF storage API - Adds the ble key to NVRAM
**
** Returns          BT_STATUS_SUCCESS if the store was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_save()
{
    btif_config_save();
    return TLS_BT_STATUS_SUCCESS;
}
tls_bt_status_t btif_storage_flush(int force)
{
    btif_config_flush(force);
    return TLS_BT_STATUS_SUCCESS;
}

tls_bt_status_t btif_storage_add_ble_local_key(char *key,
        uint8_t key_type,
        uint8_t key_length)
{
    const char *name;

    switch(key_type) {
        case BTIF_DM_LE_LOCAL_KEY_IR:
            name = "LE_LOCAL_KEY_IR";
            break;

        case BTIF_DM_LE_LOCAL_KEY_IRK:
            name = "LE_LOCAL_KEY_IRK";
            break;

        case BTIF_DM_LE_LOCAL_KEY_DHK:
            name = "LE_LOCAL_KEY_DHK";
            break;

        case BTIF_DM_LE_LOCAL_KEY_ER:
            name = "LE_LOCAL_KEY_ER";
            break;

        default:
            return TLS_BT_STATUS_FAIL;
    }

    //hci_dbg_hexstring(name,key,key_length);
    int ret = btif_config_set("Local", "Adapter", name, (const char *)key, key_length,
                              BTIF_CFG_TYPE_BIN);
    return ret ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         btif_storage_get_ble_local_key
**
** Description
**
** Returns          BT_STATUS_SUCCESS if the fetch was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_get_ble_local_key(uint8_t key_type,
        char *key_value,
        int key_length)
{
    const char *name;
    int type = BTIF_CFG_TYPE_BIN;

    switch(key_type) {
        case BTIF_DM_LE_LOCAL_KEY_IR:
            name = "LE_LOCAL_KEY_IR";
            break;

        case BTIF_DM_LE_LOCAL_KEY_IRK:
            name = "LE_LOCAL_KEY_IRK";
            break;

        case BTIF_DM_LE_LOCAL_KEY_DHK:
            name = "LE_LOCAL_KEY_DHK";
            break;

        case BTIF_DM_LE_LOCAL_KEY_ER:
            name = "LE_LOCAL_KEY_ER";
            break;

        default:
            return TLS_BT_STATUS_FAIL;
    }

    int ret = btif_config_get("Local", "Adapter", name, key_value, &key_length, &type);
    return ret ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         btif_storage_remove_ble_local_keys
**
** Description      BTIF storage API - Deletes the bonded device from NVRAM
**
** Returns          BT_STATUS_SUCCESS if the deletion was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_remove_ble_local_keys(void)
{
    int ret = 1;

    if(btif_config_exist("Local", "Adapter", "LE_LOCAL_KEY_IR")) {
        ret &= btif_config_remove("Local", "Adapter", "LE_LOCAL_KEY_IR");
    }

    if(btif_config_exist("Local", "Adapter", "LE_LOCAL_KEY_IRK")) {
        ret &= btif_config_remove("Local", "Adapter", "LE_LOCAL_KEY_IRK");
    }

    if(btif_config_exist("Local", "Adapter", "LE_LOCAL_KEY_DHK")) {
        ret &= btif_config_remove("Local", "Adapter", "LE_LOCAL_KEY_DHK");
    }

    if(btif_config_exist("Local", "Adapter", "LE_LOCAL_KEY_ER")) {
        ret &= btif_config_remove("Local", "Adapter", "LE_LOCAL_KEY_ER");
    }

    btif_config_save();
    return ret ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}

tls_bt_status_t btif_in_fetch_bonded_ble_device(char *remote_bd_addr, int add,
        btif_bonded_devices_t *p_bonded_devices)
{
    int device_type;
    int addr_type;
    tls_bt_addr_t bd_addr;
    BD_ADDR bta_bd_addr;
    bool device_added = false;
    bool key_found = false;

    if(!btif_config_get_int("Remote", remote_bd_addr, "DevType", &device_type)) {
        return TLS_BT_STATUS_FAIL;
    }

    if((device_type & BT_DEVICE_TYPE_BLE) == BT_DEVICE_TYPE_BLE) {
        printf(">>>%s Found a LE device: %s", __func__, remote_bd_addr);
        string_to_bdaddr(remote_bd_addr, &bd_addr);
        bdcpy(bta_bd_addr, bd_addr.address);

        if(btif_storage_get_remote_addr_type(&bd_addr, &addr_type) != TLS_BT_STATUS_SUCCESS) {
            addr_type = BLE_ADDR_PUBLIC;
            btif_storage_set_remote_addr_type(&bd_addr, BLE_ADDR_PUBLIC, 1);
        }

        btif_read_le_key(BTIF_DM_LE_KEY_PENC, sizeof(tBTM_LE_PENC_KEYS),
                         bd_addr, addr_type, add, &device_added, &key_found);
        btif_read_le_key(BTIF_DM_LE_KEY_PID, sizeof(tBTM_LE_PID_KEYS),
                         bd_addr, addr_type, add, &device_added, &key_found);
        btif_read_le_key(BTIF_DM_LE_KEY_LID, sizeof(tBTM_LE_PID_KEYS),
                         bd_addr, addr_type, add, &device_added, &key_found);
        btif_read_le_key(BTIF_DM_LE_KEY_PCSRK, sizeof(tBTM_LE_PCSRK_KEYS),
                         bd_addr, addr_type, add, &device_added, &key_found);
        btif_read_le_key(BTIF_DM_LE_KEY_LENC, sizeof(tBTM_LE_LENC_KEYS),
                         bd_addr, addr_type, add, &device_added, &key_found);
        btif_read_le_key(BTIF_DM_LE_KEY_LCSRK, sizeof(tBTM_LE_LCSRK_KEYS),
                         bd_addr, addr_type, add, &device_added, &key_found);

        // Fill in the bonded devices
        if(device_added) {
            memcpy(&p_bonded_devices->devices[p_bonded_devices->num_devices++], &bd_addr,
                   sizeof(tls_bt_addr_t));
            btif_gatts_add_bonded_dev_from_nv(bta_bd_addr);
        }

        if(key_found) {
            return TLS_BT_STATUS_SUCCESS;
        }
    }

    return TLS_BT_STATUS_FAIL;
}

tls_bt_status_t btif_storage_set_remote_addr_type(tls_bt_addr_t *remote_bd_addr,
        uint8_t addr_type, uint8_t wr_flash)
{
    bdstr_t bdstr;
    bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr_t));
    int ret = btif_config_set_int("Remote", bdstr, "AddrType", (int)addr_type);

    if(wr_flash) {
        btif_config_save();
    }

    return ret ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}
tls_bt_status_t btif_storage_set_remote_device_type(tls_bt_addr_t *remote_bd_addr,
        uint8_t dev_type, uint8_t wr_flash)
{
    bdstr_t bdstr;
    bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr_t));
    int ret = btif_config_set_int("Remote", bdstr, "DevType", (int)dev_type);

    if(wr_flash) {
        btif_config_save();
    }

    return ret ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}
tls_bt_status_t btif_storage_get_remote_device_type(tls_bt_addr_t *remote_bd_addr,
        int *dev_type)
{
    bdstr_t bdstr;
    bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr_t));
    int ret = btif_config_get_int("Remote", bdstr, "DevType", dev_type);
    return ret ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         btif_storage_get_remote_addr_type
**
** Description      BTIF storage API - Fetches the remote addr type
**
** Returns          BT_STATUS_SUCCESS if the fetch was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_get_remote_addr_type(tls_bt_addr_t *remote_bd_addr,
        int *addr_type)
{
    bdstr_t bdstr;
    bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr_t));
    int ret = btif_config_get_int("Remote", bdstr, "AddrType", addr_type);
    return ret ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}
#endif
/*******************************************************************************
**
** Function         btif_storage_add_hid_device_info
**
** Description      BTIF storage API - Adds the hid information of bonded hid devices-to NVRAM
**
** Returns          BT_STATUS_SUCCESS if the store was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/

tls_bt_status_t btif_storage_add_hid_device_info(tls_bt_addr_t *remote_bd_addr,
        uint16_t attr_mask, uint8_t sub_class,
        uint8_t app_id, uint16_t vendor_id,
        uint16_t product_id, uint16_t version,
        uint8_t ctry_code, uint16_t ssr_max_latency,
        uint16_t ssr_min_tout, uint16_t dl_len, uint8_t *dsc_list)
{
    bdstr_t bdstr;
    BTIF_TRACE_DEBUG("btif_storage_add_hid_device_info:");
    bd2str(remote_bd_addr, &bdstr);
    btif_config_set_int("Remote", bdstr, "HidAttrMask", attr_mask);
    btif_config_set_int("Remote", bdstr, "HidSubClass", sub_class);
    btif_config_set_int("Remote", bdstr, "HidAppId", app_id);
    btif_config_set_int("Remote", bdstr, "HidVendorId", vendor_id);
    btif_config_set_int("Remote", bdstr, "HidProductId", product_id);
    btif_config_set_int("Remote", bdstr, "HidVersion", version);
    btif_config_set_int("Remote", bdstr, "HidCountryCode", ctry_code);
    btif_config_set_int("Remote", bdstr, "HidSSRMaxLatency", ssr_max_latency);
    btif_config_set_int("Remote", bdstr, "HidSSRMinTimeout", ssr_min_tout);

    if(dl_len > 0)
        btif_config_set("Remote", bdstr, "HidDescriptor", (const char *)dsc_list, dl_len,
                        BTIF_CFG_TYPE_BIN);

    btif_config_save();
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_storage_load_bonded_hid_info
**
** Description      BTIF storage API - Loads hid info for all the bonded devices from NVRAM
**                  and adds those devices  to the BTA_HH.
**
** Returns          BT_STATUS_SUCCESS if successful, BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_load_bonded_hid_info(void)
{
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_storage_remove_hid_info
**
** Description      BTIF storage API - Deletes the bonded hid device info from NVRAM
**
** Returns          BT_STATUS_SUCCESS if the deletion was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_remove_hid_info(tls_bt_addr_t *remote_bd_addr)
{
    bdstr_t bdstr;
    bd2str(remote_bd_addr, &bdstr);
    btif_config_remove("Remote", bdstr, "HidAttrMask");
    btif_config_remove("Remote", bdstr, "HidSubClass");
    btif_config_remove("Remote", bdstr, "HidAppId");
    btif_config_remove("Remote", bdstr, "HidVendorId");
    btif_config_remove("Remote", bdstr, "HidProductId");
    btif_config_remove("Remote", bdstr, "HidVersion");
    btif_config_remove("Remote", bdstr, "HidCountryCode");
    btif_config_remove("Remote", bdstr, "HidSSRMaxLatency");
    btif_config_remove("Remote", bdstr, "HidSSRMinTimeout");
    btif_config_remove("Remote", bdstr, "HidDescriptor");
    btif_config_save();
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_storage_read_hl_apps_cb
**
** Description      BTIF storage API - Read HL application control block from NVRAM
**
** Returns          BT_STATUS_SUCCESS if the operation was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_read_hl_apps_cb(char *value, int value_size)
{
    tls_bt_status_t bt_status = TLS_BT_STATUS_SUCCESS;
    int read_size = value_size, read_type = BTIF_CFG_TYPE_BIN;

    if(!btif_config_exist("Local", BTIF_STORAGE_HL_APP, BTIF_STORAGE_HL_APP_CB)) {
        wm_memset(value, 0, value_size);

        if(!btif_config_set("Local", BTIF_STORAGE_HL_APP, BTIF_STORAGE_HL_APP_CB,
                            value, value_size, BTIF_CFG_TYPE_BIN)) {
            bt_status = TLS_BT_STATUS_FAIL;
        } else {
            btif_config_save();
        }
    } else {
        if(!btif_config_get("Local", BTIF_STORAGE_HL_APP, BTIF_STORAGE_HL_APP_CB,
                            value, &read_size, &read_type)) {
            bt_status = TLS_BT_STATUS_FAIL;
        } else {
            if((read_size != value_size) || (read_type != BTIF_CFG_TYPE_BIN)) {
                BTIF_TRACE_ERROR("%s  value_size=%d read_size=%d read_type=%d",
                                 __FUNCTION__, value_size, read_size, read_type);
                bt_status = TLS_BT_STATUS_FAIL;
            }
        }
    }

    BTIF_TRACE_DEBUG("%s  status=%d value_size=%d", __FUNCTION__, bt_status, value_size);
    return bt_status;
}


/*******************************************************************************
**
** Function         btif_storage_load_autopair_device_list
**
** Description      BTIF storage API - Populates auto pair device list
**
** Returns          BT_STATUS_SUCCESS if the auto pair blacklist is successfully populated
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_load_autopair_device_list()
{
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_storage_is_device_autopair_blacklisted
**
** Description      BTIF storage API  Checks if the given device is blacklisted for auto pairing
**
** Returns          TRUE if the device is found in the auto pair blacklist
**                  FALSE otherwise
**
*******************************************************************************/
uint8_t  btif_storage_is_device_autopair_blacklisted(tls_bt_addr_t *remote_bd_addr)
{
    char *token;
    bdstr_t bdstr;
    char *dev_name_str;
    char value[BTIF_STORAGE_MAX_LINE_SZ];
    int value_size = sizeof(value);
    bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr_t));
    /* Consider only  Lower Address Part from BD Address */
    bdstr[8] = '\0';

    if(btif_config_get_str("Local", BTIF_STORAGE_PATH_AUTOPAIR_BLACKLIST,
                           BTIF_STORAGE_KEY_AUTOPAIR_BLACKLIST_ADDR, value, &value_size)) {
        if(strstr(value, bdstr) != NULL) {
            return TRUE;
        }
    }

    dev_name_str = BTM_SecReadDevName((remote_bd_addr->address));

    if(dev_name_str != NULL) {
        value_size = sizeof(value);

        if(btif_config_get_str("Local", BTIF_STORAGE_PATH_AUTOPAIR_BLACKLIST,
                               BTIF_STORAGE_KEY_AUTOPAIR_BLACKLIST_EXACTNAME, value, &value_size)) {
            if(strstr(value, dev_name_str) != NULL) {
                return TRUE;
            }
        }

        value_size = sizeof(value);

        if(btif_config_get_str("Local", BTIF_STORAGE_PATH_AUTOPAIR_BLACKLIST,
                               BTIF_STORAGE_KEY_AUTOPAIR_BLACKLIST_PARTIALNAME, value, &value_size)) {
            token = strtok((char *)value, (const char *)BTIF_AUTO_PAIR_CONF_VALUE_SEPARATOR);

            while(token != NULL) {
                if(strstr(dev_name_str, token) != NULL) {
                    return TRUE;
                }

                token = strtok(NULL, BTIF_AUTO_PAIR_CONF_VALUE_SEPARATOR);
            }
        }
    }

    if(btif_config_get_str("Local", BTIF_STORAGE_PATH_AUTOPAIR_BLACKLIST,
                           BTIF_STORAGE_KEY_AUTOPAIR_DYNAMIC_BLACKLIST_ADDR, value, &value_size)) {
        if(strstr(value, bdstr) != NULL) {
            return TRUE;
        }
    }

    return FALSE;
}

/*******************************************************************************
**
** Function         btif_storage_add_device_to_autopair_blacklist
**
** Description      BTIF storage API - Add a remote device to the auto pairing blacklist
**
** Returns          BT_STATUS_SUCCESS if the device is successfully added to the auto pair blacklist
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_add_device_to_autopair_blacklist(tls_bt_addr_t *remote_bd_addr)
{
#if 0
    int ret;
    bdstr_t bdstr;
    char linebuf[BTIF_STORAGE_MAX_LINE_SZ + 20];
    char input_value [20];
    bd2str(remote_bd_addr, &bdstr);
    strlcpy(input_value, (char *)bdstr, sizeof(input_value));
    strlcat(input_value, BTIF_AUTO_PAIR_CONF_VALUE_SEPARATOR, sizeof(input_value));
    int line_size = sizeof(linebuf);

    if(btif_config_get_str("Local", BTIF_STORAGE_PATH_AUTOPAIR_BLACKLIST,
                           BTIF_STORAGE_KEY_AUTOPAIR_DYNAMIC_BLACKLIST_ADDR, linebuf, &line_size)) {
        /* Append this address to the dynamic List of BD address  */
        strncat(linebuf, input_value, BTIF_STORAGE_MAX_LINE_SZ);
    } else {
        strncpy(linebuf, input_value, BTIF_STORAGE_MAX_LINE_SZ);
    }

    /* Write back the key value */
    ret = btif_config_set_str("Local", BTIF_STORAGE_PATH_AUTOPAIR_BLACKLIST,
                              BTIF_STORAGE_KEY_AUTOPAIR_DYNAMIC_BLACKLIST_ADDR, linebuf);
    return ret ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
#endif
    return TLS_BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_storage_is_fixed_pin_zeros_keyboard
**
** Description      BTIF storage API - checks if this device has fixed PIN key device list
**
** Returns          TRUE   if the device is found in the fixed pin keyboard device list
**                  FALSE otherwise
**
*******************************************************************************/
uint8_t btif_storage_is_fixed_pin_zeros_keyboard(tls_bt_addr_t *remote_bd_addr)
{
//    int ret;
    bdstr_t bdstr;
//    char *dev_name_str;
//    uint8_t i = 0;
    char linebuf[BTIF_STORAGE_MAX_LINE_SZ];
    bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr_t));
    /*consider on LAP part of BDA string*/
    bdstr[8] = '\0';
    int line_size = sizeof(linebuf);

    if(btif_config_get_str("Local", BTIF_STORAGE_PATH_AUTOPAIR_BLACKLIST,
                           BTIF_STORAGE_KEY_AUTOPAIR_FIXPIN_KBLIST, linebuf, &line_size)) {
        if(strstr(linebuf, bdstr) != NULL) {
            return TRUE;
        }
    }

    return FALSE;
}

/*******************************************************************************
**
** Function         btif_storage_set_dmt_support_type
**
** Description      Sets DMT support status for a remote device
**
** Returns          BT_STATUS_SUCCESS if config update is successful
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/

tls_bt_status_t btif_storage_set_dmt_support_type(const tls_bt_addr_t *remote_bd_addr,
        uint8_t dmt_supported)
{
    int ret;
    bdstr_t bdstr = {0};

    if(remote_bd_addr) {
        bd2str(remote_bd_addr, &bdstr);
    } else {
        BTIF_TRACE_ERROR("%s  NULL BD Address", __FUNCTION__);
        return TLS_BT_STATUS_FAIL;
    }

    ret = btif_config_set_int("Remote", bdstr, "DMTSupported", (int)dmt_supported);
    return ret ? TLS_BT_STATUS_SUCCESS : TLS_BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         btif_storage_is_dmt_supported_device
**
** Description      checks if a device supports Dual mode topology
**
** Returns         TRUE if remote address is valid and supports DMT else FALSE
**
*******************************************************************************/

uint8_t btif_storage_is_dmt_supported_device(const tls_bt_addr_t *remote_bd_addr)
{
    int    dmt_supported = 0;
    bdstr_t bdstr = {0};

    if(remote_bd_addr) {
        bd2str(remote_bd_addr, &bdstr);
    }

    if(remote_bd_addr) {
        bd2str(remote_bd_addr, &bdstr);
    } else {
        BTIF_TRACE_ERROR("%s  NULL BD Address", __FUNCTION__);
        return FALSE;
    }

    btif_config_get_int("Remote", bdstr, "DMTSupported", &dmt_supported);
    return dmt_supported == 1 ? TRUE : FALSE;
}
/*******************************************************************************
 * Device information
 *******************************************************************************/

uint8_t btif_get_device_type(tls_bt_addr_t *bd_addr, int *addr_type, int *device_type)
{
    if(device_type == NULL || addr_type == NULL) {
        hci_dbg_msg("why return trur\r\n");
        return FALSE;
    }

    char bd_addr_str[18] = {0};
    bdaddr_to_string(bd_addr, bd_addr_str, sizeof(bdstr_t));

    if(!btif_config_get_int("Remote", bd_addr_str, "DevType", device_type)) {
        return FALSE;
    }

    if(!btif_config_get_int("Remote", bd_addr_str, "AddrType", addr_type)) {
        return FALSE;
    }

    //ALOGD("%s: Device [%s] type %d, addr. type %d", __FUNCTION__, bd_addr_str, *device_type, *addr_type);
    return TRUE;
}


