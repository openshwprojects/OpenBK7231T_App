/**
 * @file    wm_flash_map.h
 *
 * @brief   flash zone map
 *
 * @author  dave
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd.
 */
#ifndef __WM_FLASH_MAP_H__
#define __WM_FLASH_MAP_H__

/**FLASH MAP**/

/**Flash Base Address */
#define FLASH_BASE_ADDR						(0x8000000UL)

/**Upgrade image area*/
#define CODE_UPD_START_ADDR					(0x8010000UL)

/**Run-time image header area*/
#define CODE_RUN_START_ADDR                 (0x80D0000UL)

/**Area can be used by User*/
#define USER_ADDR_START						(0x81E0000UL)


/**System parameter defined in wm_internal_fls.c*/
extern unsigned int TLS_FLASH_MESH_PARAM_ADDR;
extern unsigned int TLS_FLASH_PARAM_DEFAULT;
extern unsigned int TLS_FLASH_PARAM1_ADDR;
extern unsigned int TLS_FLASH_PARAM2_ADDR;
extern unsigned int TLS_FLASH_PARAM_RESTORE_ADDR;
extern unsigned int TLS_FLASH_OTA_FLAG_ADDR;
extern unsigned int TLS_FLASH_END_ADDR;

#define SIGNATURE_WORD      				(0xA0FFFF9FUL)
#define IMAGE_START_ADDR_MSK			    (0x400)
#endif /*__WM_CONFIG_H__*/

