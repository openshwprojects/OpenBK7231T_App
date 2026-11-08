#include "../../new_common.h"
#include "../hal_flashVars.h"
#include "../../logging/logging.h"
#include "hal_88paax01.h"

FLASH_VARS_STRUCTURE g_flashVars;
static int g_vars_loaded = 0;

static void flash_vars_load(void)
{
	if (g_vars_loaded)
		return;
	HAL_FlashRead((char *)&g_flashVars, sizeof(g_flashVars), (int)VFLASH_VARS_OFF);
	if (g_flashVars.boot_count == 0xFFFFu) {
		memset(&g_flashVars, 0, sizeof(g_flashVars));
	}
	g_vars_loaded = 1;
}

static void flash_vars_save(void)
{
	flash_vars_load();
	/* Never trigger safe-mode boot-loop lockouts on 88PAAX01 */
	g_flashVars.boot_count = 0;
	g_flashVars.boot_success_count = 0;
	HAL_FlashEraseSector((int)VFLASH_VARS_OFF);
	HAL_FlashWrite((char *)&g_flashVars, sizeof(g_flashVars), VFLASH_VARS_OFF);
}

void HAL_FlashVars_IncreaseBootCount(void)
{
	flash_vars_load();
	g_flashVars.boot_count = 0;
}

void HAL_FlashVars_SaveBootComplete(void)
{
	flash_vars_load();
	g_flashVars.boot_count = 0;
}

int HAL_FlashVars_GetBootCount(void)
{
	flash_vars_load();
	return 0;
}

int HAL_FlashVars_GetBootFailures(void)
{
	flash_vars_load();
	return 0;
}

void HAL_FlashVars_SaveChannel(int index, int value)
{
	flash_vars_load();
	if (index < 0 || index >= MAX_RETAIN_CHANNELS)
		return;
	g_flashVars.savedValues[index] = (short)value;
	flash_vars_save();
}

void HAL_FlashVars_SaveLED(byte mode, short brightness, short temperature,
                           byte r, byte g, byte b, byte bEnableAll)
{
	flash_vars_load();
	g_flashVars.savedValues[MAX_RETAIN_CHANNELS - 1] = brightness;
	g_flashVars.savedValues[MAX_RETAIN_CHANNELS - 2] = temperature;
	g_flashVars.savedValues[MAX_RETAIN_CHANNELS - 3] = mode;
	g_flashVars.savedValues[MAX_RETAIN_CHANNELS - 4] = bEnableAll;
	g_flashVars.rgb[0] = r;
	g_flashVars.rgb[1] = g;
	g_flashVars.rgb[2] = b;
	flash_vars_save();
}

void HAL_FlashVars_ReadLED(byte *mode, short *brightness, short *temperature,
                           byte *rgb, byte *bEnableAll)
{
	flash_vars_load();
	*bEnableAll  = (byte)g_flashVars.savedValues[MAX_RETAIN_CHANNELS - 4];
	*mode        = (byte)g_flashVars.savedValues[MAX_RETAIN_CHANNELS - 3];
	*temperature = g_flashVars.savedValues[MAX_RETAIN_CHANNELS - 2];
	*brightness  = g_flashVars.savedValues[MAX_RETAIN_CHANNELS - 1];
	rgb[0]       = g_flashVars.rgb[0];
	rgb[1]       = g_flashVars.rgb[1];
	rgb[2]       = g_flashVars.rgb[2];
}

int HAL_FlashVars_GetChannelValue(int ch)
{
	flash_vars_load();
	if (ch < 0 || ch >= MAX_RETAIN_CHANNELS)
		return 0;
	return g_flashVars.savedValues[ch];
}

void HAL_FlashVars_SaveTotalConsumption(float total_consumption)
{
	(void)total_consumption;
}

float HAL_FlashVars_GetTotalConsumption(void)
{
	return 0.0f;
}

void HAL_FlashVars_SaveTotalCounter(int index, float value)
{
	(void)index;
	(void)value;
}

float HAL_FlashVars_GetTotalCounter(int index)
{
	(void)index;
	return 0.0f;
}
