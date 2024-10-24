/**
 * @file    wm_bt_spp.h
 *
 * @brief   Bluetooth API
 *
 * @author  WinnerMicro
 *
 * Copyright (c) 2020 Winner Microelectronics Co., Ltd.
 */

#ifndef __WM_BT_SPP_H__
#define __WM_BT_SPP_H__

#include "wm_bt.h"

/**
 * @defgroup BT_APIs Bluetooth APIs
 * @brief Bluetooth related APIs
 */

/**
 * @addtogroup BT_APIs
 * @{
 */

/**
 * @defgroup BT_SPP_APIs
 * @brief BT_SPP APIs
 */

/**
 * @addtogroup BT_SPP_APIs
 * @{
 */

/**spp realed api*/
/**
 * @brief          Initializes the SPP interface
 *
 * @param[in]     callback      pointer on callback function
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_spp_init(tls_bt_spp_callback_t callback);

/**
 * @brief          Shuts down the SPP interface and does the cleanup
 *
 * @param       None
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_spp_deinit(void);

/**
 * @brief          Enable the bta jv interface
 *
 * @param[in]     None
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_spp_enable(void);

/**
 * @brief          Disable the bta jv interface and cleanup internal resource
 *
 * @param[in]     None
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */

tls_bt_status_t tls_bt_spp_disable(void);


/**
 * @brief          Discovery the spp service by the given peer device.
 *
 * @param[in]      *bd_addr         remote device bluetooth device address
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */

tls_bt_status_t tls_bt_spp_start_discovery(tls_bt_addr_t *bd_addr, tls_bt_uuid_t *uuid);

/**
 * @brief          Create a spp connection to the remote device 
 * 
 * @param[in]   sec_mask:     Security Setting Mask
 * @param[in]   role:         Server or client
 * @param[in]   remote_scn:   Remote device bluetooth device SCN
 * @param[in]      *bd_addr         remote device bluetooth device address
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_spp_connect(wm_spp_sec_t sec_mask,
                          tls_spp_role_t role, uint8_t remote_scn, tls_bt_addr_t *bd_addr);

/**
 * @brief          Close a spp connection
 *
 * @param[in]   handle:    The connection handle
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_spp_disconnect(uint32_t handle);

/**
 * @brief       This function create a SPP server and starts listening for an
 *              SPP connection request from a remote Bluetooth device
 *
 * @param[in]   sec_mask:     Security Setting Mask .
 * @param[in]   role:         Server or client.
 * @param[in]   local_scn:    The specific channel you want to get.
 *                            If channel is 0, means get any channel.
 * @param[in]   name:         Server's name.
 *
 * @retval         @ref tls_bt_status_t

 */
tls_bt_status_t tls_bt_spp_start_server(wm_spp_sec_t sec_mask,
                            tls_spp_role_t role, uint8_t local_scn, const char *name);

/**
 * @brief       This function is used to write data
 *
 * @param[in]   handle: The connection handle.
 * @param[in]   len:    The length of the data written.
 * @param[in]   p_data: The data written.
 *
 * @retval         @ref tls_bt_status_t

 */
tls_bt_status_t tls_bt_spp_write(uint32_t handle, uint8_t *p_data, int length);

#endif


