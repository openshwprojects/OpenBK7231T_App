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
	float ConsumptionHistory[2];
	time_t ConsumptionResetTime;
	unsigned char reseved[3];
	char actual_mday;
} ENERGY_METERING_DATA;

typedef struct flash_vars_structure
{
	// offset  0
	unsigned short boot_count; // number of times the device has booted
	unsigned short boot_success_count; // if a device boots completely (>30s), will equal boot_success_count
	// offset  4
	short savedValues[MAX_RETAIN_CHANNELS];
	// offset 28
	ENERGY_METERING_DATA emetering;
	// offset 60
	unsigned char rgb[3];
	unsigned char len; // length of the whole structure (i.e. 2+2+1 = 5)  MUST NOT BE 255
	// size   64
} FLASH_VARS_STRUCTURE;

#define MAGIC_FLASHVARS_SIZE 64

// call at startup
void HAL_FlashVars_IncreaseBootCount();
// call once started (>30s?)
void HAL_FlashVars_SaveBootComplete();
// call to return the number of boots since a HAL_FlashVars_SaveBootComplete
int HAL_FlashVars_GetBootFailures();
int HAL_FlashVars_GetBootCount();
void HAL_FlashVars_SaveChannel(int index, int value);
void HAL_FlashVars_SaveLED(byte mode, short brightness, short temperature, byte r, byte g, byte b, byte bEnableAll);
void HAL_FlashVars_ReadLED(byte* mode, short* brightness, short* temperature, byte* rgb, byte* bEnableAll);
int HAL_FlashVars_GetChannelValue(int ch);
int HAL_GetEnergyMeterStatus(ENERGY_METERING_DATA* data);
int HAL_SetEnergyMeterStatus(ENERGY_METERING_DATA* data);
void HAL_FlashVars_SaveTotalConsumption(float total_consumption);
void HAL_FlashVars_SaveEnergyExport(float f);
float HAL_FlashVars_GetEnergyExport();

#endif /* __HALK_FLASH_VARS_H__ */

