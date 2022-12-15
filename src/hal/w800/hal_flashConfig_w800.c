#if defined(PLATFORM_W800) || defined(PLATFORM_W600)

#include "../hal_flashConfig.h"
#include "wm_include.h"
#include "wm_internal_flash.h"


static int bFlashReady = 0;

void initFlashIfNeeded() {
	if (bFlashReady)
		return;

	tls_fls_init();									//initialize flash driver

	bFlashReady = 1;

}

#if defined(PLATFORM_W800) 

//Based on sdk\OpenW600\demo\wm_flash_demo.c
#define FLASH_CONFIG_ADDR      0x1F0303

#else

//Based on sdk\OpenW600\example\flash\user\main.c
#define FLASH_CONFIG_ADDR      0xF0000

#endif


int HAL_Configuration_ReadConfigMemory(void* target, int dataLen) {
	initFlashIfNeeded();
	tls_fls_read(FLASH_CONFIG_ADDR, target, dataLen);
	return dataLen;
}

int HAL_Configuration_SaveConfigMemory(void* src, int dataLen) {
	initFlashIfNeeded();
	tls_fls_write(FLASH_CONFIG_ADDR, src, dataLen);
	return dataLen;
}

#endif

