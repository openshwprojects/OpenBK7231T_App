#if defined(PLATFORM_W800) || defined(PLATFORM_W600)
#include "../hal_flashVars.h"
#include "../../logging/logging.h"
#include "../../new_pins.h"

FLASH_VARS_STRUCTURE flash_vars;
static int FLASH_VARS_STRUCTURE_SIZE = sizeof(FLASH_VARS_STRUCTURE);

//W800 - 0x1F0303 is based on sdk\OpenW600\demo\wm_flash_demo.c
//W600 - 0xF0000 is based on sdk\OpenW600\demo\wm_flash_demo.c
//2528 was picked based on current sizeof(mainConfig_t) which is 2016 with 512 buffer bytes.
//20260106 - try for V4 of config - (3584 + 512 instead of 2016 + 512) so use 3584+512 = 4096

#if defined(PLATFORM_W600) 
#define FLASH_VARS_STRUCTURE_ADDR_V3 (0xF0000 + 2528)
#define FLASH_VARS_STRUCTURE_ADDR (0xF0000 + 4096)
#else
#include "easyflash.h"
#endif

#define SAVE_CHANGE_IF_REQUIRED_AND_COUNT(target, source, counter) \
	if((target) != (source)) { \
		(target) = (source); \
		counter++; \
	}

void initFlashIfNeeded();

#if PLATFORM_W600

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
/*
To check for an update from config V3 we'll need the first bytes from config to check the version ...

typedef struct mainConfig_s {
	byte ident0;
	byte ident1;
	byte ident2;
	byte crc;
	// 0x4
	int version;
	// 0x08
	uint32_t genericFlags;
	....
	}
	
we will use some magic to check ident as int:

Read two "int", the first memory bytes will be
	"434647ZZ"	(ZZ is the crc, the rest "CFG")
	since we are on little endian, this represents
	0xZZ474643
	so first int & 0x00FFFFFF = 0x00474643 
	as an 32 bit int this is 4671043 
	
	
	

*/
	struct {
		int dummy;
		// 0x4
		int version;
	}tempstruct;
	tls_fls_read(0xF0000, &tempstruct, sizeof(tempstruct));
	if ( ((tempstruct.dummy & 0x00FFFFFF) == 0x00474643) && tempstruct.version < 4) {
		// we have an old version, so flashvars start at FLASH_VARS_STRUCTURE_ADDR_V3 ...
		// ... so let's read from old location.
		// After increasing boot_count, it will be written to the new location!
		tls_fls_read(FLASH_VARS_STRUCTURE_ADDR_V3, &flash_vars, FLASH_VARS_STRUCTURE_SIZE);
	} else {
		tls_fls_read(FLASH_VARS_STRUCTURE_ADDR, &flash_vars, FLASH_VARS_STRUCTURE_SIZE);
	}
	flash_vars.boot_count++;
	ADDLOG_INFO(LOG_FEATURE_CFG, "####### Boot Count %d #######", flash_vars.boot_count);
	write_flash_boot_content();
}

#else

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
	ADDLOG_INFO(LOG_FEATURE_CFG, "####### Boot Count %d #######", flash_vars.boot_count);
	write_flash_boot_content();
}

#endif

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
#ifndef DISABLE_FLASH_VARS_VARS
	int iChangesCount = 0;
#if PLATFORM_W600
	tls_fls_read(FLASH_VARS_STRUCTURE_ADDR, &flash_vars, FLASH_VARS_STRUCTURE_SIZE);
#else
	ef_get_env_blob(KV_KEY_FLASH_VARS, &flash_vars, FLASH_VARS_STRUCTURE_SIZE, NULL);
#endif

	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 1], brightness, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 2], temperature, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 3], mode, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 4], bEnableAll, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.rgb[0], r, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.rgb[1], g, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.rgb[2], b, iChangesCount);

	if (iChangesCount > 0) {
		ADDLOG_INFO(LOG_FEATURE_CFG, "####### Flash Save LED #######");
#if PLATFORM_W600
		tls_fls_write(FLASH_VARS_STRUCTURE_ADDR, &flash_vars, FLASH_VARS_STRUCTURE_SIZE);
#else
		ef_set_env_blob(KV_KEY_FLASH_VARS, &flash_vars, FLASH_VARS_STRUCTURE_SIZE);
#endif
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
