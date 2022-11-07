#ifdef PLATFORM_BL602

#include "../hal_flashConfig.h"
#include "../hal_flashVars.h"
#include "../../logging/logging.h"

#include <easyflash.h>


void BL602_InitEasyFlashIfNeeded();

#define EASYFLASH_MY_BOOTCOUNTS     "myBtCnts"
#define BL602_SAVED_CHANNELS_MAX 8

typedef struct bl602_bootCounts_s {
    unsigned short boot_count; // number of times the device has booted
    unsigned short boot_success_count; // if a device boots completely (>30s), will equal boot_success_count
    short channelStates[BL602_SAVED_CHANNELS_MAX];
    ENERGY_METERING_DATA emetering;
} bl602_bootCounts_t;

static bl602_bootCounts_t g_bootCounts;
static int g_loaded = 0;

static int BL602_ReadFlashVars(void *target, int dataLen){
	int readLen;

	BL602_InitEasyFlashIfNeeded();

	g_loaded = 1;
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "BL602_ReadFlashVars: will read %d bytes", dataLen);
	readLen = ef_get_env_blob(EASYFLASH_MY_BOOTCOUNTS, target, dataLen , NULL);
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "BL602_ReadFlashVars: really loaded %d bytes", readLen);

    return dataLen;
}



static int BL602_SaveFlashVars(void *src, int dataLen){

	EfErrCode  res;

	BL602_InitEasyFlashIfNeeded();

	res = ef_set_env_blob(EASYFLASH_MY_BOOTCOUNTS, src, dataLen);
	if(res == EF_ENV_INIT_FAILED) {
		ADDLOG_DEBUG(LOG_FEATURE_CFG, "BL602_SaveFlashVars: EF_ENV_INIT_FAILED for %d bytes", dataLen);
		return 0;
	}
	if(res == EF_ENV_ARG_ERR) {
		ADDLOG_DEBUG(LOG_FEATURE_CFG, "BL602_SaveFlashVars: EF_ENV_ARG_ERR for %d bytes", dataLen);
		return 0;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CFG, "BL602_SaveFlashVars: saved %d bytes", dataLen);
    return dataLen;
}


void HAL_FlashVars_SaveBootComplete(){
	g_bootCounts.boot_success_count = g_bootCounts.boot_count;
	// save after set
	BL602_SaveFlashVars(&g_bootCounts,sizeof(g_bootCounts));
}

int HAL_FlashVars_GetBootCount(){
	return g_bootCounts.boot_count;
}
int HAL_FlashVars_GetBootFailures(){
    int diff = 0;
    diff = g_bootCounts.boot_count - g_bootCounts.boot_success_count;
    return diff;
}
void HAL_FlashVars_IncreaseBootCount(){
	// defaults - in case read fails
	memset(&g_bootCounts,0,sizeof(g_bootCounts));
	// read saved
	BL602_ReadFlashVars(&g_bootCounts,sizeof(g_bootCounts));
	g_bootCounts.boot_count++;
	// save after increase
	BL602_SaveFlashVars(&g_bootCounts,sizeof(g_bootCounts));
}


void HAL_FlashVars_SaveChannel(int index, int value) {
	if(index<0||index>=BL602_SAVED_CHANNELS_MAX)
		return;
	if(g_loaded==0) {
		BL602_ReadFlashVars(&g_bootCounts,sizeof(g_bootCounts));
	}
	g_bootCounts.channelStates[index] = value;
	// save after increase
	BL602_SaveFlashVars(&g_bootCounts,sizeof(g_bootCounts));
}
void HAL_FlashVars_SaveLED(byte mode, short brightness, short temperature, byte r, byte g, byte b, byte bEnableAll) {

}
void HAL_FlashVars_ReadLED(byte *mode, short *brightness, short *temperature, byte *rgb, byte *bEnableAll) {

}

int HAL_FlashVars_GetChannelValue(int ch) {
	if(ch<0||ch>=BL602_SAVED_CHANNELS_MAX)
		return 0;
	if(g_loaded==0) {
		BL602_ReadFlashVars(&g_bootCounts,sizeof(g_bootCounts));
	}
	return g_bootCounts.channelStates[ch];
}

int HAL_GetEnergyMeterStatus(ENERGY_METERING_DATA *data)
{
    if (data != NULL)
    {
        if (g_loaded==0) 
        {
            BL602_ReadFlashVars(&g_bootCounts,sizeof(g_bootCounts));
        }
        memcpy(data, &g_bootCounts.emetering, sizeof(ENERGY_METERING_DATA));
    }
    return 0;
}

int HAL_SetEnergyMeterStatus(ENERGY_METERING_DATA *data)
{
    if (data != NULL)
    {
        memcpy(&g_bootCounts.emetering, data, sizeof(ENERGY_METERING_DATA));
        BL602_SaveFlashVars(&g_bootCounts,sizeof(g_bootCounts));
    }
    return 0;
}

void HAL_FlashVars_SaveTotalConsumption(float total_consumption)
{
    g_bootCounts.emetering.TotalConsumption = total_consumption;
}

#endif // PLATFORM_BL602

