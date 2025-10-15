/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for SFUD flash driver.
 * Created on: 2015-01-16
 */

#include <easyflash.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#if !WINDOWS && !PLATFORM_TXW81X && !PLATFORM_RDA5981 && !LINUX
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#endif

#if PLATFORM_REALTEK

#include "flash_api.h"
#include "device_lock.h"

flash_t flash;

#elif defined(PLATFORM_W800) || defined(PLATFORM_W600)

#include "wm_internal_flash.h"
#include "wm_flash.h"
#define QueueHandle_t xQueueHandle

#elif PLATFORM_XRADIO

#include "driver/chip/hal_flash.h"
#include <image/flash.h>
#define QueueHandle_t xQueueHandle

#elif PLATFORM_TXW81X

#include "sys_config.h"
#include "typesdef.h"
#include "csi_kernel.h"
#include "osal/csky/defs.h"
#include "dev.h"
#include "hal/spi_nor.h"
#include "lib/syscfg/syscfg.h"
#include "osal/csky/string.h"

typedef k_mutex_handle_t QueueHandle_t;
#define xSemaphoreCreateMutex csi_kernel_mutex_new
#define xSemaphoreTake(a, b) csi_kernel_mutex_lock(a, b, 0)
#define xSemaphoreGive(a) csi_kernel_mutex_unlock(a)
extern struct spi_nor_flash* obk_flash;

#elif PLATFORM_RDA5981

#include "stdbool.h"
#include "rda_sys_wrapper.h"

typedef void* QueueHandle_t;
#define xSemaphoreCreateMutex rda_mutex_create
#define xSemaphoreTake rda_mutex_wait
#define xSemaphoreGive rda_mutex_realease

#elif WINDOWS

#include "framework.h"

#define QueueHandle_t HANDLE
extern QueueHandle_t ef_mutex;

BYTE* env_area = NULL;
uint32_t ENV_AREA_SIZE = 0;

DllExport BYTE* get_env_area(void)
{
	return env_area;
}

DllExport void set_env_size(uint32_t size)
{
	ENV_AREA_SIZE = size;
	if(env_area) free(env_area);
	env_area = malloc(size * sizeof(BYTE));
}

HANDLE xSemaphoreCreateMutex()
{
	return CreateMutex(NULL, FALSE, NULL);
}

void xSemaphoreTake(HANDLE handle, int time)
{
	WaitForSingleObject(ef_mutex, time);
}

void xSemaphoreGive(HANDLE handle)
{
	ReleaseMutex(ef_mutex);
}

#elif LINUX

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#define QueueHandle_t pthread_mutex_t
extern QueueHandle_t ef_mutex;

uint8_t* env_area = NULL;
uint32_t ENV_AREA_SIZE = 0;

DllExport uint8_t* get_env_area(void)
{
	return env_area;
}

DllExport void set_env_size(uint32_t size)
{
	ENV_AREA_SIZE = size;
	if(env_area) free(env_area);
	env_area = malloc(size * sizeof(uint8_t));
}

QueueHandle_t xSemaphoreCreateMutex()
{
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_init(&mutex, NULL);
	return mutex;
}

void xSemaphoreTake(QueueHandle_t handle, int time)
{
	pthread_mutex_lock(&handle);
}

void xSemaphoreGive(QueueHandle_t handle)
{
	pthread_mutex_unlock(&handle);
}

#endif

/* default ENV set for user */
static const ef_env default_env_set[] =
{
	{"nv_version","0.01"}
};

QueueHandle_t ef_mutex;

/**
 * Flash port for hardware initialize.
 *
 * @param default_env default ENV set for user
 * @param default_env_size default ENV size
 *
 * @return result
 */
EfErrCode ef_port_init(ef_env const** default_env, size_t* default_env_size)
{
	EfErrCode result = EF_NO_ERR;

	*default_env = default_env_set;
	*default_env_size = sizeof(default_env_set) / sizeof(default_env_set[0]);

	ef_mutex = xSemaphoreCreateMutex();
#if defined(PLATFORM_W800) || defined(PLATFORM_W600)
	tls_fls_init();
#endif

	return result;
}

/**
 * Read data from flash.
 * @note This operation's units is word.
 *
 * @param addr flash address
 * @param buf buffer to store read data
 * @param size read bytes size
 *
 * @return result
 */
EfErrCode ef_port_read(uint32_t addr, uint32_t* buf, size_t size)
{
#if PLATFORM_REALTEK
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	int res = flash_stream_read(&flash, addr, size, buf);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	if(res) return EF_NO_ERR;
	else return EF_READ_ERR;
#elif defined(PLATFORM_W800) || defined(PLATFORM_W600)
	int res = tls_fls_read(addr, (uint8_t*)buf, size);
	if(res != TLS_FLS_STATUS_OK) return EF_READ_ERR;
	else return EF_NO_ERR;
#elif PLATFORM_XRADIO
	int res = flash_rw(0, addr, (void*)buf, size, 0);
	if(res == 0) res = EF_READ_ERR;
	else res = EF_NO_ERR;
	return res;
#elif WINDOWS || LINUX
	memcpy(buf, env_area + addr, size);
	return EF_NO_ERR;
#elif PLATFORM_TXW81X || PLATFORM_RDA5981
	HAL_FlashRead(buf, size, addr);
	return EF_NO_ERR;
#endif
}

/**
 * Erase data on flash.
 * @note This operation is irreversible.
 * @note This operation's units is different which on many chips.
 *
 * @param addr flash address
 * @param size erase bytes size
 *
 * @return result
 */
EfErrCode ef_port_erase(uint32_t addr, size_t size)
{
	EfErrCode result = EF_NO_ERR;

	/* make sure the start address is a multiple of FLASH_ERASE_MIN_SIZE */
	EF_ASSERT(addr % EF_ERASE_MIN_SIZE == 0);

#if PLATFORM_REALTEK
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_erase_sector(&flash, addr);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

#elif defined(PLATFORM_W800) || defined(PLATFORM_W600)
	int res = tls_fls_erase(addr / EF_ERASE_MIN_SIZE);
	if(res != TLS_FLS_STATUS_OK) return EF_ERASE_ERR;
	else return EF_NO_ERR;
#elif PLATFORM_XRADIO
	int res = flash_erase(0, addr, EF_ERASE_MIN_SIZE);
	if(res != 0) res = EF_ERASE_ERR;
	else res = EF_NO_ERR;
	return res;
#elif WINDOWS || LINUX
	memset(env_area + addr, 0xFF, size);
#elif PLATFORM_TXW81X || PLATFORM_RDA5981
	HAL_FlashEraseSector(addr);
	return EF_NO_ERR;
#endif
	return result;
}
/**
 * Write data to flash.
 * @note This operation's units is word.
 * @note This operation must after erase. @see flash_erase.
 *
 * @param addr flash address
 * @param buf the write data buffer
 * @param size write bytes size
 *
 * @return result
 */
EfErrCode ef_port_write(uint32_t addr, const uint32_t* buf, size_t size)
{
#if PLATFORM_REALTEK
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	int res = flash_stream_write(&flash, addr, size, buf);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	if(res) return EF_NO_ERR;
	else return EF_WRITE_ERR;

#elif defined(PLATFORM_W800) || defined(PLATFORM_W600)
	int res = tls_fls_write(addr, (uint8_t*)buf, size);
	if(res != TLS_FLS_STATUS_OK) return EF_WRITE_ERR;
	else return EF_NO_ERR;
#elif PLATFORM_XRADIO
	int res = flash_rw(0, addr, (void*)buf, size, 1);
	if(res == 0) res = EF_WRITE_ERR;
	else res = EF_NO_ERR;
	return res;
#elif WINDOWS || LINUX
	memcpy(env_area + addr, buf, size);
	return EF_NO_ERR;
#elif PLATFORM_TXW81X || PLATFORM_RDA5981
	HAL_FlashWrite(buf, size, addr);
	return EF_NO_ERR;
#endif
}

/**
 * lock the ENV ram cache
 */
void ef_port_env_lock(void)
{
	xSemaphoreTake(ef_mutex, 0xFFFFFFFF);
}

/**
 * unlock the ENV ram cache
 */
void ef_port_env_unlock(void)
{
	xSemaphoreGive(ef_mutex);
}

/**
 * This function is print flash debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 *
 */
void ef_log_debug(const char* file, const long line, const char* format, ...)
{

}

/**
 * This function is print flash routine info.
 *
 * @param format output format
 * @param ... args
 */
void ef_log_info(const char* format, ...)
{
	va_list args;

	/* args point to the first variable parameter */
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}
/**
 * This function is print flash non-package info.
 *
 * @param format output format
 * @param ... args
 */
void ef_print(const char* format, ...)
{
	va_list args;

	/* args point to the first variable parameter */
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}
