/**
 * @file    wm_include.h
 *
 * @brief   the configuration file of sdk
 *
 * @author  winnermicro
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd.
 */
#ifndef __WM_INCLUDE_H__
#define __WM_INCLUDE_H__

/**
 * @mainpage WinnerMicro SDK
 *
 * Quick Start of WinnerMicro SDK.
 *
 *
 * HOW TO CODE ?
 *
 * Function UserMain(void) is the entrance function of the application:
 * @code
 * void UserMain(void)
 * {
 *     printf("\n user task\n");
 *
 * #if DEMO_CONSOLE
 *	   CreateDemoTask();
 * #endif
 *
 *     //user's task
 * }
 * @endcode
 *
 *
 * \n
 * HOW TO COMPILE ?
 *
 * To build with the SDK you can use the CDS or Make.
 * Please refer to the CDS and script compilation documentation for details.
 *
 *
 * \n
 * HOW TO DOWNLOAD THE FIRMWARE ?
 *
 * Download the "w800.fls" image
 *
 * This will download image which includes secboot & user application image into flash by ROM using xmodem-protocol for factory burning.
 * @code
 * Pulling down the bootmode pin(PA0) and reset the device. Then UART0 will output: 
 * CCC...
 * For details,please refer to the sdk manual.
 * @endcode
 *
 * Download the "w800.img" image
 *
 * This will download image which only includes user application using xmodem-protocol.
 * @code
 * Press "ESC" and then reset the device. Then UART0 will output:
 * CCC...
 * For details,please refer to the sdk manual.
 * @endcode
 *
 * \n
 */

#include <stdio.h>
#include <stdlib.h>
#include "wm_type_def.h"
#include "wm_uart.h"
#include "wm_gpio.h"
#include "wm_hostspi.h"
#include "wm_socket.h"
#include "wm_sockets.h"
#include "wm_wifi.h"
#include "wm_hspi.h"
#include "wm_pwm.h"
#include "wm_params.h"
#include "wm_osal.h"
#include "wm_netif.h"
#include "wm_efuse.h"
#include "wm_mem.h"
#include "wm_demo.h"
#include "wm_regs.h"


#endif
