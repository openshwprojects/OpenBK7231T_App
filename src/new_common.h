#ifndef __NEW_COMMON_H__
#define __NEW_COMMON_H__

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#if WINDOWS
#include <crtdbg.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#endif

#include "obk_config.h"

typedef int OBK_Publish_Result;

// Our fast/lightweight/stack-saving sprintfs?
// NOTE: I did not replace whole vsnprintf on Beken
// because when I did, the WiFi broke somehow
// It seems some libraries must have the full version of vsnprintf available.

#if 0
#define vsnprintf vsnprintf2
#define snprintf snprintf2
#define sprintf sprintf2
#endif

int vsnprintf2(char *o, size_t olen, char const *fmt, va_list arg);
int snprintf2(char *o, size_t olen, const char* fmt, ...);
int sprintf2(char *o, const char* fmt, ...);

// from http_fns.  should move to a utils file.
extern unsigned char hexbyte(const char* hex);

void OTA_RequestDownloadFromHTTP(const char *s);

#if WINDOWS
#define DEVICENAME_PREFIX_FULL "WinTest"
#define DEVICENAME_PREFIX_SHORT "WT"
#define PLATFORM_MCU_NAME "WIN32"
#define MANUFACTURER "Microsoft"
#elif PLATFORM_XR809
#define DEVICENAME_PREFIX_FULL "OpenXR809"
#define DEVICENAME_PREFIX_SHORT "oxr"
#define PLATFORM_MCU_NAME "XR809"
#define MANUFACTURER "Xradio Technology"
#elif PLATFORM_BK7231N
#define DEVICENAME_PREFIX_FULL "OpenBK7231N"
#define DEVICENAME_PREFIX_SHORT "obk"
#define PLATFORM_MCU_NAME "BK7231N"
#define MANUFACTURER "Beken Corporation"
#elif PLATFORM_BK7231T
#define DEVICENAME_PREFIX_FULL "OpenBK7231T"
#define DEVICENAME_PREFIX_SHORT "obk"
#define PLATFORM_MCU_NAME "BK7231T"
#define MANUFACTURER "Beken Corporation"
#elif PLATFORM_BL602
#define DEVICENAME_PREFIX_FULL "OpenBL602"
#define DEVICENAME_PREFIX_SHORT "obl"
#define PLATFORM_MCU_NAME "BL602"
#define MANUFACTURER "BLAITEK"
#elif PLATFORM_W800
#define DEVICENAME_PREFIX_FULL "OpenW800"
#define DEVICENAME_PREFIX_SHORT "w800"
#define PLATFORM_MCU_NAME "W800"
#define MANUFACTURER "WinnerMicro"
#elif PLATFORM_W600
#define DEVICENAME_PREFIX_FULL "OpenW600"
#define DEVICENAME_PREFIX_SHORT "w600"
#define PLATFORM_MCU_NAME "W600"
#define MANUFACTURER "WinnerMicro"

typedef long BaseType_t;

#elif PLATFORM_LN882H
#define DEVICENAME_PREFIX_FULL "OpenLN882H"
#define DEVICENAME_PREFIX_SHORT "ln882h"
#define PLATFORM_MCU_NAME "LN882H"
#define MANUFACTURER "LightningSemi"
#elif PLATFORM_ESPIDF
#define MANUFACTURER "Espressif"
#define DEVICENAME_PREFIX_SHORT "esp"
#ifdef CONFIG_IDF_TARGET_ESP32C3
#define PLATFORM_MCU_NAME "ESP32C3"
#elif CONFIG_IDF_TARGET_ESP32C2
#define PLATFORM_MCU_NAME "ESP32C2"
#elif CONFIG_IDF_TARGET_ESP32
#define PLATFORM_MCU_NAME "ESP32"
#elif CONFIG_IDF_TARGET_ESP32C6
#define PLATFORM_MCU_NAME "ESP32C6"
#elif CONFIG_IDF_TARGET_ESP32S2
#define PLATFORM_MCU_NAME "ESP32S2"
#elif CONFIG_IDF_TARGET_ESP32S3
#define PLATFORM_MCU_NAME "ESP32S3"
#else
#define PLATFORM_MCU_NAME MANUFACTURER
#endif
#define DEVICENAME_PREFIX_FULL "Open" PLATFORM_MCU_NAME
#else
#error "You must define a platform.."
This platform is not supported, error!
#endif

// make sure that USER_SW_VER is set on all platforms
// Automatic Github builds are setting it externally,
// but it may not be set while doing a test build on developer PC
#ifndef USER_SW_VER
#ifdef WINDOWS
#define USER_SW_VER "Win_Test"
#elif PLATFORM_XR809
#define USER_SW_VER "XR809_Test"
#elif defined(PLATFORM_BK7231N)
#define USER_SW_VER "BK7231N_Test"
#elif defined(PLATFORM_BK7231T)
#define USER_SW_VER "BK7231T_Test"
#elif defined(PLATFORM_BL602)
#define USER_SW_VER "BL602_Test"
#elif defined(PLATFORM_W600)
#define USER_SW_VER "W600_Test"
#elif defined(PLATFORM_W800)
#define USER_SW_VER "W800_Test"
#elif defined(PLATFORM_LN882H)
#define USER_SW_VER "LN882H_Test"
#elif defined(PLATFORM_ESPIDF)
#define USER_SW_VER PLATFORM_MCU_NAME "_Test"
#else
#define USER_SW_VER "unknown"
#endif
#endif


#define BIT_SET(PIN,N) (PIN |=  (1<<N))
#define BIT_CLEAR(PIN,N) (PIN &= ~(1<<N))
#define BIT_TGL(PIN,N) (PIN ^=  (1<<N))
#define BIT_CHECK(PIN,N) !!((PIN & (1<<N)))
#define BIT_SET_TO(PIN,N, TG) if(TG) { BIT_SET(PIN,N); } else { BIT_CLEAR(PIN,N); }

#ifndef MIN
#define MIN(a,b)	(((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b)	(((a)>(b))?(a):(b))
#endif

#define OBK_IS_NAN(x) ((x)!=(x))

#if WINDOWS

#include <time.h>
#include <stdint.h>
#include <math.h>

#define portTICK_RATE_MS 1000
#define bk_printf printf

// generic
typedef int bool;
#define true 1
#define false 0

typedef int BaseType_t;
typedef unsigned char u8;
typedef unsigned char u8_t;
typedef unsigned int u32;
typedef unsigned int u32_t;
typedef unsigned short u16_t;
//typedef unsigned char uint8_t;
//typedef unsigned int uint32_t;
//typedef unsigned long uint64_t;
//typedef unsigned short uint16_t;
//typedef char int8_t;
//typedef int int32_t;

// it is not in my Windows compiler, but I added it manually
#include <stdint.h>
//
//#ifndef UINT32_MAX
//#define UINT32_MAX  (0xffffffff)
//#endif

#define		LWIP_UNUSED_ARG(x)
#define 	LWIP_CONST_CAST(target_type, val)   ((target_type)(val))

void doNothing();

#define 	GLOBAL_INT_DECLARATION		doNothing
#define 	GLOBAL_INT_DISABLE			doNothing
#define 	GLOBAL_INT_RESTORE			doNothing

// os
#define os_free free
#define os_malloc malloc
#define os_strlen strlen
#define os_memset memset
#define os_memcpy memcpy
#define os_strstr strstr
#define os_strcpy strcpy
#define os_strchr strchr
#define os_strcmp strcmp
#define os_memmove memmove

// RTOS
typedef long portTickType;
#define portTICK_PERIOD_MS 1
#define configTICK_RATE_HZ 1
typedef int SemaphoreHandle_t;
#define pdTRUE 1
#define pdFALSE 0
typedef int OSStatus;

int rtos_delay_milliseconds(int sec);
int delay_ms(int sec);

enum {
	kNoErr = 0,
};

typedef void * beken_thread_arg_t;
typedef int (*beken_thread_function_t)(void *p);
#define BEKEN_APPLICATION_PRIORITY 1

#elif PLATFORM_BL602

#include <FreeRTOS.h>
#include <task.h>
#include <portable.h>
#include <semphr.h>
#include <stdbool.h>
#include <stdint.h>

#define ASSERT
#define os_strcpy strcpy
#define os_malloc malloc
#define os_free free
#define os_memset memset

#define bk_printf printf

#define rtos_delay_milliseconds vTaskDelay
#define delay_ms vTaskDelay

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
typedef unsigned int u32;
		

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

// OS_MSleep?
#define rtos_delay_milliseconds OS_ThreadSleep
#define delay_ms OS_ThreadSleep

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

#elif PLATFORM_W600 || PLATFORM_W800

#include <string.h>
#include "wm_include.h"
#include <FreeRTOS.h>
#include <task.h>
#include <portable.h>
#include <semphr.h>
#include "lwip/sys.h"

#define portTICK_RATE_MS                      ( ( portTickType ) 1000 / configTICK_RATE_HZ )

#define bk_printf printf
#define os_strcpy strcpy
#define os_malloc malloc
#define os_free free
#define os_memset memset

#define portTICK_PERIOD_MS	portTICK_RATE_MS

#define rtos_delay_milliseconds sys_msleep
#define delay_ms sys_msleep

#define SemaphoreHandle_t xSemaphoreHandle

#define os_strcpy strcpy

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


#elif PLATFORM_LN882H

// TODO:LN882H Platform setup here.
#include <stdbool.h>

#define ASSERT
#define os_strcpy strcpy
#define os_malloc malloc
#define os_free free
#define os_memset memset

#define bk_printf printf

#define kNoErr                      0       //! No error occurred.
#define rtos_delay_milliseconds OS_MsDelay
typedef void *beken_thread_arg_t;
typedef void *beken_thread_t;
typedef void (*beken_thread_function_t)( beken_thread_arg_t arg );
typedef int OSStatus;

#define BEKEN_DEFAULT_WORKER_PRIORITY      (6)
#define BEKEN_APPLICATION_PRIORITY         (7)

// wrappers for XR809??? threads to work like bekken
OSStatus rtos_delete_thread( beken_thread_t* thread );
OSStatus rtos_create_thread( beken_thread_t* thread,
							uint8_t priority, const char* name,
							beken_thread_function_t function,
							uint32_t stack_size, beken_thread_arg_t arg );

#elif PLATFORM_ESPIDF

#include <stdbool.h>
#include <arch/sys_arch.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_idf_version.h"

#define ASSERT
#define os_strcpy strcpy
#define os_malloc malloc
#define os_free free
#define os_memset memset

//#define bk_printf printf

#define bk_printf(...) ESP_LOGI("OpenBeken", __VA_ARGS__);

#define kNoErr                      0       //! No error occurred.
#define rtos_delay_milliseconds sys_delay_ms
typedef void* beken_thread_arg_t;
typedef void* beken_thread_t;
typedef void (*beken_thread_function_t)(beken_thread_arg_t arg);
typedef int OSStatus;

#define BEKEN_DEFAULT_WORKER_PRIORITY      (6)
#define BEKEN_APPLICATION_PRIORITY         (7)

// wrappers for XR809??? threads to work like bekken
OSStatus rtos_delete_thread(beken_thread_t* thread);
OSStatus rtos_create_thread(beken_thread_t* thread,
	uint8_t priority, const char* name,
	beken_thread_function_t function,
	uint32_t stack_size, beken_thread_arg_t arg);

#define portTICK_RATE_MS portTICK_PERIOD_MS
#define portTickType TickType_t
#define xTaskHandle TaskHandle_t
#define delay_ms sys_delay_ms
#define UINT32 uint32_t

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

#define printf(...) addLog(__VA_ARGS__)

void delay_ms(UINT32 ms_count);

#endif

typedef unsigned char byte;


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
#include <stdlib.h>
#include <stdio.h>

#else

#include "lwip/sockets.h"

#endif


// stricmp fix
#if WINDOWS


#else

int wal_stricmp(const char *a, const char *b) ;
#define stricmp wal_stricmp
char *strdup(const char *s);

#endif

const char* skipToNextWord(const char* p);
void stripDecimalPlaces(char *p, int maxDecimalPlaces);
int wal_stricmp(const char *a, const char *b);
int wal_strnicmp(const char *a, const char *b, int count);
int strcat_safe(char *tg, const char *src, int tgMaxLen);
int strcpy_safe(char *tg, const char *src, int tgMaxLen);
int strcpy_safe_checkForChanges(char *tg, const char *src, int tgMaxLen);
void urldecode2_safe(char *dst, const char *srcin, int maxDstLen);
int strIsInteger(const char *s);

#ifndef PLATFORM_ESPIDF
const char* strcasestr(const char* str1, const char* str2);
#endif

// user_main.c
char Tiny_CRC8(const char *data,int length);
void RESET_ScheduleModuleReset(int delSeconds);
void MAIN_ScheduleUnsafeInit(int delSeconds);
void Main_ScheduleHomeAssistantDiscovery(int seconds);
int Main_IsConnectedToWiFi();
int Main_IsOpenAccessPointMode();
void Main_Init();
bool Main_HasFastConnect();
void Main_OnEverySecond();
int Main_HasMQTTConnected();
int Main_HasWiFiConnected();
void Main_OnPingCheckerReply(int ms);

// new_ping.c
void Main_SetupPingWatchDog(const char *target/*, int delayBetweenPings_Seconds*/);
int PingWatchDog_GetTotalLost();
int PingWatchDog_GetTotalReceived();

// my addon to LWIP library

int LWIP_GetMaxSockets();
int LWIP_GetActiveSockets();

#ifndef PLATFORM_ESPIDF
//delay function do 10*r nops, because rtos_delay_milliseconds is too much
void usleep(int r);
#endif

#define RESTARTS_REQUIRED_FOR_SAFE_MODE 4

// linear mapping function --> https://www.arduino.cc/reference/en/language/functions/math/map/
#define MAP(x, in_min, in_max, out_min, out_max) (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;

typedef enum lcdPrintType_e {
	LCD_PRINT_DEFAULT,
	LCD_PRINT_FLOAT,
	LCD_PRINT_INT,
} lcdPrintType_t;

typedef enum
{
    NOT_CONNECTED,
    WEAK,
    FAIR,
    GOOD,
    EXCELLENT,
} WIFI_RSSI_LEVEL;

#if PLATFORM_LN882H
#define IP_STRING_FORMAT	"%u.%u.%u.%u"
#else
#define IP_STRING_FORMAT	"%hhu.%hhu.%hhu.%hhu"
#endif
WIFI_RSSI_LEVEL wifi_rssi_scale(int8_t rssi_value);
extern const char *str_rssi[];
extern int bSafeMode;
extern int g_bWantPinDeepSleep;
extern int g_pinDeepSleepWakeUp;
extern int g_timeSinceLastPingReply;
extern int g_startPingWatchDogAfter;
extern int g_openAP;
extern int g_bootFailures;
extern int g_secondsElapsed;
extern int g_rebootReason;
extern float g_wifi_temperature;

typedef int(*jsonCb_t)(void *userData, const char *fmt, ...);
#if ENABLE_TASMOTA_JSON
int JSON_ProcessCommandReply(const char *cmd, const char *args, void *request, jsonCb_t printer, int flags);
#endif
void ScheduleDriverStart(const char *name, int delay);
bool isWhiteSpace(char ch);
void convert_IP_to_string(char *o, unsigned char *ip);
int str_to_ip(const char *s, byte *ip);
int STR_ReplaceWhiteSpacesWithUnderscore(char *p);

#endif /* __NEW_COMMON_H__ */

