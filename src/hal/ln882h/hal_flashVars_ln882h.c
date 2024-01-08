
#ifndef PLATFORM_XR809


#include "../../new_common.h"
#include "../hal_flashVars.h"


//#define DISABLE_FLASH_VARS_VARS


// call at startup
void HAL_FlashVars_IncreaseBootCount() {

}
void HAL_FlashVars_SaveChannel(int index, int value) {

}
void HAL_FlashVars_ReadLED(byte* mode, short* brightness, short* temperature, byte* rgb, byte* bEnableAll) {

}


void HAL_FlashVars_SaveLED(byte mode, short brightness, short temperature, byte r, byte g, byte b, byte bEnableAll) {

}

short HAL_FlashVars_ReadUsage() {
	return 0;
}
void HAL_FlashVars_SaveTotalUsage(short usage) {

}
// call once started (>30s?)
void HAL_FlashVars_SaveBootComplete() {

}

// call to return the number of boots since a HAL_FlashVars_SaveBootComplete
int HAL_FlashVars_GetBootFailures() {
	int diff = 0;

	return diff;
}

int HAL_FlashVars_GetBootCount() {
	return 0;
}
int HAL_FlashVars_GetChannelValue(int ch) {

	return 0;
}

int HAL_GetEnergyMeterStatus(ENERGY_METERING_DATA* data)
{

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

