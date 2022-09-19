#ifdef PLATFORM_W800
#include "../hal_flashVars.h"

// call at startup
void HAL_FlashVars_IncreaseBootCount(){
}
void HAL_FlashVars_SaveChannel(int index, int value) {
}

// call once started (>30s?)
void HAL_FlashVars_SaveBootComplete(){
}

// call to return the number of boots since a HAL_FlashVars_SaveBootComplete
int HAL_FlashVars_GetBootFailures(){
    int diff = 0;

    return diff;
}

int HAL_FlashVars_GetBootCount(){
	return 0;
}
int HAL_FlashVars_GetChannelValue(int ch){

	return 0;
}
void HAL_FlashVars_SaveLED(byte mode, short brightness, short temperature, byte r, byte g, byte b) {

}
void HAL_FlashVars_ReadLED(byte *mode, short *brightness, short *temperature, byte *rgb) {

}


#endif
