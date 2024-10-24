/**
 * @file    wm_bt_av.h
 *
 * @brief   Bluetooth API
 *
 * @author  WinnerMicro
 *
 * Copyright (c) 2020 Winner Microelectronics Co., Ltd.
 */

#ifndef __WM_BT_A2DP_H__
#define __WM_BT_A2DP_H__

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
 * @defgroup BT_AV_APIs
 * @brief BT_AV APIs
 */

/**
 * @addtogroup BT_AV_APIs
 * @{
 */

/**sink realed api*/
/**
 * @brief          Initializes the AV interface for sink mode
 *
 * @param[in]     callback      pointer on callback function
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_av_sink_init(tls_bt_a2dp_sink_callback_t callback);

/**
 * @brief          Shuts down the AV sink interface and does the cleanup
 *
 * @param       None
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_av_sink_deinit(void);

/**
 * @brief          Establishes the AV signalling channel with the source
 *
 * @param[in]      *bd_addr         remote device bluetooth device address
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_av_sink_connect_src(tls_bt_addr_t *bd_addr);


/**
 * @brief          Tears down the AV signalling channel with the source side
 *
 * @param[in]      *bd_addr         remote device bluetooth device address
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_av_sink_disconnect(tls_bt_addr_t *bd_addr);

/**src realed api*/

/**
 * @brief          Initializes the AV interface for source mode
 *
 * @param[in]     callback      pointer on callback function
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_av_src_init(tls_bt_a2dp_src_callback_t callback);

/**
 * @brief          Shuts down the AV source interface and does the cleanup
 *
 * @param       None
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_av_src_deinit(void);

/**
 * @brief          Establishes the AV signalling channel with the sink
 *
 * @param[in]      *bd_addr         remote device bluetooth device address
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_av_src_connect_sink(tls_bt_addr_t *bd_addr);

/**
 * @brief          Tears down the AV signalling channel with the sink side
 *
 * @param[in]      *bd_addr         remote device bluetooth device address
 * 
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_bt_av_src_disconnect(tls_bt_addr_t *bd_addr);

/**btrc related api supported by now*/

/**
 * @brief          Initializes the AVRC interface
 *
 * @param[in]     callback      pointer on callback function
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_btrc_init(tls_btrc_callback_t callback);

/**
 * @brief          Closes the AVRC interface
 *
 * @param       None
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_btrc_deinit(void);

/**
 * @brief          Returns the current play status.
 *
 * @param[in]     tls_btrc_play_status_t      stopped, playing, paused...
 * @param[in]     song_len     seconds of the song 
 * @param[in]     song_pos    played seconds of the song
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           This method is called in response to GetPlayStatus request.
 */
tls_bt_status_t tls_btrc_get_play_status_rsp(tls_btrc_play_status_t play_status, uint32_t song_len,
                                       uint32_t song_pos);

/**
 * @brief          Returns the current songs' element attributes in text
 *
 * @param[in]     num_attr     counter of song`s element attributes
 * @param[in]     p_attrs     pointer of element attributes
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_btrc_get_element_attr_rsp(uint8_t num_attr, tls_btrc_element_attr_val_t *p_attrs);

/**
 * @brief          Response to the register notification request in text
 *
 * @param[in]     event_id      play_status, track or play_pos changed
 * @param[in]     type     notification type
 * @param[in]     p_param    pointer to details of notification structer
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_btrc_register_notification_rsp(tls_btrc_event_id_t event_id,
        tls_btrc_notification_type_t type, tls_btrc_register_notification_t *p_param);


/**
 * @brief          Send current volume setting to remote side
 *
 * @param[in]    volue      Should be in the range 0-127. bit7 is reseved and cannot be set
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           Support limited to SetAbsoluteVolume
 * 			 This can be enhanced to support Relative Volume (AVRCP 1.0).
 *                   With RelateVolume, we will send VOLUME_UP/VOLUME_DOWN
 *                   as opposed to absolute volume level
 */
tls_bt_status_t tls_btrc_set_volume(uint8_t volume);

/**btrc ctrl related api supported by now*/

/**
 * @brief          Initializes the AVRC ctrl interface
 *
 * @param[in]     callback      pointer on callback function
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_btrc_ctrl_init(tls_btrc_ctrl_callback_t callback);

/**
 * @brief          Closes the AVRC ctrl interface
 *
 * @param       None
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_btrc_ctrl_deinit(void);

/**
 * @brief           Send Pass-Through command
 *
 * @param[in]     bd_addr      remote device bluetooth device address
 * @param[in]     key_code    code definition of the key
 * @param[in]     key_state    key stae
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_btrc_ctrl_send_passthrough_cmd(tls_bt_addr_t *bd_addr, uint8_t key_code, uint8_t key_state);

/**
 * @brief           Send group  navigation command
 *
 * @param[in]     bd_addr      remote device bluetooth device address
 * @param[in]     key_code    code definition of the key
 * @param[in]     key_state    key stae
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_btrc_ctrl_send_groupnavigation_cmd(tls_bt_addr_t *bd_addr, uint8_t key_code, uint8_t key_state);

/**
 * @brief           Set current values of Player Attributes
 *
 * @param[in]     bd_addr        remote device bluetooth device address
 * @param[in]     num_attrib    couner of attributes
 * @param[in]     attrib_ids     atrribute of index indicator
  * @param[in]    attrib_vals    attribute of values
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_btrc_ctrl_change_player_app_setting(tls_bt_addr_t *bd_addr, uint8_t num_attrib, uint8_t *attrib_ids, uint8_t *attrib_vals);


/**
 * @brief          Rsp for SetAbsoluteVolume Command
 *
 * @param[in]     bd_addr        remote device bluetooth device address
 * @param[in]     abs_vol       the absolute volume
 * @param[in]     label           label indicator
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_btrc_ctrl_set_volume_rsp(tls_bt_addr_t *bd_addr, uint8_t abs_vol, uint8_t label);


/**
 * @brief          Rsp for Notification of Absolute Volume
 *
 * @param[in]     bd_addr        remote device bluetooth device address
 * @param[in]     rsp_type       interim or changed
  * @param[in]    abs_vol      the absolute volume value
 * @param[in]     label           label indicator
 *
 * @retval         @ref tls_bt_status_t
 *
 * @note           None
 */
tls_bt_status_t tls_btrc_ctrl_volume_change_notification_rsp(tls_bt_addr_t *bd_addr, tls_btrc_notification_type_t rsp_type,uint8_t abs_vol, uint8_t label);

#endif