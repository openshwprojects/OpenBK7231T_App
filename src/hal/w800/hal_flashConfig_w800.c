#ifdef PLATFORM_W800

#include "../hal_flashConfig.h"
#include "../../logging/logging.h"
#include "wm_include.h"
#include "wm_internal_flash.h"


static int bFlashReady = 0;

void initFlashIfNeeded(){
	if(bFlashReady)
		return;

    tls_fls_init();									//initialize flash driver

	bFlashReady = 1;

}
#define TEST_FLASH_BUF_SIZE    4000
#define FLASH_CONFIG_ADDR 		0x1F0303


int HAL_Configuration_ReadConfigMemory(void *target, int dataLen){

	initFlashIfNeeded();

	tls_fls_read(FLASH_CONFIG_ADDR, target, dataLen);
    return dataLen;
}




int HAL_Configuration_SaveConfigMemory(void *src, int dataLen){
	initFlashIfNeeded();

    tls_fls_write(FLASH_CONFIG_ADDR, src, dataLen);
    return dataLen;
}



#endif

