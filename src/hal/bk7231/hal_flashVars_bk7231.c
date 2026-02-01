#if PLATFORM_BEKEN

#include "../../new_common.h"
#include "../hal_flashVars.h"
#include "../../logging/logging.h"
#include <easyflash.h>

static int g_easyFlash_Ready = 0;
void InitEasyFlashIfNeeded()
{
	if(g_easyFlash_Ready == 0)
	{
		easyflash_init();
		g_easyFlash_Ready = 1;
	}
}

FLASH_VARS_STRUCTURE flash_vars;
static int g_loaded = 0;

#define KV_KEY_FLASH_VARS "OBK_FV"
#define SAVE_CHANGE_IF_REQUIRED_AND_COUNT(target, source, counter) \
	if((target) != (source)) { \
		(target) = (source); \
		counter++; \
	}

extern void InitEasyFlashIfNeeded();

static int ReadFlashVars(void* target, int dataLen)
{
	InitEasyFlashIfNeeded();
	int readLen;
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "%s: to read %d b", __func__, dataLen);
	readLen = ef_get_env_blob(KV_KEY_FLASH_VARS, target, dataLen, NULL);
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "%s: read %d b", __func__, readLen);
	g_loaded = 1;
	return dataLen;
}

static int SaveFlashVars(void* src, int dataLen)
{
	InitEasyFlashIfNeeded();
	EfErrCode res;

	res = ef_set_env_blob(KV_KEY_FLASH_VARS, src, dataLen);
	if(res == EF_ENV_INIT_FAILED)
	{
		ADDLOG_DEBUG(LOG_FEATURE_CFG, "%s: init failed for %d b", __func__, dataLen);
		return 0;
	}
	if(res == EF_ENV_NAME_ERR)
	{
		ADDLOG_DEBUG(LOG_FEATURE_CFG, "%s: name err for %d b", __func__, dataLen);
		return 0;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "%s: saved %d b", __func__, dataLen);
	return dataLen;
}

// call at startup
void HAL_FlashVars_IncreaseBootCount()
{
	memset(&flash_vars, 0, sizeof(flash_vars));
	if(g_loaded == 0)
	{
		ReadFlashVars(&flash_vars, sizeof(flash_vars));
	}
	flash_vars.boot_count++;
	SaveFlashVars(&flash_vars, sizeof(flash_vars));
}

void HAL_FlashVars_SaveChannel(int index, int value)
{
	if(index < 0 || index >= MAX_RETAIN_CHANNELS)
		return;
	if(g_loaded == 0)
	{
		ReadFlashVars(&flash_vars, sizeof(flash_vars));
	}
	flash_vars.savedValues[index] = value;
	// save after increase
	SaveFlashVars(&flash_vars, sizeof(flash_vars));
}

void HAL_FlashVars_ReadLED(byte* mode, short* brightness, short* temperature, byte* rgb, byte* bEnableAll)
{
	if(g_loaded == 0)
	{
		ReadFlashVars(&flash_vars, sizeof(flash_vars));
	}
	*bEnableAll = flash_vars.savedValues[MAX_RETAIN_CHANNELS - 4];
	*mode = flash_vars.savedValues[MAX_RETAIN_CHANNELS - 3];
	*temperature = flash_vars.savedValues[MAX_RETAIN_CHANNELS - 2];
	*brightness = flash_vars.savedValues[MAX_RETAIN_CHANNELS - 1];
	rgb[0] = flash_vars.rgb[0];
	rgb[1] = flash_vars.rgb[1];
	rgb[2] = flash_vars.rgb[2];
}

void HAL_FlashVars_SaveLED(byte mode, short brightness, short temperature, byte r, byte g, byte b, byte bEnableAll)
{
	int iChangesCount = 0;

	if(g_loaded == 0)
	{
		ReadFlashVars(&flash_vars, sizeof(flash_vars));
	}

	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 1], brightness, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 2], temperature, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 3], mode, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.savedValues[MAX_RETAIN_CHANNELS - 4], bEnableAll, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.rgb[0], r, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.rgb[1], g, iChangesCount);
	SAVE_CHANGE_IF_REQUIRED_AND_COUNT(flash_vars.rgb[2], b, iChangesCount);

	if(iChangesCount > 0)
	{
		SaveFlashVars(&flash_vars, sizeof(flash_vars));
	}
}

short HAL_FlashVars_ReadUsage()
{
	return flash_vars.savedValues[MAX_RETAIN_CHANNELS - 1];
}

void HAL_FlashVars_SaveTotalUsage(short usage)
{
	flash_vars.savedValues[MAX_RETAIN_CHANNELS - 1] = usage;
}

// call once started (>30s?)
void HAL_FlashVars_SaveBootComplete()
{
	flash_vars.boot_success_count = flash_vars.boot_count;
	SaveFlashVars(&flash_vars, sizeof(flash_vars));
}

// call to return the number of boots since a HAL_FlashVars_SaveBootComplete
int HAL_FlashVars_GetBootFailures()
{
	int diff = 0;
	diff = flash_vars.boot_count - flash_vars.boot_success_count;
	return diff;
}

int HAL_FlashVars_GetBootCount()
{
	return flash_vars.boot_count;
}

int HAL_FlashVars_GetChannelValue(int ch)
{
	if(ch < 0 || ch >= MAX_RETAIN_CHANNELS)
		return 0;
	if(g_loaded == 0)
	{
		ReadFlashVars(&flash_vars, sizeof(flash_vars));
	}
	return flash_vars.savedValues[ch];
}

int HAL_GetEnergyMeterStatus(ENERGY_METERING_DATA* data)
{
	if(data != NULL)
	{
		if(g_loaded == 0)
		{
			ReadFlashVars(&flash_vars, sizeof(flash_vars));
		}
		memcpy(data, &flash_vars.emetering, sizeof(ENERGY_METERING_DATA));
	}
	return 0;
}

int HAL_SetEnergyMeterStatus(ENERGY_METERING_DATA* data)
{
	if(data != NULL)
	{
		memcpy(&flash_vars.emetering, data, sizeof(ENERGY_METERING_DATA));
		SaveFlashVars(&flash_vars, sizeof(flash_vars));
	}
	return 0;
}

void HAL_FlashVars_SaveTotalConsumption(float total_consumption)
{
	flash_vars.emetering.TotalConsumption = total_consumption;
}

//#ifdef ENABLE_DRIVER_HLW8112SPI
void HAL_FlashVars_SaveEnergy(ENERGY_DATA** data, int channel_count)
{
	if(data != NULL)
	{
		for(int i = 0; i < channel_count; i++)
		{
			char buffer[20];
			sprintf(buffer, "HLW_CH%i", i);
			ef_set_env_blob(buffer, data[i], sizeof(ENERGY_DATA));
		}
	}
}
void HAL_FlashVars_GetEnergy(ENERGY_DATA* data, ENERGY_CHANNEL channel)
{
	if(data != NULL)
	{
		char buffer[20];
		sprintf(buffer, "HLW_CH%i", channel);
		ef_get_env_blob(buffer, data, sizeof(ENERGY_DATA), NULL);
	}
}
//#endif

#endif
