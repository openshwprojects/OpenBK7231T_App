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

#ifndef BTIF_STORAGE_H
#define BTIF_STORAGE_H

#include "bt_types.h"


/*******************************************************************************
**  Constants & Macros
********************************************************************************/
#define BTIF_STORAGE_FILL_PROPERTY(p_prop, t, l, p_v) \
    (p_prop)->type = t;(p_prop)->len = l; (p_prop)->val = (p_v);

#define  BTIF_STORAGE_MAX_ALLOWED_REMOTE_DEVICE 512

/*******************************************************************************
**  Functions
********************************************************************************/

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
tls_bt_status_t btif_storage_get_adapter_property(tls_bt_property_t *property);

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
tls_bt_status_t btif_storage_set_adapter_property(tls_bt_property_t *property);

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
        tls_bt_property_t *property);

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
        tls_bt_property_t *property);

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
        tls_bt_property_t *properties);

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
        uint8_t pin_length);

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
tls_bt_status_t btif_storage_remove_bonded_device(tls_bt_addr_t *remote_bd_addr);

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
tls_bt_status_t btif_storage_load_bonded_devices(void);

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
tls_bt_status_t btif_storage_read_hl_apps_cb(char *value, int value_size);

/*******************************************************************************
**
** Function         btif_storage_write_hl_apps_cb
**
** Description      BTIF storage API - Write HL application control block to NVRAM
**
** Returns          BT_STATUS_SUCCESS if the operation was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_write_hl_apps_cb(char *value, int value_size);

/*******************************************************************************
**
** Function         btif_storage_read_hl_apps_cb
**
** Description      BTIF storage API - Read HL application configuration from NVRAM
**
** Returns          BT_STATUS_SUCCESS if the operation was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_read_hl_app_data(uint8_t app_idx, char *value, int value_size);

/*******************************************************************************
**
** Function         btif_storage_write_hl_app_data
**
** Description      BTIF storage API - Write HL application configuration to NVRAM
**
** Returns          BT_STATUS_SUCCESS if the operation was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_write_hl_app_data(uint8_t app_idx, char *value, int value_size);

/*******************************************************************************
**
** Function         btif_storage_read_hl_mdl_data
**
** Description      BTIF storage API - Read HL application MDL configuration from NVRAM
**
** Returns          BT_STATUS_SUCCESS if the operation was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_read_hl_mdl_data(uint8_t app_idx, char *value, int value_size);

/*******************************************************************************
**
** Function         btif_storage_write_hl_mdl_data
**
** Description      BTIF storage API - Write HL application MDL configuration from NVRAM
**
** Returns          BT_STATUS_SUCCESS if the operation was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
tls_bt_status_t btif_storage_write_hl_mdl_data(uint8_t app_idx, char *value, int value_size);

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
        uint16_t ssr_min_tout, uint16_t dl_len, uint8_t *dsc_list);

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
tls_bt_status_t btif_storage_load_bonded_hid_info(void);

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
tls_bt_status_t btif_storage_remove_hid_info(tls_bt_addr_t *remote_bd_addr);

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
tls_bt_status_t btif_storage_load_autopair_device_list();

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

uint8_t  btif_storage_is_device_autopair_blacklisted(tls_bt_addr_t *remote_bd_addr);

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

tls_bt_status_t btif_storage_add_device_to_autopair_blacklist(tls_bt_addr_t *remote_bd_addr);

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
uint8_t btif_storage_is_fixed_pin_zeros_keyboard(tls_bt_addr_t *remote_bd_addr);

#if (BLE_INCLUDED == TRUE)
tls_bt_status_t btif_storage_add_ble_bonding_key(tls_bt_addr_t *remote_bd_addr,
        char *key,
        uint8_t key_type,
        uint8_t key_length);
tls_bt_status_t btif_storage_get_ble_bonding_key(tls_bt_addr_t *remote_bd_addr,
        uint8_t key_type,
        char *key_value,
        int key_length);

tls_bt_status_t btif_storage_add_ble_local_key(char *key,
        uint8_t key_type,
        uint8_t key_length);
tls_bt_status_t btif_storage_remove_ble_bonding_keys(tls_bt_addr_t *remote_bd_addr);
tls_bt_status_t btif_storage_remove_ble_local_keys(void);
tls_bt_status_t btif_storage_get_ble_local_key(uint8_t key_type,
        char *key_value,
        int key_len);

tls_bt_status_t btif_storage_get_remote_addr_type(tls_bt_addr_t *remote_bd_addr,
        int *addr_type);

tls_bt_status_t btif_storage_set_remote_addr_type(tls_bt_addr_t *remote_bd_addr,
        uint8_t addr_type, uint8_t wr_flash);

tls_bt_status_t btif_storage_set_remote_device_type(tls_bt_addr_t *remote_bd_addr,
        uint8_t dev_type, uint8_t wr_flash);
tls_bt_status_t btif_storage_get_remote_device_type(tls_bt_addr_t *remote_bd_addr,
        int *dev_type);

#endif
/*******************************************************************************
**
** Function         btif_storage_get_remote_version
**
** Description      Fetch remote version info on cached remote device
**
** Returns          BT_STATUS_SUCCESS if found
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/

tls_bt_status_t btif_storage_get_remote_version(const tls_bt_addr_t *remote_bd_addr,
        bt_remote_version_t *p_ver);

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
        uint8_t dmt_supported);



/*******************************************************************************
**
** Function         btif_storage_is_dmt_supported_device
**
** Description      checks if a device supports Dual mode topology
**
** Returns         TRUE if remote supports DMT else FALSE
**
*******************************************************************************/

uint8_t btif_storage_is_dmt_supported_device(const tls_bt_addr_t *remote_bd_addr);

uint8_t btif_get_device_type(tls_bt_addr_t *bd_addr, int *addr_type, int *device_type);

uint8_t btif_storage_update_remote_devices(const tls_bt_addr_t *remote_bd_addr);

#endif /* BTIF_STORAGE_H */
