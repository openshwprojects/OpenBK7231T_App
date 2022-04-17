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



#endif // PLATFORM_XR809


