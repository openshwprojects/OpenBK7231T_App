#ifdef PLATFORM_XR809

#include "../hal_flashConfig.h"
#include "../hal_flashVars.h"
#include "../../logging/logging.h"

void HAL_FlashVars_SaveBootComplete(){
}

int HAL_FlashVars_GetBootCount(){
	return 0;
}
int HAL_FlashVars_GetBootFailures(){
    int diff = 0;
    return diff;
}
void HAL_FlashVars_IncreaseBootCount(){
}
void HAL_FlashVars_SaveChannel(int index, int value) {

}
int HAL_FlashVars_GetChannelValue(int ch) {
	return 0;
}
void HAL_FlashVars_SaveLED(byte mode, short brightness, short temperature, byte r, byte g, byte b, byte bEnableAll) {

}
void HAL_FlashVars_ReadLED(byte *mode, short *brightness, short *temperature, byte *rgb, byte *bEnableAll) {

}

int HAL_GetEnergyMeterStatus(ENERGY_METERING_DATA *data)
{
    /* default values */
    if (data != NULL)
    {
        memset(data, 0, sizeof(ENERGY_METERING_DATA));
        data->actual_mday = -1;
    }
    return 0;
}

int HAL_SetEnergyMeterStatus(ENERGY_METERING_DATA *data)
{
    return 0;
}

void HAL_FlashVars_SaveTotalConsumption(float total_consumption)
{
}

#endif // PLATFORM_XR809


