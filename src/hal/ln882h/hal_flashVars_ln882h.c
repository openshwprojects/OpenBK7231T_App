#ifdef PLATFORM_LN882H

#include "../../new_common.h"
#include "../hal_flashVars.h"
#include "ln_kv_api.h"


FLASH_VARS_STRUCTURE flash_vars;
static int flash_vars_init_flag = 0;

#define KV_KEY_FLASH_VARS "OBK_FLASH_VARS"

int flash_vars_store() {
	if (KV_ERR_NONE != ln_kv_set(KV_KEY_FLASH_VARS, &flash_vars, sizeof(FLASH_VARS_STRUCTURE))) {
		LOG(LOG_LVL_ERROR, "ln_kv_set() failed!\r\n");
		return 0;
	}
	return 1;
}

int flash_vars_init() {
	// if the key has been loaded, don't load again.
	if (flash_vars_init_flag) {
		return 1;
	}

	// first load, init
	memset(&flash_vars, 0, sizeof(FLASH_VARS_STRUCTURE));
	flash_vars.len = sizeof(flash_vars);

	// has the key ever been stored?
	int has_key = ln_kv_has_key(KV_KEY_FLASH_VARS);

	if ((has_key != LN_TRUE) && (has_key != LN_FALSE)) {
		LOG(LOG_LVL_ERROR, "ln_kv_has_key() failed! %d\r\n", has_key);
		return 0;
	}

	// never saved, so store and exit with initial values, mark load flag
	if (has_key == LN_FALSE) {
		if (flash_vars_store()) {
			flash_vars_init_flag = 1;
			return 1;
		}
		return 0;
	}

	size_t v_len = 0;

	// load previously stored values and mark load flag on exit
	if (KV_ERR_NONE != ln_kv_get(KV_KEY_FLASH_VARS, &flash_vars, sizeof(FLASH_VARS_STRUCTURE), &v_len)) {
		LOG(LOG_LVL_ERROR, "ln_kv_get() failed!\r\n");
		return 0;
	}
	flash_vars_init_flag = 1;
	return 1;
}


// call at startup
void HAL_FlashVars_IncreaseBootCount() {
#ifndef DISABLE_FLASH_VARS_VARS
	if (flash_vars_init()) {
		flash_vars.boot_count ++;
		flash_vars_store();
	}
	LOG(LOG_LVL_INFO, "************************************************* Boot count: %d\r\n", flash_vars.boot_count);
#endif
}

void HAL_FlashVars_SaveChannel(int index, int value) {
#ifndef DISABLE_FLASH_VARS_VARS
	if (flash_vars_init()) {
		if (index < 0 || index >= MAX_RETAIN_CHANNELS) {
				LOG(LOG_LVL_ERROR, "####### Flash Save Can't Save Channel %d as %d (not enough space in array) #######", index, value);
				return;
		}		
		flash_vars.savedValues[index] = value;
		flash_vars_store();
	}
#endif

}
void HAL_FlashVars_ReadLED(byte* mode, short* brightness, short* temperature, byte* rgb, byte* bEnableAll) {
#ifndef DISABLE_FLASH_VARS_VARS
	if (flash_vars_init()) {
		*bEnableAll = flash_vars.savedValues[MAX_RETAIN_CHANNELS - 4];
		*mode = flash_vars.savedValues[MAX_RETAIN_CHANNELS - 3];
		*temperature = flash_vars.savedValues[MAX_RETAIN_CHANNELS - 2];
		*brightness = flash_vars.savedValues[MAX_RETAIN_CHANNELS - 1];
		rgb[0] = flash_vars.rgb[0];
		rgb[1] = flash_vars.rgb[1];
		rgb[2] = flash_vars.rgb[2];
	}
#endif
}


#define SAVE_CHANGE_IF_REQUIRED_AND_COUNT(target, source, counter) \
	if((target) != (source)) { \
		(target) = (source); \
		counter++; \
	}

void HAL_FlashVars_SaveLED(byte mode, short brightness, short temperature, byte r, byte g, byte b, byte bEnableAll) {
#ifndef DISABLE_FLASH_VARS_VARS
	int iChangesCount = 0;


	if (flash_vars_init()) {
		SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 1], brightness, iChangesCount);
		SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 2], temperature, iChangesCount);
		SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 3], mode, iChangesCount);
		SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 4], bEnableAll, iChangesCount);
		SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.rgb[0], r, iChangesCount);
		SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.rgb[1], g, iChangesCount);
		SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.rgb[2], b, iChangesCount);
		if (iChangesCount > 0) {
			flash_vars_store();
		}
	}
#endif
}

short HAL_FlashVars_ReadUsage() {
#ifndef DISABLE_FLASH_VARS_VARS
	if (flash_vars_init()) {
		return  flash_vars.savedValues[MAX_RETAIN_CHANNELS - 1];

	}
#endif
	return 0;
}

void HAL_FlashVars_SaveTotalUsage(short usage) {
#ifndef DISABLE_FLASH_VARS_VARS
	if (flash_vars_init()) {
		flash_vars.savedValues[MAX_RETAIN_CHANNELS - 1] = usage;
		flash_vars_store();
	}
#endif
}

// call once started (>30s?)
void HAL_FlashVars_SaveBootComplete() {
#ifndef DISABLE_FLASH_VARS_VARS
	if (flash_vars_init()) {
		flash_vars.boot_success_count = flash_vars.boot_count;
		flash_vars_store();
	}
#endif
}

// call to return the number of boots since a HAL_FlashVars_SaveBootComplete
int HAL_FlashVars_GetBootFailures() {
	int diff = 0;
#ifndef DISABLE_FLASH_VARS_VARS
	if (flash_vars_init()) {
		diff = flash_vars.boot_count - flash_vars.boot_success_count;
	}
#endif
	return diff;
}

int HAL_FlashVars_GetBootCount() {
#ifndef DISABLE_FLASH_VARS_VARS
	if (flash_vars_init()) {
		return flash_vars.boot_count;
	}
#endif
}

int HAL_FlashVars_GetChannelValue(int ch) {
#ifndef DISABLE_FLASH_VARS_VARS
	if (flash_vars_init()) {
		if (ch < 0 || ch >= MAX_RETAIN_CHANNELS) {
			LOG(LOG_LVL_ERROR, "####### Flash Save Can't Get Channel %d (not enough space in array) #######", ch);
			return 0;
		}
		return flash_vars.savedValues[ch];
	}
#endif
	return 0;
}

int HAL_GetEnergyMeterStatus(ENERGY_METERING_DATA* data)
{
#ifndef DISABLE_FLASH_VARS_VARS
	if (flash_vars_init()) {
		if (data != NULL) {
			memcpy(data, &flash_vars.emetering, sizeof(ENERGY_METERING_DATA));
		}
	}
#endif    
	return 0;
}

int HAL_SetEnergyMeterStatus(ENERGY_METERING_DATA* data)
{
#ifndef DISABLE_FLASH_VARS_VARS
	if (flash_vars_init()) {
		if (data != NULL) {
			memcpy(&flash_vars.emetering, data, sizeof(ENERGY_METERING_DATA));
			flash_vars_store();
		}
	}
#endif
	return 0;
}

void HAL_FlashVars_SaveTotalConsumption(float total_consumption)
{
#ifndef DISABLE_FLASH_VARS_VARS
	if (flash_vars_init()) {
		flash_vars.emetering.TotalConsumption = total_consumption;
		flash_vars_store();
	}
#endif
}

#endif // PLATFORM_LN882H

