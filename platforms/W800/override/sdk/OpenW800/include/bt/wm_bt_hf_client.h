/**
 * @file    wm_bt_hf_client.h
 *
 * @brief   Bluetooth API
 *
 * @author  WinnerMicro
 *
 * Copyright (c) 2020 Winner Microelectronics Co., Ltd.
 */


#ifndef __WM_BT_HF_CLIENT_H__
#define __WM_BT_HF_CLIENT_H__

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
 * @defgroup BT_HF_CLIENT_APIs
 * @brief BT_HF_CLIENT APIs
 */

/**
 * @addtogroup BT_HF_CLIENT_APIs
 * @{
 */



/**
 * @brief		   initializes the hf client interface
 *
 * @param[in]	  callback		pointer on callback function
 *
 * @retval		   @ref tls_bt_status_t
 *
 * @note		   None
 */
tls_bt_status_t tls_bt_hf_client_init(tls_bthf_client_callback_t callback);

/**
 * @brief          Closes the HF client interface
 *
 * @param       None
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_hf_client_deinit(void);

/**
 * @brief          connect to audio gateway
 *
 * @param[in]      *bd_addr         remote device bluetooth device address
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_hf_client_connect(tls_bt_addr_t *bd_addr);


/**
 * @brief          disconnect from audio gateway
 *
 * @param[in]      *bd_addr         remote device bluetooth device address
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */

tls_bt_status_t tls_bt_hf_client_disconnect(tls_bt_addr_t *bd_addr);

/**
 * @brief           create an audio connection
 *
 * @param[in]      *bd_addr         remote device bluetooth device address
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_hf_client_connect_audio(tls_bt_addr_t *bd_addr);

/**
 * @brief           close the audio connection
 *
 * @param[in]      *bd_addr         remote device bluetooth device address
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_hf_client_disconnect_audio(tls_bt_addr_t *bd_addr);

/**
 * @brief           start voice recognition
 *
 * @param[in]    None
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_hf_client_start_voice_recognition(void);

/**
 * @brief           stop voice recognition
 *
 * @param[in]    None
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_hf_client_stop_voice_recognition(void);

/**
 * @brief           volume control
 *
 * @param[in]    type Mic or speaker
 * @param[in]    volume index value
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_hf_client_volume_control(tls_bthf_client_volume_type_t type, int volume);

/**
 * @brief           place a call
 *
 * @param[in]    number  phone number to be called
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_hf_client_dial(const char *number);

/**
 * @brief          place a call with number specified by location (speed dial)
 *
 * @param[in]    location
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_hf_client_dial_memory(int location);

/**
 * @brief           handle specified call related action
 *
 * @param[in]    action  call action
 * @param[in]    idx      index indicator
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_hf_client_handle_call_action(tls_bthf_client_call_action_t action, int idx);

/**
 * @brief           query list of current calls
 *
 * @param[in]    None
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */

tls_bt_status_t tls_bt_hf_client_query_current_calls(void);

/**
 * @brief           query current selected operator name
 *
 * @param[in]    None
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_hf_client_query_current_operator_name(void);

/**
 * @brief            retrieve subscriber number information
 *
 * @param[in]    None
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */

tls_bt_status_t tls_bt_hf_client_retrieve_subscriber_info(void);

/**
 * @brief            send dtmf
 *
 * @param[in]    code   number code
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */

tls_bt_status_t tls_bt_hf_client_send_dtmf(char code);

/**
 * @brief            Request number from AG for VR purposes
 *
 * @param[in]    None
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */

tls_bt_status_t tls_bt_hf_client_request_last_voice_tag_number(void);

/**
 * @brief             Send requested AT command to remote device
 *
 * @param[in]    cmd
 * @param[in]    val1
 * @param[in]    val2
 * @param[in]    arg
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */

tls_bt_status_t tls_bt_hf_client_send_at_cmd(int cmd, int val1, int val2, const char *arg);

/**
 * @brief             Send audio to audio gateway
 *
 * @param[in]    bd_addr    bluetooth address of audio gateway
 * @param[in]    p_data      audio data
 * @param[in]    length       audio length
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */

tls_bt_status_t tls_bt_hf_client_send_audio(tls_bt_addr_t *bd_addr, uint8_t *p_data, uint16_t length);
#endif
