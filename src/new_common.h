#ifndef __NEW_COMMON_H__
#define __NEW_COMMON_H__

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
// it is not in my Windows compiler, but I added it manually
#include <stdint.h>
#include <stdbool.h>

#if WINDOWS && !LINUX
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
#elif PLATFORM_XR806
#define DEVICENAME_PREFIX_FULL "OpenXR806"
#define DEVICENAME_PREFIX_SHORT "oxr"
#define PLATFORM_MCU_NAME "XR806"
#define MANUFACTURER "Xradio Technology"
#elif PLATFORM_XR809
#define DEVICENAME_PREFIX_FULL "OpenXR809"
#define DEVICENAME_PREFIX_SHORT "oxr"
#define PLATFORM_MCU_NAME "XR809"
#define MANUFACTURER "Xradio Technology"
#elif PLATFORM_XR872
#define DEVICENAME_PREFIX_FULL "OpenXR872"
#define DEVICENAME_PREFIX_SHORT "oxr"
#define PLATFORM_MCU_NAME "XR872"
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
#elif PLATFORM_BK7238
#define DEVICENAME_PREFIX_FULL "OpenBK7238"
#define DEVICENAME_PREFIX_SHORT "obk"
#define PLATFORM_MCU_NAME "BK7238"
#define MANUFACTURER "Beken Corporation"
#elif PLATFORM_BK7231U
#define DEVICENAME_PREFIX_FULL "OpenBK7231U"
#define DEVICENAME_PREFIX_SHORT "obk"
#define PLATFORM_MCU_NAME "BK7231U"
#define MANUFACTURER "Beken Corporation"
#elif PLATFORM_BK7252
#define DEVICENAME_PREFIX_FULL "OpenBK7252"
#define DEVICENAME_PREFIX_SHORT "obk"
#define PLATFORM_MCU_NAME "BK7252"
#define MANUFACTURER "Beken Corporation"
#elif PLATFORM_BK7252N
#define DEVICENAME_PREFIX_FULL "OpenBK7252N"
#define DEVICENAME_PREFIX_SHORT "obk"
#define PLATFORM_MCU_NAME "BK7252N"
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
#elif PLATFORM_LN8825
#define DEVICENAME_PREFIX_FULL "OpenLN8825"
#define DEVICENAME_PREFIX_SHORT "ln8825"
#define PLATFORM_MCU_NAME "LN8825"
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
#elif CONFIG_IDF_TARGET_ESP32C5
#define PLATFORM_MCU_NAME "ESP32C5"
#elif CONFIG_IDF_TARGET_ESP32C61
#define PLATFORM_MCU_NAME "ESP32C61"
#else
#define PLATFORM_MCU_NAME MANUFACTURER
#endif
#define DEVICENAME_PREFIX_FULL "Open" PLATFORM_MCU_NAME
#elif PLATFORM_TR6260
#define DEVICENAME_PREFIX_FULL "OpenTR6260"
#define DEVICENAME_PREFIX_SHORT "tr6260"
#define PLATFORM_MCU_NAME "TR6260"
#define MANUFACTURER "Transa Semi"
#elif PLATFORM_RTL87X0C
#define DEVICENAME_PREFIX_FULL "OpenRTL87X0C"
#define DEVICENAME_PREFIX_SHORT "rtl87x0C"
#define PLATFORM_MCU_NAME "RTL87X0C"
#define MANUFACTURER "Realtek"
#elif PLATFORM_RTL8710B
#define DEVICENAME_PREFIX_FULL "OpenRTL8710B"
#define DEVICENAME_PREFIX_SHORT "rtl8710b"
#define PLATFORM_MCU_NAME "RTL8710B"
#define MANUFACTURER "Realtek"
#elif PLATFORM_RTL8710A
#define DEVICENAME_PREFIX_FULL "OpenRTL8710A"
#define DEVICENAME_PREFIX_SHORT "rtl8710a"
#define PLATFORM_MCU_NAME "RTL8710A"
#define MANUFACTURER "Realtek"
#elif PLATFORM_RTL8720D
#define DEVICENAME_PREFIX_FULL "OpenRTL8720D"
#define DEVICENAME_PREFIX_SHORT "rtl8720d"
#define PLATFORM_MCU_NAME "RTL8720D"
#define MANUFACTURER "Realtek"
#elif PLATFORM_ECR6600
#define DEVICENAME_PREFIX_FULL "OpenECR6600"
#define DEVICENAME_PREFIX_SHORT "ecr6600"
#define PLATFORM_MCU_NAME "ECR6600"
#define MANUFACTURER "ESWIN"
#elif PLATFORM_ESP8266
#define DEVICENAME_PREFIX_FULL "OpenESP8266"
#define DEVICENAME_PREFIX_SHORT "esp8266"
#define PLATFORM_MCU_NAME "ESP8266"
#define MANUFACTURER "Espressif"
#elif PLATFORM_RTL8721DA
#define DEVICENAME_PREFIX_FULL "OpenRTL8721DA"
#define DEVICENAME_PREFIX_SHORT "rtl8721da"
#define PLATFORM_MCU_NAME "RTL8721DA"
#define MANUFACTURER "Realtek"
#elif PLATFORM_RTL8720E
#define DEVICENAME_PREFIX_FULL "OpenRTL8720E"
#define DEVICENAME_PREFIX_SHORT "rtl8720e"
#define PLATFORM_MCU_NAME "RTL8720E"
#define MANUFACTURER "Realtek"
#elif PLATFORM_TXW81X
#define DEVICENAME_PREFIX_FULL "OpenTXW81X"
#define DEVICENAME_PREFIX_SHORT "txw81x"
#define PLATFORM_MCU_NAME "TXW81X"
#define MANUFACTURER "Taixin"
#elif PLATFORM_RDA5981
#define DEVICENAME_PREFIX_FULL "OpenRDA5981"
#define DEVICENAME_PREFIX_SHORT "rda5981"
#define PLATFORM_MCU_NAME "RDA5981"
#define MANUFACTURER "RDA Microelectronics"
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
#elif PLATFORM_XR872
#define USER_SW_VER "XR872_Test"
#elif PLATFORM_XR806
#define USER_SW_VER "XR806_Test"
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
#elif defined(PLATFORM_LN8825)
#define USER_SW_VER "LN8825_Test"
#elif defined(PLATFORM_ESPIDF)
#define USER_SW_VER PLATFORM_MCU_NAME "_Test"
#elif defined(PLATFORM_TR6260)
#define USER_SW_VER "TR6260_Test"
#elif defined(PLATFORM_RTL87X0C)
#define USER_SW_VER "RTL87X0C_Test"
#elif defined(PLATFORM_RTL8710B)
#define USER_SW_VER "RTL8710B_Test"
#elif defined(PLATFORM_RTL8710A)
#define USER_SW_VER "RTL8710A_Test"
#elif defined(PLATFORM_RTL8720D)
#define USER_SW_VER "RTL8720D_Test"
#elif defined(PLATFORM_BK7238)
#define USER_SW_VER "BK7238_Test"
#elif defined(PLATFORM_BK7231U)
#define USER_SW_VER "BK7231U_Test"
#elif defined(PLATFORM_BK7252)
#define USER_SW_VER "BK7252_Test"
#elif defined(PLATFORM_BK7252N)
#define USER_SW_VER "BK7252N_Test"
#elif PLATFORM_ECR6600
#define USER_SW_VER "ECR6600_Test"
#elif PLATFORM_ESP8266
#define USER_SW_VER "ESP8266_Test"
#elif PLATFORM_RTL8721DA
#define USER_SW_VER "RTL8721DA_Test"
#elif PLATFORM_RTL8720E
#define USER_SW_VER "RTL8720E_Test"
#elif PLATFORM_TXW81X
#define USER_SW_VER "TXW81X_Test"
#elif PLATFORM_RDA5981
#define USER_SW_VER "RDA5981_Test"
#else
#warning "USER_SW_VER undefined"
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
#include <math.h>

#ifdef LINUX

#include <netdb.h>  // For gethostbyname and struct hostent
#include <limits.h>
#define closesocket close
#define SOCKET int
#define ISVALIDSOCKET(s) ((s) >= 0)
#define GETSOCKETERRNO() (errno)
#define ioctlsocket ioctl
#define WSAEWOULDBLOCK EWOULDBLOCK
#define SOCKET_ERROR	-1
#define INVALID_SOCKET	-1
#define WSAGetLastError() (errno)
// TODO
#define SD_SEND	 0 

#else

#define close closesocket
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define GETSOCKETERRNO() (WSAGetLastError())

#endif

#define portTICK_RATE_MS 1000
#define bk_printf printf

// generic
#include <stdbool.h>

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

#define ASSERT
#define os_free free
#define os_malloc malloc

#define bk_printf printf

#define rtos_delay_milliseconds vTaskDelay
#define delay_ms vTaskDelay

#define kNoErr                      0       //! No error occurred.
typedef void *beken_thread_arg_t;
typedef xTaskHandle beken_thread_t;
typedef void (*beken_thread_function_t)( beken_thread_arg_t arg );
typedef int OSStatus;

#define BEKEN_DEFAULT_WORKER_PRIORITY      (6)
#define BEKEN_APPLICATION_PRIORITY         (7)

// wrappers for XR809 threads to work like beken
OSStatus rtos_delete_thread( beken_thread_t* thread );
OSStatus rtos_create_thread( beken_thread_t* thread,
							uint8_t priority, const char* name,
							beken_thread_function_t function,
							uint32_t stack_size, beken_thread_arg_t arg );
OSStatus rtos_suspend_thread(beken_thread_t* thread);
typedef unsigned int u32;

#define lwip_close_force(x) lwip_close(x)

#define OBK_OTA_EXTENSION ".bin.xz.ota"

#elif PLATFORM_XRADIO

#include "FreeRTOS.h"
#include "task.h"

#if !PLATFORM_XR809
#define OBK_OTA_EXTENSION ".img"
#endif
//typedef unsigned char u8;
//typedef unsigned char uint8_t;
//typedef unsigned int uint32_t;
//typedef unsigned int UINT32;

#define ASSERT
#define bk_printf printf
#define os_malloc malloc
#define os_free free

#define lwip_close_force(x) lwip_close(x)

#define HAL_UART_Init OBK_HAL_UART_Init
#define HAL_ADC_Init OBK_HAL_ADC_Init

#if PLATFORM_XR806
#define DS_MS_TO_S 1000
#else
#define DS_MS_TO_S 1111
#endif

#if PLATFORM_XR809
#define close lwip_close
#endif

#ifdef __CONFIG_LWIP_V1
#define sockaddr_storage sockaddr
#define ss_family sa_family
#define ip4_addr ip_addr
#endif

// OS_MSleep?
#define rtos_delay_milliseconds OS_ThreadSleep
#define delay_ms OS_ThreadSleep

#define kNoErr                      0       //! No error occurred.
typedef void *beken_thread_arg_t;
typedef xTaskHandle* beken_thread_t;
typedef void (*beken_thread_function_t)( beken_thread_arg_t arg );
typedef int OSStatus;

#define BEKEN_DEFAULT_WORKER_PRIORITY      (6)
#define BEKEN_APPLICATION_PRIORITY         (7)

// wrappers for XR809 threads to work like beken
OSStatus rtos_delete_thread( beken_thread_t* thread );
OSStatus rtos_create_thread( beken_thread_t* thread,
							uint8_t priority, const char* name,
							beken_thread_function_t function,
							uint32_t stack_size, beken_thread_arg_t arg );
OSStatus rtos_suspend_thread(beken_thread_t* thread);

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
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
#include <stdbool.h>


#define GLOBAL_INT_DECLARATION()	;
#define GLOBAL_INT_DISABLE()		;
#define GLOBAL_INT_RESTORE()		;

#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS ( ( portTickType ) 1000 / configTICK_RATE_HZ )
#endif

#define bk_printf printf
#define os_malloc pvPortMalloc
#define os_free vPortFree
#define realloc pvPortRealloc

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS	portTICK_RATE_MS
#endif

#define rtos_delay_milliseconds(x) vTaskDelay(x / portTICK_PERIOD_MS)
#define delay_ms sys_msleep

#define SemaphoreHandle_t xSemaphoreHandle

#define kNoErr                      0       //! No error occurred.
typedef void *beken_thread_arg_t;
typedef xTaskHandle *beken_thread_t;
typedef void (*beken_thread_function_t)( beken_thread_arg_t arg );
typedef int OSStatus;

#define BEKEN_DEFAULT_WORKER_PRIORITY      (6)
#define BEKEN_APPLICATION_PRIORITY         (7)

// wrappers for XR809 threads to work like beken
OSStatus rtos_delete_thread( beken_thread_t* thread );
OSStatus rtos_create_thread( beken_thread_t* thread,
							uint8_t priority, const char* name,
							beken_thread_function_t function,
							uint32_t stack_size, beken_thread_arg_t arg );
OSStatus rtos_suspend_thread(beken_thread_t* thread);

#define OBK_OTA_EXTENSION ".img"

#elif PLATFORM_LN882H || PLATFORM_LN8825

#include <FreeRTOS.h>
#include <task.h>

#define ASSERT
//#define os_free free
//#define os_malloc malloc
#define os_malloc pvPortMalloc
#define os_free vPortFree
#define realloc pvPortRealloc

#define lwip_close_force(x) lwip_close(x)
#define bk_printf printf
#define kNoErr                      0       //! No error occurred.
#define rtos_delay_milliseconds OS_MsDelay
typedef void *beken_thread_arg_t;
typedef xTaskHandle beken_thread_t;
typedef void (*beken_thread_function_t)( beken_thread_arg_t arg );
typedef int OSStatus;

#define BEKEN_DEFAULT_WORKER_PRIORITY      (6)
#define BEKEN_APPLICATION_PRIORITY         (7)

// wrappers for XR809??? threads to work like beken
OSStatus rtos_delete_thread( beken_thread_t* thread );
OSStatus rtos_create_thread( beken_thread_t* thread,
							uint8_t priority, const char* name,
							beken_thread_function_t function,
							uint32_t stack_size, beken_thread_arg_t arg );
OSStatus rtos_suspend_thread(beken_thread_t* thread);

#if PLATFORM_LN8825
#define malloc os_malloc
#include "utils/debug/log.h"
#undef bk_printf
#define bk_printf(...) LOG(LOG_LVL_INFO, __VA_ARGS__)
#define printf(...) LOG(LOG_LVL_INFO, __VA_ARGS__)
#define hal_flash_read FLASH_Read
#define hal_flash_program FLASH_Program
#define hal_flash_erase FLASH_Erase
#define OBK_OTA_EXTENSION		".img"
#define OBK_OTA_NAME_EXTENSION	"_ota"
#else
#define OBK_OTA_EXTENSION		".bin"
#define OBK_OTA_NAME_EXTENSION	"_OTA"
#endif


#elif PLATFORM_ESPIDF || PLATFORM_ESP8266

#include <arch/sys_arch.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_idf_version.h"

#define ASSERT
#define os_free free
#define os_malloc malloc

#define bk_printf printf

//#define bk_printf(...) ESP_LOGI("OpenBeken", __VA_ARGS__);

#define kNoErr                      0       //! No error occurred.
#define rtos_delay_milliseconds sys_delay_ms
typedef void* beken_thread_arg_t;
typedef TaskHandle_t beken_thread_t;
typedef void (*beken_thread_function_t)(beken_thread_arg_t arg);
typedef int OSStatus;

#define lwip_close_force(x) lwip_close(x)

#define BEKEN_DEFAULT_WORKER_PRIORITY      (6)
#define BEKEN_APPLICATION_PRIORITY         (7)

// wrappers for XR809??? threads to work like beken
OSStatus rtos_delete_thread(beken_thread_t* thread);
OSStatus rtos_create_thread(beken_thread_t* thread,
	uint8_t priority, const char* name,
	beken_thread_function_t function,
	uint32_t stack_size, beken_thread_arg_t arg);
OSStatus rtos_suspend_thread(beken_thread_t* thread);

#define portTICK_RATE_MS portTICK_PERIOD_MS
#define portTickType TickType_t
#define xTaskHandle TaskHandle_t
#define delay_ms sys_delay_ms
#define UINT32 uint32_t

#define OBK_OTA_EXTENSION ".img"

#if PLATFORM_ESP8266
#define xPortGetFreeHeapSize() esp_get_free_heap_size()
#endif

#elif PLATFORM_TR6260

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

typedef unsigned int UINT32;

#define ASSERT
#define free    os_free
#define malloc  os_malloc
#define realloc  os_realloc
#define strncpy  os_strncpy

#define close lwip_close
#define bk_printf system_printf
#define printf system_printf

#define lwip_close_force(x) lwip_close(x)
// OS_MSleep?
#define rtos_delay_milliseconds sys_delay_ms
#define delay_ms sys_delay_ms

#define kNoErr                      0       //! No error occurred.
typedef void* beken_thread_arg_t;
typedef xTaskHandle beken_thread_t;
typedef void (*beken_thread_function_t)(beken_thread_arg_t arg);
typedef int OSStatus;

#define BEKEN_DEFAULT_WORKER_PRIORITY      (6)
#define BEKEN_APPLICATION_PRIORITY         (7)

OSStatus rtos_delete_thread(beken_thread_t* thread);
OSStatus rtos_create_thread(beken_thread_t* thread,
	uint8_t priority, const char* name,
	beken_thread_function_t function,
	uint32_t stack_size, beken_thread_arg_t arg);
OSStatus rtos_suspend_thread(beken_thread_t* thread);

#define GLOBAL_INT_DECLARATION()	;
#define GLOBAL_INT_DISABLE()		;
#define GLOBAL_INT_RESTORE()		;

#define OBK_OTA_EXTENSION ".img"

#elif PLATFORM_REALTEK

#if __cplusplus && !PLATFORM_REALTEK_NEW
typedef uint32_t in_addr_t;
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "PinNames.h"
#if PLATFORM_REALTEK_NEW
#include "wifi_api_event.h"
#include "wifi_api_types.h"
#include "wifi_api_ext.h"
#include "kv.h"
#if PLATFORM_RTL8721DA
#include "autoconf_8721da.h"
#elif PLATFORM_RTL8720E
#include "autoconf_8720e.h"
#endif
#define wifi_is_up wifi_is_running
#define RTW_STA_INTERFACE STA_WLAN_INDEX
#define RTW_AP_INTERFACE SOFTAP_WLAN_INDEX
#define RTW_MODE_STA_AP RTW_MODE_AP
#define wifi_enable_powersave() wifi_set_lps_enable(1)
#define wifi_disable_powersave() wifi_set_lps_enable(0)
#define RTW_SUCCESS 0
typedef enum rtw_security rtw_security_t;
typedef enum rtw_drv_op_mode rtw_mode_t;
#endif

typedef unsigned int UINT32;
typedef uint8_t UINT8;
extern int g_sleepfactor;
extern u32 pwmout_pin2chan(PinName pin);

#undef ASSERT
#define ASSERT

#define lwip_close_force(x) lwip_close(x)
#define os_malloc pvPortMalloc
#define os_free vPortFree

#if PLATFORM_RTL8720D
#undef vsnprintf
#undef sprintf
#undef atoi
#undef printf
#endif

// for wifi
#define BUFLEN_LEN				1
#define MAC_LEN					6
#define RSSI_LEN				4
#define SECURITY_LEN			1
#define SECURITY_LEN_EXTENDED	4
#define WPS_ID_LEN				1
#define CHANNEL_LEN				1

#define bk_printf printf

#if PLATFORM_RTL8710B || PLATFORM_RTL8710A
#define atoi __wrap_atoi
#endif

#if PLATFORM_RTL8710B || PLATFORM_RTL8720D
#define rtos_delay_milliseconds(x) vTaskDelay(x / portTICK_PERIOD_MS / g_sleepfactor)
#define delay_ms(x) vTaskDelay(x / portTICK_PERIOD_MS / g_sleepfactor)
#else
#define rtos_delay_milliseconds(x) vTaskDelay(x / portTICK_PERIOD_MS)
#define delay_ms(x) vTaskDelay(x / portTICK_PERIOD_MS)
#endif

#define kNoErr                      0       //! No error occurred.
typedef void* beken_thread_arg_t;
typedef xTaskHandle beken_thread_t;
typedef void (*beken_thread_function_t)(beken_thread_arg_t arg);
typedef int OSStatus;

#define BEKEN_DEFAULT_WORKER_PRIORITY      (6)
#define BEKEN_APPLICATION_PRIORITY         (7)

OSStatus rtos_delete_thread(beken_thread_t* thread);
OSStatus rtos_create_thread(beken_thread_t* thread,
	uint8_t priority, const char* name,
	beken_thread_function_t function,
	uint32_t stack_size, beken_thread_arg_t arg);
OSStatus rtos_suspend_thread(beken_thread_t* thread);

#define GLOBAL_INT_DECLARATION()	;
#define GLOBAL_INT_DISABLE()		;
#define GLOBAL_INT_RESTORE()		;

#define OBK_OTA_EXTENSION ".img"

#elif PLATFORM_ECR6600

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "os/oshal.h"

typedef unsigned int UINT32;

#define ASSERT
#undef os_malloc
#undef os_free
#define os_malloc	pvPortMalloc
#define os_free		vPortFree
#define malloc		os_malloc
#define free		os_free
#define calloc		os_calloc
#define realloc		os_realloc

#define bk_printf printf

extern void sys_delay_ms(uint32_t ms);
// OS_MSleep?
#define rtos_delay_milliseconds sys_delay_ms
#define delay_ms sys_delay_ms

#define lwip_close_force(x) lwip_close(x)
#define kNoErr                      0       //! No error occurred.
typedef void* beken_thread_arg_t;
typedef xTaskHandle beken_thread_t;
typedef void (*beken_thread_function_t)(beken_thread_arg_t arg);
typedef int OSStatus;

#define BEKEN_DEFAULT_WORKER_PRIORITY      (6)
#define BEKEN_APPLICATION_PRIORITY         (7)

OSStatus rtos_delete_thread(beken_thread_t* thread);
OSStatus rtos_create_thread(beken_thread_t* thread,
	uint8_t priority, const char* name,
	beken_thread_function_t function,
	uint32_t stack_size, beken_thread_arg_t arg);
OSStatus rtos_suspend_thread(beken_thread_t* thread);

#define GLOBAL_INT_DECLARATION()	;
#define GLOBAL_INT_DISABLE()		;
#define GLOBAL_INT_RESTORE()		;

#define OBK_OTA_EXTENSION ".img"

#elif PLATFORM_TXW81X

#include "csi_kernel.h"
#include "stdbool.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "osal/csky/defs.h"
#include "lib/heap/sysheap.h"
#include "osal/csky/string.h"

#define bk_printf printf

#define rtos_delay_milliseconds csi_kernel_delay_ms
#define delay_ms csi_kernel_delay_ms

#define lwip_close_force(x) lwip_close(x)
#define kNoErr                      0       //! No error occurred.
typedef void* beken_thread_arg_t;
typedef k_task_handle_t beken_thread_t;
typedef void (*beken_thread_function_t)(beken_thread_arg_t arg);
typedef int OSStatus;
typedef k_mutex_handle_t SemaphoreHandle_t;
#define xSemaphoreCreateMutex csi_kernel_mutex_new
#define xSemaphoreTake(a, b) (csi_kernel_mutex_lock(a, csi_kernel_ms2tick(b), 0) == 0)
#define xSemaphoreGive csi_kernel_mutex_unlock
#define pdTRUE true

#define xPortGetFreeHeapSize() sysheap_freesize(&sram_heap)
#define portTICK_RATE_MS RHINO_CONFIG_TICKS_PER_SECOND 
typedef int BaseType_t;
typedef uint64_t portTickType;
#define xTaskGetTickCount csi_kernel_get_ticks

#define BEKEN_DEFAULT_WORKER_PRIORITY      (6)
#define BEKEN_APPLICATION_PRIORITY         (7)

OSStatus rtos_delete_thread(beken_thread_t* thread);
OSStatus rtos_create_thread(beken_thread_t* thread,
	uint8_t priority, const char* name,
	beken_thread_function_t function,
	uint32_t stack_size, beken_thread_arg_t arg);
OSStatus rtos_suspend_thread(beken_thread_t* thread);

#define GLOBAL_INT_DECLARATION()	;
#define GLOBAL_INT_DISABLE()		;
#define GLOBAL_INT_RESTORE()		;

#elif PLATFORM_RDA5981

#include "stdbool.h"
#include "rda_sys_wrapper.h"
#include "cmsis_os.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#define bk_printf printf

#define rtos_delay_milliseconds osDelay
#define delay_ms osDelay
#define os_malloc malloc
#define os_free free

#define lwip_close_force(x) lwip_close(x)
#define kNoErr                      0       //! No error occurred.
typedef void* beken_thread_arg_t;
typedef void* beken_thread_t;
typedef void (*beken_thread_function_t)(beken_thread_arg_t arg);
typedef int OSStatus;
typedef void* SemaphoreHandle_t;
#define xSemaphoreCreateMutex rda_mutex_create
#define xSemaphoreTake(a, b) (rda_mutex_wait(a, b) == 0)
#define xSemaphoreGive rda_mutex_realease
#define pdTRUE true

#define portTICK_RATE_MS osKernelSysTickMicroSec(1000 * 1000) 
typedef int BaseType_t;
typedef uint64_t portTickType;
#define xTaskGetTickCount osKernelSysTick

#define BEKEN_DEFAULT_WORKER_PRIORITY      (6)
#define BEKEN_APPLICATION_PRIORITY         (7)

OSStatus rtos_delete_thread(beken_thread_t thread);
OSStatus rtos_create_thread(beken_thread_t thread,
	uint8_t priority, const char* name,
	beken_thread_function_t function,
	uint32_t stack_size, beken_thread_arg_t arg);
OSStatus rtos_suspend_thread(beken_thread_t thread);

#define GLOBAL_INT_DECLARATION()	;
#define GLOBAL_INT_DISABLE()		;
#define GLOBAL_INT_RESTORE()		;


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

#define OBK_OTA_EXTENSION ".rbl"

#endif

typedef unsigned char byte;

#if WINDOWS

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#ifndef LINUX

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#else

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#endif

#else

#include "lwip/sockets.h"

#endif


// stricmp fix
#if WINDOWS

#if LINUX

#define stricmp strcasecmp

#endif

#else

int wal_stricmp(const char *a, const char *b) ;
#undef stricmp
#define stricmp wal_stricmp
char *strdup(const char *s);

#endif

const char* skipToNextWord(const char* p);
void stripDecimalPlaces(char *p, int maxDecimalPlaces);
int wal_stricmp(const char *a, const char *b);
char *wal_stristr(const char *haystack, const char *needle);
int wal_strnicmp(const char *a, const char *b, int count);
int strcat_safe(char *tg, const char *src, int tgMaxLen);
int strcpy_safe(char *tg, const char *src, int tgMaxLen);
int strcpy_safe_checkForChanges(char *tg, const char *src, int tgMaxLen);
void urldecode2_safe(char *dst, const char *srcin, int maxDstLen);
int strIsInteger(const char *s);

#if !defined(PLATFORM_ESPIDF) && !defined(PLATFORM_TR6260) && !defined(PLATFORM_ECR6600) && !defined(PLATFORM_BL602) && \
	!defined(PLATFORM_ESP8266)

const char* strcasestr(const char* str1, const char* str2);
#endif

// user_main.c
char Tiny_CRC8(const char *data,int length);
void RESET_ScheduleModuleReset(int delSeconds);
void MAIN_ScheduleUnsafeInit(int delSeconds);
#if ENABLE_HA_DISCOVERY
void Main_ScheduleHomeAssistantDiscovery(int seconds);
#endif
int Main_IsConnectedToWiFi();
int Main_IsOpenAccessPointMode();
void Main_Init();
bool Main_HasFastConnect();
void Main_OnEverySecond();
int Main_HasMQTTConnected();
int Main_HasWiFiConnected();
void Main_OnPingCheckerReply(int ms);

// new_ping.c
#if ENABLE_PING_WATCHDOG
void Main_SetupPingWatchDog(const char *target/*, int delayBetweenPings_Seconds*/);
int PingWatchDog_GetTotalLost();
int PingWatchDog_GetTotalReceived();
#endif

// my addon to LWIP library

int LWIP_GetMaxSockets();
int LWIP_GetActiveSockets();

#ifndef LINUX
#if !PLATFORM_ESPIDF && !PLATFORM_ESP8266 && !PLATFORM_REALTEK_NEW && !PLATFORM_TXW81X
//delay function do 10*r nops, because rtos_delay_milliseconds is too much
void usleep(int r);
#endif
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

#if PLATFORM_LN882H || PLATFORM_REALTEK || PLATFORM_ECR6600 || PLATFORM_TR6260 || PLATFORM_XRADIO \
 || PLATFORM_TXW81X || PLATFORM_LN8825 || PLATFORM_ESP8266
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
extern char g_wifi_bssid[33];
extern uint8_t g_wifi_channel;

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

