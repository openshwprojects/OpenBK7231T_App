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
#if ENABLE_BL_TWIN
	float TotalConsumption_b;
	float TodayConsumpion_b;
#else
	float ConsumptionHistory[2];
#endif
#ifdef PLATFORM_BEKEN_NEW
	unsigned int ConsumptionResetTime;
#else
	time_t ConsumptionResetTime;
#endif
	unsigned char reseved[3];
	char actual_mday;
} ENERGY_METERING_DATA;

// size 8 bytes
typedef struct {
	float Import;
	float Export;
} ENERGY_DATA;

typedef enum {
	ENERGY_CHANNEL_A = 0,
	ENERGY_CHANNEL_B = 1,
} ENERGY_CHANNEL;

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

#ifdef ENABLE_DRIVER_HLW8112SPI
void HAL_FlashVars_SaveEnergy(ENERGY_DATA** data, int channel_count);
void HAL_FlashVars_GetEnergy(ENERGY_DATA* data, ENERGY_CHANNEL channel);
#endif

#endif /* __HALK_FLASH_VARS_H__ */

