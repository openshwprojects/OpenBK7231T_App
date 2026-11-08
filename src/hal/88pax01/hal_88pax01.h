#ifndef HAL_88PAX01_H
#define HAL_88PAX01_H

#include <stdint.h>
#include <stddef.h>

/*
 * Marvell 88PAAX01-BPK2 (AXON, ARM946E-S big-endian) HAL definitions
 * for HP DeskJet 2620 (64 MB DDR SDRAM, 16 MB W25Q128JV SPI NOR Flash,
 * Broadcom BCM43143 SDIO Wi-Fi).
 *
 * Cached RW DRAM is at 0x20000000..0x23FFFFFF (64 MB).
 * We place the 64 KB virtual flash mirror at 0x21400000 and persist it to
 * SPI NOR flash at offset 0x000B0000 (the top 64 KB of Slot A 0x040000..0x0BFFFF,
 * which is inside check_writable() bounds and above the ~220 KB app image).
 */
#define VFLASH_MEM_BASE      0x21400000u
#define VFLASH_TOTAL_SIZE    0x00010000u /* 64 KB (0x0B0000..0x0BFFFF) */

#define VFLASH_CONFIG_OFF    0x00000000u /* 12 KB for hal_config (settings_t) */
#define VFLASH_VARS_OFF      0x00003000u /* 4 KB for hal_flashVars */
#define VFLASH_LFS_OFF       0x00004000u /* 48 KB for LittleFS */
#define VFLASH_LFS_SIZE      0x0000C000u

#define SPI_VFLASH_OFFSET    0x000B0000u /* 0x000B0000 .. 0x000BFFFF in Slot A */

/* Staging buffer for OTA firmware updates (STAGING_ADDR in DRAM) */
#define OTA_BUF_MEM_BASE     0x20400000u
#define OTA_BUF_MAX_SIZE     0x00070000u /* 448 KB max app image size */

void pax_flash_init(void);
int  HAL_FlashRead(char *buffer, int readlen, int startaddr);
int  HAL_FlashWrite(char *buf, unsigned int len, unsigned int addr);
int  HAL_FlashEraseSector(int startaddr);

void HAL_WiFi_Init_88PAX01(void);
void HAL_WiFi_Poll_88PAX01(void);
int  HAL_WiFi_IsApMode(void);

#endif /* HAL_88PAX01_H */
