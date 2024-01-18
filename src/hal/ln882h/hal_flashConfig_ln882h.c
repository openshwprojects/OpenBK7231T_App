#ifdef PLATFORM_LN882H

#include "../hal_flashConfig.h"
#include "hal/hal_flash.h"
#include "flash_partition_table.h"

#define CONFIG_OFFSET USER_SPACE_OFFSET + USER_SPACE_SIZE - 4096

int HAL_Configuration_ReadConfigMemory(void *target, int dataLen){

	uint8_t ret = hal_flash_read(CONFIG_OFFSET, dataLen, (uint8_t *)target);

    return ret;
}





int HAL_Configuration_SaveConfigMemory(void *src, int dataLen){
	//hal_flash_write_enable();
	hal_flash_erase(CONFIG_OFFSET, 4096);
	hal_flash_program(CONFIG_OFFSET, dataLen, (uint8_t *)src);
	//hal_flash_write_disable();
	return 0;
}

#endif // PLATFORM_LN882H


