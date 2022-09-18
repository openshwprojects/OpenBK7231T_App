#ifdef PLATFORM_XR809

#include "../hal_flashConfig.h"
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
void HAL_FlashVars_SaveLED(byte mode, short brightness, short temperature, byte r, byte g, byte b) {

}
void HAL_FlashVars_ReadLED(byte *mode, short *brightness, short *temperature, byte *rgb) {

}




#endif // PLATFORM_XR809


