#ifndef __NEW_COMMON_H__
#define __NEW_COMMON_H__


#if WINDOWS

#include <stdlib.h>
#include <stdio.h>


typedef int bool;
#define true 1
#define false 0

typedef unsigned char u8;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;

	


#elif PLATFORM_XR809



typedef int bool;
#define true 1
#define false 0

typedef unsigned char u8;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned int UINT32;

#define ASSERT
#define os_strcpy strcpy
#define os_malloc malloc
#define os_free free
#define os_memset memset

#define close lwip_close

#define rtos_delay_milliseconds OS_ThreadSleep


#define kNoErr                      0       //! No error occurred.
typedef void *beken_thread_arg_t;
typedef void *beken_thread_t;
typedef void (*beken_thread_function_t)( beken_thread_arg_t arg );
typedef int OSStatus;

#define BEKEN_DEFAULT_WORKER_PRIORITY      (6)
#define BEKEN_APPLICATION_PRIORITY         (7)

// wrappers for XR809 threads to work like bekken
OSStatus rtos_delete_thread( beken_thread_t* thread );
OSStatus rtos_create_thread( beken_thread_t* thread, 
							uint8_t priority, const char* name, 
							beken_thread_function_t function,
							uint32_t stack_size, beken_thread_arg_t arg );

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "common/framework/platform_init.h"

#include "kernel/os/os.h"

#else
#define _TUYA_DEVICE_GLOBAL

/* Includes ------------------------------------------------------------------*/
#include "uni_log.h"
#include "tuya_iot_wifi_api.h"
#include "tuya_hal_system.h"
#include "tuya_iot_com_api.h"
#include "tuya_cloud_com_defs.h"
#include "gw_intf.h"
#include "gpio_test.h"
#include "tuya_gpio.h"
#include "tuya_key.h"
#include "tuya_led.h"
#include "wlan_ui_pub.h"

#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"

#include "../../beken378/func/key/multi_button.h"

#define printf addLog


#endif

typedef unsigned char byte;


#endif

// stricmp fix
#if WINDOWS


#else

int wal_stricmp(const char *a, const char *b) ;
#define stricmp wal_stricmp

#endif


int strcat_safe(char *tg, const char *src, int tgMaxLen);
int strcpy_safe(char *tg, const char *src, int tgMaxLen);
void urldecode2_safe(char *dst, const char *srcin, int maxDstLen);
int Time_getUpTimeSeconds();
char Tiny_CRC8(const char *data,int length);
void RESET_ScheduleModuleReset(int delSeconds);
int Main_IsConnectedToWiFi();









