/**
 * @file    wm_ble_gatt.h
 *
 * @brief   Bluetooth API
 *
 * @author  WinnerMicro
 *
 * Copyright (c) 2020 Winner Microelectronics Co., Ltd.
 */
#ifndef WM_BLE_GATT_H
#define WM_BLE_GATT_H

#include "wm_bt_def.h"

/**
 * @defgroup BT_APIs Bluetooth APIs
 * @brief Bluetooth related APIs
 */

/**
 * @addtogroup BT_APIs
 * @{
 */

/**
 * @defgroup BLE_GATT_Client_APIs BLE GATT Client APIs
 * @brief BLE GATT Client APIs
 */

/**
 * @addtogroup BLE_GATT_Client_APIs
 * @{
 */

/**
 * @brief          initialize the btif_gatt_client callback function
 *
 * @param[in]      *p_callback      pointer on callback function
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_app_init(tls_ble_callback_t callback);

/**
 * @brief          free the tls_ble_callback_t pointer
 *
 * @param          None
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_app_deinit(void);

/**
 * @brief          this function is called to register client application
 *
 * @param[in]      *uuid      pointer on uuid
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_app_register(tls_bt_uuid_t *uuid);

/**
 * @brief          this function is called to unregister client application
 *
 * @param[in]      client_if      gatt client access interface
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_app_unregister(uint8_t client_if);

/**
 * @brief          this function is called to open an BLE connection  to a remote
 *                 device or add a background auto connection
 *
 * @param[in]      client_if        gatt client access interface
 * @param[in]      *bd_addr         remote device bluetooth device address
 * @param[in]      is_direct        direct connection or background auto connection
 * @param[in]      transport        specific BLE/BR-EDR/mixed
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_connect(uint8_t client_if, const tls_bt_addr_t *bd_addr, uint8_t is_direct, int transport);

/**
 * @brief          this function is called to disconnect with gatt server connection
 *
 * @param[in]      client_if        gatt client access interface
 * @param[in]      *bd_addr         remote device bluetooth device address
 * @param[in]      conn_id          connection ID to be closed
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_disconnect(uint8_t client_if, const tls_bt_addr_t *bd_addr,  int conn_id);

/**
 * @brief          start or stop advertisements to listen for incoming connections
 *
 * @param[in]      client_if        gatt client access interface
 * @param[in]      start            start: 1; stop 0
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_listen(uint8_t client_if, uint8_t start);

/**
 * @brief          clear the attribute cache for a given device
 *
 * @param[in]      client_if        gatt client access interface
 * @param[in]      *bd_addr         remote device address
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_refresh(uint8_t client_if, const tls_bt_addr_t *bd_addr);

/**
 * @brief          enumerate all GATT services on a connected device
 *
 * @param[in]      conn_id          connection indicator return value when connected
 * @param[in]      *filter_uuid     filter this uuid
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           Optionally, the results can be filtered for a given UUID
 */
tls_bt_status_t tls_ble_client_search_service(uint16_t conn_id, tls_bt_uuid_t *filter_uuid);

/**
 * @brief          write a remote characteristic
 *
 * @param[in]      conn_id        connection indicator return value when connected
 * @param[in]      handle         the character attribute handle
 * @param[in]      write_type     the type of attribute write operation
 * @param[in]      len            length of the value to be written
 * @param[in]      auth_req       authentication request
 * @param[in]      *p_value       the value to be written
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_write_characteristic(uint16_t conn_id, uint16_t handle, int write_type, int len, int auth_req, char *p_value);

/**
 * @brief          read a characteristic on a remote device
 *
 * @param[in]      conn_id        connection indicator return value when connected
 * @param[in]      handle         the character attribute handle
 * @param[in]      auth_req       authentication request
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_read_characteristic(uint16_t conn_id, uint16_t handle, int auth_req);

/**
 * @brief          read the descriptor for a given characteristic
 *
 * @param[in]      conn_id        connection indicator return value when connected
 * @param[in]      handle         the character attribute handle
 * @param[in]      auth_req       authentication request
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_read_descriptor(uint16_t conn_id, uint16_t handle, int auth_req);

/**
 * @brief          write a remote descriptor for a given characteristic
 *
 * @param[in]      conn_id        connection indicator return value when connected
 * @param[in]      handle         the character attribute handle
 * @param[in]      write_type     the type of attribute write operation
 * @param[in]      len            length of the value to be written
 * @param[in]      auth_req       authentication request
 * @param[in]      *p_value       the value to be written
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_write_descriptor(uint16_t conn_id, uint16_t handle, int write_type, int len, int auth_req, char *p_value);

/**
 * @brief          execute a prepared write operation
 *
 * @param[in]      conn_id        connection indicator return value when connected
 * @param[in]      execute        execute or cancel
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_execute_write(uint16_t conn_id, int execute);

/**
 * @brief          Register to receive notifications or indications for a given
 *                 characteristic
 *
 * @param[in]      client_if        gatt client access interface
 * @param[in]      *bd_addr         the target server address
 * @param[in]      handle           the attribute handle of characteristic
 * @param[in]     conn_id          the connection id 
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_register_for_notification(int client_if, const tls_bt_addr_t *bd_addr, uint16_t handle, uint16_t conn_id);

/**
 * @brief          deregister a previous request for notifications/indications
 *
 * @param[in]      client_if        gatt client access interface
 * @param[in]      *bd_addr         the target server address
 * @param[in]      handle           the attribute handle of characteristic
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_deregister_for_notification(int client_if, const tls_bt_addr_t *bd_addr, uint16_t handle,uint16_t conn_id);

/**
 * @brief          configure the MTU for a given connection
 *
 * @param[in]      conn_id      connection indicator return value when connected
 * @param[in]      mtu          the max transmit unit of this connection
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_configure_mtu(uint16_t conn_id, uint16_t mtu);

/**
 * @brief          get gatt db content
 *
 * @param[in]      conn_id      connection indicator return value when connected
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_client_get_gatt_db(uint16_t conn_id);

/**
 * @}
 */



/**
 * @defgroup BLE_GATT_Server_APIs BLE GATT Server APIs
 * @brief BLE GATT Server APIs
 */

/**
 * @addtogroup BLE_GATT_Server_APIs
 * @{
 */

/**
 * @brief          initialize the btif_gatt_server callback function
 *
 * @param[in]      *p_callback      pointer on callback function
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_server_app_init(tls_ble_callback_t callback);

 /*******************************************************************************
 **
 ** Function         tls_ble_server_app_deinit
 **
 ** Description      free the tls_ble_callback_t pointer
 **
 ** Parameters       None
 **
 ** Returns          TLS_BT_STATUS_SUCCESS
 **                  TLS_BT_STATUS_DONE
 **       
 *******************************************************************************/
tls_bt_status_t tls_ble_server_app_deinit();

/**
 * @brief          this function is called to register server application
 *
 * @param[in]      *uuid      pointer on uuid
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_server_app_register(tls_bt_uuid_t *uuid);

/**
 * @brief          this function is called to unregister server application
 *
 * @param[in]      server_if      assigned after app registering
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_server_app_unregister(uint8_t server_if);

/**
 * @brief          create a new service
 *
 * @param[in]      server_if        the gatt server access interface created by app register
 * @param[in]      inst_id          instance identifier of this service
 * @param[in]      primay           is primary or not service
 * @param[in]      *uuid            the id property of this service
 * @param[in]      num_handles      number of handle requested for this service
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_server_add_service(uint8_t server_if, int inst_id, int primay, tls_bt_uuid_t *uuid, int num_handles);

/**
 * @brief          add a characteristic to a service
 *
 * @param[in]      server_if        the gatt server access interface created by app register
 * @param[in]      service_handle   the handle of this service assigned when creating a service
 * @param[in]      *uuid            the id property of this characteristic
 * @param[in]      properties       access properties
 * @param[in]      permission       access permission
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_server_add_characteristic(uint8_t server_if, uint16_t service_handle, tls_bt_uuid_t *uuid, int properties, int permission);

/**
 * @brief          add a descriptor to a given service
 *
 * @param[in]      server_if        the gatt server access interface created by app register
 * @param[in]      service_handle   the handle of this service assigned when creating a service
 * @param[in]      *uuid            the id property of this characteristic
 * @param[in]      permission       access permission
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_server_add_descriptor(uint8_t server_if, uint16_t service_handle, tls_bt_uuid_t *uuid, int permissions);

/**
 * @brief          starts a local service
 *
 * @param[in]      server_if        the gatt server access interface created by app register
 * @param[in]      service_handle   the handle of this service assigned when creating a service
 * @param[in]      transport        tranport type, BLE/BR-EDR/MIXED
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_server_start_service(uint8_t server_if, uint16_t service_handle, int transport);

/**
 * @brief          stop a local service
 *
 * @param[in]      server_if        the gatt server access interface created by app register
 * @param[in]      service_handle   the handle of this service assigned when creating a service
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_server_stop_service(uint8_t server_if, uint16_t service_handle);

/**
 * @brief          delete a local service
 *
 * @param[in]      server_if        the gatt server access interface created by app register
 * @param[in]      service_handle   the handle of this service assigned when creating a service
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_server_delete_service(uint8_t server_if, uint16_t service_handle);

/**
 * @brief          create a connection to a remote peripheral
 *
 * @param[in]      server_if        the gatt server access interface created by app register
 * @param[in]      *bd_addr         the remote device address
 * @param[in]      is_direct        true direct connection; false: background auto connection
 * @param[in]      transport        tranport type, BLE/BR-EDR/MIXED
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_server_connect(uint8_t server_if, const tls_bt_addr_t *bd_addr, uint8_t is_direct, int transport);

/**
 * @brief          disconnect an established connection or cancel a pending one
 *
 * @param[in]      server_if        the gatt server access interface created by app register
 * @param[in]      *bd_addr         the remote device address
 * @param[in]      conn_id          connection id create when connection established
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_server_disconnect(uint8_t server_if, const tls_bt_addr_t *bd_addr, uint16_t conn_id);

/**
 * @brief          send value indication to a remote device
 *
 * @param[in]      server_if            the gatt server access interface created by app register
 * @param[in]      attribute_handle     the handle of characteristic
 * @param[in]      conn_id              connection id create when connection established
 * @param[in]      len                  the length of value to be sent
 * @param[in]      confirm              need the remote device acked after receive the message , normally
 *                                      Whether a confirmation is required. FALSE sends a GATT notification, 
 *                                      TRUE sends a GATT indication
 * @param[in]      *p_value             the value to be written
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_server_send_indication(uint8_t server_if, uint16_t attribute_handle, uint16_t conn_id, int len, int confirm, char *p_value);

/**
 * @brief          send a response to a read/write operation
 *
 * @param[in]      conn_id          connection id create when connection established
 * @param[in]      trans_id         the transation identifier
 * @param[in]      status           TODO:
 * @param[in]      offset           the offset the fragmented value
 * @param[in]      attr_handle      the attribute handle
 * @param[in]      auth_req         access properties
 * @param[in]      *p_value         the value to be written
 * @param[in]      len              the length of value to be written
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_server_send_response(uint16_t conn_id, uint32_t trans_id, uint8_t status, int offset, uint16_t attr_handle, int auth_req, uint8_t *p_value, int len);

/**
 * @}
 */

/**
 * @}
 */

#endif /* WM_BLE_GATT_H */

