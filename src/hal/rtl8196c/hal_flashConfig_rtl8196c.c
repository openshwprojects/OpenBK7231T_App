#ifdef PLATFORM_RTL8196C

#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../hal_flashConfig.h"
#include "../hal_wifi.h"
#include "hal_rtl8196c.h"
#include "spiflash.h"
#include "flash_layout.h"

struct spi_storage_hdr {
    uint32_t magic;
    uint32_t version;
};

static int g_spi_storage_ready = 0;

void rtl8196c_vflash_init_if_needed(void)
{
    if (vflash_hdr->magic != VFLASH_MAGIC || vflash_hdr->version != 3) {
        memset(vflash_mem, 0xFF, VFLASH_TOTAL_SIZE);
        memset(vflash_hdr, 0, sizeof(*vflash_hdr));
        vflash_hdr->magic = VFLASH_MAGIC;
        vflash_hdr->version = 3;
    }

    if (!g_spi_storage_ready) {
        const struct spi_storage_hdr *sh =
            (const struct spi_storage_hdr *)(uintptr_t)(SPIFLASH_MMAP + FLASH_LFS_OFF);
        if (sh->magic != VFLASH_MAGIC || sh->version != 1u) {
            printf("[obk-rtl8196c] Formatting initial SPI storage at 0x%06x...\n",
                   (unsigned)FLASH_LFS_OFF);
            spiflash_erase_sector(FLASH_LFS_OFF);
            spiflash_erase_sector(FLASH_LFS_OFF + VFLASH_CFG_OFFSET);
            spiflash_erase_sector(FLASH_LFS_OFF + VFLASH_VARS_OFFSET);
            /* Erase LittleFS root metadata pair blocks {0, 1} at +0x10000 */
            spiflash_erase_sector(FLASH_LFS_OFF + 0x010000u);
            spiflash_erase_sector(FLASH_LFS_OFF + 0x011000u);
            struct spi_storage_hdr init_hdr = { VFLASH_MAGIC, 1u };
            spiflash_write(FLASH_LFS_OFF, &init_hdr, sizeof(init_hdr));
        }
        g_spi_storage_ready = 1;
    }
}

int HAL_Configuration_ReadConfigMemory(void *target, int dataLen)
{
    rtl8196c_vflash_init_if_needed();
    if (!target || dataLen <= 0)
        return 0;
    if (VFLASH_CFG_OFFSET + (uint32_t)dataLen > VFLASH_VARS_OFFSET)
        dataLen = (int)(VFLASH_VARS_OFFSET - VFLASH_CFG_OFFSET);
    spiflash_read(FLASH_LFS_OFF + VFLASH_CFG_OFFSET, target, (uint32_t)dataLen);
    if (dataLen >= (int)sizeof(mainConfig_t)) {
        mainConfig_t *cfg = (mainConfig_t *)target;
        if (strcmp(cfg->wifi_ssid, "RTL8196C-Ethernet") == 0) {
            cfg->wifi_ssid[0] = 0;
            cfg->wifi_pass[0] = 0;
            int hdr_sz = (int)((byte *)&cfg->version - (byte *)cfg);
            cfg->crc = Tiny_CRC8((const char *)&cfg->version, (int)sizeof(mainConfig_t) - hdr_sz);
        }
    }
    return dataLen;
}

int HAL_Configuration_SaveConfigMemory(void *src, int dataLen)
{
    rtl8196c_vflash_init_if_needed();
    if (!src || dataLen <= 0)
        return 0;
    if (VFLASH_CFG_OFFSET + (uint32_t)dataLen > VFLASH_VARS_OFFSET)
        dataLen = (int)(VFLASH_VARS_OFFSET - VFLASH_CFG_OFFSET);

    mainConfig_t *cfg = (mainConfig_t *)src;
    /* Migrate legacy placeholder SSID from Ethernet-only bring-up to empty (AP mode by default) */
    if (strcmp(cfg->wifi_ssid, "RTL8196C-Ethernet") == 0) {
        cfg->wifi_ssid[0] = 0;
        cfg->wifi_pass[0] = 0;
        int hdr_sz = (int)((byte *)&cfg->version - (byte *)cfg);
        cfg->crc = Tiny_CRC8((const char *)&cfg->version, (int)sizeof(mainConfig_t) - hdr_sz);
    }
    /* On initial default config creation, populate device name & 7 router LEDs */
    if (cfg->changeCounter <= 1 && cfg->shortDeviceName[0] == 0) {
        cfg->wifi_ssid[0] = 0;
        strcpy(cfg->shortDeviceName, "OpenRTL8196C");
        strcpy(cfg->longDeviceName, "OpenRTL8196C Router");
        WiFI_GetMacAddress((char *)cfg->mac);

        /* Map the 7 physical router LEDs to Channels 1..7 */
        cfg->pins.roles[10] = IOR_LED_n;  cfg->pins.channels[10] = 1; /* PB2: Port0 LED */
        cfg->pins.roles[11] = IOR_LED_n;  cfg->pins.channels[11] = 2; /* PB3: Port1 LED */
        cfg->pins.roles[12] = IOR_LED_n;  cfg->pins.channels[12] = 3; /* PB4: Port2 LED */
        cfg->pins.roles[13] = IOR_LED_n;  cfg->pins.channels[13] = 4; /* PB5: Port3 LED */
        cfg->pins.roles[22] = IOR_LED_n;  cfg->pins.channels[22] = 5; /* PC6: Port4 LED */
        cfg->pins.roles[15] = IOR_LED_n;  cfg->pins.channels[15] = 6; /* PB7: P1 LED */
        cfg->pins.roles[16] = IOR_LED_n;  cfg->pins.channels[16] = 7; /* PC0: P2 LED */
        int hdr_sz = (int)((byte *)&cfg->version - (byte *)cfg);
        cfg->crc = Tiny_CRC8((const char *)&cfg->version, (int)sizeof(mainConfig_t) - hdr_sz);
    }

    memcpy(vflash_mem + VFLASH_CFG_OFFSET, src, (size_t)dataLen);
    uint32_t spi_cfg_off = FLASH_LFS_OFF + VFLASH_CFG_OFFSET;
    if (spiflash_verify(spi_cfg_off, src, (uint32_t)dataLen) != 0) {
        spiflash_erase_sector(spi_cfg_off);
        spiflash_write(spi_cfg_off, src, (uint32_t)dataLen);
    }
    rtl8196c_wifi_notify_cfg_saved();
    return dataLen;
}

#endif /* PLATFORM_RTL8196C */
