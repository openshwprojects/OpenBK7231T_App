/**
 * @file    wm_ble.h
 *
 * @brief   Bluetooth API
 *
 * @author  WinnerMicro
 *
 * Copyright (c) 2020 Winner Microelectronics Co., Ltd.
 */
#ifndef WM_BLE_H
#define WM_BLE_H

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
 * @defgroup BLE_APIs BLE APIs
 * @brief BLE APIs
 */

/**
 * @addtogroup BLE_APIs
 * @{
 */

/**
 * @brief          initialize the application callback function
 *
 * @param[in]      *p_callback      pointer on callback function
 *
 * @return         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_dm_init(tls_ble_dm_callback_t callback);

/**
 * @brief          start/stop ble advertisement
 *
 * @param[in]      start      1 connectable and discoverable; 2 disconnectable and discoverable; 0 stop
 *
 * @return         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_adv(uint8_t adv_state);

/**
 * @brief          configure the advertisment content
 *
 * @param[in]      *data        @ref btif_dm_adv_data_t
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           if pure_data equals to true, the filed of manufacturer equals to all fileds of advetisement data.
 *                     otherwise, the filed manufacturer will be advertised in 0xFF filed. 
 *
 */
tls_bt_status_t tls_ble_set_adv_data(tls_ble_dm_adv_data_t *data);

/**
 * @brief          configure the advertisment parameters
 *
 * @param[in]      *param        @ref btif_dm_adv_param_t
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_set_adv_param(tls_ble_dm_adv_param_t *param);

/**
 * @brief          configure the advertisment extented parameters
 *
 * @param[in]      *param        @ref tls_ble_dm_adv_ext_param_t
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           if you know how to config all the parameters, you can use this function; otherwise, tls_ble_set_adv_param will be recommanded strongly;
 */
tls_bt_status_t tls_ble_set_adv_ext_param(tls_ble_dm_adv_ext_param_t *param);


/**
 * @brief          start/stop ble scan
 *
 * @param[in]      start        TRUE enable; FALSE disable
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_scan(bool start);

/**
 * @brief          configure the scan parameters
 *
 * @param[in]      window        scan window size
 * @param[in]      interval      scan interval length
 * @param[in]     scan mode    0 passive scan; 1 active scan;
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           interval should greater or equals to windows,
 *                 both range should be within (0x0004, 0x4000)
 */
tls_bt_status_t tls_ble_set_scan_param(int window, int interval, uint8_t scan_mode);

/**
 * @brief          enable a async process evt
 *
 * @param[in]      id               user specific definition
 * @param[in]      *p_callback      callback function
 *
 * @return         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_dm_evt_triger(int id, tls_ble_dm_triger_callback_t callback);

/**
 * @brief          configure the max transmit unit
 *
 * @param[in]      *bd_addr     the remote device address
 * @param[in]      length       range [27 - 251]
 *
 * @return         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_dm_set_data_length(tls_bt_addr_t *bd_addr, uint16_t length);

/**
 * @brief          configure the ble privacy
 *
 * @param[in]      enable   TRUE:  using rpa/random address, updated every 15 mins
 **                         FALSE: public address
 *
 * @return         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_dm_set_privacy(uint8_t enable);

/**
 * @brief          update the connection parameters
 *
 * @param[in]      *bd_addr         remote device address
 * @param[in]      min_interval
 * @param[in]      max_interval
 * @param[in]      latency
 * @param[in]      timeout
 *
 * @return         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_ble_conn_parameter_update(const tls_bt_addr_t *bd_addr, 
                                             int min_interval,
                                             int max_interval, 
                                             int latency, 
                                             int timeout);

/**
 * @brief          read the remote device signal strength connected
 *
 * @param[in]      *bd_addr         remote device address
 *
 * @return         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_dm_read_remote_rssi(const tls_bt_addr_t *bd_addr);


/**
 * @brief          config the io capabilities of local device
 *
 * @param[in]      io_cap        
 *
 * @return         @ref tls_bt_status_t
 *
 * @note           None
 */

tls_bt_status_t tls_ble_set_sec_io_cap(uint8_t io_cap);

/**
 * @brief          config the auth requirement of local device
 *
 * @param[in]      auth_req        
 *
 * @return         @ref tls_bt_status_t
 *
 * @note           None
 */

tls_bt_status_t tls_ble_set_sec_auth_req(uint8_t auth_req);

/**
 * @brief       This function is called to ensure that connection is
 *                  encrypted.  Should be called only on an open connection.
 *                  Typically only needed for connections that first want to
 *                  bring up unencrypted links, then later encrypt them.

 * @param[in]sec_act       - This is the security action to indicate
 *                                 what knid of BLE security level is required for
 *                                 the BLE link if the BLE is supported      
 * @param[in]bd_addr       - Address of the peer device
 * @ref tls_bt_status_t
 *
 * @note           None
 */

tls_bt_status_t tls_ble_set_sec(const tls_bt_addr_t *bd_addr, uint8_t sec_act);

/**
 * @brief          only used to start/stop ble advertisement
 *
 * @param[in]      start  1 start advertisement; 0 stop advertisement;
 * @param[in]     duration valid for start advertisement. 0 for forever, otherwise the last seconds of advertisement
 *
 * @return         @ref tls_bt_status_t
 *
 * @note           None
 */

tls_bt_status_t tls_ble_gap_adv(uint8_t start, int duration);

/**
 * @}
 */

/**
 * @}
 */
 
#endif /* WM_BLE_H */

