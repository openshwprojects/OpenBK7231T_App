#if defined(PLATFORM_W800) || defined(PLATFORM_W600)
#include "../hal_flashVars.h"
#include "../../logging/logging.h"
#include "../../new_pins.h"

FLASH_VARS_STRUCTURE flash_vars;
static int FLASH_VARS_STRUCTURE_SIZE = sizeof(FLASH_VARS_STRUCTURE);

#include "easyflash.h"

#define SAVE_CHANGE_IF_REQUIRED_AND_COUNT(target, source, counter) \
	if((target) != (source)) { \
		(target) = (source); \
		counter++; \
	}
static const char extrahdr[] = "#######";

void initFlashIfNeeded();

#define KV_KEY_FLASH_VARS "OBK_FV"

/// @brief This prints the current boot count as a way to visually verify the flash write operation.
void print_flash_boot_count()
{
	FLASH_VARS_STRUCTURE data;
	ef_get_env_blob(KV_KEY_FLASH_VARS, &data, FLASH_VARS_STRUCTURE_SIZE, NULL);

	ADDLOG_DEBUG(LOG_FEATURE_CFG, "boot count %d, boot success %d, bootfailures %d",
		data.boot_count,
		data.boot_success_count,
		data.boot_count - data.boot_success_count
	);
}

void write_flash_boot_content()
{
	ef_set_env_blob(KV_KEY_FLASH_VARS, &flash_vars, FLASH_VARS_STRUCTURE_SIZE);
	print_flash_boot_count();
}

/// @brief Update the boot count in flash. This is called called at startup. This is what initializes flash_vars.
void HAL_FlashVars_IncreaseBootCount()
{
	initFlashIfNeeded();
	ef_get_env_blob(KV_KEY_FLASH_VARS, &flash_vars, FLASH_VARS_STRUCTURE_SIZE, NULL);

	flash_vars.boot_count++;
	ADDLOG_INFO(LOG_FEATURE_CFG, "%s Boot Count %d %s", extrahdr, flash_vars.boot_count, extrahdr);
	write_flash_boot_content();
}

void HAL_FlashVars_SaveChannel(int index, int value) {
	if (index < 0 || index >= MAX_RETAIN_CHANNELS) {
		ADDLOG_INFO(LOG_FEATURE_CFG, "%s Flash Save Can't Save Channel %d as %d (not enough space in array) %s", extrahdr, index, value, extrahdr);
		return;
	}

	flash_vars.savedValues[index] = value;
	write_flash_boot_content();
}

// call once started (>30s?)
void HAL_FlashVars_SaveBootComplete() {
	ADDLOG_INFO(LOG_FEATURE_CFG, "%s Set Boot Complete %s", extrahdr, extrahdr);
	flash_vars.boot_success_count = flash_vars.boot_count;
	write_flash_boot_content();
}

// call to return the number of boots since a HAL_FlashVars_SaveBootComplete
int HAL_FlashVars_GetBootFailures() {
	int diff = flash_vars.boot_count - flash_vars.boot_success_count;
	ADDLOG_INFO(LOG_FEATURE_CFG, "%s HAL_FlashVars_GetBootFailures= %d", extrahdr, diff);
	return diff;
}

int HAL_FlashVars_GetBootCount() {
	return flash_vars.boot_count;
}
int HAL_FlashVars_GetChannelValue(int ch) {
	if (ch < 0 || ch >= MAX_RETAIN_CHANNELS) {
		ADDLOG_INFO(LOG_FEATURE_CFG, "%s Flash Save Can't Get Channel %d (not enough space in array) %s", extrahdr, ch, extrahdr);
		return 0;
	}

	return flash_vars.savedValues[ch];
}


void HAL_FlashVars_SaveLED(byte mode, short brightness, short temperature, byte r, byte g, byte b, byte bEnableAll) {
#ifndef DISABLE_FLASH_VARS_VARS
	int iChangesCount = 0;
	ef_get_env_blob(KV_KEY_FLASH_VARS, &flash_vars, FLASH_VARS_STRUCTURE_SIZE, NULL);

	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 1], brightness, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 2], temperature, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 3], mode, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 4], bEnableAll, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.rgb[0], r, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.rgb[1], g, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.rgb[2], b, iChangesCount);

	if (iChangesCount > 0) {
		ADDLOG_INFO(LOG_FEATURE_CFG, "%s Flash Save LED %s", extrahdr, extrahdr);
		ef_set_env_blob(KV_KEY_FLASH_VARS, &flash_vars, FLASH_VARS_STRUCTURE_SIZE);
	}
#endif
}
void HAL_FlashVars_ReadLED(byte* mode, short* brightness, short* temperature, byte* rgb, byte* bEnableAll) {
#ifndef DISABLE_FLASH_VARS_VARS
	* bEnableAll = flash_vars.savedValues[MAX_RETAIN_CHANNELS - 4];
	*mode = flash_vars.savedValues[MAX_RETAIN_CHANNELS - 3];
	*temperature = flash_vars.savedValues[MAX_RETAIN_CHANNELS - 2];
	*brightness = flash_vars.savedValues[MAX_RETAIN_CHANNELS - 1];
	rgb[0] = flash_vars.rgb[0];
	rgb[1] = flash_vars.rgb[1];
	rgb[2] = flash_vars.rgb[2];
#endif
}


int HAL_GetEnergyMeterStatus(ENERGY_METERING_DATA* data)
{
	/* default values */
	if (data != NULL)
	{
		memset(data, 0, sizeof(ENERGY_METERING_DATA));
		data->actual_mday = -1;
	}
	return 0;
}

int HAL_SetEnergyMeterStatus(ENERGY_METERING_DATA* data)
{
	return 0;
}

void HAL_FlashVars_SaveTotalConsumption(float total_consumption)
{
}

#endif
