#if defined(PLATFORM_W800) || defined(PLATFORM_W600)
#include "../hal_flashVars.h"
#include "../../logging/logging.h"
#include "../../new_pins.h"

FLASH_VARS_STRUCTURE flash_vars;
static int FLASH_VARS_STRUCTURE_SIZE = sizeof(FLASH_VARS_STRUCTURE);

//W800 - 0x1F0303 is based on sdk\OpenW600\demo\wm_flash_demo.c
//W600 - 0xF0000 is based on sdk\OpenW600\demo\wm_flash_demo.c
//2528 was picked based on current sizeof(mainConfig_t) which is 2016 with 512 buffer bytes.

#if defined(PLATFORM_W800) 
#define FLASH_VARS_STRUCTURE_ADDR (0x1F0303 + 2528)
#else
#define FLASH_VARS_STRUCTURE_ADDR (0xF0000 + 2528)
#endif


void initFlashIfNeeded();


/// @brief This prints the current boot count as a way to visually verify the flash write operation.
void print_flash_boot_count() {
	FLASH_VARS_STRUCTURE data;
	tls_fls_read(FLASH_VARS_STRUCTURE_ADDR, &data, FLASH_VARS_STRUCTURE_SIZE);

	ADDLOG_DEBUG(LOG_FEATURE_CFG, "boot count %d, boot success %d, bootfailures %d",
		data.boot_count,
		data.boot_success_count,
		data.boot_count - data.boot_success_count
	);
}

void write_flash_boot_content() {
	tls_fls_write(FLASH_VARS_STRUCTURE_ADDR, &flash_vars, FLASH_VARS_STRUCTURE_SIZE);
	print_flash_boot_count();
}

/// @brief Update the boot count in flash. This is called called at startup. This is what initializes flash_vars.
void HAL_FlashVars_IncreaseBootCount() {
	initFlashIfNeeded();
	tls_fls_read(FLASH_VARS_STRUCTURE_ADDR, &flash_vars, FLASH_VARS_STRUCTURE_SIZE);

	flash_vars.boot_count++;
	ADDLOG_INFO(LOG_FEATURE_CFG, "####### Boot Count %d #######", flash_vars.boot_count);
	write_flash_boot_content();
}
void HAL_FlashVars_SaveChannel(int index, int value) {
	if (index < 0 || index >= MAX_RETAIN_CHANNELS) {
		ADDLOG_INFO(LOG_FEATURE_CFG, "####### Flash Save Can't Save Channel %d as %d (not enough space in array) #######", index, value);
		return;
	}

	flash_vars.savedValues[index] = value;
	write_flash_boot_content();
}

// call once started (>30s?)
void HAL_FlashVars_SaveBootComplete() {
	ADDLOG_INFO(LOG_FEATURE_CFG, "####### Set Boot Complete #######");
	flash_vars.boot_success_count = flash_vars.boot_count;
	write_flash_boot_content();
}

// call to return the number of boots since a HAL_FlashVars_SaveBootComplete
int HAL_FlashVars_GetBootFailures() {
	int diff = flash_vars.boot_count - flash_vars.boot_success_count;
	ADDLOG_INFO(LOG_FEATURE_CFG, "####### HAL_FlashVars_GetBootFailures= %d", diff);
	return diff;
}

int HAL_FlashVars_GetBootCount() {
	return flash_vars.boot_count;
}
int HAL_FlashVars_GetChannelValue(int ch) {
	if (ch < 0 || ch >= MAX_RETAIN_CHANNELS) {
		ADDLOG_INFO(LOG_FEATURE_CFG, "####### Flash Save Can't Get Channel %d (not enough space in array) #######", ch);
		return 0;
	}

	return flash_vars.savedValues[ch];
}
void HAL_FlashVars_SaveLED(byte mode, short brightness, short temperature, byte r, byte g, byte b, byte bEnableAll) {

}
void HAL_FlashVars_ReadLED(byte* mode, short* brightness, short* temperature, byte* rgb, byte* bEnableAll) {

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
