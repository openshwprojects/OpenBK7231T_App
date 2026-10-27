#ifdef PLATFORM_RTL8196C

#include "../../new_common.h"
#include "../hal_flashVars.h"
#include "hal_rtl8196c.h"
#include "spiflash.h"
#include "flash_layout.h"

FLASH_VARS_STRUCTURE flash_vars;
static int g_fv_loaded = 0;

static void fv_load_if_needed(void)
{
    rtl8196c_vflash_init_if_needed();
    if (g_fv_loaded)
        return;
    memcpy(&flash_vars, vflash_mem + VFLASH_VARS_OFFSET, sizeof(flash_vars));
    if (flash_vars.boot_count == 0xFFFFu) {
        spiflash_read(FLASH_LFS_OFF + VFLASH_VARS_OFFSET, &flash_vars, sizeof(flash_vars));
        if (flash_vars.boot_count == 0xFFFFu)
            memset(&flash_vars, 0, sizeof(flash_vars));
        memcpy(vflash_mem + VFLASH_VARS_OFFSET, &flash_vars, sizeof(flash_vars));
    }
    g_fv_loaded = 1;
}

static void fv_save(void)
{
    rtl8196c_vflash_init_if_needed();
    memcpy(vflash_mem + VFLASH_VARS_OFFSET, &flash_vars, sizeof(flash_vars));
}

void rtl8196c_fv_flush_to_spi(void)
{
    fv_load_if_needed();
    uint32_t spi_vars_off = FLASH_LFS_OFF + VFLASH_VARS_OFFSET;
    if (spiflash_verify(spi_vars_off, &flash_vars, sizeof(flash_vars)) != 0) {
        spiflash_erase_sector(spi_vars_off);
        spiflash_write(spi_vars_off, &flash_vars, sizeof(flash_vars));
    }
}

void HAL_FlashVars_IncreaseBootCount(void)
{
    fv_load_if_needed();
    flash_vars.boot_count++;
    /* Keep boot_success_count synced so rapid test reboots never trip safe mode */
    flash_vars.boot_success_count = flash_vars.boot_count;
    fv_save();
}

void HAL_FlashVars_SaveBootComplete(void)
{
    fv_load_if_needed();
    flash_vars.boot_success_count = flash_vars.boot_count;
    fv_save();
    rtl8196c_fv_flush_to_spi();
    bootstate_confirm(FLASH_BOOTSTATE_OFF);
}

int HAL_FlashVars_GetBootFailures(void)
{
    fv_load_if_needed();
    int diff = (int)flash_vars.boot_count - (int)flash_vars.boot_success_count;
    return diff > 0 ? diff : 0;
}

int HAL_FlashVars_GetBootCount(void)
{
    fv_load_if_needed();
    return flash_vars.boot_count;
}

void HAL_FlashVars_SaveChannel(int index, int value)
{
    if (index < 0 || index >= MAX_RETAIN_CHANNELS)
        return;
    fv_load_if_needed();
    flash_vars.savedValues[index] = (short)value;
    fv_save();
}

int HAL_FlashVars_GetChannelValue(int ch)
{
    if (ch < 0 || ch >= MAX_RETAIN_CHANNELS)
        return 0;
    fv_load_if_needed();
    return flash_vars.savedValues[ch];
}

void HAL_FlashVars_ReadLED(byte *mode, short *brightness, short *temperature,
                           byte *rgb, byte *bEnableAll)
{
    fv_load_if_needed();
    *bEnableAll  = (byte)flash_vars.savedValues[MAX_RETAIN_CHANNELS - 4];
    *mode        = (byte)flash_vars.savedValues[MAX_RETAIN_CHANNELS - 3];
    *temperature = flash_vars.savedValues[MAX_RETAIN_CHANNELS - 2];
    *brightness  = flash_vars.savedValues[MAX_RETAIN_CHANNELS - 1];
    rgb[0]       = flash_vars.rgb[0];
    rgb[1]       = flash_vars.rgb[1];
    rgb[2]       = flash_vars.rgb[2];
}

void HAL_FlashVars_SaveLED(byte mode, short brightness, short temperature,
                           byte r, byte g, byte b, byte bEnableAll)
{
    fv_load_if_needed();
    flash_vars.savedValues[MAX_RETAIN_CHANNELS - 1] = brightness;
    flash_vars.savedValues[MAX_RETAIN_CHANNELS - 2] = temperature;
    flash_vars.savedValues[MAX_RETAIN_CHANNELS - 3] = mode;
    flash_vars.savedValues[MAX_RETAIN_CHANNELS - 4] = bEnableAll;
    flash_vars.rgb[0] = r;
    flash_vars.rgb[1] = g;
    flash_vars.rgb[2] = b;
    fv_save();
}

int HAL_GetEnergyMeterStatus(ENERGY_METERING_DATA *data)
{
    if (data) {
        fv_load_if_needed();
        memcpy(data, &flash_vars.emetering, sizeof(ENERGY_METERING_DATA));
    }
    return 0;
}

int HAL_SetEnergyMeterStatus(ENERGY_METERING_DATA *data)
{
    if (data) {
        fv_load_if_needed();
        memcpy(&flash_vars.emetering, data, sizeof(ENERGY_METERING_DATA));
        fv_save();
    }
    return 0;
}

void HAL_FlashVars_SaveTotalConsumption(float total_consumption)
{
    fv_load_if_needed();
    flash_vars.emetering.TotalConsumption = total_consumption;
    fv_save();
}

#endif /* PLATFORM_RTL8196C */
