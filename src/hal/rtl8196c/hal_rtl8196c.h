#ifndef HAL_RTL8196C_H
#define HAL_RTL8196C_H

#ifdef PLATFORM_RTL8196C

#include <stdint.h>
#include <stddef.h>

/*
 * SDRAM layout above heap (0xA0700000..0xA0880000):
 *   0xA0880000..0xA08FFFFF (512 KB): Persistent Virtual Flash Disk ("OBKF")
 *     - 0x000000: 64-byte disk header
 *     - 0x001000: mainConfig_t (4 KB)
 *     - 0x002000: FLASH_VARS_STRUCTURE (4 KB)
 *     - 0x010000..0x070000: LittleFS (384 KB)
 *   0xA0900000..0xA09FFFFF (1 MB): Clean boot image snapshot for soft-reboot
 *   0xA0A00000..0xA0BFFFFF (2 MB): Web/TCP OTA staging buffer
 */
#define VFLASH_BASE_ADDR       0xA0880000u
#define VFLASH_TOTAL_SIZE      (512u * 1024u)
#define VFLASH_MAGIC           0x4F424B46u /* "OBKF" */
#define VFLASH_CFG_OFFSET      0x001000u
#define VFLASH_VARS_OFFSET     0x002000u
#define VFLASH_PMK_OFFSET      0x003000u
#define VFLASH_PMK_MAGIC       0x57504D4Bu /* "WPMK" */

#define SNAPSHOT_BASE_ADDR     0xA0900000u
#define SNAPSHOT_MAX_SIZE      (1024u * 1024u)

#define WEB_OTA_STAGE_ADDR     0xA0A00000u
#define WEB_OTA_MAX_SIZE       (1024u * 1024u)

struct vflash_hdr {
    uint32_t magic;
    uint32_t version;
    uint32_t snapshot_len;
    uint32_t pending_ota_len;
};

struct vflash_wifi_pmk {
    uint32_t magic;
    char     ssid[33];
    char     pass[64];
    uint8_t  pmk[32];
};

#define vflash_mem ((uint8_t *)(uintptr_t)VFLASH_BASE_ADDR)
#define vflash_hdr ((struct vflash_hdr *)(uintptr_t)VFLASH_BASE_ADDR)

void rtl8196c_vflash_init_if_needed(void);
void rtl8196c_fv_flush_to_spi(void);

void rtl8196c_wifi_init(void);
void rtl8196c_wifi_poll(void);
void rtl8196c_wifi_notify_cfg_saved(void);

#endif /* PLATFORM_RTL8196C */
#endif /* HAL_RTL8196C_H */
