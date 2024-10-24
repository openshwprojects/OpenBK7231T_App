/**
 * @file    wm_bt.h
 *
 * @brief   Bluetooth API
 *
 * @author  WinnerMicro
 *
 * Copyright (c) 2020 Winner Microelectronics Co., Ltd.
 */
#ifndef WM_BT_H
#define WM_BT_H

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
 * @defgroup BT_Host_APIs BT Host APIs
 * @brief BT Host APIs
 */

/**
 * @addtogroup BT_Host_APIs
 * @{
 */


/**
 * @brief          reply the pin request
 *
 * @param[in]      *bd_addr         remote device address
 * @param[in]      accept
 * @param[in]      pin_len
 * @param[in]      *pin_code
 *
 * @return		@ref tls_bt_status_t

 *
 * @note           None
 */

tls_bt_status_t tls_bt_pin_reply(const tls_bt_addr_t *bd_addr, uint8_t accept,
                     uint8_t pin_len, tls_bt_pin_code_t *pin_code);

/**
 * @brief          reply the ssp request
 *
 * @param[in]      *bd_addr         remote device address
 * @param[in]      variant           @ref tls_bt_ssp_variant_t
 * @param[in]      accept
 * @param[in]      passkey
 *
 * @return		@ref tls_bt_status_t
 *
 * @note           None
 */

tls_bt_status_t tls_bt_ssp_reply(const tls_bt_addr_t *bd_addr, tls_bt_ssp_variant_t variant,
                     uint8_t accept, uint32_t passkey);

/**
 * @brief          set the adapter property
 *
 * @param[in]      *property         remote device address
 * @param[in]      update_to_flash  save the property to flash or not
 *
 * @return	       @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_set_adapter_property(const tls_bt_property_t *property, uint8_t update_to_flash);

/**
 * @brief          get the adapter property
 *
 * @param[in]      type         @ref tls_bt_property_type_t
 *
 * @return	       @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_get_adapter_property(tls_bt_property_type_t type);

/**
 * @brief          
 *
 * @param          None
 *
 * @return         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_start_discovery(void);

/**
 * @brief          
 *
 * @param          None
 *
 * @return         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_cancel_discovery(void);

/**
 * @brief          
 *
 * @param[in]      *bd_addr
 * @param[in]      transport
 *
 * @return	       @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_create_bond(const tls_bt_addr_t *bd_addr, int transport);

/**
 * @brief          
 *
 * @param[in]      *bd_addr
 *
 * @return	       @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_cancel_bond(const tls_bt_addr_t *bd_addr);

/**
 * @brief          
 *
 * @param[in]      *bd_addr
 *
 * @return	       @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_remove_bond(const tls_bt_addr_t *bd_addr);

/**
 * @brief          
 *
 * @param          None
 *
 * @return         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_host_cleanup(void);

/**
 * @brief          
 *
 * @param[in]      callback
 * @param[in]      *p_hci_if
 * @param[in]      log_level
 *
 * @return	       @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_enable(tls_bt_host_callback_t callback, tls_bt_hci_if_t *p_hci_if, tls_bt_log_level_t log_level);

/**
 * @brief          
 *
 * @param          None
 *
 * @return         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_disable();


/**
 * @}
 */



/**
 * @defgroup BT_Controller_APIs BT Controller APIs
 * @brief BT Controller APIs
 */

/**
 * @addtogroup BT_Controller_APIs
 * @{
 */

/**
 * @brief          enable the bluetooth controller stack
 *
 * @param[in]      *p_hci_if     pointer on uart property
 * @param[in]       log_level    @ref tls_bt_log_level_t
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_ctrl_enable(tls_bt_hci_if_t *p_hci_if, tls_bt_log_level_t log_level);


/**
 * @brief          disable the bluetooth controller stack
 *
 * @param          None
 *
 * @return         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_ctrl_disable(void);

/**
 * @brief          configure the ble emit power of different ble handle type
 *
 * @param[in]      power_type     @ref tls_ble_power_type_t
 * @param[in]      power_level_index    [1,2,3,4,5] map to[1,4,7,10,13]dBm
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           power_type, supports TLS_BLE_PWR_TYPE_DEFAULT only. 
 */
tls_bt_status_t tls_ble_set_tx_power(tls_ble_power_type_t power_type, int8_t power_level_index);

/**
 * @brief          get the ble emit power of different ble handle type
 *
 * @param[in]      power_type     @ref tls_ble_power_type_t
 *
 * @retval         power value db
 *
 * @note           power_type, supports TLS_BLE_PWR_TYPE_DEFAULT only. 
 */
int8_t  tls_ble_get_tx_power(tls_ble_power_type_t power_type);

/**
 * @brief          configure the classic/enhanced bluetooth transmit power
 *
 * @param[in]      min_power_level    power level[1,13]dBm
 * @param[in]      max_power_level    power level[1,13]dBm
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bredr_set_tx_power(int8_t min_power_level,int8_t max_power_level);

/**
 * @brief          get the classic/enhanced bluetooth transmit power level
 *
 * @param[in]      *min_power_level    pointer on min_power_level
 * @param[in]      *max_power_level    pointer on max_power_level
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t  tls_bredr_get_tx_power(int8_t* min_power_level, int8_t* max_power_level);

/**
 * @brief          configure the voice output path
 *
 * @param[in]      data_path    @ref tls_sco_data_path_t
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bredr_sco_datapath_set(tls_sco_data_path_t data_path);

/**
 * @brief          get controller stack status
 *
 * @param          None
 *
 * @retval         @ref tls_bt_ctrl_status_t
 *
 * @note           None
 */
tls_bt_ctrl_status_t tls_bt_controller_get_status(void);

/**
 * @brief          this function receive the hci message from host hci_h4 inteface
 *
 * @param[in]      *data    hci formated message
 * @param[in]       len     command length
 *
 * @retval         @ref tls_bt_ctrl_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_vuart_host_send_packet( uint8_t *data, uint16_t len);

/**
 * @brief          this function register the host stack receive message function 
 *                 and indication the controller receive hci command avaiable
 *
 * @param[in]      *p_host_if       @ref tls_bt_host_if_t
 *
 * @retval         @ref tls_bt_ctrl_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_ctrl_if_register(const tls_bt_host_if_t *p_host_if);

/**
 * @brief          this function unregister the host stack receive message function 
 *                 and indication the controller receive hci command avaiable
 *
 * @param     None
 *
 * @retval         @ref tls_bt_ctrl_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_ctrl_if_unregister();


/**
 * @brief          this function configure the controller enter into sleep mode when controller
 *                 is in idle mode
 *
 * @param[in]      enable       TRUE:  enable
 *                              FALSE: didsable
 *
 * @retval         @ref tls_bt_ctrl_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_ctrl_sleep(bool enable);

/**
 * @brief          this function look up the controller is in sleep mode or not
 *
 * @param          None
 *
 * @retval         TRUE:  sleep mode
 *                 FALSE: not sleep mode
 *
 * @note           None
 */
bool  tls_bt_ctrl_is_sleep(void);

/**
 * @brief          this function wake up the controller, in other words exit sleep mode
 *
 * @param          None
 *
 * @retval         @ref tls_bt_ctrl_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_ctrl_wakeup(void);

/**
 * @brief          this function check controller can handle hci commands yes or no
 *
 * @param          None
 *
 * @retval         @ref bool TRUE or FALSE
 *
 * @note           None
 */

bool tls_bt_vuart_host_check_send_available();

/**
 * @brief          this function exit bluetooth test mode
 *
 * @param          None
 *
 * @retval         @ref tls_bt_ctrl_status_t
 *
 * @note           None
 */
tls_bt_status_t exit_bt_test_mode();

/**
 * @brief          this function enable bluetooth test mode
 *
 * @param[in]       p_hci_if, specific the uart port property
 *
 * @retval         @ref tls_bt_ctrl_status_t
 *
 * @note           None
 */

tls_bt_status_t enable_bt_test_mode(tls_bt_hci_if_t *p_hci_if);

/**
 * @brief          this function enable rf to bluetooth mode
 *
 * @param[in]       1, bluetooth mode, 0 wifi/bluetooth mode
 *
 * @retval         None
 *
 * @note           None
 */

void tls_rf_bt_mode(uint8_t enable);

/**
 * @brief          this function enable controller to running in mesh mode or not
 *
 * @param[in]       1, mesh mode, 0 normal mode
 *
 * @retval         None
 *
 * @note           None
 */

tls_bt_status_t tls_bt_set_mesh_mode(uint8_t enable);

/**
 * @brief          this function register callback function when controller entering or exiting sleep mode 
 *
 * @param[in]      sleep_enter, sleep starting callback;sleep_exit, sleep exiting callback
 *
 * @retval         TLS_BT_STATUS_SUCCESS or TLS_BT_STATUS_UNSUPPORTED;
 *
 * @note           None
 */

tls_bt_status_t tls_bt_register_sleep_callback(tls_bt_controller_sleep_enter_func_ptr sleep_enter, tls_bt_controller_sleep_exit_func_ptr sleep_exit);

/**
 * @brief          this function register blocking operation function(eg. flash read/write).
 *
 * @param[in]      process_ptr blocking operation function pointer
 *
 * @retval         always TLS_BT_STATUS_SUCCESS;
 *
 * @note           if the function is running, the system interrupt will be holded. so we have to do ti when bt is in idle state
 */

tls_bt_status_t tls_bt_register_pending_process_callback(tls_bt_app_pending_process_func_ptr process_ptr);

/**
 * @}
 */

/**
 * @}
 */

#endif /* WM_BT_H */

