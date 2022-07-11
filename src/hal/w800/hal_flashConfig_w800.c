#include "../hal_flashConfig.h"


#include "../../logging/logging.h"

static SemaphoreHandle_t config_mutex = 0;


int HAL_Configuration_ReadConfigMemory(void *target, int dataLen){

    return dataLen;
}




int bekken_hal_flash_read(const unsigned int addr, void *dst, const unsigned int size)
{


    return 0;
}




int HAL_Configuration_SaveConfigMemory(void *src, int dataLen){

    return dataLen;
}



