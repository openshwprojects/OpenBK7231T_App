#ifndef __NEW_COMMON_H__
#define __NEW_COMMON_H__

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

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
#ifdef PLATFORM_W600
	typedef portTickType TickType_t;		// W600/W800: xTaskGetTickCount() is of type "portTickType", all others "TickType_t" , W600 has no definition for TickType_t
#endif
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
typedef int bool;
#define true 1
#define false 0
#include <FreeRTOS.h>
#include <task.h>

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

#endif

const char* skipToNextWord(const char* p);
char *strdup(const char *s);
void stripDecimalPlaces(char *p, int maxDecimalPlaces);
int wal_stricmp(const char *a, const char *b);
int wal_strnicmp(const char *a, const char *b, int count);
int strcat_safe(char *tg, const char *src, int tgMaxLen);
int strcpy_safe(char *tg, const char *src, int tgMaxLen);
int strcpy_safe_checkForChanges(char *tg, const char *src, int tgMaxLen);
void urldecode2_safe(char *dst, const char *srcin, int maxDstLen);
int strIsInteger(const char *s);
const char* strcasestr(const char* str1, const char* str2);

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

//delay function do 10*r nops, because rtos_delay_milliseconds is too much
void usleep(int r);

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

#define IP_STRING_FORMAT	"%hhu.%hhu.%hhu.%hhu"

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

// some variables and functions for handling device time even without NTP driver present
// vars will be initialised in new_common.c 
// "eoch" on startup of device; If we add g_secondsElapsed we get the actual time  
extern uint32_t g_epochOnStartup ;
// UTC offset
extern int g_UTCoffset;
extern int g_DST_offset;
extern uint32_t g_next_dst_change;
#if ENABLE_LOCAL_CLOCK_ADVANCED
// handle daylight saving time

// define a union to hold all settings regarding local clock (timezone, dst settings ...)
// we will use somthing simmilar to tasmotas handling of dst with the commands:
// TimeStd
// TimeDst
// use a union, to be able to save/restore all values as one 64 bit integer
union DST{
	struct {
		uint32_t DST_H : 1;	// 0-1		only once - you can only be in one DST_hemisphere ;-)
		uint32_t DST_Ws : 3; // 0-7
		uint32_t DST_Ms : 4; // 0-15
		uint32_t DST_Ds : 3; // 0-7
		uint32_t DST_hs : 5; // 0-31
		// up to here: 16 bits
		int32_t Tstd : 11; // -1024 - +1023 	( regular (non DST)  offset to UTC max 14 h = 840 Min) 
		uint32_t DST_he : 5; // 0-31
		// up to here: 32 bits
		uint32_t DST_We : 3; // 0-7
		uint32_t DST_Me : 4; // 0-15
		uint32_t DST_De : 3; // 0-7
		int32_t Tdst : 8; // -128 - +127  (Eire has "negative" DST in winter (-60) ...; max DST offset is 120 minutes)
		int32_t TZ : 12; //  -2048 - 2047 for TZ hours (max offset is +14h and - 12h  -- we will use 99 as indicator for "DST" like tasmota, all otDST_hers values are interpreted as the <hour><minutes> of the time zone eg: -1400 for -14:00)
		// up to here: 62 bits
		};
	uint64_t value;	// to save/store all values in one large integer 
};

extern union DST g_clocksettings;

#endif

int testNsetDST(uint32_t val);

// to use ticks for time keeping
// since usual ticktimer (uint32_t) rolls over after approx 50 days,
// we need to count this rollovers
extern TickType_t lastTick;
extern uint8_t timer_rollover; // I don't expect uptime > 35 years ...

uint32_t getSecondsElapsed();

uint32_t Clock_GetCurrentTime(); 			// might replace for NTP_GetCurrentTime() to return time regardless of NTP present/running
uint32_t Clock_GetCurrentTimeWithoutOffset(); 		// ... same for NTP_GetCurrentTimeWithoutOffset()...
bool Clock_IsTimeSynced(); 				// ... and for NTP_IsTimeSynced()
int Clock_GetTimesZoneOfsSeconds();			// ... and for NTP_GetTimesZoneOfsSeconds()

typedef int(*jsonCb_t)(void *userData, const char *fmt, ...);
int JSON_ProcessCommandReply(const char *cmd, const char *args, void *request, jsonCb_t printer, int flags);
void ScheduleDriverStart(const char *name, int delay);
bool isWhiteSpace(char ch);
void convert_IP_to_string(char *o, unsigned char *ip);
int str_to_ip(const char *s, byte *ip);
int STR_ReplaceWhiteSpacesWithUnderscore(char *p);

#endif /* __NEW_COMMON_H__ */

