/**
 * @file    wm_config.h
 *
 * @brief   w800 chip inferface configure
 *
 * @author  dave
 *
 * @copyright (c) 2014 Winner Microelectronics Co., Ltd.
 */
#ifndef __WM_CONFIG_H__
#define __WM_CONFIG_H__
#include <csi_config.h>

#define	CFG_ON											1
#define CFG_OFF											0

#define WM_CONFIG_DEBUG_UART1							CFG_OFF/*PRINTF PORT USE UART1*/
/**Driver Support**/
#define TLS_CONFIG_HS_SPI          						CFG_ON /*High Speed SPI*/
#define TLS_CONFIG_LS_SPI          						CFG_ON /*Low Speed SPI*/
#define TLS_CONFIG_UART									CFG_ON  /*UART*/

/**Only Factory Test At Command**/
#define TLS_CONFIG_ONLY_FACTORY_ATCMD                   CFG_OFF

/**Host Interface&Command**/
#define TLS_CONFIG_HOSTIF 								CFG_ON
#define TLS_CONFIG_AT_CMD								(CFG_ON && TLS_CONFIG_HOSTIF)
#define TLS_CONFIG_RI_CMD								(CFG_ON && TLS_CONFIG_HOSTIF)
#define TLS_CONFIG_RMMS									(CFG_ON && TLS_CONFIG_HOSTIF)

//LWIP CONFIG
#define TLS_CONFIG_IPV4                 				CFG_ON      //must ON
#define TLS_CONFIG_IPV6                 				CFG_OFF
#define TLS_CONFIG_DHCP_OPTION60						"Winnermicro:W800_01"

/** SOCKET CONFIG **/
#define TLS_CONFIG_SOCKET_STD							CFG_ON
#define TLS_CONFIG_SOCKET_RAW							CFG_ON
#define TLS_CONFIG_CMD_USE_RAW_SOCKET                   (CFG_ON && TLS_CONFIG_SOCKET_RAW)
#define TLS_CONFIG_CMD_NET_USE_LIST_FTR                     CFG_ON



#define	TLS_CONFIG_HARD_CRYPTO							CFG_ON

#define TLS_CONFIG_NTO                                  CFG_ON
#define TLS_CONFIG_CRYSTAL_24M                          CFG_OFF

/** HTTP CLIENT **/
/*
HTTP Lib
HTTPS Lib
SSL LIB
CRYPTO
*/
#define TLS_CONFIG_HTTP_CLIENT							(CFG_ON)
#define TLS_CONFIG_HTTP_CLIENT_PROXY					CFG_OFF
#define TLS_CONFIG_HTTP_CLIENT_AUTH_BASIC				CFG_OFF
#define TLS_CONFIG_HTTP_CLIENT_AUTH_DIGEST				CFG_OFF
#define TLS_CONFIG_HTTP_CLIENT_AUTH						(TLS_CONFIG_HTTP_CLIENT_AUTH_BASIC || TLS_CONFIG_HTTP_CLIENT_AUTH_DIGEST)
#define TLS_CONFIG_HTTP_CLIENT_SECURE					(CFG_ON && (TLS_CONFIG_USE_POLARSSL || TLS_CONFIG_USE_MBEDTLS))
#define TLS_CONFIG_HTTP_CLIENT_TASK						(CFG_ON && TLS_CONFIG_HTTP_CLIENT)

/*MatrixSSL will be used except one of the following two Macros is CFG_ON*/
#define TLS_CONFIG_USE_POLARSSL           				CFG_OFF
#define TLS_CONFIG_USE_MBEDTLS           				CFG_ON

#define TLS_CONFIG_SERVER_SIDE_SSL                      (CFG_ON && TLS_CONFIG_HTTP_CLIENT_SECURE && TLS_CONFIG_USE_MBEDTLS)         /*MUST configure TLS_CONFIG_HTTP_CLIENT_SECURE CFG_ON */


/**IGMP**/
#define TLS_CONFIG_IGMP            				        CFG_ON


#define TLS_CONFIG_NTP 									CFG_ON

#if NIMBLE_FTR
#define TLS_CONFIG_BLE                                  CFG_ON
#define TLS_CONFIG_BR_EDR								CFG_OFF
#else
#define TLS_CONFIG_BLE                                  CFG_OFF
#define TLS_CONFIG_BR_EDR								CFG_ON
#endif

#define TLS_CONFIG_BT                                  (TLS_CONFIG_BR_EDR || TLS_CONFIG_BLE)

#include "wm_os_config.h"  //if you want to use source code,please open
#include "wm_wifi_config.h"

#include "wm_ram_config.h"
#endif /*__WM_CONFIG_H__*/

