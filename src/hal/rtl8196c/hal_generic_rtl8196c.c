#ifdef PLATFORM_RTL8196C

#include "../../new_common.h"
#include "../hal_generic.h"
#include "hal_rtl8196c.h"
#include "spiflash.h"
#include "flash_layout.h"
#include "timer.h"
#include "net.h"
#include "tcp.h"
#include "ota.h"

extern char _start[];

int HAL_FlashRead(char *buffer, int readlen, int startaddr)
{
    rtl8196c_vflash_init_if_needed();
    if (!buffer || readlen <= 0)
        return 0;
    if (startaddr < 0 || (uint32_t)(startaddr + readlen) > FLASH_LFS_SIZE)
        return -1;
    spiflash_read(FLASH_LFS_OFF + (uint32_t)startaddr, buffer, (uint32_t)readlen);
    return 0;
}

int HAL_FlashWrite(char *buf, unsigned int len, unsigned int addr)
{
    rtl8196c_vflash_init_if_needed();
    if (!buf || len == 0)
        return 0;
    if (addr + len > FLASH_LFS_SIZE)
        return -1;
    return spiflash_write(FLASH_LFS_OFF + addr, buf, len);
}

int HAL_FlashEraseSector(int startaddr)
{
    rtl8196c_vflash_init_if_needed();
    if (startaddr < 0 || (uint32_t)(startaddr + SPIFLASH_SECTOR) > FLASH_LFS_SIZE)
        return -1;
    return spiflash_erase_sector(FLASH_LFS_OFF + (uint32_t)startaddr);
}

void HAL_RebootModule(void)
{
    printf("[obk-rtl8196c] Rebooting module...\n");
    rtl8196c_fv_flush_to_spi();
    net_poll();
    tcp_poll();
    delay_ms(50);

    uint32_t load_addr = (uint32_t)(uintptr_t)_start;
    if (vflash_hdr->magic == VFLASH_MAGIC && vflash_hdr->pending_ota_len > 0) {
        uint32_t len = vflash_hdr->pending_ota_len;
        vflash_hdr->pending_ota_len = 0;
        memcpy((void *)(uintptr_t)SNAPSHOT_BASE_ADDR,
               (const void *)(uintptr_t)WEB_OTA_STAGE_ADDR, len);
        vflash_hdr->snapshot_len = len;
        ota_ram_jump((const void *)(uintptr_t)WEB_OTA_STAGE_ADDR, load_addr, len, load_addr);
    }

    if (vflash_hdr->magic == VFLASH_MAGIC && vflash_hdr->snapshot_len > 0) {
        ota_ram_jump((const void *)(uintptr_t)SNAPSHOT_BASE_ADDR, load_addr,
                     vflash_hdr->snapshot_len, load_addr);
    }

    /* Fallback: if a valid A/B slot is installed in SPI flash, boot from SPI flash */
    ota_boot_active_slot(0);
    wdt_reboot();
}

void HAL_Delay_us(int delay)
{
    delay_us((uint32_t)(delay > 0 ? delay : 1));
}

#endif /* PLATFORM_RTL8196C */
