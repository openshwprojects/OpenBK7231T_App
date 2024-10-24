/**
 * @file    wm_wifi_oneshot.h
 *
 * @brief   Wi-Fi OneShot
 *
 * @author  winnermicro
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */

#ifndef WM_WIFI_ONESHOT_H
#define WM_WIFI_ONESHOT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wm_type_def.h>
#if (GCC_COMPILE==1)
#include "wm_ieee80211_gcc.h"
#else
#include <wm_ieee80211.h>
#endif
#include "wm_config.h"
#include "wm_bt_config.h"

/**   DEBUG USE MAC FILTER START   */
#define CONFIG_ONESHOT_MAC_FILTER               0
extern int tls_filter_module_srcmac(u8 *mac);



#define ONESHOT_ON                              1
#define ONESHOT_OFF                             0

/* ONE SHOT */
/** UDP MULTICAST ONE SHOT */
#define TLS_CONFIG_UDP_ONE_SHOT          ONESHOT_ON

/** WinnerMicro ONSHOT */
#define TLS_CONFIG_UDP_LSD_SPECIAL	 	(ONESHOT_ON&& TLS_CONFIG_UDP_ONE_SHOT)

/** AP ONESHOT */
#define TLS_CONFIG_AP_MODE_ONESHOT      (ONESHOT_ON && TLS_CONFIG_AP)
#define TLS_CONFIG_WEB_SERVER_MODE      (ONESHOT_ON && TLS_CONFIG_AP_MODE_ONESHOT)
#define TLS_CONFIG_SOCKET_MODE          (ONESHOT_ON && TLS_CONFIG_AP_MODE_ONESHOT)


/** AIRKISS ONESHOT */
#define TLS_CONFIG_AIRKISS_MODE_ONESHOT (ONESHOT_OFF && TLS_CONFIG_UDP_ONE_SHOT)
#define AIRKISS_USE_SELF_WRITE                 1


/** BLE ONESHOT */
#define TLS_CONFIG_BLE_WIFI_ONESHOT  (ONESHOT_ON && (WM_BLE_INCLUDED == CFG_ON || WM_NIMBLE_INCLUDED == CFG_ON))

typedef enum{
	ONESHOT_SCAN_START,
	ONESHOT_SCAN_FINISHED,
	ONESHOT_SWITCH_CHANNEL,
	ONESHOT_STOP_TMP_CHAN_SWITCH,	
	ONESHOT_STOP_CHAN_SWITCH,
	ONESHOT_HANDSHAKE_TIMEOUT,
	ONESHOT_RECV_TIMEOUT,	
	ONESHOT_RECV_ERR,
	ONESHOT_STOP_DATA_CLEAR,
	ONESHOT_NET_UP,
	AP_SOCK_S_MSG_SOCKET_RECEIVE_DATA,
	AP_WEB_S_MSG_RECEIVE_DATA,
	AP_SOCK_S_MSG_SOCKET_CREATE,
	AP_SOCK_S_MSG_WJOIN_FAILD,	
}ONESHOT_MSG_ENUM;

/**
 * @defgroup APP_APIs APP APIs
 * @brief APP APIs
 */

/**
 * @addtogroup APP_APIs
 * @{
 */

/**
 * @defgroup Oneshot_APIs Oneshot APIs
 * @brief Wi-Fi oneshot APIs
 */

/**
 * @addtogroup Oneshot_APIs
 * @{
 */

/**
 * @brief		  This function is used to set oneshot flag.
 *
 * @param[in]	  flag, 0: one shot closed
 *                      1: one shot open
 *                      2: AP+socket
 *                      3: AP+WEBSERVER
 *                      4: bt
 *
 * @param[out]    None
 * 
 * @retval	      0: success
 *                  -1: failed
 *
 * @note		  None
 */
int tls_wifi_set_oneshot_flag(u8 flag);

/**
 * @brief		  This function is used to get oneshot flag.
 *
 * @param[in]	  None 
 *
 * @param[out]    None
 * 
 * @retval	      0: one shot closed
 *                1: one shot open
 *                2: AP+socket
 *                3: AP+WEBSERVER
 *                4: bt
 *
 * @note		  None
 */
int tls_wifi_get_oneshot_flag(void);

/**
 * @}
 */

/**
 * @}
 */

/**
 * @brief          Handle wild packets coming from the air.
 *
 * @param[in]      *hdr     point to ieee80211 data header
 * @param[in]      data_len data len of ieee80211 data
 *
 * @retval         no mean
 *
 * @note           None
 */
u8 tls_wifi_dataframe_recv(struct ieee80211_hdr *hdr, u32 data_len);

#if TLS_CONFIG_AIRKISS_MODE_ONESHOT
/**
 * @brief          This function is used to acknowledge app when airkiss process is done.
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
void oneshot_airkiss_send_reply(void);
/**
 * @brief          This function is used to deal with airkiss's
 				   wild packet
 *
 * @param[in]      *data       ieee80211 packet
 * @param[in]      data_len    packet length
 *
 * @return         None
 *
 * @note           None
 */
void tls_airkiss_recv(u8 *data, u16 data_len);
/**
 * @brief          This function is used to start airkiss
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
void tls_airkiss_start(void);
/**
 * @brief          This function is used to stop airkiss
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
void tls_airkiss_stop(void);

/**
 * @brief          This function is used to change channel for airkiss
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
void tls_oneshot_airkiss_change_channel(void);
#endif /*TLS_CONFIG_AIRKISS_MODE_ONESHOT*/

/**
 * @brief          This function is used to init oneshot task
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           Not in use now
 */
int wm_oneshot_task_init(void);

/**
 * @brief          This function is used to stop oneshot timer
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
void tls_oneshot_switch_channel_tim_stop(struct ieee80211_hdr *hdr);

/**
 * @brief          This function is used to stop oneshot temp timer
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
void tls_oneshot_switch_channel_tim_temp_stop(void);

/**
 * @brief          handle if use bssid to connect wifi.
 *
 * @param[in]      *ssid      : ap name to connect
 * @param[in]      *ssid_len: ap name's length to connect
 * @param[in]      *bssid    : ap bssid
 *
 * @retval         no mean
 *
 * @note           None
 */
int tls_oneshot_if_use_bssid(u8 *ssid, u8 *ssid_len, u8 *bssid);

/**
 * @brief          Find channel according to ssid
 *
 * @param[in]      *ssid     ssid to be compared
 * @param[in]       ssid_len  ssid length
 * @param[out]     chlist    chlist to be add according to ssid info
 *
 * @retval          None
 *
 * @note           None
 */
void tls_oneshot_find_chlist(u8 *ssid, u8 ssid_len, u16 *chlist);

/**
 * @brief		  This function is to deal with oneshot event according netif status.
 *
 * @param[in]	  status:net status
 *
 * @param[out]     None
 *
 * @retval	        None
 *
 * @note		        None
 */
void wm_oneshot_netif_status_event(u8 status );

#if TLS_CONFIG_WEB_SERVER_MODE
/**
 * @brief		   This function is used to send web config msg to oneshot task.
 *
 * @param[in]	  None 
 *
 * @param[out]    None
 * 
 * @retval	   None
 *
 * @note		   None
 */
void tls_oneshot_send_web_connect_msg(void);
#endif

#endif /*WM_WIFI_ONESHOT_H*/

