#include "../../new_common.h"
#include "../hal_flashVars.h"
#include "../../logging/logging.h"

// call at startup
void __attribute__((weak)) HAL_FlashVars_IncreaseBootCount()
{

}

void __attribute__((weak)) HAL_FlashVars_SaveChannel(int index, int value)
{

}

void __attribute__((weak)) HAL_FlashVars_ReadLED(byte* mode, short* brightness, short* temperature, byte* rgb, byte* bEnableAll)
{

}

void __attribute__((weak)) HAL_FlashVars_SaveLED(byte mode, short brightness, short temperature, byte r, byte g, byte b, byte bEnableAll)
{

}

short __attribute__((weak)) HAL_FlashVars_ReadUsage()
{
	return 0;
}

void __attribute__((weak)) HAL_FlashVars_SaveTotalUsage(short usage)
{

}

// call once started (>30s?)
void __attribute__((weak)) HAL_FlashVars_SaveBootComplete()
{

}

// call to return the number of boots since a HAL_FlashVars_SaveBootComplete
int __attribute__((weak)) HAL_FlashVars_GetBootFailures()
{
	int diff = 0;
	return diff;
}

int __attribute__((weak)) HAL_FlashVars_GetBootCount()
{
	return 0;
}

int __attribute__((weak)) HAL_FlashVars_GetChannelValue(int ch)
{
	return 0;
}

int __attribute__((weak)) HAL_GetEnergyMeterStatus(ENERGY_METERING_DATA* data)
{
	return 0;
}

int __attribute__((weak)) HAL_SetEnergyMeterStatus(ENERGY_METERING_DATA* data)
{
	return 0;
}

void __attribute__((weak)) HAL_FlashVars_SaveTotalConsumption(float total_consumption)
{

}
