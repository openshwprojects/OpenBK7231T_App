#ifndef __NEW_COMMON_H__
#define __NEW_COMMON_H__

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#include "obk_config.h"


#if WINDOWS
#define DEVICENAME_PREFIX_FULL "WinTest"
#define DEVICENAME_PREFIX_SHORT "WT"
#elif PLATFORM_XR809
#define DEVICENAME_PREFIX_FULL "OpenXR809"
#define DEVICENAME_PREFIX_SHORT "oxr"
#elif PLATFORM_BK7231N
#define DEVICENAME_PREFIX_FULL "OpenBK7231N"
#define DEVICENAME_PREFIX_SHORT "obk"
#elif PLATFORM_BK7231T
#define DEVICENAME_PREFIX_FULL "OpenBK7231T"
#define DEVICENAME_PREFIX_SHORT "obk"
#elif PLATFORM_BL602
#define DEVICENAME_PREFIX_FULL "OpenBL602"
#define DEVICENAME_PREFIX_SHORT "obl"
#else
#error "You must define a platform.."
This platform is not supported, error!
#endif




#if WINDOWS


typedef int bool;
#define true 1
#define false 0

typedef unsigned char u8;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;

	

#elif PLATFORM_BL602

#include <FreeRTOS.h>
#include <task.h>
#include <portable.h>
#include <semphr.h>

typedef int bool;
#define true 1
#define false 0

#define ASSERT
#define os_strcpy strcpy
#define os_malloc malloc
#define os_free free
#define os_memset memset

#define bk_printf printf

#define rtos_delay_milliseconds vTaskDelay

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



#include "common/framework/platform_init.h"

#include "kernel/os/os.h"

#else

#include "gw_intf.h"
#include "wlan_ui_pub.h"
#include "mem_pub.h"
#include "str_pub.h"
#include "drv_model_pub.h"

#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"

#include "../../beken378/func/key/multi_button.h"

#define printf addLog


#endif

typedef unsigned char byte;


#endif

#if PLATFORM_XR809
#define LWIP_COMPAT_SOCKETS 1
#define LWIP_POSIX_SOCKETS_IO_NAMES 1
#endif



#if WINDOWS

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#else

#include "lwip/sockets.h"

#endif


// stricmp fix
#if WINDOWS


#else

int wal_stricmp(const char *a, const char *b) ;
#define stricmp wal_stricmp

#endif


int strcat_safe(char *tg, const char *src, int tgMaxLen);
int strcpy_safe(char *tg, const char *src, int tgMaxLen);
int strcpy_safe_checkForChanges(char *tg, const char *src, int tgMaxLen);
void urldecode2_safe(char *dst, const char *srcin, int maxDstLen);
int Time_getUpTimeSeconds();
char Tiny_CRC8(const char *data,int length);
void RESET_ScheduleModuleReset(int delSeconds);
int Main_IsConnectedToWiFi();
void Main_Init();
void Main_OnEverySecond();
int Main_GetLastRebootBootFailures();








