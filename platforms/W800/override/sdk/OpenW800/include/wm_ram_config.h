/**
 * @file    wm_ram_config.h
 *
 * @brief   WM ram model configure
 *
 * @author  winnermicro
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */
#ifndef __WM_RAM_CONFIG_H__
#define __WM_RAM_CONFIG_H__
#include "wm_config.h"

/*see gcc_csky.ld in directory ld/w800,__heap_end must be bigger than 0x20028000
if __heap_end is lower than 0x20028000,then SLAVE_HSPI_SDIO_ADDR must be changed to 0x20028000 or bigger.
*/
extern unsigned int __heap_end;
extern unsigned int __heap_start;

/*High speed SPI or SDIO buffer to exchange data*/
#define SLAVE_HSPI_SDIO_ADDR        ((unsigned int)(&__heap_end))

#if TLS_CONFIG_HS_SPI
#define SLAVE_HSPI_MAX_SIZE         (0x2000)
#else
#define SLAVE_HSPI_MAX_SIZE         (0x0)
#endif

/*Wi-Fi use buffer to exchange data*/
#define WIFI_MEM_START_ADDR		(SLAVE_HSPI_SDIO_ADDR + SLAVE_HSPI_MAX_SIZE)

/*Store reboot reason by RAM's Last Word*/
#define SYS_REBOOT_REASON_ADDRESS (0x20047EFC)

enum SYS_REBOOT_REASON
{
	REBOOT_REASON_POWER_ON 	   = 0,   /*power on or reset button*/
	REBOOT_REASON_STANDBY	   = 1,   /*chip standby*/
	REBOOT_REASON_EXCEPTION    = 2,   /*exception reset*/
	REBOOT_REASON_WDG_TIMEOUT  = 3,   /*watchdog timeout*/
	REBOOT_REASON_ACTIVE       = 4,   /*user active reset*/
	REBOOT_REASON_MAX
};

#endif /*__WM_RAM_CONFIG_H__*/

