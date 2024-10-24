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

/*******************************************************************************
 *
 *  Filename:      btif_api.h
 *
 *  Description:   Main API header file for all BTIF functions accessed
 *                 from main bluetooth HAL. All HAL extensions will not
 *                 require headerfiles as they would be accessed through
 *                 callout/callins.
 *
 *******************************************************************************/

#ifndef BTIF_API_H
#define BTIF_API_H

#include "btif_common.h"
#include "btif_dm.h"

/*******************************************************************************
**  BTIF CORE API
********************************************************************************/

/*******************************************************************************
**
** Function         btif_init_bluetooth
**
** Description      Creates BTIF task and prepares BT scheduler for startup
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_init_bluetooth(void);

/*******************************************************************************
**
** Function         btif_enable_bluetooth
**
** Description      Performs chip power on and kickstarts OS scheduler
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_enable_bluetooth(void);

/*******************************************************************************
**
** Function         btif_disable_bluetooth
**
** Description      Inititates shutdown of Bluetooth system.
**                  Any active links will be dropped and device entering
**                  non connectable/discoverable mode
**
** Returns          void
**
*******************************************************************************/
tls_bt_status_t btif_disable_bluetooth(void);

/*******************************************************************************
**
** Function         btif_cleanup_bluetooth
**
** Description      Cleanup BTIF state.
**
**
** Returns          void
**
*******************************************************************************/
tls_bt_status_t btif_cleanup_bluetooth(void);

/*******************************************************************************
**
** Function         is_restricted_mode
**
** Description      Checks if BT was enabled in restriced mode. In restricted
**                  mode, bonds that are created are marked as temporary.
**                  These bonds persist until we leave restricted mode, at
**                  which point they will be deleted from the config. Also
**                  while in restricted mode, the user can access devices
**                  that are already paired before entering restricted mode,
**                  but they cannot remove any of these devices.
**
** Returns          uint8_t
**
*******************************************************************************/
uint8_t is_restricted_mode(void);

/*******************************************************************************
**
** Function         btif_get_adapter_properties
**
** Description      Fetches all local adapter properties
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_get_adapter_properties(void);

/*******************************************************************************
**
** Function         btif_get_adapter_property
**
** Description      Fetches property value from local cache
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_get_adapter_property(tls_bt_property_type_t type);

/*******************************************************************************
**
** Function         btif_set_adapter_property
**
** Description      Updates core stack with property value and stores it in
**                  local cache
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_set_adapter_property(const tls_bt_property_t *property,
        uint8_t update_to_flash);

/*******************************************************************************
**
** Function         btif_get_remote_device_property
**
** Description      Fetches the remote device property from the NVRAM
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_get_remote_device_property(tls_bt_addr_t *remote_addr,
        tls_bt_property_type_t type);

/*******************************************************************************
**
** Function         btif_get_remote_device_properties
**
** Description      Fetches all the remote device properties from NVRAM
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_get_remote_device_properties(tls_bt_addr_t *remote_addr);

/*******************************************************************************
**
** Function         btif_set_remote_device_property
**
** Description      Writes the remote device property to NVRAM.
**                  Currently, BT_PROPERTY_REMOTE_FRIENDLY_NAME is the only
**                  remote device property that can be set
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_set_remote_device_property(tls_bt_addr_t *remote_addr,
        const tls_bt_property_t *property);

/*******************************************************************************
**
** Function         btif_get_remote_service_record
**
** Description      Looks up the service matching uuid on the remote device
**                  and fetches the SCN and service_name if the UUID is found
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_get_remote_service_record(tls_bt_addr_t *remote_addr,
        tls_bt_uuid_t *uuid);


/*******************************************************************************
**  BTIF DM API
********************************************************************************/

/*******************************************************************************
**
** Function         btif_dm_start_discovery
**
** Description      Start device discovery/inquiry
**
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_start_discovery(void);

/*******************************************************************************
**
** Function         btif_dm_cancel_discovery
**
** Description      Cancels search
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_cancel_discovery(void);

/*******************************************************************************
**
** Function         btif_dm_create_bond
**
** Description      Initiate bonding with the specified device
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_create_bond(const tls_bt_addr_t *bd_addr, int transport);

/*******************************************************************************
**
** Function         btif_dm_create_bond_out_of_band
**
** Description      Initiate bonding with the specified device using OOB data.
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_create_bond_out_of_band(const tls_bt_addr_t *bd_addr, int transport,
        const bt_out_of_band_data_t *oob_data);

/*******************************************************************************
**
** Function         btif_dm_cancel_bond
**
** Description      Initiate bonding with the specified device
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_cancel_bond(const tls_bt_addr_t *bd_addr);

/*******************************************************************************
**
** Function         btif_dm_remove_bond
**
** Description      Removes bonding with the specified device
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_remove_bond(const tls_bt_addr_t *bd_addr);

/*******************************************************************************
**
** Function         btif_dm_get_connection_state
**
** Description      Returns whether the remote device is currently connected
**
** Returns          0 if not connected
**
*******************************************************************************/
uint16_t btif_dm_get_connection_state(const tls_bt_addr_t *bd_addr);

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
                                  uint8_t pin_len, tls_bt_pin_code_t *pin_code);

/*******************************************************************************
**
** Function         btif_dm_passkey_reply
**
** Description      BT SSP passkey reply
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_passkey_reply(const tls_bt_addr_t *bd_addr,
                                      uint8_t accept, uint32_t passkey);

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
                                  uint32_t passkey);

/*******************************************************************************
**
** Function         btif_dm_get_adapter_property
**
** Description      Queries the BTA for the adapter property
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_get_adapter_property(tls_bt_property_t *prop);

/*******************************************************************************
**
** Function         btif_dm_get_remote_services
**
** Description      Start SDP to get remote services
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_get_remote_service_record(tls_bt_addr_t *remote_addr,
        tls_bt_uuid_t *uuid);


/*******************************************************************************
**
** Function         btif_dm_get_remote_services
**
** Description      Start SDP to get remote services
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_get_remote_services(tls_bt_addr_t *remote_addr);

/*******************************************************************************
**
** Function         btif_dm_get_remote_services_by_transport
**
** Description      Start SDP to get remote services by transport
**
** Returns          bt_status_t
**
*******************************************************************************/
tls_bt_status_t btif_dm_get_remote_services_by_transport(tls_bt_addr_t *remote_addr, int transport);

/*******************************************************************************
**
** Function         btif_dut_mode_configure
**
** Description      Configure Test Mode - 'enable' to 1 puts the device in test mode and 0 exits
**                       test mode
**
** Returns          BT_STATUS_SUCCESS on success
**
*******************************************************************************/
tls_bt_status_t btif_dut_mode_configure(uint8_t enable);

/*******************************************************************************
**
** Function         btif_dut_mode_send
**
** Description     Sends a HCI Vendor specific command to the controller
**
** Returns          BT_STATUS_SUCCESS on success
**
*******************************************************************************/
tls_bt_status_t btif_dut_mode_send(uint16_t opcode, uint8_t *buf, uint8_t len);

/*******************************************************************************
**
** Function         btif_le_test_mode
**
** Description     Sends a HCI BLE Test command to the Controller
**
** Returns          BT_STATUS_SUCCESS on success
**
*******************************************************************************/
tls_bt_status_t btif_le_test_mode(uint16_t opcode, uint8_t *buf, uint8_t len);

/*******************************************************************************
**
** Function         btif_dm_read_energy_info
**
** Description     Reads the energy info from controller
**
** Returns          void
**
*******************************************************************************/
void btif_dm_read_energy_info();

/*******************************************************************************
**
** Function         btif_config_hci_snoop_log
**
** Description     enable or disable HCI snoop log
**
** Returns          BT_STATUS_SUCCESS on success
**
*******************************************************************************/
tls_bt_status_t btif_config_hci_snoop_log(uint8_t enable);

#endif /* BTIF_API_H */
