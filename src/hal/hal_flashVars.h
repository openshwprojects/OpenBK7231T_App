#ifndef __HALK_FLASH_VARS_H__
#define __HALK_FLASH_VARS_H__

//#define DISABLE_FLASH_VARS_VARS
#include "../new_common.h"

#define BOOT_COMPLETE_SECONDS 30
#define MAX_RETAIN_CHANNELS 12

/* Fixed size 32 bytes */
typedef struct ENERGY_METERING_DATA {
    float TotalConsumption;
    float TodayConsumpion;
    float YesterdayConsumption;
    long save_counter;
    unsigned char reseved[15];
    unsigned char actual_mday;
} ENERGY_METERING_DATA;

// call at startup
void HAL_FlashVars_IncreaseBootCount();
// call once started (>30s?)
void HAL_FlashVars_SaveBootComplete();
// call to return the number of boots since a HAL_FlashVars_SaveBootComplete
int HAL_FlashVars_GetBootFailures();
int HAL_FlashVars_GetBootCount();
void HAL_FlashVars_SaveChannel(int index, int value);
void HAL_FlashVars_SaveLED(byte mode, short brightness, short temperature, byte r, byte g, byte b);
void HAL_FlashVars_ReadLED(byte *mode, short *brightness, short *temperature, byte *rgb);
int HAL_FlashVars_GetChannelValue(int ch);
int HAL_GetEnergyMeterStatus(ENERGY_METERING_DATA *data);
int HAL_SetEnergyMeterStatus(ENERGY_METERING_DATA *data);
void HAL_FlashVars_SaveTotalConsumption(float total_consumption);

#endif /* __HALK_FLASH_VARS_H__ */

