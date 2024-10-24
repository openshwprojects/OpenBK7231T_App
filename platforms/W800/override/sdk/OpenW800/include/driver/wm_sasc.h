#ifndef __WM_SASC_H_
#define __WM_SASC_H_

#include <core_804.h>
#include "wm_regs.h"

#define HR_SASC_B1_BASE         (DEVICE_BASE_ADDR + 0xB000)
#define HR_SASC_FLASH_BASE      (DEVICE_BASE_ADDR + 0xB100)
#define HR_SASC_B2_BASE         (DEVICE_BASE_ADDR + 0xB200)

#define _R1_Pos(val, rgn)    ((val&0x1) << (rgn))
#define _R1_Msk(rgn)         (0x1 << (rgn))
#define _R2_Pos(val, rgn)    ((val&0x3) << (2*rgn))
#define _R2_Msk(rgn)         (0x3 << (2*rgn))

typedef enum {
	SASC_UN_SE_USER  =  0,
	SASC_UN_SE_SUPER,
	SASC_SE_USER,
	SASC_SE_SUPER
} sasc_car_e;

typedef enum {
	SASC_AP_RW  =  0,
	SASC_AP_RO,
	SASC_AP_WO,
	SASC_AP_DENYALL
} sasc_ap_e;

typedef enum {
	SASC_CD_DA_OF  =  0,
	SASC_CD_DA,
	SASC_CD_OF,
	SASC_CD_DENYALL
} sasc_cd_e;

typedef enum {
	SASC_REGION_SIZE_4B = 0x5,
	SASC_REGION_SIZE_8B,
	SASC_REGION_SIZE_16B,
	SASC_REGION_SIZE_32B,
	SASC_REGION_SIZE_64B,
	SASC_REGION_SIZE_128B,
	SASC_REGION_SIZE_256B,
	SASC_REGION_SIZE_512B,
	SASC_REGION_SIZE_1KB,
	SASC_REGION_SIZE_2KB,
	SASC_REGION_SIZE_4KB,
	SASC_REGION_SIZE_8KB,
	SASC_REGION_SIZE_16KB,
	SASC_REGION_SIZE_32KB,
	SASC_REGION_SIZE_64KB,
	SASC_REGION_SIZE_128KB,
	SASC_REGION_SIZE_256KB,
	SASC_REGION_SIZE_512KB,
	SASC_REGION_SIZE_1MB,
	SASC_REGION_SIZE_2MB,
	SASC_REGION_SIZE_4MB,
	SASC_REGION_SIZE_8MB
} sasc_region_size_e;

typedef struct {
	sasc_car_e car;               /* security and user or super */
	sasc_ap_e ap;                /* super user and normal user access.*/
	sasc_cd_e cd;                /* instruction fetched excution */
} sasc_region_attr_t;

typedef struct {
	__IOM uint32_t CAR;
	__IOM uint32_t CR;
	__IOM uint32_t AP0;
	__IOM uint32_t CD0;
	__IOM uint32_t AP1;
	__IOM uint32_t CD1;
	__IOM uint32_t AP2;
	__IOM uint32_t CD2;
	__IOM uint32_t REGION[8];
} SASC_Type;

#define SASC_B1                 ((SASC_Type    *) HR_SASC_B1_BASE)
#define SASC_FLASH              ((SASC_Type    *) HR_SASC_FLASH_BASE)
#define SASC_B2                 ((SASC_Type    *) HR_SASC_B2_BASE)

/**
 * @defgroup Driver_APIs Driver APIs
 * @brief Driver APIs
 */

/**
 * @addtogroup Driver_APIs
 * @{
 */

/**
 * @defgroup SASC_Driver_APIs SASC Driver APIs
 * @brief SASC driver APIs
 */

/**
 * @addtogroup SASC_Driver_APIs
 * @{
 */

/**
 * @brief          This function is used enable region.
 *
 * @param[in]      block    sasc type
 * @param[in]      idx      index
 *
 * @return         None
 *
 * @note           None
 */
void wm_sasc_enable_region(SASC_Type *block, uint32_t idx);

/**
 * @brief          This function is used disable region.
 *
 * @param[in]      block    sasc type
 * @param[in]      idx      index
 *
 * @return         None
 *
 * @note           None
 */
void wm_sasc_disable_region(SASC_Type *block, uint32_t idx);

/**
 * @brief          This function is used set region protect.
 *
 * @param[in]      base_addr    base address
 * @param[in]      idx          index
 * @param[in]      size         size
 * @param[in]      attr         attribute
 *
 * @return         None
 *
 * @note           None
 */
void set_region_protect(uint32_t base_addr, uint32_t idx, sasc_region_size_e size, sasc_region_attr_t *attr);

/**
 * @}
 */

/**
 * @}
 */

#endif
