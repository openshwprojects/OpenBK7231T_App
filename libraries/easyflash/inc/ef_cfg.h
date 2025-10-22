/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2015-2019, Armink, <armink.ztl@gmail.com>
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
 * Function: It is the configure head file for this library.
 * Created on: 2015-07-14
 */


#ifndef EF_CFG_H_
#define EF_CFG_H_

#define PKG_EASYFLASH_ENV

/* using ENV function */
#ifdef PKG_EASYFLASH_ENV
#define EF_USING_ENV

#ifdef PKG_EASYFLASH_ENV_AUTO_UPDATE
/* Auto update ENV to latest default when current ENV version number is changed. */
#define EF_ENV_AUTO_UPDATE
/**
 * ENV version number defined by user.
 * Please change it when your firmware add a new ENV to default_env_set.
 */
#define EF_ENV_VER_NUM            PKG_EASYFLASH_ENV_VER_NUM
#endif

#endif /* PKG_EASYFLASH_ENV */

/* using IAP function */
#ifdef PKG_EASYFLASH_IAP
#define EF_USING_IAP
#endif

/* using save log function */
#ifdef PKG_EASYFLASH_LOG
#define EF_USING_LOG
/* saved log area size */
#define LOG_AREA_SIZE             (PKG_EASYFLASH_LOG_AREA_SIZE)
#endif

/* the minimum size of flash erasure */
#define EF_ERASE_MIN_SIZE         4096

/* the flash write granularity, unit: bit
 * only support 1(nor flash)/ 8(stm32f4)/ 32(stm32f1)/ 64(stm32l4) */
#define EF_WRITE_GRAN             1

/*
 *
 * This all Backup Area Flash storage index. All used flash area configure is under here.
 * |----------------------------|   Storage Size
 * | Environment variables area |   ENV area size @see ENV_AREA_SIZE
 * |----------------------------|
 * |      Saved log area        |   Log area size @see LOG_AREA_SIZE
 * |----------------------------|
 * |(IAP)Downloaded application |   IAP already downloaded application, unfixed size
 * |----------------------------|
 *
 * @note all area sizes must be aligned with EF_ERASE_MIN_SIZE
 *
 * The EasyFlash add the NG (Next Generation) mode start from V4.0. All old mode before V4.0, called LEGACY mode.
 *
 * - NG (Next Generation) mode is default mode from V4.0. It's easy to settings, only defined the ENV_AREA_SIZE.
 * - The LEGACY mode has been DEPRECATED. It is NOT RECOMMENDED to continue using.
 *   Beacuse it will use ram to buffer the ENV and spend more flash erase times.
 *   If you want use it please using the V3.X version.
 */

#ifdef PLATFORM_RTL87X0C

/* backup area start address */
#define EF_START_ADDR             0x1F0000

/* ENV area size. It's at least one empty sector for GC. So it's definition must more then or equal 2 flash sector size. */
#define ENV_AREA_SIZE             0x10000 /* default is the double erase min size */

#elif PLATFORM_RTL8710B

#define EF_START_ADDR             0x195000
#define ENV_AREA_SIZE             0x8000

#elif PLATFORM_RTL8710A

#define EF_START_ADDR             0x2E3000
#define ENV_AREA_SIZE             0x1C000

#elif PLATFORM_RTL8720D

#define EF_START_ADDR             0x1B0000
#define ENV_AREA_SIZE             0x40000

#elif PLATFORM_W800

#define EF_START_ADDR             0x1DB000
// extend to 0x21000 if moving obk config to ef
#define ENV_AREA_SIZE             0x15000

#elif PLATFORM_XR809

#define EF_START_ADDR             0x1E0000
#define ENV_AREA_SIZE             0x10000

#elif PLATFORM_XR806

#define EF_START_ADDR             0x1F0000
#define ENV_AREA_SIZE             0x10000

#elif PLATFORM_XR872

#define EF_START_ADDR             0xEF000
#define ENV_AREA_SIZE             0x8000

#elif PLATFORM_TXW81X

#define EF_START_ADDR             0xEF000
#define ENV_AREA_SIZE             0x6000

#elif PLATFORM_RDA5981

#define EF_START_ADDR             0xF4000
#define ENV_AREA_SIZE             0xA000

#elif WINDOWS

#define EF_START_ADDR             0
extern uint32_t ENV_AREA_SIZE;
#define DllExport __declspec(dllexport)

#elif LINUX

#define EF_START_ADDR             0
extern uint32_t ENV_AREA_SIZE;
#define DllExport __attribute__((dllexport))

#endif
/* print debug information of flash */
#ifdef PKG_EASYFLASH_DEBUG
#define PRINT_DEBUG
#endif

#ifndef DllExport
#define DllExport
#endif

#endif /* EF_CFG_H_ */
